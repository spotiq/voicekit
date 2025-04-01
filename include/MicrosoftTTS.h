#ifndef MICROSOFTTTS_H
#define MICROSOFTTTS_H

#include "TTSModuleBase.h"

#include <iostream>
#include <speechapi_cxx.h>

using namespace std;
using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

class MicrosoftTTS : public TTSModuleBase {
public:
    using TTSModuleBase::TTSModuleBase;
    bool Initialise(const std::string& apiKey, const std::string& region) override;
    void ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) override; 
private:
    std::shared_ptr<SpeechConfig> speechConfig;
    std::shared_ptr<SpeechSynthesizer> synthesizer;
    std::unique_ptr<std::thread> synthesisThread;
};

#endif // MICROSOFTTTS_H
