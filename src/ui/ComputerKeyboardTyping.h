#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "TransportShortcuts.h"
#include "engine/ComputerKeyboard.h"
#include "engine/Engine.h"

namespace daw
{
/**
    Playing notes with the computer's letter keys: the one place that decides what a plain letter does
    while "Play with Computer Keyboard" is on. It turns key presses into `ComputerKeyboard` calls and sends the
    notes to the engine as a MIDI keyboard would, so previewing, recording and the scale filter all work.

    It is consulted before `TransportShortcuts` (in the main window and in the key listener the plugin windows
    share): a mapped letter plays, anything else falls through to the transport keys. K plays a note while it is on.

    Key-up is the risky part: JUCE has no key-up event and a child can swallow `keyStateChanged`. So held keys
    are checked on every key change and on a 30 Hz timer, and every held note is released when typing is turned
    off, when DawIt stops being the foreground app and before the selected track changes.

    The on state, the octave and the velocity are remembered between sessions (`typing.on`, `typing.octave`,
    `typing.velocity`). Message thread only.
*/
class ComputerKeyboardTyping final : public juce::KeyListener, private juce::Timer
{
public:
    ComputerKeyboardTyping (Engine&, TransportShortcuts& next);
    ~ComputerKeyboardTyping() override;

    /** Called when typing is turned on or off, so the strip and the menu can follow. */
    std::function<void()> onOnChanged;

    bool isOn() const noexcept { return on; }

    /** Turns typing on or off and remembers it. Turning off releases every held note. */
    void setOn (bool);

    /** Reads the three settings again (the --screenshots runner writes them after start-up). */
    void reloadSettings();

    const ComputerKeyboard& getKeyboard() const noexcept { return keyboard; }

    /** Octave (the note A plays) and velocity, remembered. Held notes keep sounding. */
    void setOctaveNote (int note);
    bool moveOctave (int steps);
    void setVelocity (int velocity);
    bool moveVelocity (int steps);

    /** Plays key `index` (0 to 15) for the mouse, and stops it again. Held keys of the mouse are not
        checked against the keyboard. */
    void mouseKeyDown (int index);
    void mouseKeyUp();

    /** Sends Note Off for every held note. Call it before the selected track changes. */
    void releaseAll();

    /** Handles a key press: true when it was a letter of the map and was used up. */
    bool handle (const juce::KeyPress&);

    /** Releases every held letter whose key is no longer down. */
    void releaseKeysThatAreUp();

    bool keyPressed (const juce::KeyPress& key, juce::Component*) override;
    bool keyStateChanged (bool isKeyDown, juce::Component*) override;

private:
    void timerCallback() override;
    void send (const juce::MidiMessage&);
    void save();

    Engine& engine;
    TransportShortcuts& transportKeys;
    ComputerKeyboard keyboard;
    bool on = false;
    char mouseLetter = 0;   // the letter the mouse holds down, 0 for none
};
} // namespace daw
