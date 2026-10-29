#include "GuideBooklet.h"

#include <array>
#include <cmath>
#include <unordered_map>

namespace daw::booklet
{
namespace
{
    struct Lab { float L, a, b; };

    float toLinear (float c)
    {
        return c <= 0.04045f ? c / 12.92f : std::pow ((c + 0.055f) / 1.055f, 2.4f);
    }

    float fromLinear (float c)
    {
        c = juce::jlimit (0.0f, 1.0f, c);
        return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow (c, 1.0f / 2.4f) - 0.055f;
    }

    Lab toOkLab (juce::Colour c)
    {
        const float r = toLinear (c.getFloatRed()), g = toLinear (c.getFloatGreen()), b = toLinear (c.getFloatBlue());
        const float l = std::cbrt (0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b);
        const float m = std::cbrt (0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b);
        const float s = std::cbrt (0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b);

        return { 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s,
                 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s,
                 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s };
    }

    /** The linear sRGB of an OKLab colour; outside 0..1 when it's out of gamut. */
    std::array<float, 3> toLinearRgb (const Lab& lab)
    {
        const float l = std::pow (lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b, 3.0f);
        const float m = std::pow (lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b, 3.0f);
        const float s = std::pow (lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b, 3.0f);

        return { 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
                -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
                -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s };
    }

    float smoothstep (float lo, float hi, float x)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (x - lo) / (hi - lo));
        return t * t * (3.0f - 2.0f * t);
    }

    float relativeLuminance (juce::Colour c)
    {
        return 0.2126f * toLinear (c.getFloatRed()) + 0.7152f * toLinear (c.getFloatGreen()) + 0.0722f * toLinear (c.getFloatBlue());
    }

    /** The lightness of the app's background and of its text (theme::background and theme::text), the two ends
        that become white and near-black on paper. */
    const float darkEnd = toOkLab (juce::Colour (0xff1b1c20)).L, lightEnd = toOkLab (juce::Colour (0xffd9dce3)).L;
}

juce::Point<float> getSize (Paper paper)
{
    return paper == Paper::letter ? juce::Point<float> (pdf::letterWidth, pdf::letterHeight)
                                  : juce::Point<float> (pdf::a4Width, pdf::a4Height);
}

juce::String getName (Paper paper)
{
    return paper == Paper::letter ? "Letter (8.5 x 11 in)" : "A4 (210 x 297 mm)";
}

Paper getDefaultPaper (const juce::String& region)
{
    // Unicode CLDR's supplementalData: the territories that use US Letter.
    static const juce::StringArray letter { "BZ", "CA", "CL", "CO", "CR", "GT", "MX", "NI", "PA", "PH", "PR", "SV", "US", "VE" };
    return letter.contains (region.trim().toUpperCase()) ? Paper::letter : Paper::a4;
}

juce::Point<int> getPixelSize (Paper paper, float dpi)
{
    const auto size = getSize (paper);
    return { juce::roundToInt (size.x / 72.0f * dpi), juce::roundToInt (size.y / 72.0f * dpi) };
}

float getOkLightness (juce::Colour c)
{
    return toOkLab (c).L;
}

float getContrastRatio (juce::Colour a, juce::Colour b)
{
    const float la = relativeLuminance (a), lb = relativeLuminance (b);
    return (std::max (la, lb) + 0.05f) / (std::min (la, lb) + 0.05f);
}

juce::Colour toPaper (juce::Colour colour)
{
    auto lab = toOkLab (colour);
    const float chroma = std::sqrt (lab.a * lab.a + lab.b * lab.b);

    // Where the colour sits between the app's background and its text, turned round: background 1 (white), text 0.15.
    const float t = (lab.L - darkEnd) / (lightEnd - darkEnd);
    const float neutral = juce::jlimit (0.0f, 1.0f, 1.0f - 0.85f * t);

    // Greys and dim tints count as neutral; real colours keep their hue and are only darkened.
    const float w = smoothstep (0.06f, 0.12f, chroma);
    const float lightness = (1.0f - w) * neutral + w * std::min (lab.L, 0.62f);

    Lab mapped { lightness, lab.a, lab.b };

    for (int i = 0; i < 100; ++i)
    {
        const auto rgb = toLinearRgb (mapped);
        const float eps = 0.0005f;

        if (rgb[0] >= -eps && rgb[0] <= 1.0f + eps && rgb[1] >= -eps && rgb[1] <= 1.0f + eps && rgb[2] >= -eps && rgb[2] <= 1.0f + eps)
            break;

        mapped.a *= 0.99f;
        mapped.b *= 0.99f;
    }

    const auto rgb = toLinearRgb (mapped);
    return juce::Colour::fromFloatRGBA (fromLinear (rgb[0]), fromLinear (rgb[1]), fromLinear (rgb[2]), colour.getFloatAlpha());
}

void toPaper (juce::Image& image)
{
    if (! image.isValid())
        return;

    std::unordered_map<juce::uint32, juce::uint32> done;
    juce::Image::BitmapData pixels (image, juce::Image::BitmapData::readWrite);

    for (int y = 0; y < pixels.height; ++y)
        for (int x = 0; x < pixels.width; ++x)
        {
            const auto c = pixels.getPixelColour (x, y);
            const auto key = c.getARGB();
            auto found = done.find (key);

            if (found == done.end())
                found = done.emplace (key, toPaper (c).getARGB()).first;

            pixels.setPixelColour (x, y, juce::Colour (found->second));
        }
}

//==============================================================================
namespace
{
    constexpr float margin = 40.0f, lineHeight = 24.0f, headingHeight = 28.0f, paragraphGap = 6.0f, footerSpace = 20.0f;
    constexpr float continuedTop = margin + 26.0f;   // below the running head on a page that carries on
    constexpr float textSize = 10.5f;

    float getContentBottom (Paper paper) { return getSize (paper).y - margin - footerSpace; }
    float getTextWidth (Paper paper) { return getSize (paper).x - 2.0f * margin; }

    juce::TextLayout makeLayout (const juce::String& text, float size, bool bold, juce::Colour colour, float width,
                                 juce::Justification justification = juce::Justification::topLeft)
    {
        juce::AttributedString s;
        s.setWordWrap (juce::AttributedString::byWord);
        s.setJustification (justification);
        s.append (text, juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain), colour);
        juce::TextLayout layout;
        layout.createLayout (s, width);
        return layout;
    }

    float getTextWidthOf (const juce::String& text, float size, bool bold)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)), text, 0.0f, 0.0f);
        return glyphs.getBoundingBox (0, -1, true).getWidth();
    }

    void drawLine (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, bool bold, juce::Colour c,
                   juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (text, r, j, true);
    }

    juce::Colour inkColour (const Options& o, juce::Colour c) { return o.paperColours ? toPaper (c) : c; }

    /** The song's name: the first part of "My Song · A minor · 96 BPM". */
    juce::String getSongName (const Booklet& b)
    {
        const auto at = b.songLine.indexOf (juce::String::fromUTF8 (" \xc2\xb7 "));
        return (at >= 0 ? b.songLine.substring (0, at) : b.songLine).trim();
    }

    /** A paragraph of a topic's text and where it starts in the whole text. */
    struct Paragraph { int start; juce::String text; };

    std::vector<Paragraph> splitParagraphs (const juce::String& text)
    {
        std::vector<Paragraph> result;
        int from = 0;

        while (from <= text.length())
        {
            int to = text.indexOf (from, "\n\n");

            if (to < 0)
                to = text.length();

            const auto piece = text.substring (from, to);

            if (piece.trim().isNotEmpty())
                result.push_back ({ from, piece.trim() });

            from = to + 2;

            while (from < text.length() && text[from] == '\n')
                ++from;

            if (to >= text.length())
                break;
        }

        // Paragraph starts must point at their trimmed text, not at the whitespace before it.
        for (auto& p : result)
        {
            const auto lead = text.substring (p.start).length() - text.substring (p.start).trimStart().length();
            p.start += lead;
        }

        return result;
    }

    /** One line of a topic's text: where its characters start and where it sits in the block. */
    struct TextLine { int start; float y, h; };

    std::vector<TextLine> measureText (const juce::String& text, float width)
    {
        std::vector<TextLine> lines;
        float y = 0.0f;

        for (auto& paragraph : splitParagraphs (text))
        {
            const auto layout = makeLayout (paragraph.text, textSize, false, ink, width);

            for (int i = 0; i < layout.getNumLines(); ++i)
            {
                auto& line = layout.getLine (i);
                const auto bounds = line.getLineBoundsY();
                lines.push_back ({ paragraph.start + line.stringRange.getStart(), y + bounds.getStart(), bounds.getLength() });
            }

            y += layout.getHeight() + paragraphGap;
        }

        return lines;
    }

    /** What a topic's pages look like: the first page's picture and text top, then where each page's text begins. */
    std::vector<PageInfo> layoutTopic (const Topic& topic, int guideIndex, int topicIndex, const Options& options)
    {
        const auto size = getSize (options.paper);
        const float width = getTextWidth (options.paper), bottom = getContentBottom (options.paper);

        PageInfo first;
        first.kind = PageInfo::Kind::topic;
        first.guide = guideIndex;
        first.topic = topicIndex;

        float y = margin + 26.0f;
        y += makeLayout (topic.name, 18.0f, true, ink, width).getHeight() + 4.0f;

        if (topic.summary.isNotEmpty())
            y += makeLayout (topic.summary, 11.0f, false, inkDim, width).getHeight() + 10.0f;

        if (topic.pictureSize.x > 0 && topic.pictureSize.y > 0)
        {
            float w = width, h = w * (float) topic.pictureSize.y / (float) topic.pictureSize.x;
            const float most = 0.45f * size.y;

            if (h > most)
            {
                h = most;
                w = h * (float) topic.pictureSize.x / (float) topic.pictureSize.y;
            }

            first.pictureArea = { margin + (width - w) * 0.5f, y, w, h };
            y += h + 14.0f;
        }

        first.textTop = y;

        // Fill the pages with lines.
        const auto lines = measureText (topic.text, width);
        std::vector<PageInfo> pages { first };
        std::vector<int> starts { 0 };
        float pageStartY = lines.empty() ? 0.0f : lines.front().y;

        for (auto& line : lines)
        {
            const float top = pages.back().textTop;

            if (top + (line.y + line.h - pageStartY) > bottom && line.start > starts.back())
            {
                PageInfo next;
                next.kind = PageInfo::Kind::topic;
                next.guide = guideIndex;
                next.topic = topicIndex;
                next.continued = true;
                next.textTop = continuedTop + makeLayout (topic.name + " (continued)", 14.0f, true, ink, width).getHeight() + 10.0f;
                pages.push_back (next);
                starts.push_back (line.start);
                pageStartY = line.y;
            }
        }

        // The first page of a topic with lines starts at 0 so the text is covered exactly.
        for (size_t i = 0; i < pages.size(); ++i)
            pages[i].text = { starts[i], i + 1 < starts.size() ? starts[i + 1] : topic.text.length() };

        return pages;
    }

    /** The lines of a cover's contents. */
    struct ContentsLine
    {
        juce::String name, detail;
        juce::Colour colour;
        int page = 0;
        bool dot = false;
    };

    /** The top of a cover: the stripe's label, the name, the summary and where the pictures come from. */
    struct CoverHeader
    {
        CoverHeader (const Booklet& b, int guide, bool showSong, const Options& options)
        {
            const float width = getTextWidth (options.paper);
            label = guide >= 0 ? juce::String::fromUTF8 ("DawIt guide \xc2\xb7 ") + b.guides[(size_t) guide].area : juce::String ("DawIt guides");
            name = makeLayout (guide >= 0 ? b.guides[(size_t) guide].name : b.title, 30.0f, true, ink, width);
            summary = makeLayout (guide >= 0 ? b.guides[(size_t) guide].summary : b.summary, 13.0f, false, inkDim, width);
            const auto source = guide >= 0 ? b.guides[(size_t) guide].source : juce::String();
            sourceLine = makeLayout (source.isNotEmpty() ? "Pictures from: " + source : juce::String(), 10.0f, false, inkDim, width);
            songLine = makeLayout (showSong ? (b.songLine + (b.date.isNotEmpty() ? juce::String::fromUTF8 (" \xc2\xb7 ") + b.date : juce::String())) : juce::String(),
                                   10.0f, false, inkDim, width);

            float y = margin + 16.0f;
            nameY = y;
            y += name.getHeight() + 6.0f;
            summaryY = y;
            y += summary.getHeight() + 14.0f;
            sourceY = y;

            if (source.isNotEmpty())
                y += sourceLine.getHeight() + 3.0f;

            songY = y;

            if (showSong && b.songLine.isNotEmpty())
                y += songLine.getHeight() + 3.0f;
            else
                songLine = {};

            bottom = y + 16.0f;
        }

        void draw (juce::Graphics& g, const Options& options, juce::Colour area) const
        {
            const float width = getTextWidth (options.paper);
            g.setColour (inkColour (options, area));
            g.fillRect (0.0f, 0.0f, getSize (options.paper).x, 14.0f);
            drawLine (g, label, { margin, margin - 2.0f, width, 12.0f }, 9.0f, false, inkDim);
            name.draw (g, { margin, nameY, width, name.getHeight() });
            summary.draw (g, { margin, summaryY, width, summary.getHeight() });
            sourceLine.draw (g, { margin, sourceY, width, sourceLine.getHeight() });
            songLine.draw (g, { margin, songY, width, songLine.getHeight() });
        }

        juce::String label;
        juce::TextLayout name, summary, sourceLine, songLine;
        float nameY = 0.0f, summaryY = 0.0f, sourceY = 0.0f, songY = 0.0f, bottom = 0.0f;
    };

    std::vector<ContentsLine> getContentsLines (const Booklet& b, int guide, const Plan& plan, const Options& options)
    {
        std::vector<ContentsLine> lines;

        if (guide >= 0)
        {
            auto& g = b.guides[(size_t) guide];

            for (size_t t = 0; t < g.topics.size(); ++t)
                lines.push_back ({ g.topics[t].name, g.topics[t].summary, {}, plan.topicPages[(size_t) guide][t], false });
        }
        else
        {
            for (size_t i = 0; i < b.guides.size(); ++i)
                lines.push_back ({ b.guides[i].name, b.guides[i].area, inkColour (options, b.guides[i].colour), plan.guidePages[i], true });
        }

        return lines;
    }

    int getFirstCapacity (float headerBottom, Paper paper)
    {
        return std::max (1, (int) std::floor ((getContentBottom (paper) - (headerBottom + headingHeight)) / lineHeight));
    }

    int getLaterCapacity (Paper paper)
    {
        return std::max (1, (int) std::floor ((getContentBottom (paper) - (continuedTop + 30.0f)) / lineHeight));
    }

    /** How many pages a cover with this many contents lines takes. */
    int countCoverPages (int numLines, float headerBottom, Paper paper)
    {
        const int first = getFirstCapacity (headerBottom, paper);
        return 1 + (numLines > first ? (numLines - first + getLaterCapacity (paper) - 1) / getLaterCapacity (paper) : 0);
    }

    void drawContentsLines (juce::Graphics& g, const std::vector<ContentsLine>& lines, int from, int count, float top, const Options& options)
    {
        const float width = getTextWidth (options.paper);
        float y = top;

        for (int i = from; i < from + count && i < (int) lines.size(); ++i)
        {
            auto& line = lines[(size_t) i];
            float x = margin;

            g.setColour (rule);
            g.fillRect (margin, y + lineHeight - 0.5f, width, 0.5f);

            if (line.dot)
            {
                g.setColour (line.colour);
                g.fillEllipse (x, y + lineHeight * 0.5f - 4.0f, 8.0f, 8.0f);
                x += 16.0f;
            }

            const float nameWidth = std::min (getTextWidthOf (line.name, 11.0f, true) + 4.0f, width * 0.6f);
            drawLine (g, line.name, { x, y, nameWidth, lineHeight }, 11.0f, true, ink);
            const float detailX = x + nameWidth + 8.0f;
            drawLine (g, line.detail, { detailX, y, margin + width - 40.0f - detailX, lineHeight }, 9.0f, false, inkDim);
            drawLine (g, juce::String (line.page + 1), { margin + width - 36.0f, y, 36.0f, lineHeight }, 11.0f, false, inkDim, juce::Justification::centredRight);
            y += lineHeight;
        }
    }

    juce::String getFooterName (const Booklet& b, const PageInfo& page)
    {
        const bool inGuide = page.guide >= 0 && page.guide < (int) b.guides.size();
        return inGuide ? b.guides[(size_t) page.guide].name : b.title;
    }
}

Plan makePlan (const Booklet& b, const Options& options)
{
    Plan plan;
    const bool isSet = b.guides.size() > 1;
    const size_t n = b.guides.size();
    const auto paper = options.paper;
    const float width = getTextWidth (paper);

    // Topic pages first: they depend on the text alone.
    std::vector<std::vector<std::vector<PageInfo>>> topicLayouts (n);

    for (size_t g = 0; g < n; ++g)
        for (size_t t = 0; t < b.guides[g].topics.size(); ++t)
            topicLayouts[g].push_back (layoutTopic (b.guides[g].topics[t], (int) g, (int) t, options));

    // The covers: how many pages each takes.
    const CoverHeader setHeader (b, n == 1 ? 0 : -1, true, options);
    const int setCoverPages = countCoverPages (n == 1 ? (int) b.guides[0].topics.size() : (int) n, setHeader.bottom, paper);
    std::vector<CoverHeader> guideHeaders;
    std::vector<int> guideCoverPages;

    if (isSet)
        for (size_t g = 0; g < n; ++g)
        {
            guideHeaders.emplace_back (b, (int) g, false, options);
            guideCoverPages.push_back (countCoverPages ((int) b.guides[g].topics.size(), guideHeaders.back().bottom, paper));
        }

    // Page numbers.
    int index = setCoverPages;
    plan.guidePages.assign (n, 0);
    plan.topicPages.resize (n);

    for (size_t g = 0; g < n; ++g)
    {
        if (isSet)
        {
            plan.guidePages[g] = index;
            index += guideCoverPages[g];
        }
        else
        {
            plan.guidePages[g] = 0;
        }

        for (size_t t = 0; t < topicLayouts[g].size(); ++t)
        {
            plan.topicPages[g].push_back (index);
            index += (int) topicLayouts[g][t].size();
        }
    }

    // The pages themselves.
    auto addCover = [&] (int guide, const CoverHeader& header, int pagesForIt, bool showKind)
    {
        const auto lines = getContentsLines (b, guide, plan, options);
        const int firstCap = getFirstCapacity (header.bottom, paper), laterCap = getLaterCapacity (paper);
        int done = 0;

        for (int p = 0; p < pagesForIt; ++p)
        {
            PageInfo page;
            page.kind = p == 0 ? (showKind ? PageInfo::Kind::guideCover : PageInfo::Kind::cover) : PageInfo::Kind::contents;
            page.guide = guide;
            page.contentsContinued = p > 0;
            page.firstLine = done;
            page.numLines = std::min ((int) lines.size() - done, p == 0 ? firstCap : laterCap);
            const float top = p == 0 ? header.bottom + headingHeight : continuedTop + 30.0f;

            for (int i = 0; i < page.numLines; ++i)
                page.links.push_back ({ { margin, top + (float) i * lineHeight, width, lineHeight }, lines[(size_t) (done + i)].page });

            done += page.numLines;
            plan.pages.push_back (page);
        }
    };

    if (n == 0)
    {
        addCover (-1, setHeader, 1, false);
    }
    else if (! isSet)
    {
        addCover (0, setHeader, setCoverPages, false);
    }
    else
    {
        addCover (-1, setHeader, setCoverPages, false);

        for (size_t g = 0; g < n; ++g)
            addCover ((int) g, guideHeaders[g], guideCoverPages[g], true);
    }

    // Interleave: in a set, the guide covers come before their topics; rebuild the order from the numbers.
    std::vector<PageInfo> covers = std::move (plan.pages);
    plan.pages.clear();
    size_t nextCover = 0;
    auto takeCovers = [&] (int count) { for (int i = 0; i < count; ++i) plan.pages.push_back (covers[nextCover++]); };
    takeCovers (setCoverPages);

    for (size_t g = 0; g < n; ++g)
    {
        if (isSet)
            takeCovers (guideCoverPages[g]);

        for (auto& pages : topicLayouts[g])
            for (auto& page : pages)
                plan.pages.push_back (page);
    }

    // Bookmarks.
    for (size_t g = 0; g < n; ++g)
    {
        if (isSet)
            plan.bookmarks.push_back ({ b.guides[g].name, plan.guidePages[g], 0 });

        for (size_t t = 0; t < b.guides[g].topics.size(); ++t)
            plan.bookmarks.push_back ({ b.guides[g].topics[t].name, plan.topicPages[g][t], isSet ? 1 : 0 });
    }

    return plan;
}

juce::Image renderPage (const Booklet& b, const Plan& plan, int pageIndex, const Options& options)
{
    const auto size = getSize (options.paper);
    const auto pixels = getPixelSize (options.paper, options.dpi);
    juce::Image image (juce::Image::RGB, pixels.x, pixels.y, true, juce::SoftwareImageType());
    juce::Graphics g (image);
    g.fillAll (juce::Colours::white);
    g.addTransform (juce::AffineTransform::scale ((float) pixels.x / size.x, (float) pixels.y / size.y));

    const auto& page = plan.pages[(size_t) pageIndex];
    const float width = getTextWidth (options.paper);
    const bool isSet = b.guides.size() > 1;
    const bool hasGuide = page.guide >= 0 && page.guide < (int) b.guides.size();
    const auto areaColour = hasGuide ? b.guides[(size_t) page.guide].colour : (b.guides.empty() ? ink : b.guides.front().colour);
    const auto areaInk = inkColour (options, areaColour);

    switch (page.kind)
    {
        case PageInfo::Kind::cover:
        case PageInfo::Kind::guideCover:
        {
            const bool showSong = page.kind == PageInfo::Kind::cover;
            const CoverHeader header (b, page.guide, showSong, options);
            header.draw (g, options, isSet && page.guide < 0 ? ink : areaColour);
            drawLine (g, page.guide >= 0 ? "In this guide" : "In this booklet", { margin, header.bottom, width, headingHeight }, 13.0f, true, ink);
            drawContentsLines (g, getContentsLines (b, page.guide, plan, options), page.firstLine, page.numLines, header.bottom + headingHeight, options);
            break;
        }

        case PageInfo::Kind::contents:
        {
            drawLine (g, getFooterName (b, page), { margin, margin, width, 12.0f }, 8.5f, false, inkDim);
            g.setColour (rule);
            g.fillRect (margin, margin + 14.0f, width, 0.6f);
            drawLine (g, "Contents, continued", { margin, continuedTop - 4.0f, width, 28.0f }, 14.0f, true, ink);
            drawContentsLines (g, getContentsLines (b, page.guide, plan, options), page.firstLine, page.numLines, continuedTop + 30.0f, options);
            break;
        }

        case PageInfo::Kind::topic:
        {
            const auto& guide = b.guides[(size_t) page.guide];
            const auto& topic = guide.topics[(size_t) page.topic];
            drawLine (g, guide.name, { margin, margin, width * 0.7f, 12.0f }, 8.5f, false, inkDim);
            drawLine (g, guide.area, { margin + width * 0.7f, margin, width * 0.3f, 12.0f }, 8.5f, false, areaInk, juce::Justification::centredRight);
            g.setColour (rule);
            g.fillRect (margin, margin + 14.0f, width, 0.6f);

            if (! page.continued)
            {
                float y = margin + 26.0f;
                const auto name = makeLayout (topic.name, 18.0f, true, ink, width);
                name.draw (g, { margin, y, width, name.getHeight() });
                y += name.getHeight() + 4.0f;

                if (topic.summary.isNotEmpty())
                {
                    const auto summary = makeLayout (topic.summary, 11.0f, false, inkDim, width);
                    summary.draw (g, { margin, y, width, summary.getHeight() });
                }

                if (! page.pictureArea.isEmpty() && topic.picture.isValid())
                {
                    auto picture = topic.picture;

                    if (options.paperColours)
                    {
                        picture = picture.createCopy();
                        toPaper (picture);
                    }

                    juce::Graphics::ScopedSaveState state (g);
                    juce::Path clip;
                    clip.addRoundedRectangle (page.pictureArea, 6.0f);
                    g.reduceClipRegion (clip);
                    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
                    g.drawImage (picture, page.pictureArea, juce::RectanglePlacement::stretchToFit);

                    g.setColour (rule);
                    g.drawRoundedRectangle (page.pictureArea, 6.0f, 0.6f);
                }
            }
            else
            {
                const auto name = makeLayout (topic.name + " (continued)", 14.0f, true, ink, width);
                name.draw (g, { margin, continuedTop, width, name.getHeight() });
            }

            // The text of this page, paragraph by paragraph.
            float y = page.textTop;
            const auto text = topic.text.substring (page.text.getStart(), page.text.getEnd());

            for (auto& paragraph : splitParagraphs (text))
            {
                const auto layout = makeLayout (paragraph.text, textSize, false, ink, width);
                layout.draw (g, { margin, y, width, layout.getHeight() });
                y += layout.getHeight() + paragraphGap;
            }

            break;
        }
    }

    // The foot of every page.
    const auto footer = getFooterName (b, page) + (getSongName (b).isNotEmpty() ? juce::String::fromUTF8 (" \xc2\xb7 ") + getSongName (b) : juce::String())
                        + juce::String::fromUTF8 (" \xc2\xb7 made with DawIt");
    g.setColour (rule);
    g.fillRect (margin, size.y - margin - 8.0f, width, 0.6f);
    const juce::Rectangle<float> line (margin, size.y - margin - 4.0f, width, 12.0f);
    drawLine (g, footer, line.withTrimmedRight (80.0f), 8.5f, false, inkDim);
    drawLine (g, "Page " + juce::String (pageIndex + 1) + " of " + juce::String ((int) plan.pages.size()), line, 8.5f, false, inkDim,
              juce::Justification::centredRight);

    return image;
}

juce::Result write (const Booklet& b, const Options& options, juce::OutputStream& out)
{
    const auto plan = makePlan (b, options);
    const auto size = getSize (options.paper);
    pdf::Document document (b.guides.size() == 1 ? b.guides.front().name : b.title);

    for (int i = 0; i < (int) plan.pages.size(); ++i)
        document.addPage (renderPage (b, plan, i, options), size.x, size.y, plan.pages[(size_t) i].links);

    document.setBookmarks (plan.bookmarks);
    return document.write (out);
}
} // namespace daw::booklet
