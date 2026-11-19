#include "PluginEditor.h"
#include <algorithm>

using namespace berrycol;

static juce::Font F (float h, int style = juce::Font::bold)
{
    return juce::Font (juce::FontOptions (h, style));
}

//==============================================================================
BerryLookAndFeel::BerryLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, pink.withAlpha (0.3f));
    setColour (juce::Label::textColourId, ink);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::backgroundColourId, cream);
    setColour (juce::CaretComponent::caretColourId, pink);
}

juce::Font BerryLookAndFeel::getLabelFont (juce::Label&) { return F (12.f); }

// Always draw parameter names / values in purple so they read clearly on the cream cards
void BerryLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));
    if (label.isBeingEdited()) return;

    const auto font = getLabelFont (label);
    g.setColour (ink.withMultipliedAlpha (label.isEnabled() ? 1.f : 0.5f));
    g.setFont (font);
    const auto area = label.getBorderSize().subtractedFrom (label.getLocalBounds());
    g.drawFittedText (label.getText(), area, label.getJustificationType(),
                      juce::jmax (1, (int) ((float) area.getHeight() / font.getHeight())),
                      label.getMinimumHorizontalScale());
}

void BerryLookAndFeel::drawBerry (juce::Graphics& g, float cx, float cy, float r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7ab8), cx - r * 0.35f, cy - r * 0.4f,
                                             juce::Colour (0xff8a0f5c), cx + r * 0.7f, cy + r * 0.8f, true));
    g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (cx - r * 0.6f, cy - r * 0.7f, r * 0.5f, r * 0.32f);

    juce::Path leafP;
    leafP.startNewSubPath (cx, cy - r);
    leafP.quadraticTo (cx + r * 0.9f, cy - r * 1.7f, cx + r * 1.2f, cy - r * 0.9f);
    leafP.quadraticTo (cx + r * 0.4f, cy - r * 0.8f, cx, cy - r);
    g.setColour (leaf);
    g.fillPath (leafP);
}

void BerryLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float sa, float ea, juce::Slider&)
{
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float rad = d * 0.5f, trackR = rad - 2.5f, br = rad - 9.f;
    const float ang = sa + pos * (ea - sa);

    juce::Path bg, val;
    bg.addCentredArc (c.x, c.y, trackR, trackR, 0.f, sa, ea, true);
    val.addCentredArc (c.x, c.y, trackR, trackR, 0.f, sa, ang, true);
    g.setColour (deep.withAlpha (0.16f));
    g.strokePath (bg, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (pink);
    g.strokePath (val, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // berry body
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (c.x - br, c.y - br + 2.f, br * 2.f, br * 2.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc2308a), c.x - br * 0.35f, c.y - br * 0.4f,
                                             juce::Colour (0xff4a0d63), c.x + br * 0.7f, c.y + br * 0.8f, true));
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillEllipse (c.x - br * 0.62f, c.y - br * 0.72f, br * 0.5f, br * 0.3f);

    // pointer
    juce::Path ptr;
    ptr.startNewSubPath (c.x + std::sin (ang) * br * 0.25f, c.y - std::cos (ang) * br * 0.25f);
    ptr.lineTo (c.x + std::sin (ang) * br * 0.88f, c.y - std::cos (ang) * br * 0.88f);
    g.setColour (cream);
    g.strokePath (ptr, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void BerryLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    g.setColour (on ? mint : juce::Colour (0xffcfc2cf));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    const float d = r.getHeight() - 4.f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (on ? r.getRight() - d - 2.f : r.getX() + 2.f, r.getY() + 2.f, d, d);
    if (hl)
    {
        g.setColour (deep.withAlpha (0.35f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    }
}

//==============================================================================
BerryToneAudioProcessorEditor::BerryToneAudioProcessorEditor (BerryToneAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // Signal flow order: 1..7, then master
    addModule ("Low Freq Tightener", "tart cherry", ID::tightOn,
               { { "Cut", ID::tightFreq }, { "Tight", ID::tightAmt } });
    addModule ("Horizon Gate", "blueberry", ID::gateOn,
               { { "Thresh", ID::gateThresh }, { "Release", ID::gateRel }, { "Depth", ID::gateDepth } })
        .hasLed = true;
    addModule ("Chug Enhancer", "blackberry", ID::chugOn,
               { { "Punch", ID::chugPunch }, { "Body", ID::chugBody }, { "Speed", ID::chugSpeed } });
    addModule ("Low Octave Blender", "concord grape", ID::octOn,
               { { "Blend", ID::octBlend }, { "Tone", ID::octTone }, { "Track", ID::octTrack } });
    addModule ("Impedance Curve", "boysenberry", ID::impOn,
               { { "Freq", ID::impFreq }, { "Peak", ID::impPeak }, { "Load", ID::impLoad } });
    addModule ("Tone Tilt", "raspberry", ID::tiltOn,
               { { "Tilt", ID::tiltAmt }, { "Pivot", ID::tiltPivot } });
    addModule ("Sound Destroyer", "ghost pepper berry", ID::destOn,
               { { "Drive", ID::destDrive }, { "Crush", ID::destCrush }, { "Mix", ID::destMix } });
    addModule ("Juice Level", "input / output", nullptr,
               { { "Input", ID::inGain }, { "Output", ID::outGain } })
        .isMaster = true;

    order = proc.getOrder();

    resetBtn.setColour (juce::TextButton::buttonColourId, cream.withAlpha (0.9f));
    resetBtn.setColour (juce::TextButton::textColourOffId, deep);
    resetBtn.onClick = [this]
    {
        for (int i = 0; i < 7; ++i) order[(size_t) i] = i;
        applyOrder();
    };
    addAndMakeVisible (resetBtn);

    setSize (940, 540);
    startTimerHz (30);
    timerCallback();
}

BerryToneAudioProcessorEditor::~BerryToneAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

BerryToneAudioProcessorEditor::Module& BerryToneAudioProcessorEditor::addModule (
    const juce::String& title, const juce::String& flavor, const char* onId,
    std::initializer_list<std::pair<const char*, const char*>> knobs)
{
    auto m = std::make_unique<Module>();
    m->title = title;
    m->flavor = flavor;
    m->id = (int) modules.size();   // 0..6 = DSP module ids, 7 = master card
    m->stage = m->id + 1;

    if (onId != nullptr)
    {
        addAndMakeVisible (m->on);
        m->onAtt = std::make_unique<ButtonAtt> (proc.apvts, onId, m->on);
    }

    for (auto& k : knobs)
    {
        auto kn = std::make_unique<Knob>();
        kn->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 16);
        kn->label.setText (k.first, juce::dontSendNotification);
        kn->label.setJustificationType (juce::Justification::centred);
        kn->label.setInterceptsMouseClicks (false, false);
        kn->label.setColour (juce::Label::textColourId, ink);
        kn->slider.setColour (juce::Slider::textBoxTextColourId, ink);
        addAndMakeVisible (kn->slider);
        addAndMakeVisible (kn->label);
        kn->att = std::make_unique<SliderAtt> (proc.apvts, k.second, kn->slider);
        m->knobs.push_back (std::move (kn));
    }

    modules.push_back (std::move (m));
    return *modules.back();
}

//==============================================================================
void BerryToneAudioProcessorEditor::resized()
{
    resetBtn.setBounds (getWidth() - 22 - 96, getHeight() - 33, 96, 20);

    for (auto& mp : modules)
    {
        auto& m = *mp;
        m.bounds = slotRect (slotOf (m.id));

        m.on.setBounds (m.bounds.getRight() - 46, m.bounds.getY() + 14, 34, 18);
        if (m.hasLed)
            gateLed = { m.bounds.getRight() - 64, m.bounds.getY() + 19, 9, 9 };

        auto body = m.bounds.withTrimmedTop (50).reduced (4, 0).withTrimmedBottom (8);
        const int cellW = body.getWidth() / 3;
        const int n = (int) m.knobs.size();
        int x = m.isMaster ? body.getX() : body.getX() + (body.getWidth() - n * cellW) / 2;

        for (auto& k : m.knobs)
        {
            auto cell = juce::Rectangle<int> (x, body.getY(), cellW, body.getHeight());
            k->label.setBounds (cell.removeFromTop (16));
            k->slider.setBounds (cell);
            x += cellW;
        }

        if (m.isMaster)
        {
            auto t = juce::Rectangle<int> (body.getX() + 2 * cellW + 6, body.getY() + 2, body.getRight() - (body.getX() + 2 * cellW + 6), body.getHeight() - 22);
            inTube  = t.removeFromLeft (t.getWidth() / 2 - 2);
            outTube = t.withTrimmedLeft (4);
        }
    }
}

//==============================================================================
juce::Rectangle<int> BerryToneAudioProcessorEditor::slotRect (int slot) const
{
    const int margin = 22, gap = 12, top = 104, cardH = 186, cols = 4;
    const int cardW = (getWidth() - 2 * margin - (cols - 1) * gap) / cols;
    return { margin + (slot % cols) * (cardW + gap), top + (slot / cols) * (cardH + gap), cardW, cardH };
}

int BerryToneAudioProcessorEditor::slotOf (int id) const
{
    if (id >= 7) return 7;   // master card is fixed
    return (int) (std::find (order.begin(), order.end(), id) - order.begin());
}

int BerryToneAudioProcessorEditor::moduleAtHeader (juce::Point<int> p) const
{
    for (auto& m : modules)
        if (! m->isMaster && m->bounds.withHeight (46).withTrimmedRight (72).contains (p))
            return m->id;
    return -1;
}

void BerryToneAudioProcessorEditor::applyOrder()
{
    proc.setOrder (order);
    resized();
    repaint();
}

void BerryToneAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (moduleAtHeader (e.getPosition()) >= 0 ? juce::MouseCursor::DraggingHandCursor
                                                          : juce::MouseCursor::NormalCursor);
}

void BerryToneAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    dragId = moduleAtHeader (e.getPosition());
    if (dragId >= 0) repaint();
}

void BerryToneAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragId < 0) return;
    const auto pos = e.getPosition();

    for (int slot = 0; slot < 7; ++slot)
    {
        if (! slotRect (slot).contains (pos)) continue;

        const int from = slotOf (dragId);
        if (slot != from)
        {
            if (from < slot) std::rotate (order.begin() + from, order.begin() + from + 1, order.begin() + slot + 1);
            else             std::rotate (order.begin() + slot, order.begin() + from, order.begin() + from + 1);
            applyOrder();
        }
        break;
    }
}

void BerryToneAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragId >= 0) { dragId = -1; repaint(); }
}

//==============================================================================
void BerryToneAudioProcessorEditor::drawTube (juce::Graphics& g, juce::Rectangle<float> r, float level, const juce::String& name)
{
    g.setColour (deep.withAlpha (0.88f));
    g.fillRoundedRectangle (r, 6.f);

    auto inner = r.reduced (3.f);
    const float fh = inner.getHeight() * juce::jlimit (0.f, 1.f, level);
    auto fill = inner.withTop (inner.getBottom() - fh);
    if (fh > 1.f)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7ab8), 0.f, inner.getBottom() - inner.getHeight(),
                                                 juce::Colour (0xffc2108a), 0.f, inner.getBottom(), false));
        g.fillRoundedRectangle (fill, 4.f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.fillRoundedRectangle (inner.getX() + 2.f, inner.getY() + 2.f, 3.f, inner.getHeight() - 4.f, 1.5f);

    g.setColour (deep);
    g.setFont (F (9.5f));
    g.drawText (name, (int) r.getX() - 6, (int) r.getBottom() + 3, (int) r.getWidth() + 12, 12, juce::Justification::centred);
}

void BerryToneAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    // --- carton body ---
    g.setGradientFill (juce::ColourGradient::vertical (cartonTop, 0.f, cartonBot, h));
    g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (0.f, 0.f, 16.f, h);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRect (w - 16.f, 0.f, 16.f, h);

    // --- crimped top seal ---
    g.setColour (juce::Colours::black.withAlpha (0.12f));
    g.fillRect (0.f, 0.f, w, 18.f);
    for (float x = 0.f; x < w; x += 5.f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawLine (x, 0.f, x, 18.f, 1.f);
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        g.drawLine (x + 2.5f, 0.f, x + 2.5f, 18.f, 1.f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.drawLine (0.f, 18.f, w, 18.f, 1.5f);

    // --- berries + title ---
    BerryLookAndFeel::drawBerry (g, 50.f, 66.f, 15.f);
    BerryLookAndFeel::drawBerry (g, 78.f, 54.f, 13.f);
    BerryLookAndFeel::drawBerry (g, 74.f, 82.f, 12.f);

    g.setFont (F (46.f));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText ("BERRY TONE", 112, 34, 420, 54, juce::Justification::centredLeft);
    g.setColour (cream);
    g.drawText ("BERRY TONE", 110, 31, 420, 54, juce::Justification::centredLeft);

    g.setFont (F (11.f));
    g.setColour (cream.withAlpha (0.85f));
    g.drawText ("100% PURE SIGNAL  -  LOW-TUNED GUITAR JUICER  -  0 SAMPLES LATENCY",
                112, 82, 520, 14, juce::Justification::centredLeft);

    // --- straw ---
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillEllipse (822.f, 82.f, 32.f, 12.f);          // foil hole
    juce::Path straw;
    straw.startNewSubPath (838.f, 88.f);
    straw.lineTo (838.f, 36.f);
    straw.lineTo (886.f, 10.f);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.strokePath (straw, juce::PathStrokeType (11.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::translation (2.f, 3.f));
    g.setColour (juce::Colours::white);
    g.strokePath (straw, juce::PathStrokeType (10.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path dashed;
    const float dashes[] = { 5.f, 6.f };
    juce::PathStrokeType (1.f).createDashedStroke (dashed, straw, dashes, 2);
    g.setColour (pink);
    g.strokePath (dashed, juce::PathStrokeType (10.f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    // --- module cards ---
    for (auto& mp : modules)
    {
        auto& m = *mp;
        const auto r = m.bounds;
        const bool on = m.isMaster || m.on.getToggleState();

        juce::Path card;
        card.addRoundedRectangle (r.toFloat(), 14.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 10, { 0, 4 }).drawForPath (g, card);
        g.setColour (cream);
        g.fillPath (card);
        if (m.id == dragId)
        {
            g.setColour (pink);
            g.strokePath (card, juce::PathStrokeType (3.f));
        }

        g.setColour (pink.withAlpha (0.25f));
        g.drawLine ((float) r.getX() + 14.f, (float) r.getY() + 46.f, (float) r.getRight() - 14.f, (float) r.getY() + 46.f, 1.5f);

        // stage badge
        const float bx = (float) r.getX() + 22.f, by = (float) r.getY() + 25.f;
        if (m.isMaster)
            BerryLookAndFeel::drawBerry (g, bx, by + 2.f, 8.f);
        else
        {
            g.setColour (on ? pink : juce::Colour (0xffcfc2cf));
            g.fillEllipse (bx - 11.f, by - 11.f, 22.f, 22.f);
            g.setColour (juce::Colours::white);
            g.setFont (F (12.f));
            g.drawText (juce::String (slotOf (m.id) + 1), (int) bx - 11, (int) by - 11, 22, 22, juce::Justification::centred);
        }

        const int tw = r.getWidth() - 40 - (m.hasLed ? 72 : 52);
        g.setColour (deep.withAlpha (on ? 1.f : 0.45f));
        g.setFont (F (14.f));
        g.drawFittedText (m.title, r.getX() + 40, r.getY() + 10, tw, 18, juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (pink.withAlpha (on ? 0.9f : 0.4f));
        g.setFont (F (10.5f, juce::Font::italic));
        g.drawText (m.flavor, r.getX() + 40, r.getY() + 28, tw, 13, juce::Justification::centredLeft);

        if (m.isMaster)
        {
            drawTube (g, inTube.toFloat(),  inLvl,  "IN");
            drawTube (g, outTube.toFloat(), outLvl, "OUT");
        }
    }

    // gate activity LED (opens = mint)
    g.setColour (gateOpenLed ? mint : juce::Colour (0xffcfc2cf));
    g.fillEllipse (gateLed.toFloat());
    g.setColour (deep.withAlpha (0.3f));
    g.drawEllipse (gateLed.toFloat(), 1.f);

    // footer
    g.setColour (cream.withAlpha (0.6f));
    g.setFont (F (10.5f));
    g.drawText ("DRAG A CARD HEADER TO REORDER  -  NO ADDED NOISE  -  PUT ME BEFORE YOUR AMP SIM",
                0, getHeight() - 28, getWidth(), 14, juce::Justification::centred);
}

//==============================================================================
void BerryToneAudioProcessorEditor::timerCallback()
{
    auto norm = [] (float lin)
    {
        return juce::jlimit (0.f, 1.f, (juce::Decibels::gainToDecibels (lin, -60.f) + 60.f) / 60.f);
    };

    const float i = norm (proc.inPeak.exchange (0.f));
    const float o = norm (proc.outPeak.exchange (0.f));
    inLvl  = i > inLvl  ? i : inLvl  * 0.88f;
    outLvl = o > outLvl ? o : outLvl * 0.88f;

    const bool gOpen = proc.gateGain.load() > 0.5f;
    if (gOpen != gateOpenLed) { gateOpenLed = gOpen; repaint (gateLed.expanded (3)); }

    for (auto& mp : modules)
    {
        auto& m = *mp;
        if (m.isMaster)
        {
            repaint (m.bounds);
            continue;
        }
        const int on = m.on.getToggleState() ? 1 : 0;
        if (on != m.lastOn)
        {
            m.lastOn = on;
            for (auto& k : m.knobs)
            {
                k->slider.setAlpha (on ? 1.f : 0.4f);
                k->label.setAlpha (on ? 1.f : 0.4f);
            }
            repaint (m.bounds);
        }
    }
}
