#include "HitFinder.h"
#include "MonoAudio.h"
#include "TempoFinder.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

namespace daw::analysis::hits
{
namespace
{
    namespace tf = tempofinder;

    constexpr int fftOrder = 8;
    constexpr int fftSize = 1 << fftOrder;       // 11.6 ms windows: two hits 12 ms apart are two peaks
    constexpr int numBands = 40;
    constexpr double lowestHz = 30.0, highestHz = 11000.0;
    constexpr int fluxLag = 2;                   // compares with 11.6 ms earlier
    constexpr float riseFloor = 8.0f;            // the smallest rise (dB summed over the bands) that's a candidate
    constexpr float quietestDb = -80.0f;         // candidates under this are noise
    constexpr double lookBack = 0.015, lookForward = 0.005, peakWindow = 0.030, energyWindow = 0.050;
    constexpr double onsetWindow = 0.010, beforeWindow = 0.008;
    constexpr float onsetRatio = 1.4f;           // a hit's change is at least this much more than just before it
    constexpr float sharpRatio = 3.0f;
    constexpr double nearSeconds = 0.006;
    constexpr double lowEdgeHz = 150.0, highEdgeHz = 6000.0;

    float toDb (float amplitude)
    {
        return 20.0f * std::log10 (juce::jmax (amplitude, 1.0e-6f));
    }

    float peakOf (const juce::AudioBuffer<float>& audio, int from, int to)
    {
        float peak = 0.0f;
        from = juce::jmax (0, from);
        to = juce::jmin (audio.getNumSamples(), to);

        for (int c = 0; c < audio.getNumChannels(); ++c)
        {
            const float* d = audio.getReadPointer (c);

            for (int i = from; i < to; ++i)
                peak = juce::jmax (peak, std::abs (d[i]));
        }

        return peak;
    }

    /** How fast the sound changes at sample i (the larger of the channels' first differences): a bass note
        that's already sounding hardly shows in it, so a hit on top of one still stands out. */
    float changeAt (const juce::AudioBuffer<float>& audio, int i)
    {
        float change = 0.0f;

        if (i >= 1 && i < audio.getNumSamples())
            for (int c = 0; c < audio.getNumChannels(); ++c)
                change = juce::jmax (change, std::abs (audio.getReadPointer (c)[i] - audio.getReadPointer (c)[i - 1]));

        return change;
    }

    float peakChange (const juce::AudioBuffer<float>& audio, int from, int to)
    {
        float peak = 0.0f;

        for (int i = juce::jmax (1, from); i < juce::jmin (audio.getNumSamples(), to); ++i)
            peak = juce::jmax (peak, changeAt (audio, i));

        return peak;
    }

    /** The start of the rise: the first sample, between `from` and `to`, whose change reaches a tenth of `reference`. */
    int findStart (const juce::AudioBuffer<float>& audio, int from, int to, float reference, int fallback)
    {
        for (int i = juce::jmax (1, from); i < juce::jmin (audio.getNumSamples(), to); ++i)
            if (changeAt (audio, i) >= 0.1f * reference)
                return i;

        return fallback;
    }

    /** The energy the hit adds in its first 50 ms, split into low, middle and high: the spectrum of the 50 ms
        from `time` minus that of the 50 ms before it (never below 0), so a kick that is still ringing doesn't
        make a hi-hat on top of it a kick. */
    void measureBands (const juce::AudioBuffer<float>& mono, double time, Candidate& c)
    {
        constexpr int order = 11, size = 1 << order;
        const juce::dsp::FFT fft (order);
        const int length = juce::jmin (size, (int) (energyWindow * tf::sampleRate));
        const int start = (int) std::round (time * tf::sampleRate);

        auto spectrum = [&] (int from)
        {
            std::vector<float> data ((size_t) size * 2, 0.0f);

            for (int i = 0; i < length; ++i)
                if (from + i >= 0 && from + i < mono.getNumSamples())
                    data[(size_t) i] = mono.getReadPointer (0)[from + i]
                                       * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * ((float) i + 0.5f) / (float) length));

            fft.performFrequencyOnlyForwardTransform (data.data(), true);
            return data;
        };

        const auto after = spectrum (start), before = spectrum (start - length);
        double low = 0.0, mid = 0.0, high = 0.0;

        for (int bin = 1; bin <= size / 2; ++bin)
        {
            const double hz = bin * tf::sampleRate / size;
            const double p = juce::jmax (0.0, (double) after[(size_t) bin] * after[(size_t) bin] - (double) before[(size_t) bin] * before[(size_t) bin]);
            (hz < lowEdgeHz ? low : hz < highEdgeHz ? mid : high) += p;
        }

        c.low = (float) low;
        c.mid = (float) mid;
        c.high = (float) high;
    }

    void fillLevels (const juce::AudioBuffer<float>& audio, double sampleRate, Candidate& c);

    /** The spectral rise is measured over windows of about 12 ms, so a second hit a few ms after another may not
        rise at all. This finds those from the sound itself: a millisecond whose change is 3 times (9.5 dB) the
        largest of the 8 ms before, and not near a candidate already found. */
    void addSharpAttacks (const juce::AudioBuffer<float>& audio, const juce::AudioBuffer<float>& mono, double sampleRate,
                          std::vector<Candidate>& found)
    {
        const int block = juce::jmax (1, (int) std::round (0.001 * sampleRate));
        const int numBlocks = audio.getNumSamples() / block;
        std::vector<float> envelope ((size_t) numBlocks, 0.0f);

        for (int k = 0; k < numBlocks; ++k)
            envelope[(size_t) k] = peakChange (audio, k * block, (k + 1) * block);

        std::vector<Candidate> extra;

        for (int k = 1; k < numBlocks; ++k)
        {
            float before = 0.0f;

            for (int j = juce::jmax (0, k - 8); j < k; ++j)
                before = juce::jmax (before, envelope[(size_t) j]);

            if (envelope[(size_t) k] < sharpRatio * before || envelope[(size_t) k] < 1.0e-3f)
                continue;

            Candidate c;
            c.time = (double) findStart (audio, (k - 1) * block, (k + 1) * block, envelope[(size_t) k], k * block) / sampleRate;
            bool near = false;

            for (const auto& other : found)
                near = near || std::abs (other.time - c.time) < nearSeconds;

            for (const auto& other : extra)
                near = near || std::abs (other.time - c.time) < nearSeconds;

            if (near)
                continue;

            fillLevels (audio, sampleRate, c);

            if (c.levelDb < quietestDb)
                continue;

            c.rise = riseFloor;
            measureBands (mono, c.time, c);
            extra.push_back (c);
        }

        found.insert (found.end(), extra.begin(), extra.end());
        std::sort (found.begin(), found.end(), [] (const Candidate& a, const Candidate& b) { return a.time < b.time; });
    }

    void fillLevels (const juce::AudioBuffer<float>& audio, double sampleRate, Candidate& c)
    {
        const int s = (int) std::round (c.time * sampleRate);
        c.levelDb = toDb (peakOf (audio, s, s + (int) (peakWindow * sampleRate)));
    }
}

//==============================================================================
int getNoteForDrum (Drum d)
{
    switch (d)
    {
        case Drum::kick:  return 36;
        case Drum::snare: return 38;
        case Drum::hihat: return 42;
    }

    return 38;
}

std::vector<Candidate> findCandidates (const juce::AudioBuffer<float>& audio, double sampleRate,
                                       const std::function<bool()>& shouldExit,
                                       const std::function<void (double)>& progress)
{
    std::vector<Candidate> result;

    if (audio.getNumSamples() < fftSize || audio.getNumChannels() == 0)
        return result;

    const auto mono = tf::prepareInput (audio, sampleRate);
    const int numSamples = mono.getNumSamples();
    const int hop = tf::hopSize;
    const int numFrames = numSamples / hop + 1;
    const float* samples = mono.getReadPointer (0);

    std::array<int, numBands + 1> edges {};

    for (int b = 0; b <= numBands; ++b)
    {
        const double hz = lowestHz * std::pow (highestHz / lowestHz, (double) b / numBands);
        edges[(size_t) b] = juce::jmax (b > 0 ? edges[(size_t) b - 1] + 1 : 1, (int) std::round (hz * fftSize / tf::sampleRate));
    }

    juce::dsp::FFT fft (fftOrder);
    juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    std::vector<float> data (fftSize * 2);
    std::vector<std::array<float, numBands>> history ((size_t) fluxLag + 1);

    for (auto& h : history)
        h.fill (quietestDb);   // silence before the start, so a hit on the very first sample is found

    std::vector<float> rise ((size_t) numFrames, 0.0f);

    for (int f = 0; f < numFrames; ++f)
    {
        if ((f & 255) == 0)
        {
            if (shouldExit && shouldExit())
                return {};

            if (progress)
                progress (0.9 * f / numFrames);
        }

        std::fill (data.begin(), data.end(), 0.0f);
        const int start = f * hop - fftSize / 2;

        for (int i = juce::jmax (0, -start); i < fftSize && start + i < numSamples; ++i)
            data[(size_t) i] = samples[start + i];

        window.multiplyWithWindowingTable (data.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform (data.data(), true);
        auto& levels = history[(size_t) f % history.size()];

        for (int b = 0; b < numBands; ++b)
        {
            double power = 0.0;

            for (int bin = edges[(size_t) b]; bin < juce::jmin (edges[(size_t) b + 1], fftSize / 2 + 1); ++bin)
                power += (double) data[(size_t) bin] * data[(size_t) bin];

            levels[(size_t) b] = (float) (10.0 * std::log10 (1.0e-10 + power / ((double) fftSize * fftSize)));
        }

        {
            const auto& before = history[(size_t) (f + (int) history.size() - fluxLag) % history.size()];

            for (int b = 0; b < numBands; ++b)
                rise[(size_t) f] += juce::jmax (0.0f, juce::jmax (levels[(size_t) b], -80.0f) - juce::jmax (before[(size_t) b], -80.0f));
        }
    }

    // Every local peak of the rise above the floor is a candidate; its time is then refined on the source audio.
    double previousTime = -1.0;

    for (int f = 0; f + 1 < numFrames; ++f)
    {
        if (rise[(size_t) f] < riseFloor || (f > 0 && rise[(size_t) f] <= rise[(size_t) f - 1]) || rise[(size_t) f] < rise[(size_t) f + 1])
            continue;

        const double peakTime = (double) f * hop / tf::sampleRate;
        const int peakSample = (int) std::round (peakTime * sampleRate);
        const int from = juce::jmax ((int) std::floor ((previousTime + 0.001) * sampleRate), peakSample - (int) (lookBack * sampleRate));
        const float reference = peakChange (audio, peakSample - (int) (lookBack * sampleRate), peakSample + (int) (peakWindow * sampleRate));

        Candidate c;
        c.rise = rise[(size_t) f];
        c.time = (double) findStart (audio, from, peakSample + (int) (lookForward * sampleRate), reference, peakSample) / sampleRate;

        if (c.time <= previousTime)
            continue;

        // A real hit changes the sound much faster than it was changing just before; the ring of the last
        // hit, rising and falling again, doesn't.
        const int at = (int) std::round (c.time * sampleRate);
        const float after = peakChange (audio, at, at + (int) (onsetWindow * sampleRate));
        const float before = peakChange (audio, at - (int) (beforeWindow * sampleRate), at - (int) (0.001 * sampleRate));

        if (after < onsetRatio * before)
            continue;

        fillLevels (audio, sampleRate, c);

        if (c.levelDb < quietestDb)
            continue;

        measureBands (mono, c.time, c);
        result.push_back (c);
        previousTime = c.time;
    }

    addSharpAttacks (audio, mono, sampleRate, result);

    if (progress)
        progress (1.0);

    return result;
}

Candidate measureCandidate (const juce::AudioBuffer<float>& audio, double sampleRate, double time,
                            double searchSeconds, const std::vector<Candidate>& candidates, bool exact)
{
    Candidate best;
    best.time = juce::jmax (0.0, time);
    float strongest = -1.0f;

    if (! exact)
        for (const auto& c : candidates)
            if (std::abs (c.time - time) <= searchSeconds && c.rise > strongest)
            {
                strongest = c.rise;
                best = c;
            }

    if (strongest >= 0.0f)
        return best;

    float maxRise = 0.0f;

    for (const auto& c : candidates)
        maxRise = juce::jmax (maxRise, c.rise);

    best.rise = maxRise;
    fillLevels (audio, sampleRate, best);
    measureBands (tf::prepareInput (audio, sampleRate), best.time, best);
    return best;
}

Drum classify (const Candidate& c)
{
    if (c.low >= c.mid && c.low >= c.high)
        return Drum::kick;

    return c.high > c.mid ? Drum::hihat : Drum::snare;
}

int getVelocity (VelocityCurve curve, double dbBelowLoudest)
{
    double k = 40.0;

    switch (curve)
    {
        case VelocityCurve::same:    return 100;
        case VelocityCurve::natural: k = 40.0; break;
        case VelocityCurve::gentle:  k = 60.0; break;
        case VelocityCurve::strong:  k = 20.0; break;
    }

    return juce::jlimit (1, 127, (int) std::lround (127.0 * std::pow (10.0, -juce::jmax (0.0, dbBelowLoudest) / k)));
}

std::vector<Hit> pick (const std::vector<Candidate>& candidates, const PickSettings& settings, const std::vector<HandEdit>& edits)
{
    std::vector<Hit> hits;
    float loudest = -200.0f;

    for (const auto& c : candidates)
        loudest = juce::jmax (loudest, c.levelDb);

    const double gap = settings.minGapMs / 1000.0;
    double lastTime = -1.0e9;

    auto makeHit = [&] (const Candidate& c)
    {
        Hit h;
        h.time = c.time;
        h.levelDb = c.levelDb;
        h.rise = c.rise;
        h.drum = settings.singleDrum.value_or (classify (c));
        return h;
    };

    for (const auto& c : candidates)
    {
        if (c.levelDb < loudest + (float) settings.sensitivityDb || c.time - lastTime < gap)
            continue;

        hits.push_back (makeHit (c));
        lastTime = c.time;
    }

    auto nearest = [&] (double time) -> int
    {
        int index = -1;
        double distance = matchSeconds;

        for (int i = 0; i < (int) hits.size(); ++i)
            if (const double d = std::abs (hits[(size_t) i].time - time); d <= distance)
            {
                distance = d;
                index = i;
            }

        return index;
    };

    for (const auto& e : edits)
    {
        switch (e.kind)
        {
            case HandEdit::Kind::add:
                hits.push_back (makeHit (e.target));
                break;

            case HandEdit::Kind::move:
                if (const int i = nearest (e.time); i >= 0)
                {
                    const auto drum = hits[(size_t) i].drum;
                    const bool fixed = hits[(size_t) i].drumFixed;
                    hits[(size_t) i] = makeHit (e.target);

                    if (fixed)
                    {
                        hits[(size_t) i].drum = drum;
                        hits[(size_t) i].drumFixed = true;
                    }
                }
                break;

            case HandEdit::Kind::remove:
                if (const int i = nearest (e.time); i >= 0)
                    hits.erase (hits.begin() + i);
                break;

            case HandEdit::Kind::setDrum:
                if (const int i = nearest (e.time); i >= 0)
                {
                    hits[(size_t) i].drum = e.drum;
                    hits[(size_t) i].drumFixed = true;
                }
                break;
        }
    }

    std::stable_sort (hits.begin(), hits.end(), [] (const Hit& a, const Hit& b) { return a.time < b.time; });

    // Velocity: how far under the loudest hit of the same drum.
    std::map<int, float> loudestOfDrum;

    for (const auto& h : hits)
    {
        auto [it, added] = loudestOfDrum.try_emplace ((int) h.drum, h.levelDb);

        if (! added)
            it->second = juce::jmax (it->second, h.levelDb);
    }

    for (auto& h : hits)
        h.velocity = getVelocity (settings.curve, (double) (loudestOfDrum[(int) h.drum] - h.levelDb));

    return hits;
}
} // namespace daw::analysis::hits
