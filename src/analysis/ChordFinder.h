#pragma once

#include "engine/Chord.h"
#include "engine/TempoMap.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <optional>

namespace daw::analysis
{
/**
    Finding the chords of a recording, one per bar of the song, and the key they're in, with plain
    signal processing, no model:

    1. Which notes sound: the level of each of the 12 note names (C, C#, D...), whatever the octave
       ("chroma"), about 11 times a second, separately for the harmony (65 Hz to 2.1 kHz) and the
       bass (40 to 250 Hz). Short sounds (drums) are filtered out first: only what lasts over a few
       frames counts.
    2. Per bar of the song: the notes over the bar compared with every major, minor, 7, maj7 and m7
       chord. A seventh chord has to fit clearly better than its triad; the bass note decides
       between close ones (the chord whose root it is).
    3. The key: the notes of the whole recording against the major and minor key profiles
       (Krumhansl and Kessler), with the chord it starts on and its most frequent chord deciding
       between a major key and its relative minor (C major or A minor).
    4. Again with the key known: chords of the key win a near tie, and a chord that goes on into
       the next bar wins a tie with a change.

    The bars are the song's, so the recording has to be at the song's tempo with its downbeats on
    the bar lines: Detect tempo does that.
*/
namespace chordfinder
{
    /** The analysis listens to mono audio at this rate. */
    constexpr double sampleRate = 11025.0;

    using PitchClasses = std::array<float, 12>;

    /** The level of each note name, frame by frame. */
    struct Chroma
    {
        double frameRate = 0.0;
        std::vector<PitchClasses> harmony, bass;
    };

    /** A bar's chord, or none where nothing sounds. */
    struct BarChord
    {
        int bar = 0;
        std::optional<Chord> chord;
        double score = 0.0;   // how well the notes fit it, 0 to about 1
    };

    struct Result
    {
        std::vector<BarChord> bars;
        MusicalKey key;
    };

    juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>&, double sourceSampleRate);

    Chroma findChroma (const juce::AudioBuffer<float>& mono);

    /** The chords it can name: major, minor, 7, maj7 and m7 on every root. */
    const std::vector<Chord>& getVocabulary();

    /** How well a chord fits the notes: the match of its notes with the harmony (cosine, 0 to 1),
        a little more when the bass plays its root, a little less for a seventh chord. */
    double scoreChord (const Chord&, const PitchClasses& harmony, const PitchClasses& bass);

    /** The major or minor key the notes and chords are in. */
    MusicalKey findKey (const PitchClasses& allNotes, const std::vector<Chord>& chordsInOrder);

    /** The chords on the song's bars for audio that starts at `startBeat`, through the song's tempo
        (and its changes). A bar counts when at least half of it is in the audio. */
    Result analyse (const juce::AudioBuffer<float>& mono, double startBeat, const TempoMap&);

    /** The same at one tempo all through. */
    inline Result analyse (const juce::AudioBuffer<float>& mono, double startBeat, double bpm) { return analyse (mono, startBeat, TempoMap (bpm)); }

    /** The chords as text, four bars to a line: "Am | F | C | G". A bar without a chord is "-". */
    juce::String toText (const std::vector<BarChord>&);
} // namespace chordfinder
} // namespace daw::analysis
