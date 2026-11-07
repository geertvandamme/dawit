#include "NoteIcon.h"

#include "engine/Notebook.h"
#include "Theme.h"

namespace daw
{
NoteIcon::NoteIcon()
{
    setSize (size, size);
    setVisible (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (false);
}

bool NoteIcon::update (const Track* track)
{
    const bool has = track != nullptr && notebook::hasNotes (track->trackNotes);

    if (has)
        setTooltip ("Notes: " + notebook::getFirstLine (track->trackNotes) + juce::String::fromUTF8 (" \xc2\xb7 Click to open them in the Notebook"));

    const bool changed = has != isVisible();
    setVisible (has);
    return changed;
}

void NoteIcon::draw (juce::Graphics& g, juce::Rectangle<float> area, bool bright)
{
    g.setColour (bright ? theme::text : theme::textDim);
    auto page = area.reduced (1.5f, 0.5f);
    g.drawRoundedRectangle (page, 1.5f, 1.0f);

    for (int i = 0; i < 3; ++i)
    {
        const float y = page.getY() + 3.0f + (float) i * 2.5f;
        g.fillRect (page.getX() + 2.0f, y, page.getWidth() - 4.0f, 1.0f);
    }
}

void NoteIcon::paint (juce::Graphics& g)
{
    draw (g, getLocalBounds().toFloat(), isMouseOver());
}

void NoteIcon::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && contains (e.getPosition()) && onClick != nullptr)
        onClick();
}
}
