#pragma once

#include "engine/Engine.h"

namespace daw
{
/** "Convert to MIDI" for an audio clip: asks a few questions, finds the notes with Basic Pitch
    in the background behind a progress window, then puts them in a new MIDI track below the
    clip's track. */
class AudioToMidiConverter
{
public:
    AudioToMidiConverter (Engine&, juce::Component& parent);
    ~AudioToMidiConverter();

    void convert (Track&, AudioClip&);

    /** Called with the new track and clip once the notes are in. */
    std::function<void (Track&, MidiClip&)> onConverted;

private:
    struct Choices
    {
        bool oneAtATime = false;
        int sensitivity = 1;      // 0 fewer notes, 1 normal, 2 more notes
        double gridBeats = 0.0;   // 0: not quantised
    };

    class Job;

    void start (Track&, AudioClip&, Choices);
    void finished (bool cancelled);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (AudioToMidiConverter)
};
} // namespace daw
