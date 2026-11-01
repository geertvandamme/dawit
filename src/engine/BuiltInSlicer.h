#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "EffectTools.h"

namespace daw
{
namespace slc
{
    /** What a step does: plays the track through, plays the step before again, plays it backwards, or is silent. */
    enum class Mode { through, repeat, reverse, mute };
    constexpr int numModes = 4;
    constexpr int maxSteps = 16;

    /** "Through", "Repeat", "Reverse", "Mute". */
    const char* getModeName (Mode);

    /** The controls with a value of their own, in the order of the window. The 16 step modes are parameters too, but not knobs. */
    enum class Knob { stepLength, steps, roll, smooth, mix, tempo, ownBpm };
    constexpr int numKnobs = 7;

    /** A step length as a note value. `beats` are quarter notes. */
    struct StepLength
    {
        const char* name;
        double beats;
    };

    /** From short to long: 1/32 to 1/4, with the two triplets. */
    inline constexpr StepLength stepLengths[] = { { "1/32", 0.125 }, { "1/16T", 1.0 / 6.0 }, { "1/16", 0.25 }, { "1/8T", 1.0 / 3.0 }, { "1/8", 0.5 }, { "1/4", 1.0 } };
    constexpr int numStepLengths = (int) std::size (stepLengths);
    constexpr int defaultStepLength = 2;   // 1/16

    struct Settings
    {
        int stepLength = defaultStepLength;   // index in stepLengths
        int steps = maxSteps;                 // how many steps before the pattern starts again
        int roll = 1;                         // a Repeat step plays the first 1/roll of its slice this many times
        float smoothMs = 3.0f;                // the fade at every cut
        float mixPercent = 100.0f;
        bool ownTempo = false;                // false: follow the song's tempo
        float ownBpm = 120.0f;                // the tempo of its own clock, also while the song gives none
        std::array<Mode, maxSteps> modes {};  // all Through
    };

    constexpr float minSmooth = 0.5f, maxSmooth = 30.0f, minOwnBpm = 20.0f, maxOwnBpm = 300.0f;
    constexpr int maxRoll = 4;

    /** The tempo to slice at: the given one, kept between DawIt's slowest and fastest. */
    inline double clampBpm (double bpm) { return juce::jlimit ((double) minOwnBpm, (double) maxOwnBpm, bpm); }

    /** How many quarter notes a step lasts. */
    double getStepBeats (int stepLength);

    /** How long a step lasts at this tempo, in ms. */
    double getStepMs (int stepLength, double bpm);

    /** The step (0 is the first) of the pattern that a position in quarter notes falls in. Steps are counted from the start of the song. */
    int getStepIndex (double beats, int stepLength, int steps);

    /** How many samples each of the slicer's two slice memories holds per channel: the longest step, 1/4 at 20 BPM, with a little to spare. */
    int getSliceCapacity (double sampleRate);

    /** "1/16", "16", "1", "3.0 ms", "100%", "Song" / "Own", "120 BPM": as the window shows them. */
    juce::String formatKnob (Knob, const Settings&);

    /** The pattern as a line of text, to copy and paste: "DawIt Slicer pattern: 1/16 16 TTRRVVMM...", with the step length, Steps and one letter per step (T, R, V, M). */
    juce::String patternToText (const Settings&);

    /** Reads such a line into `s`, changing only its step length, Steps and modes. False (and `s` untouched) when the text isn't a pattern. */
    bool patternFromText (const juce::String&, Settings& s);

    /** A starting point with why its settings are what they are. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
    };

    /** The neutral start first, then the patterns. */
    const std::vector<Preset>& getPresets();

    //==============================================================================
    /** What happened in one step, as the slicer heard it: for the pictures and for `analyse`. Levels are linear. */
    struct StepRecord
    {
        int step = 0;                 // 0 is the first step of the pattern
        Mode mode = Mode::through;
        bool sliced = false;          // a Repeat or Reverse step that had a slice to play (otherwise it played through)
        float inPeak = 0.0f, inRms = 0.0f, outPeak = 0.0f, outRms = 0.0f;
        float slicePeak = 0.0f;       // the peak of the slice a Repeat or Reverse step played
        double beat = 0.0;            // where the step starts, in quarter notes
    };

    /** The newest step records: written by the audio thread, read (without taking them) by the window and the guide. */
    class StepHistory
    {
    public:
        static constexpr int size = 256;

        void push (const StepRecord&) noexcept;

        /** Copies up to `max` of the newest records, oldest first, and returns how many. */
        int getRecent (StepRecord* dest, int max) const;

        /** Forgets everything. Message thread. */
        void clear() noexcept { cleared = written.load(); }

    private:
        struct Slot
        {
            std::array<std::atomic<float>, 8> values {};
            std::atomic<double> beat { 0.0 };
        };

        std::array<Slot, (size_t) size> slots;
        std::atomic<unsigned long long> written { 0 };
        std::atomic<unsigned long long> cleared { 0 };
    };

    /** What `analyse` looks at. `records` are the steps of the last 2 bars, oldest first. */
    struct AnalysisInput
    {
        Settings settings;
        std::vector<StepRecord> records;
        double bpm = 120.0;              // the tempo the slicer used
        bool hostHasTempo = true;        // the host gave a tempo
    };

    /** The advice for *Your track*, rules S0 to S6 of the plan, as lines of text. */
    std::vector<juce::String> analyse (const AnalysisInput&);
} // namespace slc

//==============================================================================
/**
    DawIt Slicer: cuts a track into steps on the song's beat grid and, for each step, plays it
    through, plays the step before again (Repeat, with Roll for shorter pieces), plays it
    backwards (Reverse) or mutes it. It follows the song's clock when the song plays (tempo and
    position from the play head), and a clock of its own at the song's tempo otherwise. Every
    change of sound is a raised-cosine crossfade of Smooth ms, and the result is mixed with the
    untouched track in straight proportion. It holds nothing back: no latency.

    For learning it also offers: a test bar of drums over a chord that plays from step 1, the
    changes on their own, and a history of the steps for the window's pictures.
*/
class BuiltInSlicer final : public juce::AudioPluginInstance
{
public:
    BuiltInSlicer();

    static constexpr const char* identifier = "DawIt Slicer";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInSlicer&)> createEditorFunction;

    /** The parameters: the knobs, and "mode1" to "mode16" for the steps. */
    static const char* getParameterId (slc::Knob);
    static juce::String getModeParameterId (int step);
    juce::RangedAudioParameter* getKnobParameter (slc::Knob) const;
    juce::RangedAudioParameter* getModeParameter (int step) const;

    slc::Settings getSettings() const;
    slc::Mode getMode (int step) const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const slc::Settings&);
    void setKnob (slc::Knob, float value);
    void setMode (int step, slc::Mode);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** The tempo the steps follow now, in BPM. */
    double getUsedBpm() const { return usedBpm.load(); }

    /** How many samples each slice memory holds per channel (0 before playback is prepared). Never changes while playing. */
    int getSliceBufferSize() const { return (int) store[0][0].size(); }

    /** Play only what the slicer changes (the repeats and reverses), at full level. Not saved. */
    std::atomic<bool> listenToEffect { false };

    /** The host's tempo, 0 while unknown. */
    std::atomic<double> tempoBpm { 0.0 };

    /** The step (0 is the first) playing now, -1 before any sound; the position in quarter notes at the start of the last block; and whether it follows the song's play head. */
    std::atomic<int> playingStep { -1 };
    std::atomic<double> clockBeat { 0.0 };
    std::atomic<bool> usingSongClock { false };

    /** Whether the step that is playing is a Repeat or Reverse step that has a slice to play. */
    std::atomic<bool> playingSliced { false };

    fx::Clap clap;   // only its request flag is used: the slicer plays its own test bar
    slc::StepHistory history;

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
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

private:
    /** Where a sound comes from while a step plays: the track, silence, or a slice forwards or backwards. */
    struct Voice
    {
        enum class Kind { silent, input, forward, reverse };

        Kind kind = Kind::input;
        std::array<const float*, 2> data {};
        int pos = 0;      // samples played so far
        int length = 0;   // samples in the slice
        int piece = 0;    // a forward voice starts again after this many samples
        bool changes = false;   // counts as changed by the slicer (a repeat or reverse)
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void resetState();
    void beginStep (long long step, double phaseSamples, const slc::Settings&, double stepSamples);
    void finishStep();
    void startFade (const Voice& next, int pieceLength, float smoothMs);
    float readVoice (const Voice&, int channel, const float* in) const;
    void renderRange (juce::AudioBuffer<float>&, int from, int to, int numChannels, const slc::Settings&, bool listen);
    void startTestBar();
    void addTestBar (juce::AudioBuffer<float>&, int numChannels, double beatsPerSample, double bpm);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, slc::numKnobs> knobValues {};
    std::array<std::atomic<float>*, slc::maxSteps> modeValues {};

    double rate = 44100.0;
    int capacity = 0;
    std::vector<float> store[2][2];   // slice memory: [buffer][channel]
    int recIdx = 0;                   // the buffer the current step is recorded into; the other holds the slice of a run

    // The clock.
    bool havePrev = false, prevSongBased = false;
    double prevEndBeat = 0.0, ownBeat = 0.0;
    std::atomic<double> usedBpm { 120.0 };

    // The step in progress.
    bool stepActive = false, stepComplete = false, runHasSlice = false, lastComplete = false, usesSlice = false;
    long long stepAbs = 0;
    int stepIndex = 0, recCount = 0, lastLength = 0, heldLength = 0, tInStep = 0, pieceCount = 0;
    slc::Mode stepMode = slc::Mode::through;
    float lastInPeak = 0.0f, heldPeak = 0.0f;
    double stepBeatStart = 0.0, meterInSq = 0.0, meterOutSq = 0.0;
    float meterInPeak = 0.0f, meterOutPeak = 0.0f;
    int meterSamples = 0, heldIdx = 1;
    float mixNow = 1.0f;

    // What plays, and the fade from the sound before.
    Voice cur, old;
    bool fading = false;
    int fadePos = 0, fadeLength = 0;

    // The test bar.
    bool barActive = false;
    double barBeat = 0.0;
    juce::Random barNoise { 11 };
    float barHatState = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInSlicer)
};
} // namespace daw
