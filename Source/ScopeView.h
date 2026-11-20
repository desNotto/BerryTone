#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// DI Scope screen.
//  REAL DI   : the raw DI (grey) and the signal after the modules (colour), 1 s window.
//  FLAT LINE : a flat test signal goes through the chain; the line shows how the
//              modules bend it (gain in dB, 20 Hz - 20 kHz).
class ScopeView : public juce::Component, private juce::Timer
{
public:
    enum class Mode { real = 0, flat = 1 };

    explicit ScopeView (BerryToneAudioProcessor&);

    void setDesign (Design d) { design = d; repaint(); }
    void setMode (Mode);
    Mode getMode() const { return mode; }
    // HOLD (REAL DI): freezes the DI; the OUT line still follows the modules.
    void setHold (bool);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void paintReal (juce::Graphics&, juce::Rectangle<float>);
    void paintFlat (juce::Graphics&, juce::Rectangle<float>);
    void captureHeld();
    void renderHeld();
    void updateLevel (bool smooth);
    void drawBox (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Justification, juce::Colour);

    BerryToneAudioProcessor& proc;
    Design design = Design::berry;
    Mode mode = Mode::real;
    bool hold = false;

    std::vector<BerryToneAudioProcessor::ScopeCol> cols;
    std::vector<float> freqs, resp;
    float scale = 0.25f;      // auto level: amplitude at the top of the screen
    float inPeakDb = -100.f;
    int tick = 0;

    // HOLD: the frozen raw DI (warm-up + 1 s) and the chain key of the last render
    std::vector<float> heldRaw, heldKey;
    double heldSr = 48000.0;
    int heldWarm = 0;
};
