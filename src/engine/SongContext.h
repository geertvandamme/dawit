#pragma once

namespace daw
{
/** Implemented by a built-in plugin that wants to know the song while it plays: the notes of the key,
    and of the chord in the bar being played. The track passes them before each block, on the audio thread. */
class SongContextReceiver
{
public:
    virtual ~SongContextReceiver() = default;

    /** Pitch classes as bits (bit 0 = C). `chordPitchClasses` is 0 in a bar with no chord. */
    virtual void setSongContext (int keyPitchClasses, int chordPitchClasses) noexcept = 0;
};

/** Marks a plugin that holds the sound back by its reported latency even while bypassed, so that
    bypassing it doesn't move the track in time: the track's latency counts it either way. */
class KeepsLatencyWhenBypassed
{
public:
    virtual ~KeepsLatencyWhenBypassed() = default;
};
}
