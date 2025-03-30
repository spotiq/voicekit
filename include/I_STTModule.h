#ifndef I_STT_MODULE_H
#define I_STT_MODULE_H

#include <string>
#include <vector>
#include <functional>

class I_STTModule {
public:
    virtual ~I_STTModule() = default;
    virtual void InitialiseSTTModule(const std::string& subscriptionKey, const std::string& region) = 0;
    virtual void StartRecognition() = 0;
    virtual void StopRecognition() = 0;
    virtual void StreamAudioData(std::vector<uint8_t> audioData) = 0;
};

#endif // I_STT_MODULE_H
