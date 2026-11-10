#pragma once

#include <algorithm>

namespace daw
{
/** How the window's width is shared between the inspector, the Notebook and the tracks. */
namespace panelbudget
{
    constexpr int inspectorMin = 200, inspectorEdge = 4, notebookPreferred = 360, notebookMin = 240, minCanvas = 630;

    /** What one layout pass gives each panel. */
    struct Result
    {
        int inspectorWidth = 0;   // 0 when the inspector is closed
        int edgeWidth = 0;        // the inspector's drag edge
        int notebookWidth = 0;    // 0 when the Notebook is closed
        int canvasWidth = 0;      // what the tracks get
    };

    /** Works out the panel widths for a window. Both panels always stay docked. When the tracks would get less than
        `minCanvas`, the inspector shrinks first (down to its minimum), then the Notebook (down to `notebookMin`);
        below that the tracks simply get less. The preferred inspector width is only an input, never changed. */
    inline Result compute (int windowWidth, bool inspectorOpen, int preferredInspector, bool notebookOpen) noexcept
    {
        Result r;
        int free = windowWidth;

        if (inspectorOpen)
        {
            r.edgeWidth = inspectorEdge;
            free -= inspectorEdge;
            r.inspectorWidth = std::max (inspectorMin, std::min (preferredInspector, windowWidth / 2));
        }

        if (notebookOpen)
            r.notebookWidth = notebookPreferred;

        int deficit = minCanvas - (free - r.inspectorWidth - r.notebookWidth);

        if (deficit > 0 && r.inspectorWidth > inspectorMin)
        {
            const int give = std::min (deficit, r.inspectorWidth - inspectorMin);
            r.inspectorWidth -= give;
            deficit -= give;
        }

        if (deficit > 0 && r.notebookWidth > notebookMin)
            r.notebookWidth -= std::min (deficit, r.notebookWidth - notebookMin);

        r.canvasWidth = free - r.inspectorWidth - r.notebookWidth;
        return r;
    }
}
}
