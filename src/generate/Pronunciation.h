#pragma once

#include "engine/Voice.h"

#include <functional>
#include <vector>

namespace daw::gen
{
/** How an English word sounds, for Sing this: a word becomes phonemes (one sound of a language: "night" is
    n, ay, t). The voice's own dictionary is tried first, then the CMU Pronouncing Dictionary built into the app,
    and a word in neither is guessed from its spelling and flagged, so the window can list it. */
namespace pronunciation
{
    enum class Source { voiceDictionary, cmudict, guessed };

    /** One sound of a word: in the voice's own spelling ("en/ay") when a voice is given, otherwise the dictionary's
        lower-case name ("ay"). */
    struct Sound
    {
        juce::String symbol;
        bool vowel = false;
    };

    struct Pronounced
    {
        std::vector<Sound> sounds;   // empty when the "word" has no letters (a dash, a number)
        Source source = Source::cmudict;
    };

    /** The word without the punctuation around it, in lower case ("Night," gives "night"; "don't" stays). */
    juce::String cleanWord (const juce::String&);

    /** The word's sounds. `voice` may be null (the tests, or no voice installed yet). */
    Pronounced pronounce (const juce::String& word, const sing::Voice* voice);

    /** The CMU Pronouncing Dictionary's sounds for a word as lower-case names without stress ("n", "ay", "t"),
        or an empty list when it is not in it. */
    std::vector<juce::String> lookupCmu (const juce::String& word);

    /** One sound of a dictionary word with its stress: 1 primary, 2 secondary, 0 none, -1 for a consonant. */
    struct StressedSound
    {
        juce::String symbol;   // "ay"
        int stress = -1;
        bool isVowel() const noexcept { return stress >= 0; }
    };

    /** Like lookupCmu, but each vowel keeps its stress. Empty when the word is not in the dictionary. */
    std::vector<StressedSound> lookupCmuStressed (const juce::String& word);

    /** Calls `callback` for every word of the dictionary (lower case, no alternative pronunciations) with its
        sounds. Slow (about 126,000 words): for building an index once. Safe from any thread. */
    void forEachCmuWord (const std::function<void (const juce::String&, const std::vector<StressedSound>&)>& callback);

    /** Sounds guessed from the spelling by simple rules, for words no dictionary has. */
    std::vector<juce::String> guessFromSpelling (const juce::String& word);

    /** The 39 sounds the dictionary uses: 15 vowels and 24 consonants. */
    std::vector<juce::String> getAllCmuPhonemes();
    bool isCmuVowel (const juce::String&);

    /** How many different words the dictionary has (about 126,000). */
    int getCmuWordCount();

    /** How many entries it has, the other ways to say a word counted too (more than 134,000). */
    int getCmuEntryCount();
} // namespace pronunciation
} // namespace daw::gen
