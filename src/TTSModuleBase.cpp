#include "TTSModuleBase.h"


TTSModuleBase::TTSModuleBase(const std::string& sid, std::function<void(const std::vector<uint8_t>&)> cb, std::string voiceName)
    : stream_sid(sid), callback(cb), m_voiceName(voiceName), processingThread(&TTSModuleBase::ProcessText, this) {}

TTSModuleBase::~TTSModuleBase() {
    stopProcessing = true;
    queueCV.notify_all();
    if (processingThread.joinable()) processingThread.join();
}

std::vector<std::string> TTSModuleBase::splitText(const std::string& text) {
    std::vector<std::string> segments;
    if (text.empty()) {
        SPDLOG_ERROR("[{}] Input text is empty. Returning empty segments.", stream_sid);
        return segments;
    }

    std::string temp;
    const std::string delimiters = ".?"; // Use std::string instead of char array
    size_t length = text.length();

    for (size_t i = 0; i < length; ++i) {
        char ch = text[i];
        temp += ch;

        if (delimiters.find(ch) != std::string::npos || i == length - 1) { // Check for delimiter
            if (!temp.empty() && temp.find_first_not_of(" \t\n\r") != std::string::npos) {
                segments.push_back(temp);
            }
            temp.clear();
        }
    }

    return segments;
}

void TTSModuleBase::PlayAudioBuffer(std::vector<uint8_t> audioBuffer){
    /* 
        Temporary buffer for reading 20ms=160 40ms = 320 80ms = 640

        Why is 320 Samples Correct for 40ms?
            8kHz sample rate means 8,000 samples per second.
            40ms frame duration = (40/1000) * 8000 = 320 samples.
            SpeexDSP allows variable frame sizes, so 320 samples per frame is valid.
            Each sample is 16-bit (int16_t = 2 bytes per sample) => Total frame size in bytes: 320×2=640 bytes
    */

    int ms = 20;
    int sampleRate = 8000;
    int samples = (ms * sampleRate) / 1000;
    int bitDepth = 16;
    int bytesPerChunk = samples * (bitDepth / 8);

    if (audioBuffer.size()==0){
        if(!stopProcessing){
            callback(audioBuffer); // Process each chunk
        }
    }

    for (size_t i = 0; i < audioBuffer.size(); i += bytesPerChunk) {
        std::vector<uint8_t> tempBuffer(audioBuffer.begin() + i, 
                                        audioBuffer.begin() + std::min(i + bytesPerChunk, audioBuffer.size()));
        
        if(!stopProcessing){
            callback(tempBuffer); // Process each chunk
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms)); // Simulate playback delay
    }
}

void TTSModuleBase::ProcessText() {
    pthread_setname_np(pthread_self(), "TTSModuleBaseThread");
    while (!stopProcessing) {
        std::pair<std::string, std::string> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] { return stopProcessing || !textQueue.empty(); });
            
            if (stopProcessing) {
                while (!textQueue.empty()) {
                    textQueue.pop();
                }
                SPDLOG_INFO("[{}] Stopping synthesis thread.",stream_sid);
                return; // Exit thread
            }
            
            if (textQueue.empty()) {
                continue; // Continue waiting if queue is empty but stopThread is false
            }
            task = std::move(textQueue.front());
            textQueue.pop();
        }

        std::string command = task.first;
        std::string text = task.second;

        std::vector<std::string> segments = splitText(text);

        for (auto it = segments.begin(); it != segments.end(); ++it) {
            auto segment = *it;  
            
            SPDLOG_INFO("[{}] Segment : {}", stream_sid, segment);

            if (it == segments.begin()) {
                SPDLOG_INFO( "[{}] Start PLAY",stream_sid );
            } 

            std::string hashKey = TTSCache::generateHash(m_vendorName, m_voiceName, segment);

            // Check cache first
            if (TTSCache::getInstance().isCached(hashKey)) {
                SPDLOG_INFO("[{}] Using cached TTS for '{}'", stream_sid, segment);
                std::vector<uint8_t> cachedAudio = TTSCache::getInstance().getCachedAudio(hashKey);
                PlayAudioBuffer(cachedAudio);
                if (std::next(it) == segments.end()) {
                    SPDLOG_INFO( "[{}] Stop PLAY",stream_sid );
                    std::vector<uint8_t> tempBuffer;
                    tempBuffer.clear();
                    PlayAudioBuffer(tempBuffer);
                }
                continue;
            }

            // Call text to speech synthesiser 
            ImplSynthesiseVoice(segment,hashKey);

            if (std::next(it) == segments.end()) {
                SPDLOG_INFO( "[{}] Stop PLAY",stream_sid );
                std::vector<uint8_t> tempBuffer;
                tempBuffer.clear();
                PlayAudioBuffer(tempBuffer);
            }
        }
    }
}

void TTSModuleBase::SynthesisedAudioData(std::vector<uint8_t> audioData,std::string hashKey,int latencyMs) {
    TTSCache::getInstance().saveToCache(hashKey, audioData);
    PlayAudioBuffer(audioData);
}

void TTSModuleBase::Speak(const std::string& text) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        textQueue.push({"start",text});
    }
    queueCV.notify_one();
}


void TTSModuleBase::StopSpeak() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        textQueue.push({"stop",""});
    }
    queueCV.notify_one();
}