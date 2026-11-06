#pragma once

#include "engine/GuideBooklet.h"
#include "generate/GuideSources.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <vector>

namespace daw
{
/**
    What the last topic of every guide, *Sources*, shows: one scrolling panel in the guide's `panelLight` box with the
    intro (where the numbers come from), any notes written for readers, then the pages read as links and the searches
    that shaped a fact. The list is read from `SOURCES.md` (`gen::guidesources`), so it cannot drift from the file.

    A click on a link opens it in the default browser; DawIt sends nothing itself. A right-click offers *Open in browser*
    and *Copy link*. Over a link, the line is underlined and the tooltip gives the full address.
*/
class GuideSourcesPanel final : public juce::Component
{
public:
    explicit GuideSourcesPanel (const juce::String& guideKey = {});
    ~GuideSourcesPanel() override;

    /** Shows the sources of the guide with this key, scrolled to the top. */
    void setKey (const juce::String& guideKey);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Content;

    juce::Viewport viewport;
    std::unique_ptr<Content> content;
};

//==============================================================================
/**
    What a guide window owns to show its *Sources* topic. It adds the last row of the topics list (after *Your song*),
    paints that row like the others, owns the panel that takes the place of the picture and the explanation while the
    row is selected, and gives the topic for the PDF. The window keeps its own topics as they are: it asks `isRow` first
    and handles the extra row before its own switch.
*/
class GuideSourcesTopic
{
public:
    /** The panel becomes a hidden child of `parent`. */
    GuideSourcesTopic (juce::Component& parent, const juce::String& guideKey);

    /** "Sources". */
    static juce::String getName();

    /** "12 pages read, 5 searches" or "No sources recorded yet". */
    juce::String getSummary() const { return summary; }

    /** True for the list row after the guide's own `ownTopics` topics. */
    static bool isRow (int row, int ownTopics) noexcept { return row == ownTopics; }

    /** Paints the list row in the look of a guide's topic rows (name over summary, accent when selected). */
    void paintRow (juce::Graphics&, int width, int height, bool selected) const;

    /** Shows or hides the panel, placing it where the picture and the explanation were. */
    void show (bool shown, juce::Rectangle<int> area);
    bool isShown() const { return panel.isVisible(); }

    /** Shows the Sources topic in a guide window that has a list of topics: selects its row (`ownTopics`, one past the
        guide's own), hides every child of `host` except the panel and those in `keep` (the title, the list, the export
        button), and asks `host` to lay out again. The window's `resized()` then calls `show (true, area)` and returns. */
    void select (juce::Component& host, juce::ListBox&, int ownTopics, std::initializer_list<juce::Component*> keep);

    /** Leaves the Sources topic: shows again what `select` hid. Does nothing when the topic is not shown. Call it first in
        whatever shows another topic. */
    void close();

    /** The topic for the PDF: name, summary and the full text, with no picture. */
    booklet::Topic getPrintTopic() const;

    GuideSourcesPanel& getPanel() { return panel; }

private:
    GuideSourcesPanel panel;
    juce::String summary, printText;
    juce::Component* hostComponent = nullptr;
    std::vector<juce::Component::SafePointer<juce::Component>> hidden;   // safe: a window may delete tiles while Sources is shown
};
} // namespace daw
