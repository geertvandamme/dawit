#pragma once

#include "engine/Engine.h"
#include "engine/MidiQuantise.h"

namespace daw::quantiseui
{
/** The grid, strength and "ends too" choice the last Quantise used, from the app's settings (quantise.grid,
    quantise.strength, quantise.ends): 1/16, 100% and starts only until the user chooses otherwise. */
quantise::Settings getLastSettings (Engine&);

/** Remembers what Quantise... was last used with. */
void saveLastSettings (Engine&, const quantise::Settings&);

/** Quantises the notes in `selection` (all of them when it is empty) with the last settings, at once and without a
    dialog: the piano roll's Q key. One step in the undo history. Returns false when nothing needed to move. */
bool applyLast (Engine&, MidiClip&, const juce::SortedSet<int>& selection);

/** The *Quantise notes* window: grid, strength and whether the ends move too, with a picture of where each note goes
    (outlines now, filled after), a summary line, Cancel and Quantise. Quantise applies the settings to `selection` (all
    notes when empty) as one undo step and remembers them. The guide icon calls `onShowGuide`. Used by the piano
    roll's Quantise... button and a MIDI clip's right-click menu. */
void showDialog (Engine&, MidiClip&, const juce::SortedSet<int>& selection, std::function<void()> onShowGuide);

/** The tooltip of the piano roll's Quantise... button. */
juce::String getButtonTooltip();
} // namespace daw::quantiseui
