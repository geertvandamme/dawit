#pragma once

#include "BuiltInCompressor.h"
#include "BuiltInEq.h"
#include "EffectTools.h"

namespace daw
{
namespace gate
{
    /** The knobs, in the order of the window and of a preset's notes. */
    enum class Knob { threshold, hysteresis, range, ratio, attack, hold, release, keyLow, keyHigh };
    constexpr int numKnobs = 9;

    struct Settings
    {
        float thresholdDb = -40.0f;    // the level the key must reach to open the gate
        float hysteresisDb = 4.0f;     // how far under the threshold it must fall before the gate closes
        float rangeDb = 80.0f;         // the most the gate turns the sound down, stored positive
        float ratio = 10.0f;           // 1:1 to 10:1; the far end (10) is a gate
        float attackMs = 0.5f;         // time constant of the opening
        float holdMs = 20.0f;          // how long it stays open after falling under the closing level
        float releaseMs = 100.0f;      // time constant of the closing
        float keyLowHz = 20.0f;        // low cut of the key; 20 is off
        float keyHighHz = 20000.0f;    // high cut of the key; 20000 is off
    };

    constexpr float minThreshold = -80.0f, maxThreshold = 0.0f, maxHysteresis = 12.0f, maxRange = 80.0f, minRatio = 1.0f, maxRatio = 10.0f,
                    minAttack = 0.01f, maxAttack = 50.0f, maxHold = 1000.0f, minRelease = 5.0f, maxRelease = 2000.0f,
                    minKeyLow = 20.0f, maxKeyLow = 4000.0f, minKeyHigh = 100.0f, maxKeyHigh = 20000.0f;

    /** How fast the detector lets go of a peak (a time constant), how soon after a close a reopening
        counts as a flicker, how long one moment of the history is, and how much history there is. */
    constexpr double detectorMs = 10.0, flickerMs = 20.0, momentMs = 2.0, recentSeconds = 4.0;

    /** How many moments the history holds: 4 seconds in steps of 2 ms. */
    constexpr int numMoments = 2000;

    /** The rate of the examples' level curves. */
    constexpr double exampleRate = 10000.0;

    /** Whether a ratio is the far end of the knob: a gate rather than an expander. */
    inline bool isGate (float ratio) { return ratio >= 9.95f; }
    inline bool isKeyLowOn (float hz) { return hz > minKeyLow + 0.5f; }
    inline bool isKeyHighOn (float hz) { return hz < maxKeyHigh - 1.0f; }

    /** How many dB the sound is turned down by at a key level: 0 while the gate is open; closed, the
        whole range for a gate, else (threshold - level) x (ratio - 1), at most the range and never
        below 0. */
    double getReductionDb (const Settings&, double levelDb, bool open);

    /** What the gate listens to, in dB at `hz`: the low cut and the high cut together (12 dB/oct each).
        The default rate is high, so the numbers are those of the ideal Butterworth filter (a high cut
        at 2 kHz is 24.1 dB down at 8 kHz) rather than of one running at 48 kHz, where the top is a little lower. */
    double getKeyResponseDb (const Settings&, double hz, double sampleRate = 192000.0);

    /** The key's peak: rises at once, falls with the detector's time constant (about 8.7 dB per 10 ms),
        so the moments a wave crosses zero don't count as silence. */
    struct Detector
    {
        double coefficient = 0.0, level = 0.0;

        void setRate (double sampleRate) { coefficient = comp::getCoefficient (detectorMs, sampleRate); }

        double process (double x)
        {
            level = juce::jmax (x, level * coefficient);
            return level;
        }
    };

    /** Open or closed. Opens when the level reaches the threshold; while the level stays at or above
        the closing level (threshold minus hysteresis) the hold is reloaded; closes when the level has
        been under it for the whole hold. Counts closes, and flickers: a reopening within flickerMs of a close. */
    struct OpenState
    {
        bool open = false;
        int holdLeft = 0, closes = 0, flickers = 0;
        long sinceClose = 1000000000;

        /** Returns true while the gate is open. */
        bool process (double levelDb, const Settings& s, int holdSamples, int flickerSamples)
        {
            if (sinceClose < 1000000000)
                ++sinceClose;

            if (levelDb >= (double) s.thresholdDb && ! open)
            {
                open = true;
                holdLeft = holdSamples;

                if (sinceClose <= flickerSamples)
                    ++flickers;
            }
            else if (open)
            {
                if (levelDb >= (double) s.thresholdDb - (double) s.hysteresisDb)
                {
                    holdLeft = holdSamples;
                }
                else if (holdLeft > 0)
                {
                    --holdLeft;
                }
                else
                {
                    open = false;
                    ++closes;
                    sinceClose = 0;
                }
            }

            return open;
        }
    };

    /** The gain (linear), moved towards its target with the attack time while it rises and the
        release time while it falls. Starts closed, at the range's gain. */
    struct Smoother
    {
        double attack = 0.0, release = 0.0, state = 1.0;

        void setTimes (double attackMs, double releaseMs, double sampleRate)
        {
            attack = comp::getCoefficient (attackMs, sampleRate);
            release = comp::getCoefficient (releaseMs, sampleRate);
        }

        double process (double target)
        {
            const double c = target > state ? attack : release;
            state = target + c * (state - target);
            return state;
        }
    };

    /** What the gate did to a level curve. */
    struct Run
    {
        std::vector<float> gainDb;   // per sample, 0 or less
        std::vector<char> open;      // per sample, 1 while open
        int closes = 0, flickers = 0;
    };

    /** The same detector, open state and smoother the processor runs, on a curve of key levels in dB
        (one per sample at `rate`). The pictures use it, so they show what the gate really does. */
    Run simulate (const Settings&, const std::vector<float>& levelsDb, double rate);

    /** Example levels, in dB at exampleRate (values in the plan's Theory): drum hits over hiss at -55 dB. */
    std::vector<float> makeDrumHits (double seconds);

    /** A note fading from -20 to -60 dB over two seconds, with a slight wobble. */
    std::vector<float> makeFadingNote();

    /** 30 ms: -60 dB, then at 10 ms a hit at -6 dB. */
    std::vector<float> makeHitStart();

    /** A tom hit dying away over bleed at -50 dB, 1.5 s. */
    std::vector<float> makeTomHit();

    /** A snare hit with a long reverb tail over faint hiss, 1.5 s. */
    std::vector<float> makeReverbSnare();

    /** "-40.0 dB", "Gate" / "2.0:1", "-80 dB", "0.50 ms", "1.50 s", "150 Hz" / "Off": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point for a sound, with why each knob is set the way it is. `followTrack`: the
        threshold is set from what the track plays, when there is a suggestion. */
    struct Preset
    {
        juce::String name, why;
        bool followTrack = false;
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob
    };

    /** The neutral start first, then the sounds. */
    const std::vector<Preset>& getPresets();

    /** One moment (2 ms) of what the gate heard and did, for the graphs and for the advice. */
    struct Moment
    {
        float inputDb = -100.0f, outputDb = -100.0f, detectorDb = -100.0f, reductionDb = 0.0f;
        bool open = false;
        int flickers = 0;
    };

    /** What the gate makes of the last 4 seconds of a track, and what to try. */
    struct Analysis
    {
        int heard = 0;                          // moments with something above -90 dB (digital silence is ignored by the rest)
        bool silentGaps = false;                // the quietest audible parts are under -80 dB
        float floorDb = -100.0f, loudDb = -100.0f;
        int flickers = 0;
        std::optional<float> suggestedDb;       // a threshold about 6 dB above the noise floor
        juce::StringArray advice;
    };

    /** The rules of the plan's Theory (A0 to A7). `matchGainDb` is what match level would add. */
    Analysis analyse (const std::vector<Moment>&, const Settings&, float matchGainDb);
} // namespace gate

//==============================================================================
/**
    DawIt Gate: a noise gate and downward expander. It opens for the notes and closes in the gaps,
    to remove hiss, hum and the bleed of other instruments, or to tighten drums and make gated
    reverb. A threshold with hysteresis, a range, a ratio (its far end is a gate), attack, hold and
    release, and a key (what it listens to) with a low and a high cut. Its stereo channels share one
    key and one gain, so the image doesn't shift. It has no lookahead, so no delay. It runs in the
    track's chain like DawIt EQ (see BuiltInFormat.h).

    For learning it also offers: key listen, hearing only what it removes, a test sound (hits over
    hiss), match level, and the last 4 seconds as moments (getRecent).
*/
class BuiltInGate final : public juce::AudioPluginInstance
{
public:
    BuiltInGate();

    static constexpr const char* identifier = "DawIt Gate";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInGate&)> createEditorFunction;

    static const char* getParameterId (gate::Knob);
    juce::RangedAudioParameter* getKnobParameter (gate::Knob) const;

    gate::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const gate::Settings&);
    void setKnob (gate::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Sets the knobs from a starting point; with `trackThreshold` and a preset that follows the
        track, the threshold is that one. Switches match level off. */
    void applyPreset (const gate::Preset&, std::optional<float> trackThreshold = std::nullopt);

    bool isMatchingLevel() const { return matchLevel->load() >= 0.5f; }
    void setMatchingLevel (bool);

    /** Play only what the gate takes away (input minus output). Not saved. */
    std::atomic<bool> listenToRemoved { false };

    /** Play what the gate listens to (the key, through its filters) instead of the track. Not saved. */
    std::atomic<bool> listenToKey { false };

    /** The gain that keeps the output as loud as the input right now, in dB; match level applies it. */
    std::atomic<float> matchGainDb { 0.0f };

    /** The host's tempo, 0 while unknown. */
    std::atomic<double> tempoBpm { 0.0 };

    /** How many flickers the moments written so far hold. */
    std::atomic<int> flickerTotal { 0 };

    fx::Clap clap;

    /** The last 4 seconds, one moment per 2 ms, oldest first (fewer while it hasn't played that long).
        Any thread: a moment being written may be read torn, which is harmless. */
    std::vector<gate::Moment> getRecent() const;

    /** The loudest key level the gate heard since the last call (dB, -100 when nothing played), how
        much it turned the sound down, and whether it was open. For the device bar's small curve. */
    struct Preview { float detectorDb = -100.0f, reductionDb = 0.0f; bool open = false; };
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

    struct Slot
    {
        std::atomic<float> input { -100.0f }, output { -100.0f }, detector { -100.0f }, reduction { 0.0f };
        std::atomic<int> openAndFlickers { 0 };   // flickers x 2, plus 1 while open
    };

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, gate::numKnobs> knobValues {};
    std::atomic<float>* matchLevel = nullptr;

    double rate = 44100.0;
    gate::Detector detector;
    gate::OpenState openState;
    gate::Smoother smoother;
    float lowShown = -1.0f, highShown = -1.0f;
    eq::Stages lowStages, highStages;
    double lowMemory[2][2][2] {}, highMemory[2][2][2] {};
    double powerIn = 0.0, powerOut = 0.0;
    juce::SmoothedValue<float> matchGain;

    int momentSamples = 88, momentCount = 0, lastFlickers = 0;
    float momentIn = 0.0f, momentOut = 0.0f;
    double momentDetector = 0.0, momentReduction = 0.0;

    std::array<Slot, gate::numMoments> slots;
    std::atomic<long long> written { 0 };

    std::atomic<float> previewDetectorDb { -100.0f }, previewReductionDb { 0.0f };
    std::atomic<bool> previewOpen { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInGate)
};
} // namespace daw
