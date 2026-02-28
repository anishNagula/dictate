#include "AudioRecorder.h"

#include <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <cmath>
#include <atomic>
#include <unistd.h>
#include <cstdio>

struct RecorderState {
    std::vector<float>* samples;
    std::atomic<bool> recording;
};

static void audioCallback(void* userData,
                          AudioQueueRef queue,
                          AudioQueueBufferRef buffer,
                          const AudioTimeStamp*,
                          UInt32,
                          const AudioStreamPacketDescription*) {

    RecorderState* state = static_cast<RecorderState*>(userData);

    int16_t* data = (int16_t*)buffer->mAudioData;
    int count = buffer->mAudioDataByteSize / sizeof(int16_t);

    for (int i = 0; i < count; ++i) {
        state->samples->push_back(data[i] / 32768.0f);
    }

    if (state->recording.load()) {
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }
}

AudioRecorder::AudioRecorder() {}
AudioRecorder::~AudioRecorder() {}

bool AudioRecorder::record(std::vector<float>& outSamples) {

    // 🔥 CRITICAL: disable stdout buffering for real-time AMP streaming
    setvbuf(stdout, NULL, _IONBF, 0);

    outSamples.clear();

    AudioStreamBasicDescription format{};
    format.mSampleRate       = SAMPLE_RATE;
    format.mFormatID         = kAudioFormatLinearPCM;
    format.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel   = 16;
    format.mChannelsPerFrame = 1;
    format.mBytesPerFrame    = 2;
    format.mFramesPerPacket  = 1;
    format.mBytesPerPacket   = 2;

    RecorderState state;
    state.samples = &outSamples;
    state.recording = true;

    AudioQueueRef queue;
    if (AudioQueueNewInput(&format, audioCallback, &state,
                           nullptr, nullptr, 0, &queue) != noErr) {
        return false;
    }

    const int bufferSize = 4096;

    for (int i = 0; i < 3; ++i) {
        AudioQueueBufferRef buffer;
        AudioQueueAllocateBuffer(queue, bufferSize, &buffer);
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }

    AudioQueueStart(queue, nullptr);

    const float SILENCE_THRESHOLD    = 0.015f;
    const int   SILENCE_FRAMES       = SAMPLE_RATE * 1.0;   // 1 second silence
    const int   MAX_FRAMES           = SAMPLE_RATE * 15;    // 15 sec max
    const int   SILENCE_GRACE_FRAMES = SAMPLE_RATE * 3;     // 3 sec grace

    int silentFrames = 0;
    bool speechDetected = false;

    while (true) {

        usleep(10000); // 10ms polling (correct value)

        int total = outSamples.size();

        if (total >= MAX_FRAMES)
            break;

        int window = SAMPLE_RATE / 10; // 100ms window

        if (total < window)
            continue;

        float rms = 0.0f;
        for (int i = total - window; i < total; ++i)
            rms += outSamples[i] * outSamples[i];

        rms = sqrt(rms / window);

        // 🔥 Stream amplitude to overlay
        std::cout << "AMP:" << rms << "\n";

        // Track if user actually spoke
        if (rms > SILENCE_THRESHOLD)
            speechDetected = true;

        // Don't check silence too early
        if (total < SILENCE_GRACE_FRAMES)
            continue;

        if (speechDetected) {
            if (rms < SILENCE_THRESHOLD)
                silentFrames += window;
            else
                silentFrames = 0;

            if (silentFrames >= SILENCE_FRAMES)
                break;
        }
    }

    state.recording = false;

    AudioQueueStop(queue, true);
    AudioQueueDispose(queue, true);

    std::cout << "DONE\n";

    return speechDetected; // prevent blank_audio case
}