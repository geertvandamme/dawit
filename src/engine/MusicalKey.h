#pragma once

#include <juce_core/juce_core.h>

namespace daw
{
/** The key of the song: a tonic pitch class plus a scale. */
struct MusicalKey
{
    enum class Scale
    {
        major,
        minor,          // natural minor (aeolian)
        dorian,
        phrygian,
        lydian,
        mixolydian,
        locrian,
        harmonicMinor,
        melodicMinor
    };

    static constexpr int numScales = 9;

    int tonic = 0;                  // pitch class: 0 = C, 1 = C#/Db ... 11 = B
    Scale scale = Scale::major;

    bool operator== (const MusicalKey&) const = default;

    /** Semitone offsets from the tonic of each scale degree. */
    static juce::Array<int> getIntervals (Scale s)
    {
        switch (s)
        {
            case Scale::major:         return { 0, 2, 4, 5, 7, 9, 11 };
            case Scale::minor:         return { 0, 2, 3, 5, 7, 8, 10 };
            case Scale::dorian:        return { 0, 2, 3, 5, 7, 9, 10 };
            case Scale::phrygian:      return { 0, 1, 3, 5, 7, 8, 10 };
            case Scale::lydian:        return { 0, 2, 4, 6, 7, 9, 11 };
            case Scale::mixolydian:    return { 0, 2, 4, 5, 7, 9, 10 };
            case Scale::locrian:       return { 0, 1, 3, 5, 6, 8, 10 };
            case Scale::harmonicMinor: return { 0, 2, 3, 5, 7, 8, 11 };
            case Scale::melodicMinor:  return { 0, 2, 3, 5, 7, 9, 11 };
        }

        return {};
    }

    static juce::String getScaleName (Scale s)
    {
        switch (s)
        {
            case Scale::major:         return "major";
            case Scale::minor:         return "minor";
            case Scale::dorian:        return "dorian";
            case Scale::phrygian:      return "phrygian";
            case Scale::lydian:        return "lydian";
            case Scale::mixolydian:    return "mixolydian";
            case Scale::locrian:       return "locrian";
            case Scale::harmonicMinor: return "harmonic minor";
            case Scale::melodicMinor:  return "melodic minor";
        }

        return {};
    }

    static juce::String getTonicName (int pitchClass)
    {
        static const char* names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
        return names[((pitchClass % 12) + 12) % 12];
    }

    juce::Array<int> getIntervals() const { return getIntervals (scale); }

    /** e.g. "A minor" */
    juce::String getName() const { return getTonicName (tonic) + " " + getScaleName (scale); }

    /** True if the MIDI note's pitch class belongs to this key. */
    bool contains (int midiNote) const
    {
        return getIntervals().contains (((midiNote - tonic) % 12 + 12) % 12);
    }
};
} // namespace daw
