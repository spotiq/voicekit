#ifndef MICROSOFT_STT_H
#define MICROSOFT_STT_H

#include "STTModuleBase.h"
#include <iostream>
#include <speechapi_cxx.h>

using namespace std;
using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

class MicrosoftSTT : public STTModuleBase, public std::enable_shared_from_this<MicrosoftSTT> {
public:
    using STTModuleBase::STTModuleBase;
    void InitialiseSTTModule(const std::string& subscriptionKey, const std::string& region) override;
    void ImplStreamAudioData(std::vector<uint8_t> audioData) override;
    void ImplStartRecognition() override;
    void ImplStopRecognition() override;
    void ImplRecognize() override;
private:
    std::shared_ptr<SpeechConfig> speechConfig;
    std::shared_ptr<AudioConfig> audioConfig;
    std::shared_ptr<SpeechRecognizer> recognizer;
    std::shared_ptr<PushAudioInputStream> pushStream;
};

#endif // MICROSOFT_STT_H
