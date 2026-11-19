#pragma once
#include <JuceHeader.h>
#include <vector>

class BerryToneAudioProcessor;

// The two looks of the editor. The value is saved in the session and in presets.
enum class Design { berry = 0, grape = 1 };

// Factory presets (built in, read only) + user presets (one XML file each).
// A preset stores every parameter, the module order and the design.
class PresetManager
{
public:
    struct Entry
    {
        juce::String name;
        bool factory = false;
        int factoryIndex = -1;
        juce::File file;
        Design design = Design::berry;
    };

    explicit PresetManager (BerryToneAudioProcessor&);

    static juce::File getUserDir();          // %APPDATA%\desNotto\BerryTone\Presets
    static constexpr const char* fileExt = ".btpreset";

    void refresh();
    const std::vector<Entry>& getEntries() const { return entries; }

    int getCurrentIndex() const;             // -1 = no preset (unsaved state)
    juce::String getCurrentName() const;
    bool currentIsFactory() const { return curFactory; }
    bool isModified() const;

    bool loadIndex (int index);
    void next();
    void previous();

    juce::Result save();                     // overwrite the current user preset
    juce::Result saveAs (const juce::String& name);
    juce::Result rename (const juce::String& newName);
    juce::Result remove();
    bool userPresetExists (const juce::String& name) const;

    // Call after the host restores a session.
    void syncFromState();

private:
    void applyFactory (int index);
    void setCurrent (const juce::String& name, bool factory);
    std::vector<float> snapshot() const;
    juce::File fileFor (const juce::String& name) const;
    static juce::String cleanName (const juce::String&);
    juce::Result writeFile (const juce::File&);

    BerryToneAudioProcessor& proc;
    std::vector<Entry> entries;
    juce::String curName;
    bool curFactory = false;
    std::vector<float> loadedSnapshot;
};
