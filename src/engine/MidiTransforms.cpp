#include "MidiTransforms.h"

#include <algorithm>
#include <random>

namespace daw::miditransform
{
juce::String getKindName (Kind kind)
{
    switch (kind)
    {
        case Kind::reverseInTime: return "Reverse in time";
        case Kind::invertInKey:   return "Reverse in pitch, in the key";
        case Kind::invertExactly: return "Reverse in pitch, exactly";
        case Kind::randomise:     return "Randomise pitch";
    }

    return {};
}

//==============================================================================
namespace
{
    /** Every pitch of the key from MIDI note 0 to 127. */
    std::vector<int> getAllKeyPitches (const MusicalKey& key)
    {
        return getKeyPitches (key, 0, 127);
    }

    /** The index in `all` of the key pitch nearest to `pitch`, ties to the lower. */
    int getStepOfSnapped (const std::vector<int>& all, int pitch)
    {
        const auto it = std::lower_bound (all.begin(), all.end(), pitch);
        const int above = (int) (it - all.begin());

        if (above >= (int) all.size())
            return (int) all.size() - 1;

        if (all[(size_t) above] == pitch || above == 0)
            return above;

        return (pitch - all[(size_t) above - 1] <= all[(size_t) above] - pitch) ? above - 1 : above;
    }

    /** One uniform number below `count`, from the raw output of mt19937 (not std::uniform_int_distribution,
        which gives other numbers in other standard libraries). */
    int pickBelow (std::mt19937& generator, int count)
    {
        return (int) (((uint64_t) generator() * (uint64_t) count) >> 32);
    }
}

int snapToKey (int pitch, const MusicalKey& key)
{
    pitch = juce::jlimit (0, 127, pitch);

    if (key.contains (pitch))
        return pitch;

    for (int d = 1; d < 12; ++d)
    {
        if (pitch - d >= 0 && key.contains (pitch - d))
            return pitch - d;

        if (pitch + d <= 127 && key.contains (pitch + d))
            return pitch + d;
    }

    return pitch;
}

std::vector<int> getKeyPitches (const MusicalKey& key, int lowest, int highest)
{
    std::vector<int> result;

    for (int p = juce::jmax (0, lowest); p <= juce::jmin (127, highest); ++p)
        if (key.contains (p))
            result.push_back (p);

    return result;
}

int getScaleStep (int pitch, const MusicalKey& key)
{
    return getStepOfSnapped (getAllKeyPitches (key), juce::jlimit (0, 127, pitch));
}

int getPitchOfStep (int step, const MusicalKey& key)
{
    const auto all = getAllKeyPitches (key);
    return all.empty() ? 60 : all[(size_t) juce::jlimit (0, (int) all.size() - 1, step)];
}

Range getRandomRange (int lowestPitch, int highestPitch)
{
    Range r { juce::jlimit (0, 127, lowestPitch), juce::jlimit (0, 127, highestPitch) };

    if (r.highest - r.lowest < 12)
        r.highest = juce::jmin (127, r.lowest + 12);

    return r;
}

//==============================================================================
std::vector<int> getAffected (const Notes& notes, const Selection& selection, double clipLength)
{
    std::vector<int> result;

    for (int i = 0; i < (int) notes.size(); ++i)
        if ((selection.isEmpty() || selection.contains (i)) && notes[(size_t) i].start < clipLength)
            result.push_back (i);

    return result;
}

Range getPitchRange (const Notes& notes, const Selection& selection, double clipLength)
{
    Range r { 127, 0 };
    const auto affected = getAffected (notes, selection, clipLength);

    for (int i : affected)
    {
        r.lowest = juce::jmin (r.lowest, notes[(size_t) i].pitch);
        r.highest = juce::jmax (r.highest, notes[(size_t) i].pitch);
    }

    return affected.empty() ? Range {} : r;
}

int countDistinctPitches (const Notes& notes, const Selection& selection, double clipLength)
{
    std::vector<int> pitches;

    for (int i : getAffected (notes, selection, clipLength))
        pitches.push_back (notes[(size_t) i].pitch);

    std::sort (pitches.begin(), pitches.end());
    return (int) (std::unique (pitches.begin(), pitches.end()) - pitches.begin());
}

Notes reverseInTime (const Notes& notes, const Selection& selection, double clipLength)
{
    auto result = notes;
    const auto affected = getAffected (notes, selection, clipLength);

    if (affected.empty())
        return result;

    // A note ringing past the end is shortened first, so the mirror stays inside the clip.
    for (int i : affected)
    {
        auto& n = result[(size_t) i];
        n.length = juce::jmin (n.length, clipLength - n.start);
    }

    double from = 0.0, to = clipLength;

    if (! selection.isEmpty())
    {
        from = result[(size_t) affected.front()].start;
        to = result[(size_t) affected.front()].getEnd();

        for (int i : affected)
        {
            from = juce::jmin (from, result[(size_t) i].start);
            to = juce::jmax (to, result[(size_t) i].getEnd());
        }
    }

    for (int i : affected)
    {
        auto& n = result[(size_t) i];
        n.start = juce::jmax (0.0, from + to - n.getEnd());
    }

    return result;
}

Notes invertExactly (const Notes& notes, const Selection& selection, double clipLength)
{
    auto result = notes;

    if (countDistinctPitches (notes, selection, clipLength) < 2)
        return result;

    const auto range = getPitchRange (notes, selection, clipLength);

    for (int i : getAffected (notes, selection, clipLength))
        result[(size_t) i].pitch = juce::jlimit (0, 127, range.lowest + range.highest - notes[(size_t) i].pitch);

    return result;
}

Notes invertInKey (const Notes& notes, const Selection& selection, double clipLength, const MusicalKey& key)
{
    auto result = notes;

    if (countDistinctPitches (notes, selection, clipLength) < 2)
        return result;

    const auto all = getAllKeyPitches (key);
    const auto affected = getAffected (notes, selection, clipLength);
    int lowStep = (int) all.size(), highStep = 0;

    for (int i : affected)
    {
        const int step = getStepOfSnapped (all, notes[(size_t) i].pitch);
        lowStep = juce::jmin (lowStep, step);
        highStep = juce::jmax (highStep, step);
    }

    for (int i : affected)
    {
        const int step = getStepOfSnapped (all, notes[(size_t) i].pitch);
        result[(size_t) i].pitch = all[(size_t) juce::jlimit (0, (int) all.size() - 1, lowStep + highStep - step)];
    }

    return result;
}

Notes randomise (const Notes& notes, const Selection& selection, double clipLength, const MusicalKey& key,
                 const RandomOptions& options, uint32_t seed)
{
    auto result = notes;
    auto affected = getAffected (notes, selection, clipLength);

    if (affected.empty())
        return result;

    const auto all = getAllKeyPitches (key);
    const auto range = [&]
    {
        const auto own = getPitchRange (notes, selection, clipLength);
        return getRandomRange (own.lowest, own.highest);
    }();

    // The scale steps that are in range: a contiguous run of the key's pitches.
    const int lastStep = (int) all.size() - 1;
    const int cLow = juce::jmin (lastStep, (int) (std::lower_bound (all.begin(), all.end(), range.lowest) - all.begin()));
    const int cHigh = juce::jmax (cLow, (int) (std::upper_bound (all.begin(), all.end(), range.highest) - all.begin()) - 1);

    // Order the notes by onset and group chords (starts closer than the window to the group's first).
    std::stable_sort (affected.begin(), affected.end(),
                      [&notes] (int a, int b) { return notes[(size_t) a].start < notes[(size_t) b].start; });

    std::vector<std::vector<int>> groups;

    for (int i : affected)
    {
        const bool joins = options.chordsTogether && ! groups.empty()
                           && notes[(size_t) i].start - notes[(size_t) groups.back().front()].start < chordWindowBeats;

        if (joins)
            groups.back().push_back (i);
        else
            groups.push_back ({ i });
    }

    std::mt19937 generator (seed);
    int previousLowest = -1;

    for (auto& group : groups)
    {
        std::vector<int> oldSteps;
        for (int i : group)
            oldSteps.push_back (getStepOfSnapped (all, notes[(size_t) i].pitch));

        const int oldLow = *std::min_element (oldSteps.begin(), oldSteps.end());
        const int oldHigh = *std::max_element (oldSteps.begin(), oldSteps.end());

        // Every shift of the group, in scale steps, that keeps all of it in range.
        const auto collect = [&] (bool avoidRepeats)
        {
            std::vector<int> shifts;

            for (int d = cLow - oldLow; d <= cHigh - oldHigh; ++d)
            {
                if (options.stayNear > 0 && std::abs (d) > options.stayNear)
                    continue;

                if (avoidRepeats && previousLowest >= 0 && all[(size_t) (oldLow + d)] == previousLowest)
                    continue;

                shifts.push_back (d);
            }

            return shifts;
        };

        auto shifts = collect (options.avoidRepeats);

        if (shifts.empty() && options.avoidRepeats)
            shifts = collect (false);

        const int shift = shifts.empty() ? 0 : shifts[(size_t) pickBelow (generator, (int) shifts.size())];
        int newLow = lastStep;

        for (size_t k = 0; k < group.size(); ++k)
        {
            // With nothing allowed, each note goes to the nearest in-range note of the key.
            const int step = shifts.empty() ? juce::jlimit (cLow, cHigh, oldSteps[k]) : oldSteps[k] + shift;
            result[(size_t) group[k]].pitch = all[(size_t) step];
            newLow = juce::jmin (newLow, step);
        }

        previousLowest = all[(size_t) newLow];
    }

    return result;
}

Notes apply (Kind kind, const Notes& notes, const Selection& selection, double clipLength, const MusicalKey& key,
             const RandomOptions& options, uint32_t seed)
{
    switch (kind)
    {
        case Kind::reverseInTime: return reverseInTime (notes, selection, clipLength);
        case Kind::invertInKey:   return invertInKey (notes, selection, clipLength, key);
        case Kind::invertExactly: return invertExactly (notes, selection, clipLength);
        case Kind::randomise:     return randomise (notes, selection, clipLength, key, options, seed);
    }

    return notes;
}

bool sameNotes (const Notes& a, const Notes& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
        if (a[i].pitch != b[i].pitch || ! juce::exactlyEqual (a[i].start, b[i].start)
            || ! juce::exactlyEqual (a[i].length, b[i].length) || ! juce::exactlyEqual (a[i].velocity, b[i].velocity))
            return false;

    return true;
}
} // namespace daw::miditransform
