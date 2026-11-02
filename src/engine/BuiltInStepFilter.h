#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "EffectTools.h"
#include "StateVariableFilter.h"
#include "StepGrid.h"

namespace daw
{
namespace stf
{
    using filt::FilterType;
    using stepgrid::StepLength;
    using stepgrid::stepLengths;
    using stepgrid::numStepLengths;
    using stepgrid::defaultStepLength;
    using stepgrid::clampBpm;
    using stepgrid::getStepBeats;
    using stepgrid::getStepMs;
    using stepgrid::getStepIndex;

    constexpr int maxSteps = stepgrid::maxSteps;
    constexpr float minOwnBpm = stepgrid::minOwnBpm, maxOwnBpm = stepgrid::maxOwnBpm;

    /** The controls with a value of their own, in the order of the window. The 16 step heights are parameters too, but not knobs. */
    enum class Knob { stepLength, steps, tempo, ownBpm, type, slope, lowCutoff, highCutoff, resonance, glide, mix, output };
    constexpr int numKnobs = 12;

    constexpr float minCutoff = 20.0f, maxCutoff = 20000.0f;
    constexpr float minOutputDb = -24.0f, maxOutputDb = 6.0f;

    /** The cutoff never goes above this share of the sample rate (19.8 kHz at 44.1 kHz). */
    constexpr double maxCutoffShare = 0.45;

    /** Even Glide 0% takes this long to move to a step's cutoff, so it doesn't click. */
    constexpr double minGlideMs = 2.0;

    /** Every step open: the heights of a pattern that does nothing until you lower a step. */
    constexpr std::array<float, maxSteps> fullHeights()
    {
        std::array<float, maxSteps> h {};

        for (auto& v : h)
            v = 100.0f;

        return h;
    }

    struct Settings
    {
        int stepLength = defaultStepLength;   // index in stepLengths
        int steps = maxSteps;                 // how many steps before the pattern starts again
        bool ownTempo = false;                // false: follow the song's tempo
        float ownBpm = 120.0f;                // the tempo of its own clock, also while the song gives none
        FilterType type = FilterType::lowPass;
        bool steep = true;                    // 24 dB per octave; false is 12
        float lowHz = 300.0f;                 // the cutoff of a step at 0%
        float highHz = 20000.0f;              // the cutoff of a step at 100%
        float resonancePercent = 0.0f;
        float glidePercent = 10.0f;           // of a step
        float mixPercent = 100.0f;
        float outputDb = 0.0f;
        std::array<float, maxSteps> heights = fullHeights();   // 0 to 100% of the way from Low to High, in octaves
    };

    /** "Low-pass", "Band-pass", "High-pass". */
    juce::String getTypeName (FilterType);

    /** The cutoff of a step at `heightPercent` between the two knobs, spaced in octaves: Low x (High / Low) ^ height. The lower of the two is Low whichever knob holds it. */
    double getHeightCutoffHz (double lowHz, double highHz, double heightPercent);

    /** The cutoff of step `step` (0 is the first) of the pattern, before the limit of the sample rate. */
    double getStepCutoffHz (const Settings&, int step);

    /** A cutoff kept between 20 Hz and 45% of the sample rate. */
    double clampCutoff (double hz, double sampleRate);

    /** How long the cutoff takes to move to a new step's value, in ms: Glide's share of the step, never under 2 ms. */
    double getGlideMs (const Settings&, double bpm);

    /** "20 Hz" to "20.0 kHz". */
    juce::String formatHz (double hz);

    /** "1/16", "16", "Song" / "Own", "120 BPM", "Low-pass", "24 dB", "300 Hz", "20.0 kHz", "65%", "10%", "100%", "-9.0 dB": as the window shows them. */
    juce::String formatKnob (Knob, const Settings&);

    /** The same from a bare parameter value (a choice's index, a percentage, Hz, dB or BPM). */
    juce::String formatKnobValue (Knob, float value);

    /** The pattern as a line of text, to copy and paste: "DawIt Step Filter pattern: 1/16 16 100 50 0 ...", with the step length, Steps and the 16 heights in percent. */
    juce::String patternToText (const Settings&);

    /** Reads such a line into `s`, changing only its step length, Steps and heights. False (and `s` untouched) when the text isn't a pattern. */
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
    /** What happened in one step, as the step filter heard it: for the pictures and for `analyse`. Levels are linear. */
    struct StepRecord
    {
        int step = 0;                 // 0 is the first step of the pattern
        float cutoffHz = 0.0f;        // where the step's cutoff was heading
        float inPeak = 0.0f, inRms = 0.0f, outPeak = 0.0f, outRms = 0.0f;
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
            std::array<std::atomic<float>, 6> values {};
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
        double bpm = 120.0;              // the tempo the step filter used
        bool hostHasTempo = true;        // the host gave a tempo
    };

    /** The advice for *Your track*, rules F0 to F6 of the plan, as lines of text. */
    std::vector<juce::String> analyse (const AnalysisInput&);
} // namespace stf

//==============================================================================
/**
    DawIt Step Filter: gives each step of a repeating pattern its own filter cutoff, in time with the
    song. It follows the song's clock when the song plays (tempo and position from the play head), and
    a clock of its own at the song's tempo otherwise. Each step's height (0 to 100%) sets its cutoff
    between Low and High in octaves; the cutoff glides to each new step over Glide's share of the step,
    never under 2 ms. The result is mixed with the untouched track in straight proportion, then Output
    sets the level. It holds nothing back: no latency.

    For learning it also offers: a test bar of a held chord that plays from step 1, the part the filter
    takes away on its own, and a history of the steps for the window's pictures.
*/
class BuiltInStepFilter final : public juce::AudioPluginInstance
{
public:
    BuiltInStepFilter();

    static constexpr const char* identifier = "DawIt Step Filter";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInStepFilter&)> createEditorFunction;

    /** The parameters: the knobs, and "height1" to "height16" for the steps. */
    static const char* getParameterId (stf::Knob);
    static juce::String getHeightParameterId (int step);
    juce::RangedAudioParameter* getKnobParameter (stf::Knob) const;
    juce::RangedAudioParameter* getHeightParameter (int step) const;

    stf::Settings getSettings() const;
    float getHeight (int step) const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const stf::Settings&);
    void setKnob (stf::Knob, float value);
    void setHeight (int step, float percent);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** The tempo the steps follow now, in BPM. */
    double getUsedBpm() const { return usedBpm.load(); }

    /** Play only what the filter takes away (the input minus the filtered sound), at full level. Not saved. */
    std::atomic<bool> listenToEffect { false };

    /** The host's tempo, 0 while unknown. */
    std::atomic<double> tempoBpm { 0.0 };

    /** The step (0 is the first) playing now, -1 before any sound; the position in quarter notes at the start of the last block; and whether it follows the song's play head. */
    std::atomic<int> playingStep { -1 };
    std::atomic<double> clockBeat { 0.0 };
    std::atomic<bool> usingSongClock { false };

    /** The filter's cutoff at the end of the last block, in Hz (the glide in progress included). */
    std::atomic<float> currentCutoffHz { 20000.0f };

    fx::Clap clap;   // only its request flag is used: the step filter plays its own test chord
    stf::StepHistory history;
    fx::LevelHistory levels;   // per 64 samples: the input's peak, and the peak of what the filter changed (output minus input)

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
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void resetState();
    void beginStep (long long step, bool jumped, const stf::Settings&, double stepSamples);
    void finishStep();
    void startChord();
    void addChord (juce::AudioBuffer<float>&, int numChannels, double beatsPerSample);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, stf::numKnobs> knobValues {};
    std::array<std::atomic<float>*, stf::maxSteps> heightValues {};

    double rate = 44100.0;
    stepgrid::Clock stepClock;
    std::atomic<double> usedBpm { 120.0 };

    // The filter: a stage of each kind per channel.
    std::array<filt::Svf, 2> first, second;
    bool wasSteep = true;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;

    // The step in progress, and the cutoff moving to its value (in octaves, as log2 of Hz).
    bool stepKnown = false, stepComplete = false, cutoffInit = false;
    long long stepAbs = 0;
    int stepIndex = 0;
    double stepBeatStart = 0.0;
    double startOct = 0.0, targetOct = 0.0, currentOct = 0.0, cachedOct = -1000.0, cachedG = 0.0;
    int glidePos = 0, glideLength = 1;
    double meterInSq = 0.0, meterOutSq = 0.0;
    float meterInPeak = 0.0f, meterOutPeak = 0.0f;
    int meterSamples = 0;

    // The level history's chunk.
    float chunkIn = 0.0f, chunkChange = 0.0f;
    int chunkCount = 0;

    // The test chord.
    bool chordActive = false;
    double chordBeat = 0.0;
    std::array<double, 5> chordPhase {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInStepFilter)
};
} // namespace daw
