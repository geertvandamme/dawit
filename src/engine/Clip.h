#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <memory>
#include <vector>

#include "TempoMap.h"

namespace daw
{
/** A note inside a MIDI clip. Times are in beats, relative to the clip start. */
struct MidiNote
{
    int pitch = 60;
    double start = 0.0;
    double length = 1.0;
    float velocity = 0.8f;

    double getEnd() const noexcept { return start + length; }
};

/** Base class for anything that sits on a track's timeline. Positions are in beats. */
class Clip
{
public:
    enum class Kind { audio, midi };

    explicit Clip (Kind k) : kind (k) {}
    Clip (const Clip&) = default;
    virtual ~Clip() = default;

    /** How many beats it lasts: for audio, its seconds at the tempo where it plays. */
    virtual double getLengthBeats (const TempoMap&) const = 0;
    double getEndBeat (const TempoMap& tempo) const { return startBeat + getLengthBeats (tempo); }

    const Kind kind;
    juce::String name;
    double startBeat = 0.0;

    // A muted clip stays where it is but doesn't play. Use Engine::setClipMuted().
    bool muted = false;
};

/** An audio file loaded fully into memory. Its length in beats follows the tempo. */
class AudioClip final : public Clip
{
public:
    AudioClip() : Clip (Kind::audio) {}

    double getLengthSeconds() const noexcept
    {
        return sourceSampleRate > 0.0 ? audio.getNumSamples() / sourceSampleRate : 0.0;
    }

    double getLengthBeats (const TempoMap& tempo) const override { return tempo.getBeatAfter (startBeat, getLengthSeconds()) - startBeat; }

    /** Pre-computes a min/max overview used to draw the waveform. */
    void buildPeaks()
    {
        const int numSamples = audio.getNumSamples();
        const int numBlocks = (numSamples + peakBlockSize - 1) / peakBlockSize;
        peakMin.assign ((size_t) numBlocks, 0.0f);
        peakMax.assign ((size_t) numBlocks, 0.0f);

        for (int b = 0; b < numBlocks; ++b)
        {
            const int start = b * peakBlockSize;
            const int num = juce::jmin (peakBlockSize, numSamples - start);

            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
            {
                const auto range = juce::FloatVectorOperations::findMinAndMax (audio.getReadPointer (ch, start), num);
                peakMin[(size_t) b] = juce::jmin (peakMin[(size_t) b], range.getStart());
                peakMax[(size_t) b] = juce::jmax (peakMax[(size_t) b], range.getEnd());
            }
        }
    }

    /** The clip's gain as a factor, from gainDb. */
    float getGain() const noexcept { return juce::Decibels::decibelsToGain (gainDb); }

    static constexpr int peakBlockSize = 256;
    static constexpr float minGainDb = -24.0f, maxGainDb = 12.0f;

    // Makes the clip louder or quieter before the track's effects and fader. Use Engine::setClipGain().
    float gainDb = 0.0f;

    juce::File file;
    juce::AudioBuffer<float> audio;     // 1 or 2 channels at sourceSampleRate
    double sourceSampleRate = 44100.0;
    std::vector<float> peakMin, peakMax;
};

class MidiClip final : public Clip
{
public:
    MidiClip() : Clip (Kind::midi) {}

    double getLengthBeats (const TempoMap&) const override { return lengthBeats; }

    double lengthBeats = 16.0;
    std::vector<MidiNote> notes;
};

/** Takes: audio clips of one track that cover the same stretch of time, of which one plays and the others are muted
    (what recording over a clip, or loop recording, leaves behind). Pure functions on the track's clips. */
namespace takes
{
    using ClipList = std::vector<std::unique_ptr<Clip>>;

    /** The audio clips of the list that overlap `clip` in time (`clip` too), in order of start, and for the same start in
        the order of the list (the order they were recorded in). Clips that only touch don't overlap. */
    inline std::vector<AudioClip*> findOverlapping (const ClipList& clips, const AudioClip& clip, const TempoMap& tempo)
    {
        std::vector<AudioClip*> found;
        const double start = clip.startBeat, end = clip.getEndBeat (tempo);

        for (auto& c : clips)
            if (c->kind == Clip::Kind::audio && c->startBeat < end - 1.0e-9 && start < c->getEndBeat (tempo) - 1.0e-9)
                found.push_back (static_cast<AudioClip*> (c.get()));

        std::stable_sort (found.begin(), found.end(), [] (const AudioClip* a, const AudioClip* b) { return a->startBeat < b->startBeat; });
        return found;
    }

    /** Makes `chosen` the take that plays: it is unmuted and every other clip that overlaps it is muted. Returns
        the clips whose muted state changed. */
    inline std::vector<AudioClip*> choose (const ClipList& clips, AudioClip& chosen, const TempoMap& tempo)
    {
        std::vector<AudioClip*> changed;

        for (auto* other : findOverlapping (clips, chosen, tempo))
        {
            const bool shouldMute = other != &chosen;

            if (other->muted != shouldMute)
            {
                other->muted = shouldMute;
                changed.push_back (other);
            }
        }

        return changed;
    }

    /** The other takes of `chosen`: the clips that overlap it, without it. */
    inline std::vector<AudioClip*> findOthers (const ClipList& clips, const AudioClip& chosen, const TempoMap& tempo)
    {
        auto found = findOverlapping (clips, chosen, tempo);
        found.erase (std::remove (found.begin(), found.end(), &chosen), found.end());
        return found;
    }
}
} // namespace daw
