#include "MonoAudio.h"

namespace daw::analysis
{
juce::AudioBuffer<float> toMono (const juce::AudioBuffer<float>& audio, double sourceRate, double targetRate)
{
    const int numIn = audio.getNumSamples();
    const int numChannels = audio.getNumChannels();
    juce::AudioBuffer<float> mono (1, numIn);
    mono.clear();

    for (int ch = 0; ch < numChannels; ++ch)
        mono.addFrom (0, 0, audio, ch, 0, numIn, 1.0f / (float) numChannels);

    if (sourceRate <= 0.0 || juce::exactlyEqual (sourceRate, targetRate))
        return mono;

    if (sourceRate > targetRate)
    {
        for (int stage = 0; stage < 2; ++stage)
        {
            juce::IIRFilter filter;
            filter.setCoefficients (juce::IIRCoefficients::makeLowPass (sourceRate, targetRate * 0.45));
            filter.processSamples (mono.getWritePointer (0), numIn);
        }
    }

    const double ratio = sourceRate / targetRate;
    const int numOut = juce::roundToInt (numIn / ratio);
    juce::AudioBuffer<float> out (1, numOut);
    juce::LagrangeInterpolator resampler;
    resampler.process (ratio, mono.getReadPointer (0), out.getWritePointer (0), numOut, numIn, 0);
    return out;
}
} // namespace daw::analysis
