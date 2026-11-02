#include "StepFilterEditor.h"

namespace daw
{
using stf::Knob;
using namespace fxui;

//==============================================================================
StepFilterPatternView::StepFilterPatternView (BuiltInStepFilter& p) : processor (p)
{
    setTopic (*this, stftext::pattern);
    setWantsKeyboardFocus (true);
}

stftext::PatternGrid StepFilterPatternView::getGrid() const
{
    return stftext::layoutPattern (getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (16.0f));
}

int StepFilterPatternView::getStepAt (juce::Point<int> p) const
{
    return getGrid().getStepAt (p.toFloat());
}

void StepFilterPatternView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE PATTERN", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    const auto s = processor.getSettings();
    stftext::drawPattern (g, getGrid(), s, processor.getUsedBpm(), processor.playingStep.load(), hovered);
}

void StepFilterPatternView::mouseMove (const juce::MouseEvent& e)
{
    const int step = getStepAt (e.getPosition());

    if (step != hovered)
    {
        hovered = step;
        repaint();
    }
}

void StepFilterPatternView::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

float StepFilterPatternView::getPaintedHeight (const juce::MouseEvent& e) const
{
    const float exact = getGrid().getHeightAt ((float) e.y);

    // Snap to 5%, unless Alt is held for an exact value.
    return e.mods.isAltDown() ? exact : std::round (exact / 5.0f) * 5.0f;
}

void StepFilterPatternView::setStep (int step, float percent)
{
    if (juce::isPositiveAndBelow (step, processor.getSettings().steps))
        processor.setHeight (step, percent);
}

void StepFilterPatternView::mouseDown (const juce::MouseEvent& e)
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

    lastHeight = getPaintedHeight (e);
    setStep (step, lastHeight);
    lastPainted = step;
    painting = true;
}

void StepFilterPatternView::mouseDrag (const juce::MouseEvent& e)
{
    if (! painting)
        return;

    const int step = getStepAt (e.getPosition());
    const float height = getPaintedHeight (e);

    if (step < 0)
        return;

    // Fill in the steps a fast drag jumped over, with heights in between.
    for (int i = lastPainted; i != step;)
    {
        i += step > i ? 1 : -1;
        const float t = (float) (i - lastPainted) / (float) (step - lastPainted);
        setStep (i, juce::jmap (t, lastHeight, height));
    }

    setStep (step, height);
    lastPainted = step;
    lastHeight = height;
    hovered = step;
}

void StepFilterPatternView::mouseUp (const juce::MouseEvent&)
{
    painting = false;
}

void StepFilterPatternView::mouseDoubleClick (const juce::MouseEvent& e)
{
    setStep (getStepAt (e.getPosition()), 100.0f);
}

bool StepFilterPatternView::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        setStep (hovered, 100.0f);
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
    }

    return false;
}

void StepFilterPatternView::copyPattern() const
{
    juce::SystemClipboard::copyTextToClipboard (stf::patternToText (processor.getSettings()));
}

void StepFilterPatternView::pastePattern()
{
    auto s = processor.getSettings();

    if (stf::patternFromText (juce::SystemClipboard::getTextFromClipboard(), s))
        processor.setSettings (s);
}

void StepFilterPatternView::showMenu (int step)
{
    const auto s = processor.getSettings();
    const bool onStep = juce::isPositiveAndBelow (step, s.steps);
    auto pasted = s;
    const bool canPaste = stf::patternFromText (juce::SystemClipboard::getTextFromClipboard(), pasted);
    juce::PopupMenu menu;

    menu.addItem (1, "Lowest", onStep);
    menu.addItem (2, "Halfway", onStep);
    menu.addItem (3, "Highest", onStep);
    menu.addItem (4, "Make every step the same as this one", onStep);
    menu.addSeparator();
    menu.addItem (5, "Copy pattern");
    menu.addItem (6, "Paste pattern", canPaste);
    menu.addSeparator();
    guideicon::addItem (menu, "What is a step filter?...", [safe = juce::Component::SafePointer<StepFilterPatternView> (this)]
    {
        if (safe != nullptr && safe->onOpenGuide)
            safe->onOpenGuide();
    });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<StepFilterPatternView> (this), step] (int choice)
    {
        if (safe == nullptr || choice == 0)
            return;

        if (choice >= 1 && choice <= 3)
            safe->setStep (step, choice == 1 ? 0.0f : choice == 2 ? 50.0f : 100.0f);
        else if (choice == 4)
            for (int i = 0; i < stf::maxSteps; ++i)
                safe->processor.setHeight (i, safe->processor.getHeight (step));
        else if (choice == 5)
            safe->copyPattern();
        else if (choice == 6)
            safe->pastePattern();
    });
}

juce::String StepFilterPatternView::getTooltip()
{
    const int step = getStepAt (getMouseXYRelative());

    if (step < 0)
        return "The steps of the pattern. Click or drag to set their heights.";

    return stftext::getCellDescription (processor.getSettings(), step);
}

//==============================================================================
StepFilterResponseView::StepFilterResponseView (BuiltInStepFilter& p) : processor (p)
{
    setTopic (*this, stftext::filterNow);
}

void StepFilterResponseView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE FILTER NOW", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    const auto s = processor.getSettings();
    const double cutoff = processor.playingStep.load() >= 0 ? (double) processor.currentCutoffHz.load()
                                                            : stf::clampCutoff (stf::getStepCutoffHz (s, 0), 44100.0);
    stftext::drawFilterNow (g, r, s, cutoff);
}

//==============================================================================
BuiltInStepFilterEditor::BuiltInStepFilterEditor (BuiltInStepFilter& p)
    : EffectEditor (p, "DawIt Step Filter", "What is a step filter?", "Hear only what it removes", p.listenToEffect, p.clap, stftext::colour, fx::Clap::Sound::note),
      filter (p),
      history (p.levels, [&p] { return p.getSampleRate(); }, [&p] { return p.tempoBpm.load(); }, "what the filter changed", stftext::colour, "the track", {})
{
    addAndMakeVisible (patternView);
    addAndMakeVisible (responseView);
    addAndMakeVisible (history);
    patternView.onOpenGuide = [this] { openGuide(); };

    clapButton.setButtonText ("Play a bar");
    clapButton.setTooltip (stftext::getTooltip (CommonTopic::clap));
    listenButton.setTooltip (stftext::getTooltip (CommonTopic::listen));

    const std::tuple<Knob, const char*, int> names[] = {
        { Knob::stepLength, "Step length", (int) Knob::stepLength },
        { Knob::steps, "Steps", (int) Knob::steps },
        { Knob::ownBpm, "BPM", (int) Knob::tempo },
        { Knob::lowCutoff, "Low cutoff", (int) Knob::lowCutoff },
        { Knob::highCutoff, "High cutoff", (int) Knob::highCutoff },
        { Knob::resonance, "Resonance", (int) Knob::resonance },
        { Knob::glide, "Glide", (int) Knob::glide },
        { Knob::mix, "Mix", (int) Knob::mix },
        { Knob::output, "Output", (int) Knob::output } };

    for (auto& [knob, name, topic] : names)
    {
        knobs.add (new EffectKnob (filter.getParameters(), BuiltInStepFilter::getParameterId (knob), name, topic, stftext::colour));
        knobs.getLast()->slider.setTooltip (stftext::getTooltip ((int) knob));
    }

    tempoSwitch.button.setTooltip (stftext::getTooltip ((int) Knob::tempo));
    tempoSwitch.button.onClick = [this] { filter.setKnob (Knob::tempo, filter.getSettings().ownTempo ? 0.0f : 1.0f); };

    for (auto* sw : { &typeSwitch, &slopeSwitch })
        sw->button.setColour (juce::TextButton::buttonColourId, stftext::colour.withAlpha (0.25f));

    typeSwitch.button.setTooltip (stftext::getTooltip ((int) Knob::type));
    typeSwitch.button.onClick = [this] { cycle (Knob::type); };
    slopeSwitch.button.setTooltip (stftext::getTooltip ((int) Knob::slope));
    slopeSwitch.button.onClick = [this] { cycle (Knob::slope); };

    setGroups ({ { "GRID", { knobs[0], knobs[1], &tempoSwitch, knobs[2] } },
                 { "FILTER", { &typeSwitch, &slopeSwitch, knobs[3], knobs[4], knobs[5] } },
                 { "SOUND", { knobs[6], knobs[7], knobs[8] } } });

    juce::StringArray presetNames;

    for (auto& preset : stf::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 640);
}

void BuiltInStepFilterEditor::cycle (Knob knob)
{
    if (auto* p = filter.getKnobParameter (knob))
    {
        const int count = juce::jmax (2, (int) std::round (p->getNormalisableRange().end) + 1);
        const int next = (juce::roundToInt (p->convertFrom0to1 (p->getValue())) + 1) % count;
        filter.setKnob (knob, (float) next);
    }
}

void BuiltInStepFilterEditor::applyPreset (int index)
{
    const auto& list = stf::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    filter.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInStepFilterEditor::createGuidePages()
{
    return stftext::makeGuide (filter);
}

void BuiltInStepFilterEditor::layoutPictures (juce::Rectangle<int> r)
{
    auto top = r.removeFromTop (r.getHeight() * 55 / 100);
    r.removeFromTop (10);
    responseView.setBounds (top.removeFromRight (top.getWidth() * 30 / 100));
    top.removeFromRight (10);
    patternView.setBounds (top);
    history.setBounds (r);
}

void BuiltInStepFilterEditor::tick()
{
    const auto s = filter.getSettings();
    tempoSwitch.button.setButtonText (s.ownTempo ? "Own" : "Song");
    tempoSwitch.button.setToggleState (s.ownTempo, juce::dontSendNotification);
    tempoSwitch.button.setColour (juce::TextButton::buttonOnColourId, stftext::colour.darker (0.4f));
    typeSwitch.button.setButtonText (stf::formatKnob (Knob::type, s));
    slopeSwitch.button.setButtonText (stf::formatKnob (Knob::slope, s));
    knobs[2]->setAlpha (s.ownTempo || filter.tempoBpm.load() <= 0.0 ? 1.0f : 0.55f);

    patternView.repaint();
    responseView.repaint();
    history.update();
}

juce::AttributedString BuiltInStepFilterEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = filter.getSettings();
    const double bpm = filter.tempoBpm.load();
    const auto& list = stf::getPresets();
    const stf::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, stftext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear. The pattern is the main thing: click or drag across the bars to change it.");

        for (const auto k : { Knob::stepLength, Knob::steps, Knob::type, Knob::slope, Knob::lowCutoff, Knob::highCutoff, Knob::resonance, Knob::glide, Knob::mix, Knob::output })
            addSection (text, stftext::getTitle ((int) k) + ": " + stf::formatKnob (k, preset->settings), {});

        return text;
    }

    if (topic >= 0 && topic < stf::numKnobs)
    {
        addTitle (text, stftext::getTitle (topic), stftext::colour);
        addLead (text, stftext::describeSettings (topic, s, bpm));
        addSection (text, "What it does", stftext::describe (topic));
        addSection (text, "When and how it's set", stftext::getTypical (topic));

        if (preset != nullptr && topic != (int) Knob::tempo && topic != (int) Knob::ownBpm)
            addSection (text, "In the " + preset->name + " starting point: " + stf::formatKnob ((Knob) topic, preset->settings), preset->why);
        else if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that use.");

        return text;
    }

    addTitle (text, stftext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "DawIt Step Filter gives each step of a pattern its own filter cutoff, in time with the song, so a steady sound moves in tone: "
                           "dark and bright, thin and full.");
            addSection (text, "When to use it", "For a held chord that pulses between dark and bright, a squelchy acid line, a talking band-pass, or a "
                                                "high-pass that rises over four bars before a drop. It works best on steady, bright sounds such as pads, saw basses and loops.");
            addSection (text, "How it works", "The song's beats are cut into steps of the Step length. Each step has a height, from Low cutoff at the bottom "
                                              "to High cutoff at the top, spaced in octaves. The filter slides to each step's cutoff over Glide, then the "
                                              "result is mixed with the untouched track.");
            addSection (text, "Reading the window", "Top left: the pattern. Click or drag across the bars to set their heights; the lit one is playing. "
                                                    "Top right: the filter at the cutoff playing now. Under them: the last 4 seconds of the track and of what the filter changed. "
                                                    "Hover anything to read about it here; scroll here when the text runs long.");
            addSection (text, "Getting started", "Pick a starting point, then click Play a bar to hear the pattern even while the song is stopped. Read "
                                                 "What is a step filter? for how cutoff, resonance, steps and glide work.");
            addPluginParagraph (text, filter);
            break;

        case CommonTopic::clap:
            addLead (text, "Plays one bar of a held chord through the step filter, from step 1.");
            addSection (text, {}, stftext::describe (topic));
            break;

        case CommonTopic::listen:
            addLead (text, filter.listenToEffect.load() ? "On: you hear only what the filter takes away." : "Off: you hear the track with its filtering.");
            addSection (text, {}, stftext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [in, change] = history.getLoudest();

            if (in > -70.0f && change > -90.0f)
                addLead (text, "The last 4 seconds: the track up to " + juce::String (in, 1) + " dB, and the filter changing it by at most " + juce::String (change, 1) + " dB.");
            else
                addLead (text, "Play the track, or click Play a bar, to see it here.");

            addSection (text, {}, stftext::describe (topic));
            break;
        }

        case stftext::pattern:
            addLead (text, juce::String (s.steps) + " steps of " + stf::formatKnob (Knob::stepLength, s) + ", " + stf::formatHz (juce::jmin (s.lowHz, s.highHz)) + " to "
                               + stf::formatHz (juce::jmax (s.lowHz, s.highHz)) + ".");
            addSection (text, {}, stftext::describe (topic));
            addSection (text, "Shortcuts", "Click or drag to set steps; values snap to 5%, and Alt drags without snapping. Double-click or Delete over a step "
                                           "opens it fully. Cmd+C copies the pattern as a line of text and Cmd+V pastes one. Right-click a step for quick heights and more.");
            break;

        default:
        {
            const double cutoff = filter.playingStep.load() >= 0 ? (double) filter.currentCutoffHz.load() : stf::getStepCutoffHz (s, 0);
            addLead (text, topic == stftext::filterNow ? "A " + juce::String (s.steep ? "24" : "12") + " dB " + stf::getTypeName (s.type).toLowerCase() + " with its cutoff at " + stf::formatHz (cutoff) + "."
                                                       : stftext::describeSettings (topic, s, bpm));
            addSection (text, {}, stftext::describe (topic));
            break;
        }
    }

    return text;
}
} // namespace daw
