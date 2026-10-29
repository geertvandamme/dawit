#include "TuneAnalysis.h"
#include "SingingGuide.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace daw::gen::tunecheck
{
namespace
{
double median (std::vector<double> values)
{
    if (values.empty())
        return 0.0;

    std::sort (values.begin(), values.end());
    return values[values.size() / 2];
}

juce::String pitchClassName (int midi)
{
    static const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((midi % 12) + 12) % 12];
}

/** "the chorus"; with two sections of the same name, "the verse (bars 5 to 12)" so they can be told apart. */
juce::String sectionName (const SectionList& sections, int index)
{
    if (! juce::isPositiveAndBelow (index, (int) sections.size()))
        return "the song";

    const auto& s = sections[(size_t) index];
    const auto name = s.getDisplayName().toLowerCase();
    const bool shared = std::count_if (sections.begin(), sections.end(), [&] (const auto& other) { return other.getDisplayName().toLowerCase() == name; }) > 1;
    return "the " + name + (shared ? " (bars " + juce::String (s.startBar + 1) + " to " + juce::String (s.getEndBar()) + ")" : juce::String());
}

int findSection (const SectionList& sections, int bar)
{
    for (size_t i = 0; i < sections.size(); ++i)
        if (bar >= sections[i].startBar && bar < sections[i].getEndBar())
            return (int) i;

    return -1;
}

juce::String cents (double value) { return juce::String (juce::roundToInt (std::abs (value))); }
} // namespace

Analysis analyse (const std::vector<tune::Frame>& log, const TempoMap& tempo, const MusicalKey& key, const SectionList& sections)
{
    Analysis a;

    struct F { double seconds, beat, sung, corrected, rms; };
    std::vector<F> frames;

    for (const auto& f : log)
        if (f.beat >= 0.0f)
            frames.push_back ({ tempo.beatToSeconds (f.beat), f.beat, f.sung, f.corrected, f.rmsDb });

    if (frames.size() < 2)
        return a;

    std::vector<double> gaps;

    for (size_t i = 1; i < frames.size(); ++i)
        if (const double d = frames[i].seconds - frames[i - 1].seconds; d > 0.0 && d < 0.1)
            gaps.push_back (d);

    const double dt = juce::jlimit (0.0025, 0.02, median (gaps));
    tune::CentreFilter filter;
    filter.prepare (1.0 / dt);

    // Notes: runs of voiced frames whose vibrato centre stays on one semitone; gaps under 50 ms are bridged.
    struct Run { int semitone = 0; std::vector<double> errors; double startSec = 0, endSec = 0, startBeat = 0, endBeat = 0; std::vector<double> allPitches, times; };
    std::vector<Run> runs;
    bool open = false;
    double lastVoiced = -1.0e9, lastPitch = 0.0;
    int previousSemitone = -1;

    auto closeRun = [&] { open = false; };

    for (const auto& f : frames)
    {
        if (f.sung < 0.0)
            continue;

        if (open && (f.seconds - lastVoiced > bridgeSeconds || std::abs (f.sung - lastPitch) > 1.0))
            closeRun();

        if (! open)
        {
            filter.restart (f.sung);
            previousSemitone = -1;
        }

        const double centre = filter.process (f.sung);
        const int semitone = tune::chooseTarget (centre, tune::allNotes, previousSemitone);
        previousSemitone = semitone;

        if (! open || runs.back().semitone != semitone)
        {
            runs.push_back ({});
            runs.back().semitone = semitone;
            runs.back().startSec = f.seconds;
            runs.back().startBeat = f.beat;
            open = true;
        }

        auto& run = runs.back();
        run.endSec = f.seconds;
        run.endBeat = f.beat;
        run.allPitches.push_back (f.sung);
        run.times.push_back (f.seconds);
        lastVoiced = f.seconds;
        lastPitch = f.sung;
    }

    for (const auto& run : runs)
    {
        if (run.endSec - run.startSec < minNoteSeconds)
            continue;

        std::vector<double> errors;

        for (size_t i = 0; i < run.allPitches.size(); ++i)
            if (run.times[i] - run.startSec >= skipSeconds)
                errors.push_back ((run.allPitches[i] - run.semitone) * 100.0);

        if (errors.empty())
            continue;

        Note n;
        n.midi = run.semitone;
        n.startBeat = run.startBeat;
        n.endBeat = run.endBeat;
        n.startSeconds = run.startSec;
        n.endSeconds = run.endSec;
        n.errorCents = median (errors);
        n.inKey = key.contains (run.semitone);
        n.bar = (int) std::floor (run.startBeat / 4.0);
        n.section = findSection (sections, n.bar);
        a.notes.push_back (n);
    }

    // Phrases, split by half a second of silence.
    for (size_t i = 0; i < a.notes.size(); ++i)
    {
        if (i == 0 || a.notes[i].startSeconds - a.notes[i - 1].endSeconds >= phraseGapSeconds)
            a.phrases.push_back ({ (int) i, 1 });
        else
            ++a.phrases.back().second;
    }

    if (! a.notes.empty())
    {
        int in25 = 0, in10 = 0;

        for (const auto& n : a.notes)
        {
            in25 += std::abs (n.errorCents) <= 25.0 ? 1 : 0;
            in10 += std::abs (n.errorCents) <= 10.0 ? 1 : 0;
        }

        a.within25 = (double) in25 / (double) a.notes.size();
        a.within10 = (double) in10 / (double) a.notes.size();
    }

    std::map<int, std::pair<double, int>> perSection;

    for (const auto& n : a.notes)
        if (n.section >= 0)
        {
            perSection[n.section].first += n.errorCents;
            ++perSection[n.section].second;
        }

    for (const auto& [index, sum] : perSection)
        a.sections.push_back ({ index, sum.second, sum.first / sum.second });

    // How far the correction moved things, and how much of the loud sound had no pitch.
    double moved = 0.0;
    int voiced = 0, loud = 0, unclear = 0;

    for (const auto& f : frames)
    {
        if (f.sung >= 0.0)
        {
            moved += std::abs (f.corrected - f.sung) * 100.0;
            ++voiced;
        }

        if (f.rms > -40.0)
        {
            ++loud;
            unclear += f.sung < 0.0 ? 1 : 0;
        }
    }

    a.meanCorrectionCents = voiced > 0 ? moved / voiced : 0.0;
    a.unclearShare = loud > 0 ? (double) unclear / loud : 0.0;
    a.unclearSeconds = unclear * dt;
    return a;
}

juce::String formatBars (const std::vector<int>& bars)
{
    juce::StringArray shown;

    for (const int b : bars)
        shown.add (juce::String (b + 1));

    if (shown.size() == 1)
        return "bar " + shown[0];

    const auto last = shown[shown.size() - 1];
    shown.remove (shown.size() - 1);
    return "bars " + shown.joinIntoString (", ") + " and " + last;
}

std::vector<juce::String> getAdvice (const std::vector<tune::Frame>& log, const TempoMap& tempo, const MusicalKey& key, const SectionList& sections, bool correcting)
{
    return getAdvice (analyse (log, tempo, key, sections), key, sections, correcting);
}

std::vector<juce::String> getAdvice (const Analysis& a, const MusicalKey& key, const SectionList& sections, bool correcting)
{
    std::vector<juce::String> lines;

    // 1. Not enough to go on.
    if (a.notes.size() < 8)
    {
        lines.push_back ("Play the song with DawIt Tune on this track. It listens while the song plays, then shows how in tune each section is.");
        return lines;
    }

    // 2. The overall picture.
    const int total = (int) a.notes.size();
    const int in25 = juce::roundToInt (a.within25 * total), in10 = juce::roundToInt (a.within10 * total);
    lines.push_back ("Of " + juce::String (total) + " notes, " + juce::String (in25) + " (" + juce::String (juce::roundToInt (100.0 * in25 / total)) + "%) are within 25 cents of a note, and "
                     + juce::String (in10) + " (" + juce::String (juce::roundToInt (100.0 * in10 / total)) + "%) within 10.");

    bool saidAnyOfThreeToSix = false;

    // 3. Sections that sit flat or sharp.
    std::vector<SectionSummary> off;

    for (const auto& s : a.sections)
        if (s.numNotes >= 5 && std::abs (s.meanCents) >= 12.0)
            off.push_back (s);

    std::stable_sort (off.begin(), off.end(), [] (const auto& x, const auto& y) { return std::abs (x.meanCents) > std::abs (y.meanCents); });

    for (size_t i = 0; i < off.size() && i < 3; ++i)
    {
        const auto name = sectionName (sections, off[i].section);
        const auto sentence = name.substring (0, 1).toUpperCase() + name.substring (1);

        if (off[i].meanCents < 0.0)
            lines.push_back (sentence + " sits " + cents (off[i].meanCents) + " cents flat on average. A whole section flat often comes from tiredness, low energy or notes that sit awkwardly in your voice; "
                                        "try it with a little more energy, or check the key in Singing techniques: Your voice and your song.");
        else
            lines.push_back (sentence + " sits " + cents (off[i].meanCents) + " cents sharp on average. Singing sharp often comes from pushing; try it a little lighter.");

        saidAnyOfThreeToSix = true;
    }

    // 4. Lines that run flat at the end.
    {
        int lines2 = 0;

        for (const auto& phrase : a.phrases)
            lines2 += phrase.second >= 2 ? 1 : 0;

        if (lines2 >= 4)
        {
            // Compare per phrase: how much flatter the last note is than the others in its own line, on average.
            double difference = 0.0;

            for (const auto& [first, count] : a.phrases)
            {
                if (count < 2)
                    continue;

                double rest = 0.0;

                for (int i = 0; i < count - 1; ++i)
                    rest += a.notes[(size_t) (first + i)].errorCents;

                difference += rest / (count - 1) - a.notes[(size_t) (first + count - 1)].errorCents;
            }

            difference /= lines2;

            if (difference >= 15.0)
            {
                lines.push_back ("The last notes of your lines are " + cents (difference) + " cents flatter than the rest, which often means running out of breath. Take a breath before each line "
                                 "rather than during it (Singing techniques: Posture and breathing).");
                saidAnyOfThreeToSix = true;
            }
        }
    }

    // 5. Notes outside the key.
    {
        std::map<std::pair<int, int>, std::vector<int>> outside;   // (section, pitch class) -> bars

        for (const auto& n : a.notes)
            if (! n.inKey && n.endSeconds - n.startSeconds >= 0.25)
                outside[{ n.section, ((n.midi % 12) + 12) % 12 }].push_back (n.bar);

        std::vector<std::pair<std::pair<int, int>, std::vector<int>>> groups (outside.begin(), outside.end());
        std::stable_sort (groups.begin(), groups.end(), [] (const auto& x, const auto& y) { return x.second.size() > y.second.size(); });

        for (size_t i = 0; i < groups.size() && i < 3; ++i)
        {
            auto bars = groups[i].second;
            std::sort (bars.begin(), bars.end());
            bars.erase (std::unique (bars.begin(), bars.end()), bars.end());
            if (bars.size() > 5)
                bars.resize (5);

            const int pc = groups[i].first.second;
            const auto name = pitchClassName (pc);
            const int count = (int) groups[i].second.size();
            auto text = juce::String (count) + (count == 1 ? " note in " : " notes in ") + sectionName (sections, groups[i].first.first) + (count == 1 ? " is " : " are ") + name
                        + ", outside " + key.getName() + " (" + formatBars (bars) + "): wrong notes, or on purpose? Listen to them against the chords.";

            if (key.scale == MusicalKey::Scale::minor && pc == (key.tonic + 11) % 12)
                text << " " << name << " is the raised seventh that harmonic minor uses, common in minor songs. If it's meant, set the key to "
                     << MusicalKey::getTonicName (key.tonic) << " harmonic minor so DawIt Tune pulls to it.";

            lines.push_back (text);
            saidAnyOfThreeToSix = true;
        }
    }

    // 6. The furthest out.
    {
        std::vector<Note> far;

        for (const auto& n : a.notes)
            if (std::abs (n.errorCents) > 25.0)
                far.push_back (n);

        std::stable_sort (far.begin(), far.end(), [] (const auto& x, const auto& y) { return std::abs (x.errorCents) > std::abs (y.errorCents); });

        if (! far.empty())
        {
            juce::StringArray parts;

            for (size_t i = 0; i < far.size() && i < 3; ++i)
                parts.add ("bar " + juce::String (far[i].bar + 1) + ", " + singing::getNoteName (far[i].midi) + ", " + cents (far[i].errorCents) + " cents "
                           + (far[i].errorCents < 0.0 ? "flat" : "sharp"));

            lines.push_back ("Furthest out: " + parts.joinIntoString ("; ") + ".");
            saidAnyOfThreeToSix = true;
        }
    }

    // 7. Correction that is heavy-handed.
    if (correcting && a.meanCorrectionCents > 40.0)
        lines.push_back ("DawIt Tune is moving notes by " + cents (a.meanCorrectionCents) + " cents on average. That much correction tends to sound processed; another take may sound better.");

    // 8. No clear pitch.
    if (a.unclearShare > 0.4 && a.unclearSeconds >= 2.0)
        lines.push_back ("Much of this track has no clear pitch: is it a chord, a stack of harmonies or drums? DawIt Tune follows one note at a time.");

    // 9. Nicely in tune.
    if (a.within25 >= 0.9 && ! saidAnyOfThreeToSix)
        lines.push_back ("Nicely in tune. Correction would change little; if you use it, Gentle polish is enough.");

    return lines;
}
}
