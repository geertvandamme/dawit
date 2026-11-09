#include "SlicerEditor.h"

namespace daw
{
using slc::Knob;
using slc::Mode;
using namespace fxui;

//==============================================================================
SlicerPatternView::SlicerPatternView (BuiltInSlicer& p, Mode& b) : processor (p), brush (b)
{
    setTopic (*this, slctext::pattern);
    setWantsKeyboardFocus (true);
}

slctext::PatternGrid SlicerPatternView::getGrid() const
{
    return slctext::layoutPattern (getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (18.0f));
}

int SlicerPatternView::getStepAt (juce::Point<int> p) const
{
    return getGrid().getStepAt (p.toFloat());
}

void SlicerPatternView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE PATTERN", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    const auto s = processor.getSettings();
    slctext::drawPattern (g, getGrid(), s, processor.playingStep.load(), hovered, true);
}

void SlicerPatternView::mouseMove (const juce::MouseEvent& e)
{
    const int step = getStepAt (e.getPosition());

    if (step != hovered)
    {
        hovered = step;
        repaint();
    }
}

void SlicerPatternView::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void SlicerPatternView::paintStep (int step, Mode mode)
{
    if (juce::isPositiveAndBelow (step, processor.getSettings().steps) && processor.getMode (step) != mode)
        processor.setMode (step, mode);
}

void SlicerPatternView::mouseDown (const juce::MouseEvent& e)
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

    // Clicking a step that already has the brush's mode clears it; dragging paints the same thing across.
    paintMode = processor.getMode (step) == brush ? Mode::through : brush;
    paintStep (step, paintMode);
    lastPainted = step;
    painting = true;
}

void SlicerPatternView::mouseDrag (const juce::MouseEvent& e)
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
        paintStep (i, paintMode);
    }

    lastPainted = step;
    hovered = step;
}

void SlicerPatternView::mouseUp (const juce::MouseEvent&)
{
    painting = false;
}

void SlicerPatternView::mouseDoubleClick (const juce::MouseEvent& e)
{
    paintStep (getStepAt (e.getPosition()), Mode::through);
}

bool SlicerPatternView::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        paintStep (hovered, Mode::through);
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

    if (juce::isPositiveAndBelow (digit, slc::numModes))
    {
        brush = (Mode) digit;

        if (onBrushChanged)
            onBrushChanged();

        return true;
    }

    return false;
}

void SlicerPatternView::copyPattern() const
{
    juce::SystemClipboard::copyTextToClipboard (slc::patternToText (processor.getSettings()));
}

void SlicerPatternView::pastePattern()
{
    auto s = processor.getSettings();

    if (slc::patternFromText (juce::SystemClipboard::getTextFromClipboard(), s))
        processor.setSettings (s);
}

void SlicerPatternView::showMenu (int step)
{
    const auto s = processor.getSettings();
    const bool onStep = juce::isPositiveAndBelow (step, s.steps);
    auto pasted = s;
    const bool canPaste = slc::patternFromText (juce::SystemClipboard::getTextFromClipboard(), pasted);
    juce::PopupMenu menu;

    for (int m = 0; m < slc::numModes; ++m)
        menu.addItem (1 + m, juce::String (slc::getModeName ((Mode) m)), onStep, onStep && s.modes[(size_t) step] == (Mode) m);

    menu.addSeparator();
    menu.addItem (5, "Make every step Through");
    menu.addItem (6, "Copy pattern");
    menu.addItem (7, "Paste pattern", canPaste);
    menu.addSeparator();
    guideicon::addItem (menu, "What is a slicer?...", [safe = juce::Component::SafePointer<SlicerPatternView> (this)]
    {
        if (safe != nullptr && safe->onOpenGuide)
            safe->onOpenGuide();
    });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<SlicerPatternView> (this), step] (int choice)
    {
        if (safe == nullptr || choice == 0)
            return;

        if (choice >= 1 && choice <= slc::numModes)
            safe->paintStep (step, (Mode) (choice - 1));
        else if (choice == 5)
            for (int i = 0; i < slc::maxSteps; ++i)
                safe->processor.setMode (i, Mode::through);
        else if (choice == 6)
            safe->copyPattern();
        else if (choice == 7)
            safe->pastePattern();
    });
}

juce::String SlicerPatternView::getTooltip()
{
    const int step = getStepAt (getMouseXYRelative());

    if (step < 0)
        return "The steps of the pattern. Click one to paint it with the chosen mode.";

    const auto s = processor.getSettings();
    const auto mode = s.modes[(size_t) step];
    const bool missing = step == processor.playingStep.load() && (mode == Mode::repeat || mode == Mode::reverse) && ! processor.playingSliced.load();
    return slctext::getCellDescription (s, step, processor.getUsedBpm(), missing);
}

//==============================================================================
SlicerLastBarsView::SlicerLastBarsView (BuiltInSlicer& p) : processor (p)
{
    setTopic (*this, slctext::lastBars);
}

void SlicerLastBarsView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE LAST 2 BARS", r.removeFromTop (16.0f), juce::Justification::centredLeft);
    slctext::drawLastBars (g, r, slctext::getLastBars (processor), processor.getSettings());
}

//==============================================================================
BuiltInSlicerEditor::BuiltInSlicerEditor (BuiltInSlicer& p)
    : EffectEditor (p, "DawIt Slicer", "What is a slicer?", "Hear only what it changes", p.listenToEffect, p.clap, slctext::colour, fx::Clap::Sound::note),
      slicer (p)
{
    addAndMakeVisible (patternView);
    addAndMakeVisible (lastBars);
    patternView.onBrushChanged = [this] { setBrush (brush); };
    patternView.onOpenGuide = [this] { openGuide(); };

    for (int m = 0; m < slc::numModes; ++m)
    {
        auto& b = brushButtons[m];
        b.setButtonText (slc::getModeName ((Mode) m));
        b.setClickingTogglesState (false);
        b.setColour (juce::TextButton::buttonOnColourId, slctext::getModeColour ((Mode) m).brighter (m == (int) Mode::mute ? 0.3f : 0.0f));
        b.setWantsKeyboardFocus (false);
        b.setTooltip ("Brush: " + juce::String (slc::getModeName ((Mode) m)) + " (key " + juce::String (m + 1) + "). " + slctext::describe (slctext::modeButton + m));
        b.onClick = [this, m] { setBrush ((Mode) m); };
        setTopic (b, slctext::modeButton + m);
        addAndMakeVisible (b);
    }

    clapButton.setButtonText ("Play a bar");
    clapButton.setTooltip ("Play a bar of drums and a chord through the slicer");
    listenButton.setTooltip ("Silence the Through steps, to hear only the repeats and reverses (not saved)");

    const std::tuple<Knob, const char*, int, const char*> names[] = {
        { Knob::stepLength, "Step length", (int) Knob::stepLength, "How long a step lasts, as a note value" },
        { Knob::steps, "Steps", (int) Knob::steps, "How many steps before the pattern repeats" },
        { Knob::ownBpm, "BPM", (int) Knob::tempo, "The tempo of the slicer's own clock" },
        { Knob::roll, "Roll", (int) Knob::roll, "Repeat steps play a shorter piece this many times" },
        { Knob::smooth, "Smooth", (int) Knob::smooth, "Fade at every cut, so it doesn't click" },
        { Knob::mix, "Mix", (int) Knob::mix, "The sliced sound against the untouched track" } };

    for (auto& [knob, name, topicIndex, tip] : names)
    {
        knobs.add (new EffectKnob (slicer.getParameters(), BuiltInSlicer::getParameterId (knob), name, topicIndex, slctext::colour));
        knobs.getLast()->slider.setTooltip (tip);
    }

    tempoSwitch.button.setTooltip ("Follow the song's tempo, or a tempo of its own");
    tempoSwitch.button.onClick = [this] { slicer.setKnob (Knob::tempo, slicer.getSettings().ownTempo ? 0.0f : 1.0f); };

    setGroups ({ { "GRID", { knobs[0], knobs[1], &tempoSwitch, knobs[2] } }, { "SOUND", { knobs[3], knobs[4], knobs[5] } } });

    juce::StringArray presetNames;

    for (auto& preset : slc::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    setBrush (brush);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInSlicerEditor::setBrush (Mode mode)
{
    brush = mode;

    for (int m = 0; m < slc::numModes; ++m)
        brushButtons[m].setToggleState (m == (int) mode, juce::dontSendNotification);
}

void BuiltInSlicerEditor::applyPreset (int index)
{
    const auto& list = slc::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    slicer.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInSlicerEditor::createGuidePages()
{
    return slctext::makeGuide (slicer);
}

void BuiltInSlicerEditor::layoutPictures (juce::Rectangle<int> r)
{
    const auto patternArea = r.removeFromTop (r.getHeight() * 11 / 20);
    patternView.setBounds (patternArea);
    r.removeFromTop (10);
    lastBars.setBounds (r);

    auto buttons = patternArea.reduced (10, 6).removeFromTop (22).removeFromRight (4 * 84);

    for (auto& b : brushButtons)
        b.setBounds (buttons.removeFromLeft (84).reduced (2, 0));
}

void BuiltInSlicerEditor::tick()
{
    const auto s = slicer.getSettings();
    tempoSwitch.button.setButtonText (s.ownTempo ? "Own" : "Song");
    tempoSwitch.button.setToggleState (s.ownTempo, juce::dontSendNotification);
    tempoSwitch.button.setColour (juce::TextButton::buttonOnColourId, slctext::colour.darker (0.4f));
    knobs[2]->setAlpha (s.ownTempo || slicer.tempoBpm.load() <= 0.0 ? 1.0f : 0.55f);

    patternView.repaint();
    lastBars.repaint();
}

juce::AttributedString BuiltInSlicerEditor::buildText (int topicIndex)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = slicer.getSettings();
    const double bpm = slicer.tempoBpm.load();
    const auto& list = slc::getPresets();
    const slc::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topicIndex == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, slctext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear. The pattern is the main thing: click steps to change it.");

        for (const auto k : { Knob::stepLength, Knob::steps, Knob::roll, Knob::smooth, Knob::mix })
            addSection (text, slctext::getTitle ((int) k) + ": " + slc::formatKnob (k, preset->settings), {});

        return text;
    }

    if (topicIndex >= 0 && topicIndex < slc::numKnobs)
    {
        addTitle (text, slctext::getTitle (topicIndex), slctext::colour);
        addLead (text, slctext::describeSettings (topicIndex, s, bpm));
        addSection (text, "What it does", slctext::describe (topicIndex));
        addSection (text, "When and how it's set", slctext::getTypical (topicIndex));

        if (preset != nullptr && topicIndex != (int) Knob::tempo && topicIndex != (int) Knob::ownBpm)
            addSection (text, "In the " + preset->name + " starting point: " + slc::formatKnob ((Knob) topicIndex, preset->settings), preset->why);
        else if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that use.");

        return text;
    }

    addTitle (text, slctext::getTitle (topicIndex));

    switch (topicIndex)
    {
        case CommonTopic::overview:
            addLead (text, "DawIt Slicer cuts the track into steps on the beat and plays each one through, repeats the step before, plays it "
                           "backwards, or mutes it, in time with the song.");
            addSection (text, "When to use it", "For a stutter in the bar before a chorus, a hit that swells backwards into the next bar, a trance gate on a "
                                                "held pad, or a drum roll. It works best as a contrast, kept for fills and the last bar before a new section.");
            addSection (text, "How it works", "The song's beats are cut into steps of the Step length. Each step plays the track through, plays the "
                                              "step before it again (Repeat), plays it backwards (Reverse) or is silent (Mute). Every cut fades over "
                                              "Smooth, so it doesn't click.");
            addSection (text, "Reading the window", "Top: the pattern, over the beats. Pick a brush (Through, Repeat, Reverse, Mute), then click or drag across "
                                                    "steps; the lit one is playing. Under it, the last 2 bars: the track in grey, what comes out in colour. "
                                                    "Hover anything to read about it here; scroll here when the text runs long.");
            addSection (text, "Getting started", "Pick a starting point, then click Play a bar to hear the pattern even while the song is stopped. Read "
                                                 "What is a slicer? for how the steps, repeats and fades work.");
            addPluginParagraph (text, slicer);
            break;

        case CommonTopic::clap:
            addLead (text, "Plays one bar through the slicer, from step 1.");
            addSection (text, {}, slctext::describe (topicIndex));
            break;

        case CommonTopic::listen:
            addLead (text, slicer.listenToEffect.load() ? "On: you hear only the repeats and reverses." : "Off: you hear the track with its slicing.");
            addSection (text, {}, slctext::describe (topicIndex));
            break;

        case slctext::pattern:
            addLead (text, juce::String (s.steps) + " steps of " + slc::formatKnob (Knob::stepLength, s) + ". Brush: " + slc::getModeName (brush) + ".");
            addSection (text, {}, slctext::describe (topicIndex));
            addSection (text, "Shortcuts", "Keys 1 to 4 choose the brush. Delete over a step makes it Through. Cmd+C copies the pattern as a line of text and "
                                           "Cmd+V pastes one. Right-click a step for the modes and more.");
            break;

        default:
        {
            const auto lead = slctext::describeSettings (topicIndex, s, bpm);

            if (lead.isNotEmpty())
                addLead (text, lead);

            addSection (text, {}, slctext::describe (topicIndex));
            break;
        }
    }

    return text;
}
} // namespace daw
