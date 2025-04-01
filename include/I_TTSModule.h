#ifndef I_TTSMODULE_H
#define I_TTSMODULE_H

#include <string>
#include <vector>
#include <functional>


class I_TTSModule {
public:
    virtual ~I_TTSModule() = default;

    // Initialize TTS module with necessary parameters
    virtual bool Initialise(const std::string& apiKey, const std::string& region) = 0;

    // Convert text to speech and play audio at intervals
    virtual void Speak(const std::string& text) = 0;

    // Stop speak in case of interruption
    virtual void StopSpeak() = 0;
};

#endif // I_TTSMODULE_H
