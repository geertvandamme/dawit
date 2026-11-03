#pragma once

#include <memory>
#include <vector>

#include "Chord.h"
#include "Clip.h"
#include "Lyrics.h"
#include "MusicalKey.h"
#include "Section.h"
#include "TempoMap.h"
#include "Track.h"

namespace daw
{
class Engine;

/** Undo and redo: snapshots of the song's undoable state, compared and named. The history itself (the list of
    snapshots, undo and redo) lives in the Engine; this file holds what a snapshot is and the pure functions on it. */
namespace undo
{
    /** A MIDI clip's notes. Snapshots of a clip whose notes did not change share one list. */
    using NoteList = std::shared_ptr<const std::vector<MidiNote>>;

    /** A clip as the snapshot knows it: the object itself (kept alive while a snapshot needs it, so undo can put
        the same object back) and the fields an edit can change. Audio samples never change once a clip is in the
        edit, so they are not copied. */
    struct ClipState
    {
        std::shared_ptr<Clip> clip;
        juce::String name;
        double startBeat = 0.0;
        bool muted = false;
        float gainDb = 0.0f;          // audio only
        double lengthBeats = 0.0;     // MIDI only
        NoteList notes;               // MIDI only
    };

    /** A send of a track: where it goes, how loud, and whether it is taken before the fader. */
    struct SendState
    {
        Track* target = nullptr;
        float levelDb = 0.0f;
        bool preFader = false;
    };

    /** A plugin slot in a track's chain, with its bypass. The plugin's own settings are not part of a snapshot. */
    struct PluginState
    {
        std::shared_ptr<PluginSlot> slot;
        bool bypassed = false;
    };

    /** A track as the snapshot knows it. Mute, solo, arm, monitor, input, fold and the scale filter are left out:
        they are listening tools, not part of the song. */
    struct TrackState
    {
        std::shared_ptr<Track> track;
        Track* bus = nullptr;
        juce::String name;
        juce::Colour colour;
        float volumeDb = 0.0f, pan = 0.0f;
        BuiltInSound builtInSound = BuiltInSound::piano;
        ScoreClef clef = ScoreClef::automatic;
        std::vector<SendState> sends;
        std::vector<PluginState> plugins;
        std::vector<ClipState> clips;
    };

    /** Everything undo puts back: the song rows, the master volume and the tracks in edit order. */
    struct Snapshot
    {
        MusicalKey key;
        TempoMap tempo;
        ChordMap chords;
        SectionList sections;
        std::shared_ptr<const Lyrics> lyrics;
        LyricsBrief brief;
        float masterVolumeDb = 0.0f;
        std::vector<TrackState> tracks;
    };

    /** Takes a snapshot of the edit (message thread). With `previous`, a MIDI clip whose notes and the lyrics that
        did not change share their storage with it, so a step costs about what it changed. */
    Snapshot capture (const Engine&, const Snapshot* previous = nullptr);

    /** True when the edit is, in everything a snapshot holds, as the snapshot says. Makes no copies, so a timer can
        call it. */
    bool matches (const Snapshot&, const Engine&);

    /** True when two snapshots hold the same song. */
    bool equal (const Snapshot&, const Snapshot&);

    /** A short name for what changed from `before` to `after`, for the Edit menu: "Delete track 'Bass'", "Add note in
        'Lead'", "Change chords". Names the most structural change first: tracks added or removed, tracks moved,
        plugins, clips, notes, song rows, the mixer, names and colours. Empty when the snapshots are equal. */
    juce::String describe (const Snapshot& before, const Snapshot& after);
} // namespace undo
} // namespace daw
