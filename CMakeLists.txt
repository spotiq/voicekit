cmake_minimum_required(VERSION 3.10)
project(stt-library)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define the output directory for built libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set paths
set(SPEECHSDK_ROOT "$ENV{HOME}/speechsdk")

# === Download and build cpr ===

# Enable SSL support in CPR
set(CPR_ENABLE_SSL ON CACHE BOOL "Enable SSL for CPR")
set(CPR_FORCE_USE_SYSTEM_CURL ON CACHE BOOL "Use system-installed libcurl")

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG master
)

FetchContent_MakeAvailable(cpr)

# === Download and build IXWebSocket ===
FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG master
)

FetchContent_MakeAvailable(ixwebsocket)

# Include directory for headers
include_directories(
    include
    /usr/include
    ${SPEECHSDK_ROOT}/include/cxx_api
    ${SPEECHSDK_ROOT}/include/c_api
)

# Library directories
link_directories(
    /usr/local/lib
    ${SPEECHSDK_ROOT}/lib/x64
)

# Add library sources
set(STT_SOURCES
    src/STTFactory.cpp
    src/STTModuleBase.cpp
    src/MicrosoftSTT.cpp
    src/TTSFactory.cpp
    src/TTSModuleBase.cpp
    src/MicrosoftTTS.cpp
    src/TTSCache.cpp
    src/DeepgramSTT.cpp
    src/DeepgramTTS.cpp
    src/ElevenlabsTTS.cpp
)

# Create a static library
add_library(stt STATIC ${STT_SOURCES})

# Link pthread to support threading
target_link_libraries(stt
    PRIVATE 
    pthread
    Microsoft.CognitiveServices.Speech.core
    crypto
    ixwebsocket
    z
    ssl
    cpr
)

# Enable testing
enable_testing()

# Add test executable
add_executable(test_stt test/test_stt.cpp)
target_link_libraries(test_stt PRIVATE stt pthread)  # Ensure test executable links pthread

# Define a test
add_test(NAME STT_Test COMMAND test_stt)

# Install the library and headers
install(TARGETS stt
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# Build Instructions
message(STATUS "To build the library: mkdir -p build && cd build && cmake .. && make")
message(STATUS "To run tests: cd build && ctest --verbose")
