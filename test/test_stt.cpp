#include "STTFactory.h"
#include <iostream>
#include <cassert>

void TestMicrosoftSTT() {
    auto stt = STTFactory::CreateSTTModule("Microsoft", "test_session", [](std::string& text) {
        assert(!text.empty());
        std::cout << "Test Recognized: " << text << "\n";
    }, "en-US");

    stt->InitialiseSTTModule("test-key", "test-region");
    stt->StartRecognition();
    stt->StreamAudioData({0x01, 0x02, 0x03});
    stt->StopRecognition();
}

int main() {
    TestMicrosoftSTT();
    std::cout << "All tests passed!\n";
    return 0;
}
