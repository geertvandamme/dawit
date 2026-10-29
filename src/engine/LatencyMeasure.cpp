#include "LatencyMeasure.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>

namespace daw::latency
{
std::vector<float> makeClick (double sampleRate)
{
    const int length = juce::jmax (8, (int) std::lround (0.004 * sampleRate));
    const double tau = 0.0006 * sampleRate;
    const float peak = juce::Decibels::decibelsToGain (clickLevelDb);
    std::vector<float> click ((size_t) length);

    for (int i = 0; i < length; ++i)
        click[(size_t) i] = peak * (float) (std::exp (-i / tau) * std::cos (juce::MathConstants<double>::twoPi * 2500.0 * i / sampleRate));

    return click;
}

int getClickStart (int index, double sampleRate)
{
    return (int) std::lround ((leadSeconds + index * spacingSeconds) * sampleRate);
}

int getTotalSamples (double sampleRate)
{
    return (int) std::lround ((leadSeconds + numClicks * spacingSeconds + tailSeconds) * sampleRate);
}

Result findRoundTrip (const std::vector<float>& heard, double sampleRate)
{
    constexpr float absoluteMinimum = 0.002f;   // -54 dB
    struct Window { int start = 0, length = 0; float peak = 0.0f; };
    std::vector<Window> windows;
    float loudest = 0.0f;

    for (int i = 0; i < numClicks; ++i)
    {
        Window w;
        w.start = getClickStart (i, sampleRate);
        w.length = juce::jmin ((int) heard.size() - w.start, (int) std::lround (spacingSeconds * sampleRate));

        for (int n = 0; n < w.length; ++n)
            w.peak = juce::jmax (w.peak, std::abs (heard[(size_t) (w.start + n)]));

        loudest = juce::jmax (loudest, w.peak);
        windows.push_back (w);
    }

    std::vector<double> delays;

    for (auto& w : windows)
    {
        if (w.length <= 0 || w.peak < absoluteMinimum || w.peak < 0.2f * loudest)
            continue;

        for (int n = 0; n < w.length; ++n)
            if (std::abs (heard[(size_t) (w.start + n)]) >= 0.25f * w.peak)
            {
                delays.push_back (n);
                break;
            }
    }

    Result result;

    if (delays.empty())
        return result;

    auto sorted = delays;
    std::sort (sorted.begin(), sorted.end());
    const double middle = sorted[sorted.size() / 2];
    std::vector<double> agreeing;

    for (auto d : delays)
        if (std::abs (d - middle) <= agreeSeconds * sampleRate)
            agreeing.push_back (d);

    std::sort (agreeing.begin(), agreeing.end());
    result.clicksFound = (int) agreeing.size();
    result.roundTripSamples = agreeing[agreeing.size() / 2];
    result.ok = result.clicksFound >= minClicksFound;
    return result;
}
} // namespace daw::latency
