// DawIt Spectral Freeze: the engine tests (included by EngineTests.cpp, after check and checkThroughProject).
#pragma once

namespace sfztests
{
using namespace daw;

constexpr double twoPi = juce::MathConstants<double>::twoPi;

using Input = std::function<float (int channel, int sample)>;

/** Plays `numSamples` through the freeze in blocks of `block`; `beforeBlock` gets the sample position each block starts at. */
inline juce::AudioBuffer<float> drive (BuiltInSpectralFreeze& plugin, double rate, int numSamples, const Input& input, int block = 512, bool prepare = true,
                                       const std::function<void (int)>& beforeBlock = {})
{
    if (prepare)
        plugin.prepareToPlay (rate, block);

    juce::AudioBuffer<float> out (2, numSamples), buffer (2, block);
    juce::MidiBuffer midi;

    for (int pos = 0; pos < numSamples; pos += block)
    {
        const int n = juce::jmin (block, numSamples - pos);
        buffer.setSize (2, n, false, false, true);

        if (beforeBlock)
            beforeBlock (pos);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buffer.setSample (ch, i, input (ch, pos + i));

        plugin.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, buffer, ch, 0, n);
    }

    return out;
}

inline Input sine (double hz, double rate, float amplitude, int stopAt = 1 << 30)
{
    return [=] (int, int i) { return i < stopAt ? amplitude * (float) std::sin (twoPi * hz * i / rate) : 0.0f; };
}

/** The amplitude (peak level of a sine with that power) in dB over `from` to `to` of a channel. */
inline double sineDb (const juce::AudioBuffer<float>& b, int from, int to, int ch = 0)
{
    double sum = 0.0;

    for (int i = from; i < to; ++i)
        sum += (double) b.getSample (ch, i) * b.getSample (ch, i);

    return 10.0 * std::log10 (juce::jmax (1.0e-20, 2.0 * sum / (to - from)));
}

inline double meanDb (const juce::AudioBuffer<float>& b, int from, int to, int ch = 0)
{
    double sum = 0.0;

    for (int i = from; i < to; ++i)
        sum += (double) b.getSample (ch, i) * b.getSample (ch, i);

    return 10.0 * std::log10 (juce::jmax (1.0e-20, sum / (to - from)));
}

inline double peakDb (const juce::AudioBuffer<float>& b, int from, int to, int ch = 0)
{
    float peak = 0.0f;

    for (int i = from; i < to; ++i)
        peak = juce::jmax (peak, std::abs (b.getSample (ch, i)));

    return juce::Decibels::gainToDecibels (peak, -200.0f);
}

/** The frequency of a steady sine from its rising zero crossings, with the crossing times interpolated. */
inline double measureHz (const juce::AudioBuffer<float>& b, int from, int to, double rate, int ch = 0)
{
    double first = -1.0, last = -1.0;
    int crossings = 0;

    for (int i = from + 1; i < to; ++i)
    {
        const float a = b.getSample (ch, i - 1), c = b.getSample (ch, i);

        if (a < 0.0f && c >= 0.0f)
        {
            const double t = i - 1 + (double) (-a) / (double) (c - a);

            if (first < 0.0)
                first = t;

            last = t;
            ++crossings;
        }
    }

    return crossings > 1 ? (crossings - 1) * rate / (last - first) : 0.0;
}

/** How loud one frequency is in `from` to `to`, as the amplitude of a sine in dB (a single-bin DFT: use a whole number of cycles). */
inline double toneDb (const juce::AudioBuffer<float>& b, int from, int to, double hz, double rate)
{
    double re = 0.0, im = 0.0;

    for (int i = from; i < to; ++i)
    {
        const double phase = twoPi * hz * (i - from) / rate;
        re += b.getSample (0, i) * std::cos (phase);
        im += b.getSample (0, i) * std::sin (phase);
    }

    const double amplitude = 2.0 * std::sqrt (re * re + im * im) / (to - from);
    return 20.0 * std::log10 (juce::jmax (1.0e-12, amplitude));
}

/** The first block start at or after `seconds`: where a test turns Freeze on, so the capture is at a block boundary. */
inline int captureSample (double seconds, double rate, int block)
{
    return (int) std::ceil (seconds * rate / block) * block;
}

struct Held
{
    juce::AudioBuffer<float> out;
    int captured = 0;   // the sample Freeze was switched on at
};

/** Plays `signal` for about half a second, switches Freeze on and stops the signal, and plays on until `seconds`. */
inline Held holdSignal (BuiltInSpectralFreeze& plugin, double rate, double seconds, const std::function<float (int channel, int i)>& signal,
                        int block = 512, bool prepare = true, double freezeAt = 0.5)
{
    Held h;
    h.captured = captureSample (freezeAt, rate, block);
    const int stop = h.captured;
    h.out = drive (plugin, rate, (int) (seconds * rate), [&] (int ch, int i) { return i < stop ? signal (ch, i) : 0.0f; }, block, prepare,
                   [&] (int pos) { if (pos == h.captured) plugin.setFreeze (true); });
    return h;
}

inline const double steadyDb = -12.0;   // a sine of amplitude 0.2512

inline float steadyAmplitude() { return juce::Decibels::decibelsToGain (-12.0f); }
}

static void runBuiltInSpectralFreezeTests()
{
    using namespace daw;
    using namespace sfztests;
    using sfz::Knob;
    std::cout << "Built-in spectral freeze" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // The maths (the plan's table).
    check (near (sfz::getBinSpacingHz (44100.0, 4096), 10.7666, 0.001) && near (sfz::getBinSpacingHz (48000.0, 4096), 11.7188, 0.001) && near (sfz::getBinSpacingHz (96000.0, 8192), 11.7188, 0.001),
           "spectral freeze: bins are 10.77 Hz apart at 44.1 kHz and 4,096, and 11.72 Hz at 48 kHz and at 96 kHz with 8,192");
    check (near (sfz::getWindowMs (44100.0, 4096), 92.88, 0.01) && near (sfz::getWindowMs (48000.0, 4096), 85.33, 0.01)
               && sfz::getFftSize (44100.0) == 4096 && sfz::getFftSize (48000.0) == 4096 && sfz::getFftSize (88200.0) == 8192 && sfz::getFftSize (96000.0) == 8192 && sfz::getFftSize (192000.0) == 16384,
           "spectral freeze: a piece lasts 92.88 ms at 44.1 kHz and 85.33 ms at 48 kHz; the size is 4,096 up to 48 kHz, 8,192 at 88.2 and 96 kHz, 16,384 above");
    check (sfz::countBins (20.0, 40.0, 44100.0, 4096) == 2 && sfz::countBins (10000.0, 20000.0, 44100.0, 4096) == 929 && sfz::getNumBins (4096) == 2049,
           "spectral freeze: there are 2,049 bins, 2 between 20 and 40 Hz and 929 between 10 and 20 kHz at 44.1 kHz");
    check (near (sfz::getSemitoneGapBins (55.0, 44100.0, 4096), 0.304, 0.01) && near (sfz::getSemitoneGapBins (880.0, 44100.0, 4096), 4.86, 0.01)
               && near (sfz::getSemitoneGapBins (220.0, 44100.0, 4096), 1.215, 0.01),
           "spectral freeze: a semitone is 0.30 bins at A1, 1.2 at A3 and 4.9 at A5");
    check (near (sfz::getBinSpacingHz (44100.0, 4096) * sfz::getWindowMs (44100.0, 4096) / 1000.0, 1.0, 1.0e-9),
           "spectral freeze: bin spacing times window length is 1");

    {
        double worst = 0.0;

        for (int size : { 4096, 8192, 16384 })
            for (int at : { 0, 1, 7, 1023, 1024, 2500, 4095, 5000, 12345 })
                worst = juce::jmax (worst, std::abs (sfz::getOverlapSum (size, at) - 1.5));

        check (worst < 1.0e-6, "spectral freeze: the Hann window squared, overlapped at a quarter, sums to 1.5 at every sample");
    }

    check (near (sfz::getTiltDb (8000.0, 3.0), 9.0, 0.01) && near (sfz::getTiltDb (125.0, 3.0), -9.0, 0.01) && near (sfz::getTiltDb (1000.0, 3.0), 0.0, 0.01),
           "spectral freeze: a tilt of +3 dB per octave is +9 dB at 8 kHz, -9 dB at 125 Hz and 0 dB at 1 kHz");
    {
        const auto half = sfz::getFadeGains (0.5, 100.0);
        const auto none = sfz::getFadeGains (0.0, 100.0), full = sfz::getFadeGains (1.0, 100.0), mixHalf = sfz::getFadeGains (1.0, 50.0);
        check (near (20.0 * std::log10 (half.first), -3.01, 0.1) && near (20.0 * std::log10 (half.second), -3.01, 0.1)
                   && near (none.first, 1.0, 1.0e-12) && near (none.second, 0.0, 1.0e-12) && near (full.first, 0.0, 1.0e-9) && near (full.second, 1.0, 1.0e-9)
                   && near (20.0 * std::log10 (mixHalf.first), -3.01, 0.1) && near (20.0 * std::log10 (mixHalf.second), -3.01, 0.1),
               "spectral freeze: half way through the fade at Mix 100% the dry sound and the hold are both -3.01 dB; at Mix 50% they end there");
    }
    check (sfz::getCaptureSpan (4096) == 7168 && near (7168.0 / 44100.0 * 1000.0, 162.5, 0.1), "spectral freeze: a capture spans 7,168 samples (162.5 ms) at 44.1 kHz");
    {
        const auto a3 = sfz::getNearestNote (220.0), as3 = sfz::getNearestNote (233.08), a4 = sfz::getNearestNote (440.0);
        check (sfz::getNoteName (220.0) == "A3" && near (a3.cents, 0.0, 1.0) && sfz::getNoteName (233.08) == "A#3" && near (as3.cents, 0.0, 1.0)
                   && sfz::getNoteName (440.0) == "A4" && a4.midi == 69 && sfz::getPitchClass (as3.midi) == 10 && sfz::getNoteName (261.63) == "C4",
               "spectral freeze: 220 Hz is A3 and 233.08 Hz is A#3, each within a cent");
    }
    {
        // A pure sine at fractional bin 100.3 over 4,096 samples: the phase step between pieces gives its exact frequency.
        const double hz = 100.3 * 44100.0 / 4096.0;
        const int size = 4096, hopSize = 1024;
        auto w = sfz::makeHann (size);
        juce::dsp::FFT fft (12);
        std::array<std::vector<float>, 2> frames;

        for (int f = 0; f < 2; ++f)
        {
            frames[(size_t) f].assign ((size_t) size * 2, 0.0f);

            for (int i = 0; i < size; ++i)
                frames[(size_t) f][(size_t) i] = (float) std::sin (twoPi * hz * (i + f * hopSize) / 44100.0) * w[(size_t) i];

            fft.performRealOnlyForwardTransform (frames[(size_t) f].data(), true);
        }

        const int bin = 100;
        const double p0 = std::atan2 (frames[0][2 * bin + 1], frames[0][2 * bin]), p1 = std::atan2 (frames[1][2 * bin + 1], frames[1][2 * bin]);
        check (near (sfz::getPhaseVocoderHz (bin, p0, p1, 44100.0, size), hz, 0.05),
               "spectral freeze: the phase step between two pieces gives the exact frequency of a bin (" + juce::String (sfz::getPhaseVocoderHz (bin, p0, p1, 44100.0, size), 2) + " Hz for " + juce::String (hz, 2) + ")");
    }
    {
        std::vector<double> flat (500, 1.0), tone (500, 1.0e-14);
        tone[10] = 1.0;
        check (near (sfz::getSpectralFlatness (flat.data(), 500), 1.0, 1.0e-9) && sfz::getSpectralFlatness (tone.data(), 500) < 0.01,
               "spectral freeze: spectral flatness is 1 for a flat spectrum and near 0 for a single tone");
    }

    // The inverse transform's scale and the forward transform's size: a sine of amplitude A has a peak bin of A * N / 4.
    {
        juce::dsp::FFT fft (12);
        std::vector<float> data (8192, 0.0f);
        const auto w = sfz::makeHann (4096);

        for (int i = 0; i < 4096; ++i)
            data[(size_t) i] = 0.5f * (float) std::sin (twoPi * 100.0 * i / 4096.0) * w[(size_t) i];

        fft.performRealOnlyForwardTransform (data.data(), true);
        check (near (std::hypot (data[200], data[201]), 0.5 * 4096.0 / 4.0, 1.0), "spectral freeze: the forward FFT's peak for a sine of amplitude A is A * N / 4 with a Hann window");
    }

    // The parameters, their text and the six starting points.
    {
        BuiltInSpectralFreeze plugin;
        const auto s = plugin.getSettings();
        check (! s.freeze && near (s.fadeMs, 300.0, 0.01) && near (s.brightnessDb, 0.0, 0.01) && near (s.movementPercent, 20.0, 0.01) && near (s.mixPercent, 100.0, 0.01) && near (s.outputDb, 0.0, 0.01),
               "spectral freeze: the defaults are Fade 300 ms, Brightness 0, Movement 20%, Mix 100%, Output 0 dB, not holding");
        plugin.setKnob (Knob::fade, 10000.0f);
        plugin.setKnob (Knob::brightness, -6.0f);
        plugin.setKnob (Knob::output, 12.0f);
        check (near (plugin.getSettings().fadeMs, 10000.0, 0.5) && near (plugin.getSettings().brightnessDb, -6.0, 0.01) && near (plugin.getSettings().outputDb, 12.0, 0.01),
               "spectral freeze: Fade goes to 10 s, Brightness to -6 dB per octave and Output to +12 dB");
        check (sfz::formatKnobValue (Knob::fade, 300.0f) == "300 ms" && sfz::formatKnobValue (Knob::fade, 1500.0f) == "1.5 s" && sfz::formatKnobValue (Knob::fade, 10000.0f) == "10 s"
                   && sfz::formatKnobValue (Knob::brightness, 3.0f) == "+3.0 dB/oct" && sfz::formatKnobValue (Knob::movement, 20.0f) == "20%" && sfz::formatKnobValue (Knob::output, -9.0f) == "-9.0 dB",
               "spectral freeze: knob texts are 300 ms, 1.5 s, 10 s, +3.0 dB/oct, 20% and -9.0 dB");
        check (plugin.getLatencySamples() == 0, "spectral freeze: no latency");
        plugin.setKnob (Knob::fade, 2000.0f);
        check (near (plugin.getTailLengthSeconds(), 2.0, 0.01), "spectral freeze: the tail is as long as Fade");

        const auto& presets = sfz::getPresets();
        bool valid = presets.size() == 6 && presets[0].name == "Held chord";

        for (auto& p : presets)
        {
            const auto& q = p.settings;
            valid = valid && p.why.isNotEmpty() && p.name.isNotEmpty() && q.fadeMs >= sfz::minFadeMs && q.fadeMs <= sfz::maxFadeMs && q.brightnessDb >= sfz::minBrightnessDb && q.brightnessDb <= sfz::maxBrightnessDb
                    && q.movementPercent >= 0.0f && q.movementPercent <= 100.0f && q.mixPercent >= 0.0f && q.mixPercent <= 100.0f && ! q.freeze;
        }

        check (valid, "spectral freeze: six starting points, each with why and its values in range, Held chord first");
        check (near (presets[1].settings.fadeMs, 1500.0, 0.01) && near (presets[1].settings.brightnessDb, -1.0, 0.01) && near (presets[1].settings.movementPercent, 30.0, 0.01) && near (presets[1].settings.mixPercent, 50.0, 0.01)
                   && near (presets[5].settings.fadeMs, 10.0, 0.01) && near (presets[5].settings.movementPercent, 0.0, 0.01) && presets[4].name == "Shimmer",
               "spectral freeze: Pad under the playing is 1.5 s, -1 dB/oct, 30%, 50%; Sudden stop is 10 ms and 0%");
    }

    // With nothing held the track passes untouched, at any Mix; bypass leaves it as it is.
    {
        const double rate = 48000.0;
        const auto in = sine (440.0, rate, 0.3f);

        for (float mix : { 0.0f, 50.0f, 100.0f })
        {
            BuiltInSpectralFreeze plugin;
            plugin.setKnob (Knob::mix, mix);
            const auto out = drive (plugin, rate, 24000, in, 333);
            double worst = 0.0;

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 24000; ++i)
                    worst = juce::jmax (worst, (double) std::abs (out.getSample (ch, i) - in (ch, i)));

            check (worst < 1.0e-6, "spectral freeze: with nothing held the output equals the input at Mix " + juce::String (juce::roundToInt (mix)) + "%");
        }

        BuiltInSpectralFreeze plugin;
        plugin.prepareToPlay (rate, 256);
        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;

        for (int i = 0; i < 256; ++i)
            buffer.setSample (0, i, in (0, i)), buffer.setSample (1, i, in (1, i));

        plugin.setFreeze (true);
        plugin.processBlockBypassed (buffer, midi);
        double worst = 0.0;

        for (int i = 0; i < 256; ++i)
            worst = juce::jmax (worst, (double) std::abs (buffer.getSample (0, i) - in (0, i)));

        check (worst < 1.0e-9, "spectral freeze: bypassed, the input comes out unchanged");

        BuiltInSpectralFreeze silent;
        const auto quiet = drive (silent, rate, 24000, [] (int, int) { return 0.0f; });
        check (peakDb (quiet, 0, 24000) < -180.0, "spectral freeze: silence in, silence out");
    }

    // A held 1 kHz sine at -12 dBFS, Movement 0, held for 10 s: its level and pitch stay, at three rates and three block sizes, after the input has stopped.
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
        for (const int block : { 17, 512, 4096 })
        {
            BuiltInSpectralFreeze plugin;
            plugin.setKnob (Knob::movement, 0.0f);
            const auto held = holdSignal (plugin, rate, 10.5, [&] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / rate); }, block);
            const int from = held.captured + (int) (0.5 * rate), to = (int) (10.4 * rate);
            double worstLevel = 0.0;

            for (int a = from; a + (int) (0.05 * rate) <= to; a += (int) (0.05 * rate))
                worstLevel = juce::jmax (worstLevel, std::abs (sineDb (held.out, a, a + (int) (0.05 * rate)) - steadyDb));

            const double hz = measureHz (held.out, from, to, rate);
            check (worstLevel < 0.5 && near (hz, 1000.0, 1.0) && near (sineDb (held.out, from, to), steadyDb, 0.5),
                   "spectral freeze: a held 1 kHz sine stays at -12 dBFS (worst 50 ms off by " + juce::String (worstLevel, 2) + " dB) and 1,000 Hz (" + juce::String (hz, 2) + ") for 10 s at "
                       + juce::String (rate, 0) + " Hz with blocks of " + juce::String (block));
        }

    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        const auto held = holdSignal (plugin, rate, 4.0, [&] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 220.0 * i / rate); });
        const int from = held.captured + (int) (0.5 * rate), to = (int) (3.9 * rate);
        const double hz = measureHz (held.out, from, to, rate);
        check (near (hz, 220.0, 0.5) && near (sineDb (held.out, from, to), steadyDb, 0.5), "spectral freeze: a 220 Hz sine, which falls between bins, stays at 220 Hz (" + juce::String (hz, 3) + ") and -12 dBFS");
    }

    // Noise holds at about its level, with and without Movement.
    for (const float movement : { 0.0f, 100.0f })
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, movement);
        juce::Random random (11);
        std::vector<float> noise ((size_t) (0.6 * rate) + 1024);

        for (auto& v : noise)
            v = (random.nextFloat() * 2.0f - 1.0f) * 0.1732f;   // RMS 0.1, -20 dBFS

        double sum = 0.0;

        for (auto v : noise)
            sum += (double) v * v;

        const double inputDb = 10.0 * std::log10 (sum / (double) noise.size());
        const auto held = holdSignal (plugin, rate, 6.0, [&] (int, int i) { return noise[(size_t) i]; });
        const double level = meanDb (held.out, held.captured + (int) rate, (int) (5.9 * rate));
        check (near (level, inputDb, 1.5), "spectral freeze: white noise held at Movement " + juce::String (juce::roundToInt (movement)) + "% stays within 1.5 dB of its level (" + juce::String (level - inputDb, 2) + " dB off)");
    }

    // Brightness tilts the colour and keeps the level.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        plugin.setKnob (Knob::brightness, 3.0f);
        auto two = [&] (int, int i) { return 0.1f * (float) (std::sin (twoPi * 250.0 * i / rate) + std::sin (twoPi * 4000.0 * i / rate)); };
        const auto held = holdSignal (plugin, rate, 4.0, two);
        const int from = held.captured + (int) rate, to = from + (int) rate;
        const double ratio = toneDb (held.out, from, to, 4000.0, rate) - toneDb (held.out, from, to, 250.0, rate);
        const double inputPower = 10.0 * std::log10 (0.01);   // two sines of amplitude 0.1: 0.005 each
        const double power = meanDb (held.out, from, to);
        check (near (ratio, 12.0, 0.5) && near (power - inputPower, 0.0, 0.2),
               "spectral freeze: +3 dB per octave puts 4 kHz 12 dB over 250 Hz (" + juce::String (ratio, 2) + ") and keeps the total power (" + juce::String (power - inputPower, 2) + " dB off)");
    }

    // Repeatable: Movement 0 and Movement 100 (a bounce twice gives the same sound).
    for (const float movement : { 0.0f, 100.0f })
    {
        const double rate = 48000.0;
        auto run = [&]
        {
            BuiltInSpectralFreeze plugin;
            plugin.setKnob (Knob::movement, movement);
            return holdSignal (plugin, rate, 2.0, [&] (int, int i) { return 0.2f * (float) std::sin (twoPi * 330.0 * i / rate) + 0.05f * (float) std::sin (twoPi * 1230.0 * i / rate); }).out;
        };

        const auto a = run(), b = run();
        double worst = 0.0;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                worst = juce::jmax (worst, (double) std::abs (a.getSample (ch, i) - b.getSample (ch, i)));

        check (worst == 0.0, "spectral freeze: two renders of the same hold at Movement " + juce::String (juce::roundToInt (movement)) + "% are identical");
    }

    // Fade: the hold comes in and goes out over Fade; a short Fade doesn't click.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        plugin.setKnob (Knob::fade, 300.0f);
        const int block = 256;
        const int captured = captureSample (0.5, rate, block), off = captureSample (3.0, rate, block);
        const auto out = drive (plugin, rate, (int) (4.5 * rate), [&] (int, int i) { return i < captured ? steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / rate) : 0.0f; }, block, true,
                                [&] (int pos) { if (pos == captured) plugin.setFreeze (true); if (pos == off) plugin.setFreeze (false); });
        const int win = (int) (0.02 * rate);
        const int mid = captured + (int) (0.15 * rate);
        const double atHalf = sineDb (out, mid - win / 2, mid + win / 2) - steadyDb;
        const int full = captured + (int) (0.3 * rate + 1100);
        const double atFull = sineDb (out, full, full + (int) (0.1 * rate)) - steadyDb;
        const double after = peakDb (out, off + (int) (0.3 * rate) + 1100, (int) (4.5 * rate));
        check (near (atHalf, -3.01, 0.5) && near (atFull, 0.0, 0.3) && after < -100.0,
               "spectral freeze: Fade 300 ms is 3 dB down half way in (" + juce::String (atHalf, 2) + "), full by 300 ms and a hop, and below -100 dBFS by Fade and a hop after Freeze goes off (" + juce::String (after, 1) + ")");
        check (! plugin.isHolding() && plugin.getHoldAmount() < 1.0e-6f, "spectral freeze: the hold lets go when Freeze goes off");
    }

    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        plugin.setKnob (Knob::fade, 10.0f);
        const int block = 256, captured = captureSample (0.5, rate, block);
        const auto out = drive (plugin, rate, (int) (1.5 * rate), [&] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / rate); }, block, true,
                                [&] (int pos) { if (pos == captured) plugin.setFreeze (true); });
        const double steadyStep = steadyAmplitude() * twoPi * 1000.0 / rate;
        double largest = 0.0;

        for (int i = captured - 100; i < (int) (1.4 * rate); ++i)
            largest = juce::jmax (largest, (double) std::abs (out.getSample (0, i) - out.getSample (0, i - 1)));

        check (largest <= 2.0 * steadyStep, "spectral freeze: Fade 10 ms makes no step larger than twice the sine's own (" + juce::String (largest / steadyStep, 2) + " times)");
    }

    // Capture again: the level stays within -3.5 and +1 dB while the old hold gives way to the new.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        plugin.setKnob (Knob::fade, 400.0f);
        const int block = 256, captured = captureSample (0.5, rate, block), again = captureSample (3.0, rate, block);
        const auto out = drive (plugin, rate, (int) (5.0 * rate), [&] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / rate); }, block, true,
                                [&] (int pos) { if (pos == captured) plugin.setFreeze (true); if (pos == again) plugin.requestCapture(); });
        double lowest = 100.0, highest = -100.0;
        const int win = (int) (0.02 * rate);

        for (int a = again - (int) (0.1 * rate); a < again + (int) (0.6 * rate); a += win)
        {
            const double level = sineDb (out, a, a + win) - steadyDb;
            lowest = juce::jmin (lowest, level);
            highest = juce::jmax (highest, level);
        }

        check (lowest > -3.5 && highest < 1.0, "spectral freeze: Capture again crossfades without a hole or a bump (" + juce::String (lowest, 2) + " to " + juce::String (highest, 2) + " dB)");
    }

    // Stereo: L = R stays L = R; a left-only sound has a silent right hold.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 100.0f);
        const auto held = holdSignal (plugin, rate, 3.0, [&] (int, int i) { return 0.2f * (float) std::sin (twoPi * 500.0 * i / rate); });
        double worst = 0.0;

        for (int i = held.captured; i < held.out.getNumSamples(); ++i)
            worst = juce::jmax (worst, (double) std::abs (held.out.getSample (0, i) - held.out.getSample (1, i)));

        check (worst < 1.0e-6 && sineDb (held.out, held.captured + (int) rate, held.captured + 2 * (int) rate) > -20.0, "spectral freeze: a sound that is the same on both sides stays the same at Movement 100% (" + juce::String (worst, 9) + ")");

        BuiltInSpectralFreeze leftOnly;
        const auto lh = holdSignal (leftOnly, rate, 3.0, [&] (int ch, int i) { return ch == 0 ? 0.2f * (float) std::sin (twoPi * 500.0 * i / rate) : 0.0f; });
        check (peakDb (lh.out, lh.captured, lh.out.getNumSamples(), 1) < -100.0 && sineDb (lh.out, lh.captured + (int) rate, lh.captured + 2 * (int) rate, 0) > -20.0,
               "spectral freeze: a sound on the left only has a silent hold on the right");
    }

    // Silence and what's left after the sound stops.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        const auto held = holdSignal (plugin, rate, 2.0, [] (int, int) { return 0.0f; });
        check (peakDb (held.out, 0, held.out.getNumSamples()) < -100.0, "spectral freeze: holding silence gives silence");
    }

    // Rate change: the hold is kept and remapped.
    {
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        const auto held = holdSignal (plugin, 44100.0, 2.0, [&] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / 44100.0); });
        juce::ignoreUnused (held);
        const auto out = drive (plugin, 96000.0, 3 * 96000, [] (int, int) { return 0.0f; }, 512, true);
        const int from = 96000, to = 3 * 96000 - 4800;
        const double hz = measureHz (out, from, to, 96000.0), level = sineDb (out, from, to);
        check (near (hz, 1000.0, 2.0) && near (level, steadyDb, 0.5), "spectral freeze: a hold captured at 44.1 kHz and prepared at 96 kHz is still 1 kHz (" + juce::String (hz, 2) + ") at -12 dBFS (" + juce::String (level, 2) + ")");
    }

    // A mono layout is held too.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        juce::AudioProcessor::BusesLayout mono;
        mono.inputBuses.add (juce::AudioChannelSet::mono());
        mono.outputBuses.add (juce::AudioChannelSet::mono());
        const bool accepted = plugin.isBusesLayoutSupported (mono) && plugin.setBusesLayout (mono);
        plugin.setKnob (Knob::movement, 0.0f);
        plugin.prepareToPlay (rate, 256);
        juce::AudioBuffer<float> buffer (1, 256), out (1, (int) (3.0 * rate / 256) * 256);
        juce::MidiBuffer midi;
        const int captured = captureSample (0.5, rate, 256);

        for (int pos = 0; pos < out.getNumSamples(); pos += 256)
        {
            if (pos == captured)
                plugin.setFreeze (true);

            for (int i = 0; i < 256; ++i)
                buffer.setSample (0, i, pos + i < captured ? steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * (pos + i) / rate) : 0.0f);

            plugin.processBlock (buffer, midi);
            out.copyFrom (0, pos, buffer, 0, 0, 256);
        }

        check (accepted && near (sineDb (out, captured + (int) rate, out.getNumSamples() - 1000), steadyDb, 0.5), "spectral freeze: a mono track holds at -12 dBFS too");
    }

    // The test chord: captured 0.25 s in, held after it has died away.
    {
        const double rate = 48000.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        plugin.prepareToPlay (rate, 256);
        plugin.playTestChord();
        const int blocks = (int) (3.0 * rate / 256);
        juce::AudioBuffer<float> out (2, blocks * 256), buffer (2, 256);
        juce::MidiBuffer midi;
        int capturedBlock = -1;

        for (int b = 0; b < blocks; ++b)
        {
            buffer.clear();
            plugin.processBlock (buffer, midi);

            if (capturedBlock < 0 && plugin.getChordCaptures() > 0)
                capturedBlock = b;

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, b * 256, buffer, ch, 0, 256);
        }

        sfz::Snapshot snap;
        const bool got = plugin.getSnapshot (snap);
        const double start = meanDb (out, (int) (0.05 * rate), (int) (0.1 * rate));
        const double late = peakDb (out, (int) (2.5 * rate), out.getNumSamples());
        check (plugin.getChordCaptures() == 1 && capturedBlock == (int) (0.25 * rate) / 256 && plugin.isHolding() && got && snap.isHeld(),
               "spectral freeze: the test chord is captured once, 0.25 s in, and held");
        check (got && sfz::getSnapshotFlatness (snap, 0) < 0.05 && late > -40.0 && start > -40.0, "spectral freeze: the chord's hold is made of notes (flatness " + juce::String (got ? sfz::getSnapshotFlatness (snap, 0) : 1.0, 4) + ") and sounds on after the chord has gone");

        float peak = 0.0f;
        std::vector<float> ringCopy (4096);
        plugin.readInput (0, ringCopy.data(), 4096);

        for (auto v : ringCopy)
            peak = juce::jmax (peak, std::abs (v));

        check (peak < 0.01f, "spectral freeze: the live input can be read without consuming it");
    }

    // A white noise snapshot is flat, a chord's is not.
    {
        const double rate = 44100.0;
        BuiltInSpectralFreeze plugin;
        juce::Random random (5);
        const auto held = holdSignal (plugin, rate, 1.5, [&] (int, int) { return (random.nextFloat() * 2.0f - 1.0f) * 0.2f; });
        juce::ignoreUnused (held);
        sfz::Snapshot snap;
        check (plugin.getSnapshot (snap) && sfz::getSnapshotFlatness (snap, 0) > 0.6, "spectral freeze: the hold of white noise has a spectral flatness above 0.6");
    }

    // Band-limited noise is noise too: a cymbal-like wash (2 to 8 kHz) and a breath-like pink noise.
    {
        const double rate = 44100.0;
        auto flatnessOf = [&] (auto&& source)
        {
            BuiltInSpectralFreeze plugin;
            holdSignal (plugin, rate, 1.5, source);
            sfz::Snapshot snap;
            return plugin.getSnapshot (snap) ? sfz::getSnapshotFlatness (snap, 0) : -1.0;
        };

        juce::Random random (9);
        juce::dsp::IIR::Filter<float> high, low, high2, low2;
        high.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (rate, 2000.0f);
        high2.coefficients = high.coefficients;
        low.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (rate, 8000.0f);
        low2.coefficients = low.coefficients;
        const double cymbal = flatnessOf ([&] (int, int) { return 0.3f * low2.processSample (low.processSample (high2.processSample (high.processSample (random.nextFloat() * 2.0f - 1.0f)))); });

        float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        const double pink = flatnessOf ([&] (int, int)
        {
            const float white = random.nextFloat() * 2.0f - 1.0f;
            b0 = 0.99886f * b0 + white * 0.0555179f; b1 = 0.99332f * b1 + white * 0.0750759f; b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f; b4 = 0.55000f * b4 + white * 0.5329522f; b5 = -0.7616f * b5 - white * 0.0168980f;
            const float out = 0.05f * (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f);
            b6 = white * 0.115926f;
            return out;
        });
        check (cymbal >= 0.3 && pink >= 0.3, "spectral freeze: band-limited (2 to 8 kHz) and pink noise holds count as noise (flatness " + juce::String (cymbal, 3) + ", " + juce::String (pink, 3) + ")");
    }

    // Saved with the plugin's state: the hold is in it only while holding, and sounds the same after loading.
    {
        const double rate = 44100.0;
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (Knob::movement, 0.0f);
        const auto held = holdSignal (plugin, rate, 1.5, [&] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / rate); });
        juce::ignoreUnused (held);
        juce::MemoryBlock holding;
        plugin.getStateInformation (holding);
        plugin.setFreeze (false);
        juce::MemoryBlock idle;
        plugin.getStateInformation (idle);
        auto xmlHolding = juce::AudioProcessor::getXmlFromBinary (holding.getData(), (int) holding.getSize());
        auto xmlIdle = juce::AudioProcessor::getXmlFromBinary (idle.getData(), (int) idle.getSize());
        check (xmlHolding != nullptr && xmlHolding->getChildByName ("Hold") != nullptr && xmlIdle != nullptr && xmlIdle->getChildByName ("Hold") == nullptr && idle.getSize() < 2000
                   && holding.getSize() < 200000,
               "spectral freeze: the state holds the snapshot (" + juce::String ((int) holding.getSize() / 1024) + " KB) only while Freeze is on");

        for (const bool prepareFirst : { false, true })
        {
            BuiltInSpectralFreeze restored;

            if (prepareFirst)
                restored.prepareToPlay (rate, 512);

            restored.setStateInformation (holding.getData(), (int) holding.getSize());
            const auto out = drive (restored, rate, (int) (3.0 * rate), [] (int, int) { return 0.0f; }, 512, ! prepareFirst);
            const int from = (int) (1.0 * rate), to = (int) (2.9 * rate);
            check (restored.getSettings().freeze && near (sineDb (out, from, to), steadyDb, 0.5) && near (measureHz (out, from, to, rate), 1000.0, 1.0),
                   juce::String ("spectral freeze: a loaded state with Freeze on and a snapshot sounds the same (") + (prepareFirst ? "prepared first" : "prepared after") + ", " + juce::String (sineDb (out, from, to), 2) + " dBFS)");
        }

        // Freeze on without a snapshot (a host that dropped it) holds silence.
        BuiltInSpectralFreeze dropped;
        dropped.setFreeze (true);
        const auto silent = drive (dropped, rate, (int) (2.0 * rate), [] (int, int) { return 0.0f; });
        check (peakDb (silent, 0, silent.getNumSamples()) < -100.0, "spectral freeze: Freeze on without a snapshot holds silence");
    }

    checkThroughProject<BuiltInSpectralFreeze> (BuiltInSpectralFreeze::getDescription(), "DawIt Spectral Freeze",
                                                [] (BuiltInSpectralFreeze& p)
                                                {
                                                    p.setKnob (Knob::movement, 0.0f);
                                                    p.setKnob (Knob::brightness, 2.5f);
                                                    p.setKnob (Knob::fade, 1500.0f);
                                                    holdSignal (p, 44100.0, 1.5, [] (int, int i) { return steadyAmplitude() * (float) std::sin (twoPi * 1000.0 * i / 44100.0); });
                                                },
                                                [&] (BuiltInSpectralFreeze& p)
                                                {
                                                    const auto s = p.getSettings();
                                                    p.setKnob (Knob::brightness, 0.0f);
                                                    p.setKnob (Knob::fade, 300.0f);
                                                    const auto out = drive (p, 44100.0, 3 * 44100, [] (int, int) { return 0.0f; });
                                                    return s.freeze && near (s.brightnessDb, 2.5, 0.01) && near (s.fadeMs, 1500.0, 1.0) && near (sineDb (out, 44100, 3 * 44100 - 4410), steadyDb, 0.5);
                                                });
}

static void runSpectralFreezeAnalysisTests()
{
    using namespace daw;
    std::cout << "Spectral freeze advice" << std::endl;

    auto minus = [] { return juce::String::fromUTF8 ("\xe2\x88\x92"); };
    auto holding = [] (double holdDb, double priorDb, double flatness, double peakHz, float movement)
    {
        sfz::AnalysisInput in;
        in.holding = true;
        in.holdDb = holdDb;
        in.priorPeakDb = priorDb;
        in.flatness = flatness;
        in.peakHz = peakHz;
        in.movementPercent = movement;
        return in;
    };
    auto has = [] (const std::vector<juce::String>& lines, const juce::String& start)
    {
        return std::any_of (lines.begin(), lines.end(), [&] (auto& l) { return l.startsWith (start); });
    };

    // Z1: nothing held.
    {
        sfz::AnalysisInput idle;
        const auto lines = sfz::analyse (idle);
        check (lines.size() == 1 && lines[0] == "Nothing is held. Play your track and click Freeze while the sound you want rings.", "spectral freeze advice: Z1 says nothing is held, and only that");
        check (! has (sfz::analyse (holding (-30.0, -20.0, 0.05, 220.0, 20.0f)), "Nothing is held"), "spectral freeze advice: Z1 is quiet while holding");
    }

    // Z2: a hold below -60 dBFS.
    {
        const auto lines = sfz::analyse (holding (-61.0, -20.0, 0.05, 220.0, 20.0f));
        check (has (lines, "The hold is almost silent (" + minus() + "61 dBFS)") && lines.size() == 1 && ! has (sfz::analyse (holding (-59.0, -59.0, 0.05, 220.0, 20.0f)), "The hold is almost silent"),
               "spectral freeze advice: Z2 fires at -61 dBFS and not at -59");
    }

    // Z3: 10 dB or more below the loudest moment before.
    {
        const auto fires = sfz::analyse (holding (-30.0, -19.0, 0.2, 220.0, 20.0f));
        check (has (fires, "You caught the sound as it faded: the hold is 11 dB quieter than the loudest moment just before.") && ! has (sfz::analyse (holding (-30.0, -21.0, 0.2, 220.0, 20.0f)), "You caught the sound"),
               "spectral freeze advice: Z3 fires at 11 dB below the loudest moment and not at 9");
        check (! has (sfz::analyse (holding (-65.0, -20.0, 0.2, 220.0, 20.0f)), "You caught the sound"), "spectral freeze advice: Z3 gives way to Z2");
    }

    // Z4: noise with little Movement.
    {
        const auto fires = sfz::analyse (holding (-20.0, -20.0, 0.35, 0.0, 10.0f));
        check (has (fires, "Your hold is mostly noise (like a breath, a cymbal or a drum). At Movement 10% it can sound static or mechanical: try 30 to 60%.")
                   && ! has (sfz::analyse (holding (-20.0, -20.0, 0.25, 0.0, 10.0f)), "Your hold is mostly noise") && ! has (sfz::analyse (holding (-20.0, -20.0, 0.35, 0.0, 25.0f)), "static or mechanical"),
               "spectral freeze advice: Z4 fires at flatness 0.35 with Movement 10% and not at flatness 0.25 or Movement 25%");
    }

    // Z4 also speaks for noise with enough Movement.
    {
        const auto lines = sfz::analyse (holding (-20.0, -20.0, 0.35, 0.0, 25.0f));
        check (has (lines, "Your hold is mostly noise") && lines.size() == 1 && lines[0].contains ("At Movement 25% its levels and phases keep wandering") && ! lines[0].contains ("static or mechanical"),
               "spectral freeze advice: a noisy hold with Movement 25% still gets a line about what it is made of");
    }

    // Z5 to Z7: a tone, and the song's key.
    {
        auto withKey = [&] (double hz)
        {
            auto in = holding (-20.0, -20.0, 0.05, hz, 20.0f);
            in.keyKnown = true;
            in.keyName = "A minor";
            in.keyPitchClasses = (1 << 9) | (1 << 11) | (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7);
            return in;
        };

        const auto noKey = sfz::analyse (holding (-20.0, -20.0, 0.05, 220.0, 20.0f));
        check (has (noKey, "Your hold is mostly notes. Its tallest peak is 220 Hz, nearest A3 (0 cents); it may be a harmonic of the note played.") && noKey.size() == 1,
               "spectral freeze advice: Z5 names 220 Hz as A3, and says nothing about a key without one");
        check (has (sfz::analyse (holding (-20.0, -20.0, 0.05, 226.0, 20.0f)), "Your hold is mostly notes. Its tallest peak is 226 Hz, nearest A3 (+") && ! has (sfz::analyse (holding (-20.0, -20.0, 0.1, 220.0, 20.0f)), "Your hold is mostly notes"),
               "spectral freeze advice: Z5 gives the cents and is quiet from flatness 0.1");
        check (has (sfz::analyse (withKey (220.0)), "A is in the song's key (A minor), so the hold can stay under most chords as a drone.")
                   && has (sfz::analyse (withKey (233.08)), "A# is not in the song's key (A minor): it will clash with some chords. Capture a note from the key, or hold it only where it fits."),
               "spectral freeze advice: Z6 for A3 and Z7 for A#3 in A minor");
    }

    // Z8: notes and noise.
    {
        check (has (sfz::analyse (holding (-18.0, -18.0, 0.2, 220.0, 20.0f)), "Your hold is a mix of notes and noise (" + minus() + "18 dBFS).")
                   && ! has (sfz::analyse (holding (-18.0, -18.0, 0.05, 220.0, 20.0f)), "Your hold is a mix") && ! has (sfz::analyse (holding (-18.0, -18.0, 0.3, 220.0, 50.0f)), "Your hold is a mix"),
               "spectral freeze advice: Z8 for flatness from 0.1 to 0.3");
    }

    // The same from a real hold: the test chord's hold is notes, the noise one is not.
    {
        BuiltInSpectralFreeze plugin;
        plugin.setKnob (sfz::Knob::movement, 0.0f);
        sfztests::holdSignal (plugin, 48000.0, 1.0, [] (int, int i) { return 0.2f * (float) std::sin (sfztests::twoPi * 220.0 * i / 48000.0); });
        sfz::Snapshot snap;
        check (plugin.getSnapshot (snap) && std::abs (sfz::getSnapshotPeakHz (snap) - 220.0) < 0.5 && std::abs (20.0 * std::log10 (sfz::getSnapshotRms (snap) / 0.2 * std::sqrt (2.0))) < 0.3,
               "spectral freeze advice: a held 220 Hz sine has its tallest peak at 220 Hz and an RMS of 0.14");
    }
}
