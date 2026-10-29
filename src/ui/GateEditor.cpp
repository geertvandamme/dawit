#include "GuideButton.h"
#include "GateEditor.h"

namespace daw
{
using gate::Knob;
using namespace fxui;

namespace
{
    /** The page of "What is a gate?" for a topic of the window (see gatetext::makeGuide). */
    constexpr int thresholdPage = 1, keyPage = 7;

    float roundTo (float value, bool fine)
    {
        return fine ? std::round (value * 10.0f) / 10.0f : std::round (value);
    }

    bool isNearTo (juce::Point<float> a, juce::Point<float> b, float distance = 9.0f)
    {
        return a.getDistanceFrom (b) <= distance;
    }
}

//==============================================================================
GateCurveView::GateCurveView (BuiltInGate& p, const std::vector<gate::Moment>& recent, std::function<void (int)> openGuide)
    : processor (p), moments (recent), onOpenGuide (std::move (openGuide))
{
    setTopic (*this, gatetext::curve);
}

juce::Rectangle<float> GateCurveView::getCurveBounds() const
{
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    r.removeFromTop (18.0f);
    r.removeFromBottom (26.0f);
    return r;
}

void GateCurveView::paint (juce::Graphics& g)
{
    const auto s = processor.getSettings();
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE CURVE: IN ACROSS, OUT UP", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    // The track as a dot, with a short trail of where it was.
    std::optional<std::pair<float, float>> dot;
    std::vector<std::pair<float, float>> trail;
    const int count = (int) moments.size();

    for (int i = juce::jmax (0, count - 24); i < count; i += 2)
        trail.push_back ({ moments[(size_t) i].detectorDb, moments[(size_t) i].detectorDb - moments[(size_t) i].reductionDb });

    if (count > 0 && moments.back().detectorDb > -90.0f)
        dot = std::pair<float, float> { moments.back().detectorDb, moments.back().detectorDb - moments.back().reductionDb };

    gatetext::drawCurve (g, getCurveBounds(), s, true, dot, trail);

    // The light: open or closed, and how far the sound is turned down.
    const bool heard = count > 0 && moments.back().detectorDb > -90.0f, open = heard && moments.back().open;
    const float reduction = count > 0 ? moments.back().reductionDb : 0.0f;
    auto light = r.removeFromBottom (20.0f);
    g.setColour (open ? gatetext::colour : theme::textDim.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ light.getX() + 8.0f, light.getCentreY() }));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.setColour (open ? gatetext::colour : theme::textDim);
    g.drawText (heard ? (open ? "OPEN" : "CLOSED") : "NO SOUND", light.withTrimmedLeft (20.0f).removeFromLeft (80.0f), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (12.0f));
    g.setColour (theme::textDim);
    g.drawText (heard ? "turned down by " + juce::String (juce::roundToInt (reduction)) + " dB" : juce::String ("play the track, or click Hits and hiss"),
                light.withTrimmedLeft (100.0f), juce::Justification::centredLeft, true);
}

GateCurveView::Dot GateCurveView::getDotAt (juce::Point<float> p) const
{
    const auto dots = gatetext::getCurveDots (gatetext::getCurveSquare (getCurveBounds(), true), processor.getSettings());

    const bool nearThreshold = isNearTo (p, dots.threshold), nearClosing = isNearTo (p, dots.closing);

    // At a range of 0 the range dot sits on the threshold dot: the threshold wins there.
    if (isNearTo (p, dots.range) && ! (nearThreshold && processor.getSettings().rangeDb < 0.5f))
        return Dot::range;

    if (nearThreshold && nearClosing)
        return p.x < dots.threshold.x ? Dot::closing : Dot::threshold;

    return nearThreshold ? Dot::threshold : nearClosing ? Dot::closing : Dot::none;
}

juce::RangedAudioParameter* GateCurveView::getParameter (Dot d) const
{
    switch (d)
    {
        case Dot::threshold: return processor.getKnobParameter (Knob::threshold);
        case Dot::closing:   return processor.getKnobParameter (Knob::hysteresis);
        case Dot::range:     return processor.getKnobParameter (Knob::range);
        case Dot::none:      break;
    }

    return nullptr;
}

void GateCurveView::mouseMove (const juce::MouseEvent& e)
{
    const auto dot = getDotAt (e.position);
    setMouseCursor (dot == Dot::range ? juce::MouseCursor::UpDownResizeCursor
                                      : dot != Dot::none ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void GateCurveView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    dragging = getDotAt (e.position);
    dragStart = processor.getSettings();

    if (auto* p = getParameter (dragging))
        p->beginChangeGesture();
}

void GateCurveView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Dot::none)
        return;

    const auto square = gatetext::getCurveSquare (getCurveBounds(), true);
    const bool fine = e.mods.isAltDown();
    const float db = gatetext::curvePositionToDb (square, e.position.x, false);

    switch (dragging)
    {
        case Dot::threshold:
            processor.setKnob (Knob::threshold, juce::jlimit (gate::minThreshold, gate::maxThreshold, roundTo (db, fine)));
            break;

        case Dot::closing:
            processor.setKnob (Knob::hysteresis, juce::jlimit (0.0f, gate::maxHysteresis, roundTo (processor.getSettings().thresholdDb - db, fine)));
            break;

        case Dot::range:
            processor.setKnob (Knob::range, juce::jlimit (0.0f, gate::maxRange, roundTo (dragStart.rangeDb + (float) e.getDistanceFromDragStartY() * 80.0f / square.getHeight(), fine)));
            break;

        case Dot::none:
            break;
    }

    repaint();
}

void GateCurveView::mouseUp (const juce::MouseEvent&)
{
    if (auto* p = getParameter (dragging))
        p->endChangeGesture();

    dragging = Dot::none;
}

void GateCurveView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const gate::Settings defaults;

    switch (getDotAt (e.position))
    {
        case Dot::threshold: processor.setKnob (Knob::threshold, defaults.thresholdDb); break;
        case Dot::closing:   processor.setKnob (Knob::hysteresis, defaults.hysteresisDb); break;
        case Dot::range:     processor.setKnob (Knob::range, defaults.rangeDb); break;
        case Dot::none:      break;
    }
}

void GateCurveView::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const float delta = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
    float ratio = processor.getSettings().ratio * std::exp (delta * 0.8f);

    if (ratio > gate::maxRatio - 0.5f && delta > 0.0f)
        ratio = gate::maxRatio;

    processor.setKnob (Knob::ratio, juce::jlimit (gate::minRatio, gate::maxRatio, ratio));
    repaint();
}

juce::String GateCurveView::getTooltip()
{
    const auto s = processor.getSettings();

    switch (getDotAt (getMouseXYRelative().toFloat()))
    {
        case Dot::threshold: return "Threshold " + gate::formatKnob (Knob::threshold, s) + ": drag sideways";
        case Dot::closing:   return "Closes at " + gatetext::formatLevel (s.thresholdDb - s.hysteresisDb) + ": drag to set the hysteresis";
        case Dot::range:     return "Range " + gate::formatKnob (Knob::range, s) + ": drag up or down";
        case Dot::none:      break;
    }

    return "The gate's curve: level in across, out up. Scroll to change the ratio, right-click for more.";
}

void GateCurveView::showMenu()
{
    const auto analysis = gate::analyse (moments, processor.getSettings(), processor.matchGainDb.load());
    juce::PopupMenu menu;

    if (analysis.suggestedDb)
        menu.addItem (1, "Set the threshold from the track (" + gatetext::formatLevel (*analysis.suggestedDb) + ")");
    else
        menu.addItem (1, "Set the threshold from the track (play the track first)", false);

    menu.addItem (2, "Reset all");
    menu.addSeparator();
    guideicon::addItem (menu, 3, "What is a gate?");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, suggested = analysis.suggestedDb, safe = juce::Component::SafePointer<GateCurveView> (this)] (int result)
    {
        if (safe == nullptr)
            return;

        if (result == 1 && suggested)
            processor.setKnob (Knob::threshold, *suggested);
        else if (result == 2)
            processor.setSettings ({});
        else if (result == 3 && onOpenGuide != nullptr)
            onOpenGuide (thresholdPage);
    });
}

//==============================================================================
GateHistoryView::GateHistoryView (BuiltInGate& p, const std::vector<gate::Moment>& recent, std::function<double()> getBpm)
    : processor (p), moments (recent), bpm (std::move (getBpm))
{
    setTopic (*this, CommonTopic::history);
}

juce::Rectangle<float> GateHistoryView::getPlotBounds() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromTop (18.0f);
    r.removeFromBottom (14.0f);
    return r;
}

void GateHistoryView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto bounds = getLocalBounds().toFloat();
    auto legend = bounds.removeFromTop (18.0f).reduced (8.0f, 0.0f);
    auto times = bounds.removeFromBottom (14.0f);

    g.setFont (juce::FontOptions (10.5f));
    auto key = [&] (juce::Colour c, const juce::String& text)
    {
        g.setColour (c);
        g.fillRect (legend.removeFromLeft (9.0f).withSizeKeepingCentre (9.0f, 9.0f));
        legend.removeFromLeft (4.0f);
        g.setColour (theme::textDim);
        const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 12.0f;
        g.drawText (text, legend.removeFromLeft (w), juce::Justification::centredLeft);
    };

    key (theme::textDim.withAlpha (0.6f), "the track");
    key (gatetext::colour, "what comes out");
    key (gatetext::removedColour, "turned down");
    key (gatetext::closingColour, "closing level");

    g.setColour (theme::textDim);
    g.drawText ("4 s ago", times.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
    g.drawText ("now", times.withTrimmedRight (8.0f), juce::Justification::centredRight);

    gatetext::drawMoments (g, bounds, moments, processor.getSettings(), true, bpm != nullptr ? bpm() : 0.0);
}

juce::String GateHistoryView::getTooltip()
{
    const auto area = getPlotBounds().withTrimmedLeft (30.0f).withTrimmedRight (8.0f).reduced (0.0f, 3.0f);
    const float x = (float) getMouseXYRelative().x;

    if (moments.empty() || x < area.getX() || x > area.getRight())
        return "The last 4 seconds: grey is the track, colour what comes out, red how much was turned down, and the bar along the bottom when the gate was open.";

    const double seconds = (double) (x - area.getRight()) / area.getWidth() * gate::recentSeconds;
    const int index = (int) moments.size() - 1 + juce::roundToInt (seconds / (gate::momentMs / 1000.0));

    if (! juce::isPositiveAndBelow (index, (int) moments.size()))
        return {};

    const auto& m = moments[(size_t) index];
    return juce::String (seconds, 1) + " s · in " + gatetext::formatLevel (m.inputDb) + " · " + (m.open ? "open" : "closed") + ", "
           + (m.reductionDb < 0.5f ? juce::String ("not turned down") : "-" + gatetext::formatLevel (m.reductionDb));
}

//==============================================================================
KeyFilterView::KeyFilterView (BuiltInGate& p, std::function<void (int)> openGuide) : processor (p), onOpenGuide (std::move (openGuide))
{
    setTopic (*this, gatetext::key);
}

juce::Rectangle<float> KeyFilterView::getPlot() const
{
    return gatetext::getKeyPlot (getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (18.0f), true);
}

void KeyFilterView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("WHAT THE GATE LISTENS TO", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    if (processor.listenToKey.load())
    {
        g.setColour (gatetext::colour);
        g.drawText ("KEY LISTEN IS ON", r.withTrimmedTop (0.0f).removeFromTop (16.0f), juce::Justification::centredRight);
    }

    gatetext::drawKeyFilters (g, r.withTrimmedTop (2.0f), processor.getSettings(), true, true);
}

KeyFilterView::Handle KeyFilterView::getHandleAt (juce::Point<float> p) const
{
    const auto s = processor.getSettings();
    const auto plot = getPlot();
    auto handleAt = [&] (double hz, bool on) { return juce::Point<float> (gatetext::keyHzToPosition (plot, hz), juce::jmap (on ? -3.0f : 0.0f, -48.0f, 6.0f, plot.getBottom(), plot.getY())); };
    const auto low = handleAt (s.keyLowHz, gate::isKeyLowOn (s.keyLowHz)), high = handleAt (s.keyHighHz, gate::isKeyHighOn (s.keyHighHz));

    if (isNearTo (p, low, 10.0f) && (! isNearTo (p, high, 10.0f) || p.getDistanceFrom (low) <= p.getDistanceFrom (high)))
        return Handle::low;

    return isNearTo (p, high, 10.0f) ? Handle::high : Handle::none;
}

void KeyFilterView::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (getHandleAt (e.position) != Handle::none ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void KeyFilterView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    dragging = getHandleAt (e.position);

    if (dragging != Handle::none)
        processor.getKnobParameter (dragging == Handle::low ? Knob::keyLow : Knob::keyHigh)->beginChangeGesture();
}

void KeyFilterView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Handle::none)
        return;

    const double hz = gatetext::keyPositionToHz (getPlot(), e.position.x);

    if (dragging == Handle::low)
        processor.setKnob (Knob::keyLow, (float) juce::jlimit ((double) gate::minKeyLow, (double) gate::maxKeyLow, hz));
    else
        processor.setKnob (Knob::keyHigh, (float) juce::jlimit ((double) gate::minKeyHigh, (double) gate::maxKeyHigh, hz));

    repaint();
}

void KeyFilterView::mouseUp (const juce::MouseEvent&)
{
    if (dragging != Handle::none)
        processor.getKnobParameter (dragging == Handle::low ? Knob::keyLow : Knob::keyHigh)->endChangeGesture();

    dragging = Handle::none;
}

void KeyFilterView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto handle = getHandleAt (e.position);

    if (handle == Handle::low)
        processor.setKnob (Knob::keyLow, gate::minKeyLow);
    else if (handle == Handle::high)
        processor.setKnob (Knob::keyHigh, gate::maxKeyHigh);
}

juce::String KeyFilterView::getTooltip()
{
    const auto s = processor.getSettings();

    switch (getHandleAt (getMouseXYRelative().toFloat()))
    {
        case Handle::low:  return "Key low cut " + gate::formatKnob (Knob::keyLow, s) + ": drag sideways, double-click to turn it off";
        case Handle::high: return "Key high cut " + gate::formatKnob (Knob::keyHigh, s) + ": drag sideways, double-click to turn it off";
        case Handle::none: break;
    }

    return "What the gate hears of each frequency. Drag the two handles; double-click one to turn it off.";
}

void KeyFilterView::showMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Key listen", true, processor.listenToKey.load());
    menu.addItem (2, "Turn the key filters off");
    menu.addSeparator();
    guideicon::addItem (menu, 3, "What is a gate?");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, safe = juce::Component::SafePointer<KeyFilterView> (this)] (int result)
    {
        if (safe == nullptr)
            return;

        if (result == 1)
        {
            processor.listenToKey = ! processor.listenToKey.load();
            repaint();
        }
        else if (result == 2)
        {
            processor.setKnob (Knob::keyLow, gate::minKeyLow);
            processor.setKnob (Knob::keyHigh, gate::maxKeyHigh);
        }
        else if (result == 3 && onOpenGuide != nullptr)
        {
            onOpenGuide (keyPage);
        }
    });
}

//==============================================================================
BuiltInGateEditor::BuiltInGateEditor (BuiltInGate& p)
    : EffectEditor (p, "DawIt Gate", "What is a gate?", "Hear what it removes", p.listenToRemoved, p.clap, gatetext::colour, fx::Clap::Sound::hitsInHiss),
      gateProcessor (p),
      curve (p, recent, [this] (int page) { openGuide (page); }),
      history (p, recent, [&p] { return p.tempoBpm.load(); }),
      keyView (p, [this] (int page) { openGuide (page); })
{
    addAndMakeVisible (curve);
    addAndMakeVisible (history);
    addAndMakeVisible (keyView);

    const std::pair<Knob, const char*> names[] = { { Knob::threshold, "Threshold" }, { Knob::hysteresis, "Hysteresis" }, { Knob::range, "Range" },
                                                   { Knob::ratio, "Ratio" }, { Knob::attack, "Attack" }, { Knob::hold, "Hold" },
                                                   { Knob::release, "Release" }, { Knob::keyLow, "Low cut" }, { Knob::keyHigh, "High cut" } };

    for (auto& [knob, name] : names)
        knobs.add (new EffectKnob (gateProcessor.getParameters(), BuiltInGate::getParameterId (knob), name, (int) knob, gatetext::colour));

    keyListenSwitch.button.onClick = [this] { gateProcessor.listenToKey = ! gateProcessor.listenToKey.load(); };
    matchSwitch.button.onClick = [this] { gateProcessor.setMatchingLevel (! gateProcessor.isMatchingLevel()); };

    for (auto* button : { &keyListenSwitch.button, &matchSwitch.button })
        button->setColour (juce::TextButton::buttonOnColourId, gatetext::colour.darker (0.4f));

    setGroups ({ { "WHEN", { knobs[0], knobs[1] }, 0 }, { "HOW FAR", { knobs[2], knobs[3] }, 0 }, { "HOW FAST", { knobs[4], knobs[5], knobs[6] }, 0 },
                 { "KEY: WHAT IT LISTENS TO", { knobs[7], knobs[8], &keyListenSwitch }, 1 }, { "LEVEL", { &matchSwitch }, 1 } });

    juce::StringArray presetNames;

    for (auto& preset : gate::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

BuiltInGateEditor::~BuiltInGateEditor()
{
    // Listening to the key is for while the window is open.
    gateProcessor.listenToKey = false;
}

void BuiltInGateEditor::applyPreset (int index)
{
    const auto& list = gate::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    const auto analysis = gate::analyse (gateProcessor.getRecent(), gateProcessor.getSettings(), gateProcessor.matchGainDb.load());
    gateProcessor.applyPreset (list[(size_t) index], analysis.suggestedDb);
    presetThreshold = list[(size_t) index].followTrack ? analysis.suggestedDb : std::nullopt;
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInGateEditor::createGuidePages()
{
    return gatetext::makeGuide (gateProcessor);
}

void BuiltInGateEditor::layoutPictures (juce::Rectangle<int> r)
{
    r.reduce (10, 10);
    curve.setBounds (r.removeFromLeft (juce::jmin (430, r.getHeight())));
    r.removeFromLeft (10);
    history.setBounds (r.removeFromTop (r.getHeight() * 11 / 20));
    r.removeFromTop (10);
    keyView.setBounds (r);
}

void BuiltInGateEditor::tick()
{
    const bool listening = gateProcessor.listenToKey.load();
    keyListenSwitch.button.setButtonText (listening ? "On" : "Off");
    keyListenSwitch.button.setToggleState (listening, juce::dontSendNotification);
    matchSwitch.button.setButtonText (gateProcessor.isMatchingLevel() ? "On" : "Off");
    matchSwitch.button.setToggleState (gateProcessor.isMatchingLevel(), juce::dontSendNotification);

    recent = gateProcessor.getRecent();
    curve.repaint();
    history.repaint();
    keyView.repaint();
}

juce::AttributedString BuiltInGateEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = gateProcessor.getSettings();
    const double bpm = gateProcessor.tempoBpm.load();
    const auto moments = gateProcessor.getRecent();
    const auto& list = gate::getPresets();
    const gate::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, gatetext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear, with match level on.");

        if (preset->followTrack)
            addSection (text, {}, presetThreshold ? "Its threshold was set from your track: " + gatetext::formatLevel (*presetThreshold) + "."
                                                  : juce::String ("Its threshold is a guess. Play the track, then right-click the curve to set it from the track."));

        for (int k = 0; k < gate::numKnobs; ++k)
            addSection (text, gatetext::getTitle (k).upToFirstOccurrenceOf (":", false, false) + ": " + gate::formatKnob ((Knob) k, preset->settings),
                        preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < gate::numKnobs)
    {
        addTitle (text, gatetext::getTitle (topic), gatetext::colour);
        addLead (text, gatetext::describeSettings (topic, s, moments, bpm));
        addSection (text, "What it does", gatetext::describe (topic));
        addSection (text, "When and how it's set", gatetext::getTypical (topic));

        if (preset != nullptr)
            addSection (text, "In the " + preset->name + " starting point: " + gate::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);
        else
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that sound.");

        return text;
    }

    addTitle (text, gatetext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "DawIt Gate turns a sound down while it's quiet: it opens for the notes and closes in the gaps, to remove hiss, "
                           "hum and the bleed of other instruments, or to make drums tighter.");
            addSection (text, "When to use it", "On drum microphones (toms, kick, snare), distorted guitars, and voices with noise between "
                                                "phrases; and for gated reverb. Not on software instruments, which are already silent between notes.");
            addSection (text, "How it works", "The key filters decide what it listens to; threshold and hysteresis when it opens and closes; "
                                              "range and ratio how far; attack, hold and release how fast.");
            addSection (text, "How it's usually set", "Attack as fast as the sound allows, hold 20-50 ms, release by ear to the end of the "
                                                      "notes, threshold above the noise. First in the chain, before compression.");
            addSection (text, "Reading the window", "Left, the curve with the track's dot and the open light. Right, the last 4 seconds and "
                                                    "the key filters. Hover anything to read about it here.");
            addSection (text, "Getting started", "Pick a starting point; each tells why every knob is set the way it is. Click Hits and hiss "
                                                 "to hear the gate at work, even while the song is stopped. Read What is a gate? for how it "
                                                 "works and when it's used.");
            break;

        case CommonTopic::listen:
            addLead (text, gateProcessor.listenToRemoved.load() ? "On: you hear only what the gate takes away." : "Off: you hear the gated track.");
            addSection (text, {}, gatetext::describe (topic));
            break;

        case gatetext::keyListen:
            addLead (text, gateProcessor.listenToKey.load() ? "On: you hear what the gate listens to, not the track." : "Off: you hear the track.");
            addSection (text, {}, gatetext::describe (topic));
            break;

        case gatetext::match:
            addLead (text, gateProcessor.isMatchingLevel() ? "On: it turns the sound by " + juce::String (gateProcessor.matchGainDb.load(), 1) + " dB right now."
                                                           : "Off. Turned on, it would turn the sound by " + juce::String (gateProcessor.matchGainDb.load(), 1) + " dB right now.");
            addSection (text, {}, gatetext::describe (topic));
            break;

        case CommonTopic::history:
            addLead (text, gatetext::describeSettings (topic, s, moments, bpm));
            addSection (text, {}, gatetext::describe (topic));
            break;

        case gatetext::curve:
        {
            const auto analysis = gate::analyse (moments, s, gateProcessor.matchGainDb.load());
            addLead (text, "Opens at " + gatetext::formatLevel (s.thresholdDb) + ", closes at " + gatetext::formatLevel (s.thresholdDb - s.hysteresisDb)
                               + ", closed it turns the sound down " + (gate::isGate (s.ratio) ? juce::String ("by ") : "by up to ") + gatetext::formatLevel (s.rangeDb) + ".");
            addSection (text, {}, gatetext::describe (topic));

            if (analysis.heard >= 500 && ! analysis.advice.isEmpty())
                addSection (text, "Your track", analysis.advice.joinIntoString (" "));

            break;
        }

        default:
        {
            const auto lead = gatetext::describeSettings (topic, s, moments, bpm);

            if (lead.isNotEmpty())
                addLead (text, lead);

            addSection (text, {}, gatetext::describe (topic));
            break;
        }
    }

    return text;
}
} // namespace daw
