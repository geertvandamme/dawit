#include "RangeFinder.h"
#include "MonoAudio.h"
#include "engine/PitchTracker.h"
#include "engine/Tune.h"

namespace daw::analysis::rangefinder
{
std::vector<float> findFrames (const juce::AudioBuffer<float>& audio, double sampleRate, const Progress& progress)
{
    if (audio.getNumSamples() <= 0 || sampleRate <= 0.0)
        return {};

    const auto mono = toMono (audio, sampleRate, sampleRate);
    const float* data = mono.getReadPointer (0);
    const int total = mono.getNumSamples();
    const int window = pitch::getWindowSize (sampleRate);
    const int hop = juce::jmax (1, juce::roundToInt (sampleRate * frameSeconds));
    const int numFrames = (total + hop - 1) / hop;

    pitch::Tracker tracker;
    tracker.prepare (sampleRate, window);
    std::vector<float> frames ((size_t) numFrames, -1.0f), chunk ((size_t) window, 0.0f);

    for (int i = 0; i < numFrames; ++i)
    {
        if (progress && i % 64 == 0 && ! progress ((double) i / numFrames))
            return {};

        // The window is centred on the frame's moment; outside the clip it is silent.
        const int first = i * hop - window / 2;

        for (int k = 0; k < window; ++k)
        {
            const int s = first + k;
            chunk[(size_t) k] = (s >= 0 && s < total) ? data[s] : 0.0f;
        }

        const auto reading = tracker.analyse (chunk.data(), window, pitch::Tracker::minClarity, true);

        if (reading.voiced)
            frames[(size_t) i] = reading.midi;
    }

    return frames;
}

namespace
{
    /** The held notes of one run of smoothed pitches (fractional MIDI), found with a window that grows while the
        pitch stays within twice the tolerance, then trimmed to what's within the tolerance of its median. */
    void findInRun (const std::vector<double>& smooth, int firstFrame, double secondsPerFrame, std::vector<HeldNote>& out)
    {
        const double tol = heldToleranceCents * 0.01;
        const int minFrames = juce::jmax (1, (int) std::ceil (minHeldSeconds / secondsPerFrame - 1.0e-9));
        const int n = (int) smooth.size();
        int i = 0;

        while (i + minFrames <= n)
        {
            double lo = smooth[(size_t) i], hi = lo;
            int j = i + 1;

            while (j < n && juce::jmax (hi, smooth[(size_t) j]) - juce::jmin (lo, smooth[(size_t) j]) <= 2.0 * tol)
            {
                lo = juce::jmin (lo, smooth[(size_t) j]);
                hi = juce::jmax (hi, smooth[(size_t) j]);
                ++j;
            }

            if (j - i < minFrames)
            {
                ++i;
                continue;
            }

            std::vector<double> sorted (smooth.begin() + i, smooth.begin() + j);
            std::sort (sorted.begin(), sorted.end());
            const double median = sorted[sorted.size() / 2];

            // The longest stretch inside the window that stays within the tolerance of the median.
            int bestStart = 0, bestLength = 0, runStart = i;

            for (int k = i; k <= j; ++k)
            {
                const bool inside = k < j && std::abs (smooth[(size_t) k] - median) <= tol;

                if (! inside)
                {
                    if (k - runStart > bestLength)
                    {
                        bestStart = runStart;
                        bestLength = k - runStart;
                    }

                    runStart = k + 1;
                }
            }

            if (bestLength >= minFrames)
            {
                std::vector<double> inner (smooth.begin() + bestStart, smooth.begin() + bestStart + bestLength);
                std::sort (inner.begin(), inner.end());
                const double middle = inner[inner.size() / 2];
                HeldNote held;
                held.note = juce::roundToInt (middle);
                held.start = (firstFrame + bestStart) * secondsPerFrame;
                held.length = bestLength * secondsPerFrame;
                held.centsOff = (middle - held.note) * 100.0;
                out.push_back (held);
            }

            i = j;
        }
    }
}

Result findHeldNotes (const std::vector<float>& frames, double secondsPerFrame)
{
    Result r;
    r.frames = frames;
    const int maxGap = (int) std::floor (maxGapSeconds / secondsPerFrame + 1.0e-9);
    const int n = (int) frames.size();

    int firstClear = -1, lastClear = -1;

    for (int i = 0; i < n; ++i)
        if (frames[(size_t) i] >= 0.0f)
        {
            if (firstClear < 0)
                firstClear = i;

            lastClear = i;
        }

    if (firstClear >= 0)
    {
        r.firstPitchSeconds = firstClear * secondsPerFrame;
        r.lastPitchSeconds = (lastClear + 1) * secondsPerFrame;
    }

    int i = 0;

    while (i < n)
    {
        if (frames[(size_t) i] < 0.0f)
        {
            ++i;
            continue;
        }

        // A run: clear frames with gaps of no more than maxGap unclear ones between them.
        const int runStart = i;
        int end = i, k = i;

        while (k < n)
        {
            if (frames[(size_t) k] >= 0.0f)
            {
                end = k + 1;
                ++k;
            }
            else if (k - end < maxGap)
                ++k;
            else
                break;
        }

        // Smooth the clear frames, one value per frame of the run (a bridged gap keeps the last value).
        tune::CentreFilter filter;
        filter.prepare (1.0 / secondsPerFrame);
        std::vector<double> smooth;
        smooth.reserve ((size_t) (end - runStart));
        bool started = false;
        double last = 0.0;

        for (int f = runStart; f < end; ++f)
        {
            const float v = frames[(size_t) f];

            if (v >= 0.0f)
            {
                if (! started)
                {
                    filter.restart (v);
                    started = true;
                }

                last = filter.process (v);
            }

            smooth.push_back (last);
        }

        findInRun (smooth, runStart, secondsPerFrame, r.held);
        i = end;
    }

    if (r.held.empty())
        return r;

    r.rawLowest = r.rawHighest = r.held[0].note;

    for (const auto& h : r.held)
    {
        r.rawLowest = juce::jmin (r.rawLowest, h.note);
        r.rawHighest = juce::jmax (r.rawHighest, h.note);
    }

    r.lowest = juce::jlimit (lowestNote, highestNote, r.rawLowest);
    r.highest = juce::jlimit (lowestNote, highestNote, r.rawHighest);
    r.lowestClamped = r.lowest != r.rawLowest;
    r.highestClamped = r.highest != r.rawHighest;
    r.verdict = r.getSpan() < minSpan ? Result::Verdict::tooNarrow : Result::Verdict::measured;
    return r;
}

std::optional<Result> measure (const juce::AudioBuffer<float>& audio, double sampleRate, const Progress& progress)
{
    auto frames = findFrames (audio, sampleRate, progress);

    if (frames.empty() && audio.getNumSamples() > 0)
        return std::nullopt;   // cancelled

    return findHeldNotes (frames);
}
} // namespace daw::analysis::rangefinder
