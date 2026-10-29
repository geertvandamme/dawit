#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "EffectTools.h"
#include "StereoMeasure.h"

namespace daw
{
namespace stereo
{
/** The controls of DawIt Stereo. */
enum class Knob { monoBelow, slope, width, listen, output };
constexpr int numKnobs = 5;
/** Settings saved with the effect. */
struct Settings { float monoBelowHz = 120.0f; int slope = 24; float width = 100.0f; int listen = 0; float outputDb = 0.0f; };
/** A starting point and its explanation. */
struct Preset { juce::String name, why; Settings settings; bool match = true; };
/** Starting points for common uses. */
const std::vector<Preset>& getPresets();
/** Advice derived from the recent measurements. */
juce::StringArray analyse (const std::vector<Block>& input, const std::vector<Block>& output, const Settings&);
}
/** Makes the low end mono and adjusts the width above it. */
class BuiltInStereo final : public juce::AudioPluginInstance
{
public:
    BuiltInStereo();
    static constexpr const char* identifier = "DawIt Stereo";
    static juce::PluginDescription getDescription();
    static inline std::function<juce::AudioProcessorEditor* (BuiltInStereo&)> createEditorFunction;
    static const char* getParameterId (stereo::Knob);
    juce::RangedAudioParameter* getKnobParameter (stereo::Knob) const;
    stereo::Settings getSettings() const;
    void setSettings (const stereo::Settings&);
    void setKnob (stereo::Knob, float);
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    void applyPreset (const stereo::Preset&);
    bool isMatchingLevel() const { return matchLevel->load() >= 0.5f; }
    void setMatchingLevel (bool);
    std::atomic<bool> listenToRemoved { false };
    std::atomic<float> matchGainDb { 0.0f };
    fx::Clap clap;
    stereo::Measurement inputMeasurement, outputMeasurement;
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
    struct Filter
    {
        double z1 = 0, z2 = 0;
        double run (double x, double b0, double b1, double b2, double a1, double a2) noexcept
        { const auto y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    };
    struct Path { Filter low[2][4], high[2][4]; int slope = 24; bool on = true; };
    void split (Path&, int channel, double sample, double cutoff, double& low, double& high);
    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, stereo::numKnobs> values {};
    std::atomic<float>* matchLevel = nullptr;
    double rate = 44100.0, powerIn = 0.0, powerOut = 0.0, gain = 1.0;
    juce::SmoothedValue<float> widthSmooth, cutoffSmooth;
    Path paths[2], measurementOutput; int activePath = 0, fadeRemaining = 0, fadeLength = 1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInStereo)
};
}
