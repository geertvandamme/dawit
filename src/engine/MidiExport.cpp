#include "MidiExport.h"

namespace daw::midiexport
{
namespace
{
    int toTicks (double beats) { return juce::roundToInt (beats * ticksPerBeat); }

    juce::MidiMessage tempoEvent (double bpm, double beat)
    {
        return juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60'000'000.0 / bpm)).withTimeStamp (toTicks (beat));
    }

    /** Track 0 of a type 1 file: time signature and key, which apply to the whole song, and the
        tempo where the file starts (`startBeat` of the song) with an event at each change after it. */
    juce::MidiMessageSequence createConductorTrack (const SongInfo& song, double startBeat)
    {
        const auto [sharpsOrFlats, minor] = getKeySignature (song.key);

        juce::MidiMessageSequence conductor;
        conductor.addEvent (tempoEvent (song.tempo.getBpmAt (startBeat), 0.0));

        for (auto& change : song.tempo.getChanges())
            if (change.beat > startBeat + 1.0e-9)
                conductor.addEvent (tempoEvent (change.bpm, change.beat - startBeat));

        conductor.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4));
        conductor.addEvent (juce::MidiMessage::keySignatureMetaEvent (sharpsOrFlats, minor));
        conductor.addEvent (juce::MidiMessage::endOfTrack().withTimeStamp (conductor.getEndTime()));
        return conductor;
    }

    /** Adds a clip's notes, `offset` beats from the start, trimmed to the clip like playback does. */
    void addClipNotes (juce::MidiMessageSequence& sequence, const MidiClip& clip, double offset)
    {
        // Note-offs go in first, so a pitch that is re-triggered at the same spot is released before it restarts
        // (events at the same time keep the order they were added in).
        for (int pass = 0; pass < 2; ++pass)
        {
            for (auto& note : clip.notes)
            {
                if (note.start >= clip.lengthBeats || note.pitch < 0 || note.pitch > 127)
                    continue;

                const double on = offset + note.start;
                const double off = offset + juce::jmin (note.getEnd(), clip.lengthBeats);

                if (pass == 0)
                    sequence.addEvent (juce::MidiMessage::noteOff (1, note.pitch).withTimeStamp (toTicks (off)));
                else
                    sequence.addEvent (juce::MidiMessage::noteOn (1, note.pitch, note.velocity).withTimeStamp (toTicks (on)));
            }
        }
    }

    /** A note track starts with its name, which DAWs use to name the imported region or track. */
    juce::MidiMessageSequence startTrack (const juce::String& name)
    {
        juce::MidiMessageSequence sequence;
        sequence.addEvent (juce::MidiMessage::textMetaEvent (3, name));
        return sequence;
    }

    juce::MidiFile createFile (const SongInfo& song, juce::MidiMessageSequence notes, double endBeat, double startBeat)
    {
        notes.addEvent (juce::MidiMessage::endOfTrack().withTimeStamp (toTicks (endBeat)));
        notes.updateMatchedPairs();

        juce::MidiFile file;
        file.setTicksPerQuarterNote (ticksPerBeat);
        file.addTrack (createConductorTrack (song, startBeat));
        file.addTrack (notes);
        return file;
    }
}

std::pair<int, bool> getKeySignature (const MusicalKey& key)
{
    // Sharps (+) or flats (-) of each major key, by tonic pitch class (C, Db, D, Eb, E, F, F#, G, Ab, A, Bb, B).
    static const int majorKeys[] = { 0, -5, 2, -3, 4, -1, 6, 1, -4, 3, -2, 5 };

    // A mode uses the signature of the major scale it is a rotation of; the minor scales use the relative major's.
    int majorTonicOffset = 0;
    bool minor = false;

    switch (key.scale)
    {
        case MusicalKey::Scale::major:         majorTonicOffset = 0; break;
        case MusicalKey::Scale::dorian:        majorTonicOffset = -2; break;
        case MusicalKey::Scale::phrygian:      majorTonicOffset = -4; break;
        case MusicalKey::Scale::lydian:        majorTonicOffset = -5; break;
        case MusicalKey::Scale::mixolydian:    majorTonicOffset = -7; break;
        case MusicalKey::Scale::locrian:       majorTonicOffset = -11; break;
        case MusicalKey::Scale::minor:
        case MusicalKey::Scale::harmonicMinor:
        case MusicalKey::Scale::melodicMinor:  majorTonicOffset = 3; minor = true; break;
    }

    return { majorKeys[((key.tonic + majorTonicOffset) % 12 + 12) % 12], minor };
}

juce::MidiFile createClipFile (const MidiClip& clip, const juce::String& trackName, const SongInfo& song)
{
    auto notes = startTrack (clip.name.isNotEmpty() ? clip.name : trackName);
    addClipNotes (notes, clip, 0.0);
    return createFile (song, std::move (notes), clip.lengthBeats, clip.startBeat);
}

juce::MidiFile createTrackFile (const Track& track, const SongInfo& song)
{
    auto notes = startTrack (track.name);
    double end = 0.0;

    for (auto& c : track.clips)
    {
        if (c->kind != Clip::Kind::midi || c->muted)
            continue;   // a muted clip isn't heard, so it isn't exported with the track

        auto& clip = static_cast<const MidiClip&> (*c);
        addClipNotes (notes, clip, clip.startBeat);
        end = juce::jmax (end, clip.startBeat + clip.lengthBeats);
    }

    return createFile (song, std::move (notes), end, 0.0);
}

bool write (const juce::MidiFile& midi, const juce::File& target, juce::String& error)
{
    target.getParentDirectory().createDirectory();
    juce::TemporaryFile temp (target);

    if (auto out = temp.getFile().createOutputStream())
    {
        const bool written = midi.writeTo (*out, 1);
        out.reset();

        if (written && temp.overwriteTargetFileWithTemporary())
            return true;
    }

    error = "Couldn't write " + target.getFullPathName();
    return false;
}
} // namespace daw::midiexport
