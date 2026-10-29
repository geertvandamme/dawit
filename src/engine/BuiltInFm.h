#pragma once

#include "BuiltInSubtractive.h"

#include <juce_dsp/juce_dsp.h>

namespace daw
{
namespace fm
{
    /** Four operators, as on Yamaha's DX21, DX100 and TX81Z: enough for every classic FM sound in two pairs or one stack. */
    constexpr int numOperators = 4;

    /** The knobs and switches, in the order of the window and of a preset's notes. The first seven belong to an
        operator: the window shows them for the one selected, and each operator has its own parameters. */
    enum class Knob
    {
        coarse, fine, level, attack, decay, sustain, release,
        algorithm, feedback,
        lfoRate, lfoDepth, lfoTarget,
        glide, mode, velocity, output
    };

    constexpr int numKnobs = 16, numOperatorKnobs = 7;

    bool isOperatorKnob (Knob);

    /** Pitch: vibrato. Brightness: the modulators' levels, so the sound gets brighter and duller. Volume: tremolo. */
    enum class LfoTarget { pitch, brightness, volume };
    constexpr int numLfoTargets = 3;

    juce::String getLfoTargetName (LfoTarget);

    struct Operator
    {
        int coarse = 1;              // the ratio to the note: 0 is 0.5, then 1 to 16
        float finePercent = 0.0f;    // the ratio a little higher: 41% makes 1 into 1.41
        float levelPercent = 0.0f;   // a carrier's loudness, or a modulator's index (100% is maxIndex)
        sub::Envelope env;
    };

    struct Settings
    {
        std::array<Operator, numOperators> ops;   // operator 1 first
        int algorithm = 0;
        float feedbackPercent = 0.0f;             // operator 4 modulating itself

        float lfoRateHz = 5.0f;
        float lfoDepthPercent = 0.0f;
        LfoTarget lfoTarget = LfoTarget::pitch;

        float glideSeconds = 0.0f;
        sub::Mode mode = sub::Mode::poly;
        float velocityPercent = 50.0f;            // how much softer notes are quieter and duller
        float outputDb = 0.0f;

        Settings();
    };

    constexpr int maxCoarse = 16;
    constexpr float maxFine = 99.0f, minOutput = -24.0f, maxOutput = 6.0f, maxGlide = 2.0f, minTime = 0.001f, maxTime = 20.0f,
                    minLfoRate = 0.05f, maxLfoRate = 20.0f;

    /** A modulator at 100% swings its carrier's phase by up to 10 radians: the modulation index, I. */
    constexpr double maxIndex = 10.0;

    /** At 100%, operator 4 feeds back 1.5 times its own output, about where its sine has become a saw. */
    constexpr double maxFeedback = 1.5;

    /** At 100% depth, the LFO moves the pitch by up to 2 semitones, the modulators from nothing to twice
        their level, and the volume all the way down. */
    constexpr float lfoPitchSemitones = 2.0f;

    /** How loud one voice is at full velocity before the output knob: a carrier at 100% peaks at about -12 dB. */
    constexpr float voiceGain = 0.25f, sinePeakDb = -12.0f;

    //==============================================================================
    /** How the operators are wired: which ones modulate each operator, and which are carriers, heard at the
        output. Operators are bits: 1 is operator 1, 2 operator 2, 4 operator 3, 8 operator 4. A modulator
        always has a higher number than what it modulates, so they're worked out from 4 down to 1. */
    struct Algorithm
    {
        juce::String name, wiring;
        std::array<int, numOperators> modulatedBy;
        int carriers;
    };

    constexpr int numAlgorithms = 8;

    /** Stack, Two into a chain, Chain and branch, Three into one, Two pairs, One into three, A pair and two sines, Four sines. */
    const Algorithm& getAlgorithm (int index);

    /** Whether operator `op` (0 to 3) is heard at the output in this algorithm. */
    bool isCarrier (const Settings&, int op);

    /** Which operators operator `op` modulates, as bits; 0 for a carrier. */
    int getTargets (const Settings&, int op);

    /** The ratio of a coarse setting: 0.5 for 0, otherwise the number. */
    double getCoarseRatio (int coarse);

    /** How many times the note's frequency an operator runs at: coarse x (1 + fine / 100), as on a DX7. */
    double getRatio (const Operator&);

    /** How much a note at this velocity (0 to 1) keeps of its level and of its modulation. */
    float getVelocityScale (const Settings&, float velocity);

    /** The turn-down that keeps the carriers' power no more than one full sine's: 1 / sqrt (the sum of
        their levels squared), when that's more than 1. Adding carriers makes the sound fuller, not louder. */
    double getNormalisation (const Settings&);

    /** How far operator `op` swings what it modulates, at the peak of its envelope: its index, in radians. 0 for a carrier. */
    double getIndex (const Settings&, int op, float velocity = 1.0f, float lfo = 0.0f);

    /** What operator `op` puts out at the peak of its envelope: a carrier's amplitude (1 is a full sine,
        after the normalisation), or a modulator's index. */
    double getAmplitude (const Settings&, int op, float velocity = 1.0f, float lfo = 0.0f);

    /** Each operator's envelope `seconds` after a note's start, let go `releasedAt` seconds after it (or
        held when negative); all 1 when `seconds` is negative. */
    std::array<float, numOperators> getEnvelopes (const Settings&, double seconds, double releasedAt = -1.0);

    //==============================================================================
    /** J_n(x), the Bessel function of the first kind: how loud the nth sideband is at an index of x, when
        a sine modulates a sine. */
    double getBessel (int n, double x);

    /** The bandwidth that holds about 98% of an FM sound's power, by Carson's rule: 2 (I + 1) times the
        modulator's frequency. */
    double getCarsonHz (double index, double modulatorHz);

    /** A partial of the sound: its frequency (or ratio to the note) and its amplitude. */
    struct Peak
    {
        double hz = 0.0, amplitude = 0.0;
    };

    /** The sidebands of one sine modulating another, worked out from the Bessel functions: at carrier
        +/- k x modulator, sidebands below 0 folding back upside down. As ratios to the note. */
    std::vector<Peak> getSidebands (double carrierRatio, double modulatorRatio, double index);

    /** A stretch of the sound as the voice plays it before its gain (a full carrier peaks at 1), with each
        operator's envelope at `envelopes`; without the modulators when `modulated` is false. */
    std::vector<float> render (const Settings&, double noteHz, const std::array<float, numOperators>& envelopes, double sampleRate,
                               int numSamples, float velocity = 1.0f, bool modulated = true);

    /** The partials of a note at `noteHz` with each operator's envelope at `envelopes`, measured from the
        sound itself, up to `maxHz`, louder than -70 dB. */
    std::vector<Peak> getSpectrum (const Settings&, double noteHz, const std::array<float, numOperators>& envelopes, float velocity = 1.0f,
                                   double maxHz = 12000.0);

    //==============================================================================
    /** "x2", "+41%", "80%", "350 ms", "Stack", "Off": as the knobs show them, for operator `op` where it's one of its knobs. */
    juce::String formatKnob (Knob, const Settings&, int op = 0);

    /** "1.41" or "14": an operator's ratio as a number. */
    juce::String formatRatio (double ratio);

    /** The knob's name in the window. */
    juce::String getKnobName (Knob);

    /** A starting point for a sound, with why it's made the way it is, what each operator does in it, and
        notes on the other knobs that matter. */
    struct Preset
    {
        juce::String name, why;
        std::array<juce::String, numOperators> operators;   // empty for an operator that's off
        Settings settings;
        std::array<juce::String, numKnobs> notes;           // by Knob, for the knobs that aren't an operator's
    };

    /** Init (one sine, the modulator at 0) first, then the sounds. */
    const std::vector<Preset>& getPresets();
} // namespace fm

//==============================================================================
/**
    DawIt FM: an FM synthesiser that explains itself. Four operators, each a sine at a ratio of the note
    with its own level and envelope, are wired by one of eight algorithms: a modulator swings the phase
    of the operator below it at audio speed, which adds sidebands (new partials) at the carrier plus and
    minus multiples of the modulator, more the higher its level (the index). Operator 4 can modulate
    itself (feedback). An LFO moves the pitch, the modulators or the volume. Poly or mono with glide.
    It's an instrument in the track's chain (see BuiltInFormat.h), played by the track's MIDI notes.

    Like a DX7 it does phase modulation, which gives the same sidebands as frequency modulation and
    keeps the pitch steady. It runs at twice the sample rate inside, so the sidebands above what the
    rate can hold fold back much less.

    For learning it also offers: a test note to play from the window, the carriers without their
    modulators, a history of how loud the carriers were and the whole sound, and the newest note's
    envelopes for the pictures.
*/
class BuiltInFm final : public juce::AudioPluginInstance
{
public:
    BuiltInFm();

    static constexpr const char* identifier = "DawIt FM";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInFm&)> createEditorFunction;

    /** The parameter of a knob, for operator `op` (0 to 3) when it's one of an operator's: "op2Level", "feedback". */
    static juce::String getParameterId (fm::Knob, int op = 0);
    juce::RangedAudioParameter* getKnobParameter (fm::Knob, int op = 0) const;

    fm::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const fm::Settings&);
    void setKnob (fm::Knob, float value, int op = 0);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Hear the carriers without their modulators: plain sines. Not saved. */
    std::atomic<bool> withoutModulators { false };

    /** "Play a note" in the window plays the test note, A3, for 0.8 seconds. */
    fx::Clap clap;

    /** How loud the carriers would peak without modulation (as the input), and the whole sound (as the effect). */
    fx::LevelHistory history;

    /** The newest note, for the pictures: each operator's envelope now, how long ago it started (-1 when
        no note plays), how long after its start it was let go (-1 while held), its velocity and the LFO's value. */
    std::array<std::atomic<float>, fm::numOperators> shownEnvelopes;
    std::atomic<float> noteAge { -1.0f }, releaseAge { -1.0f }, shownVelocity { 1.0f }, lfoValue { 0.0f };

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return createEditorFunction != nullptr; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int numVoices = 16;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Voice
    {
        int note = -1;
        float velocity = 0.0f;
        bool held = false;
        juce::uint64 startedAt = 0;              // which note-on it was, for stealing the oldest
        double pitch = 0.0, targetPitch = 0.0;   // in MIDI notes; they differ while gliding
        std::array<double, fm::numOperators> phases {};   // in cycles
        std::array<juce::ADSR, fm::numOperators> envs;
        std::array<float, fm::numOperators> lastEnvs {};
        double feedback1 = 0.0, feedback2 = 0.0;          // operator 4's last two outputs
        double age = 0.0, releasedAt = -1.0;              // seconds

        bool isActive (int carriers) const noexcept;
    };

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void startVoice (Voice&, int note, float velocity, bool retrigger);
    void renderVoice (Voice&, const fm::Settings&, float lfo, float* out, float& carrierPeak, int numSamples);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::array<std::atomic<float>*, fm::numOperatorKnobs>, fm::numOperators> operatorValues {};
    std::array<std::atomic<float>*, fm::numKnobs> knobValues {};

    double rate = 44100.0;
    int maxBlock = 512;
    std::array<Voice, numVoices> voices;
    std::vector<int> heldNotes;   // in the order they were pressed, for mono
    juce::uint64 noteCounter = 0;
    double lastPitch = -1.0;      // where the next note glides from
    double lfoPhase = 0.0;
    juce::SmoothedValue<float> outputGain;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> mono;
    std::vector<float> carrierLevels;   // per sample of a block: how loud the carriers would peak unmodulated
    int testNoteLeft = -1;        // samples until the test note is let go
    float chunkBefore = 0.0f, chunkAfter = 0.0f;
    int chunkFill = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInFm)
};
} // namespace daw
