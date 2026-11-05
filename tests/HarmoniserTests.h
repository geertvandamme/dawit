#pragma once

#include "engine/BuiltInHarmoniser.h"
#include "engine/Harmoniser.h"
#include "generate/HarmoniserGuideText.h"

/** DawIt Harmoniser: the rules, what the voices made of a take, the processor and the guide's text. Uses the helpers of TuneTests.h. */

namespace harmtests
{
/** Settings with only voice 1 on, at full level in the centre, Mix 100%: just the voice. */
inline daw::harm::Settings voiceOnly (daw::harm::Mode mode, int interval, float detuneCents = 0.0f)
{
    daw::harm::Settings s;
    s.mode = mode;
    s.mixPercent = 100.0f;
    s.voices[0] = { true, mode == daw::harm::Mode::fixed ? interval : 0, mode == daw::harm::Mode::key ? interval : 0, 0.0f, 0.0f, detuneCents, 0.0f };
    s.voices[1].on = false;
    return s;
}

/** Plays a steady "ah" at `hz` through the processor and says how far its pitch is from `targetHz`, in cents (mean and worst between 0.4 and 1.0 s). */
inline std::pair<double, double> sing (const daw::harm::Settings& s, double hz, double targetHz, int songMask = 0xfff, bool useRight = false)
{
    constexpr double rate = 48000.0;
    daw::BuiltInHarmoniser plugin;
    plugin.prepareToPlay (rate, 512);
    plugin.setSettings (s);
    plugin.setSongContext (songMask, 0);
    const auto input = tunetests::makeBuzz (hz, rate, (int) (rate * 1.2));
    const auto out = tunetests::runThrough (plugin, input, input);
    return tunetests::trackedStretch (useRight ? out.right : out.left, (size_t) (rate * 0.4), (size_t) (rate * 1.0), rate, targetHz);
}

inline double rms (const std::vector<float>& x, size_t from, size_t to)
{
    double sum = 0.0;

    for (size_t i = from; i < to; ++i)
        sum += (double) x[i] * x[i];

    return std::sqrt (sum / (double) (to - from));
}

struct LogPart { int frames; float lead; float shift1; float shift2; };

/** A made-up log of 100 frames a second: each part holds a lead note and the shifts of the voices (-100: voice quiet). */
inline std::vector<daw::harm::Frame> makeLog (const std::vector<LogPart>& parts)
{
    std::vector<daw::harm::Frame> log;

    for (const auto& p : parts)
        for (int i = 0; i < p.frames; ++i)
        {
            daw::harm::Frame f;
            f.beat = (float) log.size() * 0.02f;
            f.rmsDb = -20.0f;
            f.lead = p.lead;

            if (p.lead >= 0.0f)
            {
                f.voice[0] = p.shift1 > -50.0f ? p.lead + p.shift1 : -1.0f;
                f.voice[1] = p.shift2 > -50.0f ? p.lead + p.shift2 : -1.0f;
            }

            log.push_back (f);
        }

    return log;
}
}

static void runHarmoniserRulesTests()
{
    using namespace daw;
    namespace h = daw::harm;
    std::cout << "Harmoniser: the rules" << std::endl;
    const int cMajor = tunetests::getCMajor();

    // The shift tables of the plan.
    {
        h::Voice thirdUp;
        thirdUp.steps = 2;
        h::Voice fifthUp;
        fifthUp.steps = 4;
        std::vector<int> third, fifth;

        for (const int note : { 60, 62, 64, 65, 67, 69, 71 })
        {
            int previous = -1;
            third.push_back ((int) h::getShift ((double) note, thirdUp, h::Mode::key, cMajor, previous));
            previous = -1;
            fifth.push_back ((int) h::getShift ((double) note, fifthUp, h::Mode::key, cMajor, previous));
        }

        check (third == std::vector<int> { 4, 3, 3, 4, 4, 3, 3 }, "Harmoniser: in C major a 3rd up (+2 steps) is 4 3 3 4 4 3 3 semitones on C to B");
        check (fifth == std::vector<int> { 7, 7, 7, 7, 7, 7, 6 }, "Harmoniser: in C major a 5th up (+4 steps) is 7 on every note except B, where it is 6");
    }

    // A note outside the key takes the nearest key note's interval, and keeps its own cents.
    {
        h::Voice thirdUp;
        thirdUp.steps = 2;
        int previous = -1;
        const double shift = h::getShift (66.0, thirdUp, h::Mode::key, cMajor, previous);
        check (previous == 65 && std::abs (shift - 4.0) < 1.0e-9 && std::abs (tune::getHz (66.0 + shift) - 466.16) < 0.01,
               "Harmoniser: F#4 exactly in C major, 3rd up: the tie goes to F, the shift is +4 and the voice is A#4 (466.16 Hz)");
        previous = 65;
        check (std::abs (h::getShift (66.1, thirdUp, h::Mode::key, cMajor, previous) - 4.0) < 1.0e-9 && previous == 65,
               "Harmoniser: a note 10 cents past the halfway point keeps its choice (15-cent hysteresis)");
        previous = 65;
        h::getShift (66.2, thirdUp, h::Mode::key, cMajor, previous);
        check (previous == 67, "Harmoniser: 20 cents past the halfway point moves to the next key note");
    }

    // Fixed, clamping and the ratio.
    {
        h::Voice v;
        v.semitones = 4;
        int previous = -1;
        check (std::abs (h::getShift (57.0, v, h::Mode::fixed, cMajor, previous) - 4.0) < 1.0e-9, "Harmoniser: Fixed +4 shifts every note by 4 semitones");
        v.semitones = -12;
        check (std::abs (h::getShift (57.0, v, h::Mode::fixed, cMajor, previous) + 12.0) < 1.0e-9, "Harmoniser: Fixed -12 shifts by an octave down");
        v.steps = 7;
        check (std::abs (h::getShift (57.0, v, h::Mode::key, cMajor, previous) - 12.0) < 1.0e-9, "Harmoniser: Key +7 steps is an octave");
        check (std::abs (h::getRatio (4.0, 0.0f) - 1.2599) < 0.0001 && std::abs (h::getRatio (0.0, 7.0f) * 220.0 - 220.89) < 0.01 && std::abs (h::getRatio (12.0, 0.0f) - 2.0) < 1.0e-9,
               "Harmoniser: the ratio is 1.2599 for +4, 2 for an octave, and 7 cents at 220 Hz make 220.89 Hz");

        // Fixed +4 out of key on the C major scale: 4 of 7.
        int outside = 0;

        for (const int note : { 60, 62, 64, 65, 67, 69, 71 })
            outside += (cMajor & (1 << ((note + 4) % 12))) == 0 ? 1 : 0;

        check (outside == 4, "Harmoniser: Fixed +4 puts 4 of the 7 notes of C major outside the key (57%)");
    }

    // Keys: song, none, manual.
    {
        const int gMajor = (1 << 7) | (1 << 9) | (1 << 11) | (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
        check (h::getKeyMask (h::keyFromSong, h::Scale::major, gMajor) == gMajor, "Harmoniser: Song uses the song's key");
        check (h::getKeyMask (h::keyFromSong, h::Scale::major, 0xfff) == cMajor && h::getKeyMask (h::keyFromSong, h::Scale::major, 0) == cMajor,
               "Harmoniser: a mask without exactly 7 notes (no key) gives C major");
        check (h::getKeyMask (8, h::Scale::major, cMajor) == gMajor, "Harmoniser: a key chosen here (G) wins over the song's");
        check (h::getKeyMask (10, h::Scale::minor, gMajor) == cMajor, "Harmoniser: A minor has the notes of C major");
        check (h::stepInKey (59, 4, cMajor) == 65 && h::stepInKey (60, -2, cMajor) == 57 && h::stepInKey (60, 7, cMajor) == 72 && h::stepInKey (60, -7, cMajor) == 48,
               "Harmoniser: B3 four steps up is F4, C4 two steps down is A3, and seven steps are an octave");
    }

    // Names.
    {
        check (h::getIntervalName (h::Mode::fixed, 4) == "major 3rd up" && h::getIntervalName (h::Mode::fixed, -12) == "octave down"
                   && h::getIntervalName (h::Mode::fixed, 0) == "unison" && h::getIntervalName (h::Mode::fixed, 7) == "perfect 5th up",
               "Harmoniser: Fixed +4 is a major 3rd up, -12 an octave down, 0 a unison, +7 a perfect 5th up");
        check (h::getIntervalName (h::Mode::key, 2) == "3rd up" && h::getIntervalName (h::Mode::key, -5) == "6th down" && h::getIntervalName (h::Mode::key, 7) == "octave up"
                   && h::getIntervalName (h::Mode::key, 1) == "2nd up" && h::getIntervalName (h::Mode::key, 4) == "5th up",
               "Harmoniser: Key +2 steps is a 3rd up, -5 a 6th down, +7 an octave up");
        check (h::getNoteName (1) == "C#" && h::getNoteName (11) == "B", "Harmoniser: note names");
    }

    // Gains.
    {
        const auto zero = h::getMixGains (0.0f), half = h::getMixGains (50.0f), full = h::getMixGains (100.0f);
        check (juce::exactlyEqual (zero.first, 1.0f) && juce::exactlyEqual (zero.second, 0.0f) && juce::exactlyEqual (half.first, 1.0f) && juce::exactlyEqual (half.second, 1.0f)
                   && juce::exactlyEqual (full.first, 0.0f) && juce::exactlyEqual (full.second, 1.0f),
               "Harmoniser: the mix gains (lead, voices) are (1, 0), (1, 1) and (0, 1) at 0, 50 and 100%");
        const auto centre = h::getPanGains (0.0f), left = h::getPanGains (-100.0f), right = h::getPanGains (100.0f);
        check (std::abs (20.0 * std::log10 (centre.first) + 3.01) < 0.05 && std::abs (centre.first - centre.second) < 1.0e-6f, "Harmoniser: the centre pan is 3.01 dB down in each side");
        check (std::abs (left.first - 1.0f) < 1.0e-6f && std::abs (left.second) < 1.0e-6f && std::abs (right.second - 1.0f) < 1.0e-6f && std::abs (right.first) < 1.0e-6f,
               "Harmoniser: pan 100% left puts everything on the left, 100% right on the right");
        check (juce::exactlyEqual (h::getLevelGain (-30.0f), 0.0f) && std::abs (h::getLevelGain (-6.0f) - 0.501f) < 0.001f && std::abs (h::getLevelGain (6.0f) - 1.995f) < 0.001f,
               "Harmoniser: the bottom of the level range is silence, -6 dB is 0.501");
    }

    // Starting points.
    {
        const auto& presets = h::getPresets();
        bool whys = presets.size() == 6;

        for (const auto& p : presets)
            whys = whys && p.name.isNotEmpty() && p.why.isNotEmpty();

        const h::Settings defaults;
        const auto& first = presets[0].settings;
        check (whys && presets[0].name == "Third above" && first.mode == defaults.mode && first.voices[0].on == defaults.voices[0].on && first.voices[0].steps == defaults.voices[0].steps
                   && juce::exactlyEqual (first.voices[0].levelDb, defaults.voices[0].levelDb) && ! first.voices[1].on && juce::exactlyEqual (first.mixPercent, defaults.mixPercent),
               "Harmoniser: six starting points, each with a why; the first, Third above, is the default");
        const auto& wide = presets[5].settings;
        check (presets[5].name == "Wide double" && wide.mode == h::Mode::fixed && juce::exactlyEqual (wide.voices[0].detuneCents, 7.0f) && juce::exactlyEqual (wide.voices[1].detuneCents, -7.0f)
                   && juce::exactlyEqual (wide.voices[0].delayMs, 12.0f) && juce::exactlyEqual (wide.voices[1].delayMs, 20.0f) && juce::exactlyEqual (wide.voices[0].panPercent, -70.0f)
                   && juce::exactlyEqual (wide.voices[1].panPercent, 70.0f),
               "Harmoniser: Wide double is two copies at +7 and -7 cents, 12 and 20 ms, panned 70% left and right");
    }

    // The test sound.
    {
        const auto notes = h::getPhraseNotes();
        check (notes.size() == 6 && juce::roundToInt (notes[0].midi) == 64 && juce::roundToInt (notes[2].midi) == 67 && juce::roundToInt (notes[5].midi) == 60 && std::abs (notes.back().end - h::phraseSeconds) < 1.0e-9,
               "Harmoniser: the made-up phrase is E4 E4 G4 E4 D4 C4 and lasts 8 beats at 90 BPM");
    }
}

static void runHarmoniserAdviceTests()
{
    using namespace daw;
    namespace h = daw::harm;
    using harmtests::LogPart;
    std::cout << "Harmoniser: your track" << std::endl;
    const auto cMajorSettings = [] { return h::Settings(); };
    h::KeyInfo cKey;
    cKey.known = true;
    cKey.mask = tunetests::getCMajor();
    cKey.name = "C major";
    auto joined = [] (const std::vector<juce::String>& lines) { juce::StringArray all; for (const auto& l : lines) all.add (l); return all.joinIntoString ("\n"); };
    constexpr float quiet = -100.0f;

    // Rule 1: nothing heard.
    {
        const auto lines = h::getAdvice ({}, cMajorSettings(), cKey, 100.0, {});
        check (lines.size() == 1 && lines[0] == "Play the song once with DawIt Harmoniser on: this page fills with what it heard.", "Harmoniser advice: with nothing heard it says to play the song");
    }

    // Rule 2: few clear notes.
    {
        const auto few = joined (h::getAdvice (harmtests::makeLog ({ { 140, 60.0f, 4.0f, quiet }, { 260, -1.0f, 0.0f, 0.0f } }), cMajorSettings(), cKey, 100.0, {}));
        check (few.contains ("Only 35% of the sound had one clear note, so the voices were quiet most of the time."), "Harmoniser advice: 35% clear notes gives the few-clear-notes paragraph");
        const auto enough = joined (h::getAdvice (harmtests::makeLog ({ { 200, 60.0f, 4.0f, quiet }, { 200, -1.0f, 0.0f, 0.0f } }), cMajorSettings(), cKey, 100.0, {}));
        check (! enough.contains ("Only "), "Harmoniser advice: at 50% clear notes it says nothing about it");
    }

    // Rule 3 and 4: sizes and the diminished fifth.
    {
        h::Settings s;
        s.voices[1] = { true, 7, 4, -6.0f, 0.0f, 0.0f, 0.0f };
        const auto log = harmtests::makeLog ({ { 145, 62.0f, 3.0f, 7.0f }, { 105, 60.0f, 4.0f, 7.0f } });
        const auto text = joined (h::getAdvice (log, s, cKey, 100.0, {}));
        check (text.contains ("Voice 1 (3rd up) made a minor 3rd on 58% of the notes and a major 3rd on 42%: the key decides which."), "Harmoniser advice: voice 1 made a minor 3rd on 58% and a major 3rd on 42%");
        check (text.contains ("Voice 2 (5th up) made a perfect 5th on every note.") && ! text.contains ("diminished"), "Harmoniser advice: a 5th up that is always 7 semitones is told so, with no diminished fifth");

        const auto bLog = harmtests::makeLog ({ { 30, 71.0f, 3.0f, 6.0f }, { 220, 60.0f, 4.0f, 7.0f } });
        const auto b = joined (h::getAdvice (bLog, s, cKey, 100.0, {}));
        check (b.contains ("Voice 2 (5th up) made a diminished 5th (6 semitones) on 12% of the notes, on B in C major. If it sounds harsh there, try a 3rd or a 6th, or sing that line yourself."),
               "Harmoniser advice: a diminished fifth on 12% of the notes names B in C major");
        const auto fewB = joined (h::getAdvice (harmtests::makeLog ({ { 10, 71.0f, 3.0f, 6.0f }, { 240, 60.0f, 4.0f, 7.0f } }), s, cKey, 100.0, {}));
        check (! fewB.contains ("(6 semitones)"), "Harmoniser advice: a diminished fifth on 4% of the notes isn't mentioned");

        // A 4th up in Key: F to B is 6 semitones and an augmented 4th.
        h::Settings fourth;
        fourth.voices[1] = { true, 5, 3, -6.0f, 0.0f, 0.0f, 0.0f };
        const auto fLog = harmtests::makeLog ({ { 30, 65.0f, 3.0f, 6.0f }, { 220, 60.0f, 4.0f, 5.0f } });
        const auto f = joined (h::getAdvice (fLog, fourth, cKey, 100.0, {}));
        check (f.contains ("Voice 2 (4th up) made an augmented 4th (6 semitones) on 12% of the notes, on F in C major.") && ! f.contains ("diminished"),
               "Harmoniser advice: a 4th up that makes 6 semitones on F is an augmented 4th, not a diminished 5th");
    }

    // Rule 5: Fixed out of key.
    {
        h::Settings s;
        s.mode = h::Mode::fixed;
        std::vector<LogPart> parts;

        for (const float note : { 60.0f, 62.0f, 64.0f, 65.0f, 67.0f, 69.0f, 71.0f })
            parts.push_back ({ 40, note, 4.0f, quiet });

        const auto text = joined (h::getAdvice (harmtests::makeLog (parts), s, cKey, 100.0, {}));
        check (text.contains ("In Fixed, voice 1 (+4) landed outside C major on 57% of the notes (C#, D#, F#, G#). Try Harmony: Key to keep the voices in the key, unless you want that sound."),
               "Harmoniser advice: Fixed +4 on the C major scale lands outside the key on 57% of the notes");
        const auto inKey = joined (h::getAdvice (harmtests::makeLog ({ { 100, 60.0f, 4.0f, quiet }, { 100, 65.0f, 4.0f, quiet }, { 100, 67.0f, 4.0f, quiet } }), s, cKey, 100.0, {}));
        check (! inKey.contains ("landed outside"), "Harmoniser advice: Fixed +4 on C, F and G stays in the key and says nothing");
    }

    // Rule 6: lead out of tune.
    {
        h::Settings s;
        s.voices[0].on = false;
        const auto off = joined (h::getAdvice (harmtests::makeLog ({ { 250, 60.3f, 0.0f, quiet } }), s, cKey, 100.0, {}));
        check (off.contains ("The lead is on average 30 cents from the nearest note, and the voices copy that. Put DawIt Tune before DawIt Harmoniser to tune the lead first."),
               "Harmoniser advice: a lead 30 cents off is told so, pointing to DawIt Tune");
        const auto fine = joined (h::getAdvice (harmtests::makeLog ({ { 250, 60.2f, 0.0f, quiet } }), s, cKey, 100.0, {}));
        check (! fine.contains ("The lead is on average"), "Harmoniser advice: a lead 20 cents off is left alone");
    }

    // Rule 7 and 8.
    {
        const auto line = h::getLatencyLine ("bus");
        check (line == "On a bus, DawIt Harmoniser makes everything in it about 27 ms late. Put it on the audio track itself." && h::getLatencyLine ("instrument track").startsWith ("On an instrument track"),
               "Harmoniser advice: the latency line for a bus and an instrument track, with \"about\" when the rate isn't known");
        const auto late44 = 1000.0 * BuiltInHarmoniser::getLatencyFor (44100.0) / 44100.0;
        const auto late48 = 1000.0 * BuiltInHarmoniser::getLatencyFor (48000.0) / 48000.0;
        check (h::getLatencyLine ("return", late44).contains ("everything in it 29 ms late") && h::getLatencyLine ("bus", late48).contains ("everything in it 27 ms late"),
               "Harmoniser advice: the latency line says 29 ms at 44.1 kHz and 27 ms at 48 kHz");
        const auto text = joined (h::getAdvice (harmtests::makeLog ({ { 250, 60.0f, 4.0f, quiet } }), cMajorSettings(), cKey, 100.0, line));
        check (text.contains (line) && ! text.contains ("followed the lead"), "Harmoniser advice: the latency line is shown, and the nothing-to-advise sentence is not");

        std::vector<LogPart> parts;

        for (const float note : { 60.0f, 62.0f, 64.0f, 65.0f, 67.0f, 69.0f, 71.0f })
            parts.push_back ({ 60, note, note == 60.0f || note == 65.0f || note == 67.0f ? 4.0f : 3.0f, quiet });

        parts.push_back ({ 40, -1.0f, 0.0f, 0.0f });
        const auto fine = joined (h::getAdvice (harmtests::makeLog (parts), cMajorSettings(), cKey, 100.0, {}));
        check (fine.contains ("The voices followed the lead on 91% of the loud sound, all inside the key."), "Harmoniser advice: nothing to advise says how much of the loud sound the voices followed, all inside the key (" + fine + ")");
    }
}

static void runBuiltInHarmoniserTests()
{
    using namespace daw;
    namespace h = daw::harm;
    std::cout << "Built-in harmoniser" << std::endl;
    const int cMajor = tunetests::getCMajor();
    const double a3 = 220.0;

    // Latency and bypass.
    for (const double rate : { 44100.0, 48000.0 })
    {
        BuiltInHarmoniser plugin;
        plugin.prepareToPlay (rate, 512);
        check (plugin.getLatencySamples() == 1280 && BuiltInHarmoniser::getLatencyFor (rate) == 1280, "DawIt Harmoniser reports 1280 samples of latency at " + juce::String (rate) + " Hz");
    }

    {
        constexpr double rate = 48000.0;
        const auto buzz = tunetests::makeBuzz (tune::shiftHz (a3, -30.0), rate, 48000);

        for (const bool bypassed : { false, true })
        {
            BuiltInHarmoniser plugin;
            plugin.prepareToPlay (rate, 512);
            h::Settings s;
            s.mixPercent = 0.0f;
            plugin.setSettings (s);
            plugin.setSongContext (cMajor, 0);
            const auto out = tunetests::runThrough (plugin, buzz, buzz, 512, bypassed);
            check (juce::exactlyEqual (tunetests::maxDifference (out.left, buzz, 1280, 0), 0.0) && juce::exactlyEqual (tunetests::maxDifference (out.right, buzz, 1280, 0), 0.0),
                   juce::String (bypassed ? "bypassed" : "at Mix 0") + ", DawIt Harmoniser gives the input held back by its latency, bit for bit");
        }

        BuiltInHarmoniser plugin;
        plugin.prepareToPlay (rate, 512);
        std::vector<float> silence (48000, 0.0f);
        const auto out = tunetests::runThrough (plugin, silence, silence);
        double biggest = 0.0;

        for (const float v : out.left)
            biggest = juce::jmax (biggest, (double) std::abs (v));

        check (biggest < 1.0e-6, "Harmoniser: silence in, silence out");

        // The lead is the same at every setting: the latency doesn't move.
        BuiltInHarmoniser wet;
        wet.prepareToPlay (rate, 512);
        wet.setSettings (h::getPresets()[5].settings);
        wet.setSongContext (cMajor, 0);
        const auto both = tunetests::runThrough (wet, buzz, buzz);
        check (both.left.size() == buzz.size() && wet.getLatencySamples() == 1280, "Harmoniser: the latency stays 1280 with voices on");
    }

    // The voices' notes.
    {
        auto report = [] (const char* what, const std::pair<double, double>& r, double limit = 5.0)
        {
            check (std::abs (r.second) < limit, juce::String (what) + " (mean " + juce::String (r.first, 2) + ", worst " + juce::String (r.second, 2) + " cents)");
        };

        report ("Harmoniser: Fixed +4 on A3 gives 277.18 Hz, within 5 cents", harmtests::sing (harmtests::voiceOnly (h::Mode::fixed, 4), a3, 277.18));
        report ("Harmoniser: Fixed +7 on A3 gives 329.63 Hz, within 5 cents", harmtests::sing (harmtests::voiceOnly (h::Mode::fixed, 7), a3, 329.63));
        report ("Harmoniser: Fixed -12 on A3 gives 110.00 Hz, within 5 cents", harmtests::sing (harmtests::voiceOnly (h::Mode::fixed, -12), a3, 110.0));
        report ("Harmoniser: Key +2 steps on E4 in C major gives G4 392.00 Hz", harmtests::sing (harmtests::voiceOnly (h::Mode::key, 2), tune::getHz (64.0), 392.0, cMajor));
        report ("Harmoniser: Key +2 steps on F4 in C major gives A4 440.00 Hz", harmtests::sing (harmtests::voiceOnly (h::Mode::key, 2), tune::getHz (65.0), 440.0, cMajor));
        report ("Harmoniser: Key +4 steps on B4 in C major gives F5 698.46 Hz", harmtests::sing (harmtests::voiceOnly (h::Mode::key, 4), tune::getHz (71.0), 698.46, cMajor));
        report ("Harmoniser: Key +2 steps on F#4 in C major gives A#4 466.16 Hz", harmtests::sing (harmtests::voiceOnly (h::Mode::key, 2), tune::getHz (66.0), 466.16, cMajor));
        report ("Harmoniser: Key with no song key follows C major (E4 gives G4)", harmtests::sing (harmtests::voiceOnly (h::Mode::key, 2), tune::getHz (64.0), 392.0, 0xfff));

        auto minor = harmtests::voiceOnly (h::Mode::key, 4);
        minor.key = 10;
        minor.scale = h::Scale::minor;
        report ("Harmoniser: a manual A minor with +4 steps on B3 gives F4 349.23 Hz", harmtests::sing (minor, tune::getHz (59.0), 349.23, 0xfff));

        report ("Harmoniser: the voice keeps the lead's cents (A3 30 cents flat, Fixed +4, gives 272.45 Hz)",
               harmtests::sing (harmtests::voiceOnly (h::Mode::fixed, 4), tune::shiftHz (a3, -30.0), tune::shiftHz (277.18, -30.0)));
        report ("Harmoniser: detune +7 cents on a unison gives 220.89 Hz within 2 cents", harmtests::sing (harmtests::voiceOnly (h::Mode::fixed, 0, 7.0f), a3, 220.89), 2.0);
    }

    // Pan and level.
    {
        constexpr double rate = 48000.0;
        auto run = [&] (float pan, float levelDb)
        {
            BuiltInHarmoniser plugin;
            plugin.prepareToPlay (rate, 512);
            auto s = harmtests::voiceOnly (h::Mode::fixed, 4);
            s.voices[0].panPercent = pan;
            s.voices[0].levelDb = levelDb;
            plugin.setSettings (s);
            const auto input = tunetests::makeBuzz (a3, rate, 48000);
            return tunetests::runThrough (plugin, input, input);
        };
        const auto centre = run (0.0f, 0.0f), left = run (-100.0f, 0.0f), quieter = run (0.0f, -6.0f);
        const double centreDb = 20.0 * std::log10 (harmtests::rms (centre.left, 20000, 40000) / harmtests::rms (left.left, 20000, 40000));
        check (std::abs (centreDb + 3.01) < 0.05, "Harmoniser: the centre pan is 3.01 dB under a voice panned fully left (" + juce::String (centreDb, 2) + " dB)");
        double rightPeak = 0.0;

        for (const float v : left.right)
            rightPeak = juce::jmax (rightPeak, (double) std::abs (v));

        check (rightPeak < 1.0e-6, "Harmoniser: a voice panned 100% left is silent on the right (" + juce::String (rightPeak) + ")");
        const double levelDb = 20.0 * std::log10 (harmtests::rms (quieter.left, 20000, 40000) / harmtests::rms (centre.left, 20000, 40000));
        check (std::abs (levelDb + 6.0) < 0.05, "Harmoniser: Level -6 dB makes the voice 6 dB quieter (" + juce::String (levelDb, 2) + " dB)");
    }

    // Delay: the voice starts 20 ms later.
    {
        constexpr double rate = 48000.0;
        std::vector<float> input (48000, 0.0f);
        const auto tone = tunetests::makeBuzz (a3, rate, 24000);
        std::copy (tone.begin(), tone.end(), input.begin() + 12000);
        auto onset = [&] (float delayMs)
        {
            BuiltInHarmoniser plugin;
            plugin.prepareToPlay (rate, 512);
            auto s = harmtests::voiceOnly (h::Mode::fixed, 4);
            s.voices[0].delayMs = delayMs;
            plugin.setSettings (s);
            const auto out = tunetests::runThrough (plugin, input, input);

            for (size_t i = 0; i < out.left.size(); ++i)
                if (std::abs (out.left[i]) > 0.02f)
                    return (double) i;

            return -1.0;
        };
        const double plain = onset (0.0f), delayed = onset (20.0f), ms = (delayed - plain) * 1000.0 / rate;
        check (plain > 12000.0 + 1280.0 && std::abs (ms - 20.0) < 3.0, "Harmoniser: a 20 ms delay starts the voice 20 ms later (" + juce::String (ms, 2) + " ms)");
    }

    // What isn't pitched: noise and notes below the shifter's reach.
    {
        constexpr double rate = 48000.0;
        auto voices = [&] (const std::vector<float>& input)
        {
            BuiltInHarmoniser plugin;
            plugin.prepareToPlay (rate, 512);
            plugin.setSettings (harmtests::voiceOnly (h::Mode::fixed, 4));
            plugin.setSongContext (cMajor, 0);
            return tunetests::runThrough (plugin, input, input);
        };

        juce::Random random (11);
        std::vector<float> noise (48000);

        for (auto& v : noise)
            v = (random.nextFloat() - 0.5f) * 0.6f;

        const auto noiseOut = voices (noise);
        const double noiseDb = 20.0 * std::log10 (harmtests::rms (noiseOut.left, 12000, 40000) / harmtests::rms (noise, 12000, 40000));
        check (noiseDb <= -40.0, "Harmoniser: white noise gives voices at least 40 dB below it (" + juce::String (noiseDb, 1) + " dB)");

        const auto low = tunetests::makeBuzz (80.0, rate, 48000);
        const auto lowOut = voices (low);
        const double lowDb = 20.0 * std::log10 (juce::jmax (1.0e-9, harmtests::rms (lowOut.left, 12000, 40000)) / harmtests::rms (low, 12000, 40000));
        check (lowDb <= -60.0, "Harmoniser: a sung 80 Hz note is below the shifter's reach and gets no voice (" + juce::String (lowDb, 1) + " dB)");

        // A voice fades away on a consonant between two notes.
        auto input = tunetests::makeBuzz (a3, rate, 48000);

        for (size_t i = 20000; i < 26000; ++i)
            input[i] = (random.nextFloat() - 0.5f) * 0.6f;

        const auto burst = voices (input);
        check (harmtests::rms (burst.left, 20000 + 1280 + 1500, 26000 + 1280 - 700) < 0.25 * harmtests::rms (burst.left, 12000, 18000),
               "Harmoniser: the voice fades out on a burst of noise between two notes");
    }

    // What it heard.
    {
        constexpr double rate = 48000.0;
        const auto input = tunetests::makeBuzz (a3, rate, 96000);
        BuiltInHarmoniser plugin;
        plugin.prepareToPlay (rate, 512);
        plugin.setSettings (harmtests::voiceOnly (h::Mode::fixed, 4));
        tunetests::FakePlayHead head;
        plugin.setPlayHead (&head);
        tunetests::runThrough (plugin, input, input, 512, false, &head);
        plugin.drainFrames();
        check (! plugin.getRecentFrames().empty() && std::abs (plugin.liveLead.load() - 57.0f) < 0.05f && std::abs (plugin.liveVoice[0].load() - 61.0f) < 0.05f && plugin.liveVoice[1].load() < 0.0f,
               "Harmoniser: the live lead is A3, voice 1 is C#4 and voice 2 is off (" + juce::String (plugin.liveLead.load(), 2) + ", " + juce::String (plugin.liveVoice[0].load(), 2) + ")");
        const auto log = plugin.getTakeLog();
        bool ordered = ! log.empty();

        for (size_t i = 1; i < log.size(); ++i)
            ordered = ordered && log[i].beat > log[i - 1].beat;

        check (ordered && log.back().beat > 3.5f && std::abs (log[log.size() / 2].voice[0] - 61.0f) < 0.05f && log[log.size() / 2].voice[1] < 0.0f, "Harmoniser: the take log has the frames, with the voices' notes, in song order");
        const size_t first = log.size();
        tunetests::runThrough (plugin, input, input, 512, false, &head);
        plugin.drainFrames();
        check (plugin.getTakeLog().size() <= first + 8, "Harmoniser: playing the part again replaces what was heard there");
        plugin.clearTakeLog();
        check (plugin.getTakeLog().empty(), "Harmoniser: the take log can be cleared");
        plugin.setPlayHead (nullptr);

        // The test sound plays through the effect.
        BuiltInHarmoniser sound;
        sound.prepareToPlay (rate, 512);
        sound.clap.request (fx::Clap::Sound::sungLine);
        std::vector<float> silence ((size_t) (rate * 6.0), 0.0f);
        const auto out = tunetests::runThrough (sound, silence, silence);
        double peak = 0.0;

        for (const float v : out.left)
            peak = juce::jmax (peak, (double) std::abs (v));

        check (peak > 0.3 && peak < 0.7, "Harmoniser: the test sound, a sung line, peaks near -6 dB (" + juce::String (20.0 * std::log10 (peak), 1) + " dB)");
    }

    // The song context reaches the plugin.
    {
        BuiltInHarmoniser plugin;
        plugin.setSongContext (cMajor, 0);
        check (plugin.getSongKeyMask() == cMajor && plugin.getEffectiveKeyMask() == cMajor, "Harmoniser: the plugin takes the key it is told");
    }

    checkThroughProject<BuiltInHarmoniser> (BuiltInHarmoniser::getDescription(), "DawIt Harmoniser",
        [] (BuiltInHarmoniser& plugin)
        {
            h::Settings s;
            s.mode = h::Mode::fixed;
            s.key = 4;
            s.scale = h::Scale::minor;
            s.mixPercent = 70.0f;
            s.voices[0] = { false, -5, 3, -12.0f, -35.0f, 9.0f, 18.0f };
            s.voices[1] = { true, 12, -6, 3.0f, 60.0f, -20.0f, 40.0f };
            plugin.setSettings (s);
        },
        [] (BuiltInHarmoniser& plugin)
        {
            const auto s = plugin.getSettings();
            return s.mode == h::Mode::fixed && s.key == 4 && s.scale == h::Scale::minor && std::abs (s.mixPercent - 70.0f) < 0.5f
                   && ! s.voices[0].on && s.voices[0].semitones == -5 && s.voices[0].steps == 3 && std::abs (s.voices[0].levelDb + 12.0f) < 0.1f && std::abs (s.voices[0].panPercent + 35.0f) < 0.5f
                   && std::abs (s.voices[0].detuneCents - 9.0f) < 0.5f && std::abs (s.voices[0].delayMs - 18.0f) < 0.5f
                   && s.voices[1].on && s.voices[1].semitones == 12 && s.voices[1].steps == -6 && std::abs (s.voices[1].levelDb - 3.0f) < 0.1f && std::abs (s.voices[1].panPercent - 60.0f) < 0.5f
                   && std::abs (s.voices[1].detuneCents + 20.0f) < 0.5f && std::abs (s.voices[1].delayMs - 40.0f) < 0.5f;
        });
}

static void runHarmoniserGuideTests()
{
    using namespace daw;
    namespace tg = daw::gen::guides;
    namespace hg = daw::gen::harmoniserguide;
    std::cout << "Harmoniser: the guide" << std::endl;

    bool texts = hg::numPages == 10, spelling = true;

    for (int i = 0; i < hg::numPages; ++i)
    {
        const auto text = hg::getPageText (i);
        texts = texts && hg::getPageName (i).isNotEmpty() && hg::getPageSummary (i).isNotEmpty() && text.length() >= 200;
        const auto all = (hg::getPageName (i) + text + hg::getPageSummary (i)).toLowerCase();
        spelling = spelling && ! all.contains ("color") && ! all.contains ("center") && ! all.contains ("analyze");
    }

    check (texts, "the harmoniser's guide has 10 pages, each with a name, a summary and text");
    check (spelling, "the harmoniser guide's texts use British spelling");
    check (hg::getPageName (2) == "Fixed or in the key" && hg::getPageText (1).contains ("554.4 Hz") && hg::getPageText (6).contains ("1,094 Hz") && hg::getPageText (5).contains ("Ken Townsend")
               && hg::getPageText (7).contains ("110 Hz") && hg::getPageText (3).contains ("Two voices are often enough"),
           "the harmoniser guide's texts carry the planned facts");

    bool controls = true;

    for (const auto c : { hg::Control::harmony, hg::Control::key, hg::Control::scale, hg::Control::mix, hg::Control::on, hg::Control::intervalFixed, hg::Control::intervalKey,
                          hg::Control::level, hg::Control::pan, hg::Control::detune, hg::Control::delay })
        controls = controls && hg::getControlText (c).length() > 20;

    check (controls && hg::getControlText (hg::Control::mix).contains ("At 50% both play at full level"), "every harmoniser control has its explanation");

    const auto entry = tg::getEntry (tg::Guide::harmoniser);
    check (entry.area == tg::Area::singing && entry.plugin == BuiltInHarmoniser::identifier && entry.name == "What is a harmoniser?", "the guide is in the Singing area and opens DawIt Harmoniser's pages");
    const auto startPath = tg::getStartPath();
    check (std::find (startPath.begin(), startPath.end(), tg::Guide::harmoniser) == startPath.end(), "the harmoniser guide is not on the Start here path");

    const auto entries = tg::getEntries();
    int index = -1;

    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].guide == tg::Guide::harmoniser)
            index = (int) i;

    bool found = index >= 0;

    for (const auto* query : { "harmonizer", "doubling", "diatonic", "harmoniser" })
    {
        const auto matches = tg::search (entries, query);
        found = found && std::any_of (matches.begin(), matches.end(), [&] (const auto& m) { return m.entry == index; });
    }

    check (found, "the hub finds the harmoniser guide for harmonizer, doubling, diatonic and harmoniser");
    check (gen::singing::explain (gen::singing::Topic::harmony, {}).contains (hg::getSingingParagraph()), "the singing guide's Harmonies and doubles points to DawIt Harmoniser and its guide");
}
