#include "HitsToMidi.h"

#include "BuiltInDrums.h"

#include <algorithm>
#include <cmath>

namespace daw::hitstomidi
{
namespace
{
    using analysis::hits::Drum;

    drums::Drum toKitDrum (Drum d)
    {
        switch (d)
        {
            case Drum::kick:  return drums::Drum::kick;
            case Drum::snare: return drums::Drum::snare;
            case Drum::hihat: return drums::Drum::closedHat;
        }

        return drums::Drum::snare;
    }

    /** A hit of the starting kit as mono, scaled so its peak is `peak`. */
    std::vector<float> renderScaled (drums::Drum drum, double rate, double peak)
    {
        const drums::Settings settings;
        const auto hit = drums::render (settings, drum, rate, juce::jmin (0.6, drums::getLength (settings.drums[(size_t) drum], drum)));
        std::vector<float> out (hit.tone.size());
        float top = 1.0e-6f;

        // Scaled by the peak of its first 30 ms, which is what the hit finder measures as its level.
        for (size_t i = 0; i < out.size(); ++i)
        {
            out[i] = hit.tone[i] + hit.noise[i];

            if (i < (size_t) (0.030 * rate))
                top = juce::jmax (top, std::abs (out[i]));
        }

        for (auto& v : out)
            v = (float) (v / top * peak);

        return out;
    }

    void mixIn (juce::AudioBuffer<float>& buffer, const std::vector<float>& hit, int start, float gain)
    {
        for (int c = 0; c < buffer.getNumChannels(); ++c)
        {
            auto* d = buffer.getWritePointer (c);

            for (int i = 0; i < (int) hit.size() && start + i < buffer.getNumSamples(); ++i)
                if (start + i >= 0)
                    d[start + i] += hit[(size_t) i] * gain;
        }
    }

    /** Settles the plugin's state so a project saved and opened again is byte for byte the same. */
    void stabiliseState (juce::AudioPluginInstance& plugin)
    {
        juce::MemoryBlock before, after;
        plugin.getStateInformation (before);

        for (int pass = 0; pass < 16; ++pass)
        {
            plugin.setStateInformation (before.getData(), (int) before.getSize());
            plugin.getStateInformation (after);

            if (after == before)
                return;

            before = after;
        }
    }
}

//==============================================================================
std::vector<MidiNote> toClipNotes (const std::vector<analysis::hits::Hit>& hits, const TempoMap& tempo, double clipStartBeat,
                                   double clipLengthBeats, const Timing& timing)
{
    std::vector<MidiNote> notes;

    for (const auto& hit : hits)
    {
        double start = tempo.getBeatAfter (clipStartBeat, hit.time) - clipStartBeat;

        if (timing.snap && timing.gridBeats > 0.0)
        {
            const double absolute = clipStartBeat + start;
            const double nearest = std::round (absolute / timing.gridBeats) * timing.gridBeats;
            start += timing.strength * (nearest - absolute);
        }

        start = juce::jmax (0.0, start);

        if (start >= clipLengthBeats - 1.0e-9)
            continue;

        notes.push_back ({ analysis::hits::getNoteForDrum (hit.drum), start, juce::jmin (noteLengthBeats, clipLengthBeats - start),
                           (float) hit.velocity / 127.0f });
    }

    std::stable_sort (notes.begin(), notes.end(), [] (const MidiNote& a, const MidiNote& b)
    {
        return a.pitch != b.pitch ? a.pitch < b.pitch : a.start < b.start;
    });

    for (size_t i = 0; i + 1 < notes.size(); ++i)
        if (notes[i].pitch == notes[i + 1].pitch && notes[i + 1].start < notes[i].getEnd())
            notes[i].length = juce::jmax (1.0e-6, notes[i + 1].start - notes[i].start);

    std::stable_sort (notes.begin(), notes.end(), [] (const MidiNote& a, const MidiNote& b) { return a.start < b.start; });
    return notes;
}

MidiClip* addDrumTrack (Engine& engine, Track& source, AudioClip& audio, std::vector<MidiNote> notes, bool muteOriginal)
{
    if (notes.empty())
        return nullptr;

    auto* track = engine.addTrack (Track::Type::midi, engine.indexOf (&source) + 1);
    track->name = source.name + " hits";

    juce::String error;

    if (auto* slot = engine.addPlugin (*track, BuiltInDrums::getDescription(), error))
    {
        if (slot->instance != nullptr)
            stabiliseState (*slot->instance);
    }
    else
    {
        engine.removeTrack (track);
        return nullptr;
    }

    const double length = audio.getLengthBeats (engine.getTempoMap());
    auto* clip = engine.addMidiClip (*track, audio.startBeat, length);
    {
        const juce::ScopedLock sl (engine.getLock());
        clip->name = audio.name;
        clip->lengthBeats = length;
        clip->notes = std::move (notes);
    }

    engine.notesChanged();

    if (muteOriginal)
        engine.setClipMuted (audio, true);

    return clip;
}

juce::AudioBuffer<float> renderListen (const AudioClip& clip, const std::vector<MidiNote>& notes, const TempoMap& tempo,
                                       double clipStartBeat, double sampleRate, Hear hear)
{
    const int length = (int) std::ceil (clip.getLengthSeconds() * sampleRate);
    juce::AudioBuffer<float> out (2, juce::jmax (1, length));
    out.clear();

    if (hear != Hear::hits && clip.audio.getNumChannels() > 0 && clip.sourceSampleRate > 0.0)
    {
        const double ratio = clip.sourceSampleRate / sampleRate;
        const float gain = clip.getGain();

        for (int c = 0; c < 2; ++c)
        {
            const auto* in = clip.audio.getReadPointer (juce::jmin (c, clip.audio.getNumChannels() - 1));
            auto* d = out.getWritePointer (c);

            for (int i = 0; i < length; ++i)
            {
                const double pos = i * ratio;
                const int a = (int) pos;

                if (a >= clip.audio.getNumSamples())
                    break;

                const float frac = (float) (pos - a);
                const float next = a + 1 < clip.audio.getNumSamples() ? in[a + 1] : in[a];
                d[i] = gain * (in[a] + frac * (next - in[a]));
            }
        }
    }

    if (hear != Hear::original)
    {
        const drums::Settings settings;
        const double t0 = tempo.beatToSeconds (clipStartBeat);

        for (const auto& n : notes)
        {
            const auto drum = drums::getDrumForNote (n.pitch);

            if (! drum.has_value())
                continue;

            const auto sound = drums::render (settings, *drum, sampleRate,
                                              juce::jmin (0.6, drums::getLength (settings.drums[(size_t) *drum], *drum)), n.velocity);
            std::vector<float> mono (sound.tone.size());

            for (size_t i = 0; i < mono.size(); ++i)
                mono[i] = sound.tone[i] + sound.noise[i];

            mixIn (out, mono, (int) std::lround ((tempo.beatToSeconds (clipStartBeat + n.start) - t0) * sampleRate), 1.0f);
        }
    }

    return out;
}

//==============================================================================
const std::vector<ExampleHit>& getExampleLoop()
{
    static const std::vector<ExampleHit> loop = []
    {
        std::vector<ExampleHit> h;

        for (double beat : { 0.0, 2.0, 4.0, 6.0, 6.75 })
            h.push_back ({ beat, Drum::kick, 0.0, 0.0 });

        for (double beat : { 1.0, 3.0, 5.0, 7.0 })
            h.push_back ({ beat, Drum::snare, -2.0, 6.0 });

        h.push_back ({ 3.75, Drum::snare, -24.0, 0.0 });

        for (int i = 0; i < 8; ++i)
            h.push_back ({ i + 0.5, Drum::hihat, i % 2 == 0 ? -14.0 : -16.0, (double) (i % 5) * 2.0 });

        h.push_back ({ 4.0 + 0.012 / 0.5, Drum::kick, -12.0, 0.0 });   // the bounce, 12 ms after beat 4
        std::stable_sort (h.begin(), h.end(), [] (const ExampleHit& a, const ExampleHit& b) { return a.beat < b.beat; });
        return h;
    }();

    return loop;
}

juce::AudioBuffer<float> renderExampleLoop (double sampleRate)
{
    juce::AudioBuffer<float> out (1, (int) std::lround (4.0 * sampleRate));
    out.clear();

    for (const auto& h : getExampleLoop())
    {
        const auto hit = renderScaled (toKitDrum (h.drum), sampleRate, 0.8 * juce::Decibels::decibelsToGain (h.levelDb));
        mixIn (out, hit, (int) std::lround ((h.beat * 0.5 + h.lateMs / 1000.0) * sampleRate), 1.0f);
    }

    return out;
}
} // namespace daw::hitstomidi
