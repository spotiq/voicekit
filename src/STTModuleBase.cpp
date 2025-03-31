#include "STTModuleBase.h"

STTModuleBase::STTModuleBase(const std::string& sid, std::function<void(std::string&)> cb, std::string lang)
    : stream_sid(sid), callback(cb), language(lang), processingThread(&STTModuleBase::ProcessAudioStream, this) {}

STTModuleBase::~STTModuleBase() {
    stopProcessing = true;
    queueCV.notify_all();
    if (processingThread.joinable()) processingThread.join();
}

void STTModuleBase::RecognisedText(std::string& text) {
    if (!stopProcessing){
        callback(text);
    }
}

void STTModuleBase::ProcessAudioStream() {
    pthread_setname_np(pthread_self(), "STTModuleBaseThread");
    while (!stopProcessing) {
        std::tuple<std::string, std::function<void(std::string&)>,std::vector<uint8_t>> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] { return stopProcessing || !audioQueue.empty(); });
            
            if (stopProcessing) {
                while (!audioQueue.empty()) {
                    audioQueue.pop();
                }
                SPDLOG_INFO("[{}] Stopping recognise thread.",stream_sid);
                return; // Exit thread
            }
            
            if (audioQueue.empty()) {
                continue; // Continue waiting if queue is empty but stopThread is false
            }
            task = std::move(audioQueue.front());
            audioQueue.pop();
        }

        std::string taskType = std::get<0>(task);
        std::function<void(std::string&)> lambdaCallback = std::get<1>(task);
        std::vector<uint8_t> audioData = std::get<2>(task);

        try{
            if (taskType == "media") {
                SPDLOG_INFO("Stream Audio. size={}",audioData.size());
                ImplStreamAudioData(audioData);
            }else if (taskType == "start"){
                SPDLOG_INFO("[{}] Start RecognizeSpeech.",stream_sid);
                ImplStartRecognition();
            }else if (taskType == "stop"){
                SPDLOG_INFO("[{}] Stop RecognizeSpeech.",stream_sid);
                ImplStopRecognition();
            }
        } catch (const std::exception &e) {
            SPDLOG_ERROR( "{}" , e.what() );
        }
    }
}

void STTModuleBase::StreamAudioData(std::vector<uint8_t> audioData) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        audioQueue.push({"media", callback, audioData});
    }
    queueCV.notify_one();
}

void STTModuleBase::StartRecognition() {
    {
        std::vector<uint8_t> audioData;
        std::lock_guard<std::mutex> lock(queueMutex);
        audioQueue.push({"start", callback, audioData});
    }
    queueCV.notify_one();
}

void STTModuleBase::StopRecognition() {
    {
        std::vector<uint8_t> audioData;
        std::lock_guard<std::mutex> lock(queueMutex);
        audioQueue.push({"stop", callback, audioData});
    }
    queueCV.notify_one();
}
