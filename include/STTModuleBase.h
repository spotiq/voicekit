#ifndef STT_MODULE_BASE_H
#define STT_MODULE_BASE_H

#include "I_STTModule.h"
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <spdlog/spdlog.h>


class STTModuleBase : public I_STTModule {
protected:
    std::string stream_sid;
    std::function<void(std::string&)> callback;
    std::string language;
    std::queue<std::tuple<std::string, std::function<void(std::string&)>,std::vector<uint8_t>>> audioQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    bool stopProcessing = false;
    std::thread processingThread;

    void ProcessAudioStream();
    void RecognisedText(std::string& text);

    virtual void ImplStreamAudioData(std::vector<uint8_t> audioData) = 0;
    virtual void ImplStartRecognition() = 0;
    virtual void ImplStopRecognition() = 0;
    virtual void ImplRecognize() = 0;

public:
    STTModuleBase(const std::string& sid, std::function<void(std::string&)> cb, std::string lang);
    virtual ~STTModuleBase();
    void StreamAudioData(std::vector<uint8_t> audioData) override;
    void StartRecognition() override;
    void StopRecognition() override;
};

#endif // STT_MODULE_BASE_H
