#include "GuideButton.h"
#include "ComputerKeyboardStrip.h"

#include "engine/MidiGuide.h"

namespace daw
{
namespace keyboardpicture
{
    namespace
    {
        constexpr float whiteCount = 9.0f, overhang = 0.3f;   // the last black key sticks out to the right

        int whitesBefore (int index)
        {
            int n = 0;

            for (int i = 0; i < index; ++i)
                if (! ComputerKeyboard::isBlackKey (i))
                    ++n;

            return n;
        }

        juce::Rectangle<float> getKeyBounds (juce::Rectangle<float> area, int index)
        {
            const float ww = area.getWidth() / (whiteCount + overhang);
            const float x = area.getX() + (float) whitesBefore (index) * ww;

            if (! ComputerKeyboard::isBlackKey (index))
                return { x, area.getY(), ww, area.getHeight() };

            const float bw = ww * 0.62f;
            return { x - bw * 0.5f, area.getY(), bw, area.getHeight() * 0.6f };
        }
    }

    juce::String getNoteName (int note)
    {
        return midiguide::getNoteName (note).replace ("#", juce::String::fromUTF8 ("\xe2\x99\xaf"));
    }

    int getKeyAt (juce::Rectangle<int> area, juce::Point<int> point)
    {
        const auto a = area.toFloat();
        const auto p = point.toFloat();

        for (int pass = 0; pass < 2; ++pass)   // the black keys sit on top, so they are tested first
            for (int i = 0; i < ComputerKeyboard::numKeys; ++i)
                if (ComputerKeyboard::isBlackKey (i) == (pass == 0) && getKeyBounds (a, i).contains (p))
                    return i;

        return -1;
    }

    void paint (juce::Graphics& g, juce::Rectangle<int> area, const ComputerKeyboard& keyboard,
                const std::string& silencedLetters, bool dimmed)
    {
        const auto a = area.toFloat();

        for (int pass = 0; pass < 2; ++pass)
        {
            for (int i = 0; i < ComputerKeyboard::numKeys; ++i)
            {
                const bool black = ComputerKeyboard::isBlackKey (i);

                if (black != (pass == 1))
                    continue;

                const char letter = ComputerKeyboard::getLetter (i);
                const bool silenced = silencedLetters.find (letter) != std::string::npos;
                const bool held = keyboard.isKeyHeld (i);
                auto r = getKeyBounds (a, i).reduced (black ? 0.0f : 0.5f);
                const float alpha = silenced ? 0.35f : dimmed ? 0.45f : 1.0f;

                const auto fill = held ? theme::accent : black ? juce::Colour (0xff121316) : juce::Colour (0xffe4e6ec);
                g.setColour (fill.withAlpha (alpha));
                g.fillRoundedRectangle (r, 3.0f);
                g.setColour (juce::Colours::black.withAlpha (0.5f * alpha));
                g.drawRoundedRectangle (r, 3.0f, 1.0f);

                const auto text = held ? juce::Colours::white : black ? theme::text : juce::Colour (0xff1b1c20);
                g.setColour (text.withAlpha (alpha));
                g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
                g.drawText (juce::String::charToString ((juce::juce_wchar) std::toupper (letter)),
                            r.withHeight (black ? r.getHeight() : 20.0f).toNearestInt(), juce::Justification::centred);

                const int note = keyboard.getNote (i);

                if (! black && ! silenced && note >= 0)
                {
                    g.setFont (juce::FontOptions (10.5f));
                    g.setColour (text.withAlpha (0.75f * alpha));
                    g.drawText (getNoteName (note), r.removeFromBottom (18.0f).toNearestInt(), juce::Justification::centred);
                }
            }
        }
    }
}

//==============================================================================
/** A piece of text that can be double-clicked, with a tooltip. */
class ComputerKeyboardStrip::Readout final : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onDoubleClick;

    void setText (const juce::String& t)
    {
        if (t != text)
        {
            text = t;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText (text, getLocalBounds(), juce::Justification::centred, 2, 0.7f);
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (onDoubleClick)
            onDoubleClick();
    }

private:
    juce::String text;
};

/** The keys: plays them under the mouse and says what is under it. */
class ComputerKeyboardStrip::Keys final : public juce::Component, public juce::TooltipClient
{
public:
    Keys (Engine& e, ComputerKeyboardTyping& t) : engine (e), typing (t) {}

    std::string silenced;
    bool dimmed = false;
    juce::String targetName;

    void paint (juce::Graphics& g) override
    {
        keyboardpicture::paint (g, getLocalBounds().reduced (0, 2), typing.getKeyboard(), silenced, dimmed);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            play (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            play (e);
    }

    void mouseUp (const juce::MouseEvent&) override { typing.mouseKeyUp(); current = -1; }

    juce::String getTooltip() override
    {
        const int index = keyboardpicture::getKeyAt (getLocalBounds().reduced (0, 2), getMouseXYRelative());

        if (index < 0)
            return "The keys of the computer keyboard. Click one to play it.";

        const auto& kb = typing.getKeyboard();
        const auto letter = juce::String::charToString ((juce::juce_wchar) std::toupper (ComputerKeyboard::getLetter (index)));
        const int note = kb.getNote (index);

        if (note < 0)
            return letter + " plays nothing at this octave: the note would be above 127.";

        const auto name = keyboardpicture::getNoteName (note) + " (note " + juce::String (note) + ")";

        if (silenced.find (ComputerKeyboard::getLetter (index)) != std::string::npos)
            return letter + " plays " + name + ". It's not in " + engine.getKey().getName() + ", so the scale filter on "
                 + targetName + " keeps it silent.";

        return letter + " plays " + name + " at velocity " + juce::String (kb.getVelocity()) + ". Click to hear it.";
    }

private:
    void play (const juce::MouseEvent& e)
    {
        const int index = keyboardpicture::getKeyAt (getLocalBounds().reduced (0, 2), e.getPosition());

        if (index == current)
            return;

        current = index;

        if (index < 0)
            typing.mouseKeyUp();
        else
            typing.mouseKeyDown (index);
    }

    Engine& engine;
    ComputerKeyboardTyping& typing;
    int current = -1;
};

//==============================================================================
std::vector<Track*> ComputerKeyboardStrip::getTargets (Engine& engine)
{
    std::vector<Track*> armed;

    for (auto& t : engine.getTracks())
        if (t->isMidi() && t->armed.load())
            armed.push_back (t.get());

    if (! armed.empty())
        return armed;

    if (auto* live = engine.getLiveMidiTrack(); live != nullptr && live->isMidi())
        return { live };

    return {};
}

std::string ComputerKeyboardStrip::getSilencedLetters (Engine& engine, const ComputerKeyboard& keyboard)
{
    const auto targets = getTargets (engine);

    if (targets.empty() || ! targets.front()->scaleFilter.load())
        return {};

    // The same bits Engine keeps for the scale filter: bit n is set when pitch class n is in the key.
    const auto& key = engine.getKey();
    int bits = 0;

    for (int interval : key.getIntervals())
        bits |= 1 << ((key.tonic + interval) % 12);

    return keyboard.getSilencedLetters (bits);
}

ComputerKeyboardStrip::ComputerKeyboardStrip (Engine& e, ComputerKeyboardTyping& t) : engine (e), typing (t)
{
    octaveReadout = std::make_unique<Readout>();
    velocityReadout = std::make_unique<Readout>();
    playsReadout = std::make_unique<Readout>();
    keys = std::make_unique<Keys> (engine, typing);

    for (auto* b : std::initializer_list<juce::TextButton*> { &octaveDown, &octaveUp, &velocityDown, &velocityUp, &helpButton, &closeButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    for (auto* r : { octaveReadout.get(), velocityReadout.get(), playsReadout.get() })
        addAndMakeVisible (r);

    addAndMakeVisible (*keys);

    octaveDown.onClick = [this] { typing.moveOctave (-1); refresh(); };
    octaveUp.onClick = [this] { typing.moveOctave (1); refresh(); };
    velocityDown.onClick = [this] { typing.moveVelocity (-1); refresh(); };
    velocityUp.onClick = [this] { typing.moveVelocity (1); refresh(); };
    octaveReadout->onDoubleClick = [this] { typing.setOctaveNote (ComputerKeyboard::defaultOctaveNote); refresh(); };
    velocityReadout->onDoubleClick = [this] { typing.setVelocity (ComputerKeyboard::defaultVelocity); refresh(); };
    helpButton.onClick = [this] { if (onShowGuide) onShowGuide(); };
    closeButton.onClick = [this] { if (onClose) onClose(); };

    helpButton.setTooltip ("What is MIDI? The computer keyboard: what it can and can't do.");
    closeButton.setTooltip ("Stop playing with the computer keyboard (Cmd+K): the letters go back to being shortcuts.");
    playsReadout->setTooltip ("The keys play the selected track, or the armed tracks when any are armed, as a MIDI keyboard does.");

    addMouseListener (this, true);   // a right-click anywhere on the strip opens its menu
    refresh();
    startTimerHz (15);
}

ComputerKeyboardStrip::~ComputerKeyboardStrip()
{
    stopTimer();
}

void ComputerKeyboardStrip::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (theme::grid);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
}

void ComputerKeyboardStrip::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    const int buttonWidth = 36, gap = 6;

    auto place = [&] (juce::Component& c, int width)
    {
        c.setBounds (r.removeFromLeft (width));
        r.removeFromLeft (gap);
    };

    octaveDown.setBounds (r.removeFromLeft (buttonWidth).withSizeKeepingCentre (buttonWidth, 26));
    r.removeFromLeft (gap);
    place (*octaveReadout, 128);
    octaveUp.setBounds (r.removeFromLeft (buttonWidth).withSizeKeepingCentre (buttonWidth, 26));
    r.removeFromLeft (gap * 3);
    velocityDown.setBounds (r.removeFromLeft (buttonWidth).withSizeKeepingCentre (buttonWidth, 26));
    r.removeFromLeft (gap);
    place (*velocityReadout, 84);
    velocityUp.setBounds (r.removeFromLeft (buttonWidth).withSizeKeepingCentre (buttonWidth, 26));
    r.removeFromLeft (gap * 3);

    closeButton.setBounds (r.removeFromRight (26).withSizeKeepingCentre (26, 26));
    r.removeFromRight (gap);
    helpButton.setBounds (r.removeFromRight (26).withSizeKeepingCentre (26, 26));
    r.removeFromRight (gap * 2);
    playsReadout->setBounds (r.removeFromRight (190));
    r.removeFromRight (gap * 2);
    keys->setBounds (r.withTrimmedTop (-4).withTrimmedBottom (-4));
}

void ComputerKeyboardStrip::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        showMenu();
}

void ComputerKeyboardStrip::showMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Reset octave (A plays C4)");
    menu.addItem (2, "Reset velocity (100)");
    menu.addItem (3, "Hide the computer keyboard");
    menu.addSeparator();
    guideicon::addItem (menu, 4, "What is MIDI? The computer keyboard...");

    menu.showMenuAsync ({}, [safe = juce::Component::SafePointer<ComputerKeyboardStrip> (this)] (int result)
    {
        if (safe == nullptr)
            return;

        if (result == 1)
            safe->typing.setOctaveNote (ComputerKeyboard::defaultOctaveNote);
        else if (result == 2)
            safe->typing.setVelocity (ComputerKeyboard::defaultVelocity);
        else if (result == 3 && safe->onClose)
            safe->onClose();
        else if (result == 4 && safe->onShowGuide)
            safe->onShowGuide();

        safe->refresh();
    });
}

void ComputerKeyboardStrip::timerCallback()
{
    refresh();
}

void ComputerKeyboardStrip::refresh()
{
    const auto& kb = typing.getKeyboard();
    using CK = ComputerKeyboard;
    const int octave = kb.getOctaveNote(), velocity = kb.getVelocity();
    const auto noteName = [] (int note) { return keyboardpicture::getNoteName (note); };

    octaveReadout->setText ("Octave: A plays " + noteName (octave));
    velocityReadout->setText ("Velocity " + juce::String (velocity));

    octaveReadout->setTooltip ("A plays " + noteName (octave) + " (note " + juce::String (octave) + "), L plays "
                               + noteName (octave + 14) + " (note " + juce::String (octave + 14)
                               + "). Z and X move it an octave. Double-click for C4.");
    velocityReadout->setTooltip ("Every note gets velocity " + juce::String (velocity) + " of 127. A computer key can't tell how hard you press, "
                                 "so set it here. Double-click for 100.");

    octaveDown.setTooltip (octave <= CK::lowestOctaveNote ? juce::String ("A already plays C0, the lowest")
                                                          : "Octave down (Z): A plays " + noteName (octave - 12));
    octaveUp.setTooltip (octave >= CK::highestOctaveNote ? juce::String ("A already plays C8, the highest")
                                                         : "Octave up (X): A plays " + noteName (octave + 12));

    const auto& steps = CK::velocitySteps;
    const auto position = (int) (std::find (steps.begin(), steps.end(), velocity) - steps.begin());
    velocityDown.setTooltip (position <= 0 ? juce::String ("Already the softest: velocity 20")
                                           : "Softer (C): velocity " + juce::String (steps[(size_t) position - 1]));
    velocityUp.setTooltip (position >= (int) steps.size() - 1 ? juce::String ("Already the hardest: velocity 127")
                                                              : "Harder (V): velocity " + juce::String (steps[(size_t) position + 1]));

    const auto targets = getTargets (engine);
    keys->dimmed = targets.empty();
    keys->silenced = getSilencedLetters (engine, kb);
    keys->targetName = targets.empty() ? juce::String() : targets.front()->name;

    if (targets.empty())
        playsReadout->setText ("Select a MIDI track to hear the keys");
    else if (targets.size() == 1)
        playsReadout->setText ("Plays: " + targets.front()->name);
    else
        playsReadout->setText ("Plays: " + targets.front()->name + " and " + juce::String ((int) targets.size() - 1) + " more");

    keys->repaint();
}

} // namespace daw
