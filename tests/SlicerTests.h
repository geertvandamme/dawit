// DawIt Slicer: the engine tests (included by EngineTests.cpp, after check, render and runEffect).
#pragma once

namespace slicertests
{
using namespace daw;

/** A play head that says what a test wants: tempo, position and whether the song plays. */
struct Head final : public juce::AudioPlayHead
{
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;

        if (hasBpm)
            info.setBpm (bpm);

        info.setIsPlaying (playing);

        if (hasPpq)
            info.setPpqPosition (ppq);

        info.setTimeInSeconds (seconds);
        return info;
    }

    double bpm = 120.0, ppq = 0.0, seconds = 0.0;
    bool hasBpm = true, hasPpq = true, playing = true;
};

using Clock = std::function<void (Head&, int)>;

/** The song at a steady tempo from `startPpq`: the position at each block's first sample. */
inline Clock steady (double bpm, double startPpq = 0.0)
{
    return [bpm, startPpq] (Head& head, int sample)
    {
        head.bpm = bpm;
        head.ppq = startPpq + sample * bpm / 60.0 / testRate;
        head.seconds = sample / testRate;
    };
}

/** Plays `numSamples` through the slicer in blocks, the head set by `clock` before each block (null head: no play head). */
inline juce::AudioBuffer<float> drive (BuiltInSlicer& plugin, Head* head, int numSamples, const std::function<float (int)>& input,
                                       const Clock& clock = {}, bool prepare = true)
{
    if (prepare)
        plugin.prepareToPlay (testRate, testBlock);

    plugin.setPlayHead (head);
    juce::AudioBuffer<float> out (2, numSamples), block (2, testBlock);
    juce::MidiBuffer midi;

    for (int pos = 0; pos < numSamples; pos += testBlock)
    {
        const int n = juce::jmin (testBlock, numSamples - pos);
        block.setSize (2, n, false, false, true);

        for (int i = 0; i < n; ++i)
            block.setSample (0, i, input (pos + i)), block.setSample (1, i, input (pos + i));

        if (head != nullptr && clock)
            clock (*head, pos);

        plugin.processBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, n);
    }

    return out;
}

/** The index of the biggest value (by size) from `from` up to `to`. */
inline int loudestIn (const juce::AudioBuffer<float>& b, int from, int to)
{
    int best = from;

    for (int i = from; i < to; ++i)
        if (std::abs (b.getSample (0, i)) > std::abs (b.getSample (0, best)))
            best = i;

    return best;
}

inline float peakIn (const juce::AudioBuffer<float>& b, int from, int to)
{
    return b.getMagnitude (0, from, to - from);
}

inline std::function<float (int)> impulseAt (int sample) { return [sample] (int i) { return i == sample ? 1.0f : 0.0f; }; }
inline std::function<float (int)> sine (double hz, float level) { return [hz, level] (int i) { return level * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / testRate); }; }
}

static void runBuiltInSlicerTests()
{
    using namespace daw;
    using namespace slicertests;
    using slc::Mode;
    using slc::Knob;
    std::cout << "Built-in slicer" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // The pattern as copied text.
    {
        slc::Settings s;
        s.stepLength = 4;
        s.steps = 8;
        s.modes[1] = Mode::repeat;
        s.modes[2] = Mode::reverse;
        s.modes[7] = Mode::mute;
        const auto text = slc::patternToText (s);
        slc::Settings back;
        back.roll = 3;
        const bool ok = slc::patternFromText (text, back);
        slc::Settings untouched = back;
        const bool bad = slc::patternFromText ("DawIt Slicer pattern: 1/16 17 TTTT", untouched) || slc::patternFromText ("hello", untouched)
                         || slc::patternFromText ("DawIt Slicer pattern: 1/16 16 TTXT", untouched);
        check (ok && back.stepLength == 4 && back.steps == 8 && back.modes == s.modes && back.roll == 3 && ! bad && untouched.steps == 8
                   && text.startsWith ("DawIt Slicer pattern: 1/8 8 TRVTTTTM"),
               "slicer: a pattern copies to a line of text and pastes back (step length, Steps, modes); text that isn't a pattern is refused");
    }

    // Step values.
    check (near (slc::getStepMs (2, 120.0), 125.0, 0.01) && near (slc::getStepMs (3, 120.0), 166.67, 0.01) && near (slc::getStepMs (1, 120.0), 83.33, 0.01)
               && near (slc::getStepMs (0, 120.0), 62.5, 0.01) && near (slc::getStepMs (4, 120.0), 250.0, 0.01) && near (slc::getStepMs (5, 120.0), 500.0, 0.01),
           "step lengths at 120 BPM: 1/16 125 ms, 1/8T 166.7, 1/16T 83.3, 1/32 62.5, 1/8 250, 1/4 500");
    check (near (slc::getStepMs (5, 20.0), 3000.0, 0.01) && near (slc::getStepMs (2, 90.0), 166.67, 0.01) && near (slc::getStepMs (2, 140.0), 107.14, 0.01),
           "a 1/4 at 20 BPM lasts 3000 ms; a 1/16 lasts 166.7 ms at 90 BPM and 107.1 ms at 140");
    check (slc::getStepIndex (3.75, 2, 16) + 1 == 16 && slc::getStepIndex (4.0, 2, 16) + 1 == 1 && slc::getStepIndex (1.5, 2, 6) + 1 == 1
               && slc::getStepIndex (3.0, 2, 3) == 0 && slc::getStepIndex (0.25, 2, 3) == 1,
           "the step is counted from the start of the song: ppq 3.75 is step 16, 4.0 step 1, and 1.5 with 6 steps step 1");
    check (slc::getSliceCapacity (48000.0) >= 144000 && slc::getSliceCapacity (48000.0) <= 150000, "a slice memory holds one beat at 20 BPM: 144,000 to 150,000 samples at 48 kHz");

    // Texts.
    {
        slc::Settings s;
        check (slc::formatKnob (Knob::stepLength, s) == "1/16" && slc::formatKnob (Knob::steps, s) == "16" && slc::formatKnob (Knob::smooth, s) == "3.0 ms"
                   && slc::formatKnob (Knob::mix, s) == "100%" && slc::formatKnob (Knob::tempo, s) == "Song" && slc::formatKnob (Knob::ownBpm, s) == "120 BPM",
               "the slicer's controls read as 1/16, 16, 3.0 ms, 100%, Song and 120 BPM");
    }

    // The starting points.
    {
        const auto& presets = slc::getPresets();
        auto find = [&] (const juce::String& name) -> const slc::Preset*
        {
            for (auto& p : presets)
                if (p.name == name)
                    return &p;

            return nullptr;
        };

        bool valid = presets.size() == 6 && presets.front().name.startsWith ("Neutral");

        for (auto& p : presets)
            valid = valid && p.name.isNotEmpty() && p.why.isNotEmpty() && p.settings.steps >= 1 && p.settings.steps <= 16 && p.settings.roll >= 1 && p.settings.roll <= 4
                    && p.settings.smoothMs >= slc::minSmooth && p.settings.smoothMs <= slc::maxSmooth;

        check (valid, "the slicer has six starting points, the neutral one first, each in range and with a reason");

        const auto* stutter = find ("Stutter into the next bar");
        const auto* gate = find ("Trance gate");
        const auto* soft = find ("Soft pulse");
        const auto* swell = find ("Reverse swell");
        const auto* rolls = find ("Triplet rolls");
        bool shapes = stutter != nullptr && gate != nullptr && soft != nullptr && swell != nullptr && rolls != nullptr;

        if (shapes)
        {
            shapes = stutter->settings.modes[12] == Mode::through && stutter->settings.modes[13] == Mode::repeat && stutter->settings.modes[15] == Mode::repeat
                     && gate->settings.modes[0] == Mode::through && gate->settings.modes[1] == Mode::mute && gate->settings.modes[15] == Mode::mute
                     && soft->settings.modes == gate->settings.modes && near (soft->settings.smoothMs, 15.0, 1.0e-6) && near (soft->settings.mixPercent, 50.0, 1.0e-6)
                     && swell->settings.stepLength == 4 && swell->settings.steps == 8 && swell->settings.modes[7] == Mode::reverse && swell->settings.modes[6] == Mode::through
                     && rolls->settings.roll == 3 && rolls->settings.modes[1] == Mode::repeat && rolls->settings.modes[5] == Mode::repeat && rolls->settings.modes[2] == Mode::through;
        }

        check (shapes, "the starting points put the repeats, mutes and reverse on the steps the plan names");

        bool loads = true;

        for (auto& p : presets)
        {
            BuiltInSlicer plugin;
            plugin.setSettings (p.settings);
            const auto back = plugin.getSettings();
            loads = loads && back.modes == p.settings.modes && back.steps == p.settings.steps && back.stepLength == p.settings.stepLength && back.roll == p.settings.roll
                    && near (back.smoothMs, p.settings.smoothMs, 0.01) && near (back.mixPercent, p.settings.mixPercent, 0.01);
        }

        check (loads, "every starting point loads its sixteen modes and knobs into the plugin");
    }

    // Repeat: the slice of the step before plays again, on the grid.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        plugin.setSettings (s);
        Head head;
        const auto out = drive (plugin, &head, 24000, impulseAt (500), steady (120.0));
        const int at = loudestIn (out, 6000, 12000);
        check (std::abs (at - 6500) <= 1 && near (out.getSample (0, at), 1.0, 1.0e-5) && near (out.getSample (0, 500), 1.0, 1.0e-6) && peakIn (out, 700, 6300) < 1.0e-6f
                   && peakIn (out, 6700, 24000) < 1.0e-6f,
               "a Repeat step plays the step before again: the impulse at 500 comes again at 6500 (" + juce::String (at) + ")");

        // The first step after a start is a Repeat: nothing to repeat yet, so it plays through.
        auto first = plugin.getSettings();
        first.modes[0] = Mode::repeat;
        plugin.setSettings (first);
        const auto input = sine (440.0, 0.5f);
        const auto started = drive (plugin, &head, 6000, input, steady (120.0));
        bool same = true;

        for (int i = 0; i < 6000; ++i)
            same = same && std::abs (started.getSample (0, i) - input (i)) < 1.0e-6f;

        check (same, "a Repeat step right after a start has no slice yet and plays the track through");
    }

    // Roll.
    for (int roll : { 2, 3 })
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        s.roll = roll;
        plugin.setSettings (s);
        Head head;
        const auto out = drive (plugin, &head, 12000, impulseAt (500), steady (120.0));
        const std::vector<int> expected = roll == 2 ? std::vector<int> { 6500, 9500 } : std::vector<int> { 6500, 8500, 10500 };
        bool ok = true;

        for (int hit : expected)
            ok = ok && near (out.getSample (0, hit), 1.0, 1.0e-5) && std::abs (loudestIn (out, hit - 20, hit + 20) - hit) <= 1;

        const int count = (int) std::count_if (expected.begin(), expected.end(), [&] (int hit) { return std::abs (out.getSample (0, hit)) > 0.5f; });
        int loud = 0;

        for (int i = 6000; i < 12000; ++i)
            loud += std::abs (out.getSample (0, i)) > 0.01f ? 1 : 0;

        check (ok && count == roll && loud == roll, "Roll " + juce::String (roll) + " plays the first 1/" + juce::String (roll) + " of the slice " + juce::String (roll) + " times in the step");
    }

    // A run of Repeat steps plays the same slice, each at the level of the input.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = s.modes[2] = s.modes[3] = Mode::repeat;
        plugin.setSettings (s);
        Head head;
        const auto out = drive (plugin, &head, 30000, impulseAt (500), steady (120.0));
        bool ok = true;

        for (int hit : { 6500, 12500, 18500 })
            ok = ok && std::abs (loudestIn (out, hit - 50, hit + 50) - hit) <= 1 && std::abs (juce::Decibels::gainToDecibels (std::abs (out.getSample (0, hit))) - 0.0f) < 0.1f;

        check (ok && peakIn (out, 24500, 30000) < 1.0e-6f, "Through then three Repeat steps: the impulse comes again at 6500, 12500 and 18500, each at the input's level");
    }

    // Reverse.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::reverse;
        plugin.setSettings (s);
        Head head;
        const auto out = drive (plugin, &head, 12000, impulseAt (1000), steady (120.0));
        const int at = loudestIn (out, 6000, 12000);
        check (std::abs (at - 10999) <= 1 && near (out.getSample (0, at), 1.0, 1.0e-5), "a Reverse step plays the step before backwards: the impulse at 1000 comes at 10999 (" + juce::String (at) + ")");
    }

    // Mute.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::mute;
        plugin.setSettings (s);
        Head head;
        const auto input = sine (440.0, 0.5f);
        const auto out = drive (plugin, &head, 18000, input, steady (120.0));
        check (peakIn (out, 6200, 12000) == 0.0f && peakIn (out, 5000, 6000) > 0.4f && peakIn (out, 12200, 18000) > 0.4f,
               "a Mute step is exactly silent past its fade at Mix 100%, with the track through on both sides");

        s.mixPercent = 50.0f;
        plugin.setSettings (s);
        const auto half = drive (plugin, &head, 18000, input, steady (120.0));
        double inSq = 0.0, outSq = 0.0;

        for (int i = 6200; i < 11900; ++i)
        {
            inSq += (double) input (i) * input (i);
            outSq += (double) half.getSample (0, i) * half.getSample (0, i);
        }

        check (near (10.0 * std::log10 (outSq / inSq), -6.02, 0.05), "a Mute step at Mix 50% is 6.02 dB down on the track");
    }

    // Through steps are the track at any Mix; silence stays silent in every mode.
    {
        juce::Random random (5);
        std::vector<float> noise (12000);

        for (auto& v : noise)
            v = random.nextFloat() * 1.6f - 0.8f;

        bool same = true;

        for (float mix : { 0.0f, 37.0f, 100.0f })
        {
            BuiltInSlicer plugin;
            plugin.setKnob (Knob::mix, mix);
            Head head;
            const auto out = drive (plugin, &head, 12000, [&] (int i) { return noise[(size_t) i]; }, steady (120.0));

            for (int i = 0; i < 12000; ++i)
                same = same && std::abs (out.getSample (0, i) - noise[(size_t) i]) < 1.0e-6f && std::abs (out.getSample (1, i) - noise[(size_t) i]) < 1.0e-6f;
        }

        check (same, "Through steps come out as they went in at Mix 0, 37 and 100%");

        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        s.modes[2] = Mode::reverse;
        s.modes[3] = Mode::mute;
        s.modes[4] = Mode::through;
        s.modes[5] = Mode::repeat;
        plugin.setSettings (s);
        Head head;
        const auto out = drive (plugin, &head, 48000, [] (int) { return 0.0f; }, steady (120.0));
        check (peakIn (out, 0, 48000) == 0.0f && out.getMagnitude (1, 0, 48000) == 0.0f, "with silence in, every mode gives exactly silence out");
    }

    // Levels and clicks over a mixed pattern.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes = { Mode::through, Mode::repeat, Mode::reverse, Mode::mute, Mode::through, Mode::repeat, Mode::repeat, Mode::mute,
                    Mode::through, Mode::reverse, Mode::reverse, Mode::through, Mode::repeat, Mode::mute, Mode::through, Mode::repeat };
        s.roll = 2;
        plugin.setSettings (s);
        Head head;
        const auto input = sine (1000.0, 0.5f);
        const auto out = drive (plugin, &head, 96000, input, steady (120.0));
        float biggest = 0.0f, jump = 0.0f;

        for (int i = 0; i < 96000; ++i)
        {
            biggest = juce::jmax (biggest, std::abs (out.getSample (0, i)));

            if (i > 0)
                jump = juce::jmax (jump, std::abs (out.getSample (0, i) - out.getSample (0, i - 1)));
        }

        check (biggest <= 0.5f + 1.0e-6f && jump <= 0.1f, "a 1 kHz sine through a mixed pattern never gets louder and never jumps more than 0.1 between samples (" + juce::String (jump, 3) + ")");
    }

    // Half and three quarters of the steps muted.
    for (int muted : { 8, 12 })
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();

        for (int i = 0; i < 16; ++i)
            s.modes[(size_t) i] = muted == 8 ? (i % 2 == 1 ? Mode::mute : Mode::through) : (i % 4 == 0 ? Mode::through : Mode::mute);

        plugin.setSettings (s);
        Head head;
        const auto input = sine (1000.0, 0.5f);
        const auto out = drive (plugin, &head, 96000, input, steady (120.0));
        double inSq = 0.0, outSq = 0.0;

        for (int i = 0; i < 96000; ++i)
        {
            inSq += (double) input (i) * input (i);
            outSq += (double) out.getSample (0, i) * out.getSample (0, i);
        }

        const double db = 10.0 * std::log10 (outSq / inSq);
        check (near (db, muted == 8 ? -3.01 : -6.02, 0.1), juce::String (muted) + " of 16 steps muted make a steady sound " + juce::String (db, 2) + " dB quieter on average");
    }

    // Hear only what it changes.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        plugin.setSettings (s);
        plugin.listenToEffect = true;
        Head head;
        const auto out = drive (plugin, &head, 12000, impulseAt (500), steady (120.0));
        check (peakIn (out, 0, 6000) == 0.0f && near (out.getSample (0, 6500), 1.0, 1.0e-5), "Hear only what it changes: a Through step is silent and the repeat is heard");
    }

    // The tempo: its own clock, the song's tempo missing, the song stopped, and a change of tempo.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        s.ownTempo = true;
        s.ownBpm = 100.0f;
        plugin.setSettings (s);
        const auto out = drive (plugin, nullptr, 12000, impulseAt (500));
        check (std::abs (loudestIn (out, 7200, 12000) - 7700) <= 1, "with no play head, Own at 100 BPM makes a 1/16 last 150 ms: the repeat is at 7700");

        s.ownTempo = false;
        plugin.setSettings (s);
        Head head;
        head.hasBpm = false;
        const auto noBpm = drive (plugin, &head, 12000, impulseAt (500), steady (100.0));
        check (std::abs (loudestIn (noBpm, 7200, 12000) - 7700) <= 1 && near (plugin.getUsedBpm(), 100.0, 1.0e-9), "with the song source and a play head that gives no tempo, it uses Own BPM");
    }

    {
        // 120 BPM until beat 4, then 60: the next step boundary is a quarter beat (250 ms) after the change.
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::mute;
        plugin.setSettings (s);
        Head head;
        constexpr int change = 96000;
        const auto input = sine (440.0, 0.5f);
        auto clock = [] (Head& h, int sample)
        {
            h.bpm = sample < change ? 120.0 : 60.0;
            h.ppq = sample < change ? sample * 120.0 / 60.0 / testRate : 4.0 + (sample - change) * 60.0 / 60.0 / testRate;
        };

        // Step 17 (beat 4.0) is the first step again, step 18 (beat 4.25) is muted.
        const auto out = drive (plugin, &head, change + 24000, input, clock);
        check (std::abs (out.getSample (0, change + 11999)) > 0.01f && peakIn (out, change + 12150, change + 23900) == 0.0f && peakIn (out, change + 100, change + 11990) > 0.4f,
               "after a tempo change to 60 BPM the next step starts 250 ms later: the muted 1/16 begins at beat 4.25");
    }

    {
        // Stopped: its own clock, started at step 1 by Play a bar.
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();

        for (int i = 1; i < 16; ++i)
            s.modes[(size_t) i] = Mode::mute;

        plugin.setSettings (s);
        Head head;
        head.playing = false;
        head.ppq = 7.3;
        drive (plugin, &head, testBlock * 10, [] (int) { return 0.0f; }, [] (Head& h, int) { h.ppq = 7.3; });
        plugin.clap.request();
        const auto out = drive (plugin, &head, 96000, [] (int) { return 0.0f; }, [] (Head& h, int) { h.ppq = 7.3; }, false);
        check (peakIn (out, 0, 5800) > 0.3f && peakIn (out, 6200, 95000) == 0.0f && near (plugin.getUsedBpm(), 120.0, 1.0e-9) && ! plugin.usingSongClock.load(),
               "while the song is stopped it slices on its own clock at the song's tempo, and Play a bar starts on step 1");

        // Play a bar lasts one bar.
        BuiltInSlicer bar;
        bar.clap.request();
        Head h2;
        h2.playing = false;
        const auto bout = drive (bar, &h2, 110000, [] (int) { return 0.0f; });
        int last = 0;

        for (int i = 0; i < 110000; ++i)
            if (bout.getSample (0, i) != 0.0f)
                last = i;

        check (std::abs (last - 96000) <= testBlock && peakIn (bout, 0, 96000) > 0.4f && peakIn (bout, 0, 96000) <= 1.0f, "Play a bar at 120 BPM lasts 2.0 s and peaks below 0 dB (" + juce::String (last) + ", " + juce::String (peakIn (bout, 0, 96000), 2) + ")");
    }

    // A jump clears the slices.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[2] = Mode::repeat;
        plugin.setSettings (s);
        Head head;
        const auto input = sine (440.0, 0.5f);
        // Two steps, then the song jumps to the start of a step that repeats: nothing was heard since the jump.
        auto clock = [] (Head& h, int sample)
        {
            h.bpm = 120.0;
            h.ppq = sample < 12000 ? sample / testRate * 2.0 : 8.5 + (sample - 12000) / testRate * 2.0;
        };

        const auto out = drive (plugin, &head, 18000, input, clock);
        bool same = true;

        for (int i = 12000; i < 18000; ++i)
            same = same && std::abs (out.getSample (0, i) - input (i)) < 1.0e-6f;

        check (same, "after a jump in the song a Repeat step plays through: the slice from before the jump is gone");
    }

    // No latency, and no allocation while playing at the slowest tempo.
    {
        BuiltInSlicer plugin;
        plugin.setKnob (Knob::tempo, 1.0f);
        plugin.setKnob (Knob::ownBpm, 20.0f);
        plugin.setKnob (Knob::stepLength, 5.0f);
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        plugin.setSettings (s);
        plugin.prepareToPlay (testRate, testBlock);
        const int before = plugin.getSliceBufferSize();
        drive (plugin, nullptr, 3 * 48000, sine (220.0, 0.3f), {}, false);
        check (plugin.getLatencySamples() == 0 && before >= 144000 && before <= 150000 && plugin.getSliceBufferSize() == before,
               "the slicer has no latency, and its slice memory keeps its size while a 1/4 step at 20 BPM plays");
    }

    // Saving and loading.
    {
        BuiltInSlicer a, b;
        auto s = a.getSettings();
        s.modes[3] = Mode::reverse;
        s.modes[9] = Mode::mute;
        s.steps = 11;
        s.roll = 4;
        s.stepLength = 3;
        s.smoothMs = 12.0f;
        s.mixPercent = 63.0f;
        s.ownTempo = true;
        s.ownBpm = 97.5f;
        a.setSettings (s);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto t = b.getSettings();
        check (t.modes == s.modes && t.steps == 11 && t.roll == 4 && t.stepLength == 3 && near (t.smoothMs, 12.0, 0.05) && near (t.mixPercent, 63.0, 0.01) && t.ownTempo && near (t.ownBpm, 97.5, 0.05),
               "the slicer's pattern and settings survive saving and loading its state");
    }

    checkThroughProject<BuiltInSlicer> (BuiltInSlicer::getDescription(), "DawIt Slicer",
                                        [] (BuiltInSlicer& p)
                                        {
                                            p.setMode (13, Mode::repeat);
                                            p.setMode (14, Mode::mute);
                                            p.setKnob (Knob::roll, 3.0f);
                                        },
                                        [] (BuiltInSlicer& p) { return p.getMode (13) == Mode::repeat && p.getMode (14) == Mode::mute && p.getSettings().roll == 3 && p.getMode (0) == Mode::through; });

    // The step history.
    {
        BuiltInSlicer plugin;
        auto s = plugin.getSettings();
        s.modes[1] = Mode::repeat;
        s.modes[2] = Mode::mute;
        plugin.setSettings (s);
        Head head;
        drive (plugin, &head, 24000, sine (440.0, 0.5f), steady (120.0));
        std::vector<slc::StepRecord> records (64);
        const int n = plugin.history.getRecent (records.data(), 64);
        check (n == 3 && records[0].step == 0 && records[1].step == 1 && records[1].mode == Mode::repeat && records[1].sliced && records[2].mode == Mode::mute
                   && records[2].outRms < records[0].outRms * 0.2f && records[0].inPeak > 0.4f && std::abs (records[1].slicePeak - records[0].inPeak) < 1.0e-6f && near (records[1].beat, 0.25, 1.0e-9),
               "the history records each finished step: its mode, whether it had a slice, and the levels in and out (" + juce::String (n) + ")");
    }
}

/** The advice of *Your track*: rules S0 to S6 on made-up step records. */
static void runSlicerAnalysisTests()
{
    using namespace daw;
    using slc::Mode;
    std::cout << "Slicer: advice" << std::endl;

    auto record = [] (int step, Mode mode, float inDb, float outDb, bool sliced = false, float sliceDb = -100.0f)
    {
        slc::StepRecord r;
        r.step = step;
        r.mode = mode;
        r.sliced = sliced;
        r.inPeak = juce::Decibels::decibelsToGain (inDb);
        r.inRms = r.inPeak * 0.7f;
        r.outPeak = juce::Decibels::decibelsToGain (outDb);
        r.outRms = r.outPeak * 0.7f;
        r.slicePeak = juce::Decibels::decibelsToGain (sliceDb);
        return r;
    };

    auto loud = [&] (int count) { std::vector<slc::StepRecord> v; for (int i = 0; i < count; ++i) v.push_back (record (i, Mode::through, -6.0f, -6.0f)); return v; };
    auto contains = [] (const std::vector<juce::String>& lines, const juce::String& text) { return std::any_of (lines.begin(), lines.end(), [&] (auto& l) { return l.contains (text); }); };

    // S0: nothing heard.
    {
        slc::AnalysisInput in;
        in.settings.modes[1] = Mode::repeat;
        in.records.assign (8, record (0, Mode::through, -70.0f, -70.0f));
        auto lines = slc::analyse (in);
        check (lines.size() == 1 && lines[0] == "Play the track, or click Play a bar, to see it here.", "advice S0: below -60 dBFS everywhere, it asks to play the track and says nothing else");
        in.records.clear();
        check (slc::analyse (in).size() == 1, "advice S0: no records at all says the same");
        in.records.assign (8, record (0, Mode::through, -59.0f, -59.0f));
        check (slc::analyse (in).front() != "Play the track, or click Play a bar, to see it here.", "advice S0: at -59 dBFS the track counts as heard");
    }

    // S1: no song tempo.
    {
        slc::AnalysisInput in;
        in.settings.modes[1] = Mode::repeat;
        in.records = loud (16);
        in.hostHasTempo = false;
        in.settings.ownBpm = 100.0f;
        check (contains (slc::analyse (in), "slices at its own 100 BPM"), "advice S1: no tempo from the song says it slices at its own tempo");
        in.hostHasTempo = true;
        check (! contains (slc::analyse (in), "own 100 BPM"), "advice S1: not when the song gives a tempo");
        in.hostHasTempo = false;
        in.settings.ownTempo = true;
        check (! contains (slc::analyse (in), "own 100 BPM"), "advice S1: not when Own is chosen");
    }

    // S2: all Through.
    {
        slc::AnalysisInput in;
        in.records = loud (16);
        const auto lines = slc::analyse (in);
        check (contains (lines, "Every step plays through") && contains (lines, "every step plays through."), "advice S2: all Through says so, and the summary agrees");
        in.settings.modes[3] = Mode::mute;
        check (! contains (slc::analyse (in), "Every step plays through"), "advice S2: one changed step is enough to drop it");
    }

    // S3: a repeat of a nearly silent slice.
    for (const auto [gap, fires] : { std::pair<float, bool> { 35.0f, true }, { 25.0f, false } })
    {
        slc::AnalysisInput in;
        in.settings.modes[5] = Mode::repeat;
        in.records = loud (16);
        in.records[5] = record (5, Mode::repeat, -6.0f, -6.0f - gap, true, -6.0f - gap);
        const auto lines = slc::analyse (in);
        check (contains (lines, "Step 6 repeats a slice that was 35 dB quieter than the loudest step") == fires,
               "advice S3: a repeat of a slice " + juce::String (gap, 0) + " dB under the loudest step " + (fires ? "is" : "is not") + " pointed out");
    }

    {
        slc::AnalysisInput in;
        in.settings.modes[2] = Mode::reverse;
        in.settings.modes[3] = Mode::repeat;
        in.settings.modes[4] = Mode::repeat;
        in.settings.modes[5] = Mode::repeat;
        in.settings.modes[6] = Mode::repeat;
        in.records = loud (16);

        for (int i = 2; i < 7; ++i)
            in.records[(size_t) i] = record (i, i == 2 ? Mode::reverse : Mode::repeat, -6.0f, -50.0f, true, -50.0f);

        in.records.push_back (record (2, Mode::reverse, -6.0f, -50.0f, true, -50.0f));
        const auto lines = slc::analyse (in);
        const int shown = (int) std::count_if (lines.begin(), lines.end(), [] (auto& l) { return l.contains ("a slice that was"); });
        check (shown == 3 && contains (lines, "Step 3 plays backwards a slice that was 44 dB quieter"), "advice S3: one line per step, at most three, and Reverse says it plays backwards");

        in.records[3].sliced = false;
        const auto again = slc::analyse (in);
        check (std::none_of (again.begin(), again.end(), [] (auto& l) { return l.contains ("Step 4 "); }), "advice S3: a step that had no slice (it played through) is not pointed out");
    }

    // S4: the level drops.
    for (const auto [drop, fires] : { std::pair<float, bool> { 7.0f, true }, { 5.0f, false } })
    {
        slc::AnalysisInput in;

        for (int i = 0; i < 10; ++i)
            in.settings.modes[(size_t) i] = Mode::mute;

        in.records = loud (16);

        for (auto& r : in.records)
            r.outRms = r.inRms * juce::Decibels::decibelsToGain (-drop);

        const auto lines = slc::analyse (in);
        check (contains (lines, "about 7 dB quieter on average, mostly from its 10 Mute steps") == fires && ! (! fires && contains (lines, "quieter on average")),
               "advice S4: an output " + juce::String (drop, 0) + " dB under the input " + (fires ? "is" : "is not") + " pointed out");
    }

    // S5: busy, and S6: the summary.
    {
        slc::AnalysisInput in;

        for (int i = 0; i < 11; ++i)
            in.settings.modes[(size_t) i] = i % 3 == 0 ? Mode::repeat : i % 3 == 1 ? Mode::reverse : Mode::mute;

        in.records = loud (16);
        check (! contains (slc::analyse (in), "steps change the sound"), "advice S5: 11 changed steps are not busy");
        in.settings.modes[11] = Mode::repeat;
        in.settings.modes[12] = Mode::mute;
        const auto lines = slc::analyse (in);
        check (contains (lines, "13 of 16 steps change the sound"), "advice S5: 13 of 16 changed steps are busy");

        slc::AnalysisInput sum;
        sum.settings.modes[1] = sum.settings.modes[2] = sum.settings.modes[3] = Mode::repeat;
        sum.settings.modes[5] = Mode::reverse;

        for (int i : { 8, 9, 10, 11 })
            sum.settings.modes[(size_t) i] = Mode::mute;

        sum.records = loud (16);
        const auto summary = slc::analyse (sum).back();
        check (summary == "The last 2 bars: 16 steps of 1/16 (125 ms at 120 BPM): 3 repeats, 1 reverse, 4 mutes.", "advice S6: the summary counts the steps by mode (" + summary + ")");
    }
}

/** DawIt Slicer on an audio track in the engine: from bar 2, over a tempo change and over a loop wrap, the repeats land on the grid. */
static void runSlicerEngineTests()
{
    using namespace daw;
    using slc::Mode;
    std::cout << "Slicer: the engine" << std::endl;

    struct Song
    {
        Song (int impulseSample)
        {
            engine.deviceManager.removeAudioCallback (&engine);
            engine.audioDeviceAboutToStart (&device);
            track = engine.addTrack (Track::Type::audio);
            juce::AudioBuffer<float> clip (1, 48000 * 16);
            clip.clear();
            clip.setSample (0, impulseSample, 0.5f);
            engine.addAudioClip (*track, clip, 48000.0, 0.0, "Impulse");
            juce::String error;
            juce::OwnedArray<juce::PluginDescription> found;
            BuiltInFormat format;
            format.findAllTypesForFile (found, BuiltInSlicer::identifier);
            auto* slot = found.isEmpty() ? nullptr : engine.addPlugin (*track, *found.getFirst(), error);
            slicer = slot != nullptr ? dynamic_cast<BuiltInSlicer*> (slot->instance.get()) : nullptr;
        }

        /** The samples where the left channel is above a hundredth of its biggest value, one per group of nearby samples. */
        static std::vector<int> peaks (const juce::AudioBuffer<float>& out)
        {
            // (the engine fades the master in when it starts, so the first sound is quieter than the next)
            const float top = out.getMagnitude (0, 0, out.getNumSamples());
            std::vector<int> found;

            for (int i = 0; i < out.getNumSamples(); ++i)
                if (std::abs (out.getSample (0, i)) > top / 100.0f && (found.empty() || i - found.back() > 100))
                    found.push_back (i);

            return found;
        }

        Engine engine;
        FakeDevice device;
        Track* track = nullptr;
        BuiltInSlicer* slicer = nullptr;
    };

    auto isNear = [] (const std::vector<int>& found, const std::vector<int>& expected)
    {
        if (found.size() != expected.size())
            return false;

        for (size_t i = 0; i < found.size(); ++i)
            if (std::abs (found[i] - expected[i]) > 1)
                return false;

        return true;
    };

    auto describe = [] (const std::vector<int>& found)
    {
        juce::String text;

        for (int i : found)
            text << i << " ";

        return text;
    };

    // Playing from bar 2: step 1 is on beat 5 of the song (bar 2), so the repeat of the impulse lands 6000 samples later.
    {
        Song song (96000 + 500);
        check (song.slicer != nullptr, "the engine adds DawIt Slicer to an audio track");

        if (song.slicer != nullptr)
        {
            song.slicer->setMode (1, Mode::repeat);
            song.engine.setPositionBeats (4.0);
            song.engine.play();
            const auto out = render (song.engine, 0.5);
            const auto found = Song::peaks (out);
            check (isNear (found, { 500, 6500 }), "playing from bar 2, the impulse and its repeat are at 500 and 6500 (" + describe (found) + ")");
            song.engine.stop();

        }
    }

    // A change of tempo: 120 BPM until beat 8, then 60 (the clip's impulse is 4 s and 500 samples in). The step after beat 8.25 is 250 ms after the change.
    {
        Song song (192000 + 500);

        if (song.slicer != nullptr)
        {
            song.slicer->setMode (1, Mode::repeat);
            song.engine.setTempoMap (TempoMap (std::vector<TempoChange> { { 0.0, 120.0 }, { 8.0, 60.0 } }));
            song.engine.setPositionBeats (7.0);
            song.engine.play();
            const auto changed = render (song.engine, 1.5);
            const auto at = Song::peaks (changed);
            song.engine.stop();
            check (at.size() == 2 && std::abs (at[0] - 24500) <= 2 && std::abs ((at[1] - at[0]) - 12000) <= 1,
                   "over a tempo change the repeat lands on the grid: 12000 samples (250 ms at 60 BPM) after the impulse (" + describe (at) + ")");
        }
    }

    // A loop wrap: the first step after the wrap plays the track through, because nothing was heard since the jump.
    {
        Song song (96000 + 500);

        if (song.slicer != nullptr)
        {
            song.slicer->setMode (0, Mode::repeat);
            song.slicer->setMode (1, Mode::repeat);
            song.engine.loopStartBeat = 4.0;
            song.engine.loopEndBeat = 8.0;
            song.engine.loopEnabled = true;
            song.engine.setPositionBeats (4.0);
            song.engine.play();
            const auto out = render (song.engine, 2.5);
            const auto found = Song::peaks (out);
            song.engine.stop();
            song.engine.loopEnabled = false;
            check (isNear (found, { 500, 6500, 96500, 102500 }),
                   "over a loop wrap the first step plays through and the next repeats it on the grid: 500, 6500, then 96500, 102500 (" + describe (found) + ")");
        }
    }
}
