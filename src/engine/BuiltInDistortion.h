#pragma once

#include "BuiltInEq.h"
#include "EffectTools.h"
#include "SongContext.h"

#include <juce_dsp/juce_dsp.h>

namespace daw
{
namespace dist
{
    /** The knobs, in the order of the window and of a preset's notes. */
    enum class Knob { tight, drive, type, tone, speaker, output, mix };
    constexpr int numKnobs = 7;

    /** Overdrive: a soft, slightly lopsided curve, like the diodes of an overdrive pedal; it keeps the
        dynamics of the playing. Distortion: nearly flat-topped, like the diodes to ground of a
        distortion pedal; even and loud. Fuzz: lopsided and hard, and quiet sounds are choked off,
        like a fuzz pedal's starved transistors; a buzzing square that sputters out. Rectify: the
        bottom half of the wave flipped up, which puts it an octave higher, like an octave fuzz.
        Fold: what goes past the top folds back down, like a synth's wavefolder; more drive, more folds. */
    enum class Type { overdrive, distortion, fuzz, rectify, fold };
    constexpr int numTypes = 5;

    juce::String getTypeName (Type);

    struct Settings
    {
        float tightHz = 20.0f;       // a low cut before the curve, so the bass isn't driven; 20 Hz is off
        float driveDb = 24.0f;       // how hard the sound is pushed into the curve
        Type type = Type::overdrive;
        float toneHz = 20000.0f;     // a high cut after the curve, to tame fizz; 20 kHz is off
        bool speaker = false;        // the sound of a guitar speaker after it all
        float outputDb = 0.0f;
        float mixPercent = 100.0f;   // below 100: parallel distortion, the clean sound underneath
    };

    constexpr float minTight = 20.0f, maxTight = 1000.0f, maxDrive = 48.0f, minTone = 1000.0f, maxTone = 20000.0f,
                    minOutput = -24.0f, maxOutput = 12.0f;

    inline bool isTightOn (float hz) { return hz > minTight + 0.5f; }
    inline bool isToneOn (float hz) { return hz < maxTone - 1.0f; }

    /** The note the pictures and the window's test note use: A3, 220 Hz. */
    constexpr double noteHz = 220.0;

    /** The curve: what comes out for a value going in. Overdrive, distortion and fold pass small
        values unchanged (a slope of 1 through 0); rectify turns them all positive; fuzz clips its
        bottom at half the height of its top. */
    float shape (Type, float x);

    /** The drive (as a gain) and the curve together: what comes out for a sample going in. The fuzz is
        biased like a starved transistor: its input is shifted before the drive, so the wave crosses
        zero off-centre (even harmonics) and quiet sounds can't get past the bias (the sputter). */
    float distort (Type, float drive, float x);

    /** The fuzz's bias, as a level going in: about -26 dB. Quieter notes are choked off. */
    constexpr float fuzzBias = 0.05f;

    /** The filters before the curve (the tight low cut) and after it (the tone's high cut, then the
        speaker), as biquads in a row. Empty when they're off. */
    std::vector<eq::Biquad> getFiltersBefore (const Settings&, double sampleRate);
    std::vector<eq::Biquad> getFiltersAfter (const Settings&, double sampleRate);

    /** How much the filters before or after the curve change the level at `hz`, as a gain. */
    double getGainBefore (const Settings&, double hz);
    double getGainAfter (const Settings&, double hz);

    /** How much a guitar speaker changes the level at `hz`, in dB: lows below about 90 Hz and
        highs above about 5 kHz are gone, with a bump in the upper mids. */
    double getSpeakerDb (double hz);

    /** The peak of the test note (A3, 220 Hz) at this level after the tight low cut: what reaches the curve. */
    float getPeakIntoCurve (const Settings&, float inputPeak);

    /** What the A3 test note with this peak (as a gain, before the drive) turns into: the level of
        each harmonic from 1 (the note itself) to `count`, in dB, after the filters and before the
        output. Exact: one cycle, with no aliasing. */
    std::vector<float> getHarmonicsDb (const Settings&, float inputPeak, int count);

    /** Total harmonic distortion: the level of all harmonics above the first against the first, in %. */
    float getThdPercent (const std::vector<float>& harmonicsDb);

    /** "+24.0 dB", "Fuzz", "6.0 kHz" / "Off", "On", "100%": as the knobs show them. */
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
} // namespace dist

//==============================================================================
/**
    DawIt Distortion: a distortion like a guitar pedal and amp. A low cut before the curve (tight),
    the drive into one of five curves (overdrive, distortion, fuzz, rectify, fold), the tone's high
    cut and a guitar speaker after it, output, a dry/wet mix for parallel distortion, and match
    level. The curve runs at four times the sample rate against aliasing, as in DawIt Saturation.
    It runs in the track's chain like DawIt EQ (see BuiltInFormat.h).

    For learning it also offers: a note to hear it with, only what it adds (the output minus a
    clean copy of the input, through the same filters, at the same level), and a history of how
    loud the track and what it adds were.
*/
class BuiltInDistortion final : public juce::AudioPluginInstance, public KeepsLatencyWhenBypassed
{
public:
    BuiltInDistortion();

    static constexpr const char* identifier = "DawIt Distortion";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInDistortion&)> createEditorFunction;

    static const char* getParameterId (dist::Knob);
    juce::RangedAudioParameter* getKnobParameter (dist::Knob) const;

    dist::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const dist::Settings&);
    void setKnob (dist::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    bool isMatchingLevel() const { return matchLevel->load() >= 0.5f; }
    void setMatchingLevel (bool);

    /** The gain that keeps the output as loud as the input right now, in dB; match level applies it. */
    std::atomic<float> matchGainDb { 0.0f };

    /** The loudest the track was over the last moment (a peak, as a gain), for the pictures. */
    std::atomic<float> recentPeak { 0.0f };

    /** Play only what the distortion adds. Not saved. */
    std::atomic<bool> listenToEffect { false };

    fx::Clap clap;
    fx::LevelHistory history;   // the effect's level is what the distortion adds, as mixed

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

    /** Biquads in a row, with their memory for each channel. */
    struct Chain
    {
        static constexpr int maxFilters = 8;
        std::array<eq::Biquad, maxFilters> filters;
        int num = 0;
        double memory[2][maxFilters][2] {};

        void set (const std::vector<eq::Biquad>&);
        float run (int channel, float x);
    };

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, dist::numKnobs> knobValues {};
    std::atomic<float>* matchLevel = nullptr;

    double rate = 44100.0;
    int blockSize = 512, latency = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> dry, delayLine;
    int delayWrite = 0;

    juce::SmoothedValue<float> drive, mix, outputGain, matchGain;
    dist::Settings shownFilters;
    bool hasFilters = false;
    Chain before, after, cleanBefore, cleanAfter;   // the clean ones make the copy that what it adds is measured against
    float dcIn[2] {}, dcOut[2] {}, cleanDcIn[2] {}, cleanDcOut[2] {};
    double sumWetDry = 0.0, sumDry = 0.0, powerIn = 0.0, powerOut = 0.0;
    float peakHold = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInDistortion)
};
} // namespace daw
