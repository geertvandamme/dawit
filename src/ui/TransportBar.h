#pragma once

#include "engine/Engine.h"
#include "Theme.h"

namespace daw
{
/** A button that draws a transport glyph instead of text. */
class GlyphButton final : public juce::Button
{
public:
    enum class Glyph { play, pause, stop, toStart, loop, record, metronome, keyboard };

    GlyphButton (const juce::String& name, Glyph g) : juce::Button (name), glyph (g)
    {
        setWantsKeyboardFocus (false);
        setTooltip (name);
    }

    void setGlyph (Glyph g)
    {
        glyph = g;
        repaint();
    }

    /** Called on a right-click instead of a click, when set. */
    std::function<void()> onPopupMenu;

    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onPopupMenu)
            onPopupMenu();
        else
            juce::Button::mouseDown (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! (e.mods.isPopupMenu() && onPopupMenu))
            juce::Button::mouseUp (e);
    }

private:
    Glyph glyph;
};

class TransportBar final : public juce::Component, private juce::Timer
{
public:
    explicit TransportBar (Engine&);
    ~TransportBar() override;

    std::function<void()> onAddAudioTrack, onAddMidiTrack, onAddBus, onToggleMixer;

    /** + Return was clicked: called with the button, which the menu of the three kinds of return opens under. */
    std::function<void (juce::Component&)> onAddReturn;

    /** The keyboard button was clicked (turn typing on or off), or right-clicked (open the guide). */
    std::function<void()> onToggleTyping, onShowTypingGuide;

    /** Opens the Chord progressions guide at "How chords work", from the last item of the key boxes. */
    std::function<void()> onShowKeyGuide;

    /** Lights the computer keyboard button while typing is on. */
    void setTypingOn (bool on) { keyboardButton.setToggleState (on, juce::dontSendNotification); }

    /** Lights the Mixer button while the mixer shows. */
    void setMixerShown (bool shown) { mixerButton.setToggleState (shown, juce::dontSendNotification); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setKeyFromBoxes();
    void showMetronomeMenu();
    void saveMetronomeSettings();

    Engine& engine;

    GlyphButton toStartButton { "Go to start", GlyphButton::Glyph::toStart };
    GlyphButton playButton { "Play / Pause (Space)", GlyphButton::Glyph::play };
    GlyphButton stopButton { "Stop", GlyphButton::Glyph::stop };
    GlyphButton recordButton { "Record on the armed tracks (R): MIDI from your keyboard, audio from the input chosen next to R. After the count-in, if it is on", GlyphButton::Glyph::record };
    GlyphButton loopButton { "Loop", GlyphButton::Glyph::loop };
    GlyphButton metronomeButton { "Metronome (K). Right-click for the count-in and when to click.", GlyphButton::Glyph::metronome };
    GlyphButton keyboardButton { "Play notes with the computer keyboard (Cmd+K): A to L are the white keys, W E T Y U O P the black ones. "
                                 "Z and X change the octave, C and V how hard. While it's on, K plays a note instead of the metronome. "
                                 "Right-click for the guide.", GlyphButton::Glyph::keyboard };
    bool savedMetronome = false;

    juce::Slider tempoSlider;
    static inline const juce::String steadyTempoTooltip { "Tempo in beats per minute: drag for whole numbers, or double-click to type one "
                                                          "with decimals, such as 97.5. Detect tempo on an audio clip sets it to the hundredth "
                                                          "(96.84), so a long recording stays on the bars" };
    juce::Label tempoLabel { {}, "BPM" };
    juce::Label positionLabel;

    juce::Label keyLabel { {}, "Key" };
    juce::ComboBox keyTonicBox, keyScaleBox;
    static constexpr int keyGuideItemId = 1000;

    juce::TextButton addAudioButton { "+ Audio Track" }, addMidiButton { "+ MIDI Track" }, addBusButton { "+ Bus" }, addReturnButton { "+ Return" },
                     mixerButton { "Mixer" };

    juce::Label masterLabel { {}, "Master" };
    juce::Slider masterSlider;
    juce::Rectangle<int> meterArea;
    float meterLeft = 0.0f, meterRight = 0.0f;
};
} // namespace daw
