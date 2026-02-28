# Dictate

Offline macOS speech-to-text using whisper.cpp (Metal accelerated).

Dictate is a lightweight native macOS speech recognition tool built in C++.  
It records from the microphone, transcribes locally using whisper.cpp with Metal acceleration, and pastes the result into the active application.


## Features

- Fully offline speech recognition
- Metal GPU acceleration
- Quantized tiny model (~28MB)
- Silence detection auto-stop
- System paste injection
- Raycast hotkey integration
- No terminal UI required


## Architecture

Raycast → Native C++ Binary → CoreAudio → Whisper (Metal) → Clipboard → CGEvent Paste


## Requirements

- macOS (Apple Silicon recommended)
- Xcode Command Line Tools
- CMake 3.16+
- Raycast (optional, for hotkey usage)

Install command line tools if needed:

```bash
xcode-select --install
```

## Clone Project
Because this repo uses a Git submodule:
```bash
git clone --recurse-submodules https://github.com/YOUR_USERNAME/dictate.git
cd dictate
```
If already cloned without submodules:
```bash
git submodule update --init --recursive
```

## Build
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

## Download Model
Create model directory:
```bash
mkdir -p ~/.dictate/models
```

Download tiny English model:
```bash
curl -L -o ~/.dictate/models/ggml-tiny.en-q5_0.bin \
https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q5_0.bin
```

## Install Binary
```bash
sudo cp build/dictate /usr/local/bin/dictate
```

Test:
```dictate
```

## First Run Permissions
macOS will request:
- Microphone access
- Accessibility permission (for paste injection)

Grant both in:
`System Settings → Privacy & Security`

## Raycast Integration (Optional)
Create a Script Command:
```bash
#!/bin/bash

/usr/local/bin/dictate > /dev/null 2>&1
```
Assign a hotkey.

Now pressing your hotkey will:
- Start recording
- Stop on silence
- Paste transcription into active app

## Silence Detection
- 5 second grace period before silence detection
- Stops after 1 second of silence
- 15 second max cap

## Performance
- ~30MB model (quantized q5_0)
- ~1 second transcription time for short speech
- Metal backend enabled
- 4 CPU threads + GPU acceleration

## Future Improvements
- Streaming transcription
- Wake word mode
- Background daemon for instant startup
- Multi-language support
- Model auto-download script

## License
MIT
