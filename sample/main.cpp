#include "google/cloud/speech/speech_client.h"
#include "google/cloud/speech/v1/cloud_speech.pb.h"
#include "google/cloud/speech/v1/cloud_speech.grpc.pb.h"

#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace speech = ::google::cloud::speech;
namespace v1 = ::google::cloud::speech::v1;

using RecognizeStream = ::google::cloud::AsyncStreamingReadWriteRpc<
    v1::StreamingRecognizeRequest,
    v1::StreamingRecognizeResponse>;

void StreamAudio(RecognizeStream& stream, const std::string& file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    std::cerr << "Cannot open input file: " << file_path << "\n";
    return;
  }

  constexpr size_t kChunkSize = 320;  // ~20ms for 8kHz 16-bit mono
  std::vector<char> buffer(kChunkSize);
  v1::StreamingRecognizeRequest request;

  while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
    request.set_audio_content(buffer.data(), file.gcount());
    if (!stream.Write(request, grpc::WriteOptions()).get()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));  // simulate real-time
  }

  stream.WritesDone().get();
}

int main(int argc, char* argv[]) try {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <raw_pcm_audio_file>\n";
    return 1;
  }

  std::string audio_path = argv[1];
  auto client = speech::SpeechClient(speech::MakeSpeechConnection());

  v1::StreamingRecognizeRequest config_request;
  auto* streaming_config = config_request.mutable_streaming_config();
  auto* rec_config = streaming_config->mutable_config();

  rec_config->set_language_code("hi-IN");  // Hindi - returns Devanagari script
  rec_config->set_sample_rate_hertz(8000);
  rec_config->set_encoding(v1::RecognitionConfig::LINEAR16);
  rec_config->set_model("default");
  rec_config->set_enable_automatic_punctuation(true);

  auto stream = client.AsyncStreamingRecognize();

  if (!stream->Start().get()) throw stream->Finish().get();
  if (!stream->Write(config_request, grpc::WriteOptions{}).get())
    throw stream->Finish().get();

  std::thread mic_thread(StreamAudio, std::ref(*stream), audio_path);

  for (auto response = stream->Read().get(); response.has_value();
       response = stream->Read().get()) {
    for (const auto& result : response->results()) {
      if (result.is_final()) {
        for (const auto& alt : result.alternatives()) {
          std::cout << "Transcript (confidence " << alt.confidence() << "): "
                    << alt.transcript() << "\n";
        }
      }
    }
  }

  auto status = stream->Finish().get();
  mic_thread.join();
  if (!status.ok()) throw status;

  return 0;
} catch (const google::cloud::Status& s) {
  std::cerr << "Google Cloud error: " << s << "\n";
  return 1;
} catch (const std::exception& e) {
  std::cerr << "Exception: " << e.what() << "\n";
  return 1;
}
