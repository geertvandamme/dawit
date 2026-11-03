#pragma once

#include "engine/Engine.h"
#include "generate/SingingGuide.h"

namespace daw
{
/** "Measure singing range..." for an audio clip: finds the notes the singer held in the background (a progress window
    with Cancel), then hands the measurement to `onMeasured`, which opens the singing guide at *Measuring your range*.
    It never changes the song or the clip. */
class RangeMeasurer
{
public:
    RangeMeasurer (Engine&, juce::Component& parent);
    ~RangeMeasurer();

    /** Starts measuring (nothing happens while one runs, or for an empty clip). */
    void measure (Track&, AudioClip&);

    /** Measures a clip now on this thread, with no window (the screenshots use it). Not present when the clip is empty. */
    static gen::singing::Measurement measureNow (const AudioClip&);

    /** Called on the message thread with the finished measurement. */
    std::function<void (const gen::singing::Measurement&)> onMeasured;

private:
    class Job;

    void finished (bool cancelled);

    Engine& engine;
    juce::Component& parent;
    std::unique_ptr<Job> job;

    JUCE_DECLARE_NON_COPYABLE (RangeMeasurer)
};
} // namespace daw
