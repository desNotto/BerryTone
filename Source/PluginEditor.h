#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include "ScopeView.h"
#include "Updater.h"

class BerryToneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit BerryToneAudioProcessorEditor (BerryToneAudioProcessor&);
    ~BerryToneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // drag a card's header to reorder the signal chain; click the logo to change the design
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<SliderAtt> att;
    };

    struct Module
    {
        juce::String title, flavor;
        int id = 0;
        bool isMaster = false, hasLed = false;
        float lastAlpha = -1.f;
        juce::ToggleButton on;
        std::unique_ptr<ButtonAtt> onAtt;
        std::vector<std::unique_ptr<Knob>> knobs;
        juce::Rectangle<int> bounds;
    };

    Module& addModule (const juce::String& title, const juce::String& flavor, const char* onId,
                       std::initializer_list<std::pair<const char*, const char*>> knobs);
    void timerCallback() override;
    juce::Rectangle<int> slotRect (int slot) const;
    int slotOf (int id) const;
    int moduleAtHeader (juce::Point<int>) const;
    void applyOrder();
    void applyDesign (Design);
    void styleButton (juce::Button&, const juce::String& style);

    // painting
    void paintBerry (juce::Graphics&);
    void paintGrape (juce::Graphics&);
    void drawTube (juce::Graphics&, juce::Rectangle<float>, float level, const juce::String& name);
    void drawLeds (juce::Graphics&, juce::Rectangle<float>, float level, const juce::String& name);

    // presets
    void showPresetMenu();
    void savePreset();
    void savePresetAs();
    void renamePreset();
    void deletePreset();
    void updatePresetLabel();

    // updates
    void showUpdateDialog();
    void startInstall();

    // dialogs (shown inside the plugin window)
    void showMessage (const juce::String& title, const juce::String& text);
    void askText (const juce::String& title, const juce::String& text, const juce::String& initial,
                  const juce::String& okText, std::function<void (juce::String)> onOk);
    void askConfirm (const juce::String& title, const juce::String& text, const juce::String& okText,
                     bool danger, std::function<void()> onOk);
    juce::AlertWindow* makeDialog (const juce::String& title, const juce::String& text);
    void openDialog (juce::AlertWindow*, std::function<void (int, juce::AlertWindow&)> onClose);

    ThemeLookAndFeel lnf;
    BerryToneAudioProcessor& proc;
    std::vector<std::unique_ptr<Module>> modules;
    Design design = Design::berry;

    ScopeView scope;
    juce::TextButton modeReal { "REAL DI" }, modeFlat { "FLAT LINE" }, pureBtn, holdBtn { "HOLD" };
    juce::TextButton presetPrev, presetNext, presetName, saveBtn { "Save" }, saveAsBtn { "Save as" },
                     renameBtn { "Rename" }, deleteBtn { "Delete" };
    juce::TextButton updateBtn, resetBtn { "Reset order" };

    juce::SharedResourcePointer<updater::UpdateChecker> checker;
    std::unique_ptr<updater::UpdateInstaller> installer;
    juce::Component::SafePointer<juce::AlertWindow> progressDialog;

    juce::Rectangle<int> logoRect { 24, 20, 410, 80 }, scopeCard { 22, 104, 896, 150 },
                         scopeScreen { 34, 116, 708, 126 }, inMeter, outMeter, gateLed;
    std::array<int, 7> order {};
    int dragId = -1;
    float inLvl = 0.f, outLvl = 0.f;
    bool gateOpenLed = true, lastPure = false;
    juce::String lastPresetText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerryToneAudioProcessorEditor)
};
