#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace daw::analysis
{
/** The audio as one channel (the channels averaged) at `targetRate`, as the analysers want it.
    When it goes down in rate, everything above 90% of the new half rate is filtered out first,
    so nothing folds back into the band that's listened to. */
juce::AudioBuffer<float> toMono (const juce::AudioBuffer<float>&, double sourceRate, double targetRate);
} // namespace daw::analysis
