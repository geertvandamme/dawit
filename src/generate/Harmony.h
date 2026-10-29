#pragma once

#include "NoteRealizer.h"

namespace daw::gen
{
/** Music-theory helpers for suggesting chords in a key. Degrees are 1-7. */
namespace harmony
{
    /** True for the scales whose tonic chord is minor (minor, dorian, phrygian, harmonic and melodic minor, locrian). */
    inline bool isMinorKey (const MusicalKey& key)
    {
        return key.getIntervals()[2] == 3;
    }

    /** The in-key triad on a degree. */
    inline Chord triad (const MusicalKey& key, int degree)
    {
        return { (key.tonic + key.getIntervals()[degree - 1]) % 12, getChordIntervals (key, degree, "triad") };
    }

    /** Roman numeral of a chord on a degree: upper case for major, lower for minor, ° for diminished, + for augmented. */
    inline juce::String getRomanNumeral (int degree, const Chord& chord)
    {
        static const char* numerals[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        const juce::String numeral = numerals[juce::jlimit (1, 7, degree) - 1];
        const bool minor3 = chord.intervals.contains (3), dim = minor3 && chord.intervals.contains (6) && ! chord.intervals.contains (7);
        const bool aug = chord.intervals.contains (4) && chord.intervals.contains (8) && ! chord.intervals.contains (7);

        return (minor3 ? numeral.toLowerCase() : numeral) + (dim ? juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")) : aug ? "+" : "");
    }

    /** Harmonic function of a degree: tonic (rest), subdominant (moving away) or dominant (tension that resolves). */
    inline juce::String getFunction (int degree)
    {
        switch (degree)
        {
            case 1:  return "tonic";
            case 3:
            case 6:  return "tonic substitute";
            case 2:
            case 4:  return "subdominant";
            case 5:
            case 7:  return "dominant";
            default: return {};
        }
    }

    /** The degree of the key whose note is the chord's root, or 0 if the root isn't in the key. */
    inline int findDegree (const MusicalKey& key, const Chord& chord)
    {
        const auto intervals = key.getIntervals();
        const int offset = ((chord.root - key.tonic) % 12 + 12) % 12;
        return intervals.indexOf (offset) + 1;
    }

    /** Degrees that commonly follow a chord on `degree`, most usual first (common-practice and pop
        progressions; minor keys favour the moves of natural minor). */
    inline juce::Array<int> getUsualNextDegrees (const MusicalKey& key, int degree)
    {
        if (isMinorKey (key))
        {
            switch (degree)
            {
                case 1:  return { 4, 6, 7, 5 };
                case 2:  return { 5, 1 };
                case 3:  return { 6, 4, 7 };
                case 4:  return { 5, 7, 1 };
                case 5:  return { 1, 6 };
                case 6:  return { 7, 3, 4 };
                case 7:  return { 3, 1, 6 };
                default: return {};
            }
        }

        switch (degree)
        {
            case 1:  return { 4, 5, 6, 2 };
            case 2:  return { 5, 4 };
            case 3:  return { 6, 4 };
            case 4:  return { 5, 1, 2 };
            case 5:  return { 1, 6, 4 };
            case 6:  return { 4, 2, 5 };
            case 7:  return { 1, 3 };
            default: return {};
        }
    }

    /** In a minor key the dominant is usually made major (from harmonic minor), e.g. E in A minor. */
    inline std::optional<Chord> getBorrowedDominant (const MusicalKey& key)
    {
        const auto five = triad (key, 5);

        if (! isMinorKey (key) || five.intervals.contains (4))
            return std::nullopt;

        return Chord { five.root, getChordIntervals (key, 5, "borrowed_major") };
    }
} // namespace harmony
} // namespace daw::gen
