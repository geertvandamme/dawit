#include "GuideButton.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include "Theme.h"

namespace daw
{
namespace
{
    // The icon's shapes, in the pixels of the 1250 px design drawing (designs/dawit-spectrum-book-variations/variation-01.png).
    constexpr float left = 133.0f, top = 238.0f, right = 1120.0f;

    void addPage (juce::Path& p, bool mirrored)
    {
        juce::Path page;
        page.startNewSubPath (188, 702);
        page.cubicTo (330, 704, 500, 740, 585, 830);
        page.cubicTo (605, 852, 613, 875, 613, 905);
        page.lineTo (613, 1040);
        page.cubicTo (613, 1055, 600, 1060, 592, 1058);
        page.cubicTo (585, 1056, 580, 1050, 575, 1045);
        page.cubicTo (480, 975, 330, 947, 188, 947);
        page.quadraticTo (133, 947, 133, 897);
        page.lineTo (133, 752);
        page.quadraticTo (133, 702, 188, 702);
        page.closeSubPath();

        if (mirrored)
            page.applyTransform (juce::AffineTransform::scale (-1.0f, 1.0f).translated (1253.0f, 0.0f));

        p.addPath (page);
    }
}

juce::Path guideicon::getPath()
{
    juce::Path p;
    const float bars[5][4] = { { 278, 443, 115, 220 }, { 421, 333, 116, 386 }, { 569, 238, 116, 535 },
                               { 716, 333, 116, 386 }, { 861, 443, 115, 220 } };

    for (auto& b : bars)
        p.addRoundedRectangle (b[0], b[1], b[2], b[3], b[2] * 0.5f);

    addPage (p, false);
    addPage (p, true);

    const auto scale = 1.0f / (right - left);
    p.applyTransform (juce::AffineTransform::translation (-left, -top).scaled (scale));
    return p;
}

void guideicon::draw (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour)
{
    static const auto path = getPath();
    g.setColour (colour);
    g.fillPath (path, path.getTransformToScaleToFit (area, true));
}

std::unique_ptr<juce::Drawable> guideicon::makeDrawable (juce::Colour colour)
{
    auto d = std::make_unique<juce::DrawablePath>();
    d->setPath (getPath());
    d->setFill (colour);
    return d;
}

void guideicon::addItem (juce::PopupMenu& menu, int itemID, const juce::String& text, bool isEnabled)
{
    juce::PopupMenu::Item item (text);
    item.itemID = itemID;
    item.isEnabled = isEnabled;
    item.image = makeDrawable (theme::text);
    menu.addItem (std::move (item));
}

void guideicon::addItem (juce::PopupMenu& menu, const juce::String& text, std::function<void()> action)
{
    juce::PopupMenu::Item item (text);
    item.action = std::move (action);
    item.image = makeDrawable (theme::text);
    menu.addItem (std::move (item));
}

GuideButton::GuideButton (const juce::String& text) : juce::TextButton (text)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

bool GuideButton::hasLabel() const
{
    const auto t = getButtonText();
    return t.isNotEmpty() && t != "?";
}

int GuideButton::getBestWidthForHeight (int height)
{
    const auto iconWidth = (int) ((float) height * 0.62f * 1.2f);

    if (! hasLabel())
        return juce::jmax (height, iconWidth + 12);

    return iconWidth + 4 + juce::TextButton::getBestWidthForHeight (height);
}

void GuideButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto& laf = getLookAndFeel();
    laf.drawButtonBackground (g, *this, findColour (getToggleState() ? buttonOnColourId : buttonColourId), isMouseOver, isButtonDown);

    auto r = getLocalBounds().toFloat();
    const auto iconColour = isMouseOver || isButtonDown ? theme::accent : theme::textDim.brighter (0.2f);
    const auto iconHeight = juce::jmin (r.getHeight() - 8.0f, 18.0f);
    const auto iconWidth = iconHeight * 1.2f;

    if (! hasLabel())
    {
        guideicon::draw (g, r.withSizeKeepingCentre (juce::jmin (iconWidth, r.getWidth() - 6.0f), iconHeight), iconColour);
        return;
    }

    auto font = laf.getTextButtonFont (*this, getHeight());
    const auto textWidth = juce::GlyphArrangement::getStringWidth (font, getButtonText());
    const auto total = iconWidth + 4.0f + textWidth;
    auto row = r.withSizeKeepingCentre (juce::jmin (total, r.getWidth() - 4.0f), r.getHeight());
    guideicon::draw (g, row.removeFromLeft (iconWidth).withSizeKeepingCentre (iconWidth, iconHeight), iconColour);
    row.removeFromLeft (4.0f);

    g.setFont (font);
    g.setColour (findColour (getToggleState() ? textColourOnId : textColourOffId).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (getButtonText(), row.toNearestInt(), juce::Justification::centredLeft, 1);
}
}
