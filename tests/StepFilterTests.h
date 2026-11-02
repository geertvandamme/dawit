// The shared filter and step clock, and DawIt Step Filter: the engine tests (included by EngineTests.cpp, after SlicerTests.h).
#pragma once

namespace stftests
{
using namespace daw;
using slicertests::Head;
using slicertests::Clock;
using slicertests::steady;

/** Plays `numSamples` through the step filter in blocks of `block`, the head set by `clock` before each block (null head: no play head). */
inline juce::AudioBuffer<float> drive (BuiltInStepFilter& plugin, Head* head, double rate, int numSamples, const std::function<float (int)>& input,
                                       const Clock& clock = {}, int block = 256, bool prepare = true, const std::function<void (int)>& afterBlock = {})
{
    if (prepare)
        plugin.prepareToPlay (rate, block);

    plugin.setPlayHead (head);
    juce::AudioBuffer<float> out (2, numSamples), buffer (2, block);
    juce::MidiBuffer midi;

    for (int pos = 0; pos < numSamples; pos += block)
    {
        const int n = juce::jmin (block, numSamples - pos);
        buffer.setSize (2, n, false, false, true);

        for (int i = 0; i < n; ++i)
            buffer.setSample (0, i, input (pos + i)), buffer.setSample (1, i, input (pos + i));

        if (head != nullptr && clock)
            clock (*head, pos);

        plugin.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, buffer, ch, 0, n);

        if (afterBlock)
            afterBlock (pos + n);
    }

    return out;
}

inline std::function<float (int)> sine (double hz, double rate, float level)
{
    return [hz, rate, level] (int i) { return level * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / rate); };
}

/** The level of the left channel over `from` to `to`, as the amplitude of a sine with that RMS. */
inline double amplitude (const juce::AudioBuffer<float>& b, int from, int to)
{
    double sum = 0.0;

    for (int i = from; i < to; ++i)
        sum += (double) b.getSample (0, i) * b.getSample (0, i);

    return std::sqrt (2.0 * sum / (to - from));
}

inline double db (double ratio) { return 20.0 * std::log10 (juce::jmax (1.0e-12, ratio)); }

/** A step filter whose steps all have the same cutoff, as Low = High. */
inline stf::Settings steadyCutoff (double hz, filt::FilterType type, bool steep, float resonance = 0.0f)
{
    stf::Settings s;
    s.lowHz = s.highHz = (float) hz;
    s.type = type;
    s.steep = steep;
    s.resonancePercent = resonance;
    return s;
}

/** The level a steady sine comes out of a step filter at, in dB against the sine's own. */
inline double measuredDb (const stf::Settings& s, double hz, double rate)
{
    BuiltInStepFilter plugin;
    plugin.setSettings (s);
    const int n = (int) (rate * 1.0);
    const auto out = drive (plugin, nullptr, rate, n, sine (hz, rate, 0.25f));
    return db (amplitude (out, n * 3 / 4, n) / 0.25);
}
}

static void runSharedFilterAndGridTests()
{
    using namespace daw;
    using namespace stftests;
    std::cout << "Shared filter and step grid" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };
    using filt::FilterType;

    // The resonance scale and the response maths (the plan's table).
    check (near (filt::getQ (0.0f), 0.71, 0.01) && near (filt::getQ (50.0f), 3.76, 0.01) && near (filt::getQ (100.0f), 20.0, 0.01), "shared filter: Q is 0.71 at 0%, 3.76 at 50% and 20 at 100%");
    check (near (filt::getFilterDb (FilterType::lowPass, false, 1000.0, 0.0f, 1000.0), -3.0, 0.1) && near (filt::getFilterDb (FilterType::lowPass, false, 1000.0, 50.0f, 1000.0), 11.5, 0.1)
               && near (filt::getFilterDb (FilterType::lowPass, false, 1000.0, 100.0f, 1000.0), 26.0, 0.1),
           "shared filter: a 12 dB low-pass is -3.0, +11.5 and +26.0 dB at its cutoff at Resonance 0, 50 and 100%");
    check (near (filt::getFilterDb (FilterType::lowPass, true, 1000.0, 0.0f, 1000.0), -6.0, 0.1) && near (filt::getFilterDb (FilterType::lowPass, true, 1000.0, 100.0f, 1000.0), 23.0, 0.1),
           "shared filter: a 24 dB low-pass is -6.0 dB at its cutoff with no resonance and +23.0 dB with all of it");
    check (near (filt::getFilterDb (FilterType::lowPass, false, 1000.0, 0.0f, 2000.0), -12.3, 0.1) && near (filt::getFilterDb (FilterType::lowPass, true, 1000.0, 0.0f, 2000.0), -24.6, 0.1),
           "shared filter: one octave above the cutoff a 12 dB low-pass is -12.3 dB and a 24 dB one -24.6 dB");
    check (near (filt::getFilterDb (FilterType::bandPass, false, 1000.0, 50.0f, 1000.0), 0.0, 0.1), "shared filter: a band-pass peaks at 0 dB at its cutoff");

    {
        // The band-pass is cutoff / Q wide at -3 dB: 266 Hz at 1 kHz and 50%.
        double low = 0.0, high = 0.0;

        for (double hz = 500.0; hz < 2000.0; hz += 0.25)
        {
            const double level = filt::getFilterDb (FilterType::bandPass, false, 1000.0, 50.0f, hz);

            if (low == 0.0 && level >= -3.0103)
                low = hz;

            if (low != 0.0 && high == 0.0 && level < -3.0103)
                high = hz;
        }

        check (near (high - low, 266.0, 266.0 * 0.03), "shared filter: the band-pass at 1 kHz and 50% is 266 Hz wide at -3 dB (" + juce::String (high - low, 1) + ")");
    }

    check (near (filt::getBandWidthHz (false, 1000.0, 50.0f), 266.0, 266.0 * 0.03) && near (filt::getBandWidthHz (true, 1000.0, 50.0f), 257.0, 257.0 * 0.03),
           "shared filter: getBandWidthHz gives 266 Hz at 12 dB and 257 Hz at 24 dB for 1 kHz at 50% (" + juce::String (filt::getBandWidthHz (true, 1000.0, 50.0f), 1) + ")");

    check (near (db (std::abs (filt::getFilterResponse (FilterType::highPass, false, 1000.0, 0.0f, 250.0))), -24.1, 0.1),
           "shared filter: a 12 dB high-pass at 1 kHz is -24.1 dB at 250 Hz");

    // Subtractive forwards to it.
    check (near (sub::getQ (37.0f), filt::getQ (37.0f), 1.0e-9)
               && near (sub::getFilterDb (sub::FilterType::highPass, true, 800.0, 40.0f, 1500.0), filt::getFilterDb (FilterType::highPass, true, 800.0, 40.0f, 1500.0), 1.0e-9),
           "DawIt Subtractive's Q and response are the shared filter's");

    // The step grid: the same counting in both effects.
    check (stepgrid::getStepIndex (9.0, 2, 16) == 4 && slc::getStepIndex (9.0, 2, 16) == 4 && stf::getStepIndex (9.0, 2, 16) == 4,
           "step grid: at ppq 9.0 a 1/16 pattern of 16 steps is at step 5 (index 4), in the slicer and the step filter alike");
    check (stepgrid::getStepIndex (-0.25, 2, 16) == 15 && near (stepgrid::getStepMs (2, 120.0), 125.0, 1.0e-9) && near (stepgrid::getStepMs (4, 120.0), 250.0, 1.0e-9),
           "step grid: counting backwards wraps (ppq -0.25 is step 16); at 120 BPM a 1/16 lasts 125 ms and a 1/8 250 ms");
    check (std::abs (std::pow (1.0, 0.0) * (1.0 + 0.25 + 1.0 / 9.0 + 1.0 / 16.0) / (juce::MathConstants<double>::pi * juce::MathConstants<double>::pi / 6.0) - 0.865) < 0.001,
           "a saw's first 4 harmonics hold 86.5% of its power");
}

static void runBuiltInStepFilterTests()
{
    using namespace daw;
    using namespace stftests;
    using stf::Knob;
    using filt::FilterType;
    std::cout << "Built-in step filter" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // Cutoff mapping.
    {
        bool ok = true;
        const double expected[] = { 250.0, 500.0, 1000.0, 2000.0, 4000.0 };

        for (int i = 0; i < 5; ++i)
            ok = ok && near (stf::getHeightCutoffHz (250.0, 4000.0, i * 25.0), expected[i], expected[i] * 0.005)
                 && near (stf::getHeightCutoffHz (4000.0, 250.0, i * 25.0), expected[i], expected[i] * 0.005);

        check (ok, "step filter: heights 0 to 100% with Low 250 Hz and High 4 kHz give 250, 500, 1000, 2000 and 4000 Hz, and the same with the knobs swapped");
        check (stf::clampCutoff (1.0e6, 44100.0) <= 19845.0001 && near (stf::clampCutoff (1.0, 48000.0), 20.0, 1.0e-9) && near (stf::clampCutoff (1000.0, 48000.0), 1000.0, 1.0e-9),
               "step filter: the cutoff is kept between 20 Hz and 45% of the sample rate (19,845 Hz at 44.1 kHz)");
    }

    // Texts, the pattern as text.
    {
        stf::Settings s;
        check (stf::formatKnob (Knob::stepLength, s) == "1/16" && stf::formatKnob (Knob::steps, s) == "16" && stf::formatKnob (Knob::type, s) == "Low-pass" && stf::formatKnob (Knob::slope, s) == "24 dB"
                   && stf::formatKnob (Knob::lowCutoff, s) == "300 Hz" && stf::formatKnob (Knob::highCutoff, s) == "20.0 kHz" && stf::formatKnob (Knob::resonance, s) == "0%"
                   && stf::formatKnob (Knob::glide, s) == "10%" && stf::formatKnob (Knob::mix, s) == "100%" && stf::formatKnob (Knob::output, s) == "0.0 dB"
                   && stf::formatKnob (Knob::tempo, s) == "Song" && stf::formatKnob (Knob::ownBpm, s) == "120 BPM",
               "step filter: the controls read as 1/16, 16, Low-pass, 24 dB, 300 Hz, 20.0 kHz, 0%, 10%, 100%, 0.0 dB, Song and 120 BPM");
        check (near (stf::getGlideMs (s, 120.0), 12.5, 1.0e-9), "step filter: Glide 10% of a 125 ms step is 12.5 ms");
        s.glidePercent = 0.0f;
        check (near (stf::getGlideMs (s, 120.0), 2.0, 1.0e-9), "step filter: Glide 0% still takes 2 ms");

        stf::Settings p;
        p.stepLength = 4;
        p.steps = 8;
        p.heights[1] = 50.0f;
        p.heights[2] = 6.7f;
        const auto text = stf::patternToText (p);
        stf::Settings back;
        back.glidePercent = 33.0f;
        const bool ok = stf::patternFromText (text, back);
        stf::Settings untouched = back;
        const bool bad = stf::patternFromText ("DawIt Step Filter pattern: 1/16 17 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100", untouched)
                         || stf::patternFromText ("hello", untouched)
                         || stf::patternFromText ("DawIt Step Filter pattern: 1/16 16 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100 101", untouched)
                         || stf::patternFromText ("DawIt Step Filter pattern: 1/16 16 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100", untouched)
                         || stf::patternFromText ("DawIt Step Filter pattern: 1/16 16 x 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100", untouched);
        check (ok && back.stepLength == 4 && back.steps == 8 && back.heights == p.heights && near (back.glidePercent, 33.0, 1.0e-6) && ! bad && untouched.steps == 8
                   && text.startsWith ("DawIt Step Filter pattern: 1/8 8 100 50 6.7 100"),
               "step filter: a pattern copies to a line of text and pastes back (step length, Steps, heights); text that isn't a pattern is refused (" + text + ")");
    }

    // The starting points.
    {
        const auto& presets = stf::getPresets();
        bool inRange = presets.size() == 6, whys = true;

        for (auto& p : presets)
        {
            const auto& s = p.settings;
            whys = whys && p.name.isNotEmpty() && p.why.length() > 40;
            inRange = inRange && s.stepLength >= 0 && s.stepLength < stf::numStepLengths && s.steps >= 1 && s.steps <= 16 && s.lowHz >= stf::minCutoff && s.highHz <= stf::maxCutoff
                      && s.lowHz <= s.highHz && s.resonancePercent >= 0.0f && s.resonancePercent <= 100.0f && s.glidePercent >= 0.0f && s.glidePercent <= 100.0f
                      && s.outputDb >= stf::minOutputDb && s.outputDb <= stf::maxOutputDb && s.mixPercent == 100.0f
                      && std::all_of (s.heights.begin(), s.heights.end(), [] (float h) { return h >= 0.0f && h <= 100.0f; });
        }

        check (inRange && whys, "step filter: six starting points, each with a why and every value in range");

        auto find = [&] (const juce::String& name) -> const stf::Preset*
        {
            for (auto& p : presets)
                if (p.name == name)
                    return &p;

            return nullptr;
        };

        const auto* pulse = find ("Eighth-note pulse");
        const auto* acid = find ("Acid steps");
        const auto* talk = find ("Talking band-pass");
        const auto* rise = find ("Build-up rise");
        const auto* neutral = presets.empty() ? nullptr : &presets[0];
        bool alternates = pulse != nullptr;

        for (int i = 0; alternates && i < 8; ++i)
            alternates = juce::exactlyEqual (pulse->settings.heights[(size_t) i], i % 2 == 0 ? 0.0f : 100.0f);

        check (alternates && pulse->settings.stepLength == 4 && pulse->settings.steps == 8 && near (pulse->settings.lowHz, 400.0, 1.0e-3) && near (pulse->settings.highHz, 4000.0, 1.0e-3),
               "step filter: Eighth-note pulse alternates 0 and 100% on eighth notes between 400 Hz and 4 kHz");
        check (acid != nullptr && near (acid->settings.outputDb, -9.0, 1.0e-6) && near (acid->settings.resonancePercent, 65.0, 1.0e-6) && acid->settings.heights[1] == 50.0f && acid->settings.heights[15] == 25.0f
                   && talk != nullptr && talk->settings.type == FilterType::bandPass && ! talk->settings.steep
                   && rise != nullptr && rise->settings.type == FilterType::highPass && near (rise->settings.heights[1], 6.667, 0.01) && rise->settings.heights[15] == 100.0f
                   && neutral != nullptr && neutral->name == "Neutral: every step open" && std::all_of (neutral->settings.heights.begin(), neutral->settings.heights.end(), [] (float h) { return h == 100.0f; }),
               "step filter: the starting points carry the plan's settings (Acid steps -9 dB, Talking band-pass 12 dB, Build-up rise rising evenly)");
    }

    // Settings and state.
    {
        BuiltInStepFilter a, b;
        stf::Settings s;
        s.stepLength = 4;
        s.steps = 11;
        s.ownTempo = true;
        s.ownBpm = 97.5f;
        s.type = FilterType::bandPass;
        s.steep = false;
        s.lowHz = 440.0f;
        s.highHz = 3300.0f;
        s.resonancePercent = 61.0f;
        s.glidePercent = 33.0f;
        s.mixPercent = 63.0f;
        s.outputDb = -7.5f;
        s.heights[3] = 42.5f;
        s.heights[15] = 0.0f;
        a.setSettings (s);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto t = b.getSettings();
        check (t.stepLength == 4 && t.steps == 11 && t.ownTempo && near (t.ownBpm, 97.5, 0.05) && t.type == FilterType::bandPass && ! t.steep && near (t.lowHz, 440.0, 0.5)
                   && near (t.highHz, 3300.0, 3.0) && near (t.resonancePercent, 61.0, 0.01) && near (t.glidePercent, 33.0, 0.01) && near (t.mixPercent, 63.0, 0.01) && near (t.outputDb, -7.5, 0.05)
                   && near (t.heights[3], 42.5, 0.01) && t.heights[15] == 0.0f && t.heights[0] == 100.0f,
               "step filter: the pattern and settings survive saving and loading its state");
    }

    checkThroughProject<BuiltInStepFilter> (BuiltInStepFilter::getDescription(), "DawIt Step Filter",
                                            [] (BuiltInStepFilter& p)
                                            {
                                                p.setHeight (3, 25.0f);
                                                p.setHeight (9, 0.0f);
                                                p.setKnob (Knob::resonance, 70.0f);
                                                p.setKnob (Knob::type, 2.0f);
                                            },
                                            [] (BuiltInStepFilter& p)
                                            {
                                                return std::abs (p.getHeight (3) - 25.0f) < 0.01f && p.getHeight (9) == 0.0f && p.getHeight (0) == 100.0f
                                                       && std::abs (p.getSettings().resonancePercent - 70.0f) < 0.01f && p.getSettings().type == FilterType::highPass;
                                            });

    // Latency and bypass.
    {
        BuiltInStepFilter plugin;
        plugin.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;

        for (int i = 0; i < 256; ++i)
            buffer.setSample (0, i, (float) std::sin (i * 0.1)), buffer.setSample (1, i, (float) std::cos (i * 0.1));

        const auto copy = buffer;
        plugin.processBlockBypassed (buffer, midi);
        bool same = true;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                same = same && juce::exactlyEqual (buffer.getSample (ch, i), copy.getSample (ch, i));

        check (plugin.getLatencySamples() == 0 && same, "step filter: it holds nothing back (latency 0), and bypassed it passes the track unchanged");
    }

    // Measured responses: a sine through the filter against the response maths, at three sample rates.
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
    {
        const auto lp = measuredDb (steadyCutoff (1000.0, FilterType::lowPass, false), 1000.0, rate);
        const auto hp = measuredDb (steadyCutoff (1000.0, FilterType::highPass, false), 250.0, rate);
        const auto bp = measuredDb (steadyCutoff (1000.0, FilterType::bandPass, false, 50.0f), 1000.0, rate);
        const auto lp24 = measuredDb (steadyCutoff (1000.0, FilterType::lowPass, true), 2000.0, rate);
        const auto res = measuredDb (steadyCutoff (1000.0, FilterType::lowPass, false, 50.0f), 1000.0, rate);
        check (near (lp, -3.0, 0.5) && near (hp, -24.1, 0.5) && near (bp, 0.0, 0.5) && near (lp24, -24.6, 0.5) && near (res, 11.5, 0.5),
               "step filter at " + juce::String (rate, 0) + " Hz: a sine through each type matches the response (LP " + juce::String (lp, 2) + ", HP " + juce::String (hp, 2) + ", BP " + juce::String (bp, 2)
                   + ", LP24 " + juce::String (lp24, 2) + ", res " + juce::String (res, 2) + " dB)");
    }

    // Mix and Output.
    {
        auto s = steadyCutoff (1000.0, FilterType::lowPass, true);
        s.mixPercent = 50.0f;
        const auto mix24 = measuredDb (s, 1000.0, 48000.0);
        s.steep = false;
        const auto mix12 = measuredDb (s, 1000.0, 48000.0);
        check (near (mix24, -12.0, 0.3) && near (mix12, -4.3, 0.3), "step filter: at Mix 50% a sine at the cutoff comes out -12.0 dB through a 24 dB low-pass and -4.3 dB through 12 dB (" + juce::String (mix24, 2) + ", " + juce::String (mix12, 2) + ")");

        BuiltInStepFilter plugin;
        s.mixPercent = 0.0f;
        plugin.setSettings (s);
        const auto out = drive (plugin, nullptr, 48000.0, 24000, sine (440.0, 48000.0, 0.5f));
        float most = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 24000; ++i)
                most = juce::jmax (most, std::abs (out.getSample (ch, i) - 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / 48000.0)));

        check (most < 1.0e-6f, "step filter: at Mix 0% the output is the input (" + juce::String (most, 9) + ")");

        s.outputDb = -6.0f;
        BuiltInStepFilter quieter;
        quieter.setSettings (s);
        const auto lower = drive (quieter, nullptr, 48000.0, 48000, sine (440.0, 48000.0, 0.5f));
        check (near (amplitude (lower, 36000, 48000) / 0.5, 0.501, 0.501 * 0.005), "step filter: Output -6 dB scales the sound by 0.501");
    }

    // Hear only what it removes: it and the filtered sound add up to the input.
    {
        auto s = steadyCutoff (800.0, FilterType::lowPass, true);
        BuiltInStepFilter filtered, removed;
        filtered.setSettings (s);
        removed.setSettings (s);
        removed.listenToEffect = true;
        const auto input = [] (int i) { return 0.3f * (float) std::sin (i * 0.05) + 0.2f * (float) std::sin (i * 0.9); };
        const auto a = drive (filtered, nullptr, 48000.0, 9600, input);
        const auto b = drive (removed, nullptr, 48000.0, 9600, input);
        float worst = 0.0f;

        for (int i = 0; i < 9600; ++i)
            worst = juce::jmax (worst, std::abs (a.getSample (0, i) + b.getSample (0, i) - input (i)));

        check (worst < 1.0e-5f && b.getMagnitude (0, 4800, 4800) > 0.05f, "step filter: Hear only what it removes plays the input minus the filtered sound (" + juce::String (worst, 8) + ")");
    }

    // Silence, stereo.
    {
        BuiltInStepFilter plugin;
        auto s = steadyCutoff (500.0, FilterType::lowPass, true, 90.0f);
        plugin.setSettings (s);
        const auto quiet = drive (plugin, nullptr, 48000.0, 48000, [] (int) { return 0.0f; });
        check (quiet.getMagnitude (0, 0, 48000) < 1.0e-9f, "step filter: silence in is silence out");

        BuiltInStepFilter loud;
        loud.setSettings (s);
        const auto tail = drive (loud, nullptr, 48000.0, 96000, [] (int i) { return i < 48000 ? 0.8f * (float) std::sin (i * 0.07) : 0.0f; });
        check (tail.getMagnitude (0, 96000 - 4800, 4800) < 1.0e-6f, "step filter: after loud sound and a second of silence it is silent again (" + juce::String (tail.getMagnitude (0, 91200, 4800), 9) + ")");

        BuiltInStepFilter stereo;
        stereo.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;

        for (int i = 0; i < 256; ++i)
            buffer.setSample (0, i, (float) std::sin (i * 0.2)), buffer.setSample (1, i, 0.0f);

        stereo.processBlock (buffer, midi);
        check (buffer.getMagnitude (0, 0, 256) > 0.1f && buffer.getMagnitude (1, 0, 256) == 0.0f, "step filter: sound on the left only leaves the right silent");

        BuiltInStepFilter both;
        const auto sides = drive (both, nullptr, 48000.0, 4800, sine (330.0, 48000.0, 0.4f));
        float worst = 0.0f;

        for (int i = 0; i < 4800; ++i)
            worst = juce::jmax (worst, std::abs (sides.getSample (0, i) - sides.getSample (1, i)));

        check (worst == 0.0f, "step filter: equal inputs give equal outputs");
    }

    // Stability: resonance at 100%, the cutoff jumping between its lowest and highest on every 1/32, white noise.
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
    {
        BuiltInStepFilter plugin;
        stf::Settings s;
        s.stepLength = 0;   // 1/32
        s.lowHz = 20.0f;
        s.highHz = 20000.0f;
        s.resonancePercent = 100.0f;
        s.glidePercent = 0.0f;

        for (int i = 0; i < 16; ++i)
            s.heights[(size_t) i] = i % 2 == 0 ? 0.0f : 100.0f;

        plugin.setSettings (s);
        juce::Random random (5);
        Head head;
        const int n = (int) (rate * 10.0);
        const auto out = drive (plugin, &head, rate, n, [&random] (int) { return random.nextFloat() * 2.0f - 1.0f; }, steady (120.0), 512);
        bool finite = true;
        float peak = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float v = out.getSample (0, i);
            finite = finite && std::isfinite (v);
            peak = juce::jmax (peak, std::abs (v));
        }

        check (finite && peak < 100.0f, "step filter at " + juce::String (rate, 0) + " Hz: 10 s of noise at Resonance 100% and the cutoff jumping every 1/32 stays finite, peak " + juce::String (peak, 1));
    }

    // Clicks: a 110 Hz sine, Glide 0%, a 12 dB low-pass, steps alternating 0 and 100%.
    {
        stf::Settings s;
        s.type = FilterType::lowPass;
        s.steep = false;
        s.lowHz = 200.0f;
        s.highHz = 5000.0f;
        s.glidePercent = 0.0f;
        s.stepLength = 4;   // 1/8

        for (int i = 0; i < 16; ++i)
            s.heights[(size_t) i] = i % 2 == 0 ? 0.0f : 100.0f;

        auto biggestJump = [] (const juce::AudioBuffer<float>& b, int from)
        {
            float most = 0.0f;

            for (int i = from + 1; i < b.getNumSamples(); ++i)
                most = juce::jmax (most, std::abs (b.getSample (0, i) - b.getSample (0, i - 1)));

            return most;
        };

        BuiltInStepFilter moving, steadyOne;
        moving.setSettings (s);
        auto flat = s;
        flat.heights.fill (100.0f);
        steadyOne.setSettings (flat);
        Head head, head2;
        const int n = 48000 * 4;
        const auto a = drive (moving, &head, 48000.0, n, sine (110.0, 48000.0, 0.5f), steady (120.0));
        const auto b = drive (steadyOne, &head2, 48000.0, n, sine (110.0, 48000.0, 0.5f), steady (120.0));
        const float moved = biggestJump (a, 4800), calm = biggestJump (b, 4800);
        check (moved <= 3.0f * calm, "step filter: with Glide 0% a step boundary doesn't click (largest sample-to-sample change " + juce::String (moved, 4) + " against " + juce::String (calm, 4) + " steady)");
    }

    // Glide: the cutoff in octaves, straight, over Glide's share of the step.
    {
        stf::Settings s;
        s.lowHz = 250.0f;
        s.highHz = 4000.0f;
        s.glidePercent = 50.0f;
        s.heights.fill (0.0f);
        s.heights[1] = 100.0f;   // step 2 at ppq 0.25 to 0.5 (1/16)

        BuiltInStepFilter plugin;
        plugin.setSettings (s);
        Head head;
        std::vector<std::pair<double, float>> seen;   // ms since the step began, the cutoff after the block
        const int block = 24;   // 0.5 ms
        drive (plugin, &head, 48000.0, 48000 / 4, [] (int) { return 0.0f; }, steady (120.0), block, true,
               [&] (int done) { seen.push_back ({ done / 48.0 - 125.0, plugin.currentCutoffHz.load() }); });

        auto cutoffAt = [&] (double ms)
        {
            for (auto& [t, hz] : seen)
                if (t >= ms - 0.01)
                    return (double) hz;

            return 0.0;
        };

        check (near (cutoffAt (31.25), 1000.0, 20.0) && near (cutoffAt (62.5), 4000.0, 1.0) && near (cutoffAt (-1.0), 250.0, 0.5),
               "step filter: Glide 50% of a 1/16 at 120 BPM passes 1 kHz at 31.25 ms and reaches 4 kHz at 62.5 ms (" + juce::String (cutoffAt (31.25), 0) + ", " + juce::String (cutoffAt (62.5), 0) + ")");

        // Glide 0%: complete within 2 ms and one block.
        s.glidePercent = 0.0f;
        BuiltInStepFilter jumpy;
        jumpy.setSettings (s);
        Head head2;
        seen.clear();
        drive (jumpy, &head2, 48000.0, 48000 / 4, [] (int) { return 0.0f; }, steady (120.0), block, true,
               [&] (int done) { seen.push_back ({ done / 48.0 - 125.0, jumpy.currentCutoffHz.load() }); });
        double doneAt = -1.0;

        for (auto& [t, hz] : seen)
            if (t > 0.0 && doneAt < 0.0 && hz >= 3999.0f)
                doneAt = t;

        check (doneAt > 0.0 && doneAt <= 2.0 + 0.5 + 0.01, "step filter: Glide 0% still takes the 2 ms minimum to move, no more (done at " + juce::String (doneAt, 2) + " ms)");
    }

    // The clock.
    {
        stf::Settings s;
        s.steps = 16;
        BuiltInStepFilter plugin;
        plugin.setSettings (s);
        Head head;
        drive (plugin, &head, 48000.0, 256, [] (int) { return 0.0f; }, steady (120.0, 9.0));
        check (plugin.playingStep.load() == 4 && plugin.usingSongClock.load() && near (plugin.clockBeat.load(), 9.0, 1.0e-9), "step filter: at ppq 9.0 it is on step 5 (index 4), following the song");

        // A seek.
        drive (plugin, &head, 48000.0, 256, [] (int) { return 0.0f; }, steady (120.0, 5.5), 256, false);
        check (plugin.playingStep.load() == 6, "step filter: after a seek to ppq 5.5 it is on step 7");

        // A loop wrap: the song's position goes back to the start.
        drive (plugin, &head, 48000.0, 256, [] (int) { return 0.0f; }, steady (120.0, 7.99), 256, false);
        drive (plugin, &head, 48000.0, 256, [] (int) { return 0.0f; }, steady (120.0, 0.0), 256, false);
        check (plugin.playingStep.load() == 0, "step filter: after a loop wrap to ppq 0 it is on step 1");

        // A tempo change.
        drive (plugin, &head, 48000.0, 256, [] (int) { return 0.0f; }, steady (90.0, 1.0), 256, false);
        check (near (plugin.getUsedBpm(), 90.0, 1.0e-9) && plugin.playingStep.load() == 4, "step filter: a tempo change to 90 BPM is followed (ppq 1.0 is step 5)");

        // Stopped: its own clock, at the song's tempo, running on.
        BuiltInStepFilter stopped;
        Head stoppedHead;
        stoppedHead.playing = false;
        stoppedHead.bpm = 150.0;
        const auto ownClock = [] (Head& h, int) { h.playing = false; h.bpm = 150.0; };
        drive (stopped, &stoppedHead, 48000.0, 256 * 100, [] (int) { return 0.0f; }, ownClock);
        check (! stopped.usingSongClock.load() && near (stopped.getUsedBpm(), 150.0, 1.0e-9) && stopped.clockBeat.load() > 0.5, "step filter: while the song is stopped it keeps its own clock at the song's tempo");

        // No tempo from the host: its own BPM.
        BuiltInStepFilter noTempo;
        auto t = noTempo.getSettings();
        t.ownBpm = 90.0f;
        noTempo.setSettings (t);
        Head noTempoHead;
        noTempoHead.hasBpm = false;
        noTempoHead.playing = false;
        drive (noTempo, &noTempoHead, 48000.0, 512, [] (int) { return 0.0f; });
        check (near (noTempo.getUsedBpm(), 90.0, 1.0e-9) && noTempo.tempoBpm.load() == 0.0, "step filter: when the host gives no tempo it uses its own BPM");

        // Own tempo: the song's position in seconds at the own BPM.
        BuiltInStepFilter own;
        auto o = own.getSettings();
        o.ownTempo = true;
        o.ownBpm = 60.0f;
        own.setSettings (o);
        Head ownHead;
        drive (own, &ownHead, 48000.0, 256, [] (int) { return 0.0f; }, [] (Head& h, int) { h.seconds = 2.0; h.ppq = 100.0; });
        check (near (own.clockBeat.load(), 2.0, 1.0e-9) && own.playingStep.load() == 8, "step filter: with Tempo Own the song's time in seconds counts beats at the own BPM");
    }

    // The test chord.
    {
        BuiltInStepFilter plugin;
        auto s = plugin.getSettings();
        s.mixPercent = 0.0f;   // the chord as it goes in
        plugin.setSettings (s);
        plugin.prepareToPlay (48000.0, 256);
        plugin.clap.request();
        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;
        float peak = 0.0f;
        double lateEnergy = 0.0;
        int firstStep = -2;

        for (int block = 0; block < 48000 * 3 / 256; ++block)
        {
            buffer.clear();
            plugin.processBlock (buffer, midi);

            if (block == 0)
                firstStep = plugin.playingStep.load();

            peak = juce::jmax (peak, buffer.getMagnitude (0, 0, 256));

            if (block * 256 > 48000 * 21 / 10)
                lateEnergy += buffer.getMagnitude (0, 0, 256);
        }

        check (firstStep == 0 && peak > 0.12f && peak < 0.26f && lateEnergy == 0.0, "step filter: Play a bar starts on step 1 and plays a held chord of about -12 dBFS for one bar (peak " + juce::String (peak, 3) + ")");
    }

    // The history of steps.
    {
        BuiltInStepFilter plugin;
        auto s = plugin.getSettings();
        s.lowHz = 250.0f;
        s.highHz = 4000.0f;
        s.heights[1] = 50.0f;
        plugin.setSettings (s);
        Head head;
        drive (plugin, &head, 48000.0, 24000, sine (440.0, 48000.0, 0.5f), steady (120.0));
        std::vector<stf::StepRecord> records (64);
        const int n = plugin.history.getRecent (records.data(), 64);
        check (n == 2 && records[0].step == 1 && near (records[0].cutoffHz, 1000.0, 5.0) && records[0].inPeak > 0.45f && near (records[0].beat, 0.25, 1.0e-9) && records[1].step == 2 && near (records[1].cutoffHz, 4000.0, 20.0),
               "step filter: the history records each finished step with its cutoff and the levels in and out (" + juce::String (n) + ")");
    }
}

/** The advice of *Your track*: rules F0 to F6 on made-up step records. */
static void runStepFilterAnalysisTests()
{
    using namespace daw;
    std::cout << "Step filter: advice" << std::endl;

    auto record = [] (int step, float inDb, float outDb, float peakGainDb = -1000.0f)
    {
        stf::StepRecord r;
        r.step = step;
        r.inPeak = juce::Decibels::decibelsToGain (inDb);
        r.inRms = r.inPeak * 0.7f;
        r.outRms = juce::Decibels::decibelsToGain (outDb) * 0.7f;
        r.outPeak = juce::Decibels::decibelsToGain (peakGainDb > -999.0f ? inDb + peakGainDb : outDb);
        return r;
    };

    auto moving = [] (float lowHz = 300.0f, float highHz = 5000.0f)
    {
        stf::Settings s;
        s.lowHz = lowHz;
        s.highHz = highHz;

        for (int i = 0; i < 16; ++i)
            s.heights[(size_t) i] = i % 2 == 0 ? 0.0f : 100.0f;

        return s;
    };

    // Records for 16 steps: input -6 dB, the output level changes by even and odd steps' own amounts.
    auto steps = [&] (float evenChange, float oddChange)
    {
        std::vector<stf::StepRecord> v;

        for (int i = 0; i < 16; ++i)
            v.push_back (record (i, -6.0f, -6.0f + (i % 2 == 0 ? evenChange : oddChange)));

        return v;
    };

    auto contains = [] (const std::vector<juce::String>& lines, const juce::String& text) { return std::any_of (lines.begin(), lines.end(), [&] (auto& l) { return l.contains (text); }); };

    // F0: nothing heard.
    {
        stf::AnalysisInput in;
        in.settings = moving();
        in.records.assign (8, record (0, -70.0f, -70.0f));
        const auto lines = stf::analyse (in);
        check (lines.size() == 1 && lines[0] == "Play the track, or click Play a bar, to see it here.", "advice F0: below -60 dBFS everywhere, it asks to play the track and says nothing else");
        in.records.clear();
        check (stf::analyse (in).size() == 1, "advice F0: no records at all says the same");
        in.records.assign (8, record (0, -59.0f, -59.0f));
        check (stf::analyse (in).front() != "Play the track, or click Play a bar, to see it here.", "advice F0: at -59 dBFS the track counts as heard");
    }

    // F1: no song tempo.
    {
        stf::AnalysisInput in;
        in.settings = moving();
        in.settings.ownBpm = 100.0f;
        in.records = steps (-2.0f, -14.0f);
        in.hostHasTempo = false;
        check (contains (stf::analyse (in), "steps at its own 100 BPM"), "advice F1: no tempo from the song says it steps at its own tempo");
        in.hostHasTempo = true;
        check (! contains (stf::analyse (in), "own 100 BPM"), "advice F1: not when the song gives a tempo");
        in.hostHasTempo = false;
        in.settings.ownTempo = true;
        check (! contains (stf::analyse (in), "own 100 BPM"), "advice F1: not when Own is chosen");
    }

    // F2: nothing moves.
    for (const auto [octaves, fires] : { std::pair<double, bool> { 1.0 / 24.0, true }, { 1.0, false } })
    {
        stf::AnalysisInput in;
        in.settings = moving (1000.0f, (float) (1000.0 * std::pow (2.0, octaves)));
        in.records = steps (-2.0f, -14.0f);
        const auto lines = stf::analyse (in);
        check (contains (lines, "Every step has the same cutoff (") == fires, "advice F2: steps " + juce::String (octaves, 3) + " octave apart " + (fires ? "do" : "don't") + " say the tone doesn't move");
    }

    {
        stf::AnalysisInput in;
        in.settings = moving (1200.0f, 1200.0f);
        in.records = steps (-2.0f, -14.0f);
        check (contains (stf::analyse (in), "Every step has the same cutoff (1.2 kHz), so the tone doesn't move. Drag some steps up or down, or turn Low cutoff and High cutoff apart."),
               "advice F2: the text names the cutoff");
    }

    // F3: hard to hear.
    for (const auto [spread, fires] : { std::pair<float, bool> { 1.1f, true }, { 2.0f, false } })
    {
        stf::AnalysisInput in;
        in.settings = moving();
        in.records = steps (-3.0f, -3.0f - spread);
        const auto lines = stf::analyse (in);
        check (contains (lines, "The steps differ in what they do to this track's level by at most " + juce::String (spread, 1) + " dB, so the pattern may be hard to hear. The track may have little sound between 300 Hz and 5.0 kHz:") == fires,
               "advice F3: steps whose levels differ by " + juce::String (spread, 1) + " dB " + (fires ? "are" : "are not") + " pointed out");
    }

    {
        stf::AnalysisInput in;
        in.settings = moving (1000.0f, 1000.0f);
        in.records = steps (-3.0f, -3.5f);
        check (! contains (stf::analyse (in), "hard to hear"), "advice F3: not when F2 already says nothing moves");
    }

    // F4: louder.
    for (const auto [gain, fires] : { std::pair<float, bool> { 9.0f, true }, { 5.0f, false } })
    {
        stf::AnalysisInput in;
        in.settings = moving();
        in.records = steps (-2.0f, -14.0f);
        in.records[3] = record (3, -6.0f, -6.0f, gain);
        const auto lines = stf::analyse (in);
        check (contains (lines, "At its loudest the filter makes the track " + juce::String ((int) gain) + " dB louder than it came in, from the resonance peak. Turn Output down by about "
                                    + juce::String ((int) gain) + " dB, or lower Resonance, so nothing after it clips.") == fires,
               "advice F4: an output peak " + juce::String (gain, 0) + " dB above the input " + (fires ? "is" : "is not") + " pointed out");
    }

    // F5: an almost silent step.
    for (const auto [drop, fires] : { std::pair<float, bool> { 34.0f, true }, { 25.0f, false } })
    {
        stf::AnalysisInput in;
        in.settings = moving();
        in.records = steps (-2.0f, -14.0f);
        in.records[2] = record (2, -6.0f, -6.0f - drop);
        const auto lines = stf::analyse (in);
        check (contains (lines, "Step 3 takes the track down by " + juce::String ((int) drop) + " dB, almost to silence.") == fires,
               "advice F5: a step " + juce::String (drop, 0) + " dB down " + (fires ? "is" : "is not") + " pointed out");
    }

    // F6: the summary.
    {
        stf::AnalysisInput in;
        in.settings = moving();
        in.records = steps (-2.0f, -14.0f);
        const auto lines = stf::analyse (in);
        check (lines.back() == juce::String::fromUTF8 ("The last 2 bars: 16 steps of 1/16 (125 ms at 120 BPM), a low-pass moving between 300 Hz and 5.0 kHz; the steps change the level from \xe2\x88\x92" "14 to \xe2\x88\x92" "2 dB."),
               "advice F6: the summary names the steps, the tempo, the filter and the level change (" + lines.back() + ")");
        // Direction: a boost and a cut are not the same size; equal steps give one number.
        auto mixed = in;
        mixed.records = steps (3.0f, -3.0f);
        check (stf::analyse (mixed).back().endsWith (juce::String::fromUTF8 ("from \xe2\x88\x92" "3 to +3 dB.")), "advice F6: a +3 dB step and a -3 dB step read as from -3 to +3 dB (" + stf::analyse (mixed).back() + ")");
        auto same = in;
        same.records = steps (-3.0f, -3.0f);
        check (stf::analyse (same).back().endsWith (juce::String::fromUTF8 ("by \xe2\x88\x92" "3 dB.")), "advice F6: steps that all change the level by the same amount give one number, not 3 to 3 (" + stf::analyse (same).back() + ")");
        in.settings.type = filt::FilterType::bandPass;
        in.bpm = 90.0;
        in.settings.stepLength = 4;
        check (stf::analyse (in).back().startsWith ("The last 2 bars: 16 steps of 1/8 (333 ms at 90 BPM), a band-pass moving"), "advice F6: other tempos, step lengths and types read the same way");
    }
}
