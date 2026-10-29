#include "TempoMap.h"

namespace daw
{
namespace
{
    // Positions this close count as the same beat, so a render that stops a hair before a change
    // doesn't play a sliver at the old tempo.
    constexpr double sameBeat = 1.0e-9;
}

TempoMap::TempoMap (double bpm) : TempoMap (std::vector<TempoChange> { { 0.0, bpm } }) {}

TempoMap::TempoMap (std::vector<TempoChange> newChanges)
{
    std::stable_sort (newChanges.begin(), newChanges.end(), [] (const TempoChange& a, const TempoChange& b) { return a.beat < b.beat; });

    for (auto change : newChanges)
    {
        change.bpm = juce::jlimit (minBpm, maxBpm, std::isfinite (change.bpm) ? change.bpm : 120.0);

        if (changes.empty())
        {
            changes.push_back ({ 0.0, change.bpm });   // the first tempo plays from the start
            continue;
        }

        if (change.beat <= changes.back().beat + sameBeat)
            changes.back().bpm = change.bpm;   // two at the same beat: the later one counts
        else if (! juce::exactlyEqual (change.bpm, changes.back().bpm))
            changes.push_back (change);
    }

    if (changes.empty())
        changes.push_back ({ 0.0, 120.0 });

    // Two at the same beat can leave a repeat behind.
    changes.erase (std::unique (changes.begin(), changes.end(), [] (const TempoChange& a, const TempoChange& b) { return juce::exactlyEqual (a.bpm, b.bpm); }),
                   changes.end());

    changeSeconds.resize (changes.size());
    changeSeconds[0] = 0.0;

    for (size_t i = 1; i < changes.size(); ++i)
        changeSeconds[i] = changeSeconds[i - 1] + (changes[i].beat - changes[i - 1].beat) * 60.0 / changes[i - 1].bpm;
}

size_t TempoMap::indexAtBeat (double beat) const noexcept
{
    // The last change at or before the beat; the first one for beats before 0.
    const auto it = std::upper_bound (changes.begin(), changes.end(), beat + sameBeat, [] (double b, const TempoChange& c) { return b < c.beat; });
    return it == changes.begin() ? 0 : (size_t) (it - changes.begin()) - 1;
}

double TempoMap::getBpmAt (double beat) const noexcept
{
    return changes[indexAtBeat (beat)].bpm;
}

std::optional<double> TempoMap::getNextChangeAfter (double beat) const noexcept
{
    const auto it = std::upper_bound (changes.begin(), changes.end(), beat + sameBeat, [] (double b, const TempoChange& c) { return b < c.beat; });
    return it != changes.end() ? std::optional<double> (it->beat) : std::nullopt;
}

double TempoMap::beatToSeconds (double beat) const noexcept
{
    const auto i = indexAtBeat (beat);
    return changeSeconds[i] + (beat - changes[i].beat) * 60.0 / changes[i].bpm;
}

double TempoMap::secondsToBeat (double time) const noexcept
{
    const auto it = std::upper_bound (changeSeconds.begin(), changeSeconds.end(), time);
    const auto i = it == changeSeconds.begin() ? 0 : (size_t) (it - changeSeconds.begin()) - 1;
    return changes[i].beat + (time - changeSeconds[i]) * changes[i].bpm / 60.0;
}

juce::Range<double> TempoMap::getRange (double fromBeat, double toBeat) const
{
    auto range = juce::Range<double>::withStartAndLength (getBpmAt (fromBeat), 0.0);

    for (auto& c : changes)
        if (c.beat > fromBeat && c.beat < toBeat)
            range = range.getUnionWith (c.bpm);

    return range;
}

double TempoMap::getAverageBpm (double fromBeat, double toBeat) const noexcept
{
    const double minutes = getSecondsBetween (fromBeat, toBeat) / 60.0;
    return minutes > 0.0 ? (toBeat - fromBeat) / minutes : getBpmAt (fromBeat);
}
} // namespace daw
