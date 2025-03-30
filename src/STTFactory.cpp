#include "STTFactory.h"
#include "MicrosoftSTT.h"

std::unique_ptr<I_STTModule> STTFactory::CreateSTTModule(const std::string& provider, const std::string& stream_sid, std::function<void(std::string&)> callback, std::string language) {
    if (provider == "Microsoft") {
        return std::make_unique<MicrosoftSTT>(stream_sid, callback, language);
    }
    // Add other providers here
    throw std::runtime_error("Unsupported STT provider");
}
