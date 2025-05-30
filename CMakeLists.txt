cmake_minimum_required(VERSION 3.27)
project(stt-library)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define the output directory for built libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include pkg-config support
find_package(PkgConfig REQUIRED)

# Let pkg-config look in the vcpkg pkgconfig path
set(ENV{PKG_CONFIG_PATH} "/usr/local/src/vcpkg/installed/x64-linux/lib/pkgconfig")

find_package(google_cloud_cpp_speech REQUIRED)

# Set paths
set(SPEECHSDK_ROOT "/usr/local/speechsdk")

# === AWS C++ SDK (Polly Only) ===

# Set common options
set(BUILD_ONLY "polly" CACHE STRING "Build only Polly SDK" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Disable tests" FORCE)
set(AWS_NATIVE_BUILD OFF CACHE BOOL "Disable native build" FORCE)
set(ENABLE_UNITY_BUILD OFF CACHE BOOL "Disable unity build" FORCE)

FetchContent_Declare(
  aws-sdk-cpp
  GIT_REPOSITORY https://github.com/aws/aws-sdk-cpp.git
  GIT_TAG 1.11.578
  GIT_SHALLOW TRUE
  GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_MakeAvailable(aws-sdk-cpp)

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
# Enable TLS before building the library
set(USE_TLS ON CACHE BOOL "Enable TLS support for ixwebsocket")

FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.6
)

FetchContent_MakeAvailable(ixwebsocket)

# Include directory for headers
include_directories(
    include
    ${SPEECHSDK_ROOT}/include/cxx_api
    ${SPEECHSDK_ROOT}/include/c_api
)

# Library directories
link_directories(
    ${SPEECHSDK_ROOT}/lib/x64
)

# find_package(google_cloud_cpp_speech REQUIRED)

# # MSVC requires some additional code to select the correct runtime library
# if (VCPKG_TARGET_TRIPLET MATCHES "-static$")
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# else ()
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
# endif ()

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
    src/GoogleSTT.cpp
    src/AmazonTTS.cpp
    src/TranscribeManager.cpp
    src/AmazonSTT.cpp
)

# Create a static library
add_library(stt STATIC ${STT_SOURCES})

# Link pthread to support threading
target_link_libraries(stt
    PRIVATE 
    pthread
    Microsoft.CognitiveServices.Speech.core
    ixwebsocket
    cpr
    google-cloud-cpp::speech
    aws-cpp-sdk-polly
    aws-cpp-sdk-core
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
message(STATUS "To build the library: cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/usr/local/src/vcpkg/scripts/buildsystems/vcpkg.cmake && cd build && make")
message(STATUS "To run tests: cd build && ctest --verbose")
