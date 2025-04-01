#include "MicrosoftTTS.h"
#include <iostream>

bool MicrosoftTTS::Initialise(const std::string& apiKey, const std::string& region) {
    // Create speech configuration
    speechConfig = SpeechConfig::FromSubscription(apiKey, region);
    speechConfig->SetSpeechSynthesisVoiceName(m_voiceName);
    speechConfig->SetSpeechSynthesisOutputFormat(SpeechSynthesisOutputFormat::Raw8Khz16BitMonoPcm);
    synthesizer = SpeechSynthesizer::FromConfig(speechConfig);
    return true;
}

void MicrosoftTTS::ImplSynthesiseVoice(const std::string& text, const std::string& hashKey) {
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = synthesizer->SpeakTextAsync(text).get();
    
    if (result->Reason == ResultReason::SynthesizingAudioStarted) {
        SPDLOG_INFO( "[{}] TTS SynthesizingAudioStarted",stream_sid);
    }   
    else if (result->Reason == ResultReason::SynthesizingAudio) {
        SPDLOG_INFO( "[{}] TTS SynthesizingAudio",stream_sid);
    }
    else if (result->Reason == ResultReason::SynthesizingAudioCompleted) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto ttsLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        SPDLOG_INFO( "[{}] TTS SynthesizingAudioCompleted Latency: {} ms for text: {}" ,stream_sid, ttsLatency, text);

        // Get the audio data stream
        auto audioDataStream = AudioDataStream::FromResult(result);
        audioDataStream->SetPosition(0); // Reset stream position

        std::vector<uint8_t> audioBuffer;

        std::vector<uint8_t> tempBuffer(1024);

        uint32_t readBytes = 0;
        uint32_t totalBytes = 0;

        // Read data into memory
        while ((readBytes = audioDataStream->ReadData(tempBuffer.data(), static_cast<uint32_t>(tempBuffer.size()))) > 0) {
            totalBytes += readBytes;
            audioBuffer.insert(audioBuffer.end(), tempBuffer.begin(), tempBuffer.begin() + readBytes);
        }
        
        SynthesisedAudioData(audioBuffer,hashKey,ttsLatency);
    } 
    else if (result->Reason == ResultReason::Canceled) {
        auto cancellation = SpeechSynthesisCancellationDetails::FromResult(result);
        SPDLOG_ERROR( "[{}] Synthesis CANCELED: Reason= {}",stream_sid , static_cast<int>(cancellation->Reason) );

        if (cancellation->Reason == CancellationReason::Error) {
            SPDLOG_ERROR( "[{}] ErrorCode= {}",stream_sid , static_cast<int>(cancellation->ErrorCode) );
            SPDLOG_ERROR( "[{}] ErrorDetails= {}",stream_sid , cancellation->ErrorDetails );
        }
    }    

}
