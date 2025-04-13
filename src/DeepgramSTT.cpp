#include "DeepgramSTT.h"
#include <spdlog/spdlog.h>
#include <sstream>

void DeepgramSTT::InitialiseSTTModule(const std::string& apiKey, const std::string& region) {
    this->apiKey = apiKey;
}

void DeepgramSTT::ImplStartRecognition() {
    std::string model = "nova-3";
    std::string encoding = "linear16";
    int sample_rate = 8000;
    int channels = 1;
    int endpointing_ms = 500;
    int utterance_end_ms = 2000;
    bool smart_format = true;

    std::ostringstream urlStream;
    urlStream << "wss://api.deepgram.com/v1/listen"
              << "?model=" << model
              << "&language=" << language
              << "&punctuate=true"
              << "&interim_results=true"
              << "&vad_events=true"
              << "&smart_format=" << (smart_format ? "true" : "false")
              << "&utterance_end_ms=" << utterance_end_ms
              << "&endpointing=" << endpointing_ms
              << "&encoding=" << encoding
              << "&sample_rate=" << sample_rate
              << "&channels=" << channels;
    
    std::string url = urlStream.str();

    webSocket.setUrl(url);
    ix::WebSocketHttpHeaders headers;
    headers["Authorization"] = "token " + apiKey;
    webSocket.setExtraHeaders(headers);

    webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            handleMessage(msg->str);
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            SPDLOG_INFO("[{}] Deepgram connection opened", stream_sid);
            std::lock_guard<std::mutex> lock(wsMutex);
            isConnected = true;
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            SPDLOG_ERROR("[{}] WebSocket Error: {}", stream_sid, msg->errorInfo.reason);
        }
    });

    webSocket.start();
}

void DeepgramSTT::ImplStreamAudioData(std::vector<uint8_t> audioData) {
    std::lock_guard<std::mutex> lock(wsMutex);
    if (isConnected) {
        std::string binaryData(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        webSocket.sendBinary(binaryData);
    }
}

void DeepgramSTT::ImplStopRecognition() {
    std::lock_guard<std::mutex> lock(wsMutex);
    if (isConnected) {
        webSocket.sendText("{\"type\": \"CloseStream\"}");
        webSocket.stop();
        isConnected = false;
    }
}

void DeepgramSTT::ImplRecognize() {
    // Nothing to do explicitly; recognition happens in handleMessage
}

void DeepgramSTT::handleMessage(const std::string& message) {
    auto parsed = json::parse(message, nullptr, false);
    if (parsed.is_discarded()) return;

    // Handle SpeechStarted event
    if (parsed.contains("type") && parsed["type"] == "SpeechStarted") {
        SPDLOG_INFO("[{}] 🎤 Speech started at {:.2f}s", stream_sid, parsed.value("timestamp", 0.0));
        return;
    }

    // Handle UtteranceEnd event
    if (parsed.contains("type") && parsed["type"] == "UtteranceEnd") {
        SPDLOG_INFO("[{}] 🛑 Utterance ended. Last word ended at {:.2f}s", stream_sid, parsed.value("last_word_end", 0.0));
        return;
    }

    // Handle transcript messages
    if (parsed.contains("channel") && parsed["channel"].contains("alternatives")) {
        const auto& alternatives = parsed["channel"]["alternatives"];
        if (alternatives.empty()) return;

        const auto& alt = alternatives[0];
        std::string transcript = alt.value("transcript", "");
        bool isFinal = parsed.value("is_final", false);
        bool speechFinal = parsed.value("speech_final", false);

        if (!transcript.empty()) {
            if (isFinal) {
                SPDLOG_INFO("[{}] ✅ FINAL: {}", stream_sid, transcript);
                RecognisedText(transcript); // user-defined callback
            } else {
                SPDLOG_INFO("[{}] 🟡 INTERIM: {}", stream_sid, transcript);
            }

            if (speechFinal) {
                SPDLOG_INFO("[{}] 📌 Speech endpointing triggered", stream_sid);
            }
        }
    }
}

