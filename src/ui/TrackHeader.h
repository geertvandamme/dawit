#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "PluginWindows.h"
#include "Selection.h"
#include "RecordingActions.h"
#include "TrackActions.h"

namespace daw
{
/** Left-hand strip of a track: name, record arm (MIDI and audio tracks), scale filter (MIDI tracks), mute/solo, volume,
    pan, meter and plugin chain.

    An armed audio track also shows an input chip after R ("In 1": click it for the device's inputs), a Monitor
    button, and its input level on the meter instead of its output level.

    Tracks and buses have a Sends button that opens the mixer on the track's strip, where its sends are.

    A bus (a track that other tracks play into) has a fold arrow and a ? that opens the buses guide instead of the
    arm and scale buttons, and the tracks in a bus are indented by a gutter in the bus's colour.
    Click selects the track, Shift-click and Cmd-click select several; drag the header (by its name or its
    background) to move it up or down or into a bus; right-click it for the track menu (see TrackActions). */
class TrackHeader final : public juce::Component, public juce::TooltipClient, private juce::Timer
{
public:
    TrackHeader (Engine&, Track&, Selection&, PluginWindowManager&, TrackActions&);
    ~TrackHeader() override;

    Track& getTrack() const noexcept { return track; }

    /** While the header is dragged: called with each mouse position (relative to this header), when it is let go,
        and when Escape cancels the drag. */
    std::function<void (TrackHeader&, const juce::MouseEvent&)> onDragMove;
    std::function<void (TrackHeader&)> onDragEnd;
    std::function<void()> onDragCancel;

    /** Opens the name for typing (a new bus). */
    void editName();

    /** Follows the track's place in the tree: its indent, and for a bus its tracks and fold arrow. */
    void updateStructure();

    static constexpr int gutterWidth = 10, maxGutters = 4;

    /** How far the track's controls are pushed right by its bus levels. */
    int getIndent() const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::String getTooltip() override;

    void updateFxButton();

private:
    void timerCallback() override;
    void showPluginMenu();
    void updateInputControls();
    void setDragging (bool);
    juce::String getBottomText() const;
    std::vector<Track*> getBusChain() const;   // from the outermost bus to the one it plays into

    Engine& engine;
    Track& track;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    TrackActions& actions;

    juce::Label nameLabel;
    juce::TextButton armButton { "R" }, muteButton { "M" }, soloButton { "S" }, fxButton { "FX" }, deleteButton { "x" },
                     scaleButton { "Scale" }, sendsButton { "Sends" }, foldButton,
                     inputChip, monitorButton { "Mon" };
    GuideButton guideButton { "?" };
    recordingui::InputMeter inputMeter;
    bool shownInput = false;
    juce::Slider volumeSlider, panSlider;
    juce::Rectangle<int> meterArea;
    float meterLeft = 0.0f, meterRight = 0.0f;
    int shownDepth = -1;
    juce::String shownBottomText;
    bool dragging = false, collapseOnMouseUp = false;
};
} // namespace daw
