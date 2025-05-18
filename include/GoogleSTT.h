#pragma once

#include "STTModuleBase.h"

#include <google/cloud/speech/v2/speech_client.h>
#include <google/cloud/speech/v2/speech_connection.h>
#include <google/cloud/speech/v2/cloud_speech.pb.h>
#include <google/cloud/speech/v2/cloud_speech.grpc.pb.h>
#include <google/cloud/project.h>
#include <google/cloud/experimental_tag.h>
#include <google/cloud/status_or.h>
#include <google/cloud/status.h> // Make sure to include the Status header
#include <google/cloud/common_options.h>

#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <vector>

namespace speech = ::google::cloud::speech_v2;

class GoogleSTT : public STTModuleBase {
public:
    using STTModuleBase::STTModuleBase;

    void InitialiseSTTModule(const std::string& projectId, const std::string& regionId) override;
    void ImplStartRecognition() override;
    void ImplStreamAudioData(std::vector<uint8_t> audioData) override;
    void ImplStopRecognition() override;
    void ImplRecognize() override;
private:
    void ConfigureStreamRequest(google::cloud::speech::v2::StreamingRecognizeRequest& request);

    std::string session_id;
    std::function<void(std::string&)> callback;
    std::string m_projectId;
    std::string m_region;

    std::unique_ptr<speech::SpeechClient> m_client;
    std::atomic<bool> m_finished{false};
    std::thread m_reader;

    std::unique_ptr<google::cloud::AsyncStreamingReadWriteRpc<
        google::cloud::speech::v2::StreamingRecognizeRequest,
        google::cloud::speech::v2::StreamingRecognizeResponse>> m_stream;

    bool is_streaming = false;
};
