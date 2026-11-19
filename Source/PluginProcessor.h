#pragma once
#include <JuceHeader.h>
#include <array>
#include "Dsp.h"

namespace ID
{
    inline constexpr const char* inGain    = "inGain";
    inline constexpr const char* outGain   = "outGain";

    inline constexpr const char* tightOn   = "tightOn";
    inline constexpr const char* tightFreq = "tightFreq";
    inline constexpr const char* tightAmt  = "tightAmt";

    inline constexpr const char* gateOn     = "gateOn";
    inline constexpr const char* gateThresh = "gateThresh";
    inline constexpr const char* gateRel    = "gateRel";
    inline constexpr const char* gateDepth  = "gateDepth";

    inline constexpr const char* chugOn    = "chugOn";
    inline constexpr const char* chugPunch = "chugPunch";
    inline constexpr const char* chugBody  = "chugBody";
    inline constexpr const char* chugSpeed = "chugSpeed";

    inline constexpr const char* octOn     = "octOn";
    inline constexpr const char* octBlend  = "octBlend";
    inline constexpr const char* octTone   = "octTone";
    inline constexpr const char* octTrack  = "octTrack";

    inline constexpr const char* impOn     = "impOn";
    inline constexpr const char* impFreq   = "impFreq";
    inline constexpr const char* impPeak   = "impPeak";
    inline constexpr const char* impLoad   = "impLoad";

    inline constexpr const char* tiltOn    = "tiltOn";
    inline constexpr const char* tiltAmt   = "tiltAmt";
    inline constexpr const char* tiltPivot = "tiltPivot";

    inline constexpr const char* destOn    = "destOn";
    inline constexpr const char* destDrive = "destDrive";
    inline constexpr const char* destCrush = "destCrush";
    inline constexpr const char* destMix   = "destMix";
}

class BerryToneAudioProcessor : public juce::AudioProcessor
{
public:
    BerryToneAudioProcessor();
    ~BerryToneAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Berry Tone"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Processing order of the 7 modules. Slot -> module id:
    // 0 Tightener, 1 Gate, 2 Chug, 3 Octave, 4 Impedance, 5 Tilt, 6 Destroyer
    static constexpr int numModules = 7;
    std::array<int, numModules> getOrder() const;
    void setOrder (const std::array<int, numModules>& newOrder);

    // UI meters (peak since last read; gate gain)
    std::atomic<float> inPeak { 0.f }, outPeak { 0.f }, gateGain { 1.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static bool isValidOrder (const std::array<int, numModules>&);
    std::array<std::atomic<int>, numModules> order;

    struct Params
    {
        std::atomic<float> *inGain, *outGain,
            *tightOn, *tightFreq, *tightAmt,
            *gateOn, *gateThresh, *gateRel, *gateDepth,
            *chugOn, *chugPunch, *chugBody, *chugSpeed,
            *octOn, *octBlend, *octTone, *octTrack,
            *impOn, *impFreq, *impPeak, *impLoad,
            *tiltOn, *tiltAmt, *tiltPivot,
            *destOn, *destDrive, *destCrush, *destMix;
    } p {};

    berry::Tightener        tight;
    berry::HorizonGate      gate;
    berry::ChugEnhancer     chug;
    berry::LowOctaveBlender oct;
    berry::ImpedanceCurve   imp;
    berry::ToneTilt         tilt;
    berry::SoundDestroyer   dest;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inSmooth, outSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerryToneAudioProcessor)
};
