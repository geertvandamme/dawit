#include "ControllerLane.h"
#include "PianoRoll.h"
#include "TimeSignatureUi.h"
#include "engine/MidiGuide.h"

namespace daw
{
namespace
{
    constexpr float pad = 6.0f;
    constexpr int pointHitX = 6, pointHitY = 8;

    /** The controller events copied with Cmd+C (kept by the app, not the song). */
    std::vector<ControllerEvent>& getClipboard()
    {
        static std::vector<ControllerEvent> clipboard;
        return clipboard;
    }

    /** The lanes of the + Lane menu, in the order it shows them. */
    const int commonLanes[] = { ControllerLaneArea::velocityLane, controllers::sustainPedal, controllers::pitchBend, controllers::modWheel,
                                controllers::expression, controllers::volume, controllers::pan, controllers::channelPressure };

    /** Where a lane sits among the lanes a clip adds by itself: pedal, bend, mod wheel, expression, volume, pan, pressure, then by number. */
    int getOrder (int lane)
    {
        for (int i = 0; i < (int) std::size (commonLanes); ++i)
            if (commonLanes[i] == lane)
                return i;

        return 100 + lane;
    }

    bool hasEvent (const std::vector<ControllerEvent>& list, const ControllerEvent& e)
    {
        return std::find (list.begin(), list.end(), e) != list.end();
    }

    juce::String formatNumber (double value)
    {
        return juce::String (value, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
    }

    int getMaxOf (int lane) { return lane == ControllerLaneArea::velocityLane ? 127 : controllers::getMaxValue (lane); }
}

//==============================================================================
LaneDivider::LaneDivider()
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTooltip ("Drag up or down to share the height between the notes and the lanes.");
}

void LaneDivider::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (theme::gridBar);
    g.fillRoundedRectangle (getLocalBounds().toFloat().withSizeKeepingCentre (36.0f, 2.0f), 1.0f);
}

void ControllerLaneArea::reloadLanesFromSettings()
{
    auto* settings = engine.properties.getUserSettings();
    wantedLanesHeight = settings->getIntValue ("view.pianoRollLanesHeight", 128);

    remembered.clear();

    if (settings->containsKey ("view.pianoRollLanes"))
    {
        for (auto& token : juce::StringArray::fromTokens (settings->getValue ("view.pianoRollLanes"), ",", {}))
        {
            const int lane = token.getIntValue();

            if ((lane == velocityLane || controllers::isValidKind (lane)) && std::find (remembered.begin(), remembered.end(), lane) == remembered.end())
                remembered.push_back (lane);
        }
    }
    else
        remembered.push_back (velocityLane);
    rebuildLanes();
}

//==============================================================================
ControllerLaneArea::ControllerLaneArea (Engine& e) : engine (e)
{
    setWantsKeyboardFocus (true);

    reloadLanesFromSettings();

    addButton.setTooltip ("Add a lane under the notes: velocity, sustain pedal, pitch bend, mod wheel, expression, volume or any controller.");
    addButton.setWantsKeyboardFocus (false);
    addButton.onClick = [this] { showAddMenu(); };
    addAndMakeVisible (addButton);

    numberBox.setInputRestrictions (5, "0123456789");
    numberBox.setJustification (juce::Justification::centred);
    addChildComponent (numberBox);
    rebuildLanes();
}

void ControllerLaneArea::rebuildLanes()
{
    lanes = remembered;

    for (int lane : extras)
        if (std::find (lanes.begin(), lanes.end(), lane) == lanes.end())
            lanes.push_back (lane);
}

void ControllerLaneArea::saveLanes() const
{
    juce::StringArray tokens;

    for (int lane : remembered)
        tokens.add (juce::String (lane));

    engine.properties.getUserSettings()->setValue ("view.pianoRollLanes", tokens.joinIntoString (","));
}

void ControllerLaneArea::setClip (Track* t, MidiClip* c)
{
    track = t;
    clip = c;
    extras.clear();
    hidden.clear();
    selectedPoints.clear();
    mode = Mode::none;
    scrollY = 0;
    activeLane = velocityLane;
    numberBox.setVisible (false);
    refresh();
}

bool ControllerLaneArea::refresh()
{
    const auto before = lanes;

    if (clip != nullptr)
    {
        auto used = controllers::getKindsUsed (clip->controllers);
        std::sort (used.begin(), used.end(), [] (int a, int b) { return getOrder (a) < getOrder (b); });

        for (int kind : used)
            if (std::find (remembered.begin(), remembered.end(), kind) == remembered.end()
                && std::find (hidden.begin(), hidden.end(), kind) == hidden.end()
                && std::find (extras.begin(), extras.end(), kind) == extras.end())
                extras.push_back (kind);
    }

    rebuildLanes();
    scrollY = juce::jlimit (0, getMaxScrollY(), scrollY);
    repaint();
    return lanes != before;
}

void ControllerLaneArea::setView (double ppb, int x, double snap)
{
    if (juce::approximatelyEqual (ppb, pixelsPerBeat) && x == scrollX && juce::approximatelyEqual (snap, snapBeats))
        return;

    pixelsPerBeat = ppb;
    scrollX = x;
    snapBeats = snap;
    numberBox.setVisible (false);
    repaint();
}

void ControllerLaneArea::setPlayhead (double beat)
{
    const auto toX = [this] (double b) { return b < 0.0 ? -100 : (int) beatToX (b); };
    const int oldX = toX (playhead), newX = toX (beat);

    if (oldX == newX)
        return;

    playhead = beat;
    repaint (oldX - 2, 0, 6, getHeight());
    repaint (newX - 2, 0, 6, getHeight());
}

int ControllerLaneArea::getWantedLanesHeight() const { return wantedLanesHeight; }

void ControllerLaneArea::setWantedLanesHeight (int h)
{
    wantedLanesHeight = juce::jlimit (48, 2000, h);
    engine.properties.getUserSettings()->setValue ("view.pianoRollLanesHeight", wantedLanesHeight);
}

int ControllerLaneArea::getPreferredHeight (int maxHeight) const
{
    if (lanes.empty())
        return buttonRowHeight;

    const int total = getMaxLanesHeight();
    const int shown = juce::jlimit (juce::jmin (48, total), total, wantedLanesHeight);
    return juce::jmax (buttonRowHeight, juce::jmin (maxHeight, buttonRowHeight + shown));
}

int ControllerLaneArea::getMaxScrollY() const
{
    return juce::jmax (0, getMaxLanesHeight() - (getHeight() - buttonRowHeight));
}

void ControllerLaneArea::resized()
{
    addButton.setBounds (2, getHeight() - buttonRowHeight + 1, nameWidth - 4, buttonRowHeight - 2);
    scrollY = juce::jlimit (0, getMaxScrollY(), scrollY);
}

//==============================================================================
juce::String ControllerLaneArea::getShortName (int lane)
{
    switch (lane)
    {
        case velocityLane:             return "Velocity";
        case controllers::sustainPedal: return "Pedal";
        case controllers::pitchBend:   return "Bend";
        case controllers::modWheel:    return "Mod";
        case controllers::expression:  return "Expr";
        case controllers::volume:      return "Vol";
        case controllers::pan:         return "Pan";
        case controllers::channelPressure: return "Press";
        default:                       return "CC " + juce::String (lane);
    }
}

juce::String ControllerLaneArea::getLongName (int lane)
{
    if (lane == velocityLane)
        return "Velocity";

    if (lane > controllers::pan && lane != controllers::expression && lane != controllers::sustainPedal && lane <= controllers::maxController)
    {
        for (auto& c : midiguide::getCommonControllers())
            if (c.number == lane)
                return "Controller " + juce::String (lane) + " (" + c.name + ")";
    }

    return controllers::getName (lane);
}

juce::String ControllerLaneArea::getLaneTooltip (int lane)
{
    switch (lane)
    {
        case velocityLane:
            return "Velocity: how hard each note is played, 1 to 127. Drag a bar up or down; drag across empty space to draw a line over several notes.";
        case controllers::sustainPedal:
            return "Sustain pedal (controller 64): while it's down, notes ring on after their keys come up. Drag across the lane to draw a press.";
        case controllers::pitchBend:
            return "Pitch bend: slides the pitch, 2 semitones up or down on DawIt's instruments (a plugin may use another range). Click to add a point, drag to draw.";
        case controllers::modWheel:
            return "Mod wheel (controller 1): often vibrato, but it's up to the instrument. DawIt's own instruments don't use it yet.";
        case controllers::expression:
            return "Expression (controller 11): a level inside the volume, for swells and fades within a phrase. 127 is full, 64 is 12 dB down.";
        case controllers::volume:
            return "Volume (controller 7): the part's level, 0 to 127. Set it once to balance the part; draw swells with expression.";
        case controllers::pan:
            return "Pan (controller 10): left (0), middle (64) or right (127), if the instrument uses it.";
        case controllers::channelPressure:
            return "Channel pressure (aftertouch): how hard keys are pressed while held, 0 to 127. Only some keyboards send it, and DawIt's own instruments don't use it yet.";
        default:
            return "Controller " + juce::String (lane) + ": what it does depends on the instrument; its manual lists the numbers it uses.";
    }
}

int ControllerLaneArea::getGuideTopic (int lane)
{
    using midiguide::Topic;

    switch (lane)
    {
        case velocityLane:              return (int) Topic::velocity;
        case controllers::sustainPedal: return (int) Topic::sustainPedal;
        case controllers::pitchBend:    return (int) Topic::pitchBend;
        case controllers::modWheel:
        case controllers::expression:
        case controllers::volume:       return (int) Topic::modExpressionVolume;
        default:                        return (int) Topic::controllers;
    }
}

juce::String ControllerLaneArea::describeValue (int lane, int value)
{
    if (lane == controllers::pitchBend)
    {
        const double semitones = midiguide::getBendSemitones (value);
        const auto text = formatNumber (semitones);
        return (semitones > 0.0 ? "+" : "") + text + (text == "1" || text == "-1" ? " semitone" : " semitones") + " (" + juce::String (value) + ")";
    }

    if (lane == controllers::sustainPedal)
        return juce::String (value >= 64 ? "down" : "up") + " (" + juce::String (value) + ")";

    return juce::String (value);
}

juce::String ControllerLaneArea::describeBeat (double beat) const
{
    const double songBeat = (clip != nullptr ? clip->startBeat : 0.0) + beat;
    const double bar = engine.getBarLength();
    const int barNumber = (int) std::floor (songBeat / bar + 1.0e-9);
    const int inBar = (int) std::floor (songBeat - barNumber * bar + 1.0e-9);
    return "Bar " + juce::String (barNumber + 1) + " " + juce::String::fromUTF8 ("\xc2\xb7") + " beat " + juce::String (inBar + 1);
}

//==============================================================================
juce::Colour ControllerLaneArea::getBaseColour() const
{
    return track != nullptr ? track->colour : theme::accent;
}

juce::Rectangle<float> ControllerLaneArea::getLaneBounds (int index) const
{
    return { (float) nameWidth, (float) (index * laneHeight - scrollY), (float) juce::jmax (0, getWidth() - nameWidth), (float) laneHeight };
}

int ControllerLaneArea::getLaneAt (float y) const
{
    if (y < 0.0f || y >= (float) (getHeight() - buttonRowHeight))
        return -1;

    const int index = (int) std::floor ((y + (float) scrollY) / (float) laneHeight);
    return juce::isPositiveAndBelow (index, (int) lanes.size()) ? index : -1;
}

double ControllerLaneArea::snapBeat (double beat, bool free) const
{
    beat = juce::jmax (0.0, beat);
    return free ? beat : juce::jmax (0.0, std::round (beat / snapBeats) * snapBeats);
}

float ControllerLaneArea::valueToY (int lane, int value, juce::Rectangle<float> bounds)
{
    const float inner = bounds.getHeight() - 2.0f * pad;
    return bounds.getBottom() - pad - inner * (float) value / (float) getMaxOf (lane);
}

int ControllerLaneArea::yToValue (int lane, float y, juce::Rectangle<float> bounds, bool free)
{
    const float inner = bounds.getHeight() - 2.0f * pad;
    const int maxValue = getMaxOf (lane);
    int value = juce::jlimit (lane == velocityLane ? 1 : 0, maxValue, juce::roundToInt ((bounds.getBottom() - pad - y) / inner * (float) maxValue));

    // A bend catches the centre and the semitone lines.
    if (lane == controllers::pitchBend && ! free)
        for (int target : { controllers::bendCentre, controllers::bendCentre + 4096, controllers::bendCentre - 4096 })
            if (std::abs (valueToY (lane, target, bounds) - y) <= 4.0f)
                value = target;

    return value;
}

juce::Rectangle<float> ControllerLaneArea::getBarBounds (const MidiNote& n, juce::Rectangle<float> bounds) const
{
    const int v = juce::jlimit (1, 127, juce::roundToInt (n.velocity * 127.0f));
    const float y = valueToY (velocityLane, v, bounds);
    return { beatToX (n.start) - 1.0f, y, 5.0f, bounds.getBottom() - pad - y };
}

int ControllerLaneArea::findBar (juce::Point<float> p, juce::Rectangle<float> bounds) const
{
    if (clip == nullptr)
        return -1;

    int best = -1;
    float bestDistance = 1.0e9f;

    for (int i = 0; i < (int) clip->notes.size(); ++i)
    {
        const auto r = getBarBounds (clip->notes[(size_t) i], bounds).expanded (3.0f, 3.0f);

        if (r.contains (p) && std::abs (r.getCentreX() - p.x) < bestDistance)
        {
            best = i;
            bestDistance = std::abs (r.getCentreX() - p.x);
        }
    }

    return best;
}

int ControllerLaneArea::findPoint (int lane, juce::Point<float> p, juce::Rectangle<float> bounds) const
{
    if (clip == nullptr)
        return -1;

    const auto events = controllers::getLane (clip->controllers, lane);

    for (int i = (int) events.size(); --i >= 0;)
        if (std::abs (beatToX (events[(size_t) i].beat) - p.x) <= (float) pointHitX
            && std::abs (valueToY (lane, events[(size_t) i].value, bounds) - p.y) <= (float) pointHitY)
            return i;

    return -1;
}

//==============================================================================
juce::String ControllerLaneArea::getTooltip()
{
    if (clip == nullptr || mode != Mode::none)
        return {};

    const auto p = getMouseXYRelative().toFloat();
    const int index = getLaneAt (p.y);

    if (index < 0)
        return {};

    const int lane = lanes[(size_t) index];

    if (p.x < (float) nameWidth)
        return getLaneTooltip (lane);

    const auto bounds = getLaneBounds (index);
    const auto dot = juce::String::fromUTF8 (" \xc2\xb7 ");

    if (lane == velocityLane)
    {
        const int bar = findBar (p, bounds);

        if (bar < 0)
            return getLaneTooltip (lane);

        const auto& n = clip->notes[(size_t) bar];
        return describeBeat (n.start) + dot + midiguide::getNoteName (n.pitch) + dot + "velocity " + juce::String (juce::roundToInt (n.velocity * 127.0f));
    }

    double beat = juce::jmax (0.0, xToBeat (p.x));
    int value = controllers::getValueAt (clip->controllers, lane, beat);

    if (const int hit = findPoint (lane, p, bounds); hit >= 0)
    {
        const auto e = controllers::getLane (clip->controllers, lane)[(size_t) hit];
        beat = e.beat;
        value = e.value;
    }

    return describeBeat (beat) + dot + getLongName (lane) + dot + describeValue (lane, value);
}

//==============================================================================
void ControllerLaneArea::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    const int lanesBottom = getHeight() - buttonRowHeight;

    if (clip != nullptr)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (0, 0, getWidth(), lanesBottom);

        for (int i = 0; i < (int) lanes.size(); ++i)
        {
            const auto bounds = getLaneBounds (i);

            if (bounds.getBottom() < 0.0f || bounds.getY() > (float) lanesBottom)
                continue;

            paintLane (g, i, bounds);
        }

        // Playhead
        if (playhead >= 0.0)
        {
            g.setColour (theme::playhead);
            g.fillRect (beatToX (playhead), 0.0f, 1.5f, (float) lanesBottom);
        }
    }

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (0, lanesBottom, getWidth(), 1);
}

void ControllerLaneArea::paintLane (juce::Graphics& g, int index, juce::Rectangle<float> bounds)
{
    const int lane = lanes[(size_t) index];

    // Name cell at the left.
    {
        auto cell = juce::Rectangle<float> (0.0f, bounds.getY(), (float) nameWidth, bounds.getHeight());
        g.setColour (theme::panel);
        g.fillRect (cell);

        if (lane == activeLane)
        {
            g.setColour (theme::accent);
            g.fillRect (cell.removeFromLeft (3.0f));
        }

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawFittedText (getShortName (lane), cell.toNearestInt().reduced (6, 0), juce::Justification::centred, 1, 0.6f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbe"), cell.toNearestInt().removeFromBottom (14).reduced (4, 0), juce::Justification::centredRight);
    }

    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (bounds.toNearestInt());
    g.setColour (index % 2 == 0 ? theme::lane : theme::laneAlt);
    g.fillRect (bounds);

    // Time grid, as the notes have it.
    double step = snapBeats;
    while (step * pixelsPerBeat < 8.0)
        step *= 2.0;

    const auto sig = engine.getTimeSignature();
    const double firstBeat = std::floor (xToBeat (bounds.getX()) / step) * step;

    for (double b = juce::jmax (0.0, firstBeat); beatToX (b) <= bounds.getRight(); b += step)
    {
        const auto kind = timesigui::getLineKind (sig, clip->startBeat + b);
        g.setColour (kind == timesigui::Line::bar ? theme::gridBar : kind == timesigui::Line::beat ? theme::grid.brighter (0.15f) : theme::grid.withAlpha (0.5f));
        g.fillRect (beatToX (b), bounds.getY(), 1.0f, bounds.getHeight());
    }

    if (lane == controllers::pitchBend)
    {
        // Centre and semitone lines.
        const struct { int value; const char* label; } marks[] = { { 16383, "+2" }, { 12288, "+1" }, { 8192, "0" }, { 4096, "-1" }, { 0, "-2" } };
        g.setFont (juce::FontOptions (10.0f));

        for (auto& m : marks)
        {
            const float y = valueToY (lane, m.value, bounds);
            g.setColour (m.value == controllers::bendCentre ? theme::gridBar.brighter (0.3f) : theme::grid);
            g.fillRect (bounds.getX(), y, bounds.getWidth(), 1.0f);
            g.setColour (theme::textDim);
            g.drawText (m.label, juce::Rectangle<float> (bounds.getX() + 3.0f, juce::jlimit (bounds.getY(), bounds.getBottom() - 11.0f, y - 5.5f), 24.0f, 11.0f), juce::Justification::centredLeft);
        }
    }

    // Past the end of the clip.
    const float endX = beatToX (clip->lengthBeats);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (endX, bounds.getY(), juce::jmax (0.0f, bounds.getRight() - endX), bounds.getHeight());
    g.setColour (theme::accent.withAlpha (0.7f));
    g.fillRect (endX, bounds.getY(), 1.5f, bounds.getHeight());

    if (lane == velocityLane)
        paintVelocity (g, bounds);
    else
        paintSteps (g, lane, bounds);

    if (mode == Mode::rectangle && dragLane == lane)
    {
        g.setColour (theme::accent.withAlpha (0.15f));
        g.fillRect (rectangle);
        g.setColour (theme::accent);
        g.drawRect (rectangle, 1.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (bounds.getX() - (float) nameWidth, bounds.getBottom() - 1.0f, (float) getWidth(), 1.0f);
}

void ControllerLaneArea::paintVelocity (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const auto base = getBaseColour();
    const auto selected = getSelectedNotes != nullptr ? getSelectedNotes() : juce::SortedSet<int>();

    for (int i = 0; i < (int) clip->notes.size(); ++i)
    {
        const auto& n = clip->notes[(size_t) i];
        const auto r = getBarBounds (n, bounds);

        if (r.getRight() < bounds.getX() || r.getX() > bounds.getRight())
            continue;

        g.setColour (PianoRollGrid::velocityColour (base, n.velocity).withAlpha (n.start < clip->lengthBeats ? 1.0f : 0.35f));
        g.fillRect (r);
        g.setColour (selected.contains (i) ? juce::Colours::white : juce::Colours::black.withAlpha (0.4f));
        g.drawRect (r, selected.contains (i) ? 1.5f : 1.0f);
        g.setColour (selected.contains (i) ? juce::Colours::white : base);
        g.fillRect (r.getX() - 1.0f, r.getY() - 1.0f, r.getWidth() + 2.0f, 2.0f);
    }
}

void ControllerLaneArea::paintSteps (juce::Graphics& g, int lane, juce::Rectangle<float> bounds)
{
    const auto base = getBaseColour();
    const auto events = controllers::getLane (clip->controllers, lane);
    const bool pedal = lane == controllers::sustainPedal;
    const float baseline = lane == controllers::pitchBend ? valueToY (lane, controllers::bendCentre, bounds) : bounds.getBottom() - pad;
    const float right = bounds.getRight();

    // A segment is the stretch where one value holds. Before the first point the lane sits at its rest value, dimmed.
    struct Segment { double beat; int value; bool rest; };
    std::vector<Segment> segments;
    segments.push_back ({ 0.0, controllers::getRestValue (lane), true });

    for (auto& e : events)
        segments.push_back ({ e.beat, e.value, false });

    for (size_t i = 0; i < segments.size(); ++i)
    {
        const float x0 = juce::jmax (bounds.getX(), beatToX (segments[i].beat));
        const float x1 = i + 1 < segments.size() ? juce::jmin (right, beatToX (segments[i + 1].beat)) : right;

        if (x1 <= x0 || x1 < bounds.getX() || x0 > right)
            continue;

        const float y = valueToY (lane, segments[i].value, bounds);
        const float dim = segments[i].rest ? 0.35f : 1.0f;

        if (pedal)
        {
            const bool down = segments[i].value >= 64;

            if (down)
            {
                g.setColour (base.withAlpha (0.85f * dim));
                g.fillRect (x0, bounds.getY() + pad, x1 - x0, bounds.getHeight() - 2.0f * pad);
            }
            else if (segments[i].value > 0)
            {
                g.setColour (base.withAlpha (0.4f * dim));
                g.fillRect (x0, y, x1 - x0, baseline - y);
            }

            continue;
        }

        g.setColour (base.withAlpha (0.18f * dim));
        g.fillRect (x0, juce::jmin (y, baseline), x1 - x0, std::abs (baseline - y));
        g.setColour (base.brighter (0.2f).withAlpha (dim));
        g.fillRect (x0, y - 1.0f, x1 - x0, 2.0f);

        if (i + 1 < segments.size())
        {
            const float yNext = valueToY (lane, segments[i + 1].value, bounds);
            g.setColour (base.brighter (0.2f).withAlpha (segments[i + 1].rest ? 0.35f : 1.0f));
            g.fillRect (x1 - 1.0f, juce::jmin (y, yNext), 2.0f, std::abs (yNext - y) + 1.0f);
        }
    }

    // The points.
    for (auto& e : events)
    {
        const float x = beatToX (e.beat);

        if (x < bounds.getX() - 6.0f || x > right + 6.0f)
            continue;

        const bool selected = hasEvent (selectedPoints, e);
        const auto r = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ x, valueToY (lane, e.value, bounds) });
        g.setColour (selected ? juce::Colours::white : base.brighter (0.5f));
        g.fillRect (r);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (r, 1.0f);
    }
}

//==============================================================================
void ControllerLaneArea::applyEvents (std::vector<ControllerEvent> events)
{
    if (clip != nullptr && engine.setControllerEvents (*clip, std::move (events)))
    {
        if (refresh() && getParentComponent() != nullptr)
            getParentComponent()->resized();

        repaint();

        if (onEdited)
            onEdited();
    }
}

void ControllerLaneArea::selectAllInLane (int lane)
{
    if (clip == nullptr)
        return;

    activeLane = lane;

    if (lane == velocityLane)
    {
        juce::SortedSet<int> all;

        for (int i = 0; i < (int) clip->notes.size(); ++i)
            all.add (i);

        if (setSelectedNotes)
            setSelectedNotes (all);
    }
    else
        selectedPoints = controllers::getLane (clip->controllers, lane);

    repaint();
}

void ControllerLaneArea::removeSelected()
{
    if (clip == nullptr || selectedPoints.empty())
        return;

    auto events = clip->controllers;
    events.erase (std::remove_if (events.begin(), events.end(), [this] (const ControllerEvent& e) { return hasEvent (selectedPoints, e); }), events.end());
    selectedPoints.clear();
    applyEvents (std::move (events));
}

void ControllerLaneArea::copySelected()
{
    if (selectedPoints.empty())
        return;

    auto copy = selectedPoints;
    controllers::tidy (copy);
    const double first = copy.front().beat;

    for (auto& e : copy)
        e.beat -= first;

    getClipboard() = copy;
}

void ControllerLaneArea::paste()
{
    auto& clipboard = getClipboard();

    if (clip == nullptr || clipboard.empty())
        return;

    const double at = juce::jmax (0.0, engine.getPositionBeats() - clip->startBeat);
    auto events = clip->controllers;
    std::vector<ControllerEvent> pasted;

    for (auto e : clipboard)
    {
        e.beat += at;
        pasted.push_back (e);
        events.push_back (e);
    }

    addLane (clipboard.front().kind);
    selectedPoints = pasted;
    activeLane = pasted.front().kind;
    applyEvents (std::move (events));
}

void ControllerLaneArea::duplicateSelected()
{
    if (clip == nullptr || selectedPoints.empty())
        return;

    auto sorted = selectedPoints;
    controllers::tidy (sorted);
    const double offset = sorted.back().beat - sorted.front().beat + snapBeats;
    auto events = clip->controllers;

    for (auto& e : sorted)
    {
        e.beat += offset;
        events.push_back (e);
    }

    selectedPoints = sorted;
    applyEvents (std::move (events));
}

bool ControllerLaneArea::keyPressed (const juce::KeyPress& key)
{
    if (clip == nullptr)
        return false;

    const auto cmd = juce::ModifierKeys::commandModifier;

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        removeSelected();
        return true;
    }

    if (key == juce::KeyPress ('c', cmd, 0)) { copySelected();      return true; }
    if (key == juce::KeyPress ('v', cmd, 0)) { paste();             return true; }
    if (key == juce::KeyPress ('d', cmd, 0)) { duplicateSelected(); return true; }
    if (key == juce::KeyPress ('a', cmd, 0)) { selectAllInLane (activeLane); return true; }

    if (key == juce::KeyPress::escapeKey)
    {
        selectedPoints.clear();
        repaint();
        return true;
    }

    return false;
}

//==============================================================================
void ControllerLaneArea::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    numberBox.setVisible (false);
    mode = Mode::none;

    if (clip == nullptr)
        return;

    const int index = getLaneAt (e.position.y);

    if (index < 0)
        return;

    if (e.position.x < (float) nameWidth || e.mods.isPopupMenu())
    {
        showLaneMenu (index);
        return;
    }

    const int lane = lanes[(size_t) index];
    const auto bounds = getLaneBounds (index);
    const bool free = e.mods.isAltDown();
    activeLane = lane;
    dragLane = lane;
    dragBounds = bounds;
    downPosition = e.position;
    downBeat = juce::jmax (0.0, xToBeat (e.position.x));
    downValue = yToValue (lane, e.position.y, bounds, free);
    copying = false;

    if (lane == velocityLane)
    {
        selectedPoints.clear();
        auto selected = getSelectedNotes ? getSelectedNotes() : juce::SortedSet<int>();
        const int bar = findBar (e.position, bounds);

        if (bar >= 0)
        {
            if (e.mods.isShiftDown() && selected.contains (bar))
                selected.removeValue (bar);
            else if (e.mods.isShiftDown() || ! selected.contains (bar))
            {
                if (! e.mods.isShiftDown())
                    selected.clear();

                selected.add (bar);
            }

            if (setSelectedNotes)
                setSelectedNotes (selected);

            if (selected.contains (bar))
            {
                mode = Mode::velocityBars;
                anchorNote = bar;
                originalNotes = clip->notes;
            }
        }
        else if (isSelectMode && isSelectMode())
        {
            notesBefore = e.mods.isShiftDown() ? selected : juce::SortedSet<int>();

            if (! e.mods.isShiftDown() && setSelectedNotes)
                setSelectedNotes ({});

            mode = Mode::rectangle;
            rectangle = { e.position, e.position };
        }
        else
        {
            mode = Mode::velocityLine;
            originalNotes = clip->notes;
        }

        repaint();
        return;
    }

    if (! selectedPoints.empty() && selectedPoints.front().kind != lane)
        selectedPoints.clear();

    original = clip->controllers;
    const int hit = findPoint (lane, e.position, bounds);

    if (hit >= 0)
    {
        const auto point = controllers::getLane (original, lane)[(size_t) hit];

        if (e.mods.isShiftDown() && hasEvent (selectedPoints, point))
            selectedPoints.erase (std::find (selectedPoints.begin(), selectedPoints.end(), point));
        else if (e.mods.isShiftDown())
            selectedPoints.push_back (point);
        else if (! hasEvent (selectedPoints, point))
            selectedPoints = { point };

        if (hasEvent (selectedPoints, point))
        {
            mode = Mode::move;
            anchorPoint = point;
            pointsBefore = selectedPoints;
            copying = e.mods.isCommandDown();
        }

        dragStarted = false;
        repaint();
        return;
    }

    if (isSelectMode && isSelectMode())
    {
        pointsBefore = e.mods.isShiftDown() ? selectedPoints : std::vector<ControllerEvent>();

        if (! e.mods.isShiftDown())
            selectedPoints.clear();

        mode = Mode::rectangle;
        rectangle = { e.position, e.position };
        repaint();
        return;
    }

    // A click adds a point; dragging turns it into freehand, a line (Shift) or, in the pedal lane, a press.
    const ControllerEvent point { lane, snapBeat (downBeat, free), downValue };
    auto events = original;
    events.push_back (point);
    selectedPoints = { point };
    pointsBefore.clear();
    dragStarted = false;
    mode = e.mods.isShiftDown() ? Mode::line : lane == controllers::sustainPedal ? Mode::press : Mode::draw;
    justCreatedTime = juce::Time::getMillisecondCounter();
    applyEvents (std::move (events));
}

void ControllerLaneArea::mouseDrag (const juce::MouseEvent& e)
{
    if (clip == nullptr || mode == Mode::none)
        return;

    if (! dragStarted && e.getDistanceFromDragStart() < 3)
        return;

    dragStarted = true;
    updateDrag (e);
}

void ControllerLaneArea::updateDrag (const juce::MouseEvent& e)
{
    const bool free = e.mods.isAltDown();
    const double beat = juce::jmax (0.0, xToBeat (e.position.x));
    const int value = yToValue (dragLane, e.position.y, dragBounds, free);

    // Events of the lane between two beats are replaced by the ones drawn there.
    const auto replaceRange = [this] (double from, double to, const std::vector<ControllerEvent>& with)
    {
        auto events = original;
        events.erase (std::remove_if (events.begin(), events.end(), [&] (const ControllerEvent& ev)
        {
            return ev.kind == dragLane && ev.beat >= from - 1.0e-9 && ev.beat <= to + 1.0e-9;
        }), events.end());
        events.insert (events.end(), with.begin(), with.end());
        return events;
    };

    switch (mode)
    {
        case Mode::move:
        {
            double dBeat = snapBeat (anchorPoint.beat + (beat - downBeat), free) - anchorPoint.beat;
            double minBeat = 1.0e9;

            for (auto& p : pointsBefore)
                minBeat = juce::jmin (minBeat, p.beat);

            dBeat = juce::jmax (dBeat, -minBeat);
            const int dValue = value - anchorPoint.value;
            auto events = original;

            if (! copying)
                events.erase (std::remove_if (events.begin(), events.end(), [this] (const ControllerEvent& ev) { return hasEvent (pointsBefore, ev); }), events.end());

            std::vector<ControllerEvent> moved;

            for (auto p : pointsBefore)
            {
                p.beat += dBeat;
                p.value = juce::jlimit (0, controllers::getMaxValue (p.kind), p.value + dValue);
                moved.push_back (p);
                events.push_back (p);
            }

            selectedPoints = moved;
            applyEvents (std::move (events));
            break;
        }

        case Mode::draw:
        {
            // A point at most every 1/32 beat, with the gaps of a fast drag filled in.
            const double q = std::floor (beat / controllers::lineStep + 1.0e-9) * controllers::lineStep;

            if (pointsBefore.empty())
                pointsBefore.push_back ({ dragLane, std::floor (downBeat / controllers::lineStep + 1.0e-9) * controllers::lineStep, downValue });

            const auto last = pointsBefore.back();
            const auto segment = controllers::getLinePoints (dragLane, last.beat, last.value, q, value);
            pointsBefore.insert (pointsBefore.end(), segment.begin(), segment.end());

            double lo = 1.0e9, hi = -1.0;

            for (auto& p : pointsBefore)
            {
                lo = juce::jmin (lo, p.beat);
                hi = juce::jmax (hi, p.beat);
            }

            auto path = pointsBefore;
            controllers::tidy (path);
            std::vector<ControllerEvent> drawn;

            for (auto& p : path)
                if (drawn.empty() || drawn.back().value != p.value)
                    drawn.push_back (p);

            selectedPoints.clear();
            applyEvents (replaceRange (lo, hi, drawn));
            break;
        }

        case Mode::line:
        {
            const double b0 = snapBeat (downBeat, free), b1 = snapBeat (beat, free);
            const auto points = controllers::getLinePoints (dragLane, b0, downValue, b1, value);
            selectedPoints.clear();
            applyEvents (replaceRange (juce::jmin (b0, b1), juce::jmax (b0, b1), points));
            break;
        }

        case Mode::press:
        {
            const double start = snapBeat (downBeat, free);
            double end = snapBeat (beat, free);

            if (end <= start)
                end = start + snapBeats;

            selectedPoints.clear();
            applyEvents (replaceRange (start, end, { { dragLane, start, 127 }, { dragLane, end, 0 } }));
            break;
        }

        case Mode::rectangle:
        {
            rectangle = juce::Rectangle<float> (downPosition, e.position).getIntersection (dragBounds);

            if (dragLane == velocityLane)
            {
                auto selected = notesBefore;

                for (int i = 0; i < (int) clip->notes.size(); ++i)
                    if (getBarBounds (clip->notes[(size_t) i], dragBounds).intersects (rectangle))
                        selected.add (i);

                if (setSelectedNotes)
                    setSelectedNotes (selected);
            }
            else
            {
                selectedPoints = pointsBefore;

                for (auto& p : controllers::getLane (clip->controllers, dragLane))
                    if (rectangle.contains (beatToX (p.beat), valueToY (dragLane, p.value, dragBounds)) && ! hasEvent (selectedPoints, p))
                        selectedPoints.push_back (p);
            }

            repaint();
            break;
        }

        case Mode::velocityBars:
        {
            // Up is louder; every selected note moves by the same number of steps, so their differences stay.
            const float inner = dragBounds.getHeight() - 2.0f * pad;
            const int steps = juce::roundToInt (-(float) e.getDistanceFromDragStartY() * 127.0f / inner);
            engine.setNoteVelocities (*clip, originalNotes, getSelectedNotes ? getSelectedNotes() : juce::SortedSet<int>(), steps);
            repaint();

            if (onEdited)
                onEdited();

            break;
        }

        case Mode::velocityLine:
        {
            // Notes under the line take its height at their start.
            const double b0 = downBeat, b1 = beat;
            const double lo = juce::jmin (b0, b1) - 3.0 / pixelsPerBeat, hi = juce::jmax (b0, b1) + 3.0 / pixelsPerBeat;

            {
                const juce::ScopedLock sl (engine.getLock());

                for (size_t i = 0; i < clip->notes.size() && i < originalNotes.size(); ++i)
                {
                    const double start = originalNotes[i].start;

                    if (start < lo || start > hi)
                    {
                        clip->notes[i].velocity = originalNotes[i].velocity;
                        continue;
                    }

                    const double t = std::abs (b1 - b0) < 1.0e-9 ? 1.0 : juce::jlimit (0.0, 1.0, (start - b0) / (b1 - b0));
                    const int v = juce::jlimit (1, 127, juce::roundToInt (downValue + t * (value - downValue)));
                    clip->notes[i].velocity = (float) v / 127.0f;
                }
            }

            engine.notesChanged();
            repaint();

            if (onEdited)
                onEdited();

            break;
        }

        case Mode::none:
            break;
    }
}

void ControllerLaneArea::mouseUp (const juce::MouseEvent&)
{
    mode = Mode::none;
    rectangle = {};
    repaint();
}

void ControllerLaneArea::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (clip == nullptr || e.position.x < (float) nameWidth)
        return;

    const int index = getLaneAt (e.position.y);

    if (index < 0)
        return;

    const int lane = lanes[(size_t) index];
    const auto bounds = getLaneBounds (index);

    if (lane == velocityLane)
    {
        const int bar = findBar (e.position, bounds);

        if (bar < 0)
            return;

        const auto selected = getSelectedNotes ? getSelectedNotes() : juce::SortedSet<int>();
        const auto r = getBarBounds (clip->notes[(size_t) bar], bounds);
        showNumberBox (juce::Rectangle<int> (44, 18).withCentre ({ (int) r.getCentreX(), (int) juce::jmax (bounds.getY() + 12.0f, r.getY() - 12.0f) }),
                       juce::roundToInt (clip->notes[(size_t) bar].velocity * 127.0f), 127,
                       [this, bar, selected] (int v)
                       {
                           {
                               const juce::ScopedLock sl (engine.getLock());
                               juce::SortedSet<int> targets;
                               targets.add (bar);

                               if (selected.contains (bar))
                                   targets = selected;

                               for (int i : targets)
                                   if (juce::isPositiveAndBelow (i, (int) clip->notes.size()))
                                       clip->notes[(size_t) i].velocity = (float) juce::jlimit (1, 127, v) / 127.0f;
                           }

                           engine.notesChanged();
                           repaint();

                           if (onEdited)
                               onEdited();
                       });
        return;
    }

    // Ignore the second click of a double-click that just added this point.
    if (juce::Time::getMillisecondCounter() - justCreatedTime < 600)
        return;

    if (const int hit = findPoint (lane, e.position, bounds); hit >= 0)
    {
        const auto point = controllers::getLane (clip->controllers, lane)[(size_t) hit];
        showNumberBox (juce::Rectangle<int> (52, 18).withCentre ({ (int) beatToX (point.beat), (int) juce::jmax (bounds.getY() + 12.0f, valueToY (lane, point.value, bounds) - 14.0f) }),
                       point.value, controllers::getMaxValue (lane),
                       [this, point] (int v)
                       {
                           auto events = clip->controllers;

                           for (auto& ev : events)
                               if (ev == point)
                                   ev.value = v;

                           selectedPoints = { { point.kind, point.beat, v } };
                           applyEvents (std::move (events));
                       });
    }
}

void ControllerLaneArea::showNumberBox (juce::Rectangle<int> area, int value, int maxValue, std::function<void (int)> apply)
{
    numberBox.setBounds (area);
    numberBox.setText (juce::String (value), false);
    numberBox.onReturnKey = [this, maxValue, apply]
    {
        const int v = juce::jlimit (0, maxValue, numberBox.getText().getIntValue());
        numberBox.setVisible (false);
        apply (v);
    };
    numberBox.onEscapeKey = [this] { numberBox.setVisible (false); };
    numberBox.onFocusLost = [this] { numberBox.setVisible (false); };
    numberBox.setVisible (true);
    numberBox.grabKeyboardFocus();
    numberBox.selectAll();
}

void ControllerLaneArea::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if ((e.mods.isCommandDown() || e.mods.isCtrlDown()) && wheel.deltaY != 0.0f)
    {
        if (onZoom)
            onZoom (wheel.deltaY > 0 ? 1.12 : 1.0 / 1.12, (float) (e.position.x - (float) nameWidth) + (float) scrollX);

        return;
    }

    if (std::abs (wheel.deltaX) > std::abs (wheel.deltaY))
    {
        if (onScrollX)
            onScrollX (juce::roundToInt (-wheel.deltaX * 200.0f));

        return;
    }

    scrollY = juce::jlimit (0, getMaxScrollY(), scrollY - juce::roundToInt (wheel.deltaY * 120.0f));
    repaint();
}

//==============================================================================
void ControllerLaneArea::buildKindMenu (juce::PopupMenu& menu, bool withVelocity, std::function<void (int)> choose) const
{
    for (int lane : commonLanes)
    {
        if (lane == velocityLane && ! withVelocity)
            continue;

        const bool shown = std::find (lanes.begin(), lanes.end(), lane) != lanes.end();
        menu.addItem (lane == controllers::sustainPedal ? "Sustain pedal (64)" : lane == controllers::modWheel ? "Mod wheel (1)"
                      : lane == controllers::expression ? "Expression (11)" : lane == controllers::volume ? "Volume (7)"
                      : lane == controllers::pan ? "Pan (10)" : getLongName (lane),
                      true, shown, [choose, lane] { choose (lane); });
    }

    juce::PopupMenu other;

    for (int first = 0; first <= controllers::maxController; first += 30)
    {
        juce::PopupMenu group;

        for (int n = first; n < first + 30 && n <= controllers::maxController; ++n)
        {
            juce::String name = "Controller " + juce::String (n);

            for (auto& c : midiguide::getCommonControllers())
                if (c.number == n)
                    name = juce::String (n) + " " + c.name;

            group.addItem (name, true, std::find (lanes.begin(), lanes.end(), n) != lanes.end(), [choose, n] { choose (n); });
        }

        other.addSubMenu (juce::String (first) + " to " + juce::String (juce::jmin (first + 29, controllers::maxController)), group);
    }

    menu.addSubMenu ("Other controller", other);
}

void ControllerLaneArea::showAddMenu()
{
    juce::PopupMenu menu;
    buildKindMenu (menu, true, [this] (int lane) { addLane (lane); });
    menu.addSeparator();
    menu.addItem ("About pedals, wheels and knobs...", onShowGuide != nullptr, false,
                  [this] { if (onShowGuide) onShowGuide ((int) midiguide::Topic::controllers); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton).withMinimumWidth (180));
}

void ControllerLaneArea::showLaneMenu (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) lanes.size()))
        return;

    const int lane = lanes[(size_t) index];
    activeLane = lane;
    repaint();
    juce::PopupMenu menu;

    juce::PopupMenu change;
    buildKindMenu (change, true, [this, index] (int newLane) { changeLane (index, newLane); });
    menu.addSubMenu ("Change to", change);

    if (lane != velocityLane && clip != nullptr)
    {
        const auto events = controllers::getLane (clip->controllers, lane);
        menu.addItem ("Thin out points", events.size() >= 3, false, [this, lane]
        {
            applyEvents (controllers::thinOut (clip->controllers, lane, controllers::getThinTolerance (lane)));
        });
        menu.addItem ("Select all in lane", ! events.empty(), false, [this, lane] { selectAllInLane (lane); });
        menu.addItem ("Remove all points in lane", ! events.empty(), false, [this, lane]
        {
            selectedPoints.clear();
            applyEvents (controllers::withLane (clip->controllers, lane, {}));
        });
    }
    else
        menu.addItem ("Select all in lane", clip != nullptr && ! clip->notes.empty(), false, [this, lane] { selectAllInLane (lane); });

    menu.addItem ("Hide lane", true, false, [this, index] { hideLane (index); });
    menu.addSeparator();
    menu.addItem ("About " + getLongName (lane) + "...", onShowGuide != nullptr, false,
                  [this, lane] { if (onShowGuide) onShowGuide (getGuideTopic (lane)); });
    menu.showMenuAsync (juce::PopupMenu::Options());
}

void ControllerLaneArea::addLane (int lane)
{
    if (std::find (lanes.begin(), lanes.end(), lane) != lanes.end())
        return;

    remembered.push_back (lane);
    hidden.erase (std::remove (hidden.begin(), hidden.end(), lane), hidden.end());
    saveLanes();
    rebuildLanes();
    activeLane = lane;

    if (getParentComponent() != nullptr)
        getParentComponent()->resized();

    scrollY = getMaxScrollY();
    repaint();
}

void ControllerLaneArea::hideLane (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) lanes.size()))
        return;

    const int lane = lanes[(size_t) index];
    remembered.erase (std::remove (remembered.begin(), remembered.end(), lane), remembered.end());
    extras.erase (std::remove (extras.begin(), extras.end(), lane), extras.end());

    // Without this the next edit (refresh()) would bring back a lane that still has points, such as a pedal.
    if (lane != velocityLane && std::find (hidden.begin(), hidden.end(), lane) == hidden.end())
        hidden.push_back (lane);

    saveLanes();
    rebuildLanes();
    selectedPoints.clear();
    activeLane = velocityLane;

    if (getParentComponent() != nullptr)
        getParentComponent()->resized();

    repaint();
}

void ControllerLaneArea::hideLaneKind (int lane)
{
    const auto found = std::find (lanes.begin(), lanes.end(), lane);

    if (found != lanes.end())
        hideLane ((int) (found - lanes.begin()));
}

void ControllerLaneArea::changeLane (int index, int newLane)
{
    if (! juce::isPositiveAndBelow (index, (int) lanes.size()) || lanes[(size_t) index] == newLane)
        return;

    const int old = lanes[(size_t) index];

    // A lane already shown elsewhere just moves here.
    for (auto* list : { &remembered, &extras })
        list->erase (std::remove (list->begin(), list->end(), newLane), list->end());

    bool replaced = false;

    for (auto* list : { &remembered, &extras })
    {
        for (auto& lane : *list)
            if (lane == old && ! replaced)
            {
                lane = newLane;
                replaced = true;
            }
    }

    if (old != velocityLane && std::find (hidden.begin(), hidden.end(), old) == hidden.end())
        hidden.push_back (old);

    hidden.erase (std::remove (hidden.begin(), hidden.end(), newLane), hidden.end());
    saveLanes();
    rebuildLanes();
    selectedPoints.clear();
    activeLane = newLane;

    if (getParentComponent() != nullptr)
        getParentComponent()->resized();

    repaint();
}
} // namespace daw
