#include "GenerateStaging.h"

namespace daw::gen::staging
{
ChordProposal splitChordProposal (const ChordMap& existing, int firstBar, const std::vector<Chord>& proposed)
{
    ChordProposal result;

    for (size_t i = 0; i < proposed.size(); ++i)
    {
        const int bar = firstBar + (int) i;

        if (existing.count (bar) != 0)
            result.kept.push_back (bar);
        else
            result.added.emplace_back (bar, proposed[i]);
    }

    return result;
}

std::vector<MidiNote> fitNotes (const std::vector<MidiNote>& clipNotes, const std::vector<MidiNote>& result,
                                double clipLengthBeats, bool replace)
{
    std::vector<MidiNote> fitted;

    if (! replace)
        fitted = clipNotes;

    for (auto note : result)
    {
        if (note.start >= clipLengthBeats - 1.0e-9)
            continue;

        note.length = juce::jmin (note.length, clipLengthBeats - note.start);
        fitted.push_back (note);
    }

    return fitted;
}

Target Target::capture (const Engine& engine, const MidiClip& clip)
{
    Target t;
    t.clip = &clip;
    t.editGeneration = engine.getEditGeneration();
    t.startBeat = clip.startBeat;
    t.lengthBeats = clip.lengthBeats;
    t.notes = clip.notes;
    t.key = engine.getKey();
    t.timeSignature = engine.getTimeSignature();
    t.firstBar = timesig::getBarAt (t.timeSignature, clip.startBeat + 1.0e-6);
    const int lastBar = timesig::getBarAt (t.timeSignature, juce::jmax (clip.startBeat, clip.startBeat + clip.lengthBeats - 1.0e-6));
    t.numBars = juce::jmax (1, lastBar - t.firstBar + 1);

    for (int bar = t.firstBar; bar < t.firstBar + t.numBars; ++bar)
        if (const auto* chord = engine.getChord (bar))
            t.chordsInBars[bar] = *chord;

    return t;
}

bool Target::isValid (const Engine& engine) const
{
    if (clip == nullptr || engine.findTrackFor (clip) == nullptr || engine.getEditGeneration() != editGeneration
        || ! juce::exactlyEqual (clip->startBeat, startBeat) || ! juce::exactlyEqual (clip->lengthBeats, lengthBeats)
        || ! (engine.getKey() == key) || engine.getTimeSignature() != timeSignature || clip->notes.size() != notes.size())
        return false;

    for (size_t i = 0; i < notes.size(); ++i)
    {
        const auto& a = clip->notes[i];
        const auto& b = notes[i];

        if (a.pitch != b.pitch || ! juce::exactlyEqual (a.start, b.start) || ! juce::exactlyEqual (a.length, b.length)
            || ! juce::exactlyEqual (a.velocity, b.velocity))
            return false;
    }

    for (int bar = firstBar; bar < firstBar + numBars; ++bar)
    {
        const auto* now = engine.getChord (bar);
        const auto before = chordsInBars.find (bar);

        if ((now != nullptr) != (before != chordsInBars.end()) || (now != nullptr && ! (*now == before->second)))
            return false;
    }

    return true;
}

int Staging::begin (const Engine& engine, const MidiClip& clip)
{
    discard();
    target = Target::capture (engine, clip);
    cancelled = false;
    return ++requestId;
}

void Staging::cancel()
{
    cancelled = true;
    discard();
}

Staging::Outcome Staging::deliver (const Engine& engine, int id, const std::vector<MidiNote>& notes,
                                   const std::vector<Chord>& chords, bool replace)
{
    if (id != requestId)
        return Outcome::oldRequest;

    if (cancelled)
        return Outcome::cancelled;

    if (! target.isValid (engine))
        return Outcome::stale;

    rawNotes = notes;
    take.chords = chords;
    replacing = replace;
    take.notes = fitNotes ({}, rawNotes, target.lengthBeats, true);
    staged = true;
    return Outcome::staged;
}

void Staging::setReplace (bool replace)
{
    if (staged)
        replacing = replace;
}

std::vector<MidiNote> Staging::getNotesAfterApply() const
{
    return fitNotes (target.notes, rawNotes, target.lengthBeats, replacing);
}

bool Staging::isStale (const Engine& engine) const
{
    return staged && ! target.isValid (engine);
}

void Staging::discard()
{
    staged = false;
    take = {};
    rawNotes.clear();
}
} // namespace daw::gen::staging
