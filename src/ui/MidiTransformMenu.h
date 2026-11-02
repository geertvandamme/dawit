#pragma once

#include "engine/Engine.h"
#include "engine/MidiTransforms.h"

namespace daw
{
/**
    The Transform menu of the piano roll's toolbar and of a MIDI clip's right-click menu: reverse in time,
    reverse in pitch (in the key or exactly), randomise the pitches in the song's key, randomise again, the
    randomise options, undo of the latest transform and a link to the guide. One builder for both places, so
    they always agree.
*/
namespace transformmenu
{
    /** The topic number in *What is MIDI?* that explains turning a melody backwards (the first of the three). */
    inline constexpr int guideTopicRetrograde = 10, guideTopicInversion = 11, guideTopicNewNotes = 12;

    /** Adds the items to `menu`. `selection` holds the notes to change (empty: the whole clip); `drumSound` is true
        when the clip's track plays a drum sound, whose notes pick drums, not pitches. Items that would change
        nothing are greyed out. */
    void build (juce::PopupMenu& menu, Engine&, const MidiClip&, const juce::SortedSet<int>& selection, bool drumSound);

    /** Does what the chosen item says and returns true, or false when `result` isn't one of this menu's items.
        `onShowGuide` receives the topic number to open in the MIDI guide. */
    bool handle (int result, Engine&, MidiClip&, const juce::SortedSet<int>& selection,
                 const std::function<void (int topic)>& onShowGuide);

    /** The menu as a submenu named "Transform", for the clip's right-click menu. */
    juce::PopupMenu makeSubMenu (Engine&, const MidiClip&, const juce::SortedSet<int>& selection, bool drumSound);

    /** Shows the whole menu at `target` and handles the choice (nothing happens when the clip has gone by then). */
    void showFor (juce::Component& target, Engine&, MidiClip&, const juce::SortedSet<int>& selection, bool drumSound,
                  std::function<void (int topic)> onShowGuide);

    /** The tooltip of the piano roll's Transform button. */
    juce::String getButtonTooltip();
}
} // namespace daw
