#include "GuidePointer.h"

namespace daw
{
GuidePointer::GuidePointer()
{
    setInterceptsMouseClicks (true, true);
    setWantsKeyboardFocus (false);
    setAccessible (false);

    for (auto* b : { &actionButton, &closeButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        addChildComponent (b);
    }

    closeButton.setTooltip ("Close this hint (or press Esc, or click anywhere else)");
    closeButton.onClick = [this] { dismiss(); };
    actionButton.onClick = [this]
    {
        auto run = options.onButton;
        dismiss();

        if (run != nullptr)
            run();
    };

    setVisible (false);
    juce::Desktop::getInstance().addGlobalMouseListener (this);
}

GuidePointer::~GuidePointer()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
    stopTimer();
}

void GuidePointer::show (std::function<juce::Component*()> finder, Options newOptions)
{
    findTarget = std::move (finder);
    options = std::move (newOptions);
    target = nullptr;
    active = true;
    waitedMs = shownMs = 0;
    outline = bubble = {};

    text = juce::AttributedString();
    text.setJustification (juce::Justification::topLeft);
    text.append (options.caption, juce::FontOptions (13.0f), theme::text);

    actionButton.setButtonText (options.buttonText);
    actionButton.setVisible (false);
    closeButton.setVisible (false);
    setVisible (true);
    toFront (false);
    startTimer (100);
    timerCallback();
}

void GuidePointer::dismiss()
{
    stopTimer();
    active = false;
    target = nullptr;
    findTarget = nullptr;
    options = {};
    actionButton.setVisible (false);
    closeButton.setVisible (false);
    setVisible (false);
}

void GuidePointer::timerCallback()
{
    if (! active)
        return;

    if (target == nullptr)
    {
        auto* found = findTarget != nullptr ? findTarget() : nullptr;

        if (found == nullptr || ! found->isShowing())
        {
            waitedMs += getTimerInterval();

            if (waitedMs > 1000)
                dismiss();

            return;
        }

        target = found;
        shownMs = 0;
    }
    else
    {
        // The target went away or was hidden: nothing is left to point at.
        if (! target->isShowing())
        {
            dismiss();
            return;
        }

        shownMs += getTimerInterval();

        if (shownMs >= timeoutMs)
        {
            dismiss();
            return;
        }
    }

    const auto box = getLocalArea (target.getComponent(), target->getLocalBounds()).getIntersection (getLocalBounds());

    if (box != outline || bubble.isEmpty())
    {
        outline = box;
        layoutBubble();
        repaint();
    }
}

void GuidePointer::resized()
{
    if (active && target != nullptr)
    {
        layoutBubble();
        repaint();
    }
}

void GuidePointer::layoutBubble()
{
    constexpr int pad = 10, gap = 8, buttonHeight = 24, closeSize = 18;
    const int width = juce::jmin (maxBubbleWidth, juce::jmax (120, getWidth() - 16));

    juce::TextLayout layout;
    layout.createLayout (text, (float) (width - 2 * pad - closeSize));
    const int textHeight = (int) std::ceil (layout.getHeight());
    const bool hasButton = options.buttonText.isNotEmpty();
    const int height = pad + textHeight + (hasButton ? 8 + buttonHeight : 0) + pad;

    const auto bounds = getLocalBounds().reduced (6);
    const auto o = outline;

    // The side with room, never on top of the target: below, above, right, left; else inside its top left corner.
    juce::Rectangle<int> box (width, height);
    const int centredX = juce::jlimit (bounds.getX(), juce::jmax (bounds.getX(), bounds.getRight() - width), o.getX());

    if (o.getBottom() + gap + height <= bounds.getBottom())
        box.setPosition (centredX, o.getBottom() + gap);
    else if (o.getY() - gap - height >= bounds.getY())
        box.setPosition (centredX, o.getY() - gap - height);
    else if (o.getRight() + gap + width <= bounds.getRight())
        box.setPosition (o.getRight() + gap, juce::jlimit (bounds.getY(), juce::jmax (bounds.getY(), bounds.getBottom() - height), o.getY()));
    else if (o.getX() - gap - width >= bounds.getX())
        box.setPosition (o.getX() - gap - width, juce::jlimit (bounds.getY(), juce::jmax (bounds.getY(), bounds.getBottom() - height), o.getY()));
    else
        box.setPosition (o.getX() + 12, o.getY() + 12);

    bubble = box.constrainedWithin (getLocalBounds());
    textArea = juce::Rectangle<int> (bubble.getX() + pad, bubble.getY() + pad, width - 2 * pad - closeSize, textHeight);
    closeButton.setBounds (bubble.getRight() - closeSize - 4, bubble.getY() + 4, closeSize, closeSize);
    closeButton.setVisible (true);
    actionButton.setVisible (hasButton);

    if (hasButton)
    {
        const int buttonWidth = juce::jmin (width - 2 * pad, juce::GlyphArrangement::getStringWidthInt (juce::FontOptions (13.0f), options.buttonText) + 28);
        actionButton.setBounds (bubble.getX() + pad, bubble.getBottom() - pad - buttonHeight, buttonWidth, buttonHeight);
    }
}

bool GuidePointer::hitTest (int x, int y)
{
    return active && bubble.contains (x, y);
}

void GuidePointer::paint (juce::Graphics& g)
{
    if (! active || outline.isEmpty() || bubble.isEmpty())
        return;

    g.setColour (theme::accent);
    g.drawRoundedRectangle (outline.toFloat().expanded (1.0f), 3.0f, 2.0f);

    g.setColour (theme::panelLight);
    g.fillRoundedRectangle (bubble.toFloat(), 6.0f);
    g.setColour (theme::accent);
    g.drawRoundedRectangle (bubble.toFloat().reduced (0.5f), 6.0f, 1.0f);

    text.draw (g, textArea.toFloat());
}

void GuidePointer::mouseDown (const juce::MouseEvent& e)
{
    if (! active)
        return;

    // A click on the bubble (its text, button or x) is its own; anywhere else ends the hint.
    if (e.originalComponent == this || isParentOf (e.originalComponent))
        return;

    dismiss();
}
} // namespace daw
