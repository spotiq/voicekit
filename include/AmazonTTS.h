#ifndef AMAZONTTS_H
#define AMAZONTTS_H

#include "TTSModuleBase.h"
#include <aws/core/Aws.h>
#include <aws/polly/PollyClient.h>
#include <aws/polly/model/SynthesizeSpeechRequest.h>
#include <mutex>
#include <atomic>

class AmazonTTS : public TTSModuleBase {
public:
    using TTSModuleBase::TTSModuleBase;

    bool Initialise(const std::string& apiKey, const std::string& voiceName) override;
    void ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) override;
    void CloseConnection();

private:
    Aws::SDKOptions m_options;
    std::shared_ptr<Aws::Polly::PollyClient> m_pollyClient;
    std::string m_region;
    std::mutex ttsProcessingMutex;
};

#endif // AMAZONTTS_H
