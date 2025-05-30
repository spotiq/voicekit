#ifndef AMAZON_STT_H
#define AMAZON_STT_H

#include "STTModuleBase.h"
#include "TranscribeManager.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class AmazonSTT : public STTModuleBase, public std::enable_shared_from_this<AmazonSTT> {
public:
    using STTModuleBase::STTModuleBase;
    void InitialiseSTTModule(const std::string& apiKey, const std::string& language) override;
    void ImplStreamAudioData(std::vector<uint8_t> audioData) override;
    void ImplStartRecognition() override;
    void ImplStopRecognition() override;
    void ImplRecognize() override;

private:
    std::string m_accessKey;
    std::string m_secretKey;
    std::string m_region;
    ix::WebSocket webSocket;
    std::mutex wsMutex;
    bool isConnected = false;
    void handleMessage(const std::string& message);
};

#endif // AMAZON_STT_H
