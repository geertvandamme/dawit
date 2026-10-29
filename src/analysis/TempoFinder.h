#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "engine/TempoMap.h"

#include <array>
#include <optional>

namespace daw::analysis
{
/**
    Finding a recording's tempo (BPM), its beats and where its bars start, with plain signal
    processing, no model. It takes a second or two for a whole song.

    1. Onsets: where the sound suddenly gets louder, from the rise in level in 40 bands from 30 Hz
       to 11 kHz (spectral flux), 172 times a second. Mostly the drums.
    2. Tempo: how often that pattern repeats (the autocorrelation of the onsets), leaning towards
       the usual range around 120 BPM, because half and double the tempo repeat too (Ellis 2007).
    3. Beats: the beat times that land on strong onsets while keeping close to that tempo (dynamic
       programming, Ellis 2007). The tempo is then measured precisely from all the beats.
    4. Bars, in 4/4: of every four beats, the one where the chords change most and the kick and
       bass hit hardest is taken as the first beat of the bar. This is a good guess, not a certainty.
*/
namespace tempofinder
{
    /** The analysis listens to mono audio at this rate. */
    constexpr double sampleRate = 22050.0;
    constexpr int hopSize = 128;
    constexpr double frameRate = sampleRate / hopSize;   // about 172 frames a second

    /** The tempos it looks between. */
    constexpr double minBpm = 40.0, maxBpm = 240.0;

    /** What the beat finding listens to, frame by frame. */
    struct Onsets
    {
        std::vector<float> strength;                   // how suddenly the sound gets louder, all bands
        std::vector<float> low;                        // the same below 150 Hz: kick and bass
        std::vector<std::array<float, 12>> chroma;     // the level of each pitch class, C to B
    };

    /** The tempo of a recording, with its beats. */
    struct Tempo
    {
        double bpm = 120.0;                    // measured from the beats
        double slowest = 120.0, fastest = 120.0;   // the tempo over 16 beats at a time (the outer tenth left out)
        bool steady = true;                    // within 2.5% (or 2 BPM) all the way through
        double confidence = 0.0;               // 0 to 1: how strongly the beat repeats
        std::vector<double> beats;             // in seconds from the start
        int firstDownbeat = 0;                 // which of the beats is the first of a bar

        double getFirstDownbeatSeconds() const { return beats.empty() ? 0.0 : beats[(size_t) firstDownbeat]; }
    };

    /** The audio as mono at 22.05 kHz. */
    juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>&, double sourceSampleRate);

    Onsets findOnsets (const juce::AudioBuffer<float>& mono);

    /** The beat period in frames, from how the onsets repeat, or nullopt when nothing repeats. */
    std::optional<double> findPeriod (const std::vector<float>& onsetStrength, double* confidence = nullptr);

    /** The beats in frames: on strong onsets, about `period` apart. */
    std::vector<double> findBeats (const std::vector<float>& strength, double period);

    /** Which of the first four beats starts a bar. */
    int findFirstDownbeat (const Onsets&, const std::vector<double>& beatFrames);

    /** The tempo of mono audio at sampleRate, or nullopt when there's no clear beat (silence, a
        held pad, less than 4 seconds). */
    std::optional<Tempo> analyse (const juce::AudioBuffer<float>& mono);

    /** A tempo as songs are set: a whole BPM when it's within 0.05 of one (a song recorded to a
        click), otherwise to 0.01, so a long recording stays on the bars to its end. */
    double roundBpm (double bpm);

    /** The song's tempo following a recording bar by bar, and where its clip goes for that. */
    struct FollowedTempo
    {
        TempoMap map;
        double clipStartBeat = 0.0;   // the clip's new start: its first downbeat on a bar line
        int firstBar = 0;             // the bar the first downbeat starts
        int bars = 0;                 // how many bars were measured
        juce::Range<double> range;    // the slowest and fastest bar
    };

    /** Gives every bar of the recording its own tempo: bar k lasts from the k-th downbeat (every
        fourth beat from the first downbeat) to the next, so each bar line lands on the band's
        downbeat. The first downbeat goes on the bar line nearest to where it is when the clip
        starts at `clipStartBeat` at the first bar's tempo (a bar later if the clip would start
        before the song). Before it the first bar's tempo plays, after the last whole bar the last
        one's. The tempos aren't smoothed: that would add up to a drift. With fewer than two
        downbeats it's one tempo, the one found. */
    FollowedTempo followRecording (const Tempo&, double clipStartBeat);
} // namespace tempofinder
} // namespace daw::analysis
