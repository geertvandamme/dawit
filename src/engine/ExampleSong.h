#pragma once

#include "Engine.h"

namespace daw::examples
{
/**
    The example song the screenshots and the website show: a pop song in A minor at 96 BPM, 36 bars
    (intro, verse, chorus, verse, chorus) with a chord per bar, sections and timed lyrics. Its tracks
    are grouped the way a real mix is: Kick, Snare and Hats in a Drums bus (with DawIt Compressor on its
    Drum bus starting point), Bass, Keys, and a Vocals bus holding the Lead vocal and a Backing vocals
    bus with BV 1 and BV 2. Every track plays the built-in synth, so the song sounds without any
    plugins or audio files. The lead and backing vocals are MIDI notes on the lyrics' syllables.

    Builds into an empty edit (call Engine::resetEdit() first for one that isn't) and returns the
    Lead vocal track, as a good one to select.
*/
Track* buildPopSong (Engine&);
} // namespace daw::examples
