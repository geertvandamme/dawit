#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "EffectTools.h"
#include "PitchTracker.h"
#include "Psola.h"
#include "SongContext.h"
#include "Tune.h"

namespace daw
{
namespace tune
{
/** The controls of DawIt Tune. */
enum class Knob { mode, target, speed, amount, tolerance, keepVibrato };
constexpr int numKnobs = 6;
}

/**
    Finds the pitch of one voice, shows how far it is from a note, and can pull it towards the nearest
    allowed note. The sound is held back by a fixed number of samples (reported as the plugin's
    latency), so it can look ahead; the track's clips are read that much earlier to make up for it.
*/
class BuiltInTune final : public juce::AudioPluginInstance, public SongContextReceiver, public KeepsLatencyWhenBypassed, private juce::Timer
{
public:
    BuiltInTune();
    ~BuiltInTune() override;

    static constexpr const char* identifier = "DawIt Tune";
    static juce::PluginDescription getDescription();
    static inline std::function<juce::AudioProcessorEditor* (BuiltInTune&)> createEditorFunction;

    static const char* getParameterId (tune::Knob);
    juce::RangedAudioParameter* getKnobParameter (tune::Knob) const;
    tune::Settings getSettings() const;
    void setSettings (const tune::Settings&);
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    void applyPreset (const tune::Preset&);

    /** How long the sound is held back at a sample rate: half the tracker's window plus one hop. */
    static int getLatencyFor (double sampleRate);

    /** How many readings the tracker makes a second (172 at 44.1 kHz), the pace of the frames. */
    double getFramesPerSecond() const noexcept { return rate / hop; }

    /** The latency in samples, the same in every mode. */
    int getLatency() const noexcept { return latency; }

    /** Plays the track as it is, with the same delay, to compare (not saved). */
    std::atomic<bool> hearOriginal { false };

    /** The pitch heard now and what it is made into, as fractional MIDI notes (-1: no clear pitch). */
    std::atomic<float> liveSung { -1.0f }, liveCorrected { -1.0f };

    /** The notes the song's key and the bar's chord allow, as the track last told it. */
    int getKeyMask() const noexcept { return keyMask.load(); }
    int getChordMask() const noexcept { return chordMask.load(); }

    fx::Clap clap;

    //==============================================================================
    /** Takes what the audio thread heard since last time into the recent frames and the take log. Done by a timer on
        the message thread; the tests call it themselves. */
    void drainFrames();

    /** The last few seconds, in the order they were heard (for the trace). Message thread. */
    const std::vector<tune::Frame>& getRecentFrames() const noexcept { return recent; }

    /** Everything heard while the song played, in song order: playing a part again replaces what was heard there. Message thread. */
    std::vector<tune::Frame> getTakeLog() const;
    void clearTakeLog();

    /** Forgets the recent frames the trace shows. */
    void clearRecentFrames() { recent.clear(); }

    /** Seconds of frames the trace can show. */
    static constexpr double recentSeconds = 17.0;

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
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
    void setStateInformation (const void*, int) override;

    void setSongContext (int keyPitchClasses, int chordPitchClasses) noexcept override
    {
        keyMask = keyPitchClasses;
        chordMask = chordPitchClasses;
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override { drainFrames(); }

    /** The pitch and shift the tracker found, looked up for any input sample. */
    struct Controls final : tune::PsolaControls
    {
        struct Entry { juce::int64 centre = 0; float period = 0.0f, ratio = 1.0f; };

        void clear() noexcept { size = 0; }
        void add (const Entry&) noexcept;
        float getPeriod (juce::int64) const override;
        float getRatio (juce::int64) const override;

        static constexpr int capacity = 64;
        std::array<Entry, (size_t) capacity> entries {};
        int size = 0;

    private:
        /** The newest entry at or before the sample (-1: none), and whether the next one is voiced too. */
        int find (juce::int64) const noexcept;
    };

    void analyse (int numSamples, juce::int64 inputStart);
    void startDraining();

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, (size_t) tune::numKnobs> values {};
    std::atomic<int> keyMask { tune::allNotes }, chordMask { 0 };

    double rate = 44100.0;
    int window = 2048, hop = 256, latency = 1280;
    pitch::Tracker tracker;
    tune::Corrector corrector;
    tune::PsolaShifter shifter;
    Controls controls;
    std::vector<float> windowData;

    juce::AbstractFifo fifo { 4096 };
    std::vector<tune::Frame> fifoFrames = std::vector<tune::Frame> (4096);
    std::vector<tune::Frame> recent;
    std::map<int, tune::Frame> log;
    int lastLogKey = -1000000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInTune)
};
}
