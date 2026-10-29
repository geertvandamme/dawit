#pragma once

#include "NoteRealizer.h"

namespace daw::gen
{
/** The Variation setting runs from 0 (stay close to what is typical for the style) to 1 (more surprising). */
constexpr double defaultVariation = 0.5;

/** Ollama sampling temperature: 0.4 at no variation, 0.8 at the default, 1.2 at full. */
inline double getOllamaTemperature (double variation)  { return 0.4 + 0.8 * juce::jlimit (0.0, 1.0, variation); }

/** How many random ideas (pickDirections()) each take gets: none at the lowest setting, up to three. */
inline int getDirectionCount (double variation)
{
    return variation < 0.1 ? 0 : juce::jmin (3, 1 + (int) (variation * 3.0));
}

/** How good a take is, from 0 to 1, by simple musical checks of the pattern before it repeats.
    Pitched parts: chord tones on strong beats, a sensible range, density and number of pitches, varied rhythm,
    few big leaps, bars that repeat a little but not exactly. Drums: a kick, a backbeat and something keeping time.
    Drums and percussion: varied velocity and no instrument hammering every 16th at one velocity. */
double scoreTake (const Request&, const Result&);
} // namespace daw::gen
