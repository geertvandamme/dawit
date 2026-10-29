#pragma once

#include <juce_core/juce_core.h>
#include <optional>
#include <vector>

namespace daw
{
/**
    Gain staging: keeping a sensible level at every step a track's sound goes through, from the
    instrument through each effect and the fader to the master. What the levels mean, what to look
    out for in a chain, and the texts of the guide. Used by the device bar and its guide window.
*/
namespace gainstaging
{
    // Peak levels in dB below full scale.
    constexpr float silentBelowDb = -60.0f, quietBelowDb = -24.0f, hotAboveDb = -6.0f, overAboveDb = 0.0f;

    enum class Zone { silent, quiet, healthy, hot, over };

    Zone getZone (float peakDb);
    juce::String getZoneName (Zone);

    /** "+2.0 dB", "-12.5 dB", or "-inf dB" when silent. */
    juce::String formatDb (float db);

    enum class Topic { what, zones, louder, compressor, fader, bus, over, steps };
    inline constexpr Topic allTopics[] { Topic::what, Topic::zones, Topic::louder, Topic::compressor,
                                         Topic::fader, Topic::bus, Topic::over, Topic::steps };

    juce::String getTopicName (Topic);
    juce::String getTopicSummary (Topic);
    juce::String explain (Topic);

    /** One step of a track's chain, with the levels of the last moment. A bus's own chain starts with `busInput`
        (its tracks added up: no input, out = the sum) instead of an instrument. A track that plays into a bus has a
        `bus` stop after its fader (in = the sum of the bus's tracks, out = after the bus's fader). A return's chain starts
        with a `busInput` too (the sends added up, `isReturn`). A `send` is a branch off the line to a return: it is
        drawn beside the chain, takes no part in the line's levels and gets no hints. */
    enum class StopKind { instrument, busInput, effect, fader, bus, master, send };

    struct Stop
    {
        juce::String name;
        StopKind kind = StopKind::effect;
        float inDb = -100.0f, outDb = -100.0f;    // peaks; an instrument has no input
        std::optional<float> changeDb;            // effects, while they play: how much louder (+) or quieter it makes the sound
        float settingDb = 0.0f;                   // fader and master: their volume
        bool bypassed = false;
        bool canMatch = false, matching = false;  // DawIt EQ and DawIt Compressor: match level
        bool isCompressor = false;
        bool busHasEffects = true;                // busInput and bus: false for a bus with no effects (the hot-sum hint stays quiet then)
        int numTracks = 0;                        // busInput and bus: how many tracks add up there
        bool isReturn = false;                    // busInput: the sends into a return, not the tracks of a bus
        bool preFader = false;                    // send: taken before the fader
        int tapAfter = -1;                        // send: the index of the stop of the line it is taken after
    };

    /** Something in the chain worth a look, about one stop. Serious: it is heard as distortion. */
    struct Hint
    {
        int stop = 0;
        bool serious = false;
        Topic topic = Topic::what;
        juce::String text;
    };

    // When hints show.
    constexpr float veryQuietDb = -40.0f, louderHintDb = 6.0f, fightHintDb = 6.0f;

    /** What stands out in a chain, in chain order. */
    std::vector<Hint> findHints (const std::vector<Stop>&);
} // namespace gainstaging
} // namespace daw
