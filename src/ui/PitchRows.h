#pragma once

#include "engine/MusicalKey.h"

#include <array>
#include <vector>

namespace daw
{
/**
    The rows of the piano roll, top to bottom: all 128 pitches, or only those in the song key.
    The grid and the keyboard beside it share one, so they always line up.
*/
class PitchRows
{
public:
    PitchRows() { showAll(); }

    void showAll() { build ([] (int) { return true; }); }
    void showKey (const MusicalKey& key) { build ([&key] (int pitch) { return key.contains (pitch); }); }

    int getNumRows() const noexcept { return (int) pitches.size(); }
    bool isShown (int pitch) const noexcept { return getRow (pitch) >= 0; }

    /** The row of a pitch, or -1 if it is hidden. */
    int getRow (int pitch) const noexcept { return juce::isPositiveAndBelow (pitch, 128) ? rows[(size_t) pitch] : -1; }

    /** The pitch of a row; rows past either end give the top or bottom pitch. */
    int getPitch (int row) const noexcept { return pitches[(size_t) juce::jlimit (0, getNumRows() - 1, row)]; }

    /** The row of the pitch, or of the closest shown pitch below it (above it at the very bottom). */
    int getNearestRow (int pitch) const noexcept
    {
        for (int p = juce::jlimit (0, 127, pitch); p >= 0; --p)
            if (isShown (p))
                return getRow (p);

        return getNumRows() - 1;
    }

private:
    template <typename Predicate>
    void build (Predicate shouldShow)
    {
        pitches.clear();
        rows.fill (-1);

        for (int p = 127; p >= 0; --p)
        {
            if (shouldShow (p))
            {
                rows[(size_t) p] = (int) pitches.size();
                pitches.push_back (p);
            }
        }
    }

    std::vector<int> pitches;       // top row first
    std::array<int, 128> rows {};
};
} // namespace daw
