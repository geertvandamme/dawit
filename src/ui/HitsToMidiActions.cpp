#include "HitsToMidiActions.h"
#include "HitsToMidiWindow.h"

namespace daw
{
class HitsToMidiActions::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (HitsToMidiActions& o, Track& t, AudioClip& c)
        : juce::ThreadWithProgressWindow ("Hits to MIDI", true, true, 10000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), audio (c.audio), sampleRate (c.sourceSampleRate)
    {
        setStatusMessage ("Finding the hits in \"" + c.name + "\"...");
    }

    void run() override
    {
        candidates = analysis::hits::findCandidates (audio, sampleRate, [this] { return threadShouldExit(); },
                                                     [this] (double done) { setProgress (done); });
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    HitsToMidiActions& owner;
    Track* track;
    AudioClip* clip;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sampleRate;
    std::vector<analysis::hits::Candidate> candidates;
};

//==============================================================================
HitsToMidiActions::HitsToMidiActions (Engine& e, Selection& s, PluginWindowManager& w, juce::Component& p)
    : engine (e), selection (s), pluginWindows (w), parent (p)
{
}

HitsToMidiActions::~HitsToMidiActions() = default;

HitsToMidiWindow* HitsToMidiActions::getWindowFor (const AudioClip& clip) const
{
    for (const auto& w : windows)
        if (w.first == &clip)
            return w.second.getComponent();

    return nullptr;
}

HitsToMidiWindow* HitsToMidiActions::getCurrentWindow() const
{
    for (auto it = windows.rbegin(); it != windows.rend(); ++it)
        if (auto* w = it->second.getComponent())
            return w;

    return nullptr;
}

void HitsToMidiActions::open (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    if (getWindowFor (clip) != nullptr)
    {
        pluginWindows.show (&clip, {}, [] { return nullptr; });   // already open: to the front
        return;
    }

    job = std::make_unique<Job> (*this, track, clip);
    job->launchThread();
}

void HitsToMidiActions::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    if (cancelled || engine.findTrackFor (done->clip) != done->track)
        return;

    openWith (*done->track, *done->clip, std::move (done->candidates));
}

HitsToMidiWindow* HitsToMidiActions::openWith (Track& track, AudioClip& clip, std::vector<analysis::hits::Candidate> candidates)
{
    windows.erase (std::remove_if (windows.begin(), windows.end(), [] (const auto& w) { return w.second == nullptr; }), windows.end());

    HitsToMidiWindow* shown = nullptr;
    const void* key = &clip;

    pluginWindows.show (key, "Hits to MIDI: \"" + clip.name + "\"", [&]
    {
        auto* window = new HitsToMidiWindow (engine, track, clip, std::move (candidates));
        window->onShowGuide = [this] (triggerguide::Topic topic) { if (onShowGuide) onShowGuide (topic); };
        window->onClose = [this, key]
        {
            // Closing from inside the window's own callback: leave it until the call has returned.
            juce::MessageManager::callAsync ([this, key] { pluginWindows.closeFor (key); });
        };
        window->onMade = [this] (Track& newTrack, MidiClip& newClip) { selection.select (&newTrack, &newClip); };
        shown = window;
        return window;
    }, { 760, 480 });

    if (shown != nullptr)
        windows.emplace_back (&clip, juce::Component::SafePointer<HitsToMidiWindow> (shown));

    return shown;
}
} // namespace daw
