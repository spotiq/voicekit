#include "AmazonSTT.h"
#include <spdlog/spdlog.h>
#include <sstream>

void AmazonSTT::InitialiseSTTModule(const std::string& apiKey, const std::string& region) {
    size_t sep = apiKey.find(':');
    if (sep == std::string::npos) {
        SPDLOG_ERROR("[{}] Invalid API key format. Use accessKey:secretKey", stream_sid);
        return;
    }
    m_accessKey = apiKey.substr(0, sep);
    m_secretKey = apiKey.substr(sep + 1);
    m_region = region.empty() ? "us-east-1" : region;
}

void AmazonSTT::ImplStartRecognition() {
    std::string url = TranscribeManager::getSignedWebsocketUrl(m_accessKey, m_secretKey, m_region, language);
    if (url.empty()) {
        SPDLOG_ERROR("[{}] Failed to generate signed URL for Amazon Transcribe", stream_sid);
        return;
    }

    m_webSocket.setUrl(url);
    m_webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::string payload;
            bool isError = false;
            if (TranscribeManager::parseResponse(msg->str, payload, isError, true)) {
                handleMessage(payload);
            } else {
                SPDLOG_WARN("[{}] Ignored unknown response", stream_sid);
            }
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            SPDLOG_INFO("[{}] Amazon Transcribe connection opened", stream_sid);
            std::lock_guard<std::mutex> lock(wsMutex);
            isConnected = true;
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            SPDLOG_ERROR("[{}] WebSocket Error: {}", stream_sid, msg->errorInfo.reason);
        }
    });

    m_webSocket.start();
}

void AmazonSTT::ImplStreamAudioData(std::vector<uint8_t> audioData) {
    if (isConnected) {
        std::string requestPayload;
        if (!TranscribeManager::makeRequest(requestPayload, audioData)) {
            SPDLOG_ERROR("[{}] Failed to frame audio data using TranscribeManager::makeRequest", stream_sid);
            return;
        }
        m_webSocket.sendBinary(requestPayload);
    }
}

void AmazonSTT::ImplStopRecognition() {
    std::lock_guard<std::mutex> lock(wsMutex);
    if (isConnected) {
        // Send final empty audio frame
        std::vector<uint8_t> emptyAudio;
        std::string finalRequest;
        if (!TranscribeManager::makeRequest(finalRequest, emptyAudio)) {
            SPDLOG_ERROR("[{}] Failed to frame final empty audio chunk via TranscribeManager::makeRequest", stream_sid);
        } else {
            m_webSocket.sendBinary(finalRequest);
            SPDLOG_INFO("[{}] Sent final empty audio chunk to Amazon Transcribe", stream_sid);
        }
        // Close WebSocket connection
        m_webSocket.stop();
        isConnected = false;
        SPDLOG_INFO("[{}] Amazon Transcribe WebSocket stopped", stream_sid);
    }
}

void AmazonSTT::ImplRecognize() {
    // Recognition is handled in handleMessage
}

void AmazonSTT::handleMessage(const std::string& message) {
    auto parsed = json::parse(message, nullptr, false);
    if (parsed.is_discarded()) return;

    if (parsed.contains("Transcript") && parsed["Transcript"].contains("Results")) {
        auto results = parsed["Transcript"]["Results"];
        for (const auto& result : results) {
            if (result.contains("Alternatives") && !result["Alternatives"].empty()) {
                std::string transcript = result["Alternatives"][0].value("Transcript", "");
                bool isFinal = result.value("IsPartial", true) == false;
                if (!transcript.empty()) {
                    if (isFinal) {
                        SPDLOG_INFO("[{}] ✅ FINAL: {}", stream_sid, transcript);
                        RecognisedText(transcript);  // Callback
                    } else {
                        SPDLOG_INFO("[{}] 🟡 INTERIM: {}", stream_sid, transcript);
                    }
                }
            }
        }
    }
}
