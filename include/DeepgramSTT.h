#ifndef DEEPGRAM_STT_H
#define DEEPGRAM_STT_H

#include "STTModuleBase.h"
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include <string>
#include <mutex>

using json = nlohmann::json;

class DeepgramSTT : public STTModuleBase, public std::enable_shared_from_this<DeepgramSTT> {
public:
    using STTModuleBase::STTModuleBase;
    void InitialiseSTTModule(const std::string& apiKey, const std::string& language) override;
    void ImplStreamAudioData(std::vector<uint8_t> audioData) override;
    void ImplStartRecognition() override;
    void ImplStopRecognition() override;
    void ImplRecognize() override;

private:
    std::string apiKey;
    ix::WebSocket webSocket;
    std::mutex wsMutex;
    bool isConnected = false;
    void handleMessage(const std::string& message);
};

#endif // DEEPGRAM_STT_H
