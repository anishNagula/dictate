#pragma once
#include <string>
#include <vector>

class WhisperEngine {
public:
    WhisperEngine(const std::string& modelPath);
    ~WhisperEngine();

    bool initialize();
    std::string transcribe(const std::vector<float>& audioSamples);

private:
    std::string modelPath;
    void* ctx; // whisper_context*
};
