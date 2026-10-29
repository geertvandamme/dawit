#include "Singer.h"
#include "Engine.h"

namespace daw::sing
{
std::optional<Rendering> renderPhrases (Singer& singer, const std::vector<Phrase>& phrases, double spanStartSeconds, double spanEndSeconds,
                                        int steps, const RenderProgressFn& progress, juce::String& error)
{
    Rendering out;
    out.originSeconds = spanStartSeconds - paddingSeconds;
    const int length = juce::jmax (1, juce::roundToInt ((spanEndSeconds - spanStartSeconds + 2.0 * paddingSeconds) * sampleRate));
    out.audio.setSize (1, length);
    out.audio.clear();

    bool cancelled = false;
    CancelFn cancel = [&]
    {
        cancelled = cancelled || ! progress (-1.0, {});   // -1: only asking whether to stop
        return cancelled;
    };

    for (size_t i = 0; i < phrases.size(); ++i)
    {
        const auto& phrase = phrases[i];

        if (! progress ((double) i / (double) juce::jmax<size_t> (1, phrases.size()),
                        "Singing phrase " + juce::String ((int) i + 1) + " of " + juce::String ((int) phrases.size()) + "..."))
            return std::nullopt;

        auto audio = singer.sing (phrase, steps, cancel, error);

        if (! audio.has_value())
            return std::nullopt;   // cancelled, or `error` says why

        // Cut the padding, then fade the edges a little so a phrase never clicks.
        const int cut = phrase.cutFrames * hopSize;
        const int available = audio->getNumSamples() - cut;
        const int start = juce::roundToInt ((phrase.startSeconds - out.originSeconds) * sampleRate) + cut;

        for (int s = 0; s < available; ++s)
        {
            const int dest = start + s;

            if (dest < 0 || dest >= length)
                continue;

            const float fade = juce::jmin (1.0f, (float) s / 220.0f, (float) (available - s) / 220.0f);
            out.audio.getWritePointer (0)[dest] += audio->getSample (0, cut + s) * fade;
        }
    }

    if (! progress (1.0, "Done"))
        return std::nullopt;

    return out;
}

Track* addVocalTrack (Engine& engine, Track& melodyTrack, Rendering rendering, const juce::String& voiceName, bool muteMelody)
{
    if (rendering.audio.getNumSamples() == 0)
        return nullptr;

    // A melody in the first half second would start the clip before the song does: cut what's before beat 0.
    double origin = rendering.originSeconds;

    if (origin < 0.0)
    {
        const int cut = juce::jmin (rendering.audio.getNumSamples() - 1, juce::roundToInt (-origin * sampleRate));
        juce::AudioBuffer<float> trimmed (1, rendering.audio.getNumSamples() - cut);
        trimmed.copyFrom (0, 0, rendering.audio, 0, cut, trimmed.getNumSamples());
        rendering.audio = std::move (trimmed);
        origin = 0.0;
    }

    const auto name = voiceName + juce::String::fromUTF8 (" \xc2\xb7 ") + melodyTrack.name;
    auto* track = engine.addTrack (Track::Type::audio, engine.indexOf (&melodyTrack) + 1);

    if (track == nullptr)
        return nullptr;

    track->name = name;
    engine.addAudioClip (*track, std::move (rendering.audio), (double) sampleRate, engine.getTempoMap().secondsToBeat (origin), name);

    if (muteMelody)
    {
        melodyTrack.muted = true;
        engine.sendChangeMessage();
    }

    return track;
}
} // namespace daw::sing
