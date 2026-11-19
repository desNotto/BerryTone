# BerryTone

A pre-amp-sim conditioner for low-tuned guitar (7/8-string, drop tunings, baritone). VST3 (+ Standalone, + AU on macOS). JUCE 8 / C++17.
Author: **desNotto**.

## What is new in 1.1.0
- **DI Scope.** A window above the modules shows your signal in real time.
  - **REAL DI:** the grey line is the raw DI. The coloured line is the signal after the modules. One screen = 1 second, auto level.
  - **FLAT LINE:** a test signal with a flat spectrum goes through the chain. The modules bend the flat line (gain in dB, 20 Hz - 20 kHz).
  - **HOLD** freezes the screen.
- **PURE DI button.** It mutes all modules (and the trims). You hear and see the raw DI. Click again to go back. It does not change your settings.
- **Two designs.** Click the logo. **Berry** (juice box) changes to **Grape** (dark, from the UI sheet) and back.
- **Presets.** Use the bar at the top: previous / next, the list, Save, Save as, Rename, Delete.
  - A preset stores all knobs, switches, the module order and the design.
  - Factory preset **Berry Base** is read only. A new instance starts with it.
  - User presets are files in `%APPDATA%\desNotto\BerryTone\Presets` (`*.btpreset`). The menu has "Open presets folder".
- **Updates from GitHub.** The plugin reads the latest release of this repository. When a newer version exists, the button "Update to vX" shows at the bottom left. Install downloads the zip and replaces the plugin files (Windows asks for admin rights when the folder needs them). Restart the DAW after the install.
- **Low Freq Tightener:** the Tight range is now 0-300% (was 0-100%).

## Signal chain (default order - reorderable)
Drag any module card by its header to move it; the other cards shuffle along and the audio order changes instantly (the number badge shows each module's position). **Reset order** restores the default. The order is saved with your session/preset.

1. **Low Freq Tightener** - Cut (high-pass, 20-250 Hz) + Tight (0-300%, level-dependent low-band squeeze that kills boomy sustained lows)
2. **Horizon Gate** - Thresh / Release / Depth. High-passed sidechain, hysteresis + hold, activity LED
3. **Chug Enhancer** - transient shaper. Punch (+/- attack), Body (+/- sustain), Speed (detector timing)
4. **Low Octave Blender** - analog-style octave-down. Blend / Tone (smooths the sub) / Track (tracker low-pass)
5. **Impedance Curve** - pickup/cable/amp-input resonance. Freq / Peak / Load
6. **Tone Tilt** - Tilt (+/-8 dB) around an adjustable Pivot (200 Hz-3 kHz)
7. **Sound Destroyer** - Drive (tanh + wavefold), Crush (bit + sample-rate reduction), Mix

Each module has its own on/off switch. Input/Output trim + level meters on the last card.

## Latency
Reported latency is **0 samples**: only IIR filters, envelope followers and per-sample nonlinearities. No lookahead, no FFT, no oversampling in the audio path. (The FLAT LINE curve uses an FFT on the UI thread only.)

## Build
Requires CMake 3.22+, a C++17 compiler, and git (JUCE is fetched automatically).
- Windows: Visual Studio 2022 (or Build Tools 2022)
- macOS: Xcode command line tools

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
The build does not copy the plugin into the system folder. Copy `build/BerryTone_artefacts/Release/VST3/BerryTone.vst3` to `C:\Program Files\Common Files\VST3`, or use a release zip.

## Release
`tools/make_release.ps1` builds the plugin and puts `BerryTone-v<version>` (VST3, Standalone, zip) into the `Releases` folder next to the repository.
Add `-Publish` to make the git tag and the GitHub release. The zip name must end with `-win64.zip`: the updater looks for it.
Change the version in `CMakeLists.txt` (`BERRY_VERSION`) before a new release.

Put BerryTone before your amp sim in the FX chain. Set Input so peaks sit around -12 to -6 dBFS; the Tightener, Gate and Chug are level-dependent.
