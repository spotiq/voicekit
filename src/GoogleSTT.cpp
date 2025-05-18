#include "GoogleSTT.h"
#include <iostream>
#include <thread>

void GoogleSTT::InitialiseSTTModule(const std::string& projectId, const std::string& regionId) {
    project = projectId;
    region = regionId;
    connection = speech::MakeSpeechConnection();
    client = std::make_unique<speech::SpeechClient>(connection);
}

void GoogleSTT::ConfigureStreamRequest(google::cloud::speech::v2::StreamingRecognizeRequest& request) {
    auto* streaming_config = request.mutable_streaming_config();
    auto* recognition_config = streaming_config->mutable_config();
    // auto* explicit_decoding_config = recognition_config->mutable_explicit_decoding_config();

    recognition_config->add_language_codes(language);
    recognition_config->set_model("latest_long");
    // explicit_decoding_config->set_encoding(google::cloud::speech::v2::ExplicitDecodingConfig_AudioEncoding::ExplicitDecodingConfig_AudioEncoding_LINEAR16);
    // explicit_decoding_config->set_sample_rate_hertz(8000);
    // When using explicit decoding, you should ensure auto_decoding is not enabled.
    // recognition_config->clear_auto_decoding_config();
    // config_mask is automatically populated.
    *recognition_config->mutable_auto_decoding_config() = google::cloud::speech::v2::AutoDetectDecodingConfig();
}

void GoogleSTT::ImplStartRecognition() {
    if (!client) {
        std::cerr << "Error: STT client not initialized.\n";
        return;
    }

    m_stream = client->AsyncStreamingRecognize();
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

        // Read responses in a separate thread to avoid blocking the main flow.
        std::thread response_thread([this] {
            auto read = [this] { return m_stream->Read().get(); };
            while (is_streaming) {
                auto response = read();
                if (response.has_value()) {
                    // Process the response
                    for (auto const& result : response->results()) {
                        std::cout << "Result stability: " << result.stability() << "\n";
                        for (auto const& alternative : result.alternatives()) {
                            std::cout << alternative.confidence() << "\t"
                                      << alternative.transcript() << "\n";
                        }
                    }
                } else {
                    // Stream ended or an error occurred during read.
                    // We should try to get the final status.
                    google::cloud::Status finish_status = m_stream->Finish().get();
                    if (!finish_status.ok()) {
                        std::cerr << "Error reading responses: " << finish_status.message() << "\n";
                        // Consider setting an error flag in your class.
                    }
                    break; // Exit the loop
                }
            }
        });
        response_thread.detach(); // Allow the thread to run independently.

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

void GoogleSTT::ImplStreamAudioData(std::vector<uint8_t> audioData) {
    if (!is_streaming || !m_stream) return;

    if (audioData.empty()) return; // Don't send empty chunks

    google::cloud::speech::v2::StreamingRecognizeRequest audio_request;
    audio_request.mutable_audio()->append(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    if (!m_stream->Write(audio_request, grpc::WriteOptions{}).get()) {
        std::cerr << "Failed to write audio chunk to stream. : " << std::endl;
    }
}

void GoogleSTT::ImplRecognize() {
    
}

void GoogleSTT::ImplStopRecognition() {
    if (is_streaming && m_stream) {
        bool writes_done_ok = m_stream->WritesDone().get();
        if (!writes_done_ok) {
            std::cerr << "Failed to signal writes done.\n";
            // We don't have a detailed status here, just a boolean.
        }

        google::cloud::Status finish_status = m_stream->Finish().get();
        if (!finish_status.ok()) {
            std::cerr << "Stream finished with error: " << finish_status.code()
                      << ", " << finish_status.message() << "\n";
            // Handle the error appropriately.
        }

        is_streaming = false;
        m_stream.reset(); // Release the stream object.
    }
}