#include "BuiltInPhysical.h"
#include "BuiltInFormat.h"

#include <juce_dsp/juce_dsp.h>

namespace daw
{
namespace phys
{
    namespace
    {
        constexpr double pi = juce::MathConstants<double>::pi, twoPi = juce::MathConstants<double>::twoPi;

        /** dB per neper of power: a loss of x in power, for small x, is 4.343 x dB. */
        constexpr double dbPerPower = 4.342944819;

        /** The bow's top speed at full velocity, and the reed's push; in the units of the waves in the lines. */
        constexpr double bowSpeed = 0.2;

        /** What each exciter is turned up or down by, so a full-velocity note in Init peaks near -12 dB and the others
            are about as loud. */
        constexpr double pluckGain = 0.27, strikeGain = 0.9, bowGain = 0.25, blowGain = 0.3;

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

        /** The one-pole low-pass's loss at `w` (radians a sample) for a trip, in dB. */
        double getLowPassDb (double pole, double w)
        {
            return 10.0 * std::log10 (1.0 + 2.0 * pole * (1.0 - std::cos (w)) / ((1.0 - pole) * (1.0 - pole)));
        }

        /** How many samples the one-pole low-pass delays a sine at `w`. */
        double getLowPassDelay (double pole, double w)
        {
            return pole <= 0.0 ? 0.0 : std::atan2 (pole * std::sin (w), 1.0 - pole * std::cos (w)) / w;
        }

        /** The friction of rosined hair on a string: when the string moves with the bow (a small difference in speed)
            it sticks and is carried along; when they differ a lot it slips. From Perry Cook and Gary Scavone's
            Synthesis ToolKit (STK). */
        double getBowFriction (double speedDifference, double slope)
        {
            return juce::jlimit (0.01, 0.98, std::pow (std::abs ((speedDifference + 0.001) * slope) + 0.75, -4.0));
        }

        /** How far a reed lets the wave back into the tube, from the pressure across it: it closes as the player blows
            harder (STK's reed table). */
        double getReedReflection (double pressureDifference)
        {
            return juce::jlimit (-1.0, 1.0, 0.7 - 0.3 * pressureDifference);
        }
    }

    juce::String getExciterName (Exciter e)
    {
        switch (e)
        {
            case Exciter::pluck:  return "Pluck";
            case Exciter::strike: return "Strike";
            case Exciter::bow:    return "Bow";
            case Exciter::blow:   return "Blow";
        }

        return {};
    }

    juce::String getResonatorName (Resonator r)
    {
        switch (r)
        {
            case Resonator::string: return "String";
            case Resonator::tube:   return "Tube";
        }

        return {};
    }

    juce::String getKnobName (Knob k)
    {
        switch (k)
        {
            case Knob::exciter:      return "Exciter";
            case Knob::hardness:     return "Hardness";
            case Knob::position:     return "Position";
            case Knob::pressure:     return "Pressure";
            case Knob::noise:        return "Noise";
            case Knob::resonator:    return "Resonator";
            case Knob::decay:        return "Decay";
            case Knob::damping:      return "Damping";
            case Knob::release:      return "Release";
            case Knob::attack:       return "Attack";
            case Knob::vibratoRate:  return "Rate";
            case Knob::vibratoDepth: return "Depth";
            case Knob::glide:        return "Glide";
            case Knob::mode:         return "Voices";
            case Knob::velocity:     return "Velocity";
            case Knob::output:       return "Output";
        }

        return {};
    }

    bool isUsed (const Settings& s, Knob k)
    {
        const bool sustained = s.exciter == Exciter::bow || s.exciter == Exciter::blow;

        switch (k)
        {
            case Knob::hardness:  return ! sustained;
            case Knob::pressure:
            case Knob::noise:
            case Knob::attack:    return sustained;
            case Knob::position:
            case Knob::resonator: return s.exciter != Exciter::blow;
            case Knob::exciter:
            case Knob::decay:
            case Knob::damping:
            case Knob::release:
            case Knob::vibratoRate:
            case Knob::vibratoDepth:
            case Knob::glide:
            case Knob::mode:
            case Knob::velocity:
            case Knob::output:    return true;
        }

        return true;
    }

    Resonator getResonator (const Settings& s)
    {
        return s.exciter == Exciter::blow ? Resonator::tube : s.resonator;
    }

    //==============================================================================
    double getDampingDbPerSecond (float dampingPercent, double hz)
    {
        if (dampingPercent <= 0.0f)
            return 0.0;

        // 3 dB a second at 1 kHz at the bottom, 200 times that at the top: even steps on the knob are even ratios.
        const double atOneKilohertz = 3.0 * std::pow (200.0, juce::jlimit (0.0f, 100.0f, dampingPercent) / 100.0);
        return atOneKilohertz * (hz / 1000.0) * (hz / 1000.0);
    }

    Loop getLoop (const Settings& s, double noteHz, double sampleRate, bool released)
    {
        const bool tube = getResonator (s) == Resonator::tube;

        // At least 8 samples a trip, so both sides fit.
        const double hz = juce::jlimit (20.0, sampleRate / (tube ? 16.0 : 8.0), noteHz);
        Loop l;
        l.length = sampleRate / hz / (tube ? 2.0 : 1.0);
        l.tripsPerSecond = hz * (tube ? 2.0 : 1.0);

        // The low-pass in the loop: its loss per trip grows with the square of the frequency at the bottom, so it can
        // make the damping's loss per second the same at every note.
        const double w1k = twoPi * 1000.0 / sampleRate;
        const double k = getDampingDbPerSecond (s.dampingPercent, 1000.0) / (dbPerPower * w1k * w1k * l.tripsPerSecond);
        l.pole = k <= 1.0e-12 ? 0.0 : ((2.0 * k + 1.0) - std::sqrt (4.0 * k + 1.0)) / (2.0 * k);

        // The rest of the fundamental's loss comes from the gain: -60 dB in the decay time.
        const double w0 = twoPi * hz / sampleRate;
        const double seconds = released ? juce::jmin (s.decaySeconds, s.releaseSeconds) : s.decaySeconds;
        const double wanted = ringDb / (juce::jmax ((double) minRelease, seconds) * l.tripsPerSecond);
        l.gain = juce::Decibels::decibelsToGain (-juce::jmax (0.0, wanted - getLowPassDb (l.pole, w0)));

        // The low-pass delays the wave a little too, so the sides are that much shorter to keep the note in tune.
        const double filterDelay = getLowPassDelay (l.pole, w0);

        if (s.exciter == Exciter::blow)
        {
            l.sideA = 0.0;
            l.sideB = l.length - filterDelay;
        }
        else
        {
            l.sideA = juce::jlimit (2.0, juce::jmax (2.0, l.length - filterDelay - 2.0), s.positionPercent / 100.0 * l.length);
            l.sideB = l.length - l.sideA - filterDelay;
        }

        return l;
    }

    bool hasHarmonic (const Settings& s, int n)
    {
        return n >= 1 && (getResonator (s) == Resonator::string || n % 2 == 1);
    }

    double getRingSeconds (const Settings& s, double noteHz, int n, bool released, double sampleRate)
    {
        const auto l = getLoop (s, noteHz, sampleRate, released);
        const double w = twoPi * n * l.tripsPerSecond / (getResonator (s) == Resonator::tube ? 2.0 : 1.0) / sampleRate;

        if (w >= pi)
            return 0.0;

        const double perTrip = getLowPassDb (l.pole, w) - juce::Decibels::gainToDecibels (l.gain);
        return perTrip <= 1.0e-9 ? 1000.0 : ringDb / (perTrip * l.tripsPerSecond);
    }

    double getPositionLevel (const Settings& s, int n)
    {
        if (s.exciter == Exciter::blow)
            return 1.0;

        const double p = juce::jlimit (minPosition, maxPosition, s.positionPercent) / 100.0;
        return getResonator (s) == Resonator::tube ? std::abs (std::cos (pi * n * p / 2.0)) : std::abs (std::sin (pi * n * p));
    }

    double getPickHz (float hardnessPercent)
    {
        return 200.0 * std::pow (100.0, juce::jlimit (0.0f, 100.0f, hardnessPercent) / 100.0);
    }

    double getContactSeconds (float hardnessPercent)
    {
        return 0.005 * std::pow (0.03, juce::jlimit (0.0f, 100.0f, hardnessPercent) / 100.0);
    }

    double getBowSlope (float pressurePercent)
    {
        return 2.5 - 2.35 * juce::jlimit (0.0f, 100.0f, pressurePercent) / 100.0;
    }

    double getBreath (float pressurePercent, float velocityScale)
    {
        return 0.62 + 0.3 * juce::jlimit (0.0f, 100.0f, pressurePercent) / 100.0 * juce::jlimit (0.0f, 1.0f, velocityScale);
    }

    float getVelocityScale (const Settings& s, float velocity)
    {
        return 1.0f - juce::jlimit (0.0f, 1.0f, s.velocityPercent / 100.0f) * (1.0f - juce::jlimit (0.0f, 1.0f, velocity));
    }

    //==============================================================================
    void Voice::Line::clear() noexcept
    {
        std::fill (data.begin(), data.end(), 0.0f);
    }

    void Voice::Line::push (double x) noexcept
    {
        data[(size_t) write] = (float) x;
        write = (write + 1) & mask;
    }

    double Voice::Line::read (double delay) const noexcept
    {
        // Cubic Lagrange between the four samples around `delay` samples ago (1 is the newest): smooth enough to glide
        // and bend, and it hardly dulls the highs as a straight line between two samples would.
        int whole = (int) delay;
        double x = delay - whole;

        if (whole < 2)
        {
            whole = 2;
            x = 0.0;
        }

        auto at = [this] (int ago) { return (double) data[(size_t) ((write - ago) & mask)]; };
        const double ym1 = at (whole - 1), y0 = at (whole), y1 = at (whole + 1), y2 = at (whole + 2);

        return ym1 * (-x * (x - 1.0) * (x - 2.0) / 6.0) + y0 * ((x + 1.0) * (x - 1.0) * (x - 2.0) / 2.0)
             + y1 * (-(x + 1.0) * x * (x - 2.0) / 2.0) + y2 * ((x + 1.0) * x * (x - 1.0) / 6.0);
    }

    void Voice::prepare (double sampleRate)
    {
        rate = sampleRate > 0.0 ? sampleRate : 44100.0;
        const int size = juce::nextPowerOfTwo ((int) (rate / 20.0) + 16);

        for (auto* line : { &a, &b })
        {
            line->data.assign ((size_t) size, 0.0f);
            line->write = 0;
            line->mask = size - 1;
        }

        burstData.assign ((size_t) size, 0.0f);
        dcPole = 1.0 - twoPi * 10.0 / rate;
        followerFall = std::exp (-1.0 / (0.05 * rate));
        active = held = false;
    }

    void Voice::start (const Settings& s, double noteHz, float noteVelocity, juce::uint32 seed, bool keepRinging)
    {
        if (a.data.empty())
            prepare (rate);

        if (! keepRinging || ! active)
        {
            a.clear();
            b.clear();
            low = dcIn = dcOut = push = follower = 0.0;
        }

        noiseState = seed == 0 ? 1u : seed;
        hz = noteHz;
        velocity = noteVelocity;
        scale = getVelocityScale (s, noteVelocity);
        held = active = true;
        age = 0.0;
        sample = 0;
        update (s);

        // A softer pluck is a softer finger, darker; a softer strike touches the string longer.
        const double pickHz = juce::jmin (0.45 * rate, getPickHz (s.hardnessPercent) * std::pow (4.0, scale - 1.0));
        pickPole = std::exp (-twoPi * pickHz / rate);
        pickLow = 0.0;
        burst = s.exciter == Exciter::pluck ? juce::jlimit (2, (int) burstData.size(), juce::roundToInt (rate / juce::jmax (20.0, hz))) : 0;

        if (burst > 0)
            makeBurst (burst);

        contact = s.exciter == Exciter::strike ? juce::jmax (2.0, getContactSeconds (s.hardnessPercent) * std::pow (2.0, 1.0 - scale) * rate) : 0.0;
    }

    void Voice::update (const Settings& s)
    {
        settings = s;
        loop = getLoop (s, hz, rate, ! held);

        switch (s.exciter)
        {
            case Exciter::pluck:  gain = pluckGain; break;
            case Exciter::strike: gain = strikeGain; break;
            case Exciter::bow:    gain = bowGain; break;
            case Exciter::blow:   gain = blowGain * scale; break;
        }
    }

    void Voice::makeBurst (int length)
    {
        for (int i = 0; i < length; ++i)
            burstData[(size_t) i] = (float) nextNoise();

        // Random noise gives each harmonic a random level, so one pluck can have twice the body of the next. The lowest
        // eight are set to the level noise has on average, keeping their random phases: every pluck sounds as full.
        const double average = std::sqrt (4.0 / (3.0 * length));

        for (int n = 1; n <= juce::jmin (8, length / 2 - 1); ++n)
        {
            std::complex<double> sum;

            for (int i = 0; i < length; ++i)
                sum += (double) burstData[(size_t) i] * std::polar (1.0, -twoPi * n * i / length);

            const auto now = sum * (2.0 / length);
            const auto wanted = std::abs (now) > 1.0e-12 ? now * (average / std::abs (now)) : std::complex<double> (average, 0.0);
            const auto change = wanted - now;

            for (int i = 0; i < length; ++i)
                burstData[(size_t) i] += (float) std::real (change * std::polar (1.0, twoPi * n * i / length));
        }
    }

    double Voice::nextNoise() noexcept
    {
        noiseState = noiseState * 1664525u + 1013904223u;
        return (double) (noiseState >> 8) / 8388608.0 - 1.0;
    }

    double Voice::excite() noexcept
    {
        if (sample < burst)
        {
            // Karplus and Strong's pluck: a period of noise, filtered as soft as the finger or pick.
            pickLow = (1.0 - pickPole) * burstData[(size_t) sample] + pickPole * pickLow;
            return pickLow * scale;
        }

        if (sample < contact)
        {
            // A hammer: half a sine as long as it touches, the same energy however long, so a hard one is as loud.
            return scale / std::sqrt (contact) * std::sin (pi * sample / contact);
        }

        return 0.0;
    }

    float Voice::next (float* exciterPart, float* pointSpeed, float* pushSpeed) noexcept
    {
        if (! active)
        {
            if (exciterPart != nullptr) *exciterPart = 0.0f;
            if (pointSpeed != nullptr) *pointSpeed = 0.0f;
            if (pushSpeed != nullptr) *pushSpeed = 0.0f;
            return 0.0f;
        }

        const bool sustained = settings.exciter == Exciter::bow || settings.exciter == Exciter::blow;

        // The bow or breath: rising in the attack, falling in the release.
        if (sustained)
            push = held ? juce::jmin (1.0, push + 1.0 / (juce::jmax ((double) minAttack, (double) settings.attackSeconds) * rate))
                        : juce::jmax (0.0, push - 1.0 / (juce::jmax ((double) minRelease, (double) settings.releaseSeconds) * rate));

        const double aOut = settings.exciter == Exciter::blow ? 0.0 : a.read (loop.sideA);
        const double bOut = b.read (loop.sideB);

        // The far end (a string's nut, or the tube's open end) turns the wave over and takes its losses.
        low = (1.0 - loop.pole) * bOut + loop.pole * low;
        const double fromB = exciterOnly ? 0.0 : -loop.gain * low;
        double out = bOut, input = 0.0, speed = 0.0, pushed = 0.0;

        if (settings.exciter == Exciter::blow)
        {
            // A reed at the tube's closed end: the breath pushes in, and the reed lets the wave back in as far as it's open.
            double breath = push * getBreath (settings.pressurePercent, scale);
            breath += breath * settings.noisePercent / 100.0 * 0.2 * nextNoise();
            const double difference = fromB - breath;
            input = breath + difference * getReedReflection (difference);
            speed = fromB;
            pushed = breath;

            if (exciterOnly)
                out = input;
            else
                b.push (input);
        }
        else
        {
            // The end it's played near: a string's bridge turns the wave over too, a tube's closed end sends it back the same way up.
            const double fromA = exciterOnly ? 0.0 : (getResonator (settings) == Resonator::tube ? 1.0 : -1.0) * aOut;
            speed = fromA + fromB;

            if (settings.exciter == Exciter::bow)
            {
                double bow = push * bowSpeed * scale;
                bow += bow * settings.noisePercent / 100.0 * 0.3 * nextNoise();
                const double difference = bow - speed;
                input = difference * getBowFriction (difference, getBowSlope (settings.pressurePercent));
                pushed = bow;
            }
            else
            {
                input = excite();
            }

            if (exciterOnly)
            {
                out = input;
            }
            else
            {
                a.push (fromB + input);
                b.push (fromA + input);
            }
        }

        // Without the steady push of a bow or breath, which would otherwise come out as a level that never moves.
        const double y = (out - dcIn + dcPole * dcOut);
        dcIn = out;
        dcOut = y;
        const double result = y * gain;

        follower = juce::jmax (std::abs (result), follower * followerFall);
        age += 1.0 / rate;
        ++sample;

        if (follower < 1.0e-5 && sample > (int) (0.05 * rate) && (! sustained || (! held && push <= 0.0)))
            active = false;

        if (exciterPart != nullptr) *exciterPart = (float) (input * gain);
        // The string under the exciter moves with the waves arriving and what the exciter adds to both sides: when a bow
        // sticks, exactly the bow's speed.
        if (pointSpeed != nullptr) *pointSpeed = (float) (settings.exciter == Exciter::blow ? speed : speed + input);
        if (pushSpeed != nullptr) *pushSpeed = (float) pushed;
        return (float) result;
    }

    //==============================================================================
    Parts renderParts (const Settings& s, double noteHz, double sampleRate, double seconds, double holdSeconds, float velocity, bool exciterOnly)
    {
        Voice v;
        v.prepare (sampleRate);
        v.exciterOnly = exciterOnly;
        v.start (s, noteHz, velocity, 1, false);

        const size_t count = (size_t) juce::jmax (0, (int) (seconds * sampleRate));
        Parts parts;

        for (auto* part : { &parts.sound, &parts.exciter, &parts.point, &parts.push })
            part->resize (count);

        const int letGo = holdSeconds < 0.0 ? -1 : (int) (holdSeconds * sampleRate);

        for (size_t i = 0; i < count; ++i)
        {
            if ((int) i == letGo)
                v.release();

            if (i % 16 == 0)
                v.update (s);

            parts.sound[i] = v.next (&parts.exciter[i], &parts.point[i], &parts.push[i]);
        }

        return parts;
    }

    std::vector<float> render (const Settings& s, double noteHz, double sampleRate, double seconds, double holdSeconds, float velocity, bool exciterOnly)
    {
        return renderParts (s, noteHz, sampleRate, seconds, holdSeconds, velocity, exciterOnly).sound;
    }

    namespace
    {
        constexpr int analysisOrder = 12, analysisSize = 1 << analysisOrder, binsPerHarmonic = 32;
    }

    double getAnalysisRate (double noteHz)
    {
        return noteHz * analysisSize / binsPerHarmonic;
    }

    std::vector<double> measureHarmonics (const std::vector<float>& sound, double noteHz, double atSeconds, int count)
    {
        const int first = juce::jmax (0, (int) (atSeconds * getAnalysisRate (noteHz)) - analysisSize / 2);

        // A Blackman-Harris window: its side lobes are under -92 dB, so a loud harmonic doesn't hide a quiet one.
        constexpr double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;
        std::vector<float> data ((size_t) analysisSize * 2, 0.0f);

        for (int i = 0; i < analysisSize && first + i < (int) sound.size(); ++i)
        {
            const double x = twoPi * i / (analysisSize - 1);
            data[(size_t) i] = sound[(size_t) (first + i)] * (float) (a0 - a1 * std::cos (x) + a2 * std::cos (2.0 * x) - a3 * std::cos (3.0 * x));
        }

        juce::dsp::FFT fft (analysisOrder);
        fft.performFrequencyOnlyForwardTransform (data.data());

        std::vector<double> levels;

        for (int n = 1; n <= juce::jmin (count, analysisSize / 2 / binsPerHarmonic - 1); ++n)
        {
            // A bowed or blown note can sit a few cents off; the loudest bin near the harmonic is its level.
            float loudest = 0.0f;

            for (int bin = n * binsPerHarmonic - 3; bin <= n * binsPerHarmonic + 3; ++bin)
                loudest = juce::jmax (loudest, data[(size_t) bin]);

            levels.push_back (2.0 * loudest / (analysisSize * a0));
        }

        return levels;
    }

    std::vector<double> getHarmonics (const Settings& s, double noteHz, double atSeconds, int count, double holdSeconds, float velocity)
    {
        const double sampleRate = getAnalysisRate (noteHz);
        const auto sound = render (s, noteHz, sampleRate, atSeconds + analysisSize / 2 / sampleRate + 0.001, holdSeconds, velocity);
        return measureHarmonics (sound, noteHz, atSeconds, count);
    }

    bool isSame (const Settings& x, const Settings& y)
    {
        return x.exciter == y.exciter && x.resonator == y.resonator && x.mode == y.mode && juce::exactlyEqual (x.hardnessPercent, y.hardnessPercent) && juce::exactlyEqual (x.positionPercent, y.positionPercent)
               && juce::exactlyEqual (x.pressurePercent, y.pressurePercent) && juce::exactlyEqual (x.noisePercent, y.noisePercent)
               && juce::exactlyEqual (x.decaySeconds, y.decaySeconds) && juce::exactlyEqual (x.dampingPercent, y.dampingPercent)
               && juce::exactlyEqual (x.releaseSeconds, y.releaseSeconds) && juce::exactlyEqual (x.attackSeconds, y.attackSeconds)
               && juce::exactlyEqual (x.vibratoRateHz, y.vibratoRateHz) && juce::exactlyEqual (x.vibratoDepthPercent, y.vibratoDepthPercent)
               && juce::exactlyEqual (x.glideSeconds, y.glideSeconds) && juce::exactlyEqual (x.velocityPercent, y.velocityPercent)
               && juce::exactlyEqual (x.outputDb, y.outputDb);
    }

    //==============================================================================
    juce::String formatKnob (Knob k, const Settings& s)
    {
        switch (k)
        {
            case Knob::exciter:      return getExciterName (s.exciter);
            case Knob::hardness:     return formatPercent (s.hardnessPercent);
            case Knob::position:     return formatPercent (s.positionPercent);
            case Knob::pressure:     return formatPercent (s.pressurePercent);
            case Knob::noise:        return s.noisePercent < 0.5f ? juce::String ("Off") : formatPercent (s.noisePercent);
            case Knob::resonator:    return getResonatorName (getResonator (s));
            case Knob::decay:        return formatTime (s.decaySeconds);
            case Knob::damping:      return s.dampingPercent < 0.5f ? juce::String ("Off") : formatPercent (s.dampingPercent);
            case Knob::release:      return formatTime (s.releaseSeconds);
            case Knob::attack:       return formatTime (s.attackSeconds);
            case Knob::vibratoRate:  return juce::String (s.vibratoRateHz, 1) + " Hz";
            case Knob::vibratoDepth: return s.vibratoDepthPercent < 0.5f ? juce::String ("Off") : formatPercent (s.vibratoDepthPercent);
            case Knob::glide:        return s.glideSeconds < 0.0005f ? juce::String ("Off") : formatTime (s.glideSeconds);
            case Knob::mode:         return s.mode == sub::Mode::mono ? "Mono" : "Poly";
            case Knob::velocity:     return formatPercent (s.velocityPercent);
            case Knob::output:       return withSign (s.outputDb, 1) + " dB";
        }

        return {};
    }

    //==============================================================================
    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            auto add = [&list] (const juce::String& name, const juce::String& why, Settings s, std::initializer_list<std::pair<Knob, const char*>> notes)
            {
                Preset p;
                p.name = name;
                p.why = why;
                p.settings = s;

                for (auto& [knob, note] : notes)
                    p.notes[(size_t) knob] = juce::CharPointer_UTF8 (note);

                list.push_back (p);
            };

            add ("Init: a plucked string", "A string plucked a fifth of the way along, ringing for 3 seconds with its highs dying sooner. Change the "
                                           "position and listen to the harmonics come and go; change the damping and hear it turn from metal to nylon.",
                 {}, { { Knob::exciter, "Pluck: a burst of noise, one period long, sets the string going." },
                       { Knob::position, "20%: a fifth of the way along, so the 5th and 10th harmonics are missing." },
                       { Knob::damping, "40%: a partial at 2 kHz rings about 0.6 s, the fundamental 3 s." } });

            Settings ks;
            ks.hardnessPercent = 100.0f;
            ks.positionPercent = minPosition;
            ks.decaySeconds = 20.0f;
            ks.releaseSeconds = 0.5f;
            add ("Karplus-Strong, 1983", "The algorithm that started it: fill a short delay with random noise and play it round and round, "
                                         "averaging each sample with the next. The noise turns into a plucked string in a few hundred milliseconds.",
                 ks, { { Knob::hardness, "100%: plain white noise, unfiltered, as Karplus and Strong used." },
                       { Knob::position, "2%: right at the end, so every harmonic is there, as when the whole delay is filled." },
                       { Knob::decay, "20 s: nothing but the damping takes the sound away, as in the original." } });

            Settings nylon;
            nylon.hardnessPercent = 35.0f;
            nylon.positionPercent = 18.0f;
            nylon.decaySeconds = 4.0f;
            nylon.dampingPercent = 55.0f;
            nylon.releaseSeconds = 0.25f;
            add ("Nylon guitar", "A classical guitar: a soft fingertip on a nylon string, which loses its highs fast. Warm and round.",
                 nylon, { { Knob::hardness, "35%: a fingertip, not a pick: the burst is filtered at about 1 kHz." },
                          { Knob::position, "18%: over the soundhole's bridge side, the usual place." },
                          { Knob::damping, "55%: nylon damps the highs much more than steel." } });

            Settings steel;
            steel.hardnessPercent = 85.0f;
            steel.positionPercent = 12.0f;
            steel.decaySeconds = 6.0f;
            steel.dampingPercent = 30.0f;
            add ("Steel-string guitar", "A pick on a steel string near the bridge: bright, with a long ring.",
                 steel, { { Knob::hardness, "85%: a plastic pick, so the burst keeps its highs." },
                          { Knob::position, "12%: close to the bridge, thinner and brighter than over the soundhole." },
                          { Knob::damping, "30%: steel keeps its highs ringing." },
                          { Knob::decay, "6 s." } });

            Settings harp;
            harp.hardnessPercent = 50.0f;
            harp.positionPercent = 45.0f;
            harp.decaySeconds = 6.0f;
            harp.dampingPercent = 50.0f;
            harp.releaseSeconds = 2.0f;
            add ("Harp", "Harpists pluck near the middle of the string, which weakens the even harmonics: a round, hollow, bell-like note "
                         "that rings on.",
                 harp, { { Knob::position, "45%: near the middle, so the 2nd, 4th and 6th harmonics start at a third of their strength or less." },
                         { Knob::release, "2 s: harp strings ring on after you let go, until the hand stops them." } });

            Settings bass;
            bass.hardnessPercent = 40.0f;
            bass.positionPercent = 10.0f;
            bass.decaySeconds = 3.0f;
            bass.dampingPercent = 60.0f;
            bass.releaseSeconds = 0.12f;
            bass.mode = sub::Mode::mono;
            add ("Fingered bass", "An electric bass played with the fingers: a dark pluck that stops as soon as the note ends.",
                 bass, { { Knob::release, "0.12 s: the fingers stop the string as soon as the key comes up." },
                         { Knob::mode, "Mono: a note played while another is held is a hammer-on, the same string at a new pitch, not a new pluck." },
                         { Knob::damping, "60%: a dark, round tone." } });

            Settings slide;
            slide.hardnessPercent = 80.0f;
            slide.positionPercent = 12.0f;
            slide.decaySeconds = 5.0f;
            slide.dampingPercent = 30.0f;
            slide.mode = sub::Mode::mono;
            slide.glideSeconds = 0.15f;
            slide.vibratoRateHz = 5.0f;
            slide.vibratoDepthPercent = 12.0f;
            add ("Slide guitar", "A bottleneck sliding along a ringing string: play notes legato and the string slides between them without being "
                                 "plucked again.",
                 slide, { { Knob::glide, "0.15 s: the slide from one note to the next." },
                          { Knob::mode, "Mono: one string; legato notes slide on it." },
                          { Knob::vibratoDepth, "12%: the slide's wobble, 12 cents up and down." } });

            Settings dulcimer;
            dulcimer.exciter = Exciter::strike;
            dulcimer.hardnessPercent = 75.0f;
            dulcimer.positionPercent = 12.0f;
            dulcimer.decaySeconds = 6.0f;
            dulcimer.dampingPercent = 35.0f;
            dulcimer.releaseSeconds = 1.5f;
            add ("Hammered dulcimer", "A string hit by a small hard hammer: a bright ping that rings on. Piano hammers hit at about 1/7 to 1/9 of "
                                      "the string too, which keeps the 7th to 9th harmonics, the harsh ones, quiet.",
                 dulcimer, { { Knob::exciter, "Strike: a short smooth push instead of a burst of noise." },
                             { Knob::hardness, "75%: a hard hammer, touching the string for about half a millisecond." },
                             { Knob::position, "12%: about 1/8 of the way along, so the 8th harmonic is missing." } });

            Settings felt;
            felt.exciter = Exciter::strike;
            felt.hardnessPercent = 15.0f;
            felt.positionPercent = 12.0f;
            felt.decaySeconds = 5.0f;
            felt.dampingPercent = 50.0f;
            felt.releaseSeconds = 0.4f;
            felt.velocityPercent = 70.0f;
            add ("Felt hammer", "A soft felt hammer: it touches the string for a few milliseconds, too long to make the highs, so the note is "
                                "dark and gentle, like a felt piano.",
                 felt, { { Knob::hardness, "15%: soft felt, touching for about 3 ms." },
                         { Knob::velocity, "70%: soft notes are quieter, and the hammer stays on even longer, so they're darker too." } });

            Settings pizz;
            pizz.hardnessPercent = 30.0f;
            pizz.positionPercent = 25.0f;
            pizz.decaySeconds = 0.8f;
            pizz.dampingPercent = 60.0f;
            pizz.releaseSeconds = 0.15f;
            add ("Pizzicato strings", "Violins plucked with a finger: the body takes the energy quickly, so the note is short and soft.",
                 pizz, { { Knob::decay, "0.8 s: a violin's pluck dies quickly." },
                         { Knob::hardness, "30%: a fingertip." },
                         { Knob::position, "25%: a quarter of the way, over the fingerboard's end." } });

            Settings cello;
            cello.exciter = Exciter::bow;
            cello.positionPercent = 18.0f;
            cello.pressurePercent = 60.0f;
            cello.noisePercent = 15.0f;
            cello.dampingPercent = 50.0f;
            cello.attackSeconds = 0.15f;
            cello.releaseSeconds = 0.25f;
            cello.vibratoRateHz = 5.0f;
            cello.vibratoDepthPercent = 18.0f;
            add ("Bowed cello", "A bow drawn across a string: the hair grips the string and drags it along, it slips back, grips again, once a "
                                "period. The note lasts as long as the bow moves. Play it low.",
                 cello, { { Knob::exciter, "Bow: a steady drag that sticks and slips." },
                          { Knob::pressure, "60%: firm, so the string slips once a period and the note is clean." },
                          { Knob::position, "18%: between the bridge and the fingerboard. Nearer the bridge is brighter but takes more pressure, or the string slips twice a period." },
                          { Knob::attack, "0.15 s: the bow getting up to speed." },
                          { Knob::noise, "15%: a little scratch of the hair." },
                          { Knob::vibratoDepth, "18%: a cellist's vibrato, 18 cents up and down." } });

            Settings clarinet;
            clarinet.exciter = Exciter::blow;
            clarinet.pressurePercent = 55.0f;
            clarinet.noisePercent = 15.0f;
            clarinet.dampingPercent = 40.0f;
            clarinet.attackSeconds = 0.04f;
            clarinet.releaseSeconds = 0.06f;
            clarinet.mode = sub::Mode::mono;
            clarinet.glideSeconds = 0.03f;
            add ("Clarinet", "A reed on a tube closed at one end: the reed opens and closes like a valve, once a period, and the tube's closed end "
                             "keeps only the odd harmonics. The hollow sound of a clarinet's low notes.",
                 clarinet, { { Knob::exciter, "Blow: breath through a reed, which always plays a tube." },
                             { Knob::pressure, "55%: well above the point where the reed starts to move, so it speaks quickly." },
                             { Knob::noise, "15%: a little breath." },
                             { Knob::mode, "Mono with a 30 ms glide: one note at a time, as a wind player plays." } });

            Settings breathy;
            breathy.exciter = Exciter::blow;
            breathy.pressurePercent = 10.0f;
            breathy.noisePercent = 50.0f;
            breathy.dampingPercent = 70.0f;
            breathy.attackSeconds = 0.2f;
            breathy.releaseSeconds = 0.15f;
            breathy.vibratoRateHz = 4.5f;
            breathy.vibratoDepthPercent = 8.0f;
            breathy.velocityPercent = 40.0f;
            breathy.mode = sub::Mode::mono;
            add ("Breathy reed", "Blown softly, just above where the reed starts to sound: slow to speak, airy and dark.",
                 breathy, { { Knob::pressure, "10%: barely enough breath: the note takes its time to start." },
                            { Knob::noise, "50%: lots of air in the sound." },
                            { Knob::attack, "0.2 s: a slow breath in." } });

            Settings hollow;
            hollow.resonator = Resonator::tube;
            hollow.hardnessPercent = 80.0f;
            hollow.positionPercent = 15.0f;
            hollow.decaySeconds = 2.0f;
            hollow.dampingPercent = 30.0f;
            add ("Plucked tube", "No real instrument, but the model can do it: a pluck on a tube closed at one end. Only the odd harmonics ring, "
                                 "so it sounds hollow, like a square wave.",
                 hollow, { { Knob::resonator, "Tube: closed at one end, so only the odd harmonics (1, 3, 5...) fit in it." },
                           { Knob::position, "15%, near the closed end." } });

            return list;
        }();

        return presets;
    }
} // namespace phys

//==============================================================================
using phys::Knob;

namespace
{
    juce::NormalisableRange<float> logRange (float low, float high)
    {
        return { low, high,
                 [] (float start, float end, float t) { return start * std::pow (end / start, t); },
                 [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); } };
    }

    /** Puts a knob's value into the settings. */
    void apply (phys::Settings& s, Knob k, float value)
    {
        switch (k)
        {
            case Knob::exciter:      s.exciter = (phys::Exciter) juce::jlimit (0, phys::numExciters - 1, juce::roundToInt (value)); break;
            case Knob::hardness:     s.hardnessPercent = value; break;
            case Knob::position:     s.positionPercent = value; break;
            case Knob::pressure:     s.pressurePercent = value; break;
            case Knob::noise:        s.noisePercent = value; break;
            case Knob::resonator:    s.resonator = (phys::Resonator) juce::jlimit (0, phys::numResonators - 1, juce::roundToInt (value)); break;
            case Knob::decay:        s.decaySeconds = value; break;
            case Knob::damping:      s.dampingPercent = value; break;
            case Knob::release:      s.releaseSeconds = value; break;
            case Knob::attack:       s.attackSeconds = value; break;
            case Knob::vibratoRate:  s.vibratoRateHz = value; break;
            case Knob::vibratoDepth: s.vibratoDepthPercent = value; break;
            case Knob::glide:        s.glideSeconds = value; break;
            case Knob::mode:         s.mode = value >= 0.5f ? sub::Mode::mono : sub::Mode::poly; break;
            case Knob::velocity:     s.velocityPercent = value; break;
            case Knob::output:       s.outputDb = value; break;
        }
    }

    /** A knob's value in the settings, as its parameter holds it. */
    float valueOf (const phys::Settings& s, Knob k)
    {
        switch (k)
        {
            case Knob::exciter:      return (float) (int) s.exciter;
            case Knob::hardness:     return s.hardnessPercent;
            case Knob::position:     return s.positionPercent;
            case Knob::pressure:     return s.pressurePercent;
            case Knob::noise:        return s.noisePercent;
            case Knob::resonator:    return (float) (int) s.resonator;
            case Knob::decay:        return s.decaySeconds;
            case Knob::damping:      return s.dampingPercent;
            case Knob::release:      return s.releaseSeconds;
            case Knob::attack:       return s.attackSeconds;
            case Knob::vibratoRate:  return s.vibratoRateHz;
            case Knob::vibratoDepth: return s.vibratoDepthPercent;
            case Knob::glide:        return s.glideSeconds;
            case Knob::mode:         return s.mode == sub::Mode::mono ? 1.0f : 0.0f;
            case Knob::velocity:     return s.velocityPercent;
            case Knob::output:       return s.outputDb;
        }

        return 0.0f;
    }

    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float value, int)
            {
                phys::Settings s;
                apply (s, k, value);
                return phys::formatKnob (k, s);
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return 0.0f;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();
                return text.containsIgnoreCase ("ms") ? v / 1000.0f : v;
            });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;

    /** The pitch, the vibrato and the loop move from one step to the next. */
    constexpr int stepSize = 16;
}

const char* BuiltInPhysical::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::exciter:      return "exciter";
        case Knob::hardness:     return "hardness";
        case Knob::position:     return "position";
        case Knob::pressure:     return "pressure";
        case Knob::noise:        return "noise";
        case Knob::resonator:    return "resonator";
        case Knob::decay:        return "decay";
        case Knob::damping:      return "damping";
        case Knob::release:      return "release";
        case Knob::attack:       return "attack";
        case Knob::vibratoRate:  return "vibratoRate";
        case Knob::vibratoDepth: return "vibratoDepth";
        case Knob::glide:        return "glide";
        case Knob::mode:         return "mode";
        case Knob::velocity:     return "velocity";
        case Knob::output:       return "output";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInPhysical::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const phys::Settings d;

    auto add = [&layout] (Knob k, const juce::String& name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    auto choice = [&layout] (Knob k, const juce::String& name, const juce::StringArray& items, int value)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (k), 1 }, name, items, value));
    };

    juce::NormalisableRange<float> glide { 0.0f, phys::maxGlide, 0.001f };
    glide.setSkewForCentre (0.25f);

    choice (Knob::exciter, "Exciter", { "Pluck", "Strike", "Bow", "Blow" }, (int) d.exciter);
    add (Knob::hardness, "Hardness", { 0.0f, 100.0f, 1.0f }, d.hardnessPercent);
    add (Knob::position, "Position", { phys::minPosition, phys::maxPosition, 0.5f }, d.positionPercent);
    add (Knob::pressure, "Pressure", { 0.0f, 100.0f, 1.0f }, d.pressurePercent);
    add (Knob::noise, "Noise", { 0.0f, 100.0f, 1.0f }, d.noisePercent);
    choice (Knob::resonator, "Resonator", { "String", "Tube" }, (int) d.resonator);
    add (Knob::decay, "Decay", logRange (phys::minDecay, phys::maxDecay), d.decaySeconds);
    add (Knob::damping, "Damping", { 0.0f, 100.0f, 1.0f }, d.dampingPercent);
    add (Knob::release, "Release", logRange (phys::minRelease, phys::maxRelease), d.releaseSeconds);
    add (Knob::attack, "Attack", logRange (phys::minAttack, phys::maxAttack), d.attackSeconds);
    add (Knob::vibratoRate, "Vibrato rate", logRange (phys::minVibratoRate, phys::maxVibratoRate), d.vibratoRateHz);
    add (Knob::vibratoDepth, "Vibrato depth", { 0.0f, 100.0f, 1.0f }, d.vibratoDepthPercent);
    add (Knob::glide, "Glide", glide, d.glideSeconds);
    choice (Knob::mode, "Voices", { "Poly", "Mono" }, (int) d.mode);
    add (Knob::velocity, "Velocity", { 0.0f, 100.0f, 1.0f }, d.velocityPercent);
    add (Knob::output, "Output", { phys::minOutput, phys::maxOutput, 0.1f }, d.outputDb);
    return layout;
}

BuiltInPhysical::BuiltInPhysical()
    : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItPhysical", createLayout())
{
    for (int k = 0; k < phys::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));
}

juce::PluginDescription BuiltInPhysical::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Synth";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44447435;   // "DDt5"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

void BuiltInPhysical::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInPhysical::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

phys::Settings BuiltInPhysical::getSettings() const
{
    phys::Settings s;

    for (int k = 0; k < phys::numKnobs; ++k)
        apply (s, (Knob) k, knobValues[(size_t) k]->load());

    return s;
}

void BuiltInPhysical::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInPhysical::setSettings (const phys::Settings& s)
{
    for (int k = 0; k < phys::numKnobs; ++k)
        setKnob ((Knob) k, valueOf (s, (Knob) k));
}

double BuiltInPhysical::getTailLengthSeconds() const
{
    const auto s = getSettings();
    return juce::jmin (s.decaySeconds, s.releaseSeconds);
}

bool BuiltInPhysical::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet().isDisabled();
}

void BuiltInPhysical::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    clap.prepare (rate);
    outputGain.reset (rate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().outputDb));

    for (auto& slot : slots)
    {
        slot.voice.prepare (rate);
        slot.note = -1;
        slot.held = false;
    }

    heldNotes.clear();
    lastPitch = -1.0;
    testNoteLeft = -1;
    seed = 1;
}

void BuiltInPhysical::startSlot (Slot& slot, int note, float velocity, bool keepRinging)
{
    const auto s = getSettings();

    slot.note = note;
    slot.velocity = velocity;
    slot.held = true;
    slot.startedAt = ++noteCounter;
    slot.targetPitch = note;
    slot.pitch = s.glideSeconds > 0.0005f && lastPitch >= 0.0 ? lastPitch : (double) note;
    slot.releasedAt = -1.0;

    // Each pluck its own noise, so repeated notes aren't identical.
    seed = seed * 747796405u + 2891336453u;
    slot.voice.start (s, sub::getNoteHz (slot.pitch), velocity, seed, keepRinging);
    lastPitch = note;
}

void BuiltInPhysical::noteOn (int note, float velocity)
{
    const auto s = getSettings();
    const bool othersHeld = ! heldNotes.empty();
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    heldNotes.push_back (note);

    if (s.mode == sub::Mode::mono)
    {
        auto& slot = slots[0];

        for (size_t i = 1; i < slots.size(); ++i)
        {
            if (slots[i].held)
            {
                slots[i].held = false;
                slots[i].voice.release();
            }
        }

        if (othersHeld && slot.held && slot.voice.isActive())
        {
            // Legato: the same string at a new pitch, without plucking it again (a hammer-on), or the bow carrying on.
            slot.note = note;
            slot.targetPitch = note;
            slot.startedAt = ++noteCounter;

            if (s.glideSeconds <= 0.0005f)
                slot.pitch = note;

            lastPitch = note;
            return;
        }

        startSlot (slot, note, velocity, slot.voice.isActive());
        return;
    }

    // Poly: a string can only ring once, so the same note plays the same voice again; otherwise a free voice, or the oldest.
    Slot* chosen = nullptr;

    for (auto& slot : slots)
        if (slot.note == note && slot.voice.isActive())
            chosen = &slot;

    if (chosen == nullptr)
        for (auto& slot : slots)
            if (! slot.voice.isActive() && (chosen == nullptr || slot.startedAt < chosen->startedAt))
                chosen = &slot;

    if (chosen == nullptr)
        for (auto& slot : slots)
            if (chosen == nullptr || slot.startedAt < chosen->startedAt)
                chosen = &slot;

    startSlot (*chosen, note, velocity, chosen->note == note && chosen->voice.isActive());
}

void BuiltInPhysical::noteOff (int note)
{
    const auto s = getSettings();
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

    if (s.mode == sub::Mode::mono)
    {
        auto& slot = slots[0];

        if (slot.note != note || ! slot.held)
            return;

        if (! heldNotes.empty())
        {
            // Back to the note still held, legato: a pull-off.
            slot.note = heldNotes.back();
            slot.targetPitch = slot.note;

            if (s.glideSeconds <= 0.0005f)
                slot.pitch = slot.note;

            lastPitch = slot.note;
            return;
        }
    }

    for (auto& slot : slots)
    {
        if (slot.held && slot.note == note)
        {
            slot.held = false;
            slot.voice.release();
            slot.releasedAt = slot.voice.getAge();
        }
    }
}

void BuiltInPhysical::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (buffer.getNumChannels() == 0)
        return;

    const auto s = getSettings();
    const bool alone = exciterOnly.load (std::memory_order_relaxed);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    // The window's test note: on now, off after 1.5 s.
    if (clap.takeRequest())
    {
        noteOn (sub::testNote, 0.8f);
        testNoteLeft = (int) (testNoteSeconds * rate);
    }

    auto* left = buffer.getWritePointer (0);
    auto events = midi.begin();
    int pos = 0;

    while (pos < numSamples)
    {
        for (; events != midi.end() && (*events).samplePosition <= pos; ++events)
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
        int end = juce::jmin (numSamples, pos + stepSize);

        if (events != midi.end())
            end = juce::jmin (end, juce::jmax (pos + 1, (*events).samplePosition));

        const int count = end - pos;
        const double vibrato = std::sin (juce::MathConstants<double>::twoPi * vibratoPhase) * s.vibratoDepthPercent / 100.0 * phys::vibratoSemitones;
        vibratoPhase += s.vibratoRateHz * count / rate;
        vibratoPhase -= std::floor (vibratoPhase);

        std::array<float, stepSize> excited {};

        for (auto& slot : slots)
        {
            if (! slot.voice.isActive())
                continue;

            // Glide: most of the way (95%) in the glide time.
            if (s.glideSeconds > 0.0005f)
                slot.pitch += (slot.targetPitch - slot.pitch) * (1.0 - std::exp (-3.0 * count / (s.glideSeconds * rate)));
            else
                slot.pitch = slot.targetPitch;

            slot.voice.exciterOnly = alone;
            slot.voice.setPitch (sub::getNoteHz (slot.pitch + vibrato));
            slot.voice.update (s);

            for (int i = 0; i < count; ++i)
            {
                float part = 0.0f;
                left[pos + i] += slot.voice.next (&part);
                excited[(size_t) i] += part;
            }
        }

        for (int i = 0; i < count; ++i)
        {
            const float gain = outputGain.getNextValue();
            const float value = left[pos + i] * gain;
            left[pos + i] = value;

            chunkBefore = juce::jmax (chunkBefore, std::abs (excited[(size_t) i] * gain));
            chunkAfter = juce::jmax (chunkAfter, std::abs (value));

            if (++chunkFill == chunkSize)
            {
                history.push ({ juce::Decibels::gainToDecibels (chunkBefore, -100.0f), juce::Decibels::gainToDecibels (chunkAfter, -100.0f) });
                chunkBefore = chunkAfter = 0.0f;
                chunkFill = 0;
            }
        }

        if (testNoteLeft >= 0)
        {
            testNoteLeft -= count;

            if (testNoteLeft < 0)
                noteOff (sub::testNote);
        }

        pos = end;
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // The newest note, for the pictures.
    const Slot* newest = nullptr;

    for (auto& slot : slots)
        if (slot.voice.isActive() && (newest == nullptr || slot.startedAt > newest->startedAt))
            newest = &slot;

    noteAge = newest != nullptr ? (float) newest->voice.getAge() : -1.0f;
    releaseAge = newest != nullptr ? (float) newest->releasedAt : -1.0f;
    shownLevel = newest != nullptr ? newest->voice.getLevel() : 0.0f;
    shownVelocity = newest != nullptr ? newest->velocity : 1.0f;
}

juce::AudioProcessorEditor* BuiltInPhysical::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInPhysical::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInPhysical::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
