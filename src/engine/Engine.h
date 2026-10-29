#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Chord.h"
#include "Lyrics.h"
#include "Section.h"
#include "TempoMap.h"

#include <array>
#include "MusicalKey.h"
#include "SoundGuide.h"
#include "Track.h"
#include "Tuner.h"

namespace daw
{
/**
    What an offline bounce plays (see bounce::Renderer). Plain data, written on the message thread under
    Engine::getLock() and read by renderOffline(), which holds the lock; only honoured while offline.
*/
struct BounceFocus
{
    enum class Mode
    {
        mix,             // everything, as playback sounds
        heardAsSoloed,   // the tracks in `tracks` stand in for the solo buttons (which are left alone)
        oneChain         // `track` alone: its clips through its instrument and effects, at unity gain, straight out
    };

    Mode mode = Mode::mix;
    std::vector<const Track*> tracks;   // heardAsSoloed
    const Track* track = nullptr;       // oneChain
    const Clip* clip = nullptr;         // oneChain: only this clip plays (null: all the track's clips)
    bool instrumentOnly = false;        // oneChain: skip the effects
};

/**
    Owns the audio device, the plugin infrastructure and the edit (tracks, clips, transport).

    Threading model: the edit is only ever mutated on the message thread, and always while
    holding getLock(). The audio thread only try-locks it, so the UI can never stall audio
    for longer than a single (short) mutation. Mixer controls and transport state are atomics.

    A ChangeMessage is broadcast whenever the edit structure changes.
*/
class Engine final : public juce::AudioIODeviceCallback,
                     public juce::ChangeBroadcaster,
                     private juce::ChangeListener,
                     private juce::AudioPlayHead,
                     private juce::Timer
{
public:
    /** With `scratchSettings` the app settings (view state, devices, scanned plugins) live in a folder of their
        own, DawIt/Scratch, emptied each time, so a developer run such as --screenshots starts the same every
        time and never reads or changes the user's settings. */
    explicit Engine (bool scratchSettings = false);
    ~Engine() override;

    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager audioFormats;
    juce::AudioPluginFormatManager pluginFormats;
    juce::KnownPluginList knownPlugins;
    juce::ApplicationProperties properties;

    juce::File getDeadMansPedalFile();

    //==============================================================================
    // Edit — message thread only.
    juce::CriticalSection& getLock() noexcept { return lock; }
    const Tracks& getTracks() const noexcept { return tracks; }

    /** Empties the edit for a new project: no tracks, chords or lyrics, C major at 120 BPM, transport stopped
        at the start. Forget any pointers to tracks and clips first. */
    void resetEdit();

    /** Goes up with every resetEdit(), so views can tell that the tracks were replaced even when
        a new track or clip happens to get an old one's address. */
    int getEditGeneration() const noexcept { return editGeneration; }

    /** Adds a track right below the track at `index - 1`, in the same bus as that track (below everything in it
        when that track is a bus); at index 0 first, and with -1 (or past the end) last at the top level. */
    Track* addTrack (Track::Type, int index = -1);

    /** An empty bus, placed as addTrack places a track, named "Bus 1", "Bus 2"... */
    Track* addBus (int index = -1);

    /** A new return track at the end of the song, at the top level, with no effects: "Return 1", "Return 2"...,
        or `name` (made unique with a number when taken: "Reverb 2"). Nothing sends to it until setSend. */
    Track* addReturn (const juce::String& name = {});

    /** Makes `from` send a copy of itself to the return at `levelDb`, creating the send when it has none (at or below
        Off, a send that exists stays at Off, and none is made). Keeps its pre-fader choice. False when `ret` isn't a return of this edit or
        `from` is a return. */
    bool setSend (Track& from, Track& ret, float levelDb);

    /** Removes the send from `from` to the return (nothing happens when it has none). */
    void removeSend (Track& from, Track& ret);

    /** Create bus: a new bus at the place of the first of `tracks`, in the closest bus they all play into, holding
        the outermost of them in edit order. `name` empty: "Bus N". Returns null for an empty list. Never changes
        a track's sound, instrument, effects or clips. */
    Track* createBus (const std::vector<Track*>& tracks, const juce::String& name = {});

    /** Like createBus, but the new bus goes right after the bus `beside`, in the bus `beside` is in, and the tracks
        leave their buses for it. For tracks that sit in a bus of another kind (drums in Vocals): the new bus is
        next to that bus instead of inside it. Falls back to createBus when `beside` isn't a bus of this edit or
        the tracks hold it. */
    Track* createBusBeside (const std::vector<Track*>& tracks, Track& beside, const juce::String& name = {});

    /** Moves a track (with everything in it) into `bus` (null: the master), before `before` (null: at the end).
        False, changing nothing, when routing::canMoveInto says no or `before` isn't in that bus. */
    bool moveTrack (Track&, Track* bus, const Track* before = nullptr);

    /** The track goes to where its bus plays, right below the bus. False when it isn't in a bus. */
    bool removeFromBus (Track&);

    /** The bus's tracks take its place and output, in order; the bus and its effects are removed. */
    void ungroup (Track& bus);

    /** Folds a bus (hides its tracks' rows) or opens it again. */
    void setFolded (Track& bus, bool);

    /** Removes the track and, for a bus, everything in it. Removing a return removes the sends to it. */
    void removeTrack (Track*);
    int indexOf (const Track*) const;
    Track* findTrackFor (const Clip*) const;   // null if the clip no longer exists

    AudioClip* addAudioClip (Track&, const juce::File&, double startBeat, juce::String& error);

    /** Adds a clip with audio that has no file yet; saving the project writes it to one. */
    AudioClip* addAudioClip (Track&, juce::AudioBuffer<float>, double sampleRate, double startBeat, const juce::String& name);
    MidiClip* addMidiClip (Track&, double startBeat, double lengthBeats);
    Clip* duplicateClip (Track&, const Clip&);
    void removeClip (Track&, const Clip*);
    void moveClip (Track& from, const Clip*, Track& to);

    /** Cuts a clip in two at `beat` (absolute), which must lie inside the clip. Notes that cross
        the cut end at it. Returns the new right-hand part, or null if `beat` isn't inside.
        Split audio is kept in memory; saving writes each part to its own file. */
    Clip* splitClip (Track&, Clip&, double beat);
    bool canSplit (const Clip&, double beat) const;

    /** The clip of the same kind on the track that comes right after (or before) this one,
        ordered by start; null if there is none. */
    static Clip* findNeighbourClip (const Track&, const Clip&, bool next);

    /** Merges two clips of the same kind into the one that starts first, which then runs to the
        later end; the other clip is removed. The joined clip sounds the same: MIDI keeps only
        notes that could be heard, and audio gets silence in a gap and is mixed where the clips
        overlap. Returns the remaining clip, or null if the clips can't be joined. */
    Clip* joinClips (Track&, Clip&, Clip&);

    /** Mutes or unmutes one clip; the rest of its track plays on. */
    void setClipMuted (Clip&, bool);

    /** Plays the clip's audio backwards. Like split audio, it is kept in memory and saving
        writes it to a new file. */
    void reverseClip (AudioClip&);

    /** Sets an audio clip's gain in dB, limited to AudioClip::minGainDb..maxGainDb. */
    void setClipGain (AudioClip&, float gainDb);

    /** Call after editing MIDI notes, so held notes get released and the UI refreshes. */
    void notesChanged();

    /** Moves every note of the clip by `semitones`. Does nothing (and returns false) if that would
        push a note outside the MIDI range; see canTranspose(). */
    bool transposeClip (MidiClip&, int semitones);
    static bool canTranspose (const MidiClip&, int semitones);

    /** Creates a plugin and inserts it into the track's chain (-1 = at the end). */
    PluginSlot* addPlugin (Track&, const juce::PluginDescription&, juce::String& error, int insertIndex = -1);
    void removePlugin (Track&, const PluginSlot*);
    void movePlugin (Track&, const PluginSlot*, int newIndex);
    void setPluginBypassed (PluginSlot&, bool);

    /** The track that receives note previews, and MIDI input while no track is armed. */
    void setLiveMidiTrack (Track* t) noexcept { liveMidiTrack = t; }
    Track* getLiveMidiTrack() const noexcept { return liveMidiTrack; }
    void previewNote (int pitch, float velocity, int durationMs = 250);

    /** Previews play on the live track when it is a MIDI track that plays notes. Otherwise (no
        track selected, an audio track, or one playing drums) they play on the engine's own built-in
        synth, in this sound, so guides can always be heard. Read at each note-on. */
    void setPreviewSound (BuiltInSound sound) noexcept { previewSound = sound; }

    /** True when previews play on the engine's own synth rather than the live track. */
    bool previewsUseOwnSound() const noexcept { return isOwnPreviewSoundNeeded (liveMidiTrack.load()); }

    /** A plain tone for the sound guide, heard on top of everything else (but never in a bounce):
        a shape at a frequency and level. With a copy, a second one shifted by phaseDegrees (which may
        be more than a cycle) is added, flipped upside down with flipCopy, to hear two waves add up or
        cancel. It fades in and out, and glides to a new frequency or phase, so changing it doesn't click. */
    struct Tone
    {
        bool on = false;
        soundguide::Shape shape = soundguide::Shape::sine;
        float frequency = 440.0f, levelDb = -18.0f, phaseDegrees = 0.0f;
        bool withCopy = false, flipCopy = false;
    };

    void setTone (const Tone&) noexcept;

    /** Plays a buffer (mono or stereo, already at the device's sample rate) on top of everything, like
        the tone but with its own start and end: the Listen button of Hits to MIDI. It starts at
        `startSample`, stops by itself at the end, when `stopAudition()` is called, and as soon as the
        transport plays; it is never heard in a bounce. Message thread; replaces any buffer playing. */
    void startAudition (juce::AudioBuffer<float> buffer, int startSample = 0);

    /** Stops the audition player and frees its buffer. Message thread. */
    void stopAudition();

    /** True while the audition player is playing. Any thread. */
    bool isAuditioning() const noexcept { return auditionOn.load(); }

    /** The audition player's read position in samples (where it will play next, the buffer's length once
        it has ended). Any thread; for the window's playhead. */
    int getAuditionPosition() const noexcept { return auditionPosition.load(); }

    /** Plays only what differs between left and right on both speakers (left minus right, the old
        trick for taking the vocals out), for the sound guide to compare with Remove vocals. It fades
        over 50 ms, so switching doesn't click, and never changes a bounce or the meters. */
    void setLeftMinusRight (bool on) noexcept { setSpeakerMode (on ? SpeakerMode::leftMinusRight : SpeakerMode::stereo, SpeakerMode::leftMinusRight); }
    bool isLeftMinusRight() const noexcept { return speakerMode.load() == (int) SpeakerMode::leftMinusRight; }

    /** Plays left and right added together, (L+R)/2, on both speakers, as a phone or a club's subwoofers
        play it, for the sound guide's mono check. Like left minus right it fades over 50 ms and never
        changes a bounce or the meters. Mono and left minus right are never both on: switching one on
        switches the other off. */
    void setMono (bool on) noexcept { setSpeakerMode (on ? SpeakerMode::mono : SpeakerMode::stereo, SpeakerMode::mono); }
    bool isMono() const noexcept { return speakerMode.load() == (int) SpeakerMode::mono; }

    /** Arms a MIDI or audio track for recording. While any MIDI track is armed, MIDI input goes to the armed
        tracks instead of the live track. Arming an audio track opens the device's inputs (see areInputsWanted());
        disarming the last one closes them. An audio track's arm can't change while recording. */
    void setArmed (Track&, bool);

    /** Chooses the input an audio track records: the device's channel (0-based) and whether it and the next one
        are a stereo pair. Ignored for other tracks and while recording. Saved with the project. */
    void setInput (Track&, int firstChannel, bool stereo);

    /** Turns Monitor on or off for an audio track: its input played through its effects and fader. Never saved. */
    void setMonitor (Track&, bool);

    /** Feeds a message in as if it came from a MIDI input device. */
    void addMidiInput (const juce::MidiMessage&);

    //==============================================================================
    // Recording — message thread.

    /** Starts playback and records MIDI input into a new clip on each armed MIDI track. If no track
        is armed, the live track is armed first. Returns false when there is no MIDI track to record on. */
    bool startRecording();

    /** Ends recording; playback goes on. Stopping or pausing the transport also ends it, and so does the device
        stopping or changing its rate (what was recorded is kept). Takes in which nothing was played are removed.
        Audio takes become clips at the place they were recorded, one per pass of a loop. */
    void stopRecording();

    bool isRecording() const noexcept { return recording.load(); }

    /** Does what the 30 Hz recording timer does: moves what the audio thread captured into the takes. The timer
        calls it while recording; tests call it between blocks they feed through the callback. */
    void serviceRecording();

    //==============================================================================
    // Audio input and audio recording — message thread unless noted.

    /** True while the device's inputs should be open: an audio track is armed, or a view shows the input meter
        (setInputMeterWanted). The engine opens up to 8 input channels then and closes them otherwise, so the
        microphone is only on while it is needed. Opening happens only for the device the engine is playing through. */
    bool areInputsWanted() const;

    /** The Audio / MIDI Settings panel asks for the inputs while it shows its input meter. */
    void setInputMeterWanted (bool);

    /** The tuner window asks for the inputs while it listens: the engine opens them (see areInputsWanted()) and copies
        the chosen input into getTunerTap(), which the window's tuner::Listener reads. */
    void setTunerListening (bool);

    /** Chooses the input the tuner hears (the device's channel, 0-based, and whether it and the next are a stereo pair). */
    void setTunerInput (int firstChannel, bool stereo) noexcept { tunerTap.setInput (firstChannel, stereo); }

    /** While the tuner listens and this is on, monitored inputs are silent in the mix. Recording still captures them. */
    void setTunerMuteMonitoring (bool on) noexcept { tunerMuteMonitoring = on; }

    tuner::InputTap& getTunerTap() noexcept { return tunerTap; }

    /** How many input channels the device delivered in the last block (0 when none are open). Any thread. */
    int getNumInputChannels() const noexcept { return numInputChannels.load(); }

    /** The loudest peak on any input channel of the device, falling back over time (linear gain; 0 without inputs).
        What the settings panel's input meter shows. Any thread. */
    float getDeviceInputLevel() const noexcept { return deviceInputLevel.load(); }

    /** True when an armed audio track has had exactly zero input for a second: nothing is connected, or the
        system refuses the microphone (macOS gives zeros without an error). Any thread. */
    bool isInputAllZero() const noexcept { return inputAllZero.load(); }

    /** Where takes are written: the project's Audio Files folder, or (an empty folder) DawIt's own Recordings
        folder in the user's application data, which saving the project copies into the project. */
    void setRecordingFolder (const juce::File&);
    juce::File getRecordingFolder() const;

    /** Samples between the audio a take heard and what is in the file: the device's reported input and output
        latency plus the measured extra for this pair of devices. Takes start this much later in the input. */
    int getLatencyShiftSamples() const;

    /** What the measuring of the device's latency found beyond what it reports (samples, either sign); added to
        the shift of every take. */
    void setMeasuredLatencyExtraSamples (int samples) noexcept { measuredLatencyExtra = samples; }
    int getMeasuredLatencyExtraSamples() const noexcept { return measuredLatencyExtra.load(); }

    /** True when this pair of input and output devices has been measured (now or in an earlier session). */
    bool hasMeasuredLatency() const noexcept { return latencyMeasured.load(); }

    /** The settings key that remembers the measured extra for the current pair of devices, in milliseconds:
        "recording.extraLatencyMs.<input device>|<output device>". Empty when there is no device. */
    juce::String getLatencySettingsKey() const;

    /** What a measuring found. */
    struct LatencyMeasurement
    {
        bool ok = false;                 // enough clicks came back at the same delay
        int clicksFound = 0;             // of 8
        double roundTripSamples = 0.0;   // measured: from a click played to the same click heard
        double reportedSamples = 0.0;    // what the device says (input plus output latency)
        double extraSamples = 0.0;       // measured minus reported, either sign
        double sampleRate = 0.0;
        juce::String problem;            // when not ok: what to try
    };

    /** Starts playing eight clicks 0.25 s apart at -12 dBFS (outside the song, nothing else is heard meanwhile)
        and listening on the inputs. Message thread. False when there is no input, a recording or bounce is
        running, or a measuring already runs. */
    bool startLatencyMeasurement();

    /** True from startLatencyMeasurement until serviceLatencyMeasurement has taken the result. */
    bool isMeasuringLatency() const noexcept { return measureState.load() != 0; }

    /** 0 to 1: how far the measuring is. */
    double getLatencyMeasurementProgress() const noexcept;

    /** Stops a measuring without a result. */
    void cancelLatencyMeasurement();

    /** Message thread (call it from a timer while measuring): when the listening is over, reads the round trip.
        A good result is applied (the extra for every later take) and remembered for this pair of devices.
        Returns true once, when a result is ready for getLastLatencyMeasurement. */
    bool serviceLatencyMeasurement();
    LatencyMeasurement getLastLatencyMeasurement() const { return lastMeasurement; }

    /** What happened to one armed audio track's recording, for messages after stopping. */
    struct TakeReport
    {
        juce::String trackName;
        int inputChannel = 0;           // 0-based, as chosen
        bool stereo = false;
        int numTakes = 0;               // clips made (one per pass of a loop)
        bool removedAsSilent = false;   // everything came in quieter than -60 dB, so no clip was made
        double peakDb = -100.0;         // of the loudest take (or of what was recorded when silent)
        double lostSeconds = 0.0;       // samples dropped because the message thread stalled; silent in the take
        juce::String error;             // a take that couldn't be written or read back
    };

    /** The audio tracks of the last recording, and a number that goes up with each one. Message thread. */
    const std::vector<TakeReport>& getLastRecordingReport() const noexcept { return lastReport; }
    int getRecordingReportVersion() const noexcept { return reportVersion; }

    /** The recording in progress, for the lanes to draw: one entry per armed audio track, with the beat where it
        started, how many seconds are in it and a few peaks for the waveform. Message thread. */
    struct LiveTake
    {
        const Track* track = nullptr;
        double startBeat = 0.0;
        double seconds = 0.0;
        const std::vector<float>* peaks = nullptr;   // the loudest sample of each peakBlockSize samples so far
        static constexpr int peakBlockSize = 512;
    };
    std::vector<LiveTake> getLiveTakes() const;

    /** Makes `chosen` the audio take that plays: unmuted, with every clip overlapping it muted. */
    void chooseTake (Track&, AudioClip& chosen);

    /** Removes the other takes of `chosen`: the clips that overlap it. */
    void deleteOtherTakes (Track&, AudioClip& chosen);

    //==============================================================================
    // Metronome — any thread. The click is mixed in after the master volume.
    std::atomic<bool> metronomeEnabled { false };
    std::atomic<bool> metronomeOnlyWhileRecording { false };

    /** When on, startRecording() first clicks one bar while the playhead waits; recording starts after it. */
    std::atomic<bool> countInEnabled { false };

    /** Beats of count-in still to go, or 0 when not counting in. */
    double getCountInBeatsLeft() const noexcept { return countInBeatsLeft.load(); }

    //==============================================================================
    // Transport — any thread.
    void play() noexcept { playing = true; }
    void stop() noexcept { playing = false; }
    void togglePlay() noexcept { playing = ! playing.load(); }
    bool isPlaying() const noexcept { return playing.load(); }

    void setPositionBeats (double beats) noexcept;
    double getPositionBeats() const noexcept;

    /** The position being heard now: while playing, what comes out of the speakers left the
        engine the output's latency ago, so it's that much behind getPositionBeats(). Views that
        follow the music (tapping the lyrics, playing along) use it. */
    double getHeardPositionBeats() const;

    /** The tempo at the playhead: the song's tempo, or the one playing now when the tempo
        changes. Message thread only. */
    double getTempo() const { return tempoMap.getBpmAt (getPositionBeats()); }

    /** One tempo for the whole song, replacing any tempo changes. Message thread only. */
    void setTempo (double bpm) { setTempoMap (TempoMap (bpm)); }

    /** The song's tempo over time: one tempo, or a tempo per stretch of bars that follows a band
        (see TempoMap). Song data like the chords, saved with the project. Message thread only;
        the audio thread reads it under the lock. */
    const TempoMap& getTempoMap() const noexcept { return tempoMap; }
    void setTempoMap (TempoMap);

    /** Key of the song — message thread only. Not used for playback; meant for composition tools
        such as melody and chord generation. */
    const MusicalKey& getKey() const noexcept { return key; }

    void setKey (const MusicalKey& newKey)
    {
        if (newKey != key)
        {
            key = newKey;
            keyPitchClasses = getPitchClassBits (key);
            sendChangeMessage();
        }
    }

    /** Turns the track's scale filter on or off (see Track::scaleFilter): it shows only the song
        key's notes and keeps other keys of the MIDI keyboard from sounding or being recorded. */
    void setScaleFilter (Track& track, bool on)
    {
        if (track.scaleFilter != on)
        {
            track.scaleFilter = on;
            sendChangeMessage();
        }
    }

    /** Sets the clef the Score view writes this track's part in (see Track::scoreClef). Message thread, under the lock. */
    void setScoreClef (Track& track, ScoreClef clef)
    {
        if (track.scoreClef != clef)
        {
            track.scoreClef = clef;
            sendChangeMessage();
        }
    }

    /** Picks the sound of the track's built-in synth (see Track::builtInSound). Notes already
        sounding finish in the old sound. */
    void setBuiltInSound (Track& track, BuiltInSound sound)
    {
        if (track.builtInSound != sound)
        {
            track.builtInSound = sound;
            sendChangeMessage();
        }
    }

    /** The chord track — message thread only. One chord per bar at most; like the key it makes no
        sound and only guides composition tools. */
    const ChordMap& getChords() const noexcept { return chords; }
    const Chord* getChord (int bar) const
    {
        const auto it = chords.find (bar);
        return it != chords.end() ? &it->second : nullptr;
    }

    /** Sets or (with nullopt) clears the chord of one bar. */
    void setChord (int bar, const std::optional<Chord>& chord)
    {
        if (bar < 0)
            return;

        const juce::ScopedLock sl (lock);   // the audio thread reads the chords of the bar it plays (for DawIt Tune)

        if (chord.has_value())
            chords[bar] = *chord;
        else
            chords.erase (bar);

        sendChangeMessage();
    }

    void setChords (ChordMap newChords)
    {
        {
            const juce::ScopedLock sl (lock);
            chords = std::move (newChords);
        }

        sendChangeMessage();
    }

    /** The song's sections (intro, verse, chorus...) — message thread only. Like the chord track they
        make no sound: they mark the song's structure. */
    const SectionList& getSections() const noexcept { return sections; }

    /** Replaces the sections; they are sorted and made to fit together (see tidySections()). */
    void setSections (SectionList newSections)
    {
        sections = tidySections (std::move (newSections));
        sendChangeMessage();
    }

    /** The song's lyrics, placed in time — message thread only. Like the chord track they make no sound. */
    const Lyrics& getLyrics() const noexcept { return songLyrics; }

    /** Replaces the lyrics, put in order first (see lyrics::tidy()). */
    void setLyrics (Lyrics newLyrics)
    {
        lyrics::tidy (newLyrics);

        if (newLyrics != songLyrics)
        {
            songLyrics = std::move (newLyrics);
            sendChangeMessage();
        }
    }

    /** What the song is about, for the Lyrics window's Write with AI panel — message thread only. Saved with the song. */
    const LyricsBrief& getLyricsBrief() const noexcept { return lyricsBrief; }

    /** Replaces the brief, tidied first (see lyrics::brief::tidy()). */
    void setLyricsBrief (LyricsBrief newBrief)
    {
        newBrief = lyrics::brief::tidy (std::move (newBrief));

        if (newBrief != lyricsBrief)
        {
            lyricsBrief = std::move (newBrief);
            sendChangeMessage();
        }
    }

    /** The section a bar lies in, or null. */
    const Section* getSectionAt (int bar) const
    {
        for (auto& s : sections)
            if (bar >= s.startBar && bar < s.getEndBar())
                return &s;

        return nullptr;
    }

    /** The bar after the song's last clip or section ends (0 for an empty song); the chord track
        doesn't count. Message thread only. */
    int getSongEndBar() const
    {
        double endBeat = 0.0;

        for (auto& track : tracks)
            for (auto& clip : track->clips)
                endBeat = juce::jmax (endBeat, clip->getEndBeat (tempoMap));

        int endBar = (int) std::ceil (endBeat / 4.0 - 1.0e-6);

        for (auto& s : sections)
            endBar = juce::jmax (endBar, s.getEndBar());

        return endBar;
    }

    std::atomic<bool> loopEnabled { false };
    std::atomic<double> loopStartBeat { 0.0 }, loopEndBeat { 16.0 };

    std::atomic<float> masterVolumeDb { 0.0f };
    std::atomic<float> masterPeakLeft { 0.0f }, masterPeakRight { 0.0f };

    // Peak levels of all tracks together before the master volume and after it (without the
    // metronome), since the UI last read them.
    std::atomic<float> masterInputPeak { 0.0f }, masterOutputPeak { 0.0f };

    double getSampleRate() const noexcept { return sampleRate; }

    //==============================================================================
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;

    //==============================================================================
    // Offline rendering, for bouncing to a file (see bounce::Renderer).

    /** Message thread: the audio device plays silence from now on, and the edit is prepared to be
        rendered at `rate` by renderOffline(), with plugins told they run offline. */
    void beginOffline (double rate, int blockSize);

    /** Renders the next block of the song as playback would, on any one thread, between
        beginOffline() and endOffline(). */
    void renderOffline (float* const* channels, int numChannels, int numSamples);

    /** Message thread: back to playing through the audio device, as before beginOffline(). */
    void endOffline();

    bool isOffline() const noexcept { return offline.load(); }

    /** Message thread: what the bounce plays (see BounceFocus), until clearBounceFocus(). Call it after
        beginOffline(); a focus has no effect while the engine plays live. */
    void setBounceFocus (const BounceFocus&);
    void clearBounceFocus();

private:
    struct AudioRecording;

    /** A note on/off or sustain pedal message from the MIDI input, at its song position. */
    struct RecordedEvent
    {
        double beat = 0.0;
        juce::uint8 bytes[3] {};
    };

    struct Take
    {
        Track* track = nullptr;
        MidiClip* clip = nullptr;
    };

    struct HeldNote
    {
        double beat = 0.0;
        float velocity = 0.8f;
    };

    juce::Optional<PositionInfo> getPosition() const override;
    void timerCallback() override;

    static int getPitchClassBits (const MusicalKey& k)
    {
        int bits = 0;
        for (int interval : k.getIntervals())
            bits |= 1 << ((k.tonic + interval) % 12);

        return bits;
    }

    void captureInput (double startBeat, double samplesPerBeat, int liveMidiOffset, int numSamples);
    void captureAudio (double startBeat, double samplesPerBeat, int offset, int numSamples);
    void meterInputs (int numSamples);
    void drainAudioRecording (AudioRecording&);
    void finishAudioRecordings();
    void updateInputs();
    void collectRecordedMidi();
    void handleRecordedEvent (const RecordedEvent&);
    void finishNote (int pitch, double endBeat);
    void renderClicks (double startBeat, double samplesPerBeat, int offset, int numSamples, bool restart, bool trigger);
    float nextClickSample() noexcept;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void renderSegment (double startBeat, double bpm, int offset, int liveMidiOffset, int numSamples, bool flushNotes, bool restarted);
    void processBlock (const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples);
    void renderTone (int numSamples);
    void renderAudition (int numSamples);
    void prepare (double rate, int blockSize);
    void setPluginsOffline (bool);

    static bool isOwnPreviewSoundNeeded (const Track* live) noexcept
    {
        return live == nullptr || ! live->isMidi()
               || (live->usesBuiltInSynth() && builtInSounds::isPercussive (live->builtInSound.load()));
    }
    juce::Colour nextTrackColour();
    juce::String nextBusName() const;
    juce::String uniqueReturnName (const juce::String& wanted) const;

    /** Where addTrack (type, index) puts a track: the bus and the track it goes before (null: at the end). */
    std::pair<Track*, const Track*> findInsertion (int index) const;

    /** Puts a track into the list as the last of `bus` (null: the master) or before `before`, with the lock held. */
    void insertTrack (std::unique_ptr<Track>, Track* bus, const Track* before);

    /** Moves a track and everything in it, with the lock held; the caller checked the place is allowed. */
    void relocate (Track&, Track* bus, const Track* before);

    /** Rebuilds the order tracks are rendered in; call with the lock held after any change to the tree. */
    void updateRouting();

    juce::CriticalSection lock;
    Tracks tracks;
    std::vector<Track*> renderOrder;   // every track before the buses it plays into
    std::atomic<Track*> liveMidiTrack { nullptr };

    std::atomic<bool> playing { false };
    std::atomic<double> positionBeats { 0.0 };
    std::atomic<double> pendingSeek { -1.0 };
    std::atomic<int> midiEditVersion { 0 };
    std::atomic<bool> deviceRunning { false };

    // While bouncing: the device's own settings, to go back to afterwards.
    std::atomic<bool> offline { false };
    bool deviceWasRunning = false;
    BounceFocus bounceFocus;
    double deviceRate = 0.0;
    int deviceBlockSize = 512;

    // Device configuration (written under the lock when the device starts).
    double sampleRate = 0.0;
    int maxBlockSize = 512;

    // Audio-thread state.
    bool wasPlaying = false, flushPending = false, restartPending = false;
    int lastMidiEditVersion = 0;
    juce::AudioBuffer<float> mixBuffer;
    juce::MidiBuffer liveMidi, previewMidi, chunkPreviewMidi;

    // Previews when there is no live track to play them (see setPreviewSound()).
    std::atomic<BuiltInSound> previewSound { BuiltInSound::piano };
    BuiltInSynth previewSynth { previewSound };
    bool previewsOnOwnSynth = false;   // audio thread, for the current block
    juce::MidiMessageCollector midiCollector, previewCollector;

    // Recording: the audio thread stamps input with its beat and queues it; the message thread
    // turns it into notes.
    static constexpr int recordQueueSize = 4096;
    std::atomic<bool> recording { false };
    juce::AbstractFifo recordQueue { recordQueueSize };
    std::array<RecordedEvent, recordQueueSize> recordBuffer;
    std::vector<Take> takes;
    std::array<std::optional<HeldNote>, 128> heldNotes;
    std::bitset<128> sustainedNotes;
    bool pedalDown = false;
    double recordedUntil = 0.0;

    // Audio recording. The audio thread copies each armed audio track's input into a FIFO of its own once the count-in
    // is over; the message thread (the 30 Hz timer) drains them into the take and, on stop, cuts it into clips.
    std::vector<std::unique_ptr<AudioRecording>> audioRecordings;   // changed on the message thread under the lock
    juce::TimeSliceThread writerThread { "DawIt take writer" };
    std::atomic<juce::AudioIODevice*> activeDevice { nullptr };     // the device of the last audioDeviceAboutToStart
    bool inputsRequested = false;                                   // updateInputs() last asked the device for its inputs
    int recordingShift = 0;                                         // samples dropped from the start of the input
    juce::File recordingFolder, defaultRecordingFolder;
    std::vector<TakeReport> lastReport;
    int reportVersion = 0;
    std::atomic<int> measuredLatencyExtra { 0 };
    std::atomic<bool> latencyMeasured { false };
    std::atomic<int> measureState { 0 };          // 0 idle, 1 playing clicks and listening, 2 heard everything
    std::atomic<int> measurePos { 0 };
    std::vector<float> measureOutput, measureHeard;   // the clicks to play and what came in, from the start of the measuring
    double measureRate = 0.0;
    LatencyMeasurement lastMeasurement;
    void runLatencyMeasurement (const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples);
    void loadMeasuredLatency();
    std::atomic<bool> deviceChangedWhileRecording { false };
    double recordingRate = 0.0, recordingStartBeat = 0.0;
    bool captureStarted = false;          // audio thread: the first block of audio was captured
    double captureExpectedBeat = 0.0;     // audio thread: where the next captured block continues, to spot loop wraps
    bool inputMeterWanted = false;
    tuner::InputTap tunerTap;
    std::atomic<bool> tunerMuteMonitoring { false };
    const float* const* blockInputs = nullptr;   // audio thread: the device's input channels in this block
    int blockNumInputs = 0;
    std::atomic<int> numInputChannels { 0 };
    std::atomic<float> deviceInputLevel { 0.0f };
    std::atomic<bool> inputAllZero { false };
    juce::int64 zeroInputRun = 0;         // audio thread: samples in a row of exactly zero from the armed inputs

    // Count-in: requested by startRecording(), then counted down by the audio thread.
    std::atomic<double> countInRequest { 0.0 }, countInBeatsLeft { 0.0 };
    double countInLeft = 0.0;

    // Metronome click (audio thread).
    juce::AudioBuffer<float> clickBuffer;
    int clickAge = -1;                       // samples since the click started, -1 when silent
    float clickLevel = 0.0f;
    double clickFrequency = 1000.0;
    bool haveClickBeat = false;              // false after a jump: the next click waits for a beat boundary
    double clickBeatIndex = 0.0;
    juce::SmoothedValue<float> masterGain;

    // The sound guide's tone: set by setTone(), played by renderTone() on the audio thread.
    std::atomic<bool> toneOn { false }, toneWithCopy { false }, toneFlipCopy { false };
    std::atomic<soundguide::Shape> toneShape { soundguide::Shape::sine };
    std::atomic<float> toneFrequency { 440.0f }, toneLevel { 0.0f }, tonePhaseShift { 0.0f };
    juce::SmoothedValue<float> toneGain, tonePhase, toneCopy;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> toneHz { 440.0f };
    double toneCycle = 0.0;

    // The audition player: the buffer is set and cleared under `lock`, and the audio thread reads it only
    // while it holds the try-lock; the old buffer is freed on the message thread, after the lock is released.
    juce::AudioBuffer<float> auditionBuffer;
    std::atomic<bool> auditionOn { false };
    std::atomic<int> auditionPosition { 0 };

    // What the speakers play (see setLeftMinusRight() and setMono()): the mix, left plus right, or left minus right.
    enum class SpeakerMode { stereo = 0, mono = 1, leftMinusRight = 2 };
    std::atomic<int> speakerMode { (int) SpeakerMode::stereo };
    juce::SmoothedValue<float> sideOnly, monoOnly;   // 0 plays the mix, 1 the mono sum or the difference

    /** Switches `mode` on, or off (back to stereo) only when it is the one that is on. */
    void setSpeakerMode (SpeakerMode wanted, SpeakerMode owner) noexcept
    {
        if (wanted != SpeakerMode::stereo)
            speakerMode = (int) wanted;
        else if (speakerMode.load() == (int) owner)
            speakerMode = (int) SpeakerMode::stereo;
    }

    // Play-head snapshot that plugins can query while they process.
    double headBeat = 0.0, headBpm = 120.0, headSeconds = 0.0;
    bool headPlaying = false;

    int colourIndex = 0;
    int editGeneration = 0;
    MusicalKey key;
    std::atomic<int> keyPitchClasses { getPitchClassBits (MusicalKey()) };   // for the audio thread
    TempoMap tempoMap;
    ChordMap chords;
    SectionList sections;
    Lyrics songLyrics;
    LyricsBrief lyricsBrief;

    JUCE_DECLARE_WEAK_REFERENCEABLE (Engine)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
};
} // namespace daw
