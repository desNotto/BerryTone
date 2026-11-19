# Berry Tone

A pre-amp-sim conditioner for low-tuned guitar (7/8-string, drop tunings, baritone). VST3 (+ Standalone, + AU on macOS). JUCE 8 / C++17.

## Signal chain (default order - reorderable)
Drag any module card by its header to move it; the other cards shuffle along and the audio order changes instantly (the number badge shows each module's position). **Reset order** restores the default. The order is saved with your session/preset.

1. **Low Freq Tightener** - Cut (high-pass, 20-250 Hz) + Tight (level-dependent low-band squeeze that kills boomy sustained lows)
2. **Horizon Gate** - Thresh / Release / Depth. High-passed sidechain, hysteresis + hold, activity LED
3. **Chug Enhancer** - transient shaper. Punch (+/- attack), Body (+/- sustain), Speed (detector timing)
4. **Low Octave Blender** - analog-style octave-down. Blend / Tone (smooths the sub) / Track (tracker low-pass)
5. **Impedance Curve** - pickup/cable/amp-input resonance. Freq / Peak / Load
6. **Tone Tilt** - Tilt (+/-8 dB) around an adjustable Pivot (200 Hz-3 kHz)
7. **Sound Destroyer** - Drive (tanh + wavefold), Crush (bit + sample-rate reduction), Mix

Each module has its own on/off switch. Input/Output trim + juice-level meters on the last card.

## Latency
Reported latency is **0 samples**: only IIR filters, envelope followers and per-sample nonlinearities. No lookahead, no FFT, no oversampling. Your latency is just your host buffer size.
Trade-offs of that choice: the gate opens in ~0.4 ms (no lookahead), the destroyer aliases on purpose, and the octave tracker works best on single notes / palm-muted riffs.

## Build
Requires CMake 3.22+, a C++17 compiler, and git (JUCE is fetched automatically).
- Windows: Visual Studio 2022
- macOS: Xcode command line tools
- Linux: `sudo apt install build-essential cmake git libasound2-dev libjack-jackd2-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev`

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
The VST3 is copied to your system VST3 folder after the build (COPY_PLUGIN_AFTER_BUILD).
Put Berry Tone before your amp sim in the FX chain. Set Input so peaks sit around -12 to -6 dBFS; the Tightener, Gate and Chug are level-dependent.
