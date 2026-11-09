#include "GuideFooter.h"
#include "GuideButton.h"

namespace daw
{
namespace guides = gen::guides;

namespace
{
    constexpr float chipHeight = 22.0f, gap = 6.0f;
    constexpr int readAfterSeconds = 10;

    float getTextWidth (const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (juce::Font (juce::FontOptions (12.5f)), text);
    }

    juce::String tickText()
    {
        return juce::String::fromUTF8 ("\xe2\x9c\x93");
    }
}

GuideFooter::GuideFooter()
{
    setVisible (false);
    setWantsKeyboardFocus (false);
}

GuideFooter::~GuideFooter()
{
    if (navigator != nullptr)
        navigator->removeChangeListener (this);
}

bool GuideFooter::isAvailable() noexcept
{
    return GuideNavigator::getInstance() != nullptr;
}

void GuideFooter::setPage (guides::Guide g, const juce::String& t)
{
    auto* instance = GuideNavigator::getInstance();

    if (instance != navigator)
    {
        if (navigator != nullptr)
            navigator->removeChangeListener (this);

        navigator = instance;

        if (navigator != nullptr)
            navigator->addChangeListener (this);
    }

    if (navigator == nullptr || navigator->findEntry (g) == nullptr)
    {
        clearPage();
        return;
    }

    guide = g;
    topic = t;
    active = true;
    secondsOnPage = 0;
    navigator->setShown (g, t);
    links = navigator->getLinks (g, t);
    setVisible (true);
    rebuild();
    repaint();
    startTimer (1000);
}

bool GuideFooter::update (guides::Guide g, const juce::String& t)
{
    const bool was = active;

    if (isAvailable())
        setPage (g, t);
    else
        clearPage();

    return was != active;
}

void GuideFooter::takeSpace (juce::Rectangle<int>& area)
{
    if (! active)
        return;

    setBounds (area.removeFromBottom (height));
    area.removeFromBottom (6);
}

void GuideFooter::clearPage()
{
    active = false;
    stopTimer();
    setVisible (false);
}

void GuideFooter::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (! active)
        return;

    rebuild();
    repaint();

    if (onReadChanged != nullptr)
        onReadChanged();
}

void GuideFooter::timerCallback()
{
    if (! active || navigator == nullptr || ! guides::isCountedPage (topic) || ! isShowing())
        return;

    // Only the front window counts: reading is what the person has in front of them.
    auto* top = dynamic_cast<juce::TopLevelWindow*> (getTopLevelComponent());

    if (top != nullptr && ! top->isActiveWindow())
        return;

    if (++secondsOnPage >= readAfterSeconds && ! navigator->isRead (guide, topic))
        navigator->setRead (guide, topic, true);
}

void GuideFooter::rebuild()
{
    parts.clear();

    if (! active || navigator == nullptr)
        return;

    const float w = (float) getWidth(), y = (getHeight() - chipHeight) / 2.0f;
    const float usable = w - 2.0f * 12.0f;
    float right = w - 12.0f;

    // The right-hand group, from the right: Next page, Read, Back.
    if (const auto next = navigator->getNext (guide, topic))
    {
        const float width = juce::jmin (getTextWidth (next->label) + 30.0f, usable * 0.26f);
        right -= width;
        parts.push_back ({ Kind::next, 0, { right, y, width, chipHeight }, next->label + "  " + juce::String::fromUTF8 ("\xe2\x80\xba"),
                           next->otherGuide ? "Goes on to the next guide on Start here (Cmd+]). This page counts as read."
                                            : "Goes on to the next page of this guide (Cmd+]). This page counts as read." });
        right -= gap;
    }

    if (guides::isCountedPage (topic))
    {
        const bool read = navigator->isRead (guide, topic);
        const float width = 66.0f;
        right -= width;
        parts.push_back ({ Kind::read, 0, { right, y, width, chipHeight }, read ? tickText() + " Read" : "Read",
                           "Ticked when you've read this page: after 10 seconds on it, or when you go on with Next page. Click to tick or untick it." });
        right -= gap;
    }

    float backWidth = 0.0f;

    if (const auto back = navigator->getBack (guide))
    {
        backWidth = juce::jmin (getTextWidth (back->label) + 30.0f, usable * 0.3f);
        right -= backWidth;
        parts.push_back ({ Kind::back, 0, { right, y, backWidth, chipHeight }, juce::String::fromUTF8 ("\xe2\x80\xb9 ") + back->label,
                           "Goes back to where you came from (Cmd+[)" });
        right -= gap;
    }

    // See also chips take the room on the left: at most three, and the rest under +N.
    float x = 12.0f;
    const float limit = right - gap;

    if (! links.empty())
    {
        const float labelWidth = getTextWidth ("See also:") + 4.0f;
        parts.push_back ({ Kind::label, 0, { x, y, labelWidth, chipHeight }, "See also:", {} });
        x += labelWidth + gap;
    }

    shownLinks = 0;

    for (int i = 0; i < (int) links.size() && i < 3; ++i)
    {
        const bool more = i + 1 < (int) links.size();
        const float reserve = more ? 44.0f : 0.0f;
        const float wanted = juce::jmin (getTextWidth (links[(size_t) i].text) + 18.0f, 230.0f);
        const float room = limit - x - reserve;

        if (room < 90.0f)
            break;

        const float width = juce::jmin (wanted, room);
        parts.push_back ({ Kind::chip, i, { x, y, width, chipHeight }, links[(size_t) i].text, links[(size_t) i].tooltip });
        x += width + gap;
        ++shownLinks;
    }

    if (shownLinks < (int) links.size())
    {
        const auto count = juce::String (juce::String::fromUTF8 ("+")) + juce::String ((int) links.size() - shownLinks);
        parts.push_back ({ Kind::more, 0, { x, y, 40.0f, chipHeight }, count, "More pages that relate to this one" });
    }
}

void GuideFooter::resized()
{
    rebuild();
}

void GuideFooter::paint (juce::Graphics& g)
{
    if (! active)
        return;

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, 0, getWidth(), 1);
    g.setFont (juce::FontOptions (12.5f));

    for (int i = 0; i < (int) parts.size(); ++i)
    {
        const auto& p = parts[(size_t) i];

        if (p.kind == Kind::label)
        {
            g.setColour (theme::textDim);
            g.drawText (p.text, p.area, juce::Justification::centredLeft);
            continue;
        }

        const bool isHovered = hovered == i;
        const bool readOn = p.kind == Kind::read && navigator != nullptr && navigator->isRead (guide, topic);
        g.setColour (isHovered ? theme::accent.withAlpha (0.45f) : p.kind == Kind::next ? theme::accent.withAlpha (0.22f) : theme::panelLight);
        g.fillRoundedRectangle (p.area, chipHeight / 2.0f);
        g.setColour (p.kind == Kind::read && ! readOn ? theme::textDim : theme::text);

        if (readOn)
            g.setColour (theme::meterGreen);

        g.drawFittedText (p.text, p.area.reduced (9.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }
}

int GuideFooter::getPartAt (juce::Point<float> position) const
{
    for (int i = 0; i < (int) parts.size(); ++i)
        if (parts[(size_t) i].kind != Kind::label && parts[(size_t) i].area.contains (position))
            return i;

    return -1;
}

void GuideFooter::mouseMove (const juce::MouseEvent& e)
{
    const int part = getPartAt (e.position);
    setMouseCursor (part >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);

    if (part != hovered)
    {
        hovered = part;
        repaint();
    }
}

void GuideFooter::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

juce::String GuideFooter::getTooltip()
{
    const int part = getPartAt (getMouseXYRelative().toFloat());
    return part >= 0 ? parts[(size_t) part].tooltip : juce::String();
}

void GuideFooter::openLink (int index)
{
    if (navigator != nullptr && juce::isPositiveAndBelow (index, (int) links.size()))
        navigator->follow (guide, topic, links[(size_t) index].to);
}

void GuideFooter::showMoreMenu()
{
    juce::PopupMenu menu;

    for (int i = shownLinks; i < (int) links.size(); ++i)
        menu.addItem (i + 1, links[(size_t) i].text);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<GuideFooter> (this)] (int result)
    {
        if (safe != nullptr && result > 0)
            safe->openLink (result - 1);
    });
}

void GuideFooter::mouseUp (const juce::MouseEvent& e)
{
    const int index = getPartAt (e.position);

    if (index < 0 || navigator == nullptr || e.mouseWasDraggedSinceMouseDown())
        return;

    const auto part = parts[(size_t) index];

    if (e.mods.isPopupMenu())
    {
        if (part.kind != Kind::chip)
            return;

        juce::PopupMenu menu;
        menu.addItem (1, "Open " + part.text);
        menu.addSeparator();
        guideicon::addItem (menu, 2, "Theory Guides...");
        menu.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<GuideFooter> (this), part] (int result)
        {
            if (safe == nullptr || safe->navigator == nullptr)
                return;

            if (result == 1)
                safe->openLink (part.index);
            else if (result == 2 && safe->navigator->onShowGuides != nullptr)
                safe->navigator->onShowGuides();
        });
        return;
    }

    switch (part.kind)
    {
        case Kind::chip:  openLink (part.index); break;
        case Kind::more:  showMoreMenu(); break;
        case Kind::back:  navigator->goBack(); break;
        case Kind::next:  navigator->goNext (guide, topic); break;
        case Kind::read:  navigator->setRead (guide, topic, ! navigator->isRead (guide, topic)); break;
        case Kind::label: break;
    }
}

bool GuideFooter::keyPressed (const juce::KeyPress& key)
{
    if (! active || navigator == nullptr || ! key.getModifiers().isCommandDown())
        return false;

    if (key.isKeyCode (']'))
    {
        navigator->goNext (guide, topic);
        return true;
    }

    if (key.isKeyCode ('['))
    {
        navigator->goBack();
        return true;
    }

    return false;
}

void GuideFooter::paintTick (juce::Graphics& g, int rowWidth, int rowHeight, guides::Guide guide, const juce::String& topic)
{
    auto* navigator = GuideNavigator::getInstance();

    if (navigator == nullptr || ! guides::isCountedPage (topic) || ! navigator->isRead (guide, topic))
        return;

    g.setColour (theme::textDim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (tickText(), rowWidth - 26, 0, 20, rowHeight, juce::Justification::centredRight);
}
} // namespace daw
