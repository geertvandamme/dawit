#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
/**
    The DawIt app icon: the guide book (`guideicon::getPath()`) in paper colour on a dark-green rounded tile, the
    colours of the website. It is drawn from that one path, so the Dock icon, the files in `resources/icons/`, the
    window icon and the About window can't drift apart. The PNG files are made with the console tool `DawItExportIcon <folder>` (`tools/ExportAppIcon.cpp`).
*/
namespace appicon
{
    /** The tile's colour, the website's dark green (#304c37). */
    juce::Colour getTileColour();

    /** The book's colour, the website's paper (#f8f7f2). */
    juce::Colour getGlyphColour();

    /** Draws the tile (rounded square) filling `tile` and the book centred on it. Small tiles get a larger book. */
    void drawTile (juce::Graphics& g, juce::Rectangle<float> tile);

    /** Renders a square icon image. With `macInset` the tile is the 824/1024 rounded square macOS expects, with a
        transparent margin; without, the tile fills the image (Windows and Linux). */
    juce::Image render (int size, bool macInset);

    /** The sizes of the full-bleed set, from 16 to 1024 px. */
    const std::vector<int>& getSizes();

    /** A Windows .ico file holding the full-bleed icon at 16, 32, 48 and 256 px (PNG entries). */
    juce::MemoryBlock makeIco();

    /** The 256 px icon built into the app, used for the window icon. */
    juce::Image getEmbedded();

    /** Writes the whole PNG set into `folder` (created if needed); false when a file could not be written. */
    bool exportAll (const juce::File& folder);
}
}
