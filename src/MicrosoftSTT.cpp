#include "MicrosoftSTT.h"

void MicrosoftSTT::InitialiseSTTModule(const std::string& subscriptionKey, const std::string& region) {
    std::cout << "Initializing Microsoft STT with Key: " << subscriptionKey << " Region: " << region << "\n";
}

void MicrosoftSTT::StartRecognition() {
    std::cout << "Starting Microsoft STT Recognition\n";
}

void MicrosoftSTT::StopRecognition() {
    std::cout << "Stopping Microsoft STT Recognition\n";
}

void MicrosoftSTT::Recognize(const std::vector<uint8_t>& audioData) {
    std::string text = "Recognized text from Microsoft";
    callback(text);
}
