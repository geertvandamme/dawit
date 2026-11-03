#include "SingingRangeActions.h"

namespace daw
{
namespace rf = analysis::rangefinder;

class RangeMeasurer::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (RangeMeasurer& o, Track& t, AudioClip& c)
        : juce::ThreadWithProgressWindow ("Measure singing range", true, true, 10000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), name (c.name), audio (c.audio), sourceSampleRate (c.sourceSampleRate)
    {
        setStatusMessage ("Listening for your notes in \"" + c.name + "\"...");
    }

    void run() override
    {
        result = rf::measure (audio, sourceSampleRate, [this] (double done)
        {
            setProgress (done);
            return ! threadShouldExit();
        });
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    RangeMeasurer& owner;
    Track* track;
    AudioClip* clip;
    juce::String name;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sourceSampleRate;
    std::optional<rf::Result> result;
};

//==============================================================================
RangeMeasurer::RangeMeasurer (Engine& e, juce::Component& p) : engine (e), parent (p) {}

RangeMeasurer::~RangeMeasurer() = default;

void RangeMeasurer::measure (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    job = std::make_unique<Job> (*this, track, clip);
    job->launchThread();
}

gen::singing::Measurement RangeMeasurer::measureNow (const AudioClip& clip)
{
    gen::singing::Measurement m;

    if (clip.audio.getNumSamples() == 0)
        return m;

    if (auto result = rf::measure (clip.audio, clip.sourceSampleRate))
    {
        m.present = true;
        m.clipName = clip.name;
        m.result = std::move (*result);
    }

    return m;
}

void RangeMeasurer::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    // The clip may have been deleted while it ran.
    if (cancelled || ! done->result.has_value() || engine.findTrackFor (done->clip) != done->track)
        return;

    gen::singing::Measurement m;
    m.present = true;
    m.clipName = done->name;
    m.result = std::move (*done->result);

    if (onMeasured)
        onMeasured (m);
}
} // namespace daw
