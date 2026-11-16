#include "SongMove.h"

#include "FirstSong.h"
#include "engine/Routing.h"

#include <cmath>

namespace daw::gen::songmove
{
juce::String getStepName (const MusicalKey& before, int semitones)
{
    const int n = std::abs (semitones);
    return "Move the song " + juce::String (n) + (n == 1 ? " semitone " : " semitones ") + (semitones < 0 ? "lower" : "higher")
           + " (" + before.getName() + " to " + getMovedKey (before, semitones).getName() + ")";
}

MusicalKey getMovedKey (const MusicalKey& key, int semitones)
{
    MusicalKey moved = key;
    moved.tonic = (((key.tonic + semitones) % 12) + 12) % 12;
    return moved;
}

std::vector<const Track*> getMovedTracks (const Engine& engine)
{
    std::vector<const Track*> out;

    for (auto& t : engine.getTracks())
        if (t->isMidi() && ! firstsong::isDrumTrack (*t))
            out.push_back (t.get());

    return out;
}

Result check (const Engine& engine, int semitones)
{
    Result r;

    if (semitones == 0)
    {
        r.refusal = Refusal::noShift;
        r.message = "There is no key change to make.";
        return r;
    }

    for (auto* track : getMovedTracks (engine))
        for (auto& c : track->clips)
            if (c->kind == Clip::Kind::midi)
                for (auto& n : static_cast<const MidiClip&> (*c).notes)
                {
                    const int moved = n.pitch + semitones;

                    if (moved < 0 || moved > 127)
                    {
                        r.refusal = moved < 0 ? Refusal::belowMidi : Refusal::aboveMidi;
                        r.trackName = track->name;
                        r.message = "A note on '" + track->name + "' would go " + (moved < 0 ? "below the lowest" : "above the highest") + " MIDI note.";
                        return r;
                    }
                }

    return r;
}

Result moveSong (Engine& engine, int semitones)
{
    auto r = check (engine, semitones);

    if (r.refusal != Refusal::none)
        return r;

    const auto before = engine.getKey();
    const auto name = getStepName (before, semitones);

    engine.commitHistory();   // anything pending is its own step

    ChordMap chords;

    for (const auto& [bar, chord] : engine.getChords())
    {
        auto moved = chord;
        moved.root = (((chord.root + semitones) % 12) + 12) % 12;
        chords[bar] = moved;
    }

    {
        const juce::ScopedLock sl (engine.getLock());

        for (auto& t : engine.getTracks())
            if (t->isMidi() && ! firstsong::isDrumTrack (*t))
                for (auto& c : t->clips)
                    if (c->kind == Clip::Kind::midi)
                        for (auto& n : static_cast<MidiClip&> (*c).notes)
                            n.pitch += semitones;
    }

    engine.setKey (getMovedKey (before, semitones));
    engine.setChords (std::move (chords));
    engine.notesChanged();
    engine.nameNextStep (name);
    engine.commitHistory();

    r.moved = true;
    r.stepName = name;
    r.undoSteps = engine.getNumUndoSteps();
    return r;
}

//==============================================================================
BackingCheck checkBacking (const Engine& engine, const Track* melodyTrack, const std::vector<const Clip*>& ignored)
{
    BackingCheck out;

    if (melodyTrack == nullptr)
        return out;

    double start = 0.0, end = 0.0;
    bool any = false;

    for (auto& c : melodyTrack->clips)
        if (c->kind == Clip::Kind::midi)
            for (auto& n : static_cast<const MidiClip&> (*c).notes)
            {
                const double s = c->startBeat + n.start, e = c->startBeat + n.getEnd();
                start = any ? juce::jmin (start, s) : s;
                end = any ? juce::jmax (end, e) : e;
                any = true;
            }

    if (! any)
        return out;

    const auto& tempo = engine.getTempoMap();

    for (auto& t : engine.getTracks())
    {
        if (! t->isAudio() || routing::getSilence (*t, engine.getTracks()) != routing::Silence::none)
            continue;

        for (auto& c : t->clips)
        {
            if (c->kind != Clip::Kind::audio || c->muted || std::find (ignored.begin(), ignored.end(), c.get()) != ignored.end())
                continue;

            if (c->startBeat < end - 1.0e-9 && start < c->getEndBeat (tempo) - 1.0e-9)
            {
                out.recordingsLeft.push_back (t->name);
                break;
            }
        }
    }

    if (out.recordingsLeft.empty())
        return out;

    bool otherMidi = false;

    for (auto* track : getMovedTracks (engine))
    {
        if (track == melodyTrack || routing::getSilence (*track, engine.getTracks()) != routing::Silence::none)
            continue;

        for (auto& c : track->clips)
            if (c->kind == Clip::Kind::midi && ! c->muted && ! static_cast<const MidiClip&> (*c).notes.empty())
                otherMidi = true;
    }

    out.blocked = ! otherMidi;
    return out;
}
} // namespace daw::gen::songmove
