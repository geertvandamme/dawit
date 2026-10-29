#include "BuiltInSynth.h"

namespace daw
{
namespace builtInSounds
{
    juce::String getName (BuiltInSound s)
    {
        switch (s)
        {
            case BuiltInSound::piano: return "Piano";
            case BuiltInSound::organ: return "Organ";
            case BuiltInSound::bass:  return "Bass";
            case BuiltInSound::lead:  return "Synth Lead";
            case BuiltInSound::pluck: return "Pluck";
            case BuiltInSound::pad:   return "Pad";
            case BuiltInSound::drums: return "Drums";
            case BuiltInSound::percussion: return "Percussion";
        }

        return {};
    }

    juce::String getDescription (BuiltInSound s)
    {
        switch (s)
        {
            case BuiltInSound::piano: return "Acoustic piano that fades out like a real one";
            case BuiltInSound::organ: return "Drawbar organ with a rotary speaker wobble";
            case BuiltInSound::bass:  return "Round synth bass with a punchy filter";
            case BuiltInSound::lead:  return "Two detuned saws with vibrato, for melodies";
            case BuiltInSound::pluck: return "Short, bright notes for arpeggios";
            case BuiltInSound::pad:   return "Slow, wide and soft, for chords";
            case BuiltInSound::drums: return "General MIDI kit: kick 36, snare 38, hi-hats 42 and 46";
            case BuiltInSound::percussion: return "Congas, bongos, shakers, claves and more on General MIDI notes 54-82";
        }

        return {};
    }

    juce::String getId (BuiltInSound s)
    {
        switch (s)
        {
            case BuiltInSound::piano: return "piano";
            case BuiltInSound::organ: return "organ";
            case BuiltInSound::bass:  return "bass";
            case BuiltInSound::lead:  return "lead";
            case BuiltInSound::pluck: return "pluck";
            case BuiltInSound::pad:   return "pad";
            case BuiltInSound::drums: return "drums";
            case BuiltInSound::percussion: return "percussion";
        }

        return {};
    }

    std::optional<BuiltInSound> fromId (const juce::String& id)
    {
        for (auto s : all)
            if (getId (s) == id)
                return s;

        return std::nullopt;
    }
} // namespace builtInSounds

namespace
{
    // A sine table: much cheaper than std::sin for the many partials and drum tones.
    struct SineTable
    {
        static constexpr int size = 4096;
        std::array<float, size + 1> values {};

        SineTable()
        {
            for (int i = 0; i <= size; ++i)
                values[(size_t) i] = (float) std::sin (juce::MathConstants<double>::twoPi * i / size);
        }

        /** Phase in cycles, 0 <= phase < 1. */
        float operator() (double phase) const noexcept
        {
            const double pos = phase * size;
            const auto i = (size_t) pos;
            const auto frac = (float) (pos - (double) i);
            return values[i] + frac * (values[i + 1] - values[i]);
        }
    };

    const SineTable& sine()
    {
        static const SineTable table;
        return table;
    }

    void advance (double& phase, double increment) noexcept
    {
        phase += increment;

        if (phase >= 1.0)
            phase -= std::floor (phase);
    }

    // Band-limited saw (PolyBLEP), so high notes don't alias.
    float polyBlep (double t, double dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return (float) (t + t - t * t - 1.0);
        }

        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return (float) (t * t + t + t + 1.0);
        }

        return 0.0f;
    }

    float saw (double phase, double increment) noexcept { return (float) (2.0 * phase - 1.0) - polyBlep (phase, increment); }
    float square (double phase) noexcept { return phase < 0.5 ? 1.0f : -1.0f; }

    /** State-variable filter (Simper's trapezoidal form): low, band and high pass at once. */
    struct Filter
    {
        void setup (double sampleRate, double cutoff, double q) noexcept
        {
            g = std::tan (juce::MathConstants<double>::pi * juce::jlimit (20.0, sampleRate * 0.45, cutoff) / sampleRate);
            k = 1.0 / q;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        void reset() noexcept { ic1 = ic2 = 0.0; }

        void process (double v0) noexcept
        {
            const double v3 = v0 - ic2;
            const double v1 = a1 * ic1 + a2 * v3;
            const double v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0 * v1 - ic1;
            ic2 = 2.0 * v2 - ic2;
            low = (float) v2;
            band = (float) v1;
            high = (float) (v0 - k * v1 - v2);
        }

        double g = 0.0, k = 1.0, a1 = 0.0, a2 = 0.0, a3 = 0.0, ic1 = 0.0, ic2 = 0.0;
        float low = 0.0f, band = 0.0f, high = 0.0f;
    };

    /** Exponential decay from 1 with time constant `seconds`. */
    struct Decay
    {
        void start (double seconds, double sampleRate) noexcept
        {
            value = 1.0;
            factor = std::exp (-1.0 / (seconds * sampleRate));
        }

        float next() noexcept
        {
            const auto v = (float) value;
            value *= factor;
            return v;
        }

        double value = 0.0, factor = 0.0;
    };

    /** Below this an exponentially decaying note counts as finished (about -60 dB). */
    constexpr double silent = 0.001;

    enum class Drum { kick, snare, rim, clap, closedHat, pedalHat, openHat, tom, crash, ride, rideBell,
                      tambourine, cowbell, shaker, perc,
                      // hand percussion
                      conga, bongo, timbale, agogo, cabasa, maracas, whistle, guiro, claves, woodblock, cuica,
                      triangle, vibraslap };

    Drum getDrum (int note)
    {
        switch (note)
        {
            case 35: case 36:                               return Drum::kick;
            case 37:                                        return Drum::rim;
            case 38: case 40:                               return Drum::snare;
            case 39:                                        return Drum::clap;
            case 42:                                        return Drum::closedHat;
            case 44:                                        return Drum::pedalHat;
            case 46:                                        return Drum::openHat;
            case 41: case 43: case 45: case 47: case 48: case 50: return Drum::tom;
            case 49: case 52: case 55: case 57:             return Drum::crash;
            case 51: case 59:                               return Drum::ride;
            case 53:                                        return Drum::rideBell;
            case 54:                                        return Drum::tambourine;
            case 56:                                        return Drum::cowbell;
            case 58:                                        return Drum::vibraslap;
            case 60: case 61:                               return Drum::bongo;
            case 62: case 63: case 64:                      return Drum::conga;
            case 65: case 66:                               return Drum::timbale;
            case 67: case 68:                               return Drum::agogo;
            case 69:                                        return Drum::cabasa;
            case 70:                                        return Drum::maracas;
            case 71: case 72:                               return Drum::whistle;
            case 73: case 74:                               return Drum::guiro;
            case 75:                                        return Drum::claves;
            case 76: case 77:                               return Drum::woodblock;
            case 78: case 79:                               return Drum::cuica;
            case 80: case 81:                               return Drum::triangle;
            case 82:                                        return Drum::shaker;
            default:                                        return Drum::perc;
        }
    }

    // The six square waves of a TR-808 cymbal, in Hz.
    constexpr double metalFrequencies[] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };

    struct BuiltInSynthSound final : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class BuiltInVoice final : public juce::SynthesiserVoice
    {
    public:
        explicit BuiltInVoice (const std::atomic<BuiltInSound>& s) : selectedSound (s) {}

        bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<BuiltInSynthSound*> (s) != nullptr; }
        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        bool isPlayingHit (int midiNote) const { return isVoiceActive() && builtInSounds::isPercussive (sound) && note == midiNote; }

        /** Fades the note out quickly, whatever it is. */
        void choke() { gate.noteOff(); }

        void startNote (int midiNote, float noteVelocity, juce::SynthesiserSound*, int) override
        {
            sound = selectedSound.load();
            rate = getSampleRate();
            note = midiNote;
            frequency = juce::MidiMessage::getMidiNoteInHertz (midiNote);
            velocity = juce::jlimit (0.0f, 1.0f, noteVelocity);
            age = 0;
            phases.fill (0.0);
            filterLeft.reset();
            filterRight.reset();

            gate.setSampleRate (rate);

            switch (sound)
            {
                case BuiltInSound::piano: startPiano(); break;
                case BuiltInSound::organ: startOrgan(); break;
                case BuiltInSound::bass:  startBass(); break;
                case BuiltInSound::lead:  startLead(); break;
                case BuiltInSound::pluck: startPluck(); break;
                case BuiltInSound::pad:   startPad(); break;
                case BuiltInSound::drums:
                case BuiltInSound::percussion: startDrum(); break;
            }

            gate.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (! allowTailOff)
            {
                gate.reset();
                clearCurrentNote();
            }
            else if (! builtInSounds::isPercussive (sound)) // drums are one-shots: they ring out
            {
                gate.noteOff();
            }
        }

        void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override
        {
            if (! isVoiceActive())
                return;

            const bool mono = output.getNumChannels() == 1;

            for (int i = startSample; i < startSample + numSamples; ++i)
            {
                float left = 0.0f, right = 0.0f;
                const bool finished = renderSample (left, right);
                const float g = gate.getNextSample();

                if (mono)
                {
                    output.addSample (0, i, 0.5f * g * (left + right));
                }
                else
                {
                    output.addSample (0, i, g * left);
                    output.addSample (1, i, g * right);
                }

                ++age;

                if (finished || ! gate.isActive())
                {
                    gate.reset();
                    clearCurrentNote();
                    break;
                }
            }
        }

    private:
        double seconds() const noexcept { return (double) age / rate; }
        float noise() noexcept { return random.nextFloat() * 2.0f - 1.0f; }

        /** True every 16 samples, when the filters of the moving sounds get new coefficients. */
        bool filterTick() const noexcept { return (age & 15) == 0; }

        void setPan (float pan)
        {
            gainLeft = level * (1.0f - juce::jmax (0.0f, pan));
            gainRight = level * (1.0f + juce::jmin (0.0f, pan));
        }

        bool renderSample (float& left, float& right)
        {
            switch (sound)
            {
                case BuiltInSound::piano: return renderPiano (left, right);
                case BuiltInSound::organ: return renderOrgan (left, right);
                case BuiltInSound::bass:  return renderBass (left, right);
                case BuiltInSound::lead:  return renderLead (left, right);
                case BuiltInSound::pluck: return renderPluck (left, right);
                case BuiltInSound::pad:   return renderPad (left, right);
                case BuiltInSound::drums:
                case BuiltInSound::percussion: return renderDrum (left, right);
            }

            return true;
        }

        //==============================================================================
        // Piano: slightly inharmonic partials that each die away, the high ones sooner, two
        // detuned strings for the low partials and a short hammer knock.
        static constexpr int pianoPartials = 10;

        void startPiano()
        {
            gate.setParameters ({ 0.001f, 0.0f, 1.0f, 0.18f });

            const double sustain = juce::jlimit (0.8, 7.0, 6.0 * std::pow (130.8 / frequency, 0.6));
            const double inharmonicity = 0.0002;
            const double brightness = 1.7 - 0.9 * velocity;
            partialSum = 0.0;

            for (int n = 1; n <= pianoPartials; ++n)
            {
                const auto i = (size_t) n - 1;
                const double f = n * frequency * std::sqrt (1.0 + inharmonicity * n * n);
                increments[i] = f / rate;
                amplitudes[i] = f < rate * 0.45 ? (float) std::pow ((double) n, -brightness) : 0.0f;
                partialDecays[i].start (sustain / (1.0 + 0.5 * std::pow (n - 1.0, 1.3)), rate);
                partialSum += amplitudes[i];
            }

            partialSum += 0.5 * (amplitudes[0] + amplitudes[1]); // the detuned second strings
            env1.start (sustain * 0.12, rate);                    // the quick first drop of a piano note
            env2.start (0.004, rate);                             // hammer
            filterLeft.setup (rate, 1500.0 + 2500.0 * velocity, 0.8);
            level = 0.32f * std::pow (velocity, 1.4f);
            setPan ((float) (note - 64) / 64.0f * 0.35f);
        }

        bool renderPiano (float& left, float& right)
        {
            double sum = 0.0;

            for (size_t i = 0; i < (size_t) pianoPartials; ++i)
            {
                if (amplitudes[i] > 0.0f)
                    sum += amplitudes[i] * partialDecays[i].value * sine() (phases[i]);

                partialDecays[i].value *= partialDecays[i].factor;
                advance (phases[i], increments[i]);
            }

            sum += 0.5 * amplitudes[0] * partialDecays[0].value * sine() (phases[10]);
            sum += 0.5 * amplitudes[1] * partialDecays[1].value * sine() (phases[11]);
            advance (phases[10], increments[0] * 1.0009);
            advance (phases[11], increments[1] * 1.0007);

            filterLeft.process (noise());
            const float out = (float) (sum / partialSum) * (0.4f + 0.6f * env1.next()) + 0.25f * velocity * filterLeft.band * env2.next();
            left = out * gainLeft;
            right = out * gainRight;
            return partialDecays[0].value < silent * 0.5;
        }

        //==============================================================================
        // Organ: drawbar sines with a key click and a rotary speaker's tremolo and swirl.
        static constexpr int organDrawbars = 6;

        void startOrgan()
        {
            gate.setParameters ({ 0.004f, 0.0f, 1.0f, 0.07f });

            constexpr double ratios[organDrawbars] = { 0.5, 1.0, 2.0, 3.0, 4.0, 6.0 };
            constexpr float amounts[organDrawbars] = { 0.55f, 1.0f, 0.65f, 0.4f, 0.35f, 0.2f };
            partialSum = 0.0;

            for (size_t i = 0; i < (size_t) organDrawbars; ++i)
            {
                increments[i] = ratios[i] * frequency / rate;
                amplitudes[i] = ratios[i] * frequency < rate * 0.45 ? amounts[i] : 0.0f;
                partialSum += amplitudes[i];
            }

            phases[10] = random.nextDouble(); // the rotor, from wherever it happens to be
            env2.start (0.003, rate);
            level = 0.17f * (0.6f + 0.4f * velocity);
        }

        bool renderOrgan (float& left, float& right)
        {
            double sum = 0.0;

            for (size_t i = 0; i < (size_t) organDrawbars; ++i)
            {
                sum += amplitudes[i] * sine() (phases[i]);
                advance (phases[i], increments[i]);
            }

            const float rotor = sine() (phases[10]);
            advance (phases[10], 6.2 / rate);

            const float out = level * ((float) (sum / partialSum) * (1.0f + 0.12f * rotor) + 0.08f * noise() * env2.next());
            left = out * (1.0f + 0.25f * rotor);
            right = out * (1.0f - 0.25f * rotor);
            return false;
        }

        //==============================================================================
        // Bass: saw plus sine through a low-pass filter that snaps shut.
        void startBass()
        {
            gate.setParameters ({ 0.002f, 0.5f, 0.7f, 0.07f });
            increments[0] = frequency / rate;
            env1.start (0.13, rate);
            level = 0.38f * (0.35f + 0.65f * velocity);
        }

        bool renderBass (float& left, float& right)
        {
            const float filterEnv = env1.next();

            if (filterTick())
                filterLeft.setup (rate, frequency * 1.2 + 120.0 + filterEnv * (250.0 + 2200.0 * velocity), 1.1);

            filterLeft.process (0.75f * saw (phases[0], increments[0]) + 0.6f * sine() (phases[0]));
            advance (phases[0], increments[0]);

            left = right = level * filterLeft.low;
            return false;
        }

        //==============================================================================
        // Synth lead: two detuned saws, a little sub, a resonant filter and delayed vibrato.
        void startLead()
        {
            gate.setParameters ({ 0.006f, 0.25f, 0.8f, 0.18f });
            increments[0] = frequency * 1.0035 / rate;
            increments[1] = frequency * 0.9965 / rate;
            increments[2] = frequency * 0.5 / rate;
            env1.start (0.2, rate);
            level = 0.16f * (0.4f + 0.6f * velocity);
        }

        bool renderLead (float& left, float& right)
        {
            const double t = seconds();
            const double depth = 0.0045 * juce::jlimit (0.0, 1.0, (t - 0.3) / 0.4);
            const double vibrato = 1.0 + depth * sine() (phases[3]);
            advance (phases[3], 5.6 / rate);

            const float filterEnv = env1.next();

            if (filterTick())
            {
                const double cutoff = frequency * 1.5 + 1800.0 + 2600.0 * velocity + 3000.0 * filterEnv;
                filterLeft.setup (rate, cutoff, 1.8);
                filterRight.setup (rate, cutoff, 1.8);
            }

            const float a = saw (phases[0], increments[0]), b = saw (phases[1], increments[1]);
            const float sub = 0.2f * sine() (phases[2]);

            for (size_t i = 0; i < 3; ++i)
                advance (phases[i], increments[i] * vibrato);

            filterLeft.process (a + 0.6f * b + sub);
            filterRight.process (b + 0.6f * a + sub);
            left = level * filterLeft.low;
            right = level * filterRight.low;
            return false;
        }

        //==============================================================================
        // Pluck: bright saws with a fast filter sweep, fading out on their own.
        void startPluck()
        {
            gate.setParameters ({ 0.001f, 0.0f, 1.0f, 0.2f });
            increments[0] = frequency / rate;
            increments[1] = frequency * 1.006 / rate;
            env1.start (juce::jlimit (0.15, 0.8, 0.55 * std::pow (261.6 / frequency, 0.4)), rate); // level
            env2.start (0.07, rate);                                                                // filter
            level = 0.3f * (0.3f + 0.7f * velocity);
        }

        bool renderPluck (float& left, float& right)
        {
            const float filterEnv = env2.next();

            if (filterTick())
                filterLeft.setup (rate, frequency + 300.0 + filterEnv * (1200.0 + 6000.0 * velocity), 1.3);

            filterLeft.process (0.7f * saw (phases[0], increments[0]) + 0.4f * saw (phases[1], increments[1]));
            advance (phases[0], increments[0]);
            advance (phases[1], increments[1]);

            left = right = level * env1.next() * filterLeft.low;
            return env1.value < silent;
        }

        //==============================================================================
        // Pad: four detuned saws spread over left and right, a slowly moving filter, slow
        // attack and long release.
        void startPad()
        {
            gate.setParameters ({ 0.7f, 0.9f, 0.8f, 1.5f });

            constexpr double cents[] = { -11.0, -4.0, 5.0, 12.0 };

            for (size_t i = 0; i < 4; ++i)
            {
                increments[i] = frequency * std::pow (2.0, cents[i] / 1200.0) / rate;
                phases[i] = random.nextDouble();
            }

            increments[4] = frequency * 0.5 / rate;
            phases[5] = random.nextDouble(); // filter movement
            level = 0.09f * (0.6f + 0.4f * velocity);
        }

        bool renderPad (float& left, float& right)
        {
            if (filterTick())
            {
                const double cutoff = frequency * 1.3 + 700.0 + 700.0 * velocity + 350.0 * sine() (phases[5]);
                filterLeft.setup (rate, cutoff, 0.75);
                filterRight.setup (rate, cutoff, 0.75);
            }

            float saws[4];

            for (size_t i = 0; i < 4; ++i)
            {
                saws[i] = saw (phases[i], increments[i]);
                advance (phases[i], increments[i]);
            }

            const float sub = 0.2f * sine() (phases[4]);
            advance (phases[4], increments[4]);
            advance (phases[5], 0.23 / rate);

            filterLeft.process (saws[0] + saws[2] + 0.3f * (saws[1] + saws[3]) + sub);
            filterRight.process (saws[1] + saws[3] + 0.3f * (saws[0] + saws[2]) + sub);
            left = level * filterLeft.low;
            right = level * filterRight.low;
            return false;
        }

        //==============================================================================
        // Drums, after the classic drum machines: tones with pitch sweeps, filtered noise and
        // banks of square waves for the metal.
        void startDrum()
        {
            gate.setParameters ({ 0.0f, 0.0f, 1.0f, 0.02f }); // only a choke ends a drum early
            drum = getDrum (note);
            float gain = 0.4f, pan = 0.0f;

            switch (drum)
            {
                case Drum::kick:
                    env1.start (0.33, rate);  // body
                    env2.start (0.03, rate);  // pitch
                    env3.start (0.004, rate); // click
                    gain = 0.9f;
                    break;

                case Drum::snare:
                    env1.start (0.08, rate);  // tone
                    env2.start (0.15, rate);  // rattle
                    env3.start (0.008, rate); // pitch
                    filterLeft.setup (rate, 1200.0, 0.7);
                    gain = 0.85f;
                    break;

                case Drum::rim:
                    env1.start (0.01, rate);
                    env2.start (0.006, rate);
                    filterLeft.setup (rate, 3500.0, 2.0);
                    gain = 0.6f;
                    break;

                case Drum::clap:
                    env1.start (0.14, rate); // tail, after the three claps
                    filterLeft.setup (rate, 1150.0, 1.6);
                    gain = 0.9f;
                    break;

                case Drum::closedHat:
                case Drum::pedalHat:
                case Drum::openHat:
                    env1.start (drum == Drum::openHat ? 0.32 : (drum == Drum::pedalHat ? 0.03 : 0.04), rate);
                    filterLeft.setup (rate, 7000.0, 0.9);
                    gain = drum == Drum::openHat ? 0.6f : 0.7f;
                    pan = 0.25f;
                    metalScale = 1.0;
                    break;

                case Drum::tom:
                    toneFrequency = 70.0 * std::pow (2.0, (note - 41) / 7.5);
                    env1.start (0.26 * (0.8 + 21.0 / toneFrequency), rate);
                    env2.start (0.06, rate);
                    env3.start (0.008, rate);
                    gain = 0.55f;
                    pan = (float) (note - 45) / 10.0f * 0.4f;
                    break;

                case Drum::crash:
                    env1.start (0.9, rate);
                    filterLeft.setup (rate, 5000.0, 0.7);
                    metalScale = 1.45;
                    gain = 0.55f;
                    pan = -0.3f;
                    break;

                case Drum::ride:
                case Drum::rideBell:
                    env1.start (0.6, rate);
                    env2.start (0.45, rate);
                    filterLeft.setup (rate, 6000.0, 0.7);
                    metalScale = 1.1;
                    gain = 0.6f;
                    pan = 0.3f;
                    break;

                case Drum::tambourine:
                    env1.start (0.12, rate);
                    filterLeft.setup (rate, 8000.0, 0.7);
                    metalScale = 2.2;
                    gain = 0.8f;
                    pan = 0.35f;
                    break;

                case Drum::cowbell:
                    env1.start (0.025, rate);
                    env2.start (0.17, rate);
                    filterLeft.setup (rate, 800.0, 2.5);
                    gain = 0.18f;
                    pan = 0.15f;
                    break;

                case Drum::shaker:
                    env1.start (0.07, rate);
                    filterLeft.setup (rate, 6500.0, 0.7);
                    gain = 0.7f;
                    pan = -0.35f;
                    break;

                case Drum::perc:
                    env1.start (0.18, rate);
                    env2.start (0.005, rate);
                    gain = 0.4f;
                    break;

                // Skins: a pitch that drops a little, a second mode and a slap.
                case Drum::conga:
                case Drum::bongo:
                {
                    const bool muted = note == 62;
                    toneFrequency = note == 60 ? 520.0 : note == 61 ? 380.0 : note == 64 ? 220.0 : 320.0;
                    env1.start (muted ? 0.05 : (drum == Drum::bongo ? 0.1 : 0.2), rate);
                    env2.start (0.02, rate);
                    env3.start (muted ? 0.02 : 0.06, rate);
                    env4.start (muted ? 0.012 : 0.006, rate);
                    filterLeft.setup (rate, 1800.0, 1.2);
                    slap = muted ? 1.2f : 0.5f;
                    gain = drum == Drum::bongo ? 0.6f : 0.7f;
                    pan = note == 60 || note == 63 || note == 62 ? 0.3f : -0.3f;
                    break;
                }

                case Drum::timbale:
                    toneFrequency = note == 65 ? 600.0 : 420.0;
                    env1.start (0.3, rate);
                    env3.start (0.1, rate);
                    env4.start (0.004, rate);
                    filterLeft.setup (rate, 4000.0, 0.8);
                    gain = 0.45f;
                    pan = note == 65 ? 0.4f : 0.2f;
                    break;

                case Drum::agogo:
                    toneFrequency = note == 67 ? 900.0 : 640.0;
                    env1.start (0.3, rate);
                    env3.start (0.1, rate);
                    env4.start (0.002, rate);
                    gain = 0.3f;
                    pan = -0.4f;
                    break;

                case Drum::cabasa:
                case Drum::maracas:
                    env1.start (drum == Drum::cabasa ? 0.06 : 0.035, rate);
                    filterLeft.setup (rate, drum == Drum::cabasa ? 5500.0 : 7500.0, 0.7);
                    gain = 0.6f;
                    pan = drum == Drum::cabasa ? 0.35f : -0.35f;
                    break;

                case Drum::whistle:
                    hitLength = note == 71 ? 0.12 : 0.45;
                    env1.start (0.02, rate); // release, once the whistle stops
                    gain = 0.3f;
                    pan = 0.2f;
                    break;

                case Drum::guiro:
                    hitLength = note == 73 ? 0.12 : 0.35;
                    env1.start (0.02, rate);
                    filterLeft.setup (rate, 3200.0, 2.5);
                    gain = 0.35f;
                    pan = 0.3f;
                    break;

                case Drum::claves:
                    env1.start (0.045, rate);
                    env4.start (0.002, rate);
                    gain = 0.5f;
                    pan = 0.25f;
                    break;

                case Drum::woodblock:
                    toneFrequency = note == 76 ? 1200.0 : 850.0;
                    env1.start (0.04, rate);
                    env3.start (0.015, rate);
                    env4.start (0.002, rate);
                    gain = 0.5f;
                    pan = note == 76 ? 0.3f : -0.3f;
                    break;

                case Drum::cuica:
                    env1.start (note == 78 ? 0.07 : 0.18, rate);
                    env2.start (0.06, rate);
                    gain = 0.4f;
                    pan = -0.2f;
                    break;

                case Drum::triangle:
                    env1.start (note == 80 ? 0.07 : 1.0, rate);
                    env3.start (note == 80 ? 0.04 : 0.5, rate);
                    gain = 0.35f;
                    pan = 0.45f;
                    break;

                case Drum::vibraslap:
                    env1.start (0.5, rate);
                    env4.start (0.003, rate);
                    filterLeft.setup (rate, 2800.0, 3.0);
                    gain = 0.3f;
                    pan = -0.3f;
                    break;
            }

            level = 0.45f * gain * std::pow (velocity, 1.2f);
            setPan (pan);
        }

        float metal() noexcept
        {
            float sum = 0.0f;

            for (size_t i = 0; i < std::size (metalFrequencies); ++i)
            {
                sum += square (phases[i]);
                advance (phases[i], metalFrequencies[i] * metalScale / rate);
            }

            return sum;
        }

        bool renderDrum (float& left, float& right)
        {
            const double t = seconds();
            float out = 0.0f;
            bool finished = false;

            switch (drum)
            {
                case Drum::kick:
                {
                    const double f = 47.0 + 125.0 * env2.next();
                    const float body = sine() (phases[0]) * env1.next();
                    advance (phases[0], f / rate);
                    out = std::tanh (1.6f * body) / std::tanh (1.6f) + 0.25f * noise() * env3.next();
                    finished = env1.value < silent;
                    break;
                }

                case Drum::snare:
                {
                    const float tone = env1.next() * (0.5f * sine() (phases[0]) + 0.25f * sine() (phases[1]));
                    advance (phases[0], 180.0 * (1.0 + 0.5 * env3.next()) / rate);
                    advance (phases[1], 330.0 / rate);
                    filterLeft.process (noise());
                    out = tone + 0.75f * filterLeft.high * env2.next();
                    finished = env2.value < silent;
                    break;
                }

                case Drum::rim:
                {
                    filterLeft.process (noise());
                    out = 0.6f * sine() (phases[0]) * env1.next() + 1.2f * filterLeft.band * env2.next();
                    advance (phases[0], 1750.0 / rate);
                    finished = env1.value < silent;
                    break;
                }

                case Drum::clap:
                {
                    float env = 0.0f;

                    if (t < 0.06)
                        for (int k = 0; k < 3; ++k)
                            if (const double since = t - k * 0.0105; since >= 0.0)
                                env += (float) std::exp (-since / 0.004);

                    if (t >= 0.03)
                        env += 0.75f * env1.next();

                    filterLeft.process (noise());
                    out = 1.6f * filterLeft.band * env;
                    finished = t >= 0.03 && env1.value < silent;
                    break;
                }

                case Drum::closedHat:
                case Drum::pedalHat:
                case Drum::openHat:
                    filterLeft.process (0.25f * metal() + 0.5f * noise());
                    out = filterLeft.high * env1.next();
                    finished = env1.value < silent;
                    break;

                case Drum::tom:
                {
                    out = sine() (phases[0]) * env1.next() + 0.15f * noise() * env3.next();
                    advance (phases[0], toneFrequency * (1.0 + 0.6 * env2.next()) / rate);
                    finished = env1.value < silent;
                    break;
                }

                case Drum::crash:
                    filterLeft.process (0.15f * metal() + 0.6f * noise());
                    out = 0.7f * filterLeft.high * env1.next() * (float) juce::jmin (1.0, t / 0.001);
                    finished = env1.value < silent;
                    break;

                case Drum::ride:
                case Drum::rideBell:
                {
                    filterLeft.process (metal());
                    const float bell = (sine() (phases[6]) + 0.6f * sine() (phases[7])) * env2.next();
                    advance (phases[6], 1370.0 / rate);
                    advance (phases[7], 2140.0 / rate);
                    out = 0.35f * filterLeft.high * env1.next() + (drum == Drum::rideBell ? 0.45f : 0.08f) * bell;
                    finished = env1.value < silent && env2.value < silent;
                    break;
                }

                case Drum::tambourine:
                    filterLeft.process (0.7f * noise() + 0.2f * metal());
                    out = 0.6f * filterLeft.high * env1.next();
                    finished = env1.value < silent;
                    break;

                case Drum::cowbell:
                {
                    filterLeft.process (square (phases[0]) + square (phases[1]));
                    advance (phases[0], 540.0 / rate);
                    advance (phases[1], 800.0 / rate);
                    out = 0.9f * filterLeft.band * (0.65f * env1.next() + 0.35f * env2.next());
                    finished = env2.value < silent;
                    break;
                }

                case Drum::shaker:
                    filterLeft.process (noise());
                    out = 0.55f * filterLeft.high * env1.next() * (float) juce::jmin (1.0, t / 0.012);
                    finished = env1.value < silent;
                    break;

                case Drum::perc:
                    out = sine() (phases[0]) * env1.next() + 0.1f * noise() * env2.next();
                    advance (phases[0], frequency / rate);
                    finished = env1.value < silent;
                    break;

                case Drum::conga:
                case Drum::bongo:
                {
                    const double f = toneFrequency * (1.0 + 0.06 * env2.next());
                    const float body = sine() (phases[0]) * env1.next() + 0.35f * sine() (phases[1]) * env3.next();
                    advance (phases[0], f / rate);
                    advance (phases[1], f * 1.58 / rate);
                    filterLeft.process (noise());
                    out = body + slap * filterLeft.band * env4.next();
                    finished = env1.value < silent;
                    break;
                }

                case Drum::timbale:
                {
                    const float shell = env1.next() * (sine() (phases[0]) + 0.5f * sine() (phases[1]))
                                      + 0.3f * sine() (phases[2]) * env3.next();
                    advance (phases[0], toneFrequency / rate);
                    advance (phases[1], toneFrequency * 1.47 / rate);
                    advance (phases[2], toneFrequency * 2.09 / rate);
                    filterLeft.process (noise());
                    out = shell + 0.8f * filterLeft.high * env4.next();
                    finished = env1.value < silent;
                    break;
                }

                case Drum::agogo:
                {
                    const float upper = env3.next() * (0.5f * sine() (phases[1]) + 0.25f * sine() (phases[2]));
                    out = sine() (phases[0]) * env1.next() + upper + 0.2f * noise() * env4.next();
                    advance (phases[0], toneFrequency / rate);
                    advance (phases[1], toneFrequency * 2.76 / rate);
                    advance (phases[2], toneFrequency * 5.4 / rate);
                    finished = env1.value < silent;
                    break;
                }

                case Drum::cabasa:
                case Drum::maracas:
                    filterLeft.process (noise());
                    out = filterLeft.high * env1.next() * (float) juce::jmin (1.0, t / 0.004);
                    finished = env1.value < silent;
                    break;

                case Drum::whistle:
                {
                    const float amount = t < hitLength ? (float) juce::jmin (1.0, t / 0.01) : env1.next();
                    const double trill = 1.0 + 0.025 * sine() (phases[1]);
                    out = amount * (0.8f * sine() (phases[0]) + 0.05f * noise());
                    advance (phases[0], 2300.0 * trill / rate);
                    advance (phases[1], 28.0 / rate);
                    finished = t >= hitLength && env1.value < silent;
                    break;
                }

                case Drum::guiro:
                {
                    // Noise, chopped into scrapes by the stick going over the ridges.
                    const float amount = t < hitLength ? (float) juce::jmin (1.0, t / 0.005) : env1.next();
                    const float ridge = juce::jmax (0.0f, sine() (phases[0]));
                    advance (phases[0], (note == 73 ? 42.0 : 32.0) / rate);
                    filterLeft.process (noise());
                    out = 2.0f * filterLeft.band * ridge * ridge * amount;
                    finished = t >= hitLength && env1.value < silent;
                    break;
                }

                case Drum::claves:
                    out = sine() (phases[0]) * env1.next() + 0.2f * noise() * env4.next();
                    advance (phases[0], 2500.0 / rate);
                    finished = env1.value < silent;
                    break;

                case Drum::woodblock:
                    out = sine() (phases[0]) * env1.next() + 0.4f * sine() (phases[1]) * env3.next() + 0.3f * noise() * env4.next();
                    advance (phases[0], toneFrequency / rate);
                    advance (phases[1], toneFrequency * 2.3 / rate);
                    finished = env1.value < silent;
                    break;

                case Drum::cuica:
                {
                    // Open: the pitch squeaks up; muted: it drops.
                    const float bend = 1.0f - env2.next();
                    const double f = note == 78 ? 600.0 * (1.0 - 0.25 * bend) : 290.0 * (1.0 + 0.9 * bend);
                    out = sine() (phases[0]) * env1.next() * (float) juce::jmin (1.0, t / 0.01) + 0.03f * noise();
                    advance (phases[0], f / rate);
                    finished = env1.value < silent;
                    break;
                }

                case Drum::triangle:
                {
                    const float upper = env3.next() * (0.35f * sine() (phases[1]) + 0.2f * sine() (phases[2]));
                    out = 0.6f * sine() (phases[0]) * env1.next() + upper;
                    advance (phases[0], 4000.0 / rate);
                    advance (phases[1], 9600.0 / rate);
                    advance (phases[2], juce::jmin (15600.0, rate * 0.45) / rate);
                    finished = env1.value < silent;
                    break;
                }

                case Drum::vibraslap:
                {
                    const float rattle = juce::jmax (0.0f, sine() (phases[0]));
                    advance (phases[0], 26.0 / rate);
                    filterLeft.process (noise());
                    out = 2.0f * filterLeft.band * rattle * rattle * env1.next() + 0.3f * noise() * env4.next();
                    finished = env1.value < silent;
                    break;
                }
            }

            left = out * gainLeft;
            right = out * gainRight;
            return finished;
        }

        //==============================================================================
        const std::atomic<BuiltInSound>& selectedSound;
        BuiltInSound sound = BuiltInSound::piano;
        Drum drum = Drum::kick;

        double rate = 44100.0, frequency = 440.0, toneFrequency = 100.0, metalScale = 1.0, partialSum = 1.0, hitLength = 0.0;
        int note = 60;
        float velocity = 0.0f, level = 0.0f, gainLeft = 0.0f, gainRight = 0.0f, slap = 0.0f;
        juce::int64 age = 0;

        std::array<double, 12> phases {}, increments {};
        std::array<float, 12> amplitudes {};
        std::array<Decay, pianoPartials> partialDecays {};
        Decay env1, env2, env3, env4;
        Filter filterLeft, filterRight;
        juce::ADSR gate;
        juce::Random random;
    };
} // namespace

//==============================================================================
BuiltInSynth::BuiltInSynth (const std::atomic<BuiltInSound>& sound) : selectedSound (sound)
{
    sine(); // fill the table now rather than on the audio thread

    for (int i = 0; i < numVoices; ++i)
        addVoice (new BuiltInVoice (sound));

    addSound (new BuiltInSynthSound());
}

void BuiltInSynth::noteOn (int midiChannel, int midiNoteNumber, float velocity)
{
    // A closed hi-hat cuts off an open one, as on a real kit, and a muted triangle an open one.
    const int choked = midiNoteNumber == 42 || midiNoteNumber == 44 ? 46 : (midiNoteNumber == 80 ? 81 : -1);

    if (choked >= 0 && builtInSounds::isPercussive (selectedSound.load()))
    {
        const juce::ScopedLock sl (lock);

        for (auto* v : voices)
            if (auto* voice = dynamic_cast<BuiltInVoice*> (v); voice != nullptr && voice->isPlayingHit (choked))
                voice->choke();
    }

    juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
}
} // namespace daw
