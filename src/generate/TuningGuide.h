#pragma once

#include <vector>
#include <juce_core/juce_core.h>
#include "engine/MusicalKey.h"

namespace daw::gen
{
/**
    The "How to tune a guitar" guide and what the Tuner window needs from theory: the tunings, a reading worked out from
    a pitch (note, cents, string, colour and the words that go with it), the guide's topics and example data, and the
    "Your tuning and your song" advice. The content only (no UI): the windows are TunerWindow and TuningExplainer.
    The facts, texts and numbers are checked in the tests.
*/
namespace tuning
{
    /** The tunings in the Tuner's menu. Chromatic has no strings: the tuner names any note. */
    enum class Tuning { guitarStandard, dropD, dadgad, halfStepDown, bass4, bass5, ukulele, chromatic };
    inline constexpr Tuning allTunings[] { Tuning::guitarStandard, Tuning::dropD, Tuning::dadgad, Tuning::halfStepDown,
                                           Tuning::bass4, Tuning::bass5, Tuning::ukulele, Tuning::chromatic };

    /** "Guitar: standard", "Drop D"... as the menu shows them. */
    juce::String getTuningName (Tuning);

    /** A short fixed word to save in the settings ("guitarStandard"), and back (standard when unknown). */
    juce::String getTuningId (Tuning);
    Tuning getTuningFromId (const juce::String&);

    /** True for the guitar tunings and the ukulele (the ones a capo suits). */
    bool isFrettedTuning (Tuning);
    bool isBassTuning (Tuning);

    /** One string of a tuning. */
    struct TuningString
    {
        int midi = 0;          // 69 = A4
        juce::String note;     // "E2", "Db3"
        juce::String label;    // "low E", "A", "D3", "high E": see getStrings()
    };

    /** The strings of a tuning, in the order of the buttons (lowest first; the ukulele starts with its high G). Where a
        note repeats, the lowest copy is "low", the highest "high" and any other gets its octave ("D3"). */
    std::vector<TuningString> getStrings (Tuning);

    /** The pitch of a MIDI note at a reference A4 (440 Hz unless given). */
    double getHz (double midi, double referenceHz = 440.0);

    /** "A2", "Eb3", "C#4": the note of a MIDI number (flats as the song key names them), middle C = C4 = 60. */
    juce::String getNoteName (int midi);

    /** The note name without its octave. */
    juce::String getNoteLetter (int midi);

    // ---- Readings ---------------------------------------------------------------------------------------------

    /** Green within this many cents, yellow within the second, red beyond (DawIt's choice). */
    inline constexpr double inTuneCents = 3.0;
    inline constexpr double closeCents = 10.0;

    enum class Zone { inTune, close, out };

    /** The colour band of a distance in cents (either sign). 3.0 is in tune; 10.0 is close. */
    Zone getZone (double cents);

    /** What a note that is `cents` away needs, in words: "in tune"; up to 15 cents "12 cents flat, tighten it a little";
        up to 50 without "a little"; beyond 50 "2 semitones low, tighten it" (whole semitones). Negative is flat. */
    juce::String describeCents (double cents);

    /** The same without the advice: "12 cents flat", "in tune", "2 semitones low". */
    juce::String describeCentsShort (double cents);

    /** What a pitch is, measured against the tuning. */
    struct Reading
    {
        int midi = 0;               // the nearest note
        juce::String note;          // "A2" (the string's own spelling when it is that string)
        double cents = 0.0;         // from the target: the locked string, or the nearest note (then -50 to +50)
        int stringIndex = -1;       // the string the reading is about: the locked one, or the nearest within 50 cents; -1 for none
        bool locked = false;
        Zone zone = Zone::out;
        juce::String stringName;    // "A string", "low E string"; empty when no string
        juce::String words;         // "12 cents flat, tighten it a little"
        juce::String text;          // "A string: 12 cents flat, tighten it a little"
    };

    /** Works out a reading for a pitch in Hz. `lockedString` is the index of a string to measure against, or -1.
        The nearest string is the one nearest in cents, the lower on a tie. */
    Reading getReading (double hz, double referenceHz, Tuning, int lockedString = -1);

    /** The index of the string nearest to a fractional MIDI note, the lower one on a tie; -1 for Chromatic. */
    int getNearestString (double midi, Tuning);

    /** The distance from `baseHz` to `hz` in cents: 1200 log2 (hz / baseHz). */
    double getCents (double hz, double baseHz);

    // ---- Formulas of the guide --------------------------------------------------------------------------------

    /** Beats a second of two steady notes: the difference in Hz. 440 and 442: 2. */
    double getBeatsPerSecond (double hz1, double hz2);

    /** How many Hz a pitch moves for `cents`: hz x (2^(cents/1200) - 1). 5 cents at 110 Hz: 0.318. */
    double getHzForCents (double hz, double cents);

    /** The change in pitch for a change in tension of `percent` (pitch goes with the square root): 1% is 8.61 cents. */
    double getCentsForTensionPercent (double percent);

    /** Cents the reference is from 440 Hz. 442: 7.85. 432: -31.77. */
    double getReferenceCents (double referenceHz);

    /** Each of A, D, G and B's cumulative error in cents (all flat, so negative) when tuned by matching harmonics:
        -1.955, -3.910, -5.865 and -19.551 (the B from the G's 4th-fret harmonic, a pure major third 13.686 cents small). */
    std::vector<double> getHarmonicTuningErrors();

    /** How many vibrations of a note fit in `ms` of listening: ms x hz / 1000. 85.33 ms and 41.2 Hz: 3.5. */
    double getCyclesInWindow (double hz, double ms);

    // ---- Topics -----------------------------------------------------------------------------------------------

    enum class Topic { whatItShows, centsAndBeats, tunings, reference, howToTune, drift, byEar, harmonics, bassAndUkulele, yourTuning };
    inline constexpr Topic allTopics[] { Topic::whatItShows, Topic::centsAndBeats, Topic::tunings, Topic::reference, Topic::howToTune,
                                         Topic::drift, Topic::byEar, Topic::harmonics, Topic::bassAndUkulele, Topic::yourTuning };

    juce::String getTopicName (Topic);
    juce::String getTopicSummary (Topic);

    /** The text of a topic. For "Your tuning and your song" this is the part that doesn't depend on the song. */
    juce::String explain (Topic);

    /** One line saying what the topic's picture shows. */
    juce::String getExampleCaption (Topic);

    // ---- Example data for the pictures --------------------------------------------------------------------------

    /** Topic 1: A2 at 109.24 Hz, 12 cents flat. */
    inline constexpr double exampleHz = 109.2390;
    inline constexpr double exampleCents = -12.0;

    /** Topic 2: the swell of 440 plus 442 Hz at time `t` seconds: |2 cos (pi x 2 x t)|, loudest at 0, 0.5, 1.0 and 1.5 s. */
    double getBeatEnvelope (double t);

    /** Topic 5: the pitch of a string being tuned, in cents, at second `t` (0 to 6): 30 sharp, loosened to 20 flat, then up to 0. */
    double getTuningPath (double t);

    /** Topic 6: the glide after a hard pluck, 6 e^(-t/0.3) cents at second `t` (0 to 2): drawn to show the shape, not measured. */
    double getDriftGlide (double t);

    /** Topic 7: the fret on each string to match to the next open string: 5, 5, 5, 4, 5 for a guitar. */
    std::vector<int> getFifthFretSteps();

    /** Topic 8: the harmonic frequencies (2, 3 and 4 times) of a string at `hz`; the nodes are at 1/2, 1/3 and 1/4 of its length. */
    std::vector<double> getHarmonicHz (double hz);

    // ---- Your tuning and your song ----------------------------------------------------------------------------

    /** A MIDI track, for the low-notes rule: its name and its lowest note (-1 when it has none). */
    struct TrackLow
    {
        juce::String name;
        int lowestMidi = -1;
    };

    /** How far a string was off the last time the tuner heard it. */
    struct LastReading
    {
        int stringIndex = 0;
        double cents = 0.0;
    };

    /** How many of a tuning's open strings are in the key, and which (indices). */
    struct KeyFit
    {
        std::vector<int> inKey, outOfKey;
    };
    KeyFit getKeyFit (Tuning, const MusicalKey&, int capoFret = 0);

    /** The lowest fret, 1 to 5, at which a capo puts the most open strings in the key. */
    int getBestCapoFret (Tuning, const MusicalKey&);

    /** The advice, in the order of the rules: key fit, capo, low notes, reference, last readings. One paragraph each; the
        silent rules add nothing. */
    juce::StringArray analyse (Tuning, double referenceHz, const MusicalKey&, const std::vector<TrackLow>&, const std::vector<LastReading>&);
}
}
