#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <functional>
#include <optional>
#include <vector>

namespace daw
{
class Engine;
class Track;
class AudioClip;
}

namespace daw::sing
{
/** What every DiffSinger voice and vocoder works in: 44.1 kHz audio and a frame every 512 samples. */
constexpr int sampleRate = 44100;
constexpr int hopSize = 512;
constexpr double frameSeconds = (double) hopSize / (double) sampleRate;   // 11.61 ms: 86.13 frames a second

/** The padding in front of every phrase, as OpenUtau's DiffSinger does (the model sings better with a little silence first). */
constexpr double paddingSeconds = 0.5;

/** One sound of a phrase, in the voice's own spelling. */
struct SungPhoneme
{
    juce::String symbol;      // "en/ay", "SP" (silence), "AP" (a breath)
    bool vowel = false;
    int frames = 1;           // how long it lasts, at least 1
    int word = -1;            // which word of the phrase it belongs to; -1 for silence and breaths
};

/** A stretch of the phrase's pitch: a note, or a rest. Together the notes tile the whole phrase. */
struct SungNote
{
    int midi = 60;
    int frames = 1;
    bool rest = false;
};

/** A phrase: sounds that are sung without a rest of half a second or more in between, with the pitch of
    every frame. This is what the singer turns into audio. */
struct Phrase
{
    double startSeconds = 0.0;    // when frame 0 plays in the song (before the padding is cut off)
    int cutFrames = 0;            // the silence at the start that is thrown away after singing
    std::vector<SungPhoneme> phonemes;
    std::vector<SungNote> notes;
    std::vector<float> f0;        // Hz for each frame; as many entries as getNumFrames()
    bool exactPitch = true;       // true: f0 is the notes with slides; false: the pitch model may redraw it

    int getNumFrames() const
    {
        int n = 0;
        for (auto& p : phonemes)
            n += p.frames;

        return n;
    }
};

/** Asked now and then while a job runs; returning true stops it. */
using CancelFn = std::function<bool()>;

/** Something that turns a phrase into singing. The real one runs a voice's ONNX models (DiffSinger.h);
    the tests use a fake that returns a sine at the pitch. */
class Singer
{
public:
    virtual ~Singer() = default;

    /** How long each sound of the phrase should last, in seconds, as the voice's duration model would say it;
        empty when the voice has no such model. Only the consonants are used. */
    virtual std::vector<double> predictDurations (const Phrase&, const CancelFn&) { return {}; }

    /** The pitch of every frame (Hz) as the voice's pitch model draws it, with slides and vibrato; empty when the
        voice has no pitch model. */
    virtual std::vector<float> predictPitch (const Phrase&, const CancelFn&) { return {}; }

    /** The phrase as mono audio at 44.1 kHz, getNumFrames() * hopSize samples. nullopt when cancelled or on an
        error, which `error` then says. `steps` is how many steps the voice takes to paint its sound. */
    virtual std::optional<juce::AudioBuffer<float>> sing (const Phrase&, int steps, const CancelFn&, juce::String& error) = 0;
};

/** What the phrases are placed in: the span of the melody plus half a second before and after. */
struct Rendering
{
    juce::AudioBuffer<float> audio;   // mono, 44.1 kHz
    double originSeconds = 0.0;       // where sample 0 plays in the song, in seconds
};

/** Called with how far the job is, 0 to 1, and a line such as "Singing phrase 3 of 12..."; returning false cancels. */
using RenderProgressFn = std::function<bool (double, const juce::String&)>;

/** Sings every phrase and lays the audio where it belongs: one buffer from `spanStartSeconds` minus half a second to
    `spanEndSeconds` plus half a second (a melody of 8 beats at 120 BPM, 4 s, gives 5 s). nullopt when cancelled
    (nothing is made) or when a phrase fails, which `error` says. */
std::optional<Rendering> renderPhrases (Singer&, const std::vector<Phrase>&, double spanStartSeconds, double spanEndSeconds,
                                        int steps, const RenderProgressFn&, juce::String& error);

/** Adds the sung vocal as a new audio track right after the melody's track (in the same bus), with one clip that
    starts half a second before the first note, and returns the track (null when the rendering is empty). The melody
    track's notes and instrument are not touched; with `muteMelody` its mute is switched on. Message thread. */
Track* addVocalTrack (Engine&, Track& melodyTrack, Rendering, const juce::String& voiceName, bool muteMelody);
} // namespace daw::sing
