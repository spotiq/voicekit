#include "STTFactory.h"
#include "TTSFactory.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>

void TestMicrosoftTTS() {
    const char* key = std::getenv("SPEECH_KEY");
    const char* region = std::getenv("SPEECH_REGION");
    if (!key || !region) {
        std::cerr << "Environment variables MICROSOFT_STT_KEY and MICROSOFT_STT_REGION must be set.\n";
        return;
    }

    std::ofstream outFile("8KHz16BitMonoRAWGeneratedAudio.raw", std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for writing." << "\n";
        return;
    }

    std::shared_ptr<I_TTSModule> tts = TTSFactory::CreateTTSModule("Microsoft", "test_session", [&](const std::vector<uint8_t>& audioData) {
        std::cout << "Audio data size : " << audioData.size() << "\n";
        if (audioData.empty()) {
            std::cout << "Audio data is empty." << "\n";
            outFile.close();
        } else {
            outFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
            std::cout << "Audio data written to 8KHz16BitMonoRAWGeneratedAudio.raw" << "\n";
        }
    }, "en-US-AvaMultilingualNeural");

    tts->Initialise(key, region);

    tts->Speak("Hi! How are you? Hello World!");
    
    std::this_thread::sleep_for(std::chrono::seconds(20));
}


void TestDeepgramTTS() {
    const char* key = std::getenv("DEEPGRAM_SPEECH_KEY");
    const char* region = std::getenv("SPEECH_REGION");
    if (!key || !region) {
        std::cerr << "Environment variables MICROSOFT_STT_KEY and MICROSOFT_STT_REGION must be set.\n";
        return;
    }

    std::ofstream outFile("8KHz16BitMonoRAWGeneratedAudio.raw", std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for writing." << "\n";
        return;
    }

    std::shared_ptr<I_TTSModule> tts = TTSFactory::CreateTTSModule("Deepgram", "test_session", [&](const std::vector<uint8_t>& audioData) {
        std::cout << "Audio data size : " << audioData.size() << "\n";
        if (audioData.empty()) {
            std::cout << "Audio data is empty." << "\n";
            outFile.close();
        } else {
            outFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
            std::cout << "Audio data written to 8KHz16BitMonoRAWGeneratedAudio.raw" << "\n";
        }
    }, "aura-asteria-en");

    tts->Initialise(key, region);

    tts->Speak("Hi! How are you? What is plan for this weekend? Do you want to go outside somewhere near by to bangalore?");
    
    std::this_thread::sleep_for(std::chrono::seconds(20));
}


void TestDeepgramSTT() {
    const char* key = std::getenv("DEEPGRAM_SPEECH_KEY");
    const char* region = std::getenv("SPEECH_REGION");
    if (!key || !region) {
        std::cerr << "Environment variables MICROSOFT_STT_KEY and MICROSOFT_STT_REGION must be set.\n";
        return;
    }

    std::shared_ptr<I_STTModule> stt = STTFactory::CreateSTTModule("Deepgram", "test_session", [&](std::string& text) {
        assert(!text.empty());
        std::cout << "Test Recognized: " << text << "\n";
        stt->StopRecognition();
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

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    std::vector<unsigned char> buffer(320);
    while (audioFile.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
        stt->StreamAudioData(buffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Send remaining data if any
    buffer.resize(audioFile.gcount());
    if (!buffer.empty()) {
        stt->StreamAudioData(buffer);
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
}

void TestMicrosoftSTT() {
    const char* key = std::getenv("SPEECH_KEY");
    const char* region = std::getenv("SPEECH_REGION");
    if (!key || !region) {
        std::cerr << "Environment variables MICROSOFT_STT_KEY and MICROSOFT_STT_REGION must be set.\n";
        return;
    }

    std::shared_ptr<I_STTModule> stt = STTFactory::CreateSTTModule("Microsoft", "test_session", [&](std::string& text) {
        assert(!text.empty());
        std::cout << "Test Recognized: " << text << "\n";
        stt->StopRecognition();
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
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Send remaining data if any
    buffer.resize(audioFile.gcount());
    if (!buffer.empty()) {
        stt->StreamAudioData(buffer);
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
}

void TestElevenlabsTTS() {
    const char* key = std::getenv("ELEVENLABS_SPEECH_KEY");
    const char* region = std::getenv("SPEECH_REGION");
    if (!key || !region) {
        std::cerr << "Environment variables MICROSOFT_STT_KEY and MICROSOFT_STT_REGION must be set.\n";
        return;
    }

    std::ofstream outFile("8KHz16BitMonoRAWGeneratedAudio.raw", std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for writing." << "\n";
        return;
    }

    std::shared_ptr<I_TTSModule> tts = TTSFactory::CreateTTSModule("Elevenlabs", "test_session", [&](const std::vector<uint8_t>& audioData) {
        std::cout << "Audio data size : " << audioData.size() << "\n";
        if (audioData.empty()) {
            std::cout << "Audio data is empty." << "\n";
            outFile.close();
        } else {
            outFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
            std::cout << "Audio data written to 8KHz16BitMonoRAWGeneratedAudio.raw" << "\n";
        }
    }, "Chris");

    tts->Initialise(key, region);

    tts->Speak("Hi! How are you? What is plan for this weekend? Do you want to go outside somewhere near by to bangalore?");
    
    std::this_thread::sleep_for(std::chrono::seconds(20));
}

int main() {
    // TestMicrosoftSTT();
    // TestMicrosoftTTS();
    // TestDeepgramSTT();
    // TestDeepgramTTS();
    TestElevenlabsTTS();
    std::cout << "All tests passed!\n";
    return 0;
}