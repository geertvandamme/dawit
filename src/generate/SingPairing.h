#pragma once

#include "Pronunciation.h"
#include "engine/Clip.h"
#include "engine/Lyrics.h"

#include <vector>

namespace daw::gen
{
/** Sing this: which syllable of the lyrics goes on which note of a melody, and the advice the window gives about
    it. UI-free and tested. */
namespace singthis
{
    /** A note as it is in the song, before the sung melody is made: times in beats from the start of the song. */
    struct SourceNote
    {
        int pitch = 60;
        double startBeat = 0.0, lengthBeats = 1.0;
        float velocity = 0.8f;   // 0..1, kept when notes are matched to the words
    };

    /** A note the voice sings: one at a time, with its time in beats and in seconds. */
    struct MelodyNote
    {
        int pitch = 60;
        double startBeat = 0.0, endBeat = 1.0;
        double startSeconds = 0.0, endSeconds = 0.5;
        bool continuation = false;   // the same note picks up again after a higher one stopped; it starts nothing new
        float velocity = 0.8f;       // of the source note it comes from
    };

    struct Melody
    {
        std::vector<MelodyNote> notes;
        int overlaps = 0;   // places where notes sounded together, so only the top one was kept

        double getStartBeat() const { return notes.empty() ? 0.0 : notes.front().startBeat; }
        double getEndBeat() const { return notes.empty() ? 0.0 : notes.back().endBeat; }
    };

    /** The top note wherever notes overlap, in time order, with its times in seconds from the tempo map. */
    Melody buildMelody (std::vector<SourceNote>, const TempoMap&);

    //==============================================================================
    /** As timed: each note sings the syllable that starts on it, and a note inside a syllable holds it. One
        syllable per note: the syllables in order, one on each note, whatever their timing. */
    enum class Mode { asTimed, onePerNote };

    constexpr double pairingTolerance = 0.125;     // beats: a syllable and a note start together within this (lyrics::minGap)
    constexpr double asTimedShare = 0.75;          // As timed is the default when this share of the syllables start on a note

    /** A syllable of the lyrics with its place in the song. */
    struct Syllable
    {
        juce::String text;
        double beat = 0.0, endBeat = 0.0;   // where it starts and where it stops (the next syllable, or the line's end)
        bool endsWord = true;
        int line = 0;
    };

    /** A syllable's sounds: the consonants before its vowel, the vowel(s), and the consonants after. */
    struct SyllableSounds
    {
        std::vector<pronunciation::Sound> onset, nucleus, coda;
    };

    /** A word of the lyrics: the syllables that are joined, and how it sounds. */
    struct Word
    {
        std::vector<int> syllables;   // indices into Pairing::syllables
        juce::String text;            // "yeah-eh"
        pronunciation::Pronounced sounds;
    };

    /** What a note sings. */
    struct Assignment
    {
        int syllable = -1;     // -1: no words, the note is left out
        bool starts = false;   // the note starts the syllable; otherwise it holds the syllable of the note before (a melisma)
    };

    struct Pairing
    {
        Mode mode = Mode::asTimed;
        std::vector<Syllable> syllables;          // those in the melody's range
        std::vector<SyllableSounds> sounds;       // per syllable
        std::vector<int> wordOfSyllable;          // per syllable, an index into words
        std::vector<Word> words;
        std::vector<Assignment> notes;            // per melody note
        std::vector<int> unpairedSyllables;       // As timed: syllables with no note starting within the tolerance
        std::vector<int> notesWithoutWords;       // melody notes that sing nothing
        int leftOverSyllables = 0, leftOverNotes = 0;   // One syllable per note: what's left on each side
        int firstLeftOverSyllable = 0;            // the first syllable not sung, when syllables are left over
        bool anyWords = false;                    // there are lyrics over the melody
    };

    /** The syllables of the lyrics that are sung over the melody. As timed: those that start within the melody's
        span. One syllable per note: from the first one that starts at the melody on, to the end of the last line
        that starts before it ends. */
    std::vector<Syllable> findSyllables (const Lyrics&, const Melody&, Mode);

    /** The mode to start with: As timed when at least 75% of the syllables start on a note, otherwise One syllable per note. */
    Mode chooseMode (const Lyrics&, const Melody&);

    /** Puts the lyrics on the melody. `voice` may be null; it only changes how words are looked up. */
    Pairing pair (const Melody&, const Lyrics&, Mode, const sing::Voice* voice);

    /** The sounds of a word split over its syllables: the vowels make the syllables, a single consonant between two
        goes to the next one, and with two or more the first closes the syllable before. */
    std::vector<SyllableSounds> splitSounds (const std::vector<pronunciation::Sound>&, int numSyllables);

    //==============================================================================
    /** The rules of the advice, numbered as in the plan. */
    enum class Rule { noWords = 1, notesWithoutWords, syllablesWithoutNote, countsDiffer, guessedWords, overlap, outOfRange, crowded, melisma, allGood };

    struct Advice
    {
        Rule rule = Rule::allGood;
        juce::String text;
        bool blocksSinging = false;   // no words at all: Sing is disabled
    };

    constexpr int lowestMidi = 40, highestMidi = 84;   // E2 to C6: nearly every singer
    constexpr double crowdedSeconds = 0.15;            // a note shorter than this with 3 or more consonants on it is crowded
    constexpr int crowdedConsonants = 3;

    /** The advice for the pairing: the good news first, then what to fix. Bars count from 1. */
    std::vector<Advice> getAdvice (const Melody&, const Pairing&);

    /** "Bar 9 · E4 · "night" (n ay t)" or "Bar 9 · E4 · no words: left out" for a note's tooltip. */
    juce::String describeNote (const Melody&, const Pairing&, size_t noteIndex);

    /** A word as the user wrote it, with hyphens between its syllables ("to-night"). */
    juce::String getWordText (const Pairing&, const Word&);

    /** True when at least one note starts a syllable, so there is something to sing. */
    bool canSing (const Pairing&);

    int getBar (double beat);   // 1-based, 4/4
} // namespace singthis
} // namespace daw::gen
