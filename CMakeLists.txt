cmake_minimum_required(VERSION 3.16)
project(Dictate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔕 Disable ALL ggml + whisper logging at compile time
add_compile_definitions(GGML_NO_LOGGING WHISPER_NO_LOGGING)

# Add whisper.cpp
add_subdirectory(engine/whisper)

# Include directories
include_directories(
    engine/include
    engine/whisper
    audio
)

# Engine library
add_library(dictate_engine
    engine/src/WhisperEngine.cpp
)

target_link_libraries(dictate_engine
    whisper
)

# macOS frameworks
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
find_library(COREAUDIO_FRAMEWORK CoreAudio)
find_library(FOUNDATION_FRAMEWORK Foundation)
find_library(APP_SERVICES_FRAMEWORK ApplicationServices)

# Main executable
add_executable(dictate
    main.cpp
    audio/AudioRecorder.cpp
)

target_link_libraries(dictate
    dictate_engine
    whisper
    ${AUDIOTOOLBOX_FRAMEWORK}
    ${COREAUDIO_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
    ${APP_SERVICES_FRAMEWORK}
)