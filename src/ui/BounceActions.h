#pragma once

#include "engine/Engine.h"
#include "Selection.h"

namespace daw::bounceui
{
/** Bounce in place: renders the track (its unmuted clips) or one clip through its instrument and effects, behind a
    progress window with Cancel, onto a new audio track right below, and mutes the original. The new track is selected,
    and a note says when the sound had to be turned down. Says why when there is nothing to bounce. */
void bounceInPlace (Engine&, Selection&, Track&, Clip* clip = nullptr);

/** Asks where to save, then writes the audio clip (its audio times its clip gain) as a WAV file. */
void exportClipAsWav (Engine&, const Track&, const AudioClip&);

/** Writes the audio clip to a temporary WAV file and drags that file to another app. Call it from a mouseDrag callback. */
void dragClipOut (Engine&, const Track&, const AudioClip&, juce::Component& source);
} // namespace daw::bounceui
