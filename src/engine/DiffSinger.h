#pragma once

#include "Singer.h"
#include "Voice.h"

#include <memory>

namespace daw::sing
{
/** The real Singer: runs a DiffSinger voice's ONNX models on the CPU, in the order OpenUtau does (its MIT code
    is the reference): the duration and pitch models when the voice has them, the acoustic model that paints the
    sound as a spectrogram, then the vocoder that turns it into audio.

    Each model's input names are read when it loads. The acoustic model and the vocoder must be understood
    completely: an input this version doesn't know makes `create` fail, naming it. The optional duration and pitch
    models that can't be understood are left out instead, and `getWarnings()` says why; the score then uses its own
    sound lengths and the notes' exact pitch.

    Intra-op threads are the core count minus 2, so playback keeps going. CoreML is not used: it can't run
    DiffSinger's dynamic shapes. */
class DiffSinger final : public Singer
{
public:
    ~DiffSinger() override;

    /** Loads the voice's models. Returns null and sets `error` on failure (the file and what's wrong with it). */
    static std::unique_ptr<DiffSinger> create (const Voice&, juce::String& error);

    std::vector<double> predictDurations (const Phrase&, const CancelFn&) override;
    std::vector<float> predictPitch (const Phrase&, const CancelFn&) override;
    std::optional<juce::AudioBuffer<float>> sing (const Phrase&, int steps, const CancelFn&, juce::String& error) override;

    /** Things left out: "The pitch model asks for ..., so the notes' exact pitch is used". */
    const juce::StringArray& getWarnings() const noexcept { return warnings; }

    /** Each model's file with its input names, types and shapes (for finding out why a voice doesn't work). */
    juce::String describeModels() const;

    bool hasDurationModel() const noexcept;
    bool hasPitchModel() const noexcept;

private:
    DiffSinger();

    struct Impl;
    std::unique_ptr<Impl> impl;
    juce::StringArray warnings;

    JUCE_DECLARE_NON_COPYABLE (DiffSinger)
};
} // namespace daw::sing
