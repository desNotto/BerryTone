// Dev tool: draws the plugin window into PNG files, for layout checks
// without a DAW, and runs HOLD and preset checks. Not part of the release.
// Build: -DBERRY_SNAPSHOT=ON
//   BerryToneSnapshot <outDir>
#include "../../Source/PluginEditor.h"
#include <cstdio>
#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

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


// Runs the updater's install script in Windows PowerShell 5.1 (the version the updater starts).
static int installScriptCheck()
{
   #if JUCE_WINDOWS
    auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BerryTone-installtest");
    root.deleteRecursively();
    root.createDirectory();
    auto src = root.getChildFile ("new").getChildFile ("BerryTone.vst3");
    src.getParentDirectory().createDirectory();
    src.replaceWithText (juce::String::repeatedString ("NEW", 2000));

    int failures = 0;
    auto run = [&] (const juce::String& caseName, const juce::File& dst, std::function<void()> prepare)
    {
        if (prepare) prepare();
        auto log = root.getChildFile ("install.log");
        log.deleteFile();
        auto script = root.getChildFile ("install.ps1");
        script.replaceWithText (updater::makeInstallScript (src, dst, log), true, true);
        juce::ChildProcess cp;
        juce::StringArray args { "powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", script.getFullPathName() };
        const bool started = cp.start (args);
        if (started) cp.waitForProcessToFinish (60000);
        const auto code = started ? cp.getExitCode() : 999u;
        const bool ok = code == 0 && dst.existsAsFile() && dst.loadFileAsString() == src.loadFileAsString();
        int olds = 0;
        for (auto& f : dst.getParentDirectory().findChildFiles (juce::File::findFilesAndDirectories, false, dst.getFileName() + ".old*"))
            { juce::ignoreUnused (f); ++olds; }
        std::printf ("install %-22s exit %u, file ok %s, old copies %d, log: %s\n", caseName.toRawUTF8(), code,
                     ok ? "yes" : "NO", olds, log.loadFileAsString().trim().toRawUTF8());
        if (! ok) ++failures;
    };

    auto dir1 = root.getChildFile ("VstPlugIns");
    auto dst1 = dir1.getChildFile ("BerryTone.vst3");
    run ("old file", dst1, [&] { dir1.createDirectory(); dst1.replaceWithText ("OLD"); });
    run ("second update", dst1, nullptr);   // removes the first .old copy
    auto dst2 = root.getChildFile ("Bundle").getChildFile ("BerryTone.vst3");
    run ("old bundle folder", dst2, [&] { dst2.getChildFile ("Contents").getChildFile ("x86_64-win").createDirectory(); });
    run ("no folder", root.getChildFile ("Missing").getChildFile ("Sub").getChildFile ("BerryTone.vst3"), nullptr);
    auto dir5 = root.getChildFile (juce::CharPointer_UTF8 ("Vst Plug'Ins \xd0\xa2\xd0\xb5\xd1\x81\xd1\x82"));
    auto dst5 = dir5.getChildFile ("BerryTone.vst3");
    run ("space ' cyrillic", dst5, [&] { dir5.createDirectory(); dst5.replaceWithText ("OLD"); });

    // a loaded DLL: Windows lets the script rename it, not overwrite it
    auto dir6 = root.getChildFile ("Loaded");
    auto dst6 = dir6.getChildFile ("BerryTone.vst3");
    dir6.createDirectory();
    juce::File::getSpecialLocation (juce::File::windowsSystemDirectory).getChildFile ("version.dll").copyFileTo (dst6);
    HMODULE h = LoadLibraryW (dst6.getFullPathName().toWideCharPointer());
    run (h != nullptr ? "loaded DLL" : "loaded DLL (not loaded)", dst6, nullptr);
    if (h != nullptr) FreeLibrary (h);

    root.deleteRecursively();
    return failures;
   #else
    return 0;
   #endif
}

// macOS: runs the updater's /bin/sh install script on fake bundles.
static int macInstallCheck()
{
   #if JUCE_MAC
    auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BerryTone-mac-installtest");
    root.deleteRecursively();
    auto makeBundle = [] (const juce::File& b, const juce::String& content)
    {
        auto bin = b.getChildFile ("Contents/MacOS/BerryTone");
        bin.getParentDirectory().createDirectory();
        bin.replaceWithText (content);
    };
    auto newV = root.getChildFile ("new/BerryTone.vst3"), newA = root.getChildFile ("new/BerryTone.component");
    makeBundle (newV, "NEW-VST3"); makeBundle (newA, "NEW-AU");

    auto dstV = root.getChildFile ("Plug Ins/VST3/BerryTone.vst3");
    auto dstA = root.getChildFile ("Plug Ins/Components/BerryTone.component");
    auto dstQ = root.getChildFile ("it's new/VST3/BerryTone.vst3");   // no folder yet, ' in the path
    makeBundle (dstV, "OLD"); makeBundle (dstA, "OLD");

    // the running bundle is found from the binary path
    const auto targets = updater::findMacTargets (dstV.getChildFile ("Contents/MacOS/BerryTone"), newV, newA);
    const bool foundRunning = targets.size() >= 1 && targets[0].dst == dstV && targets[0].src == newV;

    juce::Array<updater::BundleSwap> items;
    items.add ({ newV, dstV });
    items.add ({ newA, dstA });
    items.add ({ newV, dstQ });
    auto log = root.getChildFile ("install.log");
    auto script = root.getChildFile ("install.sh");
    script.replaceWithText (updater::makeMacInstallScript (items, log), false, false, "\n");   // sh needs LF line ends
    juce::ChildProcess cp;
    const bool started = cp.start (juce::StringArray { "/bin/sh", script.getFullPathName() });
    if (started) cp.waitForProcessToFinish (60000);

    auto content = [] (const juce::File& b) { return b.getChildFile ("Contents/MacOS/BerryTone").loadFileAsString(); };
    const bool ok = started && cp.getExitCode() == 0 && log.loadFileAsString().trim() == "OK"
                 && content (dstV) == "NEW-VST3" && content (dstA) == "NEW-AU" && content (dstQ) == "NEW-VST3"
                 && dstV.getParentDirectory().findChildFiles (juce::File::findDirectories, false, "BerryTone.vst3.old*").size() == 1;
    std::printf ("mac install: running bundle found %s, script %s, log: %s\n", foundRunning ? "yes" : "NO",
                 ok ? "ok" : "FAILED", log.loadFileAsString().trim().toRawUTF8());
    root.deleteRecursively();
    return (ok && foundRunning) ? 0 : 1;
   #else
    return 0;
   #endif
}

static void snap (const juce::File& outDir, const juce::String& name, Design d, float scale,
                  int scopeMode, int scopeTime, int scopeLevel, bool modulesOn,
                  int hintModule = -1, int hintKnob = -1)
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
    if (hintModule >= 0)
        for (auto* c : ed->getChildren())
            if (auto* v = dynamic_cast<BerryToneView*> (c))
                v->showHintForTest (hintModule, hintKnob);
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
    const int installFailures = installScriptCheck() + macInstallCheck();

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

    // hint line: knob and module hints in both languages (the user's choice is restored)
    const auto userLang = hints::getLang();
    hints::setLang (hints::Lang::ru);
    snap (outDir, "berry_hint_knob_ru",   Design::berry, 1.0f, 0, 0, 0, true, 5, 0);   // Tone Tilt: Tilt
    snap (outDir, "grape_hint_module_ru", Design::grape, 1.0f, 0, 0, 0, true, 6, -1);  // Sound Destroyer
    snap (outDir, "berry_hint_idle_ru",   Design::berry, 1.0f, 0, 0, 0, true);
    hints::setLang (hints::Lang::en);
    snap (outDir, "grape_hint_knob_en",   Design::grape, 1.0f, 0, 0, 0, true, 3, 2);   // Octave: Track
    snap (outDir, "berry_hint_module_en", Design::berry, 1.0f, 0, 0, 0, true, 5, -1);  // Tone Tilt
    hints::setLang (userLang);

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
    return (holdResult == 0 && installFailures == 0 && onCount == 0) ? 0 : 1;
}
