#include "WhisperEngine.h"
#include "whisper.h"
#include "ggml.h"

#include <iostream>
#include <unistd.h>
#include <cstdio>

WhisperEngine::WhisperEngine(const std::string& modelPath)
    : modelPath(modelPath), ctx(nullptr) {}

WhisperEngine::~WhisperEngine() {
    if (ctx) {
        whisper_free((whisper_context*)ctx);
    }
}

bool WhisperEngine::initialize() {
    whisper_log_set([](enum ggml_log_level, const char *, void *) {}, nullptr);
    // --- Temporarily suppress stdout ---
    fflush(stdout);
    int original_stdout = dup(fileno(stdout));
    FILE* devnull = fopen("/dev/null", "w");
    dup2(fileno(devnull), fileno(stdout));

    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = true;
    params.flash_attn = true;

    ctx = whisper_init_from_file_with_params(modelPath.c_str(), params);

    // --- Restore stdout ---
    fflush(stdout);
    dup2(original_stdout, fileno(stdout));
    close(original_stdout);
    fclose(devnull);

    if (!ctx) {
        std::cerr << "Failed to load model\n";
        return false;
    }

    return true;
}

std::string WhisperEngine::transcribe(const std::vector<float>& audioSamples) {
    if (!ctx) return "";

    whisper_full_params params =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    params.n_threads = 4;
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.translate = false;
    params.language = "en";

    if (whisper_full((whisper_context*)ctx,
                     params,
                     audioSamples.data(),
                     audioSamples.size()) != 0) {
        std::cerr << "Transcription failed\n";
        return "";
    }

    std::string result;
    int n = whisper_full_n_segments((whisper_context*)ctx);

    for (int i = 0; i < n; ++i) {
        result += whisper_full_get_segment_text(
            (whisper_context*)ctx, i);
    }

    return result;
}