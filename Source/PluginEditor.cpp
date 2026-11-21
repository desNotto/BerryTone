#include "PluginEditor.h"
#include <algorithm>

using F = ThemeLookAndFeel;
using juce::Colour;

//==============================================================================
BerryToneView::BerryToneView (BerryToneAudioProcessor& p)
    : proc (p), scope (p)
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

    // --- DI scope + its controls ---
    addAndMakeVisible (scope);

    for (auto* b : { &modeReal, &modeSpec, &modeFlat })
    {
        styleButton (*b, "seg");
        b->setRadioGroupId (1001);
        b->setClickingTogglesState (true);
    }
    modeReal.setConnectedEdges (juce::Button::ConnectedOnRight);
    modeSpec.setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
    modeFlat.setConnectedEdges (juce::Button::ConnectedOnLeft);
    // saved values: 0 = REAL DI, 1 = FLAT LINE, 2 = SPECTRUM
    const int savedMode = juce::jlimit (0, 2, (int) proc.apvts.state.getProperty ("scopeMode", 0));
    const auto startMode = static_cast<ScopeView::Mode> (savedMode);
    modeReal.setToggleState (startMode == ScopeView::Mode::real, juce::dontSendNotification);
    modeSpec.setToggleState (startMode == ScopeView::Mode::spectrum, juce::dontSendNotification);
    modeFlat.setToggleState (startMode == ScopeView::Mode::flat, juce::dontSendNotification);
    scope.setMode (startMode);
    auto setMode = [this] (ScopeView::Mode m)
    {
        scope.setMode (m);
        proc.apvts.state.setProperty ("scopeMode", (int) m, nullptr);
        updateScopeChips();
    };
    modeReal.onClick = [setMode] { setMode (ScopeView::Mode::real); };
    modeSpec.onClick = [setMode] { setMode (ScopeView::Mode::spectrum); };
    modeFlat.onClick = [setMode] { setMode (ScopeView::Mode::flat); };
    modeReal.setTooltip ("Real DI: the raw DI (grey) and the signal after the modules");
    modeSpec.setTooltip ("Spectrum: the live DI (grey area) and OUT (line), 20 Hz - 20 kHz");
    modeFlat.setTooltip ("Flat line: a flat test signal. The modules bend the line (20 Hz - 20 kHz)");

    // REAL DI: time window and level scale (buttons on the screen)
    for (auto* b : { &zoom1s, &zoom200, &zoom50 }) { styleButton (*b, "chip"); b->setRadioGroupId (1002); b->setClickingTogglesState (true); }
    for (auto* b : { &lvlLin, &lvlDb })            { styleButton (*b, "chip"); b->setRadioGroupId (1003); b->setClickingTogglesState (true); }
    zoom1s.setConnectedEdges (juce::Button::ConnectedOnRight);
    zoom200.setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
    zoom50.setConnectedEdges (juce::Button::ConnectedOnLeft);
    lvlLin.setConnectedEdges (juce::Button::ConnectedOnRight);
    lvlDb.setConnectedEdges (juce::Button::ConnectedOnLeft);
    const int savedZoom = juce::jlimit (0, 2, (int) proc.apvts.state.getProperty ("scopeTime", 0));
    const int savedLevel = juce::jlimit (0, 1, (int) proc.apvts.state.getProperty ("scopeLevel", 0));
    juce::TextButton* zb[] = { &zoom1s, &zoom200, &zoom50 };
    zb[savedZoom]->setToggleState (true, juce::dontSendNotification);
    (savedLevel == 1 ? lvlDb : lvlLin).setToggleState (true, juce::dontSendNotification);
    scope.setZoom (static_cast<ScopeView::Zoom> (savedZoom));
    scope.setLevel (static_cast<ScopeView::Level> (savedLevel));
    for (int i = 0; i < 3; ++i)
        zb[i]->onClick = [this, i] { scope.setZoom (static_cast<ScopeView::Zoom> (i)); proc.apvts.state.setProperty ("scopeTime", i, nullptr); };
    lvlLin.onClick = [this] { scope.setLevel (ScopeView::Level::lin); proc.apvts.state.setProperty ("scopeLevel", 0, nullptr); };
    lvlDb.onClick  = [this] { scope.setLevel (ScopeView::Level::db);  proc.apvts.state.setProperty ("scopeLevel", 1, nullptr); };
    zoom1s.setTooltip ("Time window 1 s (100 ms/div)");
    zoom200.setTooltip ("Time window 200 ms (20 ms/div)");
    zoom50.setTooltip ("Time window 50 ms (5 ms/div): you see the wave shape");
    lvlLin.setTooltip ("Linear level, auto scale");
    lvlDb.setTooltip ("dB level, 0 to -60 dBFS: you see the quiet parts and the gate");

    styleButton (pureBtn, "power");
    pureBtn.setClickingTogglesState (true);
    pureBtn.setToggleState (proc.pureDi.load(), juce::dontSendNotification);
    pureBtn.onClick = [this] { proc.pureDi = pureBtn.getToggleState(); repaint(); };
    pureBtn.setTooltip ("PURE DI: mute all modules. You hear the raw DI.");

    styleButton (holdBtn, "hold");
    holdBtn.setClickingTogglesState (true);
    holdBtn.onClick = [this] { scope.setHold (holdBtn.getToggleState()); };
    holdBtn.setTooltip ("Freeze the DI. The OUT line still follows the modules.");
    updateScopeChips();

    // --- presets ---
    styleButton (presetPrev, "flat");
    styleButton (presetNext, "flat");
    presetPrev.getProperties().set ("arrow", "left");
    presetNext.getProperties().set ("arrow", "right");
    presetPrev.onClick = [this] { proc.presets.previous(); updatePresetLabel(); };
    presetNext.onClick = [this] { proc.presets.next(); updatePresetLabel(); };
    presetPrev.setTooltip ("Previous preset");
    presetNext.setTooltip ("Next preset");

    styleButton (presetName, "flat");
    presetName.onClick = [this] { showPresetMenu(); };

    for (auto* b : { &saveBtn, &saveAsBtn, &renameBtn, &deleteBtn })
        styleButton (*b, "pill");
    deleteBtn.getProperties().set ("danger", true);
    saveBtn.onClick   = [this] { savePreset(); };
    saveAsBtn.onClick = [this] { savePresetAs(); };
    renameBtn.onClick = [this] { renamePreset(); };
    deleteBtn.onClick = [this] { deletePreset(); };

    // --- footer ---
    styleButton (updateBtn, "update");
    updateBtn.setVisible (false);
    updateBtn.onClick = [this] { showUpdateDialog(); };

    styleButton (resetBtn, "reset");
    resetBtn.onClick = [this]
    {
        for (int i = 0; i < 7; ++i) order[(size_t) i] = i;
        applyOrder();
    };

    // hint language (the hint line only). A user setting for all instances.
    styleButton (langRu, "flag");
    styleButton (langEn, "flag");
    langRu.getProperties().set ("flag", "ru");
    langEn.getProperties().set ("flag", "en");
    langRu.setConnectedEdges (juce::Button::ConnectedOnRight);
    langEn.setConnectedEdges (juce::Button::ConnectedOnLeft);
    langRu.setTooltip (juce::String::fromUTF8 ("Подсказки на русском"));
    langEn.setTooltip ("Hints in English");
    auto setLang = [this] (hints::Lang l)
    {
        hints::setLang (l);
        langRu.setToggleState (l == hints::Lang::ru, juce::dontSendNotification);
        langEn.setToggleState (l == hints::Lang::en, juce::dontSendNotification);
        repaint (hintBarBounds().expanded (4));
    };
    langRu.onClick = [setLang] { setLang (hints::Lang::ru); };
    langEn.onClick = [setLang] { setLang (hints::Lang::en); };
    langRu.setToggleState (hints::getLang() == hints::Lang::ru, juce::dontSendNotification);
    langEn.setToggleState (hints::getLang() == hints::Lang::en, juce::dontSendNotification);

    // window size: 100 % / 125 %. The window edge gives any size in between.
    styleButton (size100, "size");
    styleButton (size125, "size");
    size100.setConnectedEdges (juce::Button::ConnectedOnRight);
    size125.setConnectedEdges (juce::Button::ConnectedOnLeft);
    size100.setTooltip ("Window size 100%");
    size125.setTooltip ("Window size 125%. Drag the window edge for other sizes.");
    size100.onClick = [this] { if (onScaleRequest) onScaleRequest (1.0f); };
    size125.onClick = [this] { if (onScaleRequest) onScaleRequest (1.25f); };

    checker->checkOnce();

    setSize (designW, designH);
    applyDesign (proc.getDesign());
    updatePresetLabel();
    startTimerHz (30);
    timerCallback();
}

BerryToneView::~BerryToneView()
{
    stopTimer();
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto* aw = dynamic_cast<juce::AlertWindow*> (getChildComponent (i)))
        {
            removeChildComponent (aw);
            aw->setLookAndFeel (nullptr);
            aw->exitModalState (0);   // deleted later by the modal manager
        }
    setLookAndFeel (nullptr);
}

void BerryToneView::styleButton (juce::Button& b, const juce::String& style)
{
    b.getProperties().set ("style", style);
    addAndMakeVisible (b);
}

BerryToneView::Module& BerryToneView::addModule (
    const juce::String& title, const juce::String& flavor, const char* onId,
    std::initializer_list<std::pair<const char*, const char*>> knobs)
{
    auto m = std::make_unique<Module>();
    m->title = title;
    m->flavor = flavor;
    m->id = (int) modules.size();   // 0..6 = DSP module ids, 7 = master card

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
        kn->label.getProperties().set ("role", "knobName");
        addAndMakeVisible (kn->slider);
        addAndMakeVisible (kn->label);
        kn->att = std::make_unique<SliderAtt> (proc.apvts, k.second, kn->slider);
        m->knobs.push_back (std::move (kn));
    }

    modules.push_back (std::move (m));
    return *modules.back();
}

//==============================================================================
void BerryToneView::applyDesign (Design d)
{
    design = d;
    lnf.setDesign (d);
    scope.setDesign (d);
    sendLookAndFeelChange();   // slider text boxes pick up the new colours
    resized();
    repaint();
    if (auto* parent = getParentComponent())
        parent->repaint();     // the corner resizer uses the same look
}

void BerryToneView::updateScopeChips()
{
    // the TIME / LEVEL buttons belong to REAL DI only
    const bool real = scope.getMode() == ScopeView::Mode::real;
    for (auto* b : { &zoom1s, &zoom200, &zoom50, &lvlLin, &lvlDb })
        b->setVisible (real);
}

void BerryToneView::setCurrentScale (float s)
{
    size100.setToggleState (std::abs (s - 1.0f)  < 0.01f, juce::dontSendNotification);
    size125.setToggleState (std::abs (s - 1.25f) < 0.01f, juce::dontSendNotification);
}

void BerryToneView::resized()
{
    const bool grape = design == Design::grape;

    // header: preset bar
    presetPrev.setBounds (476, 31, 30, 26);
    presetName.setBounds (506, 31, 262, 26);
    presetNext.setBounds (768, 31, 30, 26);
    {
        int x = 474;
        const int w = (322 - 3 * 6) / 4;
        for (auto* b : { &saveBtn, &saveAsBtn, &renameBtn, &deleteBtn })
        {
            b->setBounds (x, 65, w, grape ? 24 : 20);
            x += w + 6;
        }
    }

    // DI scope
    scope.setBounds (scopeScreen);
    if (grape)
    {
        modeReal.setBounds (734, 154, 55, 24);
        modeSpec.setBounds (792, 154, 54, 24);
        modeFlat.setBounds (849, 154, 55, 24);
        pureBtn.setBounds (744, 186, 52, 52);
        holdBtn.setBounds (scopeScreen.getRight() - 68, scopeScreen.getBottom() - 27, 60, 20);
    }
    else
    {
        modeReal.setBounds (736, 156, 57, 24);
        modeSpec.setBounds (793, 156, 56, 24);
        modeFlat.setBounds (849, 156, 57, 24);
        pureBtn.setBounds (738, 188, 46, 46);
        holdBtn.setBounds (798, 201, 108, 22);
    }
    {
        juce::TextButton* chips[] = { &zoom1s, &zoom200, &zoom50, &lvlLin, &lvlDb };
        for (int i = 0; i < 5; ++i)
            chips[i]->setBounds (ScopeView::chipBounds (design, i).translated (scopeScreen.getX(), scopeScreen.getY()));
    }

    // footer
    // the update button sits at the right end of the hint line, in place of the version
    updateBtn.setBounds (grape ? 506 : 512, grape ? 662 : 663, grape ? 124 : 118, grape ? 22 : 20);
    resetBtn.setBounds (getWidth() - 22 - 96, 663, 96, grape ? 22 : 20);
    langRu.setBounds (644, grape ? 663 : 664, 34, grape ? 22 : 20);
    langEn.setBounds (678, grape ? 663 : 664, 34, grape ? 22 : 20);
    size100.setBounds (720, grape ? 663 : 664, 46, grape ? 22 : 20);
    size125.setBounds (766, grape ? 663 : 664, 46, grape ? 22 : 20);

    // module cards
    for (auto& mp : modules)
    {
        auto& m = *mp;
        m.bounds = slotRect (slotOf (m.id));

        if (grape) m.on.setBounds (m.bounds.getRight() - 56, m.bounds.getY() + 12, 44, 22);
        else       m.on.setBounds (m.bounds.getRight() - 46, m.bounds.getY() + 14, 34, 18);
        if (m.hasLed)
            gateLed = grape ? juce::Rectangle<int> (m.bounds.getRight() - 70, m.bounds.getY() + 19, 8, 8)
                            : juce::Rectangle<int> (m.bounds.getRight() - 64, m.bounds.getY() + 19, 9, 9);

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
            auto t = juce::Rectangle<int> (body.getX() + 2 * cellW + 6, body.getY() + 2,
                                           body.getRight() - (body.getX() + 2 * cellW + 6), body.getHeight() - 22);
            inMeter  = t.removeFromLeft (t.getWidth() / 2 - 2);
            outMeter = t.withTrimmedLeft (4);
        }
    }
}

//==============================================================================
juce::Rectangle<int> BerryToneView::slotRect (int slot) const
{
    const int margin = 22, gap = 12, top = 266, cardH = 186, cols = 4;
    const int cardW = (getWidth() - 2 * margin - (cols - 1) * gap) / cols;
    return { margin + (slot % cols) * (cardW + gap), top + (slot / cols) * (cardH + gap), cardW, cardH };
}

int BerryToneView::slotOf (int id) const
{
    if (id >= 7) return 7;   // master card is fixed
    return (int) (std::find (order.begin(), order.end(), id) - order.begin());
}

int BerryToneView::moduleAtHeader (juce::Point<int> p) const
{
    for (auto& m : modules)
        if (! m->isMaster && m->bounds.withHeight (46).withTrimmedRight (72).contains (p))
            return m->id;
    return -1;
}

void BerryToneView::applyOrder()
{
    proc.setOrder (order);
    resized();
    repaint();
}

void BerryToneView::mouseMove (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    if (logoRect.contains (pos))                setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else if (moduleAtHeader (pos) >= 0)         setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    else                                        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void BerryToneView::mouseDown (const juce::MouseEvent& e)
{
    if (logoRect.contains (e.getPosition()))
    {
        // click on the logo: the other design
        const auto next = design == Design::berry ? Design::grape : Design::berry;
        proc.setDesign (next);
        applyDesign (next);
        return;
    }

    dragId = moduleAtHeader (e.getPosition());
    if (dragId >= 0) repaint();
}

void BerryToneView::mouseDrag (const juce::MouseEvent& e)
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

void BerryToneView::mouseUp (const juce::MouseEvent&)
{
    if (dragId >= 0) { dragId = -1; repaint(); }
}

//==============================================================================
void BerryToneView::drawTube (juce::Graphics& g, juce::Rectangle<float> r, float level, const juce::String& name)
{
    using namespace berrycol;
    g.setColour (deep.withAlpha (0.88f));
    g.fillRoundedRectangle (r, 6.f);

    auto inner = r.reduced (3.f);
    const float fh = inner.getHeight() * juce::jlimit (0.f, 1.f, level);
    auto fill = inner.withTop (inner.getBottom() - fh);
    if (fh > 1.f)
    {
        g.setGradientFill (juce::ColourGradient (Colour (0xffff7ab8), 0.f, inner.getY(),
                                                 Colour (0xffc2108a), 0.f, inner.getBottom(), false));
        g.fillRoundedRectangle (fill, 4.f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.fillRoundedRectangle (inner.getX() + 2.f, inner.getY() + 2.f, 3.f, inner.getHeight() - 4.f, 1.5f);

    g.setColour (deep);
    g.setFont (F::font (9.5f));
    g.drawText (name, (int) r.getX() - 6, (int) r.getBottom() + 3, (int) r.getWidth() + 12, 12, juce::Justification::centred);
}

void BerryToneView::drawLeds (juce::Graphics& g, juce::Rectangle<float> r, float level, const juce::String& name)
{
    using namespace grapecol;
    g.setColour (screen);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (borderDim);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);

    const int n = 16;
    auto in = r.reduced (3.f);
    const float sh = (in.getHeight() - (float) (n - 1) * 2.f) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        const bool lit = (float) i / (float) n < level;
        const auto base = i >= 14 ? ledRed : (i >= 11 ? ledYellow : ledGreen);
        auto seg = juce::Rectangle<float> (in.getX(), in.getBottom() - (float) (i + 1) * sh - (float) i * 2.f, in.getWidth(), sh);
        if (lit)
        {
            g.setColour (base.withAlpha (0.3f));
            g.fillRoundedRectangle (seg.expanded (1.f), 2.f);
        }
        g.setColour (lit ? base : ledOff);
        g.fillRoundedRectangle (seg, 1.5f);
    }

    g.setColour (label);
    g.setFont (F::font (9.f, true, 0.08f));
    g.drawText (name, (int) r.getX() - 8, (int) r.getBottom() + 3, (int) r.getWidth() + 16, 12, juce::Justification::centred);
}

void BerryToneView::paint (juce::Graphics& g)
{
    if (design == Design::grape) paintGrape (g);
    else                         paintBerry (g);
}

//==============================================================================
void BerryToneView::paintBerry (juce::Graphics& g)
{
    using namespace berrycol;
    const float w = (float) getWidth(), h = (float) getHeight();
    const bool pure = proc.pureDi.load();

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

    // --- logo (click = Grape design) ---
    F::drawBerry (g, 50.f, 66.f, 15.f);
    F::drawBerry (g, 78.f, 54.f, 13.f);
    F::drawBerry (g, 74.f, 82.f, 12.f);

    g.setFont (F::font (46.f));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText ("BERRY TONE", 112, 34, 360, 54, juce::Justification::centredLeft);
    g.setColour (cream);
    g.drawText ("BERRY TONE", 110, 31, 360, 54, juce::Justification::centredLeft);

    g.setFont (F::font (11.f));
    g.setColour (cream.withAlpha (0.85f));
    g.drawText ("LOW-TUNED GUITAR JUICER  -  0 SAMPLES LATENCY",      // ends before the preset bar (x 474)
                112, 84, 350, 14, juce::Justification::centredLeft);

    // --- preset bar ---
    {
        auto bar = juce::Rectangle<float> (474.f, 30.f, 322.f, 28.f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (bar.translated (0.f, 3.f), 14.f);
        g.setColour (cream);
        g.fillRoundedRectangle (bar, 14.f);
        g.setColour (pink.withAlpha (0.25f));
        g.fillRect (505.5f, 30.f, 1.5f, 28.f);
        g.fillRect (767.5f, 30.f, 1.5f, 28.f);
    }

    // --- straw ---
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillEllipse (822.f, 82.f, 32.f, 12.f);
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

    // --- DI scope card ---
    {
        juce::Path card;
        card.addRoundedRectangle (scopeCard.toFloat(), 14.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 10, { 0, 4 }).drawForPath (g, card);
        g.setColour (cream);
        g.fillPath (card);
        g.setColour (deep);
        g.setFont (F::font (14.f));
        g.drawText ("DI Scope", 736, 116, 150, 18, juce::Justification::centredLeft);
        g.setColour (pink.withAlpha (0.9f));
        g.setFont (F::font (10.5f, false).italicised());
        g.drawText ("juice x-ray", 736, 134, 150, 13, juce::Justification::centredLeft);

        auto seg = juce::Rectangle<float> (736.f, 156.f, 170.f, 24.f);
        g.setColour (pink.withAlpha (0.35f));
        g.drawRoundedRectangle (seg.reduced (0.75f), 12.f, 1.5f);

        g.setColour (deep);
        g.setFont (F::font (9.5f));
        g.drawText ("PURE DI", 731, 236, 60, 12, juce::Justification::centred);
    }

    // --- module cards ---
    for (auto& mp : modules)
    {
        auto& m = *mp;
        const auto r = m.bounds;
        const bool on = m.isMaster || (m.on.getToggleState() && ! pure);

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

        const float bx = (float) r.getX() + 22.f, by = (float) r.getY() + 25.f;
        if (m.isMaster)
            F::drawBerry (g, bx, by + 2.f, 8.f);
        else
        {
            g.setColour (on ? pink : offGrey);
            g.fillEllipse (bx - 11.f, by - 11.f, 22.f, 22.f);
            g.setColour (juce::Colours::white);
            g.setFont (F::font (12.f));
            g.drawText (juce::String (slotOf (m.id) + 1), (int) bx - 11, (int) by - 11, 22, 22, juce::Justification::centred);
        }

        const int tw = r.getWidth() - 40 - (m.hasLed ? 72 : 52);
        g.setColour (deep.withAlpha (on ? 1.f : 0.45f));
        g.setFont (F::font (14.f));
        g.drawFittedText (m.title, r.getX() + 40, r.getY() + 10, tw, 18, juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (pink.withAlpha (on ? 0.9f : 0.4f));
        g.setFont (F::font (10.5f, false).italicised());
        g.drawText (m.flavor, r.getX() + 40, r.getY() + 28, tw, 13, juce::Justification::centredLeft);

        if (m.isMaster)
        {
            drawTube (g, inMeter.toFloat(),  inLvl,  "IN");
            drawTube (g, outMeter.toFloat(), outLvl, "OUT");
        }
    }

    // gate activity LED (open = mint)
    g.setColour (gateOpenLed ? mint : offGrey);
    g.fillEllipse (gateLed.toFloat());
    g.setColour (deep.withAlpha (0.3f));
    g.drawEllipse (gateLed.toFloat(), 1.f);

    // footer
    paintHintBar (g);
}

//==============================================================================
void BerryToneView::paintGrape (juce::Graphics& g)
{
    using namespace grapecol;
    const float w = (float) getWidth(), h = (float) getHeight();
    const bool pure = proc.pureDi.load();

    // --- background ---
    juce::ColourGradient bg (bgTop, w * 0.5f, 0.f, bgBot, w * 0.5f, h * 1.1f, true);
    bg.addColour (0.45, bgMid);
    g.setGradientFill (bg);
    g.fillAll();

    // --- logo (click = Berry design) ---
    F::drawGrapeCluster (g, 26.f, 22.f, 1.f);
    {
        const auto f = F::font (52.f, true, 0.02f);
        g.setFont (f);
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (glow.withAlpha (0.12f));
            g.drawText ("Grape", 102 - i, 20 - i, 260 + 2 * i, 62 + 2 * i, juce::Justification::centredLeft);
            g.drawText ("Grape", 102 + i, 20 + i, 260, 62, juce::Justification::centredLeft);
        }
        g.setColour (border);
        g.drawText ("Grape", 102, 22, 260, 62, juce::Justification::centredLeft);
        g.setColour (text);
        g.drawText ("Grape", 102, 20, 260, 62, juce::Justification::centredLeft);
    }
    g.setFont (F::font (10.5f, true, 0.1f));
    g.setColour (flavor);
    g.drawText ("LOW-TUNED CONDITIONER  -  0 SAMPLES LATENCY",        // ends before the preset bar (x 474)
                104, 84, 360, 14, juce::Justification::centredLeft);

    // --- preset bar ---
    {
        auto bar = juce::Rectangle<float> (474.f, 29.f, 322.f, 30.f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (bar.translated (0.f, 3.f), 15.f);
        g.setGradientFill (juce::ColourGradient::vertical (panelTop, bar.getY(), panelBot, bar.getBottom()));
        g.fillRoundedRectangle (bar, 15.f);
        g.setColour (border);
        g.drawRoundedRectangle (bar.reduced (0.75f), 15.f, 1.5f);
        g.fillRect (505.5f, 31.f, 1.f, 26.f);
        g.fillRect (767.5f, 31.f, 1.f, 26.f);
    }

    // --- vine ornament (top right) ---
    {
        juce::Path vine;
        vine.startNewSubPath (820.f, 66.f);
        vine.cubicTo (832.f, 36.f, 862.f, 26.f, 876.f, 46.f);
        vine.cubicTo (886.f, 60.f, 872.f, 76.f, 860.f, 68.f);
        vine.cubicTo (852.f, 62.f, 858.f, 50.f, 866.f, 54.f);
        juce::Path vine2;
        vine2.startNewSubPath (876.f, 46.f);
        vine2.cubicTo (884.f, 26.f, 904.f, 20.f, 914.f, 28.f);
        g.setColour (gold);
        g.strokePath (vine, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.strokePath (vine2, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        F::drawLeaf (g, { 834.f, 46.f }, { 830.f, 24.f }, 7.f, leaf);
        F::drawLeaf (g, { 898.f, 24.f }, { 916.f, 12.f }, 6.f, leaf);
        F::drawGrapeCluster (g, 880.f, 62.f, 0.42f);
    }

    // --- DI scope panel ---
    auto drawPanel = [&g] (juce::Rectangle<float> r, bool live, bool highlight)
    {
        juce::Path p;
        p.addRoundedRectangle (r, 12.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 16, { 0, 6 }).drawForPath (g, p);
        g.setGradientFill (juce::ColourGradient::vertical (panelTop, r.getY(), panelBot, r.getBottom()));
        g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 10.f, r.getRight() - 10.f);
        g.setColour (highlight ? trace : (live ? borderLive : borderDim));
        g.strokePath (p, juce::PathStrokeType (highlight ? 2.f : 1.5f));
        g.setColour (border);
        for (auto pt : { juce::Point<float> (r.getX() + 7.5f, r.getY() + 7.5f), juce::Point<float> (r.getRight() - 7.5f, r.getY() + 7.5f),
                         juce::Point<float> (r.getX() + 7.5f, r.getBottom() - 7.5f), juce::Point<float> (r.getRight() - 7.5f, r.getBottom() - 7.5f) })
            g.fillEllipse (pt.x - 2.5f, pt.y - 2.5f, 5.f, 5.f);
    };

    drawPanel (scopeCard.toFloat(), true, false);
    g.setColour (text);
    g.setFont (F::font (13.f, true, 0.08f));
    g.drawText ("DI SCOPE", 734, 114, 150, 18, juce::Justification::centredLeft);
    g.setColour (flavor);
    g.setFont (F::font (10.f, false).italicised());
    g.drawText ("grape x-ray", 734, 132, 150, 13, juce::Justification::centredLeft);
    g.setColour (text);
    g.setFont (F::font (11.f, true, 0.08f));
    g.drawText ("PURE DI", 806, 198, 90, 14, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (F::font (9.5f, false));
    g.drawText ("mute all modules", 806, 214, 100, 12, juce::Justification::centredLeft);

    // --- module panels ---
    for (auto& mp : modules)
    {
        auto& m = *mp;
        const auto r = m.bounds;
        const bool on = m.isMaster || (m.on.getToggleState() && ! pure);

        drawPanel (r.toFloat(), on, m.id == dragId);

        juce::ColourGradient sep (border.withAlpha (0.f), (float) r.getX() + 14.f, 0.f, border.withAlpha (0.f), (float) r.getRight() - 14.f, 0.f, false);
        sep.addColour (0.2, border);
        sep.addColour (0.8, border);
        g.setGradientFill (sep);
        g.fillRect ((float) r.getX() + 14.f, (float) r.getY() + 45.f, (float) r.getWidth() - 28.f, 1.f);

        const float bx = (float) r.getX() + 21.f, by = (float) r.getY() + 23.f;
        if (m.isMaster)
            F::drawGrapeIcon (g, { bx - 8.f, by - 9.f, 16.f, 18.f }, accent);
        else
        {
            auto badge = juce::Rectangle<float> (bx - 10.f, by - 10.f, 20.f, 20.f);
            if (on)
            {
                g.setColour (glow.withAlpha (0.25f));
                g.fillEllipse (badge.expanded (3.f));
            }
            g.setColour (on ? border : panelBot);
            g.fillEllipse (badge);
            g.setColour (on ? accent : borderDim);
            g.drawEllipse (badge.reduced (0.75f), 1.5f);
            g.setColour (on ? text : dim);
            g.setFont (F::font (11.f));
            g.drawText (juce::String (slotOf (m.id) + 1), badge, juce::Justification::centred);
        }

        const int tw = r.getWidth() - 40 - (m.hasLed ? 80 : 62);
        g.setColour (on ? text : dim);
        g.setFont (F::font (12.f, true, 0.05f));
        g.drawFittedText (m.title.toUpperCase(), r.getX() + 40, r.getY() + 11, tw, 16, juce::Justification::centredLeft, 1, 0.6f);
        g.setColour (on ? flavor : Colour (0xff5d3a58));
        g.setFont (F::font (10.f, false).italicised());
        g.drawText (m.flavor, r.getX() + 40, r.getY() + 28, tw, 13, juce::Justification::centredLeft);

        if (m.isMaster)
        {
            drawLeds (g, inMeter.toFloat().withTrimmedLeft (2.f).withTrimmedRight (2.f),  inLvl,  "IN");
            drawLeds (g, outMeter.toFloat().withTrimmedLeft (2.f).withTrimmedRight (2.f), outLvl, "OUT");
        }
    }

    // gate activity LED (open = green)
    auto led = gateLed.toFloat();
    if (gateOpenLed)
    {
        g.setColour (ledGreen.withAlpha (0.35f));
        g.fillEllipse (led.expanded (2.5f));
    }
    g.setColour (gateOpenLed ? ledGreen : Colour (0xff2a2328));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black);
    g.drawEllipse (led, 1.f);

    // footer
    paintHintBar (g);
}

//==============================================================================
// Hint line
juce::Rectangle<int> BerryToneView::hintBarBounds() const
{
    return design == Design::grape ? juce::Rectangle<int> (22, 659, 614, 28) : juce::Rectangle<int> (22, 660, 614, 26);
}

void BerryToneView::paintHintBar (juce::Graphics& g)
{
    const bool grape = design == Design::grape;
    const auto bar = hintBarBounds().toFloat();
    if (grape)
    {
        using namespace grapecol;
        g.setGradientFill (juce::ColourGradient::vertical (panelTop, bar.getY(), panelBot, bar.getBottom()));
        g.fillRoundedRectangle (bar, 6.f);
        g.setColour (border);
        g.drawRoundedRectangle (bar.reduced (0.5f), 6.f, 1.f);
        F::drawGrapeIcon (g, { bar.getX() + 12.f, bar.getCentreY() - 6.5f, 12.f, 13.f }, accent);
    }
    else
    {
        juce::Path p;
        p.addRoundedRectangle (bar, bar.getHeight() * 0.5f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.25f), 6, { 0, 2 }).drawForPath (g, p);
        g.setColour (berrycol::cream.withAlpha (0.93f));
        g.fillPath (p);
        F::drawBerry (g, bar.getX() + 19.f, bar.getCentreY() + 0.5f, 5.5f);
    }

    // right end: the version, or the update button
    float right = bar.getRight() - 12.f;
    if (updateBtn.isVisible())
        right = (float) updateBtn.getX() - 8.f;
    else
    {
        g.setFont (F::font (10.f, true, grape ? 0.08f : 0.f));
        g.setColour (grape ? grapecol::muted : berrycol::deep.withAlpha (0.55f));
        const juce::String v ("v" JucePlugin_VersionString);
        const float vw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), v);
        g.drawText (v, juce::Rectangle<float> (right - vw, bar.getY(), vw + 2.f, bar.getHeight()), juce::Justification::centredRight);
        right -= vw + 10.f;
    }

    const auto lang = hints::getLang();
    juce::String name, text;
    if (hintModule >= 0 && hintModule < (int) modules.size())
    {
        const auto& m = *modules[(size_t) hintModule];
        if (hintKnob >= 0 && hintKnob < (int) m.knobs.size())
        {
            name = m.knobs[(size_t) hintKnob]->label.getText().toUpperCase();
            text = hints::knobText (lang, m.id, hintKnob);
        }
        else
        {
            name = m.title.toUpperCase();
            text = hints::moduleText (lang, m.id);
        }
    }
    else
        text = hints::idleText (lang);

    float x = bar.getX() + (grape ? 32.f : 31.f);
    const auto accentC = grape ? grapecol::trace : berrycol::pink;
    if (name.isNotEmpty())
    {
        g.setFont (F::font (11.f, true, grape ? 0.08f : 0.02f));
        const juce::String head = name + juce::String::fromUTF8 ("  —  ");   // name + em dash
        const float hw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), head);
        g.setColour (accentC);
        g.drawText (head, juce::Rectangle<float> (x, bar.getY(), hw + 2.f, bar.getHeight()), juce::Justification::centredLeft);
        x += hw;
    }
    g.setFont (F::font (11.f, false));
    g.setColour (grape ? grapecol::text : berrycol::deep);
    g.drawText (text, juce::Rectangle<float> (x, bar.getY(), juce::jmax (0.f, right - x), bar.getHeight()),
                juce::Justification::centredLeft, true);
}

void BerryToneView::updateHint()
{
    int mod = -1, knob = -1;
    const bool dialogOpen = juce::Component::getCurrentlyModalComponent() != nullptr;
    if (isShowing() && ! dialogOpen && isMouseOver (true))
    {
        const auto p = getLocalPoint (nullptr, juce::Desktop::getInstance().getMainMouseSource().getScreenPosition()).toInt();
        for (auto& mp : modules)
        {
            const auto& m = *mp;
            if (! m.bounds.contains (p)) continue;
            mod = (int) (&mp - &modules[0]);
            for (size_t k = 0; k < m.knobs.size(); ++k)
                if (m.knobs[k]->slider.getBounds().contains (p) || m.knobs[k]->label.getBounds().contains (p))
                    knob = (int) k;
            break;
        }
    }
    // a knob drag keeps its hint, also when the mouse leaves the knob
    for (size_t i = 0; i < modules.size(); ++i)
        for (size_t k = 0; k < modules[i]->knobs.size(); ++k)
            if (modules[i]->knobs[k]->slider.isMouseButtonDown()) { mod = (int) i; knob = (int) k; }

    if (mod != hintModule || knob != hintKnob)
    {
        hintModule = mod;
        hintKnob = knob;
        repaint (hintBarBounds().expanded (4));
    }
}

//==============================================================================
void BerryToneView::timerCallback()
{
    updateHint();

    auto norm = [] (float lin)
    {
        return juce::jlimit (0.f, 1.f, (juce::Decibels::gainToDecibels (lin, -60.f) + 60.f) / 60.f);
    };

    const float i = norm (proc.inPeak.exchange (0.f));
    const float o = norm (proc.outPeak.exchange (0.f));
    inLvl  = i > inLvl  ? i : inLvl  * 0.88f;
    outLvl = o > outLvl ? o : outLvl * 0.88f;
    repaint (inMeter.getUnion (outMeter).expanded (10, 16));

    // the host or a preset can change the design or the order
    if (proc.getDesign() != design)
        applyDesign (proc.getDesign());
    if (dragId < 0 && proc.getOrder() != order)
    {
        order = proc.getOrder();
        resized();
        repaint();
    }

    const bool pure = proc.pureDi.load();
    if (pure != pureBtn.getToggleState())
        pureBtn.setToggleState (pure, juce::dontSendNotification);
    if (pure != lastPure)
    {
        lastPure = pure;
        for (auto& m : modules) m->lastAlpha = -1.f;
        repaint();
    }

    const bool gOpen = proc.gateGain.load() > 0.5f;
    if (gOpen != gateOpenLed) { gateOpenLed = gOpen; repaint (gateLed.expanded (4)); }

    for (auto& mp : modules)
    {
        auto& m = *mp;
        if (m.isMaster) continue;
        const float a = (m.on.getToggleState() && ! pure) ? 1.f : 0.4f;
        if (a != m.lastAlpha)
        {
            m.lastAlpha = a;
            for (auto& k : m.knobs)
            {
                k->slider.setAlpha (a);
                k->label.setAlpha (a);
            }
            repaint (m.bounds.expanded (12));
        }
    }

    updatePresetLabel();

    if (checker->hasUpdate() && ! updateBtn.isVisible())
    {
        const auto v = checker->getInfo().version;
        updateBtn.setButtonText ("Update to v" + v);
        updateBtn.setTooltip ("BerryTone v" + v + " is on GitHub. Click to install.");
        updateBtn.setVisible (true);
        repaint (hintBarBounds().expanded (4));
    }
}

//==============================================================================
// Presets
void BerryToneView::updatePresetLabel()
{
    auto t = proc.presets.getCurrentName();
    if (proc.presets.isModified()) t << " *";
    if (t != lastPresetText)
    {
        lastPresetText = t;
        presetName.setButtonText (t);
        presetName.setTooltip (proc.presets.isModified() ? "The preset has changes. Click Save to keep them." : "Click to choose a preset");
    }
    const bool user = ! proc.presets.currentIsFactory() && proc.presets.getCurrentIndex() >= 0;
    renameBtn.setEnabled (user);
    deleteBtn.setEnabled (user);
}

void BerryToneView::showPresetMenu()
{
    auto& pm = proc.presets;
    pm.refresh();
    const auto& list = pm.getEntries();
    const int cur = pm.getCurrentIndex();

    juce::PopupMenu menu;
    menu.addSectionHeader ("FACTORY");
    bool userHeader = false;
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& e = list[i];
        if (! e.factory && ! userHeader)
        {
            menu.addSeparator();
            menu.addSectionHeader ("USER");
            userHeader = true;
        }
        juce::PopupMenu::Item item (e.name);
        item.itemID = (int) i + 1;
        item.isTicked = (int) i == cur;
        item.shortcutKeyDescription = e.design == Design::grape ? "GRAPE" : "BERRY";
        menu.addItem (item);
    }
    if (! userHeader)
    {
        menu.addSeparator();
        menu.addSectionHeader ("USER");
        menu.addItem (-1, "(no user presets yet)", false);
    }
    menu.addSeparator();
    menu.addItem (10000, "Open presets folder");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetName).withParentComponent (this)
                                                  .withMinimumWidth (presetName.getWidth()),
                        [safe = SafePointer<BerryToneView> (this)] (int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == 10000)
        {
            auto dir = PresetManager::getUserDir();
            dir.createDirectory();
            dir.startAsProcess();
            return;
        }
        safe->proc.presets.loadIndex (r - 1);
        safe->updatePresetLabel();
    });
}

void BerryToneView::savePreset()
{
    auto& pm = proc.presets;
    if (pm.currentIsFactory() || pm.getCurrentIndex() < 0)
    {
        savePresetAs();   // factory presets are read only
        return;
    }
    const auto r = pm.save();
    if (r.failed()) showMessage ("Cannot save", r.getErrorMessage());
    updatePresetLabel();
}

void BerryToneView::savePresetAs()
{
    const auto initial = proc.presets.currentIsFactory() || proc.presets.getCurrentIndex() < 0
                             ? juce::String() : proc.presets.getCurrentName();
    askText ("Save preset as",
             juce::String ("The preset stores all knobs, switches, module order and the current design (")
                 + (design == Design::grape ? "Grape" : "Berry") + ").",
             initial, "Save",
             [safe = SafePointer<BerryToneView> (this)] (juce::String name)
    {
        if (safe == nullptr) return;
        auto doSave = [safe, name]
        {
            if (safe == nullptr) return;
            const auto r = safe->proc.presets.saveAs (name);
            if (r.failed()) safe->showMessage ("Cannot save", r.getErrorMessage());
            safe->updatePresetLabel();
        };
        if (safe->proc.presets.userPresetExists (name))
            safe->askConfirm ("Replace preset?", "\"" + name.trim() + "\" exists. Replace it?", "Replace", true, doSave);
        else
            doSave();
    });
}

void BerryToneView::renamePreset()
{
    if (proc.presets.currentIsFactory() || proc.presets.getCurrentIndex() < 0)
    {
        showMessage ("Rename", "You cannot rename a factory preset. Use Save as.");
        return;
    }
    askText ("Rename preset", "Type a new name.", proc.presets.getCurrentName(), "Rename",
             [safe = SafePointer<BerryToneView> (this)] (juce::String name)
    {
        if (safe == nullptr) return;
        const auto r = safe->proc.presets.rename (name);
        if (r.failed()) safe->showMessage ("Cannot rename", r.getErrorMessage());
        safe->updatePresetLabel();
    });
}

void BerryToneView::deletePreset()
{
    if (proc.presets.currentIsFactory() || proc.presets.getCurrentIndex() < 0)
    {
        showMessage ("Delete", "You cannot delete a factory preset.");
        return;
    }
    askConfirm ("Delete preset?", "\"" + proc.presets.getCurrentName() + "\" will be deleted. You cannot undo this.",
                "Delete", true,
                [safe = SafePointer<BerryToneView> (this)]
    {
        if (safe == nullptr) return;
        const auto r = safe->proc.presets.remove();
        if (r.failed()) safe->showMessage ("Cannot delete", r.getErrorMessage());
        safe->updatePresetLabel();
    });
}

//==============================================================================
// Updates from GitHub
void BerryToneView::showUpdateDialog()
{
    const auto info = checker->getInfo();
    auto notes = info.notes;
    if (notes.length() > 600) notes = notes.substring (0, 600) + " ...";

    auto* aw = makeDialog ("Update available",
                           "BerryTone v" + info.version + " is on GitHub. You use v" JucePlugin_VersionString "."
                               + (notes.isNotEmpty() ? "\n\n" + notes : juce::String()));
   #if JUCE_WINDOWS
    if (info.assetUrl.isNotEmpty())
    {
        aw->addButton ("Install", 1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->getButton ("Install")->getProperties().set ("primary", true);
    }
   #endif
    aw->addButton ("Release page", 2);
    aw->addButton ("Later", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    openDialog (aw, [safe = SafePointer<BerryToneView> (this), info] (int r, juce::AlertWindow&)
    {
        if (safe == nullptr) return;
        if (r == 1) safe->startInstall();
        if (r == 2) juce::URL (info.pageUrl).launchInDefaultBrowser();
    });
}

void BerryToneView::startInstall()
{
    const auto info = checker->getInfo();
    installer = std::make_unique<updater::UpdateInstaller>();
    installer->onFinished = [safe = SafePointer<BerryToneView> (this), info] (bool ok, const juce::String& msg)
    {
        if (safe == nullptr) return;
        if (safe->progressDialog != nullptr)
            safe->progressDialog->exitModalState (0);

        if (ok)
        {
            safe->updateBtn.setButtonText ("Restart the DAW");
            safe->updateBtn.setEnabled (false);
            safe->showMessage ("Update installed", msg);
        }
        else
        {
            auto* aw = safe->makeDialog ("Update failed", msg + "\n\nYou can download the update from the release page.");
            aw->addButton ("Release page", 2);
            aw->addButton ("Close", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            safe->openDialog (aw, [info] (int r, juce::AlertWindow&)
            {
                if (r == 2) juce::URL (info.pageUrl).launchInDefaultBrowser();
            });
        }
    };

    auto* aw = makeDialog ("Installing update",
                           "Downloading BerryTone v" + info.version + " from GitHub.\n"
                           "Windows can ask for admin rights (UAC). Allow it to finish the install.");
    aw->addProgressBarComponent (installer->progress);
    aw->addButton ("Hide", 0);
    progressDialog = aw;
    openDialog (aw, nullptr);

    installer->start (info);
}

//==============================================================================
// Dialogs inside the plugin window
juce::AlertWindow* BerryToneView::makeDialog (const juce::String& title, const juce::String& text)
{
    auto* aw = new juce::AlertWindow (design == Design::grape ? title.toUpperCase() : title, text,
                                      juce::MessageBoxIconType::NoIcon, this);
    aw->setLookAndFeel (&lnf);
    return aw;
}

void BerryToneView::openDialog (juce::AlertWindow* aw, std::function<void (int, juce::AlertWindow&)> onClose)
{
    addAndMakeVisible (aw);
    aw->setCentrePosition (getLocalBounds().getCentre());
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([aw, onClose] (int r)
    {
        aw->setLookAndFeel (nullptr);
        if (onClose) onClose (r, *aw);
    }), true);
}

void BerryToneView::showMessage (const juce::String& title, const juce::String& text)
{
    auto* aw = makeDialog (title, text);
    aw->addButton ("OK", 0, juce::KeyPress (juce::KeyPress::returnKey));
    aw->getButton ("OK")->getProperties().set ("primary", true);
    openDialog (aw, nullptr);
}

void BerryToneView::askText (const juce::String& title, const juce::String& text, const juce::String& initial,
                                             const juce::String& okText, std::function<void (juce::String)> onOk)
{
    auto* aw = makeDialog (title, text);
    aw->addTextEditor ("name", initial, "Name");
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->getButton (okText)->getProperties().set ("primary", true);
    openDialog (aw, [onOk] (int r, juce::AlertWindow& w)
    {
        if (r == 1 && onOk) onOk (w.getTextEditorContents ("name"));
    });
    if (auto* te = aw->getTextEditor ("name"))
    {
        const bool grape = design == Design::grape;
        te->setColour (juce::TextEditor::backgroundColourId, grape ? grapecol::screen : juce::Colours::white);
        te->setColour (juce::TextEditor::outlineColourId, grape ? grapecol::borderLive : berrycol::pink.withAlpha (0.4f));
        te->setColour (juce::TextEditor::focusedOutlineColourId, grape ? grapecol::trace : berrycol::pink);
        te->setColour (juce::TextEditor::textColourId, grape ? grapecol::text : berrycol::deep);
        te->setFont (F::font (14.f, false));
        te->applyColourToAllText (grape ? grapecol::text : berrycol::deep);
        te->selectAll();
        te->grabKeyboardFocus();
    }
}

void BerryToneView::askConfirm (const juce::String& title, const juce::String& text, const juce::String& okText,
                                                bool danger, std::function<void()> onOk)
{
    auto* aw = makeDialog (title, text);
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->addButton (okText, 1);
    aw->getButton (okText)->getProperties().set (danger ? "danger" : "primary", true);
    openDialog (aw, [onOk] (int r, juce::AlertWindow&)
    {
        if (r == 1 && onOk) onOk();
    });
}

//==============================================================================
BerryToneAudioProcessorEditor::BerryToneAudioProcessorEditor (BerryToneAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), view (p)
{
    setLookAndFeel (&view.getLookAndFeelTheme());   // the corner resizer uses the theme
    addAndMakeVisible (view);
    view.onScaleRequest = [this] (float s) { setScale (s); };

    // read the saved scale first: setResizeLimits() resizes the editor (and calls resized())
    const float s = juce::jlimit (minScale, maxScale, proc.uiScale.load());

    constexpr int w = BerryToneView::designW, h = BerryToneView::designH;
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (w * minScale), juce::roundToInt (h * minScale),
                     juce::roundToInt (w * maxScale), juce::roundToInt (h * maxScale));
    getConstrainer()->setFixedAspectRatio ((double) w / (double) h);

    setSize (juce::roundToInt (w * s), juce::roundToInt (h * s));
}

BerryToneAudioProcessorEditor::~BerryToneAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void BerryToneAudioProcessorEditor::setScale (float s)
{
    s = juce::jlimit (minScale, maxScale, s);
    setSize (juce::roundToInt (BerryToneView::designW * s), juce::roundToInt (BerryToneView::designH * s));
}

void BerryToneAudioProcessorEditor::resized()
{
    // the view keeps its design size; a transform scales it to the window
    const float sx = (float) getWidth()  / (float) BerryToneView::designW;
    const float sy = (float) getHeight() / (float) BerryToneView::designH;
    view.setBounds (0, 0, BerryToneView::designW, BerryToneView::designH);
    view.setTransform (juce::AffineTransform::scale (sx, sy));

    proc.uiScale = sx;
    view.setCurrentScale (sx);
}
