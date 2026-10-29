#pragma once

#include "BuiltInSubtractive.h"

namespace daw
{
namespace additive
{
    /** How many harmonics there are to draw: at A3 (220 Hz) they reach 7 kHz, at C5 (523 Hz) past 16 kHz. */
    constexpr int numPartials = 32;

    /** The knobs and switches, in the order of the window and of a preset's notes. The harmonics' own
        levels are parameters too (getPartialId), drawn as bars rather than knobs. */
    enum class Knob
    {
        brightness, even, stretch, phases,
        swell, fade,
        ampAttack, ampDecay, ampSustain, ampRelease,
        lfoRate, lfoDepth, lfoTarget,
        glide, mode, velocity, output
    };

    constexpr int numKnobs = 17;

    /** Aligned: every harmonic starts at the same moment, so a saw's partials add up to the saw's shape.
        Spread: each starts at the point that keeps the wave's peaks lowest (Schroeder's phases, 1970, when
        many harmonics are about as loud), which sounds nearly the same. */
    enum class Phases { aligned, spread };

    enum class LfoTarget { pitch, brightness, even };
    constexpr int numLfoTargets = 3;

    /** The shapes the harmonics can be filled with from the picture's menu. */
    enum class Shape { sine, saw, square, triangle, organ };
    constexpr int numShapes = 5;

    juce::String getShapeName (Shape);
    juce::String getLfoTargetName (LfoTarget);

    /** A harmonic's level is in dB, from 0 (full) down to offDb, where it's off. */
    constexpr float offDb = -60.0f;

    struct Settings
    {
        std::array<float, numPartials> levelsDb;   // harmonic 1 (the fundamental) first
        float brightnessDb = 0.0f;                 // dB per octave on top of the bars: below 0 the highs are quieter
        float evenPercent = 100.0f;                // how much of the even harmonics (2nd, 4th...): 0 leaves only the odd ones
        float stretchPercent = 0.0f;               // how far the harmonics are pulled sharp, as in a stiff string
        Phases phases = Phases::aligned;

        float swellSeconds = 0.0f;                 // the highest harmonic comes in over this time, the lower ones sooner
        float fadeSeconds = 30.0f;                 // the 2nd harmonic halves in this time, harmonic n n-1 times as fast; maxFade is off

        sub::Envelope ampEnv;

        float lfoRateHz = 5.0f;
        float lfoDepthPercent = 0.0f;
        LfoTarget lfoTarget = LfoTarget::pitch;

        float glideSeconds = 0.0f;
        sub::Mode mode = sub::Mode::poly;
        float velocityPercent = 50.0f;             // how much softer notes are quieter and duller
        float outputDb = 0.0f;

        Settings();
    };

    constexpr float maxSwell = 4.0f, minFade = 0.02f, maxFade = 30.0f, minBrightness = -12.0f, maxBrightness = 6.0f,
                    minOutput = -24.0f, maxOutput = 6.0f, maxGlide = 2.0f, minTime = 0.001f, maxTime = 10.0f,
                    minLfoRate = 0.05f, maxLfoRate = 20.0f;

    /** At 100% depth, the LFO moves the pitch by up to 2 semitones, the brightness by 6 dB per octave,
        and the even harmonics all the way out and back. */
    constexpr float lfoPitchSemitones = 2.0f, lfoBrightnessDb = 6.0f;

    /** How loud one voice is at full velocity before the output knob: a sine at 0 dB peaks at about -12 dB. */
    constexpr float voiceGain = 0.25f, sinePeakDb = -12.0f;

    /** At 100% stretch, the stiffness of the string: B in f_n = n f sqrt ((1 + B n^2) / (1 + B)). */
    constexpr double maxStiffness = 0.004;

    bool isFadeOff (float fadeSeconds);

    //==============================================================================
    /** The levels (in dB) that make a shape: a sine is the 1st harmonic alone, a saw every harmonic at
        1/n (-6 dB an octave), a square the odd ones at 1/n, a triangle the odd ones at 1/n squared, and
        the organ the four lowest drawbars of a Hammond pulled out (8', 4', 2 2/3', 2'). */
    std::array<float, numPartials> getShapeLevels (Shape);

    /** How far up harmonic `n` is, against the fundamental: n for a perfect string, more with stretch
        (n sqrt ((1 + B n^2) / (1 + B)), the stiffness of a piano string), so the fundamental stays put. */
    double getRatio (int n, float stretchPercent);

    /** How many cents sharp of its perfect place harmonic `n` is with this stretch. */
    double getStretchCents (int n, float stretchPercent);

    /** Where each harmonic starts its cycle: 0 when aligned; when spread, the phases that make these
        harmonics peak lowest out of Schroeder's (from the levels, and for equal levels), a few fixed
        random sets, and lined up, which is already lowest for a saw or a square. */
    std::array<double, numPartials> getStartPhases (const Settings&);

    /** How loud harmonic `n` is (as a linear amplitude, 1 for 0 dB) from its bar, the brightness, the
        even harmonics, a note's velocity (softer is duller) and the LFO's value (-1 to 1), before the
        note's own movement. */
    double getPartialGain (const Settings&, int n, float velocity = 1.0f, float lfo = 0.0f);

    /** The turn-down that keeps the sum's power no more than one full sine's: 1 / sqrt (the sum of the
        gains squared), when that's more than 1. Adding harmonics makes the sound brighter, not louder. */
    double getNormalisation (const Settings&);

    /** How much of harmonic `n` a note has `seconds` after its start: it comes in over its share of the
        swell (n - 1) / 31 of it), and fades, halving every fade / (n - 1) seconds. 0 to 1. */
    double getPartialEnvelope (const Settings&, int n, double seconds);

    /** How much a note at this velocity (0 to 1) keeps of its level. */
    float getVelocityScale (const Settings&, float velocity);

    //==============================================================================
    /** A sine the sound is made of: which harmonic, its frequency, its amplitude (a sine at 0 dB peaks at
        1, after the normalisation) and the phase it starts at. */
    struct Partial
    {
        int n = 1;
        double hz = 0.0, amplitude = 0.0, phase = 0.0;
    };

    /** The partials of a note at `noteHz`, `seconds` after its start (or with no movement when negative),
        up to `maxHz`. */
    std::vector<Partial> getPartials (const Settings&, double noteHz, double seconds = -1.0, float velocity = 1.0f, double maxHz = 20000.0);

    /** A stretch of the sound as samples: the partials added up. */
    std::vector<float> getWave (const std::vector<Partial>&, double seconds, int points);

    //==============================================================================
    /** "-6.0 dB/oct", "35%", "Spread", "350 ms", "Off": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A harmonic's level as its bar shows it: "-14.0 dB" or "Off". */
    juce::String formatLevel (float db);

    /** The knob's name in the window. */
    juce::String getKnobName (Knob);

    /** "1st", "2nd", "3rd", "4th"... */
    juce::String getOrdinal (int n);

    /** A starting point for a sound, with why it's made the way it is and notes on what matters. */
    struct Preset
    {
        juce::String name, why, partials;           // partials: why the harmonics are drawn the way they are
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob; empty when it stays as in Init
    };

    /** Init (one sine, nothing moving) first, then the sounds. */
    const std::vector<Preset>& getPresets();
} // namespace additive

//==============================================================================
/**
    DawIt Additive: an additive synthesiser that explains itself. Each note is built from 32 sines at
    the harmonics of its pitch, each at its own level, drawn as bars; a brightness tilt, the even
    harmonics' level and a stretch (for stiff strings and bells) change them all at once. The higher
    harmonics can swell in after a note starts and fade out sooner, as they do in real instruments; an
    amp envelope shapes each note; an LFO moves the pitch, the brightness or the even harmonics. Poly
    or mono with glide. It's an instrument in the track's chain (see BuiltInFormat.h), played by the
    track's MIDI notes.

    For learning it also offers: a test note to play from the window, the fundamental alone, a
    history of how loud the fundamental and the whole sound were, and the newest note's harmonics for
    the pictures.
*/
class BuiltInAdditive final : public juce::AudioPluginInstance
{
public:
    BuiltInAdditive();

    static constexpr const char* identifier = "DawIt Additive";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInAdditive&)> createEditorFunction;

    static const char* getParameterId (additive::Knob);
    static juce::String getPartialId (int n);
    juce::RangedAudioParameter* getKnobParameter (additive::Knob) const;
    juce::RangedAudioParameter* getPartialParameter (int n) const;

    additive::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const additive::Settings&);
    void setKnob (additive::Knob, float value);
    void setPartialDb (int n, float db);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Hear only the 1st harmonic, at the level it has in the sound. Not saved. */
    std::atomic<bool> onlyFundamental { false };

    /** "Play a note" in the window plays the test note, A3, for 0.8 seconds. */
    fx::Clap clap;

    /** The level of the 1st harmonic alone (as the input) and of the whole sound (as the effect). */
    fx::LevelHistory history;

    /** The newest note, for the pictures: how much of each harmonic it has now (its swell and fade),
        its amp envelope's level, how long ago it started (-1 when no note plays), how long after its
        start it was let go (-1 while held), its velocity, and the LFO's value. */
    std::array<std::atomic<float>, additive::numPartials> shownPartials;
    std::atomic<float> ampLevel { 0.0f }, noteAge { -1.0f }, releaseAge { -1.0f }, shownVelocity { 1.0f }, lfoValue { 0.0f };

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override { return getSettings().ampEnv.release; }
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
        std::array<double, additive::numPartials> phases {}, gains {};
        juce::ADSR amp;
        double age = 0.0, releasedAt = -1.0;     // seconds
        float lastAmp = 0.0f;

        bool isActive() const noexcept { return amp.isActive(); }
    };

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void startVoice (Voice&, int note, float velocity, bool retrigger);
    void renderVoice (Voice&, const additive::Settings&, double norm, float lfo, float* out, float* fundamental, int numSamples);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, additive::numKnobs> knobValues {};
    std::array<std::atomic<float>*, additive::numPartials> partialValues {};

    double rate = 44100.0;
    std::array<Voice, numVoices> voices;
    std::vector<int> heldNotes;   // in the order they were pressed, for mono
    juce::uint64 noteCounter = 0;
    double lastPitch = -1.0;      // where the next note glides from
    double lfoPhase = 0.0;
    juce::SmoothedValue<float> outputGain;
    int testNoteLeft = -1;        // samples until the test note is let go
    float chunkBefore = 0.0f, chunkAfter = 0.0f;
    int chunkFill = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInAdditive)
};
} // namespace daw
