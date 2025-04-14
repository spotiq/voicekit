#include "ElevenlabsTTS.h"

bool ElevenlabsTTS::Initialise(const std::string& apiKey, const std::string& region) {
    m_apiKey = apiKey;
    auto voiceInfo = getVoiceIdAndModel(m_voiceName);
    if (!voiceInfo.has_value()) {
        std::cerr << "Could not fetch voice ID and model.\n";
        return 1;
    }
    m_voiceId = voiceInfo->first;
    m_modelId = voiceInfo->second;
    startWebSocket();
    return true;
}

// Fetch voice_id and best model_id for given voice name
std::optional<std::pair<std::string, std::string>> ElevenlabsTTS::getVoiceIdAndModel(const std::string& voiceName) {
    cpr::Response response = cpr::Get(
        cpr::Url{"https://api.elevenlabs.io/v2/voices"},
        cpr::Header{{"xi-api-key", m_apiKey}},
        cpr::Parameters{{"include_total_count", "true"}, {"search", voiceName}}
    );

    if (response.status_code != 200) {
        std::cerr << "Voice search failed. Status: " << response.status_code << "\n";
        return std::nullopt;
    }

    json resJson = json::parse(response.text);
    if (resJson["voices"].empty()) {
        std::cerr << "Voice not found.\n";
        return std::nullopt;
    }

    std::string voiceId = resJson["voices"][0]["voice_id"];
    std::string defaultModel = "eleven_multilingual_v2"; // Fallback model

    if (resJson["voices"][0].contains("high_quality_base_model_ids") &&
        !resJson["voices"][0]["high_quality_base_model_ids"].empty()) {
        defaultModel = resJson["voices"][0]["high_quality_base_model_ids"][0];
    }

    return std::make_pair(voiceId, defaultModel);
}


std::string ElevenlabsTTS::buildWebSocketURL() const {
    // wss://api.elevenlabs.io/v1/text-to-speech/cgSgspJ2msm6clMCkdW9/stream-input?output_format=pcm_16000
    return "wss://api.elevenlabs.io/v1/text-to-speech/" + m_voiceId + "/stream-input?output_format=pcm_16000";
}

void ElevenlabsTTS::startWebSocket() {
    webSocket.setUrl(buildWebSocketURL());

    ix::WebSocketHttpHeaders headers;
    headers["xi-api-key"] = m_apiKey;
    headers["Accept"] = "application/json";

    webSocket.setExtraHeaders(headers);

    webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            handleMessage(msg->str);
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            SPDLOG_INFO("ElevenLabs WebSocket connection opened.");
            m_isConnected = true;
            wsCv.notify_all();
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            SPDLOG_ERROR("WebSocket Error: {}", msg->errorInfo.reason);
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            SPDLOG_INFO("ElevenLabs WebSocket connection closed.");
            m_isConnected = false;
        }
    });

    webSocket.start();
}

void ElevenlabsTTS::sendInitialSettings() {
    nlohmann::json initPayload = {
        {"text", " "},
        {"voice_settings", {
            {"stability", 0.5},
            {"similarity_boost", 0.8},
            {"speed", 1.0}
        }}
    };

    std::lock_guard<std::mutex> lock(sendMutex);
    webSocket.send(initPayload.dump());
}

void ElevenlabsTTS::ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) {
    SPDLOG_INFO("[{}] Audio buffer text: {}", stream_sid, text);

    std::unique_lock<std::mutex> lock(ttsProcessingMutex);
    {
        std::lock_guard<std::mutex> audioLock(accumulatedAudioMutex);
        m_accumulatedAudioBuffer.clear();
    }

    m_isFinalReceived = false;
    m_currentHashKey = hashKey;
    m_startTime = std::chrono::high_resolution_clock::now();

    {
        std::unique_lock<std::mutex> wsLock(wsMutex);
        if (!m_isConnected) {
            wsCv.wait_for(wsLock, std::chrono::seconds(5), [this] { return m_isConnected.load(); });
        }
    }

    sendInitialSettings();

    // send text payload
    nlohmann::json speakPayload = {
        {"text", text},
        {"try_trigger_generation", true}
    };

    {
        std::lock_guard<std::mutex> sendLock(sendMutex);
        webSocket.send(speakPayload.dump());
    }

    // Send empty payload
    nlohmann::json emptyPayload = {
        {"text", ""}
    };

    {
        std::lock_guard<std::mutex> sendLock(sendMutex);
        webSocket.send(emptyPayload.dump());
    }

    {
        std::unique_lock<std::mutex> finalLock(m_finalMutex);
        m_finalCv.wait(finalLock, [this] { return m_isFinalReceived.load(); });
    }

    {
        std::lock_guard<std::mutex> audioLock(accumulatedAudioMutex);
        if (!m_accumulatedAudioBuffer.empty()) {
            SPDLOG_INFO("[{}] Audio buffer size: {}", hashKey, m_accumulatedAudioBuffer.size());
            auto endTime = std::chrono::high_resolution_clock::now();
            auto ttsLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_startTime).count();
            SynthesisedAudioData(m_accumulatedAudioBuffer, m_currentHashKey, ttsLatency);
        }
    }
}

void ElevenlabsTTS::handleMessage(const std::string& message) {
    try {
        auto jsonMsg = nlohmann::json::parse(message);
        SPDLOG_DEBUG("[{}] Text Response: {}", stream_sid, message);

        if (jsonMsg.contains("audio") && !jsonMsg["audio"].is_null()) {
            std::string base64Audio = jsonMsg["audio"];
            std::string decodedAudioStr = siprtc::base64_decode(base64Audio);
            std::vector<uint8_t> decodedAudio(decodedAudioStr.begin(), decodedAudioStr.end());
            
            std::lock_guard<std::mutex> audioLock(accumulatedAudioMutex);
            m_accumulatedAudioBuffer.insert(m_accumulatedAudioBuffer.end(), decodedAudio.begin(), decodedAudio.end());
        }

        if (jsonMsg.contains("isFinal") && !jsonMsg["isFinal"].is_null() && jsonMsg["isFinal"].get<bool>()) {
            SPDLOG_INFO("[{}] Received isFinal=true", stream_sid);
            m_isFinalReceived = true;
            m_finalCv.notify_all();
            webSocket.close();
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[{}] Failed to handle message: {}", stream_sid, e.what());
    }
}

void ElevenlabsTTS::CloseConnection() {
    if (webSocket.getReadyState() == ix::ReadyState::Open){
        SPDLOG_INFO("Closing ElevenLabs WebSocket connection.");
        webSocket.stop();
        m_isConnected = false;
    }
}
