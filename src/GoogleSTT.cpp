#include "GoogleSTT.h"
#include <iostream>
#include <thread>


GoogleSTT::GoogleSTT(const std::string& sid, std::function<void(std::string&)> cb, const std::string& lang)
    : session_id(sid), callback(cb), language(lang) {}

void GoogleSTT::InitialiseSTTModule(const std::string& projectId, const std::string& regionId) {
    project = projectId;
    region = regionId;
    connection = speech::MakeSpeechConnection(region == "global" ? "" : region);
    client = std::make_unique<speech::SpeechClient>(connection);
}

void GoogleSTT::ConfigureStreamRequest(google::cloud::speech::v2::StreamingRecognizeRequest& request) {
    auto* config = request.mutable_streaming_config();
    config->mutable_config()->add_language_codes(language);
    config->mutable_config()->set_model("latest_long");
    *config->mutable_config()->mutable_auto_decoding_config() = {};
    // config-set_config_mask("language_codes,model,auto_decoding_config");
}

void GoogleSTT::ImplStartRecognition() {
    if (!client) {
        std::cerr << "STT client not initialized.\n";
        return;
    }

    m_stream = client->AsyncStreamingRecognize();

    // Send the initial config
    google::cloud::speech::v2::StreamingRecognizeRequest request;
    ConfigureStreamRequest(request);

    // The stream can fail to start, and `.get()` returns an error in this case.
    if (!m_stream->Start().get()) throw m_stream->Finish().get();

    // Write the first request, containing the config only.
    if (!m_stream->Write(request, grpc::WriteOptions{}).get()) {
        // Write().get() returns false if the m_stream is closed.
        throw m_stream->Finish().get();
    }

    is_streaming = true;

    // Read responses.
    auto read = [this] { return m_stream->Read().get(); };
    for (auto response = read(); response.has_value(); response = read()) {
        // Dump the transcript of all the results.
        for (auto const& result : response->results()) {
            std::cout << "Result stability: " << result.stability() << "\n";
            for (auto const& alternative : result.alternatives()) {
            std::cout << alternative.confidence() << "\t"
                        << alternative.transcript() << "\n";
            }
        }
    }

    // std::thread([this]() {
    //     google::cloud::speech::v2::StreamingRecognizeResponse response;
    //     while (stream->Read(response)) {
    //         std::string result_text;
    //         for (const auto& result : response.results()) {
    //             if (result.alternatives_size() > 0) {
    //                 result_text += result.alternatives(0).transcript();
    //                 if (result.is_final()) {
    //                     callback(result_text);
    //                 }
    //             }
    //         }
    //     }
    // }).detach();
}

void GoogleSTT::ImplStreamAudioData(std::vector<uint8_t> audioData) {
    if (!is_streaming || !m_stream) return;

    google::cloud::speech::v2::StreamingRecognizeRequest audio_request;
    audio_request.mutable_audio()->append(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    m_stream->Write(audio_request, grpc::WriteOptions{});
}

void GoogleSTT::ImplStopRecognition() {
    if (is_streaming && m_stream) {
        m_stream->WritesDone();
        m_stream->Finish();
        is_streaming = false;
    }
}
