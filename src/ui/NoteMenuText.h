#pragma once

#include <juce_core/juce_core.h>

namespace daw::notemenu
{
/** The wording of the piano roll's note menu: what Delete and Quantise act on, so the menu says its scope. */
struct Text
{
    juce::String deleteItem;
    bool deleteEnabled = false;
    juce::String quantiseItem;
    juce::String selectAllItem;
    juce::String guideItem;
};

/** The menu's texts for this many selected notes: "Delete note", "Delete 3 notes", "Quantise Selected Notes..." or,
    with nothing selected, "Quantise Clip...". */
inline Text getText (int selectedCount)
{
    Text t;
    t.deleteEnabled = selectedCount > 0;
    t.deleteItem = selectedCount > 1 ? "Delete " + juce::String (selectedCount) + " notes" : juce::String ("Delete note");
    t.quantiseItem = selectedCount > 0 ? "Quantise Selected Notes..." : "Quantise Clip...";
    t.selectAllItem = "Select All Notes";
    t.guideItem = "What is MIDI?...";
    return t;
}
}
