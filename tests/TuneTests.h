#pragma once

#include "engine/BuiltInFormat.h"
#include "engine/BuiltInTune.h"
#include "engine/Bounce.h"
#include "engine/Psola.h"
#include "engine/PitchTracker.h"
#include "engine/Tune.h"
#include "generate/SingingGuide.h"
#include "generate/TheoryGuides.h"
#include "generate/TuneAnalysis.h"
#include "generate/TuneGuideText.h"

/** DawIt Tune: the pitch tracker, the rules, the shifter and (further down) the processor and the engine. */

namespace tunetests
{
/** A buzz: harmonics of `hz` under the "ah" resonances, with the fundamental `fundamentalDb` against its natural level. */
inline std::vector<float> makeBuzz (double hz, double rate, int n, double fundamentalDb = 0.0)
{
    std::vector<float> x ((size_t) n, 0.0f);

    for (int k = 1; k * hz < rate * 0.45 && k < 40; ++k)
    {
        double a = daw::tune::SungVoice::getEnvelope (k * hz) / k;

        if (k == 1)
            a *= std::pow (10.0, fundamentalDb / 20.0);

        for (int i = 0; i < n; ++i)
            x[(size_t) i] += (float) (0.3 * a * std::sin (juce::MathConstants<double>::twoPi * k * hz * i / rate));
    }

    return x;
}

inline std::vector<float> makeSine (double hz, double rate, int n, double amplitude = 0.5)
{
    std::vector<float> x ((size_t) n);

    for (int i = 0; i < n; ++i)
        x[(size_t) i] = (float) (amplitude * std::sin (juce::MathConstants<double>::twoPi * hz * i / rate));

    return x;
}

/** How far the tracker's pitch is from `hz`, in cents; 1000 when it finds none. */
inline double trackedCents (const std::vector<float>& x, double hz, double rate)
{
    daw::pitch::Tracker tracker;
    const int w = daw::pitch::getWindowSize (rate);
    tracker.prepare (rate, w);
    const auto r = tracker.analyse (x.data() + (x.size() - (size_t) w) / 2, w);
    return r.voiced ? daw::tune::getCents (daw::tune::getHz (r.midi), hz) : 1000.0;
}

/** Constant period and ratio, for the shifter on its own. */
struct Constant final : daw::tune::PsolaControls
{
    float period = 0.0f, ratio = 1.0f;
    float getPeriod (juce::int64) const override { return period; }
    float getRatio (juce::int64) const override { return ratio; }
};
}

static void runTuneTrackerTests()
{
    using namespace daw;
    std::cout << "Tune: pitch tracker" << std::endl;
    constexpr double rate = 44100.0;
    check (pitch::getWindowSize (44100.0) == 2048 && pitch::getWindowSize (48000.0) == 2048 && pitch::getWindowSize (88200.0) == 4096
               && pitch::getWindowSize (96000.0) == 4096, "the tracker's window is 2048 samples at 44.1 and 48 kHz and 4096 at 88.2 and 96 kHz (40 ms or more)");

    for (const double hz : { 55.0, 110.0, 440.0, 1760.0 })
    {
        const auto cents = tunetests::trackedCents (tunetests::makeSine (hz, rate, 8192), hz, rate);
        check (std::abs (cents) < 3.0, "the tracker finds a " + juce::String (hz) + " Hz sine within 3 cents (" + juce::String (cents, 2) + ")");
    }

    {
        std::vector<float> saw (8192);

        for (size_t i = 0; i < saw.size(); ++i)
            saw[i] = (float) (0.5 * soundguide::getSample (soundguide::Shape::saw, (double) i * 110.0 / rate, 110.0 / rate));

        const auto cents = tunetests::trackedCents (saw, 110.0, rate);
        check (std::abs (cents) < 3.0, "the tracker finds a 110 Hz saw within 3 cents (" + juce::String (cents, 2) + ")");
    }

    {
        const auto cents = tunetests::trackedCents (tunetests::makeBuzz (110.0, rate, 8192, -20.0), 110.0, rate);
        check (std::abs (cents) < 3.0, "a buzz with its fundamental 20 dB under its second harmonic is still read at the fundamental, not an octave up (" + juce::String (cents, 2) + " cents)");
    }

    for (const double hz : { 98.0, 220.0, 392.0 })
    {
        const auto cents = tunetests::trackedCents (tunetests::makeBuzz (hz, rate, 8192), hz, rate);
        check (std::abs (cents) < 3.0, "the tracker finds an \"ah\" buzz at " + juce::String (hz) + " Hz within 3 cents (" + juce::String (cents, 2) + ")");
    }

    {
        pitch::Tracker tracker;
        tracker.prepare (rate, 2048);
        juce::Random random (11);
        std::vector<float> noise (2048);

        for (auto& v : noise)
            v = random.nextFloat() - 0.5f;

        check (! tracker.analyse (noise.data(), 2048).voiced, "white noise has no clear pitch");

        auto two = tunetests::makeSine (220.0, rate, 2048, 0.25);
        const auto other = tunetests::makeSine (311.13, rate, 2048, 0.25);

        for (size_t i = 0; i < two.size(); ++i)
            two[i] += other[i];

        check (! tracker.analyse (two.data(), 2048).voiced, "two notes a tritone apart have no clear pitch");

        std::vector<float> quiet (2048);
        const auto sine = tunetests::makeSine (220.0, rate, 2048, 0.001);
        std::copy (sine.begin(), sine.end(), quiet.begin());
        check (! tracker.analyse (quiet.data(), 2048).voiced, "a sound under -50 dBFS is not followed");
    }

    {
        // The octave guard: a single reading an octave away from two that agree is dropped.
        pitch::Tracker tracker;
        tracker.prepare (rate, 2048);
        const auto a = tunetests::makeSine (220.0, rate, 2048), b = tunetests::makeSine (440.0, rate, 2048);
        tracker.analyse (a.data(), 2048, pitch::Tracker::minClarity, true);
        tracker.analyse (a.data(), 2048, pitch::Tracker::minClarity, true);
        const bool lone = tracker.analyse (b.data(), 2048, pitch::Tracker::minClarity, true).voiced;
        const bool kept = tracker.analyse (b.data(), 2048, pitch::Tracker::minClarity, true).voiced;
        check (! lone && kept, "the octave guard drops one reading an octave away but follows the pitch when it stays there");
    }
}

static void runTuneRulesTests()
{
    using namespace daw;
    using namespace daw::tune;
    std::cout << "Tune: rules" << std::endl;
    auto near = [] (double a, double b, double tolerance) { return std::abs (a - b) <= tolerance; };

    check (near (getCents (432.44, 440.0), -30.0, 0.05) && near (getHz (69.0 - 0.30), 432.44, 0.01) && near (getRatio (100.0), 1.059463, 1.0e-6),
           "cents of 432.44 Hz against A4 are -30, 69 minus 0.30 is 432.44 Hz, and 100 cents is a ratio of 1.059463");
    check (near (shiftHz (440.0, -20.0), 434.95, 0.01) && near (shiftHz (440.0, 50.0), 452.89, 0.01) && near (110.0 - shiftHz (110.0, -20.0), 1.26, 0.01),
           "20 cents flat of A4 is 434.95 Hz, 50 cents sharp is 452.89 Hz, and 20 cents at A2 is only 1.26 Hz");
    check (getZone (10.0) == Zone::inTune && getZone (-10.5) == Zone::close && getZone (25.0) == Zone::close && getZone (26.0) == Zone::out, "10 cents is in tune, 25 close, beyond that out");

    const int cMajor = 0b101010110101, aMinor = 0b101101011010 >> 0;
    (void) aMinor;
    const int cMajorMask = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11);
    const int aNaturalMinor = (1 << 9) | (1 << 11) | (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7);
    const int aHarmonicMinor = (1 << 9) | (1 << 11) | (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 8);
    const int amChord = (1 << 9) | (1 << 0) | (1 << 4);
    (void) cMajor;
    check (chooseTarget (66.10, cMajorMask, -1) == 67 && chooseTarget (65.90, cMajorMask, -1) == 65 && chooseTarget (66.00, cMajorMask, -1) == 65,
           "in C major an F sharp 10 cents sharp goes to G, 10 cents flat to F, and exactly between to the lower note");
    check (chooseTarget (66.10, cMajorMask, 65) == 65 && chooseTarget (66.20, cMajorMask, 65) == 67 && chooseTarget (65.90, cMajorMask, 67) == 67 && chooseTarget (65.80, cMajorMask, 67) == 65,
           "a note stays on its target until it is 15 cents past the halfway point");
    check (chooseTarget (66.40, allNotes, -1) == 66 && chooseTarget (66.60, allNotes, -1) == 67, "chromatic goes to the nearest semitone");
    check (chooseTarget (68.00, aNaturalMinor, -1) == 67 && chooseTarget (68.00, aHarmonicMinor, -1) == 68 && chooseTarget (71.00, amChord, -1) == 72,
           "G sharp is pulled to G in A minor, stays in A harmonic minor, and B goes to C under an Am chord");
    check (getTargetMask (Target::key, cMajorMask, amChord) == cMajorMask && getTargetMask (Target::chords, cMajorMask, amChord) == amChord
               && getTargetMask (Target::chords, cMajorMask, 0) == cMajorMask && getTargetMask (Target::chromatic, cMajorMask, amChord) == allNotes,
           "the target mask is the key, the chord (the key in a bar without one) or every note");
    check (near (applyTolerance (8.0, 10.0), 0.0, 0.001) && near (applyTolerance (30.0, 10.0), 30.0, 0.001) && near (applyAmount (30.0, 60.0), 18.0, 0.001),
           "tolerance leaves 8 cents alone and corrects 30, and 60 percent of 30 cents is 18");

    {
        Glide glide;
        double at20 = 0.0, at60 = 0.0;

        for (int ms = 1; ms <= 60; ++ms)
        {
            const double v = glide.advance (100.0, 0.001, 60.0);

            if (ms == 20) at20 = v;
            if (ms == 60) at60 = v;
        }

        Glide instant;
        check (near (at20, 63.2, 0.1) && near (at60, 95.0, 0.1) && juce::exactlyEqual (instant.advance (100.0, 0.001, 0.0), 100.0),
               "a 60 ms glide is 63.2 cents of 100 after 20 ms and 95.0 after 60 ms, and 0 ms jumps at once");
    }

    {
        constexpr double fps = 172.265625;
        CentreFilter filter;
        filter.prepare (fps);
        double peak = 0.0;

        for (int i = 0; i < 4000; ++i)
        {
            const double y = filter.process (std::sin (juce::MathConstants<double>::twoPi * 5.5 * i / fps));

            if (i > 2000)
                peak = juce::jmax (peak, std::abs (y));
        }

        filter.restart (-30.0);
        const double held = filter.process (-30.0);
        check (near (peak, 0.074, 0.005) && near (held, -30.0, 1.0e-9), "the vibrato filter passes 0.074 of a 5.5 Hz swing and a restart holds the value it started on");
    }

    check (BuiltInTune::getLatencyFor (44100.0) == 1280 && BuiltInTune::getLatencyFor (48000.0) == 1280 && BuiltInTune::getLatencyFor (96000.0) == 2560,
           "the latency is 1280 samples at 44.1 and 48 kHz and 2560 at 96 kHz");
    check (near (1280.0 / 48000.0 * 1000.0, 26.7, 0.05) && near (1280.0 / 44100.0 * 1000.0, 29.0, 0.05) && near (60.0 / 120.0 / 4.0 * 1000.0, 125.0, 0.001), "1280 samples are 26.7 ms at 48 kHz and 29.0 ms at 44.1 kHz; a 16th at 120 BPM is 125 ms");

    // The corrector on frames.
    auto rows = [&] (const std::vector<float>& frames, const Settings& s, int mask) { return simulate (frames, s, mask, 200.0); };
    const Settings hard = getPresets()[3].settings;
    {
        std::vector<float> flat (100, 69.0f - 0.30f);
        const auto out = rows (flat, hard, cMajorMask);
        check (std::abs (out[0] - 69.0f) < 0.001f && std::abs (out[6] - 69.0f) < 0.001f, "at 0 ms the note is on its target from the first frame (within 10 cents by 30 ms)");

        auto natural = getPresets()[1].settings;
        const auto slow = rows (flat, natural, cMajorMask);
        check (std::abs (slow[1] - 69.0f) > 0.1f && std::abs (slow[10] - 69.0f) < 0.05f && std::abs (slow[99] - 69.0f) < 0.01f, "at 50 ms the note slides to its target, 95% of the way there after 50 ms");

        const auto checkOnly = rows (flat, getPresets()[0].settings, cMajorMask);
        check (std::abs (checkOnly[99] - flat[99]) < 1.0e-6f, "in Check mode nothing is moved");
    }

    {
        std::vector<float> frames;

        for (int i = 0; i < 400; ++i)
            frames.push_back ((float) (69.0 + (-30.0 + 40.0 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * i / 200.0)) * 0.01));

        auto measure = [&] (const std::vector<float>& out, double& centre, double& depth)
        {
            double lo = 1e9, hi = -1e9, sum = 0.0; int count = 0;

            for (size_t i = 200; i < out.size(); ++i)
            {
                const double cents = (out[i] - 69.0) * 100.0;
                lo = juce::jmin (lo, cents); hi = juce::jmax (hi, cents); sum += cents; ++count;
            }

            centre = sum / count; depth = (hi - lo) * 0.5;
        };

        double centre = 0.0, depth = 0.0;
        measure (rows (frames, getPresets()[1].settings, cMajorMask), centre, depth);
        check (near (centre, 0.0, 5.0) && near (depth, 40.0, 10.0), "Natural fix moves the middle of a vibrato note from -30 cents to 0 and keeps its swing (centre " + juce::String (centre, 1) + ", depth " + juce::String (depth, 1) + ")");
        double centre2 = 0.0, depth2 = 0.0;
        measure (rows (frames, hard, cMajorMask), centre2, depth2);
        check (depth2 < 12.0, "Hard tune with Keep vibrato off flattens the swing (depth " + juce::String (depth2, 1) + ")");
    }

    check (getPresets().size() == 4 && getPresets()[0].name == "Check only" && getPresets()[1].name == "Natural fix" && getPresets()[2].name == "Gentle polish" && getPresets()[3].name == "Hard tune",
           "DawIt Tune has the four starting points");

    for (const auto& p : getPresets())
        check (p.why.length() > 40, "starting point " + p.name + " says why");

    check (getPresets()[1].settings.mode == Mode::correct && near (getPresets()[1].settings.speedMs, 50.0, 0.001) && near (getPresets()[1].settings.toleranceCents, 10.0, 0.001)
               && near (getPresets()[2].settings.speedMs, 120.0, 0.001) && near (getPresets()[2].settings.amountPercent, 60.0, 0.001) && near (getPresets()[2].settings.toleranceCents, 15.0, 0.001)
               && ! getPresets()[3].settings.keepVibrato && near (getPresets()[3].settings.speedMs, 0.0, 0.001),
           "the starting points' numbers are the planned ones");

    // The made-up take is where its notes say.
    {
        const auto take = makeTake (getExamplePhraseNotes(), 44100.0, 3.0);
        check (take.getNumSamples() == 132300 && take.getMagnitude (0, 0, take.getNumSamples()) > 0.1f && take.getMagnitude (0, 0, take.getNumSamples()) < 1.0f, "the made-up take is 3 seconds long and not too loud");
        const auto frames = makeFrames (getExamplePhraseNotes(), 3.0);
        check (frames.size() == 600 && frames[0] < 64.0f && std::abs (frames[100] - 63.9f) < 0.001f && frames[190] < 0.0f && std::abs (frames[599] - 68.7f) < 0.01f,
               "the example phrase starts E4 150 cents flat, rests between notes and ends A4 30 cents flat");
    }
}

static void runPsolaTests()
{
    using namespace daw;
    std::cout << "Tune: shifter" << std::endl;
    constexpr double rate = 44100.0;
    const int latency = 1280;

    auto run = [&] (const std::vector<float>& input, float period, float ratio)
    {
        tune::PsolaShifter shifter;
        shifter.prepare (rate, latency);
        tunetests::Constant controls;
        controls.period = period;
        controls.ratio = ratio;
        std::vector<float> out (input.size());
        const float* in[2] = { nullptr, nullptr };
        float* outs[2] = { nullptr, nullptr };

        for (size_t at = 0; at < input.size(); at += 512)
        {
            const int n = (int) juce::jmin ((size_t) 512, input.size() - at);
            in[0] = in[1] = input.data() + at;
            outs[0] = out.data() + at;
            std::vector<float> second ((size_t) n);
            outs[1] = second.data();
            shifter.process (in, outs, 2, n, controls);
        }

        return out;
    };

    {
        const auto input = tunetests::makeBuzz (150.0, rate, 88200);
        const auto same = run (input, (float) (rate / 150.0), 1.0f);
        double diff = 0.0;

        for (size_t i = (size_t) latency; i < input.size(); ++i)
            diff = juce::jmax (diff, (double) std::abs (same[i] - input[i - (size_t) latency]));

        check (diff < 1.0e-9, "with no move the shifter gives the input held back by its latency, bit for bit (" + juce::String (diff, 12) + ")");

        const auto up = run (input, (float) (rate / 150.0), (float) tune::getRatio (400.0));
        constexpr int size = 32768;
        juce::dsp::FFT fft (15);
        std::vector<float> data ((size_t) size * 2, 0.0f);

        for (int i = 0; i < size; ++i)
            data[(size_t) i] = up[(size_t) (20000 + i)] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / size));

        fft.performFrequencyOnlyForwardTransform (data.data());
        int best = 0;

        for (int b = (int) (500.0 * size / rate); b < (int) (1500.0 * size / rate); ++b)
            if (data[(size_t) b] > data[(size_t) best] || best == 0)
                best = b;

        const double strongest = best * rate / size;
        check (std::abs (strongest - 730.0) < 73.0, "after PSOLA up four semitones the strongest harmonic in 500 to 1,500 Hz is near the unmoved formant, not near 920 Hz (" + juce::String (strongest, 0) + " Hz)");
    }
}

namespace tunetests
{
/** A play head that says where the song is, advanced by the test as it feeds blocks. */
struct FakePlayHead final : juce::AudioPlayHead
{
    double beat = 0.0, bpm = 120.0;
    bool playing = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm (bpm);
        info.setPpqPosition (beat);
        info.setIsPlaying (playing);
        return info;
    }
};

struct Rendered
{
    std::vector<float> left, right;
};

/** Feeds a stereo signal through a plugin in blocks and collects what comes out. */
inline Rendered runThrough (juce::AudioPluginInstance& plugin, const std::vector<float>& left, const std::vector<float>& right, int block = 512,
                            bool bypassed = false, FakePlayHead* head = nullptr, double rate = 48000.0)
{
    Rendered out;
    out.left.resize (left.size());
    out.right.resize (left.size());
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    const double samplesPerBeat = rate * 60.0 / (head != nullptr ? head->bpm : 120.0);

    for (size_t at = 0; at < left.size(); at += (size_t) block)
    {
        const int n = (int) juce::jmin ((size_t) block, left.size() - at);
        juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), 2, n);

        for (int i = 0; i < n; ++i)
        {
            view.setSample (0, i, left[at + (size_t) i]);
            view.setSample (1, i, right[at + (size_t) i]);
        }

        if (head != nullptr)
            head->beat = (double) at / samplesPerBeat;

        if (bypassed)
            plugin.processBlockBypassed (view, midi);
        else
            plugin.processBlock (view, midi);

        for (int i = 0; i < n; ++i)
        {
            out.left[at + (size_t) i] = view.getSample (0, i);
            out.right[at + (size_t) i] = view.getSample (1, i);
        }
    }

    return out;
}

/** The biggest difference between what came out and the input held back by `delay`, from sample `from` on. */
inline double maxDifference (const std::vector<float>& out, const std::vector<float>& in, int delay, size_t from)
{
    double worst = 0.0;

    for (size_t i = juce::jmax (from, (size_t) delay); i < out.size(); ++i)
        worst = juce::jmax (worst, (double) std::abs (out[i] - in[i - (size_t) delay]));

    return worst;
}

/** The frequency of a steady sound from the intervals between its upward zero crossings, in cents from `reference` Hz. */
inline double crossingCents (const std::vector<float>& x, size_t from, size_t to, double rate, double reference)
{
    std::vector<double> crossings;

    for (size_t i = from + 1; i < to; ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            crossings.push_back ((double) (i - 1) + (double) (-x[i - 1]) / (double) (x[i] - x[i - 1]));

    if (crossings.size() < 3)
        return 1000.0;

    const double hz = rate * (double) (crossings.size() - 1) / (crossings.back() - crossings.front());
    return daw::tune::getCents (hz, reference);
}

/** The mean and the worst distance in cents of the pitch the tracker hears in a stretch of audio from a target frequency. */
inline std::pair<double, double> trackedStretch (const std::vector<float>& x, size_t from, size_t to, double rate, double targetHz)
{
    daw::pitch::Tracker tracker;
    const int w = daw::pitch::getWindowSize (rate);
    tracker.prepare (rate, w);
    double sum = 0.0, worst = 0.0;
    int count = 0;

    for (size_t at = from; at + (size_t) w <= to; at += 256)
    {
        const auto r = tracker.analyse (x.data() + at, w);
        const double cents = r.voiced ? daw::tune::getCents (daw::tune::getHz (r.midi), targetHz) : 1000.0;
        sum += cents;
        worst = juce::jmax (worst, std::abs (cents));
        ++count;
    }

    return { count > 0 ? sum / count : 1000.0, count > 0 ? worst : 1000.0 };
}

inline int getCMajor() { return (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11); }
}

static void runBuiltInTuneTests()
{
    using namespace daw;
    std::cout << "Built-in tune" << std::endl;
    auto near = [] (double a, double b, double tolerance) { return std::abs (a - b) <= tolerance; };

    // Latency and the held-back sound, at the three rates.
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
    {
        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        const int expected = rate > 60000.0 ? 2560 : 1280;
        const int n = (int) (rate * 0.6);
        auto buzz = tunetests::makeBuzz (tune::shiftHz (220.0, -30.0), rate, n);
        const auto out = tunetests::runThrough (plugin, buzz, buzz, 512, false, nullptr, rate);
        check (plugin.getLatencySamples() == expected, "DawIt Tune reports " + juce::String (expected) + " samples of latency at " + juce::String (rate) + " Hz");
        check (tunetests::maxDifference (out.left, buzz, expected, 0) < 1.0e-6 && tunetests::maxDifference (out.right, buzz, expected, 0) < 1.0e-6,
               "in Check mode DawIt Tune gives the input held back by its latency, at " + juce::String (rate) + " Hz");
    }

    {
        BuiltInTune plugin;
        plugin.prepareToPlay (48000.0, 512);
        std::vector<float> silence (48000, 0.0f);
        const auto out = tunetests::runThrough (plugin, silence, silence);
        double biggest = 0.0;

        for (const float v : out.left)
            biggest = juce::jmax (biggest, (double) std::abs (v));

        check (biggest < 1.0e-7, "silence in, silence out");
    }

    // Check mode, bypass, Hear the original, all give the delayed input even when the take is out of tune.
    {
        const auto buzz = tunetests::makeBuzz (tune::getHz (57.0) * tune::getRatio (-30.0), 48000.0, 48000);

        BuiltInTune check1;
        check1.prepareToPlay (48000.0, 512);
        check1.setSongContext (tunetests::getCMajor(), 0);
        auto o1 = tunetests::runThrough (check1, buzz, buzz);
        check (tunetests::maxDifference (o1.left, buzz, 1280, 0) < 1.0e-6, "Check mode leaves an out-of-tune take as it is, held back by the latency");

        BuiltInTune hear;
        hear.prepareToPlay (48000.0, 512);
        hear.setSettings (tune::getPresets()[3].settings);
        hear.hearOriginal = true;
        auto o2 = tunetests::runThrough (hear, buzz, buzz);
        check (tunetests::maxDifference (o2.left, buzz, 1280, 0) < 1.0e-6, "Hear the original gives the take as it is, held back by the latency");

        BuiltInTune bypassed;
        bypassed.prepareToPlay (48000.0, 512);
        bypassed.setSettings (tune::getPresets()[3].settings);
        auto o3 = tunetests::runThrough (bypassed, buzz, buzz, 512, true);
        check (tunetests::maxDifference (o3.left, buzz, 1280, 0) < 1.0e-6, "bypassed, DawIt Tune still holds the sound back by its latency");

        BuiltInTune hard;
        hard.prepareToPlay (48000.0, 512);
        hard.setSettings (tune::getPresets()[3].settings);
        hard.setSongContext (tunetests::getCMajor(), 0);
        auto o4 = tunetests::runThrough (hard, buzz, buzz);
        check (tunetests::maxDifference (o4.left, buzz, 1280, 4000) > 0.01, "Hard tune does change an out-of-tune take");
    }

    // Hard tune brings a note 30 cents flat to the note, and leaves a note in tune alone.
    for (const bool sine : { true, false })
    {
        constexpr double rate = 48000.0;
        const double flatHz = sine ? tune::getHz (69.0) * tune::getRatio (-30.0) : tune::getHz (57.0) * tune::getRatio (-30.0);
        const double targetHz = sine ? 440.0 : 220.0;
        const int n = (int) (rate * 1.2);
        const auto input = sine ? tunetests::makeSine (flatHz, rate, n) : tunetests::makeBuzz (flatHz, rate, n);
        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        plugin.setSettings (tune::getPresets()[3].settings);
        plugin.setSongContext (tunetests::getCMajor(), 0);
        const auto out = tunetests::runThrough (plugin, input, input);
        const auto [mean, worst] = tunetests::trackedStretch (out.left, (size_t) (rate * 0.2), (size_t) (rate * 1.0), rate, targetHz);
        check (std::abs (worst) < 5.0, juce::String (sine ? "a 432.44 Hz sine" : "an \"ah\" at A3 minus 30 cents") + " under Hard tune is within 5 cents of its note from 0.2 to 1.0 s (mean " + juce::String (mean, 2) + ", worst " + juce::String (worst, 2) + ")");
    }

    {
        constexpr double rate = 48000.0;
        const auto input = tunetests::makeSine (440.0, rate, 48000);
        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        plugin.setSettings (tune::getPresets()[1].settings);
        plugin.setSongContext (tunetests::getCMajor(), 0);
        const auto out = tunetests::runThrough (plugin, input, input);
        check (tunetests::maxDifference (out.left, input, 1280, 0) < 1.0e-6, "Natural fix leaves a 440 Hz sine in C major as it is");
    }

    // A note that starts in the middle of silence is corrected within 30 ms.
    {
        constexpr double rate = 48000.0;
        const size_t start = 24000;
        std::vector<float> input (48000, 0.0f);
        const auto sine = tunetests::makeSine (tune::getHz (69.0) * tune::getRatio (-30.0), rate, 24000);
        std::copy (sine.begin(), sine.end(), input.begin() + (std::ptrdiff_t) start);
        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        plugin.setSettings (tune::getPresets()[3].settings);
        plugin.setSongContext (tunetests::getCMajor(), 0);
        const auto out = tunetests::runThrough (plugin, input, input);
        const size_t from = start + 1280 + (size_t) (rate * 0.030), to = from + (size_t) (rate * 0.020);
        const double cents = tunetests::crossingCents (out.left, from, to, rate, 440.0);
        check (std::abs (cents) < 10.0, "at 0 ms the output is within 10 cents of its note 30 ms after the note starts (" + juce::String (cents, 2) + " cents)");
    }

    // A noise burst between two notes passes through as it came in.
    {
        constexpr double rate = 48000.0;
        auto input = tunetests::makeBuzz (tune::getHz (57.0) * tune::getRatio (-30.0), rate, 48000);
        juce::Random random (5);
        const size_t burstFrom = 20000, burstTo = 26000;

        for (size_t i = burstFrom; i < burstTo; ++i)
            input[i] = (random.nextFloat() - 0.5f) * 0.6f;

        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        plugin.setSettings (tune::getPresets()[3].settings);
        plugin.setSongContext (tunetests::getCMajor(), 0);
        const auto out = tunetests::runThrough (plugin, input, input);
        double burst = 0.0, error = 0.0;

        for (size_t i = burstFrom + 1280 + 700; i < burstTo + 1280 - 700; ++i)
        {
            burst += (double) input[i - 1280] * input[i - 1280];
            error += (double) (out.left[i] - input[i - 1280]) * (out.left[i] - input[i - 1280]);
        }

        const double db = 10.0 * std::log10 (juce::jmax (1.0e-20, error) / burst);
        check (db < -60.0, "a noise burst between notes comes out as it went in (" + juce::String (db, 1) + " dB below the burst)");
    }

    // The off-note test sound keeps its level.
    {
        constexpr double rate = 48000.0;
        std::vector<float> silence ((size_t) (rate * 3.0), 0.0f);
        auto render = [&] (const tune::Settings& s)
        {
            BuiltInTune plugin;
            plugin.prepareToPlay (rate, 512);
            plugin.setSettings (s);
            plugin.setSongContext (tunetests::getCMajor(), 0);
            plugin.clap.request (fx::Clap::Sound::offNote);
            return tunetests::runThrough (plugin, silence, silence);
        };
        const auto original = render (tune::getPresets()[0].settings), fixed = render (tune::getPresets()[1].settings);
        auto stats = [] (const std::vector<float>& x) { double peak = 0.0, sum = 0.0; for (const float v : x) { peak = juce::jmax (peak, (double) std::abs (v)); sum += (double) v * v; } return std::pair<double, double> { peak, std::sqrt (sum / (double) x.size()) }; };
        const auto [peakIn, rmsIn] = stats (original.left);
        const auto [peakOut, rmsOut] = stats (fixed.left);
        const double peakDb = 20.0 * std::log10 (peakOut / peakIn), rmsDb = 20.0 * std::log10 (rmsOut / rmsIn);
        check (peakDb <= 1.0 && std::abs (rmsDb) < 0.5, "the off note keeps its level through Natural fix (peak " + juce::String (peakDb, 2) + " dB, RMS " + juce::String (rmsDb, 2) + " dB)");
        check (peakIn > 0.3 && peakIn < 0.7, "the off-note sound peaks near -6 dB (" + juce::String (20.0 * std::log10 (peakIn), 1) + " dB)");
        const auto [mean, worst] = tunetests::trackedStretch (fixed.left, (size_t) (rate * 0.9), (size_t) (rate * 2.0), rate, 220.0);
        const auto [meanIn, worstIn] = tunetests::trackedStretch (original.left, (size_t) (rate * 0.9), (size_t) (rate * 2.0), rate, 220.0);
        std::cout << "  (off note: mean " << mean << " cents after Natural fix, " << meanIn << " before)" << std::endl;
        check (std::abs (meanIn + 30.0) < 6.0 && std::abs (mean) < 12.0, "Natural fix brings the off note from about 30 cents flat to within 12 cents of A3 (" + juce::String (meanIn, 1) + " to " + juce::String (mean, 1) + ")");
    }

    // Speed: 10 seconds in under a second.
    {
        constexpr double rate = 48000.0;
        const auto input = tunetests::makeBuzz (tune::getHz (57.0) * tune::getRatio (-30.0), rate, 480000);
        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        plugin.setSettings (tune::getPresets()[3].settings);
        plugin.setSongContext (tunetests::getCMajor(), 0);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        tunetests::runThrough (plugin, input, input);
        const double took = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;
        check (took < 1.0, "10 seconds of audio in Correct mode take " + juce::String (took, 2) + " s to process");
    }

    // What it heard: frames, the live reading, the take log.
    {
        constexpr double rate = 48000.0;
        const auto input = tunetests::makeBuzz (220.0, rate, 96000);
        BuiltInTune plugin;
        plugin.prepareToPlay (rate, 512);
        tunetests::FakePlayHead head;
        plugin.setPlayHead (&head);
        tunetests::runThrough (plugin, input, input, 512, false, &head);
        plugin.drainFrames();
        check (! plugin.getRecentFrames().empty() && near (plugin.liveSung.load(), 57.0f, 0.05), "the live reading follows the sung note (" + juce::String (plugin.liveSung.load(), 2) + ")");
        const auto log = plugin.getTakeLog();
        bool ordered = ! log.empty();

        for (size_t i = 1; i < log.size(); ++i)
            ordered = ordered && log[i].beat > log[i - 1].beat;

        check (ordered && log.back().beat > 3.5f && near (log[log.size() / 2].sung, 57.0f, 0.05), "the take log has the frames in song order");
        const size_t first = log.size();

        // Playing again replaces what was heard.
        tunetests::runThrough (plugin, input, input, 512, false, &head);
        plugin.drainFrames();
        std::cout << "  (log " << first << " then " << plugin.getTakeLog().size() << ")" << std::endl;
        check (plugin.getTakeLog().size() <= first + 8, "playing the part again replaces what was heard there");
        plugin.clearTakeLog();
        check (plugin.getTakeLog().empty(), "the take log can be cleared");
        plugin.setPlayHead (nullptr);
    }

    // The song context reaches the plugin.
    {
        BuiltInTune plugin;
        plugin.setSongContext (tunetests::getCMajor(), 0);
        check (plugin.getKeyMask() == tunetests::getCMajor() && plugin.getChordMask() == 0, "the plugin takes the key and chord it is told");
    }

    checkThroughProject<BuiltInTune> (BuiltInTune::getDescription(), "DawIt Tune",
        [] (BuiltInTune& plugin) { plugin.setSettings ({ tune::Mode::correct, tune::Target::chromatic, 120.0f, 60.0f, 15.0f, false }); },
        [] (BuiltInTune& plugin)
        {
            const auto s = plugin.getSettings();
            return s.mode == tune::Mode::correct && s.target == tune::Target::chromatic && std::abs (s.speedMs - 120.0f) < 0.5f
                   && std::abs (s.amountPercent - 60.0f) < 0.5f && std::abs (s.toleranceCents - 15.0f) < 0.5f && ! s.keepVibrato;
        });
}

namespace tunetests
{
/** Two audio tracks with the same impulse clip at beat 4, one panned hard left and one hard right, the right one with plugins: the
    impulses must come out on the same sample if the plugins' latency is made up for. */
struct ImpulseSong
{
    ImpulseSong (const juce::String& plugin = BuiltInTuneId())
    {
        engine.deviceManager.removeAudioCallback (&engine);
        engine.audioDeviceAboutToStart (&device);
        plain = engine.addTrack (daw::Track::Type::audio);
        with = engine.addTrack (daw::Track::Type::audio);
        plain->pan = -1.0f;
        with->pan = 1.0f;
        juce::AudioBuffer<float> impulse (1, 48000 * 12);
        impulse.clear();
        impulse.setSample (0, 96000, 0.5f);   // beat 4 at 120 BPM

        for (auto* track : { plain, with })
            engine.addAudioClip (*track, impulse, 48000.0, 0.0, "Impulse");

        juce::String error;
        slot = engine.addPlugin (*with, findDawIt (plugin), error);
    }

    static juce::String BuiltInTuneId() { return daw::BuiltInTune::identifier; }

    static juce::PluginDescription findDawIt (const juce::String& name)
    {
        juce::OwnedArray<juce::PluginDescription> found;
        daw::BuiltInFormat format;
        format.findAllTypesForFile (found, name);
        return found.isEmpty() ? juce::PluginDescription() : *found.getFirst();
    }

    /** Plays `seconds` from `beat` and gives the sample of each channel's loudest value (-1 for silence). */
    std::pair<int, int> peaks (double beat, double seconds, juce::AudioBuffer<float>* keep = nullptr)
    {
        engine.stop();
        render (engine, 0.02);
        engine.setPositionBeats (beat);
        engine.play();
        auto out = render (engine, seconds);
        engine.stop();

        auto where = [&] (int ch)
        {
            int best = -1;
            float biggest = 0.01f;

            for (int i = 0; i < out.getNumSamples(); ++i)
                if (std::abs (out.getSample (ch, i)) > biggest) { biggest = std::abs (out.getSample (ch, i)); best = i; }

            return best;
        };

        if (keep != nullptr)
            *keep = out;

        return { where (0), where (1) };
    }

    daw::Engine engine;
    FakeDevice device;
    daw::Track* plain = nullptr;
    daw::Track* with = nullptr;
    daw::PluginSlot* slot = nullptr;
};
}

static void runTuneEngineTests()
{
    using namespace daw;
    std::cout << "Tune: the engine" << std::endl;

    // The song context: the key, and the chord of the bar being played.
    {
        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        FakeDevice device;
        engine.audioDeviceAboutToStart (&device);
        auto* track = engine.addTrack (Track::Type::audio);
        juce::String error;
        auto* slot = engine.addPlugin (*track, tunetests::ImpulseSong::findDawIt (BuiltInTune::identifier), error);
        auto* tuner = slot != nullptr ? dynamic_cast<BuiltInTune*> (slot->instance.get()) : nullptr;
        check (tuner != nullptr && slot->songContext != nullptr, "the engine finds out that DawIt Tune wants the song's key and chords");

        if (tuner != nullptr)
        {
            engine.setKey ({ 7, MusicalKey::Scale::major });
            engine.setChords ({ { 1, *Chord::fromName ("Am") } });
            engine.setPositionBeats (4.0);
            engine.play();
            render (engine, 0.02);
            const int gMajor = (1 << 7) | (1 << 9) | (1 << 11) | (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
            check (tuner->getKeyMask() == gMajor, "setKey (G major) reaches DawIt Tune after one block");
            check (tuner->getChordMask() == ((1 << 9) | (1 << 0) | (1 << 4)), "a bar with Am gives DawIt Tune A, C and E while it plays");
            engine.setPositionBeats (8.0);
            render (engine, 0.02);
            check (tuner->getChordMask() == 0 && tune::getTargetMask (tune::Target::chords, tuner->getKeyMask(), tuner->getChordMask()) == gMajor,
                   "a bar with no chord gives no chord notes, and Chords then uses the key");
            engine.stop();
        }
    }

    // Latency: the impulses land on the same sample.
    {
        tunetests::ImpulseSong song;
        check (song.slot != nullptr && song.with->getCompensation() == 1280 && song.plain->getCompensation() == 0,
               "a track with DawIt Tune on it makes up for 1280 samples");

        auto [left, right] = song.peaks (0.0, 5.0);
        check (left == 96000 && right == 96000, "an impulse at beat 4 lands on the same sample on a track with DawIt Tune (" + juce::String (left) + ", " + juce::String (right) + ")");

        auto [l2, r2] = song.peaks (3.95, 0.2);
        check (l2 == 1200 && r2 == 1200, "started 25 ms before it, inside the latency, it still lands on the same sample (" + juce::String (l2) + ", " + juce::String (r2) + ")");

        auto [l3, r3] = song.peaks (3.0, 2.5);
        check (l3 == 24000 && r3 == 24000, "started a beat before it, it lands on the same sample (" + juce::String (l3) + ", " + juce::String (r3) + ")");

        // Bypassed: the delay stays.
        song.slot->bypassed = true;
        auto [l4, r4] = song.peaks (3.95, 0.2);
        auto [l5, r5] = song.peaks (0.0, 5.0);
        check (l4 == 1200 && r4 == 1200 && l5 == 96000 && r5 == 96000, "bypassing DawIt Tune doesn't move the track in time");
        song.slot->bypassed = false;

        // A loop from beat 4 to 8: the impulse is at the start of every pass.
        song.engine.loopStartBeat = 4.0;
        song.engine.loopEndBeat = 8.0;
        song.engine.loopEnabled = true;
        juce::AudioBuffer<float> out;
        song.peaks (4.0, 6.5, &out);
        bool everyPass = true;
        int found = 0;

        for (int pass = 0; pass < 3; ++pass)
        {
            const int at = pass * 96000;

            if (at >= out.getNumSamples())
                break;

            ++found;
            everyPass = everyPass && std::abs (out.getSample (0, at)) > 0.1f && std::abs (out.getSample (1, at)) > 0.1f
                        && std::abs (out.getSample (1, at + 1)) < 0.01f && std::abs (out.getSample (1, at - (at > 0 ? 1 : 0))) < 0.01f + (at == 0 ? 1.0f : 0.0f);
        }

        check (found == 3 && everyPass, "on every pass of a loop the impulse is on the first sample, with and without DawIt Tune");
        song.engine.loopEnabled = false;
    }

    // A bounce lines up too.
    {
        tunetests::ImpulseSong song;
        const auto file = juce::File::createTempFile ("tune-bounce.wav");
        bounce::Settings settings;
        settings.startBeat = 0.0;
        settings.endBeat = 6.0;
        bounce::Outcome outcome;

        {
            bounce::Renderer renderer (song.engine, settings);
            outcome = renderer.render (file);
        }

        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        int left = -1, right = -1;

        if (reader != nullptr)
        {
            juce::AudioBuffer<float> audio (2, (int) reader->lengthInSamples);
            reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);

            for (int ch = 0; ch < 2; ++ch)
            {
                float biggest = 0.01f;

                for (int i = 0; i < audio.getNumSamples(); ++i)
                    if (std::abs (audio.getSample (ch, i)) > biggest) { biggest = std::abs (audio.getSample (ch, i)); (ch == 0 ? left : right) = i; }
            }
        }

        file.deleteFile();
        check (outcome.result.wasOk() && left == 96000 && right == 96000, "in a bounce the impulses land on the same sample with DawIt Tune (" + juce::String (left) + ", " + juce::String (right) + ")");
    }

    // Saturation and Distortion hold the sound back too, and are made up for the same way, bypassed or not.
    for (const auto* name : { "DawIt Saturation", "DawIt Distortion" })
    {
        tunetests::ImpulseSong song (name);
        const int compensation = song.with->getCompensation();
        auto [left, right] = song.peaks (0.0, 5.0);
        check (compensation > 0 && std::abs (left - right) <= 1, juce::String (name) + " makes up for its " + juce::String (compensation) + " samples of latency (" + juce::String (left) + ", " + juce::String (right) + ")");
        song.slot->bypassed = true;
        auto [l2, r2] = song.peaks (0.0, 5.0);
        check (song.with->getCompensation() == compensation && l2 == 96000 && r2 == 96000, juce::String (name) + " bypassed keeps the track in time (" + juce::String (r2) + ")");
    }
}

namespace tunetests
{
struct LogNote { double startBeat, beats; int midi; double cents; };

/** A made-up log: every note sung steadily at its cents from the note, frames every 6.25 ms at 120 BPM. */
inline std::vector<daw::tune::Frame> makeLog (const std::vector<LogNote>& notes, bool correcting = false, double correctionCents = 0.0)
{
    std::vector<daw::tune::Frame> log;

    for (const auto& n : notes)
        for (double beat = n.startBeat; beat < n.startBeat + n.beats; beat += 0.0125)
        {
            daw::tune::Frame f;
            f.beat = (float) beat;
            f.sung = (float) (n.midi + n.cents / 100.0);
            f.corrected = correcting ? f.sung + (float) (correctionCents / 100.0) : f.sung;
            f.rmsDb = -20.0f;
            log.push_back (f);
        }

    return log;
}
}

static void runTuneAnalysisTests()
{
    using namespace daw;
    using namespace daw::gen::tunecheck;
    using tunetests::LogNote;
    std::cout << "Tune: your track" << std::endl;
    const TempoMap tempo (120.0);
    const MusicalKey cMajor { 0, MusicalKey::Scale::major };
    const SectionList sections { { 0, 8, SectionType::verse, {} }, { 8, 8, SectionType::chorus, {} } };
    auto joined = [] (const std::vector<juce::String>& lines) { juce::StringArray all; for (const auto& l : lines) all.add (l); return all.joinIntoString ("\n"); };

    // Rule 1: not enough notes.
    {
        const auto lines = getAdvice (tunetests::makeLog ({ { 0.0, 1.0, 60, 0.0 }, { 2.0, 1.0, 62, 0.0 } }), tempo, cMajor, sections, false);
        check (lines.size() == 1 && lines[0].startsWith ("Play the song with DawIt Tune on this track."), "with fewer than 8 notes the advice says to play the song through");
        check (getAdvice ({}, tempo, cMajor, sections, false).size() == 1, "with nothing heard the advice says to play the song through");
    }

    // Rule 3: a flat verse.
    {
        std::vector<LogNote> notes;

        for (int i = 0; i < 8; ++i)
            notes.push_back ({ i * 2.0, 1.5, i % 2 == 0 ? 60 : 64, -20.0 });

        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, sections, false));
        check (text.contains ("The verse sits 20 cents flat on average.") && text.contains ("Your voice and your song"), "eight notes 20 cents flat in the verse give 'The verse sits 20 cents flat on average'");
        check (text.contains ("Of 8 notes, 8 (100%) are within 25 cents of a note, and 0 (0%) within 10."), "the overall line counts the notes");
        check (! text.contains ("Nicely in tune"), "a flat section is not nicely in tune");
        const auto analysis = analyse (tunetests::makeLog (notes), tempo, cMajor, sections);
        check (analysis.notes.size() == 8 && std::abs (analysis.notes[0].errorCents + 20.0) < 0.5 && analysis.notes[0].bar == 0 && analysis.sections.size() == 1,
               "the notes are found with their errors, bars and sections");
    }

    // Two sections with the same name are told apart by their bars.
    {
        std::vector<LogNote> notes;

        for (int i = 0; i < 6; ++i)
            notes.push_back ({ 16.0 + i * 1.5, 1.0, 62, -20.0 });

        for (int i = 0; i < 6; ++i)
            notes.push_back ({ 80.0 + i * 1.5, 1.0, 62, -20.0 });

        const SectionList twoVerses { { 0, 4, SectionType::intro, {} }, { 4, 8, SectionType::verse, {} }, { 20, 8, SectionType::verse, {} } };
        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, twoVerses, false));
        check (text.contains ("The verse (bars 5 to 12) sits 20 cents flat") && text.contains ("The verse (bars 21 to 28) sits 20 cents flat"), "two verses are told apart by their bars");
    }

    // Sharp, and one section only.
    {
        std::vector<LogNote> notes;

        for (int i = 0; i < 6; ++i)
            notes.push_back ({ 32.0 + i * 1.5, 1.0, 64, 15.0 });

        for (int i = 0; i < 3; ++i)
            notes.push_back ({ 4.0 + i * 1.5, 1.0, 60, 0.0 });

        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, sections, false));
        check (text.contains ("The chorus sits 15 cents sharp on average. Singing sharp often comes from pushing; try it a little lighter."), "a sharp chorus is said to sit sharp");
    }

    // Rule 4: lines that run out of breath.
    {
        std::vector<LogNote> notes;

        for (int line = 0; line < 5; ++line)
        {
            const double start = line * 6.0;
            notes.push_back ({ start, 1.0, 60, 0.0 });
            notes.push_back ({ start + 1.25, 1.0, 62, 0.0 });
            notes.push_back ({ start + 2.5, 1.0, 64, -22.0 });
        }

        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, sections, false));
        check (text.contains ("The last notes of your lines are 22 cents flatter than the rest, which often means running out of breath.") && text.contains ("Posture and breathing"),
               "last notes 22 cents flatter than the rest are said to mean running out of breath");
    }

    // Rule 5: wrong notes in the chorus.
    {
        std::vector<LogNote> notes;

        for (int i = 0; i < 5; ++i)
            notes.push_back ({ 4.0 + i * 1.5, 1.0, 62, 0.0 });

        notes.push_back ({ 16.0 * 4.0 + 0.0, 1.0, 66, 0.0 });   // bar 17 (from 1), in the chorus? bars 8 to 15 are the chorus: use bars below
        const SectionList wide { { 0, 8, SectionType::verse, {} }, { 8, 16, SectionType::chorus, {} } };
        notes.clear();

        for (int i = 0; i < 6; ++i)
            notes.push_back ({ 40.0 + i * 1.5, 1.0, 62, 0.0 });

        notes.push_back ({ 64.0 + 0.0, 1.0, 66, 0.0 });   // bar 17
        notes.push_back ({ 68.0, 1.0, 66, 0.0 });         // bar 18
        notes.push_back ({ 80.0, 1.0, 66, 0.0 });         // bar 21
        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, wide, false));
        check (text.contains ("3 notes in the chorus are F#, outside C major (bars 17, 18 and 21): wrong notes, or on purpose? Listen to them against the chords."),
               "three F sharps in the chorus in C major name the bars");
        check (! text.contains ("raised seventh"), "an F sharp in C major isn't a raised seventh");

        std::vector<LogNote> minor;

        for (int i = 0; i < 6; ++i)
            minor.push_back ({ 40.0 + i * 1.5, 1.0, 57, 0.0 });

        minor.push_back ({ 64.0, 1.0, 68, 0.0 });
        minor.push_back ({ 68.0, 1.0, 68, 0.0 });
        const auto aMinorText = joined (getAdvice (tunetests::makeLog (minor), tempo, { 9, MusicalKey::Scale::minor }, wide, false));
        check (aMinorText.contains ("2 notes in the chorus are G#, outside A minor (bars 17 and 18)") && aMinorText.contains ("G# is the raised seventh that harmonic minor uses, common in minor songs. If it's meant, set the key to A harmonic minor so DawIt Tune pulls to it."),
               "a G sharp in A minor adds the harmonic minor sentence");
    }

    // Rule 6: the furthest out.
    {
        std::vector<LogNote> notes;

        for (int i = 0; i < 6; ++i)
            notes.push_back ({ i * 1.5, 1.0, 60, 0.0 });

        notes.push_back ({ 36.0, 1.0, 69, -38.0 });
        notes.push_back ({ 44.0, 1.0, 72, 31.0 });
        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, sections, false));
        check (text.contains ("Furthest out: bar 10, A4, 38 cents flat; bar 12, C5, 31 cents sharp."), "the notes furthest out are named by bar, note and cents (" + text + ")");
        check (! joined (getAdvice (tunetests::makeLog ({ { 0.0, 1.0, 60, 5.0 }, { 2.0, 1.0, 62, 5.0 }, { 4.0, 1.0, 64, 5.0 }, { 6.0, 1.0, 65, 5.0 }, { 8.0, 1.0, 67, 5.0 }, { 10.0, 1.0, 69, 5.0 }, { 12.0, 1.0, 71, 5.0 }, { 14.0, 1.0, 72, 5.0 } }), tempo, cMajor, sections, false)).contains ("Furthest out"),
               "with no note beyond 25 cents nothing is said to be furthest out");
    }

    // Rule 9 and the percentages.
    {
        std::vector<LogNote> notes;
        const int pitches[] = { 60, 62, 64, 65, 67, 69, 71 };

        // 46 notes: 29 within 10 cents, 9 more within 25, 8 beyond.
        for (int i = 0; i < 46; ++i)
        {
            const double cents = i < 29 ? 4.0 : (i < 38 ? 18.0 : 30.0);
            notes.push_back ({ i * 1.25, 1.0, pitches[i % 7], i % 2 == 0 ? cents : -cents });
        }

        const auto text = joined (getAdvice (tunetests::makeLog (notes), tempo, cMajor, { { 0, 60, SectionType::verse, {} } }, false));
        check (text.contains ("Of 46 notes, 38 (83%) are within 25 cents of a note, and 29 (63%) within 10."), "38 of 46 notes within 25 cents is 83 percent (" + text + ")");

        std::vector<LogNote> good;

        for (int i = 0; i < 10; ++i)
            good.push_back ({ i * 1.25, 1.0, pitches[i % 7], i % 2 == 0 ? 6.0 : -6.0 });

        check (joined (getAdvice (tunetests::makeLog (good), tempo, cMajor, sections, false)).contains ("Nicely in tune. Correction would change little; if you use it, Gentle polish is enough."),
               "a take with nearly every note within 25 cents is nicely in tune");
    }

    // Rule 7: heavy correction.
    {
        std::vector<LogNote> notes;

        for (int i = 0; i < 8; ++i)
            notes.push_back ({ i * 1.25, 1.0, 60 + (i % 3) * 2, 0.0 });

        check (joined (getAdvice (tunetests::makeLog (notes, true, 46.0), tempo, cMajor, sections, true)).contains ("DawIt Tune is moving notes by 46 cents on average."), "heavy correction is said to sound processed");
        check (! joined (getAdvice (tunetests::makeLog (notes, true, 46.0), tempo, cMajor, sections, false)).contains ("moving notes"), "in Check mode nothing is said about correction");
    }

    // Rule 8: no clear pitch.
    {
        std::vector<tune::Frame> log;

        for (int i = 0; i < 800; ++i)
        {
            tune::Frame f;
            f.beat = (float) (i * 0.0125);
            f.rmsDb = -20.0f;
            log.push_back (f);
        }

        auto notes = tunetests::makeLog ({ { 10.0, 1.0, 60, 0.0 }, { 12.0, 1.0, 62, 0.0 }, { 14.0, 1.0, 64, 0.0 }, { 16.0, 1.0, 65, 0.0 }, { 18.0, 1.0, 67, 0.0 }, { 20.0, 1.0, 69, 0.0 }, { 22.0, 1.0, 71, 0.0 }, { 24.0, 1.0, 72, 0.0 } });
        log.erase (std::remove_if (log.begin(), log.end(), [] (const tune::Frame& f) { return f.beat >= 10.0f; }), log.end());
        log.insert (log.end(), notes.begin(), notes.end());
        check (joined (getAdvice (log, tempo, cMajor, sections, false)).contains ("Much of this track has no clear pitch"), "long stretches of loud sound with no pitch are pointed out");
    }

    check (formatBars ({ 16 }) == "bar 17" && formatBars ({ 16, 17 }) == "bars 17 and 18" && formatBars ({ 16, 17, 20 }) == "bars 17, 18 and 21", "bars are listed from 1");
}

static void runTuneGuideTests()
{
    using namespace daw;
    namespace tg = daw::gen::guides;
    std::cout << "Tune: the guide" << std::endl;

    bool texts = gen::tuneguide::numPages == 10;
    bool spelling = true;

    for (int i = 0; i < gen::tuneguide::numPages; ++i)
    {
        const auto text = gen::tuneguide::getPageText (i);
        texts = texts && gen::tuneguide::getPageName (i).isNotEmpty() && gen::tuneguide::getPageSummary (i).isNotEmpty() && text.length() >= 300;
        const auto all = (gen::tuneguide::getPageName (i) + text + gen::tuneguide::getPageSummary (i)).toLowerCase();
        spelling = spelling && ! all.contains ("color") && ! all.contains ("center") && ! all.contains ("analyze");
    }

    check (texts, "pitch correction's guide has 10 pages, each with a name, a summary and at least 300 characters");
    check (spelling, "the guide's texts use British spelling");
    check (gen::tuneguide::getPageName (3) == "Target notes" && gen::tuneguide::getPageText (1).contains ("432.4 Hz") && gen::tuneguide::getPageText (6).contains ("773 Hz")
               && gen::tuneguide::getPageText (4).contains ("12 cents flat") && gen::tuneguide::getPageText (7).contains ("1997"),
           "the texts carry the planned facts: 432.4 Hz, 773 Hz, 12 cents flat and 1997");

    const auto entry = tg::getEntry (tg::Guide::tune);
    check (entry.area == tg::Area::singing && entry.plugin == BuiltInTune::identifier && entry.name == "What is pitch correction?", "the guide is in the Singing area and opens DawIt Tune's pages");
    const auto startPath = tg::getStartPath();
    check (std::find (startPath.begin(), startPath.end(), tg::Guide::tune) == startPath.end(), "the guide is not on the Start here path");

    const auto entries = tg::getEntries();
    int tuneIndex = -1;

    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].guide == tg::Guide::tune)
            tuneIndex = (int) i;

    bool found = tuneIndex >= 0;

    for (const auto* query : { "autotune", "auto-tune", "pitch correction", "formant", "cents" })
    {
        const auto matches = tg::search (entries, query);
        found = found && std::any_of (matches.begin(), matches.end(), [&] (const auto& m) { return m.entry == tuneIndex; });
    }

    const auto autotune = tg::search (entries, "autotune");
    check (found && ! autotune.empty() && autotune.front().entry == tuneIndex, "the hub finds the guide for autotune, auto-tune, pitch correction, formant and cents, first for autotune");
    check (gen::singing::explain (gen::singing::Topic::tune, {}).contains ("DawIt Tune") && gen::singing::explain (gen::singing::Topic::tune, {}).contains ("What is pitch correction?"),
           "the singing guide's Singing in tune points to DawIt Tune and its guide");
}
