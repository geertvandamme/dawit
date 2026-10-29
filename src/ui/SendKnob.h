#pragma once

#include "engine/Engine.h"
#include "TrackActions.h"
#include "Theme.h"

namespace daw
{
/** The knob of one send, shared by the mixer's channel strips and the inspector: how much of a track goes to a
    return, from Off to +6 dB with the middle at -12 dB. Double-click turns it off, a right-click opens the send's
    menu (pre- or post-fader, Off, the return's chain, the guide).

    It draws the dial and, beside it, the return's name in the return's colour and PRE when the send is taken before
    the fader. The compact look (the mixer's 22 px row) has the name only, and only the dial takes the mouse; the wide
    look (the inspector) adds the send's level in dB at the right and takes the mouse on the whole row, so the text
    drags the dial too. Before any write it checks that the track and the return are still in the edit. */
class SendKnob final : public juce::Slider
{
public:
    enum class Look { compact, wide };

    SendKnob (Engine&, TrackActions&, Track& sender, Track& returnTrack, Look);

    Track& getReturn() const noexcept { return *returnTrack; }
    Track& getSender() const noexcept { return *sender; }

    /** The send, or null when the track has none to the return (or one of them is gone). */
    Send* getSend() const;

    /** Follows the send (changed elsewhere, in the device bar or a menu) and the return's name and colour. */
    void refresh();

    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent&) override;

    /** "Send to Reverb: how much of this track goes to the Reverb return, now -12.0 dB, ...". */
    juce::String getTooltip() override;

    static constexpr int dialSize = 20;

private:
    bool isAlive() const;

    Engine& engine;
    TrackActions& actions;
    Track* const sender;
    Track* const returnTrack;
    const Look look;
    juce::String shownText;   // the name and level last drawn, to repaint only changes
};
} // namespace daw
