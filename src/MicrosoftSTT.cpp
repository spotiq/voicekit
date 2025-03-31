#include "MicrosoftSTT.h"


void MicrosoftSTT::InitialiseSTTModule(const std::string& subscriptionKey, const std::string& region) {
    std::shared_ptr<MicrosoftSTT> self = shared_from_this();  // ✅ Now safe to use

    speechConfig = SpeechConfig::FromSubscription(subscriptionKey, region);
    auto audioFormat = AudioStreamFormat::GetWaveFormatPCM(8000, 16, 1);
    pushStream = AudioInputStream::CreatePushStream(audioFormat);
    audioConfig = AudioConfig::FromStreamInput(pushStream);

    speechConfig->SetProperty(PropertyId::SpeechServiceConnection_InitialSilenceTimeoutMs, "10000");
    speechConfig->SetProperty(PropertyId::SpeechServiceConnection_EndSilenceTimeoutMs, "1000");
    speechConfig->SetProperty(PropertyId::Speech_SegmentationSilenceTimeoutMs, "3000");

    recognizer = SpeechRecognizer::FromConfig(speechConfig, audioConfig);
    speechConfig->SetSpeechRecognitionLanguage(language);
    ImplRecognize();
}

void MicrosoftSTT::ImplStreamAudioData(std::vector<uint8_t> audioData)  {
    if (pushStream){
        pushStream->Write(audioData.data(), audioData.size());
    }
}

void MicrosoftSTT::ImplStartRecognition() {
    if (recognizer){
        recognizer->StartContinuousRecognitionAsync().get();
    }
}

void MicrosoftSTT::ImplStopRecognition() {
    if (recognizer){
        recognizer->StopContinuousRecognitionAsync().get();
    }
}

void MicrosoftSTT::ImplRecognize() {
    // Subscribes to events.
    recognizer->Recognizing += [this](const SpeechRecognitionEventArgs& e)
    {
        // Intermediate result (hypothesis).
        if (e.Result->Reason == ResultReason::RecognizingSpeech)
        {
            SPDLOG_INFO("Recognizing: {}", e.Result->Text);
        }
        else if (e.Result->Reason == ResultReason::RecognizingKeyword)
        {
            // ignored
        }
    };

    recognizer->Recognized += [this](const SpeechRecognitionEventArgs& e)
    {
        if (e.Result->Reason == ResultReason::RecognizedKeyword)
        {
            // Keyword detected, speech recognition will start.
            SPDLOG_INFO("[{}] KEYWORD: Text= {}",stream_sid,  e.Result->Text);
        }
        else if (e.Result->Reason == ResultReason::RecognizedSpeech)
        {
            // Final result. May differ from the last intermediate result.
            SPDLOG_INFO("RECOGNIZED: Text= {}", e.Result->Text);
            std::string str_copy = e.Result->Text;
            RecognisedText(str_copy);
        }
        else if (e.Result->Reason == ResultReason::NoMatch)
        {
            // NoMatch occurs when no speech phrase was recognized.
            auto reason = NoMatchDetails::FromResult(e.Result)->Reason;
            switch (reason)
            {
            case NoMatchReason::NotRecognized:
                // Input audio was not silent but contained no recognizable speech.
                SPDLOG_ERROR("[{}] NO MATCH: Reason= NotRecognized",stream_sid);
                break;
            case NoMatchReason::InitialSilenceTimeout:
                // Input audio was silent and the (initial) silence timeout expired.
                // In continuous recognition this can happen multiple times during
                // a session, not just at the very beginning.
                SPDLOG_ERROR("[{}] NO MATCH: Reason= InitialSilenceTimeout",stream_sid);
                break;
            default:
                // Other reasons are not supported in embedded speech at the moment.
                SPDLOG_ERROR("[{}] NO MATCH: Other Reason= {} ",stream_sid,int(reason));
                break;
            }
        }
    };

    recognizer->Canceled += [this](const SpeechRecognitionCanceledEventArgs& e)
    {
        switch (e.Reason)
        {
        case CancellationReason::EndOfStream:
            // Input stream was closed or the end of an input file was reached.
            SPDLOG_ERROR("[{}] CANCELED: EndOfStream",stream_sid);
            break;

        case CancellationReason::Error:
            // NOTE: In case of an error, do not use the same recognizer for recognition anymore.
            SPDLOG_ERROR("[{}] CANCELED: ErrorCode= {} ",stream_sid, int(e.ErrorCode));
            SPDLOG_ERROR("[{}] CANCELED: ErrorDetails= {} ",stream_sid, e.ErrorDetails);
            break;

        default:
            SPDLOG_ERROR("[{}] CANCELED: Reason= {} ",stream_sid, int(e.Reason));
            break;
        }
    };

    recognizer->SessionStarted += [this](const SessionEventArgs& e)
    {
        UNUSED(e);
        SPDLOG_INFO("[{}] Session started.",stream_sid);
    };

    recognizer->SessionStopped += [this](const SessionEventArgs& e)
    {
        UNUSED(e);
        SPDLOG_INFO("[{}] Session stopped.",stream_sid);
    };
}
