#pragma once

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace daw::gen
{
/** Every theory guide in DawIt in one list, for the *Theory Guides* window: the area each belongs to, a line on
    what it covers, where else it opens from, its topics, and words to find it by. Each guide still opens from the
    place its theory applies; this list is the one starting point for all of them. A new guide is added here. */
namespace guides
{
    /** The areas the guides are grouped in, in the order a beginner would read them. */
    enum class Area { sound, midi, rhythm, readingMusic, harmony, songwriting, singing, mixing, effects, synthesis };

    constexpr Area allAreas[] { Area::sound, Area::midi, Area::rhythm, Area::readingMusic, Area::harmony, Area::songwriting, Area::singing,
                                Area::mixing, Area::effects, Area::synthesis };

    juce::String getAreaName (Area);      // "Chords and keys"
    juce::String getAreaSummary (Area);   // one line under the name

    enum class Guide
    {
        sound, midi, rhythm, chords, chordShapes, structure, styles, lyrics, singing, tune, harmoniser,
        gainStaging, eq, compressor, gate, stereo, routing, loudness, reverb, delay, slicer, stepFilter, spectralFreeze, saturation, distortion,
        subtractive, additive, fm, drums, physical, recording, tuning, triggers, readMusic, keyFinding, arpeggiator, sampler, handover, instruments
    };

    constexpr Guide allGuides[] { Guide::sound, Guide::midi, Guide::rhythm, Guide::chords, Guide::chordShapes,
                                  Guide::structure, Guide::styles, Guide::lyrics, Guide::singing, Guide::tune, Guide::harmoniser, Guide::gainStaging, Guide::eq, Guide::compressor,
                                  Guide::gate, Guide::stereo, Guide::routing, Guide::loudness, Guide::reverb, Guide::delay, Guide::slicer, Guide::stepFilter, Guide::spectralFreeze, Guide::saturation, Guide::distortion,
                                  Guide::subtractive, Guide::additive, Guide::fm, Guide::drums, Guide::physical, Guide::recording, Guide::tuning, Guide::triggers, Guide::readMusic, Guide::keyFinding, Guide::arpeggiator, Guide::sampler, Guide::handover, Guide::instruments };

    /** The guide's key, the short name `--screenshots` and `SOURCES.md` use for it: "sound", "readMusic", "stepFilter". */
    juce::String getKey (Guide);

    /** The guide with this key (case does not matter), or none. */
    std::optional<Guide> findGuide (const juce::String& key);

    /** The name of the last topic of every guide, which shows its sources. */
    extern const juce::String sourcesTopicName;

    /** The topic number that opens a guide at its Sources topic when the guide's own topic count varies (Chords, Structure).
        The others open at Sources with the number one past their own topics. */
    constexpr int sourcesTopicIndex = 1000;

    /** A guide as the hub lists it. */
    struct Entry
    {
        Guide guide;
        Area area;
        juce::String name;          // the guide's window title: "What is reverb?"
        juce::String summary;       // what it covers, in a sentence
        juce::String opensFrom;     // where else it opens: "DawIt Reverb's window (the guide button)"
        juce::String plugin;        // the built-in effect or synth it explains ("DawIt Reverb"), or empty
        juce::StringArray keywords; // words people search for that aren't in the name or the topics
        juce::StringArray topics;   // its pages, in order; empty for a guide built on a plugin (see getEntries)
        juce::StringArray texts;    // each page's text without a song, parallel to topics; empty for Sources and for a guide built on a plugin
    };

    /** Every guide, in the order of allGuides. The topics come from each guide's own list. The pages of a guide built
        on a built-in effect or synth are made with the plugin, so the window fills those in itself. */
    std::vector<Entry> getEntries();

    /** The guide's entry. */
    Entry getEntry (Guide);

    /** A short path through the guides for beginners: sound, rhythm, chords, structure, styles, gain staging, EQ and
        compression. */
    std::vector<Guide> getStartPath();

    /** A guide that matches a search, with the topics that match best (their indexes); none when it's the guide as a
        whole that matches. */
    struct Match
    {
        int entry = 0;   // the index into the entries searched
        std::vector<int> topics;
        int score = 0;
    };

    /** The guides and topics that match every word of `query` ("muddy vocal", "what is a triplet?"), best first:
        a word in the name counts most, then in a topic or the keywords, then in the summary. Small words such as
        "what", "is" and "a" are left out, case doesn't matter, and a word matches the start of a longer one
        ("compress" finds "compressor", "chords" finds "chord"). A word only in a page's text matches too, and ranks
        below the summary; that page is named in `topics`. An empty query matches everything, in order. */
    std::vector<Match> search (const std::vector<Entry>&, const juce::String& query);

    /** The words of a query that are searched for, lower case, without the small words. */
    juce::StringArray getSearchWords (const juce::String& query);

    /** A line of `text` (at most 160 characters, with "..." where it is cut) around the first place a word of
        `words` starts a word, or empty when none does. */
    juce::String getExcerpt (const juce::String& text, const juce::StringArray& words);

    //==============================================================================
    // Links between pages

    /** One end of a link: a guide and the name of one of its pages; an empty topic means the guide's first page. */
    struct PageRef
    {
        Guide guide = Guide::sound;
        juce::String topic;
    };

    /** A link joins two pages in two different guides. It is written once and shows on both. */
    struct Link
    {
        PageRef a, b;
    };

    /** Every link, in the order they were written. The one place links are declared. */
    const std::vector<Link>& getLinks();

    /** The pages linked to a page, from both directions of the table. `isFirstPage` says whether the page is the
        guide's first, which the links with an empty topic belong to. A returned page with an empty topic is the
        other guide's first page. */
    std::vector<PageRef> getLinksFor (Guide, const juce::String& topic, bool isFirstPage);

    /** What is wrong with the links, one line each, or nothing. Checks that the two guides of a link differ, that no
        pair is written twice (either way round), that every guide is in a link, and, for every guide whose entry
        lists its topics, that the named pages exist. */
    juce::StringArray validateLinks (const std::vector<Entry>&);

    //==============================================================================
    // Reading progress and Next page

    /** A page by position: the guide's index into the entries and the topic number. */
    struct PagePosition
    {
        Guide guide = Guide::sound;
        int topic = 0;
    };

    /** The id a read page is saved under: "<guide key>/<topic name>", "rhythm/Quantising". */
    juce::String getPageId (Guide, const juce::String& topic);

    /** True for a page that counts for progress: not Sources and not a "Your ..." page, which change with the song. */
    bool isCountedPage (const juce::String& topic);

    /** Pages read out of the pages that count. */
    struct Progress
    {
        int read = 0, total = 0;
    };

    /** How much of one guide is read, given the ids of the pages read. An id that names no page changes nothing. */
    Progress getProgress (const Entry&, const juce::StringArray& readIds);

    /** How much of the Start here path is read. */
    Progress getPathProgress (const std::vector<Entry>&, const juce::StringArray& readIds);

    /** The first page that counts and is unread along the Start here path, or none when it is all read. */
    std::optional<PagePosition> getFirstUnreadOnPath (const std::vector<Entry>&, const juce::StringArray& readIds);

    /** Where Next page goes from a page: the next page of the guide; at the end of a guide on Start here (its last
        page before Sources, or Sources) the first page of the next guide on the path; off the path, on to Sources and
        then nothing. None after the last guide of the path. */
    std::optional<PagePosition> getNextPage (const std::vector<Entry>&, Guide, int topic);
} // namespace guides
} // namespace daw::gen
