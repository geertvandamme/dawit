#include "ChordFinder.h"
#include "MonoAudio.h"
#include "generate/Harmony.h"

#include <juce_dsp/juce_dsp.h>

namespace daw::analysis::chordfinder
{
namespace
{
    constexpr int fftOrder = 12;
    constexpr int fftSize = 1 << fftOrder;   // 0.37 s windows: fine enough for the bass notes
    constexpr int hopSize = 1024;            // about 11 frames a second
    constexpr int sustainFrames = 9;         // a sound has to last about this long to count (drums don't)

    constexpr double seventhPenalty = 0.03;  // a seventh chord has to fit this much better than its triad
    constexpr double bassBonus = 0.1;        // the bass plays the root
    constexpr double keyBonus = 0.05;        // a chord of the key
    constexpr double stayBonus = 0.03;       // the chord goes on into the next bar
    constexpr double quietBar = 0.03;        // a bar this much quieter than the loudest has no chord (-30 dB)

    // Krumhansl and Kessler's key profiles: how well each note of the scale fits the key, from C.
    constexpr std::array<double, 12> majorProfile { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    constexpr std::array<double, 12> minorProfile { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

    template <typename Array>
    double correlation (const Array& a, const std::array<double, 12>& b, int shift)
    {
        double meanA = 0.0, meanB = 0.0;

        for (int i = 0; i < 12; ++i)
        {
            meanA += a[(size_t) i] / 12.0;
            meanB += b[(size_t) i] / 12.0;
        }

        double cross = 0.0, varA = 0.0, varB = 0.0;

        for (int i = 0; i < 12; ++i)
        {
            const double x = a[(size_t) ((i + shift) % 12)] - meanA, y = b[(size_t) i] - meanB;
            cross += x * y;
            varA += x * x;
            varB += y * y;
        }

        return varA > 0.0 ? cross / std::sqrt (varA * varB) : 0.0;
    }

    bool isMinorChord (const Chord& c)  { return c.intervals.contains (3); }

    /** The key's tonic chord, a seventh or not. */
    bool isTonicChord (const Chord& c, const MusicalKey& key)
    {
        return c.root == key.tonic && isMinorChord (c) == (key.scale == MusicalKey::Scale::minor);
    }

    bool isInKey (const Chord& c, const MusicalKey& key)
    {
        for (int degree = 1; degree <= 7; ++degree)
        {
            const auto triad = gen::harmony::triad (key, degree);

            if (triad.root == c.root && isMinorChord (triad) == isMinorChord (c))
                return true;
        }

        return false;
    }

    /** The best chords bar by bar: each bar's fit plus the bonuses, chained so that a chord going on
        into the next bar wins a tie (Viterbi). Bars without notes stay empty and break the chain. */
    std::vector<std::optional<size_t>> chooseChords (const std::vector<std::optional<std::vector<double>>>& fits,
                                                     const std::vector<Chord>& vocabulary, const std::optional<MusicalKey>& key)
    {
        const size_t numChords = vocabulary.size();
        std::vector<std::optional<size_t>> chosen (fits.size());
        std::vector<std::vector<double>> total (fits.size());
        std::vector<std::vector<size_t>> from (fits.size());

        for (size_t b = 0; b < fits.size(); ++b)
        {
            if (! fits[b].has_value())
                continue;

            total[b].assign (numChords, 0.0);
            from[b].assign (numChords, 0);
            const bool chained = b > 0 && fits[b - 1].has_value();

            for (size_t c = 0; c < numChords; ++c)
            {
                double best = 0.0;

                if (chained)
                {
                    best = -1.0e9;

                    for (size_t p = 0; p < numChords; ++p)
                    {
                        const double candidate = total[b - 1][p] + (p == c ? stayBonus : 0.0);

                        if (candidate > best)
                        {
                            best = candidate;
                            from[b][c] = p;
                        }
                    }
                }

                total[b][c] = best + (*fits[b])[c] + (key.has_value() && isInKey (vocabulary[c], *key) ? keyBonus : 0.0);
            }
        }

        // Back from the end of each run of bars with notes.
        for (size_t b = fits.size(); b-- > 0;)
        {
            if (! fits[b].has_value())
                continue;

            if (b + 1 < fits.size() && chosen[b + 1].has_value())
                chosen[b] = from[b + 1][*chosen[b + 1]];
            else
                chosen[b] = (size_t) std::distance (total[b].begin(), std::max_element (total[b].begin(), total[b].end()));
        }

        return chosen;
    }
}

//==============================================================================
juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    return toMono (audio, sourceSampleRate, sampleRate);
}

Chroma findChroma (const juce::AudioBuffer<float>& mono)
{
    const int numSamples = mono.getNumSamples();
    const int numFrames = numSamples / hopSize + 1;
    const int numBins = fftSize / 2 + 1;
    const float* samples = mono.getReadPointer (0);

    juce::dsp::FFT fft (fftOrder);
    juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    std::vector<float> data (fftSize * 2);
    std::vector<std::vector<float>> spectra ((size_t) numFrames);

    for (int f = 0; f < numFrames; ++f)
    {
        std::fill (data.begin(), data.end(), 0.0f);
        const int start = f * hopSize - fftSize / 2;

        for (int i = juce::jmax (0, -start); i < fftSize && start + i < numSamples; ++i)
            data[(size_t) i] = samples[start + i];

        window.multiplyWithWindowingTable (data.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform (data.data(), true);
        spectra[(size_t) f].assign (data.begin(), data.begin() + numBins);
    }

    // Each bin's note name, and how close it is to the note (1 on it, 0 half way to the next).
    std::vector<int> pitchClass ((size_t) numBins, -1);
    std::vector<float> closeness ((size_t) numBins, 0.0f);

    for (int bin = 1; bin < numBins; ++bin)
    {
        const double note = 12.0 * std::log2 (bin * sampleRate / fftSize / 440.0) + 69.0;
        const double nearest = std::round (note);
        pitchClass[(size_t) bin] = ((int) nearest % 12 + 12) % 12;
        closeness[(size_t) bin] = (float) juce::jmax (0.0, 1.0 - 2.0 * std::abs (note - nearest));
    }

    Chroma chroma;
    chroma.frameRate = sampleRate / hopSize;
    chroma.harmony.assign ((size_t) numFrames, {});
    chroma.bass.assign ((size_t) numFrames, {});
    std::vector<float> around (sustainFrames);

    for (int f = 0; f < numFrames; ++f)
    {
        for (int bin = 1; bin < numBins; ++bin)
        {
            // What lasts: the median of the bin over the frames around (a drum hit is in one or two).
            for (int k = 0; k < sustainFrames; ++k)
            {
                const int g = juce::jlimit (0, numFrames - 1, f + k - sustainFrames / 2);
                around[(size_t) k] = spectra[(size_t) g][(size_t) bin];
            }

            std::nth_element (around.begin(), around.begin() + sustainFrames / 2, around.end());
            const float level = std::sqrt (around[(size_t) sustainFrames / 2]) * closeness[(size_t) bin];
            const double hz = bin * sampleRate / fftSize;
            const auto pc = (size_t) pitchClass[(size_t) bin];

            if (hz >= 65.0 && hz <= 2100.0)
                chroma.harmony[(size_t) f][pc] += level;

            if (hz >= 40.0 && hz <= 250.0)
                chroma.bass[(size_t) f][pc] += level;
        }
    }

    return chroma;
}

const std::vector<Chord>& getVocabulary()
{
    static const std::vector<Chord> vocabulary = []
    {
        std::vector<Chord> chords;

        for (const auto& q : Chord::getQualities())
            if (juce::StringArray { "", "m", "7", "maj7", "m7" }.contains (q.suffix))
                for (int root = 0; root < 12; ++root)
                    chords.push_back ({ root, q.intervals });

        return chords;
    }();

    return vocabulary;
}

double scoreChord (const Chord& chord, const PitchClasses& harmony, const PitchClasses& bass)
{
    double dot = 0.0, energy = 0.0;
    const auto notes = chord.getPitchClasses();

    for (int pc = 0; pc < 12; ++pc)
    {
        energy += (double) harmony[(size_t) pc] * harmony[(size_t) pc];

        if (notes.contains (pc))
            dot += harmony[(size_t) pc];
    }

    if (energy <= 0.0)
        return 0.0;

    double score = dot / std::sqrt (energy * notes.size());

    const auto lowest = (int) std::distance (bass.begin(), std::max_element (bass.begin(), bass.end()));

    if (bass[(size_t) lowest] > 0.0f && lowest == chord.root)
        score += bassBonus;

    if (notes.size() > 3)
        score -= seventhPenalty;

    return score;
}

MusicalKey findKey (const PitchClasses& allNotes, const std::vector<Chord>& chordsInOrder)
{
    MusicalKey best;
    double bestScore = -1.0e9;

    for (int tonic = 0; tonic < 12; ++tonic)
    {
        for (auto scale : { MusicalKey::Scale::major, MusicalKey::Scale::minor })
        {
            const MusicalKey key { tonic, scale };
            double score = correlation (allNotes, scale == MusicalKey::Scale::major ? majorProfile : minorProfile, tonic);

            // The notes alone can't tell C major from A minor: the chord a song starts on and comes
            // home to can.
            if (! chordsInOrder.empty())
            {
                const auto onTonic = std::count_if (chordsInOrder.begin(), chordsInOrder.end(), [&key] (const Chord& c) { return isTonicChord (c, key); });
                score += 0.1 * (double) onTonic / (double) chordsInOrder.size();

                if (isTonicChord (chordsInOrder.front(), key))
                    score += 0.1;
            }

            if (score > bestScore)
            {
                bestScore = score;
                best = key;
            }
        }
    }

    return best;
}

Result analyse (const juce::AudioBuffer<float>& mono, double startBeat, const TempoMap& tempo)
{
    const auto chroma = findChroma (mono);
    const auto secondsOf = [&tempo, startBeat] (double beat) { return tempo.getSecondsBetween (startBeat, beat); };
    const double endBeat = tempo.getBeatAfter (startBeat, mono.getNumSamples() / sampleRate);

    struct Bar
    {
        int bar = 0;
        PitchClasses harmony {}, bass {};
        double energy = 0.0;
    };

    std::vector<Bar> bars;

    for (int bar = (int) std::floor (startBeat / 4.0); bar * 4.0 < endBeat; ++bar)
    {
        const double from = juce::jmax (startBeat, bar * 4.0), to = juce::jmin (endBeat, bar * 4.0 + 4.0);

        if (to - from < 2.0)
            continue;   // less than half the bar is in the audio

        Bar b;
        b.bar = bar;
        // Not the frames near the bar lines: their window still hears the chord before or after.
        const double edge = 0.5 * fftSize / sampleRate;
        const int first = (int) std::ceil ((secondsOf (from) + edge) * chroma.frameRate);
        const int last = juce::jmin ((int) chroma.harmony.size(), (int) std::ceil ((secondsOf (to) - edge) * chroma.frameRate));

        for (int f = first; f < last; ++f)
        {
            for (size_t pc = 0; pc < 12; ++pc)
            {
                b.harmony[pc] += chroma.harmony[(size_t) f][pc];
                b.bass[pc] += chroma.bass[(size_t) f][pc];
                b.energy += chroma.harmony[(size_t) f][pc];
            }
        }

        b.energy /= juce::jmax (1, last - first);

        // Drums and noise are heard in every note name alike: take that floor off (the median level).
        auto sorted = b.harmony;
        std::sort (sorted.begin(), sorted.end());
        const float floor = 0.5f * (sorted[5] + sorted[6]);

        for (auto& level : b.harmony)
            level = juce::jmax (0.0f, level - floor);

        bars.push_back (b);
    }

    double loudest = 0.0;

    for (const auto& b : bars)
        loudest = juce::jmax (loudest, b.energy);

    const auto& vocabulary = getVocabulary();
    std::vector<std::optional<std::vector<double>>> fits (bars.size());
    PitchClasses allNotes {};

    for (size_t i = 0; i < bars.size(); ++i)
    {
        if (bars[i].energy <= quietBar * loudest || loudest <= 0.0)
            continue;

        std::vector<double> scores;

        for (const auto& chord : vocabulary)
            scores.push_back (scoreChord (chord, bars[i].harmony, bars[i].bass));

        fits[i] = std::move (scores);

        for (size_t pc = 0; pc < 12; ++pc)
            allNotes[pc] += bars[i].harmony[pc];
    }

    // First without a key, to find it; then with it.
    std::vector<Chord> firstGuess;

    for (const auto& c : chooseChords (fits, vocabulary, std::nullopt))
        if (c.has_value())
            firstGuess.push_back (vocabulary[*c]);

    Result result;
    result.key = findKey (allNotes, firstGuess);
    const auto chosen = chooseChords (fits, vocabulary, result.key);

    for (size_t i = 0; i < bars.size(); ++i)
    {
        BarChord bc;
        bc.bar = bars[i].bar;

        if (chosen[i].has_value())
        {
            bc.chord = vocabulary[*chosen[i]];
            bc.score = (*fits[i])[*chosen[i]];
        }

        result.bars.push_back (bc);
    }

    return result;
}

juce::String toText (const std::vector<BarChord>& bars)
{
    juce::StringArray lines, line;

    for (size_t i = 0; i < bars.size(); ++i)
    {
        line.add (bars[i].chord.has_value() ? bars[i].chord->getName() : juce::String ("-"));

        if (line.size() == 4 || i + 1 == bars.size())
        {
            lines.add (line.joinIntoString (" | "));
            line.clear();
        }
    }

    return lines.joinIntoString ("\n");
}
} // namespace daw::analysis::chordfinder
