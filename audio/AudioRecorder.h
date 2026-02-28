#pragma once
#include <vector>

class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    bool record(std::vector<float>& outSamples);

private:
    static const int SAMPLE_RATE = 16000;
};
