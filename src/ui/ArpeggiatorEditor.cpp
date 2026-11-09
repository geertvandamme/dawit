#include "ArpeggiatorEditor.h"

namespace daw
{
using arp::Knob;
using arp::StepKind;
using namespace fxui;

namespace
{
    // The window has no "hear only the effect" and no clap of its own: it has Test chord. The frame wants both.
    std::atomic<bool> unusedListen { false };
    fx::Clap unusedClap;
}

//==============================================================================
ArpeggiatorStepView::ArpeggiatorStepView (BuiltInArpeggiator& p, StepKind& b) : processor (p), brush (b)
{
    setTopic (*this, arptext::stepRow);
    setWantsKeyboardFocus (true);
}

arptext::StepGrid ArpeggiatorStepView::getGrid() const
{
    return arptext::layoutSteps (getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (18.0f));
}

int ArpeggiatorStepView::getStepAt (juce::Point<int> p) const
{
    return getGrid().getStepAt (p.toFloat());
}

void ArpeggiatorStepView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE PATTERN", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    arptext::drawSteps (g, getGrid(), processor.getSettings(), processor.playingStep.load(), hovered, true);
}

void ArpeggiatorStepView::mouseMove (const juce::MouseEvent& e)
{
    const int step = getStepAt (e.getPosition());

    if (step != hovered)
    {
        hovered = step;
        repaint();
    }
}

void ArpeggiatorStepView::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void ArpeggiatorStepView::paintStep (int step, StepKind kind)
{
    if (juce::isPositiveAndBelow (step, processor.getSettings().steps) && processor.getKind (step) != kind)
        processor.setKind (step, kind);
}

void ArpeggiatorStepView::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    const int step = getStepAt (e.getPosition());

    if (e.mods.isPopupMenu())
    {
        showMenu (step);
        return;
    }

    if (! juce::isPositiveAndBelow (step, processor.getSettings().steps))
        return;

    if (e.mods.isShiftDown())
    {
        processor.setAccented (step, ! processor.isAccented (step));
        return;
    }

    // Clicking a step that already has the brush's kind makes it Note; dragging paints the same thing across.
    paintKind = processor.getKind (step) == brush ? StepKind::note : brush;
    paintStep (step, paintKind);
    lastPainted = step;
    painting = true;
}

void ArpeggiatorStepView::mouseDrag (const juce::MouseEvent& e)
{
    if (! painting)
        return;

    const int step = getStepAt (e.getPosition());

    if (step < 0 || step == lastPainted)
        return;

    // Fill in the steps a fast drag jumped over.
    for (int i = lastPainted; i != step;)
    {
        i += step > i ? 1 : -1;
        paintStep (i, paintKind);
    }

    lastPainted = step;
    hovered = step;
}

void ArpeggiatorStepView::mouseUp (const juce::MouseEvent&)
{
    painting = false;
}

void ArpeggiatorStepView::mouseDoubleClick (const juce::MouseEvent& e)
{
    paintStep (getStepAt (e.getPosition()), StepKind::note);
}

bool ArpeggiatorStepView::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        paintStep (hovered, StepKind::note);
        return hovered >= 0;
    }

    if (key.getModifiers().isCommandDown())
    {
        if (key.getKeyCode() == 'C')
        {
            copyPattern();
            return true;
        }

        if (key.getKeyCode() == 'V')
        {
            pastePattern();
            return true;
        }

        return false;
    }

    const int digit = (int) key.getTextCharacter() - '1';

    if (juce::isPositiveAndBelow (digit, arp::numStepKinds))
    {
        brush = (StepKind) digit;

        if (onBrushChanged)
            onBrushChanged();

        return true;
    }

    return false;
}

void ArpeggiatorStepView::copyPattern() const
{
    juce::SystemClipboard::copyTextToClipboard (arp::patternToText (processor.getSettings()));
}

void ArpeggiatorStepView::pastePattern()
{
    auto s = processor.getSettings();

    if (arp::patternFromText (juce::SystemClipboard::getTextFromClipboard(), s))
        processor.setSettings (s);
}

void ArpeggiatorStepView::showMenu (int step)
{
    const auto s = processor.getSettings();
    const bool onStep = juce::isPositiveAndBelow (step, s.steps);
    auto pasted = s;
    const bool canPaste = arp::patternFromText (juce::SystemClipboard::getTextFromClipboard(), pasted);
    juce::PopupMenu menu;

    for (int k = 0; k < arp::numStepKinds; ++k)
        menu.addItem (1 + k, juce::String (arp::getKindName ((StepKind) k)), onStep, onStep && s.kinds[(size_t) step] == (StepKind) k);

    menu.addItem (6, "Accent", onStep, onStep && s.accents[(size_t) step]);
    menu.addSeparator();
    menu.addItem (7, "Copy pattern");
    menu.addItem (8, "Paste pattern", canPaste);
    menu.addItem (9, "Clear: every step a plain Note");
    menu.addItem (10, "Randomise pattern");
    menu.addSeparator();
    guideicon::addItem (menu, "What is an arpeggiator?...", [safe = juce::Component::SafePointer<ArpeggiatorStepView> (this)]
    {
        if (safe != nullptr && safe->onOpenGuide)
            safe->onOpenGuide();
    });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<ArpeggiatorStepView> (this), step] (int choice)
    {
        if (safe == nullptr || choice == 0)
            return;

        auto& p = safe->processor;

        if (choice >= 1 && choice <= arp::numStepKinds)
            safe->paintStep (step, (StepKind) (choice - 1));
        else if (choice == 6 && juce::isPositiveAndBelow (step, arp::maxSteps))
            p.setAccented (step, ! p.isAccented (step));
        else if (choice == 7)
            safe->copyPattern();
        else if (choice == 8)
            safe->pastePattern();
        else if (choice == 10)
        {
            if (safe->onRandomise)
                safe->onRandomise();
        }
        else if (choice == 9)
            for (int i = 0; i < arp::maxSteps; ++i)
            {
                p.setKind (i, StepKind::note);
                p.setAccented (i, false);
            }
    });
}

juce::String ArpeggiatorStepView::getTooltip()
{
    return arptext::getStepDescription (processor.getSettings(), getStepAt (getMouseXYRelative()), brush);
}

//==============================================================================
ArpeggiatorKeyboardView::ArpeggiatorKeyboardView (BuiltInArpeggiator& p) : processor (p)
{
    setTopic (*this, arptext::keyboard);
    setTooltip (arptext::describe (arptext::keyboard));
}

void ArpeggiatorKeyboardView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    auto header = r.removeFromTop (16.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE KEYS YOU HOLD", header, juce::Justification::centredLeft);

    const auto state = arptext::getKeyboardState (processor);
    const auto name = arptext::getHeldChordName (state);

    g.setColour (name.isEmpty() ? theme::textDim : arptext::colour);
    g.setFont (juce::FontOptions (name.isEmpty() ? 11.5f : 15.0f, juce::Font::bold));
    g.drawText (name.isEmpty() ? "Hold a chord, or press Test chord" : name, header.withTrimmedLeft (140.0f), juce::Justification::centredRight);

    r.removeFromTop (4.0f);
    arptext::drawKeyboard (g, r, state);
}

//==============================================================================
ArpeggiatorNotesView::ArpeggiatorNotesView (BuiltInArpeggiator& p) : processor (p)
{
    setTopic (*this, arptext::notesView);
    setTooltip (arptext::describe (arptext::notesView));
}

void ArpeggiatorNotesView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE LAST 2 BARS", r.removeFromTop (16.0f), juce::Justification::centredLeft);
    arptext::drawLastBars (g, r, arptext::getLastBars (processor), processor.getSettings(), processor.clockBeat.load());
}

//==============================================================================
BuiltInArpeggiatorEditor::BuiltInArpeggiatorEditor (BuiltInArpeggiator& p)
    : EffectEditor (p, "DawIt Arpeggiator", "What is an arpeggiator?", {}, unusedListen, unusedClap, arptext::colour, fx::Clap::Sound::note),
      arpeggiator (p)
{
    addAndMakeVisible (keyboardView);
    addAndMakeVisible (stepView);
    addAndMakeVisible (notesView);
    stepView.onBrushChanged = [this] { setBrush (brush); };
    stepView.onOpenGuide = [this] { openGuide(); };

    for (int k = 0; k < arp::numStepKinds; ++k)
    {
        auto& b = brushButtons[k];
        b.setButtonText (arp::getKindName ((StepKind) k));
        b.setClickingTogglesState (false);
        b.setColour (juce::TextButton::buttonOnColourId, arptext::getKindColour ((StepKind) k).brighter (k == (int) StepKind::rest ? 0.3f : 0.0f));
        b.setWantsKeyboardFocus (false);
        b.setTooltip (juce::String (arp::getKindName ((StepKind) k)) + " (key " + juce::String (k + 1) + "): " + arp::getKindHint ((StepKind) k) + ".");
        b.onClick = [this, k] { setBrush ((StepKind) k); };
        setTopic (b, arptext::kindButton + k);
        addAndMakeVisible (b);
    }

    // The frame's clap becomes Test chord; "hear only the effect" has no meaning for a MIDI effect.
    clapButton.setButtonText ("Test chord");
    clapButton.setTooltip (arptext::describe (CommonTopic::clap));
    clapButton.onClick = [this]
    {
        arpeggiator.playTestChord();
        showTopic (CommonTopic::clap);
    };
    listenButton.setVisible (false);

    const std::tuple<Knob, const char*> names[] = {
        { Knob::octaves, "Octaves" }, { Knob::rate, "Rate" }, { Knob::gate, "Gate" },
        { Knob::swing, "Swing" }, { Knob::steps, "Steps" }, { Knob::accent, "Accent" } };

    for (auto& [knob, name] : names)
    {
        knobs.add (new EffectKnob (arpeggiator.getParameters(), BuiltInArpeggiator::getParameterId (knob), name, (int) knob, arptext::colour));
        knobs.getLast()->slider.setTooltip (arptext::describe ((int) knob));
    }

    // Values with their unit.
    auto& gate = knobs[2]->slider;
    gate.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + "%"; };
    gate.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    auto& swing = knobs[3]->slider;
    swing.textFromValueFunction = [] (double v) { return juce::String (v, v - std::floor (v) < 0.005 ? 0 : 1) + "%"; };
    swing.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    auto& accent = knobs[5]->slider;
    accent.textFromValueFunction = [] (double v) { return "+" + juce::String (juce::roundToInt (v)); };
    accent.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    gate.updateText();
    swing.updateText();
    accent.updateText();

    orderSwitch.button.setTooltip (arptext::describe ((int) Knob::order));
    orderSwitch.button.onClick = [this] { showOrderMenu(); };
    latchSwitch.button.setTooltip (arptext::describe ((int) Knob::latch));
    latchSwitch.button.onClick = [this] { arpeggiator.setKnob (Knob::latch, arpeggiator.getSettings().latch ? 0.0f : 1.0f); };

    setGroups ({ { "NOTES", { &orderSwitch, knobs[0] } },
                 { "RHYTHM", { knobs[1], knobs[2], knobs[3] } },
                 { "PATTERN", { knobs[4], knobs[5], &latchSwitch } } });

    juce::StringArray presetNames;
    std::vector<std::pair<int, juce::String>> headings;

    for (auto& preset : arp::getPresets())
    {
        if (headings.empty() || headings.back().second != preset.group)
            headings.emplace_back (presetNames.size(), preset.group);

        presetNames.add (preset.name);
    }

    setPresetNames (presetNames, headings);

    randomiseButton.setTooltip ("Paint a new random pattern of notes, chords, bass notes, rests and ties. Rate, octaves and everything else stay as they are. Press again for another.");
    randomiseButton.setWantsKeyboardFocus (false);
    randomiseButton.onClick = [this] { randomise(); };
    undoButton.setTooltip ("Go back to the pattern before the last Randomise.");
    undoButton.setWantsKeyboardFocus (false);
    undoButton.setEnabled (false);
    undoButton.onClick = [this] { undoRandomise(); };
    addAndMakeVisible (randomiseButton);
    addAndMakeVisible (undoButton);
    stepView.onRandomise = [this] { randomise(); };
    setBrush (brush);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInArpeggiatorEditor::randomise()
{
    beforeRandomise = arpeggiator.getSettings();
    arpeggiator.setSettings (arp::randomisePattern (*beforeRandomise, juce::Time::currentTimeMillis() + 7919 * ++randomiseCount));
    undoButton.setEnabled (true);
}

void BuiltInArpeggiatorEditor::undoRandomise()
{
    if (! beforeRandomise.has_value())
        return;

    arpeggiator.setSettings (*beforeRandomise);
    beforeRandomise.reset();
    undoButton.setEnabled (false);
}

void BuiltInArpeggiatorEditor::setBrush (StepKind kind)
{
    brush = kind;

    for (int k = 0; k < arp::numStepKinds; ++k)
        brushButtons[k].setToggleState (k == (int) kind, juce::dontSendNotification);
}

void BuiltInArpeggiatorEditor::showOrderMenu()
{
    const auto current = arpeggiator.getSettings().order;
    juce::PopupMenu menu;

    for (int o = 0; o < arp::numOrders; ++o)
        menu.addItem (1 + o, juce::String (arp::getOrderName ((arp::Order) o)) + "  -  " + arp::getOrderHint ((arp::Order) o), true, (arp::Order) o == current);

    menu.addSeparator();
    guideicon::addItem (menu, 100, "What is an arpeggiator?...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&orderSwitch.button),
                        [safe = juce::Component::SafePointer<BuiltInArpeggiatorEditor> (this)] (int choice)
                        {
                            if (safe == nullptr || choice == 0)
                                return;

                            if (choice == 100)
                                safe->openGuide();
                            else
                                safe->arpeggiator.setKnob (Knob::order, (float) (choice - 1));
                        });
}

void BuiltInArpeggiatorEditor::applyPreset (int index)
{
    const auto& list = arp::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    arpeggiator.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInArpeggiatorEditor::createGuidePages()
{
    return arptext::makeGuide (arpeggiator);
}

void BuiltInArpeggiatorEditor::layoutPictures (juce::Rectangle<int> r)
{
    keyboardView.setBounds (r.removeFromTop (132));
    r.removeFromTop (8);
    const auto patternArea = r.removeFromTop (128);
    stepView.setBounds (patternArea);
    r.removeFromTop (8);
    notesView.setBounds (r);

    auto row = patternArea.reduced (10, 6).removeFromTop (22);
    auto buttons = row.removeFromRight (5 * 76);
    row.removeFromLeft (92);   // the picture's own title
    randomiseButton.setBounds (row.removeFromLeft (96).reduced (2, 0));
    undoButton.setBounds (row.removeFromLeft (60).reduced (2, 0));

    for (auto& b : brushButtons)
        b.setBounds (buttons.removeFromLeft (76).reduced (2, 0));
}

void BuiltInArpeggiatorEditor::tick()
{
    const auto s = arpeggiator.getSettings();
    orderSwitch.button.setButtonText (arp::getOrderName (s.order));
    orderSwitch.button.setColour (juce::TextButton::buttonColourId, arptext::colour.darker (0.5f));
    latchSwitch.button.setButtonText (s.latch ? "On" : "Off");
    latchSwitch.button.setToggleState (s.latch, juce::dontSendNotification);
    latchSwitch.button.setColour (juce::TextButton::buttonOnColourId, arptext::colour.darker (0.4f));

    keyboardView.repaint();
    stepView.repaint();
    notesView.repaint();
}

juce::AttributedString BuiltInArpeggiatorEditor::buildText (int topicIndex)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = arpeggiator.getSettings();
    const double used = arpeggiator.getUsedBpm();
    const double bpm = used > 0.0 ? used : arpeggiator.tempoBpm.load() > 0.0 ? arpeggiator.tempoBpm.load() : 120.0;
    const auto& list = arp::getPresets();
    const arp::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topicIndex == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, arptext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: hold a chord, play the song and adjust by ear. The pattern is the main thing: click steps to change it.");
        addSection (text, "Settings", arp::getOrderName (preset->settings.order) + juce::String (", ") + juce::String (preset->settings.octaves)
                                          + (preset->settings.octaves == 1 ? " octave, " : " octaves, ") + arptext::describeSettings ((int) Knob::rate, preset->settings, bpm).upToFirstOccurrenceOf (":", false, false)
                                          + ", gate " + juce::String (juce::roundToInt (preset->settings.gatePercent)) + "%, "
                                          + juce::String (preset->settings.steps) + " steps.");
        return text;
    }

    if (topicIndex >= 0 && topicIndex < arp::numKnobs)
    {
        addTitle (text, arptext::getTitle (topicIndex), arptext::colour);
        addLead (text, arptext::describeSettings (topicIndex, s, bpm));
        addSection (text, "What it does", arptext::describe (topicIndex));
        addSection (text, "When and how it's set", arptext::getTypical (topicIndex));

        if (preset != nullptr && topicIndex != (int) Knob::latch)
            addSection (text, "In the " + preset->name + " starting point", preset->why);
        else if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that use.");

        return text;
    }

    addTitle (text, arptext::getTitle (topicIndex));

    switch (topicIndex)
    {
        case CommonTopic::overview:
            addLead (text, "DawIt Arpeggiator turns the chord you hold into a pattern: single notes in an order, over one or more octaves, plus whole-chord, "
                           "bass, rest and tie steps, in time with the song.");
            addSection (text, "When to use it", "For a busy broken-chord part from a few held keys: the climbing sixteenths of trance, an Alberti bass, "
                                                "stabs on the off-beats. Hold a chord on your keyboard or the computer keyboard, or put a MIDI clip of block "
                                                "chords on the track, and play the song. To edit single notes or print the part, right-click the clip and "
                                                "choose Create MIDI clip from arpeggiator.");
            addSection (text, "How it works", "It's a MIDI effect: it changes notes, not sound, and sits before the instrument, so what you hear depends on the "
                                              "instrument after it. It counts its steps from the start of the song, so it stays in time when you play from the "
                                              "middle or loop a section. It ends every note it starts when you let go, stop or jump.");
            addSection (text, "Reading the window", "Top: the keys you hold, named by their place in the chord, with the note playing lit. Middle: the pattern. "
                                                    "Pick a brush (Note, Chord, Bass, Rest, Tie), then click or drag across steps; Shift-click accents a step; the lit "
                                                    "one is playing. Under it: the last 2 bars of notes it played. Hover anything to read about it here.");
            addSection (text, "Getting started", "Pick a starting point, then press Test chord to hear the pattern on the chord of the bar, even while the song "
                                                 "is stopped. The Starting point menu has 51, in five groups. Randomise paints a new random pattern and keeps everything else; Undo brings back the pattern from before. Read What is an arpeggiator? for how the order, octaves, note length and accents work.");
            break;

        case CommonTopic::clap:
            addLead (text, "Plays the chord track's chord of this bar through the arpeggiator for 2 bars, from step 1.");
            addSection (text, {}, arptext::describe (topicIndex));
            addSection (text, "Without a chord", "Where the chord track has no chord, it holds C4 E4 G4: C major.");
            break;

        case arptext::keyboard:
        {
            const auto name = arptext::getHeldChordName (arptext::getKeyboardState (arpeggiator));
            addLead (text, name.isEmpty() ? juce::String ("Nothing is held at the moment.") : "You hold " + name + ".");
            addSection (text, {}, arptext::describe (topicIndex));
            addSection (text, "Reading it", "R is the root that names the chord, 3 the third (3 or 4 semitones above the root), 5 the fifth (7 semitones) and 7 "
                                            "the seventh (10 or 11). Every note it plays is one of these, so the arpeggio fits wherever its chord fits.");
            break;
        }

        case arptext::stepRow:
            addLead (text, juce::String (s.steps) + " steps of " + arptext::describeSettings ((int) Knob::rate, s, bpm).upToFirstOccurrenceOf (":", false, false) + ". Brush: " + arp::getKindName (brush) + ".");
            addSection (text, {}, arptext::describe (topicIndex));
            addSection (text, "The steps", "Note: the next note of the order. Chord: the whole chord at once. Bass: the lowest note, an octave lower. Rest: nothing. "
                                           "Tie: the step before goes on sounding.");
            addSection (text, "Shortcuts", "Keys 1 to 5 choose the brush. Shift-click accents a step. Delete over a step makes it Note. Cmd+C copies the pattern as a "
                                           "line of text and Cmd+V pastes one. Right-click a step for the kinds and more.");
            break;

        case arptext::notesView:
            addLead (text, "The notes it played over the last 2 bars, one block per note, the song's beats behind them.");
            addSection (text, {}, arptext::describe (topicIndex));
            addSection (text, "Reading it", "Time runs left to right, with a stronger line at each bar. Pitch runs up the picture. A note with a yellow outline is not "
                                            "in the chord track's chord of its bar.");
            break;

        default:
        {
            if (topicIndex >= arptext::kindButton && topicIndex < arptext::kindButton + arp::numStepKinds)
            {
                const auto kind = (StepKind) (topicIndex - arptext::kindButton);
                addLead (text, juce::String (arp::getKindName (kind)) + ": " + arp::getKindHint (kind) + ".");
                addSection (text, {}, "Pick this brush, then click or drag across steps to give them this kind. Clicking a step that already has it makes it Note.");
                break;
            }

            addSection (text, {}, arptext::describe (topicIndex));
            break;
        }
    }

    return text;
}
} // namespace daw
