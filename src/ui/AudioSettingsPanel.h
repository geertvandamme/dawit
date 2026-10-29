#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "RecordingActions.h"

namespace daw
{
/**
    The content of the Audio / MIDI Settings window: JUCE's device selector (output and input device, sample rate,
    buffer size, MIDI inputs) with, below it, a live input meter, the latency line ("Round trip 6.4 ms: 3.1 in,
    3.3 out, reported"), *Measure latency* and a ? that opens the recording guide at its timing topic.

    While it is open the engine keeps the device's inputs open for the meter (Engine::setInputMeterWanted); closing it
    lets them close again. *Measure latency* plays eight clicks and listens for them in a small progress window with
    Cancel, then shows what it found or what to try.
*/
class AudioSettingsPanel final : public juce::Component, private juce::Timer
{
public:
    explicit AudioSettingsPanel (Engine&);
    ~AudioSettingsPanel() override;

    /** The ? button: open the How recording works guide at its buffers and latency topic. Not set: the button stays off. */
    std::function<void()> onShowGuide;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void measureLatency();
    void showResult();

    Engine& engine;
    juce::AudioDeviceSelectorComponent selector;
    juce::Label meterLabel, latencyLabel;
    juce::TextButton measureButton { "Measure latency" };
    GuideButton guideButton { "?" };
    juce::Rectangle<int> meterArea;
    recordingui::InputMeter meter;
    juce::String shownLatency;
};
} // namespace daw
