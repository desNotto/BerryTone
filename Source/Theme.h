#pragma once
#include <JuceHeader.h>
#include "PresetManager.h"   // Design

// Berry: the juice-box look (classic)
namespace berrycol
{
    inline const juce::Colour cartonTop  { 0xff9a2fc4 };
    inline const juce::Colour cartonBot  { 0xff45115a };
    inline const juce::Colour cream      { 0xfffff3e3 };
    inline const juce::Colour deep       { 0xff3a0b4d };
    inline const juce::Colour ink        { 0xff5a1a80 };   // parameter names + values
    inline const juce::Colour pink       { 0xffff3d94 };
    inline const juce::Colour mint       { 0xff35d49a };
    inline const juce::Colour leaf       { 0xff7bd34b };
    inline const juce::Colour offGrey    { 0xffcfc2cf };
    inline const juce::Colour danger     { 0xffb0175f };
}

// Grape: the dark look (from the UI sheet ui1.png)
namespace grapecol
{
    inline const juce::Colour bgTop      { 0xff3a1736 };
    inline const juce::Colour bgMid      { 0xff230d20 };
    inline const juce::Colour bgBot      { 0xff170915 };
    inline const juce::Colour panelTop   { 0xff2e122b };
    inline const juce::Colour panelBot   { 0xff1d0b1b };
    inline const juce::Colour border     { 0xff51204c };
    inline const juce::Colour borderLive { 0xff5d2457 };
    inline const juce::Colour borderDim  { 0xff3a1736 };
    inline const juce::Colour text       { 0xfff0d8ed };
    inline const juce::Colour label      { 0xffd698cf };
    inline const juce::Colour flavor     { 0xffc875bf };
    inline const juce::Colour muted      { 0xffa07a9c };
    inline const juce::Colour dim        { 0xff7a5a78 };
    inline const juce::Colour accent     { 0xffc163b7 };
    inline const juce::Colour glow       { 0xffd16cff };
    inline const juce::Colour trace      { 0xffe58cff };
    inline const juce::Colour screen     { 0xff0f060d };
    inline const juce::Colour tickOff    { 0xff4a2346 };
    inline const juce::Colour pointer    { 0xfff2e2b8 };
    inline const juce::Colour leaf       { 0xff5f8a34 };
    inline const juce::Colour leafDark   { 0xff3f6420 };
    inline const juce::Colour gold       { 0xffb8963a };
    inline const juce::Colour ledGreen   { 0xff6fd13a };
    inline const juce::Colour ledYellow  { 0xffffc21a };
    inline const juce::Colour ledRed     { 0xffff3b3b };
    inline const juce::Colour ledOff     { 0xff1f1a1e };
    inline const juce::Colour danger     { 0xffff8ab0 };
}

// One LookAndFeel for both designs. setDesign() switches the drawing code.
// Buttons choose their look with the component property "style":
//   flat, pill, seg, power, hold, update, reset  (no property = dialog button)
class ThemeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ThemeLookAndFeel();

    void setDesign (Design);
    Design getDesign() const { return design; }
    bool grape() const { return design == Design::grape; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawProgressBar (juce::Graphics&, juce::ProgressBar&, int width, int height, double progress, const juce::String&) override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    juce::Font getPopupMenuFont() override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;

    // shared drawing helpers
    static juce::Font font (float height, bool bold = true, float kerning = 0.f);
    static void drawBerry (juce::Graphics&, float cx, float cy, float r);
    static void drawGrapeIcon (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
    static void drawGrapeCluster (juce::Graphics&, float x, float y, float scale);
    static void drawLeaf (juce::Graphics&, juce::Point<float> base, juce::Point<float> tip, float width, juce::Colour);
    static void drawGlow (juce::Graphics&, const juce::Path&, juce::Colour, float radius);
    static void drawFlag (juce::Graphics&, juce::Rectangle<float>, bool russia);

private:
    Design design = Design::berry;
};
