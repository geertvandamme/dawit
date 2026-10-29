#include "GuideButton.h"
#include "DrumsEditor.h"

namespace daw
{
using drums::Drum;
using drums::Knob;
using drums::numDrums;
using namespace fxui;

namespace
{
    void drawHeading (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText (text, r.removeFromTop (16.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    }

    Drum getSelected (const BuiltInDrums& p)
    {
        return (Drum) juce::jlimit (0, numDrums - 1, p.selectedDrum.load());
    }
}

//==============================================================================
DrumPadsView::DrumPadsView (BuiltInDrums& p) : processor (p)
{
    setTopic (*this, drumtext::pads);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

juce::Rectangle<float> DrumPadsView::getPlot() const
{
    return getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (20.0f);
}

void DrumPadsView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    const auto s = processor.getSettings();

    if (hovered)
        drawHeading (g, r, (juce::String (drums::getNote (*hovered)) + " " + drumtext::getNoteName (drums::getNote (*hovered)) + ": "
                            + drumtext::describeDrum (s, *hovered)).toUpperCase());
    else
        drawHeading (g, r, "THE PADS: CLICK ONE TO PLAY IT AND SHOW ITS KNOBS. NUMBERS: ITS MIDI NOTE");

    std::array<float, numDrums> levels;

    for (size_t i = 0; i < levels.size(); ++i)
        levels[i] = processor.shownLevels[i].load();

    drumtext::drawPads (g, getPlot(), getSelected (processor), &levels, true);
}

void DrumPadsView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        guideicon::addItem (menu, "What is drum synthesis?", [this] { if (onOpenGuide) onOpenGuide(); });
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
        return;
    }

    if (const auto d = drumtext::getPadAt (getPlot(), e.position))
    {
        if (onSelect)
            onSelect (*d);

        processor.clap.request (fx::Clap::Sound::note);
        repaint();
    }
}

void DrumPadsView::mouseMove (const juce::MouseEvent& e)
{
    const auto d = drumtext::getPadAt (getPlot(), e.position);

    if (d != hovered)
    {
        hovered = d;
        repaint();
    }
}

void DrumPadsView::mouseExit (const juce::MouseEvent&)
{
    hovered.reset();
    repaint();
}

//==============================================================================
/** One hit of the selected drum over its length, with the newest hit running along it. */
class BuiltInDrumsEditor::Hit final : public juce::Component
{
public:
    explicit Hit (BuiltInDrums& p) : processor (p)
    {
        setTopic (*this, drumtext::hit);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        const auto d = getSelected (processor);
        drawHeading (g, r, "ONE HIT OF THE " + drums::getDrumName (d).toUpperCase() + ": THE TONE GREEN, THE NOISE AMBER"
                               + (drums::isUsed (d, Knob::bend) ? juce::String (", THE PITCH BLUE") : juce::String()));
        r.removeFromTop (4.0f);
        drumtext::drawHit (g, r, processor.getSettings(), d, processor.selectedAge.load());
    }

private:
    BuiltInDrums& processor;
};

//==============================================================================
/** The tone's and the noise's frequencies in the selected drum's first 170 ms. */
class BuiltInDrumsEditor::Spectrum final : public juce::Component
{
public:
    explicit Spectrum (BuiltInDrums& p) : processor (p)
    {
        setTopic (*this, drumtext::spectrum);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "ITS FREQUENCIES, 30 Hz TO 16 kHz, IN dB UNDER THE LOUDEST");
        r.removeFromTop (4.0f);
        drumtext::drawSpectrum (g, r, processor.getSettings(), getSelected (processor));
    }

private:
    BuiltInDrums& processor;
};

//==============================================================================
BuiltInDrumsEditor::BuiltInDrumsEditor (BuiltInDrums& p)
    : EffectEditor (p, "DawIt Drums", "What is drum synthesis?", "Hear the drum alone", p.onlySelected, p.clap, drumtext::colour,
                    fx::Clap::Sound::note),
      synth (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "the whole kit", drumtext::colour, "the selected drum alone", {})
{
    clapButton.setButtonText ("Play a hit");
    hitView = std::make_unique<Hit> (synth);
    spectrumView = std::make_unique<Spectrum> (synth);
    padsView.onSelect = [this] (Drum d) { select (d); };
    padsView.onOpenGuide = [this] { guideButton.triggerClick(); };
    addAndMakeVisible (padsView);
    addAndMakeVisible (*hitView);
    addAndMakeVisible (*spectrumView);
    addAndMakeVisible (history);

    for (int k = 0; k < drums::numKnobs; ++k)
    {
        const auto knob = (Knob) k;

        switch (knob)
        {
            case Knob::choke:
                chokeSwitch = std::make_unique<Switch> (drums::getKnobName (knob), k);
                chokeSwitch->button.setColour (juce::TextButton::buttonColourId, drumtext::colour.withAlpha (0.25f));
                chokeSwitch->button.onClick = [this] { synth.setKnob (Knob::choke, synth.getSettings().choke ? 0.0f : 1.0f); };
                break;

            case Knob::tune:
            case Knob::bend:
            case Knob::bendTime:
            case Knob::decay:
            case Knob::noise:
            case Knob::noiseDecay:
            case Knob::tone:
            case Knob::click:
            case Knob::level:
            case Knob::velocity:
            case Knob::output:
            {
                const auto c = knob == Knob::noise || knob == Knob::noiseDecay || knob == Knob::tone || knob == Knob::click ? drumtext::noiseColour
                             : knob == Knob::bend || knob == Knob::bendTime ? drumtext::pitchColour
                             : knob == Knob::tune || knob == Knob::decay ? drumtext::toneColour
                             : drumtext::colour;
                knobs[knob] = std::make_unique<EffectKnob> (synth.getParameters(), BuiltInDrums::getParameterId (knob, Drum::kick),
                                                            drums::getKnobName (knob), k, c);
                break;
            }
        }
    }

    drumSwitch = std::make_unique<Switch> ("Drum", drumtext::selectDrum);
    drumSwitch->button.onClick = [this] { select ((Drum) (((int) selected + 1) % numDrums)); };

    juce::StringArray presetNames;

    for (auto& preset : drums::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    select (getSelected (synth));
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

void BuiltInDrumsEditor::select (Drum d)
{
    selected = d;
    synth.selectedDrum = (int) d;
    const auto c = drumtext::getDrumColour (d);

    for (int k = 0; k < drums::numDrumKnobs; ++k)
    {
        auto& knob = *knobs.at ((Knob) k);
        const bool used = drums::isUsed (d, (Knob) k);
        knob.attachTo (BuiltInDrums::getParameterId ((Knob) k, d));
        knob.setEnabled (used);
        knob.setAlpha (used ? 1.0f : 0.35f);
    }

    drumSwitch->button.setButtonText (drums::getDrumName (d));
    drumSwitch->button.setColour (juce::TextButton::buttonColourId, c.withAlpha (0.35f));
    updateGroups();
    padsView.repaint();

    if ((getTopic() >= 0 && getTopic() < drums::numDrumKnobs) || getTopic() == drumtext::selectDrum)
        showTopic (getTopic());
}

void BuiltInDrumsEditor::updateGroups()
{
    auto knob = [this] (Knob k) -> juce::Component* { return knobs.at (k).get(); };

    setGroups ({ { drums::getDrumName (selected).toUpperCase() + ": THE TONE", { drumSwitch.get(), knob (Knob::tune), knob (Knob::bend), knob (Knob::bendTime),
                                                                                  knob (Knob::decay) }, 0 },
                 { "THE NOISE", { knob (Knob::noise), knob (Knob::noiseDecay), knob (Knob::tone) }, 0 },
                 { "THE HIT", { knob (Knob::click), knob (Knob::level) }, 1 },
                 { "THE KIT", { chokeSwitch.get(), knob (Knob::velocity), knob (Knob::output) }, 1 } });
    resized();
    repaint();
}

void BuiltInDrumsEditor::applyPreset (int index)
{
    const auto& list = drums::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    synth.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInDrumsEditor::createGuidePages()
{
    return drumtext::makeGuide (synth);
}

void BuiltInDrumsEditor::layoutPictures (juce::Rectangle<int> r)
{
    padsView.setBounds (r.removeFromLeft (r.getWidth() * 36 / 100));
    r.removeFromLeft (10);
    hitView->setBounds (r.removeFromTop ((r.getHeight() - 10) * 52 / 100));
    r.removeFromTop (10);
    spectrumView->setBounds (r.removeFromLeft ((r.getWidth() - 10) / 2));
    r.removeFromLeft (10);
    history.setBounds (r);
}

void BuiltInDrumsEditor::tick()
{
    chokeSwitch->button.setButtonText (drums::formatKnob (Knob::choke, synth.getSettings()));

    // Picked from elsewhere (the guide plays the selected drum, a host can't change it): follow the processor.
    if (getSelected (synth) != selected)
        select (getSelected (synth));

    history.update();
    padsView.repaint();
    hitView->repaint();
    spectrumView->repaint();
}

juce::AttributedString BuiltInDrumsEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = synth.getSettings();
    const auto& list = drums::getPresets();
    const drums::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;
    const auto name = drums::getDrumName (selected);

    auto addDrumNote = [&] (Drum d)
    {
        if (preset != nullptr && preset->drums[(size_t) d].isNotEmpty())
            addSection (text, "In " + preset->name + ": the " + drums::getDrumName (d).toLowerCase(), preset->drums[(size_t) d]);
    };

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, drumtext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "Click the pads to hear each drum, then change one knob at a time. Drums not named here are as in Init.");

        for (int d = 0; d < numDrums; ++d)
            if (preset->drums[(size_t) d].isNotEmpty())
                addSection (text, drumtext::describeDrum (preset->settings, (Drum) d), preset->drums[(size_t) d]);

        for (int k = drums::numDrumKnobs; k < drums::numKnobs; ++k)
            if (preset->notes[(size_t) k].isNotEmpty())
                addSection (text, drumtext::getTitle (k) + ": " + drums::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < drums::numKnobs)
    {
        const bool ofDrum = drums::isDrumKnob ((Knob) topic);
        addTitle (text, (ofDrum ? name + ": " : juce::String()) + drumtext::getTitle (topic), ofDrum ? drumtext::getDrumColour (selected) : drumtext::colour);
        addLead (text, drumtext::describeSettings (topic, s, selected));
        addSection (text, "What it does", drumtext::describe (topic));
        addSection (text, "How it's usually set", drumtext::getTypical (topic));

        if (ofDrum)
            addDrumNote (selected);
        else if (preset != nullptr && preset->notes[(size_t) topic].isNotEmpty())
            addSection (text, "In " + preset->name + ": " + drums::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);

        if (preset == nullptr)
            addSection (text, {}, "Pick a kit to read why each drum is made the way it is.");

        return text;
    }

    addTitle (text, drumtext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "A drum machine that makes every drum from scratch, the way the Roland TR-808 did: a sine that drops in pitch for "
                           "the kick, a tone and noise for the snare, bursts of noise for the clap, six clashing square waves for the "
                           "hi-hats and the cymbal. Every part can be tuned and shaped, and every knob says what it does.");
            addSection (text, "How the sound is made", "Each drum has a tone (Tune, with a Bend that starts it higher, and a Decay), noise "
                                                       "(how much, how long, and a Tone filter), a click at the start, and its level. A "
                                                       "closed hi-hat chokes an open one. The track's MIDI notes play the drums on the "
                                                       "General MIDI drum map, so a MIDI file's drums play the right drums.");
            addSection (text, "Reading the window", "Left, the pads: click one to play it and show its knobs below. Right, one hit of that "
                                                    "drum over time (its tone, its noise and its pitch), its frequencies, and the last 4 "
                                                    "seconds. Hover anything to read about it here.");
            addSection (text, "Getting started", "Pick a kit; each says what matters in each drum. Click the pads, or play the track: C2 "
                                                 "is the kick, D2 the snare, F#2 the closed hi-hat. Read What is drum synthesis? for how "
                                                 "each drum is made and how to build a kit.");
            break;

        case CommonTopic::listen:
            addLead (text, synth.onlySelected.load() ? "On: you hear only the " + name.toLowerCase() + "." : juce::String ("Off: you hear the whole kit."));
            addSection (text, {}, drumtext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [before, after] = history.getLoudest();

            if (after > -70.0f)
                addLead (text, "The last 4 seconds: the " + name.toLowerCase() + " up to " + juce::String (before, 1) + " dB, the whole kit up to "
                                   + juce::String (after, 1) + " dB.");
            else
                addLead (text, "Play the track or a pad to see it here.");

            addSection (text, {}, drumtext::describe (topic));
            break;
        }

        case drumtext::pads:
        case drumtext::hit:
        case drumtext::spectrum:
        case drumtext::selectDrum:
            addLead (text, drumtext::describeDrum (s, selected) + ".");
            addSection (text, {}, drumtext::describe (topic));
            addDrumNote (selected);
            break;

        default:
            addSection (text, {}, drumtext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
