#ifndef STT_FACTORY_H
#define STT_FACTORY_H

#include "I_STTModule.h"
#include <memory>

class STTFactory {
public:
    static std::unique_ptr<I_STTModule> CreateSTTModule(const std::string& provider, const std::string& stream_sid, std::function<void(std::string&)> callback, std::string language);
};

#endif // STT_FACTORY_H
