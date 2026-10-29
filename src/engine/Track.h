#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <bitset>

#include "BuiltInSynth.h"
#include "Clip.h"
#include "SongContext.h"

namespace daw
{
class Track;

/** Which staff a MIDI track's part is written on in the Score view. Auto picks the clef that needs the fewest
    ledger lines; Guitar and Bass guitar write the notes an octave above the sound. The order matches
    gen::notation::ClefChoice. */
enum class ScoreClef { automatic, treble, bass, grand, guitar, bassGuitar };

/** The edit's tracks, in the order the rows show (each bus followed by everything in it). */
using Tracks = std::vector<std::unique_ptr<Track>>;

/** Everything a track needs to render one contiguous slice of the timeline. */
/** Meter ballistics, on the audio thread: the level jumps up to a new peak and falls back about 60 dB a
    second, over the samples just processed. Any number of views can then read the level. */
inline void updateMeterLevel (std::atomic<float>& level, float peak, int numSamples, double sampleRate)
{
    const float fallen = level.load() * (float) std::pow (0.8, 30.0 * numSamples / sampleRate);
    level = juce::jmax (peak, fallen);
}

/** Which device input channels an audio track hears, for a device with `numInputs` channels: its chosen first channel
    (Input 1 when the device has no such channel, as when a project moves to another interface), and for a stereo pair
    the next one (the same channel twice when the device has no next one). A mono track returns the channel twice:
    it plays and is metered on both sides. {-1, -1} when the device has no inputs. */
inline std::pair<int, int> resolveInputChannels (int channel, bool stereo, int numInputs) noexcept
{
    if (numInputs <= 0)
        return { -1, -1 };

    const int left = channel >= 0 && channel < numInputs ? channel : 0;
    return { left, stereo && left + 1 < numInputs ? left + 1 : left };
}

struct RenderContext
{
    double sampleRate = 44100.0;
    double bpm = 120.0;          // the tempo of this piece: the engine splits blocks at tempo changes
    const TempoMap* tempoMap = nullptr;   // the song's, for where audio clips are in seconds
    double startBeat = 0.0;      // timeline position of the first sample
    bool onSoloPath = false;     // while anything is soloed: this track is heard (see routing::isOnSoloPath)
    int numSamples = 0;
    bool playing = false;
    bool flushNotes = false;     // stop / seek / loop wrap / note edits: release held notes
    bool anySolo = false;
    const juce::MidiBuffer* liveMidi = nullptr; // incoming MIDI for the whole device block, or null
    const juce::MidiBuffer* previewMidi = nullptr; // note previews (clicked keys) for the block, or null
    int keyPitchClasses = 0xfff;   // bit per pitch class (bit 0 = C) of the song key, for the scale filter
    int chordPitchClasses = 0;     // the same for the chord in the bar at the start of this slice; 0 in a bar with no chord
    bool restarted = false;        // the song starts, a seek or a loop wrap: clips are read from a new place (not for note edits)
    int liveMidiOffset = 0;      // position of this slice inside liveMidi (and inside the device's input buffers)

    // The audio device's input channels for the whole block (null when none, and in a bounce): an armed audio track
    // with Monitor on adds its input into its chain, from `liveMidiOffset`.
    const float* const* inputs = nullptr;
    int numInputs = 0;
    bool muteMonitoring = false;   // the tuner is open with "mute while tuning": monitored inputs stay silent

    // A bounce of one track's own chain (see BounceFocus): the track plays at unity gain with no pan, no sends and
    // no mute, only `onlyClip` plays when it is set, and with `instrumentOnly` its effects are skipped.
    bool unityOutput = false;
    bool instrumentOnly = false;
    const Clip* onlyClip = nullptr;
};

struct PluginSlot
{
    std::unique_ptr<juce::AudioPluginInstance> instance;
    bool isInstrument = false;
    std::atomic<bool> bypassed { false };

    // Set when the slot is made, from what the plugin implements (see SongContext.h).
    SongContextReceiver* songContext = nullptr;
    bool keepsLatencyWhenBypassed = false;

    // Peak levels (the louder of left and right) going into and coming out of the plugin,
    // since the UI last read them.
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };

    // The energy (sum of squares of left and right) going in and coming out, since the UI last read
    // them: their ratio is how much louder or quieter the plugin makes the sound.
    std::atomic<float> inputEnergy { 0.0f }, outputEnergy { 0.0f };

    // The level coming out of the plugin, falling back over time (see updateMeterLevel()). Unlike the peaks
    // above, reading it takes nothing away, so any number of views (the Signal Flow window) can read it.
    std::atomic<float> outputLevel { 0.0f };
};

/** At or below this level (dB) a send is off. */
constexpr float sendOffDb = -60.0f;

/** A send: a copy of a track, at a level of its own, that goes to a return track while the track plays on as before.
    `target`, and the list of sends in `Track::sends`, are structural (message thread, under Engine::getLock());
    the level and the pre-fader flag are written by the UI like a fader. */
struct Send
{
    explicit Send (Track& returnTrack) : target (&returnTrack) {}

    Track* target;
    std::atomic<float> levelDb { -10.0f };
    std::atomic<bool> preFader { false };   // taken before the track's fader instead of after it
    std::atomic<float> peak { 0.0f };       // of what it sends, since the UI last read it
    std::atomic<float> level { 0.0f };      // of what it sends, falling back over time (for any number of views)

    juce::SmoothedValue<float> gain;        // audio thread only

    /** True when the send carries sound (its level is above Off). */
    bool isOn() const noexcept { return levelDb.load() > sendOffDb; }
};

/** A bus is a track without clips that other tracks play into; it has effects, a fader, pan, mute and solo, and
    plays into the master or into another bus. A return is the same kind of track, but it hears the sends of other
    tracks (copies, each at its own level) and plays into the master only. */
class Track
{
public:
    enum class Type { audio, midi, bus, fxReturn };

    Track (Type, juce::String name, juce::Colour);
    ~Track();

    bool isMidi() const noexcept { return type == Type::midi; }
    bool isAudio() const noexcept { return type == Type::audio; }
    bool isBus() const noexcept { return type == Type::bus; }
    bool isReturn() const noexcept { return type == Type::fxReturn; }

    /** A bus or a return: no clips and no instrument, it plays what comes into it. */
    bool hasInput() const noexcept { return isBus() || isReturn(); }
    bool hasInstrument() const;

    const Type type;
    juce::String name;
    juce::Colour colour;

    // Mixer controls: written by the UI, read by the audio thread.
    std::atomic<float> volumeDb { 0.0f };
    std::atomic<float> pan { 0.0f };          // -1 (left) .. +1 (right)
    std::atomic<bool> muted { false }, soloed { false };

    // Record arm: a MIDI track hears the MIDI keyboard and records it, an audio track records its input. Use Engine::setArmed().
    std::atomic<bool> armed { false };

    // The input an audio track records (0-based first channel of the device, and whether it takes that channel
    // and the next as a stereo pair). Saved with the project when not Input 1, mono. Use Engine::setInput().
    std::atomic<int> inputChannel { 0 };
    std::atomic<bool> inputStereo { false };

    // An armed audio track plays its input through its effects and fader. Not saved. Use Engine::setMonitor().
    std::atomic<bool> monitor { false };

    // The level of the input of an armed audio track, falling back over time (see updateMeterLevel()); 0 when not armed.
    std::atomic<float> inputMeterLevel { 0.0f };

    // Scale filter: the piano roll and the clip previews show only the notes of the song key, in
    // every clip of the track, and keys outside it on the MIDI keyboard neither sound nor record.
    // Notes already in clips play as they are. Use Engine::setScaleFilter().
    std::atomic<bool> scaleFilter { false };

    // The clef the Score view writes this track's part in. Saved with the project; the audio thread never reads it.
    // Use Engine::setScoreClef().
    ScoreClef scoreClef = ScoreClef::automatic;

    // The sound of the built-in synth, which plays while a MIDI track has no instrument plugin.
    // Use Engine::setBuiltInSound().
    std::atomic<BuiltInSound> builtInSound { BuiltInSound::piano };

    /** True when the built-in synth is this track's instrument. */
    bool usesBuiltInSynth() const { return isMidi() && ! hasInstrument(); }

    // Meter levels after volume and pan, falling back over time (see updateMeterLevel()).
    std::atomic<float> peakLeft { 0.0f }, peakRight { 0.0f };
    std::atomic<float> builtInSynthPeak { 0.0f }; // output of the built-in synth, when it plays

    // The level of the source (the track's clips or its built-in synth), going into the effects, falling back
    // over time. Reading it takes nothing away, so any number of views can read it.
    std::atomic<float> chainInputLevel { 0.0f };

    // Peak levels going into the volume and pan (after the plugins) and coming out of them, since
    // the UI last read them; the out level is silent while the track is muted.
    std::atomic<float> faderInputPeak { 0.0f }, faderOutputPeak { 0.0f };

    // Where this track plays into: a bus, or null for the master. Structural, like `folded` (whether a bus's tracks
    // are hidden): only the engine changes them, on the message thread while holding Engine::getLock().
    Track* bus = nullptr;
    bool folded = false;

    // Where this track sends copies of itself (structural, like `bus`). A track or a bus sends; a return doesn't.
    std::vector<std::unique_ptr<Send>> sends;

    // What comes into a bus or a return (all its tracks, or all its sends, added up, before its effects): the peak since the UI last read it,
    // and a level that falls back over time like the meters (for the guides' pictures).
    std::atomic<float> busInputPeak { 0.0f }, busInputLevel { 0.0f };

    // Audio thread only: the sum of a bus's tracks for the block being rendered, and the solo state the engine
    // works out for each block (this track or a bus it plays into is soloed / this track or a track in it is soloed).
    juce::AudioBuffer<float> busInput;
    bool soloAbove = false, soloBelow = false;

    // Audio thread only, worked out for each block: this track counts as soloed (its solo button, or while a bounce
    // plays "as soloed", the tracks it was asked for). Sends read it for the return they send to.
    bool countsAsSoloed = false;

    // Message thread, under Engine::getLock(): the next block starts from its gains, without gliding to them, as a
    // bounce must (every bounce starts exactly at its range).
    bool snapGains = false;

    // Audio thread only, worked out for each block in edit order: a bus this track plays into is muted, which
    // silences its sends too.
    bool mutedAbove = false;

    // Structural state: only mutated on the message thread while holding Engine::getLock().
    std::vector<std::unique_ptr<Clip>> clips;
    std::vector<std::unique_ptr<PluginSlot>> plugins;

    //==============================================================================
    void prepare (double sampleRate, int maxBlockSize);
    void releaseResources();

    /** Grows the plugin buffer so every plugin gets all the channels it asks for (multi-output
        instruments can want dozens). Message thread, while holding Engine::getLock(). */
    void fitPluginChannels();

    /** Silences a bus's or a return's input for the block about to be rendered. Audio thread only. */
    void clearBusInput (int numSamples);

    /** Renders this track and adds its output into `destination` from `destinationOffset`: the mix buffer, or
        a bus's input. A bus starts from the sum of its tracks instead of clips and MIDI. Audio thread only. */
    void process (const RenderContext&, juce::AudioBuffer<float>& destination, int destinationOffset);

    /** How long this track's plugins hold the sound back, in samples: the sum of their reported latencies, those
        bypassed too when they keep holding it back (DawIt's own do). An audio track reads its clips this much earlier
        so it stays in time with the other tracks; buses, returns and MIDI tracks aren't compensated. Audio thread or
        message thread. */
    int getCompensation() const;

    /** The sum of the latencies of the plugins that aren't instruments (those bypassed too when they keep holding
        the sound back), for any kind of track: a bounce of a MIDI track drops this much from the start. */
    int getEffectsLatency() const;

private:
    /** Adds `numSamples` of the audio clips to the scratch buffer, read `readAhead` samples later than the slice starts. */
    void renderAudioClips (const RenderContext&, int readAhead, int numSamples);
    /** Runs the audio thread's chain over the next `numSamples` of the clips and drops the result. */
    void preroll (const RenderContext&, int numSamples);
    void runPlugins (int numSamples, double sampleRate, bool metering, const RenderContext*);
    void collectMidiClipEvents (const RenderContext&);
    void releaseActiveNotes();
    int getRequiredChannels() const;

    juce::AudioBuffer<float> scratch;
    juce::MidiBuffer midi;
    std::bitset<128> activeNotes;
    BuiltInSynth builtInSynth { builtInSound };
    juce::SmoothedValue<float> gainLeft, gainRight;
    juce::SmoothedValue<float> sendFaderGain;   // the fader without pan or mute, for post-fader sends
    int lastCompensation = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};
} // namespace daw
