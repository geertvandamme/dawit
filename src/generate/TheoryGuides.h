#pragma once

#include <juce_core/juce_core.h>

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
        sound, midi, rhythm, chords, chordShapes, structure, styles, lyrics, singing, tune,
        gainStaging, eq, compressor, gate, stereo, routing, reverb, delay, saturation, distortion,
        subtractive, additive, fm, drums, physical, recording, tuning, triggers, readMusic
    };

    constexpr Guide allGuides[] { Guide::sound, Guide::midi, Guide::rhythm, Guide::chords, Guide::chordShapes,
                                  Guide::structure, Guide::styles, Guide::lyrics, Guide::singing, Guide::tune, Guide::gainStaging, Guide::eq, Guide::compressor,
                                  Guide::gate, Guide::stereo, Guide::routing, Guide::reverb, Guide::delay, Guide::saturation, Guide::distortion,
                                  Guide::subtractive, Guide::additive, Guide::fm, Guide::drums, Guide::physical, Guide::recording, Guide::tuning, Guide::triggers, Guide::readMusic };

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
        ("compress" finds "compressor", "chords" finds "chord"). An empty query matches everything, in order. */
    std::vector<Match> search (const std::vector<Entry>&, const juce::String& query);

    /** The words of a query that are searched for, lower case, without the small words. */
    juce::StringArray getSearchWords (const juce::String& query);
} // namespace guides
} // namespace daw::gen
