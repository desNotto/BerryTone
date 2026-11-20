#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// DI Scope screen.
//  REAL DI   : the raw DI (grey) and the signal after the modules (colour).
//              Time window 1 s / 200 ms / 50 ms. Level scale LIN (auto) or dB.
//  SPECTRUM  : the spectrum of the live DI (grey area) and OUT (line), 0 / -72 dBFS.
//  FLAT LINE : a flat test signal goes through the chain; the line shows how the
//              modules bend it (gain in dB, 20 Hz - 20 kHz).
//  HOLD      : freezes the DI. The OUT line still follows the modules.
class ScopeView : public juce::Component, private juce::Timer
{
public:
    enum class Mode  { real = 0, flat = 1, spectrum = 2 };   // values are saved in the state
    enum class Zoom  { s1 = 0, ms200 = 1, ms50 = 2 };
    enum class Level { lin = 0, db = 1 };

    explicit ScopeView (BerryToneAudioProcessor&);

    void setDesign (Design d) { design = d; repaint(); }
    void setMode (Mode);
    Mode getMode() const { return mode; }
    void setZoom (Zoom);
    void setLevel (Level);
    void setHold (bool);

    void paint (juce::Graphics&) override;

    // Where the editor puts the TIME (0..2) and LEVEL (3..4) buttons, in screen coordinates.
    static juce::Rectangle<int> chipBounds (Design, int index);

private:
    void timerCallback() override;
    void paintReal (juce::Graphics&, juce::Rectangle<float>);
    void paintSpectrum (juce::Graphics&, juce::Rectangle<float>);
    void paintFlat (juce::Graphics&, juce::Rectangle<float>);
    void paintChipGroups (juce::Graphics&);
    void drawBox (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Justification, juce::Colour);

    double windowSec() const;
    void readLive();
    void refreshNow();
    void captureHeld();
    void processHeldOut();
    void rebuild (const std::vector<float>& di, const std::vector<float>& out, bool smoothLevel, bool smoothSpectrum);
    void buildCols (const std::vector<float>& di, const std::vector<float>& out, int start, int count);
    int findTrigger (const std::vector<float>& di, int base, int search) const;
    void computeSpectrum (const std::vector<float>& x, std::vector<float>& dbOut, bool smooth);
    void updateLevel (bool smooth);

    BerryToneAudioProcessor& proc;
    Design design = Design::berry;
    Mode mode = Mode::real;
    Zoom zoom = Zoom::s1;
    Level level = Level::lin;
    bool hold = false, resetLevel = true;

    std::vector<BerryToneAudioProcessor::ScopeCol> cols;
    std::vector<float> freqs, resp;
    float scale = 0.25f;      // LIN auto level: amplitude at the top of the screen
    float inPeakDb = -100.f;
    int tick = 0;
    double sr = 48000.0;

    // live raw samples (zoom windows and spectrum)
    std::vector<float> liveDi, liveOut;

    // HOLD: the frozen raw DI (warm-up + 1 s), its 1 s part, the processed OUT
    // and the chain key of the last render
    std::vector<float> heldRaw, heldDi, heldOut, heldKey;
    int heldWarm = 0;

    // spectrum
    std::unique_ptr<juce::dsp::FFT> fft;
    int fftOrder = 0;
    std::vector<float> fftBuf, fftWin, specDi, specOut;
};
