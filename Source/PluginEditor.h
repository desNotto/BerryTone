#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Juice-box palette
namespace berrycol
{
    inline const juce::Colour cartonTop  { 0xff9a2fc4 };
    inline const juce::Colour cartonBot  { 0xff45115a };
    inline const juce::Colour cream      { 0xfffff3e3 };
    inline const juce::Colour deep       { 0xff3a0b4d };
    inline const juce::Colour ink        { 0xff5a1a80 };   // purple used for parameter names + values
    inline const juce::Colour pink       { 0xffff3d94 };
    inline const juce::Colour mint       { 0xff35d49a };
    inline const juce::Colour leaf       { 0xff7bd34b };
}

class BerryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BerryLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    static void drawBerry (juce::Graphics&, float cx, float cy, float r);
};

class BerryToneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit BerryToneAudioProcessorEditor (BerryToneAudioProcessor&);
    ~BerryToneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // drag a card's header to reorder the signal chain
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
        int id = 0, stage = 0;
        bool isMaster = false, hasLed = false;
        int lastOn = -1;
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
    void drawTube (juce::Graphics&, juce::Rectangle<float>, float level, const juce::String& name);

    BerryLookAndFeel lnf;
    BerryToneAudioProcessor& proc;
    std::vector<std::unique_ptr<Module>> modules;

    juce::Rectangle<int> inTube, outTube, gateLed;
    std::array<int, 7> order {};
    int dragId = -1;
    juce::TextButton resetBtn { "Reset order" };
    float inLvl = 0.f, outLvl = 0.f;
    bool gateOpenLed = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerryToneAudioProcessorEditor)
};
