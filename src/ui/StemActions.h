#pragma once

#include "engine/Engine.h"
#include "engine/Stems.h"

namespace daw
{
/** "Split into stems" and "Remove vocals (karaoke version)" for an audio clip: offers to download
    the Demucs model the first time, separates the clip in the background behind a progress window
    (which blocks editing until it's done or cancelled), then adds the stems as new tracks below the
    clip's track, or the song without its vocals and the vocals on a muted or quiet track. */
class StemSplitter
{
public:
    StemSplitter (Engine&, juce::Component& parent);
    ~StemSplitter();

    void split (Track&, AudioClip&);

    /** Asks for the instrumental as one clip or as stems and how loud the guide vocals are
        (remembered in the settings), then makes the karaoke version. */
    void removeVocals (Track&, AudioClip&);

    /** Called with the new tracks once the stems or the karaoke version are in. */
    std::function<void (const std::vector<Track*>&)> onStemsAdded;

private:
    class Job;

    /** Asks to download the model when it isn't there yet, then starts. */
    void downloadThenStart (Track&, AudioClip&, std::optional<stems::KaraokeChoices>);
    void start (Track&, AudioClip&, std::optional<stems::KaraokeChoices>);
    void finished (bool cancelled);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (StemSplitter)
};
} // namespace daw
