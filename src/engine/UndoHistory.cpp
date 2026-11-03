#include "UndoHistory.h"

#include <algorithm>
#include <unordered_map>

#include "Engine.h"

namespace daw::undo
{
namespace
{
    bool sameNote (const MidiNote& a, const MidiNote& b)
    {
        return a.pitch == b.pitch && juce::exactlyEqual (a.start, b.start) && juce::exactlyEqual (a.length, b.length)
               && juce::exactlyEqual (a.velocity, b.velocity);
    }

    bool sameNotes (const std::vector<MidiNote>& a, const std::vector<MidiNote>& b)
    {
        return a.size() == b.size() && std::equal (a.begin(), a.end(), b.begin(), sameNote);
    }

    bool sameNotes (const NoteList& a, const NoteList& b)
    {
        return a == b || (a != nullptr && b != nullptr && sameNotes (*a, *b));
    }

    juce::String quote (const juce::String& name)
    {
        return "'" + name + "'";
    }

    //==============================================================================
    // Edit against snapshot, without copies.
    bool matches (const ClipState& s, const Clip& c)
    {
        if (s.clip.get() != &c || s.name != c.name || ! juce::exactlyEqual (s.startBeat, c.startBeat) || s.muted != c.muted)
            return false;

        if (c.kind == Clip::Kind::audio)
            return juce::exactlyEqual (s.gainDb, static_cast<const AudioClip&> (c).gainDb);

        const auto& midi = static_cast<const MidiClip&> (c);
        return juce::exactlyEqual (s.lengthBeats, midi.lengthBeats) && s.notes != nullptr && sameNotes (*s.notes, midi.notes);
    }

    bool matches (const TrackState& s, const Track& t)
    {
        if (s.track.get() != &t || s.bus != t.bus || s.name != t.name || s.colour != t.colour
            || ! juce::exactlyEqual (s.volumeDb, t.volumeDb.load()) || ! juce::exactlyEqual (s.pan, t.pan.load())
            || s.builtInSound != t.builtInSound.load() || s.clef != t.scoreClef
            || s.sends.size() != t.sends.size() || s.plugins.size() != t.plugins.size() || s.clips.size() != t.clips.size())
            return false;

        for (size_t i = 0; i < s.sends.size(); ++i)
            if (s.sends[i].target != t.sends[i]->target || ! juce::exactlyEqual (s.sends[i].levelDb, t.sends[i]->levelDb.load())
                || s.sends[i].preFader != t.sends[i]->preFader.load())
                return false;

        for (size_t i = 0; i < s.plugins.size(); ++i)
            if (s.plugins[i].slot != t.plugins[i] || s.plugins[i].bypassed != t.plugins[i]->bypassed.load())
                return false;

        for (size_t i = 0; i < s.clips.size(); ++i)
            if (! matches (s.clips[i], *t.clips[i]))
                return false;

        return true;
    }

    //==============================================================================
    // Snapshot against snapshot.
    bool equal (const ClipState& a, const ClipState& b)
    {
        return a.clip == b.clip && a.name == b.name && juce::exactlyEqual (a.startBeat, b.startBeat) && a.muted == b.muted
               && juce::exactlyEqual (a.gainDb, b.gainDb) && juce::exactlyEqual (a.lengthBeats, b.lengthBeats) && sameNotes (a.notes, b.notes);
    }

    bool equal (const TrackState& a, const TrackState& b)
    {
        if (a.track != b.track || a.bus != b.bus || a.name != b.name || a.colour != b.colour || ! juce::exactlyEqual (a.volumeDb, b.volumeDb)
            || ! juce::exactlyEqual (a.pan, b.pan) || a.builtInSound != b.builtInSound || a.clef != b.clef
            || a.sends.size() != b.sends.size() || a.plugins.size() != b.plugins.size() || a.clips.size() != b.clips.size())
            return false;

        for (size_t i = 0; i < a.sends.size(); ++i)
            if (a.sends[i].target != b.sends[i].target || ! juce::exactlyEqual (a.sends[i].levelDb, b.sends[i].levelDb)
                || a.sends[i].preFader != b.sends[i].preFader)
                return false;

        for (size_t i = 0; i < a.plugins.size(); ++i)
            if (a.plugins[i].slot != b.plugins[i].slot || a.plugins[i].bypassed != b.plugins[i].bypassed)
                return false;

        for (size_t i = 0; i < a.clips.size(); ++i)
            if (! equal (a.clips[i], b.clips[i]))
                return false;

        return true;
    }

    //==============================================================================
    // Naming.
    const TrackState* findTrack (const Snapshot& s, const Track* t)
    {
        for (auto& ts : s.tracks)
            if (ts.track.get() == t)
                return &ts;

        return nullptr;
    }

    int indexOfTrack (const Snapshot& s, const Track* t)
    {
        for (size_t i = 0; i < s.tracks.size(); ++i)
            if (s.tracks[i].track.get() == t)
                return (int) i;

        return -1;
    }

    juce::String kindName (const Track& t)
    {
        return t.isBus() ? "bus" : t.isReturn() ? "return" : "track";
    }

    juce::String countedTracks (int n)
    {
        return juce::String (n) + " tracks";
    }

    juce::String pluginName (const PluginSlot& slot)
    {
        return slot.instance != nullptr ? slot.instance->getName() : juce::String ("plugin");
    }

    /** Names a change of the track structure (added, removed, moved), or an empty string when there is none. */
    juce::String describeStructure (const Snapshot& before, const Snapshot& after)
    {
        std::vector<const TrackState*> removed, added;

        for (auto& t : before.tracks)
            if (findTrack (after, t.track.get()) == nullptr)
                removed.push_back (&t);

        for (auto& t : after.tracks)
            if (findTrack (before, t.track.get()) == nullptr)
                added.push_back (&t);

        if (! removed.empty())
        {
            if (removed.size() == 1)
            {
                const auto& gone = *removed.front();

                // A bus that left while the tracks that played into it stayed: it was ungrouped.
                if (gone.track->isBus())
                    for (auto& t : before.tracks)
                        if (t.bus == gone.track.get() && findTrack (after, t.track.get()) != nullptr)
                            return "Ungroup " + quote (gone.name);

                return "Delete " + kindName (*gone.track) + " " + quote (gone.name);
            }

            return "Delete " + countedTracks ((int) removed.size());
        }

        if (! added.empty())
        {
            if (added.size() == 1)
                return "Add " + kindName (*added.front()->track) + " " + quote (added.front()->name);

            return "Add " + countedTracks ((int) added.size());
        }

        // The same tracks: one that now plays into another bus, or one that sits at another place.
        for (auto& t : after.tracks)
            if (auto* old = findTrack (before, t.track.get()); old != nullptr && old->bus != t.bus)
                return "Move " + kindName (*t.track) + " " + quote (t.name);

        const TrackState* moved = nullptr;
        int mostMoved = 0;

        for (size_t i = 0; i < after.tracks.size(); ++i)
        {
            const int was = indexOfTrack (before, after.tracks[i].track.get());
            const int distance = std::abs (was - (int) i);

            if (distance > mostMoved)
            {
                mostMoved = distance;
                moved = &after.tracks[i];
            }
        }

        return moved != nullptr ? "Move " + kindName (*moved->track) + " " + quote (moved->name) : juce::String();
    }

    juce::String describePlugins (const TrackState& before, const TrackState& after)
    {
        auto has = [] (const std::vector<PluginState>& list, const PluginState& p)
        {
            return std::any_of (list.begin(), list.end(), [&p] (auto& q) { return q.slot == p.slot; });
        };

        const auto where = " in " + quote (after.name);

        for (auto& p : before.plugins)
            if (! has (after.plugins, p))
                return "Remove " + pluginName (*p.slot) + " from " + quote (after.name);

        for (auto& p : after.plugins)
            if (! has (before.plugins, p))
                return "Add " + pluginName (*p.slot) + " to " + quote (after.name);

        for (size_t i = 0; i < after.plugins.size(); ++i)
            if (after.plugins[i].slot != before.plugins[i].slot)
                return "Move " + pluginName (*after.plugins[i].slot) + where;

        for (size_t i = 0; i < after.plugins.size(); ++i)
            if (after.plugins[i].bypassed != before.plugins[i].bypassed)
                return (after.plugins[i].bypassed ? "Bypass " : "Switch on ") + pluginName (*after.plugins[i].slot) + where;

        return {};
    }

    /** Names the change to the notes of one MIDI clip, given the track it is on. */
    juce::String describeNotes (const std::vector<MidiNote>& before, const std::vector<MidiNote>& after, const juce::String& trackName)
    {
        const auto in = " in " + quote (trackName);

        if (after.size() != before.size())
        {
            // Notes that only appear on one side tell an add or delete from a mix of both.
            auto remaining = before;
            std::vector<MidiNote> appeared;

            for (auto& n : after)
            {
                const auto it = std::find_if (remaining.begin(), remaining.end(), [&n] (auto& m) { return sameNote (n, m); });

                if (it != remaining.end())
                    remaining.erase (it);
                else
                    appeared.push_back (n);
            }

            if (remaining.empty() || appeared.empty())
            {
                const bool added = remaining.empty();
                const auto count = added ? appeared.size() : remaining.size();
                return juce::String (added ? "Add note" : "Delete note") + (count == 1 ? "" : "s") + in;
            }

            return "Edit notes" + in;
        }

        auto left = before, right = after;
        auto order = [] (const MidiNote& a, const MidiNote& b)
        {
            return std::tie (a.start, a.pitch, a.length, a.velocity) < std::tie (b.start, b.pitch, b.length, b.velocity);
        };
        std::sort (left.begin(), left.end(), order);
        std::sort (right.begin(), right.end(), order);

        bool placeChanged = false, lengthChanged = false, velocityChanged = false;

        for (size_t i = 0; i < left.size(); ++i)
        {
            placeChanged = placeChanged || left[i].pitch != right[i].pitch || ! juce::exactlyEqual (left[i].start, right[i].start);
            lengthChanged = lengthChanged || ! juce::exactlyEqual (left[i].length, right[i].length);
            velocityChanged = velocityChanged || ! juce::exactlyEqual (left[i].velocity, right[i].velocity);
        }

        const int kinds = (placeChanged ? 1 : 0) + (lengthChanged ? 1 : 0) + (velocityChanged ? 1 : 0);

        if (kinds != 1)
            return "Edit notes" + in;

        return placeChanged ? "Move notes" + in : lengthChanged ? "Change note length" + in : "Change note velocity" + in;
    }

    juce::String describeClips (const Snapshot& before, const TrackState& old, const TrackState& now)
    {
        auto clipName = [] (const ClipState& c) { return c.name.isNotEmpty() ? c.name : juce::String ("clip"); };
        auto find = [] (const std::vector<ClipState>& list, const Clip* c) -> const ClipState*
        {
            for (auto& s : list)
                if (s.clip.get() == c)
                    return &s;

            return nullptr;
        };
        auto elsewhere = [&before] (const Clip* c)
        {
            for (auto& t : before.tracks)
                for (auto& s : t.clips)
                    if (s.clip.get() == c)
                        return true;

            return false;
        };

        const auto on = " on " + quote (now.name);
        const ClipState* removed = nullptr;
        const ClipState* added = nullptr;
        int numRemoved = 0, numAdded = 0;

        for (auto& c : old.clips)
            if (find (now.clips, c.clip.get()) == nullptr)
            {
                removed = removed != nullptr ? removed : &c;
                ++numRemoved;
            }

        for (auto& c : now.clips)
            if (find (old.clips, c.clip.get()) == nullptr)
            {
                added = added != nullptr ? added : &c;
                ++numAdded;
            }

        if (numRemoved > 0 && numAdded > 0)
            return "Change clips" + on;

        if (numAdded > 0)
        {
            if (elsewhere (added->clip.get()))
                return "Move clip " + quote (clipName (*added)) + " to " + quote (now.name);

            return numAdded == 1 ? "Add clip " + quote (clipName (*added)) + on : "Add " + juce::String (numAdded) + " clips" + on;
        }

        if (numRemoved > 0)
            return numRemoved == 1 ? "Delete clip " + quote (clipName (*removed)) + on : "Delete " + juce::String (numRemoved) + " clips" + on;

        // The same clips: what changed in them.
        for (auto& c : now.clips)
        {
            const auto* was = find (old.clips, c.clip.get());

            if (was == nullptr || equal (*was, c))
                continue;

            if (c.notes != nullptr && was->notes != nullptr && ! sameNotes (c.notes, was->notes))
                return describeNotes (*was->notes, *c.notes, now.name);

            if (! juce::exactlyEqual (c.startBeat, was->startBeat))
                return "Move clip " + quote (clipName (c)) + on;

            if (! juce::exactlyEqual (c.lengthBeats, was->lengthBeats))
                return "Resize clip " + quote (clipName (c)) + on;

            if (c.muted != was->muted)
                return (c.muted ? "Mute clip " : "Unmute clip ") + quote (clipName (c)) + on;

            if (! juce::exactlyEqual (c.gainDb, was->gainDb))
                return "Change gain of clip " + quote (clipName (c)) + on;

            return "Rename clip " + quote (clipName (c)) + on;
        }

        return {};
    }
} // namespace

//==============================================================================
Snapshot capture (const Engine& engine, const Snapshot* previous)
{
    Snapshot s;
    s.key = engine.getKey();
    s.tempo = engine.getTempoMap();
    s.chords = engine.getChords();
    s.sections = engine.getSections();
    s.brief = engine.getLyricsBrief();
    s.masterVolumeDb = engine.masterVolumeDb.load();

    if (previous != nullptr && previous->lyrics != nullptr && *previous->lyrics == engine.getLyrics())
        s.lyrics = previous->lyrics;
    else
        s.lyrics = std::make_shared<const Lyrics> (engine.getLyrics());

    std::unordered_map<const Clip*, const ClipState*> before;

    if (previous != nullptr)
        for (auto& t : previous->tracks)
            for (auto& c : t.clips)
                before[c.clip.get()] = &c;

    for (auto& track : engine.getTracks())
    {
        TrackState ts;
        ts.track = track;
        ts.bus = track->bus;
        ts.name = track->name;
        ts.colour = track->colour;
        ts.volumeDb = track->volumeDb.load();
        ts.pan = track->pan.load();
        ts.builtInSound = track->builtInSound.load();
        ts.clef = track->scoreClef;

        for (auto& send : track->sends)
            ts.sends.push_back ({ send->target, send->levelDb.load(), send->preFader.load() });

        for (auto& slot : track->plugins)
            ts.plugins.push_back ({ slot, slot->bypassed.load() });

        for (auto& clip : track->clips)
        {
            ClipState cs;
            cs.clip = clip;
            cs.name = clip->name;
            cs.startBeat = clip->startBeat;
            cs.muted = clip->muted;

            if (clip->kind == Clip::Kind::audio)
            {
                cs.gainDb = static_cast<const AudioClip&> (*clip).gainDb;
            }
            else
            {
                const auto& midi = static_cast<const MidiClip&> (*clip);
                cs.lengthBeats = midi.lengthBeats;
                const auto old = before.find (clip.get());

                if (old != before.end() && old->second->notes != nullptr && sameNotes (*old->second->notes, midi.notes))
                    cs.notes = old->second->notes;
                else
                    cs.notes = std::make_shared<const std::vector<MidiNote>> (midi.notes);
            }

            ts.clips.push_back (std::move (cs));
        }

        s.tracks.push_back (std::move (ts));
    }

    return s;
}

bool matches (const Snapshot& s, const Engine& engine)
{
    const auto& tracks = engine.getTracks();

    if (s.tracks.size() != tracks.size() || s.key != engine.getKey() || s.tempo != engine.getTempoMap() || s.chords != engine.getChords()
        || s.sections != engine.getSections() || s.brief != engine.getLyricsBrief()
        || ! juce::exactlyEqual (s.masterVolumeDb, engine.masterVolumeDb.load())
        || s.lyrics == nullptr || *s.lyrics != engine.getLyrics())
        return false;

    for (size_t i = 0; i < tracks.size(); ++i)
        if (! matches (s.tracks[i], *tracks[i]))
            return false;

    return true;
}

bool equal (const Snapshot& a, const Snapshot& b)
{
    if (a.tracks.size() != b.tracks.size() || a.key != b.key || a.tempo != b.tempo || a.chords != b.chords || a.sections != b.sections
        || a.brief != b.brief || ! juce::exactlyEqual (a.masterVolumeDb, b.masterVolumeDb)
        || (a.lyrics != b.lyrics && (a.lyrics == nullptr || b.lyrics == nullptr || *a.lyrics != *b.lyrics)))
        return false;

    for (size_t i = 0; i < a.tracks.size(); ++i)
        if (! equal (a.tracks[i], b.tracks[i]))
            return false;

    return true;
}

juce::String describe (const Snapshot& before, const Snapshot& after)
{
    if (equal (before, after))
        return {};

    if (auto structure = describeStructure (before, after); structure.isNotEmpty())
        return structure;

    for (auto& now : after.tracks)
    {
        const auto* old = findTrack (before, now.track.get());

        if (old == nullptr)
            continue;

        if (auto text = describePlugins (*old, now); text.isNotEmpty())
            return text;
    }

    for (auto& now : after.tracks)
    {
        const auto* old = findTrack (before, now.track.get());

        if (old != nullptr)
            if (auto text = describeClips (before, *old, now); text.isNotEmpty())
                return text;
    }

    if (before.key != after.key)
        return "Change key";

    if (before.tempo != after.tempo)
        return "Change tempo";

    if (before.chords != after.chords)
        return "Change chords";

    if (before.sections != after.sections)
        return "Change sections";

    if ((before.lyrics != after.lyrics && *before.lyrics != *after.lyrics) || before.brief != after.brief)
        return "Change lyrics";

    if (! juce::exactlyEqual (before.masterVolumeDb, after.masterVolumeDb))
        return "Change master volume";

    for (auto& now : after.tracks)
    {
        const auto* old = findTrack (before, now.track.get());

        if (old == nullptr)
            continue;

        const auto of = " of " + quote (now.name);

        if (! juce::exactlyEqual (old->volumeDb, now.volumeDb))
            return "Change volume" + of;

        if (! juce::exactlyEqual (old->pan, now.pan))
            return "Change pan" + of;

        if (old->sends.size() != now.sends.size() || ! std::equal (old->sends.begin(), old->sends.end(), now.sends.begin(), [] (auto& x, auto& y)
            { return x.target == y.target && juce::exactlyEqual (x.levelDb, y.levelDb) && x.preFader == y.preFader; }))
            return "Change sends" + of;

        if (old->builtInSound != now.builtInSound)
            return "Change sound" + of;

        if (old->clef != now.clef)
            return "Change clef" + of;

        if (old->name != now.name)
            return "Rename " + kindName (*now.track) + " " + quote (old->name);

        if (old->colour != now.colour)
            return "Change colour" + of;
    }

    return "Edit";
}
} // namespace daw::undo
