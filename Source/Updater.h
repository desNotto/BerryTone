#pragma once
#include <JuceHeader.h>
#include <functional>

// Gets plugin updates from the GitHub releases of the project.
//  1. UpdateChecker reads the latest release (once per process).
//  2. UpdateInstaller downloads the "*-win64.zip" asset, unpacks it and
//     replaces the installed VST3 file (one file, not a bundle folder). Windows asks for
//     admin rights (UAC) when the plugin folder needs them.
//     The new version starts after the DAW restarts.
namespace updater
{
    inline constexpr const char* githubRepo = "desNotto/BerryTone";

    struct ReleaseInfo
    {
        juce::String version;     // "1.2.0"
        juce::String notes;       // release text from GitHub
        juce::String assetUrl;    // zip for Windows x64
        juce::String pageUrl;     // release page in the browser
    };

    // true when version a is newer than version b ("1.10.0" > "1.9.3")
    bool isNewer (const juce::String& a, const juce::String& b);

    // The PowerShell script that replaces the plugin file dst with src and writes
    // "OK" or "ERROR: ..." into log. It must run in Windows PowerShell 5.1:
    // it uses .NET path functions, not Split-Path (Split-Path -LiteralPath -Leaf
    // fails in 5.1). A loaded DLL can be renamed but not overwritten, so the
    // script renames the old file (or an old bundle folder) first.
    juce::String makeInstallScript (const juce::File& src, const juce::File& dst, const juce::File& log);

    // The release asset of this platform: "-win64.zip" (Windows) or "-mac.zip" (macOS).
    const char* assetSuffix();

    // macOS: a /bin/sh script that replaces each installed bundle (dst) with the new
    // bundle (src). It renames the old bundle to "<name>.old<time>" first: the DAW
    // can keep the loaded binary. It writes "OK" or "ERROR: ..." into log.
    struct BundleSwap { juce::File src, dst; };
    juce::String makeMacInstallScript (const juce::Array<BundleSwap>& items, const juce::File& log);

    // macOS: the bundles to update. The running bundle, plus the BerryTone bundles in the
    // standard plug-in folders (user and system). newVst3 / newAu are the unpacked bundles.
    juce::Array<BundleSwap> findMacTargets (const juce::File& running, const juce::File& newVst3, const juce::File& newAu);

    class UpdateChecker : private juce::Thread
    {
    public:
        UpdateChecker();
        ~UpdateChecker() override;

        // Starts the check one time per process. The callback runs on the
        // message thread, and only when a newer release exists.
        void checkOnce();
        bool hasUpdate() const { return found.load(); }
        ReleaseInfo getInfo() const;

        std::function<void()> onUpdateFound;   // set by the editor

    private:
        void run() override;
        std::atomic<bool> started { false }, found { false };
        mutable juce::CriticalSection lock;
        ReleaseInfo info;
        JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateChecker)
    };

    class UpdateInstaller : private juce::Thread
    {
    public:
        UpdateInstaller();
        ~UpdateInstaller() override;

        void start (const ReleaseInfo&);
        bool isRunning() const { return isThreadRunning(); }

        double progress = 0.0;                 // read by a ProgressBar (0..1, -1 = busy)
        std::function<void (bool ok, const juce::String& message)> onFinished;   // message thread

    private:
        void run() override;
        void finish (bool ok, const juce::String& msg);
        bool download (juce::File& zipOut, juce::String& err);
        bool runInstallScript (const juce::File& script, bool elevated, juce::String& err);
        bool runProcess (const juce::StringArray& args, int timeoutMs, juce::String& output);
        void runMac();

        ReleaseInfo rel;
        JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateInstaller)
    };
}
