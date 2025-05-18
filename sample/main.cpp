// main.cpp
#include <google/cloud/speech/v2/speech_client.h>
#include <google/cloud/status.h>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

namespace gc = ::google::cloud;
namespace speech = ::google::cloud::speech_v2;

// Helper alias for the bidirectional streaming call
using RecognizeStream = gc::AsyncStreamingReadWriteRpc<
   google::cloud::speech::v2::StreamingRecognizeRequest,
   google::cloud::speech::v2::StreamingRecognizeResponse>;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <raw_pcm_8kHz_16bit_mono_file>\n";
        return 1;
    }

    std::string const filename = argv[1];

    // 1) Create the SpeechClient (uses GOOGLE_APPLICATION_CREDENTIALS)
    auto client = speech::SpeechClient(speech::MakeSpeechConnection());

    // 2) Launch streaming recognize
    auto stream = client.AsyncStreamingRecognize();
    if (!stream) {
        std::cerr << "Failed to create streaming recognize stream\n";
        return 1;
    }

    // 3) Send the initial config message
    google::cloud::speech::v2::StreamingRecognizeRequest config_req;
    {
        auto* cfg = config_req.mutable_streaming_config()->mutable_config();
        auto* explicit_decoding_config = cfg->mutable_explicit_decoding_config();

        
        config_req.mutable_recognizer()->append("projects/814354973424/locations/global/recognizers/_");

        cfg->add_language_codes("hi-IN");                       // Hindi
        cfg->set_model("latest_long");                          // long-form model

      auto* streaming_cfg = config_req.mutable_streaming_config();
      streaming_cfg->mutable_streaming_features()->enable_voice_activity_events();
      streaming_cfg->mutable_streaming_features()->set_interim_results(true);         // ✅ Set here

        // *cfg->mutable_auto_decoding_config() =               // auto-detect PCM
        //     google::cloud::speech::v2::AutoDetectDecodingConfig{};
        explicit_decoding_config->set_encoding(google::cloud::speech::v2::ExplicitDecodingConfig_AudioEncoding::ExplicitDecodingConfig_AudioEncoding_LINEAR16);
        explicit_decoding_config->set_sample_rate_hertz(8000);
        explicit_decoding_config->set_audio_channel_count(1);
        // cfg->clear_auto_decoding_config();

        cfg->features().enable_automatic_punctuation();
    }

    // Start the RPC
    if (!stream->Start().get()) {
        std::cerr << "Stream failed to start\n";
        return 1;
    }
    // Write config
    if (!stream->Write(config_req, grpc::WriteOptions{}).get()) {
        std::cerr << "Failed to write initial config\n";
        return 1;
    }

    // 4) Spawn a thread to read server responses
    std::atomic<bool> finished{false};
    std::thread reader([&] {
        while (true) {
            auto resp = stream->Read().get();
            if (!resp.has_value()) break;  // stream closed

            for (auto const& result : resp->results()) {
                if (result.is_final()){
                  std::cout << "[Final] ";
                }else{
                  std::cout << "[Interim] ";
                }
                for (auto const& alt : result.alternatives()) {
                    std::cout << alt.transcript() << std::endl;
                }
            }
        }
        // Get final status
        auto status = stream->Finish().get();
        if (!status.ok()) {
            std::cerr << "Stream finished with error: "
                      << status.code() << ": " << status.message() << "\n";
        }
        finished = true;
    });

    // 5) Read raw PCM file and send in 100 ms chunks (1600 bytes)
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file: " << filename << "\n";
        return 1;
    }
    constexpr std::size_t kChunkSize = 1600;  // 0.1 s @ 8 kHz × 2 bytes/sample
    std::vector<char> buffer(kChunkSize);

    while (in) {
        in.read(buffer.data(), buffer.size());
        std::streamsize n = in.gcount();
        if (n <= 0) break;

        google::cloud::speech::v2::StreamingRecognizeRequest audio_req;
        audio_req.mutable_audio()->append(buffer.data(), static_cast<size_t>(n));

        if (!stream->Write(audio_req, grpc::WriteOptions{}).get()) {
            std::cerr << "Failed to write audio chunk\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 6) Signal end of writes, then wait for reader to finish
    stream->WritesDone().get();
    reader.join();
    if (!finished) {
        // If the reader thread didn’t see Finish(), ensure we call it
        auto status = stream->Finish().get();
        if (!status.ok()) {
            std::cerr << "Finish() error: " << status.message() << "\n";
        }
    }
    return 0;
}
