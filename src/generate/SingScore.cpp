#include "SingScore.h"

#include <algorithm>

namespace daw::gen::singthis
{
double midiToHz (double midi) { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }

double getPhonemeStartSeconds (const sing::Phrase& phrase, size_t index)
{
    int frame = 0;
    for (size_t i = 0; i < index && i < phrase.phonemes.size(); ++i)
        frame += phrase.phonemes[i].frames;

    return phrase.startSeconds + frame * sing::frameSeconds;
}

double getPhonemeEndSeconds (const sing::Phrase& phrase, size_t index)
{
    return getPhonemeStartSeconds (phrase, index) + phrase.phonemes[index].frames * sing::frameSeconds;
}

namespace
{
    struct PitchSegment
    {
        double start = 0.0, end = 0.0;
        int midi = 60;
    };

    /** A syllable on its note (or notes). */
    struct Unit
    {
        std::vector<pronunciation::Sound> onset, nucleus, coda;
        std::vector<PitchSegment> notes;   // the note and the ones that hold it
        int word = 0;
        double noteStart = 0.0, noteEnd = 0.0;
        std::vector<double> onsetSeconds, codaSeconds;   // per consonant; filled with the fallback or the model's lengths
    };

    /** Where a unit's sounds are in the phrase's phoneme list, to read the model's lengths back. */
    struct UnitMap
    {
        int onsetFirst = 0, nucleusFirst = 0, codaFirst = 0;
    };

    struct Laid
    {
        sing::Phrase phrase;
        std::vector<UnitMap> maps;
    };

    int toFrame (double seconds, double origin) { return (int) std::lround ((seconds - origin) / sing::frameSeconds); }

    /** Lays one phrase out: the sounds with their frames, the notes and the pitch. */
    Laid layoutPhrase (const std::vector<Unit>& units, double restBefore, bool first, const ScoreSettings& settings, bool voiceHasAp)
    {
        Laid laid;
        auto& phrase = laid.phrase;

        // Times of each consonant block.
        const size_t n = units.size();
        std::vector<double> onsetStart (n), vowelStart (n), endLimit (n), codaStart (n), onsetEff (n), codaEff (n);
        const double fs = sing::frameSeconds;

        auto sum = [] (const std::vector<double>& v) { double t = 0.0; for (auto d : v) t += d; return t; };

        for (size_t k = 0; k < n; ++k)
        {
            onsetEff[k] = sum (units[k].onsetSeconds);
            codaEff[k] = sum (units[k].codaSeconds);
        }

        // The consonants that close a note and the ones that lead into the next must leave the vowel at least 40% of
        // the note (and three frames, but never more than 80%): when they don't fit, both are shortened in step, so a
        // fast note keeps its vowel and the next word's vowel still starts on its own note.
        for (size_t k = 0; k < n; ++k)
        {
            const double length = units[k].noteEnd - units[k].noteStart;
            const double vowelMin = std::min (0.8 * length, std::max (0.4 * length, 3.0 * fs));
            const double room = std::max (0.0, length - vowelMin);
            const double gap = k + 1 < n ? std::max (0.0, units[k + 1].noteStart - units[k].noteEnd) : 0.0;
            double encroach = k + 1 < n ? std::max (0.0, onsetEff[k + 1] - gap) : 0.0;   // the part of the next lead that is over this note
            const double want = codaEff[k] + encroach;

            if (want > room && want > 0.0)
            {
                const double scale = room / want;
                codaEff[k] *= scale;
                encroach *= scale;

                if (k + 1 < n)
                    onsetEff[k + 1] = std::min (onsetEff[k + 1], gap) + encroach;
            }
        }

        for (size_t k = 0; k < n; ++k)
        {
            vowelStart[k] = units[k].noteStart;
            onsetStart[k] = units[k].noteStart - onsetEff[k];
        }

        for (size_t k = 0; k < n; ++k)
        {
            endLimit[k] = k + 1 < n ? std::min (units[k].noteEnd, onsetStart[k + 1]) : units[k].noteEnd;
            endLimit[k] = std::max (endLimit[k], vowelStart[k] + fs);
            codaStart[k] = endLimit[k] - codaEff[k];
        }

        const double t0 = onsetStart.front();
        const double paddingStart = t0 - sing::paddingSeconds;
        phrase.startSeconds = paddingStart;

        struct Seg { juce::String symbol; bool vowel; double from, to; int word; };
        std::vector<Seg> segs;

        double breath = 0.0;
        if (settings.breaths && ! first)
            breath = std::min (maxBreathSeconds, 0.5 * restBefore);

        const bool breathHere = breath > sing::frameSeconds && voiceHasAp;
        const double breathStart = t0 - (breathHere ? breath : 0.0);
        segs.push_back ({ "SP", false, paddingStart, breathStart, -1 });

        if (breathHere)
            segs.push_back ({ "AP", false, breathStart, t0, -1 });

        double cursor = t0;

        for (size_t k = 0; k < n; ++k)
        {
            const auto& u = units[k];

            if (onsetStart[k] > cursor + 0.5 * sing::frameSeconds)
                segs.push_back ({ "SP", false, cursor, onsetStart[k], -1 });

            laid.maps.push_back ({});
            laid.maps.back().onsetFirst = (int) segs.size();

            // Consonants before the vowel, each as long as the model or fallback says, squeezed to fit.
            const double onsetTotal = sum (u.onsetSeconds);
            const double onsetSpan = vowelStart[k] - onsetStart[k];
            const double scaleOnset = onsetTotal > 1.0e-9 ? onsetSpan / onsetTotal : 0.0;
            double t = onsetStart[k];

            for (size_t j = 0; j < u.onset.size(); ++j)
            {
                const double d = u.onsetSeconds[j] * scaleOnset;
                segs.push_back ({ u.onset[j].symbol, false, t, t + d, u.word });
                t += d;
            }

            // The vowel(s).
            laid.maps.back().nucleusFirst = (int) segs.size();
            const double vowelEnd = codaStart[k];
            const double each = (vowelEnd - vowelStart[k]) / (double) u.nucleus.size();

            for (size_t j = 0; j < u.nucleus.size(); ++j)
                segs.push_back ({ u.nucleus[j].symbol, u.nucleus[j].vowel, vowelStart[k] + each * (double) j, vowelStart[k] + each * (double) (j + 1), u.word });

            // Consonants after, ending where the note (or the next consonants) end.
            laid.maps.back().codaFirst = (int) segs.size();
            const double codaTotal = sum (u.codaSeconds);
            const double codaSpan = endLimit[k] - codaStart[k];
            const double scaleCoda = codaTotal > 1.0e-9 ? codaSpan / codaTotal : 0.0;
            t = codaStart[k];

            for (size_t j = 0; j < u.coda.size(); ++j)
            {
                const double d = u.codaSeconds[j] * scaleCoda;
                segs.push_back ({ u.coda[j].symbol, false, t, t + d, u.word });
                t += d;
            }

            cursor = endLimit[k];
        }

        const double phraseEnd = cursor + 5.0 * sing::frameSeconds;
        segs.push_back ({ "SP", false, cursor, phraseEnd, -1 });

        // Seconds to frames. Each boundary is rounded on its own, so the rounding never adds up; a sound that would get
        // less than one frame takes it from the sounds before it, so the vowels keep starting on their notes and the
        // phrase keeps its length.
        std::vector<int> bounds;
        for (auto& s : segs)
            bounds.push_back (toFrame (s.to, paddingStart));

        bounds.back() = std::max (bounds.back(), (int) segs.size());

        for (int i = (int) bounds.size() - 2; i >= 0; --i)
            bounds[(size_t) i] = std::min (bounds[(size_t) i], bounds[(size_t) i + 1] - 1);

        for (size_t i = 0; i < bounds.size(); ++i)
            bounds[i] = std::max (bounds[i], (int) i + 1);

        int frameCursor = 0;
        for (size_t i = 0; i < segs.size(); ++i)
        {
            phrase.phonemes.push_back ({ segs[i].symbol, segs[i].vowel, bounds[i] - frameCursor, segs[i].word });
            frameCursor = bounds[i];
        }

        phrase.cutFrames = std::max (0, toFrame (breathStart, paddingStart));

        // The notes tile the phrase: rests in the padding and the gaps, the sung notes between.
        struct Tile { double from, to; int midi; bool rest; };
        std::vector<Tile> tiles;
        tiles.push_back ({ paddingStart, onsetStart.front(), units.front().notes.front().midi, true });

        for (size_t k = 0; k < n; ++k)
        {
            const auto& u = units[k];

            if (k > 0 && tiles.back().to < onsetStart[k] - 1.0e-9)
                tiles.push_back ({ tiles.back().to, onsetStart[k], u.notes.front().midi, true });

            for (size_t j = 0; j < u.notes.size(); ++j)
            {
                const double from = j == 0 ? onsetStart[k] : u.notes[j].start;
                double to = u.notes[j].end;

                if (j + 1 == u.notes.size())
                    to = k + 1 < n ? std::min (to, onsetStart[k + 1]) : to;

                if (to > from + 1.0e-9)
                    tiles.push_back ({ from, to, u.notes[j].midi, false });
            }
        }

        tiles.push_back ({ tiles.back().to, phraseEnd, units.back().notes.back().midi, true });

        int tileCursor = 0;
        for (size_t i = 0; i < tiles.size(); ++i)
        {
            const int end = i + 1 == tiles.size() ? frameCursor : toFrame (tiles[i].to, paddingStart);
            const int frames = std::max (i + 1 == tiles.size() ? 0 : 1, end - tileCursor);

            if (frames > 0)
                phrase.notes.push_back ({ tiles[i].midi, frames, tiles[i].rest });

            tileCursor += frames;
        }

        // Pitch of every frame: the note under it, sliding into a note over slideSeconds before its vowel.
        struct Vowelled { double from, to; int midi; double vowelFrom; };
        std::vector<Vowelled> all;

        for (size_t k = 0; k < n; ++k)
            for (size_t j = 0; j < units[k].notes.size(); ++j)
                all.push_back ({ j == 0 ? onsetStart[k] : units[k].notes[j].start, units[k].notes[j].end, units[k].notes[j].midi,
                                 units[k].notes[j].start });

        phrase.f0.assign ((size_t) frameCursor, 0.0f);

        for (int f = 0; f < frameCursor; ++f)
        {
            const double t = paddingStart + ((double) f + 0.5) * sing::frameSeconds;
            double midi = all.front().midi;
            size_t idx = 0;

            // The last segment that has started by t; before the first, the first.
            for (size_t i = 0; i < all.size(); ++i)
                if (t >= all[i].from - 1.0e-9)
                    idx = i;

            midi = all[idx].midi;

            if (! settings.exactPitch || true)
            {
                // Slide: in the last slideSeconds before the next segment's vowel, glide to its pitch.
                if (idx + 1 < all.size())
                {
                    const auto& next = all[idx + 1];
                    const double slideFrom = next.vowelFrom - slideSeconds;

                    if (t >= slideFrom && next.vowelFrom - all[idx].to < slideSeconds + 1.0e-9)
                    {
                        const double x = juce::jlimit (0.0, 1.0, (t - slideFrom) / slideSeconds);
                        midi = all[idx].midi + (next.midi - all[idx].midi) * x;
                    }
                }
            }

            phrase.f0[(size_t) f] = (float) midiToHz (midi);
        }

        phrase.exactPitch = true;
        return laid;
    }
}

TimingReport measureTiming (const Score& score)
{
    TimingReport r;
    r.shortestVowelMs = 1.0e9;

    for (auto& t : score.syllables)
    {
        const double ms = (t.vowelStartSeconds - t.noteStartSeconds) * 1000.0;
        ++r.count;
        r.meanErrorMs += ms;
        r.meanAbsErrorMs += std::abs (ms);
        r.maxAbsErrorMs = std::max (r.maxAbsErrorMs, std::abs (ms));
        r.overOneFrame += std::abs (ms) > sing::frameSeconds * 1000.0 + 1.0e-6 ? 1 : 0;
        r.shortestVowelMs = std::min (r.shortestVowelMs, t.vowelSeconds * 1000.0);
        r.smallestVowelShare = std::min (r.smallestVowelShare, t.vowelSeconds / std::max (1.0e-9, t.noteSeconds));
    }

    if (r.count > 0)
    {
        r.meanErrorMs /= r.count;
        r.meanAbsErrorMs /= r.count;
    }
    else
    {
        r.shortestVowelMs = 0.0;
    }

    return r;
}

Score buildScore (const Melody& melody, const Pairing& pairing, const sing::Voice* voice, const ScoreSettings& settings,
                  sing::Singer* singer, const sing::CancelFn& cancel)
{
    Score score;

    if (melody.notes.empty())
        return score;

    score.spanStartSeconds = melody.notes.front().startSeconds;
    score.spanEndSeconds = melody.notes.back().endSeconds;

    // The units: every note that starts a syllable, with the notes that hold it.
    std::vector<Unit> units;

    for (size_t i = 0; i < melody.notes.size(); ++i)
    {
        const auto& a = pairing.notes[i];

        if (! a.starts)
            continue;

        const auto& sounds = pairing.sounds[(size_t) a.syllable];

        if (sounds.nucleus.empty())
            continue;   // nothing to sing (a number, a dash)

        Unit u;
        u.onset = sounds.onset;
        u.nucleus = sounds.nucleus;
        u.coda = sounds.coda;
        u.word = pairing.wordOfSyllable[(size_t) a.syllable];
        u.onsetSeconds.assign (u.onset.size(), fallbackConsonantSeconds);
        u.codaSeconds.assign (u.coda.size(), fallbackConsonantSeconds);
        u.noteStart = melody.notes[i].startSeconds;
        u.noteEnd = melody.notes[i].endSeconds;
        u.notes.push_back ({ melody.notes[i].startSeconds, melody.notes[i].endSeconds, melody.notes[i].pitch });

        for (size_t j = i + 1; j < melody.notes.size() && pairing.notes[j].syllable == a.syllable && ! pairing.notes[j].starts; ++j)
        {
            u.notes.push_back ({ melody.notes[j].startSeconds, melody.notes[j].endSeconds, melody.notes[j].pitch });
            u.noteEnd = melody.notes[j].endSeconds;
        }

        units.push_back (std::move (u));
    }

    // Phrases: split at rests of half a second or more.
    std::vector<std::vector<Unit>> groups;

    for (auto& u : units)
    {
        if (groups.empty() || u.noteStart - groups.back().back().noteEnd >= phraseRestSeconds - 1.0e-9)
            groups.emplace_back();

        groups.back().push_back (std::move (u));
    }

    const bool hasAp = voice == nullptr || voice->phonemeIds.count ("AP") > 0;
    const juce::String apName = "AP";
    double previousEnd = 0.0;

    for (size_t g = 0; g < groups.size(); ++g)
    {
        auto& group = groups[g];
        const double restBefore = g == 0 ? 0.0 : group.front().noteStart - previousEnd;
        previousEnd = group.back().noteEnd;

        auto laid = layoutPhrase (group, restBefore, g == 0, settings, hasAp);

        // The voice's duration model decides how long the consonants last.
        if (singer != nullptr)
        {
            const auto predicted = singer->predictDurations (laid.phrase, cancel);

            if (predicted.size() == laid.phrase.phonemes.size())
            {
                for (size_t k = 0; k < group.size(); ++k)
                {
                    for (size_t j = 0; j < group[k].onset.size(); ++j)
                        group[k].onsetSeconds[j] = juce::jlimit (0.02, 0.25, predicted[(size_t) laid.maps[k].onsetFirst + j]);

                    for (size_t j = 0; j < group[k].coda.size(); ++j)
                        group[k].codaSeconds[j] = juce::jlimit (0.02, 0.25, predicted[(size_t) laid.maps[k].codaFirst + j]);
                }

                laid = layoutPhrase (group, restBefore, g == 0, settings, hasAp);
            }

            if (! settings.exactPitch)
            {
                const auto f0 = singer->predictPitch (laid.phrase, cancel);

                if (f0.size() == laid.phrase.f0.size())
                {
                    laid.phrase.f0 = f0;
                    laid.phrase.exactPitch = false;
                }
            }
        }

        for (size_t k = 0; k < group.size(); ++k)
        {
            SyllableTiming t;
            t.phrase = (int) score.phrases.size();
            t.noteStartSeconds = group[k].noteStart;
            t.noteSeconds = group[k].noteEnd - group[k].noteStart;
            const auto first = (size_t) laid.maps[k].nucleusFirst;
            t.vowelStartSeconds = getPhonemeStartSeconds (laid.phrase, first);
            t.vowelSeconds = getPhonemeEndSeconds (laid.phrase, first + group[k].nucleus.size() - 1) - t.vowelStartSeconds;
            t.word = group[k].word;
            score.syllables.push_back (t);
        }

        score.phrases.push_back (std::move (laid.phrase));
    }

    return score;
}
} // namespace daw::gen::singthis
