#pragma once

#include "Engine.h"
#include "analysis/HitFinder.h"

#include <vector>

namespace daw::hitstomidi
{
/**
    Turning the hits found in an audio clip (see analysis/HitFinder.h) into a part for DawIt Drums:
    the notes, the new MIDI track, and the sound of "Listen" in the Hits to MIDI window.
*/

/** Where the hits go in time. */
struct Timing
{
    bool snap = false;          // false: keep feel, every hit stays where it was played
    double gridBeats = 0.25;    // 1/16
    double strength = 1.0;      // 0 to 1: how far each hit moves towards the nearest grid line
};

/** Length of every note, in beats (a sixteenth): short enough for a drum, which ends itself. */
constexpr double noteLengthBeats = 0.25;

/** Clip notes (beats from the clip's start) for the hits, which are in seconds from the clip's
    start, through the song's tempo (and its changes). With snap, a hit moves to the song's grid
    (absolute beats, not the clip's start) by `strength` of the way to the nearest line. A note is
    a sixteenth long, shortened to end before the next note of the same pitch; notes at or after
    the clip's end are dropped. Velocity is the hit's velocity over 127. */
std::vector<MidiNote> toClipNotes (const std::vector<analysis::hits::Hit>&, const TempoMap&, double clipStartBeat,
                                   double clipLengthBeats, const Timing& = {});

/** Puts the notes in a new MIDI track "<source name> hits" right below the source track, played
    by DawIt Drums as a plugin, with one clip at the audio clip's place and of its length. With
    `muteOriginal` the audio clip is muted (replace); otherwise it keeps playing (layer). The
    source track's instrument and effects are never touched. Returns null, and makes nothing,
    when there are no notes or the plugin can't be made. Message thread. */
MidiClip* addDrumTrack (Engine&, Track& source, AudioClip&, std::vector<MidiNote> notes, bool muteOriginal);

/** What Listen plays. */
enum class Hear { original, both, hits };

/** The clip (at its gain) and/or the notes played by DawIt Drums' starting kit, as stereo audio
    at `sampleRate`, as long as the clip. The notes are in beats from the clip's start at
    `clipStartBeat`. For the audition player. */
juce::AudioBuffer<float> renderListen (const AudioClip&, const std::vector<MidiNote>&, const TempoMap&, double clipStartBeat,
                                       double sampleRate, Hear);

/** One hit of the guide's example loop. */
struct ExampleHit
{
    double beat;               // from the start, at 120 BPM
    analysis::hits::Drum drum;
    double levelDb;            // against the kick's 0 dB
    double lateMs;             // how late it was played
};

/** The example loop of the guide, the tests and the screenshots: 2 bars of 4/4 at 120 BPM,
    from DawIt Drums' starting kit: kick on 1 and 3 of each bar and before the end, snare on 2 and
    4, a ghost snare, eight hi-hats and a kick bounce 12 ms after beat 4 at -12 dB. */
const std::vector<ExampleHit>& getExampleLoop();

/** The example loop as mono audio, 4 s long. Every hit is scaled to its stated level. */
juce::AudioBuffer<float> renderExampleLoop (double sampleRate);
} // namespace daw::hitstomidi
