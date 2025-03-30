#ifndef STT_MODULE_BASE_H
#define STT_MODULE_BASE_H

#include "I_STTModule.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

class STTModuleBase : public I_STTModule {
protected:
    std::string stream_sid;
    std::function<void(std::string&)> callback;
    std::string language;
    std::queue<std::vector<uint8_t>> audioQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    bool stopProcessing = false;
    std::thread processingThread;

    void ProcessAudioStream();
    virtual void Recognize(const std::vector<uint8_t>& audioData) = 0;

public:
    STTModuleBase(const std::string& sid, std::function<void(std::string&)> cb, std::string lang);
    virtual ~STTModuleBase();
    void StreamAudioData(std::vector<uint8_t> audioData) override;
};

#endif // STT_MODULE_BASE_H
