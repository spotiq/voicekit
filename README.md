# VoiceKit: Unified Speech Processing for C++

## About
**VoiceKit** is a C++ library designed to unify **Speech-to-Text (STT) and Text-to-Speech (TTS) services** into a single, efficient, and streaming-friendly framework. It aggregates multiple cloud and on-premise speech APIs, enabling developers to seamlessly switch between different providers while focusing on real-time audio processing.

### Key Features
- 🎙 **STT (Speech-to-Text) Aggregation**: Supports multiple providers with a unified API.
- 🔊 **TTS (Text-to-Speech) Aggregation**: Integrates various voice synthesis engines.
- 🚀 **Streaming First**: Optimized for real-time audio processing.
- 🌎 **Cross-Provider Support**: Microsoft, Google, Amazon Polly, Deepgram, PlayHT, ElevenLabs, and more.
- 🛠 **C++ Core**: Designed for performance-critical applications.

## Installation
```sh
# Clone the repository
git clone https://github.com/SpeechBrainHub/VoiceKit.git
cd VoiceKit

# Build (CMake Example)
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Roadmap
We aim to integrate a wide range of **STT & TTS** providers into VoiceKit. Below is the roadmap:

### ✅ Initial Release (MVP)
- [x] Basic STT support (Microsoft)
- [x] Basic TTS support (Microsoft)
- [x] Streaming audio processing

### 🔜 Upcoming Integrations
- [ ] **Google STT/TTS**
- [ ] **Amazon Polly STT/TTS**
- [ ] **Deepgram STT/TTS**
- [ ] **ElevenLabs TTS**
- [ ] **PlayHT TTS**
- [ ] **IBM Watson STT & TTS**
- [ ] **OpenAI Whisper STT**
- [ ] **Coqui TTS**
- [ ] **On-device/offline STT (Vosk, Kaldi, Whisper.cpp)**
- [ ] **Custom voice models integration**

### 🚀 Future Enhancements
- [ ] Support for **edge devices & embedded systems**
- [ ] GPU acceleration for real-time speech processing
- [ ] Plug-and-play vendor configuration

## Usage Example
```cpp
#include "VoiceKit.h"

VoiceKit vk;
vk.setSTTProvider("google");
vk.setTTSProvider("amazon");

std::string text = vk.transcribe("audio.wav");
vk.synthesize(text, "output.wav");
```

## Contributing
We welcome contributions from the community! Feel free to submit **issues, pull requests, and feature suggestions.**

## License
MIT License. See `LICENSE` file for details.

---

### **Join Us**
💬 Discussions: [GitHub Discussions](https://github.com/SpeechBrainHub/VoiceKit/discussions)  
📢 Updates: [Twitter](https://twitter.com/SpeechBrainHub)  

