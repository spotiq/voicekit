#include "STTFactory.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>

void TestMicrosoftSTT() {
    const char* key = std::getenv("SPEECH_KEY");
    const char* region = std::getenv("SPEECH_REGION");
    if (!key || !region) {
        std::cerr << "Environment variables MICROSOFT_STT_KEY and MICROSOFT_STT_REGION must be set.\n";
        return;
    }

    auto stt = STTFactory::CreateSTTModule("Microsoft", "test_session", [](std::string& text) {
        assert(!text.empty());
        std::cout << "Test Recognized: " << text << "\n";
    }, "en-US");

    std::cout << "InitialiseSTTModule" << "\n";

    stt->InitialiseSTTModule(key, region);

    std::cout << "StartRecognition" << "\n";

    stt->StartRecognition();

    std::cout << "audioFile " << "\n";

    // This is hello world test with counting from 1 to 10. 1 2 3 4 5 6 7 8 9 10. 
    std::ifstream audioFile("8KHz16BitMonoRawAudioSample.raw", std::ios::binary);
    if (!audioFile) {
        std::cerr << "Failed to open audio file.\n";
        return;
    }

    std::vector<unsigned char> buffer(320);
    while (audioFile.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
        stt->StreamAudioData(buffer);
        std::this_thread::sleep_for(std::chrono::microseconds(20));
    }
    
    // Send remaining data if any
    buffer.resize(audioFile.gcount());
    if (!buffer.empty()) {
        stt->StreamAudioData(buffer);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    stt->StopRecognition();
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

int main() {
    TestMicrosoftSTT();
    std::cout << "All tests passed!\n";
    return 0;
}