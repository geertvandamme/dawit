#pragma once

#include "engine/Engine.h"
#include "engine/RoutingGuide.h"
#include "engine/SignalFlow.h"
#include "GuidePrint.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The Buses, sends and routing guide: where each track's sound goes, what a bus is for, grouping tracks, why levels add
    up in a bus, sends and returns (a shared reverb, insert or send, depth, pre- and post-fader), glue compression,
    solo and mute, and a look at the song's own routing. At the top it draws the routing tree: the tracks on the
    left, the buses to their right, the returns before the master at the far right, with arrows for where each
    sound goes (dashed lines for sends, with their levels) and a small live meter on every box; some topics draw
    their own picture (the send knobs and a return's chain, an insert beside a send, the two places a send can be
    taken, and parts placed by their send levels). It shows the song when the topic is about it
    (and always when there are no tracks), and an example otherwise. Click a box to select that track.
    "Your song's routing" suggests what to try, with a button that does it (Create a Drums bus from Kick, Snare
    and Hats; Put DawIt Compressor on Drums; Share one reverb on a return; Put DawIt Compressor before DawIt
    Reverb). The Signal flow topic draws the example flow diagram with the Signal Flow window's painter, and has a
    button that opens the window for the song.
*/
class RoutingExplainer final : public juce::Component,
                               public guideprint::Source,
                               private juce::ListBoxModel,
                               private juce::ChangeListener,
                               private juce::Timer
{
public:
    RoutingExplainer (Engine&, Selection&);
    ~RoutingExplainer() override;

    /** Create bus from these tracks, with this name (the button on "Your song's routing"); `beside` is the bus of another
        kind they sit in now (the new bus goes next to it), or null. */
    std::function<void (std::vector<Track*>, juce::String name, Track* beside)> onCreateBus;

    /** Move these tracks into this existing bus (the button on "Your song's routing"). */
    std::function<void (std::vector<Track*>, Track& bus)> onMoveIntoBus;

    /** Put DawIt Compressor with the Drum bus starting point on this bus. */
    std::function<void (Track& bus)> onAddGlue;

    /** Share one reverb or delay: these tracks send to a return (an existing one, or null for a new one) instead of
        each having their own effect, which is switched off. The effect is the identifier ("DawIt Reverb"). */
    std::function<void (std::vector<Track*>, juce::String effect, Track* existingReturn)> onShareEffect;

    /** Put DawIt Reverb or DawIt Delay on an empty return, and set one that isn't at 100% mix to 100%. */
    std::function<void (Track& returnTrack, juce::String effect)> onAddReturnEffect, onMakeWet;

    /** Move an effect in a track's chain to a place (Engine::movePlugin): the button of the effect-order advice. */
    std::function<void (Track&, const PluginSlot*, int newIndex)> onMoveEffect;

    /** Open the Signal Flow window (the button on the Signal flow topic). */
    std::function<void()> onShowSignalFlow;

    void showTopic (routingguide::Topic);

    juce::String getPrintTitle() const override { return "Buses, sends and routing"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class RoutingPicture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh (true); }
    void timerCallback() override { refresh (false); }

    /** Brings the picture, the text and the button up to date with the song. */
    void refresh (bool structureChanged);
    float getMasterPeakDb() const;
    void updateText();
    void updateAction();
    void doAction();

    Engine& engine;
    Selection& selection;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<RoutingPicture> picture;
    juce::TextEditor explanation;
    juce::TextButton actionButton;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::routing };
    std::optional<routingguide::Topic> topicBeforePrinting;
    routingguide::Topic shown = routingguide::Topic::what;

    std::optional<routingguide::Advice> shownAction;
    Track* actionBus = nullptr;
    juce::String shownText;

    static constexpr int width = 980, height = 720;
};
} // namespace daw
