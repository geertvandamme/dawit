#pragma once

#include "BuiltInEq.h"

namespace daw
{
namespace comp
{
    enum class Detection { peak, rms };

    /** The knobs, in the order of the window and of a preset's notes. */
    enum class Knob { threshold, ratio, knee, attack, release, makeup, mix, sidechain, detection };
    constexpr int numKnobs = 9;

    struct Settings
    {
        float thresholdDb = -18.0f;
        float ratio = 3.0f;
        float kneeDb = 6.0f;
        float attackMs = 10.0f;
        float releaseMs = 150.0f;
        float makeupDb = 0.0f;
        float mixPercent = 100.0f;
        float sidechainHz = 20.0f;   // the corner of the low cut in what the compressor listens to; 20 is off
        Detection detection = Detection::peak;
    };

    constexpr float minThreshold = -60.0f, maxThreshold = 0.0f, minRatio = 1.0f, maxRatio = 20.0f, maxKnee = 24.0f,
                    minAttack = 0.1f, maxAttack = 200.0f, minRelease = 10.0f, maxRelease = 2000.0f,
                    minMakeup = -12.0f, maxMakeup = 24.0f, minSidechain = 20.0f, maxSidechain = 500.0f;

    /** The time over which RMS detection averages the level. */
    constexpr double rmsWindowMs = 10.0;

    inline bool isSidechainFilterOn (float hz) { return hz > minSidechain + 0.5f; }

    /** The static curve: how many dB a detector level is turned down by (0 or more). With a knee,
        the ratio fades in from knee/2 below the threshold to knee/2 above it (Giannoulis, Massberg
        and Reiss, "Digital Dynamic Range Compressor Design", 2012). */
    double getReductionDb (const Settings&, double levelDb);

    /** The threshold at which the most these levels get turned down is `reductionDb`, with the
        settings' ratio, knee, attack and release: what a track that played them needs. The levels
        are what the detector heard, `rate` per second. Short hits need a lower threshold than a
        steady level, because a slow attack doesn't fully catch them. The nearest end of the range
        when no threshold gets there. */
    float findThreshold (const Settings&, const std::vector<float>& levelsDb, double rate, double reductionDb);

    /** The memory of a one-pole smoother with this time constant: after `ms` it has gone about 63%
        (1 - 1/e) of the way to a new value. */
    double getCoefficient (double ms, double sampleRate);

    /** Moves the gain reduction (in dB) towards the curve's value: with the attack time while the
        compressor turns down more, with the release time while it lets go. */
    struct Ballistics
    {
        double attack = 0.0, release = 0.0, state = 0.0;

        void setTimes (double attackMs, double releaseMs, double sampleRate);

        double process (double targetDb)
        {
            const double c = targetDb > state ? attack : release;
            state = targetDb + c * (state - targetDb);
            return state;
        }
    };

    /** For the explainer's picture: the gain reduction over time, in dB, for a level that follows
        `levelDb` (one value per sample at `sampleRate`). RMS detection averages the level first. */
    std::vector<float> simulateReduction (const Settings&, const std::vector<float>& levelDb, double sampleRate);

    /** "-18.0 dB", "4.0:1", "10 ms", "1.20 s", "120 Hz" / "Off", "100%": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point for an instrument, with why each knob is set the way it is. */
    struct Preset
    {
        juce::String name, why;
        float targetReductionDb = 4.0f;   // how much the loudest parts should be turned down
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob
    };

    /** The neutral start first, then the instruments. */
    const std::vector<Preset>& getPresets();
} // namespace comp

//==============================================================================
/**
    DawIt Compressor: turns down whatever goes over a threshold, by a ratio, with a knee, attack and
    release, makeup gain, a dry/wet mix for parallel compression, a low cut on what it listens to
    (sidechain filter) and peak or RMS detection. Its stereo channels are compressed together, so
    the image doesn't shift. It runs in the track's chain like DawIt EQ (see BuiltInFormat.h).

    For learning it also offers, while the track plays:
      - hear what it removes: the difference between the input and the compressed signal;
      - match level: makeup that keeps the output as loud as the input, to compare honestly;
      - a history of the levels and the gain reduction, for the editor's graphs (readHistory);
      - the song's tempo, to suggest a release that breathes with it.
*/
class BuiltInCompressor final : public juce::AudioPluginInstance
{
public:
    BuiltInCompressor();

    static constexpr const char* identifier = "DawIt Compressor";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInCompressor&)> createEditorFunction;

    static const char* getParameterId (comp::Knob);
    juce::RangedAudioParameter* getKnobParameter (comp::Knob) const;

    comp::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const comp::Settings&);
    void setKnob (comp::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    void applyPreset (const comp::Preset&);

    bool isMatchingLevel() const { return matchLevel->load() >= 0.5f; }
    void setMatchingLevel (bool);

    /** Play only what the compressor takes away (input minus compressed). Not saved. */
    std::atomic<bool> listenToRemoved { false };

    /** The makeup that would make the output as loud as the input right now, in dB; match level applies it. */
    std::atomic<float> matchGainDb { 0.0f };

    /** The host's tempo, 0 while unknown. */
    std::atomic<double> tempoBpm { 0.0 };

    /** The levels of one short stretch (historyChunk samples): peaks in and out, what the detector
        heard, and the most it turned down, all in dB. */
    struct HistoryPoint
    {
        float inputDb = -100.0f, outputDb = -100.0f, detectorDb = -100.0f, reductionDb = 0.0f;
    };

    static constexpr int historyChunk = 32;

    /** Takes up to `max` of the newest points, oldest first. One reader at a time (the editor). */
    int readHistory (HistoryPoint* dest, int max);

    /** The loudest level the compressor heard since the last call (after its sidechain filter and
        detection, in dB; -100 when nothing played) and how much it turned that down. For a dot on a
        small curve elsewhere than the editor, such as the device bar; it works next to the editor. */
    struct Preview { float detectorDb = -100.0f, reductionDb = 0.0f; };
    Preview takePreview();

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

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, comp::numKnobs> knobValues {};
    std::atomic<float>* matchLevel = nullptr;

    double rate = 44100.0;
    comp::Ballistics ballistics;
    double rmsCoefficient = 0.0, rmsState = 0.0;
    float sidechainShown = -1.0f;
    eq::Stages sidechainStages;
    double sidechainMemory[2][2][2] {};
    double powerIn = 0.0, powerOut = 0.0;
    juce::SmoothedValue<float> makeupGain, mix, matchGain;

    static constexpr int historySize = 32768;
    juce::AbstractFifo historyFifo { historySize };
    std::vector<HistoryPoint> history = std::vector<HistoryPoint> ((size_t) historySize);

    std::atomic<float> previewDetectorDb { -100.0f }, previewReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInCompressor)
};
} // namespace daw
