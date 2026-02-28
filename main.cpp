#include "WhisperEngine.h"
#include "AudioRecorder.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

void pasteText(const std::string& text) {
    PasteboardRef pb;
    PasteboardCreate(kPasteboardClipboard, &pb);
    PasteboardClear(pb);

    CFDataRef data = CFDataCreate(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(text.c_str()),
        text.size()
    );

    PasteboardPutItemFlavor(
        pb,
        (PasteboardItemID)1,
        CFSTR("public.utf8-plain-text"),
        data,
        0
    );

    CFRelease(data);

    // Simulate Cmd+V
    CGEventRef cmdDown = CGEventCreateKeyboardEvent(NULL, 0x37, true);
    CGEventRef vDown   = CGEventCreateKeyboardEvent(NULL, 0x09, true);
    CGEventSetFlags(vDown, kCGEventFlagMaskCommand);

    CGEventRef vUp     = CGEventCreateKeyboardEvent(NULL, 0x09, false);
    CGEventSetFlags(vUp, kCGEventFlagMaskCommand);

    CGEventRef cmdUp   = CGEventCreateKeyboardEvent(NULL, 0x37, false);

    CGEventPost(kCGHIDEventTap, cmdDown);
    CGEventPost(kCGHIDEventTap, vDown);
    CGEventPost(kCGHIDEventTap, vUp);
    CGEventPost(kCGHIDEventTap, cmdUp);

    CFRelease(cmdDown);
    CFRelease(vDown);
    CFRelease(vUp);
    CFRelease(cmdUp);
    CFRelease(pb);
}

int main() {
    std::string modelPath =
        std::string(getenv("HOME")) +
        "/.dictate/models/ggml-tiny.en-q5_0.bin";

    WhisperEngine engine(modelPath);
    if (!engine.initialize()) {
        std::cerr << "Engine failed\n";
        return 1;
    }

    AudioRecorder recorder;
    std::vector<float> audio;

    if (!recorder.record(audio)) {
        std::cerr << "Recording failed\n";
        return 1;
    }

    std::cout << "Transcribing...\n";
    std::string text = engine.transcribe(audio);

    std::cout << "Pasting...\n";
    pasteText(text);

    return 0;
}
