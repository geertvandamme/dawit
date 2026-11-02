#include "SpectralFreezeEditor.h"

namespace daw
{
using sfz::Knob;
using namespace fxui;

//==============================================================================
SpectralFreezeSpectrumView::SpectralFreezeSpectrumView (BuiltInSpectralFreeze& p) : processor (p)
{
    setTopic (*this, sfztext::spectrum);
}

sfztext::SpectrumPlot SpectralFreezeSpectrumView::getPlot() const
{
    return sfztext::layoutSpectrum (getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (16.0f));
}

void SpectralFreezeSpectrumView::update()
{
    live = sfztext::getLiveSpectrum (processor);

    // The hold changes only when the user asks, so a few copies a second are plenty.
    if (ticks++ % 5 == 0)
    {
        sfz::Snapshot snapshot;
        const bool held = processor.getSnapshot (snapshot) && snapshot.isHeld();
        holdInfo = sfztext::describeHold (snapshot);
        hold = held ? sfztext::getHoldSpectrum (snapshot) : sfztext::Spectrum {};
    }

    repaint();
}

void SpectralFreezeSpectrumView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE SPECTRUM: 20 Hz TO 20 kHz", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    const auto plot = getPlot();
    sfztext::drawSpectrum (g, plot, &live, &hold, holdInfo.held ? holdInfo.peakHz : 0.0, hoverHz);

    if (! holdInfo.held)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("Nothing is held. Click Freeze while a sound rings, or click Test sound.", plot.area.withTrimmedTop (plot.area.getHeight() * 0.4f), juce::Justification::centredTop);
    }
}

void SpectralFreezeSpectrumView::mouseMove (const juce::MouseEvent& e)
{
    const auto plot = getPlot();
    hoverHz = plot.area.contains (e.position) ? plot.getHzAt (e.position.x) : 0.0;
    repaint();
}

void SpectralFreezeSpectrumView::mouseExit (const juce::MouseEvent&)
{
    hoverHz = 0.0;
    repaint();
}

void SpectralFreezeSpectrumView::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;   // clicking plays nothing: the picture is a measurement

    juce::PopupMenu menu;
    const bool holding = processor.getSettings().freeze;
    menu.addItem (1, "Capture again", holding);
    menu.addItem (2, "Let go", holding);
    menu.addSeparator();
    guideicon::addItem (menu, "What is a spectral freeze?...", [safe = juce::Component::SafePointer<SpectralFreezeSpectrumView> (this)]
    {
        if (safe != nullptr && safe->onOpenGuide)
            safe->onOpenGuide();
    });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<SpectralFreezeSpectrumView> (this)] (int choice)
    {
        if (safe == nullptr)
            return;

        if (choice == 1)
            safe->processor.requestCapture();
        else if (choice == 2)
            safe->processor.setFreeze (false);
    });
}

juce::String SpectralFreezeSpectrumView::getTooltip()
{
    if (hoverHz > 0.0)
        return sfztext::getHoverText (hoverHz, &live, hold.isEmpty() ? nullptr : &hold);

    return sfztext::getTooltip (sfztext::spectrum);
}

//==============================================================================
BuiltInSpectralFreezeEditor::BuiltInSpectralFreezeEditor (BuiltInSpectralFreeze& p)
    : EffectEditor (p, "DawIt Spectral Freeze", "What is a spectral freeze?", {}, unusedListen, unusedClap, sfztext::colour, fx::Clap::Sound::note),
      freeze (p),
      history (p.levels, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "the hold", sfztext::colour, "the track", ", as it is mixed in")
{
    addAndMakeVisible (spectrumView);
    addAndMakeVisible (history);
    addAndMakeVisible (status);
    spectrumView.onOpenGuide = [this] { openGuide(); };

    status.setJustificationType (juce::Justification::centredRight);
    status.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    status.setInterceptsMouseClicks (false, false);

    // A freeze has no "hear only what it adds"; its test sound plays and holds the chord.
    listenButton.setVisible (false);
    clapButton.setButtonText ("Test sound");
    clapButton.setTooltip (sfztext::getTooltip (CommonTopic::clap));
    clapButton.onClick = [this] { freeze.playTestChord(); };

    const std::pair<Knob, const char*> names[] = {
        { Knob::fade, "Fade" }, { Knob::brightness, "Brightness" }, { Knob::movement, "Movement" }, { Knob::mix, "Mix" }, { Knob::output, "Output" } };

    for (auto& [knob, name] : names)
    {
        knobs.add (new EffectKnob (freeze.getParameters(), BuiltInSpectralFreeze::getParameterId (knob), name, (int) knob, sfztext::colour));
        knobs.getLast()->slider.setTooltip (sfztext::getTooltip ((int) knob));
    }

    freezeSwitch.button.setTooltip (sfztext::getTooltip (sfztext::freezeSwitch));
    freezeSwitch.button.setColour (juce::TextButton::buttonOnColourId, sfztext::colour.darker (0.2f));
    freezeSwitch.button.onClick = [this] { freeze.setFreeze (! freeze.getSettings().freeze); };
    captureSwitch.button.setTooltip (sfztext::getTooltip (sfztext::captureAgain));
    captureSwitch.button.setButtonText ("Capture again");
    captureSwitch.button.onClick = [this] { freeze.requestCapture(); };

    setGroups ({ { "HOLD", { &freezeSwitch, &captureSwitch, knobs[0] } },
                 { "COLOUR", { knobs[1], knobs[2] } },
                 { "OUTPUT", { knobs[3], knobs[4] } } });

    juce::StringArray presetNames;

    for (auto& preset : sfz::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInSpectralFreezeEditor::resized()
{
    EffectEditor::resized();

    // The title is longer than the frame's usual 130 pixels and there is no "hear only" switch, so the header is laid out again.
    auto header = getLocalBounds().removeFromTop (44).reduced (12, 9);
    title.setBounds (header.removeFromLeft (190));
    presets.setBounds (header.removeFromLeft (180));
    header.removeFromLeft (10);
    guideButton.setBounds (header.removeFromLeft (216));
    helpButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (14);
    status.setBounds (header.removeFromRight (180));
    header.removeFromRight (10);
    clapButton.setBounds (header.removeFromRight (100));
}

void BuiltInSpectralFreezeEditor::applyPreset (int index)
{
    const auto& list = sfz::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    // A starting point sets the knobs and leaves Freeze as it is.
    auto s = freeze.getSettings();
    const auto& p = list[(size_t) index].settings;
    s.fadeMs = p.fadeMs;
    s.brightnessDb = p.brightnessDb;
    s.movementPercent = p.movementPercent;
    s.mixPercent = p.mixPercent;
    s.outputDb = p.outputDb;
    freeze.setSettings (s);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInSpectralFreezeEditor::createGuidePages()
{
    return sfztext::makeGuide (freeze);
}

void BuiltInSpectralFreezeEditor::layoutPictures (juce::Rectangle<int> r)
{
    spectrumView.setBounds (r.removeFromTop (r.getHeight() * 63 / 100));
    r.removeFromTop (10);
    history.setBounds (r);
}

void BuiltInSpectralFreezeEditor::tick()
{
    const auto s = freeze.getSettings();
    spectrumView.update();
    history.update();

    const auto& info = spectrumView.getHoldInfo();
    status.setText (sfztext::getStatus (s.freeze, info), juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, s.freeze ? sfztext::colour : theme::textDim);

    freezeSwitch.button.setToggleState (s.freeze, juce::dontSendNotification);
    freezeSwitch.button.setButtonText (s.freeze ? "Holding" : "Freeze");
    captureSwitch.button.setEnabled (s.freeze);
}

juce::AttributedString BuiltInSpectralFreezeEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = freeze.getSettings();
    const auto& list = sfz::getPresets();
    const sfz::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;
    const auto& info = spectrumView.getHoldInfo();

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, sfztext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track, click Freeze while a sound rings and adjust by ear.");

        for (const auto k : { Knob::fade, Knob::brightness, Knob::movement, Knob::mix, Knob::output })
            addSection (text, sfztext::getTitle ((int) k) + ": " + sfz::formatKnob (k, preset->settings), {});

        return text;
    }

    if (topic >= 0 && topic < sfz::numKnobs)
    {
        addTitle (text, sfztext::getTitle (topic), sfztext::colour);
        addLead (text, sfztext::describeSettings (topic, s));
        addSection (text, "What it does", sfztext::describe (topic));

        if (preset != nullptr)
            addSection (text, "In the " + preset->name + " starting point: " + sfz::formatKnob ((Knob) topic, preset->settings), preset->why);
        else
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that use.");

        return text;
    }

    addTitle (text, sfztext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "DawIt Spectral Freeze takes the last moment of a track's sound and holds it for as long as you like: a piano chord becomes a pad, a breath a soft wind, a cymbal a steady wash.");
            addSection (text, "When to use it", "For pads, drones and textures made from sounds you already have. Hold the last chord of a song under a spoken line. "
                                                "Hold a sung \"ah\" as a bed under a verse. Or stop a sound dead for a break.");
            addSection (text, "How it works", "It doesn't keep the wave itself. It keeps the sound's spectrum: how loud each frequency was in about the last 0.15 seconds before you clicked. "
                                              "From that list it builds new sound, over and over, so the hold has no beginning, no end and no loop that repeats.");
            addSection (text, "Reading the window", "Top: the spectrum, with the live sound as a thin line and the hold as a filled shape; its tallest peak is labelled with its frequency and nearest note. "
                                                    "Under it: the last 4 seconds of the track and of the hold. The Hold, Colour and Output controls are at the bottom. "
                                                    "Hover anything to read about it here; scroll here when the text runs long.");
            addSection (text, "Getting started", "Click Test sound to hear a plucked chord and see it held, or play your track and click Freeze while a sound rings. "
                                                 "Read What is a spectral freeze? for how spectra, frequency bins and the hold work.");
            addPluginParagraph (text, freeze);
            break;

        case CommonTopic::clap:
            addLead (text, "Plays a plucked A minor chord and holds it 0.25 s in.");
            addSection (text, {}, sfztext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [in, held] = history.getLoudest();

            if (in > -70.0f && held > -90.0f)
                addLead (text, "The last 4 seconds: the track up to " + juce::String (in, 1) + " dB, and the hold up to " + juce::String (held, 1) + " dB.");
            else
                addLead (text, "Play the track, or click Test sound, to see it here.");

            addSection (text, {}, sfztext::describe (topic));
            break;
        }

        case sfztext::freezeSwitch:
            addLead (text, s.freeze ? "Holding: " + sfztext::getStatus (true, info).fromFirstOccurrenceOf (juce::String::fromUTF8 ("\xc2\xb7 "), false, false) + "." : "Not holding.");
            addSection (text, {}, sfztext::describe (topic));
            break;

        case sfztext::spectrum:
            addLead (text, info.held ? "The hold's tallest peak is " + sfztext::formatHz (info.peakHz) + ", nearest " + sfz::getNoteName (info.peakHz) + ", at " + juce::String (juce::roundToInt (info.rmsDb)) + " dBFS."
                                     : "Nothing is held yet.");
            addSection (text, {}, sfztext::describe (topic));
            break;

        default:
            addSection (text, {}, sfztext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
