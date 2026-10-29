#pragma once

#include "EffectTools.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <complex>

namespace daw
{
namespace sub
{
    /** The knobs and switches, in the order of the window and of a preset's notes. */
    enum class Knob
    {
        wave1, width, wave2, pitch2, detune, mix, noise,
        filterType, slope, cutoff, resonance, keyTrack,
        envAmount, filterAttack, filterDecay, filterSustain, filterRelease,
        ampAttack, ampDecay, ampSustain, ampRelease,
        lfoRate, lfoDepth, lfoTarget,
        glide, mode, velocity, output
    };

    constexpr int numKnobs = 28;

    /** Saw: every harmonic, falling as 1/n: bright and buzzy. Square: the odd harmonics only, hollow
        (a pulse when the width is under 50%, thinner and more nasal). Triangle: odd harmonics falling
        fast, soft. Sine: the fundamental alone, pure. */
    enum class Wave { saw, square, triangle, sine };
    constexpr int numWaves = 4;

    enum class FilterType { lowPass, bandPass, highPass };
    constexpr int numFilterTypes = 3;

    enum class LfoTarget { pitch, filter, volume, width };
    constexpr int numLfoTargets = 4;

    /** Poly: a voice per note, for chords. Mono: one note at a time; a note played while another is
        held glides to it without starting the envelopes again (legato), as on a Minimoog. */
    enum class Mode { poly, mono };

    juce::String getWaveName (Wave);
    juce::String getFilterTypeName (FilterType);
    juce::String getLfoTargetName (LfoTarget);

    /** An ADSR envelope: attack, decay and release in seconds, sustain from 0 to 1. */
    struct Envelope
    {
        float attack = 0.005f, decay = 0.3f, sustain = 1.0f, release = 0.2f;
    };

    struct Settings
    {
        Wave wave1 = Wave::saw;
        float widthPercent = 50.0f;      // the square's pulse width: 50% is a square, smaller is a narrower pulse
        Wave wave2 = Wave::saw;
        float pitch2 = 0.0f;             // oscillator 2 against 1, in semitones
        float detuneCents = 0.0f;        // oscillator 2 a little higher still
        float mixPercent = 0.0f;         // 0: oscillator 1 alone, 50: both as loud, 100: oscillator 2 alone
        float noisePercent = 0.0f;

        FilterType filterType = FilterType::lowPass;
        bool steep = true;               // 24 dB per octave; off, 12
        float cutoffHz = 20000.0f;
        float resonancePercent = 0.0f;
        float keyTrackPercent = 0.0f;    // 100%: the cutoff follows the notes all the way
        float envOctaves = 0.0f;         // how far the filter envelope moves the cutoff at its peak

        Envelope filterEnv { 0.001f, 0.3f, 0.0f, 0.3f };
        Envelope ampEnv;

        float lfoRateHz = 5.0f;
        float lfoDepthPercent = 0.0f;
        LfoTarget lfoTarget = LfoTarget::pitch;

        float glideSeconds = 0.0f;
        Mode mode = Mode::poly;
        float velocityPercent = 50.0f;   // how much softer notes are quieter and duller
        float outputDb = 0.0f;
    };

    constexpr float minCutoff = 20.0f, maxCutoff = 20000.0f, maxEnvOctaves = 5.0f, minOutput = -24.0f, maxOutput = 6.0f,
                    minWidth = 5.0f, maxDetune = 50.0f, maxGlide = 2.0f, minTime = 0.001f, maxTime = 10.0f,
                    minLfoRate = 0.05f, maxLfoRate = 20.0f;

    /** At 100% depth, the LFO moves the pitch by up to 2 semitones, the cutoff by 2 octaves, the
        volume all the way down, and the pulse width by 45 points. */
    constexpr float lfoPitchSemitones = 2.0f, lfoFilterOctaves = 2.0f, lfoWidthPoints = 45.0f;

    /** How loud one voice is at full velocity before the output knob: a saw note peaks at about -12 dB
        (sawPeakDb), the filter's ringing at the wave's jump adding about 1.5 dB to the wave's own peak. */
    constexpr float voiceGain = 0.21f, sawPeakDb = -12.0f;

    /** The note the window's pictures and test note use: A3, 220 Hz (MIDI 57). */
    constexpr int testNote = 57;
    constexpr double testNoteHz = 220.0;

    double getNoteHz (double note);

    //==============================================================================
    /** The filter's resonance as Q: 0.71 at 0% (no peak) up to 20 at 100%, where it nearly rings on its own. */
    float getQ (float resonancePercent);

    /** The filter's response at `hz`, as the analog filter it's modelled on: a 2-pole (12 dB per octave)
        state-variable filter, or for 24 dB two in a row, the first without resonance. */
    std::complex<double> getFilterResponse (FilterType, bool steep, double cutoffHz, float resonancePercent, double hz);

    /** How much the filter changes the level at `hz`, in dB. */
    double getFilterDb (FilterType, bool steep, double cutoffHz, float resonancePercent, double hz);

    /** Where the filter is for a note: the cutoff knob moved by key tracking (from middle C), the
        filter envelope (at `envelope`, 0 to 1, scaled by velocity) and the LFO's octaves; 20 Hz to 20 kHz. */
    double getCutoffAt (const Settings&, double note, float velocity, float envelope, float lfoOctaves);

    /** How much a note at this velocity (0 to 1) keeps of its level and of the filter envelope. */
    float getVelocityScale (const Settings&, float velocity);

    /** The level of an ADSR envelope `seconds` after the note started; released `releasedAt` seconds
        after it started (negative while it's held). Linear attack, decay and release, as the voices run them. */
    float getEnvelopeLevel (const Envelope&, double seconds, double releasedAt = -1.0);

    /** How many times a second two oscillators this far apart beat against each other at this note. */
    double getBeatHz (double noteHz, float detuneCents);

    //==============================================================================
    /** A sine the oscillators are made of: its frequency, and its amplitude and phase before the
        filter (as the mix plays it, a wave peaking at 1 at full level) and after it, as a complex number
        c, so the sine is |c| sin (2 pi f t + arg c). Which oscillator it comes from: 1 or 2. */
    struct Partial
    {
        double hz = 0.0;
        std::complex<double> before, after;
        int oscillator = 1;

        float getBefore() const { return (float) std::abs (before); }
        float getAfter() const { return (float) std::abs (after); }
    };

    /** Harmonic `n` (1 is the fundamental) of a wave peaking at 1, as a complex amplitude (see Partial):
        a saw has every harmonic at 2 / (pi n), a square the odd ones at 4 / (pi n), a pulse of width w
        4 / (pi n) |sin (pi n w)|, a triangle the odd ones at 8 / (pi n)^2, a sine only the first. */
    std::complex<double> getHarmonic (Wave, float widthPercent, int n);

    /** The partials of the two oscillators at this note up to `maxHz`, mixed, and what the filter at
        `cutoffHz` leaves of each. */
    std::vector<Partial> getPartials (const Settings&, double noteHz, double cutoffHz, double maxHz = 20000.0);

    /** A stretch of the sound as samples, from the partials: before the filter, or after it. */
    std::vector<float> getWave (const std::vector<Partial>&, double seconds, int points, bool filtered);

    //==============================================================================
    /** "Saw", "35%", "+7 st", "1.20 kHz", "350 ms", "+3.0 oct": as the knobs show them. */
    juce::String formatKnob (Knob, const Settings&);

    /** The knob's name in the window. */
    juce::String getKnobName (Knob);

    /** A starting point for a sound, with why it's made the way it is and notes on the knobs that matter. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob; empty when it stays as in Init
    };

    /** Init (a plain saw, nothing moving) first, then the sounds. */
    const std::vector<Preset>& getPresets();
} // namespace sub

//==============================================================================
/**
    DawIt Subtractive: a subtractive synthesiser that explains itself. Two oscillators (saw, square or
    pulse, triangle, sine) and noise are mixed and sent through a filter (low-pass, band-pass or
    high-pass, 12 or 24 dB per octave, with resonance and key tracking) that a filter envelope opens
    and closes; an amp envelope shapes each note's level; an LFO moves the pitch, the filter, the
    volume or the pulse width. Poly or mono with glide. It's an instrument in the track's chain (see
    BuiltInFormat.h), played by the track's MIDI notes.

    For learning it also offers: a test note to play from the window, the sound without the filter,
    a history of how loud the oscillators and the filtered sound were, and the newest note's
    envelopes and cutoff for the pictures.
*/
class BuiltInSubtractive final : public juce::AudioPluginInstance
{
public:
    BuiltInSubtractive();

    static constexpr const char* identifier = "DawIt Subtractive";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInSubtractive&)> createEditorFunction;

    static const char* getParameterId (sub::Knob);
    juce::RangedAudioParameter* getKnobParameter (sub::Knob) const;

    sub::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const sub::Settings&);
    void setKnob (sub::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Hear the oscillators without the filter. Not saved. */
    std::atomic<bool> withoutFilter { false };

    /** "Play a note" in the window plays the test note, A3, for 0.8 seconds. */
    fx::Clap clap;

    /** The level of the oscillators before the filter (as the input) and after the filter and amp (as the effect). */
    fx::LevelHistory history;

    /** The newest note, for the pictures: its cutoff now, its envelopes' levels, how long ago it
        started (-1 when no note plays), how long after its start it was let go (-1 while held),
        and the LFO's value. */
    std::atomic<float> shownCutoffHz { 20000.0f }, ampLevel { 0.0f }, filterLevel { 0.0f }, noteAge { -1.0f }, releaseAge { -1.0f }, lfoValue { 0.0f };
    std::atomic<int> shownNote { sub::testNote };

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

    /** A 2-pole state-variable filter (topology-preserving transform), cheap enough to move every sample. */
    struct Svf
    {
        double ic1 = 0.0, ic2 = 0.0;

        void reset() noexcept { ic1 = ic2 = 0.0; }
        double run (double x, double g, double k, sub::FilterType) noexcept;
    };

    struct Voice
    {
        int note = -1;
        float velocity = 0.0f;
        bool held = false;
        juce::uint64 startedAt = 0;   // which note-on it was, for stealing the oldest
        double pitch = 0.0, targetPitch = 0.0;   // in MIDI notes; they differ while gliding
        double phase1 = 0.0, phase2 = 0.0;
        juce::ADSR amp, filter;
        Svf first, second;
        double age = 0.0, releasedAt = -1.0;   // seconds
        float lastCutoff = 20000.0f, lastAmp = 0.0f, lastFilter = 0.0f;

        bool isActive() const noexcept { return amp.isActive(); }
    };

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void startVoice (Voice&, int note, float velocity, bool retrigger);
    void updateEnvelopes (const sub::Settings&);
    float renderVoice (Voice&, const sub::Settings&, float lfo, float& beforeFilter);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, sub::numKnobs> knobValues {};

    double rate = 44100.0;
    std::array<Voice, numVoices> voices;
    std::vector<int> heldNotes;   // in the order they were pressed, for mono
    juce::uint64 noteCounter = 0;
    double lastPitch = -1.0;      // where the next note glides from
    double lfoPhase = 0.0;
    juce::Random noise;
    juce::SmoothedValue<float> outputGain;
    int testNoteLeft = -1;        // samples until the test note is let go
    float chunkBefore = 0.0f, chunkAfter = 0.0f;
    int chunkFill = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInSubtractive)
};
} // namespace daw
