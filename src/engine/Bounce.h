#pragma once

#include "Engine.h"

namespace daw::bounce
{
/** What to bounce, and in which format. */
struct Settings
{
    double startBeat = 0.0, endBeat = 16.0;
    double sampleRate = 48000.0;
    int bitDepth = 24;           // 16, 24, or 32 for 32-bit float
    double tailSeconds = 0.0;    // after the end, while reverbs and released notes still sound
    bool keepWholeTail = false;  // keep all of the tail instead of cutting it where it falls below -80 dB (files that must match in length)
    BounceFocus focus;           // what plays: the mix (the default), the tracks as if soloed, or one track's chain
};

/** The loop range, as whole bars. */
juce::Range<double> getLoopRange (const Engine&);

/** The whole song: from bar 1 to the end of its last clip or section. Empty for an empty song. */
juce::Range<double> getSongRange (const Engine&);

/** The loop range when the loop is on, otherwise the whole song. */
juce::Range<double> getDefaultRange (const Engine&);

/** How a bounce went. */
struct Outcome
{
    juce::Result result = juce::Result::ok();
    double seconds = 0.0;   // the length of the file
    float peak = 0.0f;      // the loudest sample; above 1 a 16 or 24-bit file clips
    bool cancelled = false;
};

/**
    Renders the edit offline into a WAV file, as playback would sound through every instrument,
    effect and the master volume, but without the metronome or a count-in.

    Create it on the message thread: the audio device goes silent and the engine is prepared for the
    bounce, with the playhead at the start and the loop off. render() can then run on another thread.
    Destroy it on the message thread, which puts the transport and the device back as they were.
*/
class Renderer
{
public:
    Renderer (Engine&, const Settings&);
    ~Renderer();

    /** Writes the file (through a temporary file, so a cancelled or failed bounce leaves nothing).
        `keepGoing` is called with the progress, 0 to 1, after each block; returning false cancels. */
    Outcome render (const juce::File& target, const std::function<bool (double progress)>& keepGoing = {});

    /** The same, into memory: `audio` becomes the stereo result at the settings' sample rate. Nothing is
        written to disk. */
    Outcome renderToBuffer (juce::AudioBuffer<float>& audio, const std::function<bool (double progress)>& keepGoing = {});

    /** Renders and throws the sound away: for a notes-only focus, whose result is the capture. */
    Outcome renderDiscarding (const std::function<bool (double progress)>& keepGoing = {});

    /** Hands each block of the stereo result to a callback: the `count` samples from `start` of the buffer. For
        measuring without keeping the audio (see loudness::measureSong). Can run on another thread. */
    using Sink = std::function<void (const juce::AudioBuffer<float>&, int start, int count)>;
    Outcome renderToCallback (const Sink& sink, const std::function<bool (double progress)>& keepGoing = {});

private:

    /** Renders the range and the tail, handing every piece to `sink`; drops the first `latencySamples` of what is rendered. */
    Outcome run (const Sink& sink, const std::function<bool (double progress)>& keepGoing);

    int latencySamples = 0;   // a MIDI track's effects hold the sound back this much: it's rendered extra and dropped

    Engine& engine;
    const Settings settings;

    // The transport as it was.
    bool wasPlaying, wasLooping, wasClicking, wasCountingIn;
    double oldPosition;

    static constexpr int blockSize = 512;
};

//==============================================================================
// Bounce in place: a track, or one clip of it, printed through its instrument and effects onto a new audio track.

/** What a bounce in place covers. Make it with planInPlace() on the message thread. */
struct InPlacePlan
{
    Track* track = nullptr;
    const Clip* clip = nullptr;   // null: the whole track (its unmuted clips)
    double startBeat = 0.0, endBeat = 0.0;
    juce::String problem;         // not empty when there is nothing to bounce, and why

    bool isOk() const noexcept { return track != nullptr && problem.isEmpty(); }
};

/** The longest tail kept after the last clip, while reverbs and released notes die away. */
constexpr double inPlaceTailSeconds = 4.0;

/** The level a bounce that would clip is turned down to (-1 dBFS), and the most its clip gain is raised (dB). */
constexpr float inPlaceTargetPeak = 0.8912509f;
constexpr float inPlaceMaxGainDb = 12.0f;

/** The track's range from its first unmuted clip's start to its last one's end, or the clip's own range.
    `problem` says why when there is nothing unmuted to bounce. */
InPlacePlan planInPlace (const Engine&, Track&, const Clip* clip = nullptr);

/** The renderer settings for the plan: one chain, at 48 kHz, with the tail. */
Settings getInPlaceSettings (const InPlacePlan&);

/** How a bounce in place went. */
struct InPlaceResult
{
    juce::Result result = juce::Result::ok();
    Track* track = nullptr;         // the audio track the result is on
    AudioClip* clip = nullptr;
    float peak = 0.0f;              // of the rendered sound
    float gainDb = 0.0f;            // the clip gain it got, to make up for the turning down
    juce::String note;              // what to tell the user when the sound was turned down
};

/** Message thread, once `audio` (stereo, at the sample rate of getInPlaceSettings) is rendered: puts it on an audio
    track right below the original (or the "(bounced)" track below it, for a clip, when its place is free), with the
    original's colour, bus, volume, pan and sends and no plugins, and mutes the original track or clip. A sound that
    peaks over 0 dBFS is turned down to -1 dBFS and the clip gain raised to match, up to +12 dB. The original's instrument,
    notes and effects never change. */
InPlaceResult addBouncedTrack (Engine&, const InPlacePlan&, juce::AudioBuffer<float> audio);

/** Plans, renders and adds, all on the calling thread (the message thread): for tests and small jobs. */
InPlaceResult bounceInPlace (Engine&, Track&, const Clip* clip = nullptr);

//==============================================================================
// Create MIDI clip from arpeggiator: the notes a track's MIDI effects play, written out into a new clip.

/** What a write-out covers. Make it with planWriteOut() on the message thread. */
struct WriteOutPlan
{
    Track* track = nullptr;
    const Clip* clip = nullptr;   // null: the whole track (its unmuted clips)
    double startBeat = 0.0, endBeat = 0.0;
    juce::String problem;         // not empty when there is nothing to write out, and why

    bool isOk() const noexcept { return track != nullptr && problem.isEmpty(); }
};

/** How far past the range the render goes, so the arpeggiator hears the clip's last note-offs (beats). */
constexpr double writeOutExtraBeats = 1.0;

/** The range of the clip (or of the track's unmuted clips with notes), or why there is nothing to write out: not a MIDI
    track, no MIDI effect that isn't bypassed, a muted or empty clip, or DawIt is recording. */
WriteOutPlan planWriteOut (const Engine&, Track&, const Clip* clip = nullptr);

/** The renderer settings for the plan: one chain, its MIDI effects only, at 48 kHz, filling `capture`. */
Settings getWriteOutSettings (const WriteOutPlan&, MidiCapture& capture);

/** Turns captured MIDI events into notes relative to `clipStartBeat`: a note-on is paired with the next note-off of its
    pitch (or a velocity-0 note-on; all-notes-off ends every note). Notes still sounding at `lengthBeats` are cut there,
    notes starting at or after it and zero-length notes are dropped. Velocity is the 7-bit value / 127. */
std::vector<MidiNote> pairNotes (const std::vector<MidiCapture::Event>&, double clipStartBeat, double lengthBeats);

/** How a write-out went. */
struct WriteOutResult
{
    juce::Result result = juce::Result::ok();
    Track* track = nullptr;       // the MIDI track the result is on
    MidiClip* clip = nullptr;
    int numNotes = 0;
    juce::String note;            // what to tell the user: a plugin that couldn't be loaded on the new track
};

/** Message thread, once the render filled `capture`: puts the notes in a new clip at the plan's start on a MIDI track
    right below the original ("<name> (arpeggio)", reused when it is already there with no MIDI effect, the same bus and
    a free place), copies the instrument, audio effects, colour, volume, pan, sends and clef (not the MIDI effects) onto a
    new track, mutes the source clip (or track) and remembers the undo. The source's notes and plugins never change.
    Fails, changing nothing, when no note came out. */
WriteOutResult addWrittenOutTrack (Engine&, const WriteOutPlan&, const MidiCapture&);

/** Plans, renders and adds, all on the calling thread (the message thread): for tests and small jobs. */
WriteOutResult writeOutArpeggio (Engine&, Track&, const Clip* clip = nullptr);

/** Writes an audio clip's own audio times its clip gain to a WAV file at its own rate and channels: 24-bit, or
    32-bit float when that peaks over 0 dB. `bitsPerSample`, when given, gets the one used. */
juce::Result writeClipToWav (const AudioClip&, const juce::File&, int* bitsPerSample = nullptr);

/** Bounces on the calling thread, which must be the message thread (for tests and small bounces). */
Outcome bounceToFile (Engine&, const Settings&, const juce::File&);
} // namespace daw::bounce
