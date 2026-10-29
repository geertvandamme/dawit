#pragma once

#include "analysis/StructureFinder.h"
#include "analysis/TempoFinder.h"
#include "engine/Engine.h"

namespace daw
{
/** "Detect song structure..." for an audio clip: finds its parts (intro, verse, chorus, bridge...)
    on the song's bars in the background, shows them with why each got its name, and puts them on
    the Sections row over the clip's bars when asked. It also checks the recording's tempo and
    downbeats against the song's bars, and advises Detect tempo first when they don't line up. */
class StructureDetector
{
public:
    StructureDetector (Engine&, juce::Component& parent);
    ~StructureDetector();

    void detect (Track&, AudioClip&);

    /** Called when "Detect tempo first" is chosen. */
    std::function<void (Track&, AudioClip&)> onDetectTempo;

    /** Called after the sections went on the Sections row. */
    std::function<void()> onSectionsSet;

private:
    class Job;

    void finished (bool cancelled);
    void offer (Track&, AudioClip&, const std::vector<analysis::structurefinder::FoundSection>&,
                const std::optional<analysis::tempofinder::Tempo>&, const TempoMap& songTempo);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (StructureDetector)
};
} // namespace daw
