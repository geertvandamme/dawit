#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Theme.h"

namespace daw
{
/**
    The pointer: a click-through overlay on the main window that shows the user where the next control is. It draws a
    2 px accent outline around a target component and a caption bubble beside it (13 pt, at most 300 px wide), on the
    side with room. Only the bubble takes clicks: its optional action button and its x. One pointer shows at a time.

    It goes away on the next mouse-down outside the bubble, on Esc (the owner calls `dismiss()` from its key handler),
    after 20 seconds, when the target goes away, or when something else is pointed at. The target is found by a function
    that is asked until it answers, so a control that is still being built (a new track's R button) can be pointed at.
    Message thread only.
*/
class GuidePointer final : public juce::Component, private juce::Timer
{
public:
    GuidePointer();
    ~GuidePointer() override;

    /** What a pointer says and, optionally, one button under the text. */
    struct Options
    {
        juce::String caption;
        juce::String buttonText;            // empty: no button
        std::function<void()> onButton;     // runs when the button is clicked; the pointer then goes away
    };

    /** Points at the component `findTarget` returns. It is asked every 100 ms until it gives one that is showing (for at
        most a second; then the pointer gives up), and the pointer goes away when that component goes away. */
    void show (std::function<juce::Component*()> findTarget, Options);

    /** As `show`, but outlines only the part of the target `findArea` returns, in the target's own coordinates (asked on
        every follow, so a clip or a bar that moves is followed; an empty area outlines the whole target). */
    void show (std::function<juce::Component*()> findTarget, std::function<juce::Rectangle<int>()> findArea, Options);

    /** Removes the pointer at once. */
    void dismiss();

    /** True while a pointer is up (also while it waits for its target). */
    bool isActive() const noexcept { return active; }

    /** The caption of the pointer that is up (for the screenshots and tests); empty when none. */
    juce::String getCaption() const { return active ? options.caption : juce::String(); }

    /** The component pointed at, or null. */
    juce::Component* getTarget() const noexcept { return target.getComponent(); }

    /** Where the bubble is, in this overlay's coordinates. */
    juce::Rectangle<int> getBubbleBounds() const noexcept { return bubble; }

    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    static constexpr int maxBubbleWidth = 300, timeoutMs = 20000;

private:
    void timerCallback() override;
    void layoutBubble();

    bool active = false;
    std::function<juce::Component*()> findTarget;
    std::function<juce::Rectangle<int>()> findArea;   // optional: the part of the target to outline
    juce::Component::SafePointer<juce::Component> target;
    Options options;
    int waitedMs = 0, shownMs = 0;
    juce::Rectangle<int> outline, bubble, textArea;
    juce::TextButton actionButton, closeButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) };
    juce::AttributedString text;
};
} // namespace daw
