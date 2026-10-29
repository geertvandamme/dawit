#pragma once

#include "PluginWindows.h"
#include "Selection.h"
#include "engine/Engine.h"
#include "engine/DrumTriggerGuide.h"
#include "analysis/HitFinder.h"

namespace daw
{
class HitsToMidiWindow;

/** "Hits to MIDI..." for an audio clip: finds every rise in level (the candidates) in the background
    behind a progress window with Cancel, then opens the Hits to MIDI window on the clip, one window per
    clip. The window makes the MIDI track itself; this class selects it afterwards. */
class HitsToMidiActions
{
public:
    HitsToMidiActions (Engine&, Selection&, PluginWindowManager&, juce::Component& parent);
    ~HitsToMidiActions();

    /** Brings the clip's window to the front, or finds its hits and opens one. */
    void open (Track&, AudioClip&);

    /** Set by the owner: opens the *How drum triggers work* guide at a topic. */
    std::function<void (triggerguide::Topic)> onShowGuide;

    /** The open window for a clip, or null. */
    HitsToMidiWindow* getWindowFor (const AudioClip&) const;

    /** The window the guide's "Your clip" looks at: the most recently opened one still open, or null. */
    HitsToMidiWindow* getCurrentWindow() const;

    /** Opens the window on a clip with candidates already found (the screenshot scene). */
    HitsToMidiWindow* openWith (Track&, AudioClip&, std::vector<analysis::hits::Candidate>);

private:
    class Job;

    void finished (bool cancelled);

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    juce::Component& parent;
    std::unique_ptr<Job> job;
    std::vector<std::pair<const AudioClip*, juce::Component::SafePointer<HitsToMidiWindow>>> windows;

    JUCE_DECLARE_NON_COPYABLE (HitsToMidiActions)
};
} // namespace daw
