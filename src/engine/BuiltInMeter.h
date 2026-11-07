#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "EffectTools.h"
#include "LoudnessMeter.h"
#include "StereoMeasure.h"

namespace daw
{
namespace meterfx
{
/** The controls of DawIt Meter. */
enum class Knob { target, ceiling };
/** Settings saved with the effect. */
struct Settings { float targetLufs = -14.0f; float ceilingDbtp = -1.0f; bool showTarget = true; bool resetOnPlay = true; };
/** A starting point and its explanation. The list is in the order of loudness::TargetKind. */
struct Preset { juce::String name, why; Settings settings; loudness::TargetKind kind = loudness::TargetKind::streaming; };
/** Starting points for common uses: streaming, Apple Music, podcast, both kinds of broadcast, club, and mixing. */
const std::vector<Preset>& getPresets();
/** One point of the loudness history: momentary and short-term LUFS, or NaN while there is no reading. */
struct HistoryPoint { float momentary = 0.0f, shortTerm = 0.0f; };
/** The history keeps two minutes at ten points a second. */
constexpr int historyLength = 1200;
}

/**
    Measures loudness (momentary, short-term and integrated LUFS, loudness range) and true peak, and passes the sound
    through untouched, with no delay. The audio thread owns the measurement and publishes it as atomics and a ring, which
    any number of views read without taking from each other.
*/
class BuiltInMeter final : public juce::AudioPluginInstance
{
public:
    BuiltInMeter();
    static constexpr const char* identifier = "DawIt Meter";
    static juce::PluginDescription getDescription();
    static inline std::function<juce::AudioProcessorEditor* (BuiltInMeter&)> createEditorFunction;
    static const char* getParameterId (meterfx::Knob);
    juce::RangedAudioParameter* getKnobParameter (meterfx::Knob) const;
    meterfx::Settings getSettings() const;
    void setSettings (const meterfx::Settings&);
    void setKnob (meterfx::Knob, float);
    void setShowTarget (bool);
    void setResetOnPlay (bool);
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    /** Sets target, ceiling and show target from a starting point, and remembers which one. */
    void applyPreset (const meterfx::Preset&);
    /** The starting point whose settings are still in place, or loudness::TargetKind::own when the knobs were changed. */
    loudness::TargetKind getTargetKind() const;
    /** Asks the audio thread to start a new measurement. */
    void requestReset() noexcept { resetRequested = true; }
    /** How many times a measurement was started: the editor sees a change. */
    int getResetCount() const noexcept { return resetCount.load(); }
    /** The readings as of the last block; a reading with no data yet has no value. */
    loudness::Readings getReadings() const;
    /** The last two minutes, oldest first. */
    std::vector<meterfx::HistoryPoint> getHistory() const;
    /** The loudest sample of each channel in the last block (linear), for the level bars. */
    float getBlockPeak (int channel) const noexcept { return blockPeak[(size_t) juce::jlimit (0, 1, channel)].load(); }
    /** True while the host is playing, or reports nothing; false while a host says it is stopped. */
    bool isMeasuring() const noexcept { return measuring.load(); }
    std::atomic<bool> listenToRemoved { false };   // the frame's button; this meter never changes the sound, so it is hidden
    fx::Clap clap;
    stereo::Measurement measurement;
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;
    void prepareToPlay (double, int) override;
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
    void setStateInformation (const void*, int) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void publish() noexcept;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* target = nullptr;
    std::atomic<float>* ceiling = nullptr;
    std::atomic<float>* showTarget = nullptr;
    std::atomic<float>* resetOnPlay = nullptr;
    std::atomic<float>* startingPoint = nullptr;
    loudness::Meter meter;
    double rate = 44100.0;
    int tickInterval = 4410, tickCounter = 0;
    bool wasPlaying = false, prepared = false;
    std::atomic<bool> resetRequested { false }, measuring { true };
    std::atomic<int> resetCount { 0 };
    std::array<std::atomic<float>, 11> published {};
    std::array<std::atomic<float>, 2> blockPeak {};
    std::array<std::atomic<float>, meterfx::historyLength> historyMomentary {}, historyShortTerm {};
    std::atomic<unsigned long long> historyWritten { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInMeter)
};
}
