#pragma once

#include "BuiltInSubtractive.h"

#include <optional>

namespace daw
{
namespace drums
{
    /** The drums of the kit, each played by its General MIDI note (see getNote). */
    enum class Drum { kick, snare, clap, lowTom, highTom, closedHat, openHat, cymbal, cowbell };
    constexpr int numDrums = 9;

    /** The knobs and switches, in the order of the window. The first nine belong to a drum: the window
        shows them for the one selected, and each drum has its own parameters. */
    enum class Knob
    {
        tune, bend, bendTime, decay,
        noise, noiseDecay, tone,
        click, level,
        choke, velocity, output
    };

    constexpr int numKnobs = 12, numDrumKnobs = 9;

    bool isDrumKnob (Knob);

    /** Where a drum's tone comes from. Sine: one sine, for the kick and the toms. Head: a sine and the
        drumhead's second mode at 1.59 times it, for the snare. Metal: six square waves at the TR-808's
        clashing frequencies, for the hi-hats and the cymbal. Bell: two of those square waves (540 and
        800 Hz), the 808's cowbell. None: the clap, which is all noise. */
    enum class Body { sine, head, metal, bell, none };

    /** What the noise (and the metal) goes through: low-pass keeps the lows, band-pass a band, high-pass the highs. */
    enum class Filter { lowPass, bandPass, highPass };

    /** One drum's settings. */
    struct Sound
    {
        float tuneHz = 50.0f;            // the tone's pitch once the bend is over; for metal and bell, the lowest square
        float bendSemitones = 0.0f;      // how far above the tune a hit starts
        float bendMs = 30.0f;            // how fast the pitch falls back: about two thirds of the way in this time
        float decaySeconds = 0.5f;       // the tone fades to -60 dB in this time
        float noisePercent = 0.0f;       // how much of the hit is noise rather than tone
        float noiseDecaySeconds = 0.1f;  // the noise fades to -60 dB in this time
        float toneHz = 2000.0f;          // where the noise filter sits
        float clickPercent = 0.0f;       // a burst of noise under 2 ms long at the start
        float levelDb = 0.0f;
    };

    struct Settings
    {
        std::array<Sound, numDrums> drums;   // by Drum
        bool choke = true;                   // a closed hi-hat cuts off an open one, as on a real hi-hat stand
        float velocityPercent = 70.0f;       // how much softer hits are quieter
        float outputDb = 0.0f;

        Settings();
    };

    constexpr float minTune = 20.0f, maxTune = 2000.0f, maxBend = 48.0f, minBendMs = 1.0f, maxBendMs = 500.0f, minDecay = 0.01f,
                    maxDecay = 5.0f, minTone = 100.0f, maxTone = 16000.0f, minLevel = -24.0f, maxLevel = 6.0f, minOutput = -24.0f,
                    maxOutput = 6.0f;

    /** How loud a hit is before its level: a kick's sine peaks at 0.4, -8 dB, leaving room for the drums that hit with it. */
    constexpr float voiceGain = 0.4f;

    /** The clap: three bursts this far apart, then the tail. */
    constexpr double clapBurstSeconds = 0.011;
    constexpr int clapBursts = 3;

    juce::String getDrumName (Drum);
    Body getBody (Drum);
    Filter getFilter (Drum);
    float getFilterQ (Drum);

    /** The General MIDI note that plays a drum: kick 36 (C2, with middle C as C4), snare 38, clap 39, low tom 45, high tom
        50, closed hi-hat 42, open hi-hat 46, cymbal 49, cowbell 56. */
    int getNote (Drum);

    /** The drum a note plays: its own note, and the other General MIDI notes of the same kind (35 for a
        kick, 40 for a snare, 41 and 43 for toms, 44 for a pedal hi-hat, 51 and 57 for cymbals); none for the rest. */
    std::optional<Drum> getDrumForNote (int note);

    /** Whether a knob changes this drum: a clap has no tone to tune, metal doesn't bend. */
    bool isUsed (Drum, Knob);

    /** The six square waves of the TR-808's hi-hats and cymbal, as ratios to the lowest: 205.3, 304.4,
        369.6, 522.7, 540 and 800 Hz. They're far from any harmonic series, so they clash like metal. */
    const std::array<double, 6>& getMetalRatios();

    /** The cowbell's two square waves, 540 and 800 Hz, as ratios to the lower. */
    constexpr double bellRatio = 800.0 / 540.0;

    /** The tone's pitch `seconds` into a hit: tune x 2^(bend / 12 x e^(-t / bend time)). */
    double getPitchAt (const Sound&, double seconds);

    /** How much of a part (tone or noise) is left `seconds` into a hit that fades to -60 dB in `decaySeconds`. */
    double getEnvelope (double decaySeconds, double seconds);

    /** The clap's noise `seconds` into a hit: three quick bursts, then a tail fading in the noise decay. */
    double getClapEnvelope (const Sound&, double seconds);

    /** How long a hit lasts: until both its tone and its noise are under -60 dB. */
    double getLength (const Sound&, Drum);

    /** How much a hit at this velocity (0 to 1) keeps of its level. */
    float getVelocityScale (const Settings&, float velocity);

    //==============================================================================
    /** One hit of one drum, sample by sample: the same sound for the voices, the pictures and the tests. */
    class Voice
    {
    public:
        /** Starts a hit at `gain` (1 for the drum's level at full velocity); `seed` picks its noise. */
        void start (const Sound&, Drum, float gain, double sampleRate, juce::uint32 seed = 1);

        /** The next sample, with the tone and the noise (with the click) apart when asked for. */
        float next (float* tonePart = nullptr, float* noisePart = nullptr) noexcept;

        /** Fades the hit out in 5 ms, as a closed hi-hat cuts off an open one. */
        void choke() noexcept;

        bool isActive() const noexcept { return active; }
        Drum getDrum() const noexcept { return drum; }
        double getAge() const noexcept { return age; }

        /** How loud the hit is now, from its envelopes: 1 at the start. */
        float getLevel() const noexcept;

    private:
        struct Svf
        {
            double ic1 = 0.0, ic2 = 0.0;
            double run (double x, double cutoff, double damping, Filter) noexcept;
        };

        double nextNoise() noexcept;

        Sound sound;
        Drum drum = Drum::kick;
        Body body = Body::sine;
        Filter filter = Filter::lowPass;
        double rate = 44100.0, age = 0.0, length = 0.0, g = 0.0, k = 1.0, noiseMakeUp = 1.0;
        double phase = 0.0, phase2 = 0.0;
        std::array<double, 6> metalPhases {};
        Svf toneFilter, noiseFilter;
        juce::uint32 noiseState = 1;
        float gain = 1.0f, chokeGain = 1.0f, chokeStep = 0.0f;
        bool active = false;
    };

    /** A hit's tone and noise (with the click) apart, `seconds` long. */
    struct Hit
    {
        std::vector<float> tone, noise;
    };

    Hit render (const Settings&, Drum, double sampleRate, double seconds, float velocity = 1.0f);

    //==============================================================================
    /** "50 Hz", "+24 st", "30 ms", "1.20 s", "60%", "1.50 kHz", "On": as the knobs show them, for drum `drum`. */
    juce::String formatKnob (Knob, const Settings&, Drum = Drum::kick);

    /** The knob's name in the window. */
    juce::String getKnobName (Knob);

    /** A kit to start from, with why it sounds the way it does and what matters in each drum. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        std::array<juce::String, numDrums> drums;   // by Drum
        std::array<juce::String, numKnobs> notes;   // by Knob, for the kit's own knobs
    };

    /** Init (an 808-style kit) first, then the others. */
    const std::vector<Preset>& getPresets();
} // namespace drums

//==============================================================================
/**
    DawIt Drums: a drum synthesiser that explains itself. Nine drums, each made from scratch the way a
    Roland TR-808 makes them: a sine that drops in pitch for the kick and the toms, a tone and noise for
    the snare, bursts of noise for the clap, six clashing square waves for the hi-hats and the cymbal,
    two for the cowbell. Each drum has its tune, bend, decay, noise, tone, click and level; a closed
    hi-hat cuts off an open one. It's an instrument in the track's chain (see BuiltInFormat.h), played by
    the track's MIDI notes on the General MIDI drum map, so the drums of a MIDI file play the right drums.

    For learning it also offers: a hit of the selected drum to play from the window, that drum alone,
    a history of how loud it was against the whole kit, and how loud each drum is now for the pads.
*/
class BuiltInDrums final : public juce::AudioPluginInstance
{
public:
    BuiltInDrums();

    static constexpr const char* identifier = "DawIt Drums";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInDrums&)> createEditorFunction;

    /** The parameter of a knob, for drum `drum` when it's one of a drum's: "kickTune", "choke". */
    static juce::String getParameterId (drums::Knob, drums::Drum = drums::Drum::kick);
    juce::RangedAudioParameter* getKnobParameter (drums::Knob, drums::Drum = drums::Drum::kick) const;

    drums::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const drums::Settings&);
    void setKnob (drums::Knob, float value, drums::Drum = drums::Drum::kick);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** The drum the window shows: "Play a hit" plays it, and "Hear it alone" keeps only it. Not saved. */
    std::atomic<int> selectedDrum { 0 };

    /** Hear only the selected drum. Not saved. */
    std::atomic<bool> onlySelected { false };

    /** "Play a hit" in the window plays the selected drum. */
    fx::Clap clap;

    /** The level of the selected drum alone (as the input) and of the whole kit (as the effect). */
    fx::LevelHistory history;

    /** How loud each drum's newest hit is now (0 when silent), for the pads. */
    std::array<std::atomic<float>, drums::numDrums> shownLevels;

    /** How long ago the selected drum was last hit (-1 when it isn't sounding). */
    std::atomic<float> selectedAge { -1.0f };

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

    static constexpr int numVoices = 24;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void hit (drums::Drum, float velocity, const drums::Settings&);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::array<std::atomic<float>*, drums::numDrumKnobs>, drums::numDrums> drumValues {};
    std::array<std::atomic<float>*, drums::numKnobs> knobValues {};

    double rate = 44100.0;
    std::array<drums::Voice, numVoices> voices;
    std::array<juce::uint64, numVoices> startedAt {};
    juce::uint64 hitCounter = 0;
    juce::uint32 seed = 1;
    juce::SmoothedValue<float> outputGain;
    float chunkBefore = 0.0f, chunkAfter = 0.0f;
    int chunkFill = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInDrums)
};
} // namespace daw
