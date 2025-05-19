#include <iostream>
#include <string>
#include <thread>
#include <memory> // For std::unique_ptr

#include <google/cloud/texttospeech/v1/text_to_speech_client.h>
#include <google/cloud/common_options.h>
#include <google/cloud/status.h> // Include for google::cloud::Status

int main() {
    namespace texttospeech = ::google::cloud::texttospeech_v1;

    // Use std::unique_ptr to manage the client's lifetime.
    std::unique_ptr<texttospeech::TextToSpeechClient> client;
    try {
        client = std::make_unique<texttospeech::TextToSpeechClient>(
            texttospeech::MakeTextToSpeechConnection());
    } catch (const std::exception& ex) {
        std::cerr << "Error creating TextToSpeechClient: " << ex.what() << "\n";
        return 1;
    }

    // Declare stream outside the try block
    auto stream = client->AsyncStreamingSynthesize();
    if (!stream) {
        std::cerr << "Error: AsyncStreamingSynthesize returned a null stream.\n";
        return 1;
    }

    try {
        // Start the stream and wait for it to be ready
        auto start_status = stream->Start().get();
        if (!start_status) {
            std::cerr << "Failed to start the stream: " << start_status << "\n";
            return 1;
        }

        // Send the initial configuration
        google::cloud::texttospeech::v1::StreamingSynthesizeRequest config_request;
        google::cloud::texttospeech::v1::VoiceSelectionParams voice;
        voice.set_language_code("en-US");
        voice.set_name("en-US-Chirp-HD-F");
        google::cloud::texttospeech::v1::StreamingAudioConfig audio;
        audio.set_audio_encoding(google::cloud::texttospeech::v1::PCM);
        audio.set_sample_rate_hertz(8000);

        *config_request.mutable_streaming_config()->mutable_voice() = voice;
        *config_request.mutable_streaming_config()->mutable_streaming_audio_config() = audio;

        bool write_config_ok = stream->Write(config_request, grpc::WriteOptions{}).get();
        if (!write_config_ok) {
            google::cloud::Status finish_status = stream->Finish().get();
            std::cerr << "Failed to write config: " << finish_status << "\n";
            return 1;
        }

        // Send text input
        google::cloud::texttospeech::v1::StreamingSynthesizeRequest input_request;
        input_request.mutable_input()->set_text("Hello, this is a streaming synthesis test.");
        bool write_text_ok = stream->Write(input_request, grpc::WriteOptions{}).get();
        if (!write_text_ok) {
            google::cloud::Status finish_status = stream->Finish().get();
            std::cerr << "Failed to write text: " << finish_status << "\n";
            return 1;
        }

        // Signal that we've finished sending requests
        bool writes_done_ok = stream->WritesDone().get();
        if (!writes_done_ok) {
            google::cloud::Status finish_status = stream->Finish().get();
            std::cerr << "Failed to signal WritesDone: " << finish_status << "\n";
            return 1;
        }

        // Start a thread to read responses
        std::thread reader([&stream]() {
            for (;;) {
                auto response = stream->Read().get();
                if (!response.has_value()) break;
                // Process the audio content
                std::cout << "Received audio chunk of size: "
                          << response->audio_content().size() << " bytes\n";
            }
        });

        // Wait for the reader thread to finish
        reader.join();

        // Get the final status of the stream.
        google::cloud::Status finish_status = stream->Finish().get();
        if (!finish_status.ok()) {
            std::cerr << "Stream finished with error: " << finish_status << "\n";
            return 1;
        }
        std::cout << "Stream finished successfully.\n";

    } catch (const std::exception& ex) {
        std::cerr << "Exception during streaming: " << ex.what() << "\n";
        if (stream) {
           stream->WritesDone().get();
           stream->Finish().get();
        }
        return 1;
    }
    return 0;
}