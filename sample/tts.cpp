#include <google/cloud/texttospeech/v1/text_to_speech_client.h>
#include <iostream>
#include <fstream>


namespace gc = ::google::cloud;
namespace tts = ::google::cloud::texttospeech_v1;

int main() {
    // Create a TTS client
    auto client = tts::TextToSpeechClient(tts::MakeTextToSpeechConnection());

    // Prepare request
    google::cloud::texttospeech::v1::SynthesizeSpeechRequest request;

    // Text input
    request.mutable_input()->set_text("नमस्ते! आप कैसे हैं?");

    // Voice selection (Hindi, India)
    auto* voice = request.mutable_voice();
    voice->set_language_code("hi-IN");
    voice->set_name("hi-IN-Wavenet-A");  // Optional: pick specific voice
    voice->set_ssml_gender(google::cloud::texttospeech::v1::SsmlVoiceGender::FEMALE);

    // Audio config
    auto* audio_config = request.mutable_audio_config();
    audio_config->set_audio_encoding(google::cloud::texttospeech::v1::LINEAR16);
    audio_config->set_sample_rate_hertz(8000);

    // Call the API
    auto response = client.SynthesizeSpeech(request);

    if (!response) {
        std::cerr << "Error synthesizing speech: " << response.status() << "\n";
        return 1;
    }

    // Save to file
    std::ofstream output("output_audio.wav", std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "Failed to open output.wav for writing\n";
        return 1;
    }
    output << response->audio_content();
    output.close();

    std::cout << "Audio content written to 'output_audio.wav'\n";

    {
        auto constexpr kText = R"""(
        Four score and seven years ago our fathers brought forth on this
        continent, a new nation, conceived in Liberty, and dedicated to
        the proposition that all men are created equal.)""";

        namespace texttospeech = ::google::cloud::texttospeech_v1;

        auto client = texttospeech::TextToSpeechClient(
        texttospeech::MakeTextToSpeechConnection());

        google::cloud::texttospeech::v1::SynthesisInput input;
        input.set_text(kText);

        google::cloud::texttospeech::v1::VoiceSelectionParams voice;
        voice.set_language_code("en-US");

        google::cloud::texttospeech::v1::AudioConfig audio;
        audio.set_audio_encoding(google::cloud::texttospeech::v1::LINEAR16);

        auto response = client.SynthesizeSpeech(input, voice, audio);
        if (!response) throw std::move(response).status();

        auto constexpr kWavHeaderSize = 48;
        auto constexpr kBytesPerSample = 2;  // LINEAR16 = 16-bit
        auto const sample_count =
        (response->audio_content().size() - kWavHeaderSize) / kBytesPerSample;
        std::cout << "The audio has " << sample_count << " samples\n";

        std::ofstream out2("output_audio2.wav", std::ios::binary);
        if (!out2.is_open()) {
            std::cerr << "Failed to open output.wav for writing\n";
            return 1;
        }
        out2 << response->audio_content();
        out2.close();
        
        std::cout << "Audio content written to 'output_audio2.wav'\n";
    }
    return 0;
}
