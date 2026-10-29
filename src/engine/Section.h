#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <optional>
#include <vector>

namespace daw
{
/** What a part of the song is for. It sets the section's colour and how much energy it usually has. */
enum class SectionType
{
    intro, verse, preChorus, chorus, postChorus, bridge, solo, interlude,
    phrase, breakdown, buildUp, drop, outro
};

namespace sectionTypes
{
    constexpr std::array<SectionType, 13> all { SectionType::intro, SectionType::verse, SectionType::preChorus, SectionType::chorus,
                                                SectionType::postChorus, SectionType::bridge, SectionType::solo, SectionType::interlude,
                                                SectionType::phrase, SectionType::breakdown, SectionType::buildUp, SectionType::drop,
                                                SectionType::outro };

    /** Shown in the UI, e.g. "Pre-chorus". */
    inline juce::String getName (SectionType t)
    {
        switch (t)
        {
            case SectionType::intro:      return "Intro";
            case SectionType::verse:      return "Verse";
            case SectionType::preChorus:  return "Pre-chorus";
            case SectionType::chorus:     return "Chorus";
            case SectionType::postChorus: return "Post-chorus";
            case SectionType::bridge:     return "Bridge";
            case SectionType::solo:       return "Solo";
            case SectionType::interlude:  return "Interlude";
            case SectionType::phrase:     return "Phrase";
            case SectionType::breakdown:  return "Breakdown";
            case SectionType::buildUp:    return "Build-up";
            case SectionType::drop:       return "Drop";
            case SectionType::outro:      return "Outro";
        }

        return {};
    }

    /** Stable id for project files, e.g. "preChorus". */
    inline juce::String getId (SectionType t)
    {
        switch (t)
        {
            case SectionType::intro:      return "intro";
            case SectionType::verse:      return "verse";
            case SectionType::preChorus:  return "preChorus";
            case SectionType::chorus:     return "chorus";
            case SectionType::postChorus: return "postChorus";
            case SectionType::bridge:     return "bridge";
            case SectionType::solo:       return "solo";
            case SectionType::interlude:  return "interlude";
            case SectionType::phrase:     return "phrase";
            case SectionType::breakdown:  return "breakdown";
            case SectionType::buildUp:    return "buildUp";
            case SectionType::drop:       return "drop";
            case SectionType::outro:      return "outro";
        }

        return {};
    }

    inline std::optional<SectionType> fromId (const juce::String& id)
    {
        for (auto t : all)
            if (getId (t) == id)
                return t;

        return std::nullopt;
    }

    /** How much is usually going on, from 1 (calm, sparse) to 5 (everything playing, the peak). */
    inline int getEnergy (SectionType t)
    {
        switch (t)
        {
            case SectionType::outro:      return 1;
            case SectionType::intro:
            case SectionType::interlude:
            case SectionType::breakdown:  return 2;
            case SectionType::verse:
            case SectionType::bridge:
            case SectionType::phrase:     return 3;
            case SectionType::preChorus:
            case SectionType::postChorus:
            case SectionType::solo:
            case SectionType::buildUp:    return 4;
            case SectionType::chorus:
            case SectionType::drop:       return 5;
        }

        return 3;
    }
}

/** A named part of the song (intro, verse, chorus...) over whole bars. It only marks the
    structure; it makes no sound. */
struct Section
{
    int startBar = 0;   // bar 0 is the first
    int bars = 8;
    SectionType type = SectionType::verse;
    juce::String name;  // e.g. "Chorus 2"; empty shows the type's name

    int getEndBar() const noexcept { return startBar + bars; }
    juce::String getDisplayName() const { return name.isNotEmpty() ? name : sectionTypes::getName (type); }

    bool operator== (const Section& other) const
    {
        return startBar == other.startBar && bars == other.bars && type == other.type && name == other.name;
    }
};

/** The song's sections, sorted by start, not overlapping (see tidySections()). */
using SectionList = std::vector<Section>;

/** Sorts sections by start and makes them fit together: empty ones go, and a section that runs into
    the next one is shortened to end where that one starts (a later section wins its bars). */
inline SectionList tidySections (SectionList sections)
{
    sections.erase (std::remove_if (sections.begin(), sections.end(), [] (const Section& s) { return s.bars <= 0 || s.startBar < 0; }),
                    sections.end());
    std::stable_sort (sections.begin(), sections.end(), [] (const Section& a, const Section& b) { return a.startBar < b.startBar; });

    SectionList tidy;

    for (auto& s : sections)
    {
        if (! tidy.empty() && tidy.back().startBar == s.startBar)
            tidy.pop_back();   // two at the same bar: the later one wins

        if (! tidy.empty() && tidy.back().getEndBar() > s.startBar)
            tidy.back().bars = s.startBar - tidy.back().startBar;

        tidy.push_back (s);
    }

    return tidy;
}

/** Puts new sections in, in place of the ones over the same bars (from the first new section's
    start to the last one's end); sections outside those bars stay. */
inline SectionList replaceSections (SectionList sections, const SectionList& incoming)
{
    if (incoming.empty())
        return sections;

    const int from = incoming.front().startBar, to = incoming.back().getEndBar();
    sections.erase (std::remove_if (sections.begin(), sections.end(), [from, to] (const Section& s) { return s.startBar < to && s.getEndBar() > from; }),
                    sections.end());
    sections.insert (sections.end(), incoming.begin(), incoming.end());
    return tidySections (std::move (sections));
}
} // namespace daw
