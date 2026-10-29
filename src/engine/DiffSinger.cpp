#include "DiffSinger.h"
#include "OnnxEnvironment.h"

#include <map>
#include <stdexcept>

namespace daw::sing
{
namespace
{
    /** A model asked for something this version doesn't know, or something failed in a way worth naming. */
    struct ModelError : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct Cancelled {};

    using Inputs = std::map<std::string, Ort::Value>;

    template <typename T>
    Ort::Value makeTensor (const std::vector<T>& data, std::vector<int64_t> shape)
    {
        Ort::AllocatorWithDefaultOptions allocator;
        auto value = Ort::Value::CreateTensor<T> (allocator, shape.data(), shape.size());
        std::copy (data.begin(), data.end(), value.template GetTensorMutableData<T>());
        return value;
    }

    Ort::Value makeBoolTensor (const std::vector<char>& data, std::vector<int64_t> shape)
    {
        Ort::AllocatorWithDefaultOptions allocator;
        auto value = Ort::Value::CreateTensor<bool> (allocator, shape.data(), shape.size());
        auto* out = value.GetTensorMutableData<bool>();

        for (size_t i = 0; i < data.size(); ++i)
            out[i] = data[i] != 0;

        return value;
    }

    Ort::Value makeScalar (float v)   { return makeTensor<float> ({ v }, {}); }
    Ort::Value makeScalar (int64_t v) { return makeTensor<int64_t> ({ v }, {}); }

    /** Stops a running model from another thread when the job is cancelled. */
    class CancelWatcher final : public juce::Thread
    {
    public:
        CancelWatcher (Ort::RunOptions& o, const CancelFn& c) : juce::Thread ("Sing cancel watcher"), options (o), cancel (c) { startThread(); }
        ~CancelWatcher() override { stopThread (2000); }

        void run() override
        {
            while (! threadShouldExit())
            {
                if (cancel && cancel())
                {
                    options.SetTerminate();
                    return;
                }

                wait (40);
            }
        }

    private:
        Ort::RunOptions& options;
        CancelFn cancel;
    };

    float midiToHz (double midi) { return (float) (440.0 * std::pow (2.0, (midi - 69.0) / 12.0)); }
    double hzToMidi (double hz) { return 69.0 + 12.0 * std::log2 (juce::jmax (1.0, hz) / 440.0); }
}

//==============================================================================
struct DiffSinger::Impl
{
    struct Model
    {
        juce::String fileName;
        std::unique_ptr<Ort::Session> session;
        std::vector<std::string> inputs, outputs;
    };

    Voice voice;
    Ort::SessionOptions options;
    Model acoustic, vocoder, durLinguistic, dur, pitchLinguistic, pitch;
    std::vector<float> speakerEmbedding;   // one speaker's embedding, when the acoustic model asks for it
    std::vector<float> durEmbedding, pitchEmbedding;   // the same for the duration and pitch models

    static std::vector<float> readEmbedding (const juce::File& file)
    {
        std::vector<float> out;
        juce::MemoryBlock block;

        if (file.existsAsFile() && file.loadFileAsData (block))
        {
            out.resize (block.getSize() / sizeof (float));
            std::memcpy (out.data(), block.getData(), out.size() * sizeof (float));
        }

        return out;
    }

    static Ort::Value repeatEmbedding (const std::vector<float>& e, int64_t length)
    {
        std::vector<float> all;
        all.reserve ((size_t) length * e.size());

        for (int64_t i = 0; i < length; ++i)
            all.insert (all.end(), e.begin(), e.end());

        return makeTensor<float> (all, { 1, length, (int64_t) e.size() });
    }

    void load (Model& m, const juce::File& file)
    {
        m.fileName = file.getFileName();
#if JUCE_WINDOWS
        m.session = std::make_unique<Ort::Session> (onnx::getEnvironment(), file.getFullPathName().toWideCharPointer(), options);
#else
        m.session = std::make_unique<Ort::Session> (onnx::getEnvironment(), file.getFullPathName().toRawUTF8(), options);
#endif
        Ort::AllocatorWithDefaultOptions allocator;

        for (size_t i = 0; i < m.session->GetInputCount(); ++i)
            m.inputs.emplace_back (m.session->GetInputNameAllocated (i, allocator).get());

        for (size_t i = 0; i < m.session->GetOutputCount(); ++i)
            m.outputs.emplace_back (m.session->GetOutputNameAllocated (i, allocator).get());
    }

    static bool contains (const std::vector<std::string>& names, const std::string& name)
    {
        return std::find (names.begin(), names.end(), name) != names.end();
    }

    /** Fails naming the input when a model asks for one that isn't among those this version can give. */
    static void checkInputs (const Model& m, const std::vector<std::string>& known)
    {
        for (auto& name : m.inputs)
            if (! contains (known, name))
                throw ModelError (("The model " + m.fileName + " asks for an input called \"" + juce::String (name) + "\", "
                                   "which this version of DawIt doesn't know how to give it.").toStdString());
    }

    std::vector<Ort::Value> run (Model& m, Inputs& available, const CancelFn& cancel)
    {
        std::vector<const char*> names;
        std::vector<Ort::Value> values;

        for (auto& name : m.inputs)
        {
            auto it = available.find (name);

            if (it == available.end())
                throw ModelError (("The model " + m.fileName + " asks for an input called \"" + juce::String (name) + "\", "
                                   "which this version of DawIt doesn't know how to give it.").toStdString());

            names.push_back (name.c_str());
            values.push_back (std::move (it->second));
        }

        std::vector<const char*> outputNames;
        for (auto& name : m.outputs)
            outputNames.push_back (name.c_str());

        Ort::RunOptions runOptions;
        CancelWatcher watcher (runOptions, cancel);

        try
        {
            return m.session->Run (runOptions, names.data(), values.data(), values.size(), outputNames.data(), outputNames.size());
        }
        catch (const Ort::Exception&)
        {
            if (cancel && cancel())
                throw Cancelled();

            throw;
        }
    }

    //==========================================================================
    std::vector<int64_t> getTokens (const Phrase& phrase) const
    {
        std::vector<int64_t> tokens;

        for (auto& p : phrase.phonemes)
        {
            const auto it = voice.phonemeIds.find (p.symbol);

            if (it == voice.phonemeIds.end())
                throw ModelError (("The voice has no sound called \"" + p.symbol + "\".").toStdString());

            tokens.push_back (it->second);
        }

        return tokens;
    }

    /** The inputs the models share: tokens, languages and the speaker. */
    void addSequenceInputs (Inputs& in, const Phrase& phrase) const
    {
        const auto tokens = getTokens (phrase);
        const int64_t n = (int64_t) tokens.size();
        in.emplace ("tokens", makeTensor<int64_t> (tokens, { 1, n }));
        in.emplace ("languages", makeTensor<int64_t> (std::vector<int64_t> ((size_t) n, juce::jmax (0, voice.englishLanguageId)), { 1, n }));
    }

    void addWordInputs (Inputs& in, const Phrase& phrase) const
    {
        std::vector<int64_t> wordDiv, wordDur;
        int lastWord = -2;

        for (auto& p : phrase.phonemes)
        {
            if (p.word >= 0 && p.word == lastWord)
            {
                ++wordDiv.back();
                wordDur.back() += p.frames;
            }
            else
            {
                wordDiv.push_back (1);
                wordDur.push_back (p.frames);
            }

            lastWord = p.word;
        }

        const int64_t w = (int64_t) wordDiv.size();
        in.emplace ("word_div", makeTensor<int64_t> (wordDiv, { 1, w }));
        in.emplace ("word_dur", makeTensor<int64_t> (wordDur, { 1, w }));
    }

    /** The MIDI pitch under the middle of each phoneme, 0 for a rest. */
    std::vector<int64_t> getPhonemeMidi (const Phrase& phrase) const
    {
        std::vector<int> frameMidi;

        for (auto& n : phrase.notes)
            frameMidi.insert (frameMidi.end(), (size_t) n.frames, n.rest ? 0 : n.midi);

        std::vector<int64_t> out;
        int frame = 0;

        for (auto& p : phrase.phonemes)
        {
            const int mid = juce::jlimit (0, juce::jmax (0, (int) frameMidi.size() - 1), frame + p.frames / 2);
            out.push_back (frameMidi.empty() ? 0 : frameMidi[(size_t) mid]);
            frame += p.frames;
        }

        return out;
    }
};

//==============================================================================
DiffSinger::DiffSinger() : impl (std::make_unique<Impl>()) {}
DiffSinger::~DiffSinger() = default;

juce::String DiffSinger::describeModels() const
{
    juce::String out;

    for (auto* m : { &impl->acoustic, &impl->vocoder, &impl->durLinguistic, &impl->dur, &impl->pitchLinguistic, &impl->pitch })
    {
        if (m->session == nullptr)
            continue;

        out << m->fileName << ": in";

        for (size_t i = 0; i < m->inputs.size(); ++i)
        {
            out << " " << m->inputs[i];
            auto info = m->session->GetInputTypeInfo (i).GetTensorTypeAndShapeInfo();
            out << "(" << juce::String ((int) info.GetElementType()) << ")[";

            for (auto d : info.GetShape())
                out << juce::String ((juce::int64) d) << ",";

            out << "]";
        }

        out << "; out";
        for (auto& o : m->outputs)
            out << " " << o;

        out << "\n";
    }

    return out;
}

bool DiffSinger::hasDurationModel() const noexcept { return impl->dur.session != nullptr && impl->durLinguistic.session != nullptr; }
bool DiffSinger::hasPitchModel() const noexcept    { return impl->pitch.session != nullptr && impl->pitchLinguistic.session != nullptr; }

std::unique_ptr<DiffSinger> DiffSinger::create (const Voice& voice, juce::String& error)
{
    std::unique_ptr<DiffSinger> singer (new DiffSinger());
    auto& impl = *singer->impl;
    impl.voice = voice;
    impl.options.SetIntraOpNumThreads (juce::jmax (1, juce::SystemStats::getNumPhysicalCpus() - 2));
    impl.options.SetGraphOptimizationLevel (GraphOptimizationLevel::ORT_ENABLE_ALL);

    try
    {
        impl.load (impl.acoustic, voice.files.acoustic);
        impl.load (impl.vocoder, voice.files.vocoderModel);

        Impl::checkInputs (impl.acoustic, { "tokens", "languages", "durations", "f0", "depth", "steps", "speedup", "gender", "velocity",
                                            "energy", "breathiness", "voicing", "tension", "key_shift", "speed", "spk_embed" });
        Impl::checkInputs (impl.vocoder, { "mel", "f0" });

        if (impl.acoustic.outputs.empty() || impl.vocoder.outputs.empty())
            throw ModelError ("A model of the voice has no output.");

        if (Impl::contains (impl.acoustic.inputs, "spk_embed"))
        {
            // The embedding of the voice's first speaker: raw 32-bit floats in <speaker>.emb.
            juce::String speaker;

            if (const auto* speakers = voice.config.find ("speakers"); speakers != nullptr && speakers->isList() && ! speakers->items.empty())
                speaker = speakers->items.front().scalar;

            auto file = voice.files.root.getChildFile (speaker);

            if (! file.existsAsFile())
                file = voice.files.root.getChildFile (speaker + ".emb");

            if (speaker.isEmpty() || ! file.existsAsFile())
                throw ModelError (((speaker.isEmpty() ? juce::String ("The speaker's .emb file") : speaker + ".emb")
                                   + " is missing from " + voice.files.root.getFileName() + ".").toStdString());

            juce::MemoryBlock block;
            file.loadFileAsData (block);
            impl.speakerEmbedding.resize (block.getSize() / sizeof (float));
            std::memcpy (impl.speakerEmbedding.data(), block.getData(), impl.speakerEmbedding.size() * sizeof (float));
        }
    }
    catch (const std::exception& e)
    {
        error = juce::String (e.what());
        return nullptr;
    }

    // The optional models: left out, with a note, when they can't be understood.
    auto loadOptional = [&] (Impl::Model& linguistic, Impl::Model& model, const juce::File& linguisticFile, const juce::File& modelFile,
                             const std::vector<std::string>& linguisticKnown, const std::vector<std::string>& modelKnown, const juce::String& what)
    {
        if (modelFile == juce::File() || linguisticFile == juce::File())
            return;

        try
        {
            impl.load (linguistic, linguisticFile);
            impl.load (model, modelFile);
            Impl::checkInputs (linguistic, linguisticKnown);
            Impl::checkInputs (model, modelKnown);
        }
        catch (const std::exception& e)
        {
            singer->warnings.add (what + " is left out: " + juce::String (e.what()));
            linguistic = {};
            model = {};
        }
    };

    loadOptional (impl.durLinguistic, impl.dur, voice.files.durLinguistic, voice.files.dur,
                  { "tokens", "languages", "word_div", "word_dur" }, { "encoder_out", "x_masks", "ph_midi", "spk_embed" },
                  "The duration model");
    loadOptional (impl.pitchLinguistic, impl.pitch, voice.files.pitchLinguistic, voice.files.pitch,
                  { "tokens", "languages", "word_div", "word_dur", "ph_dur" },
                  { "encoder_out", "ph_dur", "note_midi", "note_rest", "note_dur", "pitch", "expr", "retake", "steps", "spk_embed" },
                  "The pitch model");

    impl.durEmbedding = Impl::readEmbedding (voice.files.durSpeaker);
    impl.pitchEmbedding = Impl::readEmbedding (voice.files.pitchSpeaker);

    if (impl.dur.session != nullptr && Impl::contains (impl.dur.inputs, "spk_embed") && impl.durEmbedding.empty())
    {
        singer->warnings.add ("The duration model is left out: its speaker file (.emb) is missing.");
        impl.dur = {};
        impl.durLinguistic = {};
    }

    if (impl.pitch.session != nullptr && Impl::contains (impl.pitch.inputs, "spk_embed") && impl.pitchEmbedding.empty())
    {
        singer->warnings.add ("The pitch model is left out: its speaker file (.emb) is missing.");
        impl.pitch = {};
        impl.pitchLinguistic = {};
    }

    return singer;
}

//==============================================================================
std::vector<double> DiffSinger::predictDurations (const Phrase& phrase, const CancelFn& cancel)
{
    if (! hasDurationModel())
        return {};

    try
    {
        Inputs in;
        impl->addSequenceInputs (in, phrase);
        impl->addWordInputs (in, phrase);
        auto encoded = impl->run (impl->durLinguistic, in, cancel);

        Inputs in2;
        for (size_t i = 0; i < impl->durLinguistic.outputs.size() && i < encoded.size(); ++i)
            in2.emplace (impl->durLinguistic.outputs[i], std::move (encoded[i]));

        in2.emplace ("ph_midi", makeTensor<int64_t> (impl->getPhonemeMidi (phrase), { 1, (int64_t) phrase.phonemes.size() }));

        if (! impl->durEmbedding.empty())
            in2.emplace ("spk_embed", Impl::repeatEmbedding (impl->durEmbedding, (int64_t) phrase.phonemes.size()));

        auto out = impl->run (impl->dur, in2, cancel);

        const auto* data = out[0].GetTensorData<float>();
        const auto count = out[0].GetTensorTypeAndShapeInfo().GetElementCount();

        if (count != phrase.phonemes.size())
            return {};

        std::vector<double> seconds;
        for (size_t i = 0; i < count; ++i)
            seconds.push_back (juce::jmax (0.0f, data[i]) * frameSeconds);

        return seconds;
    }
    catch (const std::exception& e)
    {
        warnings.addIfNotAlreadyThere ("The duration model failed (" + juce::String (e.what()) + "), so fixed sound lengths are used.");
        impl->dur = {};
        return {};
    }
    catch (const Cancelled&)
    {
        return {};
    }
}

std::vector<float> DiffSinger::predictPitch (const Phrase& phrase, const CancelFn& cancel)
{
    if (! hasPitchModel())
        return {};

    try
    {
        const int64_t frames = (int64_t) phrase.getNumFrames();
        std::vector<int64_t> phDur;
        for (auto& p : phrase.phonemes)
            phDur.push_back (p.frames);

        Inputs in;
        impl->addSequenceInputs (in, phrase);
        impl->addWordInputs (in, phrase);
        in.emplace ("ph_dur", makeTensor<int64_t> (phDur, { 1, (int64_t) phDur.size() }));
        auto encoded = impl->run (impl->pitchLinguistic, in, cancel);

        Inputs in2;
        for (size_t i = 0; i < impl->pitchLinguistic.outputs.size() && i < encoded.size(); ++i)
            in2.emplace (impl->pitchLinguistic.outputs[i], std::move (encoded[i]));

        if (! impl->pitchEmbedding.empty())
            in2.emplace ("spk_embed", Impl::repeatEmbedding (impl->pitchEmbedding, frames));

        std::vector<float> noteMidi;
        std::vector<char> noteRest;
        std::vector<int64_t> noteDur;

        for (auto& n : phrase.notes)
        {
            noteMidi.push_back ((float) n.midi);
            noteRest.push_back (n.rest ? 1 : 0);
            noteDur.push_back (n.frames);
        }

        std::vector<float> base;
        for (auto f : phrase.f0)
            base.push_back ((float) hzToMidi (f));

        const int64_t numNotes = (int64_t) noteMidi.size();
        in2.emplace ("ph_dur", makeTensor<int64_t> (phDur, { 1, (int64_t) phDur.size() }));
        in2.emplace ("note_midi", makeTensor<float> (noteMidi, { 1, numNotes }));
        in2.emplace ("note_rest", makeBoolTensor (noteRest, { 1, numNotes }));
        in2.emplace ("note_dur", makeTensor<int64_t> (noteDur, { 1, numNotes }));
        in2.emplace ("pitch", makeTensor<float> (base, { 1, frames }));
        in2.emplace ("expr", makeTensor<float> (std::vector<float> ((size_t) frames, 1.0f), { 1, frames }));
        in2.emplace ("retake", makeBoolTensor (std::vector<char> ((size_t) frames, 1), { 1, frames }));
        in2.emplace ("steps", makeScalar ((int64_t) 10));
        auto out = impl->run (impl->pitch, in2, cancel);

        const auto* data = out[0].GetTensorData<float>();
        const auto count = out[0].GetTensorTypeAndShapeInfo().GetElementCount();

        if ((int64_t) count != frames)
            return {};

        // The model gives either the pitch itself or the change from the notes' pitch, in semitones.
        double meanBase = 0.0, meanPred = 0.0;
        for (size_t i = 0; i < count; ++i)
        {
            meanBase += base[i];
            meanPred += data[i];
        }
        const bool offset = std::abs (meanPred - meanBase) / (double) count > 6.0;

        std::vector<float> f0;
        for (size_t i = 0; i < count; ++i)
            f0.push_back (midiToHz (offset ? base[i] + data[i] : (double) data[i]));

        return f0;
    }
    catch (const std::exception& e)
    {
        warnings.addIfNotAlreadyThere ("The pitch model failed (" + juce::String (e.what()) + "), so the notes' exact pitch is used.");
        impl->pitch = {};
        return {};
    }
    catch (const Cancelled&)
    {
        return {};
    }
}

std::optional<juce::AudioBuffer<float>> DiffSinger::sing (const Phrase& phrase, int steps, const CancelFn& cancel, juce::String& error)
{
    try
    {
        const int64_t frames = (int64_t) phrase.getNumFrames();

        if (frames <= 0 || (int64_t) phrase.f0.size() != frames)
            throw ModelError ("The phrase has no sounds or no pitch.");

        std::vector<int64_t> durations;
        for (auto& p : phrase.phonemes)
            durations.push_back (p.frames);

        auto zeros = [&] { return makeTensor<float> (std::vector<float> ((size_t) frames, 0.0f), { 1, frames }); };
        auto ones  = [&] { return makeTensor<float> (std::vector<float> ((size_t) frames, 1.0f), { 1, frames }); };

        Inputs in;
        impl->addSequenceInputs (in, phrase);
        in.emplace ("durations", makeTensor<int64_t> (durations, { 1, (int64_t) durations.size() }));
        in.emplace ("f0", makeTensor<float> (phrase.f0, { 1, frames }));
        in.emplace ("depth", makeScalar (1.0f));
        in.emplace ("steps", makeScalar ((int64_t) steps));
        in.emplace ("speedup", makeScalar ((int64_t) juce::jmax (1, 1000 / juce::jmax (1, steps))));
        in.emplace ("gender", zeros());
        in.emplace ("velocity", ones());
        in.emplace ("energy", zeros());
        in.emplace ("breathiness", zeros());
        in.emplace ("voicing", zeros());
        in.emplace ("tension", zeros());
        in.emplace ("key_shift", zeros());
        in.emplace ("speed", ones());

        if (! impl->speakerEmbedding.empty())
        {
            const int64_t width = (int64_t) impl->speakerEmbedding.size();
            std::vector<float> embedding;
            embedding.reserve ((size_t) (frames * width));

            for (int64_t f = 0; f < frames; ++f)
                embedding.insert (embedding.end(), impl->speakerEmbedding.begin(), impl->speakerEmbedding.end());

            in.emplace ("spk_embed", makeTensor<float> (embedding, { 1, frames, width }));
        }

        auto mel = impl->run (impl->acoustic, in, cancel);

        Inputs vocoderIn;
        vocoderIn.emplace ("mel", std::move (mel[0]));
        vocoderIn.emplace ("f0", makeTensor<float> (phrase.f0, { 1, frames }));
        auto wave = impl->run (impl->vocoder, vocoderIn, cancel);

        const auto* data = wave[0].GetTensorData<float>();
        const int numSamples = (int) wave[0].GetTensorTypeAndShapeInfo().GetElementCount();

        juce::AudioBuffer<float> raw (1, numSamples);
        std::memcpy (raw.getWritePointer (0), data, (size_t) numSamples * sizeof (float));

        if (impl->voice.sampleRate == sampleRate)
            return raw;

        // A voice at another rate is brought to 44.1 kHz.
        const double ratio = (double) impl->voice.sampleRate / (double) sampleRate;
        const int outSamples = juce::roundToInt (numSamples / ratio);
        juce::AudioBuffer<float> resampled (1, outSamples);
        juce::LagrangeInterpolator interpolator;
        interpolator.process (ratio, raw.getReadPointer (0), resampled.getWritePointer (0), outSamples);
        return resampled;
    }
    catch (const Cancelled&)
    {
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        error = juce::String (e.what());
        return std::nullopt;
    }
}
} // namespace daw::sing
