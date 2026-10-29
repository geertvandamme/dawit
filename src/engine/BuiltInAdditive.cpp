#include "BuiltInAdditive.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace additive
{
    namespace
    {
        constexpr double pi = juce::MathConstants<double>::pi;

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

        /** The tilt in dB per octave for a note: the brightness knob, duller for softer notes, and the LFO. */
        double getTilt (const Settings& s, float velocity, float lfo)
        {
            double tilt = s.brightnessDb - 6.0 * s.velocityPercent / 100.0 * (1.0 - juce::jlimit (0.0f, 1.0f, velocity));

            if (s.lfoTarget == LfoTarget::brightness)
                tilt += lfo * s.lfoDepthPercent / 100.0 * lfoBrightnessDb;

            return tilt;
        }
    }

    Settings::Settings()
    {
        levelsDb.fill (offDb);
        levelsDb[0] = 0.0f;
    }

    juce::String getShapeName (Shape shape)
    {
        switch (shape)
        {
            case Shape::sine:     return "Sine";
            case Shape::saw:      return "Saw";
            case Shape::square:   return "Square";
            case Shape::triangle: return "Triangle";
            case Shape::organ:    return "Organ";
        }

        return {};
    }

    juce::String getLfoTargetName (LfoTarget t)
    {
        switch (t)
        {
            case LfoTarget::pitch:      return "Pitch";
            case LfoTarget::brightness: return "Brightness";
            case LfoTarget::even:       return "Even";
        }

        return {};
    }

    bool isFadeOff (float fadeSeconds)
    {
        return fadeSeconds >= maxFade - 0.5f;
    }

    //==============================================================================
    std::array<float, numPartials> getShapeLevels (Shape shape)
    {
        std::array<float, numPartials> levels;
        levels.fill (offDb);

        for (int n = 1; n <= numPartials; ++n)
        {
            const float db = -20.0f * std::log10 ((float) n);
            const bool odd = n % 2 == 1;
            auto& level = levels[(size_t) n - 1];

            switch (shape)
            {
                case Shape::sine:     level = n == 1 ? 0.0f : offDb; break;
                case Shape::saw:      level = db; break;
                case Shape::square:   level = odd ? db : offDb; break;
                case Shape::triangle: level = odd ? juce::jmax (offDb + 0.1f, 2.0f * db) : offDb; break;
                case Shape::organ:    level = n <= 4 ? 0.0f : offDb; break;
            }
        }

        return levels;
    }

    double getRatio (int n, float stretchPercent)
    {
        const double b = maxStiffness * std::pow (juce::jlimit (0.0, 1.0, stretchPercent / 100.0), 2.0);
        return n * std::sqrt ((1.0 + b * n * n) / (1.0 + b));
    }

    double getStretchCents (int n, float stretchPercent)
    {
        return 1200.0 * std::log2 (getRatio (n, stretchPercent) / juce::jmax (1, n));
    }

    std::array<double, numPartials> getStartPhases (const Settings& s)
    {
        std::array<double, numPartials> aligned {};

        if (s.phases == Phases::aligned)
            return aligned;

        std::array<double, numPartials> gains {};

        for (int n = 1; n <= numPartials; ++n)
            gains[(size_t) n - 1] = getPartialGain (s, n);

        // The same harmonics give the same phases, and finding them takes a moment, so the last answer is kept (per thread, for the audio and the pictures).
        thread_local std::array<double, numPartials> lastGains { -1.0 };
        thread_local std::array<double, numPartials> lastPhases {};

        if (std::equal (gains.begin(), gains.end(), lastGains.begin(), [] (double a, double b) { return juce::exactlyEqual (a, b); }))
            return lastPhases;

        double total = 0.0;
        int highest = 1;

        for (int n = 1; n <= numPartials; ++n)
        {
            total += gains[(size_t) n - 1] * gains[(size_t) n - 1];

            if (gains[(size_t) n - 1] > 0.0)
                highest = n;
        }

        // How high a cycle of the wave peaks with these phases.
        auto getPeak = [&gains] (const std::array<double, numPartials>& phases)
        {
            constexpr int points = 256;
            double peak = 0.0;

            for (int i = 0; i < points; ++i)
            {
                const double x = 2.0 * pi * i / points;
                double v = 0.0;

                for (int n = 1; n <= numPartials; ++n)
                    if (gains[(size_t) n - 1] > 0.0)
                        v += gains[(size_t) n - 1] * std::sin (n * x + phases[(size_t) n - 1]);

                peak = juce::jmax (peak, std::abs (v));
            }

            return peak;
        };

        // The candidates: lined up; Schroeder's phases from the levels, -2 pi times the sum over the lower harmonics l
        // of (n - l) times l's share of the power; his phases for equal levels, -pi n (n - 1) / N, up to the highest
        // harmonic that's on; and a few fixed random sets. The one that peaks lowest wins.
        std::vector<std::array<double, numPartials>> candidates { aligned };
        std::array<double, numPartials> weighted {}, equal {};

        for (int n = 2; n <= numPartials && total > 0.0; ++n)
        {
            double sum = 0.0;

            for (int l = 1; l < n; ++l)
                sum += (n - l) * gains[(size_t) l - 1] * gains[(size_t) l - 1] / total;

            weighted[(size_t) n - 1] = std::fmod (-2.0 * pi * sum, 2.0 * pi);
            equal[(size_t) n - 1] = std::fmod (-pi * n * (n - 1) / highest, 2.0 * pi);
        }

        candidates.push_back (weighted);
        candidates.push_back (equal);

        for (int seed = 1; seed <= 8; ++seed)
        {
            juce::Random random (seed);
            std::array<double, numPartials> phases {};

            for (size_t i = 1; i < phases.size(); ++i)
                phases[i] = 2.0 * pi * random.nextDouble();

            candidates.push_back (phases);
        }

        double lowest = getPeak (aligned);
        lastPhases = aligned;

        for (auto& c : candidates)
        {
            if (const double peak = getPeak (c); peak < lowest * 0.999)
            {
                lowest = peak;
                lastPhases = c;
            }
        }

        lastGains = gains;
        return lastPhases;
    }

    double getPartialGain (const Settings& s, int n, float velocity, float lfo)
    {
        if (n < 1 || n > numPartials || s.levelsDb[(size_t) n - 1] <= offDb + 0.01f)
            return 0.0;

        double gain = juce::Decibels::decibelsToGain ((double) s.levelsDb[(size_t) n - 1])
                      * std::pow (10.0, getTilt (s, velocity, lfo) * std::log2 ((double) n) / 20.0);

        if (n % 2 == 0)
        {
            double even = juce::jlimit (0.0, 1.0, s.evenPercent / 100.0);

            if (s.lfoTarget == LfoTarget::even)
                even *= 1.0 - s.lfoDepthPercent / 100.0 * (0.5 - 0.5 * lfo);

            gain *= even;
        }

        return gain;
    }

    double getNormalisation (const Settings& s)
    {
        double power = 0.0;

        for (int n = 1; n <= numPartials; ++n)
            power += std::pow (getPartialGain (s, n), 2.0);

        return power > 1.0 ? 1.0 / std::sqrt (power) : 1.0;
    }

    double getPartialEnvelope (const Settings& s, int n, double seconds)
    {
        if (n <= 1 || seconds < 0.0)
            return 1.0;

        double level = 1.0;
        const double swell = s.swellSeconds * (n - 1) / (numPartials - 1);

        if (swell > 1.0e-4 && seconds < swell)
            level = seconds / swell;

        if (! isFadeOff (s.fadeSeconds))
            level *= std::pow (2.0, -seconds * (n - 1) / juce::jmax ((double) minFade, (double) s.fadeSeconds));

        return level;
    }

    float getVelocityScale (const Settings& s, float velocity)
    {
        return 1.0f - juce::jlimit (0.0f, 1.0f, s.velocityPercent / 100.0f) * (1.0f - juce::jlimit (0.0f, 1.0f, velocity));
    }

    //==============================================================================
    std::vector<Partial> getPartials (const Settings& s, double noteHz, double seconds, float velocity, double maxHz)
    {
        std::vector<Partial> partials;
        const double norm = getNormalisation (s);
        const auto phases = getStartPhases (s);

        for (int n = 1; n <= numPartials; ++n)
        {
            Partial p;
            p.n = n;
            p.hz = noteHz * getRatio (n, s.stretchPercent);
            p.amplitude = getPartialGain (s, n, velocity) * norm * getPartialEnvelope (s, n, seconds);
            p.phase = phases[(size_t) n - 1];

            if (p.hz < maxHz && p.amplitude > 1.0e-6)
                partials.push_back (p);
        }

        return partials;
    }

    std::vector<float> getWave (const std::vector<Partial>& partials, double seconds, int points)
    {
        std::vector<float> wave ((size_t) juce::jmax (0, points), 0.0f);

        for (auto& p : partials)
            for (int i = 0; i < points; ++i)
                wave[(size_t) i] += (float) (p.amplitude * std::sin (2.0 * pi * p.hz * seconds * i / juce::jmax (1, points - 1) + p.phase));

        return wave;
    }

    //==============================================================================
    juce::String getKnobName (Knob k)
    {
        switch (k)
        {
            case Knob::brightness: return "Brightness";
            case Knob::even:       return "Even";
            case Knob::stretch:    return "Stretch";
            case Knob::phases:     return "Phases";
            case Knob::swell:      return "Swell";
            case Knob::fade:       return "Fade";
            case Knob::ampAttack:  return "Attack";
            case Knob::ampDecay:   return "Decay";
            case Knob::ampSustain: return "Sustain";
            case Knob::ampRelease: return "Release";
            case Knob::lfoRate:    return "Rate";
            case Knob::lfoDepth:   return "Depth";
            case Knob::lfoTarget:  return "To";
            case Knob::glide:      return "Glide";
            case Knob::mode:       return "Voices";
            case Knob::velocity:   return "Velocity";
            case Knob::output:     return "Output";
        }

        return {};
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        switch (k)
        {
            case Knob::brightness: return withSign (s.brightnessDb, 1) + " dB/oct";
            case Knob::even:       return formatPercent (s.evenPercent);
            case Knob::stretch:    return formatPercent (s.stretchPercent);
            case Knob::phases:     return s.phases == Phases::spread ? "Spread" : "Aligned";
            case Knob::swell:      return s.swellSeconds < 0.0005f ? juce::String ("Off") : formatTime (s.swellSeconds);
            case Knob::fade:       return isFadeOff (s.fadeSeconds) ? juce::String ("Off") : formatTime (s.fadeSeconds);
            case Knob::ampAttack:  return formatTime (s.ampEnv.attack);
            case Knob::ampDecay:   return formatTime (s.ampEnv.decay);
            case Knob::ampSustain: return formatPercent (s.ampEnv.sustain * 100.0f);
            case Knob::ampRelease: return formatTime (s.ampEnv.release);
            case Knob::lfoRate:    return juce::String (s.lfoRateHz, s.lfoRateHz < 1.0f ? 2 : 1) + " Hz";
            case Knob::lfoDepth:   return formatPercent (s.lfoDepthPercent);
            case Knob::lfoTarget:  return getLfoTargetName (s.lfoTarget);
            case Knob::glide:      return s.glideSeconds < 0.0005f ? juce::String ("Off") : formatTime (s.glideSeconds);
            case Knob::mode:       return s.mode == sub::Mode::mono ? "Mono" : "Poly";
            case Knob::velocity:   return formatPercent (s.velocityPercent);
            case Knob::output:     return withSign (s.outputDb, 1) + " dB";
        }

        return {};
    }

    juce::String formatLevel (float db)
    {
        return db <= offDb + 0.05f ? juce::String ("Off") : fx::formatNumber (db, 1) + " dB";
    }

    juce::String getOrdinal (int n)
    {
        const int lastTwo = n % 100, last = n % 10;
        const char* suffix = (lastTwo >= 11 && lastTwo <= 13) ? "th" : last == 1 ? "st" : last == 2 ? "nd" : last == 3 ? "rd" : "th";
        return juce::String (n) + suffix;
    }

    //==============================================================================
    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            auto add = [&list] (const juce::String& name, const juce::String& why, const juce::String& partials, Settings s,
                                std::initializer_list<std::pair<Knob, const char*>> notes)
            {
                Preset p;
                p.name = name;
                p.why = why;
                p.partials = partials;
                p.settings = s;

                for (auto& [knob, note] : notes)
                    p.notes[(size_t) knob] = juce::CharPointer_UTF8 (note);

                list.push_back (p);
            };

            auto levels = [] (Settings& s, std::initializer_list<std::pair<int, float>> bars)
            {
                s.levelsDb.fill (offDb);

                for (auto& [n, db] : bars)
                    s.levelsDb[(size_t) n - 1] = db;
            };

            add ("Init: one sine", "One harmonic, the fundamental, and nothing moving: a pure sine, the raw material. Start here and add "
                                   "harmonics one at a time, listening to each.",
                 "Only the 1st harmonic: the note itself, with nothing above it. Draw in the 2nd, then the 3rd, and hear the sound fill out.",
                 {}, { { Knob::brightness, "No tilt: the bars are heard as they're drawn." },
                       { Knob::ampSustain, "Full while the key is held, like an organ: the note doesn't change." } });

            Settings organ;
            levels (organ, { { 1, 0.0f }, { 2, 0.0f }, { 3, 0.0f }, { 4, 0.0f }, { 8, -9.0f } });
            organ.phases = Phases::spread;
            organ.ampEnv = { 0.003f, 0.05f, 1.0f, 0.03f };
            organ.lfoRateHz = 6.9f;
            organ.lfoDepthPercent = 4.0f;
            organ.velocityPercent = 0.0f;
            organ.outputDb = -4.0f;
            add ("Drawbar organ", "A Hammond organ is additive synthesis you play with your hands: each drawbar is one harmonic, and "
                                  "pulling it out turns it up. This is the 8', 4', 2 2/3' and 2' drawbars all the way out, with the 1' "
                                  "halfway.",
                 "The 1st to 4th harmonics at full and the 8th at -9 dB (a drawbar at 5 of 8: each step is about 3 dB). "
                 "The 16' and 5 1/3' drawbars sound below the note, so they aren't here.",
                 organ, { { Knob::phases, "Spread: an organ's tonewheels don't start in step, and the peaks stay lower." },
                          { Knob::ampAttack, "3 ms: the note is there at once, with a hint of the key click." },
                          { Knob::ampRelease, "30 ms: it stops when the key comes up." },
                          { Knob::lfoRate, "6.9 Hz: about the speed of the Hammond's own vibrato." },
                          { Knob::lfoDepth, "4%: a gentle shimmer in the pitch." },
                          { Knob::velocity, "0%: an organ plays every note as loud, however hard the key is pressed." },
                          { Knob::output, "Down 4 dB: four full harmonics in a chord add up." } });

            Settings saw;
            saw.levelsDb = getShapeLevels (Shape::saw);
            saw.ampEnv = { 0.003f, 0.3f, 1.0f, 0.15f };
            add ("A saw, from sines", "The proof that any sound is made of sines: every harmonic at 1/n of the first adds up to a saw, the "
                                      "buzzy wave of a subtractive synth. Turn the bars off from the top down and hear it get softer.",
                 "Every harmonic at 1/n: the 2nd at -6 dB, the 3rd at -9.5, the 4th at -12, the 32nd at -30. With 32 of them it's a "
                 "touch softer than an analog saw, which has hundreds.",
                 saw, { { Knob::phases, "Aligned: the harmonics start in step, so the wave picture shows the saw's ramp." },
                        { Knob::brightness, "Try -3 dB/oct: the same sound, darker, as a low-pass filter would make it." },
                        { Knob::even, "Turn it to 0% and the saw becomes a square: only the odd harmonics are left." } });

            Settings reed;
            reed.levelsDb = getShapeLevels (Shape::square);
            reed.brightnessDb = -3.0f;
            reed.ampEnv = { 0.03f, 0.1f, 0.9f, 0.08f };
            reed.lfoRateHz = 5.0f;
            reed.lfoDepthPercent = 3.0f;
            reed.mode = sub::Mode::mono;
            reed.glideSeconds = 0.04f;
            add ("Hollow reed", "A sound close to a clarinet's low notes: its closed tube makes mostly odd harmonics, which is what gives "
                                "it that hollow, woody colour.",
                 "The odd harmonics only, at 1/n (a square's), the even ones off.",
                 reed, { { Knob::brightness, "-3 dB per octave: the highs softer than a square's, like a wooden tube." },
                         { Knob::ampAttack, "30 ms: the reed takes a moment to start." },
                         { Knob::lfoDepth, "3%: a little vibrato, as a player adds to long notes." },
                         { Knob::mode, "Mono: a wind instrument plays one note at a time." },
                         { Knob::glide, "40 ms: legato notes slide a little." } });

            Settings flute;
            levels (flute, { { 1, 0.0f }, { 2, -8.0f }, { 3, -18.0f }, { 4, -26.0f }, { 5, -34.0f } });
            flute.swellSeconds = 0.3f;
            flute.ampEnv = { 0.08f, 0.2f, 0.9f, 0.12f };
            flute.lfoRateHz = 5.0f;
            flute.lfoDepthPercent = 4.0f;
            flute.velocityPercent = 40.0f;
            add ("Flute", "A flute is close to a pure sine, with a few soft harmonics above it: little to build, so additive makes it easily.",
                 "The 1st harmonic strong, the 2nd at -8 dB, then the 3rd to 5th fading fast.",
                 flute, { { Knob::swell, "300 ms for the 32nd, about 40 ms for the 5th: the harmonics come in after the note starts, like breath getting the air column going." },
                          { Knob::ampAttack, "80 ms: a soft start." },
                          { Knob::lfoRate, "5 Hz: a flautist's vibrato." },
                          { Knob::lfoDepth, "4%: gentle." } });

            Settings keys;
            levels (keys, { { 1, 0.0f }, { 2, -6.0f }, { 3, -14.0f }, { 4, -10.0f }, { 5, -20.0f }, { 6, -24.0f }, { 7, -30.0f }, { 8, -22.0f } });
            keys.stretchPercent = 10.0f;
            keys.fadeSeconds = 0.5f;
            keys.ampEnv = { 0.002f, 2.5f, 0.0f, 0.4f };
            keys.velocityPercent = 70.0f;
            keys.outputDb = -2.0f;
            add ("Electric keys", "A struck tine, like an electric piano: bright at the start, the high harmonics fading within half a "
                                  "second, leaving a soft tone that dies away.",
                 "A few strong low harmonics and the 8th standing out a little, for the bell-like ping of a tine.",
                 keys, { { Knob::fade, "0.5 s: the 2nd halves in half a second, the 8th seven times as fast. The highs go first, as in every struck or plucked sound." },
                         { Knob::stretch, "10%: a touch of stretch, as metal tines and strings have." },
                         { Knob::ampDecay, "2.5 s: the note dies away..." },
                         { Knob::ampSustain, "...to nothing, even with the key held, as a struck note does." },
                         { Knob::velocity, "70%: soft notes are quieter and duller, hard ones brighter." } });

            Settings bell;
            levels (bell, { { 1, 0.0f }, { 2, -4.0f }, { 3, -8.0f }, { 4, -10.0f }, { 5, -12.0f }, { 6, -16.0f }, { 7, -18.0f },
                            { 8, -20.0f }, { 10, -24.0f }, { 12, -26.0f } });
            bell.stretchPercent = 100.0f;
            bell.phases = Phases::spread;
            bell.fadeSeconds = 1.2f;
            bell.ampEnv = { 0.001f, 5.0f, 0.0f, 3.0f };
            bell.velocityPercent = 60.0f;
            bell.outputDb = -3.0f;
            add ("Metal bell", "A metallic, bell-like ring: the harmonics stretched apart, so they no longer line up as whole-number "
                               "multiples of the note. That is what makes metal sound like metal and not like a string.",
                 "Many harmonics falling gently, with gaps at the 9th and 11th: the stretch then pulls them out of tune with each other.",
                 bell, { { Knob::stretch, "100%: the 8th harmonic is nearly 2 semitones sharp, the 12th almost 4." },
                         { Knob::fade, "1.2 s: the high partials ring briefly, the low ones hum on." },
                         { Knob::ampDecay, "5 s: a bell rings a long time..." },
                         { Knob::ampSustain, "...and dies away even if the key is held." },
                         { Knob::ampRelease, "3 s: letting go doesn't stop a bell." } });

            Settings harp;
            harp.levelsDb = getShapeLevels (Shape::saw);
            harp.brightnessDb = -3.0f;
            harp.stretchPercent = 5.0f;
            harp.fadeSeconds = 0.25f;
            harp.ampEnv = { 0.001f, 1.5f, 0.0f, 0.5f };
            harp.velocityPercent = 70.0f;
            add ("Plucked string", "A harp or a guitar: a pluck starts every harmonic at once, then the high ones die first, so the note "
                                   "goes from bright to round as it rings.",
                 "Every harmonic, falling at 1/n and a further 3 dB per octave.",
                 harp, { { Knob::fade, "250 ms: the 2nd halves in a quarter of a second, the 10th nine times as fast. That fall in brightness is the sound of a pluck." },
                         { Knob::ampSustain, "0%: the string rings out, whether or not the key is held." },
                         { Knob::stretch, "5%: real strings are a little stiff, so their high harmonics are a little sharp." },
                         { Knob::velocity, "70%: a harder pluck is louder and brighter." } });

            Settings brass;
            brass.levelsDb = getShapeLevels (Shape::saw);
            brass.brightnessDb = -2.0f;
            brass.swellSeconds = 0.25f;
            brass.ampEnv = { 0.03f, 0.2f, 0.9f, 0.2f };
            brass.lfoRateHz = 5.0f;
            brass.lfoDepthPercent = 4.0f;
            brass.velocityPercent = 60.0f;
            add ("Brass swell", "When a brass player starts a note, the low harmonics sound first and the high ones build up as the lips "
                                "buzz harder: the swell does that, giving each note its \"bwaah\".",
                 "Every harmonic at 1/n, as a trumpet's bright tone has.",
                 brass, { { Knob::swell, "250 ms for the 32nd: the 2nd is there in 8 ms, the 16th after 120 ms. The note gets brighter as it starts." },
                          { Knob::ampAttack, "30 ms: a soft start, like breath." },
                          { Knob::lfoDepth, "A touch of vibrato, as a player adds." },
                          { Knob::velocity, "60%: louder notes are brighter too, as when a player blows harder." } });

            Settings choir;
            levels (choir, { { 1, -6.0f }, { 2, -6.0f }, { 3, 0.0f }, { 4, 0.0f }, { 5, -3.0f }, { 6, -6.0f }, { 7, -15.0f }, { 8, -20.0f },
                             { 9, -22.0f }, { 10, -18.0f }, { 11, -12.0f }, { 12, -14.0f }, { 13, -22.0f }, { 14, -28.0f } });
            choir.phases = Phases::spread;
            choir.ampEnv = { 0.6f, 1.0f, 0.9f, 1.5f };
            choir.lfoRateHz = 4.8f;
            choir.lfoDepthPercent = 3.0f;
            choir.velocityPercent = 30.0f;
            choir.outputDb = -4.0f;
            add ("\"Ah\" pad", "A voice singing \"ah\": the vowel is made by peaks in the harmonics (formants) near 700 Hz, 1.2 kHz and "
                               "2.5 kHz. Here they're drawn for A3, so the vowel is clearest around that note.",
                 "Peaks at the 3rd and 4th harmonics (660 and 880 Hz at A3), the 5th and 6th (1.1 and 1.3 kHz) and the 11th (2.4 kHz). "
                 "The bars move with the note, so higher notes slide towards another vowel; a real voice keeps its formants in place.",
                 choir, { { Knob::phases, "Spread: many harmonics about as loud, so the peaks stay lower with the same sound." },
                          { Knob::ampAttack, "0.6 s: the voices swell in." },
                          { Knob::ampRelease, "1.5 s: they fade out slowly." },
                          { Knob::lfoRate, "4.8 Hz: a singer's vibrato." },
                          { Knob::output, "Down 4 dB: chords add up." } });

            Settings shimmer;
            shimmer.levelsDb = getShapeLevels (Shape::saw);
            shimmer.brightnessDb = -2.0f;
            shimmer.ampEnv = { 0.8f, 1.0f, 0.85f, 2.0f };
            shimmer.lfoRateHz = 0.25f;
            shimmer.lfoDepthPercent = 90.0f;
            shimmer.lfoTarget = LfoTarget::even;
            shimmer.velocityPercent = 30.0f;
            shimmer.outputDb = -4.0f;
            add ("Shimmer pad", "A pad that keeps changing colour: the LFO slowly takes the even harmonics out and brings them back, so "
                                "the sound moves between a full saw and a hollow square.",
                 "Every harmonic at 1/n; the LFO decides how much of the even ones you hear.",
                 shimmer, { { Knob::lfoTarget, "Even: the LFO moves the 2nd, 4th, 6th... harmonics." },
                            { Knob::lfoRate, "0.25 Hz: one slow change every four seconds." },
                            { Knob::lfoDepth, "90%: from all of the even harmonics to almost none." },
                            { Knob::ampAttack, "0.8 s: the chord fades in." },
                            { Knob::ampRelease, "2 s: and fades out slowly, so chord changes blur together." } });

            Settings subBass;
            levels (subBass, { { 1, 0.0f }, { 2, -14.0f }, { 3, -24.0f } });
            subBass.ampEnv = { 0.004f, 0.3f, 1.0f, 0.08f };
            subBass.mode = sub::Mode::mono;
            subBass.glideSeconds = 0.05f;
            subBass.velocityPercent = 20.0f;
            add ("Sub bass", "A deep, clean bass felt more than heard: almost a pure sine, with just enough of the 2nd and 3rd harmonics "
                             "that small speakers, which can't play the lowest notes, still let you hear the line.",
                 "The 1st harmonic, the 2nd at -14 dB and the 3rd at -24 dB.",
                 subBass, { { Knob::mode, "Mono: a bass plays one note at a time." },
                            { Knob::glide, "50 ms: overlapping notes slide." },
                            { Knob::velocity, "20%: every note about as strong, so the low end stays steady." } });

            return list;
        }();

        return presets;
    }
} // namespace additive

//==============================================================================
using additive::Knob;

namespace
{
    juce::NormalisableRange<float> logRange (float low, float high)
    {
        return { low, high,
                 [] (float start, float end, float t) { return start * std::pow (end / start, t); },
                 [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); } };
    }

    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float value, int)
            {
                additive::Settings s;

                switch (k)
                {
                    case Knob::brightness: s.brightnessDb = value; break;
                    case Knob::even:       s.evenPercent = value; break;
                    case Knob::stretch:    s.stretchPercent = value; break;
                    case Knob::swell:      s.swellSeconds = value; break;
                    case Knob::fade:       s.fadeSeconds = value; break;
                    case Knob::ampAttack:  s.ampEnv.attack = value; break;
                    case Knob::ampDecay:   s.ampEnv.decay = value; break;
                    case Knob::ampSustain: s.ampEnv.sustain = value / 100.0f; break;
                    case Knob::ampRelease: s.ampEnv.release = value; break;
                    case Knob::lfoRate:    s.lfoRateHz = value; break;
                    case Knob::lfoDepth:   s.lfoDepthPercent = value; break;
                    case Knob::glide:      s.glideSeconds = value; break;
                    case Knob::velocity:   s.velocityPercent = value; break;
                    case Knob::output:     s.outputDb = value; break;
                    case Knob::phases:
                    case Knob::lfoTarget:
                    case Knob::mode:       break;
                }

                return additive::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return k == Knob::fade ? additive::maxFade : 0.0f;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();
                return text.containsIgnoreCase ("ms") ? v / 1000.0f : v;
            });
    }

    juce::AudioParameterFloatAttributes levelText()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float value, int) { return additive::formatLevel (value); })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                return text.containsIgnoreCase ("off") ? additive::offDb : text.retainCharacters ("-0123456789.").getFloatValue();
            });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;

    /** The voices are rendered in steps this long: the harmonics' levels and the pitch move from one step to the next. */
    constexpr int stepSize = 32;
}

const char* BuiltInAdditive::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::brightness: return "brightness";
        case Knob::even:       return "even";
        case Knob::stretch:    return "stretch";
        case Knob::phases:     return "phases";
        case Knob::swell:      return "swell";
        case Knob::fade:       return "fade";
        case Knob::ampAttack:  return "ampAttack";
        case Knob::ampDecay:   return "ampDecay";
        case Knob::ampSustain: return "ampSustain";
        case Knob::ampRelease: return "ampRelease";
        case Knob::lfoRate:    return "lfoRate";
        case Knob::lfoDepth:   return "lfoDepth";
        case Knob::lfoTarget:  return "lfoTarget";
        case Knob::glide:      return "glide";
        case Knob::mode:       return "mode";
        case Knob::velocity:   return "velocity";
        case Knob::output:     return "output";
    }

    return "";
}

juce::String BuiltInAdditive::getPartialId (int n)
{
    return "h" + juce::String (n);
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInAdditive::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const additive::Settings d;

    auto add = [&layout] (Knob k, const juce::String& name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    auto choice = [&layout] (Knob k, const juce::String& name, const juce::StringArray& items, int value)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (k), 1 }, name, items, value));
    };

    for (int n = 1; n <= additive::numPartials; ++n)
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getPartialId (n), 1 }, "Harmonic " + juce::String (n),
                                                                 juce::NormalisableRange<float> { additive::offDb, 0.0f, 0.1f },
                                                                 d.levelsDb[(size_t) n - 1], levelText()));

    juce::NormalisableRange<float> glide { 0.0f, additive::maxGlide, 0.001f }, swell { 0.0f, additive::maxSwell, 0.001f };
    glide.setSkewForCentre (0.25f);
    swell.setSkewForCentre (0.4f);

    add (Knob::brightness, "Brightness", { additive::minBrightness, additive::maxBrightness, 0.1f }, d.brightnessDb);
    add (Knob::even, "Even harmonics", { 0.0f, 100.0f, 1.0f }, d.evenPercent);
    add (Knob::stretch, "Stretch", { 0.0f, 100.0f, 1.0f }, d.stretchPercent);
    choice (Knob::phases, "Phases", { "Aligned", "Spread" }, (int) d.phases);
    add (Knob::swell, "Swell", swell, d.swellSeconds);
    add (Knob::fade, "Fade", logRange (additive::minFade, additive::maxFade), d.fadeSeconds);
    add (Knob::ampAttack, "Amp attack", logRange (additive::minTime, additive::maxTime), d.ampEnv.attack);
    add (Knob::ampDecay, "Amp decay", logRange (additive::minTime, additive::maxTime), d.ampEnv.decay);
    add (Knob::ampSustain, "Amp sustain", { 0.0f, 100.0f, 1.0f }, d.ampEnv.sustain * 100.0f);
    add (Knob::ampRelease, "Amp release", logRange (additive::minTime, additive::maxTime), d.ampEnv.release);
    add (Knob::lfoRate, "LFO rate", logRange (additive::minLfoRate, additive::maxLfoRate), d.lfoRateHz);
    add (Knob::lfoDepth, "LFO depth", { 0.0f, 100.0f, 1.0f }, d.lfoDepthPercent);
    choice (Knob::lfoTarget, "LFO to", { "Pitch", "Brightness", "Even" }, (int) d.lfoTarget);
    add (Knob::glide, "Glide", glide, d.glideSeconds);
    choice (Knob::mode, "Voices", { "Poly", "Mono" }, (int) d.mode);
    add (Knob::velocity, "Velocity", { 0.0f, 100.0f, 1.0f }, d.velocityPercent);
    add (Knob::output, "Output", { additive::minOutput, additive::maxOutput, 0.1f }, d.outputDb);
    return layout;
}

BuiltInAdditive::BuiltInAdditive()
    : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItAdditive", createLayout())
{
    for (int k = 0; k < additive::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    for (int n = 1; n <= additive::numPartials; ++n)
        partialValues[(size_t) n - 1] = parameters.getRawParameterValue (getPartialId (n));

    for (auto& p : shownPartials)
        p = 1.0f;
}

juce::PluginDescription BuiltInAdditive::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Synth";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44447432;   // "DDt2"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

void BuiltInAdditive::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInAdditive::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

juce::RangedAudioParameter* BuiltInAdditive::getPartialParameter (int n) const
{
    return parameters.getParameter (getPartialId (n));
}

additive::Settings BuiltInAdditive::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };

    additive::Settings s;

    for (int n = 0; n < additive::numPartials; ++n)
        s.levelsDb[(size_t) n] = partialValues[(size_t) n]->load();

    s.brightnessDb = value (Knob::brightness);
    s.evenPercent = value (Knob::even);
    s.stretchPercent = value (Knob::stretch);
    s.phases = value (Knob::phases) >= 0.5f ? additive::Phases::spread : additive::Phases::aligned;
    s.swellSeconds = value (Knob::swell);
    s.fadeSeconds = value (Knob::fade);
    s.ampEnv = { value (Knob::ampAttack), value (Knob::ampDecay), value (Knob::ampSustain) / 100.0f, value (Knob::ampRelease) };
    s.lfoRateHz = value (Knob::lfoRate);
    s.lfoDepthPercent = value (Knob::lfoDepth);
    s.lfoTarget = (additive::LfoTarget) juce::jlimit (0, additive::numLfoTargets - 1, juce::roundToInt (value (Knob::lfoTarget)));
    s.glideSeconds = value (Knob::glide);
    s.mode = value (Knob::mode) >= 0.5f ? sub::Mode::mono : sub::Mode::poly;
    s.velocityPercent = value (Knob::velocity);
    s.outputDb = value (Knob::output);
    return s;
}

void BuiltInAdditive::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInAdditive::setPartialDb (int n, float db)
{
    if (auto* p = getPartialParameter (n))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (additive::offDb, 0.0f, db)));
}

void BuiltInAdditive::setSettings (const additive::Settings& s)
{
    for (int n = 1; n <= additive::numPartials; ++n)
        setPartialDb (n, s.levelsDb[(size_t) n - 1]);

    setKnob (Knob::brightness, s.brightnessDb);
    setKnob (Knob::even, s.evenPercent);
    setKnob (Knob::stretch, s.stretchPercent);
    setKnob (Knob::phases, s.phases == additive::Phases::spread ? 1.0f : 0.0f);
    setKnob (Knob::swell, s.swellSeconds);
    setKnob (Knob::fade, s.fadeSeconds);
    setKnob (Knob::ampAttack, s.ampEnv.attack);
    setKnob (Knob::ampDecay, s.ampEnv.decay);
    setKnob (Knob::ampSustain, s.ampEnv.sustain * 100.0f);
    setKnob (Knob::ampRelease, s.ampEnv.release);
    setKnob (Knob::lfoRate, s.lfoRateHz);
    setKnob (Knob::lfoDepth, s.lfoDepthPercent);
    setKnob (Knob::lfoTarget, (float) (int) s.lfoTarget);
    setKnob (Knob::glide, s.glideSeconds);
    setKnob (Knob::mode, s.mode == sub::Mode::mono ? 1.0f : 0.0f);
    setKnob (Knob::velocity, s.velocityPercent);
    setKnob (Knob::output, s.outputDb);
}

//==============================================================================
bool BuiltInAdditive::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet().isDisabled();
}

void BuiltInAdditive::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    clap.prepare (rate);
    outputGain.reset (rate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().outputDb));

    for (auto& v : voices)
    {
        v.amp.setSampleRate (rate);
        v.amp.reset();
        v.note = -1;
        v.held = false;
        v.gains.fill (0.0);
    }

    heldNotes.clear();
    lastPitch = -1.0;
    testNoteLeft = -1;
}

void BuiltInAdditive::startVoice (Voice& v, int note, float velocity, bool retrigger)
{
    const auto s = getSettings();
    const bool wasActive = v.isActive();

    v.note = note;
    v.velocity = velocity;
    v.held = true;
    v.startedAt = ++noteCounter;
    v.targetPitch = note;
    v.pitch = s.glideSeconds > 0.0005f && lastPitch >= 0.0 ? lastPitch : (double) note;

    if (! wasActive)
    {
        v.phases = additive::getStartPhases (s);

        v.gains.fill (0.0);
    }

    if (retrigger)
    {
        v.amp.noteOn();
        v.age = 0.0;
        v.releasedAt = -1.0;
    }

    lastPitch = note;
}

void BuiltInAdditive::noteOn (int note, float velocity)
{
    const auto s = getSettings();
    const bool othersHeld = ! heldNotes.empty();
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    heldNotes.push_back (note);

    if (s.mode == sub::Mode::mono)
    {
        auto& v = voices[0];

        for (size_t i = 1; i < voices.size(); ++i)
            if (voices[i].held)
            {
                voices[i].held = false;
                voices[i].amp.noteOff();
            }

        if (othersHeld && v.held && v.isActive())
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
        if (! v.isActive() && (chosen == nullptr || v.startedAt < chosen->startedAt))
            chosen = &v;

    if (chosen == nullptr)
        for (auto& v : voices)
            if (chosen == nullptr || v.startedAt < chosen->startedAt)
                chosen = &v;

    startVoice (*chosen, note, velocity, true);
}

void BuiltInAdditive::noteOff (int note)
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
            v.amp.noteOff();
            v.releasedAt = v.age;
        }
    }
}

void BuiltInAdditive::renderVoice (Voice& v, const additive::Settings& s, double norm, float lfo, float* out, float* fundamental, int numSamples)
{
    // Glide: most of the way (95%) in the glide time.
    if (s.glideSeconds > 0.0005f)
        v.pitch += (v.targetPitch - v.pitch) * (1.0 - std::exp (-3.0 * numSamples / (s.glideSeconds * rate)));
    else
        v.pitch = v.targetPitch;

    const double pitch = v.pitch + (s.lfoTarget == additive::LfoTarget::pitch ? lfo * s.lfoDepthPercent / 100.0 * additive::lfoPitchSemitones : 0.0);
    const double hz = sub::getNoteHz (pitch);
    const double limit = juce::jmin (20000.0, rate * 0.45);   // above it nobody hears it, and past half the rate it would fold back
    const bool alone = onlyFundamental.load (std::memory_order_relaxed);

    std::array<float, stepSize> amp {};
    const float level = additive::getVelocityScale (s, v.velocity) * additive::voiceGain;

    for (int i = 0; i < numSamples; ++i)
        amp[(size_t) i] = v.amp.getNextSample() * level;

    for (int n = 1; n <= additive::numPartials; ++n)
    {
        const auto index = (size_t) n - 1;
        const double partialHz = hz * additive::getRatio (n, s.stretchPercent);
        const double w = juce::MathConstants<double>::twoPi * partialHz / rate;
        double target = 0.0;

        if (partialHz < limit && ! (alone && n > 1))
            target = additive::getPartialGain (s, n, v.velocity, lfo) * norm * additive::getPartialEnvelope (s, n, v.age);

        const double from = v.gains[index];
        v.gains[index] = target;

        if (from > 1.0e-7 || target > 1.0e-7)
        {
            // A sine by turning a point around a circle: two multiplies and adds a sample, no sin().
            double sine = std::sin (v.phases[index]), cosine = std::cos (v.phases[index]);
            const double turnSin = std::sin (w), turnCos = std::cos (w);
            const double step = (target - from) / numSamples;
            double gain = from;
            float* const into = n == 1 ? fundamental : nullptr;

            for (int i = 0; i < numSamples; ++i)
            {
                gain += step;
                const float value = (float) (gain * sine) * amp[(size_t) i];
                out[i] += value;

                if (into != nullptr)
                    into[i] += value;

                const double nextSine = sine * turnCos + cosine * turnSin;
                cosine = cosine * turnCos - sine * turnSin;
                sine = nextSine;
            }
        }

        v.phases[index] = std::fmod (v.phases[index] + w * numSamples, juce::MathConstants<double>::twoPi);
    }

    v.age += numSamples / rate;
    v.lastAmp = amp[(size_t) numSamples - 1] / juce::jmax (1.0e-6f, level);
}

void BuiltInAdditive::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    const auto s = getSettings();
    const double norm = additive::getNormalisation (s);
    const juce::ADSR::Parameters ampParameters { s.ampEnv.attack, s.ampEnv.decay, s.ampEnv.sustain, s.ampEnv.release };

    for (auto& v : voices)
        v.amp.setParameters (ampParameters);

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    // The window's test note: on now, off after 0.8 s.
    if (clap.takeRequest())
    {
        noteOn (sub::testNote, 0.8f);
        testNoteLeft = (int) (0.8 * rate);
    }

    auto* left = buffer.getWritePointer (0);
    auto events = midi.begin();
    int pos = 0;
    float lfo = 0.0f;

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

        lfo = (float) std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
        lfoPhase += s.lfoRateHz * count / rate;
        lfoPhase -= std::floor (lfoPhase);

        std::array<float, stepSize> sum {}, fundamental {};

        for (auto& v : voices)
            if (v.isActive())
                renderVoice (v, s, norm, lfo, sum.data(), fundamental.data(), count);

        for (int i = 0; i < count; ++i)
        {
            const float gain = outputGain.getNextValue();
            left[pos + i] = sum[(size_t) i] * gain;

            chunkBefore = juce::jmax (chunkBefore, std::abs (fundamental[(size_t) i] * gain));
            chunkAfter = juce::jmax (chunkAfter, std::abs (sum[(size_t) i] * gain));

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

    lfoValue.store (lfo, std::memory_order_relaxed);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // The newest note, for the pictures.
    const Voice* newest = nullptr;

    for (auto& v : voices)
        if (v.isActive() && (newest == nullptr || v.startedAt > newest->startedAt))
            newest = &v;

    for (int n = 1; n <= additive::numPartials; ++n)
        shownPartials[(size_t) n - 1] = newest != nullptr ? (float) additive::getPartialEnvelope (s, n, newest->age) : 1.0f;

    ampLevel = newest != nullptr ? newest->lastAmp : 0.0f;
    noteAge = newest != nullptr ? (float) newest->age : -1.0f;
    releaseAge = newest != nullptr ? (float) newest->releasedAt : -1.0f;
    shownVelocity = newest != nullptr ? newest->velocity : 1.0f;
}

juce::AudioProcessorEditor* BuiltInAdditive::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInAdditive::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInAdditive::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
