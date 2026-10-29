#pragma once

#include <map>

#include "engine/Engine.h"
#include "GuideButton.h"
#include "engine/SignalFlow.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The signal flow drawn as a diagram, and the Signal Flow window around it.

    `flowview` is the painter: one set of functions that draws a `signalflow::Graph` (boxes, orthogonal lines whose
    thickness and colour follow the level, dashed sends, dots that run to the beat, bypassed and muted states, the
    selected track's path) for the window, for the routing guide's picture and for the guide's PDF. Everything is
    drawn in graph units; the caller scales it.
*/
namespace flowview
{
    constexpr float minZoom = 0.25f, maxZoom = 2.0f, overviewBelow = 0.6f, zoomStep = 1.25f, fitMargin = 24.0f;
    constexpr float dotSpacing = 24.0f;   // graph units between the dots: one step per beat

    /** Where each box sits, in graph units (100% zoom). Below the overview zoom the effect boxes shrink to 10 px squares. */
    struct Layout
    {
        std::vector<juce::Rectangle<float>> boxes;   // one per node
        juce::Rectangle<float> bounds;               // everything
    };

    Layout makeLayout (const signalflow::Graph&, float zoom);

    /** The lines and boxes that went over 0 dB lately, so the red mark stays long enough to see (2 s on a line,
        3 s for the master's CLIP). Updated from each new graph by the view that draws it. */
    class Holds
    {
    public:
        void update (const signalflow::Graph&, double nowSeconds);
        bool isOver (const signalflow::Graph&, int node, double nowSeconds) const;
        bool isClipping (double nowSeconds) const;
        bool isAnyHeld (double nowSeconds) const;

    private:
        using Key = std::tuple<const Track*, int, int>;
        static Key getKey (const signalflow::Graph&, int node);
        std::map<Key, double> over;
        double clipUntil = -1.0e9;
    };

    /** What the painter needs besides the graph. */
    struct Style
    {
        float zoom = 1.0f;
        bool dots = true;                 // the moving dots are switched on
        bool playing = false;
        double beats = 0.0;               // the playhead, for the dots' places
        double now = 0.0;                 // seconds, for the holds
        const Holds* holds = nullptr;
        juce::Rectangle<float> visible;   // graph units; empty: draw everything

        /** Draws a small picture of an effect's settings in its box (null: none). Called only at 100% zoom or more; returns false when the effect has no picture. */
        std::function<bool (juce::Graphics&, const signalflow::Graph&, int node, juce::Rectangle<float> area)> paintPicture;
    };

    /** Draws the graph in graph units: set the transform (scale and offset) before calling. */
    void paint (juce::Graphics&, const signalflow::Graph&, const Layout&, const Style&);

    /** Draws the graph fitted into `area` (the guide's picture, the PDF): boxes, lines and, while playing, dots. */
    void paintFitted (juce::Graphics&, const signalflow::Graph&, juce::Rectangle<float> area, bool playing, double beats);

    /** What is under a point (graph units). */
    struct Hit
    {
        int node = -1;     // a box
        int edge = -1;     // a line (when no box is there)
    };

    Hit hitTest (const signalflow::Graph&, const Layout&, juce::Point<float> graphPoint, float zoom);

    /** The track a box or a line belongs to (a line: the track it comes from); -1 for the master and empty places. */
    int getTrackOfHit (const signalflow::Graph&, const Hit&);

    /** The tooltip of a box or a line: what it is, its levels and its state. */
    juce::String describeNode (const signalflow::Graph&, int node, const Holds*, double nowSeconds);
    juce::String describeEdge (const signalflow::Graph&, int edge);

    /** A line's colour from its level: grey, green, yellow, red; dark when it isn't heard. */
    juce::Colour getLineColour (const signalflow::Edge&);
} // namespace flowview

//==============================================================================
/**
    The Signal Flow window's content: the whole song as one flow diagram, live. A toolbar (zoom, Fit, the moving dots,
    a button to the guide), the diagram, and a legend. It only shows the song: it reads the tracks and their meters and
    changes nothing, and a click on a box or a line only selects that track in the app (the selected track's path
    lights up). Zoom (25% to 200%) and Fit, Cmd+scroll or pinch, scrolling and dragging the empty canvas to pan; below
    60% zoom it becomes an overview.
*/
class SignalFlowView final : public juce::Component,
                             private juce::ChangeListener,
                             private juce::Timer
{
public:
    SignalFlowView (Engine&, Selection&);
    ~SignalFlowView() override;

    /** The guide's *How sound flows?* button and the menu's last item. */
    std::function<void()> onShowGuide;

    /** Scrolls the diagram to the track's row when it is out of view (nothing changes in the song). */
    void showTrack (const Track&);

    /** Zoom as a factor (1: 100%), or fitted to the window (what the toolbar's Fit does). */
    void setZoom (float zoom);
    void fit();

    float getZoom() const noexcept { return zoom; }
    bool isFitted() const noexcept { return fitted; }

    /** The size the window opens at: the last one used, or 1200 x 760. */
    static juce::Point<int> getSavedSize (Engine&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Canvas;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    /** Rebuilds the graph from the song and its meters; true when something that shows has changed. */
    bool refreshGraph();
    void zoomAround (float newZoom, juce::Point<float> viewPoint);
    void setOrigin (juce::Point<float>);
    void updateZoomLabel();
    void scrollToSelection (bool onlyWhenOutOfView);
    void saveState();
    float getFitZoom() const;
    juce::Rectangle<float> getVisibleGraphArea() const;

    Engine& engine;
    Selection& selection;

    juce::Label title { {}, "Signal flow" };
    juce::TextButton zoomOutButton { "-" }, zoomInButton { "+" }, zoomLabel { "100%" }, fitButton { "Fit" };
    GuideButton guideButton { "How sound flows?" };
    juce::ToggleButton dotsButton { "Moving dots" };
    juce::Label legend;
    std::unique_ptr<Canvas> canvas;

    signalflow::Graph graph;
    flowview::Layout layout;
    flowview::Holds holds;
    juce::String shownSignature;
    int shownTimerRate = 0;

    float zoom = 1.0f;
    bool fitted = true;
    juce::Point<float> origin;   // the top-left of what is in view, in graph units
    bool dotsOn = true;
    bool restoredOrigin = false;

    static constexpr int toolbarHeight = 36, legendHeight = 22;
};
} // namespace daw
