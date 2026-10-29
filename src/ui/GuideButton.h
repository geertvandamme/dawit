#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace daw
{
/** The guide icon (an open book with five rising bars, like a spectrum) that marks everything that opens a theory
    guide. It is drawn from a path, so it is sharp at any size and takes whatever colour it is given. */
namespace guideicon
{
    /** The icon as a path, scaled so it is 1 wide (and about 0.83 high), with its top left at 0, 0. */
    juce::Path getPath();

    /** Draws the icon, as large as fits and centred in `area`, in `colour`. */
    void draw (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour);

    /** The icon as a drawable, for a right-click menu item. */
    std::unique_ptr<juce::Drawable> makeDrawable (juce::Colour colour);

    /** Adds a menu item that opens a guide, with the icon in front of its text. */
    void addItem (juce::PopupMenu& menu, int itemID, const juce::String& text, bool isEnabled = true);

    /** The same for an item that runs a function when chosen. */
    void addItem (juce::PopupMenu& menu, const juce::String& text, std::function<void()> action);
}

/** A button that opens a theory guide. With the text "?" (or none) it shows only the icon; with any other text it
    shows the icon first and then the text. The icon is dim at rest and in the accent colour under the mouse. It
    never takes keyboard focus. */
class GuideButton final : public juce::TextButton
{
public:
    /** Makes the button; `text` is its label, or "?" / empty for the icon alone. */
    explicit GuideButton (const juce::String& text = {});

    /** The width that fits the icon (and the label, if any) at this height. */
    int getBestWidthForHeight (int height);

    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    bool hasLabel() const;
};
}
