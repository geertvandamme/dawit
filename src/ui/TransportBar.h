#pragma once

#include "engine/Engine.h"
#include "Theme.h"
#include "TimeSignatureUi.h"

namespace daw
{
/** A button that draws a transport glyph instead of text. */
class GlyphButton final : public juce::Button
{
public:
    enum class Glyph { play, pause, stop, toStart, loop, record, metronome, keyboard, notebook, inspector, undo, redo };

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

    /** The undo or redo button was clicked. */
    std::function<void()> onUndo, onRedo;

    std::function<void()> onAddAudioTrack, onAddMidiTrack, onAddBus, onToggleMixer, onToggleNotebook, onToggleInspector;

    /** + Return was clicked: called with the button, which the menu of the three kinds of return opens under. */
    std::function<void (juce::Component&)> onAddReturn;

    /** The keyboard button was clicked (turn typing on or off), or right-clicked (open the guide). */
    std::function<void()> onToggleTyping, onShowTypingGuide;

    /** Opens the Chord progressions guide at "How chords work", from the last item of the key boxes. */
    std::function<void()> onShowKeyGuide;

    /** Opens the rhythm guide at its time signatures, from the last item of the signature menu. */
    std::function<void()> onShowTimeSignatureGuide;

    /** Lights the computer keyboard button while typing is on. */
    void setTypingOn (bool on) { keyboardButton.setToggleState (on, juce::dontSendNotification); }

    /** Lights the Mixer button while the mixer shows. */
    void setMixerShown (bool shown) { mixerButton.setToggleState (shown, juce::dontSendNotification); }

    /** Lights the Notebook button while its panel is open. */
    void setNotebookShown (bool shown) { notebookButton.setToggleState (shown, juce::dontSendNotification); }

    /** Lights the inspector button while its panel is open. */
    void setInspectorShown (bool shown) { inspectorButton.setToggleState (shown, juce::dontSendNotification); }

    /** One action of the bar and the two ways to reach it: its own button while the bar is wide, and the item
        of the More... menu with `overflowId` while it is compact (0: always on the bar). */
    struct Action
    {
        juce::String name;
        juce::Component* control;
        int overflowId;
    };

    /** An item of the More... menu, with the enabled and toggled state of the button it stands for. */
    struct OverflowItem
    {
        int id;
        juce::String name;
        bool enabled;
        bool toggled;
    };

    /** Every action the bar offers, in bar order. Used by the real-app layout check. */
    std::vector<Action> getActions();

    /** The items of the More... menu in menu order, with their current states. */
    std::vector<OverflowItem> getOverflowItems() const;

    /** Does what choosing the item with this id in the More... menu does (the same as clicking its button). */
    void activateOverflowItem (int id);

    /** The More... button, shown while the bar is compact. */
    juce::Component& getOverflowButton() { return overflowButton; }

    /** Brings the undo, loop, record and metronome buttons up to date with the engine now, as the timer does 30 times a second. */
    void refreshButtonStates();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setKeyFromBoxes();

    /** Greys the undo and redo buttons out with no history, and puts the step's name in their tooltips. */
    void updateUndoButtons();
    void showMetronomeMenu();
    void showOverflowMenu();
    void saveMetronomeSettings();

    Engine& engine;

    GlyphButton undoButton { "Undo", GlyphButton::Glyph::undo };
    GlyphButton redoButton { "Redo", GlyphButton::Glyph::redo };
    GlyphButton toStartButton { "Go to start", GlyphButton::Glyph::toStart };
    GlyphButton playButton { "Play / Pause (Space)", GlyphButton::Glyph::play };
    GlyphButton stopButton { "Stop", GlyphButton::Glyph::stop };
    GlyphButton recordButton { "Record on the armed tracks (R): MIDI from your keyboard, audio from the input chosen next to R. After the count-in, if it is on", GlyphButton::Glyph::record };
    GlyphButton loopButton { "Loop", GlyphButton::Glyph::loop };
    static inline const juce::String metronomeTooltip { "Metronome (K). Right-click for the count-in and when to click." };
    GlyphButton metronomeButton { metronomeTooltip, GlyphButton::Glyph::metronome };
    GlyphButton keyboardButton { "Play notes with the computer keyboard (Cmd+K): A to L are the white keys, W E T Y U O P the black ones. "
                                 "Z and X change the octave, C and V how hard. While it's on, K plays a note instead of the metronome. "
                                 "Right-click for the guide.", GlyphButton::Glyph::keyboard };
    GlyphButton notebookButton { "Show or hide the Notebook (Cmd+9)", GlyphButton::Glyph::notebook };
    GlyphButton inspectorButton { "Show or hide the inspector (I)", GlyphButton::Glyph::inspector };
    juce::TextButton overflowButton { "More..." };
    bool savedMetronome = false;

    juce::Slider tempoSlider;
    static inline const juce::String steadyTempoTooltip { "Tempo in beats per minute: drag for whole numbers, or double-click to type one "
                                                          "with decimals, such as 97.5. Detect tempo on an audio clip sets it to the hundredth "
                                                          "(96.84), so a long recording stays on the bars" };
    juce::Label tempoLabel { {}, "BPM" };
    juce::TextButton timeSignatureButton;
    daw::TimeSignature shownSignature { 0, 0 };   // what the button and the metronome tooltip last said
    double shownBpm = 0.0;
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
