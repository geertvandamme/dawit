#pragma once

#include "engine/Engine.h"
#include "engine/MidiQuantise.h"

namespace daw::quantiseui
{
/** The grid, strength, swing and "ends too" choice the last Quantise used, from the app's settings (quantise.grid,
    quantise.strength, quantise.swing, quantise.ends): 1/16, 100%, no swing (50%) and starts only until the user chooses otherwise. */
quantise::Settings getLastSettings (Engine&);

/** Remembers what Quantise... was last used with. */
void saveLastSettings (Engine&, const quantise::Settings&);

/** Quantises the notes in `selection` (all of them when it is empty) with the last settings, at once and without a
    dialog: the piano roll's Q key. One step in the undo history. Returns false when nothing needed to move. */
bool applyLast (Engine&, MidiClip&, const juce::SortedSet<int>& selection);

/** The *Quantise notes* window: grid, strength, swing (for the 1/8 and 1/16 grids) and whether the ends move too, with a
    picture of where each note goes (outlines now, filled after, on the swung lines), a summary line, Cancel and Quantise.
    Quantise applies the settings to `selection` (all notes when empty) as one undo step and remembers them. The guide
    icon calls `onShowGuide`. Used by the piano roll's Quantise... button, a MIDI clip's right-click menu and the Build a
    Beat exercise. `start`, when given, is what the window opens with instead of the last settings; it is remembered only
    when Quantise is pressed. */
void showDialog (Engine&, MidiClip&, const juce::SortedSet<int>& selection, std::function<void()> onShowGuide,
                 const quantise::Settings* start = nullptr);

/** The grid (in beats) the open Quantise window is on, or 0 when none is open (for the exercise's tip). */
double getOpenGridBeats();

/** The tooltip of the piano roll's Quantise... button. */
juce::String getButtonTooltip();
} // namespace daw::quantiseui
