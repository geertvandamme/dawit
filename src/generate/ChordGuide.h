#pragma once

#include "Harmony.h"

namespace daw::gen
{
/** Plain-language explanations of chords and chord progressions in a key, written for beginning
    musicians: chords belong to a family (home, away or tension), and progressions are journeys
    away from home and back. Used by the chord progression explainer. */
namespace guide
{
    /** How a chord feels in the key: home (rest), away (moving off), tension (wants to go home),
        or outside the key. */
    enum class Family { home, away, tension, outside };

    juce::String getFamilyName (Family);   // "Home", "Away", "Tension", "Outside the key"

    /** The family of a chord on a degree (1-7) of the key; 0 is outside. */
    Family getFamily (int degree);

    /** The family of any chord: by its root's degree, or outside when the root isn't in the key. */
    Family getFamily (const MusicalKey&, const Chord&);

    /** Roman numeral of any chord in the key, e.g. "vi" for Am in C major, or "bVII" for Bb in C major. */
    juce::String getNumeral (const MusicalKey&, const Chord&);

    /** One chord of a progression: a degree of the key and how to build it (see getChordIntervals()). */
    struct Step
    {
        int degree = 1;
        juce::String colour = "triad";
    };

    Chord buildChord (const MusicalKey&, const Step&);

    struct Progression
    {
        juce::String name;      // e.g. "Pop progression"
        juce::String summary;   // one line, e.g. "The most used progression in pop music."
        juce::String story;     // why it works, in a few sentences
        std::vector<Step> steps;

        std::vector<Chord> getChords (const MusicalKey&) const;
    };

    /** Well-known progressions that suit the key: major ones for keys with a major home chord, minor ones otherwise. */
    std::vector<Progression> getCommonProgressions (const MusicalKey&);

    /** How chords, numerals and families work, with this key's chords as examples. */
    juce::String getIntroduction (const MusicalKey&);

    /** What a chord is in the key: its notes, its sound, its family and role, and the chords that usually follow. */
    juce::String explainChord (const MusicalKey&, const Chord&);

    /** One sentence on moving from one chord to the next, e.g. why V to I sounds like an ending. */
    juce::String explainMove (const MusicalKey&, const Chord& from, const Chord& to);

    /** A progression's story, then each move between its chords. */
    juce::String explainProgression (const MusicalKey&, const Progression&);

    /** The chords of the song's chord track in order (repeats merged): what they are in the key, whether they
        match a well-known progression, each move, and a few observations. */
    juce::String explainChordTrack (const MusicalKey&, const std::vector<Chord>&);

    /** The key's place on the circle of fifths, 0 to 11 clockwise from C (0 = C, 1 = G ... 11 = F): the place of
        the major key with the same notes. Minor keys and modes sit at their relative major's place; harmonic and
        melodic minor at the natural minor's. */
    int getCirclePosition (const MusicalKey&);

    /** The tonic (pitch class) of the major key at a place on the circle. */
    int getCircleTonic (int position);

    /** How a key is written on the circle: "D" for the major key, "Bm" for the minor key at the same place. */
    juce::String getCircleLabel (int position, bool minor);

    /** The sharps or flats of the key signature at a place on the circle, e.g. "F#", "C#" for D major or "Bb" for
        F major; empty for C major. F# major (6 sharps) and Gb major (6 flats) share a place: sharps are given. */
    juce::StringArray getKeySignature (int position);

    /** "no sharps or flats", "1 sharp", "3 flats" or "6 sharps or 6 flats". */
    juce::String describeKeySignature (int position);

    /** A chain of fifths down to the home chord: iii vi ii V I in a major key, III VI ii° V i in a minor key (with
        the major V). */
    std::vector<Chord> getCircleProgression (const MusicalKey&);

    /** What the circle of fifths is and how to use it, with this key as the example. */
    juce::String explainCircleOfFifths (const MusicalKey&);

    /** The name of the well-known progression these chords are (also when they start on another of its chords),
        or empty. */
    juce::String findProgressionName (const MusicalKey&, const std::vector<Chord>&);
} // namespace guide
} // namespace daw::gen
