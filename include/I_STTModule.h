#ifndef I_STT_MODULE_H
#define I_STT_MODULE_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

struct STTConfig {
    std::string vendor;         // E.g., "Azure", "Google", "AWS"
    std::string apiKey;         // Vendor key
    std::string region;         // Microsoft region. 
    std::string language;       // default: en-US
    std::string model;          // default: 
    int sampleRate;             // default: 8000 ; 8000 16000 48000
    int channel;                // default: mono ; mono sterio. 
    int initialSilenceInMs;     // default: 5000ms
    int segmentSilenceInMs;     // default: 2000ms
    int finalSilenceInMs;       // default: 1000ms
    std::string boostPhrases;   // comma seperated boost phrases.
    bool profanityFilter;       // Optional vendor-specific settings
};

class I_STTModule {
public:
    virtual ~I_STTModule() = default;
    virtual void InitialiseSTTModule(const std::string& subscriptionKey, const std::string& region) = 0;
    virtual void StartRecognition() = 0;
    virtual void StopRecognition() = 0;
    virtual void StreamAudioData(std::vector<uint8_t> audioData) = 0;
};

#endif // I_STT_MODULE_H
