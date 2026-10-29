#include "SaturationEditor.h"

namespace daw
{
using sat::Knob;
using namespace fxui;

//==============================================================================
SaturationCurveView::SaturationCurveView (BuiltInSaturation& p) : processor (p)
{
    setTopic (*this, sattext::curve);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void SaturationCurveView::paint (juce::Graphics& g)
{
    const auto s = processor.getSettings();
    const float peak = sattext::getShownPeak (processor.recentPeak.load());
    drawPanel (g, getLocalBounds().toFloat());

    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE CURVE: IN ACROSS, OUT UP", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    auto wave = r.removeFromBottom (juce::jmin (90.0f, r.getHeight() * 0.25f));
    sattext::drawCurve (g, r.reduced (4.0f), s, peak, true);

    g.setFont (juce::FontOptions (10.5f));
    g.setColour (theme::textDim);
    g.drawText ("a sine at the track's level: before, and after as loud", wave.removeFromTop (14.0f), juce::Justification::centredLeft);
    sattext::drawBeforeAfter (g, wave, s, peak, 2.5);
}

void SaturationCurveView::mouseDown (const juce::MouseEvent&)
{
    dragStartDrive = processor.getSettings().driveDb;

    if (auto* p = processor.getKnobParameter (Knob::drive))
        p->beginChangeGesture();
}

void SaturationCurveView::mouseDrag (const juce::MouseEvent& e)
{
    const float drive = dragStartDrive - (float) e.getDistanceFromDragStartY() * 0.1f;
    processor.setKnob (Knob::drive, juce::jlimit (0.0f, sat::maxDrive, std::round (drive * 10.0f) / 10.0f));
    repaint();
}

void SaturationCurveView::mouseUp (const juce::MouseEvent&)
{
    if (auto* p = processor.getKnobParameter (Knob::drive))
        p->endChangeGesture();
}

void SaturationCurveView::mouseDoubleClick (const juce::MouseEvent&)
{
    processor.setKnob (Knob::drive, sat::Settings().driveDb);
}

//==============================================================================
/** The harmonics the curve makes of a sine at the track's level. */
class BuiltInSaturationEditor::Harmonics final : public juce::Component
{
public:
    explicit Harmonics (BuiltInSaturation& p) : processor (p)
    {
        setTopic (*this, sattext::harmonics);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("THE HARMONICS IT ADDS TO A NOTE", r.removeFromTop (16.0f), juce::Justification::centredLeft);
        sattext::drawHarmonics (g, r, processor.getSettings(), sattext::getShownPeak (processor.recentPeak.load()), 10, true);
    }

private:
    BuiltInSaturation& processor;
};

//==============================================================================
BuiltInSaturationEditor::BuiltInSaturationEditor (BuiltInSaturation& p)
    : EffectEditor (p, "DawIt Saturation", "What is saturation?", "Hear only what it adds", p.listenToEffect, p.clap, sattext::colour,
                    fx::Clap::Sound::note),
      saturation (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "what it adds", sattext::colour)
{
    harmonics = std::make_unique<Harmonics> (saturation);
    addAndMakeVisible (curve);
    addAndMakeVisible (*harmonics);
    addAndMakeVisible (history);

    const std::pair<Knob, const char*> names[] = { { Knob::drive, "Drive" }, { Knob::tone, "Tone" }, { Knob::output, "Output" }, { Knob::mix, "Mix" } };

    for (auto& [knob, name] : names)
        knobs.add (new EffectKnob (saturation.getParameters(), BuiltInSaturation::getParameterId (knob), name, (int) knob, sattext::colour));

    typeSwitch.button.onClick = [this]
    {
        const int next = ((int) saturation.getSettings().type + 1) % sat::numTypes;
        saturation.setKnob (Knob::type, (float) next);
    };

    matchSwitch.button.onClick = [this] { saturation.setMatchingLevel (! saturation.isMatchingLevel()); };
    matchSwitch.button.setColour (juce::TextButton::buttonOnColourId, sattext::colour.darker (0.4f));

    setGroups ({ { "HOW MUCH", { knobs[0] } }, { "WHAT KIND", { &typeSwitch } }, { "ITS SOUND", { knobs[1] } },
                 { "LEVEL", { knobs[2], &matchSwitch } }, { "BLEND", { knobs[3] } } });

    juce::StringArray presetNames;

    for (auto& preset : sat::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInSaturationEditor::applyPreset (int index)
{
    const auto& list = sat::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    saturation.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInSaturationEditor::createGuidePages()
{
    return sattext::makeGuide (saturation);
}

void BuiltInSaturationEditor::layoutPictures (juce::Rectangle<int> r)
{
    curve.setBounds (r.removeFromLeft (juce::jmin (340, r.getHeight() - 20)));
    r.removeFromLeft (10);
    harmonics->setBounds (r.removeFromTop (r.getHeight() * 11 / 20));
    r.removeFromTop (10);
    history.setBounds (r);
}

void BuiltInSaturationEditor::tick()
{
    const auto s = saturation.getSettings();
    typeSwitch.button.setButtonText (sat::getTypeName (s.type));
    matchSwitch.button.setButtonText (saturation.isMatchingLevel() ? "On" : "Off");
    matchSwitch.button.setToggleState (saturation.isMatchingLevel(), juce::dontSendNotification);

    history.update();
    curve.repaint();
    harmonics->repaint();
}

juce::AttributedString BuiltInSaturationEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = saturation.getSettings();
    const float peak = sattext::getShownPeak (saturation.recentPeak.load());
    const auto& list = sat::getPresets();
    const sat::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, sattext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear, with match level on.");

        for (int k = 0; k < sat::numKnobs; ++k)
            addSection (text, sattext::getTitle (k).upToFirstOccurrenceOf (":", false, false) + ": " + sat::formatKnob ((Knob) k, preset->settings),
                        preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < sat::numKnobs)
    {
        addTitle (text, sattext::getTitle (topic), sattext::colour);
        auto lead = sattext::describeSettings (topic, s, peak);

        if (topic == (int) Knob::output && saturation.isMatchingLevel())
            lead << " Match level adds " << juce::String (saturation.matchGainDb.load(), 1) << " dB right now.";

        addLead (text, lead);
        addSection (text, "What it does", sattext::describe (topic));
        addSection (text, "When and how it's set", sattext::getTypical (topic));

        if (preset != nullptr)
            addSection (text, "In the " + preset->name + " starting point: " + sat::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);
        else
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that sound.");

        return text;
    }

    addTitle (text, sattext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "Saturation pushes a sound into a curve that rounds off or cuts off its peaks, like tape, a tube amp or "
                           "a fuzz pedal pushed too far. The new shape adds harmonics: a little gives warmth and presence, a lot gives "
                           "grit and fuzz.");
            addSection (text, "When to use it", "When a part should sound warmer, fuller, denser or more present without sounding "
                                                "distorted: a vocal that should stand out without being louder, a bass that must be heard "
                                                "on phones, drums that need punch and loudness, a thin digital synth, a bus or the whole "
                                                "mix that should feel glued. Usually a little on many tracks rather than a lot on one. "
                                                "For grit you hear as distortion, DawIt Distortion goes further.");
            addSection (text, "How it's usually set", "Tape or tube, 4-12 dB of drive, match level on, listening for the sound getting "
                                                      "fuller rather than for distortion. The tone at 8-12 kHz if the top gets fizzy. A mix "
                                                      "of 30-60% on drums and bass, to keep the clean attack and low end. Before a "
                                                      "compressor for movement, after one for steadier saturation.");
            addSection (text, "How it works", "Drive turns the sound up into the curve. The Type is the curve's shape: soft and even "
                                              "(Tape), soft and lopsided (Tube), or hard (Clip). Tone tames the fizz the harmonics can "
                                              "add, Output and Match level set the level, and Mix blends the saturated sound with the "
                                              "clean one.");
            addSection (text, "Reading the window", "Left, the curve, with the part the track uses drawn thick, and a sine before and "
                                                    "after it. Right, the harmonics it makes of a note at the track's level, and under "
                                                    "them the last 4 seconds: the track and what the saturation adds. Hover anything to "
                                                    "read about it here; scroll here when the text runs long.");
            addSection (text, "Getting started", "Pick a starting point; each tells why every knob is set the way it is. Click Play a "
                                                 "note to hear one plain note through it, even while the song is stopped. Leave match "
                                                 "level on to compare honestly. Read What is saturation? for how it works.");
            break;

        case CommonTopic::listen:
            addLead (text, saturation.listenToEffect.load() ? "On: you hear only what the saturation adds." : "Off: you hear the saturated track.");
            addSection (text, {}, sattext::describe (topic));
            break;

        case sattext::match:
            addLead (text, saturation.isMatchingLevel() ? "On: it turns the sound by " + juce::String (saturation.matchGainDb.load(), 1) + " dB right now."
                                                        : "Off. Turned on, it would turn the sound by " + juce::String (saturation.matchGainDb.load(), 1) + " dB right now.");
            addSection (text, {}, sattext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [in, added] = history.getLoudest();

            if (in > -70.0f && added > -90.0f)
                addLead (text, "The last 4 seconds: the track up to " + juce::String (in, 1) + " dB, what the saturation adds up to "
                                   + juce::String (added, 1) + " dB.");
            else
                addLead (text, "Play the track, or click Play a note, to see it here.");

            addSection (text, {}, sattext::describe (topic));
            break;
        }

        default:
        {
            const auto lead = sattext::describeSettings (topic, s, peak);

            if (lead.isNotEmpty())
                addLead (text, lead);

            addSection (text, {}, sattext::describe (topic));
            break;
        }
    }

    return text;
}
} // namespace daw
