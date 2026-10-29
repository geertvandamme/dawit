#pragma once

#include "BuiltInEq.h"
#include "EffectTools.h"
#include "SongContext.h"

#include <juce_dsp/juce_dsp.h>

namespace daw
{
namespace sat
{
    /** The knobs, in the order of the window and of a preset's notes. */
    enum class Knob { drive, type, tone, output, mix };
    constexpr int numKnobs = 5;

    /** Tape: a soft, even rounding of both halves of the wave (tanh), odd harmonics. Tube: the same,
        but one half rounds off sooner than the other, which adds even harmonics. Clip: flat at the
        top and bottom (hard clipping), many and high odd harmonics. */
    enum class Type { tape, tube, clip };
    constexpr int numTypes = 3;

    juce::String getTypeName (Type);

    struct Settings
    {
        float driveDb = 12.0f;      // how hard the sound is pushed into the curve
        Type type = Type::tape;
        float toneHz = 20000.0f;    // a high cut after the curve, to tame fizz; 20 kHz is off
        float outputDb = 0.0f;
        float mixPercent = 100.0f;  // below 100: parallel saturation, the clean sound underneath
    };

    constexpr float maxDrive = 36.0f, minTone = 1000.0f, maxTone = 20000.0f, minOutput = -24.0f, maxOutput = 12.0f;

    inline bool isToneOn (float hz) { return hz < maxTone - 1.0f; }

    /** The curve: what comes out for a value going in. Small values pass unchanged (a slope of 1
        through 0); big ones are rounded off (tape, tube) or cut off (clip). */
    float shape (Type, float x);

    /** What a sine with this peak (as a gain, before the drive) turns into: the level of each
        harmonic from 1 (the note itself) to `count`, in dB, before the tone and output. Exact:
        one cycle, with no aliasing. */
    std::vector<float> getHarmonicsDb (const Settings&, float inputPeak, int count);

    /** Total harmonic distortion: the level of all harmonics above the first against the first, in %. */
    float getThdPercent (const std::vector<float>& harmonicsDb);

    /** "Octave", "octave + fifth", "2 octaves", ...: what harmonic `n` is above the note. */
    juce::String getHarmonicInterval (int n);

    /** "+12.0 dB", "Tape", "6.0 kHz" / "Off", "100%": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point for a sound, with why each knob is set the way it is. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob
    };

    /** The neutral start first, then the sounds. */
    const std::vector<Preset>& getPresets();
} // namespace sat

//==============================================================================
/**
    DawIt Saturation: pushes a sound into a curve that rounds or cuts off its peaks, which adds
    harmonics: warmth, grit, presence or fuzz. Drive, the kind of curve (tape, tube, clip), a high
    cut after it, output, a dry/wet mix for parallel saturation, and match level. The curve runs at
    four times the sample rate, so the harmonics it makes above the highest frequency the sample
    rate can hold don't fold back down as tones that don't belong (aliasing). It runs in the
    track's chain like DawIt EQ (see BuiltInFormat.h).

    For learning it also offers: a note to hear it with, only what it adds (the output minus a
    clean copy of the input at the same level), and a history of how loud the track and what it
    adds were.
*/
class BuiltInSaturation final : public juce::AudioPluginInstance, public KeepsLatencyWhenBypassed
{
public:
    BuiltInSaturation();

    static constexpr const char* identifier = "DawIt Saturation";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInSaturation&)> createEditorFunction;

    static const char* getParameterId (sat::Knob);
    juce::RangedAudioParameter* getKnobParameter (sat::Knob) const;

    sat::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const sat::Settings&);
    void setKnob (sat::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    bool isMatchingLevel() const { return matchLevel->load() >= 0.5f; }
    void setMatchingLevel (bool);

    /** The gain that keeps the output as loud as the input right now, in dB; match level applies it. */
    std::atomic<float> matchGainDb { 0.0f };

    /** The loudest the track was over the last moment (a peak, as a gain), for the pictures. */
    std::atomic<float> recentPeak { 0.0f };

    /** Play only what the saturation adds. Not saved. */
    std::atomic<bool> listenToEffect { false };

    fx::Clap clap;
    fx::LevelHistory history;   // the effect's level is what the saturation adds, as mixed

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    /** Bypassed, it still holds the sound back as long as it does when it works, so the track stays in time. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return createEditorFunction != nullptr; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** How many times the sample rate the curve runs at. */
    static constexpr int oversampling = 4;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, sat::numKnobs> knobValues {};
    std::atomic<float>* matchLevel = nullptr;

    double rate = 44100.0;
    int blockSize = 512, latency = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> dry, delayLine;
    int delayWrite = 0;

    juce::SmoothedValue<float> drive, mix, outputGain, matchGain;
    float shownTone = -1.0f;
    eq::Stages toneStages;
    double toneMemory[2][2][2] {}, cleanToneMemory[2][2][2] {};
    float dcIn[2] {}, dcOut[2] {}, cleanDcIn[2] {}, cleanDcOut[2] {};
    double sumWetDry = 0.0, sumDry = 0.0, powerIn = 0.0, powerOut = 0.0;
    float peakHold = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInSaturation)
};
} // namespace daw
