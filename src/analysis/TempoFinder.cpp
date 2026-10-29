#include "TempoFinder.h"
#include "MonoAudio.h"

#include <juce_dsp/juce_dsp.h>

namespace daw::analysis::tempofinder
{
namespace
{
    constexpr int fftOrder = 10;
    constexpr int fftSize = 1 << fftOrder;       // 46 ms windows
    constexpr int numBands = 40;
    constexpr double lowestHz = 30.0, highestHz = 11000.0, lowBandHz = 150.0;
    constexpr int fluxLag = 4;                   // compares with 23 ms earlier
    constexpr double tightness = 100.0;          // how strongly beats keep to the tempo (librosa's default)

    /** The average of `values` over `radius` frames each side. */
    std::vector<float> movingAverage (const std::vector<float>& values, int radius)
    {
        const int n = (int) values.size();
        std::vector<double> sums ((size_t) n + 1, 0.0);

        for (int i = 0; i < n; ++i)
            sums[(size_t) i + 1] = sums[(size_t) i] + values[(size_t) i];

        std::vector<float> out ((size_t) n);

        for (int i = 0; i < n; ++i)
        {
            const int from = juce::jmax (0, i - radius), to = juce::jmin (n, i + radius + 1);
            out[(size_t) i] = (float) ((sums[(size_t) to] - sums[(size_t) from]) / (to - from));
        }

        return out;
    }

    /** Only the rises above the local average, scaled so they average 1. */
    void normalise (std::vector<float>& values)
    {
        const auto local = movingAverage (values, (int) (frameRate * 0.25));
        double sum = 0.0;

        for (size_t i = 0; i < values.size(); ++i)
        {
            values[i] = juce::jmax (0.0f, values[i] - local[i]);
            sum += values[i];
        }

        if (sum > 0.0)
            for (auto& v : values)
                v = (float) (v * (double) values.size() / sum);
    }

    double bpmOf (double periodFrames)
    {
        return 60.0 * frameRate / periodFrames;
    }

    /** Where between three values the peak lies, from -0.5 to 0.5 around the middle one. */
    double peakOffset (double before, double at, double after)
    {
        const double curve = before - 2.0 * at + after;
        return std::abs (curve) > 1.0e-12 ? juce::jlimit (-0.5, 0.5, 0.5 * (before - after) / curve) : 0.0;
    }
}

//==============================================================================
juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    return toMono (audio, sourceSampleRate, sampleRate);
}

Onsets findOnsets (const juce::AudioBuffer<float>& mono)
{
    const int numSamples = mono.getNumSamples();
    const int numFrames = numSamples / hopSize + 1;
    const float* samples = mono.getReadPointer (0);

    // The bands' first bins, spaced evenly in pitch, at least one bin each.
    std::array<int, numBands + 1> edges {};

    for (int b = 0; b <= numBands; ++b)
    {
        const double hz = lowestHz * std::pow (highestHz / lowestHz, (double) b / numBands);
        edges[(size_t) b] = juce::jmax (b > 0 ? edges[(size_t) b - 1] + 1 : 1, (int) std::round (hz * fftSize / sampleRate));
    }

    // Each bin's pitch class, for the chroma (55 Hz to 2 kHz, where the chords are).
    std::vector<int> pitchClass (fftSize / 2 + 1, -1);

    for (int bin = 1; bin <= fftSize / 2; ++bin)
    {
        const double hz = bin * sampleRate / fftSize;

        if (hz >= 55.0 && hz <= 2000.0)
            pitchClass[(size_t) bin] = ((int) std::round (12.0 * std::log2 (hz / 440.0)) + 69 + 1200) % 12;
    }

    juce::dsp::FFT fft (fftOrder);
    juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    std::vector<float> data (fftSize * 2);
    std::vector<std::array<float, numBands>> history ((size_t) fluxLag + 1);

    Onsets onsets;
    onsets.strength.assign ((size_t) numFrames, 0.0f);
    onsets.low.assign ((size_t) numFrames, 0.0f);
    onsets.chroma.assign ((size_t) numFrames, {});

    for (int f = 0; f < numFrames; ++f)
    {
        // A window centred on the frame.
        std::fill (data.begin(), data.end(), 0.0f);
        const int start = f * hopSize - fftSize / 2;

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

        for (int bin = 1; bin <= fftSize / 2; ++bin)
            if (const int pc = pitchClass[(size_t) bin]; pc >= 0)
                onsets.chroma[(size_t) f][(size_t) pc] += data[(size_t) bin] * data[(size_t) bin];

        // How much louder each band got than 23 ms ago, counting only rises (and nothing below -80 dB).
        if (f >= fluxLag)
        {
            const auto& before = history[(size_t) (f - fluxLag) % history.size()];

            for (int b = 0; b < numBands; ++b)
            {
                const float rise = juce::jmax (0.0f, juce::jmax (levels[(size_t) b], -80.0f) - juce::jmax (before[(size_t) b], -80.0f));
                onsets.strength[(size_t) f] += rise;

                if (edges[(size_t) b + 1] * sampleRate / fftSize <= lowBandHz)
                    onsets.low[(size_t) f] += rise;
            }
        }
    }

    normalise (onsets.strength);
    normalise (onsets.low);
    return onsets;
}

std::optional<double> findPeriod (const std::vector<float>& onsetStrength, double* confidence)
{
    // Onsets widened to about 15 ms, so a beat that wanders by a frame or two still lines up with itself.
    std::vector<float> strength (onsetStrength.size(), 0.0f);
    constexpr int spread = 4;

    for (size_t i = 0; i < strength.size(); ++i)
        for (int k = -spread; k <= spread; ++k)
            if ((int) i + k >= 0 && (size_t) ((int) i + k) < strength.size())
                strength[i] += onsetStrength[(size_t) ((int) i + k)] * (float) std::exp (-0.5 * juce::square (k / 2.0));

    const int n = (int) strength.size();
    const int shortest = (int) std::floor (60.0 * frameRate / maxBpm);
    const int longest = (int) std::ceil (60.0 * frameRate / minBpm);

    if (n < 4 * longest)
        return std::nullopt;

    // How alike the onsets are to themselves `lag` frames later, up to twice the longest beat. It's
    // measured over 8-second windows and averaged, so a tempo that drifts doesn't smear out.
    const int numLags = longest * 2 + 2;
    const int windowLength = juce::jmin (n, juce::jmax ((int) (frameRate * 8.0), numLags * 2));
    const int windowHop = (int) (frameRate * 2.0);
    std::vector<double> similarity ((size_t) numLags, 0.0);
    int numWindows = 0;

    for (int start = 0; start + windowLength <= n; start += windowHop)
    {
        const float* window = strength.data() + start;
        double energy = 0.0;

        for (int i = 0; i < windowLength; ++i)
            energy += (double) window[i] * window[i];

        if (energy <= 1.0e-9)
            continue;   // a silent stretch says nothing about the tempo

        for (int lag = 0; lag < numLags; ++lag)
        {
            double sum = 0.0;

            for (int i = 0; i + lag < windowLength; ++i)
                sum += (double) window[i] * window[i + lag];

            similarity[(size_t) lag] += sum / (windowLength - lag) / (energy / windowLength);
        }

        ++numWindows;
    }

    if (numWindows == 0)
        return std::nullopt;

    for (auto& v : similarity)
        v /= numWindows;

    if (similarity[0] <= 1.0e-9)
        return std::nullopt;

    // The beat repeats at its own period and at two beats; a weight centred on 120 BPM (one octave
    // wide) leans away from half and double the tempo.
    auto score = [&] (int lag)
    {
        const double bpm = bpmOf (lag);
        const double prior = std::exp (-0.5 * juce::square (std::log2 (bpm / 120.0)));
        return (similarity[(size_t) lag] + 0.5 * similarity[(size_t) lag * 2]) * prior;
    };

    int best = shortest;

    for (int lag = shortest; lag <= longest; ++lag)
        if (score (lag) > score (best))
            best = lag;

    if (confidence != nullptr)
    {
        // How much more alike at the beat than on average: 0 for noise, near 1 for a click track.
        double mean = 0.0;

        for (int lag = shortest; lag <= longest; ++lag)
            mean += similarity[(size_t) lag];

        mean /= (longest - shortest + 1);
        *confidence = juce::jlimit (0.0, 1.0, (similarity[(size_t) best] - mean) / (similarity[0] - mean + 1.0e-12));
    }

    if (best <= shortest || best >= longest)
        return (double) best;

    return best + peakOffset (score (best - 1), score (best), score (best + 1));
}

std::vector<double> findBeats (const std::vector<float>& strength, double period)
{
    const int n = (int) strength.size();

    if (n == 0 || period < 2.0)
        return {};

    // The onsets smoothed a little, so a beat may land just beside its onset.
    const int radius = juce::jmax (1, (int) std::round (period / 16.0));
    std::vector<double> local ((size_t) n, 0.0);

    for (int i = 0; i < n; ++i)
        for (int k = -radius; k <= radius; ++k)
            if (i + k >= 0 && i + k < n)
                local[(size_t) i] += strength[(size_t) (i + k)] * std::exp (-0.5 * juce::square (k * 32.0 / period));

    // Each frame's best score as a beat: its onset plus the best beat before it, about a period back.
    std::vector<double> total ((size_t) n);
    std::vector<int> previous ((size_t) n, -1);

    for (int i = 0; i < n; ++i)
    {
        double best = 0.0;
        const int from = juce::jmax (0, i - (int) std::round (2.0 * period));
        const int to = i - (int) std::round (period / 2.0);

        for (int j = from; j <= to; ++j)
        {
            const double candidate = total[(size_t) j] - tightness * juce::square (std::log ((i - j) / period));

            if (previous[(size_t) i] < 0 || candidate > best)
            {
                best = candidate;
                previous[(size_t) i] = j;
            }
        }

        total[(size_t) i] = local[(size_t) i] + (previous[(size_t) i] >= 0 ? best : 0.0);
    }

    // The last beat: the last peak of the score that's at least half as high as the typical peak.
    std::vector<double> peaks;

    for (int i = 1; i + 1 < n; ++i)
        if (total[(size_t) i] > total[(size_t) i - 1] && total[(size_t) i] >= total[(size_t) i + 1])
            peaks.push_back (total[(size_t) i]);

    if (peaks.empty())
        return {};

    std::nth_element (peaks.begin(), peaks.begin() + (std::ptrdiff_t) peaks.size() / 2, peaks.end());
    const double threshold = 0.5 * peaks[peaks.size() / 2];
    int last = n - 1;

    for (int i = n - 2; i > 0; --i)
    {
        if (total[(size_t) i] > total[(size_t) i - 1] && total[(size_t) i] >= total[(size_t) i + 1] && total[(size_t) i] >= threshold)
        {
            last = i;
            break;
        }
    }

    std::vector<double> beats;

    for (int i = last; i >= 0; i = previous[(size_t) i])
        beats.insert (beats.begin(), (double) i);

    // Beats in the quiet before the music starts and after it ends go.
    double sumSquares = 0.0;

    for (double b : beats)
        sumSquares += juce::square (local[(size_t) b]);

    const double quiet = 0.5 * std::sqrt (sumSquares / (double) beats.size());

    while (! beats.empty() && local[(size_t) beats.front()] < quiet)
        beats.erase (beats.begin());

    while (! beats.empty() && local[(size_t) beats.back()] < quiet)
        beats.pop_back();

    // And so do beats at the ends that aren't about a beat away from the next one: an echo or a fill.
    while (beats.size() > 2 && std::abs (beats[1] - beats[0] - period) > 0.2 * period)
        beats.erase (beats.begin());

    while (beats.size() > 2 && std::abs (beats.back() - beats[beats.size() - 2] - period) > 0.2 * period)
        beats.pop_back();

    return beats;
}

int findFirstDownbeat (const Onsets& onsets, const std::vector<double>& beatFrames)
{
    const int numBeats = (int) beatFrames.size();

    if (numBeats < 8)
        return 0;

    // Per beat: how much the harmony changed from the beat before, and how hard the low end hit.
    std::vector<double> change ((size_t) numBeats, 0.0), low ((size_t) numBeats, 0.0);
    std::array<double, 12> before {};

    for (int i = 0; i < numBeats; ++i)
    {
        const int from = (int) beatFrames[(size_t) i];
        const int to = i + 1 < numBeats ? (int) beatFrames[(size_t) i + 1] : juce::jmin ((int) onsets.chroma.size(), from + 1);
        std::array<double, 12> now {};

        for (int f = from; f < to; ++f)
            for (int pc = 0; pc < 12; ++pc)
                now[(size_t) pc] += onsets.chroma[(size_t) f][(size_t) pc];

        double dot = 0.0, a = 0.0, b = 0.0;

        for (int pc = 0; pc < 12; ++pc)
        {
            dot += now[(size_t) pc] * before[(size_t) pc];
            a += now[(size_t) pc] * now[(size_t) pc];
            b += before[(size_t) pc] * before[(size_t) pc];
        }

        change[(size_t) i] = i > 0 && a > 0.0 && b > 0.0 ? 1.0 - dot / std::sqrt (a * b) : 0.0;
        before = now;

        for (int f = juce::jmax (0, from - 3); f <= juce::jmin ((int) onsets.low.size() - 1, from + 3); ++f)
            low[(size_t) i] = juce::jmax (low[(size_t) i], (double) onsets.low[(size_t) f]);
    }

    auto meanOf = [] (const std::vector<double>& v)
    {
        double sum = 0.0;

        for (double x : v)
            sum += x;

        return sum / (double) v.size() + 1.0e-12;
    };

    const double changeMean = meanOf (change), lowMean = meanOf (low);
    std::array<double, 4> score {};
    std::array<int, 4> count {};

    for (int i = 1; i < numBeats; ++i)
    {
        score[(size_t) i % 4] += change[(size_t) i] / changeMean + 0.5 * low[(size_t) i] / lowMean;
        ++count[(size_t) i % 4];
    }

    int best = 0;

    for (int p = 1; p < 4; ++p)
        if (score[(size_t) p] / juce::jmax (1, count[(size_t) p]) > score[(size_t) best] / juce::jmax (1, count[(size_t) best]))
            best = p;

    return best;
}

std::optional<Tempo> analyse (const juce::AudioBuffer<float>& mono)
{
    const auto onsets = findOnsets (mono);
    double confidence = 0.0;
    const auto period = findPeriod (onsets.strength, &confidence);

    if (! period.has_value() || confidence < 0.05)
        return std::nullopt;

    const auto frames = findBeats (onsets.strength, *period);

    if (frames.size() < 8)
        return std::nullopt;

    // The tempo from all the beats: the slope of a straight line through their times.
    const double count = (double) frames.size();
    double meanIndex = 0.0, meanFrame = 0.0;

    for (size_t i = 0; i < frames.size(); ++i)
    {
        meanIndex += (double) i / count;
        meanFrame += frames[i] / count;
    }

    double covariance = 0.0, variance = 0.0;

    for (size_t i = 0; i < frames.size(); ++i)
    {
        covariance += ((double) i - meanIndex) * (frames[i] - meanFrame);
        variance += juce::square ((double) i - meanIndex);
    }

    Tempo tempo;
    tempo.bpm = bpmOf (covariance / variance);
    tempo.confidence = confidence;
    tempo.slowest = tempo.fastest = tempo.bpm;

    // The tempo over 16 beats at a time; the slowest and fastest leave out the outer tenth on
    // each side, so a few beats pulled by a sung word or a fill don't count as drifting.
    std::vector<double> local;

    for (size_t i = 0; i + 16 < frames.size(); i += 2)
        local.push_back (bpmOf ((frames[i + 16] - frames[i]) / 16.0));

    if (! local.empty())
    {
        std::sort (local.begin(), local.end());
        tempo.slowest = juce::jmin (tempo.bpm, local[local.size() / 10]);
        tempo.fastest = juce::jmax (tempo.bpm, local[local.size() - 1 - local.size() / 10]);
    }

    tempo.steady = tempo.fastest - tempo.slowest < juce::jmax (2.0, 0.025 * tempo.bpm);

    for (double f : frames)
        tempo.beats.push_back (f / frameRate);

    tempo.firstDownbeat = findFirstDownbeat (onsets, frames);
    return tempo;
}

double roundBpm (double bpm)
{
    const double whole = std::round (bpm);
    return std::abs (bpm - whole) < 0.05 ? whole : std::round (bpm * 100.0) / 100.0;
}

FollowedTempo followRecording (const Tempo& tempo, double clipStartBeat)
{
    std::vector<double> downbeats;
    for (size_t i = (size_t) juce::jmax (0, tempo.firstDownbeat); i < tempo.beats.size(); i += 4)
        downbeats.push_back (tempo.beats[i]);

    FollowedTempo followed;
    std::vector<double> barBpm;

    for (size_t k = 0; k + 1 < downbeats.size(); ++k)
        if (const double seconds = downbeats[k + 1] - downbeats[k]; seconds > 0.0)
            barBpm.push_back (juce::jlimit (TempoMap::minBpm, TempoMap::maxBpm, 240.0 / seconds));

    if (barBpm.size() != downbeats.size() - 1 || barBpm.empty())
        barBpm.assign (1, tempo.bpm);   // too short to measure a bar (or beats out of order): one tempo

    // The first downbeat on the nearest bar line, at the first bar's tempo; a bar later if the clip
    // would start before the song.
    const double lead = tempo.getFirstDownbeatSeconds() * barBpm.front() / 60.0;
    int firstBar = (int) std::round ((clipStartBeat + lead) / 4.0);

    while (firstBar * 4.0 - lead < 0.0)
        ++firstBar;

    std::vector<TempoChange> changes { { 0.0, barBpm.front() } };
    for (size_t k = 1; k < barBpm.size(); ++k)
        changes.push_back ({ (firstBar + (int) k) * 4.0, barBpm[k] });

    followed.map = TempoMap (std::move (changes));
    followed.clipStartBeat = firstBar * 4.0 - lead;
    followed.firstBar = firstBar;
    followed.bars = (int) barBpm.size();
    followed.range = juce::Range<double> (*std::min_element (barBpm.begin(), barBpm.end()), *std::max_element (barBpm.begin(), barBpm.end()));
    return followed;
}
} // namespace daw::analysis::tempofinder
