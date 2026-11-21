# BerryTone

BerryTone is a signal conditioner for low-tuned electric guitar (7, 8 and 9 strings, drop tunings, baritone).
Put it before your amp simulator. It makes the DI signal tight and clear before the amp gets it.

- Format: VST3 for Windows (AU for macOS when you build it on a Mac). There is no Standalone version.
- Latency: 0 samples.
- Author: **desNotto**.

## Install
1. Download `BerryTone-v<version>-win64.zip` from the [Releases](https://github.com/desNotto/BerryTone/releases) page.
2. Copy the file `BerryTone.vst3` to `C:\Program Files\Common Files\VST3`.
3. Start your DAW. Scan the plugins again.

`BerryTone.vst3` is one file, not a folder. Some DAWs do not find the plugin when it is a folder.
For the next versions, use the update button in the plugin (see [Updates](#updates)).

## Quick start
1. Put BerryTone on the guitar track, before the amp simulator.
2. Play hard. Set **Input** so that the peaks are between -12 and -6 dBFS. The Tightener, Gate and Chug modules use the signal level, so this step is important.
3. A new BerryTone starts with the preset **Init**. In Init, all modules are off.
4. Turn on one module at a time. Listen to the change. Look at the change on the DI Scope.
5. Click **PURE DI** to compare with the raw DI.

## The window
- **Top left:** the logo. Click it to change the design (Berry or Grape).
- **Top right:** the preset bar.
- **Middle:** the DI Scope.
- **Bottom:** eight cards. Seven cards are modules. The last card is **Juice Level** (Input, Output, meters).
- **Footer:** the hint line (with the version or the update button at its right end), the hint language flags, the window size buttons and **Reset order**.

Each module card has:
- a number. The number is the position of the module in the signal chain.
- an on/off switch. When the module is off, its knobs are dim and the module does not change the signal.
- two or three knobs. Drag a knob up or down. Double-click a knob to set the default value. Click the value under the knob to type a number.

## Hint line
The hint line is at the bottom of the window.
- Put the mouse on a module card (the header or any free area). The line shows the module name and what the module does.
- Put the mouse on a knob. The line shows the knob name and what the knob does: the type of the block, the range and the result of a higher or lower value.
- When you turn a knob, the line keeps the hint of this knob.
- When the mouse is not on a module, the line shows a general hint.

The flags **RU / EN** at the left of the window size buttons set the language of the hints. They change the hint texts only. The default language is Russian. The plugin saves the choice for all instances in `%APPDATA%\desNotto\BerryTone\settings.xml`.

## DI Scope
The DI Scope shows the signal before and after the modules.
Select a mode with the three buttons at the right of the screen.

### REAL DI
- The grey line is the raw DI.
- The coloured line (OUT) is the signal after all modules and the Output knob.
- **TIME** sets the time window:
  - **1 s** (100 ms/div): you see the notes and their level.
  - **200 ms** (20 ms/div): you see the attack and the decay of one note.
  - **50 ms** (5 ms/div): you see the shape of the wave. A trigger holds the wave still.
- **LEVEL** sets the vertical scale:
  - **LIN**: linear scale. The highest peak fills the screen (AUTO).
  - **dB**: 0 dBFS at the top and the bottom, -60 dBFS at the centre line. You see the quiet parts of the signal, for example the note tails and the gate.

### SPECTRUM
- The grey area is the spectrum of the raw DI.
- The coloured line is the spectrum of OUT.
- The scale is 20 Hz to 20 kHz and 0 to -72 dBFS.
- Use this mode to see the tone changes on your real guitar signal.

### FLAT LINE
- A test signal with a flat spectrum goes through a copy of the chain.
- The line shows the gain of the chain at each frequency (+18 to -24 dB).
- The line does not show the level-dependent and time-dependent modules well (Gate, Chug, Tight). Use REAL DI for them.

### HOLD
- HOLD freezes the DI. The OUT line still follows the modules.
- Turn a knob when HOLD is on. The OUT line changes at once on the same DI.
- HOLD works in REAL DI and in SPECTRUM. You can change TIME and LEVEL when HOLD is on.
- Click HOLD again to see the live signal.

### PURE DI
- PURE DI mutes all modules and the Input and Output knobs.
- You hear the raw DI. The OUT line is the same as the DI line.
- PURE DI does not change your settings. Click it again to go back.

### Which mode shows which knob
| Knob | Best mode |
|---|---|
| Input, Output, Octave Blend, Destroyer Drive, Destroyer Mix | REAL DI, 1 s, LIN |
| Chug Punch, Chug Body, Chug Speed | REAL DI, 200 ms |
| Tightener Tight | REAL DI, 200 ms, or SPECTRUM |
| Gate Thresh, Gate Release, Gate Depth | REAL DI, 1 s, dB |
| Tightener Cut, Tilt, Pivot, Octave Tone, Octave Track | SPECTRUM or FLAT LINE |
| Impedance Freq, Peak, Load | SPECTRUM, FLAT LINE, or REAL DI 50 ms |
| Destroyer Crush | REAL DI, 50 ms |

## Modules
The default order of the modules is 1 to 7. You can change the order (see [Module order](#module-order)).
The "Default" column shows the value after a double-click on the knob.

### 1. Low Freq Tightener ("tart cherry")
This module removes boom from the low end. Low tunings make a lot of energy below 160 Hz. Too much low energy makes the amp sound loose.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Cut** | 20 - 250 Hz | 60 Hz | A high-pass filter (12 dB/octave). It removes the frequencies below this value. |
| **Tight** | 0 - 300 % | 35 % | A low-band compressor below 160 Hz. When the low band is loud, Tight makes it quieter. A high value keeps the low end tight on long, palm-muted notes. At 0 %, Tight does nothing. |

- Start with Cut at 60 - 100 Hz for 8 and 9 strings.
- Increase Tight until the low notes are tight on long palm mutes.

### 2. Horizon Gate ("blueberry")
This module is a noise gate. It makes the signal quiet between the notes. The gate detector uses a high-pass filter at 110 Hz. Thus the low strings and the hum do not keep the gate open.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Thresh** | -80 - -20 dB | -55 dB | The gate opens when the signal is above this level. The gate closes 4 dB below this level. |
| **Release** | 10 - 500 ms | 80 ms | The time the gate needs to close. A short time gives hard stops. A long time gives soft tails. |
| **Depth** | -80 - -6 dB | -70 dB | How much the gate decreases the signal when it is closed. -80 dB is almost silence. -6 dB is a small decrease. |

- The LED next to the switch is on when the gate is open.
- The gate opens in about 0.4 ms. It stays open for 12 ms after the signal goes down.
- Set Thresh just above the noise of your guitar. Use REAL DI with LEVEL dB to see the noise level.

### 3. Chug Enhancer ("blackberry")
This module is a transient shaper. It changes the attack and the body of each note. The detector uses 90 Hz - 3 kHz only, so pick noise and low rumble do not trigger it.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Punch** | -100 - +100 % | +30 % | Changes the attack. A positive value makes the attack louder (up to +12 dB). A negative value makes the attack softer. |
| **Body** | -100 - +100 % | 0 % | Changes the part after the attack. A positive value makes the note longer. A negative value makes the note short (staccato chugs). |
| **Speed** | 0.50 - 2.00x | 1.00x | The speed of the detector. A high value reacts to short attacks only. A low value reacts to slower attacks too. |

### 4. Low Octave Blender ("concord grape")
This module adds a sub-octave (one octave down) to the signal, like an analog octave pedal. It works best on single notes and palm-muted riffs. Chords can make the sub-octave unstable.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Blend** | 0 - 100 % | 30 % | The level of the sub-octave. |
| **Tone** | 60 - 600 Hz | 180 Hz | A low-pass filter on the sub-octave. A low value makes the sub-octave round and soft. |
| **Track** | 120 - 800 Hz | 300 Hz | A low-pass filter on the signal that the octave detector uses. A low value gives stable tracking on low notes. A high value tracks higher notes. |

### 5. Impedance Curve ("boysenberry")
This module copies the high-frequency resonance of a passive pickup, a guitar cable and an amp input.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Freq** | 1.5 - 10 kHz | 4.5 kHz | The resonance frequency. Above this frequency, the highs go down. |
| **Peak** | 0 - 100 % | 40 % | The height of the resonance peak. A high value gives a bright, focused bump at Freq. |
| **Load** | 0 - 100 % | 70 % | How much of the curve the module applies. At 0 %, the module does nothing. |

### 6. Tone Tilt ("raspberry")
This module tilts the tone around one frequency. It makes the lows quieter and the highs louder, or the opposite.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Tilt** | -8 - +8 dB | 0 dB | A positive value makes the sound brighter: the highs go up and the lows go down by the same value. A negative value makes the sound darker. |
| **Pivot** | 200 Hz - 3 kHz | 700 Hz | The frequency at the centre of the tilt. This frequency does not change. |

### 7. Sound Destroyer ("ghost pepper berry")
This module adds distortion and digital damage. It has no oversampling: the aliasing is part of the sound.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Drive** | 0 - 100 % | 30 % | The distortion. Up to 40 %, the module clips softly. Above 40 %, it also folds the wave for a harsh sound. The module decreases the level when Drive increases. |
| **Crush** | 0 - 100 % | 20 % | Decreases the sample rate (up to 31 times) and the bit depth (from 16 bits to 3 bits). At 0 %, Crush is off. |
| **Mix** | 0 - 100 % | 30 % | The mix of the clean signal and the destroyed signal. |

### Juice Level ("input / output")
This card is always at the end. It has no on/off switch.

| Knob | Range | Default | What it does |
|---|---|---|---|
| **Input** | -24 - +24 dB | 0 dB | The level before the first module. |
| **Output** | -24 - +12 dB | 0 dB | The level after the last module. |

- The **IN** meter shows the level after the Input knob.
- The **OUT** meter shows the level that goes to the next plugin.

## Module order
- Drag a module card by its header to a different position. The audio order changes at once.
- The number on each card shows the position in the chain.
- **Reset order** sets the default order 1 to 7.
- The order is saved with the session and with the preset.

## Presets
Use the bar at the top right:
- **◀ ▶** load the previous or the next preset.
- Click the preset name to open the list. The list shows FACTORY and USER presets and the design of each preset.
- **Save** writes the changes into the current user preset.
- **Save as** makes a new user preset.
- **Rename** and **Delete** work on user presets only.

A preset stores all knobs, all switches, the module order and the design. A `*` after the name shows that you changed something after you loaded the preset.

Factory presets:
- **Init**: all modules off, all knobs at the default value, Berry design. A new BerryTone starts with Init.
- **9 string crunch**: Tightener, Impedance and Tilt on, Grape design.

You cannot change or delete factory presets. Use Save as.

User presets are files in `%APPDATA%\desNotto\BerryTone\Presets` (`*.btpreset`). The preset menu has "Open presets folder". A user preset file with the name of a factory preset does not show in the list.

## Designs and window size
- Click the logo to change the design: **Berry** (juice box) or **Grape** (dark). The preset stores the design.
- Drag the window edge or the corner at the bottom right to change the size (75 - 200 %). The proportions do not change.
- The **100% / 125%** buttons set a fixed size.
- The session saves the window size. Presets do not save it.

## Updates
1. The plugin reads the latest release on GitHub when it starts.
2. When a newer version exists, the button **Update to vX** shows at the bottom left.
3. Click the button. Read the release notes. Click **Install**.
4. Windows asks for administrator rights when the plugin folder needs them.
5. Restart the DAW.

## Latency
BerryTone reports 0 samples of latency. The audio path uses only IIR filters, envelope followers and sample-by-sample processing. There is no lookahead, no FFT and no oversampling in the audio path. The DI Scope uses an FFT on the user-interface thread only.

## Build
You need CMake 3.22 or newer, a C++17 compiler and git. The build gets JUCE 8.0.6 automatically.
- Windows: Visual Studio 2022 or Build Tools 2022.
- macOS: Xcode command line tools.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build does not copy the plugin into the system folder. Copy the file `build/BerryTone_artefacts/Release/VST3/BerryTone.vst3/Contents/x86_64-win/BerryTone.vst3` to `C:\Program Files\Common Files\VST3`.

Layout check without a DAW: configure with `-DBERRY_SNAPSHOT=ON` and build the target `BerryToneSnapshot`. The tool writes PNG files of the plugin window (both designs, all scope modes) and checks HOLD and the first-launch preset. It is a development tool. It is not part of the release.

## Release
1. Change the version in `CMakeLists.txt` (`BERRY_VERSION`).
2. Commit the changes.
3. Run `tools/make_release.ps1 -Publish` in PowerShell 7.

The script builds the plugin and writes `BerryTone-v<version>` (the VST3 file and the zip) into the `Releases` folder next to the repository. With `-Publish`, it also makes the git tag and the GitHub release. The zip name must end with `-win64.zip`, because the updater looks for this name.

## Version history

### 1.3.0
- New hint line at the bottom of the window: a module hint on the card, a knob hint on the knob.
- Hint texts in Russian and English. The RU / EN flags select the language (default: Russian).
- The version and the update button are at the right end of the hint line.

### 1.2.1
- The update button works again. The install script of 1.1.2 and 1.2.0 stopped with the error "Parameter set cannot be resolved" in Windows PowerShell 5.1.
- Versions 1.1.2 and 1.2.0 cannot install updates. Install 1.2.1 by hand one time: copy `BerryTone.vst3` over the old file. After that, the update button works.
- The updater replaces the plugin file in the folder that the DAW uses.

### 1.2.0
- DI Scope: new mode **SPECTRUM** (live spectrum of DI and OUT).
- REAL DI: **TIME** buttons (1 s, 200 ms, 50 ms) with a trigger for the short windows.
- REAL DI: **LEVEL** buttons (LIN, dB).
- HOLD works in SPECTRUM and with all TIME and LEVEL settings.
- The DI screen is 18 px narrower. The mode buttons are at the right.
- The README describes all modules and all knobs.

### 1.1.2
- HOLD freezes the DI. The OUT line still follows the modules.
- New first preset **Init** (all modules off). The preset Berry Base is removed.
- New factory preset **9 string crunch**.

### 1.1.1
- The Windows release has `BerryTone.vst3` as one file.
- No Standalone build.
- The window size changes (75 - 200 %). New buttons 100% / 125%.
- The line under the logo is shorter. It does not go under the preset buttons.

### 1.1.0
- DI Scope (REAL DI, FLAT LINE, HOLD) and the PURE DI button.
- Two designs: Berry and Grape.
- Presets: Save, Save as, Rename, Delete.
- Updates from GitHub.
- Low Freq Tightener: the Tight range is 0 - 300 % (was 0 - 100 %).
