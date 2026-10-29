#pragma once

#include "BuiltInEq.h"
#include "EffectTools.h"

namespace daw
{
namespace verb
{
    /** The mechanism that makes the reverb; saved values must keep this order. */
    enum class Type { digital, room, hall, chamber, plate, spring };
    constexpr int numTypes = 6;

    /** The knobs, in the order of the window and of a preset's notes. */
    enum class Knob { predelay, size, decay, damping, lowCut, early, width, mix };
    constexpr int numKnobs = 8;

    /** Saved knobs and mechanism, with the original Digital sound as the default. */
    struct Settings
    {
        float predelayMs = 20.0f;
        float sizePercent = 50.0f;
        float decaySeconds = 1.8f;      // the time the tail takes to die away by 60 dB (RT60), in the lows and mids
        float dampingHz = 6000.0f;      // above it the tail dies away at least twice as fast; 20 kHz is off
        float lowCutHz = 150.0f;        // on the reverb only; 20 is off
        float earlyPercent = 50.0f;     // how loud the early reflections are
        float widthPercent = 100.0f;    // 0 is mono
        float mixPercent = 25.0f;
        Type type = Type::digital;

        /** Whether every saved knob and the mechanism are unchanged. */
        bool operator== (const Settings& other) const noexcept
        {
            return type == other.type && juce::exactlyEqual (predelayMs, other.predelayMs)
                && juce::exactlyEqual (sizePercent, other.sizePercent) && juce::exactlyEqual (decaySeconds, other.decaySeconds)
                && juce::exactlyEqual (dampingHz, other.dampingHz) && juce::exactlyEqual (lowCutHz, other.lowCutHz)
                && juce::exactlyEqual (earlyPercent, other.earlyPercent) && juce::exactlyEqual (widthPercent, other.widthPercent)
                && juce::exactlyEqual (mixPercent, other.mixPercent);
        }
    };

    constexpr float maxPredelay = 250.0f, minDecay = 0.1f, maxDecay = 12.0f, minDamping = 1000.0f, maxDamping = 20000.0f,
                    minLowCut = 20.0f, maxLowCut = 1000.0f;

    inline bool isDampingOn (float hz) { return hz < maxDamping - 1.0f; }
    inline bool isLowCutOn (float hz) { return hz > minLowCut + 0.5f; }

    /** How far across the room is that a size stands for: 5 m at 0% to 40 m at 100%. */
    double getRoomMetres (float sizePercent);

    /** How much later than the direct sound the first reflection arrives, in ms, before pre-delay. */
    double getFirstReflectionMs (float sizePercent);

    /** How long sound takes to travel `metres` in air, in ms (343 m/s). */
    inline double getTravelMs (double metres) { return metres / 343.0 * 1000.0; }

    /** What a sound keeps of its level on one trip of `delaySeconds` through a delay line, so that
        it has died away by 60 dB after `decaySeconds`: 10^(-3 x delay / decay). */
    double getLoopGain (double delaySeconds, double decaySeconds);

    /** The pole of the one-pole low-pass in each delay line: it keeps `loopGain` at the lows, and
        at `dampingHz` only loopGain squared, so the tail dies away twice as fast there (and faster
        still above). 0 is no damping. */
    double getDampingPole (double loopGain, double dampingHz, double sampleRate);

    /** Sabine's reverberation time of a room in seconds: 0.161 x volume (m3) / absorption (m2,
        each square metre of open window counting 1). */
    inline double getSabineDecay (double volume, double absorption) { return 0.161 * volume / juce::jmax (0.01, absorption); }

    /** "20 ms", "50%", "1.80 s", "6.0 kHz" / "Off", "150 Hz" / "Off": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point for a sound, with why each knob is set the way it is. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob
        juce::String typeNote;
    };

    /** The neutral start first, then the sounds. */
    const std::vector<Preset>& getPresets();

    /** "Return: plate": where DawIt Reverb starts on a return track (100% mix). */
    const Preset& getReturnPreset();

    //==============================================================================
    /**
        The reverb itself, without the dry sound. The input (mono) is held back by the pre-delay,
        then goes two ways:
          - early reflections: twelve single echoes, spaced by the room size, panned left and right;
          - the tail: four all-pass filters smear it out, then eight delay lines feed each other
            through a mixing matrix (a feedback delay network). Each pass through a line takes a
            little level off (the decay) and a little more off the highs (the damping).
        The two are added, the lows are cut, and the width sets how stereo it is.
    */
    class Network
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void setSettings (const Settings&);

        /** Writes the reverb of `numSamples` of input into outL and outR. inR may be null. */
        void process (const float* inL, const float* inR, float* outL, float* outR, int numSamples);

        /** For pictures: leave out the early reflections or the tail. */
        bool withEarly = true, withTail = true;

        static constexpr int numLines = 8, numTaps = 12, numDiffusers = 4;

    private:
        struct Delay
        {
            std::vector<float> data;
            int write = 0;

            void setMaximum (int samples) { data.assign ((size_t) juce::jmax (1, samples), 0.0f); write = 0; }
            void push (float v) { data[(size_t) write] = v; write = (write + 1) % (int) data.size(); }
            float read (int samplesAgo) const { return data[(size_t) ((write - 1 - samplesAgo + 2 * (int) data.size()) % (int) data.size())]; }
        };

        double rate = 44100.0;
        Settings settings;

        Delay predelay;
        int predelaySamples = 0;

        std::array<int, numTaps> tapSamples {};
        std::array<float, numTaps> tapLeft {}, tapRight {};
        float earlyGain = 0.0f, earlyPole = 0.0f, earlyStateL = 0.0f, earlyStateR = 0.0f;

        std::array<Delay, numDiffusers> diffusers;
        std::array<std::array<Delay, numDiffusers>, 2> chamberDiffusers;
        std::array<int, numDiffusers> diffuserSamples {};

        std::array<Delay, numLines> lines;
        std::array<int, numLines> lineSamples {};
        std::array<float, numLines> lineGain {}, linePole {}, lineState {};
        float inputGain = 0.0f;
        float diffusionAmount = 0.6f, typeGain = 1.0f;
        std::array<Delay, 2> springLoops, springPickups;
        std::array<std::array<Delay, 100>, 2> springDispersion;
        std::array<int, 2> springSamples {}, springArrival {};
        std::array<float, 2> springGain {}, springPole {}, springState {};
        float springOutputPole = 0.0f;
        std::array<std::array<float, 6>, 2> springOutputMemory {};
        juce::SmoothedValue<float> typeFade;


        eq::Stages lowCut;
        double lowCutMemory[2][2][2] {};
        bool lowCutOn = false;
    };

    /** What one click turns into with these settings: only the reverb, stereo, `seconds` long. */
    juce::AudioBuffer<float> renderImpulse (const Settings&, double sampleRate, double seconds, bool early = true, bool tail = true);

    /** The level of a stereo response over time in dB: the RMS of each `windowMs`. */
    std::vector<float> getEnvelopeDb (const juce::AudioBuffer<float>&, double sampleRate, double windowMs);

    /** How long a response takes to die away by 60 dB, in seconds, measured the way acousticians do:
        Schroeder's backward integration, the slope from -5 to -25 dB, times three. 0 when it is too short. */
    double measureDecay (const juce::AudioBuffer<float>&, double sampleRate);
} // namespace verb

//==============================================================================
/**
    DawIt Reverb: puts a track in a room. Pre-delay, room size, decay time, damping of the highs,
    a low cut on the reverb, early reflections, stereo width and a dry/wet mix. It runs in the
    track's chain like DawIt EQ (see BuiltInFormat.h).

    For learning it also offers: a clap to hear the room with, the reverb on its own, and a history
    of how loud the track and its reverb were, for the editor's graph.
*/
class BuiltInReverb final : public juce::AudioPluginInstance
{
public:
    BuiltInReverb();

    static constexpr const char* identifier = "DawIt Reverb";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInReverb&)> createEditorFunction;

    static const char* getParameterId (verb::Knob);
    juce::RangedAudioParameter* getKnobParameter (verb::Knob) const;

    verb::Settings getSettings() const;

    /** The choice of mechanism, without changing any knob. */
    juce::RangedAudioParameter* getTypeParameter() const;
    /** Switches the mechanism while keeping every knob. Message thread. */
    void setType (verb::Type);

    /** Hear a showcase clap in its own network, leaving the track and its settings alone. */
    void requestAudition (verb::Type kind) noexcept { auditionRequest = (int) kind; }
    bool isAuditionActive() const noexcept { return auditionActive.load(); }


    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const verb::Settings&);
    void setKnob (verb::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Play only the reverb, at full level. Not saved. */
    std::atomic<bool> listenToEffect { false };

    /** The host's tempo, 0 while unknown. */
    std::atomic<double> tempoBpm { 0.0 };

    fx::Clap clap;
    fx::LevelHistory history;

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override;
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, verb::numKnobs> knobValues {};

    std::atomic<float>* typeValue = nullptr;
    double rate = 44100.0;
    verb::Network network, auditionNetwork;
    fx::Clap auditionClap;
    std::atomic<int> auditionRequest { -1 };
    std::atomic<bool> auditionActive { false };
    int auditionRemaining = 0;
    std::array<verb::Settings, verb::numTypes> auditionSettings;
    juce::AudioBuffer<float> auditionInput, auditionWet;

    verb::Settings applied;
    bool appliedOnce = false;
    juce::AudioBuffer<float> wet;
    juce::SmoothedValue<float> dryGain, wetGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInReverb)
};
} // namespace daw
