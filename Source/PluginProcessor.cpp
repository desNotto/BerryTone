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
    F (ID::tightAmt,  "Tightener Tight", 0.f, 3.f, 0.35f, 1.f, K::pct);   // 0-300 %

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
    apvts.state.setProperty ("design", "berry", nullptr);

    for (auto& c : scopeBuf)
        for (auto& v : c.v) v.store (0.f);

    presets.loadIndex (0);   // a new instance starts on the first factory preset

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

    pureStep = (float) (1.0 / (0.015 * sr));   // 15 ms crossfade, no clicks
    pureMix  = pureDi.load() ? 1.f : 0.f;

    scopeSr = sr;
    scopeBucket = juce::jmax (1, juce::roundToInt (sr * scopeWindowSec / scopeCols));
    scopeCount = 0;
    scopeAcc = { 0.f, 0.f, 0.f, 0.f };
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

    const float pureTarget = pureDi.load (std::memory_order_relaxed) ? 1.f : 0.f;
    const float chInv = 1.f / (float) nCh;
    int sw = scopeWrite.load (std::memory_order_relaxed);
    int rw = rawWrite.load (std::memory_order_relaxed);

    for (int i = 0; i < n; ++i)
    {
        float x[berry::kMaxCh] = { 0.f, 0.f };
        float raw[berry::kMaxCh] = { 0.f, 0.f };
        const float gi = inSmooth.getNextValue();
        for (int c = 0; c < nCh; ++c)
        {
            raw[c] = d[c][i];
            x[c] = raw[c] * gi;
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
        if (pureMix != pureTarget)
            pureMix = pureTarget > pureMix ? juce::jmin (pureTarget, pureMix + pureStep)
                                           : juce::jmax (pureTarget, pureMix - pureStep);

        float diMono = 0.f, outMono = 0.f;
        for (int c = 0; c < nCh; ++c)
        {
            const float wet = x[c] * go;
            d[c][i] = wet + pureMix * (raw[c] - wet);     // PURE DI = the raw input, untouched
            outPk = juce::jmax (outPk, std::fabs (d[c][i]));
            diMono  += raw[c];
            outMono += d[c][i];
        }

        // --- DI scope: one min/max column per bucket ---
        diMono *= chInv; outMono *= chInv;
        rawBuf[(size_t) rw].store (diMono, std::memory_order_relaxed);      // HOLD, zoom, spectrum
        rawOutBuf[(size_t) rw].store (outMono, std::memory_order_relaxed);
        rw = (rw + 1) & (rawRing - 1);
        if (scopeCount == 0)
            scopeAcc = { diMono, diMono, outMono, outMono };
        else
        {
            scopeAcc.diMin  = juce::jmin (scopeAcc.diMin,  diMono);
            scopeAcc.diMax  = juce::jmax (scopeAcc.diMax,  diMono);
            scopeAcc.outMin = juce::jmin (scopeAcc.outMin, outMono);
            scopeAcc.outMax = juce::jmax (scopeAcc.outMax, outMono);
        }
        if (++scopeCount >= scopeBucket)
        {
            auto& col = scopeBuf[(size_t) sw];
            col.v[0].store (scopeAcc.diMin,  std::memory_order_relaxed);
            col.v[1].store (scopeAcc.diMax,  std::memory_order_relaxed);
            col.v[2].store (scopeAcc.outMin, std::memory_order_relaxed);
            col.v[3].store (scopeAcc.outMax, std::memory_order_relaxed);
            sw = (sw + 1) % scopeRing;
            scopeWrite.store (sw, std::memory_order_release);
            scopeCount = 0;
        }
    }

    rawWrite.store (rw, std::memory_order_release);
    inPeak.store  (juce::jmax (inPeak.load(),  inPk));
    outPeak.store (juce::jmax (outPeak.load(), outPk));
    gateGain.store (gateOn ? gate.gain : 1.f);
}

juce::AudioProcessorEditor* BerryToneAudioProcessor::createEditor()
{
    return new BerryToneAudioProcessorEditor (*this);
}

Design BerryToneAudioProcessor::getDesign() const
{
    return apvts.state.getProperty ("design", "berry").toString() == "grape" ? Design::grape : Design::berry;
}

void BerryToneAudioProcessor::setDesign (Design d)
{
    apvts.state.setProperty ("design", d == Design::grape ? "grape" : "berry", nullptr);
}

std::unique_ptr<juce::XmlElement> BerryToneAudioProcessor::stateToXml()
{
    return apvts.copyState().createXml();
}

bool BerryToneAudioProcessor::stateFromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName (apvts.state.getType()))
        return false;

    apvts.replaceState (juce::ValueTree::fromXml (xml));

    std::array<int, numModules> o {};
    auto tokens = juce::StringArray::fromTokens (apvts.state.getProperty ("order", "0,1,2,3,4,5,6").toString(), ",", "");
    if (tokens.size() == numModules)
    {
        for (int i = 0; i < numModules; ++i) o[(size_t) i] = tokens[i].getIntValue();
        if (isValidOrder (o)) { setOrder (o); return true; }
    }
    for (int i = 0; i < numModules; ++i) o[(size_t) i] = i;
    setOrder (o);
    return true;
}

void BerryToneAudioProcessor::getStateInformation (juce::MemoryBlock& dest_)
{
    if (auto xml = stateToXml())
    {
        xml->setAttribute ("uiScale", (double) uiScale.load());   // session only, presets do not store it
        copyXmlToBinary (*xml, dest_);
    }
}

void BerryToneAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasAttribute ("uiScale"))
        {
            uiScale = juce::jlimit (0.75f, 2.0f, (float) xml->getDoubleAttribute ("uiScale", 1.0));
            xml->removeAttribute ("uiScale");
        }
        if (stateFromXml (*xml))
            presets.syncFromState();
    }
}

//==============================================================================
void BerryToneAudioProcessor::readScope (std::vector<ScopeCol>& out) const
{
    out.resize ((size_t) scopeCols);
    const int w = scopeWrite.load (std::memory_order_acquire);
    for (int i = 0; i < scopeCols; ++i)
    {
        const auto& c = scopeBuf[(size_t) ((w - scopeCols + i + scopeRing) % scopeRing)];
        out[(size_t) i] = { c.v[0].load (std::memory_order_relaxed), c.v[1].load (std::memory_order_relaxed),
                             c.v[2].load (std::memory_order_relaxed), c.v[3].load (std::memory_order_relaxed) };
    }
}

bool BerryToneAudioProcessor::computeResponse (const std::vector<float>& freqs, std::vector<float>& db, bool force)
{
    // Only recompute when a parameter, the order or PURE DI changed.
    auto key = chainKey();
    if (! force && key == lastResponseKey && db.size() == freqs.size())
        return false;
    lastResponseKey = key;

    db.assign (freqs.size(), 0.f);
    if (pureDi.load())
        return true;   // nothing touches the signal: the line stays flat

    constexpr double sr = 48000.0;
    constexpr int fftOrder = 13, N = 1 << fftOrder, frames = 8, settle = 8192;

    auto chain = makeOfflineChain (sr);
    const float go = chain->go;

    juce::dsp::FFT fft (fftOrder);
    std::vector<float> win ((size_t) N), fx ((size_t) N), fy ((size_t) N), bx ((size_t) (2 * N)), by ((size_t) (2 * N));
    std::vector<double> px ((size_t) (N / 2 + 1), 0.0), py ((size_t) (N / 2 + 1), 0.0);
    for (int k = 0; k < N; ++k)
        win[(size_t) k] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) k / (float) N);

    juce::Random rng (0x5eedbe11);   // same noise every time: the line does not flicker
    const int total = settle + frames * N;
    for (int i = 0; i < total; ++i)
    {
        const float x = (rng.nextFloat() * 2.f - 1.f) * 0.17f;   // about -20 dBFS RMS
        const float y = chain->process (x);
        if (i < settle) continue;

        const int k = (i - settle) % N;
        fx[(size_t) k] = x;
        fy[(size_t) k] = y * go;
        if (k == N - 1)
        {
            std::fill (bx.begin(), bx.end(), 0.f);
            std::fill (by.begin(), by.end(), 0.f);
            for (int j = 0; j < N; ++j) { bx[(size_t) j] = fx[(size_t) j] * win[(size_t) j]; by[(size_t) j] = fy[(size_t) j] * win[(size_t) j]; }
            fft.performFrequencyOnlyForwardTransform (bx.data());
            fft.performFrequencyOnlyForwardTransform (by.data());
            for (int j = 0; j <= N / 2; ++j)
            {
                px[(size_t) j] += (double) bx[(size_t) j] * bx[(size_t) j];
                py[(size_t) j] += (double) by[(size_t) j] * by[(size_t) j];
            }
        }
    }

    // 1/6-octave smoothing: sum the power in a band around each display frequency
    const double binHz = sr / N, band = std::pow (2.0, 1.0 / 12.0);
    for (size_t f = 0; f < freqs.size(); ++f)
    {
        const double fc = freqs[f];
        int k0 = (int) std::ceil (fc / band / binHz), k1 = (int) std::floor (fc * band / binHz);
        k0 = juce::jlimit (1, N / 2, k0);
        k1 = juce::jlimit (1, N / 2, k1);
        double ratio;
        if (k1 - k0 >= 2)
        {
            double sx = 0.0, sy = 0.0;
            for (int k = k0; k <= k1; ++k) { sx += px[(size_t) k]; sy += py[(size_t) k]; }
            ratio = sy / juce::jmax (1.0e-12, sx);
        }
        else
        {
            // low frequencies: too few bins in the band, so interpolate between two bins (no steps)
            const double kf = juce::jlimit (1.0, (double) (N / 2 - 1), fc / binHz);
            const int ka = (int) kf;
            const double frac = kf - ka;
            auto r = [&] (int k) { return (py[(size_t) k] + py[(size_t) (k + 1)] + py[(size_t) (k - 1)])
                                          / juce::jmax (1.0e-12, px[(size_t) k] + px[(size_t) (k + 1)] + px[(size_t) (k - 1)]); };
            ratio = r (ka) * (1.0 - frac) + r (juce::jmin (ka + 1, N / 2 - 1)) * frac;
        }
        db[f] = (float) (10.0 * std::log10 (juce::jmax (1.0e-12, ratio)));
    }
    return true;
}

//==============================================================================
std::vector<float> BerryToneAudioProcessor::chainKey()
{
    std::vector<float> key;
    for (auto* prm : getParameters()) key.push_back (prm->getValue());
    for (auto& o : order) key.push_back ((float) o.load());
    key.push_back (pureDi.load() ? 1.f : 0.f);
    return key;
}

std::unique_ptr<BerryToneAudioProcessor::OfflineChain> BerryToneAudioProcessor::makeOfflineChain (double sr) const
{
    auto c = std::make_unique<OfflineChain>();
    c->t.prepare (sr); c->ga.prepare (sr); c->ch.prepare (sr); c->oc.prepare (sr);
    c->im.prepare (sr); c->ti.prepare (sr); c->de.prepare (sr);
    c->t.set  (p.tightFreq->load(), p.tightAmt->load());
    c->ga.set (p.gateThresh->load(), p.gateRel->load(), p.gateDepth->load());
    c->ch.set (p.chugPunch->load(), p.chugBody->load(), p.chugSpeed->load());
    c->oc.set (p.octBlend->load(), p.octTone->load(), p.octTrack->load());
    c->im.set (p.impFreq->load(), p.impPeak->load(), p.impLoad->load());
    c->ti.set (p.tiltPivot->load(), p.tiltAmt->load());
    c->de.set (p.destDrive->load(), p.destCrush->load(), p.destMix->load());

    c->en[0] = p.tightOn->load() > 0.5f; c->en[1] = p.gateOn->load() > 0.5f; c->en[2] = p.chugOn->load() > 0.5f;
    c->en[3] = p.octOn->load()   > 0.5f; c->en[4] = p.impOn->load()  > 0.5f; c->en[5] = p.tiltOn->load() > 0.5f;
    c->en[6] = p.destOn->load()  > 0.5f;
    for (size_t i = 0; i < c->ord.size(); ++i) c->ord[i] = order[i].load();
    c->gi = berry::dbToLin (p.inGain->load());
    c->go = berry::dbToLin (p.outGain->load());
    return c;
}

float BerryToneAudioProcessor::OfflineChain::process (float x)
{
    float s[berry::kMaxCh] = { x * gi, 0.f };
    for (int slot = 0; slot < numModules; ++slot)
    {
        const int id = ord[(size_t) slot];
        if (! en[id]) continue;
        switch (id)
        {
            case 0: t.process (s, 1);  break;
            case 1: ga.process (s, 1); break;
            case 2: ch.process (s, 1); break;
            case 3: oc.process (s, 1); break;
            case 4: im.process (s, 1); break;
            case 5: ti.process (s, 1); break;
            case 6: de.process (s, 1); break;
            default: break;
        }
    }
    return s[0];
}

void BerryToneAudioProcessor::readRaw (std::vector<float>& diTarget, std::vector<float>* outTarget, int numSamples) const
{
    numSamples = juce::jlimit (1, rawRing, numSamples);
    diTarget.resize ((size_t) numSamples);
    if (outTarget != nullptr) outTarget->resize ((size_t) numSamples);
    const int w = rawWrite.load (std::memory_order_acquire);
    for (int i = 0; i < numSamples; ++i)
    {
        const auto idx = (size_t) ((w - numSamples + i + rawRing) & (rawRing - 1));
        diTarget[(size_t) i] = rawBuf[idx].load (std::memory_order_relaxed);
        if (outTarget != nullptr)
            (*outTarget)[(size_t) i] = rawOutBuf[idx].load (std::memory_order_relaxed);
    }
}

void BerryToneAudioProcessor::processHeld (const std::vector<float>& raw, double sr, int warmUp, std::vector<float>& outTarget) const
{
    const int n = (int) raw.size();
    warmUp = juce::jlimit (0, n, warmUp);
    outTarget.resize ((size_t) (n - warmUp));

    if (pureDi.load())
    {
        std::copy (raw.begin() + warmUp, raw.end(), outTarget.begin());
        return;
    }
    auto chain = makeOfflineChain (sr);
    for (int i = 0; i < warmUp; ++i)          // the filters and envelopes settle first
        chain->process (raw[(size_t) i]);
    for (int i = warmUp; i < n; ++i)
        outTarget[(size_t) (i - warmUp)] = chain->process (raw[(size_t) i]) * chain->go;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BerryToneAudioProcessor();
}
