#include "STTModuleBase.h"

STTModuleBase::STTModuleBase(const std::string& sid, std::function<void(std::string&)> cb, std::string lang)
    : stream_sid(sid), callback(cb), language(lang), processingThread(&STTModuleBase::ProcessAudioStream, this) {}

STTModuleBase::~STTModuleBase() {
    stopProcessing = true;
    queueCV.notify_all();
    if (processingThread.joinable()) processingThread.join();
}

void STTModuleBase::ProcessAudioStream() {
    while (!stopProcessing) {
        std::vector<uint8_t> audioData;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] { return !audioQueue.empty() || stopProcessing; });
            if (stopProcessing) break;
            audioData = std::move(audioQueue.front());
            audioQueue.pop();
        }
        Recognize(audioData);
    }
}

void STTModuleBase::StreamAudioData(std::vector<uint8_t> audioData) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        audioQueue.push(std::move(audioData));
    }
    queueCV.notify_one();
}
