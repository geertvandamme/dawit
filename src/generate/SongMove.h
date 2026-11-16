#pragma once

#include "engine/Engine.h"

#include <juce_core/juce_core.h>

#include <vector>

namespace daw::gen
{
/** Moving a whole song to another key: the song's key, its chord track and every note on every MIDI track except drums
    go up or down by the same number of semitones, as one undo step. Recordings, sections, tempo, instruments and plugin
    settings stay as they are. UI-free and tested. The key-change dialog of idea #26 builds on `moveSong`. */
namespace songmove
{
    /** Why a move didn't happen. */
    enum class Refusal { none, noShift, belowMidi, aboveMidi };

    /** What `moveSong` did. `message` says why when nothing moved ("A note on 'Bass' would go below the lowest MIDI note.");
        `undoSteps` is `Engine::getNumUndoSteps()` right after the step was recorded, and `stepName` its name. */
    struct Result
    {
        bool moved = false;
        Refusal refusal = Refusal::none;
        juce::String trackName;
        juce::String message;
        int undoSteps = 0;
        juce::String stepName;
    };

    /** The undo step's name: "Move the song 4 semitones lower (E major to C major)". One semitone is "1 semitone". */
    juce::String getStepName (const MusicalKey& before, int semitones);

    /** The key after a move: the tonic goes up by `semitones` (mod 12) and the scale stays. */
    MusicalKey getMovedKey (const MusicalKey&, int semitones);

    /** The tracks a move changes: MIDI tracks that don't play drums. */
    std::vector<const Track*> getMovedTracks (const Engine&);

    /** Whether a move by `semitones` would be refused: nothing to shift, or a note that would leave MIDI notes 0 to 127.
        Nothing changes. */
    Result check (const Engine&, int semitones);

    /** Moves the song by `semitones`. All or nothing: when any note would leave 0 to 127, nothing changes. Message thread;
        takes the engine lock for the edit, and records one named undo step. */
    Result moveSong (Engine&, int semitones);

    //==============================================================================
    /** What happens to the recordings when the song moves. */
    struct BackingCheck
    {
        bool blocked = false;                          // a heard recording is the only backing: the move is not offered
        std::vector<juce::String> recordingsLeft;      // the names of the heard recordings under the melody that stay in the old key
    };

    /** The heard audio clips (not muted, not silenced by routing) that overlap the melody's bars, leaving out the clips
        in `ignored` (the range take, by address). Blocked when there is at least one and no other heard MIDI track (not
        drums, not the melody track) has notes. */
    BackingCheck checkBacking (const Engine&, const Track* melodyTrack, const std::vector<const Clip*>& ignored);
} // namespace songmove
} // namespace daw::gen
