#pragma once

#include "ChordDiagrams.h"
#include "ChordGuide.h"

namespace daw::gen
{
/** What goes on a printed chord sheet for a key: its chords with their numerals and families, its
    sevenths, the chords often borrowed into it, its well-known progressions, and for guitar the
    capo that makes it easiest. The drawing is in ui/ChordSheetExport. */
namespace sheet
{
    /** A chord as the sheet writes it, spelled for the key: "Db" in Eb major, "G#m" in E major. */
    struct Entry
    {
        Chord chord;
        juce::String name;      // "Db", "G#m7"
        juce::String notes;     // "Db F Ab"
        juce::String numeral;   // "vi", "V7", "bVII"
        juce::String role;      // "tonic", "from harmonic minor"
        guide::Family family = guide::Family::home;
    };

    struct ProgressionRow
    {
        juce::String name, summary, numerals;   // numerals as "I - V - vi - IV"
        std::vector<Entry> chords;
    };

    struct Sheet
    {
        MusicalKey key;
        juce::String keyName;           // "Eb major", "Db major", "F# minor"
        juce::String keySignature;      // "2 sharps: F# C#"
        std::vector<Entry> chords;      // the triads on degrees 1 to 7
        std::vector<Entry> sevenths;    // the seventh chords on degrees 1 to 7
        std::vector<Entry> borrowed;    // chords from outside the key that songs in it often use
        std::vector<ProgressionRow> progressions;
    };

    Sheet make (const MusicalKey&);

    /** Any chord as the sheet writes it in the key: spelled from its scale degree when its root is in
        the key (D#dim in E major), otherwise with the key's flats or sharps (Bb in C major). */
    Entry describe (const MusicalKey&, const Chord&);

    /** The key's name with its tonic spelled as its key signature does: "Db major", not "C# major". */
    juce::String getKeyName (const MusicalKey&);

    /** A capo makes a hard key easy on guitar: with it on `fret`, the shapes of `shapesKey` sound
        in the key. `fret` is 0 when no capo makes it clearly easier (the key's seven chords rated 0
        for an easy shape, 1 medium and 2 hard, and the capo saving at least 3). `easy` counts the
        chords whose easiest shape is an easy one, without the capo and with it. */
    struct Capo
    {
        int fret = 0;
        MusicalKey shapesKey;
        int easyWithout = 0, easyWith = 0;
    };

    Capo findCapo (const MusicalKey&);
} // namespace sheet
} // namespace daw::gen
