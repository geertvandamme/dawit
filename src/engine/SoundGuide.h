#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

/**
    What sound is, for the "What is sound?" guide in the waveform editor: from moving air to the
    numbers in a WAV file. The topics and their texts, the tone shapes the guide plays (and their
    harmonics), and a few measurements of a clip's audio: its pitch, and what happens at a moment of it.
*/
namespace daw::soundguide
{
enum class Topic { air, waveform, amplitude, frequency, timbre, numbers, phase, cancellation, mono, vocals };

inline constexpr Topic allTopics[] = { Topic::air, Topic::waveform, Topic::amplitude, Topic::frequency,
                                       Topic::timbre, Topic::numbers, Topic::phase, Topic::cancellation, Topic::mono, Topic::vocals };

juce::String getTopicName (Topic);
juce::String getTopicSummary (Topic);
juce::String explain (Topic);

//==============================================================================
enum class Shape { sine, triangle, saw, square };

inline constexpr Shape allShapes[] = { Shape::sine, Shape::triangle, Shape::saw, Shape::square };

juce::String getShapeName (Shape);

/** One sample of the shape at `phase` (0 to 1 is one cycle), between -1 and 1. `increment` is how
    far the phase moves per sample: the saw and square are smoothed at their jumps by that much, so
    they don't alias at high frequencies. 0 draws the ideal shape. */
float getSample (Shape, double phase, double increment = 0.0);

/** The level of harmonic `n` (1 is the fundamental) relative to the fundamental. */
float getHarmonicLevel (Shape, int n);

//==============================================================================
inline constexpr float speedOfSound = 343.0f;   // metres per second, in air at 20 degrees
inline constexpr float lowestHeard = 20.0f, highestHeard = 20000.0f;

/** "A4", the nearest note name, with middle C as C4. */
juce::String getNoteName (float hz);

/** "440 Hz", "1.5 kHz". */
juce::String formatHz (float hz);

/** "1.2 ms", "15 ms", "0.20 s". */
juce::String formatTime (double seconds);

/** "78 cm", "1.6 m": how long one cycle is in the air. */
juce::String formatWavelength (float hz);

/** How loud a sound and a copy of it `delaySeconds` later are together at a frequency, against the
    sound alone, in dB: +6 where they are in step, down to -100 where they cancel. With the copy
    flipped (its polarity reversed), what added up cancels and the other way round. */
float getCopySumDb (double hz, double delaySeconds, bool flipped);

/** What the old karaoke trick, left minus right, leaves of a clip: the level of the clip and of
    left minus right (scaled so a sound on one side only keeps its level), over the whole clip and
    below lowCutHz, where the bass and kick drum are. RMS in dB, -100 for silence. A clip with one
    channel, or the same left and right, isn't stereo and leaves nothing. */
struct LeftMinusRight
{
    bool stereo = false;
    float mixDb = -100.0f, differenceDb = -100.0f;
    float mixLowDb = -100.0f, differenceLowDb = -100.0f;
};

inline constexpr float lowCutHz = 150.0f;

LeftMinusRight measureLeftMinusRight (const juce::AudioBuffer<float>&, double sampleRate);

/** A sound in a mix for the "Taking the vocals out" picture: where it's panned (-1 left to 1 right),
    how wide it's spread (0 for one point, 1 over both speakers), and whether it's part of the vocals. */
struct MixPart
{
    juce::String name;
    float pan = 0.0f, width = 0.0f;
    bool vocals = false, low = false;   // low: the bass and kick drum, below lowCutHz
};

/** A typical pop or rock mix from left to right: lead vocal, kick, snare and bass in the middle,
    guitars to the sides, keys and the vocal's reverb spread wide. */
const std::vector<MixPart>& getTypicalMix();

/** True when left minus right keeps a part: anything that isn't in the middle, or is spread wide. */
bool survivesLeftMinusRight (const MixPart&);

/** The pitch of a (mono) stretch of audio in Hz, between 50 Hz and 2 kHz; 0 when it has no clear
    pitch (silence, noise, a drum, or several notes at once). Needs at least 40 ms of audio for
    the lowest pitches. */
float estimatePitch (const float* samples, int numSamples, double sampleRate);

/** What a moment of a clip is, for hovering over the waveform: silence, an attack, a fade, or a
    held sound and its pitch. Levels are RMS in dB just before and at the moment, the peak at it,
    and the pitch from estimatePitch(). */
juce::String describeMoment (float rmsBeforeDb, float rmsHereDb, float peakHereDb, float pitchHz);

//==============================================================================
/** The envelope of one note in a clip, as DawIt measures it for the Timbre picture. Times are in
    seconds from the start of the audio that was measured; every time is a 10-90% time in amplitude. */
struct Envelope
{
    bool found = false;          // a note starts before the focus and dies away after it
    bool silent = false;         // nothing within -60 dBFS around the focus
    double onsetSeconds = 0.0, peakSeconds = 0.0, endSeconds = 0.0;
    float peakDb = -100.0f;      // the peak of the 5 ms RMS contour, in dBFS
    double attack = 0.0;         // 10% to 90% of the peak
    bool sustained = false;      // false: it fades all the time it is held, like a piano
    bool hasDecay = false;       // the peak is 1 dB or more above the sustain
    double decay = 0.0;          // 10% to 90% of the way from the peak down to the sustain
    float sustainDb = 0.0f;      // relative to the peak, 0 or less
    double hold = 0.0;           // from the end of the decay (or the peak) to the start of the release
    double release = 0.0;        // 90% of the sustain down to 10% of it
    bool releaseFound = false;
    double body = 0.0;           // from the peak to the release start
    float fadeDbPerSecond = 0.0f;   // for a fading note: how fast it fades, a positive number
    // Where the A, D, S and R segments sit, for drawing them.
    double attackStart = 0.0, attackEnd = 0.0, decayStart = 0.0, decayEnd = 0.0, releaseStart = 0.0, releaseEnd = 0.0;
    // The level contour (linear, 5 ms RMS blocks of both channels at the clip's gain, lightly smoothed),
    // from contourStartSeconds, around the focus.
    std::vector<float> contour;
    double contourStartSeconds = 0.0, blockSeconds = 0.005;
};

/** Measures the envelope of the note under `focusSample`: it walks back to where the note starts
    and forward to where it ends (see Envelope). `gain` is the clip's gain. */
Envelope measureEnvelope (const juce::AudioBuffer<float>&, double sampleRate, int focusSample, float gain = 1.0f);

/** The one line of text under the envelope picture: attack, decay, sustain and release in words. */
juce::String describeEnvelope (const Envelope&);

//==============================================================================
inline constexpr int numMonoBands = 5;

/** "Low: bass and kick, below 150 Hz", "Low mids 150-500 Hz", ... for the band `index` (0 to 4). */
juce::String getMonoBandName (int index);

/** What a stereo clip loses when left and right are added and halved, (L+R)/2: overall and in five
    bands (below 150 Hz, 150-500 Hz, 500 Hz-2 kHz, 2-6 kHz, above 6 kHz). Levels are RMS in dB, -100 for
    silence; losses are never above 0. */
struct MonoCheck
{
    struct Band { float stereoDb = -100.0f, monoDb = -100.0f, lossDb = 0.0f; };

    bool stereo = false;         // two channels that differ (the side isn't 60 dB under)
    bool measurable = false;     // at least 0.1 s, and louder than -60 dBFS RMS
    float stereoDb = -100.0f, monoDb = -100.0f, lossDb = 0.0f;
    float correlation = 0.0f;    // +1 left and right move together, 0 unrelated, -1 opposite
    std::array<Band, numMonoBands> bands;
    int mostLostBand = -1;       // the largest loss among bands within 30 dB of the loudest; -1 when none loses 1 dB
};

MonoCheck measureMono (const juce::AudioBuffer<float>&, double sampleRate);

/** The advice under the mono bands (see MonoCheck): mostly in the middle, wide, or working against itself. */
juce::String describeMono (const MonoCheck&);

/** The made-up stereo clip the guide shows when no clip is in the editor (8 s): four notes of a saw in
    the middle, a bass that is flipped at half level on the right, and a little unrelated air above 6 kHz. */
juce::AudioBuffer<float> makeExampleClip (double sampleRate = 44100.0);
} // namespace daw::soundguide
