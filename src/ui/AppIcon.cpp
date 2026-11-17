#include "AppIcon.h"
#include "GuideButton.h"
#include "DawItAppIcon.h"

namespace daw
{
namespace
{
    /** How much of the tile's width the book takes: more on small tiles, so the bars stay visible. */
    float getGlyphFraction (float tileSize) { return tileSize <= 48.0f ? 0.76f : 0.62f; }

    bool writePng (const juce::Image& image, const juce::File& file)
    {
        file.deleteFile();

        if (auto stream = file.createOutputStream())
            return juce::PNGImageFormat().writeImageToStream (image, *stream);

        return false;
    }
}

juce::MemoryBlock appicon::makeIco()
{
    const int sizes[] = { 16, 32, 48, 256 };
    std::vector<juce::MemoryBlock> pngs;

    for (const int size : sizes)
    {
        juce::MemoryOutputStream png;
        juce::PNGImageFormat().writeImageToStream (render (size, false), png);
        pngs.push_back (png.getMemoryBlock());
    }

    juce::MemoryOutputStream out;
    out.writeShort (0);   // reserved
    out.writeShort (1);   // icon type
    out.writeShort ((short) pngs.size());
    int offset = 6 + 16 * (int) pngs.size();

    for (size_t i = 0; i < pngs.size(); ++i)
    {
        out.writeByte ((char) (sizes[i] >= 256 ? 0 : sizes[i]));   // 0 means 256
        out.writeByte ((char) (sizes[i] >= 256 ? 0 : sizes[i]));
        out.writeByte (0);    // no palette
        out.writeByte (0);
        out.writeShort (1);   // colour planes
        out.writeShort (32);  // bits per pixel
        out.writeInt ((int) pngs[i].getSize());
        out.writeInt (offset);
        offset += (int) pngs[i].getSize();
    }

    for (const auto& png : pngs)
        out.write (png.getData(), png.getSize());

    return out.getMemoryBlock();
}

juce::Colour appicon::getTileColour() { return juce::Colour (0xff304c37); }
juce::Colour appicon::getGlyphColour() { return juce::Colour (0xfff8f7f2); }

void appicon::drawTile (juce::Graphics& g, juce::Rectangle<float> tile)
{
    g.setColour (getTileColour());
    g.fillRoundedRectangle (tile, tile.getWidth() * 0.2237f);   // Apple's icon corner proportion
    guideicon::draw (g, tile.withSizeKeepingCentre (tile.getWidth() * getGlyphFraction (tile.getWidth()), tile.getHeight()),
                     getGlyphColour());
}

juce::Image appicon::render (int size, bool macInset)
{
    juce::Image image (juce::Image::ARGB, size, size, true);
    juce::Graphics g (image);
    const auto full = juce::Rectangle<float> ((float) size, (float) size);
    const float inset = (float) size * 824.0f / 1024.0f;
    drawTile (g, macInset ? full.withSizeKeepingCentre (inset, inset) : full);
    return image;
}

const std::vector<int>& appicon::getSizes()
{
    static const std::vector<int> sizes { 16, 32, 48, 64, 128, 256, 512, 1024 };
    return sizes;
}

juce::Image appicon::getEmbedded()
{
    return juce::ImageFileFormat::loadFrom (dawicon::appicon256_png, (size_t) dawicon::appicon256_pngSize);
}

bool appicon::exportAll (const juce::File& folder)
{
    if (! folder.createDirectory().wasOk())
        return false;

    bool ok = true;

    for (const int size : getSizes())
        ok = writePng (render (size, false), folder.getChildFile ("app-icon-" + juce::String (size) + ".png")) && ok;

    // The macOS set: the 1024 master, and a 64 px one JUCE scales down for the small Finder sizes.
    ok = writePng (render (1024, true), folder.getChildFile ("app-icon-macos-1024.png")) && ok;
    ok = writePng (render (64, true), folder.getChildFile ("app-icon-macos-64.png")) && ok;

    // The Windows icon file for the installer (the app's own comes from JUCE).
    const auto ico = makeIco();
    ok = folder.getChildFile ("app-icon.ico").replaceWithData (ico.getData(), ico.getSize()) && ok;
    return ok;
}
}
