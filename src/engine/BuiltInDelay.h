#pragma once

#include "BuiltInEq.h"
#include "EffectTools.h"

namespace daw
{
namespace dly
{
    /** The controls, in the order of the window and of a preset's notes. Time covers sync and the note value. */
    enum class Knob { time, feedback, highCut, lowCut, mode, mix };
    constexpr int numKnobs = 6;

    /** Stereo: each side echoes itself. Ping-pong: the echoes bounce from left to right and back. */
    enum class Mode { stereo, pingPong };

    /** A note value the time can follow the tempo in. `beats` are quarter notes. */
    struct NoteValue
    {
        const char* name;
        double beats;
    };

    /** From short to long: 1/32 to a bar of 4/4. */
    inline constexpr NoteValue noteValues[] = {
        { "1/32", 0.125 }, { "1/16 triplet", 1.0 / 6.0 }, { "1/16", 0.25 }, { "1/8 triplet", 1.0 / 3.0 }, { "dotted 1/16", 0.375 },
        { "1/8", 0.5 }, { "1/4 triplet", 2.0 / 3.0 }, { "dotted 1/8", 0.75 }, { "1/4", 1.0 }, { "dotted 1/4", 1.5 }, { "1/2", 2.0 },
        { "1 bar", 4.0 }
    };

    constexpr int numNoteValues = (int) std::size (noteValues);
    constexpr int quarterNote = 8;

    struct Settings
    {
        float timeMs = 350.0f;          // when it doesn't follow the tempo
        bool sync = true;
        int note = 5;                   // index in noteValues, when it does: 1/8
        float feedbackPercent = 35.0f;  // how much of each echo comes back as the next one
        float highCutHz = 6000.0f;      // on the echoes, again on every repeat; 20 kHz is off
        float lowCutHz = 150.0f;        // the same for the lows; 20 is off
        Mode mode = Mode::stereo;
        float mixPercent = 25.0f;
    };

    constexpr float minTime = 1.0f, maxTime = 2000.0f, maxFeedback = 95.0f, minHighCut = 1000.0f, maxHighCut = 20000.0f,
                    minLowCut = 20.0f, maxLowCut = 1000.0f;

    /** The longest echo: a bar at 60 BPM. */
    constexpr double longestMs = 4000.0;

    inline bool isHighCutOn (float hz) { return hz < maxHighCut - 1.0f; }
    inline bool isLowCutOn (float hz) { return hz > minLowCut + 0.5f; }

    /** The tempo to use: the song's, or 120 BPM while it is unknown. */
    inline double getTempo (double bpm) { return bpm > 0.0 ? bpm : 120.0; }

    /** How long a note value lasts at this tempo, in ms. */
    double getNoteMs (int note, double bpm);

    /** The time between the sound and its first echo, in ms. */
    double getDelayMs (const Settings&, double bpm);

    /** How many echoes are heard, until they are 60 dB below the first: 1 without feedback. */
    int getAudibleRepeats (float feedbackPercent);

    /** The level of echo `n` (1 is the first) compared to the first, in dB, before the filters. */
    double getRepeatDb (float feedbackPercent, int n);

    /** How far away a wall would be to give an echo after `ms`: the sound goes there and back. */
    inline double getWallMetres (double ms) { return ms / 1000.0 * 343.0 / 2.0; }

    /** What the filters leave of a frequency after `repeats` trips through them, in dB. */
    double getFilteredDb (const Settings&, double hz, int repeats, double sampleRate);

    /** The note value that lasts closest to `ms` at this tempo. */
    int findNearestNote (double ms, double bpm);

    /** "1/8", "350 ms", "35%", "6.0 kHz" / "Off", "Ping-pong": as the window shows them. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point for a sound, with why each control is set the way it is. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        std::array<juce::String, numKnobs> notes;   // by Knob
    };

    /** The neutral start first, then the sounds. */
    const std::vector<Preset>& getPresets();

    /** "Return: quarter echo": where DawIt Delay starts on a return track (100% mix). */
    const Preset& getReturnPreset();

    //==============================================================================
    /**
        The echoes, without the dry sound: a memory for each side that is written now and read
        `delay` later. What is read goes through a low cut and a high cut, comes out, and a part of
        it (the feedback) is written back in, so each echo makes the next one, a little quieter
        and a little darker. Changing the time glides to the new time, like tape, instead of jumping.
    */
    class Echoes
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void setSettings (const Settings&, double delayMs);

        /** Writes the echoes of `numSamples` of input into outL and outR. inR may be null. */
        void process (const float* inL, const float* inR, float* outL, float* outR, int numSamples);

    private:
        double rate = 44100.0;
        Settings settings;
        std::array<std::vector<float>, 2> memory;
        int write = 0;
        juce::SmoothedValue<double> delaySamples;
        float feedback = 0.0f;
        eq::Stages highCut, lowCut;
        bool highCutOn = false, lowCutOn = false;
        double filterMemory[2][2][2][2] {};   // side, filter, stage, state
    };
} // namespace dly

//==============================================================================
/**
    DawIt Delay: echoes of a track. A time, free or following the tempo, feedback for more repeats,
    a high cut and low cut that make every repeat darker and thinner, stereo or ping-pong, and a
    dry/wet mix. It runs in the track's chain like DawIt EQ (see BuiltInFormat.h).

    For learning it also offers: a clap to hear the echoes with, the echoes on their own, and a
    history of how loud the track and its echoes were, for the editor's graph.
*/
class BuiltInDelay final : public juce::AudioPluginInstance
{
public:
    BuiltInDelay();

    static constexpr const char* identifier = "DawIt Delay";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInDelay&)> createEditorFunction;

    /** The parameters: the knobs, plus "sync" and "note" that the time knob also covers. */
    static const char* getParameterId (dly::Knob);
    static constexpr const char* syncId = "sync";
    static constexpr const char* noteId = "note";
    juce::RangedAudioParameter* getKnobParameter (dly::Knob) const;

    dly::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const dly::Settings&);
    void setKnob (dly::Knob, float value);
    void setSync (bool);
    void setNote (int);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** The time between the sound and its first echo now, in ms. */
    double getDelayMs() const { return dly::getDelayMs (getSettings(), tempoBpm.load()); }

    /** Play only the echoes, at full level. Not saved. */
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
    std::array<std::atomic<float>*, dly::numKnobs> knobValues {};
    std::atomic<float>* syncValue = nullptr, * noteValue = nullptr;

    double rate = 44100.0;
    dly::Echoes echoes;
    juce::AudioBuffer<float> wet;
    juce::SmoothedValue<float> dryGain, wetGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInDelay)
};
} // namespace daw
