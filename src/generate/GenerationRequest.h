#pragma once

#include "engine/Chord.h"

namespace daw::gen
{
/** What kind of MIDI part to generate. */
enum class Part { chords, melody, bassline, arpeggio, drums, percussion };

constexpr int numParts = 6;

/** Drums and percussion are hits on the General MIDI drum map, not pitches: no harmony or chords. */
inline bool isPercussive (Part p) { return p == Part::drums || p == Part::percussion; }

inline juce::String getPartName (Part p)
{
    switch (p)
    {
        case Part::chords:   return "Chord progression";
        case Part::melody:   return "Main melody";
        case Part::bassline: return "Bassline";
        case Part::arpeggio: return "Arpeggio";
        case Part::drums:    return "Drums";
        case Part::percussion: return "Percussion";
    }

    return {};
}

inline juce::String getPartShortName (Part p)
{
    switch (p)
    {
        case Part::chords:   return "chords";
        case Part::melody:   return "melody";
        case Part::bassline: return "bass";
        case Part::arpeggio: return "arp";
        case Part::drums:    return "drums";
        case Part::percussion: return "percussion";
    }

    return {};
}

inline juce::StringArray getStylePresets()
{
    return { "EDM", "House", "Deep House", "Techno", "Trance", "Drum & Bass", "Dubstep",
             "Pop", "Rock", "Indie", "Hip-hop", "Trap", "Lo-fi", "R&B", "Funk", "Disco",
             "Jazz", "Soul", "Reggaeton", "Afrobeats", "Ambient", "Cinematic", "Synthwave" };
}

/** Everything the model is told about the part to write. */
struct Request
{
    Part part = Part::chords;
    juce::String style = "Pop";
    int bars = 4;
    MusicalKey key;
    double bpm = 120.0;
    juce::String extraPrompt;

    /** Ideas picked at random for this take (see pickDirections()), so takes differ from each other. */
    juce::StringArray directions;

    /** Pitch names already sounding in each bar on other tracks, e.g. "A C E" (may be empty). */
    juce::StringArray harmonyPerBar;

    /** A built-in chord playing style (an id from ChordStyles.json) for a chords, arpeggio or bassline
        part; empty: the model writes the part freely. */
    juce::String playingStyle;

    /** With a playing style: true lets the model write a variation of the style's pattern, false
        plays the pattern as it is. */
    bool varyStyle = true;

    bool usesPlayingStyle() const
    {
        return playingStyle.isNotEmpty() && (part == Part::chords || part == Part::arpeggio || part == Part::bassline);
    }

    /** False for a playing style played as it is over chords the chord track already has: then the
        notes need no model. */
    bool needsModel() const { return ! usesPlayingStyle() || varyStyle || asksForBarChords(); }

    /** Bars of the varied playing pattern the model writes; it is looped over the chords. */
    int getStyleBars() const { return juce::jmin (bars, 2); }

    /** Chord set on the song's chord track for each bar of the clip (nullopt where none is set). */
    std::vector<std::optional<Chord>> chordTrack;

    std::optional<Chord> getTrackChord (int bar) const
    {
        return bar >= 0 && bar < (int) chordTrack.size() ? chordTrack[(size_t) bar] : std::nullopt;
    }

    /** True if the chord track has a chord on any bar of the clip. */
    bool hasChordTrack() const
    {
        for (int bar = 0; bar < bars; ++bar)
            if (getTrackChord (bar).has_value())
                return true;

        return false;
    }

    /** True if the model also writes a chord per bar, to fill the bars the chord track leaves empty. */
    bool needsChords() const
    {
        for (int bar = 0; bar < bars; ++bar)
            if (! getTrackChord (bar).has_value())
                return ! isPercussive (part);

        return false;
    }

    /** True if the reply has a "chords" list with one chord per bar next to the part itself. A chord
        part without a chord track is a progression already, so it doesn't need one, unless it is
        played in a playing style. */
    bool asksForBarChords() const { return needsChords() && (part != Part::chords || hasChordTrack() || usesPlayingStyle()); }

    /** A chord part over a chord track: the chords are given, the model only writes their rhythm. */
    bool isChordRhythm() const { return part == Part::chords && hasChordTrack() && ! usesPlayingStyle(); }

    /** How many bars the model writes; the result is repeated to fill `bars`.
        Long note lists are slow and small models lose the thread, so loops stay short. */
    int getPatternBars() const
    {
        int length = 0;

        switch (part)
        {
            case Part::chords: length = juce::jmin (bars, 16); break;
            case Part::drums:
            case Part::percussion: return juce::jmin (bars, 2);
            case Part::melody:
            case Part::bassline:
            case Part::arpeggio:
            default:           length = juce::jmin (bars, 8); break;
        }

        if (! hasChordTrack())
            return length;

        // A repeated pattern has to line up with the chord track, so it spans whole cycles of it.
        const int period = getChordPeriod();
        return period <= length ? period * (length / period) : period;
    }

    /** Bars of rhythm the model writes for a chord part over a chord track. */
    int getRhythmBars() const { return juce::jmin (bars, 2); }

    /** Shortest number of bars after which the chord track repeats over the clip. */
    int getChordPeriod() const
    {
        for (int period = 1; period < bars; ++period)
        {
            bool repeats = true;

            for (int bar = 0; repeats && bar + period < bars; ++bar)
                repeats = getTrackChord (bar) == getTrackChord (bar + period);

            if (repeats)
                return period;
        }

        return juce::jmax (1, bars);
    }
};
} // namespace daw::gen
