#include "ScopeView.h"
#include "Theme.h"

using F = ThemeLookAndFeel;

static constexpr float dbTop = 18.f, dbBot = -24.f;      // FLAT LINE range
static constexpr float specTop = 0.f, specBot = -72.f;   // SPECTRUM range (dBFS)
static constexpr float levelFloorDb = -60.f;             // REAL DI dB scale: centre line

ScopeView::ScopeView (BerryToneAudioProcessor& p) : proc (p)
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);

    const int n = 240;
    for (int i = 0; i < n; ++i)
        freqs.push_back (20.f * std::pow (1000.f, (float) i / (float) (n - 1)));   // 20 Hz .. 20 kHz, log
    resp.assign (freqs.size(), 0.f);

    proc.readScope (cols);
    startTimerHz (30);
}

//==============================================================================
juce::Rectangle<int> ScopeView::chipBounds (Design d, int index)
{
    // TIME: 1 s, 200 ms, 50 ms   LEVEL: LIN, dB
    if (d == Design::grape)
    {
        static const juce::Rectangle<int> r[] = { { 462, 9, 28, 15 }, { 492, 9, 44, 15 }, { 538, 9, 38, 15 },
                                                  { 627, 9, 28, 15 }, { 657, 9, 24, 15 } };
        return r[juce::jlimit (0, 4, index)];
    }
    static const juce::Rectangle<int> r[] = { { 504, 5, 30, 17 }, { 534, 5, 46, 17 }, { 580, 5, 40, 17 },
                                              { 626, 5, 30, 17 }, { 656, 5, 26, 17 } };
    return r[juce::jlimit (0, 4, index)];
}

double ScopeView::windowSec() const
{
    return zoom == Zoom::ms50 ? 0.05 : (zoom == Zoom::ms200 ? 0.2 : (double) BerryToneAudioProcessor::scopeWindowSec);
}

//==============================================================================
void ScopeView::setMode (Mode m)
{
    mode = m;
    specDi.clear(); specOut.clear();
    resetLevel = true;
    if (mode == Mode::flat)
        proc.computeResponse (freqs, resp, true);
    else if (hold)
        rebuild (heldDi, heldOut, false, false);
    else
        refreshNow();
    repaint();
}

void ScopeView::setZoom (Zoom z)
{
    zoom = z;
    resetLevel = true;
    if (hold) rebuild (heldDi, heldOut, false, false);
    else      refreshNow();
    repaint();
}

// Reads the live signal at once (a click should not wait for the next timer tick).
void ScopeView::refreshNow()
{
    if (mode == Mode::real && zoom == Zoom::s1)
    {
        proc.readScope (cols);
        updateLevel (false);
        resetLevel = false;
    }
    else if (mode != Mode::flat)
    {
        readLive();
        rebuild (liveDi, liveOut, false, false);
    }
}

void ScopeView::setLevel (Level l)
{
    level = l;
    resetLevel = true;
    repaint();
}

void ScopeView::setHold (bool h)
{
    hold = h;
    specDi.clear(); specOut.clear();
    if (hold)
        captureHeld();
    else
        heldRaw.clear();
    repaint();
}

//==============================================================================
void ScopeView::captureHeld()
{
    sr = proc.getScopeSampleRate();
    heldWarm = juce::roundToInt (sr * 0.3);   // 300 ms for the filters and envelopes to settle
    proc.readRaw (heldRaw, nullptr, heldWarm + juce::roundToInt (sr * BerryToneAudioProcessor::scopeWindowSec));
    heldDi.assign (heldRaw.begin() + heldWarm, heldRaw.end());
    processHeldOut();
    rebuild (heldDi, heldOut, false, false);
}

void ScopeView::processHeldOut()
{
    heldKey = proc.chainKey();
    proc.processHeld (heldRaw, sr, heldWarm, heldOut);
}

void ScopeView::readLive()
{
    sr = proc.getScopeSampleRate();
    int n = 0;
    if (mode == Mode::spectrum)
        n = 1 << (sr > 105000.0 ? 15 : (sr > 53000.0 ? 14 : 13));
    else
        n = juce::roundToInt (sr * windowSec()) + juce::roundToInt (sr * 0.035);   // + trigger search
    proc.readRaw (liveDi, &liveOut, n);
}

// Builds the screen data from DI and OUT samples (live or held).
void ScopeView::rebuild (const std::vector<float>& di, const std::vector<float>& out, bool smoothLevel, bool smoothSpectrum)
{
    if (di.empty() || out.size() != di.size())
        return;

    if (mode == Mode::spectrum)
    {
        computeSpectrum (di, specDi, smoothSpectrum);
        computeSpectrum (out, specOut, smoothSpectrum);
        return;
    }
    if (mode != Mode::real)
        return;

    const int total = (int) di.size();
    const int span = juce::jmin (total, juce::roundToInt (sr * windowSec()));
    if (zoom == Zoom::s1)
    {
        buildCols (di, out, total - span, span);
    }
    else
    {
        // trigger: start the window on a rising zero crossing, so the wave stands still
        const int search = juce::jmin (total - span, juce::roundToInt (sr * 0.035));
        const int base = total - span - search;
        buildCols (di, out, base + findTrigger (di, base, search), span);
    }
    updateLevel (smoothLevel && ! resetLevel);
    resetLevel = false;
}

void ScopeView::buildCols (const std::vector<float>& di, const std::vector<float>& out, int start, int count)
{
    const int nCols = BerryToneAudioProcessor::scopeCols;
    cols.resize ((size_t) nCols);
    count = juce::jmax (1, count);
    const int last = (int) di.size() - 1;
    for (int c = 0; c < nCols; ++c)
    {
        const int i0 = juce::jmin (last, start + (int) ((juce::int64) c * count / nCols));
        const int i1 = juce::jmax (i0 + 1, start + (int) ((juce::int64) (c + 1) * count / nCols));
        auto& col = cols[(size_t) c];
        col = { di[(size_t) i0], di[(size_t) i0], out[(size_t) i0], out[(size_t) i0] };
        for (int i = i0 + 1; i < i1 && i <= last; ++i)
        {
            col.diMin  = juce::jmin (col.diMin,  di[(size_t) i]);  col.diMax  = juce::jmax (col.diMax,  di[(size_t) i]);
            col.outMin = juce::jmin (col.outMin, out[(size_t) i]); col.outMax = juce::jmax (col.outMax, out[(size_t) i]);
        }
    }
}

int ScopeView::findTrigger (const std::vector<float>& di, int base, int search) const
{
    if (search <= 1) return 0;
    // low-pass the DI (about 300 Hz): the trigger follows the fundamental, not the harmonics
    const float a = std::exp (-2.f * juce::MathConstants<float>::pi * 300.f / (float) sr);
    float pk = 0.f;
    for (int i = 0; i <= search; ++i) pk = juce::jmax (pk, std::abs (di[(size_t) (base + i)]));
    const float thr = 0.05f * pk;

    float s = di[(size_t) base], prev = s;
    bool armed = false;
    int trig = search;   // no crossing: show the newest samples
    for (int i = 1; i <= search; ++i)
    {
        const float x = di[(size_t) (base + i)];
        s = x + a * (s - x);
        if (s < -thr) armed = true;
        if (armed && prev < 0.f && s >= 0.f) { trig = i; armed = false; }
        prev = s;
    }
    return trig;
}

void ScopeView::computeSpectrum (const std::vector<float>& x, std::vector<float>& dbOut, bool smooth)
{
    const int order = sr > 105000.0 ? 15 : (sr > 53000.0 ? 14 : 13);
    const int N = 1 << order;
    if (fft == nullptr || fftOrder != order)
    {
        fft = std::make_unique<juce::dsp::FFT> (order);
        fftOrder = order;
        fftWin.resize ((size_t) N);
        for (int k = 0; k < N; ++k)
            fftWin[(size_t) k] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) k / (float) N);
    }
    fftBuf.assign ((size_t) (2 * N), 0.f);
    const int n = (int) x.size(), off = n - N;   // the newest N samples (zeros in front if fewer)
    for (int k = 0; k < N; ++k)
        if (off + k >= 0) fftBuf[(size_t) k] = x[(size_t) (off + k)] * fftWin[(size_t) k];
    fft->performFrequencyOnlyForwardTransform (fftBuf.data());

    // a full-scale sine reads 0 dB: its Hann main lobe holds (N/4)^2 * 1.5 of power
    const double normPow = std::pow (N / 4.0, 2.0) * 1.5;
    auto pw = [&] (int k) { const double m = fftBuf[(size_t) juce::jlimit (0, N / 2, k)]; return m * m; };

    const double binHz = sr / N, band = std::pow (2.0, 1.0 / 12.0);   // 1/6 octave
    const bool keep = smooth && dbOut.size() == freqs.size();
    if (! keep) dbOut.assign (freqs.size(), specBot);
    for (size_t f = 0; f < freqs.size(); ++f)
    {
        const double fc = freqs[f];
        int k0 = (int) std::ceil (fc / band / binHz), k1 = (int) std::floor (fc * band / binHz);
        k0 = juce::jlimit (1, N / 2, k0);
        k1 = juce::jlimit (1, N / 2, k1);
        double p;
        if (k1 - k0 >= 2)
        {
            p = 0.0;
            for (int k = k0; k <= k1; ++k) p += pw (k);
        }
        else
        {
            // low frequencies: few bins per band, so interpolate 3-bin sums (no steps)
            const double kf = juce::jlimit (1.0, (double) (N / 2 - 2), fc / binHz);
            const int ka = (int) kf;
            const double frac = kf - ka;
            auto s3 = [&] (int k) { return pw (k - 1) + pw (k) + pw (k + 1); };
            p = s3 (ka) * (1.0 - frac) + s3 (ka + 1) * frac;
        }
        const float v = (float) (10.0 * std::log10 (juce::jmax (1.0e-12, p / normPow)));
        if (keep)
        {
            auto& o = dbOut[f];
            o += (v - o) * (v > o ? 0.6f : 0.2f);   // fast up, slow down
        }
        else
            dbOut[f] = v;
    }
}

void ScopeView::updateLevel (bool smooth)
{
    float pk = 0.f, inPk = 0.f;
    for (auto& c : cols)
    {
        pk = juce::jmax (pk, -c.diMin, c.diMax, juce::jmax (-c.outMin, c.outMax));
        inPk = juce::jmax (inPk, -c.diMin, c.diMax);
    }
    // auto level: fast up, slow down. Same scale for DI and OUT.
    const float target = juce::jmax (0.02f, pk * 1.15f);
    scale = (! smooth || target > scale) ? target : scale + (target - scale) * 0.03f;
    inPeakDb = juce::Decibels::gainToDecibels (inPk, -100.f);
}

void ScopeView::timerCallback()
{
    ++tick;

    if (mode == Mode::flat)
    {
        if ((tick & 1) == 0 && proc.computeResponse (freqs, resp))   // 15 updates per second
            repaint();
        return;
    }

    if (hold)
    {
        // the DI stays frozen; the modules may change: render the OUT line again
        if ((tick & 1) == 0 && proc.chainKey() != heldKey)
        {
            processHeldOut();
            rebuild (heldDi, heldOut, true, false);
            repaint();
        }
        return;
    }

    if (mode == Mode::real)
    {
        if (zoom == Zoom::s1)
        {
            proc.readScope (cols);
            updateLevel (! resetLevel);
            resetLevel = false;
        }
        else
        {
            readLive();
            rebuild (liveDi, liveOut, true, false);
        }
        repaint();
    }
    else if ((tick & 1) == 0)   // spectrum: 15 updates per second
    {
        readLive();
        rebuild (liveDi, liveOut, false, true);
        repaint();
    }
}

//==============================================================================
void ScopeView::drawBox (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, juce::Justification j, juce::Colour c)
{
    g.setColour (juce::Colour (0xd91d0b1b));
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (grapecol::borderDim);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);
    g.setColour (c);
    g.drawFittedText (text, r.reduced (7.f, 2.f).toNearestInt(), j, 2, 0.9f);
}

void ScopeView::paintChipGroups (juce::Graphics& g)
{
    // backgrounds and labels of the TIME / LEVEL button groups (the buttons are editor children)
    const auto t = chipBounds (design, 0).getUnion (chipBounds (design, 2)).toFloat();
    const auto l = chipBounds (design, 3).getUnion (chipBounds (design, 4)).toFloat();
    if (design == Design::grape)
    {
        const auto tb = juce::Rectangle<float> (424.f, 6.f, 154.f, 21.f), lb = juce::Rectangle<float> (583.f, 6.f, 102.f, 21.f);
        for (auto r : { tb, lb })
        {
            g.setColour (juce::Colour (0xd91d0b1b));
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (grapecol::borderDim);
            g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);
        }
        g.setColour (grapecol::muted);
        g.setFont (F::font (9.f, true, 0.06f));
        g.drawText ("TIME",  juce::Rectangle<float> (tb.getX() + 2.f, tb.getY(), t.getX() - tb.getX() - 2.f, tb.getHeight()), juce::Justification::centred);
        g.drawText ("LEVEL", juce::Rectangle<float> (lb.getX() + 2.f, lb.getY(), l.getX() - lb.getX() - 2.f, lb.getHeight()), juce::Justification::centred);
    }
    else
    {
        g.setColour (berrycol::cream.withAlpha (0.12f));
        g.fillRoundedRectangle (t, t.getHeight() * 0.5f);
        g.fillRoundedRectangle (l, l.getHeight() * 0.5f);
    }
}

void ScopeView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool grape = design == Design::grape;

    // screen
    g.setColour (grape ? grapecol::screen : berrycol::deep);
    g.fillRoundedRectangle (b, grape ? 8.f : 9.f);

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (b.reduced (grape ? 2.f : 0.f), grape ? 6.f : 9.f);
        g.reduceClipRegion (clip);
        if (mode == Mode::real)          paintReal (g, b);
        else if (mode == Mode::spectrum) paintSpectrum (g, b);
        else                             paintFlat (g, b);

        // inner shadow
        for (int i = 0; i < 6; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (grape ? 0.10f : 0.07f));
            g.drawRoundedRectangle (b.reduced ((float) i), grape ? 8.f : 9.f, 1.f);
        }
    }

    if (grape)
    {
        g.setColour (grapecol::borderDim);
        g.drawRoundedRectangle (b.reduced (1.f), 8.f, 2.f);
    }

    // HOLD label (Berry; Grape has the lit HOLD button on the screen)
    if (hold && mode != Mode::flat && ! grape)
    {
        g.setFont (F::font (10.f));
        g.setColour (berrycol::mint);
        g.drawText ("HOLD: DI FROZEN, OUT LIVE", (int) b.getRight() - 200, (int) b.getBottom() - 22, 190, 14, juce::Justification::centredRight);
    }

    // PURE DI banner
    if (proc.pureDi.load())
    {
        const juce::String t = grape ? "PURE DI  -  ALL MODULES MUTED" : "PURE DI: ALL MODULES MUTED";
        g.setFont (F::font (10.f, true, grape ? 0.08f : 0.f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 20.f;
        if (grape)
        {
            auto r = juce::Rectangle<float> (b.getCentreX() - tw * 0.5f, b.getBottom() - 26.f, tw, 18.f);
            g.setColour (grapecol::glow.withAlpha (0.25f));
            g.fillRoundedRectangle (r.expanded (3.f), 6.f);
            g.setColour (juce::Colour (0xe651204c));
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (grapecol::accent);
            g.drawRoundedRectangle (r, 4.f, 1.f);
            g.setColour (grapecol::text);
            g.drawText (t, r, juce::Justification::centred);
        }
        else
        {
            auto r = juce::Rectangle<float> (b.getX() + 10.f, b.getBottom() - 25.f, tw, 17.f);
            g.setColour (berrycol::pink);
            g.fillRoundedRectangle (r, 8.f);
            g.setColour (berrycol::cream);
            g.drawText (t, r, juce::Justification::centred);
        }
    }
}

void ScopeView::paintReal (juce::Graphics& g, juce::Rectangle<float> b)
{
    const bool grape = design == Design::grape, db = level == Level::db;
    const float w = b.getWidth(), h = b.getHeight(), mid = b.getCentreY(), half = h * 0.5f - 6.f;
    const auto gridC = grape ? grapecol::panelTop : berrycol::cream.withAlpha (0.09f);
    const auto textC = grape ? grapecol::dim : berrycol::cream.withAlpha (0.5f);

    // grid: 10 time divisions; LIN: 4 rows, dB: -12 / -24 / -48 dB on both sides
    g.setColour (gridC);
    for (int i = 1; i < 10; ++i) g.drawVerticalLine (juce::roundToInt (b.getX() + w * (float) i / 10.f), b.getY(), b.getBottom());
    auto yOfDb = [&] (float d) { return juce::jmax (0.f, (d - levelFloorDb) / -levelFloorDb) * half; };
    if (db)
    {
        g.setFont (F::font (9.f));
        for (float d : { -12.f, -24.f, -48.f })
        {
            const float y = yOfDb (d);
            g.setColour (gridC);
            g.drawHorizontalLine (juce::roundToInt (mid - y), b.getX(), b.getRight());
            g.drawHorizontalLine (juce::roundToInt (mid + y), b.getX(), b.getRight());
            g.setColour (textC);
            g.drawText (juce::String ((int) d) + " dB", (int) b.getX() + (grape ? 78 : 8), (int) (mid + y) + 1, 50, 11, juce::Justification::centredLeft);   // Grape: right of the INPUT box
        }
    }
    else
    {
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine (juce::roundToInt (b.getY() + h * (float) i / 4.f), b.getX(), b.getRight());
    }
    g.setColour (grape ? grapecol::border : berrycol::cream.withAlpha (0.25f));
    g.drawHorizontalLine (juce::roundToInt (mid), b.getX(), b.getRight());

    if (! cols.empty())
    {
        const float amp = half / juce::jmax (0.02f, scale);
        auto yOf = [&] (float v)
        {
            if (! db) return mid - v * amp;
            const float d = juce::Decibels::gainToDecibels (std::abs (v), -100.f);
            return mid - (v < 0.f ? -1.f : 1.f) * yOfDb (d);
        };
        auto buildPath = [&] (bool out)
        {
            juce::Path p;
            const int n = (int) cols.size();
            for (int i = 0; i < n; ++i)
            {
                const auto& c = cols[(size_t) i];
                const float x = b.getX() + w * (float) i / (float) (n - 1);
                const float lo = out ? c.outMin : c.diMin, hi = out ? c.outMax : c.diMax;
                const float y1 = yOf (i & 1 ? lo : hi), y2 = yOf (i & 1 ? hi : lo);
                if (i == 0) p.startNewSubPath (x, y1); else p.lineTo (x, y1);
                p.lineTo (x, y2);
            }
            return p;
        };

        const auto di = buildPath (false), out = buildPath (true);
        g.setColour (grape ? juce::Colour (0xcc7a5a78) : berrycol::cream.withAlpha (0.38f));
        g.strokePath (di, juce::PathStrokeType (1.1f));
        if (grape)
        {
            F::drawGlow (g, out, grapecol::glow, 2.2f);
            g.setColour (grapecol::trace);
            g.strokePath (out, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
        }
        else
        {
            g.setColour (berrycol::pink);
            g.strokePath (out, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
        }
    }

    // labels
    const juce::String div = zoom == Zoom::ms50 ? "5 ms/div" : (zoom == Zoom::ms200 ? "20 ms/div" : "100 ms/div");
    const juce::String info = div + (db ? "  -  0 / -60 dB" : "  -  AUTO");
    paintChipGroups (g);
    if (grape)
    {
        auto box = juce::Rectangle<float> (b.getX() + 8.f, b.getY() + 7.f, 112.f, 20.f);
        g.setColour (juce::Colour (0xd91d0b1b));
        g.fillRoundedRectangle (box, 4.f);
        g.setColour (grapecol::borderDim);
        g.drawRoundedRectangle (box.reduced (0.5f), 4.f, 1.f);
        F::drawGrapeIcon (g, { box.getX() + 8.f, box.getY() + 4.f, 11.f, 12.f }, grapecol::trace);
        g.setColour (grapecol::trace);
        g.setFont (F::font (11.f, true, 0.06f));
        g.drawText ("DI GUITAR", box.withTrimmedLeft (26.f), juce::Justification::centredLeft);

        g.setFont (F::font (9.5f, true, 0.06f));
        g.setColour (grapecol::muted);
        g.drawText ("DI", (int) box.getRight() + 10, (int) box.getY(), 20, 20, juce::Justification::centredLeft);
        g.setColour (grapecol::trace);
        g.drawText ("OUT", (int) box.getRight() + 28, (int) box.getY(), 30, 20, juce::Justification::centredLeft);
        g.setColour (grapecol::dim);
        g.setFont (F::font (9.f, true, 0.04f));
        g.drawText (info, 250, (int) box.getY(), 168, 20, juce::Justification::centredRight);

        if (! proc.pureDi.load())
            drawBox (g, { b.getX() + 8.f, b.getBottom() - 34.f, 62.f, 27.f },
                     "INPUT\n" + (inPeakDb <= -99.f ? juce::String ("-inf") : juce::String (inPeakDb, 1)) + " dB",
                     juce::Justification::centred, grapecol::label);
    }
    else
    {
        g.setFont (F::font (10.f));
        g.setColour (berrycol::cream.withAlpha (0.8f));
        g.drawText ("DI", (int) b.getX() + 10, (int) b.getY() + 5, 20, 14, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xffff7ab8));
        g.drawText ("OUT", (int) b.getX() + 32, (int) b.getY() + 5, 30, 14, juce::Justification::centredLeft);
        g.setColour (berrycol::cream.withAlpha (0.7f));
        g.setFont (F::font (9.5f));
        g.drawText (info, 300, (int) b.getY() + 6, 198, 14, juce::Justification::centredRight);
    }
}

void ScopeView::paintSpectrum (juce::Graphics& g, juce::Rectangle<float> b)
{
    const bool grape = design == Design::grape;
    const float w = b.getWidth(), h = b.getHeight();
    const float lo = std::log10 (20.f), hi = std::log10 (20000.f);
    auto xOf = [&] (float f) { return b.getX() + (std::log10 (f) - lo) / (hi - lo) * w; };
    auto yOf = [&] (float d) { return b.getY() + 8.f + (specTop - juce::jlimit (specBot, specTop, d)) / (specTop - specBot) * (h - 26.f); };

    const auto gridC = grape ? grapecol::panelTop : berrycol::cream.withAlpha (0.09f);
    const auto textC = grape ? grapecol::dim : berrycol::cream.withAlpha (0.5f);
    g.setFont (F::font (9.f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xOf (f);
        g.setColour (gridC);
        g.drawVerticalLine (juce::roundToInt (x), b.getY(), b.getBottom());
        g.setColour (textC);
        g.drawText (f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f),
                    (int) x + 3, (int) b.getBottom() - 16, 30, 12, juce::Justification::centredLeft);
    }
    for (float d : { -24.f, -48.f })
    {
        g.setColour (gridC);
        g.drawHorizontalLine (juce::roundToInt (yOf (d)), b.getX(), b.getRight());
        g.setColour (textC);
        g.drawText (juce::String ((int) d) + " dB", (int) b.getX() + 8, (int) yOf (d) + 2, 44, 12, juce::Justification::centredLeft);
    }

    if (specDi.size() == freqs.size() && specOut.size() == freqs.size())
    {
        juce::Path area, diLine, outLine;
        area.startNewSubPath (b.getX(), b.getBottom());
        for (size_t i = 0; i < freqs.size(); ++i)
        {
            const float x = xOf (freqs[i]), yd = yOf (specDi[i]), yo = yOf (specOut[i]);
            area.lineTo (x, yd);
            if (i == 0) { diLine.startNewSubPath (x, yd); outLine.startNewSubPath (x, yo); }
            else        { diLine.lineTo (x, yd);          outLine.lineTo (x, yo); }
        }
        area.lineTo (b.getRight(), b.getBottom());
        area.closeSubPath();

        g.setColour (grape ? juce::Colour (0x387a5a78) : berrycol::cream.withAlpha (0.10f));
        g.fillPath (area);
        g.setColour (grape ? juce::Colour (0xd97a5a78) : berrycol::cream.withAlpha (0.42f));
        g.strokePath (diLine, juce::PathStrokeType (1.1f));
        if (grape)
        {
            F::drawGlow (g, outLine, grapecol::glow, 2.4f);
            g.setColour (grapecol::trace);
            g.strokePath (outLine, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            g.setColour (berrycol::pink);
            g.strokePath (outLine, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    if (grape)
    {
        auto box = juce::Rectangle<float> (b.getX() + 8.f, b.getY() + 7.f, 150.f, 20.f);
        g.setColour (juce::Colour (0xd91d0b1b));
        g.fillRoundedRectangle (box, 4.f);
        g.setColour (grapecol::borderDim);
        g.drawRoundedRectangle (box.reduced (0.5f), 4.f, 1.f);
        F::drawGrapeIcon (g, { box.getX() + 8.f, box.getY() + 4.f, 11.f, 12.f }, grapecol::trace);
        g.setColour (grapecol::trace);
        g.setFont (F::font (11.f, true, 0.06f));
        g.drawText ("SPECTRUM  DI / OUT", box.withTrimmedLeft (26.f), juce::Justification::centredLeft);
        drawBox (g, { b.getRight() - 150.f, b.getY() + 7.f, 142.f, 20.f },
                 "DI vs OUT  -  0 / -72 dB", juce::Justification::centredLeft, grapecol::label);
    }
    else
    {
        g.setFont (F::font (10.f));
        g.setColour (berrycol::cream.withAlpha (0.8f));
        g.drawText ("DI", (int) b.getX() + 10, (int) b.getY() + 5, 20, 14, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xffff7ab8));
        g.drawText ("OUT", (int) b.getX() + 32, (int) b.getY() + 5, 30, 14, juce::Justification::centredLeft);
        g.setColour (berrycol::cream.withAlpha (0.7f));
        g.drawText ("DI vs OUT  -  0 / -72 dB", (int) b.getRight() - 230, (int) b.getY() + 5, 220, 14, juce::Justification::centredRight);
    }
}

void ScopeView::paintFlat (juce::Graphics& g, juce::Rectangle<float> b)
{
    const bool grape = design == Design::grape;
    const float w = b.getWidth(), h = b.getHeight();
    const float lo = std::log10 (20.f), hi = std::log10 (20000.f);
    auto xOf = [&] (float f) { return b.getX() + (std::log10 (f) - lo) / (hi - lo) * w; };
    auto yOf = [&] (float db) { return b.getY() + 12.f + (dbTop - juce::jlimit (dbBot, dbTop, db)) / (dbTop - dbBot) * (h - 30.f); };

    const auto gridC = grape ? grapecol::panelTop : berrycol::cream.withAlpha (0.09f);
    const auto textC = grape ? grapecol::dim : berrycol::cream.withAlpha (0.5f);
    g.setFont (F::font (9.f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xOf (f);
        g.setColour (gridC);
        g.drawVerticalLine (juce::roundToInt (x), b.getY(), b.getBottom());
        g.setColour (textC);
        g.drawText (f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f),
                    (int) x + 3, (int) b.getBottom() - 16, 30, 12, juce::Justification::centredLeft);
    }
    for (float d : { 12.f, -12.f })
    {
        g.setColour (gridC);
        g.drawHorizontalLine (juce::roundToInt (yOf (d)), b.getX(), b.getRight());
        g.setColour (textC);
        g.drawText ((d > 0 ? "+" : "") + juce::String ((int) d) + " dB", (int) b.getX() + 8, (int) yOf (d) + 2, 40, 12, juce::Justification::centredLeft);
    }
    // 0 dB: the flat line of the test signal
    juce::Path zero;
    zero.startNewSubPath (b.getX(), yOf (0.f));
    zero.lineTo (b.getRight(), yOf (0.f));
    juce::Path dashed;
    const float dashes[] = { 4.f, 4.f };
    juce::PathStrokeType (1.f).createDashedStroke (dashed, zero, dashes, 2);
    g.setColour (grape ? grapecol::border : berrycol::cream.withAlpha (0.28f));
    g.fillPath (dashed);

    juce::Path curve;
    for (size_t i = 0; i < freqs.size(); ++i)
    {
        const float x = xOf (freqs[i]), y = yOf (resp[i]);
        if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    if (grape)
    {
        F::drawGlow (g, curve, grapecol::glow, 2.6f);
        g.setColour (grapecol::trace);
        g.strokePath (curve, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto box = juce::Rectangle<float> (b.getX() + 8.f, b.getY() + 7.f, 122.f, 20.f);
        g.setColour (juce::Colour (0xd91d0b1b));
        g.fillRoundedRectangle (box, 4.f);
        g.setColour (grapecol::borderDim);
        g.drawRoundedRectangle (box.reduced (0.5f), 4.f, 1.f);
        F::drawGrapeIcon (g, { box.getX() + 8.f, box.getY() + 4.f, 11.f, 12.f }, grapecol::trace);
        g.setColour (grapecol::trace);
        g.setFont (F::font (11.f, true, 0.06f));
        g.drawText ("MODULE CURVE", box.withTrimmedLeft (26.f), juce::Justification::centredLeft);
        drawBox (g, { b.getRight() - 120.f, b.getY() + 7.f, 112.f, 30.f },
                 "TEST: FLAT LINE\n20 Hz - 20 kHz", juce::Justification::centredLeft, grapecol::label);
    }
    else
    {
        g.setColour (berrycol::pink);
        g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setFont (F::font (10.f));
        g.setColour (juce::Colour (0xffff7ab8));
        g.drawText ("MODULE CURVE", (int) b.getX() + 10, (int) b.getY() + 5, 120, 14, juce::Justification::centredLeft);
        g.setColour (berrycol::cream.withAlpha (0.7f));
        g.drawText ("TEST SIGNAL: FLAT  -  +18 / -24 dB", (int) b.getRight() - 230, (int) b.getY() + 5, 220, 14, juce::Justification::centredRight);
    }
}
