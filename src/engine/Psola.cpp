#include "Psola.h"

namespace daw::tune
{
void PsolaShifter::prepare (double sampleRate, int latencySamples)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    latency = juce::jmax (64, latencySamples);
    maxPeriod = (int) ((double) latency / 2.9);
    fadeStep = (float) (1.0 / (0.005 * rate));

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        ring[ch].assign ((size_t) ringSize, 0.0f);
        acc[ch].assign ((size_t) ringSize, 0.0f);
    }

    mid.assign ((size_t) ringSize, 0.0f);
    norm.assign ((size_t) ringSize, 0.0f);
    reset();
}

void PsolaShifter::reset()
{
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        std::fill (ring[ch].begin(), ring[ch].end(), 0.0f);
        std::fill (acc[ch].begin(), acc[ch].end(), 0.0f);
    }

    std::fill (mid.begin(), mid.end(), 0.0f);
    std::fill (norm.begin(), norm.end(), 0.0f);
    count = 0;
    scanFrom = 0;
    chainActive = false;
    numMarks = 0;
    wetWeight = 0.0f;
}

bool PsolaShifter::isWet (juce::int64 sample, const PsolaControls& controls) const
{
    const float p = controls.getPeriod (sample);
    return p > 0.0f && p <= (float) maxPeriod && std::abs (controls.getRatio (sample) - 1.0f) > minRatioChange;
}

void PsolaShifter::addMarksUntil (juce::int64 limit, const PsolaControls& controls)
{
    while (numMarks > 0 && marks[numMarks - 1] < limit)
    {
        const juce::int64 last = marks[numMarks - 1];
        float p = controls.getPeriod (last);

        if (p <= 0.0f)
            p = lastPeriod;

        lastPeriod = p;
        const int period = juce::jmax (8, juce::roundToInt (p));
        const juce::int64 predicted = last + period, quarter = period / 4;
        juce::int64 best = predicted;
        float bestValue = -1.0e30f;

        for (juce::int64 s = predicted - quarter; s <= predicted + quarter; ++s)
        {
            if (s <= last + period / 2 || s >= count)
                continue;

            const float v = polarity * getMid (s);

            if (v > bestValue)
            {
                bestValue = v;
                best = s;
            }
        }

        if (numMarks == 128)
        {
            std::copy (marks + 64, marks + 128, marks);
            numMarks = 64;
        }

        marks[numMarks++] = best;
    }
}

void PsolaShifter::emitGrain (juce::int64 sigma, int half, juce::int64 finalFrom, const PsolaControls& controls)
{
    addMarksUntil (sigma, controls);

    // The mark nearest to where the grain goes.
    juce::int64 alpha = marks[0];

    for (int i = 1; i < numMarks; ++i)
        if (std::abs (marks[i] - sigma) < std::abs (alpha - sigma))
            alpha = marks[i];

    const double fullCycle = juce::MathConstants<double>::pi / (double) half;

    for (int k = -half; k <= half; ++k)
    {
        const juce::int64 to = sigma + k, from = alpha + k;

        if (to < finalFrom || from < 0 || from >= count || count - from > ringSize - 8)
            continue;

        const float w = (float) (0.5 + 0.5 * std::cos (fullCycle * k));
        const auto t = (size_t) (to & mask), f = (size_t) (from & mask);

        for (int ch = 0; ch < maxChannels; ++ch)
            acc[ch][t] += w * ring[ch][f];

        norm[t] += w;
    }
}

void PsolaShifter::generateGrains (juce::int64 finalFrom, juce::int64 uEnd, const PsolaControls& controls)
{
    for (int guard = 0; guard < 4096; ++guard)
    {
        if (! chainActive)
        {
            juce::int64 x = juce::jmax (scanFrom, finalFrom);

            while (x < uEnd && ! isWet (x, controls))
                x += 8;

            scanFrom = juce::jmax (scanFrom, juce::jmin (x, uEnd));

            if (x >= uEnd)
                return;

            // A new chain: the first mark is the strongest point of the cycle around x.
            const int period = juce::roundToInt (controls.getPeriod (x));
            float peak = 0.0f;

            for (juce::int64 s = x - period; s <= x + period; ++s)
                if (s >= 0 && s < count && std::abs (getMid (s)) > std::abs (peak))
                    peak = getMid (s);

            polarity = peak < 0.0f ? -1.0f : 1.0f;
            juce::int64 best = x;
            float bestValue = -1.0e30f;

            for (juce::int64 s = x - period / 2; s <= x + period / 2; ++s)
                if (s >= 0 && s < count && polarity * getMid (s) > bestValue)
                {
                    bestValue = polarity * getMid (s);
                    best = s;
                }

            marks[0] = best;
            numMarks = 1;
            lastPeriod = (float) period;
            nextSynth = (double) x;
            chainActive = true;
        }

        const juce::int64 sigma = (juce::int64) std::llround (nextSynth);
        const float p = controls.getPeriod (sigma);
        const float r = controls.getRatio (sigma);

        if (p <= 0.0f || p > (float) maxPeriod || std::abs (r - 1.0f) <= minRatioChange)
        {
            chainActive = false;
            scanFrom = juce::jmax (scanFrom, sigma + 1);
            continue;
        }

        const int half = juce::jmax (8, juce::roundToInt (p));

        if (sigma - half > uEnd - 1)
            return;

        emitGrain (sigma, half, finalFrom, controls);
        nextSynth += (double) p / (double) r;
    }
}

void PsolaShifter::process (const float* const* in, float* const* out, int numChannels, int n, const PsolaControls& controls)
{
    push (in, numChannels, n);
    pull (out, numChannels, n, controls);
}

void PsolaShifter::push (const float* const* in, int numChannels, int n)
{
    const int channels = juce::jmin (numChannels, maxChannels);

    for (int i = 0; i < n; ++i)
    {
        const auto at = (size_t) ((count + i) & mask);
        float sum = 0.0f;

        for (int ch = 0; ch < maxChannels; ++ch)
        {
            const float v = in[juce::jmin (ch, channels - 1)][i];
            ring[ch][at] = v;
            sum += v;
        }

        mid[at] = sum * 0.5f;
    }

    count += n;
}

void PsolaShifter::copyMid (juce::int64 end, float* dest, int n) const noexcept
{
    for (int i = 0; i < n; ++i)
    {
        const juce::int64 s = end - n + i;
        dest[i] = s >= 0 ? mid[(size_t) (s & mask)] : 0.0f;
    }
}

void PsolaShifter::pull (float* const* out, int numChannels, int n, const PsolaControls& controls)
{
    const int channels = juce::jmin (numChannels, maxChannels);
    const juce::int64 uStart = count - n - latency, uEnd = count - latency;
    generateGrains (uStart, uEnd, controls);

    for (int i = 0; i < n; ++i)
    {
        const juce::int64 u = uStart + i;

        if (u < 0)
        {
            for (int ch = 0; ch < channels; ++ch)
                out[ch][i] = 0.0f;

            continue;
        }

        const auto at = (size_t) (u & mask);
        const float target = isWet (u, controls) ? 1.0f : 0.0f;
        wetWeight = target > wetWeight ? juce::jmin (target, wetWeight + fadeStep) : juce::jmax (target, wetWeight - fadeStep);
        const float scale = 1.0f / juce::jmax (norm[at], 0.5f);

        for (int ch = 0; ch < channels; ++ch)
        {
            const float dry = ring[ch][at];
            out[ch][i] = wetWeight > 0.0f ? dry + wetWeight * (acc[ch][at] * scale - dry) : dry;
        }

        for (int ch = 0; ch < maxChannels; ++ch)
            acc[ch][at] = 0.0f;

        norm[at] = 0.0f;
    }
}

void PsolaShifter::bypass (const float* const* in, float* const* out, int numChannels, int n)
{
    const juce::int64 t0 = count;
    const int channels = juce::jmin (numChannels, maxChannels);

    for (int i = 0; i < n; ++i)
    {
        const auto at = (size_t) ((t0 + i) & mask);
        float sum = 0.0f;

        for (int ch = 0; ch < maxChannels; ++ch)
        {
            const float v = in[juce::jmin (ch, channels - 1)][i];
            ring[ch][at] = v;
            sum += v;
        }

        mid[at] = sum * 0.5f;
        const juce::int64 u = t0 + i - latency;

        for (int ch = 0; ch < channels; ++ch)
            out[ch][i] = u >= 0 ? ring[ch][(size_t) (u & mask)] : 0.0f;

        const auto cleared = (size_t) (u & mask);

        for (int ch = 0; ch < maxChannels; ++ch)
            acc[ch][cleared] = 0.0f;

        norm[cleared] = 0.0f;
    }

    count += n;
    chainActive = false;
    numMarks = 0;
    wetWeight = 0.0f;
    scanFrom = count - latency;
}
}
