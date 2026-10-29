#pragma once

#include "SongTemplates.h"

namespace daw::gen::stylesguide
{
/** The pages of the Styles of music guide, in reading order. */
enum class Topic
{
    styles, parts, routing,
    popBallad, dancePop, rock, hipHop, trap, house, techno, trance, drumAndBass, loFi, blues, reggaeton, afrobeats, filmScore,
    yourSong
};

constexpr Topic allTopics[] {
    Topic::styles, Topic::parts, Topic::routing,
    Topic::popBallad, Topic::dancePop, Topic::rock, Topic::hipHop, Topic::trap, Topic::house, Topic::techno, Topic::trance,
    Topic::drumAndBass, Topic::loFi, Topic::blues, Topic::reggaeton, Topic::afrobeats, Topic::filmScore, Topic::yourSong
};

/** The title shown in the topic list. */
juce::String getTopicName (Topic);

/** The one-line summary shown under a topic. */
juce::String getTopicSummary (Topic);

/** The stable style id for a style topic, or empty for the four general topics. */
juce::String getStyleId (Topic);

/** The final beginner-facing text for a topic. Your song appends observations from the edit. */
juce::String explain (Topic, const Engine&);

/** The template picture behind a topic. General pages use Dance pop; style pages use that style. */
templates::SongPlan getExample (Topic);

/** Concrete observations about the song's tempo, structure, harmony, energy and routing. */
juce::String analyse (const Engine&);
} // namespace daw::gen::stylesguide
