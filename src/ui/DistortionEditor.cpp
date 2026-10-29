#include "DistortionEditor.h"

namespace daw
{
using dist::Knob;
using namespace fxui;

//==============================================================================
DistortionCurveView::DistortionCurveView (BuiltInDistortion& p) : processor (p)
{
    setTopic (*this, disttext::curve);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void DistortionCurveView::paint (juce::Graphics& g)
{
    const auto s = processor.getSettings();
    const float peak = disttext::getShownPeak (processor.recentPeak.load());
    drawPanel (g, getLocalBounds().toFloat());

    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE CURVE: IN ACROSS, OUT UP", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    auto wave = r.removeFromBottom (juce::jmin (90.0f, r.getHeight() * 0.25f));
    disttext::drawCurve (g, r.reduced (4.0f), s, peak, true);

    g.setFont (juce::FontOptions (10.5f));
    g.setColour (theme::textDim);
    g.drawText ("the test note at the track's level: before, and after as loud", wave.removeFromTop (14.0f), juce::Justification::centredLeft);
    disttext::drawBeforeAfter (g, wave, s, peak, 2.5);
}

void DistortionCurveView::mouseDown (const juce::MouseEvent&)
{
    dragStartDrive = processor.getSettings().driveDb;

    if (auto* p = processor.getKnobParameter (Knob::drive))
        p->beginChangeGesture();
}

void DistortionCurveView::mouseDrag (const juce::MouseEvent& e)
{
    const float drive = dragStartDrive - (float) e.getDistanceFromDragStartY() * 0.15f;
    processor.setKnob (Knob::drive, juce::jlimit (0.0f, dist::maxDrive, std::round (drive * 10.0f) / 10.0f));
    repaint();
}

void DistortionCurveView::mouseUp (const juce::MouseEvent&)
{
    if (auto* p = processor.getKnobParameter (Knob::drive))
        p->endChangeGesture();
}

void DistortionCurveView::mouseDoubleClick (const juce::MouseEvent&)
{
    processor.setKnob (Knob::drive, dist::Settings().driveDb);
}

//==============================================================================
/** The harmonics the test note at the track's level ends up with. */
class BuiltInDistortionEditor::Harmonics final : public juce::Component
{
public:
    explicit Harmonics (BuiltInDistortion& p) : processor (p)
    {
        setTopic (*this, disttext::harmonics);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("WHAT A NOTE TURNS INTO", r.removeFromTop (16.0f), juce::Justification::centredLeft);
        disttext::drawHarmonics (g, r, processor.getSettings(), disttext::getShownPeak (processor.recentPeak.load()), 12, true);
    }

private:
    BuiltInDistortion& processor;
};

//==============================================================================
/** The tight low cut before the curve, and the tone and speaker after it. */
class BuiltInDistortionEditor::Filters final : public juce::Component
{
public:
    explicit Filters (BuiltInDistortion& p) : processor (p)
    {
        setTopic (*this, disttext::filters);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        disttext::drawFilters (g, getLocalBounds().toFloat().reduced (10.0f, 8.0f), processor.getSettings(), true);
    }

private:
    BuiltInDistortion& processor;
};

//==============================================================================
BuiltInDistortionEditor::BuiltInDistortionEditor (BuiltInDistortion& p)
    : EffectEditor (p, "DawIt Distortion", "What is distortion?", "Hear only what it adds", p.listenToEffect, p.clap, disttext::colour,
                    fx::Clap::Sound::note),
      distortion (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "what it adds", disttext::colour)
{
    harmonics = std::make_unique<Harmonics> (distortion);
    filters = std::make_unique<Filters> (distortion);
    addAndMakeVisible (curve);
    addAndMakeVisible (*harmonics);
    addAndMakeVisible (*filters);
    addAndMakeVisible (history);

    const std::pair<Knob, const char*> names[] = { { Knob::tight, "Tight" }, { Knob::drive, "Drive" }, { Knob::tone, "Tone" },
                                                   { Knob::output, "Output" }, { Knob::mix, "Mix" } };

    for (auto& [knob, name] : names)
        knobs.add (new EffectKnob (distortion.getParameters(), BuiltInDistortion::getParameterId (knob), name, (int) knob, disttext::colour));

    typeSwitch.button.onClick = [this]
    {
        const int next = ((int) distortion.getSettings().type + 1) % dist::numTypes;
        distortion.setKnob (Knob::type, (float) next);
    };

    speakerSwitch.button.onClick = [this] { distortion.setKnob (Knob::speaker, distortion.getSettings().speaker ? 0.0f : 1.0f); };
    matchSwitch.button.onClick = [this] { distortion.setMatchingLevel (! distortion.isMatchingLevel()); };

    for (auto* button : { &speakerSwitch.button, &matchSwitch.button })
        button->setColour (juce::TextButton::buttonOnColourId, disttext::colour.darker (0.4f));

    setGroups ({ { "BEFORE", { knobs[0] } }, { "HOW MUCH", { knobs[1] } }, { "WHAT KIND", { &typeSwitch } }, { "AFTER", { knobs[2], &speakerSwitch } },
                 { "LEVEL", { knobs[3], &matchSwitch } }, { "BLEND", { knobs[4] } } });

    juce::StringArray presetNames;

    for (auto& preset : dist::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInDistortionEditor::applyPreset (int index)
{
    const auto& list = dist::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    distortion.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInDistortionEditor::createGuidePages()
{
    return disttext::makeGuide (distortion);
}

void BuiltInDistortionEditor::layoutPictures (juce::Rectangle<int> r)
{
    curve.setBounds (r.removeFromLeft (juce::jmin (340, r.getHeight() - 20)));
    r.removeFromLeft (10);
    harmonics->setBounds (r.removeFromTop (r.getHeight() * 11 / 20));
    r.removeFromTop (10);
    filters->setBounds (r.removeFromLeft (r.getWidth() * 2 / 5));
    r.removeFromLeft (10);
    history.setBounds (r);
}

void BuiltInDistortionEditor::tick()
{
    const auto s = distortion.getSettings();
    typeSwitch.button.setButtonText (dist::getTypeName (s.type));
    speakerSwitch.button.setButtonText (s.speaker ? "On" : "Off");
    speakerSwitch.button.setToggleState (s.speaker, juce::dontSendNotification);
    matchSwitch.button.setButtonText (distortion.isMatchingLevel() ? "On" : "Off");
    matchSwitch.button.setToggleState (distortion.isMatchingLevel(), juce::dontSendNotification);

    history.update();
    curve.repaint();
    harmonics->repaint();
    filters->repaint();
}

juce::AttributedString BuiltInDistortionEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = distortion.getSettings();
    const float peak = disttext::getShownPeak (distortion.recentPeak.load());
    const auto& list = dist::getPresets();
    const dist::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, disttext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear, with match level on.");

        for (int k = 0; k < dist::numKnobs; ++k)
            addSection (text, disttext::getTitle (k).upToFirstOccurrenceOf (":", false, false) + ": " + dist::formatKnob ((Knob) k, preset->settings),
                        preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < dist::numKnobs)
    {
        addTitle (text, disttext::getTitle (topic), disttext::colour);
        auto lead = disttext::describeSettings (topic, s, peak);

        if (topic == (int) Knob::output && distortion.isMatchingLevel())
            lead << " Match level adds " << juce::String (distortion.matchGainDb.load(), 1) << " dB right now.";

        addLead (text, lead);
        addSection (text, "What it does", disttext::describe (topic));
        addSection (text, "When and how it's set", disttext::getTypical (topic));

        if (preset != nullptr)
            addSection (text, "In the " + preset->name + " starting point: " + dist::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);
        else
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that sound.");

        return text;
    }

    addTitle (text, disttext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "Distortion drives a sound far past the point where it fits, so most of its wave is cut or folded: a growl, "
                           "a crunch, a buzz or a fuzz. It's saturation pushed all the way, modelled on guitar pedals and amps.");
            addSection (text, "When to use it", "When a part should sound distorted: rock and metal guitars, a growling bass, smashed "
                                                "drums blended under the clean ones, a telephone or megaphone voice, a synth or 808 "
                                                "that needs bite, or an effect on a break. For warmth you shouldn't hear as "
                                                "distortion, use DawIt Saturation instead.");
            addSection (text, "How it works", "Tight cuts the lows before the curve, so they don't turn to mud. Drive pushes the sound "
                                              "into the curve, and the Type is its shape: overdrive, distortion, fuzz, rectify (an "
                                              "octave up) or fold. Tone and the speaker take away the fizz after it. Output and match "
                                              "level set the level, and Mix blends the distorted sound with the clean one.");
            addSection (text, "How it's usually set", "Guitars: tight 100-300 Hz, the speaker on, as much drive as the style needs and "
                                                      "no more. Bass and drums: in parallel (mix 20-50%), the speaker off. Vocals: tight "
                                                      "and tone close together for a telephone. Put it before delay and reverb, and "
                                                      "compare with match level on.");
            addSection (text, "Reading the window", "Left, the curve, with the part the test note at the track's level uses drawn thick, "
                                                    "and that note before and after it. Right, the harmonics it ends up with; under them "
                                                    "the filters before and after the curve, and the last 4 seconds. Hover anything to "
                                                    "read about it here; scroll here when the text runs long.");
            addSection (text, "Getting started", "Pick a starting point; each tells why every knob is set the way it is. Click Play a "
                                                 "note to hear one plain note through it, even while the song is stopped. Read What is "
                                                 "distortion? for how it works and when it's used.");
            break;

        case CommonTopic::listen:
            addLead (text, distortion.listenToEffect.load() ? "On: you hear only what the distortion adds." : "Off: you hear the distorted track.");
            addSection (text, {}, disttext::describe (topic));
            break;

        case disttext::match:
            addLead (text, distortion.isMatchingLevel() ? "On: it turns the sound by " + juce::String (distortion.matchGainDb.load(), 1) + " dB right now."
                                                        : "Off. Turned on, it would turn the sound by " + juce::String (distortion.matchGainDb.load(), 1) + " dB right now.");
            addSection (text, {}, disttext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [in, added] = history.getLoudest();

            if (in > -70.0f && added > -90.0f)
                addLead (text, "The last 4 seconds: the track up to " + juce::String (in, 1) + " dB, what the distortion adds up to "
                                   + juce::String (added, 1) + " dB.");
            else
                addLead (text, "Play the track, or click Play a note, to see it here.");

            addSection (text, {}, disttext::describe (topic));
            break;
        }

        default:
        {
            const auto lead = disttext::describeSettings (topic, s, peak);

            if (lead.isNotEmpty())
                addLead (text, lead);

            addSection (text, {}, disttext::describe (topic));
            break;
        }
    }

    return text;
}
} // namespace daw
