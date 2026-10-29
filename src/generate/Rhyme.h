#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace daw::gen
{
/** The sound of English lyrics, for the lyrics guide and the Lyrics window's Write with AI panel: which words
    rhyme and how closely (perfect, family, assonance...), how many syllables a line has, where its stresses
    fall, and which words from the CMU Pronouncing Dictionary rhyme with a word. Everything works from how words
    sound (the dictionary is American English), and falls back to the spelling for words it doesn't have. */
namespace rhyme
{
    /** How two words rhyme, from the most settled to the loosest. `unknown`: a word isn't in the dictionary. */
    enum class Kind { perfect, identical, family, additive, assonance, consonance, none, unknown };

    /** "perfect", "identical", "family", "additive", "assonance", "consonance", "none" or "unknown". */
    juce::String getKindName (Kind);

    /** The consonant families as songwriters group them: plosives (p b t d k g), fricatives (f v th dh s z sh zh
        ch jh) and nasals (m n ng). The others (l, r, w, y, hh) belong to none. */
    enum class Family { none, plosive, fricative, nasal };
    Family getFamily (const juce::String& consonant);

    /** What has to match for a rhyme: the last stressed vowel and everything after it ("river" gives ih v er).
        The onset is the consonants just before that vowel ("r"). */
    struct Part
    {
        bool found = false;                  // false: the word isn't in the dictionary
        bool guessed = false;                // sounds worked out from the spelling
        juce::String vowel;                  // "ih"
        std::vector<juce::String> onset;     // "r"
        std::vector<juce::String> tail;      // "v", "er"
        int syllables = 0;                   // vowels in the rhyme part: 2 for "river"

        /** The vowel and the tail as one string: "ih v er". */
        juce::String toString() const;
    };

    /** The rhyme part of the last word of `word` (hyphens and punctuation are ignored). Not found when the word is
        in no dictionary: use `guessed` to get one from the spelling. */
    Part getRhymePart (const juce::String& word, bool guessFromSpelling = false);

    /** How the last words of `a` and `b` rhyme, or `Kind::unknown` when either isn't in the dictionary. */
    Kind classify (const juce::String& a, const juce::String& b);

    /** Whether the dictionary has the word. */
    bool isKnown (const juce::String& word);

    /** How many syllables a line sings. A word with hyphens counts its parts ("ri-ver" is 2); any other word gets the
        dictionary's vowel count, and failing that the spelling split. Dashes and other marks count 0. */
    int countSyllables (const juce::String& line);

    /** One entry per syllable of the line: true for a strong one. A primary-stressed syllable is strong; a one-syllable
        function word (the, and, so, I) is weak and any other one-syllable word strong; a word the dictionary lacks
        has its first syllable strong. */
    std::vector<bool> getStressPattern (const juce::String& line);

    /** The line with its longer words split by hyphens ("to-ge-ther") where the spelling split and the dictionary
        agree on the count; any other word stays whole. Words that already have hyphens stay as they are. */
    juce::String hyphenate (const juce::String& line);

    /** One word from the dictionary that rhymes with another. */
    struct DictionaryRhyme
    {
        juce::String word;
        Kind kind = Kind::perfect;   // perfect, identical or family
        int syllables = 0;           // of the whole word
    };

    struct RhymeList
    {
        bool guessed = false;                  // the word isn't in the dictionary: rhymes come from its spelling
        std::vector<DictionaryRhyme> items;    // perfect by syllables then length, family, then identical last
    };

    /** The dictionary's rhymes for a word: up to `maxPerKind` perfect, family and identical ones for each number of syllables (the word itself
        and words with an apostrophe left out). The first call builds an index (about a second); it is safe to call from
        a worker thread. */
    RhymeList findRhymes (const juce::String& word, int maxPerKind = 30);
} // namespace rhyme
} // namespace daw::gen
