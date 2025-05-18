#include "GoogleSTT.h"
#include <iostream>
#include <thread>

void GoogleSTT::InitialiseSTTModule(const std::string& projectId, const std::string& regionId) {
    m_projectId = projectId;
    // Construct the endpoint for asia-south1 (Bengaluru) for Speech V2.
    std::string endpoint = regionId + "-speech.googleapis.com";

    // Create options and set the endpoint.
    google::cloud::Options options;
    options.set<google::cloud::EndpointOption>(endpoint);

    // Create the SpeechClient (uses GOOGLE_APPLICATION_CREDENTIALS)
    m_client = std::make_unique<speech::SpeechClient>(speech::MakeSpeechConnection(options));
}

void GoogleSTT::ConfigureStreamRequest(google::cloud::speech::v2::StreamingRecognizeRequest& config_req) {
    auto* cfg = config_req.mutable_streaming_config()->mutable_config();
    auto* explicit_decoding_config = cfg->mutable_explicit_decoding_config();

    std::string recognizer = "projects/" + m_projectId + "/locations/global/recognizers/_";
    
    config_req.mutable_recognizer()->append(recognizer);

    cfg->add_language_codes(language);
    cfg->set_model("latest_long");                     

    auto* streaming_cfg = config_req.mutable_streaming_config();
    streaming_cfg->mutable_streaming_features()->enable_voice_activity_events();
    streaming_cfg->mutable_streaming_features()->set_interim_results(true);

    explicit_decoding_config->set_encoding(google::cloud::speech::v2::ExplicitDecodingConfig_AudioEncoding::ExplicitDecodingConfig_AudioEncoding_LINEAR16);
    explicit_decoding_config->set_sample_rate_hertz(8000);
    explicit_decoding_config->set_audio_channel_count(1);

    cfg->features().enable_automatic_punctuation();
}

void GoogleSTT::ImplStartRecognition() {
    if (!m_client) {
        std::cerr << "Error: STT client not initialized.\n";
        return;
    }

    m_stream = m_client->AsyncStreamingRecognize();
    if (!m_stream) {
        std::cerr << "Error: Failed to create streaming recognize stream.\n";
        return;
    }

    try {
        // Send the initial config
        google::cloud::speech::v2::StreamingRecognizeRequest request;
        ConfigureStreamRequest(request);

        // The stream can fail to start, and `.get()` can throw an exception
        // containing the error status in this case.
        bool start_status = m_stream->Start().get();
        if (!start_status) {
            throw std::runtime_error(
                "Error starting stream: ");
        }

        // Write the first request, containing the config only.
        bool write_config_ok = m_stream->Write(request, grpc::WriteOptions{}).get();
        if (!write_config_ok) {
            google::cloud::Status finish_status = m_stream->Finish().get();
            throw std::runtime_error(
                "Error writing initial config: " + finish_status.message());
        }

        is_streaming = true;

        m_reader = std::thread([this] {
            while (true) {
                auto resp = m_stream->Read().get();
                if (!resp.has_value()) break;  // stream closed

                for (auto const& result : resp->results()) {
                    if (result.is_final()){
                        std::cout << "[Final] ";
                        for (auto const& alt : result.alternatives()) {
                            std::string text = alt.transcript();  // Copy into a non-const std::string
                            RecognisedText(text);
                            std::cout << text << std::endl;
                        }
                    }else{
                        std::cout << "[Interim] ";
                    }
                }
            }
            // Get final status
            auto status = m_stream->Finish().get();
            if (!status.ok()) {
                std::cerr << "Stream finished with error: "
                        << status.code() << ": " << status.message() << "\n";
            }
            m_finished = true;
        });
    } catch (const std::runtime_error& e) {
        std::cerr << "Error during recognition: " << e.what() << "\n";
        if (m_stream) {
            m_stream->WritesDone().get(); // Signal no more writes.
            m_stream->Finish().get();    // Close the stream.
            m_stream.reset();
        }
        is_streaming = false;
    }
}

void GoogleSTT::ImplRecognize() {
    
}

void GoogleSTT::ImplStreamAudioData(std::vector<uint8_t> audioData) {
    if (!is_streaming || !m_stream) return;

    if (audioData.empty()) return; // Don't send empty chunks

    google::cloud::speech::v2::StreamingRecognizeRequest audio_req;
    audio_req.mutable_audio()->append(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    if (!m_stream->Write(audio_req, grpc::WriteOptions{}).get()) {
        std::cerr << "Failed to write audio chunk\n";
    }
}

void GoogleSTT::ImplStopRecognition() {
    if (is_streaming && m_stream) {
        bool writes_done_ok = m_stream->WritesDone().get();
        if (!writes_done_ok) {
            std::cerr << "Failed to signal writes done.\n";
            // We don't have a detailed status here, just a boolean.
        }
        m_reader.join();
        if (!m_finished) {
            // If the reader thread didn’t see Finish(), ensure we call it
            google::cloud::Status finish_status = m_stream->Finish().get();
            if (!finish_status.ok()) {
                std::cerr << "Stream finished with error: " << finish_status.code()
                        << ", " << finish_status.message() << "\n";
                // Handle the error appropriately.
            }
        }
        is_streaming = false;
        m_stream.reset(); // Release the stream object.
    }
}