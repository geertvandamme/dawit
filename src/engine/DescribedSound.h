#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace daw
{
/** One knob (or one harmonic bar) an answer to "describe a sound" changes. `knob` is the index in the synth's own
    list of knobs (DawIt Subtractive: sub::Knob; DawIt Additive: its 17 knobs, then the 32 bars); the values are in
    the knob's own units, as that synth's getKnobValue gives them. */
struct DescribedChange
{
    int knob = 0;
    float before = 0.0f, after = 0.0f;
    juce::String reason;   // the model's words
    juce::String note;     // DawIt's own: "The model asked for 40 kHz; the knob stops at 20 kHz."
};

/** The last answer to a description of a sound, kept while the app runs (not saved): the words, the starting
    point the changes are measured from, and the changes. Plain data, safe in the plugin builds; any synth that
    has a describe table (`gen::sounddescribe`) keeps one. */
struct DescribedSound
{
    juce::String words, base, model, summary;
    bool followUp = false;               // the base is the sound in the window, not a starting point
    std::vector<float> baseValues;       // the sound the changes start from, one value per knob
    std::vector<DescribedChange> changes;
    juce::StringArray notes;             // DawIt's notes on the words, limits and clamped values
    int made = 0;                        // changes applied so far (a group of bars counts as one)

    bool isEmpty() const { return words.isEmpty(); }
};
} // namespace daw
