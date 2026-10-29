#include "RoutingExplainer.h"
#include "SignalFlowView.h"
#include "engine/EffectTools.h"
#include "engine/GainStaging.h"
#include "engine/Routing.h"

namespace daw
{
using namespace routingguide;

/** The routing tree of the song or of an example, or for "Levels add up" bars of levels. */
class RoutingExplainer::RoutingPicture final : public juce::Component, public juce::TooltipClient
{
public:
    struct Bar
    {
        juce::String label, value;
        float db = -100.0f;
        juce::Colour colour;
        bool isSum = false;
    };

    std::function<void (const Track*)> onNodeClicked;

    void setTree (std::vector<Node> newNodes, bool isLive)
    {
        mode = Mode::tree;
        live = isLive;
        nodes = std::move (newNodes);
        bars.clear();
        fileTag = {};
        repaint();
    }

    /** The bounce topics: a file name drawn under the master, as the file the lit boxes end up in (empty: none). */
    void setFileTag (const juce::String& tag)
    {
        fileTag = tag;
        repaint();
    }

    /** For the Mix bus topic: draws the master highlighted with a headroom meter (peak in dB, -100: none yet). */
    void setMaster (bool highlight, float peakDb)
    {
        masterHighlight = highlight;
        masterPeakDb = peakDb;
        repaint();
    }

    /** A drawn explanation instead of the tree: the send knobs and the return's chain (sendHowTo), insert against
        send (insertOrSend), or the two places a send can be taken (preFader). */
    void setDiagram (Topic topic)
    {
        mode = Mode::diagram;
        diagram = topic;
        live = false;
        nodes.clear();
        bars.clear();
        boxes.clear();
        repaint();
    }

    /** The Signal flow topic: the example flow diagram, drawn fitted by the Signal Flow window's own painter, with dots
        running along the lines to the beat while the song plays. */
    void setFlow (signalflow::Graph newGraph, bool isPlaying, double newBeats)
    {
        mode = Mode::flow;
        live = false;
        flow = std::move (newGraph);
        flowPlaying = isPlaying;
        flowBeats = newBeats;
        nodes.clear();
        bars.clear();
        boxes.clear();
        repaint();
    }

    /** The depth topic: the parts with a send placed along a line from close to far, by their send level. */
    void setStage (std::vector<Node> newNodes)
    {
        mode = Mode::stage;
        live = false;
        nodes = std::move (newNodes);
        bars.clear();
        boxes.clear();
        repaint();
    }

    void setBars (std::vector<Bar> newBars, juce::String newCaption)
    {
        mode = Mode::bars;
        bars = std::move (newBars);
        caption = std::move (newCaption);
        nodes.clear();
        repaint();
    }

    juce::String getTooltip() override
    {
        const auto at = getMouseXYRelative();

        if (mode == Mode::flow)
            return "The signal flow of an example song, from left to right: each track's source, its effects and fader, the buses, the sends (dashed) "
                   "and the master. View > Signal Flow... draws your own song, live.";

        for (size_t i = 0; i < boxes.size(); ++i)
            if (boxes[i].contains (at))
                return describe (i);

        return {};
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const bool over = std::any_of (boxes.begin(), boxes.end(), [&e] (auto& b) { return b.contains (e.getPosition()); });
        setMouseCursor (over && live ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! live || onNodeClicked == nullptr)
            return;

        for (size_t i = 0; i < boxes.size(); ++i)
            if (boxes[i].contains (e.getPosition()) && i < nodes.size() && nodes[i].track != nullptr)
                onNodeClicked (nodes[i].track);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        boxes.clear();

        if (mode == Mode::bars)
            paintBars (g);
        else if (mode == Mode::diagram)
            paintDiagram (g);
        else if (mode == Mode::stage)
            paintStage (g);
        else if (mode == Mode::flow)
            paintFlow (g);
        else
            paintTree (g);
    }

private:
    enum class Mode { tree, bars, diagram, stage, flow };

    /** "Kick -> Drums -> Master · -12.0 dB" */
    juce::String describe (size_t index) const
    {
        if (index >= nodes.size())
            return masterHighlight ? "The mix bus (the master): every track and bus added up, then out to your speakers. "
                                     + describeMasterLevel (masterPeakDb)
                                   : juce::String ("The master: every track and bus that plays into it, added up, then out to your speakers");

        juce::String text = nodes[index].name;

        for (int p = nodes[index].parent; p >= 0 && p < (int) nodes.size(); p = nodes[(size_t) p].parent)
            text << juce::String::fromUTF8 (" \xe2\x86\x92 ") << nodes[(size_t) p].name;

        text << juce::String::fromUTF8 (" \xe2\x86\x92 Master");
        const auto& n = nodes[index];

        if (n.levelDb > gainstaging::silentBelowDb)
            text << juce::String::fromUTF8 (" \xc2\xb7 ") << gainstaging::formatDb (n.levelDb);

        if ((n.isBus || n.isReturn) && n.inputDb > gainstaging::silentBelowDb)
            text << ", in: " << gainstaging::formatDb (n.inputDb);

        // The sends of this box, or for a return the boxes that send to it.
        juce::StringArray sends;

        if (n.isReturn)
        {
            for (auto& other : nodes)
                for (auto& link : other.sends)
                    if (link.target == (int) index)
                        sends.add (other.name + " (" + juce::String (link.levelDb, 1) + " dB" + (link.preFader ? ", pre-fader" : "") + ")");

            text << (sends.isEmpty() ? ". A return: nothing sends to it." : ". A return: " + sends.joinIntoString (", ") + " send to it.");
        }
        else
        {
            for (auto& link : n.sends)
                if (link.target >= 0 && link.target < (int) nodes.size())
                    sends.add (nodes[(size_t) link.target].name + " " + juce::String (link.levelDb, 1) + " dB" + (link.preFader ? " (pre-fader)" : ""));

            if (! sends.isEmpty())
                text << ". Sends to " << sends.joinIntoString (", ");
        }

        return text;
    }

    void paintFlow (juce::Graphics& g)
    {
        auto area = getLocalBounds().reduced (14, 10);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("An example song: thicker, brighter lines are louder; dashed lines are sends",
                    area.removeFromTop (16), juce::Justification::centredLeft, true);
        area.removeFromTop (4);
        flowview::paintFitted (g, flow, area.toFloat(), flowPlaying, flowBeats);
    }

    void paintTree (juce::Graphics& g)
    {
        auto area = getLocalBounds().reduced (14, 10);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (live ? "Your song: click a box to select that track" : "An example song", area.removeFromTop (16),
                    juce::Justification::centredLeft, false);
        area.removeFromTop (4);

        if (nodes.empty())
            return;

        const auto points = layOut (nodes);
        float maxX = 0.0f, maxY = 0.0f;

        for (auto& p : points)
        {
            maxX = juce::jmax (maxX, p.x);
            maxY = juce::jmax (maxY, p.y);
        }

        const float columnWidth = (float) area.getWidth() / (maxX + 1.0f);
        const float rowHeight = juce::jmin (36.0f, (float) area.getHeight() / (maxY + 1.0f));
        const float boxHeight = juce::jlimit (14.0f, 28.0f, rowHeight - 4.0f);
        const float trackWidth = juce::jmin (120.0f, columnWidth * 0.62f), busWidth = juce::jmin (150.0f, columnWidth * 0.74f);
        const float yOffset = (float) area.getY() + ((float) area.getHeight() - rowHeight * (maxY + 1.0f)) * 0.5f;

        const auto boxFor = [&] (size_t i)
        {
            const bool master = i >= nodes.size();
            const bool bus = master || nodes[i].isBus || nodes[i].isReturn;
            const float w = master ? juce::jmin (90.0f, columnWidth * 0.62f) : bus ? busWidth : trackWidth;
            const float h = master ? juce::jmin (boxHeight + 10.0f, 40.0f) : nodes[i].isReturn ? juce::jmin (rowHeight - 2.0f, boxHeight + 8.0f)
                          : bus ? boxHeight + 4.0f : boxHeight;
            const float cx = (float) area.getX() + columnWidth * (points[i].x + 0.5f);
            const float cy = yOffset + rowHeight * (points[i].y + 0.5f);
            return juce::Rectangle<float> (w, h).withCentre ({ cx, cy });
        };

        std::vector<juce::Rectangle<float>> rects;

        for (size_t i = 0; i < points.size(); ++i)
            rects.push_back (boxFor (i));

        // The arrows first, so the boxes sit on top: each box to the bus it plays into, or the master.
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const size_t to = nodes[i].parent >= 0 ? (size_t) nodes[i].parent : nodes.size();
            const auto from = rects[i].getRight(), toX = rects[to].getX();
            const float y1 = rects[i].getCentreY(), y2 = rects[to].getCentreY();
            const bool onPath = nodes[i].onSelectedPath && (to == nodes.size() || nodes[to].onSelectedPath);

            juce::Path path;
            path.startNewSubPath (from, y1);
            path.cubicTo (from + (toX - from) * 0.5f, y1, from + (toX - from) * 0.5f, y2, toX - 5.0f, y2);
            g.setColour ((onPath ? theme::accent : theme::gridBar).withAlpha (nodes[i].silent ? 0.35f : 1.0f));
            g.strokePath (path, juce::PathStrokeType (onPath ? 2.4f : 1.4f));

            juce::Path head;
            head.addTriangle (toX - 6.0f, y2 - 3.5f, toX - 6.0f, y2 + 3.5f, toX, y2);
            g.fillPath (head);
        }

        // The sends: dashed curves from a box to its return, labelled with the level.
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            for (auto& link : nodes[i].sends)
            {
                if (link.target < 0 || link.target >= (int) nodes.size())
                    continue;

                const auto from = rects[i].getRight(), toX = rects[(size_t) link.target].getX();
                const float y1 = rects[i].getCentreY(), y2 = rects[(size_t) link.target].getCentreY();
                juce::Path path;
                path.startNewSubPath (from, y1);
                path.cubicTo (from + (toX - from) * 0.5f, y1, from + (toX - from) * 0.5f, y2, toX - 5.0f, y2);
                const auto colour = nodes[(size_t) link.target].colour;
                g.setColour (colour.withAlpha (nodes[i].silent ? 0.35f : 0.95f));
                const float dashes[] { 5.0f, 4.0f };
                juce::Path dashed;
                juce::PathStrokeType (1.6f).createDashedStroke (dashed, path, dashes, 2);
                g.fillPath (dashed);

                juce::Path head;
                head.addTriangle (toX - 6.0f, y2 - 3.5f, toX - 6.0f, y2 + 3.5f, toX, y2);
                g.fillPath (head);

                // The level, on the curve a third of the way along.
                const float t = 0.3f;
                const float lx = from + (toX - from) * t, ly = y1 + (y2 - y1) * (t * t * (3.0f - 2.0f * t));
                const auto text = juce::String (link.preFader ? "PRE " : "") + juce::String (link.levelDb, 0);
                g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
                const auto label = juce::Rectangle<float> ((float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text) + 6.0f, 12.0f).withCentre ({ lx, ly });
                g.setColour (theme::panelLight);
                g.fillRoundedRectangle (label, 3.0f);
                g.setColour (link.preFader ? theme::meterYellow : theme::text);
                g.drawText (text, label, juce::Justification::centred, false);
            }
        }

        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));

        for (size_t i = 0; i < points.size(); ++i)
        {
            const auto r = rects[i];
            boxes.push_back (r.toNearestInt());

            if (i >= nodes.size())
            {
                g.setColour (theme::lane);
                g.fillRoundedRectangle (r, 5.0f);
                g.setColour (masterHighlight ? theme::accent : theme::text);
                g.drawRoundedRectangle (r, 5.0f, masterHighlight ? 2.6f : 1.2f);
                g.setColour (theme::text);
                g.drawText ("Master", r, juce::Justification::centred, false);

                if (masterHighlight)
                    paintHeadroom (g, r);

                if (fileTag.isNotEmpty())
                {
                    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
                    const float tagWidth = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), fileTag) + 18.0f;
                    auto tag = juce::Rectangle<float> (tagWidth, 20.0f).withCentre ({ r.getCentreX(), r.getBottom() + 18.0f });
                    tag = tag.withX (juce::jlimit (4.0f, juce::jmax (4.0f, (float) getWidth() - tagWidth - 4.0f), tag.getX()));
                    g.setColour (theme::accent.withAlpha (0.2f));
                    g.fillRoundedRectangle (tag, 4.0f);
                    g.setColour (theme::accent);
                    g.drawRoundedRectangle (tag, 4.0f, 1.2f);
                    g.setColour (theme::text);
                    g.drawText (fileTag, tag, juce::Justification::centred, false);
                    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
                }

                continue;
            }

            const auto& n = nodes[i];
            const float alpha = n.silent ? 0.4f : 1.0f;

            g.setColour (n.colour.withAlpha (0.30f * alpha));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour ((n.onSelectedPath ? theme::accent : n.colour).withAlpha (0.9f * alpha));
            g.drawRoundedRectangle (r, 4.0f, n.onSelectedPath ? 2.0f : 1.0f);

            // The level: a thin meter along the bottom, and for a bus the sum coming in as a bar on its left edge.
            const float level = juce::Decibels::decibelsToGain (n.levelDb);
            g.setColour ((level > 1.0f ? theme::meterRed : theme::meterGreen).withAlpha (0.9f * alpha));
            g.fillRect (r.getX() + 3.0f, r.getBottom() - 4.0f, (r.getWidth() - 6.0f) * meterProportion (level), 2.0f);

            if (n.isBus || n.isReturn)
            {
                const float in = juce::Decibels::decibelsToGain (n.inputDb);
                const float h = (r.getHeight() - 4.0f) * meterProportion (in);
                g.setColour ((in > 1.0f ? theme::meterRed : theme::meterGreen).withAlpha (0.9f * alpha));
                g.fillRect (r.getX() + 1.0f, r.getBottom() - 2.0f - h, 2.5f, h);
            }

            g.setColour (theme::text.withAlpha (alpha));
            const auto label = n.folded ? n.name + " (" + juce::String (n.numInside) + ")" : n.name;
            auto textArea = r.reduced (n.isBus || n.isReturn ? 7.0f : 5.0f, 1.0f).withTrimmedRight (n.muted || n.soloed ? 12.0f : 0.0f);

            if (n.isReturn)
            {
                // The name above a small RETURN tag.
                g.drawFittedText (label, textArea.withTrimmedBottom (9.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
                g.setColour (n.colour.withAlpha (alpha));
                g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
                g.drawText ("RETURN", textArea.removeFromBottom (11.0f).translated (0.0f, -2.0f), juce::Justification::centred, false);
                g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            }
            else
            {
                g.drawFittedText (label, textArea.toNearestInt(), juce::Justification::centred, 1, 0.8f);
            }

            if (n.muted || n.soloed)
            {
                g.setColour (n.muted ? juce::Colour (0xffe0a030) : juce::Colour (0xff4fc06f));
                g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
                g.drawText (n.muted ? "M" : "S", r.withTrimmedLeft (r.getWidth() - 14.0f), juce::Justification::centred, false);
                g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            }
        }
    }

    //==========================================================================
    // The drawn explanations of the send topics.

    static void drawBox (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, const juce::String& sub, juce::Colour colour, bool strong = false)
    {
        g.setColour (colour.withAlpha (strong ? 0.38f : 0.22f));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (colour.withAlpha (strong ? 1.0f : 0.8f));
        g.drawRoundedRectangle (r, 5.0f, strong ? 1.8f : 1.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));

        if (sub.isEmpty())
        {
            g.drawFittedText (text, r.reduced (4.0f, 1.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
        }
        else
        {
            g.drawFittedText (text, r.reduced (4.0f, 1.0f).withTrimmedBottom (r.getHeight() * 0.38f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (10.5f));
            g.drawFittedText (sub, r.reduced (4.0f, 0.0f).withTrimmedTop (r.getHeight() * 0.55f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
        }
    }

    static void drawArrowBetween (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour colour, bool dashed = false)
    {
        g.setColour (colour);

        if (dashed)
        {
            juce::Path line;
            line.startNewSubPath (a);
            line.lineTo (b);
            const float dashes[] { 4.0f, 3.0f };
            juce::Path dashedPath;
            juce::PathStrokeType (1.6f).createDashedStroke (dashedPath, line, dashes, 2);
            g.fillPath (dashedPath);
            juce::Path head;
            const auto direction = (b - a) / juce::jmax (1.0f, a.getDistanceFrom (b));
            const auto side = juce::Point<float> (-direction.y, direction.x) * 3.5f;
            head.addTriangle (b, b - direction * 6.0f + side, b - direction * 6.0f - side);
            g.fillPath (head);
        }
        else
        {
            g.drawArrow ({ a, b }, 1.6f, 7.0f, 6.0f);
        }
    }

    /** A row of boxes joined by arrows, from `x`; returns the boxes' rectangles. */
    std::vector<juce::Rectangle<float>> drawChain (juce::Graphics& g, float x, float y, float boxWidth, float boxHeight, float gap,
                                                    const std::vector<std::pair<juce::String, juce::String>>& items, juce::Colour colour, int strongIndex = -1)
    {
        std::vector<juce::Rectangle<float>> rects;

        for (size_t i = 0; i < items.size(); ++i)
        {
            const juce::Rectangle<float> r (x + (boxWidth + gap) * (float) i, y, boxWidth, boxHeight);
            drawBox (g, r, items[i].first, items[i].second, colour, (int) i == strongIndex);
            rects.push_back (r);

            if (i > 0)
                drawArrowBetween (g, { rects[i - 1].getRight() + 2.0f, r.getCentreY() }, { r.getX() - 2.0f, r.getCentreY() }, theme::textDim);
        }

        return rects;
    }

    void paintDiagram (juce::Graphics& g)
    {
        auto area = getLocalBounds().reduced (14, 10);
        const auto reverbColour = juce::Colour (0xffff5c8a), trackColour = juce::Colour (0xff4fa3ff);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        const float left = (float) area.getX(), top = (float) area.getY();
        const float areaWidth = (float) area.getWidth();

        auto note = [&g] (const juce::String& text, float x, float y, float w, bool strong = false)
        {
            g.setColour (strong ? theme::text : theme::textDim);
            g.setFont (juce::FontOptions (strong ? 12.5f : 11.5f, strong ? juce::Font::bold : juce::Font::plain));
            g.drawText (text, juce::Rectangle<float> (x, y, w, 16.0f), juce::Justification::centredLeft, true);
        };

        if (diagram == Topic::sendHowTo)
        {
            g.drawText ("Two tracks with a Reverb knob each, and the return they send to", area.removeFromTop (16), juce::Justification::centredLeft, false);
            const float stripY = top + 28.0f, stripW = 92.0f, stripH = 118.0f;
            const std::pair<const char*, float> strips[] { { "Vocal", -12.0f }, { "Guitar", -9.0f } };

            std::vector<juce::Rectangle<float>> stripRects;

            for (int i = 0; i < 2; ++i)
            {
                const juce::Rectangle<float> r (left + 6.0f + (stripW + 18.0f) * (float) i, stripY, stripW, stripH);
                stripRects.push_back (r);
                g.setColour (theme::panel);
                g.fillRoundedRectangle (r, 5.0f);
                g.setColour (trackColour.withAlpha (i == 0 ? 1.0f : 0.7f));
                g.fillRect (r.withHeight (3.0f));
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
                g.drawText (strips[i].first, r.withTrimmedTop (8.0f).withHeight (16.0f), juce::Justification::centred, false);
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (10.0f));
                g.drawText ("pan", r.withTrimmedTop (26.0f).withHeight (12.0f), juce::Justification::centred, false);
                g.drawText ("Reverb", r.withTrimmedTop (60.0f).withHeight (12.0f), juce::Justification::centred, false);

                // The knob: an arc from Off (-60) to +6 dB, with the middle at -12 dB.
                const auto knob = juce::Rectangle<float> (30.0f, 30.0f).withCentre ({ r.getCentreX(), r.getY() + 86.0f });
                const float proportion = std::pow ((strips[i].second + 60.0f) / 66.0f, std::log (0.5f) / std::log (48.0f / 66.0f));
                const float angle = juce::MathConstants<float>::pi * (1.2f + 1.6f * juce::jlimit (0.0f, 1.0f, proportion));
                juce::Path track, value;
                track.addCentredArc (knob.getCentreX(), knob.getCentreY(), 13.0f, 13.0f, 0.0f, juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
                value.addCentredArc (knob.getCentreX(), knob.getCentreY(), 13.0f, 13.0f, 0.0f, juce::MathConstants<float>::pi * 1.2f, angle, true);
                g.setColour (theme::gridBar);
                g.strokePath (track, juce::PathStrokeType (3.0f));
                g.setColour (reverbColour);
                g.strokePath (value, juce::PathStrokeType (3.0f));
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
                g.drawText (juce::String (strips[i].second, 0) + " dB", r.withTrimmedTop (r.getHeight() - 14.0f), juce::Justification::centred, false);
            }

            const float chainX = left + 2.0f * stripW + 70.0f;
            const float boxW = juce::jmin (124.0f, (areaWidth - 2.0f * stripW - 70.0f - 3.0f * 24.0f) / 4.0f);
            const float chainY = stripY + 36.0f;
            const auto rects = drawChain (g, chainX, chainY, boxW, 46.0f, 24.0f,
                                          { { "Sends in", "Vocal, Guitar" }, { "DawIt Reverb", "100% mix" }, { "Fader", "" }, { "Master", "" } }, reverbColour, 1);
            note ("The return Reverb", chainX, chainY - 22.0f, 200.0f, true);

            for (auto& r : stripRects)
                drawArrowBetween (g, { r.getRight() - 2.0f, r.getY() + 86.0f }, { chainX - 4.0f, chainY + 23.0f }, reverbColour, true);

            note ("The return plays into the master. Shape the reverb once, here.", chainX, chainY + 56.0f, areaWidth - chainX + left, false);
            return;
        }

        // The two chains of insertOrSend and preFader.
        const float rowHeight = 142.0f;
        const float boxH = 40.0f, gap = 26.0f;
        const float boxW = juce::jmin (112.0f, (areaWidth - 4.0f * gap - 150.0f) / 4.0f);
        const bool insertDiagram = diagram == Topic::insertOrSend;
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (insertDiagram ? "The same reverb as an insert and as a send" : "Where the send takes its copy", area.removeFromTop (16), juce::Justification::centredLeft, false);

        for (int row = 0; row < 2; ++row)
        {
            const float y = top + 24.0f + rowHeight * (float) row;
            const float chainY = y + 22.0f;

            if (insertDiagram)
            {
                if (row == 0)
                {
                    note ("Insert: the whole sound goes through the reverb", left, y, areaWidth, true);
                    const auto rects = drawChain (g, left, chainY, boxW, boxH, gap, { { "EQ", "" }, { "Compressor", "" }, { "Reverb", "20% mix" }, { "Fader", "" } }, trackColour, 2);
                    note (juce::String::fromUTF8 ("the reverb comes out ") + juce::String (-getWetToDb (20.0f), 1) + " dB under the dry sound",
                             rects[2].getX(), chainY + boxH + 8.0f, 330.0f);
                }
                else
                {
                    note ("Send: a copy goes to a return, and is added to the sound", left, y, areaWidth, true);
                    const auto rects = drawChain (g, left, chainY, boxW, boxH, gap, { { "EQ", "" }, { "Compressor", "" }, { "Fader", "" }, { "Master", "" } }, trackColour);
                    const float tapX = rects[2].getRight() + gap * 0.5f;
                    const juce::Rectangle<float> ret (tapX + 24.0f, chainY + boxH + 14.0f, boxW + 10.0f, 34.0f);
                    drawBox (g, ret, "Reverb", "100% mix, on a return", reverbColour, true);
                    g.setColour (reverbColour);
                    g.drawLine (tapX, chainY + boxH * 0.5f, tapX, ret.getCentreY(), 1.6f);
                    drawArrowBetween (g, { tapX, ret.getCentreY() }, { ret.getX() - 2.0f, ret.getCentreY() }, reverbColour, true);
                    note ("send " + juce::String (getSendDb (20.0f), 1) + " dB: the same balance", ret.getRight() + 10.0f, ret.getCentreY() - 8.0f, 300.0f);
                }
            }
            else
            {
                const bool pre = row == 1;
                note (pre ? "Pre-fader: the send ignores the fader" : "Post-fader: the send follows the fader (the usual choice)", left, y, areaWidth, true);
                const auto rects = drawChain (g, left, chainY, boxW, boxH, gap, { { "Effects", "" }, { "Fader", "-6 dB" }, { "Pan", "" }, { "Master", "" } }, trackColour, 1);
                const float tapX = pre ? rects[0].getRight() + gap * 0.5f : rects[1].getRight() + gap * 0.5f;
                const juce::Rectangle<float> ret (tapX + 24.0f, chainY + boxH + 14.0f, boxW + 10.0f, 34.0f);
                drawBox (g, ret, "Reverb", "return", reverbColour, true);
                g.setColour (reverbColour);
                g.fillEllipse (tapX - 3.0f, chainY + boxH * 0.5f - 3.0f, 6.0f, 6.0f);
                g.drawLine (tapX, chainY + boxH * 0.5f, tapX, ret.getCentreY(), 1.6f);
                drawArrowBetween (g, { tapX, ret.getCentreY() }, { ret.getX() - 2.0f, ret.getCentreY() }, reverbColour, true);
                note (juce::String (pre ? "PRE " : "") + "send -10 dB: the reverb is " + juce::String (pre ? 4 : 10) + " dB under the dry sound",
                         ret.getRight() + 10.0f, ret.getCentreY() - 8.0f, 330.0f);
            }
        }
    }

    static float getWetToDb (float mixPercent)
    {
        return (float) fx::getWetToDryDb (mixPercent);
    }

    static float getSendDb (float mixPercent) { return getSendDbForMix (mixPercent); }

    /** The depth topic: from the listener on the left, parts with little reverb close, parts with a lot far away. */
    void paintStage (juce::Graphics& g)
    {
        auto area = getLocalBounds().reduced (14, 10);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Each part placed by its send to the Reverb: more reverb, further away", area.removeFromTop (16), juce::Justification::centredLeft, false);
        area.removeFromTop (14);

        const float lineY = (float) area.getCentreY() + 8.0f;
        const float x0 = (float) area.getX() + 50.0f, x1 = (float) area.getRight() - 40.0f;
        constexpr float nearDb = -14.0f, farDb = 0.0f;
        auto toX = [&] (float db) { return x0 + (juce::jlimit (nearDb, farDb, db) - nearDb) / (farDb - nearDb) * (x1 - x0); };

        // The listener, and the line the sends place the parts along.
        g.setColour (theme::gridBar);
        g.fillRect (x0 - 20.0f, lineY - 1.0f, x1 - x0 + 40.0f, 2.0f);
        g.setColour (theme::text);
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ (float) area.getX() + 10.0f, lineY }));
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (theme::textDim);
        g.drawText ("you", juce::Rectangle<float> ((float) area.getX() - 6.0f, lineY + 10.0f, 34.0f, 12.0f), juce::Justification::centred, false);
        g.drawText ("close: little reverb", juce::Rectangle<float> (x0 - 20.0f, (float) area.getBottom() - 14.0f, 160.0f, 14.0f), juce::Justification::centredLeft, false);
        g.drawText ("far: a lot of reverb", juce::Rectangle<float> (x1 - 140.0f, (float) area.getBottom() - 14.0f, 160.0f, 14.0f), juce::Justification::centredRight, false);

        int index = 0;

        for (auto& n : nodes)
        {
            if (n.sends.empty())
                continue;

            const float db = n.sends.front().levelDb;
            const float x = toX (db);
            const bool above = index % 2 == 0;
            const float y = lineY + (above ? -50.0f : 50.0f);
            g.setColour (n.colour.withAlpha (0.6f));
            g.drawLine (x, lineY, x, y, 1.2f);
            g.setColour (n.colour);
            g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ x, lineY }));

            const juce::Rectangle<float> label (110.0f, 34.0f);
            const auto r = label.withCentre ({ x, y + (above ? -2.0f : 2.0f) }).withX (juce::jlimit ((float) area.getX(), (float) area.getRight() - 110.0f, x - 55.0f));
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (n.colour.withAlpha (0.25f));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (n.name, r.withTrimmedBottom (16.0f), juce::Justification::centred, true);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (10.5f));
            g.drawText (juce::String (db, 1) + " dB", r.withTrimmedTop (16.0f), juce::Justification::centred, false);
            ++index;
        }
    }

    /** Under the master: a peak bar from -24 to +3 dB with the -6 dB target marked, and the headroom in words. */
    void paintHeadroom (juce::Graphics& g, juce::Rectangle<float> master)
    {
        constexpr float bottomDb = -24.0f, topDb = 3.0f;
        auto area = juce::Rectangle<float> (juce::jmax (master.getWidth(), 120.0f), 10.0f).withCentre ({ master.getCentreX(), master.getBottom() + 18.0f });
        area = area.withRightX (juce::jmin (area.getRight(), (float) getWidth() - 8.0f));
        const auto toX = [&] (float db) { return area.getX() + (juce::jlimit (bottomDb, topDb, db) - bottomDb) / (topDb - bottomDb) * area.getWidth(); };

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (area, 2.0f);
        g.setColour (theme::meterRed.withAlpha (0.25f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (toX (0.0f), area.getY(), area.getRight(), area.getBottom()));

        if (masterPeakDb > -99.0f)
        {
            g.setColour (masterPeakDb > 0.05f ? theme::meterRed : masterPeakDb > mixBusTargetPeakDb ? theme::meterYellow : theme::meterGreen);
            g.fillRoundedRectangle (area.withRightX (juce::jmax (area.getX() + 1.0f, toX (masterPeakDb))), 2.0f);
        }

        g.setColour (theme::text);
        g.fillRect (toX (mixBusTargetPeakDb) - 0.75f, area.getY() - 3.0f, 1.5f, area.getHeight() + 6.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("-6", juce::Rectangle<float> (toX (mixBusTargetPeakDb) - 15.0f, area.getBottom() + 3.0f, 30.0f, 12.0f), juce::Justification::centred, false);
        g.drawText ("0", juce::Rectangle<float> (toX (0.0f) - 15.0f, area.getBottom() + 3.0f, 30.0f, 12.0f), juce::Justification::centred, false);

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.5f));
        g.drawFittedText (describeMasterLevel (masterPeakDb), juce::Rectangle<float> (area.getX() - 40.0f, area.getBottom() + 17.0f, area.getWidth() + 80.0f, 30.0f).toNearestInt()
                              .getIntersection (getLocalBounds()), juce::Justification::centredTop, 2, 0.9f);
    }

    void paintBars (juce::Graphics& g)
    {
        auto area = getLocalBounds().reduced (14, 10);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (caption, area.removeFromTop (16), juce::Justification::centredLeft, false);
        area.removeFromTop (6);

        const int labelWidth = 230, valueWidth = 70;
        auto plot = area.withTrimmedLeft (labelWidth).withTrimmedRight (valueWidth).toFloat();
        constexpr float bottomDb = -30.0f, topDb = 6.0f;
        const auto toX = [&] (float db) { return plot.getX() + (juce::jlimit (bottomDb, topDb, db) - bottomDb) / (topDb - bottomDb) * plot.getWidth(); };

        // 0 dB, and the quiet, healthy, hot and over zones behind the bars.
        g.setColour (theme::meterRed.withAlpha (0.10f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (toX (0.0f), plot.getY(), plot.getRight(), plot.getBottom()));
        g.setColour (theme::gridBar);
        g.fillRect (toX (0.0f), plot.getY(), 1.5f, plot.getHeight());
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));

        for (float db : { -24.0f, -12.0f, 0.0f })
            g.drawText ((db > 0.0f ? "+" : "") + juce::String ((int) db) + " dB", juce::Rectangle<float> (toX (db) - 30.0f, plot.getBottom() - 12.0f, 60.0f, 12.0f),
                        juce::Justification::centred, false);

        const float rowHeight = juce::jmin (26.0f, (plot.getHeight() - 14.0f) / (float) juce::jmax (1, (int) bars.size()));
        float y = plot.getY();

        for (auto& bar : bars)
        {
            const auto row = juce::Rectangle<float> ((float) area.getX(), y, (float) area.getWidth(), rowHeight);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (bar.isSum ? 12.5f : 12.0f, bar.isSum ? juce::Font::bold : juce::Font::plain));
            g.drawText (bar.label, row.withWidth ((float) labelWidth - 8.0f), juce::Justification::centredLeft, true);

            const auto barArea = juce::Rectangle<float> (plot.getX(), y + 4.0f, juce::jmax (1.0f, toX (bar.db) - plot.getX()), rowHeight - 8.0f);
            g.setColour (bar.isSum ? (bar.db > 0.05f ? theme::meterRed : theme::meterGreen) : bar.colour.withAlpha (0.8f));
            g.fillRoundedRectangle (barArea, 3.0f);

            g.setColour (theme::text);
            g.drawText (bar.value, row.withTrimmedLeft (row.getWidth() - (float) valueWidth), juce::Justification::centredRight, false);
            y += rowHeight;
        }
    }

    Mode mode = Mode::tree;
    signalflow::Graph flow;
    bool flowPlaying = false;
    double flowBeats = 0.0;
    bool live = false;
    std::vector<Node> nodes;
    std::vector<Bar> bars;
    Topic diagram = Topic::sendHowTo;
    juce::String fileTag;
    bool masterHighlight = false;
    float masterPeakDb = -100.0f;
    juce::String caption;
    std::vector<juce::Rectangle<int>> boxes;   // where the boxes were drawn: the nodes, then the master
};

//==============================================================================
RoutingExplainer::RoutingExplainer (Engine& e, Selection& s) : engine (e), selection (s)
{
    title.setText ("Buses, sends and routing: where each sound goes", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<RoutingPicture>();
    picture->onNodeClicked = [this] (const Track* track)
    {
        if (track != nullptr && engine.indexOf (track) >= 0)
            selection.select (const_cast<Track*> (track));
    };
    addAndMakeVisible (*picture);

    actionButton.onClick = [this] { doAction(); };
    actionButton.setVisible (false);
    addChildComponent (actionButton);

    explanation.setMultiLine (true, true);
    explanation.setReadOnly (true);
    explanation.setCaretVisible (false);
    explanation.setScrollbarsShown (true);
    explanation.setFont (juce::FontOptions (15.0f));
    explanation.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    explanation.setColour (juce::TextEditor::textColourId, theme::text);
    explanation.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    explanation.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    explanation.setBorder (juce::BorderSize<int> (10, 12, 10, 12));
    addAndMakeVisible (explanation);
    addAndMakeVisible (exportButton);

    list.updateContent();
    showTopic (Topic::what);
    engine.addChangeListener (this);
    selection.addChangeListener (this);
    startTimerHz (15);
    setSize (width, height);
}

RoutingExplainer::~RoutingExplainer()
{
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
    list.setModel (nullptr);
}

juce::String RoutingExplainer::getPrintSource() const
{
    const auto& tracks = engine.getTracks();

    if (tracks.empty())
        return "an example song";

    const int buses = (int) std::count_if (tracks.begin(), tracks.end(), [] (auto& t) { return t->isBus(); });
    const int returns = (int) std::count_if (tracks.begin(), tracks.end(), [] (auto& t) { return t->isReturn(); });
    return "the song's " + juce::String ((int) tracks.size() - buses - returns) + " tracks, " + juce::String (buses) + (buses == 1 ? " bus" : " buses")
           + " and " + juce::String (returns) + (returns == 1 ? " return" : " returns");
}

int RoutingExplainer::getNumPrintTopics() const
{
    return (int) std::size (allTopics);
}

booklet::Topic RoutingExplainer::showPrintTopic (int index)
{
    if (! topicBeforePrinting)
        topicBeforePrinting = shown;

    showTopic (allTopics[juce::jlimit (0, getNumPrintTopics() - 1, index)]);
    return { getTopicName (shown), getTopicSummary (shown), explanation.getText(), { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image RoutingExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void RoutingExplainer::endPrinting()
{
    if (topicBeforePrinting)
        showTopic (*topicBeforePrinting);

    topicBeforePrinting.reset();
}

void RoutingExplainer::showTopic (Topic topic)
{
    shown = topic;
    const auto index = (int) (std::find (std::begin (allTopics), std::end (allTopics), topic) - std::begin (allTopics));

    if (list.getSelectedRow() != index)
        list.selectRow (index);   // comes back through selectedRowsChanged

    shownText = {};
    refresh (true);
    explanation.moveCaretToTop (false);
}

//==============================================================================
void RoutingExplainer::refresh (bool)
{
    const auto& tracks = engine.getTracks();
    const bool hasSong = ! tracks.empty();

    // The song for the topics about it, an example for the others (and for an empty song).
    const bool songTopic = shown == Topic::what || shown == Topic::mixBus || shown == Topic::solo || shown == Topic::yourSong || shown == Topic::inspector;

    if (shown == Topic::levels)
    {
        std::vector<RoutingPicture::Bar> bars;
        auto* bus = selection.getTrack() != nullptr && selection.getTrack()->isBus() && engine.indexOf (selection.getTrack()) >= 0 ? selection.getTrack() : nullptr;
        const auto members = bus != nullptr ? routing::getMembers (*bus, tracks) : std::vector<Track*>();

        if (bus != nullptr && engine.isPlaying() && ! members.empty())
        {
            std::vector<float> levels;

            for (auto* m : members)
            {
                const float db = juce::Decibels::gainToDecibels (juce::jmax (m->peakLeft.load(), m->peakRight.load()), -100.0f);
                levels.push_back (db);
                bars.push_back ({ m->name, gainstaging::formatDb (db), db, m->colour, false });
            }

            const float measured = juce::Decibels::gainToDecibels (bus->busInputLevel.load(), -100.0f);
            bars.push_back ({ bus->name + ": measured sum", gainstaging::formatDb (measured), measured, bus->colour, true });
            bars.push_back ({ "Powers added (a rough guide)", gainstaging::formatDb (sumDb (levels, false)), sumDb (levels, false), bus->colour, true });
            bars.push_back ({ "All peaks lined up (the most)", gainstaging::formatDb (sumDb (levels, true)), sumDb (levels, true), bus->colour, true });
            picture->setBars (std::move (bars), "The tracks going into " + bus->name + ", and what they add up to, right now");
        }
        else
        {
            const auto add = [&bars] (const juce::String& label, float db, juce::Colour colour, bool isSum)
            {
                bars.push_back ({ label, gainstaging::formatDb (db), db, colour, isSum });
            };

            const auto a = juce::Colour (0xff4fa3ff), b = juce::Colour (0xffff8a4f);
            add ("Track 1", -6.0f, a, false);
            add ("Track 2", -6.0f, b, false);
            add ("Bus: different sounds", sumDb ({ -6.0f, -6.0f }, false), a, true);
            add ("Bus: the same sound", sumDb ({ -6.0f, -6.0f }, true), a, true);
            add ("4 drum tracks at -10 dB", -10.0f, b, false);
            add ("Bus: different sounds", sumDb ({ -10.0f, -10.0f, -10.0f, -10.0f }, false), b, true);
            add ("Bus: the peaks line up", sumDb ({ -10.0f, -10.0f, -10.0f, -10.0f }, true), b, true);
            picture->setBars (std::move (bars), "Two examples (select a bus and play the song to see your own tracks add up)");
        }
    }
    else if (hasSong && songTopic)
    {
        picture->setTree (fromSong (tracks, selection.getTrack()), true);
    }
    else if (shown == Topic::sendHowTo || shown == Topic::insertOrSend || shown == Topic::preFader)
    {
        picture->setDiagram (shown);
    }
    else if (shown == Topic::depth)
    {
        picture->setStage (getExample (shown));
    }
    else if (shown == Topic::signalFlow)
    {
        picture->setFlow (signalflow::getExample(), engine.isPlaying(), engine.getPositionBeats());
    }
    else
    {
        picture->setTree (getExample (shown), false);
        picture->setFileTag (getExampleFileTag (shown));
    }

    // The Mix bus topic highlights the master and its headroom: the real peak for the song, an example otherwise.
    if (shown == Topic::mixBus)
        picture->setMaster (true, hasSong ? getMasterPeakDb() : -8.0f);
    else
        picture->setMaster (false, -100.0f);

    updateText();
    updateAction();
}

float RoutingExplainer::getMasterPeakDb() const
{
    return juce::Decibels::gainToDecibels (juce::jmax (engine.masterPeakLeft.load(), engine.masterPeakRight.load()), -100.0f);
}

void RoutingExplainer::updateText()
{
    juce::String text = explain (shown);

    if (shown == Topic::yourSong)
    {
        const auto& tracks = engine.getTracks();

        if (tracks.empty())
        {
            text = "Add some tracks to see their routing here. The picture shows an example song in the meantime.\n\n" + text;
        }
        else
        {
            Levels levels;
            levels.playing = engine.isPlaying();
            levels.masterPeakDb = getMasterPeakDb();

            for (auto& t : tracks)
                if (t->hasInput())
                    levels.busInputDb[t.get()] = juce::Decibels::gainToDecibels (t->busInputLevel.load(), -100.0f);

            const auto advice = analyse (tracks, levels);
            juce::String suggestions;

            for (auto& a : advice)
                suggestions << a.text << "\n\n";

            text = explainSong (tracks) + "\n\n" + (advice.empty() ? juce::String ("Nothing stands out: no bus is empty or holds just one track, and no group of tracks of one kind is waiting for a bus.\n\n") : "What to try:\n\n" + suggestions) + text;
        }
    }

    // The selected track's sends, at the end of the inspector topic and at the top of the song's routing.
    if (shown == Topic::inspector || (shown == Topic::yourSong && ! engine.getTracks().empty()))
    {
        auto* selected = selection.getTrack() != nullptr && engine.indexOf (selection.getTrack()) >= 0 ? selection.getTrack() : nullptr;
        const auto line = describeSends (selected, engine.getTracks());

        if (line.isNotEmpty())
            text = shown == Topic::inspector ? text + "\n\nIn your song: " + line : line + "\n\n" + text;
    }

    if (text != shownText)
    {
        shownText = text;
        explanation.setText (text, false);
    }
}

void RoutingExplainer::updateAction()
{
    shownAction.reset();
    actionBus = nullptr;
    juce::String label, tip;
    const auto& tracks = engine.getTracks();

    if (shown == Topic::yourSong && ! tracks.empty())
    {
        Levels levels;
        const auto advice = analyse (tracks, levels);

        for (auto& a : advice)
        {
            if (a.action == Advice::Action::none)
                continue;

            shownAction = a;

            if (a.action == Advice::Action::createBus || a.action == Advice::Action::moveIntoBus)
            {
                juce::StringArray names;

                for (auto* t : a.tracks)
                    names.add (t->name);

                const auto trackList = names.size() <= 1 ? names.joinIntoString ("") : names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1];

                if (a.action == Advice::Action::moveIntoBus)
                {
                    label = "Move " + trackList + " into " + a.busName;
                    tip = "Move these tracks into the bus " + a.busName + ": they keep their sounds and effects, and share its fader and effects";
                }
                else
                {
                    label = "Create a " + (a.busName.startsWith ("Bus ") ? juce::String ("bus") : a.busName + " bus") + " from " + trackList;
                    tip = "Put these tracks in a new bus: they keep their sounds and effects, and you get one fader and one set of effects for the group";
                }
            }
            else if (a.action == Advice::Action::shareEffect)
            {
                const auto kind = a.effect == "DawIt Delay" ? juce::String ("delay") : juce::String ("reverb");
                label = a.bus != nullptr ? "Send them to " + a.bus->name : "Share one " + kind + " on a return";
                tip = a.bus != nullptr ? "Turn on a send from each of these tracks to " + a.bus->name + " at the level that keeps its amount of " + kind
                                         + ", and switch off (not delete) their own " + kind + "s"
                                       : "Make a return with the first track's " + kind + " at 100% wet, send each track to it at the level that keeps its amount of "
                                         + kind + ", and switch off (not delete) their own " + kind + "s";
            }
            else if (a.action == Advice::Action::addReturnEffect)
            {
                label = "Put " + a.effect + " on " + a.bus->name;
                tip = "Add " + a.effect + " to the return at its Return starting point (100% mix), and open its window";
            }
            else if (a.action == Advice::Action::moveEffect)
            {
                label = a.buttonText;
                tip = "Move the effect in the track's chain. Nothing is deleted or switched off, and you can move it back in the device bar";
            }
            else if (a.action == Advice::Action::makeWet)
            {
                label = "Set " + a.effect + " on " + a.bus->name + " to 100% wet";
                tip = "Turn the effect's Mix to 100%, so the return carries only the effect and each send sets how much a track gets";
            }
            else
            {
                actionBus = a.tracks.front();
                label = "Put DawIt Compressor on " + actionBus->name;
                tip = "Add DawIt Compressor to the bus with the Drum bus starting point, and open its window";
            }

            break;
        }
    }
    else if (shown == Topic::signalFlow)
    {
        Advice a;
        shownAction = a;
        label = "Show my song's signal flow";
        tip = "Open the Signal Flow window: your whole song as a live flow diagram. It only shows the song";
    }
    else if (shown == Topic::glue)
    {
        auto* bus = selection.getTrack();

        if (bus != nullptr && bus->isBus() && engine.indexOf (bus) >= 0 && routing::getMembers (*bus, tracks).size() >= 2
            && std::none_of (bus->plugins.begin(), bus->plugins.end(), [] (auto& p) { return p->instance->getName() == "DawIt Compressor"; }))
        {
            actionBus = bus;
            Advice a;
            a.action = Advice::Action::addGlue;
            a.tracks = { bus };
            shownAction = a;
            label = "Put DawIt Compressor on " + bus->name;
            tip = "Add DawIt Compressor to the bus with the Drum bus starting point, and open its window";
        }
    }

    const bool show = shownAction.has_value();

    if (actionButton.getButtonText() != label)
        actionButton.setButtonText (label);

    actionButton.setTooltip (tip);

    if (actionButton.isVisible() != show)
    {
        actionButton.setVisible (show);
        resized();
    }
    else if (show)
    {
        resized();   // the label may have changed its width
    }
}

void RoutingExplainer::doAction()
{
    if (! shownAction)
        return;

    const auto action = *shownAction;

    if (shown == Topic::signalFlow)
    {
        if (onShowSignalFlow != nullptr)
            onShowSignalFlow();

        return;
    }

    if (action.action == Advice::Action::moveEffect && onMoveEffect != nullptr && ! action.tracks.empty() && engine.indexOf (action.tracks.front()) >= 0)
    {
        auto& track = *action.tracks.front();

        if (std::any_of (track.plugins.begin(), track.plugins.end(), [&action] (auto& p) { return p.get() == action.slot; }))
            onMoveEffect (track, action.slot, action.toIndex);

        return;
    }

    if (action.action == Advice::Action::createBus && onCreateBus != nullptr)
        onCreateBus (action.tracks, action.busName, action.bus);
    else if (action.action == Advice::Action::moveIntoBus && onMoveIntoBus != nullptr && action.bus != nullptr)
        onMoveIntoBus (action.tracks, *action.bus);
    else if (action.action == Advice::Action::addGlue && onAddGlue != nullptr && actionBus != nullptr && engine.indexOf (actionBus) >= 0)
        onAddGlue (*actionBus);
    else if (action.action == Advice::Action::shareEffect && onShareEffect != nullptr
             && std::all_of (action.tracks.begin(), action.tracks.end(), [this] (Track* t) { return engine.indexOf (t) >= 0; }))
        onShareEffect (action.tracks, action.effect, action.bus != nullptr && engine.indexOf (action.bus) >= 0 ? action.bus : nullptr);
    else if (action.action == Advice::Action::addReturnEffect && onAddReturnEffect != nullptr && action.bus != nullptr && engine.indexOf (action.bus) >= 0)
        onAddReturnEffect (*action.bus, action.effect);
    else if (action.action == Advice::Action::makeWet && onMakeWet != nullptr && action.bus != nullptr && engine.indexOf (action.bus) >= 0)
        onMakeWet (*action.bus, action.effect);
}

//==============================================================================
int RoutingExplainer::getNumRows()
{
    return (int) std::size (allTopics);
}

void RoutingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void RoutingExplainer::selectedRowsChanged (int row)
{
    if (row >= 0 && row < getNumRows() && allTopics[row] != shown)
        showTopic (allTopics[row]);
}

void RoutingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void RoutingExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    picture->setBounds (r.removeFromTop (320));
    r.removeFromTop (12);

    if (actionButton.isVisible())
    {
        const auto font = juce::Font (juce::FontOptions (14.0f));
        actionButton.setBounds (r.removeFromTop (28).removeFromLeft (juce::jmin (r.getWidth(), juce::GlyphArrangement::getStringWidthInt (font, actionButton.getButtonText()) + 36)));
        r.removeFromTop (10);
    }

    explanation.setBounds (r);
}
} // namespace daw
