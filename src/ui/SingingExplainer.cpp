#include "SingingExplainer.h"
#include "SectionLane.h"

namespace daw
{
namespace sg = gen::singing;

namespace
{
    const juce::Colour whiteKey { 0xffdfe2e8 }, blackKey { 0xff202226 };

    juce::String dot() { return juce::String::fromUTF8 (" \xc2\xb7 "); }

    bool isBlackKey (int note)
    {
        switch (((note % 12) + 12) % 12)
        {
            case 1: case 3: case 6: case 8: case 10: return true;
            default: return false;
        }
    }

    /** The colour of a voice type's bar, from low (blue) to high (violet). */
    juce::Colour getTypeColour (int index)
    {
        return juce::Colour::fromHSV (0.52f + 0.07f * (float) index, 0.45f, 0.85f, 1.0f);
    }

    juce::Colour getRegisterColour (int which)   // 0 chest, 1 middle, 2 head
    {
        static const juce::Colour colours[] { juce::Colour (0xffff9a5c), juce::Colour (0xfff2c94c), juce::Colour (0xff5cc8ff) };
        return colours[juce::jlimit (0, 2, which)];
    }

    /** A piano keyboard drawn over a rectangle, from a white key low to a white key high. */
    struct Keyboard
    {
        juce::Rectangle<float> area;
        int low = 0, high = 0;
        float whiteWidth = 1.0f;

        Keyboard() = default;

        Keyboard (juce::Rectangle<float> a, int lowNote, int highNote) : area (a), low (lowNote), high (highNote)
        {
            whiteWidth = area.getWidth() / (float) juce::jmax (1, countWhite (low, high + 1));
        }

        static int countWhite (int from, int to)
        {
            int n = 0;

            for (int i = from; i < to; ++i)
                n += isBlackKey (i) ? 0 : 1;

            return n;
        }

        float blackWidth() const { return whiteWidth * 0.62f; }

        /** The key's rectangle (black keys are the top 60%). */
        juce::Rectangle<float> getKey (int note) const
        {
            const int whites = countWhite (low, note);

            if (isBlackKey (note))
                return { area.getX() + (float) whites * whiteWidth - blackWidth() * 0.5f, area.getY(), blackWidth(), area.getHeight() * 0.6f };

            return { area.getX() + (float) whites * whiteWidth, area.getY(), whiteWidth, area.getHeight() };
        }

        float getLeft (int note) const { return getKey (note).getX(); }
        float getRight (int note) const { return getKey (note).getRight(); }

        int getNoteAt (juce::Point<float> p) const
        {
            if (! area.contains (p))
                return -1;

            for (int n = low; n <= high; ++n)
                if (isBlackKey (n) && getKey (n).contains (p))
                    return n;

            for (int n = low; n <= high; ++n)
                if (! isBlackKey (n) && getKey (n).withHeight (area.getHeight()).contains (p))
                    return n;

            return -1;
        }
    };

    /** A piano-roll lane: pitches up, beats across. */
    struct Roll
    {
        juce::Rectangle<float> area;
        int low = 48, high = 72;
        double beat0 = 0.0, beat1 = 8.0;

        float getX (double beat) const { return area.getX() + (float) ((beat - beat0) / (beat1 - beat0)) * area.getWidth(); }
        float getRow() const { return area.getHeight() / (float) (high - low + 1); }
        float getY (int pitch) const { return area.getY() + (float) (high - pitch) * getRow(); }

        void draw (juce::Graphics& g) const
        {
            g.setColour (theme::lane);
            g.fillRect (area);

            for (int p = low; p <= high; ++p)
            {
                if (isBlackKey (p))
                {
                    g.setColour (theme::laneAlt);
                    g.fillRect (area.getX(), getY (p), area.getWidth(), getRow());
                }

                if (p % 12 == 0)
                {
                    g.setColour (theme::gridBar);
                    g.fillRect (area.getX(), getY (p) + getRow() - 0.5f, area.getWidth(), 1.0f);
                    g.setColour (theme::textDim);
                    g.setFont (juce::FontOptions (10.5f));
                    g.drawText (sg::getNoteName (p), juce::Rectangle<float> (area.getX() - 34.0f, getY (p) + getRow() - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight, false);
                }
            }

            for (int b = (int) std::ceil (beat0); b <= (int) std::floor (beat1); ++b)
            {
                g.setColour (b % 4 == 0 ? theme::gridBar : theme::grid);
                g.fillRect (getX ((double) b), area.getY(), 1.0f, area.getHeight());
            }
        }

        juce::Rectangle<float> getNote (int pitch, double start, double length, float thickness = 0.0f) const
        {
            const float h = thickness > 0.0f ? thickness : juce::jmax (3.0f, getRow() - 1.0f);
            return { getX (start) + 0.5f, getY (pitch) + (getRow() - h) * 0.5f, juce::jmax (3.0f, getX (start + length) - getX (start) - 1.0f), h };
        }
    };

    void drawText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, juce::Colour colour,
                   juce::Justification j = juce::Justification::centredLeft, bool bold = false, int lines = 1)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (text, r.toNearestInt(), j, lines, 0.8f);
    }

    /** Diagonal lines over a rectangle: the top or bottom 3 semitones of a range. */
    void hatch (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (r.toNearestInt());
        g.setColour (colour);

        for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 5.0f)
            g.drawLine (x, r.getBottom(), x + r.getHeight(), r.getY(), 1.0f);
    }
} // namespace

//==============================================================================
/** The topic's picture. It plays a note when a key (or the picture) is clicked, and offers the right-click menu on keys. */
class SingingExplainer::Picture final : public juce::Component,
                                        public juce::TooltipClient
{
public:
    void setState (sg::Topic t, sg::Range r, const sg::Analysis& a, int lit)
    {
        topic = t;
        range = r;
        analysis = a;
        litNote = lit;
        repaint();
    }

    void setLitNote (int lit)
    {
        if (lit != litNote)
        {
            litNote = lit;
            repaint();
        }
    }

    int getPreferredHeight() const
    {
        switch (topic)
        {
            case sg::Topic::voice:      return 310;
            case sg::Topic::breathing:  return 250;
            case sg::Topic::warmup:     return 280;
            case sg::Topic::range:      return 330;
            case sg::Topic::registers:  return 280;
            case sg::Topic::tune:       return 300;
            case sg::Topic::vowels:     return 300;
            case sg::Topic::vibrato:    return 270;
            case sg::Topic::loud:       return 270;
            case sg::Topic::harmony:    return 270;
            case sg::Topic::care:       return 230;
            case sg::Topic::microphone: return 330;
            case sg::Topic::yours:      return 330;
        }

        return 300;
    }

    /** A MIDI pitch and a length in milliseconds to play. */
    std::function<void (int, int)> onPlayNote;

    /** The user chose "Set as my lowest note" (true) or "highest" (false) on a key. */
    std::function<void (bool lowest, int note)> onSetRange;

    juce::String getTooltip() override
    {
        const auto p = getMouseXYRelative().toFloat();

        if (const int note = getKeyboard().getNoteAt (p); note >= 0)
            return sg::getNoteName (note) + dot() + juce::String (sg::getFrequency (note), 1) + " Hz" + dot() + "click to hear it, right-click to set it as your lowest or highest";

        for (auto& [rect, text] : zones)
            if (rect.contains (p))
                return text;

        switch (topic)
        {
            case sg::Topic::voice:      return "Click to hear A3 (220 Hz): the buzz of the vocal folds.";
            case sg::Topic::breathing:  return "A line of the song, with the rest after it, and the breath that fits in it.";
            case sg::Topic::warmup:     return "Click to hear the five-note scale, then the same a semitone higher.";
            case sg::Topic::range:      return "The usual range of each voice type, and yours. Click a key to hear it, right-click to set it as your lowest or highest.";
            case sg::Topic::registers:  return "Chest voice, the middle and head voice, with the breaks. Typical places; yours may be a few notes away.";
            case sg::Topic::tune:       return "Pitch against the target, in cents. Click to hear the target note (A4).";
            case sg::Topic::vowels:     return "The sounds of a word under its note: vowels bright, consonants dim.";
            case sg::Topic::vibrato:    return "The pitch of a 2-second note with no vibrato, a late one, and one all the way through.";
            case sg::Topic::loud:       return "The thicker a note, the louder it's sung.";
            case sg::Topic::harmony:    return "Click to hear the lead with its harmony a third above.";
            case sg::Topic::care:       return "Every day, and when to stop and see a doctor.";
            case sg::Topic::microphone: return "How the level changes with the distance to the microphone, against 20 cm.";
            case sg::Topic::yours:      return "Your range against each section of your melody. Click a key to hear it, right-click to set it as your lowest or highest.";
        }

        return {};
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = e.position;

        if (const int note = getKeyboard().getNoteAt (p); note >= 0)
        {
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu menu;
                menu.addItem (1, "Set as my lowest note");
                menu.addItem (2, "Set as my highest note");
                menu.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<Picture> (this), note] (int result)
                {
                    if (safe != nullptr && result > 0 && safe->onSetRange)
                        safe->onSetRange (result == 1, note);
                });
            }
            else if (onPlayNote)
            {
                onPlayNote (note, 600);
            }

            return;
        }

        if (e.mods.isPopupMenu() || ! onPlayNote)
            return;

        // A beat is 625 ms at 96 BPM, the tempo of the examples.
        auto play = [this] (int pitch, double start, double length)
        {
            const int ms = juce::jmax (80, (int) (length * 625.0) - 40);
            juce::Timer::callAfterDelay ((int) (start * 625.0), [safe = juce::Component::SafePointer<Picture> (this), pitch, ms]
            {
                if (safe != nullptr && safe->onPlayNote)
                    safe->onPlayNote (pitch, ms);
            });
        };

        switch (topic)
        {
            case sg::Topic::voice:   onPlayNote (57, 1200); break;
            case sg::Topic::tune:    onPlayNote (69, 1000); break;

            case sg::Topic::warmup:
            {
                const auto e2 = sg::getWarmupExample (range);

                for (auto& n : e2.scale)
                    play (n.pitch, n.start, n.length);

                for (auto& n : e2.repeat)
                    play (n.pitch, n.start, n.length);

                break;
            }

            case sg::Topic::harmony:
            {
                const auto h = sg::getHarmonyExample();

                for (auto& n : h.lead)
                    play (n.pitch, n.start, n.length);

                for (auto& n : h.harmony)
                    play (n.pitch, n.start, n.length);

                break;
            }

            case sg::Topic::breathing:
            case sg::Topic::range:
            case sg::Topic::registers:
            case sg::Topic::vowels:
            case sg::Topic::vibrato:
            case sg::Topic::loud:
            case sg::Topic::care:
            case sg::Topic::microphone:
            case sg::Topic::yours:
                break;
        }
    }

    void paint (juce::Graphics& g) override
    {
        zones.clear();
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        const auto area = getLocalBounds().toFloat().reduced (12.0f, 8.0f);

        switch (topic)
        {
            case sg::Topic::voice:      paintVoice (g, area); break;
            case sg::Topic::breathing:  paintBreathing (g, area); break;
            case sg::Topic::warmup:     paintWarmup (g, area); break;
            case sg::Topic::range:      paintRange (g, area); break;
            case sg::Topic::registers:  paintRegisters (g, area); break;
            case sg::Topic::tune:       paintTune (g, area); break;
            case sg::Topic::vowels:     paintVowels (g, area); break;
            case sg::Topic::vibrato:    paintVibrato (g, area); break;
            case sg::Topic::loud:       paintLoud (g, area); break;
            case sg::Topic::harmony:    paintHarmony (g, area); break;
            case sg::Topic::care:       paintCare (g, area); break;
            case sg::Topic::microphone: paintMicrophone (g, area); break;
            case sg::Topic::yours:      paintYours (g, area); break;
        }
    }

private:
    //==============================================================================
    /** The keyboard of the shown topic, or an empty one when the topic has none. */
    Keyboard getKeyboard() const
    {
        auto area = getLocalBounds().toFloat().reduced (12.0f, 8.0f);

        switch (topic)
        {
            case sg::Topic::range:
                return Keyboard (area.removeFromBottom (86.0f), sg::lowestChoice, sg::highestChoice);

            case sg::Topic::registers:
                return Keyboard (area.removeFromBottom (70.0f), 45, 84);

            case sg::Topic::yours:
            {
                const auto [low, high] = getYoursSpan();
                return Keyboard (area.removeFromBottom (80.0f), low, high);
            }

            case sg::Topic::voice:
            case sg::Topic::breathing:
            case sg::Topic::warmup:
            case sg::Topic::tune:
            case sg::Topic::vowels:
            case sg::Topic::vibrato:
            case sg::Topic::loud:
            case sg::Topic::harmony:
            case sg::Topic::care:
            case sg::Topic::microphone:
                break;
        }

        return {};
    }

    /** The keyboard of Your song: from a C to a B around the melody and the range, at least two octaves. */
    std::pair<int, int> getYoursSpan() const
    {
        int low = analysis.hasMelody ? analysis.lowest : 60, high = analysis.hasMelody ? analysis.highest : 72;

        if (range.isSet())
        {
            low = juce::jmin (low, range.lowest);
            high = juce::jmax (high, range.highest);
        }

        low = juce::jmax (0, low - 2);
        high = juce::jmin (127, high + 2);
        low -= low % 12;
        high = (high / 12) * 12 + 11;

        while (high - low < 23)
            high = juce::jmin (127, high + 12);

        return { low, juce::jmin (high, 127) };
    }

    void paintKeyboard (juce::Graphics& g, const Keyboard& k, std::function<juce::Colour (int)> tint = {})
    {
        for (int pass = 0; pass < 2; ++pass)
            for (int n = k.low; n <= k.high; ++n)
            {
                if (isBlackKey (n) != (pass == 1))
                    continue;

                auto r = k.getKey (n);
                auto colour = pass == 0 ? whiteKey : blackKey;

                if (tint != nullptr)
                {
                    const auto t = tint (n);

                    if (! t.isTransparent())
                        colour = t;
                }

                if (n == litNote && topic == sg::Topic::yours)
                    colour = theme::playhead;

                g.setColour (colour);
                g.fillRect (pass == 0 ? r.reduced (0.5f, 0.0f) : r);

                if (n % 12 == 0 && k.whiteWidth > 14.0f)
                {
                    g.setColour (juce::Colours::black.withAlpha (0.7f));
                    g.setFont (juce::FontOptions (9.5f));
                    g.drawText (sg::getNoteName (n), r.removeFromBottom (14.0f), juce::Justification::centred, false);
                }
            }
    }

    //==============================================================================
    void paintVoice (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto ex = sg::getVoiceExample();
        auto top = area.removeFromTop (50.0f);
        const char* names[] { "Breath", "Vocal folds", "Throat and mouth" };
        const char* notes[] { "the power", "the buzz", "the shape" };
        const float boxWidth = (top.getWidth() - 80.0f) / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            const auto box = juce::Rectangle<float> (top.getX() + (float) i * (boxWidth + 40.0f), top.getY(), boxWidth, top.getHeight());
            g.setColour (theme::accent.withAlpha (0.22f + 0.1f * (float) i));
            g.fillRoundedRectangle (box, 5.0f);
            drawText (g, names[i], box.withTrimmedBottom (20.0f), 15.0f, theme::text, juce::Justification::centred, true);
            drawText (g, notes[i], box.withTrimmedTop (28.0f), 12.0f, theme::textDim, juce::Justification::centred);

            if (i < 2)
                drawText (g, juce::String::fromUTF8 ("\xe2\x86\x92"), { box.getRight(), box.getY(), 40.0f, box.getHeight() }, 22.0f, theme::textDim, juce::Justification::centred);
        }

        area.removeFromTop (10.0f);
        drawText (g, "The same 220 Hz buzz, shaped two ways (an illustration, not a measured spectrum). Click to hear A3.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (4.0f);

        const double maxHz = ex.fundamental * (ex.harmonics + 1);
        const char* labels[] { "\"ah\": formants about 730 and 1,090 Hz", "\"ee\": formants about 270 and 2,290 Hz" };
        const double* formants[] { ex.ahFormants, ex.eeFormants };
        const float gap = 24.0f, halfWidth = (area.getWidth() - gap) / 2.0f;

        for (int s = 0; s < 2; ++s)
        {
            auto panel = juce::Rectangle<float> (area.getX() + (float) s * (halfWidth + gap), area.getY(), halfWidth, area.getHeight());
            drawText (g, labels[s], panel.removeFromTop (18.0f), 13.0f, theme::text, juce::Justification::centredLeft, true);
            auto axis = panel.removeFromBottom (16.0f);
            g.setColour (theme::lane);
            g.fillRect (panel);
            const double f2[2] { formants[s][0], formants[s][1] };
            const double (&fs)[2] = f2;

            auto xOf = [&] (double hz) { return panel.getX() + (float) (hz / maxHz) * panel.getWidth(); };
            auto yOf = [&] (double h) { return panel.getBottom() - 4.0f - (float) h * (panel.getHeight() - 14.0f); };

            // The envelope the mouth and throat make, then the harmonics under it.
            juce::Path envelope;

            for (float x = panel.getX(); x <= panel.getRight(); x += 2.0f)
            {
                const double hz = (double) (x - panel.getX()) / panel.getWidth() * maxHz;
                const float y = yOf (sg::VoiceExample::getEnvelope (fs, hz));

                if (x <= panel.getX() + 0.1f)
                    envelope.startNewSubPath (x, y);
                else
                    envelope.lineTo (x, y);
            }

            g.setColour (theme::meterYellow.withAlpha (0.8f));
            g.strokePath (envelope, juce::PathStrokeType (1.8f));

            for (int k = 1; k <= ex.harmonics; ++k)
            {
                const double hz = ex.fundamental * k;
                const float x = xOf (hz);
                g.setColour (theme::accent);
                g.fillRect (x - 1.0f, yOf (sg::VoiceExample::getEnvelope (fs, hz)), 2.0f, panel.getBottom() - 4.0f - yOf (sg::VoiceExample::getEnvelope (fs, hz)));
            }

            for (int i = 0; i < 2; ++i)
            {
                g.setColour (theme::meterYellow.withAlpha (0.5f));
                g.drawVerticalLine ((int) xOf (fs[i]), panel.getY(), panel.getBottom());
            }

            for (int hz = 0; hz <= 4000; hz += 1000)
                drawText (g, hz == 0 ? juce::String ("0") : juce::String (hz / 1000) + " kHz", { xOf ((double) hz) - 2.0f, axis.getY(), 50.0f, axis.getHeight() }, 10.5f, theme::textDim);
        }
    }

    //==============================================================================
    void paintBreathing (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto ex = sg::getBreathingExample();
        drawText (g, "At 96 BPM a beat is 0.625 seconds. Breathe in the rests, not in the middle of a word.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (6.0f);
        const float labelW = 4.0f;
        const double beats = 24.0;
        auto xOf = [&] (double beat) { return area.getX() + labelW + (float) (beat / beats) * (area.getWidth() - labelW); };
        const float rowH = (area.getHeight() - 22.0f) / 3.0f;

        for (int b = 0; b <= 24; b += 4)
        {
            g.setColour (theme::gridBar);
            g.fillRect (xOf ((double) b), area.getY(), 1.0f, area.getHeight() - 18.0f);
            if (b < 24)
                drawText (g, "bar " + juce::String (b / 4 + 1), { xOf ((double) b) + 3.0f, area.getBottom() - 16.0f, 50.0f, 14.0f }, 10.5f, theme::textDim);
        }

        float y = area.getY();

        for (size_t i = 0; i < ex.lines.size(); ++i)
        {
            auto& line = ex.lines[i];
            const auto words = juce::Rectangle<float> (xOf (line.startBeat), y + 6.0f, xOf (line.endBeat) - xOf (line.startBeat) - 2.0f, 26.0f);
            g.setColour (theme::accent.withAlpha (0.6f));
            g.fillRoundedRectangle (words, 4.0f);
            drawText (g, line.text, words.reduced (6.0f, 0.0f), 12.5f, theme::text, juce::Justification::centredLeft);

            if (line.restEndBeat > line.endBeat)
            {
                const auto rest = juce::Rectangle<float> (xOf (line.endBeat), y + 6.0f, xOf (line.restEndBeat) - xOf (line.endBeat) - 2.0f, 26.0f);
                g.setColour (theme::meterGreen.withAlpha (0.25f));
                g.fillRoundedRectangle (rest, 4.0f);
                g.setColour (theme::meterGreen);
                g.drawRoundedRectangle (rest, 4.0f, 1.2f);
            }

            if (line.breathMark >= 0.0)
            {
                const float x = xOf (line.breathMark);
                g.setColour (theme::meterGreen);

                for (float yy = y + 2.0f; yy < y + 36.0f; yy += 5.0f)
                    g.fillRect (x - 1.0f, yy, 2.0f, 3.0f);
            }

            // The caption sits under the rest, or right-aligned under a line with none, as far as there's room.
            const float captionX = line.restEndBeat > line.endBeat ? xOf (line.endBeat) : xOf (line.startBeat);
            const float captionW = juce::jmin (330.0f, area.getRight() - captionX);
            drawText (g, line.caption, { juce::jmin (captionX, area.getRight() - 330.0f), y + 34.0f, juce::jmax (captionW, 330.0f), 16.0f }, 12.0f,
                      line.restEndBeat > line.endBeat ? theme::meterGreen : theme::meterYellow, juce::Justification::centredLeft, true);

            zones.push_back ({ words, line.text + dot() + juce::String (ex.getLineSeconds (i), 2) + " s" });
            y += rowH;
        }
    }

    //==============================================================================
    void paintWarmup (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto ex = sg::getWarmupExample (range);
        const int low = juce::jmin (ex.sirenLow, ex.startNote) - 1, high = juce::jmax (ex.sirenHigh, ex.startNote + 9) + 1;
        drawText (g, "Click to hear the scale and the same a semitone higher. Then slide through the siren.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (4.0f);
        const auto labels = area.removeFromBottom (18.0f);
        Roll roll { area.withTrimmedLeft (38.0f), low, high, 0.0, 15.0 };
        roll.draw (g);

        auto drawBlock = [&] (const sg::NoteBlock& n, juce::Colour c, float alpha)
        {
            g.setColour (c.withAlpha (alpha));
            g.fillRoundedRectangle (roll.getNote (n.pitch, n.start, n.length), 2.0f);
        };

        for (auto& n : ex.scale)
            drawBlock (n, theme::accent, 1.0f);

        for (auto& n : ex.repeat)
            drawBlock (n, theme::accent, 0.4f);

        // The siren: a smooth glide up and back down.
        juce::Path siren;

        for (int i = 0; i <= 60; ++i)
        {
            const double t = (double) i / 60.0;
            const double pitch = (double) ex.sirenLow + (double) (ex.sirenHigh - ex.sirenLow) * 0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi * t));
            const float x = roll.getX (10.5 + t * ex.sirenBeats), y = roll.area.getY() + (float) ((double) roll.high - pitch + 0.5) * roll.getRow();

            if (i == 0)
                siren.startNewSubPath (x, y);
            else
                siren.lineTo (x, y);
        }

        g.setColour (theme::meterGreen);
        g.strokePath (siren, juce::PathStrokeType (3.0f));

        drawText (g, "Five-note scale", { roll.getX (0.0), labels.getY(), 200.0f, 16.0f }, 12.0f, theme::text, juce::Justification::centredLeft, true);
        drawText (g, "A semitone higher", { roll.getX (5.0), labels.getY(), 200.0f, 16.0f }, 12.0f, theme::textDim, juce::Justification::centredLeft, true);
        drawText (g, "Siren", { roll.getX (10.5), labels.getY(), 200.0f, 16.0f }, 12.0f, theme::meterGreen, juce::Justification::centredLeft, true);
    }

    //==============================================================================
    void paintRange (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto kb = getKeyboard();
        area.removeFromBottom (86.0f);
        area.removeFromBottom (6.0f);
        const auto& types = sg::getVoiceTypes();
        const int rows = (int) types.size() + 1;
        const float rowH = juce::jmin (24.0f, area.getHeight() / (float) rows);

        auto drawBar = [&] (int row, int lo, int hi, const juce::String& label, juce::Colour colour, bool you)
        {
            const auto bar = juce::Rectangle<float> (kb.getLeft (lo), area.getY() + (float) row * rowH + 2.0f, kb.getRight (hi) - kb.getLeft (lo), rowH - 4.0f);
            g.setColour (colour.withAlpha (you ? 0.95f : 0.75f));
            g.fillRoundedRectangle (bar, 4.0f);

            if (you)
            {
                g.setColour (theme::text);
                g.drawRoundedRectangle (bar, 4.0f, 1.5f);
            }

            drawText (g, label, bar.reduced (6.0f, 0.0f), 12.0f, you ? theme::background : juce::Colour (0xff15171a), juce::Justification::centredLeft, true);
            zones.push_back ({ bar, label });
        };

        for (int i = (int) types.size() - 1; i >= 0; --i)
        {
            auto& t = types[(size_t) i];
            drawBar ((int) types.size() - 1 - i, t.lowest, t.highest, t.name + dot() + sg::getNoteName (t.lowest) + " to " + sg::getNoteName (t.highest), getTypeColour (i), false);
        }

        if (range.isSet())
            drawBar ((int) types.size(), range.lowest, range.highest, "You" + dot() + sg::getNoteName (range.lowest) + " to " + sg::getNoteName (range.highest), theme::accent, true);
        else
            drawText (g, "Your range isn't set: pick a voice type or your notes above, or right-click a key.", { area.getX(), area.getY() + (float) types.size() * rowH, area.getWidth(), rowH }, 12.5f, theme::textDim);

        paintKeyboard (g, kb, [this] (int n) { return range.isSet() && n >= range.lowest && n <= range.highest ? theme::accent.withAlpha (isBlackKey (n) ? 0.7f : 0.55f) : juce::Colour(); });
    }

    //==============================================================================
    void paintRegisters (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto kb = getKeyboard();
        area.removeFromBottom (70.0f);
        area.removeFromBottom (6.0f);
        const auto examples = sg::getRegisterExamples();
        drawText (g, "Typical places; yours may be a few notes away.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        const float rowH = (area.getHeight() - 4.0f) / (float) examples.size();
        float y = area.getY() + 12.0f;

        for (auto& e : examples)
        {
            const auto row = juce::Rectangle<float> (kb.getLeft (e.lowest), y + 6.0f, kb.getRight (e.highest) - kb.getLeft (e.lowest), rowH - 28.0f);
            const float x1 = kb.getLeft (e.chestTop), x2 = kb.getLeft (e.headBottom);
            g.setColour (getRegisterColour (0).withAlpha (0.85f));
            g.fillRect (row.withRight (x1));
            g.setColour (getRegisterColour (1).withAlpha (0.85f));
            g.fillRect (row.withLeft (x1).withRight (x2));
            g.setColour (getRegisterColour (2).withAlpha (0.85f));
            g.fillRect (row.withLeft (x2));
            drawText (g, "chest", row.withRight (x1).reduced (4.0f, 0.0f), 12.0f, juce::Colour (0xff15171a), juce::Justification::centredLeft, true);
            drawText (g, "middle", row.withLeft (x1).withRight (x2).reduced (4.0f, 0.0f), 12.0f, juce::Colour (0xff15171a), juce::Justification::centred, true);
            drawText (g, "head", row.withLeft (x2).reduced (4.0f, 0.0f), 12.0f, juce::Colour (0xff15171a), juce::Justification::centredRight, true);

            for (int br : { e.firstBreak, e.secondBreak })
            {
                const float x = kb.getLeft (br);
                g.setColour (theme::text);
                g.fillRect (x - 1.0f, row.getY() - 4.0f, 2.0f, row.getHeight() + 8.0f);
                drawText (g, "break " + sg::getNoteName (br), { x - 50.0f, row.getBottom() + 3.0f, 100.0f, 14.0f }, 11.5f, theme::text, juce::Justification::centred);
            }

            drawText (g, e.name, { row.getX(), row.getY() - 4.0f - 14.0f + 4.0f, 200.0f, 14.0f }, 11.5f, theme::textDim, juce::Justification::centredLeft, true);
            zones.push_back ({ row, e.name + ": chest " + sg::getNoteName (e.lowest) + " to " + sg::getNoteName (e.chestTop) + ", middle to " + sg::getNoteName (e.headBottom)
                                     + ", head to " + sg::getNoteName (e.highest) });
            y += rowH;
        }

        paintKeyboard (g, kb, [&] (int n)
        {
            for (auto& e : examples)
                if (n == e.firstBreak || n == e.secondBreak)
                    return theme::text.withAlpha (0.9f);

            return juce::Colour();
        });
    }

    //==============================================================================
    /** A plot of cents against time: a line per example in its own panel. */
    void paintCurves (juce::Graphics& g, juce::Rectangle<float> area, int count, std::function<juce::String (int)> heading, std::function<juce::String (int)> caption,
                      std::function<double (int, double)> centsAt, double seconds, double extent, bool tolerance)
    {
        const float gap = 14.0f, w = (area.getWidth() - gap * (float) (count - 1)) / (float) count;

        for (int i = 0; i < count; ++i)
        {
            auto panel = juce::Rectangle<float> (area.getX() + (float) i * (w + gap), area.getY(), w, area.getHeight());
            drawText (g, heading (i), panel.removeFromTop (18.0f), 13.0f, theme::text, juce::Justification::centredLeft, true);
            drawText (g, caption (i), panel.removeFromBottom (32.0f), 11.5f, theme::textDim, juce::Justification::topLeft, false, 2);
            panel.removeFromBottom (4.0f);
            g.setColour (theme::lane);
            g.fillRect (panel);
            auto yOf = [&] (double cents) { return panel.getCentreY() - (float) (cents / extent) * (panel.getHeight() * 0.5f - 4.0f); };

            g.setColour (theme::gridBar);
            g.fillRect (panel.getX(), yOf (0.0), panel.getWidth(), 1.5f);
            drawText (g, "target", { panel.getX() + 4.0f, yOf (0.0) - 15.0f, 60.0f, 13.0f }, 10.5f, theme::textDim);

            if (tolerance)
            {
                g.setColour (theme::meterYellow.withAlpha (0.6f));

                for (double c : { sg::tuneToleranceCents, -sg::tuneToleranceCents })
                    for (float x = panel.getX(); x < panel.getRight(); x += 8.0f)
                        g.fillRect (x, yOf (c), 4.0f, 1.0f);

                if (i == 0)
                    drawText (g, "about where listeners hear it", { panel.getX() + 4.0f, yOf (sg::tuneToleranceCents) - 14.0f, panel.getWidth() - 8.0f, 13.0f }, 10.5f, theme::meterYellow.withAlpha (0.9f));
            }

            juce::Path curve;

            for (int s = 0; s <= 200; ++s)
            {
                const double t = seconds * (double) s / 200.0;
                const float x = panel.getX() + (float) (t / seconds) * panel.getWidth(), y = yOf (centsAt (i, t));

                if (s == 0)
                    curve.startNewSubPath (x, y);
                else
                    curve.lineTo (x, y);
            }

            g.setColour (theme::accent);
            g.strokePath (curve, juce::PathStrokeType (2.2f));
            drawText (g, "+" + juce::String ((int) extent) + " cents", { panel.getRight() - 90.0f, panel.getY() + 1.0f, 86.0f, 12.0f }, 10.0f, theme::textDim, juce::Justification::centredRight);
            drawText (g, "-" + juce::String ((int) extent) + " cents", { panel.getRight() - 90.0f, panel.getBottom() - 13.0f, 86.0f, 12.0f }, 10.0f, theme::textDim, juce::Justification::centredRight);
            zones.push_back ({ panel, heading (i) + dot() + caption (i) });
        }
    }

    void paintTune (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto curves = sg::getTuneExamples();
        drawText (g, "Three 1.5-second notes on A4: pitch over time, in cents. Click to hear the target.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (6.0f);
        paintCurves (g, area, (int) curves.size(), [&] (int i) { return curves[(size_t) i].name; }, [&] (int i) { return curves[(size_t) i].note; },
                     [&] (int i, double t) { return curves[(size_t) i].getCents (t); }, 1.5, 175.0, true);
    }

    void paintVibrato (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto ex = sg::getVibratoExample();
        drawText (g, "A held note of 2 seconds: its pitch around the note, in cents.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (6.0f);
        paintCurves (g, area, (int) ex.curves.size(), [&] (int i) { return ex.curves[(size_t) i].name; },
                     [&] (int i)
                     {
                         auto& c = ex.curves[(size_t) i];
                         return c.extentCents <= 0.0 ? juce::String ("No swing: plain and close.")
                                                     : juce::String ((int) c.extentCents) + " cents either side, " + juce::String (c.rateHz, 1) + " times a second"
                                                           + (c.startSeconds > 0.0 ? ", from 1 s" : "");
                     },
                     [&] (int i, double t) { return ex.curves[(size_t) i].getCents (t); }, ex.seconds, 80.0, false);
    }

    //==============================================================================
    void paintVowels (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto lines = sg::getVowelExamples();
        drawText (g, "Words on notes of 4 beats, with the sounds of each word under its note: vowels bright, consonants dim.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        const float labelW = 120.0f;
        const double b0 = -0.5, b1 = 4.3;
        auto xOf = [&] (double b) { return area.getX() + labelW + (float) ((b - b0) / (b1 - b0)) * (area.getWidth() - labelW); };
        const float rowH = (area.getHeight() - 24.0f) / (float) lines.size();
        float y = area.getY() + 24.0f;

        for (int b = 0; b <= 4; ++b)
        {
            g.setColour (b == 0 || b == 4 ? theme::gridBar : theme::grid);
            g.fillRect (xOf ((double) b), y - 4.0f, 1.0f, rowH * (float) lines.size());
            drawText (g, juce::String (b % 4 + 1), { xOf ((double) b) + 3.0f, y - 22.0f, 20.0f, 14.0f }, 10.5f, theme::textDim);
        }

        const auto vowelColour = theme::meterGreen.withAlpha (0.8f);

        for (auto& line : lines)
        {
            const auto colour = line.wrong ? theme::meterRed : theme::text;
            drawText (g, line.label, { area.getX(), y, labelW - 8.0f, 18.0f }, 13.0f, colour, juce::Justification::centredLeft, true);
            drawText (g, line.note, { area.getX(), y + 18.0f, labelW - 12.0f, rowH - 20.0f }, 11.0f, theme::textDim, juce::Justification::topLeft, false, 4);

            for (auto& w : line.words)
            {
                const auto note = juce::Rectangle<float> (xOf (w.start) + 1.0f, y + 2.0f, xOf (w.end) - xOf (w.start) - 2.0f, 10.0f);
                g.setColour (line.wrong ? theme::meterRed.withAlpha (0.8f) : theme::accent);
                g.fillRoundedRectangle (note, 2.0f);
                drawText (g, w.text, { note.getX() + 2.0f, y + 12.0f, 80.0f, 18.0f }, 14.0f, colour, juce::Justification::centredLeft, true);
            }

            for (auto& p : line.parts)
            {
                const auto block = juce::Rectangle<float> (xOf (p.start) + 0.5f, y + 34.0f, juce::jmax (4.0f, xOf (p.end) - xOf (p.start) - 1.0f), 24.0f);
                const bool held = line.wrong && ! p.vowel && p.end - p.start > 1.0;
                g.setColour (held ? theme::meterRed.withAlpha (0.85f) : p.vowel ? vowelColour : theme::textDim.withAlpha (0.4f));
                g.fillRoundedRectangle (block, 3.0f);
                drawText (g, p.symbol, block, 12.5f, theme::text, juce::Justification::centred, p.vowel);
            }

            zones.push_back ({ juce::Rectangle<float> (area.getX(), y, area.getWidth(), rowH), line.label + dot() + line.note });
            y += rowH;
        }
    }

    //==============================================================================
    void paintLoud (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto notes = sg::getLoudExample();
        drawText (g, "The thicker a note, the louder it's sung.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (4.0f);
        const auto labels = area.removeFromBottom (36.0f);
        Roll roll { area.withTrimmedLeft (38.0f), 58, 74, 0.0, 13.0 };
        roll.draw (g);

        for (auto& n : notes)
        {
            const float thickness = 5.0f + (n.db + 12.0f) * (roll.getRow() - 4.0f) / 12.0f;
            const auto r = roll.getNote (n.pitch, n.start, n.length, thickness);
            g.setColour (n.db < -6.0f ? theme::accent.withAlpha (0.6f) : theme::accent);
            g.fillRoundedRectangle (r, 3.0f);
            drawText (g, juce::String ((int) n.db) + " dB", { r.getX(), labels.getY(), 56.0f, 14.0f }, 11.0f, theme::textDim);

            if (n.text.isNotEmpty())
                drawText (g, n.text, r.translated (0.0f, -16.0f).withHeight (14.0f), 12.5f, theme::text, juce::Justification::centredLeft, true);
        }

        drawText (g, "Verse: softer, lower", { roll.getX (0.0), labels.getY() + 16.0f, 200.0f, 16.0f }, 12.0f, theme::text, juce::Justification::centredLeft, true);
        drawText (g, "Chorus: higher, and 9 to 12 dB louder", { roll.getX (4.0), labels.getY() + 16.0f, 320.0f, 16.0f }, 12.0f, theme::meterYellow, juce::Justification::centredLeft, true);
    }

    void paintHarmony (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto ex = sg::getHarmonyExample();
        drawText (g, "\"Hold on, hold on tonight\": the lead, a harmony a third above, and a double (faint). Click to hear lead and harmony.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (4.0f);
        const auto labels = area.removeFromBottom (18.0f);
        Roll roll { area.withTrimmedLeft (38.0f), 58, 73, 0.0, 8.0 };
        roll.draw (g);
        const auto harmonyColour = theme::meterGreen;

        for (size_t i = 0; i < ex.lead.size(); ++i)
        {
            auto& l = ex.lead[i];
            auto& h = ex.harmony[i];
            auto d = roll.getNote (l.pitch, l.start + ex.doubleOffsetBeats, l.length).translated (0.0f, 3.0f);
            g.setColour (theme::accent.withAlpha (0.3f));
            g.fillRoundedRectangle (d, 2.0f);
            g.setColour (theme::accent);
            g.fillRoundedRectangle (roll.getNote (l.pitch, l.start, l.length), 2.0f);
            g.setColour (harmonyColour);
            g.fillRoundedRectangle (roll.getNote (h.pitch, h.start, h.length), 2.0f);
            drawText (g, l.text, roll.getNote (l.pitch, l.start, l.length).translated (0.0f, roll.getRow() * 0.8f).withHeight (14.0f), 11.5f, theme::text);
            drawText (g, juce::String (h.pitch - l.pitch), roll.getNote (h.pitch, h.start, h.length).translated (2.0f, -14.0f).withHeight (13.0f), 10.5f, harmonyColour);
        }

        drawText (g, "Lead", { roll.area.getX(), labels.getY(), 80.0f, 16.0f }, 12.0f, theme::accent, juce::Justification::centredLeft, true);
        drawText (g, "Harmony: a third above (3 or 4 semitones)", { roll.area.getX() + 80.0f, labels.getY(), 320.0f, 16.0f }, 12.0f, harmonyColour, juce::Justification::centredLeft, true);
    }

    void paintCare (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto ex = sg::getCareExample();
        const float gap = 20.0f, w = (area.getWidth() - gap) / 2.0f;

        for (int s = 0; s < 2; ++s)
        {
            auto panel = juce::Rectangle<float> (area.getX() + (float) s * (w + gap), area.getY(), w, area.getHeight());
            g.setColour (theme::lane);
            g.fillRoundedRectangle (panel, 5.0f);
            panel = panel.reduced (12.0f, 8.0f);
            const auto colour = s == 0 ? theme::meterGreen : theme::meterRed;
            drawText (g, s == 0 ? "Every day" : "Stop, and see a doctor", panel.removeFromTop (22.0f), 15.0f, colour, juce::Justification::centredLeft, true);
            panel.removeFromTop (4.0f);

            for (auto& line : s == 0 ? ex.everyDay : ex.doctor)
            {
                auto row = panel.removeFromTop (26.0f);
                g.setColour (colour);
                g.fillEllipse (row.getX(), row.getCentreY() - 3.0f, 6.0f, 6.0f);
                drawText (g, line, row.withTrimmedLeft (16.0f), 13.5f, theme::text);
            }
        }
    }

    //==============================================================================
    void paintMicrophone (juce::Graphics& g, juce::Rectangle<float> area)
    {
        drawText (g, "How loud your voice is at the microphone, against 20 cm: every halving of the distance adds 6 dB.", area.removeFromTop (18.0f), 12.5f, theme::textDim);
        area.removeFromTop (4.0f);
        auto diagram = area.removeFromBottom (64.0f);
        area.removeFromBottom (6.0f);
        auto plot = area.withTrimmedLeft (44.0f).withTrimmedBottom (18.0f);
        const double maxCm = 65.0, topDb = 14.0, bottomDb = -12.0;
        auto xOf = [&] (double cm) { return plot.getX() + (float) (cm / maxCm) * plot.getWidth(); };
        auto yOf = [&] (double db) { return plot.getY() + (float) ((topDb - db) / (topDb - bottomDb)) * plot.getHeight(); };

        g.setColour (theme::lane);
        g.fillRect (plot);
        g.setColour (theme::meterGreen.withAlpha (0.18f));
        g.fillRect (xOf (sg::micZoneNear), plot.getY(), xOf (sg::micZoneFar) - xOf (sg::micZoneNear), plot.getHeight());
        drawText (g, "studio vocal 15 to 23 cm", { xOf (sg::micZoneNear) + 6.0f, plot.getBottom() - 18.0f, 180.0f, 14.0f }, 11.0f, theme::meterGreen);
        g.setColour (theme::meterYellow.withAlpha (0.15f));
        g.fillRect (plot.getX(), plot.getY(), xOf (10.0) - plot.getX(), plot.getHeight());
        drawText (g, "under 10 cm: more bass (directional mics)", { plot.getX() + 4.0f, plot.getBottom() - 32.0f, xOf (10.0) - plot.getX() - 8.0f, 30.0f }, 11.0f, theme::meterYellow, juce::Justification::bottomLeft, false, 2);

        for (int db = -10; db <= 10; db += 5)
        {
            g.setColour (db == 0 ? theme::gridBar : theme::grid);
            g.fillRect (plot.getX(), yOf ((double) db), plot.getWidth(), 1.0f);
            drawText (g, (db > 0 ? "+" : "") + juce::String (db) + " dB", { area.getX(), yOf ((double) db) - 7.0f, 40.0f, 14.0f }, 10.5f, theme::textDim, juce::Justification::centredRight);
        }

        juce::Path curve;

        for (int i = 0; i <= 120; ++i)
        {
            const double cm = 4.0 + (maxCm - 4.0) * (double) i / 120.0;
            const float x = xOf (cm), y = yOf (sg::getMicLevelDb (cm));

            if (i == 0)
                curve.startNewSubPath (x, y);
            else
                curve.lineTo (x, y);
        }

        g.setColour (theme::accent);
        g.strokePath (curve, juce::PathStrokeType (2.2f));

        for (double cm : sg::getMicDistances())
        {
            const double db = sg::getMicLevelDb (cm);
            g.setColour (theme::text);
            g.fillEllipse (xOf (cm) - 3.5f, yOf (db) - 3.5f, 7.0f, 7.0f);
            drawText (g, juce::String (cm, 0) + " cm", { xOf (cm) - 24.0f, plot.getBottom() + 2.0f, 48.0f, 14.0f }, 10.5f, theme::textDim, juce::Justification::centred);
            drawText (g, (db > 0.0 ? "+" : "") + juce::String (db, 1), { xOf (cm) - 4.0f, yOf (db) - 18.0f, 50.0f, 13.0f }, 10.5f, theme::text);
            zones.push_back ({ juce::Rectangle<float> (xOf (cm) - 8.0f, yOf (db) - 8.0f, 16.0f, 16.0f), juce::String (cm, 0) + " cm" + dot() + (db > 0.0 ? "+" : "") + juce::String (db, 1) + " dB against 20 cm" });
        }

        // The mouth, the pop filter and the microphone, on the same distance scale (the microphone at 0).
        const float lineY = diagram.getCentreY();
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), lineY, plot.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.fillRoundedRectangle (plot.getX() - 8.0f, lineY - 14.0f, 16.0f, 28.0f, 4.0f);
        drawText (g, "microphone", { plot.getX() - 8.0f, lineY + 16.0f, 90.0f, 14.0f }, 11.0f, theme::textDim);
        g.setColour (theme::meterYellow);
        g.fillRect (xOf (sg::popFilterNear), lineY - 14.0f, xOf (sg::popFilterFar) - xOf (sg::popFilterNear), 28.0f);
        drawText (g, "pop filter, 5 to 8 cm in front", { xOf (sg::popFilterFar) + 6.0f, lineY + 4.0f, 200.0f, 14.0f }, 11.0f, theme::meterYellow);
        g.setColour (theme::accent);
        g.fillEllipse (xOf (20.0) - 7.0f, lineY - 7.0f, 14.0f, 14.0f);
        drawText (g, "your mouth, 20 cm", { xOf (20.0) + 12.0f, lineY - 24.0f, 140.0f, 14.0f }, 11.0f, theme::accent);
    }

    //==============================================================================
    void paintYours (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto kb = getKeyboard();
        area.removeFromBottom (80.0f);
        area.removeFromBottom (6.0f);

        if (! analysis.hasMelody)
        {
            drawText (g, "No melody to check yet: write or record the tune on a MIDI track and choose it above.", area, 14.0f, theme::textDim, juce::Justification::centred, false, 2);
            paintKeyboard (g, kb, [this] (int n) { return range.isSet() && n >= range.lowest && n <= range.highest ? theme::accent.withAlpha (0.55f) : juce::Colour(); });
            return;
        }

        const int rows = (int) analysis.sections.size() + 1;
        const float rowH = juce::jmin (28.0f, (area.getHeight() - 20.0f) / (float) juce::jmax (1, rows));
        float y = area.getY() + 2.0f;

        if (range.isSet())
        {
            const auto bar = juce::Rectangle<float> (kb.getLeft (range.lowest), y, kb.getRight (range.highest) - kb.getLeft (range.lowest), rowH - 4.0f);
            g.setColour (theme::accent.withAlpha (0.9f));
            g.fillRoundedRectangle (bar, 4.0f);
            const float edge = kb.getLeft (range.lowest + 3) - bar.getX(), edgeTop = bar.getRight() - kb.getLeft (range.highest - 2);
            hatch (g, bar.withWidth (edge), theme::background.withAlpha (0.6f));
            hatch (g, bar.withLeft (bar.getRight() - edgeTop), theme::background.withAlpha (0.6f));
            drawText (g, "Your range" + dot() + sg::getNoteName (range.lowest) + " to " + sg::getNoteName (range.highest) + "  (top and bottom 3 semitones hatched)",
                      bar.withTrimmedLeft (edge + 6.0f).withTrimmedRight (edgeTop + 4.0f), 12.0f, theme::background, juce::Justification::centredLeft, true);
            zones.push_back ({ bar, "Your range: " + sg::getNoteName (range.lowest) + " to " + sg::getNoteName (range.highest) + ". The hatched ends are the top and bottom 3 semitones, which are tiring to stay in." });
        }
        else
        {
            drawText (g, "Your range isn't set: pick a voice type or your notes above, or right-click a key.", { area.getX(), y, area.getWidth(), rowH }, 12.5f, theme::textDim);
        }

        y += rowH;

        for (auto& s : analysis.sections)
        {
            const auto bar = juce::Rectangle<float> (kb.getLeft (s.lowest), y + 2.0f, kb.getRight (s.highest) - kb.getLeft (s.lowest), rowH - 4.0f);
            const auto colour = SectionLane::getColour (s.type);
            g.setColour (colour.withAlpha (0.9f));
            g.fillRoundedRectangle (bar, 4.0f);

            juce::String where = " inside your range";

            if (range.isSet())
            {
                // The parts outside the range in bright red, with a light edge so they stand out from a red section colour.
                g.setColour (theme::meterRed);

                if (s.highest > range.highest)
                {
                    g.fillRoundedRectangle (juce::Rectangle<float> (kb.getLeft (range.highest + 1), bar.getY(), bar.getRight() - kb.getLeft (range.highest + 1), bar.getHeight()), 3.0f);
                    where = " " + juce::String (s.highest - range.highest) + (s.highest - range.highest == 1 ? " semitone" : " semitones") + " above your range";
                }

                if (s.lowest < range.lowest)
                {
                    g.fillRoundedRectangle (juce::Rectangle<float> (bar.getX(), bar.getY(), kb.getLeft (range.lowest) - bar.getX(), bar.getHeight()), 3.0f);
                    where = " " + juce::String (range.lowest - s.lowest) + (range.lowest - s.lowest == 1 ? " semitone" : " semitones") + " below your range";
                }
            }
            else
            {
                where = {};
            }

            drawText (g, s.name + dot() + sg::getNoteName (s.lowest) + " to " + sg::getNoteName (s.highest), bar.reduced (6.0f, 0.0f), 12.0f, juce::Colour (0xff15171a), juce::Justification::centredLeft, true);
            zones.push_back ({ bar, s.name + dot() + sg::getNoteName (s.lowest) + " to " + sg::getNoteName (s.highest) + (where.isNotEmpty() ? dot() + where.trim() : juce::String()) });
            y += rowH;
        }

        paintKeyboard (g, kb, [this] (int n) { return range.isSet() && n >= range.lowest && n <= range.highest ? theme::accent.withAlpha (isBlackKey (n) ? 0.7f : 0.5f) : juce::Colour(); });
    }

    sg::Topic topic = sg::Topic::voice;
    sg::Range range;
    sg::Analysis analysis;
    int litNote = -1;
    std::vector<std::pair<juce::Rectangle<float>, juce::String>> zones;
};

//==============================================================================
SingingExplainer::SingingExplainer (Engine& e, std::function<Track*()> selected) : engine (e), getSelectedTrack (std::move (selected))
{
    title.setText ("Singing techniques", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    picture->onPlayNote = [this] (int pitch, int ms) { engine.previewNote (pitch, 0.8f, ms); };
    picture->onSetRange = [this] (bool lowest, int note) { lowest ? setLowest (note) : setHighest (note); };
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

    // The voice row.
    for (auto* l : { &voiceLabel, &lowLabel, &highLabel, &melodyLabel })
    {
        l->setFont (juce::FontOptions (13.0f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, theme::textDim);
        addChildComponent (*l);
    }

    typeBox.addItem ("Not set", 1);

    for (int i = 0; i < (int) sg::getVoiceTypes().size(); ++i)
        typeBox.addItem (sg::getVoiceTypes()[(size_t) i].name, i + 2);

    typeBox.addItem ("Custom", 8);

    for (int n = sg::lowestChoice; n <= sg::highestChoice; ++n)
    {
        lowBox.addItem (sg::getNoteName (n), n);
        highBox.addItem (sg::getNoteName (n), n);
    }

    typeBox.setTooltip ("Your voice type: fills in its usual range. Set your own notes if they differ.");
    lowBox.setTooltip ("Your lowest note: the lowest one you can sing with a good tone. Find it by ear on the keyboard in the picture.");
    highBox.setTooltip ("Your highest note: the highest one you can sing without strain. The highest stays at least 5 semitones above the lowest.");
    melodyBox.setTooltip ("The MIDI track whose melody is checked against your range. It starts on the selected track, or the vocal track.");
    lowBox.setTextWhenNothingSelected ("-");
    highBox.setTextWhenNothingSelected ("-");

    typeBox.onChange = [this]
    {
        const int id = typeBox.getSelectedId();

        if (id == 1)
            setRange ({});
        else if (id >= 2 && id <= 7)
            setRange ({ sg::getVoiceTypes()[(size_t) (id - 2)].lowest, sg::getVoiceTypes()[(size_t) (id - 2)].highest });
        else if (id == 8 && ! getRange().isSet())
            setRange ({ sg::getVoiceTypes()[2].lowest, sg::getVoiceTypes()[2].highest });
        else
            updateVoiceRow();
    };
    lowBox.onChange = [this] { if (lowBox.getSelectedId() > 0) setLowest (lowBox.getSelectedId()); };
    highBox.onChange = [this] { if (highBox.getSelectedId() > 0) setHighest (highBox.getSelectedId()); };
    melodyBox.onChange = [this]
    {
        const int index = melodyBox.getSelectedId() - 1;

        if (juce::isPositiveAndBelow (index, (int) melodyTracks.size()))
        {
            melodyTrack = melodyTracks[(size_t) index];
            melodyChosen = true;
            refreshAnalysis();
            show (shownRow);
        }
    };

    for (auto* c : { &typeBox, &lowBox, &highBox, &melodyBox })
        addChildComponent (*c);

    engine.addChangeListener (this);
    updateMelodyChoices();
    refreshAnalysis();
    updateVoiceRow();

    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
    startTimerHz (15);
}

SingingExplainer::~SingingExplainer()
{
    stopTimer();
    engine.removeChangeListener (this);
}

gen::singing::Range SingingExplainer::getRange() const
{
    auto* settings = engine.properties.getUserSettings();

    if (settings == nullptr || ! settings->containsKey (lowestKey) || ! settings->containsKey (highestKey))
        return {};

    sg::Range r { settings->getIntValue (lowestKey, -1), settings->getIntValue (highestKey, -1) };
    r.lowest = juce::jlimit (sg::lowestChoice, sg::highestChoice, r.lowest);
    r.highest = juce::jlimit (sg::lowestChoice, sg::highestChoice, r.highest);
    return r.isSet() ? r : sg::Range();
}

void SingingExplainer::setRange (sg::Range r)
{
    if (auto* settings = engine.properties.getUserSettings())
    {
        if (r.isSet())
        {
            settings->setValue (lowestKey, r.lowest);
            settings->setValue (highestKey, r.highest);
        }
        else
        {
            settings->removeValue (lowestKey);
            settings->removeValue (highestKey);
        }
    }

    refreshAnalysis();
    updateVoiceRow();
    show (shownRow);
}

void SingingExplainer::setLowest (int note)
{
    note = juce::jlimit (sg::lowestChoice, sg::highestChoice - sg::minSpan, note);
    auto r = getRange();
    r.lowest = note;

    // The other note follows when it's missing or too close: two octaves above, as far as the keyboard goes.
    if (! r.isSet() || r.highest < note + sg::minSpan)
        r.highest = juce::jmin (sg::highestChoice, note + 24);

    setRange (r);
}

void SingingExplainer::setHighest (int note)
{
    note = juce::jlimit (sg::lowestChoice + sg::minSpan, sg::highestChoice, note);
    auto r = getRange();
    r.highest = note;

    if (! r.isSet() || r.lowest > note - sg::minSpan)
        r.lowest = juce::jmax (sg::lowestChoice, note - 24);

    setRange (r);
}

bool SingingExplainer::isVoiceRowShown() const
{
    const auto topic = sg::allTopics[shownRow];
    return topic == sg::Topic::range || topic == sg::Topic::yours;
}

void SingingExplainer::updateVoiceRow()
{
    const auto r = getRange();
    typeBox.setText (sg::getRangeLabel (r), juce::dontSendNotification);

    if (r.isSet())
    {
        lowBox.setSelectedId (r.lowest, juce::dontSendNotification);
        highBox.setSelectedId (r.highest, juce::dontSendNotification);
    }
    else
    {
        lowBox.setSelectedId (0, juce::dontSendNotification);
        highBox.setSelectedId (0, juce::dontSendNotification);
    }

    const bool shown = isVoiceRowShown(), withMelody = shown && sg::allTopics[shownRow] == sg::Topic::yours;

    for (auto* c : std::initializer_list<juce::Component*> { &voiceLabel, &typeBox, &lowLabel, &lowBox, &highLabel, &highBox })
        c->setVisible (shown);

    melodyLabel.setVisible (withMelody);
    melodyBox.setVisible (withMelody);
    resized();
}

void SingingExplainer::updateMelodyChoices()
{
    // The MIDI tracks, and the one to start on.
    std::vector<const Track*> tracks;
    std::vector<sg::TrackInfo> infos;

    {
        const juce::ScopedLock sl (engine.getLock());

        for (auto& t : engine.getTracks())
        {
            if (! t->isMidi())
                continue;

            bool hasNotes = false;

            for (auto& c : t->clips)
                if (c->kind == Clip::Kind::midi && ! static_cast<MidiClip&> (*c).notes.empty())
                    hasNotes = true;

            tracks.push_back (t.get());
            infos.push_back ({ t->name, hasNotes, t->usesBuiltInSynth() && t->builtInSound.load() == BuiltInSound::drums });
        }
    }

    if (std::find (tracks.begin(), tracks.end(), melodyTrack) == tracks.end())
    {
        melodyTrack = nullptr;
        melodyChosen = false;
    }

    if (melodyTrack == nullptr || ! melodyChosen)
    {
        const auto* selected = getSelectedTrack != nullptr ? getSelectedTrack() : nullptr;
        const auto it = std::find (tracks.begin(), tracks.end(), selected);
        const int index = sg::chooseMelodyTrack (infos, it == tracks.end() ? -1 : (int) (it - tracks.begin()));
        melodyTrack = index >= 0 ? tracks[(size_t) index] : nullptr;
    }

    melodyTracks = tracks;
    melodyBox.clear (juce::dontSendNotification);

    for (size_t i = 0; i < tracks.size(); ++i)
        melodyBox.addItem (tracks[i]->name, (int) i + 1);

    const auto it = std::find (tracks.begin(), tracks.end(), melodyTrack);
    melodyBox.setSelectedId (it == tracks.end() ? 0 : (int) (it - tracks.begin()) + 1, juce::dontSendNotification);
}

void SingingExplainer::refreshAnalysis()
{
    std::vector<gen::singthis::SourceNote> source;
    TempoMap tempo;
    juce::String name;

    {
        const juce::ScopedLock sl (engine.getLock());
        tempo = engine.getTempoMap();

        if (melodyTrack != nullptr)
        {
            name = melodyTrack->name;

            for (auto& c : melodyTrack->clips)
                if (c->kind == Clip::Kind::midi)
                    for (auto& n : static_cast<MidiClip&> (*c).notes)
                        source.push_back ({ n.pitch, c->startBeat + n.start, n.length, n.velocity });
        }
    }

    const auto melody = gen::singthis::buildMelody (std::move (source), tempo);
    melodyNotes.clear();

    for (auto& n : melody.notes)
        melodyNotes.push_back ({ n.pitch, n.startBeat, n.endBeat });

    analysis = sg::analyse (melody, name, engine.getSections(), engine.getKey(), engine.getLyrics(), engine.getTempo(), getRange());
}

int SingingExplainer::findLitNote() const
{
    if (! engine.isPlaying())
        return -1;

    const double beat = engine.getPositionBeats();

    for (auto& n : melodyNotes)
        if (beat >= n.start && beat < n.end)
            return n.pitch;

    return -1;
}

void SingingExplainer::timerCallback()
{
    // The key of the melody note at the playhead lights up on Your voice and your song.
    if (sg::allTopics[shownRow] == sg::Topic::yours && isShowing())
        picture->setLitNote (findLitNote());
}

juce::String SingingExplainer::getPrintSource() const
{
    const auto r = getRange();
    return r.isSet() ? "a range of " + sg::getNoteName (r.lowest) + " to " + sg::getNoteName (r.highest) + " and the song's melody"
                     : juce::String ("the song's melody, with no range set");
}

int SingingExplainer::getNumPrintTopics() const
{
    return (int) std::size (sg::allTopics);
}

booklet::Topic SingingExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    printHeight = picture->getPreferredHeight();   // all of it, not what fits the window
    return { sg::getTopicName (sg::allTopics[index]), sg::getTopicSummary (sg::allTopics[index]), explanation.getText(),
             { picture->getWidth(), printHeight }, {} };
}

juce::Image SingingExplainer::paintPrintPicture (float scale)
{
    picture->setSize (picture->getWidth(), printHeight);
    auto image = guideprint::snapshot (*picture, scale);
    resized();
    return image;
}

void SingingExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void SingingExplainer::showTopic (sg::Topic topic)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (sg::allTopics[i] == topic)
            list.selectRow (i);
}

int SingingExplainer::getNumRows()
{
    return (int) std::size (sg::allTopics);
}

void SingingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = sg::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (sg::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (sg::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void SingingExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        show (row);
}

void SingingExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // The tracks, notes, sections, key and lyrics change what Your voice and your song says.
    updateMelodyChoices();
    refreshAnalysis();

    if (sg::allTopics[shownRow] == sg::Topic::yours)
        show (shownRow);
}

void SingingExplainer::show (int index)
{
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    const auto topic = sg::allTopics[shownRow];
    const auto range = getRange();

    sg::Context context;
    context.range = range;
    context.analysis = analysis;
    context.key = engine.getKey();
    context.bpm = engine.getTempo();
    context.lyrics = engine.getLyrics();

    picture->setState (topic, range, analysis, topic == sg::Topic::yours ? findLitNote() : -1);
    explanation.setText (sg::explain (topic, context), false);
    explanation.moveCaretToTop (false);
    updateVoiceRow();
}

void SingingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void SingingExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    if (voiceLabel.isVisible())
    {
        auto row = r.removeFromTop (30);
        r.removeFromTop (8);
        voiceLabel.setBounds (row.removeFromLeft (84));
        typeBox.setBounds (row.removeFromLeft (200).reduced (0, 2));
        row.removeFromLeft (10);
        lowLabel.setBounds (row.removeFromLeft (52));
        lowBox.setBounds (row.removeFromLeft (66).reduced (0, 2));
        row.removeFromLeft (10);
        highLabel.setBounds (row.removeFromLeft (58));
        highBox.setBounds (row.removeFromLeft (66).reduced (0, 2));

        if (melodyLabel.isVisible())
        {
            row.removeFromLeft (10);
            melodyLabel.setBounds (row.removeFromLeft (58));
            melodyBox.setBounds (row.reduced (0, 2));
        }
    }

    picture->setBounds (r.removeFromTop (juce::jmin (picture->getPreferredHeight(), r.getHeight() / 2 + 60)));
    r.removeFromTop (10);
    explanation.setBounds (r);
}
} // namespace daw
