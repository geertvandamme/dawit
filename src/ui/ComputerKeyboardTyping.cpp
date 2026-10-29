#include "ComputerKeyboardTyping.h"

namespace daw
{
ComputerKeyboardTyping::ComputerKeyboardTyping (Engine& e, TransportShortcuts& next) : engine (e), transportKeys (next)
{
    reloadSettings();
}

ComputerKeyboardTyping::~ComputerKeyboardTyping()
{
    stopTimer();
}

void ComputerKeyboardTyping::reloadSettings()
{
    auto& settings = *engine.properties.getUserSettings();
    releaseAll();
    keyboard.setOctaveNote (settings.getIntValue ("typing.octave", ComputerKeyboard::defaultOctaveNote));
    keyboard.setVelocity (settings.getIntValue ("typing.velocity", ComputerKeyboard::defaultVelocity));
    setOn (settings.getBoolValue ("typing.on", false));
}

void ComputerKeyboardTyping::save()
{
    auto& settings = *engine.properties.getUserSettings();
    settings.setValue ("typing.on", on);
    settings.setValue ("typing.octave", keyboard.getOctaveNote());
    settings.setValue ("typing.velocity", keyboard.getVelocity());
}

void ComputerKeyboardTyping::setOn (bool shouldBeOn)
{
    if (shouldBeOn == on)
    {
        save();
        return;
    }

    if (! shouldBeOn)
        releaseAll();

    on = shouldBeOn;

    if (on)
        startTimerHz (30);
    else
        stopTimer();

    save();

    if (onOnChanged)
        onOnChanged();
}

void ComputerKeyboardTyping::send (const juce::MidiMessage& message)
{
    engine.addMidiInput (message);
}

void ComputerKeyboardTyping::setOctaveNote (int note)
{
    keyboard.setOctaveNote (note);
    save();
}

bool ComputerKeyboardTyping::moveOctave (int steps)
{
    const bool moved = keyboard.moveOctave (steps);
    save();
    return moved;
}

void ComputerKeyboardTyping::setVelocity (int velocity)
{
    keyboard.setVelocity (velocity);
    save();
}

bool ComputerKeyboardTyping::moveVelocity (int steps)
{
    const bool moved = keyboard.moveVelocity (steps);
    save();
    return moved;
}

void ComputerKeyboardTyping::mouseKeyDown (int index)
{
    mouseKeyUp();

    if (! juce::isPositiveAndBelow (index, ComputerKeyboard::numKeys))
        return;

    const char letter = ComputerKeyboard::getLetter (index);

    // A key already held on the keyboard keeps sounding; the mouse starts nothing then.
    if (keyboard.isHeld (letter))
        return;

    if (auto noteOn = keyboard.press (letter).noteOn)
    {
        send (*noteOn);
        mouseLetter = letter;
    }
}

void ComputerKeyboardTyping::mouseKeyUp()
{
    if (mouseLetter == 0)
        return;

    if (auto noteOff = keyboard.release (mouseLetter))
        send (*noteOff);

    mouseLetter = 0;
}

void ComputerKeyboardTyping::releaseAll()
{
    mouseLetter = 0;

    for (const auto& noteOff : keyboard.releaseAll())
        send (noteOff);
}

bool ComputerKeyboardTyping::handle (const juce::KeyPress& key)
{
    if (! on)
        return false;

    const auto mods = key.getModifiers();

    // Cmd, Ctrl and Alt keep their usual meaning; Shift and Caps Lock are ignored.
    if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown())
        return false;

    const auto c = key.getTextCharacter();

    if (c <= 0 || c > 127 || ! ComputerKeyboard::isMapped ((char) c))
        return false;

    const int octave = keyboard.getOctaveNote(), velocity = keyboard.getVelocity();
    const auto press = keyboard.press ((char) c);

    if (press.noteOn)
        send (*press.noteOn);

    if (octave != keyboard.getOctaveNote() || velocity != keyboard.getVelocity())
        save();

    return press.handled;
}

void ComputerKeyboardTyping::releaseKeysThatAreUp()
{
    for (const char letter : keyboard.getHeldLetters())
    {
        if (letter == mouseLetter)
            continue;

        if (! juce::KeyPress::isKeyCurrentlyDown (letter))   // JUCE tries both cases on the Mac
            if (auto noteOff = keyboard.release (letter))
                send (*noteOff);
    }
}

bool ComputerKeyboardTyping::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    return handle (key) || transportKeys.handle (key);
}

bool ComputerKeyboardTyping::keyStateChanged (bool, juce::Component*)
{
    if (on)
        releaseKeysThatAreUp();

    return false;
}

void ComputerKeyboardTyping::timerCallback()
{
    // Another app in front: the key-up would never reach us.
    if (! juce::Process::isForegroundProcess())
        releaseAll();
    else
        releaseKeysThatAreUp();
}
} // namespace daw
