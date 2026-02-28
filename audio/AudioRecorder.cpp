#include "AudioRecorder.h"

#include <AudioToolbox/AudioToolbox.h>
#include <iostream>
#include <cmath>
#include <atomic>
#include <unistd.h>

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
        std::cerr << "Failed to create AudioQueue\n";
        return false;
    }

    const int bufferSize = 4096;

    for (int i = 0; i < 3; ++i) {
        AudioQueueBufferRef buffer;
        AudioQueueAllocateBuffer(queue, bufferSize, &buffer);
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }

    // Start beep (reliable)
    system("afplay /System/Library/Sounds/Pop.aiff &");

    std::cout << "Recording... Speak now.\n";
    AudioQueueStart(queue, nullptr);

    const float SILENCE_THRESHOLD = 0.015f;      // slightly higher for stability
    const int   SILENCE_FRAMES    = SAMPLE_RATE * 1.0;  // 1 second silence
    const int   MAX_FRAMES        = SAMPLE_RATE * 15;   // 15 second cap
    const int   MIN_FRAMES        = SAMPLE_RATE * 0.5;  // 500ms minimum recording
    const int SILENCE_GRACE_FRAMES = SAMPLE_RATE * 5;  // 5 seconds before silence detection

    int silentFrames = 0;

    while (true) {
        usleep(10000); // 10ms polling
    
        int total = outSamples.size();
    
        if (total >= MAX_FRAMES)
            break;
    
        // 🔒 Do not check silence for first 5 seconds
        if (total < SILENCE_GRACE_FRAMES)
            continue;
    
        int window = SAMPLE_RATE / 10; // 100ms window
    
        if (total < window)
            continue;
    
        float rms = 0.0f;
        for (int i = total - window; i < total; ++i)
            rms += outSamples[i] * outSamples[i];
    
        rms = sqrt(rms / window);
    
        if (rms < SILENCE_THRESHOLD)
            silentFrames += window;
        else
            silentFrames = 0;
    
        if (silentFrames >= SILENCE_FRAMES)
            break;
    }

    state.recording = false;

    AudioQueueStop(queue, true);
    AudioQueueDispose(queue, true);

    // Stop beep
    system("afplay /System/Library/Sounds/Glass.aiff &");

    std::cout << "Recording finished.\n";

    return !outSamples.empty();
}