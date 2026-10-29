#pragma once

#include "Chord.h"

namespace daw
{
/** A run of bars copied from the chord track: their chords, numbered from 0, and how many bars
    the run spans (bars without a chord inside it are kept as gaps). */
struct ChordBars
{
    int bars = 0;
    ChordMap chords;

    bool isEmpty() const noexcept { return bars <= 0; }
    bool operator== (const ChordBars& other) const { return bars == other.bars && chords == other.chords; }
};

/** Copying, pasting, moving and repeating bars of the chord track. Pasting a run makes its bars
    exactly what was copied, so a gap in the copy clears the bar it lands on. */
namespace chordEdits
{
    inline ChordBars copyBars (const ChordMap& chords, int firstBar, int numBars)
    {
        ChordBars copy;
        copy.bars = juce::jmax (0, numBars);

        for (auto it = chords.lower_bound (firstBar); it != chords.end() && it->first < firstBar + copy.bars; ++it)
            copy.chords[it->first - firstBar] = it->second;

        return copy;
    }

    inline void clearBars (ChordMap& chords, int firstBar, int numBars)
    {
        chords.erase (chords.lower_bound (firstBar), chords.lower_bound (firstBar + juce::jmax (0, numBars)));
    }

    /** Puts a copy at a bar; bars before the first bar of the song are dropped. */
    inline void pasteBars (ChordMap& chords, const ChordBars& copy, int atBar)
    {
        clearBars (chords, atBar, copy.bars);

        for (auto& [bar, chord] : copy.chords)
            if (atBar + bar >= 0)
                chords[atBar + bar] = chord;
    }

    /** How many bars a paste would change that already have a chord: a different chord, or one a
        gap in the copy would clear. Used to ask before pasting over chords. */
    inline int countReplaced (const ChordMap& chords, const ChordBars& copy, int atBar)
    {
        int replaced = 0;

        for (auto it = chords.lower_bound (atBar); it != chords.end() && it->first < atBar + copy.bars; ++it)
        {
            const auto incoming = copy.chords.find (it->first - atBar);
            replaced += incoming == copy.chords.end() || ! (incoming->second == it->second) ? 1 : 0;
        }

        return replaced;
    }

    /** Moves bars to start at another bar; the bars they leave are cleared. */
    inline void moveBars (ChordMap& chords, int firstBar, int numBars, int toBar)
    {
        const auto copy = copyBars (chords, firstBar, numBars);
        clearBars (chords, firstBar, numBars);
        pasteBars (chords, copy, toBar);
    }

    /** Repeats a run of bars after itself until `endBar` (exclusive); the last repeat is cut short
        when the run doesn't fit a whole number of times. Returns the number of bars written. */
    inline int repeatUntil (ChordMap& chords, int firstBar, int numBars, int endBar)
    {
        if (numBars <= 0 || endBar <= firstBar + numBars)
            return 0;

        const auto copy = copyBars (chords, firstBar, numBars);

        for (int at = firstBar + numBars; at < endBar; at += numBars)
        {
            auto part = copy;
            part.bars = juce::jmin (numBars, endBar - at);
            part.chords.erase (part.chords.lower_bound (part.bars), part.chords.end());
            pasteBars (chords, part, at);
        }

        return endBar - firstBar - numBars;
    }

    /** The bars as text, one chord symbol per bar and "-" for a bar without one: "C Am - G". */
    inline juce::String toText (const ChordBars& copy)
    {
        juce::StringArray bars;

        for (int bar = 0; bar < copy.bars; ++bar)
        {
            const auto it = copy.chords.find (bar);
            bars.add (it != copy.chords.end() ? it->second.getName() : "-");
        }

        return bars.joinIntoString (" ");
    }

    /** Reads bars from text such as "C Am F G", "| Am | F | C | G |" or "Dm7, G7, Cmaj7": one
        chord per bar, "-" or "%" for an empty bar or the chord before it. Empty when any word
        isn't a chord DawIt knows, so other text on the clipboard isn't pasted as chords. */
    inline ChordBars fromText (const juce::String& text)
    {
        juce::StringArray words;
        words.addTokens (text.replaceCharacters (",|\n\r\t", "     "), " ", {});
        words.removeEmptyStrings();

        ChordBars bars;

        for (auto& word : words)
        {
            if (word == "-" || word == "x")
            {
                ++bars.bars;
                continue;
            }

            if (word == "%")
            {
                if (const auto previous = bars.chords.find (bars.bars - 1); previous != bars.chords.end())
                    bars.chords[bars.bars] = previous->second;

                ++bars.bars;
                continue;
            }

            const auto chord = Chord::fromName (word);

            if (! chord.has_value())
                return {};

            bars.chords[bars.bars++] = *chord;
        }

        return bars;
    }
} // namespace chordEdits
} // namespace daw
