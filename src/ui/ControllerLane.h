#pragma once

#include "engine/Engine.h"
#include "engine/MidiControllers.h"
#include "Theme.h"

namespace daw
{
/** The thin bar between the note grid and the lanes: drag it up or down to share the height. */
class LaneDivider final : public juce::Component, public juce::SettableTooltipClient
{
public:
    LaneDivider();

    /** Called while dragging, with how far the mouse moved from where the drag began (up is negative). */
    std::function<void (int dy)> onDrag;
    std::function<void()> onDragStart, onDragEnd;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onDragStart) onDragStart(); }
    void mouseDrag (const juce::MouseEvent& e) override { if (onDrag) onDrag (e.getDistanceFromDragStartY()); }
    void mouseUp (const juce::MouseEvent&) override { if (onDragEnd) onDragEnd(); }
};

/**
    The lanes under the piano roll's note grid: a velocity lane (a bar per note) and lanes for the controllers
    of the MIDI clip (sustain pedal, pitch bend, mod wheel, expression, volume, pan, channel pressure, any
    other controller number). They share the grid's horizontal scroll, zoom and Snap (see setView()).

    A controller lane shows steps: a value holds until the next point, as on a MIDI cable. Click adds a point,
    drag a point to move it (Alt: off the grid; Cmd: copy), drag across empty space to draw freehand, Shift+drag
    draws a straight line, and in the piano roll's Select mode a drag selects with a rectangle. In the sustain
    lane a drag draws a press. Delete removes, Cmd+C, Cmd+V and Cmd+D copy, paste and duplicate. The lane's name
    at the left opens its menu; + Lane adds one. The velocity lane edits the notes' velocities, selected notes
    (the grid's selection) moving together.

    The lanes shown are remembered (`view.pianoRollLanes`), and a clip also shows every lane it has data in.
*/
class ControllerLaneArea final : public juce::Component, public juce::TooltipClient
{
public:
    explicit ControllerLaneArea (Engine&);

    /** The lane number of the velocity lane (the others are controller kinds, see `daw::controllers`). */
    static constexpr int velocityLane = -1;
    static constexpr int laneHeight = 64, buttonRowHeight = 22, nameWidth = 56;

    /** Shows the lanes of this clip (nulls: none), adding a lane for every controller the clip has events in. */
    void setClip (Track*, MidiClip*);

    /** Reads the remembered lanes and their height from the app settings again (the screenshot runner writes them after start-up). */
    void reloadLanesFromSettings();

    /** Call when the clip's events may have changed (an undo, a take): adds a lane for any controller that is new.
        Returns true when the lanes shown changed, so the layout needs redoing. */
    bool refresh();

    /** The grid's zoom, horizontal scroll and snap, so the lanes line up with the notes. */
    void setView (double pixelsPerBeat, int scrollX, double snapBeats);

    /** The playhead from the clip's start in beats, or below 0 when it isn't in the clip. */
    void setPlayhead (double beatInClip);

    /** The height the area wants, at most `maxHeight`: the button row and as much of the lanes as the user
        dragged the divider to show (`view.pianoRollLanesHeight`, 128 until then). */
    int getPreferredHeight (int maxHeight) const;
    int getNumLanes() const noexcept { return (int) lanes.size(); }

    /** True when the lane (`velocityLane` or a controller kind) is shown now. */
    bool isLaneShown (int lane) const { return std::find (lanes.begin(), lanes.end(), lane) != lanes.end(); }

    /** Hides the lane as its menu's *Hide lane* does. It stays hidden for this clip, even when the clip has points in it. */
    void hideLaneKind (int lane);

    /** Divider support: the wanted height of the lanes without the button row, remembered across runs. */
    int getWantedLanesHeight() const;
    void setWantedLanesHeight (int);
    int getMaxLanesHeight() const noexcept { return (int) lanes.size() * laneHeight; }

    /** Called when something here changed the notes or lanes, to repaint the grid. */
    std::function<void()> onEdited;

    /** The note grid's selection (indices into the clip's notes), read and set by the velocity lane. */
    std::function<juce::SortedSet<int>()> getSelectedNotes;
    std::function<void (const juce::SortedSet<int>&)> setSelectedNotes;

    /** True while the piano roll is in Select mode: a drag in empty space then selects with a rectangle. */
    std::function<bool()> isSelectMode;

    /** Called with a MIDI guide topic number (`midiguide::Topic`) for "About ..." items. */
    std::function<void (int topic)> onShowGuide;

    /** Scrolls the grid sideways by this many pixels (the mouse wheel over the lanes). */
    std::function<void (int dx)> onScrollX;

    /** Zooms the grid (Cmd + wheel), as the grid does. */
    std::function<void (double factor, float anchorX)> onZoom;

    /** The name shown at the left of a lane: "Velocity", "Pedal", "Bend", "Mod", "Expr", "Vol", "Pan", "Press", "CC 74". */
    static juce::String getShortName (int lane);

    /** The longer name for menus and tooltips: "Velocity", "Sustain pedal", "Pitch bend", "Controller 74". */
    static juce::String getLongName (int lane);

    /** The tooltip of a lane's name. */
    static juce::String getLaneTooltip (int lane);

    /** The MIDI guide topic (`midiguide::Topic` as a number) a lane's "About" item opens. */
    static int getGuideTopic (int lane);

    /** "+0.5 semitone (10240)" for a bend value, "down (127)" for a pedal, the number otherwise. */
    static juce::String describeValue (int lane, int value);

    juce::String getTooltip() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    enum class Mode { none, move, rectangle, draw, line, press, velocityBars, velocityLine };

    void rebuildLanes();
    void saveLanes() const;
    int getLaneAt (float y) const;
    juce::Rectangle<float> getLaneBounds (int index) const;
    float beatToX (double beat) const noexcept { return (float) nameWidth + (float) (beat * pixelsPerBeat) - (float) scrollX; }
    double xToBeat (float x) const noexcept { return ((double) (x - (float) nameWidth) + (double) scrollX) / pixelsPerBeat; }
    double snapBeat (double beat, bool free) const;
    static float valueToY (int lane, int value, juce::Rectangle<float> bounds);
    static int yToValue (int lane, float y, juce::Rectangle<float> bounds, bool free);
    int findPoint (int lane, juce::Point<float>, juce::Rectangle<float> bounds) const;
    int findBar (juce::Point<float>, juce::Rectangle<float> bounds) const;
    juce::Rectangle<float> getBarBounds (const MidiNote&, juce::Rectangle<float> bounds) const;
    juce::String describeBeat (double beat) const;
    int getMaxScrollY() const;
    juce::Colour getBaseColour() const;

    void paintLane (juce::Graphics&, int index, juce::Rectangle<float> bounds);
    void paintSteps (juce::Graphics&, int lane, juce::Rectangle<float> bounds);
    void paintVelocity (juce::Graphics&, juce::Rectangle<float> bounds);

    void showAddMenu();
    void showLaneMenu (int index);
    void addLane (int lane);
    void hideLane (int index);
    void changeLane (int index, int newLane);
    void buildKindMenu (juce::PopupMenu&, bool withVelocity, std::function<void (int)> choose) const;
    void applyEvents (std::vector<ControllerEvent>);
    void selectAllInLane (int lane);
    void removeSelected();
    void copySelected();
    void paste();
    void duplicateSelected();
    void updateDrag (const juce::MouseEvent&);
    void showNumberBox (juce::Rectangle<int>, int value, int maxValue, std::function<void (int)> apply);

    Engine& engine;
    Track* track = nullptr;
    MidiClip* clip = nullptr;
    std::vector<int> remembered, extras, lanes, hidden;   // `hidden`: lanes the user hid for this clip, which refresh() doesn't bring back
    double pixelsPerBeat = 64.0, snapBeats = 0.25, playhead = -1.0;
    int scrollX = 0, scrollY = 0, activeLane = velocityLane;
    int wantedLanesHeight = 128;

    juce::TextButton addButton { "+ Lane" };
    juce::TextEditor numberBox;

    // The controller points selected (all of one lane).
    std::vector<ControllerEvent> selectedPoints;

    // A drag.
    Mode mode = Mode::none;
    int dragLane = velocityLane;
    std::vector<ControllerEvent> original;
    std::vector<MidiNote> originalNotes;
    ControllerEvent anchorPoint;
    int anchorNote = -1;
    bool copying = false, dragStarted = false;
    juce::uint32 justCreatedTime = 0;
    juce::Point<float> downPosition;
    double downBeat = 0.0;
    int downValue = 0;
    juce::Rectangle<float> rectangle;
    juce::Rectangle<float> dragBounds;
    juce::SortedSet<int> notesBefore;
    std::vector<ControllerEvent> pointsBefore;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllerLaneArea)
};
} // namespace daw
