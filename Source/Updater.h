#pragma once
#include <JuceHeader.h>
#include <functional>

// Gets plugin updates from the GitHub releases of the project.
//  1. UpdateChecker reads the latest release (once per process).
//  2. UpdateInstaller downloads the "*-win64.zip" asset, unpacks it and
//     replaces the installed VST3 / Standalone files. Windows asks for
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

        ReleaseInfo rel;
        JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateInstaller)
    };
}
