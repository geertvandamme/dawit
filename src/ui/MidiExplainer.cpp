#include "MidiExplainer.h"

#include "ComputerKeyboardStrip.h"

namespace daw
{
using namespace midiguide;

namespace
{
    bool isBlackKey (int note)
    {
        const int pc = (note % 12 + 12) % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    /** The example when no MIDI clip is in the piano roll: a C major arpeggio and chord, played with some feel. */
    std::vector<MidiNote> getExampleNotes()
    {
        return { { 60, 0.0, 0.5, 0.79f }, { 64, 0.5, 0.5, 0.55f }, { 67, 1.0, 0.5, 0.71f }, { 72, 1.5, 0.5, 0.5f },
                 { 60, 2.0, 2.0, 0.84f }, { 64, 2.0, 2.0, 0.72f }, { 67, 2.0, 2.0, 0.76f } };
    }

    juce::Colour velocityColour (int velocity)
    {
        return theme::textDim.interpolatedWith (theme::accent, juce::jlimit (0.0f, 1.0f, velocity / 127.0f));
    }

    /** "1.2.3" from the clip's start. */
    juce::String formatPosition (double beat)
    {
        const int bar = (int) std::floor (beat / 4.0 + 1.0e-9);
        const double inBar = beat - bar * 4.0;
        const int b = (int) std::floor (inBar + 1.0e-9);
        return juce::String (bar + 1) + "." + juce::String (b + 1) + "." + juce::String ((int) std::floor ((inBar - b) * 4.0 + 1.0e-9) + 1);
    }
}

//==============================================================================
/** The picture of the selected topic. */
class MidiExplainer::Picture final : public juce::Component
{
public:
    Picture (Engine& e, ComputerKeyboardTyping* t) : engine (e), typing (t)
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    void setTopic (Topic t)
    {
        topic = t;
        setMouseCursor (topic == Topic::noteNumbers || topic == Topic::computerKeyboard ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }

    void setClipData (const ClipData& data)
    {
        clip = data;
        repaint();
    }

    /** The playhead from the clip's start, or below 0 when the song isn't playing in the clip. */
    void setPlayhead (double beat)
    {
        if (juce::approximatelyEqual (beat, playhead))
            return;

        playhead = beat;

        if (topic == Topic::messages || topic == Topic::whatItIs || topic == Topic::velocity)
            repaint();
    }

    /** The printed guide shows the keys as a fresh start: octave C4, velocity 100, nothing held, whatever the live state. */
    void setPrinting (bool printing)
    {
        useDefaultKeyboard = printing;
        repaint();
    }

    /** Repaints the computer keyboard topic when what it shows (held keys, octave, velocity, target, scale filter) changed. */
    void refreshLive()
    {
        if (topic != Topic::computerKeyboard)
            return;

        const auto& kb = getKeyboard();
        const auto targets = ComputerKeyboardStrip::getTargets (engine);
        const auto signature = juce::String (kb.getOctaveNote()) + "|" + juce::String (kb.getVelocity()) + "|"
                             + juce::String (kb.getHeldLetters()) + "|" + juce::String (ComputerKeyboardStrip::getSilencedLetters (engine, kb))
                             + "|" + (targets.empty() ? juce::String() : targets.front()->name);

        if (signature != liveSignature)
        {
            liveSignature = signature;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (r, 4.0f);
        r = r.reduced (16.0f, 12.0f);

        switch (topic)
        {
            case Topic::whatItIs:    paintWhatItIs (g, r); break;
            case Topic::history:     paintHistory (g, r); break;
            case Topic::cable:       paintCable (g, r); break;
            case Topic::messages:    paintMessages (g, r); break;
            case Topic::noteNumbers: paintNoteNumbers (g, r); break;
            case Topic::velocity:    paintVelocity (g, r); break;
            case Topic::computerKeyboard: paintComputerKeyboard (g, r); break;
            case Topic::controllers: paintControllers (g, r); break;
            case Topic::time:        paintTime (g, r); break;
            case Topic::files:       paintFile (g, r); break;
            case Topic::yourClip:    paintYourClip (g, r); break;
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (topic == Topic::computerKeyboard)
        {
            playKeyAt (e);
            return;
        }

        if (topic != Topic::noteNumbers || ! keyboardArea.contains (e.position))
            return;

        const int note = noteAt (e.position);

        if (note < 0)
            return;

        // Lower on the key is harder, as where a finger strikes it.
        const float depth = (e.position.y - keyboardArea.getY()) / keyboardArea.getHeight();
        const int velocity = juce::jlimit (1, 127, juce::roundToInt (20.0f + depth * 107.0f));
        clicked = juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity);
        engine.previewNote (note, velocity / 127.0f, 600);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (topic == Topic::computerKeyboard)
            playKeyAt (e);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (typing != nullptr)
            typing->mouseKeyUp();

        mouseKey = -1;
    }

private:
    Engine& engine;
    ComputerKeyboardTyping* typing = nullptr;
    ComputerKeyboard defaultKeyboard;
    bool useDefaultKeyboard = false;
    juce::String liveSignature;
    juce::Rectangle<int> typingKeysArea;
    int mouseKey = -1;
    Topic topic = Topic::whatItIs;
    ClipData clip;
    double playhead = -1.0;
    juce::Rectangle<float> keyboardArea;
    std::optional<juce::MidiMessage> clicked;

    //==============================================================================
    const ComputerKeyboard& getKeyboard() const
    {
        return typing != nullptr && ! useDefaultKeyboard ? typing->getKeyboard() : defaultKeyboard;
    }

    void playKeyAt (const juce::MouseEvent& e)
    {
        if (typing == nullptr || e.mods.isPopupMenu())
            return;

        const int index = keyboardpicture::getKeyAt (typingKeysArea, e.getPosition());

        if (index == mouseKey)
            return;

        mouseKey = index;

        if (index >= 0)
            typing->mouseKeyDown (index);
        else
            typing->mouseKeyUp();
    }

    void paintComputerKeyboard (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto& kb = getKeyboard();
        const auto targets = useDefaultKeyboard ? std::vector<Track*>() : ComputerKeyboardStrip::getTargets (engine);
        const auto silenced = useDefaultKeyboard ? std::string() : ComputerKeyboardStrip::getSilencedLetters (engine, kb);

        const auto plays = targets.empty() ? juce::String ("Plays: nothing yet (select a MIDI track)")
                                           : "Plays: " + targets.front()->name + (targets.size() > 1 ? " and " + juce::String ((int) targets.size() - 1) + " more" : juce::String());
        drawCaption (g, r, "The sixteen keys: the letter on top, the note it plays below", plays);

        auto keys = r.removeFromTop (juce::jmin (r.getHeight() - 60.0f, 190.0f));
        typingKeysArea = keys.withSizeKeepingCentre (juce::jmin (keys.getWidth(), 720.0f), keys.getHeight()).toNearestInt();
        keyboardpicture::paint (g, typingKeysArea, kb, silenced, targets.empty() && ! useDefaultKeyboard);

        r.removeFromTop (10.0f);
        drawText (g, "Z and X move the octave: the lowest key (A) plays " + keyboardpicture::getNoteName (kb.getOctaveNote())
                         + " (note " + juce::String (kb.getOctaveNote()) + ").", r.removeFromTop (18.0f), juce::Justification::centred, theme::text, 13.0f);
        drawText (g, "C and V change how hard: velocity " + juce::String (kb.getVelocity()) + " of 127, the same for every key.",
                  r.removeFromTop (18.0f), juce::Justification::centred, theme::text, 13.0f);
        drawText (g, silenced.empty() ? juce::String ("Click a key to hear it.")
                                      : "Faint keys are outside the key and kept silent by the track's scale filter. Click a key to hear it.",
                  r.removeFromTop (18.0f), juce::Justification::centred);
    }

    void drawCaption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& left, const juce::String& right = {})
    {
        auto line = r.removeFromTop (18.0f);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::text);
        g.drawText (left, line, juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.drawText (right, line, juce::Justification::centredRight, true);
        r.removeFromTop (6.0f);
    }

    void drawText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Justification j,
                   juce::Colour colour = theme::textDim, float size = 12.0f, bool mono = false)
    {
        g.setColour (colour);
        g.setFont (mono ? juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain) : juce::FontOptions (size));
        g.drawFittedText (text, r.toNearestInt(), j, 1, 0.8f);
    }

    juce::String getExampleNote() const
    {
        return clip.isExample ? "an example: select a MIDI clip to see your own" : "\"" + clip.name + "\"";
    }

    /** The clip's notes as a small piano roll, the playhead's notes lit. */
    void drawNotes (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour (theme::lane);
        g.fillRoundedRectangle (area, 3.0f);

        if (clip.notes.empty())
            return;

        int lowest = 127, highest = 0;
        for (auto& n : clip.notes)
        {
            lowest = juce::jmin (lowest, n.pitch);
            highest = juce::jmax (highest, n.pitch);
        }

        lowest -= 2;
        highest += 2;
        const float rowH = area.getHeight() / (float) (highest - lowest + 1);
        const auto beatToX = [&] (double beat) { return area.getX() + (float) (beat / juce::jmax (1.0, clip.lengthBeats)) * area.getWidth(); };

        g.setColour (theme::gridBar.withAlpha (0.5f));
        for (int beat = 0; beat <= (int) clip.lengthBeats; ++beat)
            g.fillRect (beatToX (beat), area.getY(), beat % 4 == 0 ? 1.0f : 0.5f, area.getHeight());

        for (auto& n : clip.notes)
        {
            if (n.start >= clip.lengthBeats)
                continue;

            const bool sounding = playhead >= n.start && playhead < n.getEnd();
            const float x1 = beatToX (n.start), x2 = beatToX (juce::jmin (n.getEnd(), clip.lengthBeats));
            const float y = area.getBottom() - (float) (n.pitch - lowest + 1) * rowH;
            g.setColour (sounding ? theme::meterYellow : velocityColour (toMidiVelocity (n.velocity)));
            g.fillRoundedRectangle (juce::Rectangle<float> (x1, y, juce::jmax (2.0f, x2 - x1 - 1.0f), juce::jmax (2.0f, rowH - 1.0f)), 1.5f);
        }

        if (playhead >= 0.0)
        {
            g.setColour (theme::playhead);
            g.fillRect (beatToX (playhead), area.getY(), 1.5f, area.getHeight());
        }
    }

    /** The index of the event at the playhead: the last one at or before it. */
    int getPlayingEvent() const
    {
        int index = -1;
        for (int i = 0; i < (int) clip.events.size(); ++i)
            if (clip.events[(size_t) i].beat <= playhead + 1.0e-9)
                index = i;

        return playhead >= 0.0 ? index : -1;
    }

    //==============================================================================
    void paintWhatItIs (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "What's in a MIDI clip: instructions, not sound", getExampleNote());

        auto left = r.removeFromLeft (r.getWidth() * 0.42f);
        r.removeFromLeft (16.0f);
        drawText (g, "The notes, as you see them", left.removeFromTop (16.0f), juce::Justification::centredLeft);
        auto chips = left.removeFromBottom (54.0f);
        drawNotes (g, left.withTrimmedTop (4.0f));

        // The track's instrument turns the same instructions into any sound.
        chips.removeFromTop (12.0f);
        drawText (g, "The same notes can play:", chips.removeFromTop (16.0f), juce::Justification::centredLeft);
        float x = chips.getX();
        for (const auto* name : { "Piano", "Synth", "Strings", "Drums" })
        {
            const auto chip = juce::Rectangle<float> (x, chips.getY() + 2.0f, 74.0f, 20.0f);
            g.setColour (theme::accent.withAlpha (0.25f));
            g.fillRoundedRectangle (chip, 10.0f);
            drawText (g, name, chip, juce::Justification::centred, theme::text, 12.0f);
            x += 82.0f;
        }

        drawText (g, "What MIDI sends for them", r.removeFromTop (16.0f), juce::Justification::centredLeft);
        r.removeFromTop (4.0f);
        const int playing = getPlayingEvent();
        const int first = juce::jlimit (0, juce::jmax (0, (int) clip.events.size() - 10), playing - 4);

        for (int i = first; i < juce::jmin ((int) clip.events.size(), first + 10); ++i)
        {
            const auto& e = clip.events[(size_t) i];
            auto row = r.removeFromTop (22.0f);

            if (i == playing)
            {
                g.setColour (theme::meterYellow.withAlpha (0.18f));
                g.fillRoundedRectangle (row, 3.0f);
            }

            drawText (g, formatPosition (e.beat), row.removeFromLeft (52.0f).withTrimmedLeft (4.0f), juce::Justification::centredLeft, theme::textDim, 12.0f, true);
            const bool on = e.message.isNoteOn();
            drawText (g, on ? "key down" : "key up", row.removeFromLeft (64.0f), juce::Justification::centredLeft, on ? theme::meterGreen : theme::textDim);
            drawText (g, describe (e.message), row, juce::Justification::centredLeft, theme::text);
        }
    }

    void paintHistory (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "From a cable between two synths to MIDI 2.0", "more than 40 years, and MIDI 1.0 still works");

        struct Moment { int year; const char* title; const char* what; };
        static const Moment moments[] = {
            { 1981, "The idea", "Dave Smith (Sequential Circuits) proposes one connection for every maker" },
            { 1983, "MIDI 1.0", "at the NAMM show, a Sequential Prophet-600 plays a Roland Jupiter-6" },
            { 1988, "MIDI files", "the Standard MIDI File: songs move between programs" },
            { 1991, "General MIDI", "the same 128 sounds and drum map on every device" },
            { 1999, "USB", "MIDI over USB: no special cable any more" },
            { 2015, "Bluetooth", "MIDI without wires" },
            { 2018, "MPE", "a channel per note, to bend and shape each one" },
            { 2020, "MIDI 2.0", "finer steps, and devices that tell each other what they can do" },
        };

        // A line down the left with a dot per moment, the year, what happened and what it brought.
        const float rowH = juce::jmin (36.0f, r.getHeight() / (float) std::size (moments));
        const float lineX = r.getX() + 8.0f;
        g.setColour (theme::gridBar);
        g.fillRect (lineX - 1.0f, r.getY() + rowH * 0.5f, 2.0f, rowH * (float) (std::size (moments) - 1));

        for (auto& m : moments)
        {
            auto row = r.removeFromTop (rowH);
            g.setColour (m.year == 1983 ? theme::meterYellow : theme::accent);
            g.fillEllipse (lineX - 5.0f, row.getCentreY() - 5.0f, 10.0f, 10.0f);
            row.removeFromLeft (26.0f);
            drawText (g, juce::String (m.year), row.removeFromLeft (52.0f), juce::Justification::centredLeft, theme::text, 14.0f);
            drawText (g, m.title, row.removeFromLeft (120.0f), juce::Justification::centredLeft, m.year == 1983 ? theme::meterYellow : theme::text, 14.0f);
            drawText (g, m.what, row, juce::Justification::centredLeft, theme::textDim, 13.0f);
        }
    }

    void paintCable (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "A C major chord on a MIDI cable: one note after the other", "31,250 bits a second, 10 bits a byte");

        // The 5-pin DIN socket, seen from the front: pins 4 and 5 carry the current, 2 is the shield.
        auto socketArea = r.removeFromLeft (190.0f);
        const auto centre = socketArea.withSizeKeepingCentre (130.0f, 130.0f).getCentre().translated (0.0f, -10.0f);
        g.setColour (theme::lane);
        g.fillEllipse (juce::Rectangle<float> (130.0f, 130.0f).withCentre (centre));
        g.setColour (theme::gridBar);
        g.drawEllipse (juce::Rectangle<float> (130.0f, 130.0f).withCentre (centre), 2.0f);
        g.fillRect (juce::Rectangle<float> (14.0f, 8.0f).withCentre (centre.translated (0.0f, -62.0f)));   // the key, opposite pin 2

        // The pins on an arc, numbered 1, 4, 2, 5, 3 from left to right as on the socket.
        const int order[] = { 1, 4, 2, 5, 3 };
        for (int k = 0; k < 5; ++k)
        {
            const float angle = juce::degreesToRadians (180.0f + 45.0f * (float) k);
            const auto pin = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * -38.0f;
            const bool used = order[k] == 4 || order[k] == 5 || order[k] == 2;
            g.setColour (used ? theme::accent : theme::textDim.withAlpha (0.5f));
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (pin));
            drawText (g, juce::String (order[k]), juce::Rectangle<float> (20.0f, 14.0f).withCentre (pin.translated (0.0f, -15.0f)), juce::Justification::centred);
        }

        drawText (g, "Pins 4 and 5: the signal. 2: the shield.", juce::Rectangle<float> (socketArea.getX(), centre.y + 76.0f, 190.0f, 16.0f),
                  juce::Justification::centred);
        drawText (g, "1 and 3: not used.", juce::Rectangle<float> (socketArea.getX(), centre.y + 92.0f, 190.0f, 16.0f), juce::Justification::centred);

        // Three Note Ons as the bits on the wire: idle high, a start bit low, the 8 bits lowest
        // first, a stop bit high.
        r.removeFromLeft (16.0f);
        const juce::MidiMessage chord[] = { juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), juce::MidiMessage::noteOn (1, 64, (juce::uint8) 100),
                                           juce::MidiMessage::noteOn (1, 67, (juce::uint8) 100) };
        std::vector<int> bits;
        for (auto& m : chord)
            for (int b = 0; b < m.getRawDataSize(); ++b)
            {
                bits.push_back (0);
                for (int k = 0; k < 8; ++k)
                    bits.push_back ((m.getRawData()[b] >> k) & 1);
                bits.push_back (1);
            }

        const float bitW = r.getWidth() / (float) bits.size();
        auto wave = r.withTrimmedTop (40.0f).removeFromTop (70.0f);
        const float high = wave.getY() + 8.0f, low = wave.getBottom() - 8.0f;

        for (int m = 0; m < 3; ++m)
        {
            const float x = r.getX() + (float) (m * 30) * bitW;
            g.setColour ((m % 2 == 0 ? theme::accent : theme::meterGreen).withAlpha (0.12f));
            g.fillRect (x, wave.getY(), 30.0f * bitW, wave.getHeight());
            drawText (g, describe (chord[m]).upToFirstOccurrenceOf (", velocity", false, false).replace ("Note On, channel 1, ", "Note On "),
                      juce::Rectangle<float> (x, r.getY(), 30.0f * bitW, 16.0f), juce::Justification::centred, theme::text);
            drawText (g, toHex (chord[m]), juce::Rectangle<float> (x, r.getY() + 18.0f, 30.0f * bitW, 16.0f), juce::Justification::centred,
                      theme::textDim, 12.0f, true);
        }

        juce::Path line;
        line.startNewSubPath (r.getX(), high);
        for (size_t i = 0; i < bits.size(); ++i)
        {
            const float y = bits[i] != 0 ? high : low, x = r.getX() + (float) i * bitW;
            line.lineTo (x, y);
            line.lineTo (x + bitW, y);
        }

        g.setColour (theme::text);
        g.strokePath (line, juce::PathStrokeType (1.4f));

        // The time axis, in milliseconds.
        auto axis = r.withTrimmedTop (118.0f).removeFromTop (34.0f);
        for (int m = 0; m <= 3; ++m)
        {
            const float x = r.getX() + (float) (m * 30) * bitW;
            g.setColour (theme::gridBar);
            g.fillRect (x, axis.getY(), 1.0f, 8.0f);
            drawText (g, juce::String (getCableSeconds (m * 3) * 1000.0, 2) + " ms",
                      juce::Rectangle<float> (juce::jmin (x - 34.0f, r.getRight() - 68.0f), axis.getY() + 10.0f, 68.0f, 14.0f),
                      m == 3 ? juce::Justification::centredRight : juce::Justification::centred);
        }

        auto notes = r.withTrimmedTop (170.0f);
        drawText (g, "Each byte: a start bit (low), 8 bits from the lowest, a stop bit (high). 3 bytes a note take "
                         + juce::String (getCableSeconds (3) * 1000.0, 2) + " ms,",
                  notes.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text);
        drawText (g, "so the G sounds " + juce::String (getCableSeconds (6) * 1000.0, 2) + " ms after the C. With running status (no repeated "
                         "status byte) it's " + juce::String (getCableSeconds (4) * 1000.0, 2) + " ms.",
                  notes.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text);
        drawText (g, "USB carries the same messages, much faster.", notes.removeFromTop (18.0f), juce::Justification::centredLeft);
    }

    void paintMessages (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "The clip's notes as MIDI messages", getExampleNote());

        auto header = r.removeFromTop (18.0f);
        const float positionW = 64.0f, hexW = 92.0f, binW = 92.0f;
        drawText (g, "Position", header.removeFromLeft (positionW), juce::Justification::centredLeft);
        drawText (g, "Bytes", header.removeFromLeft (hexW), juce::Justification::centredLeft);
        drawText (g, "Status byte", header.removeFromLeft (binW), juce::Justification::centredLeft);
        drawText (g, "What it says", header, juce::Justification::centredLeft);
        g.setColour (theme::gridBar);
        g.fillRect (r.getX(), r.getY(), r.getWidth(), 1.0f);
        r.removeFromTop (4.0f);

        const int rows = (int) (r.getHeight() / 21.0f);
        const int playing = getPlayingEvent();
        const int first = juce::jlimit (0, juce::jmax (0, (int) clip.events.size() - rows), playing - rows / 3);

        for (int i = first; i < juce::jmin ((int) clip.events.size(), first + rows); ++i)
        {
            const auto& m = clip.events[(size_t) i].message;
            auto row = r.removeFromTop (21.0f);

            if (i == playing)
            {
                g.setColour (theme::meterYellow.withAlpha (0.18f));
                g.fillRoundedRectangle (row, 3.0f);
            }

            const auto colour = m.isNoteOn() ? theme::text : theme::textDim;
            drawText (g, formatPosition (clip.events[(size_t) i].beat), row.removeFromLeft (positionW).withTrimmedLeft (4.0f),
                      juce::Justification::centredLeft, theme::textDim, 12.0f, true);
            drawText (g, toHex (m), row.removeFromLeft (hexW), juce::Justification::centredLeft, colour, 12.5f, true);
            drawText (g, toBinary (m.getRawData()[0]), row.removeFromLeft (binW), juce::Justification::centredLeft, theme::textDim, 12.0f, true);
            drawText (g, describe (m), row, juce::Justification::centredLeft, colour);
        }
    }

    void paintNoteNumbers (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "128 notes, one number a semitone: click a key to hear it", "lower on a key plays it harder");

        // What the clicked key sends.
        auto info = r.removeFromTop (74.0f);
        if (clicked.has_value())
        {
            const int note = clicked->getNoteNumber();
            drawText (g, describe (*clicked) + "   " + juce::String (getFrequency (note), 2) + " Hz",
                      info.removeFromTop (22.0f), juce::Justification::centredLeft, theme::text, 15.0f);
            drawText (g, "Hexadecimal: " + toHex (*clicked), info.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text, 13.0f, true);
            juce::StringArray binary;
            for (int b = 0; b < 3; ++b)
                binary.add (toBinary (clicked->getRawData()[b]));
            drawText (g, "Binary:      " + binary.joinIntoString ("   "), info.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text, 13.0f, true);
        }
        else
        {
            drawText (g, "Click a key: it plays on the clip's track, and here you see the message it sends, in words, in hexadecimal and in binary.",
                      info.removeFromTop (22.0f), juce::Justification::centredLeft, theme::text, 13.0f);
        }

        // All 128 notes, with the piano's 88 keys bracketed and the clip's notes marked.
        auto area = r.withTrimmedTop (22.0f).withTrimmedBottom (46.0f);
        const int whites = 75;   // white keys from note 0 to 127
        const float keyW = area.getWidth() / (float) whites;
        keyboardArea = area;

        std::set<int> used;
        for (auto& n : clip.notes)
            used.insert (n.pitch);

        auto whiteIndex = [] (int note) { int count = 0; for (int n = 0; n < note; ++n) count += isBlackKey (n) ? 0 : 1; return count; };
        auto keyX = [&] (int note) { return area.getX() + (float) whiteIndex (note) * keyW; };

        const int clickedNote = clicked.has_value() ? clicked->getNoteNumber() : -1;

        for (int n = 0; n < 128; ++n)
        {
            if (isBlackKey (n))
                continue;

            const auto key = juce::Rectangle<float> (keyX (n), area.getY(), keyW, area.getHeight());
            const bool onPiano = n >= pianoLowest && n <= pianoHighest;
            g.setColour (n == clickedNote ? theme::meterYellow : used.count (n) > 0 ? theme::accent : onPiano ? juce::Colour (0xffe8e9ec) : juce::Colour (0xff9a9ca3));
            g.fillRect (key.reduced (0.4f, 0.0f));
        }

        for (int n = 0; n < 128; ++n)
        {
            if (! isBlackKey (n))
                continue;

            const auto key = juce::Rectangle<float> (keyX (n) - keyW * 0.32f, area.getY(), keyW * 0.64f, area.getHeight() * 0.6f);
            g.setColour (n == clickedNote ? theme::meterYellow : used.count (n) > 0 ? theme::accent.darker (0.3f) : juce::Colour (0xff1d1e22));
            g.fillRect (key);
        }

        // The piano's range above the keys, and landmarks below.
        const float x21 = keyX (pianoLowest), x108 = keyX (pianoHighest) + keyW;
        g.setColour (theme::meterGreen);
        g.fillRect (x21, area.getY() - 9.0f, x108 - x21, 2.0f);
        g.fillRect (x21, area.getY() - 9.0f, 1.5f, 7.0f);
        g.fillRect (x108 - 1.5f, area.getY() - 9.0f, 1.5f, 7.0f);
        drawText (g, "a piano's 88 keys: notes 21 to 108", juce::Rectangle<float> (x21, area.getY() - 24.0f, x108 - x21, 14.0f),
                  juce::Justification::centred, theme::meterGreen);

        struct Landmark { int note; const char* label; };
        const Landmark landmarks[] = { { 0, "0: C-1" }, { 21, "21: A0" }, { 60, "60: middle C (C4)" }, { 69, "69: A4, 440 Hz" }, { 108, "108: C8" }, { 127, "127: G9" } };
        int i = 0;
        for (auto& l : landmarks)
        {
            const float x = keyX (l.note) + keyW * 0.5f;
            const float y = area.getBottom() + 4.0f + (i % 2 == 0 ? 0.0f : 18.0f);
            g.setColour (theme::textDim);
            g.fillRect (x - 0.5f, area.getBottom(), 1.0f, y - area.getBottom() + 2.0f);
            drawText (g, l.label, juce::Rectangle<float> (juce::jlimit (r.getX(), r.getRight() - 130.0f, x - 65.0f), y + 2.0f, 130.0f, 14.0f),
                      l.note == 0 ? juce::Justification::centredLeft : l.note == 127 ? juce::Justification::centredRight : juce::Justification::centred);
            ++i;
        }
    }

    int noteAt (juce::Point<float> p) const
    {
        const float keyW = keyboardArea.getWidth() / 75.0f;

        // Black keys first: they're on top.
        if (p.y < keyboardArea.getY() + keyboardArea.getHeight() * 0.6f)
        {
            int white = 0;
            for (int n = 0; n < 128; ++n)
            {
                if (! isBlackKey (n))
                {
                    ++white;
                    continue;
                }

                const float x = keyboardArea.getX() + (float) white * keyW;
                if (std::abs (p.x - x) < keyW * 0.32f)
                    return n;
            }
        }

        const int index = (int) ((p.x - keyboardArea.getX()) / keyW);
        int white = 0;
        for (int n = 0; n < 128; ++n)
            if (! isBlackKey (n) && white++ == index)
                return n;

        return -1;
    }

    void paintVelocity (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto stats = [&]
        {
            MidiClip c;
            c.notes = clip.notes;
            c.lengthBeats = clip.lengthBeats;
            return getVelocityStats (c);
        }();

        drawCaption (g, r, "Each note's velocity, 1 to 127: taller is harder",
                     stats.count > 0 ? "from " + juce::String (stats.lowest) + " to " + juce::String (stats.highest) + ", " + juce::String (stats.distinct)
                                           + " different" : juce::String());

        auto area = r.withTrimmedLeft (34.0f).withTrimmedBottom (20.0f);
        const auto yOf = [&] (int v) { return area.getBottom() - (float) v / 127.0f * area.getHeight(); };

        for (int v : { 32, 64, 96, 127 })
        {
            g.setColour (theme::gridBar.withAlpha (0.6f));
            g.fillRect (area.getX(), yOf (v), area.getWidth(), 1.0f);
            drawText (g, juce::String (v), juce::Rectangle<float> (r.getX(), yOf (v) - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
        }

        const auto beatToX = [&] (double beat) { return area.getX() + (float) (beat / juce::jmax (1.0, clip.lengthBeats)) * area.getWidth(); };

        for (int beat = 0; beat < (int) clip.lengthBeats; beat += 4)
            drawText (g, "bar " + juce::String (beat / 4 + 1), juce::Rectangle<float> (beatToX (beat), area.getBottom() + 4.0f, 60.0f, 14.0f),
                      juce::Justification::centredLeft);

        for (auto& n : clip.notes)
        {
            if (n.start >= clip.lengthBeats)
                continue;

            const int v = toMidiVelocity (n.velocity);
            const float x = beatToX (n.start);
            const bool sounding = playhead >= n.start && playhead < n.getEnd();
            g.setColour (sounding ? theme::meterYellow : velocityColour (v));
            g.fillRect (x, yOf (v), 4.0f, area.getBottom() - yOf (v));
            g.fillEllipse (x - 2.0f, yOf (v) - 4.0f, 8.0f, 8.0f);
        }

        if (stats.count > 0)
        {
            g.setColour (theme::meterGreen.withAlpha (0.8f));
            const float y = yOf (juce::roundToInt (stats.average));
            g.drawDashedLine ({ area.getX(), y, area.getRight(), y }, std::array<float, 2> { 5.0f, 4.0f }.data(), 2, 1.2f);
            drawText (g, "average " + juce::String (juce::roundToInt (stats.average)), juce::Rectangle<float> (area.getRight() - 100.0f, y - 16.0f, 100.0f, 14.0f),
                      juce::Justification::centredRight, theme::meterGreen);
        }
    }

    void paintControllers (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "Control Change: a controller number and a value from 0 to 127", "and pitch bend, with 14 bits");

        auto table = r.removeFromLeft (r.getWidth() * 0.6f);
        for (auto& c : getCommonControllers())
        {
            auto row = table.removeFromTop (26.0f);
            drawText (g, juce::String (c.number), row.removeFromLeft (36.0f), juce::Justification::centredRight, theme::accent, 14.0f);
            row.removeFromLeft (10.0f);
            drawText (g, c.name, row.removeFromLeft (150.0f), juce::Justification::centredLeft, theme::text, 13.0f);
            drawText (g, c.what, row, juce::Justification::centredLeft);
        }

        // Pitch bend: 0 to 16,383, the middle no bend.
        r.removeFromLeft (24.0f);
        drawText (g, "Pitch bend", r.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text, 13.0f);
        auto bar = r.removeFromTop (110.0f).withSizeKeepingCentre (r.getWidth(), 18.0f);
        g.setColour (theme::lane);
        g.fillRoundedRectangle (bar, 4.0f);
        const float mid = bar.getCentreX();
        g.setColour (theme::accent.withAlpha (0.5f));
        g.fillRoundedRectangle (bar.withLeft (mid).withWidth (bar.getWidth() * 0.25f), 4.0f);   // the example below: halfway up
        g.setColour (theme::meterYellow);
        g.fillRect (mid - 1.0f, bar.getY() - 8.0f, 2.0f, bar.getHeight() + 16.0f);

        drawText (g, "0", juce::Rectangle<float> (bar.getX(), bar.getBottom() + 6.0f, 60.0f, 14.0f), juce::Justification::centredLeft);
        drawText (g, juce::String (pitchBendCentre) + ": no bend", juce::Rectangle<float> (mid - 60.0f, bar.getY() - 26.0f, 120.0f, 14.0f), juce::Justification::centred, theme::meterYellow);
        drawText (g, "16383", juce::Rectangle<float> (bar.getRight() - 60.0f, bar.getBottom() + 6.0f, 60.0f, 14.0f), juce::Justification::centredRight);
        drawText (g, "-2 semitones", juce::Rectangle<float> (bar.getX(), bar.getBottom() + 22.0f, 90.0f, 14.0f), juce::Justification::centredLeft);
        drawText (g, "+2 semitones", juce::Rectangle<float> (bar.getRight() - 90.0f, bar.getBottom() + 22.0f, 90.0f, 14.0f), juce::Justification::centredRight);

        const auto example = juce::MidiMessage::pitchWheel (1, pitchBendCentre + 4096);
        drawText (g, "Halfway up (" + juce::String (pitchBendCentre + 4096) + "): " + toHex (example), r.removeFromTop (18.0f),
                  juce::Justification::centredLeft, theme::text, 12.0f);
        drawText (g, "E0, then the lowest 7 bits, then the highest 7", r.removeFromTop (18.0f), juce::Justification::centredLeft);
        r.removeFromTop (10.0f);
        const auto sustain = juce::MidiMessage::controllerEvent (1, 64, 127);
        drawText (g, "Sustain pedal down: " + toHex (sustain), r.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text, 12.0f);
    }

    void paintTime (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const double secondsPerBeat = 60.0 / juce::jmax (1.0, clip.bpm);
        drawCaption (g, r, "One quarter note, counted three ways", "at " + juce::String (clip.bpm, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " BPM");

        // A ruler over the quarter note: `divisions` marks, every `labelEvery` one labelled with its count of `perMark`.
        auto row = [&] (const juce::String& label, int divisions, int labelEvery, int perMark, juce::Colour colour, const juce::String& note)
        {
            auto area = r.removeFromTop (70.0f);
            drawText (g, label, area.removeFromTop (18.0f), juce::Justification::centredLeft, theme::text, 13.0f);
            auto line = area.removeFromTop (30.0f);
            g.setColour (theme::lane);
            g.fillRect (line);

            for (int i = 0; i <= divisions; ++i)
            {
                const float x = line.getX() + line.getWidth() * (float) i / (float) divisions;
                const bool major = i % labelEvery == 0;
                g.setColour (colour.withAlpha (major ? 1.0f : 0.35f));
                g.fillRect (x - 0.5f, major ? line.getY() : line.getCentreY(), 1.0f, major ? line.getHeight() : line.getHeight() * 0.5f);

                if (major && i < divisions)
                    drawText (g, juce::String (i * perMark), juce::Rectangle<float> (x + 3.0f, line.getY() + 1.0f, 70.0f, 12.0f), juce::Justification::centredLeft);
            }

            drawText (g, note, area, juce::Justification::centredLeft);
        };

        row ("DawIt's MIDI files: 960 ticks", 960 / 20, 12, 20, theme::accent,
             "a sixteenth is 240 ticks, a triplet eighth 320; one tick is " + juce::String (secondsPerBeat / 960.0 * 1000.0, 2) + " ms");
        row ("MIDI clock: 24 pulses", 24, 6, 1, theme::meterGreen,
             "a pulse every " + juce::String (secondsPerBeat / clockPulsesPerQuarter * 1000.0, 1) + " ms, so a drum machine follows the tempo");
        row ("Tempo, in a MIDI file: microseconds", 4, 1, juce::roundToInt (secondsPerBeat * 1.0e6 / 4.0), theme::meterYellow,
             juce::String (juce::roundToInt (secondsPerBeat * 1.0e6)) + " microseconds a quarter note: change it, and every note moves with it");
    }

    void paintFile (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "The start of the clip's MIDI file, byte by byte", juce::String ((int) clip.file.getSize()) + " bytes in all, " + getExampleNote());

        const auto* data = static_cast<const juce::uint8*> (clip.file.getData());
        const int size = (int) clip.file.getSize();
        const juce::Colour colours[] = { theme::accent, theme::meterGreen, theme::meterYellow, juce::Colour (0xffc9a2ff), juce::Colour (0xffff8f6b) };

        auto partOf = [&] (int offset)
        {
            for (int i = 0; i < (int) clip.fileParts.size(); ++i)
                if (offset >= clip.fileParts[(size_t) i].offset && offset < clip.fileParts[(size_t) i].offset + clip.fileParts[(size_t) i].length)
                    return i;

            return -1;
        };

        // The bytes, 16 to a row, coloured by part.
        auto dump = r.removeFromLeft (16.0f * 23.0f + 10.0f);
        const int shownBytes = juce::jmin (size, clip.fileParts.empty() ? 0 : clip.fileParts.back().offset + clip.fileParts.back().length);

        for (int i = 0; i < shownBytes; ++i)
        {
            const auto cell = juce::Rectangle<float> (dump.getX() + (float) (i % 16) * 23.0f, dump.getY() + (float) (i / 16) * 22.0f, 21.0f, 19.0f);
            const int part = partOf (i);

            if (part >= 0)
            {
                g.setColour (colours[part % 5].withAlpha (0.22f));
                g.fillRoundedRectangle (cell, 2.0f);
            }

            drawText (g, juce::String::toHexString ((int) data[i]).paddedLeft ('0', 2).toUpperCase(), cell, juce::Justification::centred,
                      part >= 0 ? theme::text : theme::textDim, 12.0f, true);
        }

        // What each part says.
        r.removeFromLeft (14.0f);
        const float rowH = juce::jmin (19.0f, r.getHeight() / (float) juce::jmax (1, (int) clip.fileParts.size()));

        for (int i = 0; i < (int) clip.fileParts.size(); ++i)
        {
            const auto& part = clip.fileParts[(size_t) i];
            auto row = r.removeFromTop (rowH);
            g.setColour (colours[i % 5]);
            g.fillRoundedRectangle (row.removeFromLeft (8.0f).withSizeKeepingCentre (8.0f, 8.0f), 2.0f);
            row.removeFromLeft (6.0f);
            drawText (g, part.name, row.removeFromLeft (128.0f), juce::Justification::centredLeft, theme::text, 12.0f);
            drawText (g, part.value, row, juce::Justification::centredLeft, theme::textDim, 12.0f);
        }
    }

    void paintYourClip (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "The clip as MIDI", getExampleNote());

        MidiClip c;
        c.notes = clip.notes;
        c.lengthBeats = clip.lengthBeats;
        const auto stats = getVelocityStats (c);

        int lowest = 127, highest = 0;
        for (auto& n : clip.notes)
        {
            lowest = juce::jmin (lowest, n.pitch);
            highest = juce::jmax (highest, n.pitch);
        }

        const int running = countBytes (clip.events, true);
        const std::pair<juce::String, juce::String> tiles[] = {
            { juce::String (stats.count), "notes" },
            { stats.count > 0 ? getNoteName (lowest) + "-" + getNoteName (highest) : juce::String ("-"),
              stats.count > 0 ? "notes " + juce::String (lowest) + " to " + juce::String (highest) : juce::String ("range") },
            { stats.count > 0 ? juce::String (stats.lowest) + "-" + juce::String (stats.highest) : juce::String ("-"), "velocity" },
            { juce::String ((int) clip.events.size()), "messages, " + juce::String (countBytes (clip.events, false)) + " bytes" },
            { juce::String (getCableSeconds (running) * 1000.0, 1) + " ms", "on a 5-pin cable" },
            { juce::String ((int) clip.file.getSize()), "bytes as a MIDI file" },
        };

        auto tileRow = r.removeFromTop (78.0f);
        const float tileW = (tileRow.getWidth() - 5.0f * 10.0f) / 6.0f;

        for (auto& [value, label] : tiles)
        {
            auto tile = tileRow.removeFromLeft (tileW);
            tileRow.removeFromLeft (10.0f);
            g.setColour (theme::lane);
            g.fillRoundedRectangle (tile, 5.0f);
            drawText (g, value, tile.removeFromTop (48.0f).withTrimmedTop (8.0f), juce::Justification::centred, theme::text, 22.0f);
            drawText (g, label, tile, juce::Justification::centredTop);
        }

        r.removeFromTop (14.0f);
        drawNotes (g, r);
    }
};

//==============================================================================
MidiExplainer::MidiExplainer (Engine& e, std::function<ClipView()> clipView, ComputerKeyboardTyping* typing)
    : engine (e), getClipView (std::move (clipView))
{
    title.setText ("What is MIDI?", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    hintLabel.setFont (juce::FontOptions (12.5f));
    hintLabel.setColour (juce::Label::textColourId, theme::textDim);
    hintLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (hintLabel);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture> (engine, typing);
    addAndMakeVisible (*picture);

    explanation.setMultiLine (true, true);
    explanation.setReadOnly (true);
    explanation.setCaretVisible (false);
    explanation.setScrollbarsShown (true);
    explanation.setFont (juce::FontOptions (15.0f));
    explanation.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    explanation.setColour (juce::TextEditor::textColourId, theme::text);
    explanation.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    explanation.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    explanation.setBorder (juce::BorderSize<int> (10, 12, 10, 12));
    addAndMakeVisible (explanation);
    addAndMakeVisible (exportButton);

    list.updateContent();
    updateClip (true);
    showTopic (Topic::whatItIs);
    startTimerHz (30);
    setSize (width, height);
}

MidiExplainer::~MidiExplainer()
{
    list.setModel (nullptr);
}

int MidiExplainer::getNumPrintTopics() const
{
    return (int) std::size (allTopics);
}

booklet::Topic MidiExplainer::showPrintTopic (int index)
{
    if (! topicBeforePrinting)
        topicBeforePrinting = topic;

    showTopic (allTopics[juce::jlimit (0, getNumPrintTopics() - 1, index)]);
    return { getTopicName (topic), getTopicSummary (topic), explanation.getText(), { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image MidiExplainer::paintPrintPicture (float scale)
{
    picture->setPrinting (true);
    auto image = guideprint::snapshot (*picture, scale);
    picture->setPrinting (false);
    return image;
}

void MidiExplainer::endPrinting()
{
    if (topicBeforePrinting)
        showTopic (*topicBeforePrinting);

    topicBeforePrinting.reset();
}

void MidiExplainer::showTopic (Topic newTopic)
{
    topic = newTopic;
    const auto index = (int) (std::find (std::begin (allTopics), std::end (allTopics), topic) - std::begin (allTopics));

    if (list.getSelectedRow() != index)
        list.selectRow (index);   // comes back through selectedRowsChanged

    const auto view = getClipView != nullptr ? getClipView() : ClipView();
    const auto* clip = view.clip != nullptr && engine.findTrackFor (view.clip) != nullptr ? view.clip : nullptr;
    explanation.setText (explain (topic, clip, engine.getTempo(), engine.getKey()), false);
    explanation.moveCaretToTop (false);
    picture->setTopic (topic);
}

void MidiExplainer::updateClip (bool force)
{
    const auto view = getClipView != nullptr ? getClipView() : ClipView();
    const auto* clip = view.clip != nullptr && engine.findTrackFor (view.clip) != nullptr ? view.clip : nullptr;

    // What the picture depends on, cheaply: the clip, its notes, the tempo and the key.
    juce::int64 signature = (juce::int64) (juce::pointer_sized_int) clip;
    if (clip != nullptr)
    {
        signature = signature * 31 + (juce::int64) (clip->lengthBeats * 1000.0);
        for (auto& n : clip->notes)
            signature = signature * 31 + n.pitch * 7919 + (juce::int64) (n.start * 960.0) * 13 + (juce::int64) (n.length * 960.0) + toMidiVelocity (n.velocity) * 104729;

        signature = signature * 31 + clip->name.hashCode64();
    }

    signature = signature * 31 + (juce::int64) (engine.getTempo() * 100.0) + engine.getKey().tonic * 17 + (int) engine.getKey().scale;

    if (! force && signature == shownSignature)
        return;

    shownSignature = signature;

    ClipData data;
    data.isExample = clip == nullptr;
    data.name = clip != nullptr ? clip->name.isNotEmpty() ? clip->name : (view.track != nullptr ? view.track->name : juce::String ("the clip"))
                                : juce::String ("Example");
    data.notes = clip != nullptr ? clip->notes : getExampleNotes();
    data.lengthBeats = clip != nullptr ? clip->lengthBeats : 4.0;
    data.bpm = engine.getTempo();

    MidiClip copy;
    copy.notes = data.notes;
    copy.lengthBeats = data.lengthBeats;
    copy.name = data.name;
    data.events = getEvents (copy);
    data.file = getClipFile (copy, data.bpm, engine.getKey(), view.track != nullptr ? view.track->name : juce::String ("Example"));
    data.fileParts = describeFile (data.file);
    picture->setClipData (data);

    hintLabel.setText (clip != nullptr ? "The examples use \"" + data.name + "\", the clip in the piano roll."
                                       : juce::String ("Select a MIDI clip to use your own notes as the example."),
                       juce::dontSendNotification);

    printSource = clip != nullptr ? "the clip " + data.name + " in the piano roll" : juce::String ("a made-up example");

    if (! force)
        showTopic (topic);   // "Your clip" follows the notes
}

void MidiExplainer::timerCallback()
{
    updateClip (false);

    const auto view = getClipView != nullptr ? getClipView() : ClipView();
    const auto* clip = view.clip != nullptr && engine.findTrackFor (view.clip) != nullptr ? view.clip : nullptr;
    const double beat = engine.getPositionBeats();
    const bool inClip = clip != nullptr && engine.isPlaying() && beat >= clip->startBeat && beat < clip->startBeat + clip->lengthBeats;
    picture->setPlayhead (inClip ? beat - clip->startBeat : -1.0);
    picture->refreshLive();
}

//==============================================================================
int MidiExplainer::getNumRows()
{
    return (int) std::size (allTopics);
}

void MidiExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto t = allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (getTopicName (t), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (getTopicSummary (t), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void MidiExplainer::selectedRowsChanged (int row)
{
    if (row >= 0 && row < getNumRows() && allTopics[row] != topic)
        showTopic (allTopics[row]);
}

void MidiExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void MidiExplainer::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto top = r.removeFromTop (30);
    title.setBounds (top.removeFromLeft (240));
    exportButton.setBounds (top.removeFromRight (120));
    top.removeFromRight (10);
    hintLabel.setBounds (top);
    r.removeFromTop (10);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);
    picture->setBounds (r.removeFromTop (pictureHeight));
    r.removeFromTop (10);
    explanation.setBounds (r);
}
} // namespace daw
