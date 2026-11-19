#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace
{
    enum class K { Hz, dB, ms, pct, pctS, x };

    juce::String toText (K k, float v)
    {
        switch (k)
        {
            case K::Hz:   return v >= 1000.f ? juce::String (v / 1000.f, 2) + " kHz"
                                             : juce::String (juce::roundToInt (v)) + " Hz";
            case K::dB:   return juce::String (v, 1) + " dB";
            case K::ms:   return juce::String (juce::roundToInt (v)) + " ms";
            case K::pct:  return juce::String (juce::roundToInt (v * 100.f)) + "%";
            case K::pctS: { const int p = juce::roundToInt (v * 100.f);
                            return juce::String (p > 0 ? "+" : "") + juce::String (p) + "%"; }
            case K::x:    return juce::String (v, 2) + "x";
        }
        return {};
    }

    float fromText (K k, const juce::String& s)
    {
        float v = s.getFloatValue();
        switch (k)
        {
            case K::Hz:   if (s.containsIgnoreCase ("k")) v *= 1000.f; return v;
            case K::pct:
            case K::pctS: return v / 100.f;
            default:      return v;
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout BerryToneAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;

    auto F = [&v] (const char* id, const char* name, float lo, float hi, float def, float skew, K kind)
    {
        v.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.f, skew), def,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([kind] (float x, int) { return toText (kind, x); })
                .withValueFromStringFunction ([kind] (const String& s) { return fromText (kind, s); })));
    };
    auto B = [&v] (const char* id, const char* name, bool def)
    {
        v.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    F (ID::inGain,  "Input",  -24.f, 24.f, 0.f, 1.f, K::dB);
    F (ID::outGain, "Output", -24.f, 12.f, 0.f, 1.f, K::dB);

    B (ID::tightOn, "Tightener On", true);
    F (ID::tightFreq, "Tightener Cut",   20.f, 250.f, 60.f, 0.5f, K::Hz);
    F (ID::tightAmt,  "Tightener Tight", 0.f, 1.f, 0.35f, 1.f, K::pct);

    B (ID::gateOn, "Gate On", false);
    F (ID::gateThresh, "Gate Threshold", -80.f, -20.f, -55.f, 1.f, K::dB);
    F (ID::gateRel,    "Gate Release",   10.f, 500.f, 80.f, 0.5f, K::ms);
    F (ID::gateDepth,  "Gate Depth",     -80.f, -6.f, -70.f, 1.f, K::dB);

    B (ID::chugOn, "Chug On", true);
    F (ID::chugPunch, "Chug Punch", -1.f, 1.f, 0.3f, 1.f, K::pctS);
    F (ID::chugBody,  "Chug Body",  -1.f, 1.f, 0.f, 1.f, K::pctS);
    F (ID::chugSpeed, "Chug Speed", 0.5f, 2.f, 1.f, 1.f, K::x);

    B (ID::octOn, "Octave On", false);
    F (ID::octBlend, "Octave Blend", 0.f, 1.f, 0.3f, 1.f, K::pct);
    F (ID::octTone,  "Octave Tone",  60.f, 600.f, 180.f, 0.5f, K::Hz);
    F (ID::octTrack, "Octave Track", 120.f, 800.f, 300.f, 0.5f, K::Hz);

    B (ID::impOn, "Impedance On", true);
    F (ID::impFreq, "Impedance Freq", 1500.f, 10000.f, 4500.f, 0.5f, K::Hz);
    F (ID::impPeak, "Impedance Peak", 0.f, 1.f, 0.4f, 1.f, K::pct);
    F (ID::impLoad, "Impedance Load", 0.f, 1.f, 0.7f, 1.f, K::pct);

    B (ID::tiltOn, "Tilt On", true);
    F (ID::tiltAmt,   "Tilt",       -8.f, 8.f, 0.f, 1.f, K::dB);
    F (ID::tiltPivot, "Tilt Pivot", 200.f, 3000.f, 700.f, 0.5f, K::Hz);

    B (ID::destOn, "Destroyer On", false);
    F (ID::destDrive, "Destroyer Drive", 0.f, 1.f, 0.3f, 1.f, K::pct);
    F (ID::destCrush, "Destroyer Crush", 0.f, 1.f, 0.2f, 1.f, K::pct);
    F (ID::destMix,   "Destroyer Mix",   0.f, 1.f, 0.3f, 1.f, K::pct);

    return { v.begin(), v.end() };
}

//==============================================================================
BerryToneAudioProcessor::BerryToneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BERRY_TONE", createLayout())
{
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    p = { P (ID::inGain), P (ID::outGain),
          P (ID::tightOn), P (ID::tightFreq), P (ID::tightAmt),
          P (ID::gateOn), P (ID::gateThresh), P (ID::gateRel), P (ID::gateDepth),
          P (ID::chugOn), P (ID::chugPunch), P (ID::chugBody), P (ID::chugSpeed),
          P (ID::octOn), P (ID::octBlend), P (ID::octTone), P (ID::octTrack),
          P (ID::impOn), P (ID::impFreq), P (ID::impPeak), P (ID::impLoad),
          P (ID::tiltOn), P (ID::tiltAmt), P (ID::tiltPivot),
          P (ID::destOn), P (ID::destDrive), P (ID::destCrush), P (ID::destMix) };

    for (int i = 0; i < numModules; ++i) order[(size_t) i].store (i);
    apvts.state.setProperty ("order", "0,1,2,3,4,5,6", nullptr);

    setLatencySamples (0); // no lookahead, no oversampling, no FFT
}

bool BerryToneAudioProcessor::isValidOrder (const std::array<int, numModules>& o)
{
    std::array<bool, numModules> seen {};
    for (int v : o)
    {
        if (v < 0 || v >= numModules || seen[(size_t) v]) return false;
        seen[(size_t) v] = true;
    }
    return true;
}

std::array<int, BerryToneAudioProcessor::numModules> BerryToneAudioProcessor::getOrder() const
{
    std::array<int, numModules> o {};
    for (size_t i = 0; i < o.size(); ++i) o[i] = order[i].load();
    return o;
}

void BerryToneAudioProcessor::setOrder (const std::array<int, numModules>& o)
{
    if (! isValidOrder (o)) return;
    juce::StringArray s;
    for (size_t i = 0; i < o.size(); ++i) { order[i].store (o[i]); s.add (juce::String (o[i])); }
    apvts.state.setProperty ("order", s.joinIntoString (","), nullptr); // saved with the session
}

bool BerryToneAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void BerryToneAudioProcessor::prepareToPlay (double sr, int)
{
    tight.prepare (sr); gate.prepare (sr); chug.prepare (sr); oct.prepare (sr);
    imp.prepare (sr);   tilt.prepare (sr); dest.prepare (sr);

    inSmooth.reset (sr, 0.02);  inSmooth.setCurrentAndTargetValue (berry::dbToLin (p.inGain->load()));
    outSmooth.reset (sr, 0.02); outSmooth.setCurrentAndTargetValue (berry::dbToLin (p.outGain->load()));
}

void BerryToneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (buffer.getNumChannels(), berry::kMaxCh);
    const int n   = buffer.getNumSamples();
    for (int c = nCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (nCh == 0) return;

    // --- parameters (once per block) ---
    const bool tightOn = p.tightOn->load() > 0.5f, gateOn = p.gateOn->load() > 0.5f,
               chugOn  = p.chugOn->load()  > 0.5f, octOn  = p.octOn->load()  > 0.5f,
               impOn   = p.impOn->load()   > 0.5f, tiltOn = p.tiltOn->load() > 0.5f,
               destOn  = p.destOn->load()  > 0.5f;

    const bool en[numModules] = { tightOn, gateOn, chugOn, octOn, impOn, tiltOn, destOn };
    int ord[numModules];
    for (int s = 0; s < numModules; ++s) ord[s] = order[(size_t) s].load (std::memory_order_relaxed);

    tight.set (p.tightFreq->load(), p.tightAmt->load());
    gate.set  (p.gateThresh->load(), p.gateRel->load(), p.gateDepth->load());
    chug.set  (p.chugPunch->load(), p.chugBody->load(), p.chugSpeed->load());
    oct.set   (p.octBlend->load(), p.octTone->load(), p.octTrack->load());
    imp.set   (p.impFreq->load(), p.impPeak->load(), p.impLoad->load());
    tilt.set  (p.tiltPivot->load(), p.tiltAmt->load());
    dest.set  (p.destDrive->load(), p.destCrush->load(), p.destMix->load());

    inSmooth.setTargetValue  (berry::dbToLin (p.inGain->load()));
    outSmooth.setTargetValue (berry::dbToLin (p.outGain->load()));

    float* d[berry::kMaxCh] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float inPk = 0.f, outPk = 0.f;

    for (int i = 0; i < n; ++i)
    {
        float x[berry::kMaxCh] = { 0.f, 0.f };
        const float gi = inSmooth.getNextValue();
        for (int c = 0; c < nCh; ++c)
        {
            x[c] = d[c][i] * gi;
            inPk = juce::jmax (inPk, std::fabs (x[c]));
        }

        // User-defined module order (default: Tightener, Gate, Chug, Octave, Impedance, Tilt, Destroyer)
        for (int s = 0; s < numModules; ++s)
        {
            const int id = ord[s];
            if (! en[id]) continue;
            switch (id)
            {
                case 0: tight.process (x, nCh); break;
                case 1: gate.process  (x, nCh); break;
                case 2: chug.process  (x, nCh); break;
                case 3: oct.process   (x, nCh); break;
                case 4: imp.process   (x, nCh); break;
                case 5: tilt.process  (x, nCh); break;
                case 6: dest.process  (x, nCh); break;
                default: break;
            }
        }

        const float go = outSmooth.getNextValue();
        for (int c = 0; c < nCh; ++c)
        {
            d[c][i] = x[c] * go;
            outPk = juce::jmax (outPk, std::fabs (d[c][i]));
        }
    }

    inPeak.store  (juce::jmax (inPeak.load(),  inPk));
    outPeak.store (juce::jmax (outPeak.load(), outPk));
    gateGain.store (gateOn ? gate.gain : 1.f);
}

juce::AudioProcessorEditor* BerryToneAudioProcessor::createEditor()
{
    return new BerryToneAudioProcessorEditor (*this);
}

void BerryToneAudioProcessor::getStateInformation (juce::MemoryBlock& dest_)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest_);
}

void BerryToneAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            std::array<int, numModules> o {};
            auto tokens = juce::StringArray::fromTokens (apvts.state.getProperty ("order", "0,1,2,3,4,5,6").toString(), ",", "");
            if (tokens.size() == numModules)
            {
                for (int i = 0; i < numModules; ++i) o[(size_t) i] = tokens[i].getIntValue();
                if (isValidOrder (o)) { setOrder (o); return; }
            }
            for (int i = 0; i < numModules; ++i) o[(size_t) i] = i;
            setOrder (o);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BerryToneAudioProcessor();
}
