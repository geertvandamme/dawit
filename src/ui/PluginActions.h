#pragma once

#include "engine/Engine.h"
#include "engine/OwnPluginCopies.h"
#include "PluginWindows.h"

namespace daw::plugins
{
/** True if the slot is still part of the track's chain (it may have been removed meanwhile). */
bool contains (const Track&, const PluginSlot*);

/** Short format name for menus: "VST3" or "AU". */
juce::String getFormatLabel (const juce::PluginDescription&);

/** Adds a submenu per vendor listing the known instruments or effects (not DawIt's own VST3 and AU copies,
    see `isDawItPluginCopy`).
    Item ids are firstItemId + the description's index in `choices`. */
juce::PopupMenu createAddMenu (const juce::KnownPluginList&, bool instruments, int firstItemId,
                               juce::Array<juce::PluginDescription>& choices);

void open (PluginWindowManager&, Track&, PluginSlot&);

/** Opens the window of the track's built-in synth, where its sound is picked. */
void openBuiltInSynth (Engine&, PluginWindowManager&, Track&);

/** Switches a MIDI track back to the DawIt Synth: removes its instrument plugins (the synth plays when
    there are none), keeps its effects, and opens the synth's window to pick a sound. */
void useBuiltInSynth (Engine&, PluginWindowManager&, Track&);

/** Puts DawIt Reverb or DawIt Delay at its first "Return:" starting point (100% mix), for use on a return track.
    Other plugins are left alone. */
void applyReturnStart (PluginSlot&);

/** Instruments replace the track's current instrument (or the built-in synth) at the front of the
    chain; effects are appended at the end. Opens the new plugin's editor. */
void add (Engine&, PluginWindowManager&, Track&, const juce::PluginDescription&);

void remove (Engine&, PluginWindowManager&, Track&, PluginSlot&);

void showAddMenu (Engine&, PluginWindowManager&, Track&, bool instruments, juce::Component* target);

/** Open / bypass / move / remove menu for one plugin. */
void showSlotMenu (Engine&, PluginWindowManager&, Track&, PluginSlot&, juce::Component* target);
} // namespace daw::plugins
