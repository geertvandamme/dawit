#pragma once

#include "Rhyme.h"
#include "engine/Lyrics.h"
#include "engine/MusicalKey.h"

#include <optional>
#include <vector>

namespace daw::gen
{
/**
    The words behind the Lyrics window's Write with AI panel, without any UI and without needing a model.

    A `Request` says what to write (the whole song, a section, the next line, a rewrite of a selection, or rhymes
    for a word) over a character range of the text in the lyrics box. `buildContext` works out where that range
    sits: its verse and heading, the line numbers, the syllables and rhyme partners of the lines it replaces.
    `buildSystemPrompt`, `buildUserPrompt` and `buildSchema` make what goes to Ollama; `parseReply` turns the
    answer into clean alternatives with DawIt's own labels (language models count syllables badly, so DawIt
    counts them with the pronouncing dictionary); `applyAlternative` makes the new text with the range replaced.
    The window never changes the song itself: nothing here touches the edit.
*/
namespace lyricsai
{
    enum class Kind { song, sectionRewrite, sectionAdd, nextLine, selection, rhymes };

    /** The quick choices under Ask. Keeping the syllables and the rhyme start on. */
    struct Asks
    {
        bool moreConcrete = false, simplerWords = false, keepSyllables = true, keepRhyme = true;
        bool newWords = false, anotherAngle = false, shorter = false, longer = false;
        juce::String yourWords;   // free text: "make it about the sea"
    };

    /** Everything a request needs, as plain copies taken on the message thread, so a worker can use it. */
    struct Request
    {
        Kind kind = Kind::selection;
        juce::String text;                       // the lyrics box
        int rangeStart = 0, rangeEnd = 0;        // the selection, or the cursor (equal) for the other kinds
        SectionList sections;                    // the Sections row
        MusicalKey key;
        double bpm = 120.0;
        LyricsBrief brief;
        Asks asks;
        juce::String addHeading = "Chorus";      // sectionAdd: the heading of the new section
        int addBars = 8;                         // sectionAdd: the bars it will sit on, for its number of lines
        int seed = 0;
    };

    /** Where the range sits and what is known about the lines it replaces. */
    struct Context
    {
        struct Partner { int line = 0; juce::String word; };   // line 0: it rhymes with none

        Kind kind = Kind::selection;
        juce::String error;                // the request can't be made ("Select lines from one verse")
        juce::String heading;              // "Verse 1", "Chorus"
        int firstLine = 0, lastLine = 0;   // 1-based in the verse; nextLine: the line it follows (lastLine)
        bool wholeLines = false;           // the range is whole lines, else words inside a line
        int linesWanted = 1;               // lines an alternative has
        juce::String target;               // the text in the range, which must be unchanged to use an answer
        int replaceStart = 0, replaceEnd = 0;
        juce::String prefix;               // goes before the new text ("\n" for a next line, "\n\n[Chorus]\n" for a section)
        juce::String anchor;               // for an insertion: the text just before it, which must be unchanged
        std::vector<int> lineSyllables;                 // of the lines replaced (one entry for words inside a line)
        std::vector<std::vector<bool>> lineStress;      // their stress patterns, true = strong
        std::vector<Partner> partners;                  // per replaced line: the line it rhymed with
        juce::StringArray lastWords;                    // of every content line of the verse, in order
        juce::String scheme;                            // of the verse: "XAXA"
        bool endsLine = false;             // the range includes the last word of a line
        juce::String lineText;             // rhymes: the line the word sits in
        juce::String word;                 // rhymes: the word
        int previousSyllables = 0;         // nextLine: syllables of the line before it, 0 for none
    };

    /** Works out the context of a request. */
    Context buildContext (const Request&);

    /** Lines per section: bars divided by 2, from 2 to 8 (4 bars give 2, 8 give 4, 16 give 8). */
    int getLinesForBars (int bars);

    /** How many alternatives to ask for: 3 for a line or a selection, 2 for a section, 1 for the song. */
    int getAlternativeCount (Kind);

    /** One section of the whole song to write. */
    struct PlanItem
    {
        juce::String heading;   // "Verse", "Chorus"
        SectionType type = SectionType::verse;
        int lines = 4;
    };

    /** The sections to write for the whole song: a verse for each verse section, the chorus (and pre-chorus, post-chorus)
        once, the bridge each time; nothing for intros, solos, drops or outros. Without a sung section: verse, chorus,
        verse, chorus, bridge, chorus with 4 lines each, and `note` says so. */
    struct SongPlan
    {
        std::vector<PlanItem> items;
        std::vector<int> order;       // for the default form: which item each part of the form is (the chorus repeats)
        juce::String note;
        bool isDefaultForm = false;
    };

    SongPlan buildSongPlan (const SectionList&);

    /** The craft rules and "never reuse lines from existing songs". */
    juce::String buildSystemPrompt();

    /** The brief, key, tempo, sections, the whole lyrics with the target marked, the asks. For rhymes it also lists the
        dictionary's rhymes for the word (slow the first time: call it on the worker thread). */
    juce::String buildUserPrompt (const Request&, const Context&);

    /** The JSON schema the reply follows, with exact item counts. The prompt repeats it in words. */
    juce::var buildSchema (const Request&, const Context&);

    /** One suggestion with DawIt's checks. */
    struct Alternative
    {
        juce::StringArray lines;              // cleaned, hyphenated; a selection inside a line has one
        juce::String text;                    // what goes in the box (a song: the whole text with headings)
        juce::String note;                    // the model's few words on what it changed
        std::vector<int> lineSyllables;
        int syllables = 0;
        juce::String syllableLabel;           // "8 syllables, like the original"; empty when quiet
        bool syllableWarn = false;            // 2 or more away from the original: drawn in meterYellow
        juce::String rhymeLabel;              // "ends on light: rhymes with night (line 2)"
        bool rhymeWarn = false;
        juce::String guessedLabel;            // "guessed from the spelling" when a word isn't in the dictionary
        juce::String summary;                 // sections and the song: "4 lines, 7 to 9 syllables, rhymes XAXA"
        std::vector<juce::String> sectionSummaries;   // the song: "Verse 1: 4 lines, 7 to 9 syllables"
    };

    /** A rhyme the AI picked, labelled by its sound. */
    struct RhymePick
    {
        juce::String word;
        rhyme::Kind kind = rhyme::Kind::unknown;   // perfect, identical, family, additive, assonance, consonance or none
        bool guessed = false;                      // from the spelling
    };

    struct Result
    {
        std::vector<Alternative> alternatives;
        std::vector<RhymePick> rhymes;
        juce::String note;                    // the song's "No sections yet..." note
        juce::String error;                   // the answer couldn't be read, or nothing was left of it
        bool shouldRetry = false;             // a line target and no alternative within 2 syllables: ask once more
    };

    /** Reads the model's reply (JSON, maybe in a code fence), cleans each line (numbering, quotes, empty lines),
        drops lines that repeat the original and duplicates, forces the number of lines, and labels each alternative. A
        malformed reply gives an error, not a crash. */
    Result parseReply (const Request&, const Context&, const juce::String& reply);

    /** The text with the range replaced by the alternative, or nullopt when the text in the range has changed since the
        request (`context.target` no longer matches). Text before and after is untouched. `start` and `end` give the
        new text's range. */
    struct Applied
    {
        juce::String text;
        int start = 0, end = 0;
    };

    std::optional<Applied> applyAlternative (const juce::String& currentText, const Context&, const Alternative&);

    /** Replaces a word (a rhyme picked from a list) the same way. */
    std::optional<Applied> applyWord (const juce::String& currentText, const Context&, const juce::String& word);

    /** A reply as the model would give it, for the screenshot scene and for tests: the rewrite of line 3 of the pop song
        (three alternatives, the last one longer when `withLongOne`), a song, or rhymes for "night". */
    juce::String getCannedReply (const Request&, const Context&, bool withLongOne = false);
} // namespace lyricsai
} // namespace daw::gen
