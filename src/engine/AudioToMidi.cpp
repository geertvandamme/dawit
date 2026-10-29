#include "AudioToMidi.h"

#include "OnnxEnvironment.h"

#include <DawItModels.h>
#include <onnxruntime_cxx_api.h>

#include <queue>

namespace daw::audiotomidi
{
namespace
{
    // Basic Pitch's constants (basic_pitch/constants.py and inference.py).
    constexpr int fftHop = 256;
    constexpr int framesPerWindow = 172;                         // 2 s at 86 frames per second
    constexpr int windowSamples = 2 * 22050 - fftHop;            // 43844
    constexpr int overlapFrames = 30;
    constexpr int overlapSamples = overlapFrames * fftHop;
    constexpr int hopSamples = windowSamples - overlapSamples;   // 36164
    constexpr int framesPerSecond = 22050 / fftHop;              // 86
    constexpr int energyTolerance = 11;                          // frames below the threshold before a note ends
    constexpr int windowsPerRun = 8;

    const char* const inputName = "serving_default_input_2:0";
    const char* const noteOutput = "StatefulPartitionedCall:1";
    const char* const onsetOutput = "StatefulPartitionedCall:2";

    /** basic-pitch's get_infered_onsets: also count sudden rises in the note activations as onsets. */
    std::vector<float> inferOnsets (const Activations& a)
    {
        const int n = a.numFrames;
        std::vector<float> diff ((size_t) n * numPitches, 0.0f);
        float maxDiff = 0.0f, maxOnset = 0.0f;

        for (int t = 0; t < n; ++t)
        {
            for (int k = 0; k < numPitches; ++k)
            {
                maxOnset = juce::jmax (maxOnset, a.onset (t, k));

                if (t < 2)
                    continue;   // the first two frames have no rise

                const float rise = juce::jmin (a.note (t, k) - a.note (t - 1, k), a.note (t, k) - a.note (t - 2, k));
                diff[(size_t) (t * numPitches + k)] = juce::jmax (0.0f, rise);
                maxDiff = juce::jmax (maxDiff, rise);
            }
        }

        std::vector<float> onsets (a.onsets);

        if (maxDiff > 0.0f)
            for (size_t i = 0; i < onsets.size(); ++i)
                onsets[i] = juce::jmax (onsets[i], maxOnset * diff[i] / maxDiff);

        return onsets;
    }

    void clearAround (std::vector<float>& energy, int frame, int key)
    {
        energy[(size_t) (frame * numPitches + key)] = 0.0f;
        if (key < numPitches - 1) energy[(size_t) (frame * numPitches + key + 1)] = 0.0f;
        if (key > 0)              energy[(size_t) (frame * numPitches + key - 1)] = 0.0f;
    }

    float meanActivation (const Activations& a, int key, int from, int to)
    {
        double sum = 0.0;
        for (int t = from; t < to; ++t)
            sum += a.note (t, key);

        return (float) (sum / juce::jmax (1, to - from));
    }
}

//==============================================================================
std::vector<float> prepareInput (const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    const int numIn = audio.getNumSamples();
    const int numChannels = audio.getNumChannels();

    juce::AudioBuffer<float> mono (1, numIn);
    mono.clear();
    for (int ch = 0; ch < numChannels; ++ch)
        mono.addFrom (0, 0, audio, ch, 0, numIn, 1.0f / (float) numChannels);

    if (sourceSampleRate <= 0.0 || juce::exactlyEqual (sourceSampleRate, sampleRate))
        return { mono.getReadPointer (0), mono.getReadPointer (0) + numIn };

    // ResamplingAudioSource filters out what is too high for the new rate before going down.
    const double ratio = sourceSampleRate / sampleRate;
    const int numOut = juce::roundToInt (numIn / ratio);

    juce::MemoryAudioSource source (mono, false);
    juce::ResamplingAudioSource resampler (&source, false, 1);
    resampler.setResamplingRatio (ratio);
    resampler.prepareToPlay (8192, sampleRate);

    juce::AudioBuffer<float> out (1, numOut);
    for (int pos = 0; pos < numOut; pos += 8192)
        resampler.getNextAudioBlock (juce::AudioSourceChannelInfo (&out, pos, juce::jmin (8192, numOut - pos)));

    return { out.getReadPointer (0), out.getReadPointer (0) + numOut };
}

std::optional<Activations> runModel (const std::vector<float>& audio, const ProgressFn& progress, juce::String& error)
{
    // Half an overlap of silence in front, then windows of 43844 samples that start 36164 apart.
    std::vector<float> padded ((size_t) (overlapSamples / 2), 0.0f);
    padded.insert (padded.end(), audio.begin(), audio.end());

    const int numWindows = juce::jmax (1, ((int) padded.size() + hopSamples - 1) / hopSamples);
    constexpr int keptFrames = framesPerWindow - overlapFrames;   // half the overlap is cut from each side
    const int numFrames = (int) std::floor ((double) audio.size() * framesPerSecond / sampleRate);

    Activations result;
    result.numFrames = juce::jmin (numFrames, numWindows * keptFrames);
    result.notes.assign ((size_t) result.numFrames * numPitches, 0.0f);
    result.onsets.assign ((size_t) result.numFrames * numPitches, 0.0f);

    try
    {
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads (juce::jmax (1, juce::jmin (8, juce::SystemStats::getNumPhysicalCpus())));
        options.SetGraphOptimizationLevel (GraphOptimizationLevel::ORT_ENABLE_ALL);
        Ort::Session session (onnx::getEnvironment(), dawmodels::nmp_onnx, (size_t) dawmodels::nmp_onnxSize, options);
        const auto memory = Ort::MemoryInfo::CreateCpu (OrtArenaAllocator, OrtMemTypeDefault);

        for (int first = 0; first < numWindows; first += windowsPerRun)
        {
            if (! progress ((double) first / numWindows))
                return std::nullopt;

            const int count = juce::jmin (windowsPerRun, numWindows - first);
            std::vector<float> input ((size_t) count * windowSamples, 0.0f);

            for (int w = 0; w < count; ++w)
            {
                const size_t start = (size_t) (first + w) * hopSamples;
                const size_t available = start < padded.size() ? juce::jmin ((size_t) windowSamples, padded.size() - start) : 0;
                std::copy_n (padded.begin() + (std::ptrdiff_t) start, available, input.begin() + (std::ptrdiff_t) ((size_t) w * windowSamples));
            }

            const std::array<int64_t, 3> shape { count, windowSamples, 1 };
            auto tensor = Ort::Value::CreateTensor<float> (memory, input.data(), input.size(), shape.data(), shape.size());
            const char* outputs[] = { noteOutput, onsetOutput };
            auto out = session.Run (Ort::RunOptions { nullptr }, &inputName, &tensor, 1, outputs, 2);

            const float* notes = out[0].GetTensorData<float>();
            const float* onsets = out[1].GetTensorData<float>();

            for (int w = 0; w < count; ++w)
            {
                for (int f = 0; f < keptFrames; ++f)
                {
                    const int frame = (first + w) * keptFrames + f;
                    if (frame >= result.numFrames)
                        break;

                    const size_t from = ((size_t) w * framesPerWindow + (size_t) (f + overlapFrames / 2)) * numPitches;
                    std::copy_n (notes + from, numPitches, result.notes.begin() + (std::ptrdiff_t) ((size_t) frame * numPitches));
                    std::copy_n (onsets + from, numPitches, result.onsets.begin() + (std::ptrdiff_t) ((size_t) frame * numPitches));
                }
            }
        }
    }
    catch (const Ort::Exception& e)
    {
        error = "The audio to MIDI model failed: " + juce::String (e.what());
        return std::nullopt;
    }

    progress (1.0);
    return result;
}

double frameToSeconds (int frame)
{
    // As in basic-pitch, including its correction per 172 frames and its "magic number".
    const double windowOffset = (double) fftHop / sampleRate * (framesPerWindow - (double) windowSamples / fftHop) + 0.0018;
    return frame * (double) fftHop / sampleRate - windowOffset * std::floor (frame / (double) framesPerWindow);
}

std::vector<Note> findNotes (const Activations& a, const Settings& settings)
{
    const int n = a.numFrames;
    const int minLength = juce::roundToInt (settings.minNoteSeconds * sampleRate / fftHop);
    const float frameThreshold = settings.frameThreshold;
    std::vector<Note> notes;

    if (n < 3)
        return notes;

    auto addNote = [&] (int from, int to, int key)
    {
        notes.push_back ({ frameToSeconds (from), frameToSeconds (to), key + lowestPitch, meanActivation (a, key, from, to) });
    };

    // Onsets are the peaks in time of the (inferred) onset activations above the threshold.
    const auto onsets = inferOnsets (a);
    auto onsetAt = [&] (int t, int k) { return onsets[(size_t) (t * numPitches + k)]; };

    std::vector<float> energy (a.notes);

    // Latest first, and the highest key first within a frame, as basic-pitch walks them.
    for (int start = n - 2; start >= 1; --start)
    {
        for (int key = numPitches - 1; key >= 0; --key)
        {
            const float o = onsetAt (start, key);
            if (! (o >= settings.onsetThreshold && o > onsetAt (start - 1, key) && o > onsetAt (start + 1, key)))
                continue;

            // Follow the note until its energy stays below the threshold for a while.
            int i = start + 1, below = 0;
            while (i < n - 1 && below < energyTolerance)
            {
                below = energy[(size_t) (i * numPitches + key)] < frameThreshold ? below + 1 : 0;
                ++i;
            }
            i -= below;

            if (i - start <= minLength)
                continue;

            for (int t = start; t < i; ++t)
                clearAround (energy, t, key);

            addNote (start, i, key);
        }
    }

    if (! settings.melodiaTrick)
        return notes;

    // The melodia trick: notes the onsets missed, starting from the strongest energy left. Energy
    // only ever drops to zero, so a heap with stale entries skipped gives the same order as
    // repeatedly taking the maximum (ties go to the earliest cell, like numpy's argmax).
    using Cell = std::pair<float, int>;
    auto weaker = [] (const Cell& x, const Cell& y) { return x.first < y.first || (juce::exactlyEqual (x.first, y.first) && x.second > y.second); };
    std::priority_queue<Cell, std::vector<Cell>, decltype (weaker)> strongest (weaker);

    for (int i = 0; i < (int) energy.size(); ++i)
        if (energy[(size_t) i] > frameThreshold)
            strongest.push ({ energy[(size_t) i], i });

    while (! strongest.empty())
    {
        const auto [value, cell] = strongest.top();
        strongest.pop();

        if (! juce::exactlyEqual (energy[(size_t) cell], value))
            continue;   // cleared since

        const int mid = cell / numPitches, key = cell % numPitches;
        energy[(size_t) cell] = 0.0f;

        int i = mid + 1, below = 0;
        while (i < n - 1 && below < energyTolerance)
        {
            below = energy[(size_t) (i * numPitches + key)] < frameThreshold ? below + 1 : 0;
            clearAround (energy, i, key);
            ++i;
        }
        const int end = i - 1 - below;

        i = mid - 1;
        below = 0;
        while (i > 0 && below < energyTolerance)
        {
            below = energy[(size_t) (i * numPitches + key)] < frameThreshold ? below + 1 : 0;
            clearAround (energy, i, key);
            --i;
        }
        const int start = i + 1 + below;

        if (end - start > minLength)
            addNote (start, end, key);
    }

    return notes;
}

std::vector<Note> keepOneAtATime (std::vector<Note> notes, double minNoteSeconds)
{
    std::stable_sort (notes.begin(), notes.end(), [] (const Note& x, const Note& y) { return x.amplitude > y.amplitude; });
    std::vector<Note> kept;

    for (auto note : notes)
    {
        // The free stretches of this note's time, between the stronger notes already kept.
        std::vector<std::pair<double, double>> free { { note.start, note.end } };

        for (auto& k : kept)
        {
            std::vector<std::pair<double, double>> rest;
            for (auto [from, to] : free)
            {
                if (k.end <= from || k.start >= to)
                {
                    rest.push_back ({ from, to });
                    continue;
                }
                if (k.start > from) rest.push_back ({ from, k.start });
                if (k.end < to)     rest.push_back ({ k.end, to });
            }
            free = std::move (rest);
        }

        const auto longest = std::max_element (free.begin(), free.end(), [] (auto& x, auto& y) { return x.second - x.first < y.second - y.first; });

        if (longest != free.end() && longest->second - longest->first >= minNoteSeconds)
        {
            note.start = longest->first;
            note.end = longest->second;
            kept.push_back (note);
        }
    }

    std::sort (kept.begin(), kept.end(), [] (const Note& x, const Note& y) { return x.start < y.start; });
    return kept;
}

//==============================================================================
std::vector<MidiNote> toClipNotes (const std::vector<Note>& notes, const TempoMap& tempo, double clipStartBeat, double clipLengthBeats, double gridBeats)
{
    std::vector<MidiNote> out;

    for (auto& note : notes)
    {
        double start = tempo.getBeatAfter (clipStartBeat, note.start) - clipStartBeat;
        double end = tempo.getBeatAfter (clipStartBeat, note.end) - clipStartBeat;

        if (gridBeats > 0.0)
        {
            start = std::round (start / gridBeats) * gridBeats;
            end = juce::jmax (start + gridBeats, std::round (end / gridBeats) * gridBeats);
        }

        start = juce::jmax (0.0, start);
        end = juce::jmin (end, clipLengthBeats);

        if (start >= clipLengthBeats || end <= start)
            continue;

        out.push_back ({ note.pitch, start, end - start, juce::jlimit (0.05f, 1.0f, note.amplitude) });
    }

    std::sort (out.begin(), out.end(), [] (const MidiNote& x, const MidiNote& y)
    {
        if (x.pitch != y.pitch)
            return x.pitch < y.pitch;

        return juce::exactlyEqual (x.start, y.start) ? x.length > y.length : x.start < y.start;
    });

    // A note of the same pitch that starts later cuts the earlier one short; one that starts at
    // the same time is dropped.
    std::vector<MidiNote> cleaned;
    for (auto& note : out)
    {
        if (! cleaned.empty() && cleaned.back().pitch == note.pitch && note.start < cleaned.back().getEnd())
        {
            if (juce::approximatelyEqual (note.start, cleaned.back().start))
                continue;

            cleaned.back().length = note.start - cleaned.back().start;
        }

        cleaned.push_back (note);
    }

    std::stable_sort (cleaned.begin(), cleaned.end(), [] (const MidiNote& x, const MidiNote& y) { return x.start < y.start; });
    return cleaned;
}

MidiClip* addMidiTrack (Engine& engine, Track& source, const AudioClip& audio, std::vector<MidiNote> notes)
{
    auto* track = engine.addTrack (Track::Type::midi, engine.indexOf (&source) + 1);
    track->name = source.name + " MIDI";

    const auto names = (source.name + " " + audio.name).toLowerCase();
    if (names.contains ("bass"))
        engine.setBuiltInSound (*track, BuiltInSound::bass);
    else if (names.contains ("vocal") || names.contains ("voice") || names.contains ("lead"))
        engine.setBuiltInSound (*track, BuiltInSound::lead);

    auto* clip = engine.addMidiClip (*track, audio.startBeat, audio.getLengthBeats (engine.getTempoMap()));
    {
        const juce::ScopedLock sl (engine.getLock());
        clip->name = audio.name;
        clip->lengthBeats = audio.getLengthBeats (engine.getTempoMap());
        clip->notes = std::move (notes);
    }

    engine.notesChanged();
    return clip;
}
} // namespace daw::audiotomidi
