#include "PdfWriter.h"

#include <array>
#include <cmath>

namespace daw::pdf
{
namespace
{
    int paeth (int a, int b, int c)
    {
        const int p = a + b - c, pa = std::abs (p - a), pb = std::abs (p - b), pc = std::abs (p - c);
        return (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
    }

    /** The pixels as rows of red, green and blue bytes over white paper, each row led by the PNG
        filter (None, Sub, Up or Paeth) that makes it smallest, and compressed in the zlib format
        PDF's FlateDecode reads (with /Predictor 15). */
    juce::MemoryBlock compressRgb (const juce::Image& image)
    {
        juce::MemoryOutputStream compressed;

        {
            juce::GZIPCompressorOutputStream zip (compressed, 6);   // windowBits 0: zlib, with its header
            const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::readOnly);
            const size_t n = (size_t) image.getWidth() * 3;
            std::vector<juce::uint8> row (n), previous (n, 0);
            std::array<std::vector<juce::uint8>, 4> out;

            for (auto& o : out)
                o.resize (n + 1);

            for (int y = 0; y < image.getHeight(); ++y)
            {
                for (int x = 0; x < image.getWidth(); ++x)
                {
                    const auto c = juce::Colours::white.overlaidWith (pixels.getPixelColour (x, y));
                    row[(size_t) x * 3] = c.getRed();
                    row[(size_t) x * 3 + 1] = c.getGreen();
                    row[(size_t) x * 3 + 2] = c.getBlue();
                }

                for (size_t i = 0; i < n; ++i)
                {
                    const int left = i >= 3 ? row[i - 3] : 0, up = previous[i], upLeft = i >= 3 ? previous[i - 3] : 0;
                    out[0][i + 1] = row[i];
                    out[1][i + 1] = (juce::uint8) (row[i] - left);
                    out[2][i + 1] = (juce::uint8) (row[i] - up);
                    out[3][i + 1] = (juce::uint8) (row[i] - paeth (left, up, upLeft));
                }

                // The PNG specification's heuristic: the smallest sum of absolute values, read as signed bytes.
                int best = 0;
                juce::int64 bestSum = -1;

                for (int f = 0; f < 4; ++f)
                {
                    juce::int64 sum = 0;

                    for (size_t i = 1; i <= n; ++i)
                        sum += std::abs ((int) (juce::int8) out[(size_t) f][i]);

                    if (bestSum < 0 || sum < bestSum)
                    {
                        bestSum = sum;
                        best = f;
                    }
                }

                out[(size_t) best][0] = (juce::uint8) (best == 3 ? 4 : best);   // index 3 is Paeth, PNG filter 4
                zip.write (out[(size_t) best].data(), n + 1);
                previous = row;
            }
        }

        return compressed.getMemoryBlock();
    }

    /** A text string as UTF-16 in hex, so any character survives: <FEFF...>. */
    juce::String toPdfText (const juce::String& text)
    {
        juce::String hex ("<FEFF");

        for (auto p = text.getCharPointer(); ! p.isEmpty();)
        {
            auto c = (juce::uint32) p.getAndAdvance();

            if (c >= 0x10000)   // a surrogate pair
            {
                c -= 0x10000;
                hex << juce::String::toHexString ((int) (0xd800 + (c >> 10))).paddedLeft ('0', 4)
                    << juce::String::toHexString ((int) (0xdc00 + (c & 0x3ff))).paddedLeft ('0', 4);
            }
            else
            {
                hex << juce::String::toHexString ((int) c).paddedLeft ('0', 4);
            }
        }

        return hex.toUpperCase() + ">";
    }

    juce::String formatPoints (float v)
    {
        return juce::String (v, 2);
    }
}

Document::Document (const juce::String& t) : title (t) {}

void Document::addPage (const juce::Image& image, float pageWidth, float pageHeight, std::vector<Link> links)
{
    Page page;
    page.width = image.getWidth();
    page.height = image.getHeight();
    page.pointsWidth = pageWidth;
    page.pointsHeight = pageHeight;
    page.data = compressRgb (image);
    page.links = std::move (links);
    pages.push_back (std::move (page));
}

void Document::setBookmarks (std::vector<Bookmark> newBookmarks)
{
    bookmarks = std::move (newBookmarks);
}

juce::Result Document::write (juce::OutputStream& out) const
{
    if (pages.empty())
        return juce::Result::fail ("There are no pages to write.");

    // Objects: 1 the catalog, 2 the page list, 3 the document's properties, then three per page: the page,
    // what's drawn on it, and its picture. After them come the links, then the outline.
    juce::MemoryOutputStream pdf;
    std::vector<juce::int64> offsets;
    auto begin = [&] (int number)
    {
        jassert (number == (int) offsets.size() + 1);
        offsets.push_back (pdf.getPosition());
        pdf << number << " 0 obj\n";
    };

    auto pageObject = [] (size_t page) { return 4 + (int) page * 3; };
    int next = 4 + (int) pages.size() * 3;
    std::vector<std::vector<int>> linkObjects (pages.size());

    for (size_t i = 0; i < pages.size(); ++i)
        for (size_t l = 0; l < pages[i].links.size(); ++l)
            linkObjects[i].push_back (next++);

    const int outlineRoot = bookmarks.empty() ? 0 : next;
    const int firstBookmark = outlineRoot + 1;

    pdf << "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";   // the second line marks the file as binary

    begin (1);
    pdf << "<< /Type /Catalog /Pages 2 0 R";

    if (outlineRoot != 0)
        pdf << " /Outlines " << outlineRoot << " 0 R /PageMode /UseOutlines";

    pdf << " >>\nendobj\n";

    begin (2);
    pdf << "<< /Type /Pages /Count " << (int) pages.size() << " /Kids [";

    for (size_t i = 0; i < pages.size(); ++i)
        pdf << (i > 0 ? " " : "") << pageObject (i) << " 0 R";

    pdf << "] >>\nendobj\n";

    begin (3);
    pdf << "<< /Producer " << toPdfText ("DawIt") << (title.isNotEmpty() ? " /Title " + toPdfText (title) : juce::String()) << " >>\nendobj\n";

    for (size_t i = 0; i < pages.size(); ++i)
    {
        const auto& p = pages[i];
        const int page = pageObject (i);

        begin (page);
        pdf << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << formatPoints (p.pointsWidth) << " " << formatPoints (p.pointsHeight) << "]"
            << " /Resources << /XObject << /Im0 " << page + 2 << " 0 R >> >> /Contents " << page + 1 << " 0 R";

        if (! linkObjects[i].empty())
        {
            pdf << " /Annots [";

            for (size_t l = 0; l < linkObjects[i].size(); ++l)
                pdf << (l > 0 ? " " : "") << linkObjects[i][l] << " 0 R";

            pdf << "]";
        }

        pdf << " >>\nendobj\n";

        // Stretch the picture (a unit square to PDF) over the whole page.
        const juce::String draw = "q " + formatPoints (p.pointsWidth) + " 0 0 " + formatPoints (p.pointsHeight) + " 0 0 cm /Im0 Do Q\n";
        begin (page + 1);
        pdf << "<< /Length " << draw.length() << " >>\nstream\n" << draw << "endstream\nendobj\n";

        begin (page + 2);
        pdf << "<< /Type /XObject /Subtype /Image /Width " << p.width << " /Height " << p.height
            << " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /FlateDecode"
            << " /DecodeParms << /Predictor 15 /Colors 3 /BitsPerComponent 8 /Columns " << p.width << " >>"
            << " /Length " << (int) p.data.getSize() << " >>\nstream\n";
        pdf.write (p.data.getData(), p.data.getSize());
        pdf << "\nendstream\nendobj\n";
    }

    for (size_t i = 0; i < pages.size(); ++i)
        for (size_t l = 0; l < pages[i].links.size(); ++l)
        {
            const auto& link = pages[i].links[l];
            const int target = juce::jlimit (0, (int) pages.size() - 1, link.targetPage);
            const float top = pages[i].pointsHeight - link.area.getY();

            begin (linkObjects[i][l]);
            pdf << "<< /Type /Annot /Subtype /Link /Rect [" << formatPoints (link.area.getX()) << " " << formatPoints (top - link.area.getHeight())
                << " " << formatPoints (link.area.getRight()) << " " << formatPoints (top) << "] /Border [0 0 0] /Dest ["
                << pageObject ((size_t) target) << " 0 R /Fit] >>\nendobj\n";
        }

    if (outlineRoot != 0)
    {
        // Work out each bookmark's parent, siblings and children from the levels.
        const int count = (int) bookmarks.size();
        std::vector<int> parent ((size_t) count, -1), prev ((size_t) count, -1), nextSibling ((size_t) count, -1),
                         first ((size_t) count, -1), last ((size_t) count, -1), children ((size_t) count, 0);
        int lastTop = -1;
        int lastTopChild = -1;

        for (int i = 0; i < count; ++i)
        {
            const bool child = bookmarks[(size_t) i].level > 0 && lastTop >= 0;
            const int p = child ? lastTop : -1;
            parent[(size_t) i] = p;
            const int sibling = child ? (last[(size_t) lastTop]) : lastTopChild;

            if (sibling >= 0)
            {
                prev[(size_t) i] = sibling;
                nextSibling[(size_t) sibling] = i;
            }

            if (child)
            {
                if (first[(size_t) lastTop] < 0)
                    first[(size_t) lastTop] = i;

                last[(size_t) lastTop] = i;
                ++children[(size_t) lastTop];
            }
            else
            {
                lastTop = i;
                lastTopChild = i;
            }
        }

        int topFirst = -1, topLast = -1;

        for (int i = 0; i < count; ++i)
            if (parent[(size_t) i] < 0)
            {
                if (topFirst < 0)
                    topFirst = i;

                topLast = i;
            }

        begin (outlineRoot);
        pdf << "<< /Type /Outlines /First " << firstBookmark + topFirst << " 0 R /Last " << firstBookmark + topLast << " 0 R /Count " << count << " >>\nendobj\n";

        for (int i = 0; i < count; ++i)
        {
            const auto& b = bookmarks[(size_t) i];
            const int target = juce::jlimit (0, (int) pages.size() - 1, b.page);
            begin (firstBookmark + i);
            pdf << "<< /Title " << toPdfText (b.title) << " /Parent " << (parent[(size_t) i] >= 0 ? firstBookmark + parent[(size_t) i] : outlineRoot) << " 0 R";

            if (prev[(size_t) i] >= 0)
                pdf << " /Prev " << firstBookmark + prev[(size_t) i] << " 0 R";

            if (nextSibling[(size_t) i] >= 0)
                pdf << " /Next " << firstBookmark + nextSibling[(size_t) i] << " 0 R";

            if (first[(size_t) i] >= 0)
                pdf << " /First " << firstBookmark + first[(size_t) i] << " 0 R /Last " << firstBookmark + last[(size_t) i] << " 0 R /Count "
                    << children[(size_t) i];

            pdf << " /Dest [" << pageObject ((size_t) target) << " 0 R /Fit] >>\nendobj\n";
        }
    }

    // Where every object starts, so a reader can find them.
    const auto xref = pdf.getPosition();
    pdf << "xref\n0 " << (int) offsets.size() + 1 << "\n0000000000 65535 f \n";

    for (auto offset : offsets)
        pdf << juce::String (offset).paddedLeft ('0', 10) << " 00000 n \n";

    pdf << "trailer\n<< /Size " << (int) offsets.size() + 1 << " /Root 1 0 R /Info 3 0 R >>\nstartxref\n" << (juce::int64) xref << "\n%%EOF\n";

    if (! out.write (pdf.getData(), pdf.getDataSize()))
        return juce::Result::fail ("The PDF couldn't be written.");

    out.flush();
    return juce::Result::ok();
}

juce::Result Document::write (const juce::File& file) const
{
    juce::TemporaryFile temp (file);

    {
        juce::FileOutputStream out (temp.getFile());

        if (out.failedToOpen())
            return juce::Result::fail ("Couldn't write to " + file.getFullPathName() + ": " + out.getStatus().getErrorMessage());

        if (const auto result = write (out); result.failed())
            return result;

        if (out.getStatus().failed())
            return out.getStatus();
    }

    if (! temp.overwriteTargetFileWithTemporary())
        return juce::Result::fail ("Couldn't save " + file.getFullPathName());

    return juce::Result::ok();
}

juce::Result write (const std::vector<juce::Image>& pages, juce::OutputStream& out, const juce::String& title, float pageWidth, float pageHeight)
{
    Document document (title);

    for (auto& page : pages)
        document.addPage (page, pageWidth, pageHeight);

    return document.write (out);
}

juce::Result write (const std::vector<juce::Image>& pages, const juce::File& file, const juce::String& title, float pageWidth, float pageHeight)
{
    Document document (title);

    for (auto& page : pages)
        document.addPage (page, pageWidth, pageHeight);

    return document.write (file);
}
} // namespace daw::pdf
