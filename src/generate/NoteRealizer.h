#pragma once

#include "engine/Clip.h"
#include "GenerationRequest.h"

namespace daw::gen
{
struct Result
{
    std::vector<MidiNote> notes;   // relative to the clip start, sorted by start
    double lengthBeats = 0.0;      // request.bars * 4
    juce::String summary;          // e.g. "Am - F - C - G" or "32 notes"

    /** Chords the model wrote for the bars of the clip that have none on the chord track
        (one entry per bar, nullopt where the chord track already has one). Empty for drums and percussion. */
    std::vector<std::optional<Chord>> newChords;
};

/** Turns the model's JSON reply into notes: scale degrees become pitches in the request's key,
    chords (the model's, or the chord track's under the model's rhythm) are voiced with smooth voice leading, drums and percussion map to General MIDI, timing is quantised
    and the pattern is repeated to fill the requested number of bars.
    Returns false (with an error message) if the reply can't be used. */
bool realize (const Request&, const juce::String& modelReply, Result&, juce::String& error);

/** Semitone offsets from the root for a chord on a scale degree (1-7). The chord is built from the
    key's own notes, so "triad" on degree 2 of A minor is B D F; colours add or swap notes
    (see getChordColours()). */
juce::Array<int> getChordIntervals (const MusicalKey&, int degree, const juce::String& colour);

/** Voices a chord (its pitch classes) between `low` and `high`, picking the inversion and octave
    closest to the previous voicing; with no previous one, centred an octave above `low`.
    Empty if it doesn't fit. */
std::vector<int> voiceChord (const juce::Array<int>& pitchClasses, const std::vector<int>& previous, int low = 50, int high = 79);

/** Chord symbol from its root and intervals, e.g. (9, {0, 3, 7, 10}) -> "Am7". */
juce::String getChordName (int rootPitchClass, const juce::Array<int>& intervals);
} // namespace daw::gen
