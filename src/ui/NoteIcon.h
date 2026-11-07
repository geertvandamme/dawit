#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/Track.h"

namespace daw
{
/**
    The small note icon (a page with three lines) that marks a track with notes: in its header, its mixer strip and
    the inspector. It is shown only while the track has notes; the tooltip gives the first line, and a click calls
    `onClick` (open the Notebook at the Track tab). One painter for all three places. Message thread only.
*/
class NoteIcon final : public juce::Component, public juce::SettableTooltipClient
{
public:
    static constexpr int size = 12;

    NoteIcon();

    /** Clicked: open the track's notes. */
    std::function<void()> onClick;

    /** Follows the track's notes: visible only when it has some, with the first line as the tooltip. Returns true when
        the visibility changed, so the parent can lay out again. Null track: hidden. */
    bool update (const Track*);

    /** Draws the icon in `area` (12 x 12), in the dim colour or the bright one. */
    static void draw (juce::Graphics&, juce::Rectangle<float> area, bool bright);

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override;
};
}
