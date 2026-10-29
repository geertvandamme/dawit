#pragma once

#include "MusicalKey.h"

#include <map>
#include <optional>

namespace daw
{
/** A chord: a root pitch class plus the semitone offsets of its notes from the root. */
struct Chord
{
    int root = 0;                       // pitch class: 0 = C ... 11 = B
    juce::Array<int> intervals { 0, 4, 7 };

    bool operator== (const Chord& other) const { return root == other.root && intervals == other.intervals; }

    /** Chord symbol, e.g. "Am7". */
    juce::String getName() const { return getName (root, intervals); }

    /** The chord's notes spelled from its root, e.g. "A C E G" or "E G# B". */
    juce::String getNoteNames() const
    {
        static const char* letters = "CDEFGAB";
        static const int naturals[] = { 0, 2, 4, 5, 7, 9, 11 };
        const auto rootName = MusicalKey::getTonicName (root);
        const int rootLetter = juce::String (letters).indexOfChar (rootName[0]);
        juce::StringArray names;

        for (int i : intervals)
        {
            // Letters above the root: 2nds/9ths 1, 3rds 2, 4ths 3, 5ths 4, 7ths 6 (a dim7's 9 semitones is a bb7).
            static const int steps[] = { 0, 1, 1, 2, 2, 3, 4, 4, 4, 6, 6, 6 };
            const int letter = (rootLetter + steps[i % 12]) % 7;
            const int accidental = ((root + i - naturals[letter]) % 12 + 18) % 12 - 6;
            names.addIfNotAlreadyThere (juce::String::charToString (letters[letter])
                                        + juce::String::repeatedString (accidental > 0 ? "#" : "b", std::abs (accidental)));
        }

        return names.joinIntoString (" ");
    }

    /** Pitch classes of the chord, root first, without duplicates. */
    juce::Array<int> getPitchClasses() const
    {
        juce::Array<int> pitchClasses;
        for (int i : intervals)
            pitchClasses.addIfNotAlreadyThere ((root + i) % 12);

        return pitchClasses;
    }

    //==============================================================================
    struct Quality { const char* suffix; juce::Array<int> intervals; };

    /** The chord qualities offered when picking a chord by hand. */
    static const std::vector<Quality>& getQualities()
    {
        static const std::vector<Quality> qualities {
            { "",        { 0, 4, 7 } },         { "m",       { 0, 3, 7 } },
            { "7",       { 0, 4, 7, 10 } },     { "maj7",    { 0, 4, 7, 11 } },
            { "m7",      { 0, 3, 7, 10 } },     { "m7b5",    { 0, 3, 6, 10 } },
            { "dim",     { 0, 3, 6 } },         { "dim7",    { 0, 3, 6, 9 } },
            { "aug",     { 0, 4, 8 } },         { "sus2",    { 0, 2, 7 } },
            { "sus4",    { 0, 5, 7 } },         { "add9",    { 0, 4, 7, 14 } },
            { "m(add9)", { 0, 3, 7, 14 } },     { "9",       { 0, 4, 7, 10, 14 } },
            { "maj9",    { 0, 4, 7, 11, 14 } }, { "m9",      { 0, 3, 7, 10, 14 } }
        };

        return qualities;
    }

    /** Parses a chord symbol such as "Am7", "F#", "Bbmaj7" or "Csus4" (one of getQualities()). */
    static std::optional<Chord> fromName (const juce::String& text)
    {
        const auto name = text.trim().removeCharacters (" ");

        if (name.isEmpty())
            return std::nullopt;

        static const char* letters = "C D EF G A B";
        const int letter = juce::String (letters).indexOfChar (juce::CharacterFunctions::toUpperCase (name[0]));

        if (letter < 0)
            return std::nullopt;

        int root = letter, length = 1;

        if (name[1] == '#')      { ++root; ++length; }
        else if (name[1] == 'b') { --root; ++length; }

        auto suffix = name.substring (length);

        if (suffix == "M" || suffix.equalsIgnoreCase ("maj")) suffix = {};
        if (suffix.equalsIgnoreCase ("min"))                  suffix = "m";
        if (suffix == "madd9")                                suffix = "m(add9)";

        for (auto& q : getQualities())
            if (suffix == q.suffix)
                return Chord { (root + 12) % 12, q.intervals };

        return std::nullopt;
    }

    /** Chord symbol from its root and intervals, e.g. (9, {0, 3, 7, 10}) -> "Am7". */
    static juce::String getName (int root, const juce::Array<int>& intervals)
    {
        auto has = [&intervals] (int semitones)
        {
            for (int i : intervals)
                if (i % 12 == semitones)
                    return true;

            return false;
        };

        const bool major3 = has (4), minor3 = has (3), sus2 = ! major3 && ! minor3 && has (2), sus4 = ! major3 && ! minor3 && has (5);
        const bool dim5 = has (6) && ! has (7), aug5 = has (8) && ! has (7) && major3;
        const bool b7 = has (10), maj7 = has (11), dim7 = dim5 && minor3 && has (9) && ! b7;
        const bool ninth = std::any_of (intervals.begin(), intervals.end(), [] (int i) { return i == 14 || i == 13 || i == 15; })
                           && (b7 || maj7);
        const bool add9 = ! b7 && ! maj7 && std::any_of (intervals.begin(), intervals.end(), [] (int i) { return i == 14; });

        juce::String name = MusicalKey::getTonicName (root);

        if (sus2 || sus4)          name << (sus2 ? "sus2" : "sus4") << (b7 ? "(7)" : "");
        else if (dim7)             name << "dim7";
        else if (minor3 && dim5)   name << (b7 ? "m7b5" : "dim");
        else if (aug5)             name << (b7 ? "aug7" : "aug");
        else if (minor3)           name << "m" << (maj7 ? "(maj7)" : ninth ? "9" : b7 ? "7" : "");
        else if (maj7)             name << (ninth ? "maj9" : "maj7");
        else if (b7)               name << (ninth ? "9" : "7");

        if (add9)
            name << (minor3 ? "(add9)" : "add9");

        return name;
    }
};

/** The song's chord track: at most one chord per bar (bar 0 is the first). Bars without an
    entry have no chord set. It only describes the harmony; it makes no sound. */
using ChordMap = std::map<int, Chord>;
} // namespace daw
