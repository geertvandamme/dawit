#include "StructureFinder.h"
#include "MonoAudio.h"

#include <juce_dsp/juce_dsp.h>

#include <map>

namespace daw::analysis::structurefinder
{
namespace
{
    constexpr int fftOrder = 11;
    constexpr int fftSize = 1 << fftOrder;
    constexpr int hopSize = 1024;            // the chord finder's, so the frames line up with its chroma
    constexpr int sustainFrames = 9;         // shorter sounds than this count as drums
    constexpr double lowestBand = 50.0, highestBand = 5000.0;

    constexpr int window = 4;                // bars compared on each side of a bar line: a phrase
    constexpr int longestPart = 16;          // longer parts are found as pieces of this, then joined up
    constexpr double sameMusic = 0.8;        // parts at least this alike are the same music heard again

    // How much each kind of change counts, where a part starts and between parts.
    constexpr double notesWeight = 0.35, soundWeight = 0.3, loudnessWeight = 0.2, drumsWeight = 0.15;
    constexpr double groupNotes = 0.45, groupSound = 0.25, groupLoudness = 0.15, groupDrums = 0.15;

    /** Each band's level, frame by frame, and how much of it is short sounds. */
    struct Bands
    {
        std::vector<std::array<float, numBands>> level, shortSounds;
    };

    Bands findBands (const juce::AudioBuffer<float>& mono)
    {
        const int numSamples = mono.getNumSamples();
        const int numFrames = numSamples / hopSize + 1;
        const float* samples = mono.getReadPointer (0);

        // The FFT bins of each band, spaced evenly in pitch.
        std::array<int, numBands + 1> edges {};

        for (int b = 0; b <= numBands; ++b)
            edges[(size_t) b] = juce::jmax (1, (int) std::round (lowestBand * std::pow (highestBand / lowestBand, b / (double) numBands)
                                                                 * fftSize / sampleRate));

        juce::dsp::FFT fft (fftOrder);
        juce::dsp::WindowingFunction<float> hann (fftSize, juce::dsp::WindowingFunction<float>::hann, false);
        std::vector<float> data (fftSize * 2);
        Bands bands;
        bands.level.assign ((size_t) numFrames, {});
        bands.shortSounds.assign ((size_t) numFrames, {});

        for (int f = 0; f < numFrames; ++f)
        {
            std::fill (data.begin(), data.end(), 0.0f);
            const int start = f * hopSize - fftSize / 2;

            for (int i = juce::jmax (0, -start); i < fftSize && start + i < numSamples; ++i)
                data[(size_t) i] = samples[start + i];

            hann.multiplyWithWindowingTable (data.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform (data.data(), true);

            for (int b = 0; b < numBands; ++b)
                for (int bin = edges[(size_t) b]; bin < juce::jmax (edges[(size_t) b] + 1, edges[(size_t) b + 1]); ++bin)
                    bands.level[(size_t) f][(size_t) b] += data[(size_t) bin] * data[(size_t) bin];
        }

        // What doesn't last is drums: the level over the median of the frames around.
        std::vector<float> around (sustainFrames);

        for (int f = 0; f < numFrames; ++f)
        {
            for (size_t b = 0; b < numBands; ++b)
            {
                for (int k = 0; k < sustainFrames; ++k)
                    around[(size_t) k] = bands.level[(size_t) juce::jlimit (0, numFrames - 1, f + k - sustainFrames / 2)][b];

                std::nth_element (around.begin(), around.begin() + sustainFrames / 2, around.end());
                bands.shortSounds[(size_t) f][b] = juce::jmax (0.0f, bands.level[(size_t) f][b] - around[(size_t) sustainFrames / 2]);
            }
        }

        return bands;
    }

    /** How far apart two bars' notes are: 0 the same, 1 nothing in common (or one of them silent). */
    double notesApart (const Bar& a, const Bar& b)
    {
        if (! a.sounding || ! b.sounding)
            return a.sounding == b.sounding ? 0.0 : 1.0;

        double dot = 0.0;

        for (size_t pc = 0; pc < 12; ++pc)
            dot += (double) a.notes[pc] * b.notes[pc];

        return juce::jlimit (0.0, 1.0, 1.0 - dot);
    }

    /** A stretch of bars on average: its sound, loudness and drums. */
    struct Summary
    {
        std::array<double, numBands> sound {};
        double loudness = -100.0, drums = 0.0;
        bool sounding = false;
    };

    Summary summarise (const std::vector<Bar>& bars, int start, int count)
    {
        Summary s;
        int n = 0;
        double loudness = 0.0;

        for (int i = start; i < start + count; ++i)
        {
            const auto& bar = bars[(size_t) i];

            if (! bar.sounding)
                continue;

            for (size_t b = 0; b < numBands; ++b)
                s.sound[b] += bar.sound[b];

            loudness += bar.loudness;
            s.drums += bar.drums;
            ++n;
        }

        if (n > 0)
        {
            for (auto& level : s.sound)
                level /= n;

            s.loudness = loudness / n;
            s.drums /= n;
            s.sounding = true;
        }

        return s;
    }

    /** How far apart two stretches are in sound (a 6 dB difference in the bands' balance is all the
        way), loudness (12 dB) and drums (a third of the sound), each 0 to 1. */
    struct Apart
    {
        double sound = 0.0, loudness = 0.0, drums = 0.0;
    };

    Apart apart (const Summary& a, const Summary& b)
    {
        if (! a.sounding || ! b.sounding)
            return a.sounding == b.sounding ? Apart {} : Apart { 1.0, 1.0, 1.0 };

        double squares = 0.0;

        for (size_t band = 0; band < numBands; ++band)
            squares += (a.sound[band] - b.sound[band]) * (a.sound[band] - b.sound[band]);

        return { juce::jmin (1.0, std::sqrt (squares / numBands) / 6.0),
                 juce::jmin (1.0, std::abs (a.loudness - b.loudness) / 12.0),
                 juce::jmin (1.0, 3.0 * std::abs (a.drums - b.drums)) };
    }

    /** What a part's length costs: none for 4, 8 and 16 bars, more for lengths songs rarely use. The
        first and last parts may be anything even, for an intro of 2 bars or an outro that fades. */
    double lengthCost (int bars, bool atAnEnd)
    {
        if (bars == 4 || bars == 8 || bars == 16)
            return 0.0;

        if (atAnEnd)
            return bars == 2 ? 0.03 : bars == 12 ? 0.05 : bars % 2 == 0 ? 0.1 : 0.2;

        return bars == 12 ? 0.08 : bars == 2 ? 0.15 : bars == 6 ? 0.2 : bars % 2 == 0 ? 0.25 : 0.4;
    }

    juce::String timesText (int times)
    {
        return times == 1 ? juce::String ("once") : times == 2 ? juce::String ("twice") : juce::String (times) + " times";
    }
}

//==============================================================================
juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    return toMono (audio, sourceSampleRate, sampleRate);
}

std::vector<Bar> describeBars (const juce::AudioBuffer<float>& mono, double startBeat, const TempoMap& tempo)
{
    const auto chroma = chordfinder::findChroma (mono);
    const auto bands = findBands (mono);
    const double endBeat = tempo.getBeatAfter (startBeat, mono.getNumSamples() / sampleRate);
    const int numFrames = (int) chroma.harmony.size();
    const float* samples = mono.getReadPointer (0);

    std::vector<Bar> bars;

    for (int barNumber = (int) std::floor (startBeat / 4.0); barNumber * 4.0 < endBeat; ++barNumber)
    {
        const double from = juce::jmax (startBeat, barNumber * 4.0), to = juce::jmin (endBeat, barNumber * 4.0 + 4.0);

        if (to - from < 2.0)
            continue;   // less than half the bar is in the audio

        Bar bar;
        bar.bar = barNumber;
        const double fromSeconds = tempo.getSecondsBetween (startBeat, from), toSeconds = tempo.getSecondsBetween (startBeat, to);
        const int first = juce::jlimit (0, numFrames, (int) std::ceil (fromSeconds * chroma.frameRate));
        const int last = juce::jlimit (first, numFrames, (int) std::ceil (toSeconds * chroma.frameRate));

        std::array<double, numBands> level {};
        double all = 0.0, shortSounds = 0.0;

        for (int f = first; f < last; ++f)
        {
            for (size_t pc = 0; pc < 12; ++pc)
                bar.notes[pc] += chroma.harmony[(size_t) f][pc];

            for (size_t b = 0; b < numBands; ++b)
            {
                level[b] += bands.level[(size_t) f][b];
                all += bands.level[(size_t) f][b];
                shortSounds += bands.shortSounds[(size_t) f][b];
            }
        }

        // The notes without the floor that drums and noise put under every note name, at unit length.
        auto sorted = bar.notes;
        std::sort (sorted.begin(), sorted.end());
        const float floor = 0.5f * (sorted[5] + sorted[6]);
        double length = 0.0;

        for (auto& n : bar.notes)
        {
            n = juce::jmax (0.0f, n - floor);
            length += (double) n * n;
        }

        for (auto& n : bar.notes)
            n = length > 0.0 ? (float) (n / std::sqrt (length)) : 0.0f;

        // The balance of the bands, without the overall level (that's the loudness).
        double average = 0.0;

        for (size_t b = 0; b < numBands; ++b)
        {
            bar.sound[b] = (float) (10.0 * std::log10 (level[b] / juce::jmax (1, last - first) + 1.0e-10));
            average += bar.sound[b] / numBands;
        }

        for (auto& b : bar.sound)
            b -= (float) average;

        bar.drums = all > 0.0 ? shortSounds / all : 0.0;

        double squares = 0.0;
        const int firstSample = (int) (fromSeconds * sampleRate), lastSample = juce::jmin (mono.getNumSamples(), (int) (toSeconds * sampleRate));

        for (int i = firstSample; i < lastSample; ++i)
            squares += (double) samples[i] * samples[i];

        bar.loudness = juce::Decibels::gainToDecibels (std::sqrt (squares / juce::jmax (1, lastSample - firstSample)), -100.0);
        bars.push_back (bar);
    }

    double loudest = -100.0;

    for (const auto& bar : bars)
        loudest = juce::jmax (loudest, bar.loudness);

    for (auto& bar : bars)
    {
        bar.sounding = bar.loudness > juce::jmax (loudest - 40.0, -80.0);

        if (! bar.sounding)
            bar.notes = {};
    }

    return bars;
}

std::vector<double> findNovelty (const std::vector<Bar>& bars)
{
    const int n = (int) bars.size();
    std::vector<double> novelty ((size_t) n, 0.0);

    for (int b = 1; b < n; ++b)
    {
        const int w = juce::jmin (window, b, n - b);

        // The notes and the sound's balance compared a phrase apart, so a chord loop that goes on
        // doesn't count as a change, and the same chords in another order (Am F C G against
        // F G C Am) do. Near the ends there are fewer bars to compare, and so less to go on.
        double notes = 0.0, sound = 0.0;

        for (int k = 0; k < window; ++k)
        {
            if (b - window + k >= 0 && b + k < n)
            {
                const auto& before = bars[(size_t) (b - window + k)];
                const auto& after = bars[(size_t) (b + k)];
                notes += notesApart (before, after) / window;
                sound += apart (summarise (bars, b - window + k, 1), summarise (bars, b + k, 1)).sound / window;
            }
        }

        // Loudness and drums don't change with the chords: the bars just before and after, on average.
        const auto d = apart (summarise (bars, b - w, w), summarise (bars, b, w));
        novelty[(size_t) b] = notesWeight * notes + soundWeight * sound + loudnessWeight * d.loudness + drumsWeight * d.drums;
    }

    return novelty;
}

std::vector<int> findBoundaries (const std::vector<double>& novelty, int first, int end)
{
    if (end <= first)
        return {};

    // A change has to stand out from the song's own changes to start a part.
    std::vector<double> inside (novelty.begin() + first + 1, novelty.begin() + end);
    double threshold = 0.12;

    if (! inside.empty())
    {
        std::sort (inside.begin(), inside.end());
        threshold = juce::jmax (threshold, 0.5 * inside[(size_t) (0.9 * (double) (inside.size() - 1))]);
    }

    // A change builds up and dies down over the bars around it: only its peak counts.
    std::vector<double> gain ((size_t) end + 1, -threshold);

    for (int b = first + 1; b < end; ++b)
    {
        bool peak = true;

        for (int k = juce::jmax (first + 1, b - 2); k <= juce::jmin (end - 1, b + 2); ++k)
            peak = peak && novelty[(size_t) k] <= novelty[(size_t) b];

        if (peak)
            gain[(size_t) b] = novelty[(size_t) b] - threshold;
    }

    // The best way to cut the bars into parts: strong changes at the cuts, usual lengths between.
    std::vector<double> best ((size_t) (end - first + 1), -1.0e9);
    std::vector<int> from ((size_t) (end - first + 1), first);
    best[0] = 0.0;

    for (int b = first + 1; b <= end; ++b)
    {
        for (int length = 1; length <= juce::jmin (longestPart, b - first); ++length)
        {
            const int a = b - length;
            const double score = best[(size_t) (a - first)] - lengthCost (length, a == first || b == end)
                               + (b < end ? gain[(size_t) b] : 0.0);

            if (score > best[(size_t) (b - first)])
            {
                best[(size_t) (b - first)] = score;
                from[(size_t) (b - first)] = a;
            }
        }
    }

    std::vector<int> starts;

    for (int b = end; b > first; b = from[(size_t) (b - first)])
        starts.insert (starts.begin(), from[(size_t) (b - first)]);

    return starts;
}

double similarity (const std::vector<Bar>& bars, int startA, int barsA, int startB, int barsB)
{
    const int common = juce::jmin (barsA, barsB);
    double notes = 0.0;

    for (int k = 0; k < common; ++k)
        notes += notesApart (bars[(size_t) (startA + k)], bars[(size_t) (startB + k)]) / juce::jmax (1, common);

    const auto d = apart (summarise (bars, startA, barsA), summarise (bars, startB, barsB));
    return 1.0 - (groupNotes * notes + groupSound * d.sound + groupLoudness * d.loudness + groupDrums * d.drums);
}

std::vector<FoundSection> nameSections (const std::vector<Segment>& segments)
{
    // Neighbours of the same music are one part: a verse found as two halves of the same loop.
    std::vector<Segment> parts;

    for (const auto& s : segments)
    {
        if (! parts.empty() && parts.back().group == s.group && parts.back().startBar + parts.back().bars == s.startBar)
        {
            auto& p = parts.back();
            p.loudness = (p.loudness * p.bars + s.loudness * s.bars) / (p.bars + s.bars);
            p.bars += s.bars;
        }
        else
        {
            parts.push_back (s);
        }
    }

    const int n = (int) parts.size();
    std::map<int, int> count;
    std::map<int, double> loudness, groupBars;
    double quietest = 1.0e9, loudest = -1.0e9;

    for (const auto& p : parts)
    {
        ++count[p.group];
        loudness[p.group] += p.loudness * p.bars;
        groupBars[p.group] += p.bars;
        quietest = juce::jmin (quietest, p.loudness);
        loudest = juce::jmax (loudest, p.loudness);
    }

    for (auto& [group, level] : loudness)
        level /= groupBars[group];

    // Where a level sits between the quietest and loudest part, 0 to 1.
    auto relative = [quietest, loudest] (double level) { return loudest - quietest > 1.0 ? (level - quietest) / (loudest - quietest) : 0.5; };

    // The chorus: of the music that comes back, the loudest, and the most heard on a near tie.
    std::optional<int> chorus, verse;
    double bestScore = -1.0;

    for (const auto& [group, times] : count)
    {
        const double score = relative (loudness[group]) + 0.15 * times;

        if (times >= 2 && score > bestScore)
        {
            bestScore = score;
            chorus = group;
        }
    }

    // The verse: the other music that comes back first, in the middle of the song (an outro may be
    // the intro's music again).
    for (int i = 1; i < n - 1; ++i)
    {
        const int group = parts[(size_t) i].group;

        if (count[group] >= 2 && group != chorus)
        {
            verse = group;
            break;
        }
    }

    auto isChorus = [&parts, &chorus] (int i) { return chorus.has_value() && parts[(size_t) i].group == *chorus; };

    auto everyTime = [&parts, n] (int group, const std::function<bool (int)>& holds)
    {
        for (int i = 0; i < n; ++i)
            if (parts[(size_t) i].group == group && ! holds (i))
                return false;

        return true;
    };

    std::optional<int> secondChorus, lastChorus;

    for (int i = 0, choruses = 0; i < n; ++i)
    {
        if (isChorus (i))
        {
            if (++choruses == 2)
                secondChorus = i;

            lastChorus = i;
        }
    }

    std::vector<FoundSection> found;

    for (int i = 0; i < n; ++i)
    {
        const auto& p = parts[(size_t) i];
        const int times = count[p.group];
        FoundSection f;
        f.section.startBar = p.startBar;
        f.section.bars = p.bars;
        f.group = p.group;
        f.timesHeard = times;
        f.loudness = p.loudness;

        auto& type = f.section.type;
        auto& reason = f.reason;

        if (p.group == chorus)
        {
            type = SectionType::chorus;
            reason = "heard " + timesText (times) + ", the loudest music that comes back";
        }
        else if (p.group == verse)
        {
            type = SectionType::verse;
            reason = "heard " + timesText (times) + (chorus.has_value() && loudness[p.group] < loudness[*chorus] - 1.0 ? ", quieter than the chorus" : "");
        }
        else if (i == 0 && n > 1)
        {
            type = SectionType::intro;
            reason = times == 1 ? "the start, heard once" : "the start";
        }
        else if (i == n - 1 && n > 1)
        {
            type = SectionType::outro;
            reason = p.group == parts.front().group ? "the end, the intro's music again"
                   : lastChorus.has_value() && *lastChorus == i - 1 ? "the end, after the last chorus" : "the end";
        }
        else if (times == 1 && relative (p.loudness) < 0.25)
        {
            type = SectionType::breakdown;
            reason = "heard once, much quieter than the rest";   // before pre-chorus: a pre-chorus comes back with each chorus
        }
        else if (times == 1 && secondChorus.has_value() && i > *secondChorus)
        {
            type = SectionType::bridge;
            reason = "heard once, after the second chorus: new music before the last one";
        }
        else if (chorus.has_value() && p.bars <= 8 && everyTime (p.group, [n, &isChorus] (int k) { return k + 1 < n && isChorus (k + 1); }))
        {
            type = SectionType::preChorus;
            reason = times == 1 ? "just before a chorus" : "just before the chorus each time";
        }
        else if (chorus.has_value() && p.bars <= 8 && times >= 2 && everyTime (p.group, [&isChorus] (int k) { return k > 0 && isChorus (k - 1); }))
        {
            type = SectionType::postChorus;
            reason = "just after the chorus each time";
        }
        else if (! chorus.has_value())
        {
            type = SectionType::phrase;
            f.section.name = "Part " + getGroupName (p.group);
            reason = "new music: nothing in the recording comes back";
        }
        else
        {
            type = SectionType::interlude;
            reason = "heard " + timesText (times) + ", between the other parts";
        }

        found.push_back (f);
    }

    if (n == 1)
    {
        found[0].section.type = SectionType::phrase;
        found[0].section.name = "Part A";
        found[0].reason = "the same music all the way through";
    }

    // A type that comes back gets numbered, as the song templates do: Verse 1, Verse 2.
    std::map<SectionType, int> typeCount, seen;

    for (const auto& f : found)
        ++typeCount[f.section.type];

    for (auto& f : found)
        if (f.section.name.isEmpty() && typeCount[f.section.type] > 1)
            f.section.name = sectionTypes::getName (f.section.type) + " " + juce::String (++seen[f.section.type]);

    return found;
}

std::vector<FoundSection> analyse (const juce::AudioBuffer<float>& mono, double startBeat, const TempoMap& tempo)
{
    const auto bars = describeBars (mono, startBeat, tempo);
    int first = 0, end = (int) bars.size();

    while (first < end && ! bars[(size_t) first].sounding)
        ++first;

    while (end > first && ! bars[(size_t) end - 1].sounding)
        --end;

    if (first == end)
        return {};

    const auto starts = findBoundaries (findNovelty (bars), first, end);

    // Each part joins the earlier part it sounds most like, when that's close enough to be the same
    // music; otherwise it's new music, the next letter.
    std::vector<Segment> segments;
    std::vector<std::pair<int, int>> ranges;   // each segment's first bar index and length
    int groups = 0;

    for (size_t s = 0; s < starts.size(); ++s)
    {
        const int start = starts[s], length = (s + 1 < starts.size() ? starts[s + 1] : end) - start;
        int group = -1;
        double closest = sameMusic;

        for (size_t earlier = 0; earlier < segments.size(); ++earlier)
        {
            const double alike = similarity (bars, ranges[earlier].first, ranges[earlier].second, start, length);

            if (alike >= closest)
            {
                closest = alike;
                group = segments[earlier].group;
            }
        }

        if (group < 0)
            group = groups++;

        Segment segment;
        segment.startBar = bars[(size_t) start].bar;
        segment.bars = length;
        segment.group = group;
        segment.loudness = summarise (bars, start, length).loudness;
        segments.push_back (segment);
        ranges.emplace_back (start, length);
    }

    return nameSections (segments);
}

juce::String getGroupName (int group)
{
    return group >= 0 && group < 26 ? juce::String::charToString ((juce::juce_wchar) ('A' + group)) : juce::String (group + 1);
}

juce::String toText (const std::vector<FoundSection>& sections)
{
    juce::StringArray parts;

    for (const auto& f : sections)
        parts.add (f.section.getDisplayName() + " (" + juce::String (f.section.bars) + ")");

    return parts.joinIntoString (" | ");
}

juce::String getPattern (const std::vector<FoundSection>& sections)
{
    juce::StringArray letters;

    for (const auto& f : sections)
        letters.add (getGroupName (f.group));

    return letters.joinIntoString (" ");
}
} // namespace daw::analysis::structurefinder
