#pragma once

#include "Harmony.h"
#include <random>
#include <vector>

namespace daw::gen
{
/** A random starting progression in a key, one chord per bar, without any AI model. */
namespace chordroll
{
    /** The scale degrees (1 to 6, never the diminished 7th) for `numBars` bars. The first bar is the home chord
        (70%) or the 4th or 6th; the last bar of a run of 4 or more is the 5th (60%) or the home chord; the bars
        between lean to the 4th and 5th, then the 6th and 2nd, then the 1st and 3rd. A chord never repeats
        directly. The same seed gives the same result. */
    std::vector<int> pickDegrees (int numBars, unsigned seed);

    /** The in-key triads for `pickDegrees (numBars, seed)`. */
    std::vector<Chord> generate (const MusicalKey&, int numBars, unsigned seed);
}
}
