#include "AmazonTTS.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

bool AmazonTTS::Initialise(const std::string& key , const std::string& region) {
    m_region = region;

    Aws::InitAPI(m_options);

    Aws::Client::ClientConfiguration config;
    config.region = m_region;

    m_pollyClient = std::make_shared<Aws::Polly::PollyClient>(config);
    return true;
}

void AmazonTTS::ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) {
    std::unique_lock<std::mutex> lock(ttsProcessingMutex);
    SPDLOG_INFO("[{}] Synthesizing speech with Amazon Polly for text: {}", stream_sid, text);

    Aws::Polly::Model::SynthesizeSpeechRequest request;
    request.SetText(text);
    request.SetVoiceId(Aws::Polly::Model::VoiceIdMapper::GetVoiceIdForName(m_voiceName));
    request.SetOutputFormat(Aws::Polly::Model::OutputFormat::pcm);
    request.SetSampleRate("8000");

    auto startTime = std::chrono::high_resolution_clock::now();

    auto outcome = m_pollyClient->SynthesizeSpeech(request);

    if (!outcome.IsSuccess()) {
        SPDLOG_ERROR("[{}] Polly error: {}", stream_sid, outcome.GetError().GetMessage());
        return;
    }

    auto& stream = outcome.GetResult().GetAudioStream();
    std::vector<uint8_t> audioData((std::istreambuf_iterator<char>(stream)),
                                    std::istreambuf_iterator<char>());

    auto endTime = std::chrono::high_resolution_clock::now();
    auto ttsLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    SPDLOG_INFO("[{}] Received PCM audio of size: {}", stream_sid, audioData.size());
    SynthesisedAudioData(audioData, hashKey, ttsLatency);
}

void AmazonTTS::CloseConnection() {
    SPDLOG_INFO("[{}] Shutting down Amazon Polly TTS", stream_sid);
    Aws::ShutdownAPI(m_options);
}
