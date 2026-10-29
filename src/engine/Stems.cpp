#include "Stems.h"
#include "ModelDownload.h"

#include <model.hpp>

#include <thread>

namespace daw::stems
{
namespace
{
    // htdemucs (Demucs v4, four stems) converted for demucs.cpp, at a fixed revision of the dataset.
    const models::Model model { "ggml-model-htdemucs-4s-f16.bin",
                                "https://huggingface.co/datasets/Retrobear/demucs.cpp/resolve/"
                                "8f58ac0491bbea657275bcd3e38af1ab3a27bfc9/ggml-model-htdemucs-4s-f16.bin",
                                "72b17c42d308982ddb5069bc3bf48b81a5aac4cb6516e4366c0fa7cef6df0064",
                                83994361 };

    // Pieces overlap by this much on each side, and are cross-faded there.
    constexpr int pieceOverlap = (int) (sampleRate * 0.5);

    // demucs.cpp pads a piece by up to half a second and runs the model on 7.8 s segments that
    // advance by 5.85 s, so a piece up to about 5.3 s long takes a single run of the model.
    // Shorter cores keep each piece to one run; the overlap comes on top.
    constexpr double pieceCoreSeconds = 5.3 - 2 * 0.5;

    // Thrown from the demucs.cpp progress callback to stop a separation part-way.
    struct Cancelled {};
}

//==============================================================================
juce::File getModelFile()
{
    return models::getFile (model);
}

bool isModelInstalled()
{
    return models::isInstalled (model);
}

juce::int64 getModelDownloadSize()
{
    return model.size;
}

juce::String downloadModel (const ProgressFn& progress)
{
    return models::download (model, progress);
}

//==============================================================================
juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    const int numIn = audio.getNumSamples();
    const double ratio = sourceSampleRate / sampleRate;
    const bool resample = sourceSampleRate > 0.0 && ! juce::exactlyEqual (ratio, 1.0);
    const int numOut = resample ? juce::roundToInt (numIn / ratio) : numIn;

    juce::AudioBuffer<float> stereo (2, numOut);

    for (int ch = 0; ch < 2; ++ch)
    {
        // Mono goes to both sides.
        const float* src = audio.getReadPointer (juce::jmin (ch, audio.getNumChannels() - 1));

        if (resample)
        {
            juce::LagrangeInterpolator resampler;
            resampler.process (ratio, src, stereo.getWritePointer (ch), numOut, numIn, 0);
        }
        else
        {
            stereo.copyFrom (ch, 0, src, numOut);
        }
    }

    return stereo;
}

std::optional<Stems> separateInPieces (const juce::AudioBuffer<float>& input, int numPieces,
                                       const SeparateFn& separate, const ProgressFn& progress)
{
    const int total = input.getNumSamples();
    numPieces = juce::jlimit (1, juce::jmax (1, total), numPieces);
    const int pieceLength = (total + numPieces - 1) / numPieces;

    struct Piece
    {
        int coreStart = 0, coreEnd = 0;   // the samples this piece is responsible for
        int start = 0, end = 0;           // with the overlap on both sides
        std::optional<Stems> result;      // read once its thread has finished
        std::atomic<double> progress { 0.0 };
        std::atomic<bool> finished { false };
    };

    std::vector<std::unique_ptr<Piece>> pieces;

    for (int i = 0; i < numPieces; ++i)
    {
        auto piece = std::make_unique<Piece>();
        piece->coreStart = juce::jmin (total, i * pieceLength);
        piece->coreEnd = juce::jmin (total, piece->coreStart + pieceLength);
        piece->start = juce::jmax (0, piece->coreStart - pieceOverlap);
        piece->end = juce::jmin (total, piece->coreEnd + pieceOverlap);

        if (piece->end > piece->start)
            pieces.push_back (std::move (piece));
    }

    std::atomic<bool> stop { false };
    std::vector<std::thread> threads;

    for (auto& p : pieces)
    {
        threads.emplace_back ([&input, &separate, &stop, piece = p.get()]
        {
            juce::AudioBuffer<float> audio (2, piece->end - piece->start);

            for (int ch = 0; ch < 2; ++ch)
                audio.copyFrom (ch, 0, input, ch, piece->start, audio.getNumSamples());

            try
            {
                piece->result = separate (audio, [piece, &stop] (double done)
                {
                    piece->progress = done;
                    return ! stop.load();
                });
            }
            catch (...)
            {
                piece->result.reset();   // out of memory, or a model that doesn't fit
            }

            if (! piece->result.has_value())
                stop = true;   // no point finishing the other pieces

            piece->finished = true;
        });
    }

    // Report the progress from this thread, so the callback needn't be thread-safe.
    for (;;)
    {
        double sum = 0.0;
        bool allFinished = true;

        for (auto& p : pieces)
        {
            sum += p->progress.load();
            allFinished = allFinished && p->finished.load();
        }

        if (allFinished)
            break;

        if (! progress (juce::jlimit (0.0, 1.0, sum / (double) pieces.size())))
            stop = true;

        std::this_thread::sleep_for (std::chrono::milliseconds (100));
    }

    for (auto& t : threads)
        t.join();

    if (stop.load())
        return std::nullopt;   // cancelled, or a piece failed

    // Cross-fade: each piece counts fully over its own samples and fades out over the overlap.
    Stems out;
    for (auto& stem : out)
    {
        stem.setSize (2, total);
        stem.clear();
    }

    std::vector<float> weightSum ((size_t) total, 0.0f);

    for (auto& p : pieces)
    {
        const int length = p->end - p->start;

        for (int j = 0; j < length; ++j)
        {
            const int t = p->start + j;
            float weight = 1.0f;

            if (t < p->coreStart)
                weight = (float) (t - p->start + 1) / (float) (pieceOverlap + 1);
            else if (t >= p->coreEnd)
                weight = (float) (p->end - t) / (float) (pieceOverlap + 1);

            weightSum[(size_t) t] += weight;

            for (int s = 0; s < numStems; ++s)
                for (int ch = 0; ch < 2; ++ch)
                    out[(size_t) s].getWritePointer (ch)[t] += weight * (*p->result)[(size_t) s].getSample (ch, j);
        }
    }

    for (auto& stem : out)
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = stem.getWritePointer (ch);

            for (int t = 0; t < total; ++t)
                if (weightSum[(size_t) t] > 0.0f)
                    d[t] /= weightSum[(size_t) t];
        }

    return out;
}

int choosePieceCount (int numSamples)
{
    // Each piece running needs about 2.5 GB for Demucs' buffers; leave 4 GB for everything else.
    const int byMemory = (juce::SystemStats::getMemorySizeInMegabytes() - 4096) / 2560;
    const int byLength = (int) std::ceil (numSamples / (sampleRate * pieceCoreSeconds));
    const int cores = juce::jmin (8, juce::SystemStats::getNumPhysicalCpus());

    return juce::jmax (1, juce::jmin (cores, byMemory, byLength));
}

bool isSilent (const Stems& stems, int index)
{
    auto rms = [] (const juce::AudioBuffer<float>& b)
    {
        double sum = 0.0;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            sum += juce::square ((double) b.getRMSLevel (ch, 0, b.getNumSamples()));

        return std::sqrt (sum);
    };

    double loudest = 0.0;
    for (auto& s : stems)
        loudest = juce::jmax (loudest, rms (s));

    return rms (stems[(size_t) index]) <= loudest * 0.0316;   // -30 dB
}

//==============================================================================
Separator::Separator() = default;
Separator::~Separator() = default;

bool Separator::load (const juce::File& file, juce::String& error)
{
    auto loaded = std::make_unique<demucscpp::demucs_model>();

    if (! demucscpp::load_demucs_model (file.getFullPathName().toStdString(), loaded.get()) || ! loaded->is_4sources)
    {
        error = "Couldn't load the Demucs model from " + file.getFullPathName() + ".";
        return false;
    }

    model = std::move (loaded);
    return true;
}

std::optional<Stems> Separator::separate (const juce::AudioBuffer<float>& audio, const ProgressFn& progress) const
{
    jassert (model != nullptr);

    return separateInPieces (audio, choosePieceCount (audio.getNumSamples()),
                             [this] (const juce::AudioBuffer<float>& piece, const std::function<bool (double)>& keepGoing)
                             {
                                 return separatePiece (piece, keepGoing);
                             },
                             progress);
}

std::optional<Stems> Separator::separatePiece (const juce::AudioBuffer<float>& audio, const std::function<bool (double)>& keepGoing) const
{
    const int n = audio.getNumSamples();
    Eigen::MatrixXf mix (2, n);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < n; ++i)
            mix (ch, i) = audio.getSample (ch, i);

    Eigen::Tensor3dXf separated;

    try
    {
        separated = demucscpp::demucs_inference (*model, mix, [&keepGoing] (float done, const std::string&)
        {
            if (! keepGoing (juce::jmin (0.999, (double) done)))
                throw Cancelled();
        });
    }
    catch (const Cancelled&)
    {
        return std::nullopt;
    }

    Stems stems;

    for (int s = 0; s < numStems; ++s)
    {
        auto& stem = stems[(size_t) s];
        stem.setSize (2, n);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = stem.getWritePointer (ch);

            for (int i = 0; i < n; ++i)
                d[i] = separated (s, ch, i);
        }
    }

    keepGoing (1.0);
    return stems;
}

//==============================================================================
std::vector<Track*> addStemTracks (Engine& engine, Track& source, AudioClip& clip, Stems stems)
{
    std::array<bool, numStems> silent {};
    for (int s = 0; s < numStems; ++s)
        silent[(size_t) s] = isSilent (stems, s);

    std::vector<Track*> added;
    int index = engine.indexOf (&source) + 1;

    for (int s = 0; s < numStems; ++s)
    {
        if (silent[(size_t) s])
            continue;

        added.push_back (addStemTrack (engine, index++, clip, s, std::move (stems[(size_t) s])));
    }

    // The stems replace the clip; other clips on its track keep playing.
    if (! added.empty())
        engine.setClipMuted (clip, true);

    return added;
}

Track* addStemTrack (Engine& engine, int trackIndex, const AudioClip& source, int stem, juce::AudioBuffer<float> audio)
{
    auto* track = engine.addTrack (Track::Type::audio, trackIndex);
    track->name = names[(size_t) stem];

    if (auto* clip = engine.addAudioClip (*track, std::move (audio), sampleRate, source.startBeat,
                                          source.name + " " + names[(size_t) stem].toLowerCase()))
        clip->gainDb = source.gainDb;   // the stem sounds as loud as it did in the clip

    return track;
}
//==============================================================================
juce::AudioBuffer<float> mixStems (const Stems& stems, const std::array<bool, numStems>& which)
{
    int length = 0;
    for (auto& stem : stems)
        length = juce::jmax (length, stem.getNumSamples());

    juce::AudioBuffer<float> mix (2, length);
    mix.clear();

    for (int s = 0; s < numStems; ++s)
    {
        const auto& stem = stems[(size_t) s];

        if (! which[(size_t) s] || stem.getNumChannels() == 0)
            continue;

        for (int ch = 0; ch < 2; ++ch)
            mix.addFrom (ch, 0, stem, juce::jmin (ch, stem.getNumChannels() - 1), 0, stem.getNumSamples());
    }

    return mix;
}

float getGuideDb (GuideVocals guide)
{
    switch (guide)
    {
        case GuideVocals::off:    return 0.0f;
        case GuideVocals::quiet:  return -12.0f;
        case GuideVocals::softer: return -6.0f;
    }

    return 0.0f;
}

juce::String getGuideName (GuideVocals guide)
{
    switch (guide)
    {
        case GuideVocals::off:    return "Off: the Vocals track is muted";
        case GuideVocals::quiet:  return "Quiet: the vocals at -12 dB, to follow the melody";
        case GuideVocals::softer: return "Softer: the vocals at -6 dB";
    }

    return {};
}

Karaoke addKaraokeTracks (Engine& engine, Track& source, AudioClip& clip, Stems stems, const KaraokeChoices& choices)
{
    Karaoke added;

    // No singing, or nothing at all: the clip already is its own karaoke version.
    if (isSilent (stems, vocalsStem))
        return added;

    int index = engine.indexOf (&source) + 1;

    if (choices.asStems)
    {
        for (int s = 0; s < numStems; ++s)
            if (s != vocalsStem && ! isSilent (stems, s))
                added.instrumental.push_back (addStemTrack (engine, index++, clip, s, std::move (stems[(size_t) s])));
    }
    else
    {
        // Every stem but the vocals, the quiet ones too: together they're the mix without the vocals.
        std::array<bool, numStems> which;
        which.fill (true);
        which[(size_t) vocalsStem] = false;

        auto* track = engine.addTrack (Track::Type::audio, index++);
        track->name = "Karaoke";

        if (auto* karaoke = engine.addAudioClip (*track, mixStems (stems, which), sampleRate, clip.startBeat, clip.name + " karaoke"))
            karaoke->gainDb = clip.gainDb;

        added.instrumental.push_back (track);
    }

    added.vocals = addStemTrack (engine, index, clip, vocalsStem, std::move (stems[(size_t) vocalsStem]));
    added.vocals->volumeDb = getGuideDb (choices.guide);
    added.vocals->muted = choices.guide == GuideVocals::off;

    engine.setClipMuted (clip, true);
    return added;
}
} // namespace daw::stems
