cmake_minimum_required(VERSION 3.14)
project(google_stt_streaming)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(google_cloud_cpp_speech REQUIRED)
find_package(google_cloud_cpp_texttospeech REQUIRED)

add_executable(google_stt_stream main.cpp)
add_executable(google_tts tts.cpp)

target_link_libraries(google_stt_stream
  google-cloud-cpp::speech
)

target_link_libraries(google_tts
  google-cloud-cpp::texttospeech
)
