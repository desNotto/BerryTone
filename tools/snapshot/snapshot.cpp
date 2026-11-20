// Dev tool: draws the plugin window into PNG files, for layout checks
// without a DAW, and runs HOLD and preset checks. Not part of the release.
// Build: -DBERRY_SNAPSHOT=ON
//   BerryToneSnapshot <outDir>
#include "../../Source/PluginEditor.h"
#include <cstdio>

// Feeds 1.5 s of palm-muted low-E-like chugs (with pauses) through the processor.
static void feedAudio (BerryToneAudioProcessor& proc)
{
    const double sr = 48000.0;
    const int block = 512;
    proc.setPlayConfigDetails (1, 1, sr, block);
    proc.prepareToPlay (sr, block);
    juce::AudioBuffer<float> buf (1, block);
    juce::MidiBuffer midi;
    int t = 0;
    juce::Random rng (7);
    for (int b = 0; b < (int) (1.58 * sr) / block; ++b)   // ends inside a chug
    {
        for (int i = 0; i < block; ++i, ++t)
        {
            const double sec = (double) t / sr;
            const double ph = std::fmod (sec, 0.375);
            const double env = ph < 0.2 ? std::exp (-14.0 * ph) : 0.0;   // a chug, then a pause
            const double w = 2.0 * juce::MathConstants<double>::pi * 41.2 * sec;
            buf.setSample (0, i, (float) (0.4 * env * (std::sin (w) + 0.55 * std::sin (2 * w + 0.4) + 0.3 * std::sin (3 * w + 1.1)
                                                       + 0.12 * std::sin (6 * w)) + 0.002 * (rng.nextFloat() - 0.5f)));
        }
        proc.processBlock (buf, midi);
    }
}

static int holdCheck()
{
    BerryToneAudioProcessor proc;
    feedAudio (proc);

    std::vector<float> raw;
    const double sr = proc.getScopeSampleRate();
    const int warm = (int) (0.3 * sr);
    proc.readRaw (raw, nullptr, warm + (int) sr);

    std::vector<float> a, b, c;
    proc.processHeld (raw, sr, warm, a);

    // change a module: output trim -12 dB
    auto* out = proc.apvts.getParameter (ID::outGain);
    out->setValueNotifyingHost (out->convertTo0to1 (-12.f));
    proc.processHeld (raw, sr, warm, b);

    // PURE DI: OUT = DI
    proc.pureDi = true;
    proc.processHeld (raw, sr, warm, c);

    auto peak = [] (const std::vector<float>& v) { float p = 0.f; for (auto x : v) p = juce::jmax (p, std::abs (x)); return p; };
    bool pureSame = c.size() == raw.size() - (size_t) warm;
    for (size_t i = 0; pureSame && i < c.size(); ++i) pureSame = c[i] == raw[i + (size_t) warm];
    const float pa = peak (a), pb = peak (b);
    const float ratioDb = juce::Decibels::gainToDecibels (pb / juce::jmax (1.0e-9f, pa));
    std::printf ("HOLD check: raw %d samples, OUT peak %.3f -> %.3f (%.1f dB, want -12), PURE DI out==di=%s\n",
                 (int) raw.size(), pa, pb, ratioDb, pureSame ? "yes" : "NO");
    return (pureSame && std::abs (ratioDb + 12.f) < 0.5f && pa > 0.01f) ? 0 : 1;
}

static void snap (const juce::File& outDir, const juce::String& name, Design d, float scale,
                  int scopeMode, int scopeTime, int scopeLevel, bool modulesOn)
{
    BerryToneAudioProcessor proc;
    if (modulesOn)   // a chain that changes the signal: tightener, gate, impedance, tilt, destroyer
        for (auto* id : { ID::tightOn, ID::gateOn, ID::impOn, ID::tiltOn })
            proc.apvts.getParameter (id)->setValueNotifyingHost (1.f);
    feedAudio (proc);              // so the scope shows a signal
    proc.setDesign (d);
    proc.uiScale = scale;
    proc.apvts.state.setProperty ("scopeMode", scopeMode, nullptr);
    proc.apvts.state.setProperty ("scopeTime", scopeTime, nullptr);
    proc.apvts.state.setProperty ("scopeLevel", scopeLevel, nullptr);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::FileOutputStream out (outDir.getChildFile (name + ".png"));
    out.setPosition (0); out.truncate();
    juce::PNGImageFormat().writeImageToStream (img, out);
    std::printf ("%s %dx%d\n", name.toRawUTF8(), ed->getWidth(), ed->getHeight());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    const int holdResult = holdCheck();

    // mode: 0 real, 1 flat, 2 spectrum   time: 0 1 s, 1 200 ms, 2 50 ms   level: 0 lin, 1 dB
    snap (outDir, "berry_100",        Design::berry, 1.0f,  0, 0, 0, true);
    snap (outDir, "grape_100",        Design::grape, 1.0f,  0, 0, 0, true);
    snap (outDir, "berry_125",        Design::berry, 1.25f, 0, 0, 0, true);
    snap (outDir, "berry_50ms_lin",   Design::berry, 1.0f,  0, 2, 0, true);
    snap (outDir, "grape_50ms_lin",   Design::grape, 1.0f,  0, 2, 0, true);
    snap (outDir, "berry_1s_db",      Design::berry, 1.0f,  0, 0, 1, true);
    snap (outDir, "grape_200ms_db",   Design::grape, 1.0f,  0, 1, 1, true);
    snap (outDir, "berry_spectrum",   Design::berry, 1.0f,  2, 0, 0, true);
    snap (outDir, "grape_spectrum",   Design::grape, 1.0f,  2, 0, 0, true);
    snap (outDir, "grape_flat",       Design::grape, 1.0f,  1, 0, 0, true);

    // preset list + first-launch preset
    BerryToneAudioProcessor proc;
    for (auto& e : proc.presets.getEntries())
        std::printf ("preset: %s%s\n", e.name.toRawUTF8(), e.factory ? " (factory)" : "");
    int onCount = 0;
    for (auto* id : { ID::tightOn, ID::gateOn, ID::chugOn, ID::octOn, ID::impOn, ID::tiltOn, ID::destOn })
        onCount += proc.apvts.getRawParameterValue (id)->load() > 0.5f ? 1 : 0;
    std::printf ("first launch: preset '%s', modules on: %d, design %s, modified %s\n",
                 proc.presets.getCurrentName().toRawUTF8(), onCount,
                 proc.getDesign() == Design::grape ? "grape" : "berry", proc.presets.isModified() ? "yes" : "no");
    return (holdResult == 0 && onCount == 0) ? 0 : 1;
}
