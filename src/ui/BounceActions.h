#pragma once

#include "engine/Engine.h"
#include "Selection.h"

namespace daw::bounceui
{
/** Bounce in place: renders the track (its unmuted clips) or one clip through its instrument and effects, behind a
    progress window with Cancel, onto a new audio track right below, and mutes the original. The new track is selected,
    and a note says when the sound had to be turned down. Says why when there is nothing to bounce. */
void bounceInPlace (Engine&, Selection&, Track&, Clip* clip = nullptr);

/** True when the track has a MIDI effect (whether or not it is switched on): the write-out items are shown for it. */
bool hasMidiEffect (const Track&);

/** "arpeggiator" when DawIt Arpeggiator is the only MIDI effect that is switched on, "MIDI effects" otherwise: the end of
    "Create MIDI clip from ...". */
juce::String getWriteOutName (const Track&);

/** The track's DawIt Arpeggiator, or null. */
PluginSlot* findArpeggiator (Track&);

/** Create MIDI clip from arpeggiator: plays the clip (or the track's unmuted clips) through the track's MIDI effects
    offline, behind a progress window with Cancel, and writes the notes into a clip on a MIDI track right below. The new
    clip is selected, the source muted. Says why in a message box when it can't be done. */
void writeOutArpeggio (Engine&, Selection&, Track&, Clip* clip = nullptr);

/** Undoes the last write-out (see Engine::undoArpeggioWriteOut) and selects the source again. */
void undoWriteOut (Engine&, Selection&);

/** Asks where to save, then writes the audio clip (its audio times its clip gain) as a WAV file. */
void exportClipAsWav (Engine&, const Track&, const AudioClip&);

/** Writes the audio clip to a temporary WAV file and drags that file to another app. Call it from a mouseDrag callback. */
void dragClipOut (Engine&, const Track&, const AudioClip&, juce::Component& source);
} // namespace daw::bounceui
