#pragma once

#include "engine/Section.h"

namespace daw::gen
{
/** Plain-language explanations of song structure for beginning musicians: what each section is for,
    and the typical structures of different styles of music, with why they work. Used by the song
    structure guide. */
namespace structure
{
    /** One section of a structure. An empty name shows the type's name, numbered when the type comes
        back (Verse 1, Verse 2). */
    struct Part
    {
        Part (SectionType t, int b, juce::String n = {}) : type (t), bars (b), name (std::move (n)) {}

        SectionType type;
        int bars;
        juce::String name;
    };

    struct Structure
    {
        juce::String name;      // e.g. "Pop song (verse-chorus)"
        juce::String styles;    // the styles of music that use it, e.g. "Pop, R&B, Afrobeats"
        juce::String summary;   // one line
        juce::String story;     // why it works, in a few sentences
        juce::String tips;      // where a beginner can start
        double typicalBpm = 120.0;
        std::vector<Part> parts;

        int getTotalBars() const;

        /** The structure as sections from `startBar` on, one after the other. */
        SectionList toSections (int startBar) const;
    };

    /** Typical structures, from songs (pop, rock, hip-hop, blues, AABA, strophic) to electronic music
        (EDM, techno and house, trance, drum & bass), loop-based beats and film music. */
    const std::vector<Structure>& getStructures();

    /** A few words on a section's job, e.g. "builds tension" for the pre-chorus. */
    juce::String getRole (SectionType);

    /** What a section is for, how long it usually is and how much energy it has. */
    juce::String describeSection (SectionType);

    /** How songs are built: sections, bars and phrases, repetition and contrast, energy, and how to
        use the section track. */
    juce::String getIntroduction();

    /** Every section type, described. */
    juce::String explainSections();

    /** A structure: its story, its sections with their jobs, its length and time, and tips. */
    juce::String explainStructure (const Structure&, double songBpm);

    /** The song's own sections: their order and times, whether they match a known structure, and a
        few observations (odd lengths, gaps, a late first chorus...). */
    juce::String explainSongStructure (const SectionList&, double bpm);

    /** The name of the known structure whose sections come in this order, or empty. */
    juce::String findStructureName (const SectionList&);

    /** How long `bars` of 4/4 take at a tempo, e.g. "2:08". */
    juce::String formatDuration (int bars, double bpm);
} // namespace structure
} // namespace daw::gen
