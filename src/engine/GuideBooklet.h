#pragma once

#include "PdfWriter.h"

#include <juce_graphics/juce_graphics.h>

#include <vector>

/**
    The booklet a guide (or a set of guides) is saved as: a cover with the contents, then a page per
    topic with its picture and text, laid out and drawn from plain data so it can be tested without
    any window. Also the paper sizes and the paper colours, which turn the app's dark pictures into
    light ones that read on white and save ink.
*/
namespace daw::booklet
{
enum class Paper { a4, letter };

/** The page size in points (1/72 inch): A4 is 595.28 x 841.89, Letter 612 x 792. */
juce::Point<float> getSize (Paper);

/** "A4 (210 x 297 mm)" or "Letter (8.5 x 11 in)". */
juce::String getName (Paper);

/** Letter in the regions Unicode CLDR lists for US Letter (US, CA, MX and a few more), A4 everywhere else
    and for an empty region. `region` is a two-letter code such as "GB". */
Paper getDefaultPaper (const juce::String& region);

/** The size in pixels of a full page at this resolution; A4 at 300 dpi is 2480 x 3508. */
juce::Point<int> getPixelSize (Paper, float dpi);

/** The usual resolution for printed pictures. */
constexpr float printDpi = 300.0f;

/** Dark on white, for paper: the text, the dim text and the hairlines. The chord sheets use them too. */
inline const juce::Colour ink { 0xff1d1e22 }, inkDim { 0xff6b6f78 }, rule { 0xffd5d8de };

/** The colour on paper: the app's dark backgrounds become white, its light text near-black, and colours keep
    their hue but are darkened until they read on white (worked out in OKLab, where equal steps in lightness look
    equal). Greys and dim tints count as neutral. Alpha is kept. */
juce::Colour toPaper (juce::Colour);

/** Every pixel of the picture through `toPaper` (each distinct colour worked out once). */
void toPaper (juce::Image&);

/** The WCAG 2.x contrast ratio of two colours, from 1 (the same) to 21 (black on white). */
float getContrastRatio (juce::Colour, juce::Colour);

/** The lightness of a colour in OKLab, 0 (black) to 1 (white). */
float getOkLightness (juce::Colour);

/** A topic as the guide shows it. The picture may stay empty until its pages are drawn. */
struct Topic
{
    juce::String name, summary, text;   // text: paragraphs separated by blank lines
    juce::Point<int> pictureSize;       // on screen, in pixels; {0, 0}: no picture
    juce::Image picture;                // at print resolution, filled just before its pages are drawn
};

struct Guide
{
    juce::String name, summary, area, source;   // source: "DawIt Reverb on Vocals, as set in its window"
    juce::Colour colour;                        // its area's colour, as on screen
    std::vector<Topic> topics;
};

struct Booklet
{
    juce::String title, summary;        // a set's name and line; a single guide uses its own
    juce::String songLine, date;        // "My Song · A minor · 96 BPM", "26 October 2026"
    std::vector<Guide> guides;          // one: a guide's booklet; more: a set
};

struct Options
{
    Paper paper = Paper::a4;
    float dpi = printDpi;
    bool paperColours = true;           // false: the pictures as they are on screen
};

/** What goes on every page, worked out from the text alone, before any picture is taken. */
struct PageInfo
{
    enum class Kind { cover, contents, guideCover, topic };

    Kind kind = Kind::cover;
    int guide = -1, topic = -1;
    juce::Range<int> text;                      // a topic page: the characters of the topic's text on it
    bool continued = false;
    juce::Rectangle<float> pictureArea;         // points from the top left; empty when none
    float textTop = 0.0f;                       // a topic page: where its text starts, in points from the top
    std::vector<pdf::Link> links;               // the contents lines on it
    int firstLine = 0;                          // a contents page: the first contents line on it
    int numLines = 0;
    bool contentsContinued = false;             // "Contents, continued"
};

struct Plan
{
    std::vector<PageInfo> pages;
    std::vector<int> guidePages;                // the page each guide's cover is on
    std::vector<std::vector<int>> topicPages;   // [guide][topic]: the page the topic starts on
    std::vector<pdf::Bookmark> bookmarks;
};

/** Where every page's text and picture go. */
Plan makePlan (const Booklet&, const Options&);

/** One page drawn at the options' resolution. */
juce::Image renderPage (const Booklet&, const Plan&, int page, const Options&);

/** The whole booklet at once into a PDF. Every topic's picture must be filled first. */
juce::Result write (const Booklet&, const Options&, juce::OutputStream&);
} // namespace daw::booklet
