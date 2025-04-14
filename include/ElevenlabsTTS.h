#ifndef ELEVENLABSTTS_H
#define ELEVENLABSTTS_H

#include "TTSModuleBase.h"
#include "base64.hpp"

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <vector>
#include <atomic>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <optional>
#include <cpr/cpr.h>

using json = nlohmann::json;

class ElevenlabsTTS : public TTSModuleBase {
public:
    using TTSModuleBase::TTSModuleBase;

    bool Initialise(const std::string& apiKey, const std::string& voiceId) override;
    void ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) override;
    void CloseConnection();

private:
    void startWebSocket();
    void handleMessage(const std::string& message);
    std::string buildWebSocketURL() const;
    void sendInitialSettings();
    std::optional<std::pair<std::string, std::string>> getVoiceIdAndModel(const std::string& voiceName);

    ix::WebSocket webSocket;
    std::mutex sendMutex;
    std::mutex wsMutex;
    std::mutex audioMutex;
    std::mutex accumulatedAudioMutex;
    std::mutex ttsProcessingMutex;

    std::condition_variable wsCv;
    
    std::mutex m_finalMutex;
    std::condition_variable m_finalCv;

    std::atomic<bool> m_isConnected{false};
    std::atomic<bool> m_isFinalReceived{false};

    std::string m_apiKey;
    std::string m_currentHashKey;

    std::chrono::high_resolution_clock::time_point m_startTime;
    std::vector<uint8_t> m_accumulatedAudioBuffer;

    std::string m_voiceId;
    std::string m_modelId;
};

#endif // ELEVENLABSTTS_H
