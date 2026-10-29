#pragma once

#include "engine/Engine.h"
#include "engine/RoutingGuide.h"
#include "PluginWindows.h"
#include "RecordingActions.h"
#include "Selection.h"

namespace daw
{
/**
    What can be done with tracks and buses, shared by the track headers, the mixer strips, the device bar and the
    Track menu: the right-click menu (create a bus, Output, remove from a bus, ungroup, delete, export, and the
    guide as its last item), the Output menu, and the bus commands behind them.

    The commands act on the selected tracks (Shift-click and Cmd-click select several); a right-click on a track
    that isn't selected acts on that track alone. Sends and returns are here too: the Sends menu, New return track,
    the send menu and the guide's actions on returns. Owned by the ArrangementView, which reacts to onEditName,
    onShowGuide and onDelete.
*/
class TrackActions final
{
public:
    TrackActions (Engine&, Selection&, PluginWindowManager&, juce::Component& dialogParent);

    /** A new bus was made: open its name for typing. */
    std::function<void (Track&)> onEditName;

    /** Opens the Buses, sends and routing guide at a topic. */
    std::function<void (routingguide::Topic)> onShowGuide;

    /** *About recording audio...* (track and clip menus), the Monitor warning: open the How recording works guide at a topic. */
    std::function<void (recordingguide::Topic)> onShowRecordingGuide;

    /** The track menu's *Singing techniques...* (shown for a track with a vocal word in its name): open the singing guide. */
    std::function<void()> onShowSingingGuide;

    /** The track menu's *Show in Signal Flow...*: open the Signal Flow window at this track. */
    std::function<void (Track&)> onShowSignalFlow;

    /** Called by a header's Sends button: select the track and show its strip in the mixer. */
    std::function<void (Track&)> onShowSends;

    /** The track menu's *Bounce to WAV...*: open the Bounce window with these tracks selected and *Selected tracks* picked. */
    std::function<void (std::vector<Track*>)> onBounceToWav;

    /** The tracks should be deleted, after asking: for a bus `withItsTracks` says whether its tracks go too. */
    std::function<void (std::vector<Track*>, bool withItsTracks)> onDelete;

    /** The right-click menu of a track or a bus, for the selected tracks when `clicked` is one of them. */
    void showTrackMenu (Track& clicked, juce::Component* target = nullptr);

    /** Master and every bus that can take the track (indented by level, the current one ticked), then New bus...
        Item ids run from `firstId`; use applyOutput() with the chosen id. */
    juce::PopupMenu makeOutputMenu (const Track&, int firstId) const;

    /** Puts the track where the chosen item of makeOutputMenu() says. */
    void applyOutput (Track&, int firstId, int chosenId);

    /** Opens the Output menu under a component, and applies the choice. */
    void showOutputMenu (Track&, juce::Component* target);

    /** True when something is selected, so Create bus has something to group. */
    bool canCreateBus() const;

    /** Create bus from the selected tracks, named by routingguide::suggestBusName; selects it and opens its name. */
    Track* createBus();

    /** The same from a list of tracks and a name (the guide's action). */
    Track* createBus (const std::vector<Track*>&, const juce::String& name, Track* beside = nullptr);

    /** Moves tracks into an existing bus (Engine::moveTrack), skipping any that can't go there. */
    void moveIntoBus (const std::vector<Track*>&, Track& bus);

    /** An empty bus below the selected track, selected, with its name open. */
    Track* addEmptyBus();

    /** The track goes to where its bus plays, right below the bus. */
    void removeFromBus (Track&);

    /** Removes the bus and keeps its tracks, asking first when the bus has effects. */
    void ungroup (Track& bus);

    /** Puts DawIt Compressor on the bus with the Drum bus starting point, and opens its window. */
    void addGlueCompressor (Track& bus);

    /** What a new return track starts with. */
    enum class ReturnKind { reverb, delay, empty };

    /** A new return track at the end of the song, selected: Reverb (DawIt Reverb at "Return: plate"), Delay (DawIt Delay
        at "Return: quarter echo") or Empty ("Return 1"). Every track in `sendFrom` gets a send to it at -10 dB. */
    Track* addReturn (ReturnKind, const std::vector<Track*>& sendFrom = {});

    /** Opens the three kinds of return under a component (the transport bar's + Return button). */
    void showNewReturnMenu (juce::Component* target);

    /** One item per return ("Reverb (-10.0 dB)"), ticked when every one of `targets` sends to it: ids run from `firstId`.
        Then New return track >. Use applySendsChoice() with the chosen id. Empty when the song has no return and
        nothing to add (never: New return track is always there). */
    juce::PopupMenu makeSendsMenu (const std::vector<Track*>& targets, int firstId) const;

    /** Does what the chosen item of makeSendsMenu() says: turns the sends on (at -10 dB) or off for every target, or makes a return.
        Returns the track worth showing in the mixer afterwards: the return when a send was turned on or a return made,
        otherwise the first target (nullptr when nothing was chosen). */
    Track* applySendsChoice (const std::vector<Track*>& targets, int firstId, int chosenId);

    /** The menu of the Sends button on a track header: the returns (ticked when on), New return track >, and the guide.
        Choosing a return or a new return does it and then calls onShowSends with the return (or with this track when a
        send was switched off), so the mixer opens on it. Ids as in sendsButtonChoice(). */
    void showSendsButtonMenu (Track& from, juce::Component* target);

    /** What choosing `id` in the Sends button's menu does: ids from 1 are the returns in order, 101 to 103 a new Reverb, Delay or
        Empty return, 1000 the guide. Used by the menu and by the screenshot runner. */
    void sendsButtonChoice (Track& from, int id);

    /** The right-click menu of a send (a mixer knob or a device bar chip): Post-fader / Pre-fader, Off, Show the
        return's chain, and the guide. */
    void showSendMenu (Track& from, Track& returnTrack, juce::Component* target);

    /** Share one effect on a return: makes a return with the first track's effect at 100% wet (or uses `existing`),
        sends from every track at the level that keeps its amount, and bypasses their own effects (it doesn't delete them). */
    void shareOnReturn (const std::vector<Track*>& tracks, const juce::String& identifier, Track* existing);

    /** Puts DawIt Reverb or DawIt Delay on a return at its Return starting point, and opens its window. */
    void addReturnEffect (Track& returnTrack, const juce::String& identifier);

    /** Sets DawIt Reverb or DawIt Delay on a return to a mix of 100%. */
    void makeReturnWet (Track& returnTrack, const juce::String& identifier);

    /** Deletes the return after asking (through onDelete). */
    void deleteReturn (Track&);

    /** "Output" text for a strip: the name of the bus the track plays into, or Master. */
    static juce::String getOutputName (const Track&);

private:
    std::vector<Track*> getTargets (Track& clicked) const;
    std::vector<Track*> getOutputChoices (const std::vector<Track*>& targets) const;
    juce::PopupMenu buildOutputMenu (const std::vector<Track*>& targets, int firstId) const;
    void applyOutput (const std::vector<Track*>& targets, int firstId, int chosenId);
    void moveOut (Track&);
    bool isAlive (const Track&) const;

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    juce::Component& dialogParent;
};
} // namespace daw
