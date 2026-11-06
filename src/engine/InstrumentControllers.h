#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <bitset>
#include <cmath>

namespace daw
{
/** What DawIt's own instruments do with pedal, pitch bend, volume and expression messages. The mod wheel,
    channel pressure and the other controllers are ignored for now. */
namespace instrumentcontrollers
{
    /** How far the full pitch bend goes each way, in semitones: the General MIDI default (RPN 0). */
    constexpr double bendRangeSemitones = 2.0;

    /** Semitones for a 14-bit pitch bend value (0 to 16,383, 8,192 in the middle): 12,288 is +1 at range 2,
        4,096 is -1, 0 is -2. */
    inline double getBendSemitones (int value, double range = bendRangeSemitones) noexcept
    {
        return (double) (juce::jlimit (0, 16383, value) - 8192) / 8192.0 * range;
    }

    /** The frequency factor of a bend of that many semitones, 2^(semitones / 12): 1.05946 for a semitone. */
    inline double getBendFactor (double semitones) noexcept { return std::pow (2.0, semitones / 12.0); }

    /** The General MIDI level of volume and expression together: (volume / 127)^2 x (expression / 127)^2,
        which is 40 log10 (volume / 127) + 40 log10 (expression / 127) dB. 0 is silent. */
    inline float getGain (int volume, int expression) noexcept
    {
        const double v = juce::jlimit (0, 127, volume) / 127.0, e = juce::jlimit (0, 127, expression) / 127.0;
        return (float) (v * v * e * e);
    }

    /** The level in dB (or -100 for silence) of `getGain`. */
    inline float getGainDb (int volume, int expression) noexcept
    {
        return juce::Decibels::gainToDecibels (getGain (volume, expression), -100.0f);
    }

    /** One instrument's controller state, on the audio thread: the pedal and the notes it holds, the bend,
        and the smoothed volume and expression gain. Instruments pass every MIDI message to `handle`. */
    class State
    {
    public:
        /** Sets the sample rate used for smoothing the gain (about 10 ms). */
        void prepare (double sampleRate) noexcept
        {
            smoothing = 1.0f - (float) std::exp (-1.0 / (0.01 * juce::jmax (1.0, sampleRate)));
        }

        /** Back to the start: no pedal, no bend, volume and expression at 127. */
        void reset() noexcept
        {
            sustain = false;
            pedalled.reset();
            bend = 8192;
            volume = expression = 127;
            gain = getGain (volume, expression);
        }

        /** Takes a message. Pedal, bend, volume, expression and Reset All Controllers change the state; when
            the pedal comes up, `release (note)` is called for each key that was let go under it. Notes
            and the rest are the instrument's. Returns true if the message was one of these. */
        template <typename ReleaseNote>
        bool handle (const juce::MidiMessage& m, ReleaseNote&& release)
        {
            if (m.isPitchWheel())
            {
                bend = m.getPitchWheelValue();
                return true;
            }

            if (! m.isController())
                return false;

            const int value = m.getControllerValue();

            switch (m.getControllerNumber())
            {
                case 64:
                {
                    const bool down = value >= 64;

                    if (sustain && ! down)
                        liftPedal (release);

                    sustain = down;
                    return true;
                }
                case 7:   volume = value; return true;
                case 11:  expression = value; return true;
                case 121: // Reset All Controllers: bend centre, expression 127, pedal up; volume is kept
                    bend = 8192;
                    expression = 127;

                    if (sustain)
                        liftPedal (release);

                    sustain = false;
                    return true;
                default:  return false;
            }
        }

        /** A key came up. Returns true if the note should stop now; false if the pedal holds it until it lifts. */
        bool keyUp (int note) noexcept
        {
            if (! sustain)
                return true;

            pedalled.set ((size_t) juce::jlimit (0, 127, note));
            return false;
        }

        /** A key went down. Returns true if that note was still being held by the pedal, so the instrument
            should let the old one go before starting the new one. */
        bool keyDown (int note) noexcept
        {
            const auto i = (size_t) juce::jlimit (0, 127, note);
            const bool was = pedalled.test (i);
            pedalled.reset (i);
            return was;
        }

        /** Forgets the pedal-held notes (after All notes off). */
        void clearPedalled() noexcept { pedalled.reset(); }

        bool isPedalDown() const noexcept { return sustain; }

        /** The bend in semitones now, at range 2. */
        double getSemitones() const noexcept { return getBendSemitones (bend); }

        /** The gain for the next sample: volume and expression, smoothed so a step doesn't click. */
        float nextGain() noexcept
        {
            gain += (getGain (volume, expression) - gain) * smoothing;
            return gain;
        }

        /** The gain volume and expression ask for now, before smoothing. */
        float getTargetGain() const noexcept { return getGain (volume, expression); }

    private:
        template <typename ReleaseNote>
        void liftPedal (ReleaseNote&& release)
        {
            for (int note = 0; note < 128; ++note)
                if (pedalled.test ((size_t) note))
                    release (note);

            pedalled.reset();
        }

        bool sustain = false;
        std::bitset<128> pedalled;
        int bend = 8192, volume = 127, expression = 127;
        float gain = 1.0f, smoothing = 0.002f;
    };
} // namespace instrumentcontrollers
} // namespace daw
