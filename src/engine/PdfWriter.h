#pragma once

#include <juce_graphics/juce_graphics.h>

/**
    A small PDF writer for printable sheets drawn with juce::Graphics, such as the chord sheets and
    the guide booklets: every page is one picture filling the page, stored losslessly (Flate with
    PNG predictors, so smooth areas and text on white stay small). Pages can carry clickable links,
    and the document can have bookmarks for a reader's sidebar. JUCE draws on screen and into
    images but has no PDF output of its own.
*/
namespace daw::pdf
{
/** A4 in points (1/72 inch). */
constexpr float a4Width = 595.28f, a4Height = 841.89f;

/** US Letter in points. */
constexpr float letterWidth = 612.0f, letterHeight = 792.0f;

/** A clickable area on a page that jumps to another page. `area` is in points from the page's top left. */
struct Link
{
    juce::Rectangle<float> area;
    int targetPage = 0;   // counted from 0
};

/** A line in the reader's sidebar. `level` 1 sits under the level-0 bookmark before it. */
struct Bookmark
{
    juce::String title;
    int page = 0;         // counted from 0
    int level = 0;
};

/** A document built one page at a time, so a long booklet holds only compressed pages in memory. */
class Document final
{
public:
    /** `title` goes into the document's properties. */
    explicit Document (const juce::String& title = {});

    /** Adds a page: the image stretched over a page of this size in points, with its links. The image
        is compressed at once. */
    void addPage (const juce::Image& image, float pageWidth, float pageHeight, std::vector<Link> links = {});

    /** The sidebar bookmarks (pages counted from 0). Without any, the document has no outline. */
    void setBookmarks (std::vector<Bookmark> newBookmarks);

    /** How many pages have been added. */
    int getNumPages() const noexcept { return (int) pages.size(); }

    /** Writes the document. Fails with a message when there are no pages. */
    juce::Result write (juce::OutputStream&) const;

    /** The same, into a file (replaced if it exists; nothing is changed if writing fails). */
    juce::Result write (const juce::File&) const;

private:
    struct Page
    {
        int width = 0, height = 0;
        float pointsWidth = 0.0f, pointsHeight = 0.0f;
        juce::MemoryBlock data;
        std::vector<Link> links;
    };

    juce::String title;
    std::vector<Page> pages;
    std::vector<Bookmark> bookmarks;
};

/** Writes the pages, each image stretched over a page of this size in points. `title` goes into
    the document's properties. */
juce::Result write (const std::vector<juce::Image>& pages, juce::OutputStream&, const juce::String& title = {},
                    float pageWidth = a4Width, float pageHeight = a4Height);

/** The same, into a file (replaced if it exists). */
juce::Result write (const std::vector<juce::Image>& pages, const juce::File&, const juce::String& title = {},
                    float pageWidth = a4Width, float pageHeight = a4Height);
} // namespace daw::pdf
