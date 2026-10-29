#include "OnsetFinder.h"

#include <juce_dsp/juce_dsp.h>

namespace daw::analysis::onsets
{
namespace
{
    constexpr int fftOrder = 9;                 // 512 samples: 32 ms at 16 kHz
    constexpr double hopSeconds = 0.01;
    constexpr double lowestHz = 100.0, highestHz = 5000.0;
    constexpr double quietestDb = -40.0;        // rises this far under the loudest part don't count
    constexpr double closestOnsets = 0.06;
    constexpr double averageSeconds = 0.25;     // a rise must beat the average flux this far around it
    constexpr double minimumRiseShare = 0.08;   // ... by this share of the biggest rise
    constexpr double latencySeconds = -0.004;     // measured on bursts with known starts (see the tests)
}

std::vector<double> findOnsets (const juce::AudioBuffer<float>& mono, double rate)
{
    const int size = 1 << fftOrder;
    const int hop = juce::jmax (1, juce::roundToInt (rate * hopSeconds));
    const int numSamples = mono.getNumSamples();

    if (numSamples < size + hop || rate <= 0.0)
        return {};

    const int numFrames = (numSamples - size) / hop + 1;
    juce::dsp::FFT fft (fftOrder);
    juce::dsp::WindowingFunction<float> window ((size_t) size, juce::dsp::WindowingFunction<float>::hann, false);
    std::vector<float> data ((size_t) size * 2);

    const int lowBin = juce::jmax (1, (int) (lowestHz * size / rate));
    const int highBin = juce::jmin (size / 2 - 1, (int) (highestHz * size / rate));

    std::vector<float> previous ((size_t) size / 2, 0.0f), now (previous.size());
    std::vector<double> flux ((size_t) numFrames, 0.0), levelDb ((size_t) numFrames, -200.0);
    double loudest = -200.0;
    const float* in = mono.getReadPointer (0);

    for (int f = 0; f < numFrames; ++f)
    {
        std::fill (data.begin(), data.end(), 0.0f);
        std::copy (in + f * hop, in + f * hop + size, data.begin());
        window.multiplyWithWindowingTable (data.data(), (size_t) size);

        double energy = 0.0;

        for (int i = 0; i < size; ++i)
            energy += (double) data[(size_t) i] * data[(size_t) i];

        levelDb[(size_t) f] = energy > 0.0 ? 10.0 * std::log10 (energy / size) : -200.0;
        loudest = juce::jmax (loudest, levelDb[(size_t) f]);

        fft.performFrequencyOnlyForwardTransform (data.data());

        double sum = 0.0;

        for (int b = lowBin; b <= highBin; ++b)
        {
            now[(size_t) b] = std::log1p (1000.0f * data[(size_t) b] * 4.0f / (float) size);   // ~ the amplitude of a sine, compressed
            sum += juce::jmax (0.0f, now[(size_t) b] - previous[(size_t) b]);
        }

        flux[(size_t) f] = f == 0 ? 0.0 : sum;
        previous = now;
    }

    const double biggest = *std::max_element (flux.begin(), flux.end());

    if (biggest <= 0.0)
        return {};

    const int around = juce::jmax (1, juce::roundToInt (averageSeconds / hopSeconds));
    const int neighbourhood = 3;   // a peak is the biggest within 30 ms either side
    std::vector<std::pair<double, double>> found;   // time, strength

    for (int f = 1; f < numFrames - 1; ++f)
    {
        const double v = flux[(size_t) f];

        if (v <= 0.0)
            continue;

        bool peak = true;

        for (int k = juce::jmax (0, f - neighbourhood); k <= juce::jmin (numFrames - 1, f + neighbourhood) && peak; ++k)
            peak = flux[(size_t) k] < v || k == f || (flux[(size_t) k] <= v && k > f);

        if (! peak)
            continue;

        double mean = 0.0;
        const int from = juce::jmax (0, f - around), to = juce::jmin (numFrames - 1, f + around);

        for (int k = from; k <= to; ++k)
            mean += flux[(size_t) k];

        mean /= (to - from + 1);

        // Judged by the level a little later, when the sound has come in.
        const int later = juce::jmin (numFrames - 1, f + 2);

        // An onset doesn't make the sound quieter: the spectrum also shifts as a note ends, which isn't one.
        if (v < mean + minimumRiseShare * biggest || levelDb[(size_t) later] < loudest + quietestDb
            || levelDb[(size_t) later] < levelDb[(size_t) (f - 1)] - 1.0)
            continue;

        // The window that shows the rise is centred half a window after the frame's start.
        found.push_back ({ (f * hop + size * 0.5) / rate - latencySeconds, v });
    }

    std::vector<double> times;
    std::vector<double> strengths;

    for (const auto& [t, strength] : found)
    {
        if (! times.empty() && t - times.back() < closestOnsets)
        {
            if (strength > strengths.back())
            {
                times.back() = t;
                strengths.back() = strength;
            }

            continue;
        }

        times.push_back (t);
        strengths.push_back (strength);
    }

    return times;
}
} // namespace daw::analysis::onsets
