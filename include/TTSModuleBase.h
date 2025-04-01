#ifndef TTSBASEMODULE_H
#define TTSBASEMODULE_H

#include "I_TTSModule.h"
#include "TTSCache.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <thread>
#include <spdlog/spdlog.h>

using namespace std;

class TTSModuleBase : public I_TTSModule {
protected:
    std::string stream_sid;
    std::string m_vendorName;
    std::string m_voiceName;
    std::function<void(const std::vector<uint8_t>&)> callback;
    std::queue<std::pair<std::string,std::string>> textQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    bool stopProcessing = false;
    std::thread processingThread;

    void SynthesisedAudioData(std::vector<uint8_t> audioData,std::string hashKey,int latencyMs);
    virtual void ImplSynthesiseVoice(const std::string&, const std::string&) = 0;
public:
    TTSModuleBase(const std::string& sid, std::function<void(const std::vector<uint8_t>&)> cb, std::string voiceName);
    virtual ~TTSModuleBase();
    void Speak(const std::string& text) override;
    void StopSpeak() override;

private:
    void PlayAudioBuffer(std::vector<uint8_t> audioBuffer);
    void ProcessText();
    std::vector<std::string> splitText(const std::string& text);
};

#endif // TTSBASEMODULE_H
