#include "GuideButton.h"
#include "SignalFlowView.h"

#include <tuple>

#include "GainStagingExplainer.h"
#include "ChainPictures.h"

namespace daw
{
namespace flowview
{
namespace
{
using signalflow::Edge;
using signalflow::Graph;
using signalflow::Node;
using signalflow::NodeKind;

constexpr float boxHeight = 44.0f, rowPitch = 64.0f, columnGap = 48.0f;

juce::String arrow() { return juce::String::fromUTF8 (" \xe2\x86\x92 "); }

juce::Point<float> getBoxSize (const Node& n, float zoom)
{
    const bool overview = zoom < overviewBelow;

    switch (n.kind)
    {
        case NodeKind::source: return { 128.0f, boxHeight };
        case NodeKind::effect: return overview ? juce::Point<float> { 10.0f / zoom, 10.0f / zoom } : juce::Point<float> { 112.0f, boxHeight };
        case NodeKind::fader:  return { 80.0f, boxHeight };
        case NodeKind::sum:    return { 104.0f, boxHeight };
        case NodeKind::master: return { 112.0f, 88.0f };
    }

    return { 100.0f, boxHeight };
}

/** The text in a rectangle, shrunk to fit its width instead of being cut off. */
void drawFitted (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, bool bold, juce::Colour colour,
                 juce::Justification justification = juce::Justification::centredLeft)
{
    if (text.isEmpty() || r.getWidth() < 2.0f)
        return;

    auto makeFont = [bold] (float s) { return juce::Font (juce::FontOptions (s, bold ? juce::Font::bold : juce::Font::plain)); };
    const float width = juce::GlyphArrangement::getStringWidth (makeFont (size), text);

    if (width > r.getWidth() && width > 0.0f)
        size = juce::jmax (size * 0.45f, size * r.getWidth() / width);

    g.setFont (makeFont (size));
    g.setColour (colour);
    g.drawText (text, r, justification, true);
}

juce::String getDbText (float db)
{
    return db <= -99.0f ? juce::String ("-inf dB") : gainstaging::formatDb (db);
}

/** The points of a line, in graph units. A flow line goes right, then up or down in the gap before the box it enters
    (every line into one box shares that vertical piece, so tracks visibly merge into a bus), then right again. A send
    branches off just before or after the fader, goes down to the return's row and then right into its sum. */
std::vector<juce::Point<float>> getPath (const Layout& layout, const Edge& e)
{
    const auto& a = layout.boxes[(size_t) e.from];
    const auto& b = layout.boxes[(size_t) e.to];

    if (! e.isSend)
    {
        const juce::Point<float> p0 { a.getRight(), a.getCentreY() }, p3 { b.getX(), b.getCentreY() };

        if (std::abs (p0.y - p3.y) < 0.5f)
            return { p0, p3 };

        const float elbow = juce::jmax (p0.x + 4.0f, b.getX() - 16.0f);
        return { p0, { elbow, p0.y }, { elbow, p3.y }, p3 };
    }

    const float x = e.preFader ? a.getX() - 10.0f : a.getRight() + 10.0f;
    return { { x, a.getCentreY() }, { x, b.getCentreY() }, { b.getX(), b.getCentreY() } };
}

float getPathLength (const std::vector<juce::Point<float>>& pts)
{
    float length = 0.0f;

    for (size_t i = 1; i < pts.size(); ++i)
        length += pts[i - 1].getDistanceFrom (pts[i]);

    return length;
}

juce::Point<float> getPointAlong (const std::vector<juce::Point<float>>& pts, float distance)
{
    for (size_t i = 1; i < pts.size(); ++i)
    {
        const float segment = pts[i - 1].getDistanceFrom (pts[i]);

        if (distance <= segment || i + 1 == pts.size())
            return segment <= 0.0f ? pts[i] : pts[i - 1] + (pts[i] - pts[i - 1]) * (juce::jmin (distance, segment) / segment);

        distance -= segment;
    }

    return pts.empty() ? juce::Point<float>() : pts.back();
}

float getLineWidth (const Edge& e)
{
    return 1.5f + 4.5f * meterProportion (juce::Decibels::decibelsToGain (e.levelDb, -100.0f));
}

bool isDimmed (const Graph& g, bool lit)
{
    return g.hasSelection && ! lit;
}

/** What each of DawIt's effects does, in a few words; a third-party plugin is named by its format. */
juce::String describeEffect (const Node& n)
{
    if (n.label == "DawIt EQ")          return "turns ranges of frequencies up or down";
    if (n.label == "DawIt Compressor")  return "turns the loud moments down, so the level is more even";
    if (n.label == "DawIt Gate")        return "silences the sound between the notes, when it falls below the threshold";
    if (n.label == "DawIt Reverb")      return "adds the sound of a room";
    if (n.label == "DawIt Delay")       return "adds echoes";
    if (n.label == "DawIt Saturation")  return "adds warmth and harmonics by rounding or cutting off the peaks";
    if (n.label == "DawIt Distortion")  return "adds grit by driving the sound hard into a curve that clips or folds its peaks";

    return n.format == "VST3" ? "a VST3 effect" : n.format == "AudioUnit" ? "an AU effect" : "an effect";
}

juce::String getPanText (float pan)
{
    const int percent = juce::roundToInt (std::abs (pan) * 100.0f);
    return percent == 0 ? juce::String ("pan centre") : juce::String (percent) + (pan < 0.0f ? "% left" : "% right");
}

juce::String getSilenceText (routing::Silence silence)
{
    switch (silence)
    {
        case routing::Silence::muted:    return "Muted: the sound stops here, and so do its sends.";
        case routing::Silence::busMuted: return "Its bus is muted, so nothing from here is heard.";
        case routing::Silence::solo:     return "Not heard: another track is soloed.";
        case routing::Silence::none:     break;
    }

    return {};
}

juce::String getNodeName (const Graph& g, int index, bool asTarget)
{
    const auto& n = g.nodes[(size_t) index];
    const juce::String trackName = n.trackIndex >= 0 ? g.tracks[(size_t) n.trackIndex].name : juce::String();

    switch (n.kind)
    {
        case NodeKind::source: return trackName;
        case NodeKind::effect: return n.label;
        case NodeKind::fader:  return asTarget ? trackName + " fader" : trackName;
        case NodeKind::sum:    return trackName;
        case NodeKind::master: return "Master";
    }

    return trackName;
}
} // namespace

//==============================================================================
Layout makeLayout (const Graph& g, float zoom)
{
    Layout layout;
    layout.boxes.resize (g.nodes.size());
    int numColumns = 0;

    for (auto& n : g.nodes)
        numColumns = juce::jmax (numColumns, n.column + 1);

    std::vector<float> width ((size_t) numColumns, 0.0f), left ((size_t) numColumns, 0.0f);

    for (auto& n : g.nodes)
        width[(size_t) n.column] = juce::jmax (width[(size_t) n.column], getBoxSize (n, zoom).x);

    float x = 0.0f;

    for (int c = 0; c < numColumns; ++c)
    {
        left[(size_t) c] = x;
        x += width[(size_t) c] + columnGap;
    }

    bool first = true;

    for (size_t i = 0; i < g.nodes.size(); ++i)
    {
        const auto& n = g.nodes[i];
        const auto size = getBoxSize (n, zoom);
        const float row = n.kind == NodeKind::master || n.trackIndex < 0 ? g.masterRow : g.tracks[(size_t) n.trackIndex].row;
        const juce::Point<float> centre { left[(size_t) n.column] + width[(size_t) n.column] * 0.5f, boxHeight * 0.5f + row * rowPitch };
        layout.boxes[i] = juce::Rectangle<float> (size.x, size.y).withCentre (centre);
        layout.bounds = first ? layout.boxes[i] : layout.bounds.getUnion (layout.boxes[i]);
        first = false;
    }

    return layout;
}

//==============================================================================
Holds::Key Holds::getKey (const Graph& g, int node)
{
    const auto& n = g.nodes[(size_t) node];
    const Track* track = n.trackIndex >= 0 ? g.tracks[(size_t) n.trackIndex].track : nullptr;
    return { track, (int) n.kind, n.slot * 4 + (int) n.sourceKind };
}

void Holds::update (const Graph& g, double now)
{
    for (auto it = over.begin(); it != over.end();)
        it = it->second < now ? over.erase (it) : std::next (it);

    for (size_t i = 0; i < g.nodes.size(); ++i)
    {
        const auto& n = g.nodes[i];

        if (n.levelDb > 0.0f)
        {
            if (n.kind == NodeKind::master)
                clipUntil = now + 3.0;
            else
                over[getKey (g, (int) i)] = now + 2.0;
        }
    }
}

bool Holds::isOver (const Graph& g, int node, double now) const
{
    if (g.nodes[(size_t) node].levelDb > 0.0f)
        return true;

    const auto it = over.find (getKey (g, node));
    return it != over.end() && it->second >= now;
}

bool Holds::isClipping (double now) const
{
    return clipUntil >= now;
}

bool Holds::isAnyHeld (double now) const
{
    return clipUntil >= now || std::any_of (over.begin(), over.end(), [now] (auto& p) { return p.second >= now; });
}

//==============================================================================
juce::Colour getLineColour (const Edge& e)
{
    if (e.silenced)
        return theme::gridBar;

    return gainstaging::getZoneColour (gainstaging::getZone (e.levelDb));
}

void paint (juce::Graphics& g, const Graph& graph, const Layout& layout, const Style& style)
{
    const float zoom = style.zoom;
    const bool overview = zoom < overviewBelow;
    const auto visible = style.visible.isEmpty() ? layout.bounds.expanded (200.0f) : style.visible.expanded (40.0f);
    const float onePixel = 1.0f / zoom;
    const double phase = std::fmod (style.beats, 1.0);

    // The lines, under the boxes.
    for (const auto& e : graph.edges)
    {
        const auto points = getPath (layout, e);
        juce::Rectangle<float> area (points[0], points[0]);

        for (auto& p : points)
            area = area.getUnion (juce::Rectangle<float> (p, p));

        if (! area.expanded (10.0f).intersects (visible))
            continue;

        const float alpha = isDimmed (graph, e.lit) ? 0.3f : 1.0f;
        const auto colour = e.isSend && ! e.silenced ? graph.tracks[(size_t) graph.nodes[(size_t) e.to].trackIndex].colour : getLineColour (e);
        const float width = e.silenced ? 1.5f : getLineWidth (e);

        juce::Path path;
        path.startNewSubPath (points[0]);

        for (size_t i = 1; i < points.size(); ++i)
            path.lineTo (points[i]);

        g.setColour (colour.withMultipliedAlpha (alpha));

        if (e.isSend)
        {
            juce::Path dashed;
            const float dashes[] = { 6.0f, 4.0f };
            juce::PathStrokeType (width, juce::PathStrokeType::mitered, juce::PathStrokeType::butt).createDashedStroke (dashed, path, dashes, 2);
            g.fillPath (dashed);
        }
        else
        {
            g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        }

        // The arrowhead where the line enters a box.
        const auto tip = points.back();
        juce::Path head;
        head.addTriangle (tip.x, tip.y, tip.x - 7.0f, tip.y - 4.0f, tip.x - 7.0f, tip.y + 4.0f);
        g.fillPath (head);

        if (e.isSend && ! overview)
        {
            // The level and where it is taken from, along the part that runs to the return.
            const juce::Point<float> from { points[1].x + (points[2].x - points[1].x) * 0.4f, points[1].y };
            const juce::String text = juce::String (e.sendDb, 0) + " dB";
            const float labelWidth = 48.0f;
            const auto labelArea = juce::Rectangle<float> (from.x, from.y - 13.0f, labelWidth, 11.0f);
            drawFitted (g, text, labelArea, 10.0f, false, theme::textDim.withMultipliedAlpha (alpha));

            if (e.preFader)
                drawFitted (g, "PRE", juce::Rectangle<float> (from.x + juce::GlyphArrangement::getStringWidth (juce::Font (juce::FontOptions (10.0f)), text) + 4.0f,
                                                             from.y - 13.0f, 26.0f, 11.0f), 10.0f, true, theme::meterYellow.withMultipliedAlpha (alpha));
        }
    }

    // A switched-off effect: the line goes round it.
    for (size_t i = 0; i < graph.nodes.size(); ++i)
    {
        const auto& n = graph.nodes[i];

        if (n.kind != NodeKind::effect || ! n.bypassed)
            continue;

        const auto& r = layout.boxes[i];
        juce::Path arc;
        const float lift = overview ? 8.0f * onePixel : 14.0f;
        arc.startNewSubPath (r.getX() - 10.0f, r.getCentreY());
        arc.cubicTo (r.getX() + 4.0f, r.getY() - lift, r.getRight() - 4.0f, r.getY() - lift, r.getRight() + 10.0f, r.getCentreY());
        g.setColour (gainstaging::getZoneColour (gainstaging::getZone (n.levelDb)).withMultipliedAlpha (isDimmed (graph, n.lit) ? 0.3f : 0.9f));
        g.strokePath (arc, juce::PathStrokeType (2.0f));
    }

    // The dots, one step per beat, along the lines that carry sound.
    if (style.playing && style.dots)
    {
        for (const auto& e : graph.edges)
        {
            if (e.silenced || e.levelDb < gainstaging::silentBelowDb)
                continue;

            const auto points = getPath (layout, e);
            const float length = getPathLength (points);

            if (length <= 0.0f)
                continue;

            const float prop = meterProportion (juce::Decibels::decibelsToGain (e.levelDb, -100.0f));
            const float alpha = (0.45f + 0.5f * prop) * (isDimmed (graph, e.lit) ? 0.3f : 1.0f);
            const float radius = 1.6f + 1.8f * prop;
            g.setColour (juce::Colours::white.withAlpha (alpha));

            for (float s = (float) phase * dotSpacing; s < length; s += dotSpacing)
            {
                const auto p = getPointAlong (points, s);

                if (visible.contains (p))
                    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (p));
            }
        }
    }

    // The boxes.
    for (size_t i = 0; i < graph.nodes.size(); ++i)
    {
        const auto& n = graph.nodes[i];
        const auto r = layout.boxes[i];

        if (! r.expanded (4.0f).intersects (visible))
            continue;

        const bool dim = isDimmed (graph, n.lit);
        const bool bypassed = n.kind == NodeKind::effect && n.bypassed;
        const float a = (dim ? 0.3f : 1.0f) * (bypassed ? 0.45f : 1.0f);
        auto withAlpha = [a] (juce::Colour c) { return c.withMultipliedAlpha (a); };
        const juce::String trackName = n.trackIndex >= 0 ? graph.tracks[(size_t) n.trackIndex].name : juce::String();
        const juce::Colour trackColour = n.trackIndex >= 0 ? graph.tracks[(size_t) n.trackIndex].colour : theme::accent;
        const bool highlighted = graph.hasSelection && n.lit && n.kind != NodeKind::master;
        const float corner = overview && n.kind == NodeKind::effect ? 1.5f : 4.0f;

        g.setColour (withAlpha (theme::panelLight));
        g.fillRoundedRectangle (r, corner);
        g.setColour (withAlpha (highlighted ? theme::accent : theme::gridBar));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, highlighted ? 1.6f : 1.0f);

        if (n.kind == NodeKind::source || n.kind == NodeKind::sum)
        {
            g.setColour (withAlpha (trackColour));
            g.fillRoundedRectangle (r.withWidth (5.0f), corner);
        }

        const float textSize = overview ? 11.0f * onePixel : 12.0f;
        const float smallSize = overview ? 9.0f * onePixel : 10.0f;
        const auto inner = r.reduced (9.0f, 4.0f);

        switch (n.kind)
        {
            case NodeKind::source:
            {
                drawFitted (g, trackName, inner.withTrimmedLeft (3.0f).withHeight (overview ? inner.getHeight() : 18.0f), textSize + 1.0f, true, withAlpha (theme::text));

                if (! overview)
                {
                    const juce::String what = n.sourceKind == signalflow::SourceKind::audio ? juce::String ("audio clips")
                                              : n.sourceKind == signalflow::SourceKind::builtInSynth ? "DawIt Synth" + (n.detail.isNotEmpty() ? " (" + n.detail + ")" : juce::String())
                                                                                                     : n.label;
                    drawFitted (g, what, inner.withTrimmedLeft (3.0f).withTrimmedTop (20.0f), smallSize, false, withAlpha (theme::textDim));
                }

                break;
            }

            case NodeKind::effect:
                if (! overview)
                {
                    juce::String name = n.label;
                    drawFitted (g, name, juce::Rectangle<float> (r.getX() + 6.0f, r.getY() + 3.0f, r.getWidth() - 12.0f, 14.0f), 11.0f, true,
                                withAlpha (bypassed ? theme::textDim : theme::text), juce::Justification::centred);

                    const auto picture = juce::Rectangle<float> (r.getX() + 6.0f, r.getY() + 19.0f, r.getWidth() - 12.0f, r.getHeight() - 23.0f);

                    bool drawn = false;

                    if (style.paintPicture != nullptr && zoom >= 1.0f)
                    {
                        g.saveState();
                        g.reduceClipRegion (picture.getSmallestIntegerContainer());
                        drawn = style.paintPicture (g, graph, (int) i, picture);
                        g.restoreState();
                    }

                    if (! drawn)
                    {
                        drawFitted (g, bypassed ? juce::String ("off") : getDbText (n.levelDb), picture, 10.0f, false, withAlpha (theme::textDim), juce::Justification::centred);
                    }
                }

                break;

            case NodeKind::fader:
                if (! overview)
                {
                    drawFitted (g, "fader", inner.withHeight (12.0f), smallSize, false, withAlpha (theme::textDim), juce::Justification::centred);
                    drawFitted (g, n.muted ? juce::String ("muted") : juce::String (n.volumeDb, 1) + " dB", inner.withTrimmedTop (11.0f).withHeight (15.0f), 13.0f, true,
                                withAlpha (n.muted ? theme::meterRed : theme::text), juce::Justification::centred);
                    drawFitted (g, getPanText (n.pan), inner.withTrimmedTop (27.0f), smallSize - 1.0f, false, withAlpha (theme::textDim), juce::Justification::centred);
                }
                else
                {
                    drawFitted (g, n.muted ? juce::String ("M") : juce::String (juce::roundToInt (n.volumeDb)), inner, textSize, false, withAlpha (theme::textDim), juce::Justification::centred);
                }

                break;

            case NodeKind::sum:
            {
                const auto sigma = juce::String::fromUTF8 ("\xce\xa3");
                const bool isReturn = n.trackIndex >= 0 && graph.tracks[(size_t) n.trackIndex].isReturn;
                drawFitted (g, sigma + " " + trackName, inner.withTrimmedLeft (3.0f).withHeight (overview ? inner.getHeight() : 18.0f), textSize + 1.0f, true, withAlpha (theme::text));

                if (! overview)
                    drawFitted (g, isReturn ? juce::String ("return") : n.feeders.size() == 1 ? juce::String ("bus, 1 track") : "bus, " + juce::String (n.feeders.size()) + " tracks",
                                inner.withTrimmedLeft (3.0f).withTrimmedTop (20.0f), smallSize, false, withAlpha (theme::textDim));

                break;
            }

            case NodeKind::master:
            {
                const bool clipping = style.holds != nullptr ? style.holds->isClipping (style.now) : n.levelDb > 0.0f;
                drawFitted (g, "Master", juce::Rectangle<float> (r.getX() + 8.0f, r.getY() + 6.0f, r.getWidth() - 40.0f, 16.0f), textSize + 2.0f, true, withAlpha (theme::text));
                const auto meter = juce::Rectangle<float> (r.getRight() - 26.0f, r.getY() + 8.0f, 16.0f, r.getHeight() - 16.0f);
                drawStereoMeter (g, meter, n.levelLeft, n.levelRight);

                if (! overview)
                    drawFitted (g, getDbText (n.levelDb), juce::Rectangle<float> (r.getX() + 8.0f, r.getBottom() - 22.0f, r.getWidth() - 40.0f, 14.0f), 11.0f, false, withAlpha (theme::textDim));

                if (clipping)
                {
                    const auto badge = juce::Rectangle<float> (r.getX() + 8.0f, r.getY() + 26.0f, 42.0f, 16.0f);
                    g.setColour (theme::meterRed);
                    g.fillRoundedRectangle (badge, 3.0f);
                    drawFitted (g, "CLIP", badge, 11.0f, true, juce::Colours::white, juce::Justification::centred);
                }

                break;
            }
        }

        // A red mark where a line went over 0 dB lately.
        if (n.kind != NodeKind::master && (style.holds != nullptr ? style.holds->isOver (graph, (int) i, style.now) : n.levelDb > 0.0f) && ! bypassed)
        {
            g.setColour (theme::meterRed);
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getRight() - 2.0f, r.getY() + 2.0f }));
        }
    }
}

void paintFitted (juce::Graphics& g, const Graph& graph, juce::Rectangle<float> area, bool playing, double beats)
{
    if (graph.nodes.empty() || area.isEmpty())
        return;

    // The guide's picture is small but has room for the names, so it never uses the overview layout.
    const auto layout = makeLayout (graph, 1.0f);
    const float zoom = juce::jlimit (minZoom, 1.0f, juce::jmin (area.getWidth() / layout.bounds.getWidth(), area.getHeight() / layout.bounds.getHeight()));

    const auto size = juce::Point<float> (layout.bounds.getWidth(), layout.bounds.getHeight()) * zoom;
    const auto topLeft = area.getCentre() - size * 0.5f;

    g.saveState();
    g.reduceClipRegion (area.getSmallestIntegerContainer());
    g.addTransform (juce::AffineTransform::translation (-layout.bounds.getX(), -layout.bounds.getY()).scaled (zoom).translated (topLeft.x, topLeft.y));

    Style style;
    style.zoom = 1.0f;
    style.playing = playing;
    style.beats = beats;
    flowview::paint (g, graph, layout, style);
    g.restoreState();
}

//==============================================================================
Hit hitTest (const Graph& graph, const Layout& layout, juce::Point<float> p, float zoom)
{
    Hit hit;

    for (size_t i = layout.boxes.size(); i-- > 0;)
        if (layout.boxes[i].expanded (1.0f).contains (p))
        {
            hit.node = (int) i;
            return hit;
        }

    float best = 6.0f / zoom;

    for (size_t i = 0; i < graph.edges.size(); ++i)
    {
        const auto points = getPath (layout, graph.edges[i]);

        for (size_t k = 1; k < points.size(); ++k)
        {
            juce::Point<float> nearest;
            const float d = juce::Line<float> (points[k - 1], points[k]).getDistanceFromPoint (p, nearest);

            if (d < best)
            {
                best = d;
                hit.edge = (int) i;
            }
        }
    }

    return hit;
}

int getTrackOfHit (const Graph& graph, const Hit& hit)
{
    if (hit.node >= 0)
        return graph.nodes[(size_t) hit.node].trackIndex;

    if (hit.edge >= 0)
        return graph.nodes[(size_t) graph.edges[(size_t) hit.edge].from].trackIndex;

    return -1;
}

//==============================================================================
juce::String describeNode (const Graph& g, int index, const Holds* holds, double now)
{
    const auto& n = g.nodes[(size_t) index];
    const juce::String track = n.trackIndex >= 0 ? g.tracks[(size_t) n.trackIndex].name : juce::String();
    const auto silence = n.trackIndex >= 0 ? g.tracks[(size_t) n.trackIndex].silence : routing::Silence::none;
    const juce::String click = " Click to select the track.";

    switch (n.kind)
    {
        case NodeKind::source:
            if (n.sourceKind == signalflow::SourceKind::audio)
                return track + ": its audio clips, where its sound starts. Level " + getDbText (n.levelDb) + "." + click;

            if (n.sourceKind == signalflow::SourceKind::builtInSynth)
                return track + ": DawIt Synth" + (n.detail.isNotEmpty() ? " (" + n.detail + ")" : juce::String()) + " turns the track's MIDI notes into sound. Level " + getDbText (n.levelDb) + "." + click;

            return n.label + " on " + track + ": the instrument that turns the track's MIDI notes into sound. Level " + getDbText (n.levelDb) + "." + click;

        case NodeKind::effect:
            if (n.bypassed)
                return n.label + " on " + track + " is switched off: the sound goes past it unchanged. Switch it on in the device bar." + click;

            return n.label + " on " + track + ": " + describeEffect (n) + ". In " + getDbText (n.inputDb) + ", out " + getDbText (n.levelDb) + "." + click;

        case NodeKind::fader:
        {
            juce::String text = track + "'s fader: " + juce::String (n.volumeDb, 1) + " dB, " + getPanText (n.pan) + ". Out " + getDbText (n.levelDb) + ".";

            if (silence != routing::Silence::none)
                text << " " << getSilenceText (silence);

            return text + click;
        }

        case NodeKind::sum:
        {
            const bool isReturn = g.tracks[(size_t) n.trackIndex].isReturn;
            auto list = [] (const juce::StringArray& names)
            {
                return names.size() <= 1 ? names.joinIntoString ("") : names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1];
            };

            if (isReturn)
                return track + ": " + (n.feeders.isEmpty() ? juce::String ("nothing sends here yet") : "the sends of " + list (n.feeders) + " add up here") + ". Level " + getDbText (n.levelDb) + "." + click;

            return track + ": " + juce::String (n.feeders.size()) + (n.feeders.size() == 1 ? " track adds" : " tracks add") + " up here"
                   + (n.feeders.isEmpty() ? juce::String() : " (" + list (n.feeders) + ")") + ". Level " + getDbText (n.levelDb) + ". A bus is usually louder than any one of its tracks." + click;
        }

        case NodeKind::master:
        {
            juce::String text = "The master: everything adds up here, then the master volume (" + juce::String (n.masterVolumeDb, 1) + " dB), out to your speakers. Peak " + getDbText (n.levelDb) + ".";

            if (holds != nullptr && holds->isClipping (now))
                text << " It went over 0 dB in the last 3 seconds: that clips at your speakers and in a 16- or 24-bit bounce.";

            return text;
        }
    }

    return {};
}

juce::String describeEdge (const Graph& g, int index)
{
    const auto& e = g.edges[(size_t) index];
    const auto& from = g.nodes[(size_t) e.from];

    if (e.isSend)
        return g.tracks[(size_t) from.trackIndex].name + " sends to " + g.tracks[(size_t) g.nodes[(size_t) e.to].trackIndex].name + " at " + juce::String (e.sendDb, 1)
               + " dB, " + (e.preFader ? "pre-fader" : "post-fader") + ": " + getDbText (e.levelDb) + " peak now. Change it with the send knob in the mixer.";

    juce::String text = getNodeName (g, e.from, false) + arrow() + getNodeName (g, e.to, true) + ": " + getDbText (e.levelDb) + " peak";
    const auto zone = gainstaging::getZone (e.levelDb);

    if (zone == gainstaging::Zone::over)
        text << ", over 0 dB. Nothing distorts inside DawIt, but effects after this point react to the hot level; only the master's output clips.";
    else
        text << ", " << (zone == gainstaging::Zone::silent ? juce::String ("silent") : gainstaging::getZoneName (zone)) << ".";

    if (e.silenced && from.trackIndex >= 0)
        text << " " << getSilenceText (g.tracks[(size_t) from.trackIndex].silence);

    return text;
}
} // namespace flowview

//==============================================================================
/** The diagram: draws the graph under the view's transform, and takes the mouse (select, pan, zoom, the view's menu). */
class SignalFlowView::Canvas final : public juce::Component,
                                     public juce::TooltipClient,
                                     private juce::ScrollBar::Listener
{
public:
    explicit Canvas (SignalFlowView& o) : owner (o), verticalBar (true), horizontalBar (false)
    {
        setWantsKeyboardFocus (true);

        for (auto* bar : { &verticalBar, &horizontalBar })
        {
            addAndMakeVisible (*bar);
            bar->addListener (this);
            bar->setAutoHide (true);
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme::lane);

        if (owner.graph.nodes.empty())
            return;

        g.saveState();
        g.addTransform (juce::AffineTransform::translation (-owner.origin.x, -owner.origin.y).scaled (owner.zoom));

        flowview::Style style;
        style.zoom = owner.zoom;
        style.dots = owner.dotsOn;
        style.playing = owner.engine.isPlaying();
        style.beats = owner.engine.getPositionBeats();
        style.now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        style.holds = &owner.holds;
        style.visible = owner.getVisibleGraphArea();
        style.paintPicture = [] (juce::Graphics& gr, const signalflow::Graph& flow, int node, juce::Rectangle<float> area) -> bool
        {
            const auto& n = flow.nodes[(size_t) node];

            if (n.trackIndex < 0 || flow.tracks[(size_t) n.trackIndex].track == nullptr || n.slot < 0)
                return false;

            const auto* track = flow.tracks[(size_t) n.trackIndex].track;

            if ((size_t) n.slot >= track->plugins.size())
                return false;

            auto* instance = track->plugins[(size_t) n.slot]->instance.get();

            if (! chainpictures::hasPicture (instance))
                return false;

            gr.setColour (theme::background.withAlpha (0.7f));
            gr.fillRoundedRectangle (area, 2.0f);
            chainpictures::paintCurve (gr, area.reduced (2.0f), instance, n.bypassed);
            return true;
        };

        flowview::paint (g, owner.graph, owner.layout, style);
        g.restoreState();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        verticalBar.setBounds (r.removeFromRight (10));
        horizontalBar.setBounds (r.removeFromBottom (10));
        updateScrollBars();
    }

    /** Sets the scroll bars from the diagram's size, the zoom and where the view is. */
    void updateScrollBars()
    {
        const auto bounds = owner.layout.bounds;
        const float pad = flowview::fitMargin / owner.zoom;
        const float viewW = (float) getWidth() / owner.zoom, viewH = (float) getHeight() / owner.zoom;
        updating = true;
        horizontalBar.setRangeLimits (juce::Range<double> (juce::jmin ((double) bounds.getX() - pad, (double) owner.origin.x), juce::jmax ((double) bounds.getRight() + pad, (double) owner.origin.x + viewW)));
        horizontalBar.setCurrentRange ((double) owner.origin.x, (double) viewW);
        verticalBar.setRangeLimits (juce::Range<double> (juce::jmin ((double) bounds.getY() - pad, (double) owner.origin.y), juce::jmax ((double) bounds.getBottom() + pad, (double) owner.origin.y + viewH)));
        verticalBar.setCurrentRange ((double) owner.origin.y, (double) viewH);
        updating = false;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();
        downPosition = e.position;
        downOrigin = owner.origin;
        panned = false;

        if (e.mods.isPopupMenu())
        {
            showMenu();
            return;
        }

        hitAtDown = flowview::hitTest (owner.graph, owner.layout, toGraph (e.position), owner.zoom);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || hitAtDown.node >= 0 || hitAtDown.edge >= 0)
            return;

        if (! panned && e.position.getDistanceFrom (downPosition) < 3.0f)
            return;

        panned = true;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        owner.setOrigin (downOrigin - (e.position - downPosition) / owner.zoom);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);

        // A click on a box or a line selects its track, and that is all it does.
        if (panned || e.mods.isPopupMenu())
            return;

        const int trackIndex = flowview::getTrackOfHit (owner.graph, hitAtDown);

        if (trackIndex < 0)
            return;

        const auto* wanted = owner.graph.tracks[(size_t) trackIndex].track;

        for (auto& t : owner.engine.getTracks())
            if (t.get() == wanted)
            {
                owner.selection.select (t.get());
                return;
            }
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (e.mods.isCommandDown() || e.mods.isCtrlDown())
        {
            owner.zoomAround (owner.zoom * std::exp (wheel.deltaY * 0.8f), e.position);
            return;
        }

        float dx = wheel.deltaX, dy = wheel.deltaY;

        if (e.mods.isShiftDown() && juce::approximatelyEqual (dx, 0.0f))
        {
            dx = dy;
            dy = 0.0f;
        }

        const float k = 500.0f / owner.zoom;
        owner.setOrigin (owner.origin - juce::Point<float> (dx, dy) * k);
    }

    void mouseMagnify (const juce::MouseEvent& e, float scale) override
    {
        owner.zoomAround (owner.zoom * scale, e.position);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        const auto mods = key.getModifiers();

        if (! mods.isCommandDown())
            return false;

        const auto code = key.getKeyCode();

        if (code == '=' || code == '+')
            owner.zoomAround (owner.zoom * flowview::zoomStep, getLocalBounds().toFloat().getCentre());
        else if (code == '-')
            owner.zoomAround (owner.zoom / flowview::zoomStep, getLocalBounds().toFloat().getCentre());
        else if (code == '0')
            owner.fit();
        else
            return false;

        return true;
    }

    juce::String getTooltip() override
    {
        if (owner.graph.nodes.empty())
            return {};

        const auto hit = flowview::hitTest (owner.graph, owner.layout, toGraph (getMouseXYRelative().toFloat()), owner.zoom);
        const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;

        if (hit.node >= 0)
            return flowview::describeNode (owner.graph, hit.node, &owner.holds, now);

        if (hit.edge >= 0)
            return flowview::describeEdge (owner.graph, hit.edge);

        return "The whole song's signal flow, live. It only shows: change things in the device bar, the mixer or a track's menu. Drag to move around; scroll to scroll; "
               "Cmd+scroll or pinch to zoom; click a box to select its track. Right-click for the view's menu.";
    }

private:
    juce::Point<float> toGraph (juce::Point<float> p) const { return owner.origin + p / owner.zoom; }

    void scrollBarMoved (juce::ScrollBar* bar, double newStart) override
    {
        if (updating)
            return;

        auto moved = owner.origin;

        if (bar == &verticalBar)
            moved.y = (float) newStart;
        else
            moved.x = (float) newStart;

        owner.setOrigin (moved);
    }

    /** The view's own menu: nothing here changes the song. */
    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Fit to Window", true, false);
        menu.addItem (2, "Zoom to 100%", true, false);
        menu.addItem (3, "Show the Selected Track", owner.selection.getTrack() != nullptr, false);
        menu.addItem (4, "Moving Dots", true, owner.dotsOn);
        menu.addSeparator();
        guideicon::addItem (menu, 5, "About signal flow...");

        menu.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<SignalFlowView> (&owner)] (int result)
        {
            if (safe == nullptr)
                return;

            if (result == 1)
                safe->fit();
            else if (result == 2)
                safe->zoomAround (1.0f, safe->getLocalBounds().toFloat().getCentre());
            else if (result == 3 && safe->selection.getTrack() != nullptr)
                safe->showTrack (*safe->selection.getTrack());
            else if (result == 4)
                safe->dotsButton.triggerClick();
            else if (result == 5 && safe->onShowGuide != nullptr)
                safe->onShowGuide();
        });
    }

    SignalFlowView& owner;
    juce::ScrollBar verticalBar, horizontalBar;
    juce::Point<float> downPosition, downOrigin;
    flowview::Hit hitAtDown;
    bool panned = false, updating = false;
};

//==============================================================================
SignalFlowView::SignalFlowView (Engine& e, Selection& s)
    : engine (e), selection (s)
{
    auto* settings = engine.properties.getUserSettings();
    const double savedZoom = settings->getDoubleValue ("view.signalFlowZoom", 0.0);
    dotsOn = settings->getBoolValue ("view.signalFlowPulses", true);

    if (savedZoom > 0.0)
    {
        fitted = false;
        zoom = juce::jlimit (flowview::minZoom, flowview::maxZoom, (float) savedZoom);
        const auto scroll = juce::StringArray::fromTokens (settings->getValue ("view.signalFlowScroll", ""), ",", "");

        if (scroll.size() == 2)
        {
            origin = { scroll[0].getFloatValue(), scroll[1].getFloatValue() };
            restoredOrigin = true;
        }
    }

    title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, theme::text);
    addAndMakeVisible (title);

    for (auto* b : std::initializer_list<juce::TextButton*> { &zoomOutButton, &zoomInButton, &zoomLabel, &fitButton, &guideButton })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }

    zoomOutButton.setTooltip ("Zoom out (Cmd+minus, Cmd+scroll or pinch)");
    zoomInButton.setTooltip ("Zoom in (Cmd+plus, Cmd+scroll or pinch)");
    zoomLabel.setTooltip ("The zoom. Click for 100%");
    fitButton.setTooltip ("Fit the whole song in the window (Cmd+0). It stays fitted as the window or the song changes, until you zoom");
    guideButton.setTooltip ("The Signal flow topic of the Buses, sends and routing guide: what this picture shows, and why the order of effects matters");
    dotsButton.setTooltip ("Dots that run along the lines to the beat while the song plays. Switch them off for a still picture: the lines still show the levels");
    dotsButton.setWantsKeyboardFocus (false);
    dotsButton.setToggleState (dotsOn, juce::dontSendNotification);
    addAndMakeVisible (dotsButton);

    zoomOutButton.onClick = [this] { setZoom (zoom / flowview::zoomStep); };
    zoomInButton.onClick = [this] { setZoom (zoom * flowview::zoomStep); };
    zoomLabel.onClick = [this] { setZoom (1.0f); };
    fitButton.onClick = [this] { fit(); };
    guideButton.onClick = [this] { if (onShowGuide != nullptr) onShowGuide(); };
    dotsButton.onClick = [this]
    {
        dotsOn = dotsButton.getToggleState();
        engine.properties.getUserSettings()->setValue ("view.signalFlowPulses", dotsOn);
        canvas->repaint();
    };

    legend.setText (juce::String::fromUTF8 ("Lines: thicker and brighter is louder \xc2\xb7 grey quiet, green healthy, yellow hot, red over 0 dB \xc2\xb7 dashed: a send \xc2\xb7 "
                                            "grey box: switched off \xc2\xb7 dark line: muted, or not heard while another track is soloed \xc2\xb7 "
                                            "this window only shows the song: change it in the device bar or the mixer"),
                    juce::dontSendNotification);
    legend.setFont (juce::FontOptions (11.0f));
    legend.setColour (juce::Label::textColourId, theme::textDim);
    legend.setMinimumHorizontalScale (0.6f);
    legend.setTooltip (legend.getText());
    addAndMakeVisible (legend);

    canvas = std::make_unique<Canvas> (*this);
    addAndMakeVisible (*canvas);

    const auto saved = getSavedSize (engine);
    setSize (saved.x, saved.y);

    refreshGraph();

    if (! fitted && ! restoredOrigin)
        origin = layout.bounds.getTopLeft() - juce::Point<float> (flowview::fitMargin, flowview::fitMargin) / zoom;

    updateZoomLabel();
    engine.addChangeListener (this);
    selection.addChangeListener (this);
    startTimerHz (4);
}

SignalFlowView::~SignalFlowView()
{
    stopTimer();
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
    saveState();
}

juce::Point<int> SignalFlowView::getSavedSize (Engine& engine)
{
    const auto parts = juce::StringArray::fromTokens (engine.properties.getUserSettings()->getValue ("view.signalFlowSize", ""), ",", "");

    if (parts.size() == 2 && parts[0].getIntValue() >= 640 && parts[1].getIntValue() >= 420)
        return { parts[0].getIntValue(), parts[1].getIntValue() };

    return { 1200, 760 };
}

void SignalFlowView::saveState()
{
    auto* settings = engine.properties.getUserSettings();
    settings->setValue ("view.signalFlowZoom", fitted ? 0.0 : (double) zoom);
    settings->setValue ("view.signalFlowScroll", juce::String (origin.x, 1) + "," + juce::String (origin.y, 1));
    settings->setValue ("view.signalFlowPulses", dotsOn);

    if (getWidth() > 0 && getHeight() > 0)
        settings->setValue ("view.signalFlowSize", juce::String (getWidth()) + "," + juce::String (getHeight()));
}

void SignalFlowView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void SignalFlowView::resized()
{
    auto r = getLocalBounds();
    auto bar = r.removeFromTop (toolbarHeight).reduced (8, 4);
    title.setBounds (bar.removeFromLeft (100));
    bar.removeFromLeft (8);
    zoomOutButton.setBounds (bar.removeFromLeft (28));
    bar.removeFromLeft (2);
    zoomLabel.setBounds (bar.removeFromLeft (56));
    bar.removeFromLeft (2);
    zoomInButton.setBounds (bar.removeFromLeft (28));
    bar.removeFromLeft (8);
    fitButton.setBounds (bar.removeFromLeft (48));
    bar.removeFromLeft (12);
    dotsButton.setBounds (bar.removeFromLeft (120));
    guideButton.setBounds (bar.removeFromRight (164));

    legend.setBounds (r.removeFromBottom (legendHeight).reduced (8, 2));
    canvas->setBounds (r);

    if (fitted)
        fit();
    else
        canvas->updateScrollBars();

    saveState();
}

//==============================================================================
float SignalFlowView::getFitZoom() const
{
    if (layout.bounds.isEmpty() || canvas == nullptr)
        return 1.0f;

    const float w = (float) canvas->getWidth() - 2.0f * flowview::fitMargin, h = (float) canvas->getHeight() - 2.0f * flowview::fitMargin;
    return juce::jlimit (flowview::minZoom, 1.0f, juce::jmin (w / layout.bounds.getWidth(), h / layout.bounds.getHeight()));
}

juce::Rectangle<float> SignalFlowView::getVisibleGraphArea() const
{
    return { origin.x, origin.y, (float) canvas->getWidth() / zoom, (float) canvas->getHeight() / zoom };
}

void SignalFlowView::fit()
{
    fitted = true;

    // The overview squares change the size a little, so settle on a zoom in two passes.
    for (int pass = 0; pass < 2; ++pass)
    {
        zoom = getFitZoom();
        layout = flowview::makeLayout (graph, zoom);
    }

    if (canvas != nullptr && ! layout.bounds.isEmpty())
    {
        const auto view = juce::Point<float> ((float) canvas->getWidth(), (float) canvas->getHeight()) / zoom;
        origin = layout.bounds.getCentre() - view * 0.5f;
    }

    updateZoomLabel();

    if (canvas != nullptr)
    {
        canvas->updateScrollBars();
        canvas->repaint();
    }

    saveState();
}

void SignalFlowView::setZoom (float newZoom)
{
    zoomAround (newZoom, canvas != nullptr ? canvas->getLocalBounds().toFloat().getCentre() : juce::Point<float>());
}

void SignalFlowView::zoomAround (float newZoom, juce::Point<float> viewPoint)
{
    newZoom = juce::jlimit (flowview::minZoom, flowview::maxZoom, newZoom);
    const auto graphPoint = origin + viewPoint / zoom;
    fitted = false;
    zoom = newZoom;
    layout = flowview::makeLayout (graph, zoom);
    origin = graphPoint - viewPoint / zoom;
    updateZoomLabel();
    canvas->updateScrollBars();
    canvas->repaint();
    saveState();
}

void SignalFlowView::setOrigin (juce::Point<float> newOrigin)
{
    // Keep some of the diagram in view.
    const auto view = juce::Point<float> ((float) canvas->getWidth(), (float) canvas->getHeight()) / zoom;
    const float pad = flowview::fitMargin / zoom;
    newOrigin.x = juce::jlimit (layout.bounds.getX() - view.x + pad * 3.0f, layout.bounds.getRight() - pad * 3.0f, newOrigin.x);
    newOrigin.y = juce::jlimit (layout.bounds.getY() - view.y + pad * 3.0f, layout.bounds.getBottom() - pad * 3.0f, newOrigin.y);
    origin = newOrigin;
    canvas->updateScrollBars();
    canvas->repaint();
    saveState();
}

void SignalFlowView::updateZoomLabel()
{
    const auto text = juce::String (juce::roundToInt (zoom * 100.0f)) + "%";

    if (zoomLabel.getButtonText() != text)
        zoomLabel.setButtonText (text);
}

void SignalFlowView::showTrack (const Track& track)
{
    for (size_t i = 0; i < graph.tracks.size(); ++i)
        if (graph.tracks[i].track == &track)
        {
            juce::Rectangle<float> area;
            bool first = true;

            for (size_t k = 0; k < graph.nodes.size(); ++k)
                if (graph.nodes[k].trackIndex == (int) i)
                {
                    area = first ? layout.boxes[k] : area.getUnion (layout.boxes[k]);
                    first = false;
                }

            if (first || fitted)
                return;

            const auto visible = getVisibleGraphArea().reduced (8.0f / zoom);

            if (! visible.contains (area))
                setOrigin (area.getCentre() - juce::Point<float> ((float) canvas->getWidth(), (float) canvas->getHeight()) / zoom * 0.5f);

            return;
        }
}

void SignalFlowView::scrollToSelection (bool)
{
    if (auto* t = selection.getTrack())
        showTrack (*t);
}

//==============================================================================
void SignalFlowView::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    refreshGraph();
    canvas->repaint();

    if (source == &selection)
        scrollToSelection (true);
}

bool SignalFlowView::refreshGraph()
{
    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    signalflow::MasterState master;
    master.peakLeft = engine.masterPeakLeft.load();
    master.peakRight = engine.masterPeakRight.load();
    master.volumeDb = engine.masterVolumeDb.load();

    // Only reads the song: the tracks are taken as const.
    graph = signalflow::build (engine.getTracks(), selection.getTrack(), master);
    holds.update (graph, now);
    layout = flowview::makeLayout (graph, zoom);

    // What shows: a signature of the boxes, the lines and the levels to a tenth of a dB.
    juce::String sig;
    sig.preallocateBytes (graph.nodes.size() * 12 + graph.edges.size() * 6);

    for (auto& n : graph.nodes)
        sig << n.column << ',' << (int) n.kind << ',' << (int) n.bypassed << (int) n.silenced << (int) n.lit << (int) n.muted << ',' << juce::roundToInt (n.levelDb * 10.0f) << ','
            << juce::roundToInt (n.volumeDb * 10.0f) << ',' << juce::roundToInt (n.pan * 100.0f) << ';';

    for (auto& e : graph.edges)
        sig << e.from << '>' << e.to << (int) e.isSend << (int) e.preFader << (int) e.silenced << juce::roundToInt (e.sendDb * 10.0f) << ';';

    for (auto& t : graph.tracks)
        sig << t.name << (int) t.silence << ';';

    const bool changed = sig != shownSignature;
    shownSignature = sig;

    if (changed && fitted && canvas != nullptr && canvas->getWidth() > 0)
        fit();

    return changed;
}

void SignalFlowView::timerCallback()
{
    const bool changed = refreshGraph();
    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    bool moving = engine.isPlaying() || holds.isAnyHeld (now);

    for (auto& n : graph.nodes)
        moving = moving || n.levelDb > gainstaging::silentBelowDb;

    if (moving || changed)
        canvas->repaint();

    const int rate = moving ? 30 : 4;

    if (rate != shownTimerRate)
    {
        shownTimerRate = rate;
        startTimerHz (rate);
    }
}
} // namespace daw
