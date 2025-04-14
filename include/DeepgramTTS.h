#ifndef DEEPGRAMTTS_H
#define DEEPGRAMTTS_H

#include "TTSModuleBase.h"
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <vector>
#include <atomic>

class DeepgramTTS : public TTSModuleBase {
public:
    using TTSModuleBase::TTSModuleBase;

    bool Initialise(const std::string& apiKey, const std::string& voiceName) override;
    void ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) override;
    void CloseConnection();

private:
    void StartWebSocket();
    void handleMessage(const std::string& message);

    std::string buildWebSocketURL() const;

    ix::WebSocket webSocket;
    std::mutex sendMutex;
    std::mutex wsMutex;
    std::string m_apiKey;

    std::atomic<bool> isConnected{false};

    std::mutex ttsProcessingMutex;

    
    std::mutex audioMutex;
    std::atomic<bool> isAudioAvailable{false};
    
    std::condition_variable wsCv;
    std::condition_variable audioAvailableCv;


    std::string m_currentHashKey;
    std::chrono::high_resolution_clock::time_point m_startTime;

    std::vector<uint8_t> accumulatedAudioBuffer;
    std::mutex accumulatedAudioMutex;

    std::atomic<bool> isFlushedReceived = {false};
    std::condition_variable flushedCv;
    std::mutex flushedMutex;


};

#endif // DEEPGRAMTTS_H
