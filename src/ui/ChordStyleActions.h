#pragma once

#include "engine/Engine.h"

namespace daw
{
/** "Play chords as..." on the chord track: asks for an instrument, a style and the bars, then
    plays the chord track's chords in that style on a new MIDI track. */
class ChordStylePlayer
{
public:
    ChordStylePlayer (Engine&, juce::Component& parent);

    /** Opens the dialog. `clickedBar` offers "from this bar on"; the new track goes at
        `insertIndex` (-1: at the end). */
    void show (int clickedBar, int insertIndex);

    /** Called with the new track and clip. */
    std::function<void (Track&, MidiClip&)> onPlayed;

private:
    class Dialog;

    struct BarRange
    {
        juce::String label;
        int firstBar = 0, endBar = 0;
    };

    std::vector<BarRange> getBarRanges (int clickedBar) const;
    void play (const juce::String& styleId, const BarRange&, int insertIndex);

    Engine& engine;
    juce::Component& parent;
    juce::String lastStyleId { "piano-ballad" };   // the dialog opens on the last choice

    JUCE_DECLARE_NON_COPYABLE (ChordStylePlayer)
};
} // namespace daw
