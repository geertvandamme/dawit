#include "AudioSettingsPanel.h"
#include "Theme.h"

namespace daw
{
namespace
{
    /** The small window of a latency measuring: a progress bar and Cancel. It closes itself when the result is ready. */
    class MeasureContent final : public juce::Component, private juce::Timer
    {
    public:
        MeasureContent (Engine& e, std::function<void()> done) : engine (e), onDone (std::move (done)), bar (progress)
        {
            text.setText ("Playing eight clicks and listening for them...", juce::dontSendNotification);
            text.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (text);
            addAndMakeVisible (bar);

            cancel.setTooltip ("Stop measuring; nothing changes");
            cancel.onClick = [this] { engine.cancelLatencyMeasurement(); close(); };
            addAndMakeVisible (cancel);

            setSize (360, 130);
            startTimerHz (30);
        }

        ~MeasureContent() override { engine.cancelLatencyMeasurement(); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);
            text.setBounds (r.removeFromTop (24));
            r.removeFromTop (8);
            bar.setBounds (r.removeFromTop (20));
            r.removeFromTop (12);
            cancel.setBounds (r.removeFromTop (26).withSizeKeepingCentre (90, 26));
        }

    private:
        void timerCallback() override
        {
            progress = engine.getLatencyMeasurementProgress();

            if (engine.serviceLatencyMeasurement())
            {
                stopTimer();
                close (true);
            }
            else if (! engine.isMeasuringLatency())
            {
                stopTimer();
                close();
            }
        }

        void close (bool hasResult = false)
        {
            auto done = hasResult ? onDone : std::function<void()>();

            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);

            if (done)
                juce::MessageManager::callAsync (done);
        }

        Engine& engine;
        std::function<void()> onDone;
        double progress = 0.0;
        juce::Label text;
        juce::ProgressBar bar;
        juce::TextButton cancel { "Cancel" };
    };
}

AudioSettingsPanel::AudioSettingsPanel (Engine& e)
    : engine (e), selector (e.deviceManager, 0, 8, 2, 2, true, false, true, false)
{
    addAndMakeVisible (selector);
    engine.setInputMeterWanted (true);

    meterLabel.setText ("Input level", juce::dontSendNotification);
    meterLabel.setFont (juce::FontOptions (13.0f));
    meterLabel.setTooltip (recordingui::getInputMeterTooltip());
    addAndMakeVisible (meterLabel);

    latencyLabel.setFont (juce::FontOptions (13.0f));
    latencyLabel.setColour (juce::Label::textColourId, theme::textDim);
    latencyLabel.setMinimumHorizontalScale (1.0f);
    latencyLabel.setTooltip ("How long sound takes from the microphone to the speakers through your interface. DawIt moves every take earlier by this much");
    addAndMakeVisible (latencyLabel);

    measureButton.setTooltip ("Plays eight clicks and listens for them coming back, to find your interface's real delay. "
                              "Connect an output to an input with a cable, or put the microphone near a speaker");
    measureButton.onClick = [this] { measureLatency(); };
    addAndMakeVisible (measureButton);

    guideButton.setTooltip ("How buffers and latency work, with your own device as the example");
    guideButton.onClick = [this] { if (onShowGuide) onShowGuide(); };
    addAndMakeVisible (guideButton);

    for (auto* b : std::initializer_list<juce::TextButton*> { &measureButton, &guideButton })
        b->setWantsKeyboardFocus (false);

    setSize (520, 540);
    startTimerHz (30);
    timerCallback();
}

AudioSettingsPanel::~AudioSettingsPanel()
{
    engine.cancelLatencyMeasurement();
    engine.setInputMeterWanted (false);
}

void AudioSettingsPanel::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto bottom = r.removeFromBottom (110);
    selector.setBounds (r);

    auto meterRow = bottom.removeFromTop (22);
    meterLabel.setBounds (meterRow.removeFromLeft (80));
    meterArea = meterRow.reduced (0, 5);
    bottom.removeFromTop (6);
    latencyLabel.setBounds (bottom.removeFromTop (38));
    bottom.removeFromTop (6);
    auto buttons = bottom.removeFromTop (26);
    guideButton.setBounds (buttons.removeFromRight (30));
    buttons.removeFromRight (6);
    measureButton.setBounds (buttons.removeFromRight (130));
}

void AudioSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    recordingui::drawInputMeter (g, meterArea.toFloat(), meter, false);

    if (engine.getNumInputChannels() == 0)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("No input open: choose an input device above", meterArea, juce::Justification::centred);
    }
}

void AudioSettingsPanel::timerCallback()
{
    if (meter.update (engine.getDeviceInputLevel()))
        repaint (meterArea.expanded (4));

    const auto snapshot = recordingui::makeSnapshot (engine);
    const auto line = snapshot.sampleRate > 0.0 ? recordingguide::describeLatency (snapshot) : juce::String ("No audio device is open.");

    if (line != shownLatency)
    {
        shownLatency = line;
        latencyLabel.setText (line, juce::dontSendNotification);
        repaint (meterArea.expanded (4));
    }

    measureButton.setEnabled (engine.getNumInputChannels() > 0 && ! engine.isRecording() && ! engine.isMeasuringLatency());
    guideButton.setEnabled (onShowGuide != nullptr);
}

void AudioSettingsPanel::measureLatency()
{
    if (! engine.startLatencyMeasurement())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Measure latency",
                                                "DawIt needs an audio input to measure with. Choose an input device above, "
                                                "and stop recording first if it is running.");
        return;
    }

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new MeasureContent (engine, [safe = juce::Component::SafePointer<AudioSettingsPanel> (this)]
                                                          { if (safe != nullptr) safe->showResult(); }));
    options.dialogTitle = "Measure latency";
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void AudioSettingsPanel::showResult()
{
    timerCallback();
    const auto result = engine.getLastLatencyMeasurement();

    if (result.ok)
        return;   // the latency line now says "measured"

    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't hear the clicks",
                                            result.problem.isNotEmpty() ? result.problem
                                                                        : "DawIt couldn't hear the clicks come back. Connect an output to an input with a cable, "
                                                                          "or put the microphone near a speaker, and try again.");
}
} // namespace daw
