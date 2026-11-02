#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace daw
{
/**
    The step grid and clock DawIt Slicer and DawIt Step Filter share, so they count steps the same way
    and line up when chained: the note values a step can last, which step a position falls in, and the
    clock that follows the song's play head, or a clock of its own at the song's tempo.
*/
namespace stepgrid
{
    constexpr int maxSteps = 16;
    constexpr float minOwnBpm = 20.0f, maxOwnBpm = 300.0f;

    /** A step length as a note value. `beats` are quarter notes. */
    struct StepLength
    {
        const char* name;
        double beats;
    };

    /** From short to long: 1/32 to 1/4, with the two triplets. */
    inline constexpr StepLength stepLengths[] = { { "1/32", 0.125 }, { "1/16T", 1.0 / 6.0 }, { "1/16", 0.25 }, { "1/8T", 1.0 / 3.0 }, { "1/8", 0.5 }, { "1/4", 1.0 } };
    constexpr int numStepLengths = (int) std::size (stepLengths);
    constexpr int defaultStepLength = 2;   // 1/16

    /** The tempo to step at: the given one, kept between DawIt's slowest and fastest. */
    inline double clampBpm (double bpm) { return juce::jlimit ((double) minOwnBpm, (double) maxOwnBpm, bpm); }

    /** How many quarter notes a step lasts. */
    inline double getStepBeats (int stepLength)
    {
        return stepLengths[juce::jlimit (0, numStepLengths - 1, stepLength)].beats;
    }

    /** How long a step lasts at this tempo, in ms. */
    inline double getStepMs (int stepLength, double bpm)
    {
        return getStepBeats (stepLength) * 60000.0 / clampBpm (bpm);
    }

    /** The step (0 is the first) of the pattern that a position in quarter notes falls in. Steps are counted from the start of the song. */
    inline int getStepIndex (double beats, int stepLength, int steps)
    {
        const long long count = (long long) std::floor (beats / getStepBeats (stepLength) + 1.0e-9);
        const int n = juce::jlimit (1, maxSteps, steps);
        return (int) (((count % n) + n) % n);
    }

    /** What the clock says about one block of audio. */
    struct ClockBlock
    {
        double hostBpm = 0.0;          // the host's tempo, 0 while unknown
        double bpm = 120.0;            // the tempo used
        double beatsPerSample = 0.0;
        double beat = 0.0;             // quarter notes at the block's first sample
        bool songBased = false;        // follows the song's play head
        bool jump = false;             // a seek, loop wrap, start or change of clock: the position isn't where the last block ended
    };

    /**
        The clock: the song's position (its ppq, or its time in seconds at the own tempo) while the song plays,
        and a clock of its own, running at the song's tempo, while it doesn't. It spots jumps. Audio thread only.
    */
    class Clock
    {
    public:
        /** Forgets where it was: the next block counts as a jump, and the own clock starts at 0. Call when playback is prepared. */
        void reset() noexcept
        {
            havePrev = false;
            ownBeat = 0.0;
        }

        /** Reads the play head (may be null) and works out this block. Call `commit` after any `restartOwn`, then `finish` after the block. */
        ClockBlock begin (juce::AudioPlayHead* playHead, bool ownTempo, double ownBpm, double rate)
        {
            juce::Optional<juce::AudioPlayHead::PositionInfo> position;

            if (playHead != nullptr)
                position = playHead->getPosition();

            ClockBlock b;
            bool playing = false;
            juce::Optional<double> ppq, seconds;

            if (position.hasValue())
            {
                if (const auto bpm = position->getBpm())
                    b.hostBpm = *bpm;

                playing = position->getIsPlaying();
                ppq = position->getPpqPosition();
                seconds = position->getTimeInSeconds();
            }

            b.bpm = clampBpm (ownTempo || b.hostBpm <= 0.0 ? ownBpm : b.hostBpm);
            b.beatsPerSample = b.bpm / 60.0 / rate;
            b.beat = ownBeat;

            if (playing && ! ownTempo && ppq.hasValue())
            {
                b.beat = *ppq;
                b.songBased = true;
            }
            else if (playing && ownTempo && seconds.hasValue())
            {
                b.beat = *seconds * b.bpm / 60.0;
                b.songBased = true;
            }

            b.jump = ! havePrev || b.songBased != prevSongBased;

            if (! b.songBased && prevSongBased)
                ownBeat = b.beat = 0.0;

            if (b.songBased && ! b.jump && std::abs (b.beat - prevEndBeat) > 4.0 * b.beatsPerSample)
                b.jump = true;

            return b;
        }

        /** For a test sound: starts the own clock at step 1 (a jump) unless the song's clock is in use. */
        void restartOwn (ClockBlock& b) noexcept
        {
            if (! b.songBased)
            {
                ownBeat = b.beat = 0.0;
                b.jump = true;
            }
        }

        /** Remembers which clock this block used. */
        void commit (const ClockBlock& b) noexcept
        {
            havePrev = true;
            prevSongBased = b.songBased;
        }

        /** After the block of `numSamples`: remembers where it ended, and moves the own clock on. */
        void finish (const ClockBlock& b, int numSamples) noexcept
        {
            prevEndBeat = b.beat + numSamples * b.beatsPerSample;

            if (! b.songBased)
                ownBeat = prevEndBeat;
        }

    private:
        bool havePrev = false, prevSongBased = false;
        double prevEndBeat = 0.0, ownBeat = 0.0;
    };
} // namespace stepgrid
} // namespace daw
