#pragma once

#include "Engine.h"

#include <optional>

namespace daw::audiotomidi
{
/**
    Audio to MIDI with Spotify's Basic Pitch (ICASSP 2022 model), run by ONNX Runtime on the CPU.

    The model is built into the app. It hears pitched notes in any instrument or voice, several at
    once, and is fast: a whole song takes seconds. The windowing and the note detection follow the
    basic-pitch Python package (v0.4.0) step by step, so the notes match what it finds.
*/
constexpr double sampleRate = 22050.0;
constexpr int numPitches = 88;        // piano range
constexpr int lowestPitch = 21;       // A0

/** Reports progress from 0 to 1; returning false cancels. */
using ProgressFn = std::function<bool (double)>;

/** How sure the model is, per frame (86 per second) and piano key, that a note sounds there
    and that one starts there. */
struct Activations
{
    int numFrames = 0;
    std::vector<float> notes, onsets;   // numFrames * numPitches, frame by frame

    float note (int frame, int key) const   { return notes[(size_t) (frame * numPitches + key)]; }
    float onset (int frame, int key) const  { return onsets[(size_t) (frame * numPitches + key)]; }
};

struct Settings
{
    float onsetThreshold = 0.5f;      // how strong a note start must be
    float frameThreshold = 0.3f;      // how strong a note must stay to go on
    double minNoteSeconds = 0.1277;
    bool melodiaTrick = true;         // also pick up notes whose start wasn't heard
};

struct Note
{
    double start = 0.0, end = 0.0;    // seconds
    int pitch = 60;
    float amplitude = 0.5f;           // 0..1
};

//==============================================================================
/** The audio as mono at 22.05 kHz, as Basic Pitch wants it (low-pass filtered when going down). */
std::vector<float> prepareInput (const juce::AudioBuffer<float>&, double sourceSampleRate);

/** Runs Basic Pitch on 22.05 kHz mono audio. Returns nullopt when cancelled or on an error. Blocks;
    call it from a background thread. */
std::optional<Activations> runModel (const std::vector<float>&, const ProgressFn&, juce::String& error);

/** Turns the model's activations into notes, like basic-pitch's output_to_notes_polyphonic. */
std::vector<Note> findNotes (const Activations&, const Settings&);

/** The time of a model frame, in seconds (basic-pitch's model_frames_to_time). */
double frameToSeconds (int frame);

/** For a melody, a bass line or a voice: where notes overlap, the strongest one wins and the
    others keep only the part that's free, if that's still long enough. This also removes the
    quiet notes the model hears in a tone's overtones. */
std::vector<Note> keepOneAtATime (std::vector<Note>, double minNoteSeconds = Settings().minNoteSeconds);

//==============================================================================
/** Turns notes in seconds from the start of a clip at `clipStartBeat` into clip notes in beats,
    through the song's tempo (and its changes), dropping those after the clip end. With a grid (in
    beats), starts and ends snap to it and every note lasts at least one step; notes of the same
    pitch no longer overlap. */
std::vector<MidiNote> toClipNotes (const std::vector<Note>&, const TempoMap&, double clipStartBeat, double clipLengthBeats, double gridBeats = 0.0);

/** Puts the notes in a new MIDI track right below the audio clip's track, in a clip at the same
    place and of the same length. The track's built-in sound follows the names: bass for a bass,
    the lead for vocals, piano otherwise. Message thread. */
MidiClip* addMidiTrack (Engine&, Track& source, const AudioClip&, std::vector<MidiNote>);
} // namespace daw::audiotomidi
