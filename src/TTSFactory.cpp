#include "TTSFactory.h"

#include "MicrosoftTTS.h"
#include "DeepgramTTS.h"
#include "ElevenlabsTTS.h"
#include "AmazonTTS.h"

std::shared_ptr<I_TTSModule> TTSFactory::CreateTTSModule(const std::string& provider, const std::string& stream_sid, std::function<void(const std::vector<uint8_t>&)> callback, std::string voiceName) {
    if (provider == "Microsoft") {
        return std::make_shared<MicrosoftTTS>(stream_sid, callback, voiceName);
    } else if (provider == "Deepgram") {
        return std::make_shared<DeepgramTTS>(stream_sid, callback, voiceName);
    } else if (provider == "Elevenlabs") {
        return std::make_shared<ElevenlabsTTS>(stream_sid, callback, voiceName);
    } else if (provider == "Amazon") {
        return std::make_shared<AmazonTTS>(stream_sid, callback, voiceName);
    }
    // Add other providers here
    throw std::runtime_error("Unsupported TTS provider");
}
