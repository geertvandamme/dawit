#pragma once

#include "analysis/ChordFinder.h"
#include "analysis/TempoFinder.h"
#include "engine/Engine.h"

namespace daw
{
/** "Detect chords..." for an audio clip: finds a chord per bar of the song and the key in the
    background, shows them, and puts them on the chord track over the clip's bars (and the key on
    the song) when asked. It also checks the recording's tempo and downbeats against the song's
    bars, and advises Detect tempo first when they don't line up. */
class ChordDetector
{
public:
    ChordDetector (Engine&, juce::Component& parent);
    ~ChordDetector();

    void detect (Track&, AudioClip&);

    /** Called when "Detect tempo first" is chosen. */
    std::function<void (Track&, AudioClip&)> onDetectTempo;

    /** Called after the chords went on the chord track. */
    std::function<void()> onChordsSet;

private:
    class Job;

    void finished (bool cancelled);
    void offer (Track&, AudioClip&, const analysis::chordfinder::Result&, const std::optional<analysis::tempofinder::Tempo>&, const TempoMap& songTempo);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (ChordDetector)
};
} // namespace daw
