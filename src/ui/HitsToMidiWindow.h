#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "Theme.h"
#include "analysis/HitFinder.h"
#include "engine/DrumTriggerGuide.h"
#include "engine/HitsToMidi.h"

namespace daw
{
/**
    The Hits to MIDI window for one audio clip: the clip's waveform with a marker for every hit DawIt found,
    in the colour of its drum, and the choices that decide which hits count (Sensitivity, Minimum gap), what they
    are written as, how loud, on the grid or as played, and whether the original keeps playing.

    The candidates come from one background pass (`HitsToMidiActions`); every control re-picks from them at once.
    Hand edits (click to add, drag to move, Delete, a right-click to re-sort) are kept as a list over the automatic
    hits and re-applied after each re-pick. Listen plays the clip, the hits or both through the engine's audition
    player. Make MIDI track puts the notes in a new track below with DawIt Drums and closes the window; Cancel and
    closing make nothing. The window closes itself if its clip is deleted.
*/
class HitsToMidiWindow final : public juce::Component,
                               private juce::ChangeListener,
                               private juce::Timer
{
public:
    HitsToMidiWindow (Engine&, Track&, AudioClip&, std::vector<analysis::hits::Candidate>);
    ~HitsToMidiWindow() override;

    /** The colour of a drum's markers: kick orange, snare blue, hi-hat green. */
    static juce::Colour getDrumColour (analysis::hits::Drum);

    /** Set by the owner: opens the *How drum triggers work* guide at a topic. */
    std::function<void (triggerguide::Topic)> onShowGuide;

    /** Set by the owner: closes this window (Cancel, and after Make MIDI track). Called at most once. */
    std::function<void()> onClose;

    /** Set by the owner: called with the new track and clip after Make MIDI track. */
    std::function<void (Track&, MidiClip&)> onMade;

    /** The hits as they stand now (after hand edits, before snapping). */
    const std::vector<analysis::hits::Hit>& getHits() const noexcept { return hits; }

    /** The hits as the guide's *Your clip* looks at them: beats from the clip's start, after snapping. */
    triggerguide::Part getPart() const;

    /** What the status line shows when no hit is selected. */
    juce::String getStatusText() const { return status.getText(); }

    /** Makes the track from the current hits, as the button does. */
    void makeTrack();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;

private:
    class View;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void repick();
    void settingsChanged();
    void saveSettings() const;
    void updateEnabled();
    void updateStatus();
    juce::String describeHit (int index) const;
    juce::String positionText (double seconds) const;
    juce::String getHitTooltip (int index) const;
    std::vector<MidiNote> getNotes() const;
    hitstomidi::Timing getTiming() const;
    analysis::hits::PickSettings getPickSettings() const;
    bool isClipAlive() const;
    void close();

    // Hand edits.
    void selectHitNear (double seconds);
    void addHit (double seconds, bool exact);
    void moveHit (int index, double toSeconds, bool exact);
    void removeHit (int index);
    void setDrum (int index, analysis::hits::Drum);

    // Listen.
    void toggleListen();
    void startListening (int startSample, double endSeconds = -1.0);
    void playSnippet (double fromSeconds);
    void stopListening();
    double getListenSeconds() const;

    void showGuide (triggerguide::Topic);

    Engine& engine;
    Track& track;
    AudioClip& clip;
    std::vector<analysis::hits::Candidate> candidates;
    std::vector<analysis::hits::HandEdit> edits;
    std::vector<analysis::hits::Hit> hits;
    std::vector<double> faintTimes;   // candidates under Sensitivity or inside the gap, drawn as faint ticks
    float loudestDb = -100.0f;
    double selectedTime = -1.0;
    int selected = -1;

    // Cached from the clip so painting never touches a clip that has just been deleted.
    double clipSeconds = 0.0, sampleRate = 44100.0, startBeat = 0.0;
    std::vector<float> peaks;
    bool closing = false;

    double listenFrom = 0.0;          // where Listen starts (a click on the ruler)
    bool listening = false;
    int snippetEndSample = -1;
    int restartCountdown = 0;

    juce::Label findLabel, notesLabel, timingLabel, originalLabel, sensLabel, gapLabel, writeLabel, velLabel,
                strengthLabel, hearLabel, status;
    juce::Slider sensitivity, gap, strength;
    juce::ComboBox writeAs, velocity, grid, hear;
    juce::ToggleButton keepFeel { "Keep feel" }, snapTo { "Snap to" }, keepPlaying { "Keep playing (layer)" }, muteClip { "Mute the clip (replace)" };
    juce::TextButton listenButton, cancelButton { "Cancel" }, makeButton { "Make MIDI track" };
    GuideButton helpButton { "?" };
    std::unique_ptr<View> view;
    std::vector<std::pair<juce::Component*, triggerguide::Topic>> topicOf;
    triggerguide::Topic hoverTopic = triggerguide::Topic::whatItIs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitsToMidiWindow)
};
} // namespace daw
