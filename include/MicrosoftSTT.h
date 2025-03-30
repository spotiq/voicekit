#ifndef MICROSOFT_STT_H
#define MICROSOFT_STT_H

#include "STTModuleBase.h"
#include <iostream>

class MicrosoftSTT : public STTModuleBase {
public:
    using STTModuleBase::STTModuleBase;
    void InitialiseSTTModule(const std::string& subscriptionKey, const std::string& region) override;
    void StartRecognition() override;
    void StopRecognition() override;
    void Recognize(const std::vector<uint8_t>& audioData) override;
};

#endif // MICROSOFT_STT_H
