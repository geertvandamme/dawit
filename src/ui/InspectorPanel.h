#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "PluginWindows.h"
#include "SendKnob.h"
#include "Selection.h"
#include "Theme.h"
#include "TrackActions.h"

namespace daw
{
/** The drag edge on the right of the inspector: drag it sideways to make the inspector wider or narrower. */
class InspectorEdge final : public juce::Component, public juce::SettableTooltipClient
{
public:
    InspectorEdge();

    /** Gives the width the inspector has now (asked when a drag starts). */
    std::function<int()> getPanelWidth;

    /** Called while dragging, with the width the drag asks for (not yet limited). */
    std::function<void (int)> onResize;

    /** Called when the mouse goes up after a drag, so the width can be remembered. */
    std::function<void()> onResizeEnd;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    int startWidth = 0;
};

/**
    The inspector: a narrow, full-height panel on the left of the track headers (as in Logic Pro) with the selected
    track's effects, sends, pan and volume. It shows the track clicked last (`Selection::getTrack()`), or the master
    when none is selected: its volume and meter only.

    From top to bottom it follows the sound. The title row has the ? (the Buses, sends and routing guide at *The
    inspector*) and the x that hides the panel. Then the track's name, its type and where it plays, mute, solo and
    Output. Then, in a list that scrolls when it is long, the effects (the instrument first on a MIDI track; click one to
    open it, click its dot to bypass it, x removes it, drag it up or down to change the order, right-click for more,
    + Effect adds one) and the sends (a row per return in the song, drawn by SendKnob in its wide look, and + Send to a
    new return). A return has no sends: it shows the tracks that send to it (Fed by) instead. Pan and the volume fader
    with its meter stay at the bottom. It changes only the track it shows, and the same values as the mixer's strip, so
    the two stay in step. Right-click on the panel: Hide Inspector, and the guide. */
class InspectorPanel final : public juce::Component, public juce::TooltipClient, private juce::ChangeListener, private juce::Timer
{
public:
    InspectorPanel (Engine&, Selection&, PluginWindowManager&, TrackActions&);
    ~InspectorPanel() override;

    /** Called when the x or Hide Inspector is chosen. */
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    /** Tooltips for the title row and the name area. */
    juce::String getTooltip() override;

    static constexpr int minWidth = 200, maxWidth = 360, defaultWidth = 230, edgeWidth = 4;

private:
    class EffectRow;
    class FedRow;
    class AddRow;

    /** The scrolling list of effects and sends; draws the headings and the drop marker. */
    struct Content final : public juce::Component
    {
        std::function<void (juce::Graphics&)> onPaint, onPaintOverChildren;
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
        void paintOverChildren (juce::Graphics& g) override { if (onPaintOverChildren) onPaintOverChildren (g); }
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    /** Follows the selection and the edit: rebuilds the rows when the shown track, its effects, the returns or the edit changed. */
    void refresh();
    void rebuildRows();
    void layoutRows();
    void updateValues();

    Track* getShownTrack() const;
    bool isShownAlive() const;
    void showMenu();
    void showNewReturnMenu (juce::Component& target);

    // Dragging an effect row to a new place in the chain.
    void dragRow (EffectRow&, const juce::MouseEvent&);
    bool endRowDrag();

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    TrackActions& actions;

    Track* shownTrack = nullptr;   // null: the master
    std::vector<const PluginSlot*> shownSlots;
    std::vector<Track*> shownTargets, shownSenders;
    bool shownBuiltInSynth = false;
    int shownGeneration = -1;
    juce::String shownName, shownType, shownOutput;
    int shownSelectedCount = 0;

    GuideButton guideButton { "?" };
    juce::TextButton closeButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) };
    juce::TextButton muteButton { "M" }, soloButton { "S" }, outputButton;
    juce::Slider panKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider fader { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label panLabel;
    juce::Viewport viewport;
    Content content;
    juce::OwnedArray<EffectRow> rows;
    juce::OwnedArray<FedRow> fedRows;
    juce::OwnedArray<SendKnob> sendKnobs;
    juce::OwnedArray<AddRow> addRows;
    std::vector<std::pair<int, juce::String>> headings, notes;   // in the content: y and text

    EffectRow* draggedRow = nullptr;
    int dropIndex = -1, dropMarkerY = -1;

    juce::Rectangle<int> nameArea, typeArea, selectionArea, noteArea, panArea, volumeArea, meterArea;
    float meterLeft = 0.0f, meterRight = 0.0f;
};
} // namespace daw
