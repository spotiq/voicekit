#ifndef TTSFACTORY_H
#define TTSFACTORY_H

#include "I_TTSModule.h"
#include <memory>

class TTSFactory {
public:
    std::shared_ptr<I_TTSModule> CreateTTSModule(const std::string& provider, const std::string& stream_sid, std::function<void(const std::vector<uint8_t>&)> callback, std::string voiceName);
};

#endif // TTSFACTORY_H
