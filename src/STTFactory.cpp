#include "STTFactory.h"
#include "MicrosoftSTT.h"
#include "DeepgramSTT.h"

std::shared_ptr<I_STTModule> STTFactory::CreateSTTModule(const std::string& provider, const std::string& stream_sid, std::function<void(std::string&)> callback, std::string language) {
    if (provider == "Microsoft") {
        return std::make_shared<MicrosoftSTT>(stream_sid, callback, language);
    } else if (provider == "Deepgram") {
        return std::make_shared<DeepgramSTT>(stream_sid, callback, language);
    }
    // Add other providers here
    throw std::runtime_error("Unsupported STT provider");
}
