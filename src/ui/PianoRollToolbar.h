#pragma once

#include <vector>

namespace daw::pianorolltoolbar
{
/** The piano-roll header's tools, highest priority first: the last ones to move into the "More" menu. */
enum Tool
{
    toolQuantise = 0, toolSnap, toolGenerate, toolTransform, toolOctaves, toolVelocity, toolScale, toolKeyChords, toolRhythmGuide, toolMidiGuide, toolCount
};

/** How many of the tools (in priority order, each with its width including its gap) stay as buttons in `available`
    pixels. When all fit, that is all of them. Otherwise it is the longest start of the list that fits next to the
    "More" button, which is `moreWidth` wide. It never grows as the space shrinks. With `alwaysMore` the More button
    is there anyway (it holds something else), so its width is taken off first. */
inline int countKept (const std::vector<int>& widths, int available, int moreWidth, bool alwaysMore = false)
{
    int total = 0;

    for (int w : widths)
        total += w;

    if (! alwaysMore && total <= available)
        return (int) widths.size();

    int used = moreWidth, kept = 0;

    for (int w : widths)
    {
        if (used + w > available)
            break;

        used += w;
        ++kept;
    }

    return kept;
}

/** The width the target label keeps, at least: 170 px, or less when the header is too narrow for that and the
    "More" button (but never under 80 px). */
inline int getLabelWidth (int headerWidth, int moreWidth)
{
    const int wanted = 170;
    const int left = headerWidth - moreWidth - 8;
    return left < wanted ? (left > 80 ? left : 80) : wanted;
}
}
