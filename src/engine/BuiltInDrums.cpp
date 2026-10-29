#include "BuiltInDrums.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace drums
{
    namespace
    {
        constexpr double twoPi = juce::MathConstants<double>::twoPi;

        /** -60 dB is a thousandth: e^(-ln 1000 t / decay). */
        constexpr double ln1000 = 6.907755278982137;

        juce::String formatTime (double seconds)
        {
            if (seconds < 0.0095)
                return juce::String (seconds * 1000.0, 1) + " ms";

            return seconds < 1.0 ? juce::String (juce::roundToInt (seconds * 1000.0)) + " ms" : juce::String (seconds, 2) + " s";
        }

        juce::String formatHz (double hz)
        {
            return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
        }

        juce::String formatPercent (float value)
        {
            return juce::String (juce::roundToInt (value)) + "%";
        }

        juce::String withSign (double value, int decimals)
        {
            return (value > 0.0 ? "+" : "") + fx::formatNumber (value, decimals);
        }

        /** The jump of a square wave smoothed over one sample, so its harmonics above half the rate don't fold back (polyBLEP). */
        double polyBlep (double t, double dt) noexcept
        {
            if (t < dt)
            {
                t /= dt;
                return t + t - t * t - 1.0;
            }

            if (t > 1.0 - dt)
            {
                t = (t - 1.0) / dt;
                return t * t + t + t + 1.0;
            }

            return 0.0;
        }

        double square (double phase, double step) noexcept
        {
            double v = phase < 0.5 ? 1.0 : -1.0;
            v += polyBlep (phase, step);
            v -= polyBlep (std::fmod (phase + 0.5, 1.0), step);
            return v;
        }

        /** How much to turn filtered noise up so it stays about as loud whatever the filter keeps of it. */
        double getMakeUp (Filter f, double cutoffHz, double q, double sampleRate)
        {
            const double nyquist = sampleRate * 0.5;
            const double kept = [&]
            {
                switch (f)
                {
                    case Filter::lowPass:  return cutoffHz;
                    case Filter::bandPass: return cutoffHz / q;
                    case Filter::highPass: return nyquist - cutoffHz;
                }

                return nyquist;
            }();

            return juce::jlimit (1.0, 4.0, std::sqrt (nyquist / juce::jmax (1.0, kept)));
        }
    }

    Settings::Settings()
    {
        // An 808-style kit.
        auto set = [this] (Drum d, Sound s) { drums[(size_t) d] = s; };
        set (Drum::kick,      { 50.0f, 24.0f, 30.0f, 0.6f, 0.0f, 0.05f, 2000.0f, 25.0f, 0.0f });
        set (Drum::snare,     { 180.0f, 7.0f, 20.0f, 0.15f, 60.0f, 0.22f, 1500.0f, 15.0f, -2.0f });
        set (Drum::clap,      { 200.0f, 0.0f, 30.0f, 0.1f, 100.0f, 0.25f, 1200.0f, 0.0f, -2.0f });
        set (Drum::lowTom,    { 100.0f, 5.0f, 60.0f, 0.5f, 8.0f, 0.1f, 3000.0f, 10.0f, -3.0f });
        set (Drum::highTom,   { 160.0f, 5.0f, 60.0f, 0.4f, 8.0f, 0.1f, 3000.0f, 10.0f, -3.0f });
        set (Drum::closedHat, { 205.3f, 0.0f, 30.0f, 0.06f, 30.0f, 0.05f, 7000.0f, 0.0f, -6.0f });
        set (Drum::openHat,   { 205.3f, 0.0f, 30.0f, 0.45f, 30.0f, 0.4f, 7000.0f, 0.0f, -6.0f });
        set (Drum::cymbal,    { 205.3f, 0.0f, 30.0f, 1.8f, 30.0f, 1.5f, 5000.0f, 0.0f, -8.0f });
        set (Drum::cowbell,   { 540.0f, 0.0f, 30.0f, 0.3f, 0.0f, 0.05f, 1500.0f, 0.0f, -6.0f });
    }

    bool isDrumKnob (Knob k)
    {
        return (int) k < numDrumKnobs;
    }

    juce::String getDrumName (Drum d)
    {
        switch (d)
        {
            case Drum::kick:      return "Kick";
            case Drum::snare:     return "Snare";
            case Drum::clap:      return "Clap";
            case Drum::lowTom:    return "Low tom";
            case Drum::highTom:   return "High tom";
            case Drum::closedHat: return "Closed hi-hat";
            case Drum::openHat:   return "Open hi-hat";
            case Drum::cymbal:    return "Cymbal";
            case Drum::cowbell:   return "Cowbell";
        }

        return {};
    }

    Body getBody (Drum d)
    {
        switch (d)
        {
            case Drum::kick:
            case Drum::lowTom:
            case Drum::highTom:   return Body::sine;
            case Drum::snare:     return Body::head;
            case Drum::clap:      return Body::none;
            case Drum::closedHat:
            case Drum::openHat:
            case Drum::cymbal:    return Body::metal;
            case Drum::cowbell:   return Body::bell;
        }

        return Body::sine;
    }

    Filter getFilter (Drum d)
    {
        switch (d)
        {
            case Drum::kick:
            case Drum::lowTom:
            case Drum::highTom:   return Filter::lowPass;
            case Drum::clap:
            case Drum::cowbell:   return Filter::bandPass;
            case Drum::snare:
            case Drum::closedHat:
            case Drum::openHat:
            case Drum::cymbal:    return Filter::highPass;
        }

        return Filter::lowPass;
    }

    float getFilterQ (Drum d)
    {
        switch (getFilter (d))
        {
            case Filter::bandPass: return d == Drum::cowbell ? 2.5f : 1.5f;
            case Filter::lowPass:
            case Filter::highPass: return 0.707f;
        }

        return 0.707f;
    }

    int getNote (Drum d)
    {
        switch (d)
        {
            case Drum::kick:      return 36;
            case Drum::snare:     return 38;
            case Drum::clap:      return 39;
            case Drum::lowTom:    return 45;
            case Drum::highTom:   return 50;
            case Drum::closedHat: return 42;
            case Drum::openHat:   return 46;
            case Drum::cymbal:    return 49;
            case Drum::cowbell:   return 56;
        }

        return 36;
    }

    std::optional<Drum> getDrumForNote (int note)
    {
        switch (note)
        {
            case 35: case 36:                   return Drum::kick;
            case 37: case 38: case 40:          return Drum::snare;
            case 39:                            return Drum::clap;
            case 41: case 43: case 45:          return Drum::lowTom;
            case 47: case 48: case 50:          return Drum::highTom;
            case 42: case 44:                   return Drum::closedHat;
            case 46:                            return Drum::openHat;
            case 49: case 51: case 52: case 55:
            case 57: case 59:                   return Drum::cymbal;
            case 56:                            return Drum::cowbell;
            default:                            return std::nullopt;
        }
    }

    bool isUsed (Drum d, Knob k)
    {
        const auto body = getBody (d);

        switch (k)
        {
            case Knob::tune:       return body != Body::none;
            case Knob::bend:
            case Knob::bendTime:   return body == Body::sine || body == Body::head;
            case Knob::decay:      return body != Body::none;
            case Knob::noise:      return body != Body::none;
            case Knob::noiseDecay:
            case Knob::tone:
            case Knob::click:
            case Knob::level:
            case Knob::choke:
            case Knob::velocity:
            case Knob::output:     return true;
        }

        return true;
    }

    const std::array<double, 6>& getMetalRatios()
    {
        static const std::array<double, 6> ratios { 1.0, 304.4 / 205.3, 369.6 / 205.3, 522.7 / 205.3, 540.0 / 205.3, 800.0 / 205.3 };
        return ratios;
    }

    double getPitchAt (const Sound& s, double seconds)
    {
        const double bend = s.bendSemitones / 12.0 * std::exp (-juce::jmax (0.0, seconds) / juce::jmax (0.0005, s.bendMs / 1000.0));
        return s.tuneHz * std::pow (2.0, bend);
    }

    double getEnvelope (double decaySeconds, double seconds)
    {
        return seconds < 0.0 ? 0.0 : std::exp (-ln1000 * seconds / juce::jmax (0.001, decaySeconds));
    }

    double getClapEnvelope (const Sound& s, double seconds)
    {
        if (seconds < 0.0)
            return 0.0;

        const double bursts = clapBursts * clapBurstSeconds;

        if (seconds < bursts)
            return std::exp (-std::fmod (seconds, clapBurstSeconds) / 0.0035);

        return 0.7 * getEnvelope (s.noiseDecaySeconds, seconds - bursts);
    }

    double getLength (const Sound& s, Drum d)
    {
        if (getBody (d) == Body::none)
            return clapBursts * clapBurstSeconds + s.noiseDecaySeconds + 0.01;

        double length = 0.01;

        if (s.noisePercent < 99.5f)
            length = juce::jmax (length, (double) s.decaySeconds);

        if (s.noisePercent > 0.5f)
            length = juce::jmax (length, (double) s.noiseDecaySeconds);

        return length + 0.01;
    }

    float getVelocityScale (const Settings& s, float velocity)
    {
        return 1.0f - juce::jlimit (0.0f, 1.0f, s.velocityPercent / 100.0f) * (1.0f - juce::jlimit (0.0f, 1.0f, velocity));
    }

    //==============================================================================
    double Voice::Svf::run (double x, double cutoff, double damping, Filter f) noexcept
    {
        // A state-variable filter (topology-preserving transform), as in DawIt Subtractive.
        const double a1 = 1.0 / (1.0 + cutoff * (cutoff + damping)), a2 = cutoff * a1, a3 = cutoff * a2;
        const double v3 = x - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;

        switch (f)
        {
            case Filter::lowPass:  return v2;
            case Filter::bandPass: return damping * v1;
            case Filter::highPass: return x - damping * v1 - v2;
        }

        return x;
    }

    void Voice::start (const Sound& s, Drum d, float hitGain, double sampleRate, juce::uint32 noiseSeed)
    {
        sound = s;
        drum = d;
        body = getBody (d);
        filter = getFilter (d);
        rate = sampleRate > 0.0 ? sampleRate : 44100.0;
        age = 0.0;
        length = getLength (s, d);
        const double cutoff = juce::jlimit (20.0, rate * 0.45, (double) s.toneHz);
        g = std::tan (juce::MathConstants<double>::pi * cutoff / rate);
        k = 1.0 / getFilterQ (d);
        noiseMakeUp = getMakeUp (filter, cutoff, getFilterQ (d), rate);
        phase = phase2 = 0.0;
        metalPhases.fill (0.0);
        toneFilter = {};
        noiseFilter = {};
        noiseState = noiseSeed == 0 ? 1u : noiseSeed;
        gain = hitGain;
        chokeGain = 1.0f;
        chokeStep = 0.0f;
        active = true;
    }

    double Voice::nextNoise() noexcept
    {
        // A xorshift: fast, and the same seed gives the same noise.
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return noiseState / 2147483648.0 - 1.0;
    }

    float Voice::next (float* tonePart, float* noisePart) noexcept
    {
        if (! active)
        {
            if (tonePart != nullptr) *tonePart = 0.0f;
            if (noisePart != nullptr) *noisePart = 0.0f;
            return 0.0f;
        }

        double tone = 0.0;

        switch (body)
        {
            case Body::sine:
            case Body::head:
            {
                const double hz = getPitchAt (sound, age);
                tone = std::sin (twoPi * phase);
                phase += hz / rate;

                if (body == Body::head)
                {
                    // A drumhead's second mode rings at 1.59 times its first (an ideal membrane's).
                    tone = 0.7 * tone + 0.3 * std::sin (twoPi * phase2);
                    phase2 += hz * 1.59 / rate;
                    phase2 -= std::floor (phase2);
                }

                phase -= std::floor (phase);
                break;
            }

            case Body::metal:
            case Body::bell:
            {
                const auto& ratios = getMetalRatios();
                const int count = body == Body::metal ? 6 : 2;

                for (int i = 0; i < count; ++i)
                {
                    const double ratio = body == Body::metal ? ratios[(size_t) i] : (i == 0 ? 1.0 : bellRatio);
                    const double step = sound.tuneHz * ratio / rate;
                    auto& p = metalPhases[(size_t) i];
                    tone += square (p, step);
                    p += step;
                    p -= std::floor (p);
                }

                // The squares through the drum's filter, turned up as the noise is, so the metal stays as loud as the noise.
                tone = toneFilter.run (tone / count, g, k, filter) * noiseMakeUp * (body == Body::metal ? 1.5 : 1.0);
                break;
            }

            case Body::none:
                break;
        }

        const double mix = body == Body::none ? 1.0 : juce::jlimit (0.0, 1.0, sound.noisePercent / 100.0);
        const double toneOut = tone * (1.0 - mix) * getEnvelope (sound.decaySeconds, age);

        const double white = nextNoise();
        const double noiseEnvelope = drum == Drum::clap ? getClapEnvelope (sound, age) : getEnvelope (sound.noiseDecaySeconds, age);
        double noiseOut = noiseFilter.run (white, g, k, filter) * noiseMakeUp * mix * noiseEnvelope;
        noiseOut += sound.clickPercent / 100.0 * white * std::exp (-age / 0.0006);

        const float scale = gain * chokeGain;

        if (tonePart != nullptr) *tonePart = (float) toneOut * scale;
        if (noisePart != nullptr) *noisePart = (float) noiseOut * scale;

        if (chokeStep > 0.0f)
            chokeGain = juce::jmax (0.0f, chokeGain - chokeStep);

        age += 1.0 / rate;

        if (age > length || (chokeStep > 0.0f && chokeGain <= 0.0f))
            active = false;

        return (float) (toneOut + noiseOut) * scale;
    }

    void Voice::choke() noexcept
    {
        if (active)
            chokeStep = (float) (1.0 / (0.005 * rate));
    }

    float Voice::getLevel() const noexcept
    {
        if (! active)
            return 0.0f;

        const double mix = body == Body::none ? 1.0 : sound.noisePercent / 100.0;
        const double noiseEnvelope = drum == Drum::clap ? getClapEnvelope (sound, age) : getEnvelope (sound.noiseDecaySeconds, age);
        return (float) juce::jmax ((1.0 - mix) * getEnvelope (sound.decaySeconds, age), mix * noiseEnvelope) * chokeGain;
    }

    Hit render (const Settings& s, Drum d, double sampleRate, double seconds, float velocity)
    {
        Voice voice;
        voice.start (s.drums[(size_t) d], d, voiceGain * juce::Decibels::decibelsToGain (s.drums[(size_t) d].levelDb) * getVelocityScale (s, velocity),
                     sampleRate);

        Hit hit;
        const auto count = (size_t) juce::jmax (0, (int) (seconds * sampleRate));
        hit.tone.resize (count);
        hit.noise.resize (count);

        for (size_t i = 0; i < count; ++i)
            voice.next (&hit.tone[i], &hit.noise[i]);

        return hit;
    }

    //==============================================================================
    juce::String getKnobName (Knob k)
    {
        switch (k)
        {
            case Knob::tune:       return "Tune";
            case Knob::bend:       return "Bend";
            case Knob::bendTime:   return "Bend time";
            case Knob::decay:      return "Decay";
            case Knob::noise:      return "Noise";
            case Knob::noiseDecay: return "Noise decay";
            case Knob::tone:       return "Tone";
            case Knob::click:      return "Click";
            case Knob::level:      return "Level";
            case Knob::choke:      return "Choke";
            case Knob::velocity:   return "Velocity";
            case Knob::output:     return "Output";
        }

        return {};
    }

    juce::String formatKnob (Knob k, const Settings& s, Drum d)
    {
        const auto& o = s.drums[(size_t) d];

        switch (k)
        {
            case Knob::tune:       return formatHz (o.tuneHz);
            case Knob::bend:       return o.bendSemitones < 0.05f ? juce::String ("Off") : "+" + juce::String (juce::roundToInt (o.bendSemitones)) + " st";
            case Knob::bendTime:   return formatTime (o.bendMs / 1000.0);
            case Knob::decay:      return formatTime (o.decaySeconds);
            case Knob::noise:      return formatPercent (o.noisePercent);
            case Knob::noiseDecay: return formatTime (o.noiseDecaySeconds);
            case Knob::tone:       return formatHz (o.toneHz);
            case Knob::click:      return formatPercent (o.clickPercent);
            case Knob::level:      return withSign (o.levelDb, 1) + " dB";
            case Knob::choke:      return s.choke ? "On" : "Off";
            case Knob::velocity:   return formatPercent (s.velocityPercent);
            case Knob::output:     return withSign (s.outputDb, 1) + " dB";
        }

        return {};
    }

    //==============================================================================
    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            auto add = [&list] (const juce::String& name, const juce::String& why, Settings s, std::initializer_list<std::pair<Drum, const char*>> drumNotes,
                                std::initializer_list<std::pair<Knob, const char*>> notes)
            {
                Preset p;
                p.name = name;
                p.why = why;
                p.settings = s;

                for (auto& [drum, note] : drumNotes)
                    p.drums[(size_t) drum] = juce::CharPointer_UTF8 (note);

                for (auto& [knob, note] : notes)
                    p.notes[(size_t) knob] = juce::CharPointer_UTF8 (note);

                list.push_back (p);
            };

            auto at = [] (Settings& s, Drum d) -> Sound& { return s.drums[(size_t) d]; };

            add ("Init: an 808-style kit", "The sounds of Roland's TR-808 drum machine (1980), made the way it made them: a kick that's a falling "
                                           "sine, a snare of tone and noise, metal from six square waves. Every drum machine sound since builds "
                                           "on these.",
                 {}, { { Drum::kick, "A sine that starts 2 octaves up (+24 st) and falls to 50 Hz in about 30 ms: the falling pitch is the punch, "
                                     "the long low sine the boom." },
                       { Drum::snare, "A short tone at 180 Hz with the drumhead's second mode above it, and 60% noise above 1.5 kHz for the "
                                      "rattle of the snares under the drum." },
                       { Drum::clap, "Three bursts of noise 11 ms apart and a short tail: several hands that don't quite clap together." },
                       { Drum::lowTom, "A sine at 100 Hz that drops a little after the hit, as a tom's skin does." },
                       { Drum::highTom, "The same at 160 Hz." },
                       { Drum::closedHat, "Six square waves that clash like metal, through a high-pass at 7 kHz, gone in 60 ms." },
                       { Drum::openHat, "The same metal ringing for 450 ms; a closed hi-hat cuts it off." },
                       { Drum::cymbal, "The metal again, lower (a high-pass at 5 kHz) and ringing for almost 2 seconds." },
                       { Drum::cowbell, "Two square waves at 540 and 800 Hz through a band-pass: the 808's famous cowbell." } },
                 { { Knob::choke, "On: a closed hi-hat cuts off an open one, as the two cymbals of a hi-hat close on each other." } });

            Settings nine;
            at (nine, Drum::kick) = { 55.0f, 30.0f, 18.0f, 0.4f, 0.0f, 0.05f, 2000.0f, 60.0f, 0.0f };
            at (nine, Drum::snare) = { 200.0f, 9.0f, 15.0f, 0.12f, 70.0f, 0.18f, 3000.0f, 30.0f, -2.0f };
            at (nine, Drum::clap).noiseDecaySeconds = 0.35f;
            at (nine, Drum::closedHat).noisePercent = 70.0f;
            at (nine, Drum::closedHat).toneHz = 9000.0f;
            at (nine, Drum::openHat).noisePercent = 70.0f;
            at (nine, Drum::openHat).toneHz = 9000.0f;
            at (nine, Drum::cymbal).noisePercent = 60.0f;
            add ("909-style", "Closer to Roland's TR-909 (1983), the drum machine of house and techno: a shorter kick with a hard click on top, "
                              "a brighter snare, and hi-hats that are more noise than metal (the 909 played recorded cymbals).",
                 nine, { { Drum::kick, "A faster bend (18 ms) from higher up, a shorter decay and a 60% click: it punches through a mix instead of booming." },
                         { Drum::snare, "Tuned higher, with more noise above 3 kHz: a crack rather than a thud." },
                         { Drum::closedHat, "70% noise: bright and hissy, nearer the 909's sampled hats." } },
                 {});

            Settings trap;
            at (trap, Drum::kick) = { 43.65f, 12.0f, 40.0f, 2.5f, 0.0f, 0.05f, 2000.0f, 20.0f, 0.0f };
            at (trap, Drum::clap) = { 200.0f, 0.0f, 30.0f, 0.1f, 100.0f, 0.3f, 1600.0f, 0.0f, 0.0f };
            at (trap, Drum::snare).levelDb = -6.0f;
            at (trap, Drum::closedHat).decaySeconds = 0.04f;
            at (trap, Drum::closedHat).noiseDecaySeconds = 0.035f;
            add ("Trap: a long 808", "The kick of trap and hip-hop: an 808 kick with a decay of seconds, so it's also the bass line. Tune it to "
                                     "the song's key, and play the bass notes on the kick's note with a pitch bend, or tune each kick.",
                 trap, { { Drum::kick, "Tuned to F1 (43.65 Hz) and ringing for 2.5 seconds: a sub-bass note as much as a drum. Tune it to the root of "
                                       "the song's key so it doesn't clash with the chords." },
                         { Drum::clap, "Louder and brighter than the snare, on beats 2 and 4." },
                         { Drum::closedHat, "Very short (40 ms), for the fast rolls of trap hi-hats." } },
                 {});

            Settings techno;
            at (techno, Drum::kick) = { 48.0f, 36.0f, 22.0f, 0.5f, 0.0f, 0.05f, 2000.0f, 70.0f, 0.0f };
            at (techno, Drum::clap).noiseDecaySeconds = 0.6f;
            at (techno, Drum::clap).toneHz = 1000.0f;
            at (techno, Drum::openHat).decaySeconds = 0.25f;
            at (techno, Drum::openHat).noiseDecaySeconds = 0.2f;
            at (techno, Drum::cymbal).decaySeconds = 0.8f;
            at (techno, Drum::cymbal).noiseDecaySeconds = 0.6f;
            at (techno, Drum::cymbal).toneHz = 8000.0f;
            techno.velocityPercent = 40.0f;
            add ("Techno", "A kick on every beat that has to cut through on a big system: a hard click and a short boom, an open hi-hat on the "
                           "off-beats, and a clap with a long tail like a reverb.",
                 techno, { { Drum::kick, "Bend +36 st, click 70%: the attack carries in a club even when the low end is masked." },
                           { Drum::openHat, "250 ms: short enough to sit between the kicks on the off-beats." },
                           { Drum::cymbal, "Short and bright, closer to a ride for steady eighths." } },
                 { { Knob::velocity, "40%: machines play every hit about as hard." } });

            Settings boomBap;
            at (boomBap, Drum::kick) = { 58.0f, 14.0f, 35.0f, 0.35f, 6.0f, 0.06f, 600.0f, 15.0f, 0.0f };
            at (boomBap, Drum::snare) = { 170.0f, 5.0f, 25.0f, 0.2f, 50.0f, 0.25f, 900.0f, 10.0f, -1.0f };
            at (boomBap, Drum::closedHat).toneHz = 5000.0f;
            at (boomBap, Drum::closedHat).noisePercent = 40.0f;
            at (boomBap, Drum::openHat).toneHz = 5000.0f;
            boomBap.velocityPercent = 90.0f;
            add ("Boom bap", "The dusty, warm drums of 1990s hip-hop, which were sampled from old records: a rounder kick, a fat snare with "
                             "its noise lower down, and darker hi-hats.",
                 boomBap, { { Drum::snare, "Noise from 900 Hz up: a thick snare rather than a bright one." },
                            { Drum::closedHat, "Darker (a high-pass at 5 kHz): like a sample played from vinyl." } },
                 { { Knob::velocity, "90%: soft and hard hits differ a lot, so a played groove breathes." } });

            Settings electro;
            at (electro, Drum::kick) = { 45.0f, 48.0f, 10.0f, 0.5f, 0.0f, 0.05f, 2000.0f, 40.0f, 0.0f };
            at (electro, Drum::lowTom) = { 110.0f, 24.0f, 120.0f, 0.7f, 0.0f, 0.1f, 3000.0f, 0.0f, -3.0f };
            at (electro, Drum::highTom) = { 180.0f, 24.0f, 120.0f, 0.6f, 0.0f, 0.1f, 3000.0f, 0.0f, -3.0f };
            at (electro, Drum::cowbell).levelDb = -3.0f;
            add ("Electro: zaps and pews", "Early 1980s electro and disco: a kick that zaps down four octaves, and toms that \"pew\" down two "
                                           "octaves like a Syndrum, with the cowbell up front.",
                 electro, { { Drum::kick, "Bend +48 st in 10 ms: four octaves in the blink of an eye, heard as a zap." },
                            { Drum::lowTom, "Bend +24 st over 120 ms: the falling pitch is slow enough to hear as a slide, the Syndrum's \"pew\"." },
                            { Drum::highTom, "The same, higher." } },
                 {});

            Settings pop;
            at (pop, Drum::kick) = { 60.0f, 20.0f, 20.0f, 0.3f, 0.0f, 0.05f, 2000.0f, 45.0f, 0.0f };
            at (pop, Drum::snare) = { 220.0f, 8.0f, 15.0f, 0.1f, 65.0f, 0.15f, 2500.0f, 25.0f, -1.0f };
            at (pop, Drum::closedHat).decaySeconds = 0.05f;
            at (pop, Drum::openHat).decaySeconds = 0.3f;
            at (pop, Drum::openHat).noiseDecaySeconds = 0.25f;
            add ("Tight pop", "Short, punchy drums that leave room for the vocal and the bass: every decay a little shorter, the kick tuned "
                              "higher so it's heard on small speakers.",
                 pop, { { Drum::kick, "Tuned to 60 Hz with a 300 ms decay: punch more than boom, and out of the bass's way." },
                        { Drum::snare, "A short, bright crack at 220 Hz." } },
                 {});

            return list;
        }();

        return presets;
    }
} // namespace drums

//==============================================================================
using drums::Drum;
using drums::Knob;

namespace
{
    juce::NormalisableRange<float> logRange (float low, float high)
    {
        return { low, high,
                 [] (float start, float end, float t) { return start * std::pow (end / start, t); },
                 [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); } };
    }

    void apply (drums::Settings& s, Knob k, Drum d, float value)
    {
        auto& o = s.drums[(size_t) d];

        switch (k)
        {
            case Knob::tune:       o.tuneHz = value; break;
            case Knob::bend:       o.bendSemitones = value; break;
            case Knob::bendTime:   o.bendMs = value; break;
            case Knob::decay:      o.decaySeconds = value; break;
            case Knob::noise:      o.noisePercent = value; break;
            case Knob::noiseDecay: o.noiseDecaySeconds = value; break;
            case Knob::tone:       o.toneHz = value; break;
            case Knob::click:      o.clickPercent = value; break;
            case Knob::level:      o.levelDb = value; break;
            case Knob::choke:      s.choke = value >= 0.5f; break;
            case Knob::velocity:   s.velocityPercent = value; break;
            case Knob::output:     s.outputDb = value; break;
        }
    }

    float valueOf (const drums::Settings& s, Knob k, Drum d)
    {
        const auto& o = s.drums[(size_t) d];

        switch (k)
        {
            case Knob::tune:       return o.tuneHz;
            case Knob::bend:       return o.bendSemitones;
            case Knob::bendTime:   return o.bendMs;
            case Knob::decay:      return o.decaySeconds;
            case Knob::noise:      return o.noisePercent;
            case Knob::noiseDecay: return o.noiseDecaySeconds;
            case Knob::tone:       return o.toneHz;
            case Knob::click:      return o.clickPercent;
            case Knob::level:      return o.levelDb;
            case Knob::choke:      return s.choke ? 1.0f : 0.0f;
            case Knob::velocity:   return s.velocityPercent;
            case Knob::output:     return s.outputDb;
        }

        return 0.0f;
    }

    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float value, int)
            {
                drums::Settings s;
                apply (s, k, Drum::kick, value);
                return drums::formatKnob (k, s, Drum::kick);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return 0.0f;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();

                if (text.containsIgnoreCase ("khz"))
                    return v * 1000.0f;

                // The bend time is held in milliseconds, the decays in seconds.
                const bool inMs = text.containsIgnoreCase ("ms");

                if (k == Knob::bendTime)
                    return inMs ? v : v * 1000.0f;

                return inMs ? v / 1000.0f : v;
            });
    }

    juce::String getDrumId (Drum d)
    {
        switch (d)
        {
            case Drum::kick:      return "kick";
            case Drum::snare:     return "snare";
            case Drum::clap:      return "clap";
            case Drum::lowTom:    return "lowTom";
            case Drum::highTom:   return "highTom";
            case Drum::closedHat: return "closedHat";
            case Drum::openHat:   return "openHat";
            case Drum::cymbal:    return "cymbal";
            case Drum::cowbell:   return "cowbell";
        }

        return "kick";
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;
}

juce::String BuiltInDrums::getParameterId (Knob k, Drum d)
{
    const auto prefix = getDrumId (d);

    switch (k)
    {
        case Knob::tune:       return prefix + "Tune";
        case Knob::bend:       return prefix + "Bend";
        case Knob::bendTime:   return prefix + "BendTime";
        case Knob::decay:      return prefix + "Decay";
        case Knob::noise:      return prefix + "Noise";
        case Knob::noiseDecay: return prefix + "NoiseDecay";
        case Knob::tone:       return prefix + "Tone";
        case Knob::click:      return prefix + "Click";
        case Knob::level:      return prefix + "Level";
        case Knob::choke:      return "choke";
        case Knob::velocity:   return "velocity";
        case Knob::output:     return "output";
    }

    return {};
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInDrums::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const drums::Settings d;

    auto add = [&layout] (Knob k, Drum drum, const juce::String& name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k, drum), 1 }, name, range, value, knobText (k)));
    };

    for (int i = 0; i < drums::numDrums; ++i)
    {
        const auto drum = (Drum) i;
        const auto& o = d.drums[(size_t) i];
        const juce::String name = drums::getDrumName (drum) + " ";
        add (Knob::tune, drum, name + "tune", logRange (drums::minTune, drums::maxTune), juce::jmax (drums::minTune, o.tuneHz));
        add (Knob::bend, drum, name + "bend", { 0.0f, drums::maxBend, 0.1f }, o.bendSemitones);
        add (Knob::bendTime, drum, name + "bend time", logRange (drums::minBendMs, drums::maxBendMs), o.bendMs);
        add (Knob::decay, drum, name + "decay", logRange (drums::minDecay, drums::maxDecay), o.decaySeconds);
        add (Knob::noise, drum, name + "noise", { 0.0f, 100.0f, 1.0f }, o.noisePercent);
        add (Knob::noiseDecay, drum, name + "noise decay", logRange (drums::minDecay, drums::maxDecay), o.noiseDecaySeconds);
        add (Knob::tone, drum, name + "tone", logRange (drums::minTone, drums::maxTone), o.toneHz);
        add (Knob::click, drum, name + "click", { 0.0f, 100.0f, 1.0f }, o.clickPercent);
        add (Knob::level, drum, name + "level", { drums::minLevel, drums::maxLevel, 0.1f }, o.levelDb);
    }

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::choke), 1 }, "Choke",
                                                              juce::StringArray { "Off", "On" }, d.choke ? 1 : 0));
    add (Knob::velocity, Drum::kick, "Velocity", { 0.0f, 100.0f, 1.0f }, d.velocityPercent);
    add (Knob::output, Drum::kick, "Output", { drums::minOutput, drums::maxOutput, 0.1f }, d.outputDb);
    return layout;
}

BuiltInDrums::BuiltInDrums()
    : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItDrums", createLayout())
{
    for (int d = 0; d < drums::numDrums; ++d)
        for (int k = 0; k < drums::numDrumKnobs; ++k)
            drumValues[(size_t) d][(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k, (Drum) d));

    for (int k = drums::numDrumKnobs; k < drums::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    for (auto& l : shownLevels)
        l = 0.0f;
}

juce::PluginDescription BuiltInDrums::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Drum";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44447434;   // "DDt4"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

void BuiltInDrums::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInDrums::getKnobParameter (Knob k, Drum d) const
{
    return parameters.getParameter (getParameterId (k, d));
}

drums::Settings BuiltInDrums::getSettings() const
{
    drums::Settings s;

    for (int d = 0; d < drums::numDrums; ++d)
        for (int k = 0; k < drums::numDrumKnobs; ++k)
            apply (s, (Knob) k, (Drum) d, drumValues[(size_t) d][(size_t) k]->load());

    for (int k = drums::numDrumKnobs; k < drums::numKnobs; ++k)
        apply (s, (Knob) k, Drum::kick, knobValues[(size_t) k]->load());

    return s;
}

void BuiltInDrums::setKnob (Knob k, float value, Drum d)
{
    if (auto* p = getKnobParameter (k, d))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInDrums::setSettings (const drums::Settings& s)
{
    for (int d = 0; d < drums::numDrums; ++d)
        for (int k = 0; k < drums::numDrumKnobs; ++k)
            setKnob ((Knob) k, valueOf (s, (Knob) k, (Drum) d), (Drum) d);

    for (int k = drums::numDrumKnobs; k < drums::numKnobs; ++k)
        setKnob ((Knob) k, valueOf (s, (Knob) k, Drum::kick));
}

double BuiltInDrums::getTailLengthSeconds() const
{
    const auto s = getSettings();
    double longest = 0.0;

    for (int d = 0; d < drums::numDrums; ++d)
        longest = juce::jmax (longest, drums::getLength (s.drums[(size_t) d], (Drum) d));

    return longest;
}

//==============================================================================
bool BuiltInDrums::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet().isDisabled();
}

void BuiltInDrums::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    clap.prepare (rate);
    outputGain.reset (rate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().outputDb));

    for (auto& v : voices)
        v = {};

    startedAt.fill (0);
}

void BuiltInDrums::hit (Drum d, float velocity, const drums::Settings& s)
{
    // A closed hi-hat cuts off an open one (and a new hit on the stand cuts off the last), as the two cymbals of a hi-hat close.
    const bool hat = d == Drum::closedHat || d == Drum::openHat;

    if (hat && s.choke)
        for (auto& v : voices)
            if (v.isActive() && (v.getDrum() == Drum::closedHat || v.getDrum() == Drum::openHat))
                v.choke();

    // A free voice, or the oldest one.
    int chosen = -1;

    for (int i = 0; i < numVoices; ++i)
        if (! voices[(size_t) i].isActive() && (chosen < 0 || startedAt[(size_t) i] < startedAt[(size_t) chosen]))
            chosen = i;

    if (chosen < 0)
        for (int i = 0; i < numVoices; ++i)
            if (chosen < 0 || startedAt[(size_t) i] < startedAt[(size_t) chosen])
                chosen = i;

    seed = seed * 1664525u + 1013904223u;
    const auto& sound = s.drums[(size_t) d];
    const float gain = drums::voiceGain * juce::Decibels::decibelsToGain (sound.levelDb) * drums::getVelocityScale (s, velocity);
    voices[(size_t) chosen].start (sound, d, gain, rate, seed);
    startedAt[(size_t) chosen] = ++hitCounter;
}

void BuiltInDrums::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    const auto s = getSettings();
    const auto selected = (Drum) juce::jlimit (0, drums::numDrums - 1, selectedDrum.load());
    const bool alone = onlySelected.load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    // The window's test hit: the selected drum.
    if (clap.takeRequest())
        hit (selected, 0.9f, s);

    auto* left = buffer.getWritePointer (0);
    auto events = midi.begin();

    for (int i = 0; i < numSamples; ++i)
    {
        // Drums are one-shots: a note-on hits, how long the key is held doesn't matter.
        for (; events != midi.end() && (*events).samplePosition <= i; ++events)
        {
            const auto m = (*events).getMessage();

            if (m.isNoteOn())
                if (const auto d = drums::getDrumForNote (m.getNoteNumber()))
                    hit (*d, m.getFloatVelocity(), s);
        }

        float sum = 0.0f, mine = 0.0f;

        for (auto& v : voices)
        {
            if (! v.isActive())
                continue;

            const bool isSelected = v.getDrum() == selected;
            const float value = v.next();

            if (isSelected)
                mine += value;

            if (isSelected || ! alone)
                sum += value;
        }

        const float gain = outputGain.getNextValue();
        left[i] = sum * gain;

        chunkBefore = juce::jmax (chunkBefore, std::abs (mine * gain));
        chunkAfter = juce::jmax (chunkAfter, std::abs (sum * gain));

        if (++chunkFill == chunkSize)
        {
            history.push ({ juce::Decibels::gainToDecibels (chunkBefore, -100.0f), juce::Decibels::gainToDecibels (chunkAfter, -100.0f) });
            chunkBefore = chunkAfter = 0.0f;
            chunkFill = 0;
        }
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // How loud each drum is now, for the pads, and how old the selected drum's newest hit is.
    std::array<float, drums::numDrums> levels {};
    float newestAge = -1.0f;
    juce::uint64 newest = 0;

    for (int i = 0; i < numVoices; ++i)
    {
        const auto& v = voices[(size_t) i];

        if (! v.isActive())
            continue;

        auto& level = levels[(size_t) v.getDrum()];
        level = juce::jmax (level, v.getLevel());

        if (v.getDrum() == selected && startedAt[(size_t) i] > newest)
        {
            newest = startedAt[(size_t) i];
            newestAge = (float) v.getAge();
        }
    }

    for (size_t d = 0; d < levels.size(); ++d)
        shownLevels[d] = levels[d];

    selectedAge = newestAge;
}

juce::AudioProcessorEditor* BuiltInDrums::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInDrums::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInDrums::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
