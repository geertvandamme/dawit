#pragma once

#include "analysis/LyricsFinder.h"
#include "engine/Engine.h"

namespace daw
{
/** "Detect lyrics (English only)" for an audio clip: asks whether the clip is only vocals and
    which Whisper model to use (saying what has to be downloaded the first time), finds the sung
    words in the background behind a progress window, then puts them on the lyrics track over the
    clip's bars, each syllable pinned where it's sung. From a full mix, the vocals Demucs finds are
    kept on a new muted track below the clip. Works for English vocals only for now. */
class LyricsDetector
{
public:
    LyricsDetector (Engine&, juce::Component& parent);
    ~LyricsDetector();

    void detect (Track&, AudioClip&);

    /** Called once the words are on the lyrics track. */
    std::function<void()> onLyricsFound;

private:
    struct Choices
    {
        bool onlyVocals = false;   // otherwise Demucs finds the vocals first
        bool snapToSinging = true; // move word starts to the singing's onsets
        analysis::lyricsfinder::ModelSize model = analysis::lyricsfinder::ModelSize::small;
    };

    class Job;

    void start (Track&, AudioClip&, Choices);
    void finished (bool cancelled);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (LyricsDetector)
};
} // namespace daw
