#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "BuiltInTune.h"
#include "EffectTools.h"
#include "Harmoniser.h"
#include "PitchTracker.h"
#include "Psola.h"
#include "SongContext.h"

namespace daw
{
namespace harm
{
/** The shared controls of DawIt Harmoniser. */
enum class Knob { harmony, key, scale, mix };
constexpr int numKnobs = 4;

/** The controls of each voice. */
enum class VoiceKnob { on, interval, steps, level, pan, detune, delay };
constexpr int numVoiceKnobs = 7;
}

/**
    Adds two pitched copies ("voices") of one voice or a single-note line: a harmony in the song's key, an
    octave, a fifth or a wide double. The lead is held back by the same fixed number of samples as DawIt Tune
    (reported as the plugin's latency), so it can look ahead at the notes it moves; the track's clips are read
    that much earlier to make up for it. Each voice is moved by PSOLA, so the vowel keeps its colour, and
    fades out where there is no clear pitch, so only the lead carries consonants and breaths.
*/
class BuiltInHarmoniser final : public juce::AudioPluginInstance, public SongContextReceiver, public KeepsLatencyWhenBypassed, private juce::Timer
{
public:
    BuiltInHarmoniser();
    ~BuiltInHarmoniser() override;

    static constexpr const char* identifier = "DawIt Harmoniser";
    static juce::PluginDescription getDescription();
    static inline std::function<juce::AudioProcessorEditor* (BuiltInHarmoniser&)> createEditorFunction;

    static const char* getParameterId (harm::Knob);

    /** The id of a voice's control; `voice` is 0 or 1 (the parameters are called v1On, v1Interval ...). */
    static juce::String getParameterId (int voice, harm::VoiceKnob);
    juce::RangedAudioParameter* getKnobParameter (harm::Knob) const;
    juce::RangedAudioParameter* getKnobParameter (int voice, harm::VoiceKnob) const;
    harm::Settings getSettings() const;
    void setSettings (const harm::Settings&);
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    void applyPreset (const harm::Preset&);

    /** How long the sound is held back at a sample rate: the same as DawIt Tune's. */
    static int getLatencyFor (double sampleRate) { return BuiltInTune::getLatencyFor (sampleRate); }

    /** How many readings the tracker makes a second (172 at 44.1 kHz), the pace of the frames. */
    double getFramesPerSecond() const noexcept { return rate / hop; }

    /** The latency in samples, the same in every state. */
    int getLatency() const noexcept { return latency; }

    /** The pitch heard now and each voice's note, as fractional MIDI notes (-1: no clear pitch, or the voice is off). */
    std::atomic<float> liveLead { -1.0f };
    std::atomic<float> liveVoice[harm::numVoices] { { -1.0f }, { -1.0f } };

    /** The song's key as the track last told it, and the notes Key harmony follows now (the key chosen here, or the song's). */
    int getSongKeyMask() const noexcept { return keyMask.load(); }
    int getEffectiveKeyMask() const;

    fx::Clap clap;

    //==============================================================================
    /** Takes what the audio thread heard since last time into the recent frames and the take log. Done by a timer on
        the message thread; the tests call it themselves. */
    void drainFrames();

    /** The last few seconds, in the order they were heard (for the trace). Message thread. */
    const std::vector<harm::Frame>& getRecentFrames() const noexcept { return recent; }

    /** Everything heard while the song played, in song order: playing a part again replaces what was heard there. Message thread. */
    std::vector<harm::Frame> getTakeLog() const;
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

    void setSongContext (int keyPitchClasses, int) noexcept override { keyMask = keyPitchClasses; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override { drainFrames(); }

    /** The pitch and the shift of each voice that the tracker found, looked up for any input sample. */
    struct Controls
    {
        struct Entry { juce::int64 centre = 0; float period = 0.0f; std::array<float, harm::numVoices> ratio { 1.0f, 1.0f }; };

        void clear() noexcept { size = 0; }
        void add (const Entry&) noexcept;
        float getPeriod (juce::int64) const;
        float getRatio (juce::int64, int voice) const;

        static constexpr int capacity = 64;
        std::array<Entry, (size_t) capacity> entries {};
        int size = 0;

    private:
        int find (juce::int64) const noexcept;
    };

    /** What one shifter is told: the shared period and its voice's ratio. */
    struct VoiceControls final : tune::PsolaControls
    {
        const Controls* controls = nullptr;
        int voice = 0;
        float getPeriod (juce::int64 s) const override { return controls->getPeriod (s); }
        float getRatio (juce::int64 s) const override { return controls->getRatio (s, voice); }
    };

    void processChunk (juce::AudioBuffer<float>&, int offset, int n, const harm::Settings&, double ppq, double bpm, bool playing);
    void analyse (int n, int offset, juce::int64 inputStart, const harm::Settings&, const std::array<bool, harm::numVoices>& active, double ppq, double bpm, bool playing);
    void startDraining();
    void writeLead (const juce::AudioBuffer<float>&, int offset, int n, int channels);
    void resetState();

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, (size_t) harm::numKnobs> values {};
    std::array<std::array<std::atomic<float>*, (size_t) harm::numVoiceKnobs>, harm::numVoices> voiceValues {};
    std::atomic<int> keyMask { tune::allNotes };

    static constexpr int chunkSize = 1024;
    double rate = 44100.0;
    int window = 2048, hop = 256, latency = 1280;
    pitch::Tracker tracker;
    std::array<tune::PsolaShifter, harm::numVoices> shifters;
    std::array<VoiceControls, harm::numVoices> views;
    Controls controls;
    std::vector<float> windowData, mid, scratch;
    std::array<std::vector<float>, harm::numVoices> voiceOut, delayRing;
    std::array<std::vector<float>, 2> leadRing;
    int leadMask = 0, delayMask = 0;
    juce::int64 leadCount = 0;
    std::array<float, harm::numVoices> gate {};
    std::array<juce::SmoothedValue<float>, harm::numVoices> gainLeft, gainRight;
    bool gainsStarted = false;
    int reference = -1;

    juce::AbstractFifo fifo { 4096 };
    std::vector<harm::Frame> fifoFrames = std::vector<harm::Frame> (4096);
    std::vector<harm::Frame> recent;
    std::map<int, harm::Frame> log;
    int lastLogKey = -1000000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInHarmoniser)
};
}
