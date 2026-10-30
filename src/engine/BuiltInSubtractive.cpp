#include "BuiltInSubtractive.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace sub
{
    namespace
    {
        constexpr double pi = juce::MathConstants<double>::pi;
        constexpr float flatQ = 0.7071f;

        /** One 2-pole stage of the analog filter, at x = hz / cutoff. */
        std::complex<double> getStage (FilterType type, double x, double q)
        {
            const std::complex<double> s (0.0, x);
            const auto denominator = s * s + s / q + 1.0;

            switch (type)
            {
                case FilterType::lowPass:  return 1.0 / denominator;
                case FilterType::bandPass: return (s / q) / denominator;
                case FilterType::highPass: return (s * s) / denominator;
            }

            return 1.0;
        }

        juce::String formatHz (double hz)
        {
            return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
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
    }

    juce::String getWaveName (Wave w)
    {
        switch (w)
        {
            case Wave::saw:      return "Saw";
            case Wave::square:   return "Square";
            case Wave::triangle: return "Triangle";
            case Wave::sine:     return "Sine";
        }

        return {};
    }

    juce::String getFilterTypeName (FilterType t)
    {
        switch (t)
        {
            case FilterType::lowPass:  return "Low-pass";
            case FilterType::bandPass: return "Band-pass";
            case FilterType::highPass: return "High-pass";
        }

        return {};
    }

    juce::String getLfoTargetName (LfoTarget t)
    {
        switch (t)
        {
            case LfoTarget::pitch:  return "Pitch";
            case LfoTarget::filter: return "Filter";
            case LfoTarget::volume: return "Volume";
            case LfoTarget::width:  return "Width";
        }

        return {};
    }

    double getNoteHz (double note)
    {
        return 440.0 * std::pow (2.0, (note - 69.0) / 12.0);
    }

    //==============================================================================
    float getQ (float resonancePercent)
    {
        return flatQ * std::pow (20.0f / flatQ, juce::jlimit (0.0f, 100.0f, resonancePercent) / 100.0f);
    }

    std::complex<double> getFilterResponse (FilterType type, bool steep, double cutoffHz, float resonancePercent, double hz)
    {
        const double x = hz / juce::jmax (1.0, cutoffHz);
        const auto resonant = getStage (type, x, getQ (resonancePercent));
        return steep ? getStage (type, x, flatQ) * resonant : resonant;
    }

    double getFilterDb (FilterType type, bool steep, double cutoffHz, float resonancePercent, double hz)
    {
        return 20.0 * std::log10 (juce::jmax (1.0e-10, std::abs (getFilterResponse (type, steep, cutoffHz, resonancePercent, hz))));
    }

    float getVelocityScale (const Settings& s, float velocity)
    {
        return 1.0f - juce::jlimit (0.0f, 1.0f, s.velocityPercent / 100.0f) * (1.0f - juce::jlimit (0.0f, 1.0f, velocity));
    }

    double getCutoffAt (const Settings& s, double note, float velocity, float envelope, float lfoOctaves)
    {
        const double octaves = s.envOctaves * envelope * getVelocityScale (s, velocity) + lfoOctaves
                               + s.keyTrackPercent / 100.0 * (note - 60.0) / 12.0;
        return juce::jlimit ((double) minCutoff, (double) maxCutoff, s.cutoffHz * std::pow (2.0, octaves));
    }

    float getEnvelopeLevel (const Envelope& e, double seconds, double releasedAt)
    {
        const auto held = [&e] (double t)
        {
            if (t < 0.0)
                return 0.0;

            if (t < e.attack)
                return t / juce::jmax (1.0e-6, (double) e.attack);

            if (t < e.attack + e.decay)
                return 1.0 - (1.0 - e.sustain) * (t - e.attack) / juce::jmax (1.0e-6, (double) e.decay);

            return (double) e.sustain;
        };

        if (releasedAt < 0.0 || seconds < releasedAt)
            return (float) held (seconds);

        const double from = held (releasedAt);
        return (float) juce::jmax (0.0, from * (1.0 - (seconds - releasedAt) / juce::jmax (1.0e-6, (double) e.release)));
    }

    double getBeatHz (double noteHz, float detuneCents)
    {
        return noteHz * (std::pow (2.0, detuneCents / 1200.0) - 1.0);
    }

    //==============================================================================
    std::complex<double> getHarmonic (Wave wave, float widthPercent, int n)
    {
        if (n < 1)
            return 0.0;

        switch (wave)
        {
            case Wave::saw:
                // A rising ramp: -(2 / pi) * sum of sin (n w t) / n.
                return -2.0 / (pi * n);

            case Wave::square:
            {
                // +1 for the first `width` of the cycle, -1 after it, without its DC.
                const double w = juce::jlimit (0.01, 0.99, widthPercent / 100.0);
                return { 2.0 / (pi * n) * (1.0 - std::cos (2.0 * pi * n * w)), 2.0 / (pi * n) * std::sin (2.0 * pi * n * w) };
            }

            case Wave::triangle:
                if (n % 2 == 0)
                    return 0.0;

                return (((n - 1) / 2) % 2 == 0 ? 1.0 : -1.0) * 8.0 / (pi * pi * n * n);

            case Wave::sine:
                return n == 1 ? 1.0 : 0.0;
        }

        return 0.0;
    }

    std::vector<Partial> getPartials (const Settings& s, double noteHz, double cutoffHz, double maxHz)
    {
        std::vector<Partial> partials;
        const float mix = juce::jlimit (0.0f, 1.0f, s.mixPercent / 100.0f);
        const double secondHz = noteHz * std::pow (2.0, (s.pitch2 + s.detuneCents / 100.0) / 12.0);

        for (int osc = 1; osc <= 2; ++osc)
        {
            const double level = osc == 1 ? 1.0 - mix : mix;
            const double hz = osc == 1 ? noteHz : secondHz;
            const auto wave = osc == 1 ? s.wave1 : s.wave2;

            if (level < 1.0e-4)
                continue;

            for (int n = 1; n * hz < maxHz && n <= 256; ++n)
            {
                const auto c = getHarmonic (wave, s.widthPercent, n) * level;

                if (std::abs (c) < 1.0e-5)
                    continue;

                Partial p;
                p.hz = n * hz;
                p.before = c;
                p.after = c * getFilterResponse (s.filterType, s.steep, cutoffHz, s.resonancePercent, p.hz);
                p.oscillator = osc;
                partials.push_back (p);
            }
        }

        return partials;
    }

    std::vector<float> getWave (const std::vector<Partial>& partials, double seconds, int points, bool filtered)
    {
        std::vector<float> wave ((size_t) juce::jmax (0, points), 0.0f);

        for (auto& p : partials)
        {
            const auto c = filtered ? p.after : p.before;
            const double amplitude = std::abs (c), phase = std::arg (c);

            if (amplitude < 1.0e-5)
                continue;

            for (int i = 0; i < points; ++i)
                wave[(size_t) i] += (float) (amplitude * std::sin (2.0 * pi * p.hz * seconds * i / juce::jmax (1, points - 1) + phase));
        }

        return wave;
    }

    //==============================================================================
    juce::String getKnobName (Knob k)
    {
        switch (k)
        {
            case Knob::wave1:         return "Wave";
            case Knob::width:         return "Width";
            case Knob::wave2:         return "Wave";
            case Knob::pitch2:        return "Pitch";
            case Knob::detune:        return "Detune";
            case Knob::mix:           return "Mix";
            case Knob::noise:         return "Noise";
            case Knob::filterType:    return "Type";
            case Knob::slope:         return "Slope";
            case Knob::cutoff:        return "Cutoff";
            case Knob::resonance:     return "Resonance";
            case Knob::keyTrack:      return "Key track";
            case Knob::envAmount:     return "Amount";
            case Knob::filterAttack:  return "Attack";
            case Knob::filterDecay:   return "Decay";
            case Knob::filterSustain: return "Sustain";
            case Knob::filterRelease: return "Release";
            case Knob::ampAttack:     return "Attack";
            case Knob::ampDecay:      return "Decay";
            case Knob::ampSustain:    return "Sustain";
            case Knob::ampRelease:    return "Release";
            case Knob::lfoRate:       return "Rate";
            case Knob::lfoDepth:      return "Depth";
            case Knob::lfoTarget:     return "To";
            case Knob::glide:         return "Glide";
            case Knob::mode:          return "Voices";
            case Knob::velocity:      return "Velocity";
            case Knob::output:        return "Output";
        }

        return {};
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        switch (k)
        {
            case Knob::wave1:         return getWaveName (s.wave1);
            case Knob::width:         return s.widthPercent >= 49.5f ? juce::String ("50%: square") : formatPercent (s.widthPercent);
            case Knob::wave2:         return getWaveName (s.wave2);
            case Knob::pitch2:        return withSign (juce::roundToInt (s.pitch2), 0) + " st";
            case Knob::detune:        return juce::String (s.detuneCents, 1) + " ct";
            case Knob::mix:           return formatPercent (s.mixPercent);
            case Knob::noise:         return formatPercent (s.noisePercent);
            case Knob::filterType:    return getFilterTypeName (s.filterType);
            case Knob::slope:         return s.steep ? "24 dB/oct" : "12 dB/oct";
            case Knob::cutoff:        return formatHz (s.cutoffHz);
            case Knob::resonance:     return formatPercent (s.resonancePercent);
            case Knob::keyTrack:      return formatPercent (s.keyTrackPercent);
            case Knob::envAmount:     return withSign (s.envOctaves, 1) + " oct";
            case Knob::filterAttack:  return formatTime (s.filterEnv.attack);
            case Knob::filterDecay:   return formatTime (s.filterEnv.decay);
            case Knob::filterSustain: return formatPercent (s.filterEnv.sustain * 100.0f);
            case Knob::filterRelease: return formatTime (s.filterEnv.release);
            case Knob::ampAttack:     return formatTime (s.ampEnv.attack);
            case Knob::ampDecay:      return formatTime (s.ampEnv.decay);
            case Knob::ampSustain:    return formatPercent (s.ampEnv.sustain * 100.0f);
            case Knob::ampRelease:    return formatTime (s.ampEnv.release);
            case Knob::lfoRate:       return juce::String (s.lfoRateHz, s.lfoRateHz < 1.0f ? 2 : 1) + " Hz";
            case Knob::lfoDepth:      return formatPercent (s.lfoDepthPercent);
            case Knob::lfoTarget:     return getLfoTargetName (s.lfoTarget);
            case Knob::glide:         return s.glideSeconds < 0.0005f ? juce::String ("Off") : formatTime (s.glideSeconds);
            case Knob::mode:          return s.mode == Mode::mono ? "Mono" : "Poly";
            case Knob::velocity:      return formatPercent (s.velocityPercent);
            case Knob::output:        return withSign (s.outputDb, 1) + " dB";
        }

        return {};
    }

    //==============================================================================
    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            auto add = [&list] (const juce::String& name, const juce::String& why, Settings s,
                                std::initializer_list<std::pair<Knob, const char*>> notes)
            {
                Preset p;
                p.name = name;
                p.why = why;
                p.settings = s;

                for (auto& [knob, note] : notes)
                    p.notes[(size_t) knob] = juce::CharPointer_UTF8 (note);

                list.push_back (p);
            };

            add ("Init: a plain saw", "One saw, the filter wide open, nothing moving: the raw material. Start here to build your own "
                                      "sound one knob at a time, and hear what each one does.",
                 {}, { { Knob::wave1, "A saw has every harmonic, so there's the most for the filter to take away." },
                       { Knob::cutoff, "Wide open: the filter lets everything through. Turn it down first to hear what it does." },
                       { Knob::ampSustain, "Full while the key is held, like an organ: the note doesn't change." } });

            Settings bass;
            bass.wave2 = Wave::square;
            bass.pitch2 = -12.0f;
            bass.mixPercent = 35.0f;
            bass.cutoffHz = 220.0f;
            bass.resonancePercent = 20.0f;
            bass.keyTrackPercent = 50.0f;
            bass.envOctaves = 2.5f;
            bass.filterEnv = { 0.001f, 0.25f, 0.2f, 0.15f };
            bass.ampEnv = { 0.002f, 0.3f, 0.9f, 0.12f };
            bass.mode = Mode::mono;
            bass.velocityPercent = 60.0f;
            add ("Bass", "A classic analog bass: a saw for the edge and a square an octave below for weight, through a low-pass that "
                         "opens briefly at the start of each note, so it has a short bright attack and then sits low and round.",
                 bass, { { Knob::wave2, "A square: its odd harmonics give a hollow, solid low end." },
                         { Knob::pitch2, "An octave down (-12 semitones): more weight without muddying the saw." },
                         { Knob::mix, "Mostly the saw: the square is the body under it." },
                         { Knob::cutoff, "Low: 220 Hz keeps the bass round and out of the way of the other parts." },
                         { Knob::keyTrack, "Halfway: higher notes stay a little brighter, as on a real bass." },
                         { Knob::envAmount, "The envelope opens the filter 2.5 octaves for the attack, the pluck of the string." },
                         { Knob::filterDecay, "A quarter of a second: the brightness falls away quickly." },
                         { Knob::filterSustain, "Low, so the held note is darker than its start." },
                         { Knob::mode, "Mono: a bass plays one note at a time, and overlapping notes slide without a new attack." } });

            Settings acid;
            acid.cutoffHz = 300.0f;
            acid.resonancePercent = 75.0f;
            acid.envOctaves = 3.5f;
            acid.filterEnv = { 0.001f, 0.18f, 0.0f, 0.1f };
            acid.ampEnv = { 0.001f, 0.4f, 0.8f, 0.05f };
            acid.mode = Mode::mono;
            acid.glideSeconds = 0.08f;
            acid.velocityPercent = 80.0f;
            acid.outputDb = -3.0f;
            add ("Acid bass", "The squelch of acid house, after the Roland TB-303 (1981): one saw, a resonant low-pass and a short filter "
                              "envelope that snaps open on every note. Overlap notes for the slides.",
                 acid, { { Knob::resonance, "High: the peak at the cutoff is what squelches. Turn it up and down while it plays." },
                         { Knob::envAmount, "3.5 octaves: each note starts with a wide-open \"yow\" and closes again." },
                         { Knob::filterDecay, "180 ms: short, so the filter snaps shut; longer sounds more like a \"wow\"." },
                         { Knob::filterSustain, "0%: after the snap, the filter goes back to the cutoff." },
                         { Knob::glide, "80 ms: overlapping notes slide, like the 303's slide." },
                         { Knob::velocity, "High: louder notes open the filter more, like the 303's accent." },
                         { Knob::output, "Down 3 dB: the resonance makes it louder than it looks." } });

            Settings subBass;
            subBass.wave1 = Wave::sine;
            subBass.wave2 = Wave::triangle;
            subBass.pitch2 = 12.0f;
            subBass.mixPercent = 20.0f;
            subBass.cutoffHz = 2000.0f;
            subBass.steep = false;
            subBass.ampEnv = { 0.005f, 0.3f, 1.0f, 0.1f };
            subBass.mode = Mode::mono;
            subBass.velocityPercent = 20.0f;
            add ("Sub bass", "A deep, clean bass felt more than heard, for hip-hop, trap and electronic music: a sine, with a little "
                             "triangle an octave up so you can still hear the notes on small speakers.",
                 subBass, { { Knob::wave1, "A sine: just the fundamental, the cleanest and deepest there is." },
                            { Knob::wave2, "A triangle: a few soft harmonics that phones and laptops can play." },
                            { Knob::pitch2, "An octave up: its harmonics land where small speakers still work." },
                            { Knob::mix, "Only 20%: the sine does the work." },
                            { Knob::cutoff, "Open enough not to matter: there's little to take away." } });

            Settings pad;
            pad.detuneCents = 9.0f;
            pad.mixPercent = 50.0f;
            pad.cutoffHz = 900.0f;
            pad.resonancePercent = 15.0f;
            pad.keyTrackPercent = 30.0f;
            pad.envOctaves = 1.5f;
            pad.filterEnv = { 1.5f, 2.0f, 0.4f, 2.0f };
            pad.ampEnv = { 0.8f, 1.0f, 0.85f, 1.8f };
            pad.lfoRateHz = 0.3f;
            pad.lfoDepthPercent = 15.0f;
            pad.lfoTarget = LfoTarget::filter;
            pad.velocityPercent = 30.0f;
            pad.outputDb = -4.0f;
            add ("Warm pad", "A soft bed of chords: two saws a little out of tune for width, a filter that slowly opens as you hold the "
                             "chord, slow fades in and out, and a slow wobble in the filter so it never stands still.",
                 pad, { { Knob::detune, "9 cents: the two saws beat slowly against each other, which sounds wide and alive." },
                        { Knob::mix, "Both as loud, for the most beating." },
                        { Knob::cutoff, "900 Hz: warm and in the background, out of the singer's way." },
                        { Knob::filterAttack, "1.5 s: the filter opens slowly as the chord is held." },
                        { Knob::ampAttack, "0.8 s: the chord fades in rather than starting with a hit." },
                        { Knob::ampRelease, "1.8 s: it fades out slowly after you let go, so chord changes blur together." },
                        { Knob::lfoRate, "0.3 Hz: one slow sweep every three seconds." },
                        { Knob::lfoTarget, "The filter: a gentle movement in brightness." },
                        { Knob::output, "Down 4 dB: chords of four or more notes add up." } });

            Settings pluck;
            pluck.wave2 = Wave::square;
            pluck.pitch2 = 12.0f;
            pluck.mixPercent = 25.0f;
            pluck.cutoffHz = 350.0f;
            pluck.resonancePercent = 25.0f;
            pluck.keyTrackPercent = 60.0f;
            pluck.envOctaves = 4.0f;
            pluck.filterEnv = { 0.001f, 0.22f, 0.0f, 0.2f };
            pluck.ampEnv = { 0.001f, 0.6f, 0.0f, 0.3f };
            pluck.velocityPercent = 70.0f;
            add ("Pluck", "A short, bright note that dies away, for arpeggios and riffs: everything happens in the envelopes. The "
                          "filter snaps open and closes, and the sound fades out even while the key is held.",
                 pluck, { { Knob::envAmount, "4 octaves: a bright click at the start of every note." },
                          { Knob::filterDecay, "220 ms: the brightness falls away like a plucked string's." },
                          { Knob::ampDecay, "0.6 s: the note dies away..." },
                          { Knob::ampSustain, "...to nothing, even with the key held, as a plucked note does." },
                          { Knob::keyTrack, "60%: high notes stay bright, as on a harp or a guitar." },
                          { Knob::velocity, "70%: soft notes are quieter and duller, hard ones brighter." } });

            Settings brass;
            brass.detuneCents = 6.0f;
            brass.mixPercent = 50.0f;
            brass.steep = false;
            brass.cutoffHz = 600.0f;
            brass.resonancePercent = 10.0f;
            brass.keyTrackPercent = 50.0f;
            brass.envOctaves = 2.5f;
            brass.filterEnv = { 0.06f, 0.4f, 0.6f, 0.2f };
            brass.ampEnv = { 0.04f, 0.2f, 0.9f, 0.25f };
            brass.lfoRateHz = 5.0f;
            brass.lfoDepthPercent = 5.0f;
            brass.velocityPercent = 60.0f;
            add ("Synth brass", "The \"bwaah\" of 1980s synth brass: two saws through a filter whose envelope has its own attack, so "
                                "each note swells brighter like a horn player blowing harder.",
                 brass, { { Knob::wave1, "Saws: brass instruments are rich in every harmonic." },
                          { Knob::slope, "12 dB: a gentler filter sounds more open and brassy." },
                          { Knob::filterAttack, "60 ms: the brightness swells in after the note starts. That's the \"bwaah\"." },
                          { Knob::filterSustain, "60%: it stays fairly bright while held." },
                          { Knob::ampAttack, "40 ms: a soft start, like breath." },
                          { Knob::lfoDepth, "A touch of vibrato on the pitch, as a player would." } });

            Settings lead;
            lead.wave1 = Wave::square;
            lead.detuneCents = 12.0f;
            lead.mixPercent = 40.0f;
            lead.cutoffHz = 2500.0f;
            lead.resonancePercent = 30.0f;
            lead.keyTrackPercent = 50.0f;
            lead.envOctaves = 1.0f;
            lead.filterEnv = { 0.005f, 0.3f, 0.5f, 0.2f };
            lead.ampEnv = { 0.005f, 0.2f, 1.0f, 0.15f };
            lead.lfoRateHz = 5.5f;
            lead.lfoDepthPercent = 10.0f;
            lead.mode = Mode::mono;
            lead.glideSeconds = 0.06f;
            lead.velocityPercent = 40.0f;
            add ("Lead", "A solo synth line that cuts through: a square and a saw, a bright filter, vibrato, and glide between notes, "
                         "in the tradition of the Minimoog solos of the 1970s.",
                 lead, { { Knob::wave1, "A square: hollow and penetrating, like a clarinet." },
                         { Knob::detune, "12 cents against the saw: a thick, slightly wild sound." },
                         { Knob::cutoff, "2.5 kHz: bright enough to cut through a mix." },
                         { Knob::lfoRate, "5.5 Hz: about the speed of a singer's vibrato." },
                         { Knob::lfoDepth, "10%: about a fifth of a semitone up and down, felt more than heard." },
                         { Knob::mode, "Mono: a lead plays one line." },
                         { Knob::glide, "60 ms: legato notes slide into each other." } });

            Settings strings;
            strings.detuneCents = 14.0f;
            strings.mixPercent = 50.0f;
            strings.steep = false;
            strings.cutoffHz = 2200.0f;
            strings.keyTrackPercent = 40.0f;
            strings.ampEnv = { 0.35f, 0.5f, 0.9f, 0.7f };
            strings.lfoRateHz = 5.0f;
            strings.lfoDepthPercent = 3.0f;
            strings.outputDb = -4.0f;
            add ("String machine", "Synth strings, as on the string machines and polysynths of the 1970s: detuned saws for the sound of "
                                   "many players, a gentle filter, and a bow-like fade in and out.",
                 strings, { { Knob::detune, "14 cents: like many players never quite in tune with each other." },
                            { Knob::cutoff, "2.2 kHz with a gentle slope: soft, not dull." },
                            { Knob::ampAttack, "350 ms: the bow takes a moment to get the string going." },
                            { Knob::ampRelease, "0.7 s: the sound rings on in the hall after the bow stops." },
                            { Knob::lfoDepth, "3%: a hint of vibrato." } });

            Settings pwm;
            pwm.wave1 = Wave::square;
            pwm.widthPercent = 30.0f;
            pwm.cutoffHz = 3000.0f;
            pwm.steep = false;
            pwm.keyTrackPercent = 30.0f;
            pwm.ampEnv = { 0.15f, 0.5f, 0.9f, 0.6f };
            pwm.lfoRateHz = 0.6f;
            pwm.lfoDepthPercent = 40.0f;
            pwm.lfoTarget = LfoTarget::width;
            pwm.outputDb = -3.0f;
            add ("PWM strings", "A lush, moving sound from one oscillator: a pulse whose width the LFO keeps changing (pulse-width "
                                "modulation), which sounds like several detuned oscillators, as on the Roland Juno.",
                 pwm, { { Knob::wave1, "A square, so the width counts." },
                        { Knob::width, "30%: a pulse, thinner than a square." },
                        { Knob::lfoRate, "0.6 Hz: a slow sweep." },
                        { Knob::lfoDepth, "40%: the width swings widely, from very thin to past a square." },
                        { Knob::lfoTarget, "Width: the changing harmonics sound like a chorus." } });

            Settings wobble;
            wobble.wave2 = Wave::square;
            wobble.pitch2 = -12.0f;
            wobble.mixPercent = 40.0f;
            wobble.cutoffHz = 250.0f;
            wobble.resonancePercent = 45.0f;
            wobble.ampEnv = { 0.002f, 0.3f, 1.0f, 0.1f };
            wobble.lfoRateHz = 2.0f;
            wobble.lfoDepthPercent = 70.0f;
            wobble.lfoTarget = LfoTarget::filter;
            wobble.mode = Mode::mono;
            wobble.outputDb = -2.0f;
            add ("Wobble bass", "The \"wub\" of dubstep: an LFO opening and closing a resonant filter. At 120 BPM, 2 Hz is a wobble "
                                "every beat; 4 Hz every half beat.",
                 wobble, { { Knob::cutoff, "Low, so the LFO has room to open it." },
                           { Knob::resonance, "45%: a vocal \"wow\" on every sweep." },
                           { Knob::lfoRate, "2 Hz: one wobble a beat at 120 BPM. Twice the rate is twice the wobbles." },
                           { Knob::lfoDepth, "70%: the filter swings almost 3 octaves." },
                           { Knob::lfoTarget, "The filter: that's the wobble." } });

            Settings keys;
            keys.wave1 = Wave::triangle;
            keys.cutoffHz = 1800.0f;
            keys.envOctaves = 1.0f;
            keys.filterEnv = { 0.005f, 0.6f, 0.0f, 0.3f };
            keys.ampEnv = { 0.005f, 1.2f, 0.0f, 0.4f };
            keys.velocityPercent = 60.0f;
            add ("Soft keys", "A gentle, fading keyboard sound for chords or a melody. Listen for the brightness and level falling after each note.",
                 keys, { { Knob::wave1, "Triangle: a soft starting wave." },
                         { Knob::cutoff, "Softens the wave." },
                         { Knob::envAmount, "The filter envelope moves the cutoff by up to one octave." },
                         { Knob::filterAttack, "The filter opens quickly at the start." },
                         { Knob::filterDecay, "The brightness falls after the attack." },
                         { Knob::ampDecay, "The level fades after the attack." },
                         { Knob::ampSustain, "At zero, the note fades even while held." },
                         { Knob::ampRelease, "The note fades after you release the key." },
                         { Knob::velocity, "Playing softly changes the response." } });

            Settings organ;
            organ.wave1 = Wave::square;
            organ.cutoffHz = 5000.0f;
            organ.ampEnv = { 0.01f, 0.3f, 1.0f, 0.08f };
            organ.velocityPercent = 20.0f;
            add ("Hollow organ", "A steady, hollow sound for held chords. Listen for the note staying at a level while the key is held.",
                 organ, { { Knob::wave1, "Square: gives the sound its hollow character." },
                          { Knob::cutoff, "Leaves the tone fairly bright." },
                          { Knob::ampAttack, "Softens the start." },
                          { Knob::ampSustain, "The note holds while the key is down." },
                          { Knob::ampRelease, "The sound stops shortly after the key is released." },
                          { Knob::velocity, "A light response to how hard you play." } });

            Settings stab;
            stab.detuneCents = 7.0f;
            stab.mixPercent = 50.0f;
            stab.cutoffHz = 1200.0f;
            stab.resonancePercent = 15.0f;
            stab.envOctaves = 3.0f;
            stab.filterEnv = { 0.001f, 0.2f, 0.0f, 0.12f };
            stab.ampEnv = { 0.002f, 0.3f, 0.0f, 0.12f };
            stab.outputDb = -4.0f;
            add ("Bright stab", "A short, bright chord for rhythmic accents. Listen for the sharp start and quick fade.",
                 stab, { { Knob::detune, "The two saws move against each other." },
                         { Knob::mix, "Both saws are as loud." },
                         { Knob::cutoff, "A lower resting cutoff leaves room for the bright attack." },
                         { Knob::resonance, "Emphasises the area around the cutoff." },
                         { Knob::envAmount, "The envelope opens the filter for the attack." },
                         { Knob::filterDecay, "Brightness falls quickly." },
                         { Knob::filterRelease, "The filter closes after release." },
                         { Knob::ampAttack, "A quick start." },
                         { Knob::ampDecay, "The level falls quickly." },
                         { Knob::ampSustain, "At zero, the sound fades while held." },
                         { Knob::ampRelease, "The sound fades after release." },
                         { Knob::output, "Leaves more room for chords." } });

            Settings tremolo;
            tremolo.wave1 = Wave::triangle;
            tremolo.cutoffHz = 3500.0f;
            tremolo.ampEnv = { 0.01f, 0.4f, 0.8f, 0.25f };
            tremolo.lfoRateHz = 4.0f;
            tremolo.lfoDepthPercent = 35.0f;
            tremolo.lfoTarget = LfoTarget::volume;
            add ("Tremolo keys", "A pulsing keyboard sound for a simple repeated line or chord. Listen to the level rise and fall while a note is held.",
                 tremolo, { { Knob::wave1, "Triangle: a softer wave." },
                            { Knob::cutoff, "Lets the note through while softening it." },
                            { Knob::ampAttack, "Softens the start." },
                            { Knob::ampDecay, "Moves the level towards sustain." },
                            { Knob::ampSustain, "The note holds while the key is down." },
                            { Knob::ampRelease, "The note fades after release." },
                            { Knob::lfoRate, "Four cycles per second." },
                            { Knob::lfoDepth, "Sets how much the volume moves." },
                            { Knob::lfoTarget, "Volume: hear the level rise and fall." } });

            Settings airy;
            airy.noisePercent = 20.0f;
            airy.cutoffHz = 700.0f;
            airy.resonancePercent = 10.0f;
            airy.envOctaves = 2.0f;
            airy.filterEnv = { 1.2f, 1.0f, 0.7f, 1.5f };
            airy.ampEnv = { 0.6f, 0.5f, 0.8f, 1.2f };
            airy.outputDb = -5.0f;
            add ("Airy sweep", "A slow, airy layer for held notes. Listen for the noisy edge and the gradual rise in brightness.",
                 airy, { { Knob::noise, "Adds an airy layer." },
                         { Knob::cutoff, "A low starting cutoff gives the filter room to open." },
                         { Knob::resonance, "Emphasises the area around the cutoff." },
                         { Knob::envAmount, "The envelope opens the filter." },
                         { Knob::filterAttack, "Brightness rises slowly." },
                         { Knob::filterDecay, "Brightness moves towards the held level." },
                         { Knob::filterSustain, "The filter stays partly open while held." },
                         { Knob::filterRelease, "The filter closes after release." },
                         { Knob::ampAttack, "The sound fades in." },
                         { Knob::ampDecay, "The level moves towards sustain." },
                         { Knob::ampSustain, "The sound continues while held." },
                         { Knob::ampRelease, "The sound fades out." },
                         { Knob::output, "Gives held chords more room." } });

            Settings reed;
            reed.wave1 = Wave::square;
            reed.filterType = FilterType::bandPass;
            reed.cutoffHz = 1200.0f;
            reed.resonancePercent = 30.0f;
            reed.keyTrackPercent = 50.0f;
            reed.ampEnv = { 0.02f, 0.3f, 0.85f, 0.2f };
            reed.velocityPercent = 60.0f;
            add ("Resonant reed", "A focused, reed-like synth tone for a melody. Listen to the coloured tone and try moving the cutoff.",
                 reed, { { Knob::wave1, "Square: a hollow source." },
                         { Knob::filterType, "Band-pass: favours a region of the sound." },
                         { Knob::cutoff, "Moves that region." },
                         { Knob::resonance, "Emphasises the region around the cutoff." },
                         { Knob::keyTrack, "The filter follows higher notes part of the way." },
                         { Knob::ampAttack, "Softens the start." },
                         { Knob::ampSustain, "The note continues while held." },
                         { Knob::velocity, "Changes the response to playing strength." } });

            return list;
        }();

        return presets;
    }
} // namespace sub

//==============================================================================
using sub::Knob;

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
                sub::Settings s;

                switch (k)
                {
                    case Knob::width:         s.widthPercent = value; break;
                    case Knob::pitch2:        s.pitch2 = value; break;
                    case Knob::detune:        s.detuneCents = value; break;
                    case Knob::mix:           s.mixPercent = value; break;
                    case Knob::noise:         s.noisePercent = value; break;
                    case Knob::cutoff:        s.cutoffHz = value; break;
                    case Knob::resonance:     s.resonancePercent = value; break;
                    case Knob::keyTrack:      s.keyTrackPercent = value; break;
                    case Knob::envAmount:     s.envOctaves = value; break;
                    case Knob::filterAttack:  s.filterEnv.attack = value; break;
                    case Knob::filterDecay:   s.filterEnv.decay = value; break;
                    case Knob::filterSustain: s.filterEnv.sustain = value / 100.0f; break;
                    case Knob::filterRelease: s.filterEnv.release = value; break;
                    case Knob::ampAttack:     s.ampEnv.attack = value; break;
                    case Knob::ampDecay:      s.ampEnv.decay = value; break;
                    case Knob::ampSustain:    s.ampEnv.sustain = value / 100.0f; break;
                    case Knob::ampRelease:    s.ampEnv.release = value; break;
                    case Knob::lfoRate:       s.lfoRateHz = value; break;
                    case Knob::lfoDepth:      s.lfoDepthPercent = value; break;
                    case Knob::glide:         s.glideSeconds = value; break;
                    case Knob::velocity:      s.velocityPercent = value; break;
                    case Knob::output:        s.outputDb = value; break;
                    case Knob::wave1:
                    case Knob::wave2:
                    case Knob::filterType:
                    case Knob::slope:
                    case Knob::lfoTarget:
                    case Knob::mode:          break;
                }

                return sub::formatKnob (k, s);
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return 0.0f;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();

                if (text.containsIgnoreCase ("khz"))
                    return v * 1000.0f;

                return text.containsIgnoreCase ("ms") ? v / 1000.0f : v;
            });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;

    /** A polynomial band-limited step: smooths the jump of a saw or square over a sample on each side, so it doesn't alias. */
    inline double blep (double t, double dt) noexcept
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

    /** One sample of a wave at `phase` (0 to 1), moving `dt` of a cycle a sample, peaking at 1. */
    inline double oscillate (sub::Wave wave, double phase, double dt, double width) noexcept
    {
        switch (wave)
        {
            case sub::Wave::saw:
                return 2.0 * phase - 1.0 - blep (phase, dt);

            case sub::Wave::square:
            {
                double v = phase < width ? 1.0 : -1.0;
                v += blep (phase, dt);
                v -= blep (std::fmod (phase + 1.0 - width, 1.0), dt);
                return v - (2.0 * width - 1.0);   // without the pulse's DC
            }

            case sub::Wave::triangle:
                return phase < 0.25 ? 4.0 * phase : phase < 0.75 ? 2.0 - 4.0 * phase : 4.0 * phase - 4.0;

            case sub::Wave::sine:
                return std::sin (juce::MathConstants<double>::twoPi * phase);
        }

        return 0.0;
    }
}

const char* BuiltInSubtractive::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::wave1:         return "wave1";
        case Knob::width:         return "width";
        case Knob::wave2:         return "wave2";
        case Knob::pitch2:        return "pitch2";
        case Knob::detune:        return "detune";
        case Knob::mix:           return "mix";
        case Knob::noise:         return "noise";
        case Knob::filterType:    return "filterType";
        case Knob::slope:         return "slope";
        case Knob::cutoff:        return "cutoff";
        case Knob::resonance:     return "resonance";
        case Knob::keyTrack:      return "keyTrack";
        case Knob::envAmount:     return "envAmount";
        case Knob::filterAttack:  return "filterAttack";
        case Knob::filterDecay:   return "filterDecay";
        case Knob::filterSustain: return "filterSustain";
        case Knob::filterRelease: return "filterRelease";
        case Knob::ampAttack:     return "ampAttack";
        case Knob::ampDecay:      return "ampDecay";
        case Knob::ampSustain:    return "ampSustain";
        case Knob::ampRelease:    return "ampRelease";
        case Knob::lfoRate:       return "lfoRate";
        case Knob::lfoDepth:      return "lfoDepth";
        case Knob::lfoTarget:     return "lfoTarget";
        case Knob::glide:         return "glide";
        case Knob::mode:          return "mode";
        case Knob::velocity:      return "velocity";
        case Knob::output:        return "output";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInSubtractive::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const sub::Settings d;

    auto add = [&layout] (Knob k, const juce::String& name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    auto choice = [&layout] (Knob k, const juce::String& name, const juce::StringArray& items, int value)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (k), 1 }, name, items, value));
    };

    const juce::StringArray waves { "Saw", "Square", "Triangle", "Sine" };
    juce::NormalisableRange<float> glide { 0.0f, sub::maxGlide, 0.001f };
    glide.setSkewForCentre (0.25f);

    choice (Knob::wave1, "Oscillator 1 wave", waves, (int) d.wave1);
    add (Knob::width, "Pulse width", { sub::minWidth, 50.0f, 1.0f }, d.widthPercent);
    choice (Knob::wave2, "Oscillator 2 wave", waves, (int) d.wave2);
    add (Knob::pitch2, "Oscillator 2 pitch", { -24.0f, 24.0f, 1.0f }, d.pitch2);
    add (Knob::detune, "Oscillator 2 detune", { 0.0f, sub::maxDetune, 0.1f }, d.detuneCents);
    add (Knob::mix, "Oscillator mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    add (Knob::noise, "Noise", { 0.0f, 100.0f, 1.0f }, d.noisePercent);
    choice (Knob::filterType, "Filter type", { "Low-pass", "Band-pass", "High-pass" }, (int) d.filterType);
    choice (Knob::slope, "Filter slope", { "12 dB/oct", "24 dB/oct" }, (int) d.steep);
    add (Knob::cutoff, "Cutoff", logRange (sub::minCutoff, sub::maxCutoff), d.cutoffHz);
    add (Knob::resonance, "Resonance", { 0.0f, 100.0f, 1.0f }, d.resonancePercent);
    add (Knob::keyTrack, "Key tracking", { 0.0f, 100.0f, 1.0f }, d.keyTrackPercent);
    add (Knob::envAmount, "Filter envelope amount", { -sub::maxEnvOctaves, sub::maxEnvOctaves, 0.1f }, d.envOctaves);
    add (Knob::filterAttack, "Filter attack", logRange (sub::minTime, sub::maxTime), d.filterEnv.attack);
    add (Knob::filterDecay, "Filter decay", logRange (sub::minTime, sub::maxTime), d.filterEnv.decay);
    add (Knob::filterSustain, "Filter sustain", { 0.0f, 100.0f, 1.0f }, d.filterEnv.sustain * 100.0f);
    add (Knob::filterRelease, "Filter release", logRange (sub::minTime, sub::maxTime), d.filterEnv.release);
    add (Knob::ampAttack, "Amp attack", logRange (sub::minTime, sub::maxTime), d.ampEnv.attack);
    add (Knob::ampDecay, "Amp decay", logRange (sub::minTime, sub::maxTime), d.ampEnv.decay);
    add (Knob::ampSustain, "Amp sustain", { 0.0f, 100.0f, 1.0f }, d.ampEnv.sustain * 100.0f);
    add (Knob::ampRelease, "Amp release", logRange (sub::minTime, sub::maxTime), d.ampEnv.release);
    add (Knob::lfoRate, "LFO rate", logRange (sub::minLfoRate, sub::maxLfoRate), d.lfoRateHz);
    add (Knob::lfoDepth, "LFO depth", { 0.0f, 100.0f, 1.0f }, d.lfoDepthPercent);
    choice (Knob::lfoTarget, "LFO to", { "Pitch", "Filter", "Volume", "Width" }, (int) d.lfoTarget);
    add (Knob::glide, "Glide", glide, d.glideSeconds);
    choice (Knob::mode, "Voices", { "Poly", "Mono" }, (int) d.mode);
    add (Knob::velocity, "Velocity", { 0.0f, 100.0f, 1.0f }, d.velocityPercent);
    add (Knob::output, "Output", { sub::minOutput, sub::maxOutput, 0.1f }, d.outputDb);
    return layout;
}

BuiltInSubtractive::BuiltInSubtractive()
    : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItSubtractive", createLayout())
{
    for (int k = 0; k < sub::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));
}

juce::PluginDescription BuiltInSubtractive::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Synth";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44447431;   // "DDt1"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

void BuiltInSubtractive::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInSubtractive::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

sub::Settings BuiltInSubtractive::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    auto index = [&value] (Knob k, int count) { return juce::jlimit (0, count - 1, juce::roundToInt (value (k))); };

    sub::Settings s;
    s.wave1 = (sub::Wave) index (Knob::wave1, sub::numWaves);
    s.widthPercent = value (Knob::width);
    s.wave2 = (sub::Wave) index (Knob::wave2, sub::numWaves);
    s.pitch2 = value (Knob::pitch2);
    s.detuneCents = value (Knob::detune);
    s.mixPercent = value (Knob::mix);
    s.noisePercent = value (Knob::noise);
    s.filterType = (sub::FilterType) index (Knob::filterType, sub::numFilterTypes);
    s.steep = value (Knob::slope) >= 0.5f;
    s.cutoffHz = value (Knob::cutoff);
    s.resonancePercent = value (Knob::resonance);
    s.keyTrackPercent = value (Knob::keyTrack);
    s.envOctaves = value (Knob::envAmount);
    s.filterEnv = { value (Knob::filterAttack), value (Knob::filterDecay), value (Knob::filterSustain) / 100.0f, value (Knob::filterRelease) };
    s.ampEnv = { value (Knob::ampAttack), value (Knob::ampDecay), value (Knob::ampSustain) / 100.0f, value (Knob::ampRelease) };
    s.lfoRateHz = value (Knob::lfoRate);
    s.lfoDepthPercent = value (Knob::lfoDepth);
    s.lfoTarget = (sub::LfoTarget) index (Knob::lfoTarget, sub::numLfoTargets);
    s.glideSeconds = value (Knob::glide);
    s.mode = value (Knob::mode) >= 0.5f ? sub::Mode::mono : sub::Mode::poly;
    s.velocityPercent = value (Knob::velocity);
    s.outputDb = value (Knob::output);
    return s;
}

void BuiltInSubtractive::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInSubtractive::setSettings (const sub::Settings& s)
{
    setKnob (Knob::wave1, (float) (int) s.wave1);
    setKnob (Knob::width, s.widthPercent);
    setKnob (Knob::wave2, (float) (int) s.wave2);
    setKnob (Knob::pitch2, s.pitch2);
    setKnob (Knob::detune, s.detuneCents);
    setKnob (Knob::mix, s.mixPercent);
    setKnob (Knob::noise, s.noisePercent);
    setKnob (Knob::filterType, (float) (int) s.filterType);
    setKnob (Knob::slope, s.steep ? 1.0f : 0.0f);
    setKnob (Knob::cutoff, s.cutoffHz);
    setKnob (Knob::resonance, s.resonancePercent);
    setKnob (Knob::keyTrack, s.keyTrackPercent);
    setKnob (Knob::envAmount, s.envOctaves);
    setKnob (Knob::filterAttack, s.filterEnv.attack);
    setKnob (Knob::filterDecay, s.filterEnv.decay);
    setKnob (Knob::filterSustain, s.filterEnv.sustain * 100.0f);
    setKnob (Knob::filterRelease, s.filterEnv.release);
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
bool BuiltInSubtractive::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet().isDisabled();
}

void BuiltInSubtractive::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    clap.prepare (rate);
    outputGain.reset (rate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().outputDb));

    for (auto& v : voices)
    {
        v.amp.setSampleRate (rate);
        v.filter.setSampleRate (rate);
        v.amp.reset();
        v.filter.reset();
        v.first.reset();
        v.second.reset();
        v.note = -1;
        v.held = false;
    }

    heldNotes.clear();
    lastPitch = -1.0;
    testNoteLeft = -1;
}

double BuiltInSubtractive::Svf::run (double x, double g, double k, sub::FilterType type) noexcept
{
    const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const double v3 = x - ic2;
    const double v1 = a1 * ic1 + a2 * v3;
    const double v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0 * v1 - ic1;
    ic2 = 2.0 * v2 - ic2;

    switch (type)
    {
        case sub::FilterType::lowPass:  return v2;
        case sub::FilterType::bandPass: return k * v1;   // a peak of 1 at the cutoff, as the analog band-pass
        case sub::FilterType::highPass: return x - k * v1 - v2;
    }

    return v2;
}

void BuiltInSubtractive::updateEnvelopes (const sub::Settings& s)
{
    const juce::ADSR::Parameters amp { s.ampEnv.attack, s.ampEnv.decay, s.ampEnv.sustain, s.ampEnv.release };
    const juce::ADSR::Parameters filter { s.filterEnv.attack, s.filterEnv.decay, s.filterEnv.sustain, s.filterEnv.release };

    for (auto& v : voices)
    {
        v.amp.setParameters (amp);
        v.filter.setParameters (filter);
    }
}

void BuiltInSubtractive::startVoice (Voice& v, int note, float velocity, bool retrigger)
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
        v.phase1 = v.phase2 = 0.0;
        v.first.reset();
        v.second.reset();
    }

    if (retrigger)
    {
        v.amp.noteOn();
        v.filter.noteOn();
        v.age = 0.0;
        v.releasedAt = -1.0;
    }

    lastPitch = note;
}

void BuiltInSubtractive::noteOn (int note, float velocity)
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
                voices[i].filter.noteOff();
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

void BuiltInSubtractive::noteOff (int note)
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
            v.filter.noteOff();
            v.releasedAt = v.age;
        }
    }
}

float BuiltInSubtractive::renderVoice (Voice& v, const sub::Settings& s, float lfo, float& beforeFilter)
{
    const float depth = s.lfoDepthPercent / 100.0f;

    // Glide: most of the way (95%) in the glide time.
    if (s.glideSeconds > 0.0005f)
        v.pitch += (v.targetPitch - v.pitch) * (1.0 - std::exp (-3.0 / (s.glideSeconds * rate)));
    else
        v.pitch = v.targetPitch;

    const double pitch = v.pitch + (s.lfoTarget == sub::LfoTarget::pitch ? lfo * depth * sub::lfoPitchSemitones : 0.0);
    const double hz1 = sub::getNoteHz (pitch);
    const double hz2 = hz1 * std::pow (2.0, (s.pitch2 + s.detuneCents / 100.0) / 12.0);
    const double dt1 = juce::jmin (0.49, hz1 / rate), dt2 = juce::jmin (0.49, hz2 / rate);

    double width = s.widthPercent / 100.0;
    if (s.lfoTarget == sub::LfoTarget::width)
        width += lfo * depth * sub::lfoWidthPoints / 100.0;
    width = juce::jlimit (0.05, 0.95, width);

    const float mix = s.mixPercent / 100.0f;
    double x = (1.0 - mix) * oscillate (s.wave1, v.phase1, dt1, width) + mix * oscillate (s.wave2, v.phase2, dt2, width);

    if (s.noisePercent > 0.0f)
        x += s.noisePercent / 100.0f * (noise.nextFloat() * 2.0f - 1.0f);

    v.phase1 += dt1;
    v.phase2 += dt2;
    v.phase1 -= std::floor (v.phase1);
    v.phase2 -= std::floor (v.phase2);

    const float filterEnv = v.filter.getNextSample();
    const float ampEnv = v.amp.getNextSample();
    const float lfoOctaves = s.lfoTarget == sub::LfoTarget::filter ? lfo * depth * sub::lfoFilterOctaves : 0.0f;
    const double cutoff = juce::jmin (sub::getCutoffAt (s, v.pitch, v.velocity, filterEnv, lfoOctaves), rate * 0.45);

    double y = x;

    if (! withoutFilter.load (std::memory_order_relaxed))
    {
        const double g = std::tan (juce::MathConstants<double>::pi * cutoff / rate);
        const double k = 1.0 / sub::getQ (s.resonancePercent);
        y = s.steep ? v.second.run (v.first.run (x, g, 1.0 / 0.7071, s.filterType), g, k, s.filterType)
                    : v.first.run (x, g, k, s.filterType);
    }

    float volume = ampEnv * sub::getVelocityScale (s, v.velocity) * sub::voiceGain;

    if (s.lfoTarget == sub::LfoTarget::volume)
        volume *= 1.0f - depth * (0.5f - 0.5f * lfo);

    v.age += 1.0 / rate;
    v.lastCutoff = (float) cutoff;
    v.lastAmp = ampEnv;
    v.lastFilter = filterEnv;

    beforeFilter = (float) x * volume;
    return (float) y * volume;
}

void BuiltInSubtractive::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    const auto s = getSettings();
    updateEnvelopes (s);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    // The window's test note: on now, off after 0.8 s.
    if (clap.takeRequest())
    {
        noteOn (sub::testNote, 0.8f);
        testNoteLeft = (int) (0.8 * rate);
    }

    auto* left = buffer.getWritePointer (0);
    auto events = midi.begin();

    for (int i = 0; i < numSamples; ++i)
    {
        for (; events != midi.end() && (*events).samplePosition <= i; ++events)
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

        if (testNoteLeft >= 0 && --testNoteLeft < 0)
            noteOff (sub::testNote);

        const float lfo = (float) std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
        lfoPhase += s.lfoRateHz / rate;
        lfoPhase -= std::floor (lfoPhase);

        float sum = 0.0f, before = 0.0f;

        for (auto& v : voices)
        {
            if (! v.isActive())
                continue;

            float voiceBefore = 0.0f;
            sum += renderVoice (v, s, lfo, voiceBefore);
            before += voiceBefore;
        }

        const float gain = outputGain.getNextValue();
        left[i] = sum * gain;

        chunkBefore = juce::jmax (chunkBefore, std::abs (before * gain));
        chunkAfter = juce::jmax (chunkAfter, std::abs (sum * gain));

        if (++chunkFill == chunkSize)
        {
            history.push ({ juce::Decibels::gainToDecibels (chunkBefore, -100.0f), juce::Decibels::gainToDecibels (chunkAfter, -100.0f) });
            chunkBefore = chunkAfter = 0.0f;
            chunkFill = 0;
        }

        lfoValue.store (lfo, std::memory_order_relaxed);
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // The newest note, for the pictures.
    const Voice* newest = nullptr;

    for (auto& v : voices)
        if (v.isActive() && (newest == nullptr || v.startedAt > newest->startedAt))
            newest = &v;

    if (newest != nullptr)
    {
        shownCutoffHz = newest->lastCutoff;
        ampLevel = newest->lastAmp;
        filterLevel = newest->lastFilter;
        noteAge = (float) newest->age;
        releaseAge = (float) newest->releasedAt;
        shownNote = newest->note;
    }
    else
    {
        shownCutoffHz = (float) sub::getCutoffAt (s, sub::testNote, 1.0f, 0.0f, 0.0f);
        ampLevel = filterLevel = 0.0f;
        noteAge = releaseAge = -1.0f;
    }
}

juce::AudioProcessorEditor* BuiltInSubtractive::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInSubtractive::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInSubtractive::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
