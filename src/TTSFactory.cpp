#include "TTSFactory.h"

#include "MicrosoftTTS.h"

std::shared_ptr<I_TTSModule> TTSFactory::CreateTTSModule(const std::string& provider, const std::string& stream_sid, std::function<void(const std::vector<uint8_t>&)> callback, std::string voiceName) {
    if (provider == "Microsoft") {
        return std::make_shared<MicrosoftTTS>(stream_sid, callback, voiceName);
    }
    // Add other providers here
    throw std::runtime_error("Unsupported TTS provider");
}
