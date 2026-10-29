#pragma once

#include "SingWindow.h"
#include "PluginWindows.h"
#include "Selection.h"

namespace daw
{
/** "Sing this melody..." for a MIDI clip and "Sing the lyrics with a computer voice..." for the Lyrics row: opens the
    Sing window, and when the user presses Sing, loads the voice, builds the sung score and renders it phrase by phrase
    in the background behind a progress window with a Cancel button ("Singing phrase 3 of 12..."). The vocal then goes
    on a new audio track after the melody's track, which is selected. The melody track only changes if the user chose
    to mute it. */
class SingActions
{
public:
    SingActions (Engine&, Selection&, PluginWindowManager&, juce::Component& parent);
    ~SingActions();

    /** Opens the Sing window on a MIDI clip's melody (`clip` null: the whole track), or brings it to the front. */
    void open (Track&, MidiClip* clip);

    /** Opens it on the selected MIDI track, or the first one that has notes; says so when there is none. */
    void openForLyrics();

    /** Opens the lyrics guide at a topic (set by ArrangementView). */
    std::function<void (gen::lyricsguide::Topic)> onOpenGuide;

    /** The window, when it's open (for the screenshots). */
    SingWindow* getWindow() const { return window.getComponent(); }

private:
    class Job;

    void start (SingRequest);
    void finished (bool cancelled);

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    juce::Component& parent;
    juce::Component::SafePointer<SingWindow> window;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (SingActions)
};
} // namespace daw
