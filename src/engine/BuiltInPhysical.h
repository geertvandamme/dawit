#pragma once

#include "BuiltInSubtractive.h"

namespace daw
{
namespace phys
{
    /** The knobs and switches, in the order of the window and of a preset's notes. */
    enum class Knob
    {
        exciter, hardness, position, pressure, noise,
        resonator, decay, damping, release,
        attack, vibratoRate, vibratoDepth,
        glide, mode, velocity, output
    };

    constexpr int numKnobs = 16;

    /** How the note is started. Pluck: a burst of noise, like a pick or a finger (Karplus and Strong's trick).
        Strike: a short smooth push, like a hammer or a mallet. Bow: a steady drag that sticks and slips.
        Blow: breath through a reed, which opens and closes like a valve. */
    enum class Exciter { pluck, strike, bow, blow };
    constexpr int numExciters = 4;

    /** What rings. String: fixed at both ends, with every harmonic. Tube: closed at one end and open at
        the other, like a clarinet, with only the odd harmonics, an octave lower than a string as long. */
    enum class Resonator { string, tube };
    constexpr int numResonators = 2;

    juce::String getExciterName (Exciter);
    juce::String getResonatorName (Resonator);

    struct Settings
    {
        Exciter exciter = Exciter::pluck;
        float hardnessPercent = 70.0f;   // pluck: a soft finger to a hard pick; strike: a felt mallet to a hard one
        float positionPercent = 20.0f;   // where along the string or tube it's played, from the end
        float pressurePercent = 50.0f;   // bow: how hard it presses; blow: how hard the player blows
        float noisePercent = 0.0f;       // the bow's scratch or the breath's hiss

        Resonator resonator = Resonator::string;
        float decaySeconds = 3.0f;       // how long the fundamental rings, to -60 dB, while the key is held
        float dampingPercent = 40.0f;    // how much sooner the high partials die
        float releaseSeconds = 0.3f;     // after the key is let go, it rings at most this long: a finger stopping the string

        float attackSeconds = 0.08f;     // bow and blow: how long the bow or breath takes to get going
        float vibratoRateHz = 5.5f;
        float vibratoDepthPercent = 0.0f;

        float glideSeconds = 0.0f;
        sub::Mode mode = sub::Mode::poly;
        float velocityPercent = 60.0f;   // how much softer notes are quieter and softer-sounding
        float outputDb = 0.0f;
    };

    constexpr float minPosition = 2.0f, maxPosition = 50.0f, minDecay = 0.05f, maxDecay = 20.0f, minRelease = 0.01f, maxRelease = 10.0f,
                    minAttack = 0.001f, maxAttack = 2.0f, minVibratoRate = 0.5f, maxVibratoRate = 10.0f, maxGlide = 2.0f,
                    minOutput = -24.0f, maxOutput = 6.0f;

    /** At 100% the vibrato moves the pitch a semitone up and down; a violinist's is about 20 cents (20%). */
    constexpr float vibratoSemitones = 1.0f;

    /** Notes ring down to -60 dB: the usual measure of how long a sound lasts (T60). */
    constexpr double ringDb = 60.0;

    /** How loud a full-velocity note peaks before the output knob, in Init: about -12 dB, like the other synths. */
    constexpr float peakDb = -12.0f;

    juce::String getKnobName (Knob);

    /** Whether a knob changes the sound with this exciter: hardness is for plucks and strikes, pressure, noise and
        attack for the bow and breath; a reed always plays a tube, so the resonator and position are a bow's,
        pluck's or strike's. */
    bool isUsed (const Settings&, Knob);

    /** The resonator that plays: a tube for a reed, otherwise the switch's. */
    Resonator getResonator (const Settings&);

    //==============================================================================
    /** The loop the note runs around: a wave travelling from where it's played to one end and back (side A),
        then to the other end and back (side B), losing some of itself each trip. A string's loop is one period;
        a tube's is half, as its closed end turns the wave over, so it takes two trips to come back the same. */
    struct Loop
    {
        double length = 0.0;          // samples for one trip around
        double sideA = 0.0, sideB = 0.0;
        double pole = 0.0;            // the low-pass in the loop that damps the highs (0: none)
        double gain = 1.0;            // what's left after a trip, besides the damping
        double tripsPerSecond = 0.0;
    };

    /** The loop for a note at `noteHz`, while held or after the key is let go. */
    Loop getLoop (const Settings&, double noteHz, double sampleRate, bool released = false);

    /** How fast the damping alone makes a partial at `hz` fade, in dB a second: it grows with the square of the
        frequency, as in real strings, from nothing at 0% to 600 dB a second at 1 kHz at 100%. */
    double getDampingDbPerSecond (float dampingPercent, double hz);

    /** Whether harmonic `n` (1 is the fundamental) is in the resonator at all: a tube has only the odd ones. */
    bool hasHarmonic (const Settings&, int n);

    /** How long harmonic `n` of a note rings, to -60 dB, held or after the key is let go. */
    double getRingSeconds (const Settings&, double noteHz, int n, bool released = false, double sampleRate = 48000.0);

    /** How strongly playing at the position excites harmonic `n`, 0 to 1: on a string |sin (pi n p)|, so a pluck at
        1/5 of the length leaves out the 5th, 10th...; in a tube |cos (pi n p / 2)|. 1 for a reed. */
    double getPositionLevel (const Settings&, int n);

    /** A pluck: where the burst of noise is filtered, from 200 Hz (a soft finger) to 20 kHz (a hard pick). */
    double getPickHz (float hardnessPercent);

    /** A strike: how long the hammer touches the string, from 5 ms (soft felt) to 0.15 ms (hard). */
    double getContactSeconds (float hardnessPercent);

    /** The bow: how far the string slips before the hair lets go; lower pressure is a steeper slope (STK's bow table). */
    double getBowSlope (float pressurePercent);

    /** Blow: the breath pressure, as a share of what shuts the reed; a tube starts to sound at about 0.55. */
    double getBreath (float pressurePercent, float velocityScale);

    /** How much a note at this velocity (0 to 1) keeps of its level. */
    float getVelocityScale (const Settings&, float velocity);

    //==============================================================================
    /** One note: the string or tube as two delay lines that meet where it's played, with the exciter between
        them. The same sound for the voices, the pictures and the tests. */
    class Voice
    {
    public:
        /** Makes room for notes down to 20 Hz. */
        void prepare (double sampleRate);

        /** Starts a note. A pluck or a strike adds its burst to whatever already rings when `keepRinging` (the same
            string played again); otherwise the lines start silent. A bow or breath starts pushing. */
        void start (const Settings&, double noteHz, float velocity, juce::uint32 seed, bool keepRinging);

        /** Moves the pitch (glide, vibrato, legato) without starting the note again: a hammer-on. */
        void setPitch (double noteHz) noexcept { hz = noteHz; }

        /** Takes the settings for the next samples: the loop's lengths and losses. Called every few samples. */
        void update (const Settings&);

        /** The key is let go: the loop rings out in the release time, and a bow or breath stops. */
        void release() noexcept { held = false; }

        /** The next sample, before the output gain; the exciter alone when asked, and the string's speed at the
            exciter and the bow's (or the breath's) for the pictures. */
        float next (float* exciterPart = nullptr, float* pointSpeed = nullptr, float* pushSpeed = nullptr) noexcept;

        /** Hear only what the exciter puts in, without the resonator. */
        bool exciterOnly = false;

        bool isActive() const noexcept { return active; }
        bool isHeld() const noexcept { return held; }
        double getAge() const noexcept { return age; }
        double getPitch() const noexcept { return hz; }

        /** How loud the note is now, from a follower on its output. */
        float getLevel() const noexcept { return (float) follower; }

    private:
        struct Line
        {
            std::vector<float> data;
            int write = 0, mask = 0;

            void clear() noexcept;
            void push (double x) noexcept;
            double read (double delay) const noexcept;
        };

        double nextNoise() noexcept;
        double excite() noexcept;
        void makeBurst (int length);

        Settings settings;
        Loop loop;
        Line a, b;
        std::vector<float> burstData;
        double rate = 44100.0, hz = 220.0, age = 0.0, push = 0.0, low = 0.0, pickLow = 0.0, pickPole = 0.0,
               contact = 0.0, dcIn = 0.0, dcOut = 0.0, dcPole = 0.999, follower = 0.0, followerFall = 0.999, gain = 1.0;
        float velocity = 1.0f, scale = 1.0f;
        int sample = 0, burst = 0;
        juce::uint32 noiseState = 1;
        bool held = false, active = false;
    };

    /** A note sample by sample: its sound before the output gain, what the exciter put in, and the string's speed at
        the exciter and the bow's (or the breath), for the pictures. */
    struct Parts
    {
        std::vector<float> sound, exciter, point, push;
    };

    /** A note held for `holdSeconds` (all of it when negative), then let go. */
    Parts renderParts (const Settings&, double noteHz, double sampleRate, double seconds, double holdSeconds = -1.0, float velocity = 1.0f,
                       bool exciterOnly = false);

    /** Just the sound of renderParts. */
    std::vector<float> render (const Settings&, double noteHz, double sampleRate, double seconds, double holdSeconds = -1.0, float velocity = 1.0f,
                               bool exciterOnly = false);

    /** The rate getHarmonics renders a note at, so its harmonics fall exactly on the analysis's bins: 128 times the note. */
    double getAnalysisRate (double noteHz);

    /** The level of each harmonic (1 up to `count`, at most 63) of a note rendered at getAnalysisRate, `atSeconds`
        after it starts, as the amplitude of a sine. */
    std::vector<double> measureHarmonics (const std::vector<float>& sound, double noteHz, double atSeconds, int count);

    /** The level of each harmonic of a note `atSeconds` after it starts, measured from the sound itself. Held for
        `holdSeconds`, then let go. */
    std::vector<double> getHarmonics (const Settings&, double noteHz, double atSeconds, int count, double holdSeconds = -1.0, float velocity = 1.0f);

    /** Whether two settings sound the same, for the pictures to know when to measure again. */
    bool isSame (const Settings&, const Settings&);

    //==============================================================================
    /** "Pluck", "70%", "1.20 s", "5.5 Hz", "Off": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point for a sound, with why it's made the way it is and notes on the knobs that matter. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob; empty when it stays as in Init
    };

    /** Init (a plucked string, Karplus-Strong's) first, then the sounds. */
    const std::vector<Preset>& getPresets();
} // namespace phys

//==============================================================================
/**
    DawIt Physical: a physical-modelling synthesiser that explains itself. Instead of building a sound from
    waves, it models the instrument: a string or a tube as a delay line that a wave runs around, losing a
    little of its highs each trip, and an exciter that sets it going: a pluck (a burst of noise, as Karplus
    and Strong did in 1983), a strike, a bow that sticks and slips, or breath through a reed. The length of
    the loop is the pitch; where it's played picks the harmonics; the losses are the decay and the colour.
    It's an instrument in the track's chain (see BuiltInFormat.h), played by the track's MIDI notes.

    For learning it also offers: a test note to play from the window, the exciter without the resonator, a
    history of how loud the exciter and the whole sound were, and the newest note's age and level.
*/
class BuiltInPhysical final : public juce::AudioPluginInstance
{
public:
    BuiltInPhysical();

    static constexpr const char* identifier = "DawIt Physical";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInPhysical&)> createEditorFunction;

    static const char* getParameterId (phys::Knob);
    juce::RangedAudioParameter* getKnobParameter (phys::Knob) const;

    phys::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const phys::Settings&);
    void setKnob (phys::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Hear only what the exciter puts in, without the string or tube. Not saved. */
    std::atomic<bool> exciterOnly { false };

    /** "Play a note" in the window plays the test note, A3, for 1.5 seconds. */
    fx::Clap clap;

    /** How loud the exciter was (as the input) and the whole sound (as the effect). */
    fx::LevelHistory history;

    /** The newest note, for the pictures: how long ago it started (-1 when none plays), how long after its start
        it was let go (-1 while held), its level and its velocity. */
    std::atomic<float> noteAge { -1.0f }, releaseAge { -1.0f }, shownLevel { 0.0f }, shownVelocity { 1.0f };

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

    /** The test note lasts this long before it's let go. */
    static constexpr double testNoteSeconds = 1.5;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Slot
    {
        phys::Voice voice;
        int note = -1;
        bool held = false;
        juce::uint64 startedAt = 0;              // which note-on it was, for stealing the oldest
        double pitch = 0.0, targetPitch = 0.0;   // in MIDI notes; they differ while gliding
        double releasedAt = -1.0;                // seconds after its start
        float velocity = 1.0f;
    };

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void startSlot (Slot&, int note, float velocity, bool retrigger);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, phys::numKnobs> knobValues {};

    double rate = 44100.0;
    std::array<Slot, numVoices> slots;
    std::vector<int> heldNotes;   // in the order they were pressed, for mono
    juce::uint64 noteCounter = 0;
    juce::uint32 seed = 1;
    double lastPitch = -1.0;      // where the next note glides from
    double vibratoPhase = 0.0;
    juce::SmoothedValue<float> outputGain;
    int testNoteLeft = -1;        // samples until the test note is let go
    float chunkBefore = 0.0f, chunkAfter = 0.0f;
    int chunkFill = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInPhysical)
};
} // namespace daw
