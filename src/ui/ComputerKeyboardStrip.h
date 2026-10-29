#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ComputerKeyboardTyping.h"
#include "GuideButton.h"
#include "Theme.h"

namespace daw
{
/** The picture of the computer keyboard's sixteen keys, used by the strip and by the "What is MIDI?" guide, so a
    key looks the same everywhere. */
namespace keyboardpicture
{
    /** Draws the keys in `area`: nine white keys (letter on top, note name below) and seven black ones (letter),
        the held ones lit in the accent colour. A letter in `silencedLetters` (the scale filter keeps it quiet) is
        drawn faint with no note name; `dimmed` fades all of them (nothing to play). */
    void paint (juce::Graphics& g, juce::Rectangle<int> area, const ComputerKeyboard& keyboard,
                const std::string& silencedLetters, bool dimmed);

    /** The key (0 to 15) under a point of `area`, or -1. */
    int getKeyAt (juce::Rectangle<int> area, juce::Point<int> point);

    /** "D4", with a proper sharp sign: the name of a note for the keys. */
    juce::String getNoteName (int note);
}

/**
    The strip docked at the bottom of the main window while "Play with Computer Keyboard" is on: the octave and
    velocity with their buttons, the sixteen keys (click or drag to play them), which track hears them, a button
    to the guide and one to close the strip.

    It follows the typing state and the song on a short timer, and greys out the keys the target track's scale
    filter keeps silent.
*/
class ComputerKeyboardStrip final : public juce::Component, private juce::Timer
{
public:
    ComputerKeyboardStrip (Engine&, ComputerKeyboardTyping&);
    ~ComputerKeyboardStrip() override;

    static constexpr int height = 64;

    /** The × button or the "Hide" menu item. */
    std::function<void()> onClose;

    /** The ? button or the "What is MIDI?" menu item. */
    std::function<void()> onShowGuide;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    /** The tracks the keys play now: the armed MIDI tracks, otherwise the live track when it is a MIDI track. */
    static std::vector<Track*> getTargets (Engine&);

    /** The letters the first target's scale filter keeps silent (empty with the filter off), in key order. */
    static std::string getSilencedLetters (Engine&, const ComputerKeyboard&);

private:
    class Readout;
    class Keys;

    void timerCallback() override;
    void refresh();
    void showMenu();

    Engine& engine;
    ComputerKeyboardTyping& typing;

    juce::TextButton octaveDown { "Z -" }, octaveUp { "+ X" }, velocityDown { "C -" }, velocityUp { "+ V" }, closeButton { juce::String::fromUTF8 ("\xc3\x97") };
    GuideButton helpButton { "?" };
    std::unique_ptr<Readout> octaveReadout, velocityReadout, playsReadout;
    std::unique_ptr<Keys> keys;
};
} // namespace daw
