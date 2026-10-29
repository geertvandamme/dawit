#include "SubtractiveEditor.h"

namespace daw
{
using sub::Knob;
using namespace fxui;

namespace
{
    /** The cutoff the pictures show: the newest note's, or where the filter rests for the test note. */
    double getShownCutoff (const BuiltInSubtractive& p)
    {
        return p.noteAge.load() >= 0.0f ? (double) p.shownCutoffHz.load() : sub::getCutoffAt (p.getSettings(), sub::testNote, 1.0f, 0.0f, 0.0f);
    }

    void drawHeading (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (text, r.removeFromTop (16.0f), juce::Justification::centredLeft);
    }
}

//==============================================================================
SubtractiveSpectrumView::SubtractiveSpectrumView (BuiltInSubtractive& p) : processor (p)
{
    setTopic (*this, subtext::spectrum);
    setMouseCursor (juce::MouseCursor::UpDownLeftRightResizeCursor);
}

void SubtractiveSpectrumView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    drawHeading (g, r, "THE HARMONICS OF A3: GREY MADE, COLOURED LEFT AFTER THE FILTER");
    subtext::drawSpectrum (g, r, processor.getSettings(), sub::testNoteHz, getShownCutoff (processor), true);
}

void SubtractiveSpectrumView::mouseDown (const juce::MouseEvent&)
{
    const auto s = processor.getSettings();
    dragStartCutoff = s.cutoffHz;
    dragStartResonance = s.resonancePercent;

    for (auto k : { Knob::cutoff, Knob::resonance })
        if (auto* p = processor.getKnobParameter (k))
            p->beginChangeGesture();
}

void SubtractiveSpectrumView::mouseDrag (const juce::MouseEvent& e)
{
    // Across the width is the picture's three decades; up and down 200 pixels is the whole resonance.
    const float decades = (float) e.getDistanceFromDragStartX() / juce::jmax (1.0f, (float) getWidth() - 54.0f) * 3.0f;
    processor.setKnob (Knob::cutoff, juce::jlimit (sub::minCutoff, sub::maxCutoff, dragStartCutoff * std::pow (10.0f, decades)));
    processor.setKnob (Knob::resonance, juce::jlimit (0.0f, 100.0f, std::round (dragStartResonance - (float) e.getDistanceFromDragStartY() * 0.5f)));
    repaint();
}

void SubtractiveSpectrumView::mouseUp (const juce::MouseEvent&)
{
    for (auto k : { Knob::cutoff, Knob::resonance })
        if (auto* p = processor.getKnobParameter (k))
            p->endChangeGesture();
}

void SubtractiveSpectrumView::mouseDoubleClick (const juce::MouseEvent&)
{
    processor.setKnob (Knob::cutoff, sub::maxCutoff);
    processor.setKnob (Knob::resonance, 0.0f);
}

//==============================================================================
/** A few cycles of the test note, before and after the filter. */
class BuiltInSubtractiveEditor::Wave final : public juce::Component
{
public:
    explicit Wave (BuiltInSubtractive& p) : processor (p)
    {
        setTopic (*this, subtext::wave);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "THE WAVE: BEFORE AND AFTER THE FILTER");
        subtext::drawWave (g, r, processor.getSettings(), sub::testNoteHz, getShownCutoff (processor), 3.0);
    }

private:
    BuiltInSubtractive& processor;
};

//==============================================================================
/** The filter and amp envelopes, with the newest note on them. */
class BuiltInSubtractiveEditor::Envelopes final : public juce::Component
{
public:
    explicit Envelopes (BuiltInSubtractive& p) : processor (p)
    {
        setTopic (*this, subtext::envelopes);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "THE ENVELOPES");
        const auto s = processor.getSettings();
        const double age = processor.noteAge.load(), releasedAt = processor.releaseAge.load();
        auto left = r.removeFromLeft (r.getWidth() * 0.5f - 6.0f);
        r.removeFromLeft (12.0f);
        subtext::drawEnvelope (g, left, s.filterEnv, subtext::filterColour,
                               "Filter " + juce::String (std::abs (s.envOctaves) < 0.05f ? "(amount 0: off)" : sub::formatKnob (Knob::envAmount, s)),
                               age, releasedAt, true);
        subtext::drawEnvelope (g, r, s.ampEnv, subtext::ampColour, "Amp", age, releasedAt, true);
    }

private:
    BuiltInSubtractive& processor;
};

//==============================================================================
BuiltInSubtractiveEditor::BuiltInSubtractiveEditor (BuiltInSubtractive& p)
    : EffectEditor (p, "DawIt Subtractive", "What is subtractive synthesis?", "Hear it without the filter", p.withoutFilter, p.clap,
                    subtext::colour, fx::Clap::Sound::note),
      synth (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "after the filter and amp", subtext::colour, "the oscillators", {})
{
    wave = std::make_unique<Wave> (synth);
    envelopes = std::make_unique<Envelopes> (synth);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (*wave);
    addAndMakeVisible (*envelopes);
    addAndMakeVisible (history);

    for (int k = 0; k < sub::numKnobs; ++k)
    {
        const auto knob = (Knob) k;

        switch (knob)
        {
            case Knob::wave1:
            case Knob::wave2:
            case Knob::filterType:
            case Knob::slope:
            case Knob::lfoTarget:
            case Knob::mode:
                addSwitch (knob, sub::getKnobName (knob));
                break;

            case Knob::width:
            case Knob::pitch2:
            case Knob::detune:
            case Knob::mix:
            case Knob::noise:
            case Knob::cutoff:
            case Knob::resonance:
            case Knob::keyTrack:
            case Knob::envAmount:
            case Knob::filterAttack:
            case Knob::filterDecay:
            case Knob::filterSustain:
            case Knob::filterRelease:
            case Knob::ampAttack:
            case Knob::ampDecay:
            case Knob::ampSustain:
            case Knob::ampRelease:
            case Knob::lfoRate:
            case Knob::lfoDepth:
            case Knob::glide:
            case Knob::velocity:
            case Knob::output:
            {
                const auto colour = k >= (int) Knob::envAmount && k <= (int) Knob::filterRelease ? subtext::filterColour
                                  : k >= (int) Knob::ampAttack && k <= (int) Knob::ampRelease ? subtext::ampColour
                                  : k >= (int) Knob::lfoRate && k <= (int) Knob::lfoTarget ? subtext::lfoColour
                                  : k >= (int) Knob::cutoff && k <= (int) Knob::keyTrack ? subtext::filterColour
                                  : k >= (int) Knob::pitch2 && k <= (int) Knob::detune ? subtext::secondColour
                                  : subtext::colour;
                knobs[knob] = std::make_unique<EffectKnob> (synth.getParameters(), BuiltInSubtractive::getParameterId (knob),
                                                            sub::getKnobName (knob), k, colour);
                break;
            }
        }
    }

    auto knob = [this] (Knob k) -> juce::Component* { return knobs.at (k).get(); };
    auto toggle = [this] (Knob k) -> juce::Component* { return switches.at (k).get(); };

    setGroups ({ { "OSCILLATOR 1", { toggle (Knob::wave1), knob (Knob::width) }, 0 },
                 { "OSCILLATOR 2", { toggle (Knob::wave2), knob (Knob::pitch2), knob (Knob::detune) }, 0 },
                 { "MIXER", { knob (Knob::mix), knob (Knob::noise) }, 0 },
                 { "FILTER", { toggle (Knob::filterType), toggle (Knob::slope), knob (Knob::cutoff), knob (Knob::resonance), knob (Knob::keyTrack) }, 0 },
                 { "FILTER ENVELOPE", { knob (Knob::envAmount), knob (Knob::filterAttack), knob (Knob::filterDecay), knob (Knob::filterSustain),
                                        knob (Knob::filterRelease) }, 1 },
                 { "AMP ENVELOPE", { knob (Knob::ampAttack), knob (Knob::ampDecay), knob (Knob::ampSustain), knob (Knob::ampRelease) }, 1 },
                 { "LFO", { knob (Knob::lfoRate), knob (Knob::lfoDepth), toggle (Knob::lfoTarget) }, 1 },
                 { "PLAYING", { knob (Knob::glide), toggle (Knob::mode), knob (Knob::velocity), knob (Knob::output) }, 1 } });

    juce::StringArray presetNames;

    for (auto& preset : sub::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

BuiltInSubtractiveEditor::Switch& BuiltInSubtractiveEditor::addSwitch (Knob k, const juce::String& name)
{
    auto& s = *(switches[k] = std::make_unique<Switch> (name, (int) k));
    s.button.setColour (juce::TextButton::buttonColourId, subtext::colour.withAlpha (0.25f));
    s.button.onClick = [this, k]
    {
        if (auto* p = synth.getKnobParameter (k))
        {
            const int count = juce::jmax (2, (int) std::round (p->getNormalisableRange().end) + 1);
            const int next = (juce::roundToInt (p->convertFrom0to1 (p->getValue())) + 1) % count;
            synth.setKnob (k, (float) next);
        }
    };
    return s;
}

void BuiltInSubtractiveEditor::applyPreset (int index)
{
    const auto& list = sub::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    synth.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInSubtractiveEditor::createGuidePages()
{
    return subtext::makeGuide (synth);
}

void BuiltInSubtractiveEditor::layoutPictures (juce::Rectangle<int> r)
{
    auto right = r.removeFromRight (r.getWidth() * 42 / 100);
    r.removeFromRight (10);
    spectrum.setBounds (r);
    const int third = (right.getHeight() - 20) / 3;
    wave->setBounds (right.removeFromTop (third));
    right.removeFromTop (10);
    envelopes->setBounds (right.removeFromTop (third));
    right.removeFromTop (10);
    history.setBounds (right);
}

void BuiltInSubtractiveEditor::tick()
{
    const auto s = synth.getSettings();

    for (auto& [k, sw] : switches)
        sw->button.setButtonText (sub::formatKnob (k, s));

    history.update();
    spectrum.repaint();
    wave->repaint();
    envelopes->repaint();
}

juce::AttributedString BuiltInSubtractiveEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = synth.getSettings();
    const auto& list = sub::getPresets();
    const sub::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, subtext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "Play a note, then change one knob at a time and listen. The knobs not named here are as in Init.");

        for (int k = 0; k < sub::numKnobs; ++k)
            if (preset->notes[(size_t) k].isNotEmpty())
                addSection (text, subtext::getTitle (k) + ": " + sub::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < sub::numKnobs)
    {
        addTitle (text, subtext::getTitle (topic), subtext::colour);
        addLead (text, subtext::describeSettings (topic, s));
        addSection (text, "What it does", subtext::describe (topic));
        addSection (text, "How it's usually set", subtext::getTypical (topic));

        if (preset != nullptr && preset->notes[(size_t) topic].isNotEmpty())
            addSection (text, "In " + preset->name + ": " + sub::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);
        else if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why each knob is set the way it is for that sound.");

        return text;
    }

    addTitle (text, subtext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "A subtractive synthesiser: it starts with oscillators full of harmonics and takes away the ones you don't "
                           "want with a filter, then shapes each note with envelopes and keeps it moving with an LFO. The way most "
                           "synth basses, leads, pads and brass are made, since the Minimoog.");
            addSection (text, "How the sound flows", "The oscillators (and noise) make the raw sound, the mixer blends them, the filter "
                                                     "takes harmonics away, and the amp sets each note's level. The filter envelope "
                                                     "moves the cutoff during a note, the amp envelope its level, and the LFO moves "
                                                     "the pitch, the filter, the level or the pulse width over and over.");
            addSection (text, "Reading the window", "Left, the harmonics of the test note (A3) as bars, grey as the oscillators make "
                                                    "them, coloured after the filter, with the filter's curve over them: drag it left "
                                                    "and right for the cutoff, up and down for the resonance. Right, the wave before "
                                                    "and after the filter, the two envelopes with the newest note running along them, "
                                                    "and the last 4 seconds. The knobs are in the order the sound flows through them. "
                                                    "Hover anything to read about it here.");
            addSection (text, "Getting started", "Pick a starting point; each tells why its knobs are set the way they are. Click Play a "
                                                 "note, or play the track's notes or a MIDI keyboard. Click Hear it without the filter to "
                                                 "hear what the filter takes away. Read What is subtractive synthesis? for how it works "
                                                 "and how to build a sound step by step.");
            break;

        case CommonTopic::listen:
            addLead (text, synth.withoutFilter.load() ? "On: you hear the oscillators without the filter." : "Off: you hear the filtered sound.");
            addSection (text, {}, subtext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [before, after] = history.getLoudest();

            if (before > -70.0f)
                addLead (text, "The last 4 seconds: the oscillators up to " + juce::String (before, 1) + " dB, after the filter up to "
                                   + juce::String (after, 1) + " dB.");
            else
                addLead (text, "Play a note to see it here.");

            addSection (text, {}, subtext::describe (topic));
            break;
        }

        case subtext::spectrum:
        {
            const double cutoff = getShownCutoff (synth);
            addLead (text, "The filter is at " + juce::String (cutoff >= 1000.0 ? juce::String (cutoff / 1000.0, 2) + " kHz" : juce::String (juce::roundToInt (cutoff)) + " Hz")
                               + (synth.noteAge.load() >= 0.0f ? " for the note playing now." : " for A3, while nothing plays."));
            addSection (text, {}, subtext::describe (topic));
            break;
        }

        default:
            addSection (text, {}, subtext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
