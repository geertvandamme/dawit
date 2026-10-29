#include "PitchTracker.h"

namespace daw::pitch
{
int getWindowSize (double sampleRate)
{
    int size = 256;

    while (size < sampleRate * 0.04)
        size *= 2;

    return size;
}

int getTunerWindowSize (double sampleRate)
{
    int size = 256;

    while (size < sampleRate * 0.085)
        size *= 2;

    return size;
}

void Tracker::prepare (double sampleRate, int maxWindow, double lowestHz)
{
    lowest = lowestHz > 1.0 ? lowestHz : 50.0;
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxSize = juce::jmax (64, maxWindow);
    order = 1;

    while ((1 << order) < 2 * maxSize)
        ++order;

    fft = std::make_unique<juce::dsp::FFT> (order);
    data.assign ((size_t) (2 << order), 0.0f);
    squares.assign ((size_t) maxSize + 1, 0.0f);
    nsdf.assign ((size_t) maxSize + 2, 0.0f);
    peaks.reserve (256);
    reset();
}

Reading Tracker::analyse (const float* x, int n, float minHighest, bool guard)
{
    Reading result;
    n = juce::jmin (n, maxSize);
    const int minLag = juce::jmax (2, (int) (rate / 2000.0));
    const int maxLag = juce::jmin (n / 2, (int) (rate / lowest) + 1);

    double energy = 0.0;

    for (int i = 0; i < n; ++i)
        energy += (double) x[i] * x[i];

    result.rmsDb = n > 0 ? juce::Decibels::gainToDecibels ((float) std::sqrt (energy / n), -100.0f) : -100.0f;
    std::fill (nsdf.begin(), nsdf.end(), 0.0f);

    if (maxLag <= minLag + 2 || energy / n < 1.0e-8)
    {
        // Silence: nothing to compare. The octave guard forgets it.
        history[0] = history[1] = -1000.0f;
        return result;
    }

    // The autocorrelation through the FFT, with the audio padded to twice its length so it doesn't wrap round.
    const int size = 1 << order;
    std::fill (data.begin(), data.end(), 0.0f);
    std::copy_n (x, n, data.begin());
    fft->performRealOnlyForwardTransform (data.data(), true);

    for (int bin = 0; bin <= size / 2; ++bin)
    {
        const float re = data[(size_t) bin * 2], im = data[(size_t) bin * 2 + 1];
        data[(size_t) bin * 2] = re * re + im * im;
        data[(size_t) bin * 2 + 1] = 0.0f;
    }

    fft->performRealOnlyInverseTransform (data.data());
    const float scale = data[0] > 0.0f ? (float) (energy / data[0]) : 1.0f;   // whatever the FFT's scaling, lag 0 is the energy

    squares[0] = 0.0f;

    for (int i = 0; i < n; ++i)
        squares[(size_t) i + 1] = squares[(size_t) i] + x[i] * x[i];

    for (int lag = 0; lag <= maxLag + 1 && lag < n; ++lag)
    {
        const double m = (double) squares[(size_t) (n - lag)] + ((double) squares[(size_t) n] - (double) squares[(size_t) lag]);
        nsdf[(size_t) lag] = m > 0.0 ? (float) (2.0 * (double) data[(size_t) lag] * scale / m) : 0.0f;
    }

    // The highest point of every positive stretch after the first dip.
    peaks.clear();
    int lag = 1;

    while (lag <= maxLag && nsdf[(size_t) lag] > 0.0f)
        ++lag;

    while (lag <= maxLag)
    {
        while (lag <= maxLag && nsdf[(size_t) lag] <= 0.0f)
            ++lag;

        int best = -1;

        for (; lag <= maxLag && nsdf[(size_t) lag] > 0.0f; ++lag)
            if (lag >= minLag && (best < 0 || nsdf[(size_t) lag] > nsdf[(size_t) best]))
                best = lag;

        if (best > 0)
            peaks.push_back (best);
    }

    float highest = 0.0f;

    for (const int p : peaks)
        highest = juce::jmax (highest, nsdf[(size_t) p]);

    if (peaks.empty() || highest < minHighest)
        return result;

    // The first peak nearly as high as the highest: the period, rather than two or three of them.
    int chosen = -1;

    for (const int p : peaks)
    {
        if (nsdf[(size_t) p] >= 0.9f * highest)
        {
            chosen = p;
            break;
        }
    }

    if (chosen < 0)
        return result;

    // A weak lowest harmonic makes half the period look nearly as good: when twice the delay matches
    // better still, that is the period.
    for (const int p : peaks)
        if (std::abs (p - 2 * chosen) <= juce::jmax (2, chosen / 25) && nsdf[(size_t) p] >= nsdf[(size_t) chosen] + 0.008f)
        {
            chosen = p;
            break;
        }

    const float a = nsdf[(size_t) chosen - 1], b = nsdf[(size_t) chosen], c = nsdf[(size_t) chosen + 1];
    const float denominator = a - 2.0f * b + c;
    const float shift = std::abs (denominator) > 1.0e-9f ? juce::jlimit (-0.5f, 0.5f, 0.5f * (a - c) / denominator) : 0.0f;
    const double hz = rate / ((double) chosen + shift);

    if (hz < lowest || hz > 2000.0)
        return result;

    result.clarity = juce::jlimit (0.0f, 1.0f, b - 0.25f * (a - c) * shift);
    result.midi = midiFromHz (hz);
    result.voiced = result.clarity >= minHighest && result.rmsDb >= minRmsDb;

    if (guard)
    {
        // A single reading an octave away from two that agree is a slip of the method, not a note.
        const bool agree = history[1] > -500.0f && std::abs (history[0] - history[1]) < 1.0f;
        const bool octaveAway = agree && std::abs (std::abs (result.midi - history[0]) - 12.0f) < 1.0f;
        history[1] = history[0];
        history[0] = result.voiced ? result.midi : -1000.0f;

        if (octaveAway)
            result.voiced = false;
    }

    return result;
}
}
