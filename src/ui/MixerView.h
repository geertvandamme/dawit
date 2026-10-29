#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "engine/Routing.h"
#include "RecordingActions.h"
#include "SendKnob.h"
#include "Selection.h"
#include "Theme.h"
#include "TrackActions.h"

namespace daw
{
/** One channel of the mixer: name, mute and solo, pan, a fader, a meter and, under the fader, where its sound goes
    (the Output button: the master or a bus). A track's strip selects the track when clicked (Shift-click and
    Cmd-click select several, right-click opens the track menu); the master strip (no track) sets the master volume.
    A bus's strip has a fold arrow in its top left. The master strip has a ? (and a right-click menu) that opens the
    Buses, sends and routing guide at its mix bus topic.

    When the song has a return track, every track's and bus's strip also has a row for each of the first four
    returns, between the pan and the fader: a small knob with the return's name to its right (how much of the track
    goes to that return, Off to +6 dB; PRE marks a send taken before the fader). A return's strip says RETURN and plays into the master only. */
class ChannelStrip final : public juce::Component, public juce::TooltipClient
{
public:
    ChannelStrip (Engine&, Selection&, TrackActions&, Track*);
    ~ChannelStrip() override;

    Track* getTrack() const noexcept { return track; }

    /** Follows the track (or master): volume, pan, mute and solo changed elsewhere, its name, and the meter. */
    void update();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    /** Over the meter of an armed audio track: how to read the input level. */
    juce::String getTooltip() override;

    static constexpr int width = 88;

    /** The send knobs show for this many returns at most. */
    static constexpr int maxSendKnobs = 4, sendRowHeight = 22;

private:
    bool isAlive() const;
    int getNumSendKnobs() const;
    juce::Rectangle<int> getFoldArea() const;

    Engine& engine;
    Selection& selection;
    TrackActions& actions;
    Track* const track;   // null for the master

    juce::TextButton muteButton { "M" }, soloButton { "S" }, outputButton;
    GuideButton guideButton { "?" };   // ? on the master only
    bool shownSilenced = false;   // muted, or another track soloed: the strip dims
    routing::Silence shownSilence = routing::Silence::none;
    bool shownFolded = false;
    juce::String shownOutput;
    juce::Slider panKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider fader { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label panLabel;
    juce::String shownName;
    juce::OwnedArray<SendKnob> sendKnobs;
    juce::Rectangle<int> sendArea;
    juce::Rectangle<int> meterArea;
    float meterLeft = 0.0f, meterRight = 0.0f;
    recordingui::InputMeter inputMeter;   // the meter of an armed audio track shows what comes in
    bool shownInput = false;
};

/** Bottom panel, in place of the clip editor: a strip per track side by side, in the order of the arrangement's
    rows (a bus's strip comes right before its tracks, joined by a bracket in its colour, and folding a bus hides
    their strips), and the master on the right. Clicking a strip selects its track in the arrangement too; the x
    closes the panel. */
class MixerView final : public juce::Component, private juce::Timer
{
public:
    MixerView (Engine&, Selection&, TrackActions&);

    /** Called when the close button is clicked: hide the panel. */
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Scrolls the strip of the track into view (the track's Sends button uses it). */
    void showStripOf (const Track&);

private:
    void timerCallback() override;

    /** Rebuilds the strips when tracks were added, removed or moved. */
    void syncStrips();

    /** The strips' holder, which draws the brackets over them. */
    struct StripHolder final : public juce::Component
    {
        std::function<void (juce::Graphics&)> onPaintOverChildren;
        void paintOverChildren (juce::Graphics& g) override { if (onPaintOverChildren) onPaintOverChildren (g); }
    };

    void paintBrackets (juce::Graphics&);

    Engine& engine;
    Selection& selection;
    TrackActions& actions;

    juce::Label title { {}, "Mixer" }, hint;
    juce::TextButton closeButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) };
    juce::Viewport viewport;
    StripHolder stripHolder;
    juce::OwnedArray<ChannelStrip> strips;
    ChannelStrip master { engine, selection, actions, nullptr };
    std::vector<Track*> shownSelection;

    static constexpr int headerHeight = 30;
};
} // namespace daw
