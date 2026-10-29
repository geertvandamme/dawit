#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/Engine.h"

namespace daw
{
/**
    Global transport keys: Space plays/pauses, Home or Return goes to the start, R records,
    K turns the metronome on or off.

    The main window uses it directly; it is also attached as a KeyListener to other windows
    (plugin editors, the plugin list), so keys those windows don't use still control playback.
*/
class TransportShortcuts final : public juce::KeyListener
{
public:
    explicit TransportShortcuts (Engine& e) : engine (e) {}

    /** Starts or ends recording on the armed tracks (MIDI and audio together), explaining what to do when nothing can record. */
    static void toggleRecording (Engine& engine)
    {
        if (engine.isRecording())
            engine.stopRecording();
        else if (! engine.startRecording())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Recording",
                                                    "Nothing to record on. Arm a track with its R button: a MIDI track records your keyboard, "
                                                    "an audio track your microphone or instrument.");
    }

    bool handle (const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::spaceKey)
        {
            engine.togglePlay();
            return true;
        }

        if (key == juce::KeyPress::homeKey || key == juce::KeyPress::returnKey)
        {
            engine.setPositionBeats (0.0);
            return true;
        }

        if (key == juce::KeyPress ('r'))
        {
            toggleRecording (engine);
            return true;
        }

        if (key == juce::KeyPress ('k'))
        {
            engine.metronomeEnabled = ! engine.metronomeEnabled.load();
            return true;
        }

        return false;
    }

    bool keyPressed (const juce::KeyPress& key, juce::Component*) override { return handle (key); }

private:
    Engine& engine;
};
} // namespace daw
