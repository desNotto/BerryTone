#include "Theme.h"

using juce::Colour;
using juce::Graphics;

static juce::String styleOf (const juce::Component& c) { return c.getProperties()["style"].toString(); }

//==============================================================================
ThemeLookAndFeel::ThemeLookAndFeel()
{
    setDesign (Design::berry);
}

juce::Font ThemeLookAndFeel::font (float h, bool bold, float kerning)
{
    auto f = juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    return kerning != 0.f ? f.withExtraKerningFactor (kerning) : f;
}

void ThemeLookAndFeel::setDesign (Design d)
{
    design = d;
    using namespace juce;

    if (grape())
    {
        using namespace grapecol;
        setColour (Slider::textBoxTextColourId, text);
        setColour (Slider::textBoxHighlightColourId, glow.withAlpha (0.35f));
        setColour (Label::textColourId, text);
        setColour (TextEditor::textColourId, text);
        setColour (TextEditor::backgroundColourId, screen);
        setColour (TextEditor::outlineColourId, borderLive);
        setColour (TextEditor::focusedOutlineColourId, trace);
        setColour (TextEditor::highlightColourId, glow.withAlpha (0.35f));
        setColour (CaretComponent::caretColourId, trace);
        setColour (AlertWindow::backgroundColourId, panelTop);
        setColour (AlertWindow::textColourId, text);
        setColour (AlertWindow::outlineColourId, borderLive);
        setColour (TextButton::buttonColourId, panelTop);
        setColour (TextButton::textColourOffId, label);
        setColour (TextButton::textColourOnId, Colours::white);
        setColour (PopupMenu::backgroundColourId, Colour (0xff24101f));
        setColour (PopupMenu::textColourId, text);
        setColour (PopupMenu::headerTextColourId, accent);
        setColour (PopupMenu::highlightedBackgroundColourId, border);
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
        setColour (ProgressBar::backgroundColourId, screen);
        setColour (ProgressBar::foregroundColourId, ledGreen);
    }
    else
    {
        using namespace berrycol;
        setColour (Slider::textBoxTextColourId, ink);
        setColour (Slider::textBoxHighlightColourId, pink.withAlpha (0.3f));
        setColour (Label::textColourId, ink);
        setColour (TextEditor::textColourId, deep);
        setColour (TextEditor::backgroundColourId, Colours::white);
        setColour (TextEditor::outlineColourId, pink.withAlpha (0.4f));
        setColour (TextEditor::focusedOutlineColourId, pink);
        setColour (TextEditor::highlightColourId, pink.withAlpha (0.3f));
        setColour (CaretComponent::caretColourId, pink);
        setColour (AlertWindow::backgroundColourId, cream);
        setColour (AlertWindow::textColourId, deep);
        setColour (AlertWindow::outlineColourId, pink.withAlpha (0.5f));
        setColour (TextButton::buttonColourId, cream);
        setColour (TextButton::textColourOffId, deep);
        setColour (TextButton::textColourOnId, cream);
        setColour (PopupMenu::backgroundColourId, cream);
        setColour (PopupMenu::textColourId, deep);
        setColour (PopupMenu::headerTextColourId, pink);
        setColour (PopupMenu::highlightedBackgroundColourId, pink.withAlpha (0.18f));
        setColour (PopupMenu::highlightedTextColourId, deep);
        setColour (ProgressBar::backgroundColourId, deep.withAlpha (0.12f));
        setColour (ProgressBar::foregroundColourId, mint);
    }
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
}

//==============================================================================
juce::Font ThemeLookAndFeel::getLabelFont (juce::Label& l)
{
    const bool isName = l.getProperties()["role"].toString() == "knobName";
    if (grape())
        return isName ? font (10.5f, true, 0.08f) : font (11.f);
    return font (12.f);
}

// Parameter names / values: one colour per design, so they always read clearly
void ThemeLookAndFeel::drawLabel (Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));
    if (label.isBeingEdited()) return;
    if (label.findColour (juce::Label::textColourId).isTransparent()) return;   // hidden labels (e.g. accessibility text in dialogs)

    const bool isName = label.getProperties()["role"].toString() == "knobName";
    Colour c = grape() ? (isName ? grapecol::label : grapecol::text) : berrycol::ink;

    const auto f = getLabelFont (label);
    g.setColour (c.withMultipliedAlpha (label.isEnabled() ? 1.f : 0.5f));
    g.setFont (f);
    auto text = label.getText();
    if (grape() && isName) text = text.toUpperCase();
    const auto area = label.getBorderSize().subtractedFrom (label.getLocalBounds());
    g.drawFittedText (text, area, label.getJustificationType(),
                      juce::jmax (1, (int) ((float) area.getHeight() / f.getHeight())),
                      label.getMinimumHorizontalScale());
}

//==============================================================================
void ThemeLookAndFeel::drawBerry (Graphics& g, float cx, float cy, float r)
{
    g.setGradientFill (juce::ColourGradient (Colour (0xffff7ab8), cx - r * 0.35f, cy - r * 0.4f,
                                             Colour (0xff8a0f5c), cx + r * 0.7f, cy + r * 0.8f, true));
    g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (cx - r * 0.6f, cy - r * 0.7f, r * 0.5f, r * 0.32f);

    juce::Path leafP;
    leafP.startNewSubPath (cx, cy - r);
    leafP.quadraticTo (cx + r * 0.9f, cy - r * 1.7f, cx + r * 1.2f, cy - r * 0.9f);
    leafP.quadraticTo (cx + r * 0.4f, cy - r * 0.8f, cx, cy - r);
    g.setColour (berrycol::leaf);
    g.fillPath (leafP);
}

// Small flat grape icon (toggles, power button, labels). Fits the given box.
void ThemeLookAndFeel::drawGrapeIcon (Graphics& g, juce::Rectangle<float> r, Colour c)
{
    const float s = juce::jmin (r.getWidth() / 20.f, r.getHeight() / 22.f);
    const float ox = r.getCentreX() - 10.f * s, oy = r.getCentreY() - 11.f * s;
    g.setColour (c);
    const float pts[][2] = { { 5, 7 }, { 10, 7 }, { 15, 7 }, { 7.5f, 11.5f }, { 12.5f, 11.5f }, { 10, 16 } };
    for (auto& p : pts)
        g.fillEllipse (ox + (p[0] - 2.6f) * s, oy + (p[1] - 2.6f) * s, 5.2f * s, 5.2f * s);
    g.fillEllipse (ox + (10 - 1.8f) * s, oy + (20 - 1.8f) * s, 3.6f * s, 3.6f * s);
    juce::Path stem;
    stem.startNewSubPath (ox + 10 * s, oy + 4 * s);
    stem.quadraticTo (ox + 10 * s, oy + 1 * s, ox + 12.5f * s, oy + 0.5f * s);
    g.strokePath (stem, juce::PathStrokeType (1.4f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ThemeLookAndFeel::drawLeaf (Graphics& g, juce::Point<float> a, juce::Point<float> b, float w, Colour c)
{
    const auto d = b - a;
    const float len = d.getDistanceFromOrigin();
    if (len < 0.5f) return;
    const juce::Point<float> n (-d.y / len * w, d.x / len * w);
    const auto m = a + d * 0.5f;
    juce::Path p;
    p.startNewSubPath (a);
    p.quadraticTo (m + n, b);
    p.quadraticTo (m - n, a);
    g.setColour (c);
    g.fillPath (p);
    g.setColour (grapecol::leafDark.withAlpha (0.8f));
    g.drawLine ({ a, b }, 0.8f);
}

// Shaded grape bunch (logo + decoration). Base size = 62 x 70 at scale 1.
void ThemeLookAndFeel::drawGrapeCluster (Graphics& g, float x, float y, float s)
{
    juce::Path stem;
    stem.startNewSubPath (x + 31 * s, y + 14 * s);
    stem.quadraticTo (x + 32 * s, y + 4 * s, x + 40 * s, y + 2 * s);
    g.setColour (Colour (0xff6b8f3a));
    g.strokePath (stem, juce::PathStrokeType (3.f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    drawLeaf (g, { x + 33 * s, y + 10 * s }, { x + 56 * s, y + 8 * s }, 7.f * s, grapecol::leaf);

    const float berries[][3] = { { 16, 22, 8 }, { 31, 20, 8 }, { 46, 22, 8 }, { 23, 35, 8 }, { 39, 35, 8 }, { 31, 48, 8 }, { 31, 61, 6 } };
    for (auto& b : berries)
    {
        const float cx = x + b[0] * s, cy = y + b[1] * s, r = b[2] * s;
        g.setGradientFill (juce::ColourGradient (Colour (0xffe9a6ff), cx - r * 0.4f, cy - r * 0.5f,
                                                 Colour (0xff6a1f7a), cx + r * 0.8f, cy + r * 0.9f, true));
        g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillEllipse (cx - r * 0.55f, cy - r * 0.55f, r * 0.42f, r * 0.42f);
    }
}

void ThemeLookAndFeel::drawGlow (Graphics& g, const juce::Path& p, Colour c, float radius)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (c.withAlpha (0.10f));
        g.strokePath (p, juce::PathStrokeType (radius * (float) i * 0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
void ThemeLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float sa, float ea, juce::Slider&)
{
    const float ang = sa + pos * (ea - sa);

    if (! grape())
    {
        using namespace berrycol;
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float d = juce::jmin (area.getWidth(), area.getHeight());
        const auto c = area.getCentre();
        const float rad = d * 0.5f, trackR = rad - 2.5f, br = rad - 9.f;

        juce::Path bg, val;
        bg.addCentredArc (c.x, c.y, trackR, trackR, 0.f, sa, ea, true);
        val.addCentredArc (c.x, c.y, trackR, trackR, 0.f, sa, ang, true);
        g.setColour (deep.withAlpha (0.16f));
        g.strokePath (bg, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (pink);
        g.strokePath (val, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (c.x - br, c.y - br + 2.f, br * 2.f, br * 2.f);
        g.setGradientFill (juce::ColourGradient (Colour (0xffc2308a), c.x - br * 0.35f, c.y - br * 0.4f,
                                                 Colour (0xff4a0d63), c.x + br * 0.7f, c.y + br * 0.8f, true));
        g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillEllipse (c.x - br * 0.62f, c.y - br * 0.72f, br * 0.5f, br * 0.3f);

        juce::Path ptr;
        ptr.startNewSubPath (c.x + std::sin (ang) * br * 0.25f, c.y - std::cos (ang) * br * 0.25f);
        ptr.lineTo (c.x + std::sin (ang) * br * 0.88f, c.y - std::cos (ang) * br * 0.88f);
        g.setColour (cream);
        g.strokePath (ptr, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    // Grape: dark knob with a tick scale (ui1.png). Lit ticks show the value.
    using namespace grapecol;
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float R = d * 0.5f, s = d / 60.f;

    for (int t = 0; t <= 20; ++t)
    {
        if (t == 10) continue;   // the top position holds the centre dot
        const float ta = sa + (float) t / 20.f * (ea - sa);
        const bool big = t % 5 == 0;
        const float r0 = R - (big ? 6.8f : 5.8f) * s, r1 = R - 2.8f * s;
        g.setColour ((float) t / 20.f <= pos + 0.001f ? glow : tickOff);
        g.drawLine (c.x + std::sin (ta) * r0, c.y - std::cos (ta) * r0,
                    c.x + std::sin (ta) * r1, c.y - std::cos (ta) * r1, 1.6f * s);
    }
    g.setColour (trace);
    g.fillEllipse (c.x - 1.9f * s, c.y - R + 0.6f * s, 3.8f * s, 3.8f * s);

    const float kr = 19.5f * s;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - kr, c.y - kr + 1.5f * s, kr * 2.f, kr * 2.f);
    g.setColour (Colour (0xff0b080a));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    const float br = kr - 1.5f * s;
    juce::ColourGradient grad (Colour (0xff3a3036), c.x - br * 0.3f, c.y - br * 0.35f,
                               Colour (0xff0e0a0d), c.x + br, c.y + br, true);
    grad.addColour (0.7, Colour (0xff1c161a));
    g.setGradientFill (grad);
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (Colour (0xff3a3238));
    g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 0.8f);

    juce::Path ptr;
    ptr.startNewSubPath (c.x + std::sin (ang) * 6.f * s, c.y - std::cos (ang) * 6.f * s);
    ptr.lineTo (c.x + std::sin (ang) * 15.5f * s, c.y - std::cos (ang) * 15.5f * s);
    g.setColour (pointer);
    g.strokePath (ptr, juce::PathStrokeType (2.4f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
void ThemeLookAndFeel::drawToggleButton (Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();

    if (! grape())
    {
        using namespace berrycol;
        g.setColour (on ? mint : offGrey);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        const float d = r.getHeight() - 4.f;
        g.setColour (juce::Colours::white);
        g.fillEllipse (on ? r.getRight() - d - 2.f : r.getX() + 2.f, r.getY() + 2.f, d, d);
        if (hl)
        {
            g.setColour (deep.withAlpha (0.35f));
            g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
        }
        return;
    }

    using namespace grapecol;
    const float rad = r.getHeight() * 0.5f;
    juce::Path pill;
    pill.addRoundedRectangle (r, rad);
    if (on)
    {
        drawGlow (g, pill, glow, 2.f);
        g.setGradientFill (juce::ColourGradient (Colour (0xff6a1f7a), r.getX(), 0.f, accent, r.getRight(), 0.f, false));
        g.fillPath (pill);
        g.setColour (trace);
        g.strokePath (pill, juce::PathStrokeType (1.f));
    }
    else
    {
        g.setColour (Colour (0xff120710));
        g.fillPath (pill);
        g.setColour (borderDim);
        g.strokePath (pill, juce::PathStrokeType (1.f));
    }
    if (hl)
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillPath (pill);
    }

    const float d = r.getHeight() - 2.f;
    const float iconW = r.getHeight() * 0.48f;
    auto icon = on ? juce::Rectangle<float> (r.getX() + 5.f, r.getCentreY() - iconW * 0.55f, iconW, iconW * 1.1f)
                   : juce::Rectangle<float> (r.getRight() - 5.f - iconW, r.getCentreY() - iconW * 0.55f, iconW, iconW * 1.1f);
    drawGrapeIcon (g, icon, on ? juce::Colours::white : border);

    const float tx = on ? r.getRight() - d - 1.f : r.getX() + 1.f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (tx, r.getY() + 2.f, d, d);
    g.setGradientFill (juce::ColourGradient (on ? Colour (0xffe9c7e5) : Colour (0xff4a4248), tx + d * 0.35f, r.getY() + d * 0.3f,
                                             on ? Colour (0xff8c5a94) : Colour (0xff1c161a), tx + d, r.getY() + d, true));
    g.fillEllipse (tx, r.getY() + 1.f, d, d);
}

//==============================================================================
juce::Font ThemeLookAndFeel::getTextButtonFont (juce::TextButton& b, int h)
{
    const auto st = styleOf (b);
    if (st == "chip") return font (9.5f, true, grape() ? 0.04f : 0.f);
    if (st == "seg")  return font (9.5f, true, grape() ? 0.03f : 0.f);   // three modes in 170 px
    if (grape())
        return font (st == "flat" ? 13.f : juce::jmin (11.f, (float) h * 0.5f), true, st == "flat" ? 0.03f : 0.06f);
    return font (st == "flat" ? 13.f : (st == "reset" ? 11.f : juce::jmin (11.f, (float) h * 0.55f)), st != "reset");
}

void ThemeLookAndFeel::drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool hl, bool down)
{
    const auto st = styleOf (b);
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const bool isDanger = (bool) b.getProperties()["danger"];

    if (st == "chip")   // scope TIME / LEVEL buttons (the group background is drawn by the scope)
    {
        if (grape())
        {
            using namespace grapecol;
            auto rr = r.reduced (0.5f);
            if (on)
            {
                juce::Path p; p.addRoundedRectangle (rr, 3.f);
                drawGlow (g, p, glow, 1.2f);
                g.setGradientFill (juce::ColourGradient::vertical (Colour (0xff7a2a8f), rr.getY(), border, rr.getBottom()));
                g.fillRoundedRectangle (rr, 3.f);
                g.setColour (trace);
                g.drawRoundedRectangle (rr, 3.f, 1.f);
            }
            else if (hl)
            {
                g.setColour (Colour (0xff2a1027));
                g.fillRoundedRectangle (rr, 3.f);
            }
        }
        else if (on || hl)
        {
            const bool l = ! b.isConnectedOnLeft(), rt = ! b.isConnectedOnRight();
            const float rad = r.getHeight() * 0.5f;
            juce::Path p;
            p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad, l, rt, l, rt);
            g.setColour (on ? berrycol::pink : berrycol::cream.withAlpha (0.1f));
            g.fillPath (p);
        }
        return;
    }

    if (st == "size" || st == "flag")   // window size / hint language: two halves of one pill
    {
        const bool l = ! b.isConnectedOnLeft(), rt = ! b.isConnectedOnRight();
        const float rad = grape() ? 6.f : r.getHeight() * 0.5f;
        juce::Path p;
        p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad, l, rt, l, rt);
        if (grape())
        {
            using namespace grapecol;
            if (on) g.setGradientFill (juce::ColourGradient::vertical (Colour (0xff6a2a63), r.getY(), Colour (0xff4a1a45), r.getBottom()));
            else    g.setGradientFill (juce::ColourGradient::vertical (hl ? Colour (0xff3e1a3a) : Colour (0xff341431), r.getY(), Colour (0xff220d20), r.getBottom()));
            g.fillPath (p);
            g.setColour (border);
            g.strokePath (p, juce::PathStrokeType (1.f));
        }
        else
        {
            using namespace berrycol;
            g.setColour (on ? pink : cream.withAlpha (hl ? 1.f : 0.9f));
            g.fillPath (p);
        }
        return;
    }

    if (st == "flat")
    {
        if (hl || down)
        {
            g.setColour (grape() ? grapecol::glow.withAlpha (down ? 0.25f : 0.14f) : berrycol::pink.withAlpha (down ? 0.22f : 0.12f));
            g.fillRect (r);
        }
        return;
    }

    if (st == "power")   // PURE DI
    {
        auto c = r.reduced (2.f);
        const float d = juce::jmin (c.getWidth(), c.getHeight());
        c = c.withSizeKeepingCentre (d, d);
        if (! grape())
        {
            using namespace berrycol;
            if (on)
            {
                for (int i = 3; i >= 1; --i)
                {
                    g.setColour (pink.withAlpha (0.12f));
                    g.fillEllipse (c.expanded (2.f * (float) i));
                }
            }
            else
            {
                g.setColour (juce::Colours::black.withAlpha (0.3f));
                g.fillEllipse (c.translated (0.f, 2.f));
            }
            g.setGradientFill (juce::ColourGradient (on ? Colour (0xffff7ab8) : Colour (0xffc2308a), c.getX() + d * 0.35f, c.getY() + d * 0.3f,
                                                     on ? Colour (0xffc2108a) : Colour (0xff4a0d63), c.getRight(), c.getBottom(), true));
            g.fillEllipse (c);
            if (hl) { g.setColour (juce::Colours::white.withAlpha (0.15f)); g.fillEllipse (c); }

            // power symbol
            const auto ce = c.getCentre();
            const float pr = d * 0.24f;
            juce::Path arc;
            arc.addCentredArc (ce.x, ce.y + d * 0.02f, pr, pr, 0.f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
            arc.startNewSubPath (ce.x, ce.y - pr * 1.25f);
            arc.lineTo (ce.x, ce.y - pr * 0.1f);
            g.setColour (on ? juce::Colours::white : cream);
            g.strokePath (arc, juce::PathStrokeType (d * 0.055f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            using namespace grapecol;
            if (on)
            {
                for (int i = 4; i >= 1; --i)
                {
                    g.setColour (glow.withAlpha (0.11f));
                    g.fillEllipse (c.expanded (2.2f * (float) i));
                }
            }
            else
            {
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.fillEllipse (c.translated (0.f, 3.f));
            }
            g.setGradientFill (juce::ColourGradient (on ? Colour (0xff7a2a8f) : Colour (0xff3a3036), c.getX() + d * 0.4f, c.getY() + d * 0.35f,
                                                     on ? Colour (0xff2e0f33) : Colour (0xff120c10), c.getRight(), c.getBottom(), true));
            g.fillEllipse (c);
            g.setColour (on ? trace : Colour (0xff2a2328));
            g.drawEllipse (c.reduced (1.f), 2.f);
            if (hl) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillEllipse (c); }
            drawGrapeIcon (g, c.withSizeKeepingCentre (d * 0.4f, d * 0.44f), on ? juce::Colours::white : Colour (0xff6a5a66));
        }
        return;
    }

    if (! grape())
    {
        using namespace berrycol;
        if (st == "seg")
        {
            if (on || hl)
            {
                const bool l = ! b.isConnectedOnLeft(), rt = ! b.isConnectedOnRight();
                const float rad = r.getHeight() * 0.5f;
                juce::Path p;
                p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad, l, rt, l, rt);
                g.setColour (on ? pink : pink.withAlpha (0.12f));
                g.fillPath (p);
            }
            return;
        }
        if (st == "hold")
        {
            g.setColour (on ? mint.withAlpha (0.25f) : deep.withAlpha (hl ? 0.14f : 0.08f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            return;
        }
        if (st == "update")
        {
            g.setColour (hl ? mint.brighter (0.15f) : mint);
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            return;
        }
        if (st == "reset")
        {
            g.setColour (cream.withAlpha (hl ? 1.f : 0.9f));
            g.fillRoundedRectangle (r, 4.f);
            return;
        }
        // pill + dialog buttons
        const float rad = r.getHeight() * 0.5f;
        Colour fill = st == "pill" ? cream.withAlpha (0.9f)
                                   : (b.getButtonText() == "Delete" || isDanger ? danger
                                                                              : (b.getProperties()["primary"] ? pink : deep.withAlpha (0.1f)));
        if (st.isEmpty() && (bool) b.getProperties()["primary"]) fill = pink;
        g.setColour (hl ? fill.brighter (0.08f) : fill);
        if (down) g.setColour (fill.darker (0.1f));
        g.fillRoundedRectangle (r, rad);
        return;
    }

    using namespace grapecol;
    if (st == "seg")
    {
        auto rr = r.reduced (0.5f);
        if (on)
        {
            juce::Path p; p.addRoundedRectangle (rr, 5.f);
            drawGlow (g, p, glow, 1.5f);
            g.setGradientFill (juce::ColourGradient::vertical (Colour (0xff7a2a8f), rr.getY(), border, rr.getBottom()));
            g.fillRoundedRectangle (rr, 5.f);
            g.setColour (trace);
            g.drawRoundedRectangle (rr, 5.f, 1.f);
        }
        else
        {
            g.setColour (hl ? Colour (0xff22101f) : Colour (0xff160814));
            g.fillRoundedRectangle (rr, 5.f);
            g.setColour (borderDim);
            g.drawRoundedRectangle (rr, 5.f, 1.f);
        }
        return;
    }
    if (st == "hold")
    {
        auto rr = r.reduced (0.5f);
        g.setColour (Colour (0xd91d0b1b));
        g.fillRoundedRectangle (rr, 4.f);
        g.setColour (hl ? border : borderDim);
        g.drawRoundedRectangle (rr, 4.f, 1.f);
        return;
    }
    if (st == "update")
    {
        auto rr = r.reduced (0.5f);
        g.setGradientFill (juce::ColourGradient::vertical (Colour (hl ? 0xff2c4021 : 0xff24331b), rr.getY(), Colour (0xff172412), rr.getBottom()));
        g.fillRoundedRectangle (rr, 6.f);
        g.setColour (Colour (0xff3f7a2a));
        g.drawRoundedRectangle (rr, 6.f, 1.f);
        return;
    }
    // pill, reset and dialog buttons
    auto rr = r.reduced (0.5f);
    const bool primary = (bool) b.getProperties()["primary"];
    if (primary)
    {
        g.setGradientFill (juce::ColourGradient::vertical (Colour (0xff7a2a8f), rr.getY(), border, rr.getBottom()));
        g.fillRoundedRectangle (rr, 6.f);
        g.setColour (trace);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient::vertical (Colour (hl ? 0xff42183e : 0xff341431), rr.getY(), Colour (0xff220d20), rr.getBottom()));
        g.fillRoundedRectangle (rr, 6.f);
        g.setColour (isDanger && hl ? grapecol::danger.withAlpha (0.7f) : border);
    }
    if (down) { g.setColour (juce::Colours::black.withAlpha (0.2f)); g.fillRoundedRectangle (rr, 6.f); g.setColour (border); }
    g.drawRoundedRectangle (rr, 6.f, 1.f);
}

void ThemeLookAndFeel::drawButtonText (Graphics& g, juce::TextButton& b, bool hl, bool)
{
    const auto st = styleOf (b);
    if (st == "power") return;
    if (st == "flag")
    {
        auto fr = b.getLocalBounds().toFloat().withSizeKeepingCentre (18.f, 12.f);
        juce::Graphics::ScopedSaveState ss (g);
        if (! b.getToggleState()) g.setOpacity (hl ? 0.8f : 0.55f);
        drawFlag (g, fr, b.getProperties()["flag"].toString() == "ru");
        return;
    }

    auto r = b.getLocalBounds();
    const bool on = b.getToggleState();
    const bool isDanger = (bool) b.getProperties()["danger"];
    const bool primary = (bool) b.getProperties()["primary"];
    const auto arrow = b.getProperties()["arrow"].toString();
    auto text = b.getButtonText();
    g.setFont (getTextButtonFont (b, r.getHeight()));

    if (arrow.isNotEmpty())   // preset prev / next
    {
        const auto c = r.toFloat().getCentre();
        juce::Path tri;
        if (arrow == "left") tri.addTriangle (c.x + 3.5f, c.y - 5.f, c.x + 3.5f, c.y + 5.f, c.x - 4.5f, c.y);
        else                 tri.addTriangle (c.x - 3.5f, c.y - 5.f, c.x - 3.5f, c.y + 5.f, c.x + 4.5f, c.y);
        auto col = grape() ? grapecol::label : berrycol::pink;
        g.setColour (hl ? col.brighter (0.25f) : col);
        g.fillPath (tri);
        return;
    }

    if (st == "hold")
    {
        const float d = grape() ? 7.f : 8.f;
        const auto tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text);
        const float x0 = (float) r.getCentreX() - (tw + d + 5.f) * 0.5f;
        const float cy = (float) r.getCentreY();
        Colour dot = grape() ? (on ? grapecol::ledGreen : Colour (0xff2a2328)) : (on ? berrycol::mint : berrycol::offGrey);
        if (grape() && on)
        {
            g.setColour (dot.withAlpha (0.35f));
            g.fillEllipse (x0 - 2.f, cy - d * 0.5f - 2.f, d + 4.f, d + 4.f);
        }
        g.setColour (dot);
        g.fillEllipse (x0, cy - d * 0.5f, d, d);
        g.setColour (grape() ? grapecol::label : berrycol::deep);
        g.drawText (text, juce::Rectangle<float> (x0 + d + 5.f, (float) r.getY(), tw + 4.f, (float) r.getHeight()), juce::Justification::centredLeft);
        return;
    }

    if (st == "update" && grape())
    {
        const float d = 7.f, cy = (float) r.getCentreY();
        const auto tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text);
        const float x0 = (float) r.getCentreX() - (tw + d + 6.f) * 0.5f;
        g.setColour (grapecol::ledGreen.withAlpha (0.35f));
        g.fillEllipse (x0 - 2.f, cy - 5.5f, 11.f, 11.f);
        g.setColour (grapecol::ledGreen);
        g.fillEllipse (x0, cy - 3.5f, d, d);
        g.setColour (Colour (0xffb6f08a));
        g.drawText (text, juce::Rectangle<float> (x0 + d + 6.f, (float) r.getY(), tw + 4.f, (float) r.getHeight()), juce::Justification::centredLeft);
        return;
    }

    Colour c;
    if (grape())
    {
        using namespace grapecol;
        if (st == "seg")         c = on ? juce::Colours::white : muted;
        else if (st == "size")   c = on ? grapecol::text : muted;
        else if (st == "chip")   c = on ? juce::Colours::white : muted;
        else if (st == "flat")   c = grapecol::text;
        else if (primary)        c = juce::Colours::white;
        else if (isDanger)       c = grapecol::danger;
        else                     c = st == "pill" ? Colour (0xffe9c7e5) : label;
    }
    else
    {
        using namespace berrycol;
        if (st == "seg" || st == "size") c = on ? cream : deep;
        else if (st == "chip")   c = on ? cream : cream.withAlpha (0.75f);
        else if (st == "flat")   c = deep;
        else if (st == "update") c = deep;
        else if (st == "pill")   c = isDanger ? berrycol::danger : deep;
        else if (primary || isDanger || text == "Delete") c = cream;
        else                     c = deep;
    }
    if (! b.isEnabled()) c = c.withMultipliedAlpha (0.45f);
    g.setColour (hl && st == "flat" ? c.brighter (0.2f) : c);

    if (st == "flat")
    {
        // preset name: text on the left, small arrow on the right
        auto area = r.reduced (10, 0);
        if (grape())
        {
            drawGrapeIcon (g, area.removeFromLeft (14).toFloat().withSizeKeepingCentre (13.f, 14.f), grapecol::accent);
            area.removeFromLeft (8);
        }
        auto arrowArea = area.removeFromRight (10).toFloat();
        g.drawFittedText (text, area, juce::Justification::centredLeft, 1, 0.8f);
        juce::Path tri;
        const auto ac = arrowArea.getCentre();
        tri.addTriangle (ac.x - 4.f, ac.y - 2.f, ac.x + 4.f, ac.y - 2.f, ac.x, ac.y + 3.f);
        g.setColour (grape() ? grapecol::flavor : berrycol::pink);
        g.fillPath (tri);
        return;
    }

    if (grape() && st != "flat" && st != "chip") text = text.toUpperCase();   // "1 s", "dB" keep their case
    if (st == "chip") { g.drawFittedText (text, r, juce::Justification::centred, 1, 0.8f); return; }
    g.drawFittedText (text, r.reduced (4, 0), juce::Justification::centred, 1, 0.8f);
}

//==============================================================================
void ThemeLookAndFeel::drawProgressBar (Graphics& g, juce::ProgressBar&, int w, int h, double progress, const juce::String&)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h);
    const bool busy = progress < 0.0 || progress > 1.0;
    const float t = (float) (juce::Time::getMillisecondCounter() % 1200) / 1200.f;

    if (! grape())
    {
        g.setColour (berrycol::deep.withAlpha (0.12f));
        g.fillRoundedRectangle (r, h * 0.5f);
        g.setColour (berrycol::mint);
        if (busy) g.fillRoundedRectangle (r.withWidth (r.getWidth() * 0.3f).withX (t * r.getWidth() * 0.7f), h * 0.5f);
        else      g.fillRoundedRectangle (r.withWidth (r.getWidth() * (float) progress), h * 0.5f);
        return;
    }

    g.setColour (grapecol::screen);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (grapecol::borderDim);
    g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);
    const int segs = 20;
    auto in = r.reduced (2.f);
    const float sw = (in.getWidth() - (segs - 1) * 2.f) / segs;
    for (int i = 0; i < segs; ++i)
    {
        const float f = (float) i / segs;
        const bool lit = busy ? std::abs (f - t) < 0.15f : f < (float) progress;
        g.setColour (lit ? grapecol::ledGreen : grapecol::ledOff);
        g.fillRoundedRectangle (in.getX() + (float) i * (sw + 2.f), in.getY(), sw, in.getHeight(), 1.f);
    }
}

juce::Font ThemeLookAndFeel::getAlertWindowTitleFont()   { return grape() ? font (15.f, true, 0.06f) : font (17.f); }
juce::Font ThemeLookAndFeel::getAlertWindowMessageFont() { return font (13.f, false); }
juce::Font ThemeLookAndFeel::getAlertWindowFont()        { return font (13.f, false); }
juce::Font ThemeLookAndFeel::getPopupMenuFont()          { return font (14.f, ! grape()); }

//==============================================================================
void ThemeLookAndFeel::drawCornerResizer (Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    auto c = grape() ? grapecol::dim : berrycol::cream.withAlpha (0.7f);
    if (isMouseOver || isMouseDragging) c = grape() ? grapecol::label : berrycol::cream;
    g.setColour (c);
    const float sx = (float) w / 14.f, sy = (float) h / 14.f;
    juce::Path p;
    p.startNewSubPath (13.f * sx, 3.f * sy);  p.lineTo (3.f * sx, 13.f * sy);
    p.startNewSubPath (13.f * sx, 7.f * sy);  p.lineTo (7.f * sx, 13.f * sy);
    p.startNewSubPath (13.f * sx, 11.f * sy); p.lineTo (11.f * sx, 13.f * sy);
    g.strokePath (p, juce::PathStrokeType (1.5f * sx, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
void ThemeLookAndFeel::drawFlag (Graphics& g, juce::Rectangle<float> r, bool russia)
{
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip;
    clip.addRoundedRectangle (r, 1.5f);
    g.reduceClipRegion (clip);
    if (russia)
    {
        const float h = r.getHeight() / 3.f;
        g.setColour (juce::Colours::white);       g.fillRect (r.withHeight (h));
        g.setColour (Colour (0xff1c57c4));        g.fillRect (r.withY (r.getY() + h).withHeight (h));
        g.setColour (Colour (0xffd52b1e));        g.fillRect (r.withY (r.getY() + 2.f * h).withHeight (h));
    }
    else
    {
        g.setColour (juce::Colours::white);
        g.fillRect (r);
        const float sh = r.getHeight() / 13.f;
        g.setColour (Colour (0xffc8102e));
        for (int i = 0; i < 13; i += 2)
            g.fillRect (r.getX(), r.getY() + (float) i * sh, r.getWidth(), sh);
        const auto canton = juce::Rectangle<float> (r.getX(), r.getY(), r.getWidth() * 0.42f, sh * 7.f);
        g.setColour (Colour (0xff1f3a93));
        g.fillRect (canton);
        g.setColour (juce::Colours::white);
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                g.fillEllipse (canton.getX() + 1.f + (float) col * canton.getWidth() / 4.f,
                               canton.getY() + 0.8f + (float) row * canton.getHeight() / 4.f, 0.9f, 0.9f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawRoundedRectangle (r.reduced (0.25f), 1.5f, 0.5f);
}
