#include "ScopeView.h"
#include "Theme.h"

using F = ThemeLookAndFeel;

static constexpr float dbTop = 18.f, dbBot = -24.f;

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

void ScopeView::setMode (Mode m)
{
    mode = m;
    if (mode == Mode::flat)
        proc.computeResponse (freqs, resp, true);
    repaint();
}

void ScopeView::timerCallback()
{
    ++tick;
    if (hold) return;

    if (mode == Mode::real)
    {
        proc.readScope (cols);
        float pk = 0.f, inPk = 0.f;
        for (auto& c : cols)
        {
            pk = juce::jmax (pk, -c.diMin, c.diMax, juce::jmax (-c.outMin, c.outMax));
            inPk = juce::jmax (inPk, -c.diMin, c.diMax);
        }
        // auto level: fast up, slow down. Same scale for DI and OUT.
        const float target = juce::jmax (0.02f, pk * 1.15f);
        scale = target > scale ? target : scale + (target - scale) * 0.03f;
        inPeakDb = juce::Decibels::gainToDecibels (inPk, -100.f);
        repaint();
    }
    else if ((tick & 1) == 0)   // 15 updates per second are enough for the curve
    {
        if (proc.computeResponse (freqs, resp))   // PURE DI is part of the change check
            repaint();
    }
}

void ScopeView::drawBox (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, juce::Justification j, juce::Colour c)
{
    g.setColour (juce::Colour (0xd91d0b1b));
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (grapecol::borderDim);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);
    g.setColour (c);
    g.drawFittedText (text, r.reduced (7.f, 2.f).toNearestInt(), j, 2, 0.9f);
}

void ScopeView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool grape = design == Design::grape;

    // screen
    if (grape)
    {
        g.setColour (grapecol::screen);
        g.fillRoundedRectangle (b, 8.f);
    }
    else
    {
        g.setColour (berrycol::deep);
        g.fillRoundedRectangle (b, 9.f);
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (b.reduced (grape ? 2.f : 0.f), grape ? 6.f : 9.f);
        g.reduceClipRegion (clip);
        if (mode == Mode::real) paintReal (g, b);
        else                    paintFlat (g, b);

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
    const bool grape = design == Design::grape;
    const float w = b.getWidth(), h = b.getHeight(), mid = b.getCentreY();

    // grid: 10 x 100 ms, 4 rows
    g.setColour (grape ? grapecol::panelTop : berrycol::cream.withAlpha (0.09f));
    for (int i = 1; i < 10; ++i) g.drawVerticalLine (juce::roundToInt (b.getX() + w * (float) i / 10.f), b.getY(), b.getBottom());
    for (int i = 1; i < 4; ++i)  g.drawHorizontalLine (juce::roundToInt (b.getY() + h * (float) i / 4.f), b.getX(), b.getRight());
    g.setColour (grape ? grapecol::border : berrycol::cream.withAlpha (0.25f));
    g.drawHorizontalLine (juce::roundToInt (mid), b.getX(), b.getRight());

    if (cols.empty()) return;

    const float amp = (h * 0.5f - 6.f) / juce::jmax (0.02f, scale);
    auto buildPath = [&] (bool out)
    {
        juce::Path p;
        const int n = (int) cols.size();
        for (int i = 0; i < n; ++i)
        {
            const auto& c = cols[(size_t) i];
            const float x = b.getX() + w * (float) i / (float) (n - 1);
            const float lo = out ? c.outMin : c.diMin, hi = out ? c.outMax : c.diMax;
            const float y1 = mid - (i & 1 ? lo : hi) * amp, y2 = mid - (i & 1 ? hi : lo) * amp;
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

    // labels
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

        drawBox (g, { b.getRight() - 112.f, b.getY() + 7.f, 104.f, 30.f },
                 "TIME: 100ms/div\nLEVEL: AUTO", juce::Justification::centredLeft, grapecol::label);
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
        g.drawText ("100 ms/div  -  AUTO LEVEL", (int) b.getRight() - 170, (int) b.getY() + 5, 160, 14, juce::Justification::centredRight);
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
