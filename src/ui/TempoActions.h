#pragma once

#include "analysis/TempoFinder.h"
#include "engine/Engine.h"

namespace daw
{
/** "Detect tempo..." for an audio clip: finds its tempo and beats in the background, then says what
    it found (steady, or drifting like a band without a click) and how, offers half and double the
    tempo to choose from, and sets the song's tempo to it, moving the clip so its first downbeat
    lands on a bar line if you like. For a drifting recording it first offers to follow it bar by
    bar (a tempo per bar, see tempofinder::followRecording). */
class TempoDetector
{
public:
    TempoDetector (Engine&, juce::Component& parent);
    ~TempoDetector();

    void detect (Track&, AudioClip&);

    /** Called after the song's tempo was set. */
    std::function<void()> onTempoSet;

private:
    class Job;

    void finished (bool cancelled);
    void offer (Track&, AudioClip&, const analysis::tempofinder::Tempo&);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (TempoDetector)
};

/** Whether a recording's beats and bars fall on the song's. With one song tempo: the same tempo as
    the recording (or half or double it), the recording steady, and its first downbeat within a
    third of a beat of a bar line. With tempo changes: at least 80% of its downbeats within a third
    of a beat of a bar line. When they don't, it returns advice to run Detect tempo first, saying
    what falls off the bars ("chords", "sections"); when they do, nothing. */
juce::String adviseLiningUp (const AudioClip&, const analysis::tempofinder::Tempo&, const TempoMap& songTempo, const juce::String& what);
} // namespace daw
