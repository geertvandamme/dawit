#include "BuiltInFm.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace fm
{
    namespace
    {
        constexpr double pi = juce::MathConstants<double>::pi, twoPi = juce::MathConstants<double>::twoPi;

        /** A sine as a table, read between its points: the voices need four a sample at twice the rate. */
        constexpr int tableSize = 4096;

        const std::array<double, tableSize + 1>& getTable()
        {
            static const auto table = []
            {
                std::array<double, tableSize + 1> t {};

                for (int i = 0; i <= tableSize; ++i)
                    t[(size_t) i] = std::sin (twoPi * i / tableSize);

                return t;
            }();

            return table;
        }

        /** sin (2 pi x), with x in cycles. */
        inline double sine (double cycles) noexcept
        {
            static const auto& table = getTable();
            const double x = (cycles - std::floor (cycles)) * tableSize;
            const int i = juce::jlimit (0, tableSize - 1, (int) x);
            return table[(size_t) i] + (table[(size_t) i + 1] - table[(size_t) i]) * (x - i);
        }

        juce::String formatTime (double seconds)
        {
            if (seconds < 0.0095)
                return juce::String (seconds * 1000.0, 1) + " ms";

            return seconds < 1.0 ? juce::String (juce::roundToInt (seconds * 1000.0)) + " ms" : juce::String (seconds, 2) + " s";
        }

        juce::String formatPercent (float value)
        {
            return juce::String (juce::roundToInt (value)) + "%";
        }

        juce::String withSign (double value, int decimals)
        {
            return (value > 0.0 ? "+" : "") + fx::formatNumber (value, decimals);
        }

        juce::String times()
        {
            return juce::String (juce::CharPointer_UTF8 ("\xc3\x97"));
        }
    }

    /** One sample of the operators, from 4 down to 1: each one's phase (in cycles) and amplitude, how much
        operator 4 feeds back and its last two outputs (averaged, as on a DX7, so it doesn't run away); the
        carriers added up. */
    static double runOperators (const Algorithm& a, const std::array<double, numOperators>& phases, const std::array<double, numOperators>& amps,
                                double feedback, double& last1, double& last2) noexcept
    {
        std::array<double, numOperators> outs {};
        double sum = 0.0;

        for (int op = numOperators - 1; op >= 0; --op)
        {
            double modulation = 0.0;

            for (int other = op + 1; other < numOperators; ++other)
                if ((a.modulatedBy[(size_t) op] & (1 << other)) != 0)
                    modulation += outs[(size_t) other];

            if (op == numOperators - 1)
                modulation += feedback * 0.5 * (last1 + last2);

            const double raw = sine (phases[(size_t) op] + modulation / twoPi);

            if (op == numOperators - 1)
            {
                last2 = last1;
                last1 = raw;
            }

            outs[(size_t) op] = amps[(size_t) op] * raw;

            if ((a.carriers & (1 << op)) != 0)
                sum += outs[(size_t) op];
        }

        return sum;
    }

    Settings::Settings()
    {
        ops[0].levelPercent = 100.0f;
    }

    bool isOperatorKnob (Knob k)
    {
        return (int) k < numOperatorKnobs;
    }

    juce::String getLfoTargetName (LfoTarget t)
    {
        switch (t)
        {
            case LfoTarget::pitch:      return "Pitch";
            case LfoTarget::brightness: return "Brightness";
            case LfoTarget::volume:     return "Volume";
        }

        return {};
    }

    //==============================================================================
    const Algorithm& getAlgorithm (int index)
    {
        static const std::array<Algorithm, numAlgorithms> list = []
        {
            auto wiring = [] (const char* text) { return juce::String (text).replace (">", juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x92"))); };

            return std::array<Algorithm, numAlgorithms> {{
                { "Stack", wiring ("4 > 3 > 2 > 1"), { 2, 4, 8, 0 }, 1 },
                { "Two into a chain", wiring ("3 + 4 > 2 > 1"), { 2, 4 | 8, 0, 0 }, 1 },
                { "Chain and branch", wiring ("4 > 3 > 1, 2 > 1"), { 2 | 4, 0, 8, 0 }, 1 },
                { "Three into one", wiring ("2 + 3 + 4 > 1"), { 2 | 4 | 8, 0, 0, 0 }, 1 },
                { "Two pairs", wiring ("2 > 1, 4 > 3"), { 2, 0, 8, 0 }, 1 | 4 },
                { "One into three", wiring ("4 > 1, 2 and 3"), { 8, 8, 8, 0 }, 1 | 2 | 4 },
                { "A pair and two sines", wiring ("4 > 3, and 1 and 2"), { 0, 0, 8, 0 }, 1 | 2 | 4 },
                { "Four sines", "1, 2, 3 and 4, nothing modulated", { 0, 0, 0, 0 }, 1 | 2 | 4 | 8 } }};
        }();

        return list[(size_t) juce::jlimit (0, numAlgorithms - 1, index)];
    }

    bool isCarrier (const Settings& s, int op)
    {
        return (getAlgorithm (s.algorithm).carriers & (1 << op)) != 0;
    }

    int getTargets (const Settings& s, int op)
    {
        const auto& a = getAlgorithm (s.algorithm);
        int targets = 0;

        for (int other = 0; other < numOperators; ++other)
            if ((a.modulatedBy[(size_t) other] & (1 << op)) != 0)
                targets |= 1 << other;

        return targets;
    }

    double getCoarseRatio (int coarse)
    {
        return coarse <= 0 ? 0.5 : (double) juce::jmin (coarse, maxCoarse);
    }

    double getRatio (const Operator& o)
    {
        return getCoarseRatio (o.coarse) * (1.0 + juce::jlimit (0.0f, maxFine, o.finePercent) / 100.0);
    }

    float getVelocityScale (const Settings& s, float velocity)
    {
        return 1.0f - juce::jlimit (0.0f, 1.0f, s.velocityPercent / 100.0f) * (1.0f - juce::jlimit (0.0f, 1.0f, velocity));
    }

    double getNormalisation (const Settings& s)
    {
        double power = 0.0;

        for (int op = 0; op < numOperators; ++op)
            if (isCarrier (s, op))
                power += std::pow (s.ops[(size_t) op].levelPercent / 100.0, 2.0);

        return power > 1.0 ? 1.0 / std::sqrt (power) : 1.0;
    }

    double getIndex (const Settings& s, int op, float velocity, float lfo)
    {
        if (isCarrier (s, op))
            return 0.0;

        double index = s.ops[(size_t) op].levelPercent / 100.0 * maxIndex * getVelocityScale (s, velocity);

        if (s.lfoTarget == LfoTarget::brightness)
            index *= juce::jmax (0.0, 1.0 + s.lfoDepthPercent / 100.0 * lfo);

        return index;
    }

    double getAmplitude (const Settings& s, int op, float velocity, float lfo)
    {
        if (! isCarrier (s, op))
            return getIndex (s, op, velocity, lfo);

        double amp = s.ops[(size_t) op].levelPercent / 100.0 * getNormalisation (s) * getVelocityScale (s, velocity);

        if (s.lfoTarget == LfoTarget::volume)
            amp *= 1.0 - s.lfoDepthPercent / 100.0 * (0.5 - 0.5 * lfo);

        return amp;
    }

    std::array<float, numOperators> getEnvelopes (const Settings& s, double seconds, double releasedAt)
    {
        std::array<float, numOperators> levels;

        for (int op = 0; op < numOperators; ++op)
            levels[(size_t) op] = seconds < 0.0 ? 1.0f : sub::getEnvelopeLevel (s.ops[(size_t) op].env, seconds, releasedAt);

        return levels;
    }

    //==============================================================================
    double getBessel (int n, double x)
    {
        if (n < 0)
            return (n % 2 == 0 ? 1.0 : -1.0) * getBessel (-n, x);

        // Bessel's integral, J_n(x) = 1/pi times the integral from 0 to pi of cos (n t - x sin t); smooth and periodic, so a sum of points is exact enough.
        constexpr int steps = 400;
        double sum = 0.0;

        for (int i = 0; i <= steps; ++i)
        {
            const double t = pi * i / steps;
            sum += (i == 0 || i == steps ? 0.5 : 1.0) * std::cos (n * t - x * std::sin (t));
        }

        return sum / steps;
    }

    double getCarsonHz (double index, double modulatorHz)
    {
        return 2.0 * (index + 1.0) * modulatorHz;
    }

    std::vector<Peak> getSidebands (double carrierRatio, double modulatorRatio, double index)
    {
        std::vector<Peak> peaks;
        const int reach = (int) index + 8;

        for (int k = -reach; k <= reach; ++k)
        {
            double ratio = carrierRatio + k * modulatorRatio;
            double amplitude = getBessel (k, index);

            // A sideband below 0 Hz is heard at the same frequency above 0, upside down: sin (-x) = -sin (x).
            if (ratio < 0.0)
            {
                ratio = -ratio;
                amplitude = -amplitude;
            }

            auto same = std::find_if (peaks.begin(), peaks.end(), [ratio] (const Peak& p) { return std::abs (p.hz - ratio) < 1.0e-9; });

            if (same != peaks.end())
                same->amplitude += amplitude;
            else
                peaks.push_back ({ ratio, amplitude });
        }

        std::vector<Peak> heard;

        for (auto& p : peaks)
            if (std::abs (p.amplitude) > 1.0e-4 && p.hz > 1.0e-9)
                heard.push_back ({ p.hz, std::abs (p.amplitude) });

        std::sort (heard.begin(), heard.end(), [] (const Peak& a, const Peak& b) { return a.hz < b.hz; });
        return heard;
    }

    std::vector<float> render (const Settings& s, double noteHz, const std::array<float, numOperators>& envelopes, double sampleRate,
                               int numSamples, float velocity, bool modulated)
    {
        const auto& a = getAlgorithm (s.algorithm);
        std::array<double, numOperators> phases {}, amps {}, steps {};

        for (int op = 0; op < numOperators; ++op)
        {
            const double hz = noteHz * getRatio (s.ops[(size_t) op]);
            steps[(size_t) op] = hz / sampleRate;
            amps[(size_t) op] = hz < 0.45 * sampleRate && (modulated || isCarrier (s, op)) ? getAmplitude (s, op, velocity) * envelopes[(size_t) op] : 0.0;
        }

        const double feedback = modulated ? s.feedbackPercent / 100.0 * maxFeedback : 0.0;
        double last1 = 0.0, last2 = 0.0;
        std::vector<float> wave ((size_t) juce::jmax (0, numSamples));

        for (auto& v : wave)
        {
            v = (float) runOperators (a, phases, amps, feedback, last1, last2);

            for (int op = 0; op < numOperators; ++op)
                phases[(size_t) op] += steps[(size_t) op];
        }

        return wave;
    }

    std::vector<Peak> getSpectrum (const Settings& s, double noteHz, const std::array<float, numOperators>& envelopes, float velocity, double maxHz)
    {
        constexpr int order = 13, size = 1 << order;

        // A rate that puts the note's harmonics exactly on the analysis's bins, high enough to hold maxHz.
        int perHarmonic = 32;

        while (perHarmonic > 1 && noteHz * size / perHarmonic < 2.2 * maxHz)
            perHarmonic /= 2;

        const double sampleRate = noteHz * size / perHarmonic;
        const auto wave = render (s, noteHz, envelopes, sampleRate, size, velocity);

        // A Blackman-Harris window: its side lobes are under -92 dB, so a loud partial doesn't make quiet false ones next to it.
        constexpr double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;
        std::vector<float> data ((size_t) size * 2, 0.0f);

        for (int i = 0; i < size; ++i)
        {
            const double x = twoPi * i / (size - 1);
            data[(size_t) i] = wave[(size_t) i] * (float) (a0 - a1 * std::cos (x) + a2 * std::cos (2.0 * x) - a3 * std::cos (3.0 * x));
        }

        juce::dsp::FFT fft (order);
        fft.performFrequencyOnlyForwardTransform (data.data());

        auto db = [&data] (int bin) { return juce::Decibels::gainToDecibels (2.0 * data[(size_t) bin] / (size * a0), -200.0); };
        std::vector<Peak> peaks;
        const int top = juce::jmin (size / 2 - 2, (int) (maxHz / sampleRate * size));

        for (int bin = 2; bin <= top; ++bin)
        {
            const double here = db (bin), before = db (bin - 1), after = db (bin + 1);

            if (here < -70.0 || here <= before || here < after)
                continue;

            // The true top between the bins, from a parabola through the three.
            const double curve = before - 2.0 * here + after;
            const double shift = std::abs (curve) > 1.0e-12 ? 0.5 * (before - after) / curve : 0.0;
            peaks.push_back ({ (bin + shift) * sampleRate / size, juce::Decibels::decibelsToGain (here - 0.25 * (before - after) * shift) });
        }

        return peaks;
    }

    //==============================================================================
    juce::String getKnobName (Knob k)
    {
        switch (k)
        {
            case Knob::coarse:    return "Ratio";
            case Knob::fine:      return "Fine";
            case Knob::level:     return "Level";
            case Knob::attack:    return "Attack";
            case Knob::decay:     return "Decay";
            case Knob::sustain:   return "Sustain";
            case Knob::release:   return "Release";
            case Knob::algorithm: return "Algorithm";
            case Knob::feedback:  return "Feedback";
            case Knob::lfoRate:   return "Rate";
            case Knob::lfoDepth:  return "Depth";
            case Knob::lfoTarget: return "To";
            case Knob::glide:     return "Glide";
            case Knob::mode:      return "Voices";
            case Knob::velocity:  return "Velocity";
            case Knob::output:    return "Output";
        }

        return {};
    }

    juce::String formatRatio (double ratio)
    {
        if (std::abs (ratio - std::round (ratio)) < 0.001)
            return juce::String ((int) std::round (ratio));

        return juce::String (ratio, 2);
    }

    juce::String formatKnob (Knob k, const Settings& s, int op)
    {
        const auto& o = s.ops[(size_t) juce::jlimit (0, numOperators - 1, op)];

        switch (k)
        {
            case Knob::coarse:    return times() + formatRatio (getCoarseRatio (o.coarse));
            case Knob::fine:      return o.finePercent < 0.5f ? juce::String ("0%") : "+" + formatPercent (o.finePercent);
            case Knob::level:     return formatPercent (o.levelPercent);
            case Knob::attack:    return formatTime (o.env.attack);
            case Knob::decay:     return formatTime (o.env.decay);
            case Knob::sustain:   return formatPercent (o.env.sustain * 100.0f);
            case Knob::release:   return formatTime (o.env.release);
            case Knob::algorithm: return getAlgorithm (s.algorithm).name;
            case Knob::feedback:  return s.feedbackPercent < 0.5f ? juce::String ("Off") : formatPercent (s.feedbackPercent);
            case Knob::lfoRate:   return juce::String (s.lfoRateHz, s.lfoRateHz < 1.0f ? 2 : 1) + " Hz";
            case Knob::lfoDepth:  return formatPercent (s.lfoDepthPercent);
            case Knob::lfoTarget: return getLfoTargetName (s.lfoTarget);
            case Knob::glide:     return s.glideSeconds < 0.0005f ? juce::String ("Off") : formatTime (s.glideSeconds);
            case Knob::mode:      return s.mode == sub::Mode::mono ? "Mono" : "Poly";
            case Knob::velocity:  return formatPercent (s.velocityPercent);
            case Knob::output:    return withSign (s.outputDb, 1) + " dB";
        }

        return {};
    }

    //==============================================================================
    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            auto add = [&list] (const juce::String& name, const juce::String& why, std::array<const char*, numOperators> operators, Settings s,
                                std::initializer_list<std::pair<Knob, const char*>> notes)
            {
                Preset p;
                p.name = name;
                p.why = why;
                p.settings = s;

                for (size_t i = 0; i < operators.size(); ++i)
                    p.operators[i] = juce::CharPointer_UTF8 (operators[i]);

                for (auto& [knob, note] : notes)
                    p.notes[(size_t) knob] = juce::CharPointer_UTF8 (note);

                list.push_back (p);
            };

            auto set = [] (Settings& s, int op, int coarse, float fine, float level, sub::Envelope env)
            {
                auto& o = s.ops[(size_t) op - 1];
                o.coarse = coarse;
                o.finePercent = fine;
                o.levelPercent = level;
                o.env = env;
            };

            add ("Init: one sine", "One carrier, operator 1, playing a plain sine: operator 2 is wired to modulate it, but its level is 0. "
                                   "Turn operator 2's level up slowly and hear the sine turn bright and buzzy: that's FM.",
                 { "The carrier: a sine at the note, at full level.", "A modulator at the note's frequency (ratio 1), at 0, so it does nothing yet. "
                   "At 10% its index is 1; at 20%, 2.", "", "" },
                 {}, { { Knob::algorithm, "Stack: 4 modulates 3, 3 modulates 2, 2 modulates 1. With 3 and 4 at 0, only 2 can change the sound." } });

            Settings basic;
            set (basic, 2, 1, 0.0f, 20.0f, {});
            add ("Two operators, 1 : 1", "The simplest FM sound: one sine modulating another at the same frequency. The sidebands land on every "
                                         "harmonic of the note, falling away above the index, so it sounds like a saw played through a filter.",
                 { "The carrier, at the note.", "The modulator, also at the note (1 : 1), at an index of 2: the 2nd and 3rd harmonics are strong, "
                   "the 5th already 20 dB down. Turn its level and hear the brightness follow.", "", "" },
                 basic, { { Knob::algorithm, "Stack, with only operators 1 and 2 on." } });

            Settings keys;
            keys.algorithm = 4;
            set (keys, 1, 1, 0.0f, 85.0f, { 0.002f, 3.5f, 0.0f, 0.5f });
            set (keys, 2, 1, 0.0f, 18.0f, { 0.002f, 1.2f, 0.1f, 0.4f });
            set (keys, 3, 1, 0.0f, 45.0f, { 0.001f, 0.8f, 0.0f, 0.3f });
            set (keys, 4, 14, 0.0f, 32.0f, { 0.001f, 0.25f, 0.0f, 0.1f });
            keys.velocityPercent = 70.0f;
            keys.outputDb = -2.0f;
            add ("Electric piano", "The sound that made the DX7 famous in 1983: two pairs, one for the round body of the note and one for the "
                                   "metallic \"tine\" at its start. Hit it softly and it's mellow, hard and it rings.",
                 { "The body's carrier, dying away over 3.5 seconds like a struck note.",
                   "The body's modulator at 1 : 1, index 1.8 falling to almost nothing in 1.2 s: bright at the start, rounder as it rings.",
                   "The tine's carrier, short.",
                   "The tine's modulator at 14 times the note, gone in a quarter of a second: the clang of the metal at the moment of the hit." },
                 keys, { { Knob::algorithm, "Two pairs: 2 modulates 1, 4 modulates 3, and both 1 and 3 are heard." },
                         { Knob::velocity, "70%: softer notes are quieter and, because the modulators turn down too, duller." } });

            Settings bass;
            set (bass, 1, 1, 0.0f, 100.0f, { 0.002f, 2.0f, 0.6f, 0.08f });
            set (bass, 2, 1, 0.0f, 40.0f, { 0.001f, 0.25f, 0.25f, 0.08f });
            set (bass, 3, 3, 0.0f, 15.0f, { 0.001f, 0.08f, 0.0f, 0.05f });
            bass.mode = sub::Mode::mono;
            bass.glideSeconds = 0.04f;
            bass.velocityPercent = 60.0f;
            add ("FM bass", "A punchy synth bass: the index drops fast after the attack, so every note starts with a bright \"pluck\" and "
                            "settles into a round tone, the way a filter envelope does on a subtractive synth.",
                 { "The carrier, holding at 60% while the key is down.",
                   "The main modulator at 1 : 1: an index of 4 at the attack, falling to 1 in a quarter of a second.",
                   "A short burst at 3 times the note into operator 2, for the click of the attack.", "" },
                 bass, { { Knob::mode, "Mono: a bass plays one note at a time." },
                         { Knob::glide, "40 ms: legato notes slide." } });

            Settings bell;
            set (bell, 1, 1, 0.0f, 100.0f, { 0.001f, 12.0f, 0.0f, 6.0f });
            set (bell, 2, 1, 40.0f, 100.0f, { 0.001f, 9.0f, 0.0f, 6.0f });
            bell.velocityPercent = 40.0f;
            bell.outputDb = -2.0f;
            add ("Chowning's bell", "The bell from John Chowning's 1973 paper that started it all: a modulator at 1.4 times the carrier, so "
                                    "the sidebands fall between the harmonics and the sound rings like metal, not like a string.",
                 { "The carrier, ringing for 12 seconds.", "The modulator at 1.4 times the note (ratio 1, fine +40%), at an index of 10 that "
                   "falls with the note: the clang at the strike fades into a purer hum.", "", "" },
                 bell, {});

            Settings brass;
            set (brass, 1, 1, 0.0f, 100.0f, { 0.06f, 0.2f, 0.85f, 0.15f });
            set (brass, 2, 1, 0.0f, 35.0f, { 0.08f, 0.2f, 0.7f, 0.15f });
            brass.lfoRateHz = 5.0f;
            brass.lfoDepthPercent = 4.0f;
            brass.velocityPercent = 60.0f;
            add ("Brass", "Chowning's brass: in a trumpet the sound gets brighter as it gets louder, so the modulator's envelope follows the "
                          "carrier's. Each note blooms from dark to bright as it starts.",
                 { "The carrier, rising in 60 ms.", "The modulator at 1 : 1, its index rising to 3.5 a little slower than the note, then settling at 2.5.", "", "" },
                 brass, { { Knob::lfoDepth, "4%: a little vibrato, as a player adds." },
                          { Knob::velocity, "60%: harder notes are louder and brighter, as when a player blows harder." } });

            Settings reed;
            set (reed, 1, 1, 0.0f, 100.0f, { 0.04f, 0.2f, 0.9f, 0.1f });
            set (reed, 2, 2, 0.0f, 18.0f, { 0.05f, 0.3f, 0.8f, 0.1f });
            reed.lfoDepthPercent = 3.0f;
            reed.mode = sub::Mode::mono;
            reed.glideSeconds = 0.04f;
            add ("Hollow reed, 1 : 2", "A clarinet-like sound from a single trick: with the modulator at twice the carrier, every sideband lands "
                                       "on an odd harmonic (1, 3, 5...), the hollow sound of a closed tube.",
                 { "The carrier.", "The modulator at 2 times the note: sidebands at 1 + 2, 1 - 2 (folding back onto 1), 1 + 4... all odd.", "", "" },
                 reed, { { Knob::mode, "Mono with a 40 ms glide: a wind instrument plays one note at a time." } });

            Settings organ;
            organ.algorithm = 7;
            set (organ, 1, 1, 0.0f, 100.0f, { 0.003f, 0.05f, 1.0f, 0.03f });
            set (organ, 2, 2, 0.0f, 80.0f, { 0.003f, 0.05f, 1.0f, 0.03f });
            set (organ, 3, 3, 0.0f, 60.0f, { 0.003f, 0.05f, 1.0f, 0.03f });
            set (organ, 4, 4, 0.0f, 60.0f, { 0.003f, 0.05f, 1.0f, 0.03f });
            organ.feedbackPercent = 12.0f;
            organ.velocityPercent = 0.0f;
            organ.outputDb = -2.0f;
            add ("Organ from four sines", "No modulation at all: four carriers at the 1st to 4th harmonics, like four drawbars of an organ. FM "
                                          "synths can add sines too.",
                 { "The note itself, the 8' drawbar.", "The 2nd harmonic, an octave up (4').", "The 3rd, a fifth above that (2 2/3').",
                   "The 4th, two octaves up (2'), with a little feedback for some grit." },
                 organ, { { Knob::algorithm, "Four sines: every operator is a carrier." },
                          { Knob::feedback, "12%: operator 4 modulates itself a little, adding a few soft harmonics above it." },
                          { Knob::velocity, "0%: an organ plays every note as loud." } });

            Settings marimba;
            marimba.algorithm = 4;
            set (marimba, 1, 1, 0.0f, 100.0f, { 0.001f, 0.9f, 0.0f, 0.3f });
            set (marimba, 2, 1, 0.0f, 12.0f, { 0.001f, 0.12f, 0.0f, 0.1f });
            set (marimba, 3, 4, 0.0f, 35.0f, { 0.001f, 0.25f, 0.0f, 0.1f });
            marimba.velocityPercent = 70.0f;
            add ("Marimba", "A wooden bar: a marimba's bars are carved so their second mode rings two octaves up, at 4 times the note, and dies "
                            "away much faster than the note itself.",
                 { "The bar's note, dying in under a second.", "A soft, quick modulation for the knock of the mallet.",
                   "The bar's second mode, at 4 times the note, gone in a quarter of a second.", "" },
                 marimba, { { Knob::algorithm, "Two pairs, with operator 4 off: 3 is a plain sine." } });

            Settings pad;
            pad.algorithm = 4;
            set (pad, 1, 1, 0.0f, 80.0f, { 0.8f, 1.5f, 0.8f, 2.0f });
            set (pad, 2, 3, 0.0f, 12.0f, { 1.5f, 2.0f, 0.6f, 2.0f });
            set (pad, 3, 1, 1.0f, 70.0f, { 0.8f, 1.5f, 0.8f, 2.0f });
            set (pad, 4, 7, 0.0f, 8.0f, { 2.0f, 2.0f, 0.5f, 2.0f });
            pad.lfoRateHz = 0.3f;
            pad.lfoDepthPercent = 40.0f;
            pad.lfoTarget = LfoTarget::brightness;
            pad.velocityPercent = 30.0f;
            pad.outputDb = -4.0f;
            add ("Glass pad", "A slow, shimmering pad: two pairs a hair apart in pitch so they beat gently, with modulators that fade in after "
                              "the note and an LFO that makes the brightness breathe.",
                 { "The first carrier, swelling in over 0.8 s.", "Its modulator at 3 times the note, coming in slower still.",
                   "The second carrier at 1.01 times the note: at A3 2.2 Hz above operator 1, so the two beat slowly, like two players.",
                   "Its modulator at 7 times the note, for a glassy top." },
                 pad, { { Knob::lfoTarget, "Brightness: the LFO moves the modulators' levels." },
                        { Knob::lfoRate, "0.3 Hz: one slow breath every three seconds." },
                        { Knob::output, "Down 4 dB: chords add up." } });

            Settings lead;
            set (lead, 1, 1, 0.0f, 100.0f, { 0.005f, 0.3f, 0.9f, 0.15f });
            set (lead, 2, 1, 0.0f, 22.0f, { 0.005f, 0.5f, 0.7f, 0.15f });
            set (lead, 3, 1, 0.0f, 12.0f, { 0.005f, 0.5f, 0.8f, 0.15f });
            set (lead, 4, 1, 0.0f, 10.0f, { 0.005f, 0.5f, 0.8f, 0.15f });
            lead.feedbackPercent = 60.0f;
            lead.lfoRateHz = 5.5f;
            lead.lfoDepthPercent = 5.0f;
            lead.mode = sub::Mode::mono;
            lead.glideSeconds = 0.06f;
            add ("Feedback lead", "A bright, buzzy lead from a stack of four: operator 4 modulates itself into something like a saw, and each "
                                  "operator below passes it on, so the sound gets richer at every step.",
                 { "The carrier.", "Modulates the carrier at 1 : 1.", "Modulates operator 2.", "Modulates itself (feedback) and operator 3." },
                 lead, { { Knob::feedback, "60%: operator 4's sine has turned most of the way to a saw." },
                         { Knob::mode, "Mono with a 60 ms glide, for a lead that slides." },
                         { Knob::lfoDepth, "5%: vibrato." } });

            return list;
        }();

        return presets;
    }
} // namespace fm

//==============================================================================
using fm::Knob;

namespace
{
    juce::NormalisableRange<float> logRange (float low, float high)
    {
        return { low, high,
                 [] (float start, float end, float t) { return start * std::pow (end / start, t); },
                 [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); } };
    }

    /** Puts a knob's value into the settings, for operator `op` when it's one of an operator's. */
    void apply (fm::Settings& s, Knob k, int op, float value)
    {
        auto& o = s.ops[(size_t) op];

        switch (k)
        {
            case Knob::coarse:    o.coarse = juce::jlimit (0, fm::maxCoarse, juce::roundToInt (value)); break;
            case Knob::fine:      o.finePercent = value; break;
            case Knob::level:     o.levelPercent = value; break;
            case Knob::attack:    o.env.attack = value; break;
            case Knob::decay:     o.env.decay = value; break;
            case Knob::sustain:   o.env.sustain = value / 100.0f; break;
            case Knob::release:   o.env.release = value; break;
            case Knob::algorithm: s.algorithm = juce::jlimit (0, fm::numAlgorithms - 1, juce::roundToInt (value)); break;
            case Knob::feedback:  s.feedbackPercent = value; break;
            case Knob::lfoRate:   s.lfoRateHz = value; break;
            case Knob::lfoDepth:  s.lfoDepthPercent = value; break;
            case Knob::lfoTarget: s.lfoTarget = (fm::LfoTarget) juce::jlimit (0, fm::numLfoTargets - 1, juce::roundToInt (value)); break;
            case Knob::glide:     s.glideSeconds = value; break;
            case Knob::mode:      s.mode = value >= 0.5f ? sub::Mode::mono : sub::Mode::poly; break;
            case Knob::velocity:  s.velocityPercent = value; break;
            case Knob::output:    s.outputDb = value; break;
        }
    }

    /** A knob's value in the settings, as its parameter holds it. */
    float valueOf (const fm::Settings& s, Knob k, int op)
    {
        const auto& o = s.ops[(size_t) op];

        switch (k)
        {
            case Knob::coarse:    return (float) o.coarse;
            case Knob::fine:      return o.finePercent;
            case Knob::level:     return o.levelPercent;
            case Knob::attack:    return o.env.attack;
            case Knob::decay:     return o.env.decay;
            case Knob::sustain:   return o.env.sustain * 100.0f;
            case Knob::release:   return o.env.release;
            case Knob::algorithm: return (float) s.algorithm;
            case Knob::feedback:  return s.feedbackPercent;
            case Knob::lfoRate:   return s.lfoRateHz;
            case Knob::lfoDepth:  return s.lfoDepthPercent;
            case Knob::lfoTarget: return (float) (int) s.lfoTarget;
            case Knob::glide:     return s.glideSeconds;
            case Knob::mode:      return s.mode == sub::Mode::mono ? 1.0f : 0.0f;
            case Knob::velocity:  return s.velocityPercent;
            case Knob::output:    return s.outputDb;
        }

        return 0.0f;
    }

    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float value, int)
            {
                fm::Settings s;
                apply (s, k, 0, value);
                return fm::formatKnob (k, s, 0);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return 0.0f;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();

                if (k == Knob::coarse)
                    return v < 0.75f ? 0.0f : v;

                return text.containsIgnoreCase ("ms") ? v / 1000.0f : v;
            });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;

    /** The pitch, the LFO and the operators' levels move from one step to the next. */
    constexpr int stepSize = 16;
}

juce::String BuiltInFm::getParameterId (Knob k, int op)
{
    const juce::String prefix = "op" + juce::String (juce::jlimit (0, fm::numOperators - 1, op) + 1);

    switch (k)
    {
        case Knob::coarse:    return prefix + "Coarse";
        case Knob::fine:      return prefix + "Fine";
        case Knob::level:     return prefix + "Level";
        case Knob::attack:    return prefix + "Attack";
        case Knob::decay:     return prefix + "Decay";
        case Knob::sustain:   return prefix + "Sustain";
        case Knob::release:   return prefix + "Release";
        case Knob::algorithm: return "algorithm";
        case Knob::feedback:  return "feedback";
        case Knob::lfoRate:   return "lfoRate";
        case Knob::lfoDepth:  return "lfoDepth";
        case Knob::lfoTarget: return "lfoTarget";
        case Knob::glide:     return "glide";
        case Knob::mode:      return "mode";
        case Knob::velocity:  return "velocity";
        case Knob::output:    return "output";
    }

    return {};
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInFm::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const fm::Settings d;

    auto add = [&layout] (Knob k, int op, const juce::String& name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k, op), 1 }, name, range, value, knobText (k)));
    };

    auto choice = [&layout] (Knob k, const juce::String& name, const juce::StringArray& items, int value)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (k), 1 }, name, items, value));
    };

    for (int op = 0; op < fm::numOperators; ++op)
    {
        const auto& o = d.ops[(size_t) op];
        const juce::String name = "Operator " + juce::String (op + 1) + " ";
        add (Knob::coarse, op, name + "ratio", { 0.0f, (float) fm::maxCoarse, 1.0f }, (float) o.coarse);
        add (Knob::fine, op, name + "fine", { 0.0f, fm::maxFine, 1.0f }, o.finePercent);
        add (Knob::level, op, name + "level", { 0.0f, 100.0f, 0.5f }, o.levelPercent);
        add (Knob::attack, op, name + "attack", logRange (fm::minTime, fm::maxTime), o.env.attack);
        add (Knob::decay, op, name + "decay", logRange (fm::minTime, fm::maxTime), o.env.decay);
        add (Knob::sustain, op, name + "sustain", { 0.0f, 100.0f, 1.0f }, o.env.sustain * 100.0f);
        add (Knob::release, op, name + "release", logRange (fm::minTime, fm::maxTime), o.env.release);
    }

    juce::StringArray algorithms;

    for (int a = 0; a < fm::numAlgorithms; ++a)
        algorithms.add (fm::getAlgorithm (a).name);

    juce::NormalisableRange<float> glide { 0.0f, fm::maxGlide, 0.001f };
    glide.setSkewForCentre (0.25f);

    choice (Knob::algorithm, "Algorithm", algorithms, d.algorithm);
    add (Knob::feedback, 0, "Feedback", { 0.0f, 100.0f, 1.0f }, d.feedbackPercent);
    add (Knob::lfoRate, 0, "LFO rate", logRange (fm::minLfoRate, fm::maxLfoRate), d.lfoRateHz);
    add (Knob::lfoDepth, 0, "LFO depth", { 0.0f, 100.0f, 1.0f }, d.lfoDepthPercent);
    choice (Knob::lfoTarget, "LFO to", { "Pitch", "Brightness", "Volume" }, (int) d.lfoTarget);
    add (Knob::glide, 0, "Glide", glide, d.glideSeconds);
    choice (Knob::mode, "Voices", { "Poly", "Mono" }, (int) d.mode);
    add (Knob::velocity, 0, "Velocity", { 0.0f, 100.0f, 1.0f }, d.velocityPercent);
    add (Knob::output, 0, "Output", { fm::minOutput, fm::maxOutput, 0.1f }, d.outputDb);
    return layout;
}

BuiltInFm::BuiltInFm()
    : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItFm", createLayout())
{
    for (int op = 0; op < fm::numOperators; ++op)
        for (int k = 0; k < fm::numOperatorKnobs; ++k)
            operatorValues[(size_t) op][(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k, op));

    for (int k = fm::numOperatorKnobs; k < fm::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    for (auto& e : shownEnvelopes)
        e = 1.0f;
}

juce::PluginDescription BuiltInFm::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Synth";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44447433;   // "DDt3"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

void BuiltInFm::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInFm::getKnobParameter (Knob k, int op) const
{
    return parameters.getParameter (getParameterId (k, op));
}

fm::Settings BuiltInFm::getSettings() const
{
    fm::Settings s;

    for (int op = 0; op < fm::numOperators; ++op)
        for (int k = 0; k < fm::numOperatorKnobs; ++k)
            apply (s, (Knob) k, op, operatorValues[(size_t) op][(size_t) k]->load());

    for (int k = fm::numOperatorKnobs; k < fm::numKnobs; ++k)
        apply (s, (Knob) k, 0, knobValues[(size_t) k]->load());

    return s;
}

void BuiltInFm::setKnob (Knob k, float value, int op)
{
    if (auto* p = getKnobParameter (k, op))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInFm::setSettings (const fm::Settings& s)
{
    for (int op = 0; op < fm::numOperators; ++op)
        for (int k = 0; k < fm::numOperatorKnobs; ++k)
            setKnob ((Knob) k, valueOf (s, (Knob) k, op), op);

    for (int k = fm::numOperatorKnobs; k < fm::numKnobs; ++k)
        setKnob ((Knob) k, valueOf (s, (Knob) k, 0));
}

double BuiltInFm::getTailLengthSeconds() const
{
    const auto s = getSettings();
    double longest = 0.0;

    for (auto& o : s.ops)
        longest = juce::jmax (longest, (double) o.env.release);

    return longest;
}

//==============================================================================
bool BuiltInFm::Voice::isActive (int carriers) const noexcept
{
    for (int op = 0; op < fm::numOperators; ++op)
        if ((carriers & (1 << op)) != 0 && envs[(size_t) op].isActive())
            return true;

    return false;
}

bool BuiltInFm::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet().isDisabled();
}

void BuiltInFm::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlock = juce::jmax (16, maximumBlockSize);
    clap.prepare (rate);
    outputGain.reset (rate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().outputDb));

    // Twice the rate inside: sidebands between half the rate and the rate are filtered out instead of folding back.
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing ((size_t) maxBlock);
    mono.setSize (1, maxBlock);
    carrierLevels.assign ((size_t) maxBlock, 0.0f);

    for (auto& v : voices)
    {
        for (auto& e : v.envs)
        {
            e.setSampleRate (rate);
            e.reset();
        }

        v.note = -1;
        v.held = false;
        v.lastEnvs.fill (0.0f);
    }

    heldNotes.clear();
    lastPitch = -1.0;
    testNoteLeft = -1;
}

void BuiltInFm::startVoice (Voice& v, int note, float velocity, bool retrigger)
{
    const auto s = getSettings();

    v.note = note;
    v.velocity = velocity;
    v.held = true;
    v.startedAt = ++noteCounter;
    v.targetPitch = note;
    v.pitch = s.glideSeconds > 0.0005f && lastPitch >= 0.0 ? lastPitch : (double) note;

    if (! v.isActive (fm::getAlgorithm (s.algorithm).carriers))
    {
        v.phases.fill (0.0);
        v.feedback1 = v.feedback2 = 0.0;
    }

    if (retrigger)
    {
        for (auto& e : v.envs)
            e.noteOn();

        v.age = 0.0;
        v.releasedAt = -1.0;
    }

    lastPitch = note;
}

void BuiltInFm::noteOn (int note, float velocity)
{
    const auto s = getSettings();
    const int carriers = fm::getAlgorithm (s.algorithm).carriers;
    const bool othersHeld = ! heldNotes.empty();
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    heldNotes.push_back (note);

    if (s.mode == sub::Mode::mono)
    {
        auto& v = voices[0];

        for (size_t i = 1; i < voices.size(); ++i)
        {
            if (voices[i].held)
            {
                voices[i].held = false;

                for (auto& e : voices[i].envs)
                    e.noteOff();
            }
        }

        if (othersHeld && v.held && v.isActive (carriers))
        {
            // Legato: glide to the new note without starting the envelopes again.
            v.note = note;
            v.targetPitch = note;
            v.startedAt = ++noteCounter;

            if (s.glideSeconds <= 0.0005f)
                v.pitch = note;

            lastPitch = note;
            return;
        }

        startVoice (v, note, velocity, true);
        return;
    }

    // Poly: a free voice, or the oldest one.
    Voice* chosen = nullptr;

    for (auto& v : voices)
        if (! v.isActive (carriers) && (chosen == nullptr || v.startedAt < chosen->startedAt))
            chosen = &v;

    if (chosen == nullptr)
        for (auto& v : voices)
            if (chosen == nullptr || v.startedAt < chosen->startedAt)
                chosen = &v;

    startVoice (*chosen, note, velocity, true);
}

void BuiltInFm::noteOff (int note)
{
    const auto s = getSettings();
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

    if (s.mode == sub::Mode::mono)
    {
        auto& v = voices[0];

        if (v.note != note || ! v.held)
            return;

        if (! heldNotes.empty())
        {
            // Back to the note still held, legato.
            v.note = heldNotes.back();
            v.targetPitch = v.note;

            if (s.glideSeconds <= 0.0005f)
                v.pitch = v.note;

            lastPitch = v.note;
            return;
        }
    }

    for (auto& v : voices)
    {
        if (v.held && v.note == note)
        {
            v.held = false;

            for (auto& e : v.envs)
                e.noteOff();

            v.releasedAt = v.age;
        }
    }
}

void BuiltInFm::renderVoice (Voice& v, const fm::Settings& s, float lfo, float* out, float& carrierPeak, int numSamples)
{
    // Glide: most of the way (95%) in the glide time.
    if (s.glideSeconds > 0.0005f)
        v.pitch += (v.targetPitch - v.pitch) * (1.0 - std::exp (-3.0 * numSamples / (s.glideSeconds * rate)));
    else
        v.pitch = v.targetPitch;

    const double pitch = v.pitch + (s.lfoTarget == fm::LfoTarget::pitch ? lfo * s.lfoDepthPercent / 100.0 * fm::lfoPitchSemitones : 0.0);
    const double hz = sub::getNoteHz (pitch), innerRate = rate * 2.0;
    const auto& a = fm::getAlgorithm (s.algorithm);
    const bool plain = withoutModulators.load (std::memory_order_relaxed);
    std::array<double, fm::numOperators> peaks {}, steps {}, amps {};

    for (int op = 0; op < fm::numOperators; ++op)
    {
        const bool carrier = (a.carriers & (1 << op)) != 0;
        const double opHz = hz * fm::getRatio (s.ops[(size_t) op]);
        steps[(size_t) op] = opHz / innerRate;

        // An operator above what twice the rate can hold would fold back, so it's left out.
        peaks[(size_t) op] = opHz < 0.45 * innerRate && (carrier || ! plain) ? fm::getAmplitude (s, op, v.velocity, lfo) : 0.0;
    }

    const double feedback = plain ? 0.0 : s.feedbackPercent / 100.0 * fm::maxFeedback;

    for (int i = 0; i < numSamples; ++i)
    {
        double carriers = 0.0;

        for (int op = 0; op < fm::numOperators; ++op)
        {
            v.lastEnvs[(size_t) op] = v.envs[(size_t) op].getNextSample();
            amps[(size_t) op] = peaks[(size_t) op] * v.lastEnvs[(size_t) op];

            if ((a.carriers & (1 << op)) != 0)
                carriers += amps[(size_t) op];
        }

        carrierPeak = juce::jmax (carrierPeak, (float) carriers * fm::voiceGain);

        for (int half = 0; half < 2; ++half)
        {
            out[2 * i + half] += (float) fm::runOperators (a, v.phases, amps, feedback, v.feedback1, v.feedback2) * fm::voiceGain;

            for (int op = 0; op < fm::numOperators; ++op)
            {
                auto& phase = v.phases[(size_t) op];
                phase += steps[(size_t) op];
                phase -= std::floor (phase);
            }
        }
    }

    v.age += numSamples / rate;
}

void BuiltInFm::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (oversampling == nullptr)
        return;

    const auto s = getSettings();
    const int carriers = fm::getAlgorithm (s.algorithm).carriers;

    for (auto& v : voices)
        for (int op = 0; op < fm::numOperators; ++op)
        {
            const auto& e = s.ops[(size_t) op].env;
            v.envs[(size_t) op].setParameters ({ e.attack, e.decay, e.sustain, e.release });
        }

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    // The window's test note: on now, off after 0.8 s.
    if (clap.takeRequest())
    {
        noteOn (sub::testNote, 0.8f);
        testNoteLeft = (int) (0.8 * rate);
    }

    auto* left = buffer.getWritePointer (0);
    auto events = midi.begin();
    float lfo = 0.0f;

    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int length = juce::jmin (maxBlock, numSamples - start);
        juce::dsp::AudioBlock<float> block (mono);
        auto part = block.getSubBlock (0, (size_t) length);
        part.clear();
        auto inner = oversampling->processSamplesUp (part);
        inner.clear();
        float* innerData = inner.getChannelPointer (0);
        int pos = 0;

        while (pos < length)
        {
            for (; events != midi.end() && (*events).samplePosition <= start + pos; ++events)
            {
                const auto m = (*events).getMessage();

                if (m.isNoteOn())
                    noteOn (m.getNoteNumber(), m.getFloatVelocity());
                else if (m.isNoteOff())
                    noteOff (m.getNoteNumber());
                else if (m.isAllNotesOff() || m.isAllSoundOff())
                    for (int note : std::vector<int> (heldNotes))
                        noteOff (note);
            }

            // A step: up to stepSize samples, ending at the next MIDI event.
            int end = juce::jmin (length, pos + stepSize);

            if (events != midi.end())
                end = juce::jmin (end, juce::jmax (pos + 1, (*events).samplePosition - start));

            const int count = end - pos;

            lfo = (float) std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
            lfoPhase += s.lfoRateHz * count / rate;
            lfoPhase -= std::floor (lfoPhase);

            float carrierPeak = 0.0f;

            for (auto& v : voices)
            {
                if (v.isActive (carriers))
                {
                    float peak = 0.0f;
                    renderVoice (v, s, lfo, innerData + 2 * pos, peak, count);
                    carrierPeak += peak;
                }
            }

            for (int i = pos; i < end; ++i)
                carrierLevels[(size_t) i] = carrierPeak;

            if (testNoteLeft >= 0)
            {
                testNoteLeft -= count;

                if (testNoteLeft < 0)
                    noteOff (sub::testNote);
            }

            pos = end;
        }

        oversampling->processSamplesDown (part);
        const float* result = mono.getReadPointer (0);

        for (int i = 0; i < length; ++i)
        {
            const float gain = outputGain.getNextValue();
            const float value = result[i] * gain;
            left[start + i] = value;

            chunkBefore = juce::jmax (chunkBefore, carrierLevels[(size_t) i] * gain);
            chunkAfter = juce::jmax (chunkAfter, std::abs (value));

            if (++chunkFill == chunkSize)
            {
                history.push ({ juce::Decibels::gainToDecibels (chunkBefore, -100.0f), juce::Decibels::gainToDecibels (chunkAfter, -100.0f) });
                chunkBefore = chunkAfter = 0.0f;
                chunkFill = 0;
            }
        }
    }

    lfoValue.store (lfo, std::memory_order_relaxed);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // The newest note, for the pictures.
    const Voice* newest = nullptr;

    for (auto& v : voices)
        if (v.isActive (carriers) && (newest == nullptr || v.startedAt > newest->startedAt))
            newest = &v;

    for (int op = 0; op < fm::numOperators; ++op)
        shownEnvelopes[(size_t) op] = newest != nullptr ? newest->lastEnvs[(size_t) op] : 1.0f;

    noteAge = newest != nullptr ? (float) newest->age : -1.0f;
    releaseAge = newest != nullptr ? (float) newest->releasedAt : -1.0f;
    shownVelocity = newest != nullptr ? newest->velocity : 1.0f;
}

juce::AudioProcessorEditor* BuiltInFm::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInFm::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInFm::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
