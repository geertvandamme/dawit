#include "GuideSourcesPanel.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include "Theme.h"

namespace daw
{
namespace
{
    constexpr int marginX = 12, marginY = 10, linkHeight = 22, resultHeight = 20, resultIndent = 18;

    juce::String withoutScheme (const juce::String& url)
    {
        return url.startsWith ("https://") ? url.substring (8) : url;
    }

    float getTextWidth (const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (font, text, 0.0f, 0.0f);
        return glyphs.getBoundingBox (0, -1, false).getWidth();
    }
}

//==============================================================================
/** The scrolled content: items placed from the top for the width, drawn and hit-tested by hand. */
class GuideSourcesPanel::Content final : public juce::Component, public juce::TooltipClient
{
public:
    void setSources (const gen::guidesources::Sources& s)
    {
        sources = s;
        hovered = -1;
        layoutFor (getWidth());
        repaint();
    }

    void layoutFor (int width)
    {
        items.clear();
        const int inner = juce::jmax (80, width - 2 * marginX);
        int y = marginY;

        auto addText = [&] (const juce::String& text, float size, bool bold, juce::Colour colour, int x, int gap, Item::Kind kind,
                            const juce::String& tip = {})
        {
            Item item;
            item.kind = kind;
            item.tooltip = tip;
            juce::AttributedString s;
            s.setJustification (juce::Justification::topLeft);
            s.append (text, juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain), colour);
            item.layout.createLayout (s, (float) (inner - (x - marginX)));
            const int h = (int) std::ceil (item.layout.getHeight());
            y += gap;
            item.bounds = { x, y, inner - (x - marginX), h };
            y += h;
            items.push_back (std::move (item));
        };

        auto addLink = [&] (const juce::String& url, const juce::String& note, int x, int h)
        {
            Item item;
            item.kind = Item::link;
            item.url = url;
            item.note = note;
            item.bounds = { x, y, inner - (x - marginX), h };
            item.tooltip = "Opens " + url + " in your browser. Right-click to copy the link.";
            y += h;
            items.push_back (std::move (item));
        };

        juce::StringArray intro;
        auto rest = gen::guidesources::getIntro (! sources.isEmpty());

        for (int split = rest.indexOf ("\n\n"); split >= 0; split = rest.indexOf ("\n\n"))
        {
            intro.add (rest.substring (0, split));
            rest = rest.substring (split + 2);
        }

        intro.add (rest);

        for (int i = 0; i < intro.size(); ++i)
            addText (intro[i], 14.5f, false, theme::text, marginX, i == 0 ? 0 : 8, Item::text);

        for (auto& note : sources.notes)
            addText (note, 14.5f, false, theme::text, marginX, 8, Item::text);

        if (! sources.pages.empty())
        {
            addText ("Pages read", 15.0f, true, theme::text, marginX, 16, Item::heading);
            y += 2;

            for (auto& page : sources.pages)
                addLink (page.url, page.note, marginX, linkHeight);
        }

        if (! sources.searches.empty())
        {
            addText ("Searched for", 15.0f, true, theme::text, marginX, 16, Item::heading);
            y += 2;

            for (auto& search : sources.searches)
            {
                addText (search.text, 13.5f, false, theme::textDim, marginX, 4, Item::search,
                         "Searched for this while writing the guide; the links under it are results that were used.");

                for (auto& url : search.resultUrls)
                    addLink (url, {}, marginX + resultIndent, resultHeight);
            }
        }

        setSize (width, y + marginY);
    }

    void resized() override
    {
        layoutFor (getWidth());
    }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < (int) items.size(); ++i)
        {
            const auto& item = items[(size_t) i];

            if (item.kind == Item::link)
                paintLink (g, item, i == hovered);
            else
                item.layout.draw (g, item.bounds.toFloat());
        }
    }

    juce::String getTooltip() override
    {
        const int i = findItem (getMouseXYRelative());
        return i >= 0 ? items[(size_t) i].tooltip : juce::String();
    }

    juce::MouseCursor getMouseCursor() override
    {
        const int i = findItem (getMouseXYRelative());
        return i >= 0 && items[(size_t) i].kind == Item::link ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor;
    }

    void mouseMove (const juce::MouseEvent& e) override    { setHovered (findLink (e.getPosition())); }
    void mouseExit (const juce::MouseEvent&) override      { setHovered (-1); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || e.mouseWasDraggedSinceMouseDown())
            return;

        if (const int i = findLink (e.getPosition()); i >= 0)
            open (items[(size_t) i].url);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            return;

        const int i = findLink (e.getPosition());

        if (i < 0)
            return;

        const auto url = items[(size_t) i].url;
        juce::PopupMenu menu;
        menu.addItem ("Open in browser", [url] { open (url); });
        menu.addItem ("Copy link", [url] { juce::SystemClipboard::copyTextToClipboard (url); });
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea ({ e.getScreenX(), e.getScreenY(), 1, 1 }));
    }

private:
    struct Item
    {
        enum Kind { text, heading, search, link };

        Kind kind = text;
        juce::Rectangle<int> bounds;
        juce::TextLayout layout;
        juce::String url, note, tooltip;
    };

    static void open (const juce::String& url)
    {
        juce::URL (url).launchInDefaultBrowser();
    }

    int findItem (juce::Point<int> p) const
    {
        for (int i = 0; i < (int) items.size(); ++i)
            if (items[(size_t) i].bounds.contains (p) && ! items[(size_t) i].tooltip.isEmpty())
                return i;

        return -1;
    }

    int findLink (juce::Point<int> p) const
    {
        const int i = findItem (p);
        return i >= 0 && items[(size_t) i].kind == Item::link ? i : -1;
    }

    void setHovered (int i)
    {
        if (i != hovered)
        {
            hovered = i;
            repaint();
        }
    }

    void paintLink (juce::Graphics& g, const Item& item, bool isHovered) const
    {
        auto area = item.bounds;
        const auto text = withoutScheme (item.url);
        const juce::Font font { juce::FontOptions (14.0f) };
        const int maxLink = item.note.isEmpty() ? area.getWidth() : (int) (area.getWidth() * 0.62f);
        const int linkWidth = juce::jmin (maxLink, (int) std::ceil (getTextWidth (font, text)) + 2);
        const auto linkArea = area.removeFromLeft (linkWidth);

        g.setColour (isHovered ? theme::accent.brighter (0.4f) : theme::accent);
        g.setFont (font);
        g.drawFittedText (text, linkArea, juce::Justification::centredLeft, 1, 0.8f);

        if (isHovered)
        {
            g.setColour (theme::accent);
            g.fillRect (linkArea.getX(), linkArea.getBottom() - 3, linkArea.getWidth(), 1);
        }

        if (item.note.isNotEmpty() && area.getWidth() > 30)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (13.0f));
            g.drawText (item.note, area.withTrimmedLeft (10), juce::Justification::centredLeft, true);
        }
    }

    gen::guidesources::Sources sources;
    std::vector<Item> items;
    int hovered = -1;
};

//==============================================================================
GuideSourcesPanel::GuideSourcesPanel (const juce::String& guideKey) : content (std::make_unique<Content>())
{
    viewport.setViewedComponent (content.get(), false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
    setKey (guideKey);
}

GuideSourcesPanel::~GuideSourcesPanel()
{
    viewport.setViewedComponent (nullptr, false);
}

void GuideSourcesPanel::setKey (const juce::String& guideKey)
{
    content->setSources (gen::guidesources::getSources (guideKey));
    viewport.setViewPosition (0, 0);
}

void GuideSourcesPanel::paint (juce::Graphics& g)
{
    g.setColour (theme::panelLight);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
}

void GuideSourcesPanel::resized()
{
    viewport.setBounds (getLocalBounds());
    content->setSize (juce::jmax (100, getWidth() - viewport.getScrollBarThickness()), content->getHeight());
}

//==============================================================================
GuideSourcesTopic::GuideSourcesTopic (juce::Component& parent, const juce::String& guideKey) : panel (guideKey)
{
    const auto sources = gen::guidesources::getSources (guideKey);
    summary = gen::guidesources::getSummary (sources);
    printText = gen::guidesources::getPrintText (sources);
    parent.addChildComponent (panel);
}

juce::String GuideSourcesTopic::getName()
{
    return "Sources";   // the same as gen::guides::sourcesTopicName, which a plugin does not include
}

void GuideSourcesTopic::paintRow (juce::Graphics& g, int w, int h, bool selected) const
{
    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (getName(), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (summary, 10, 23, w - 16, 16, juce::Justification::centredLeft, true);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void GuideSourcesTopic::show (bool shown, juce::Rectangle<int> area)
{
    panel.setVisible (shown);

    if (shown)
        panel.setBounds (area);
}

void GuideSourcesTopic::select (juce::Component& host, juce::ListBox& list, int ownTopics, std::initializer_list<juce::Component*> keep)
{
    hostComponent = &host;

    // Also when already shown: a window may have made something visible again (new tiles, a button).
    for (auto* child : host.getChildren())
        if (child != &panel && child->isVisible() && std::find (keep.begin(), keep.end(), child) == keep.end())
        {
            hidden.push_back (child);
            child->setVisible (false);
        }

    panel.setVisible (true);

    if (list.getSelectedRow() != ownTopics)
        list.selectRow (ownTopics);   // comes back through selectedRowsChanged

    host.resized();
}

void GuideSourcesTopic::close()
{
    if (! isShown())
        return;

    panel.setVisible (false);

    for (auto& child : hidden)
        if (child != nullptr)
            child->setVisible (true);

    hidden.clear();

    if (hostComponent != nullptr)
        hostComponent->resized();
}

booklet::Topic GuideSourcesTopic::getPrintTopic() const
{
    booklet::Topic topic;
    topic.name = getName();
    topic.summary = summary;
    topic.text = printText;
    return topic;
}
} // namespace daw
