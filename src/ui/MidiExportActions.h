#pragma once

#include "engine/Engine.h"

namespace daw::midiactions
{
/** Asks where to save, then writes the clip as a Standard MIDI File. */
void exportClip (Engine&, const Track&, const MidiClip&);

/** Asks where to save, then writes the track's MIDI clips that aren't muted, at their song positions. */
void exportTrack (Engine&, const Track&);

/** Writes the clip to a temporary .mid file and drags that file to another app, such as Logic Pro
    or the Finder. Call it from a mouseDrag callback. */
void dragClipOut (Engine&, const Track&, const MidiClip&, juce::Component& source);
} // namespace daw::midiactions
