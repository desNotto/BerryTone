// Dev tool: draws the plugin window into PNG files, for layout checks
// without a DAW, and runs a HOLD check. Not part of the release.
// Build: -DBERRY_SNAPSHOT=ON
//   BerryToneSnapshot <outDir>
#include "../../Source/PluginEditor.h"
#include <cstdio>

// Feeds 1.5 s of a palm-muted low-E-like signal through the processor.
static void feedAudio (BerryToneAudioProcessor& proc)
{
    const double sr = 48000.0;
    const int block = 512;
    proc.setPlayConfigDetails (1, 1, sr, block);
    proc.prepareToPlay (sr, block);
    juce::AudioBuffer<float> buf (1, block);
    juce::MidiBuffer midi;
    int t = 0;
    for (int b = 0; b < (int) (1.5 * sr) / block; ++b)
    {
        for (int i = 0; i < block; ++i, ++t)
        {
            const double sec = (double) t / sr;
            const double env = std::exp (-6.0 * std::fmod (sec, 0.25));   // a chug every 250 ms
            buf.setSample (0, i, (float) (0.4 * env * (std::sin (2.0 * juce::MathConstants<double>::pi * 41.2 * sec)
                                                      + 0.5 * std::sin (2.0 * juce::MathConstants<double>::pi * 123.6 * sec))));
        }
        proc.processBlock (buf, midi);
    }
}

static float peakOut (const std::vector<BerryToneAudioProcessor::ScopeCol>& c)
{
    float p = 0.f;
    for (auto& x : c) p = juce::jmax (p, -x.outMin, x.outMax);
    return p;
}

static int holdCheck()
{
    BerryToneAudioProcessor proc;
    feedAudio (proc);

    std::vector<float> raw;
    const double sr = proc.getScopeSampleRate();
    const int warm = (int) (0.3 * sr);
    proc.readRawDi (raw, warm + (int) sr);

    std::vector<BerryToneAudioProcessor::ScopeCol> a, b, c;
    proc.renderHeld (raw, sr, warm, a);

    // change a module: output trim -12 dB
    auto* out = proc.apvts.getParameter (ID::outGain);
    out->setValueNotifyingHost (out->convertTo0to1 (-12.f));
    proc.renderHeld (raw, sr, warm, b);

    // PURE DI: OUT = DI
    proc.pureDi = true;
    proc.renderHeld (raw, sr, warm, c);

    bool diSame = true, pureSame = true;
    for (size_t i = 0; i < a.size(); ++i)
    {
        diSame   = diSame   && a[i].diMin == b[i].diMin && a[i].diMax == b[i].diMax;
        pureSame = pureSame && c[i].outMin == c[i].diMin && c[i].outMax == c[i].diMax;
    }
    const float pa = peakOut (a), pb = peakOut (b);
    const float ratioDb = juce::Decibels::gainToDecibels (pb / juce::jmax (1.0e-9f, pa));
    std::printf ("HOLD check: raw %d samples, DI unchanged=%s, OUT peak %.3f -> %.3f (%.1f dB, want about -12), PURE DI out==di=%s\n",
                 (int) raw.size(), diSame ? "yes" : "NO", pa, pb, ratioDb, pureSame ? "yes" : "NO");
    return (diSame && pureSame && std::abs (ratioDb + 12.f) < 0.5f && pa > 0.01f) ? 0 : 1;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    const int holdResult = holdCheck();

    for (auto d : { Design::berry, Design::grape })
        for (float s : { 1.0f, 1.25f, 0.8f })
        {
            BerryToneAudioProcessor proc;
            feedAudio (proc);              // so the scope shows a signal
            proc.setDesign (d);
            proc.uiScale = s;
            std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            const auto name = juce::String (d == Design::grape ? "grape" : "berry") + "_" + juce::String (juce::roundToInt (s * 100)) + ".png";
            juce::FileOutputStream out (outDir.getChildFile (name));
            out.setPosition (0); out.truncate();
            juce::PNGImageFormat().writeImageToStream (img, out);
            std::printf ("%s %dx%d\n", name.toRawUTF8(), ed->getWidth(), ed->getHeight());
            ed.reset();
        }

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
