#include "DeepgramTTS.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

bool DeepgramTTS::Initialise(const std::string& apiKey, const std::string& region) {
    m_apiKey = apiKey;
    StartWebSocket();
    return true;
}

std::string DeepgramTTS::buildWebSocketURL() const {
    std::string url = "wss://api.deepgram.com/v1/speak?";
    url += "model=" + m_voiceName;
    url += "&encoding=linear16&sample_rate=8000";
    return url;
}

void DeepgramTTS::StartWebSocket() {
    if (isConnected) return;

    webSocket.setUrl(buildWebSocketURL());
    ix::WebSocketHttpHeaders headers;
    headers["Authorization"] = "token " + m_apiKey;
    webSocket.setExtraHeaders(headers);

    webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            handleMessage(msg->str);
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            SPDLOG_INFO("[{}] Deepgram connection opened", stream_sid);
            {
                std::lock_guard<std::mutex> lock(wsMutex);
                isConnected.store(true);
            }
            wsCv.notify_all();  // Notify the waiting thread that the connection is established
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            SPDLOG_ERROR("[{}] WebSocket Error: {}", stream_sid, msg->errorInfo.reason);
            isConnected.store(false);
        }
    });

    webSocket.start();
}

void DeepgramTTS::handleMessage(const std::string& message) {
    if (message.rfind("{", 0) == 0) {
        SPDLOG_DEBUG("[{}] Text Response: {}", stream_sid, message);

        try {
            auto jsonMsg = nlohmann::json::parse(message);
            if (jsonMsg.contains("type")) {
                std::string type = jsonMsg["type"];

                if (type == "Flushed") {
                    SPDLOG_INFO("[{}] Received Flushed message", stream_sid);
                    {
                        std::lock_guard<std::mutex> lock(flushedMutex);
                        isFlushedReceived.store(true);
                    }
                    flushedCv.notify_all();
                }
            }
        } catch (const std::exception& e) {
            SPDLOG_WARN("[{}] Failed to parse JSON message: {}", stream_sid, e.what());
        }

        return;
    }

    // Handle binary audio data
    std::vector<uint8_t> audioChunk(message.begin(), message.end());
    SPDLOG_INFO("[{}] Received audio chunk of size {}", stream_sid, audioChunk.size());

    {
        std::lock_guard<std::mutex> lock(accumulatedAudioMutex);
        accumulatedAudioBuffer.insert(accumulatedAudioBuffer.end(), audioChunk.begin(), audioChunk.end());
    }
}

void DeepgramTTS::ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) {
    std::unique_lock<std::mutex> ttsLock(ttsProcessingMutex);
    SPDLOG_INFO("[{}] Starting synthesis for text: {}", stream_sid, text);

    {
        std::unique_lock<std::mutex> lock(wsMutex);
        if (!isConnected.load()) {
            if (!wsCv.wait_for(lock, std::chrono::seconds(5), [this] { return isConnected.load(); })) {
                SPDLOG_ERROR("[{}] TTS WebSocket connection timeout!", stream_sid);
                return;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(accumulatedAudioMutex);
        accumulatedAudioBuffer.clear();
    }

    isFlushedReceived.store(false);
    m_startTime = std::chrono::high_resolution_clock::now();
    m_currentHashKey = hashKey;

    {
        std::lock_guard<std::mutex> sendLock(sendMutex);
        nlohmann::json speakMsg = {{"type", "Speak"}, {"text", text}};
        webSocket.send(speakMsg.dump());

        nlohmann::json flushMsg = {{"type", "Flush"}};
        webSocket.send(flushMsg.dump());
    }

    // Wait for "Flushed" message
    {
        std::unique_lock<std::mutex> lock(flushedMutex);
        if (!flushedCv.wait_for(lock, std::chrono::seconds(30), [this] { return isFlushedReceived.load(); })) {
            SPDLOG_ERROR("[{}] TTS Flushed message timeout!", stream_sid);
            return;
        }
    }

    std::vector<uint8_t> finalAudio;
    {
        std::lock_guard<std::mutex> lock(accumulatedAudioMutex);
        finalAudio = accumulatedAudioBuffer;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto ttsLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_startTime).count();

    SPDLOG_INFO("[{}] Final audio collected with size: {}", stream_sid, finalAudio.size());
    SynthesisedAudioData(finalAudio, hashKey, ttsLatency);
}


void DeepgramTTS::CloseConnection() {
    if (isConnected) {
        SPDLOG_INFO("[{}] Closing WebSocket connection", stream_sid);
        nlohmann::json closeMsg = {{"type", "Close"}};
        webSocket.send(closeMsg.dump());
        webSocket.stop();
        isConnected = false;
    }
}
