#pragma once

#include "engine/Lyrics.h"

#include <optional>
#include <vector>

namespace daw::gen
{
/** How lyrics are written, for beginning songwriters: what each section's words do, lines and
    phrases, rhyme, stress on the beat, putting words on a melody, prosody, the hook, writing tips,
    song forms and singing along, each with examples to look at; and a look at the song's own lyrics. Used by the
    lyrics guide. */
namespace lyricsguide
{
    enum class Topic { sections, lines, rhyme, findRhyme, stress, melody, prosody, hook, craft, writeWithAi, forms, singing, computer, voices, yours };

    constexpr Topic allTopics[] { Topic::sections, Topic::lines, Topic::rhyme, Topic::findRhyme, Topic::stress, Topic::melody,
                                  Topic::prosody, Topic::hook, Topic::craft, Topic::writeWithAi, Topic::forms, Topic::singing,
                                  Topic::computer, Topic::voices, Topic::yours };

    juce::String getTopicName (Topic);      // "Rhyme"
    juce::String getTopicSummary (Topic);   // one line for the list

    /** One sound of a sung word (a phoneme), in beats from the start of the example: a vowel carries the note. */
    struct Sound
    {
        juce::String symbol;      // "ay"
        double start = 0.0, length = 0.0;
        bool vowel = false;
    };

    /** A word or syllable of an example. */
    struct Token
    {
        juce::String text;                    // empty: the syllable before is held on this note
        double beat = 0.0, length = 0.0;      // on a beat grid or melody
        int pitch = -1;                       // a melody note
        bool stressed = false;                // said louder: drawn bold
        bool wrong = false;                   // what not to do: drawn red
        int rhyme = -1;                       // which rhyme it belongs to (A = 0, B = 1...)
        std::optional<SectionType> section;   // drawn as a section chip
        std::vector<Sound> sounds;            // a sounds example: the sounds of the word, drawn under its note
    };

    struct ExampleLine
    {
        juce::String label;                   // "Verse", "A", "Right"
        int labelRhyme = -1;                  // colours the label as that rhyme
        std::optional<SectionType> section;   // colours the label as that section
        std::vector<Token> tokens;
        juce::String note;                    // what it shows
    };

    /** Words: lines of text. Beats: each line's syllables on a grid of beats, strong beats marked.
        Melody: each line's syllables under the notes they're sung on. Sounds: the melody, with the sounds
        (phonemes) of each word as blocks under its note. */
    enum class Kind { words, beats, melody, sounds };

    struct Example
    {
        Kind kind = Kind::words;
        double startBeat = 0.0, endBeat = 8.0;   // the grid of a beats or melody example (negative: a pickup)
        std::vector<ExampleLine> lines;
    };

    /** The pictures of a topic; "yours" shows the song's own lyrics, verse by verse. */
    Example getExample (Topic, const Lyrics&);

    /** The explanation of a topic; "yours" looks at the song's own lyrics and sections. */
    juce::String explain (Topic, const Lyrics&, const SectionList&, double bpm);

    //==============================================================================
    /** The end of a word that rhymes, by its spelling: "night" and "light" give "ite", "late" "ate". Used for words the
        pronouncing dictionary doesn't have. */
    juce::String getRhymeKey (const juce::String& word);

    /** Whether two words rhyme perfectly or are the same sound (repetition): by sound (`gen::rhyme`) where the dictionary
        has both words, otherwise by spelling. */
    bool rhymes (const juce::String& word, const juce::String& other);

    /** The rhyme scheme of lines ending in these words: "XAXA", "AABB", "ABAB"; X for a line that
        rhymes with none of the others. */
    juce::String findRhymeScheme (const juce::StringArray& lastWords);

    /** A verse of the song's lyrics: its lines with how many syllables each (counted by sound, `rhyme::countSyllables`), over how many beats,
        and the rhyme scheme. */
    struct LineReport
    {
        juce::String text;
        int syllables = 0;
        double beats = 0.0;
        juce::String rhyme;   // its letter in the scheme
    };

    struct VerseReport
    {
        juce::String heading;
        std::vector<LineReport> lines;
        juce::String scheme;
    };

    std::vector<VerseReport> analyse (const Lyrics&);
} // namespace lyricsguide
} // namespace daw::gen
