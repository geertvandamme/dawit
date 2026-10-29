#pragma once

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace daw
{
/** A change of tempo: from `beat` on, the song plays at `bpm`, until the next change. */
struct TempoChange
{
    double beat = 0.0;
    double bpm = 120.0;

    bool operator== (const TempoChange& other) const
    {
        return juce::exactlyEqual (beat, other.beat) && juce::exactlyEqual (bpm, other.bpm);
    }
};

/**
    The song's tempo over time: one tempo for the whole song, or a tempo per stretch of bars, as
    when the song follows a band that played without a click. The timeline is in beats; this says
    when each beat is heard, so audio clips (which last a number of seconds), the time display
    and playback can turn beats into seconds and back.

    There is always a tempo: the first change is at beat 0, and before it the first tempo carries
    on. Between changes the tempo is constant.
*/
class TempoMap
{
public:
    /** One tempo for the whole song. */
    explicit TempoMap (double bpm = 120.0);

    /** Tempo changes, tidied: sorted, the first at beat 0, each BPM within 20 to 300, and a change
        to the tempo already playing dropped. An empty list is 120 BPM. */
    explicit TempoMap (std::vector<TempoChange>);

    static constexpr double minBpm = 20.0, maxBpm = 300.0;

    /** The changes, at least one, the first at beat 0. */
    const std::vector<TempoChange>& getChanges() const noexcept { return changes; }

    /** True for one tempo all through the song. */
    bool isSteady() const noexcept { return changes.size() == 1; }

    double getBpmAt (double beat) const noexcept;

    /** The beat of the first change after `beat`, or nullopt when the tempo stays as it is. */
    std::optional<double> getNextChangeAfter (double beat) const noexcept;

    /** When a beat is heard, in seconds from beat 0 (negative before it). */
    double beatToSeconds (double beat) const noexcept;

    /** The beat heard at a time, in seconds from beat 0. */
    double secondsToBeat (double seconds) const noexcept;

    /** How long it takes to play from one beat to another. */
    double getSecondsBetween (double fromBeat, double toBeat) const noexcept { return beatToSeconds (toBeat) - beatToSeconds (fromBeat); }

    /** The beat reached `seconds` after `fromBeat`, as at the end of an audio clip that starts there. */
    double getBeatAfter (double fromBeat, double seconds) const noexcept { return secondsToBeat (beatToSeconds (fromBeat) + seconds); }

    /** The slowest and fastest tempo between two beats. */
    juce::Range<double> getRange (double fromBeat, double toBeat) const;

    /** The average tempo between two beats: the beats played divided by the minutes it takes. */
    double getAverageBpm (double fromBeat, double toBeat) const noexcept;

    bool operator== (const TempoMap& other) const { return changes == other.changes; }
    bool operator!= (const TempoMap& other) const { return ! operator== (other); }

private:
    std::vector<TempoChange> changes;
    std::vector<double> changeSeconds;   // when each change starts

    size_t indexAtBeat (double beat) const noexcept;
};
} // namespace daw
