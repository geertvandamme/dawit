#include "RecordingActions.h"
#include "Theme.h"
#include "engine/GainStaging.h"

namespace daw::recordingui
{
namespace
{
    constexpr int maxInputs = 8;   // as many as the engine opens

    juce::StringArray getDeviceInputNames (Engine& engine)
    {
        if (auto* device = engine.deviceManager.getCurrentAudioDevice())
        {
            auto names = device->getInputChannelNames();

            while (names.size() > maxInputs)
                names.remove (names.size() - 1);

            return names;
        }

        return {};
    }

    juce::String channelLabel (int channel, bool stereo, const juce::StringArray& names)
    {
        auto text = stereo ? "Inputs " + juce::String (channel + 1) + "+" + juce::String (channel + 2) + " (stereo)"
                           : "Input " + juce::String (channel + 1) + " (mono)";

        if (juce::isPositiveAndBelow (channel, names.size()) && names[channel].isNotEmpty()
            && names[channel] != "Input " + juce::String (channel + 1))
            text << "  " << names[channel] << (stereo && channel + 1 < names.size() ? " + " + names[channel + 1] : juce::String());

        return text;
    }

    juce::String inputWords (int channel, bool stereo)
    {
        return stereo ? "Inputs " + juce::String (channel + 1) + "+" + juce::String (channel + 2) : "Input " + juce::String (channel + 1);
    }
}

juce::String getInputChipText (const Track& track)
{
    const int c = track.inputChannel.load();
    return track.inputStereo.load() ? "In " + juce::String (c + 1) + "+" + juce::String (c + 2) : "In " + juce::String (c + 1);
}

juce::PopupMenu makeInputMenu (Engine& engine, const Track& track, int firstId)
{
    juce::PopupMenu menu;
    const auto names = getDeviceInputNames (engine);
    const int current = track.inputChannel.load();
    const bool stereo = track.inputStereo.load();

    if (names.isEmpty())
    {
        menu.addItem (-1, "No audio input found: choose one in Audio / MIDI Settings", false);
        return menu;
    }

    for (int c = 0; c < names.size(); ++c)
        menu.addItem (firstId + c, channelLabel (c, false, names), true, ! stereo && current == c);

    if (names.size() > 1)
    {
        menu.addSeparator();

        for (int c = 0; c + 1 < names.size(); c += 2)
            menu.addItem (firstId + 100 + c, channelLabel (c, true, names), true, stereo && current == c);
    }

    return menu;
}

void applyInputChoice (Engine& engine, Track& track, int firstId, int chosenId)
{
    if (chosenId < firstId || chosenId >= firstId + 200)
        return;

    const int offset = chosenId - firstId;
    engine.setInput (track, offset >= 100 ? offset - 100 : offset, offset >= 100);
}

void showInputMenu (Engine& engine, Track& track, juce::Component* target)
{
    auto menu = makeInputMenu (engine, track, 1);
    auto options = juce::PopupMenu::Options();

    if (target != nullptr)
        options = options.withTargetComponent (target);

    juce::Component::SafePointer<juce::Component> alive (target);
    menu.showMenuAsync (options, [&engine, &track, alive, target] (int result)
    {
        if (result > 0 && (target == nullptr || alive != nullptr) && engine.indexOf (&track) >= 0)
            applyInputChoice (engine, track, 1, result);
    });
}

void setMonitor (Engine& engine, Track& track, bool on, std::function<void (recordingguide::Topic)> showGuide)
{
    engine.setMonitor (track, on);

    auto& settings = *engine.properties.getUserSettings();

    if (! on || settings.getBoolValue ("recording.monitorWarned", false))
        return;

    settings.setValue ("recording.monitorWarned", true);
    settings.saveIfNeeded();

    const auto roundTrip = recordingguide::getRoundTripMs (makeSnapshot (engine));
    const auto late = roundTrip > 0.0 ? "about " + juce::String (roundTrip, 1) + " ms late now" : juce::String ("a little late");
    const auto text = "Use headphones. On speakers the microphone hears itself and howls. Monitor plays the input through this track's "
                      "effects, " + late + "; your interface's direct monitoring has no delay.";

    auto options = juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::InfoIcon, "Monitor", text, "OK", "About hearing yourself...");
    juce::AlertWindow::showAsync (options, [openGuide = std::move (showGuide)] (int result)
    {
        if (result == 0 && openGuide)   // the second button
            openGuide (recordingguide::Topic::monitoring);
    });
}

recordingguide::Snapshot makeSnapshot (Engine& engine)
{
    recordingguide::Snapshot s;
    const auto setup = engine.deviceManager.getAudioDeviceSetup();
    s.inputDevice = setup.inputDeviceName;
    s.outputDevice = setup.outputDeviceName;

    if (auto* device = engine.deviceManager.getCurrentAudioDevice())
    {
        s.sampleRate = device->getCurrentSampleRate();
        s.bufferSamples = device->getCurrentBufferSizeSamples();
        s.numInputs = juce::jmin (maxInputs, device->getInputChannelNames().size());

        if (s.sampleRate > 0.0)
        {
            s.reportedInputMs = device->getInputLatencyInSamples() * 1000.0 / s.sampleRate;
            s.reportedOutputMs = device->getOutputLatencyInSamples() * 1000.0 / s.sampleRate;
            s.measured = engine.hasMeasuredLatency();
            s.measuredExtraMs = engine.getMeasuredLatencyExtraSamples() * 1000.0 / s.sampleRate;
        }
    }

    for (auto& t : engine.getTracks())
    {
        if (t->isAudio() && t->armed.load())
            s.armed.push_back ({ t->name, t->inputChannel.load(), t->inputStereo.load(), t->monitor.load(),
                                 juce::Decibels::gainToDecibels (t->inputMeterLevel.load(), -100.0f) });
    }

    s.inputAllZero = engine.isInputAllZero();

    for (auto& report : engine.getLastRecordingReport())
        if (report.numTakes > 0)
        {
            s.hasLastTake = true;
            s.lastTakeName = report.trackName;
            s.lastTakePeakDb = report.peakDb;
        }

    return s;
}

//==============================================================================
bool InputMeter::isClipHeld() const
{
    return juce::Time::getMillisecondCounterHiRes() - clippedAtMs < 2000.0;
}

bool InputMeter::update (float newLevel)
{
    const bool wasHeld = isClipHeld();

    if (newLevel > 1.0f)
        clippedAtMs = juce::Time::getMillisecondCounterHiRes();

    const bool changed = ! juce::approximatelyEqual (newLevel, level) || wasHeld != isClipHeld();
    level = newLevel;
    return changed;
}

void drawInputMeter (juce::Graphics& g, juce::Rectangle<float> area, const InputMeter& meter, bool vertical)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (area);

    const float db = juce::Decibels::gainToDecibels (meter.level, -100.0f);
    const auto zone = gainstaging::getZone (db);
    const auto colour = zone == gainstaging::Zone::over ? theme::meterRed
                      : zone == gainstaging::Zone::hot ? theme::meterYellow
                      : zone == gainstaging::Zone::healthy ? theme::meterGreen : theme::meterGreen.withAlpha (0.45f);

    const float length = vertical ? area.getHeight() : area.getWidth();
    const float filled = length * meterProportion (meter.level);

    g.setColour (colour);

    if (vertical)
        g.fillRect (area.withTop (area.getBottom() - filled));
    else
        g.fillRect (area.withWidth (filled));

    // Marks at -6 dB (where hot starts) and 0 dB.
    for (const float markDb : { gainstaging::hotAboveDb, 0.0f })
    {
        const float p = meterProportion (juce::Decibels::decibelsToGain (markDb));
        g.setColour (theme::textDim.withAlpha (0.8f));

        if (vertical)
            g.fillRect (area.getX() - 2.0f, area.getBottom() - length * p, area.getWidth() + 4.0f, 1.0f);
        else
            g.fillRect (area.getX() + length * p, area.getY() - 2.0f, 1.0f, area.getHeight() + 4.0f);
    }

    if (meter.isClipHeld())
    {
        g.setColour (theme::meterRed);

        if (vertical)
            g.fillRect (area.getX(), area.getY(), area.getWidth(), 3.0f);
        else
            g.fillRect (area.getRight() - 3.0f, area.getY(), 3.0f, area.getHeight());
    }
}

juce::String getInputMeterTooltip()
{
    return "Input level: aim for peaks around -18 to -10 dB while you play the loudest part; red means it clipped";
}

//==============================================================================
void showReportMessages (Engine& engine, int& seenVersion)
{
    const int version = engine.getRecordingReportVersion();

    if (version == seenVersion)
        return;

    seenVersion = version;
    juce::StringArray lines;

    for (auto& r : engine.getLastRecordingReport())
    {
        if (r.removedAsSilent)
            lines.add ("Nothing came in on " + inputWords (r.inputChannel, r.stereo) + " for " + r.trackName
                       + ": check the cable, the input chosen next to R, and the gain knob.");

        if (r.lostSeconds > 0.05)
            lines.add (juce::String (r.lostSeconds, 1) + " s of the take on " + r.trackName + " was lost and is silent.");

        if (r.error.isNotEmpty())
            lines.add (r.trackName + ": " + r.error);
    }

    const bool zero = engine.isInputAllZero();

    if (zero)
    {
        lines.clearQuick();
        lines.add (recordingguide::getZeroInputMessage());
    }

    if (lines.isEmpty())
        return;

   #if JUCE_MAC
    if (zero)
    {
        auto options = juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::WarningIcon, "No sound from the input",
                                                                     lines.joinIntoString ("\n\n"), "Open Privacy settings", "OK");
        juce::AlertWindow::showAsync (options, [] (int result)
        {
            if (result == 1)
                juce::URL ("x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone").launchInDefaultBrowser();
        });
        return;
    }
   #endif

    juce::AlertWindow::showMessageBoxAsync (zero ? juce::MessageBoxIconType::WarningIcon : juce::MessageBoxIconType::InfoIcon,
                                            zero ? "No sound from the input" : "Recording", lines.joinIntoString ("\n\n"));
}
} // namespace daw::recordingui
