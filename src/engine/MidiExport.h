#pragma once

#include "Track.h"
#include "MusicalKey.h"

namespace daw::midiexport
{
/** Ticks per quarter note in exported files. */
constexpr int ticksPerBeat = 960;

/** Tempo, key and time signature written at the start of every exported file, with a tempo event
    at each of the song's tempo changes. */
struct SongInfo
{
    TempoMap tempo;
    MusicalKey key;
};

/** A Standard MIDI File (type 1) holding one clip's notes, starting at 0. Notes past the clip's end
    are dropped or shortened, as in playback, and the file ends where the clip ends. */
juce::MidiFile createClipFile (const MidiClip&, const juce::String& trackName, const SongInfo&);

/** A Standard MIDI File (type 1) holding all MIDI clips of a track that aren't muted, at their song positions. */
juce::MidiFile createTrackFile (const Track&, const SongInfo&);

/** Writes the file, replacing any existing one. */
bool write (const juce::MidiFile&, const juce::File&, juce::String& error);

/** Key signature meta event values: sharps (positive) or flats (negative), and whether it is minor. */
std::pair<int, bool> getKeySignature (const MusicalKey&);
} // namespace daw::midiexport
