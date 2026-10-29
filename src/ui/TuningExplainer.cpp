#include "TuningExplainer.h"
#include "TunePictures.h"
#include "TunerWindow.h"
#include "engine/SoundGuide.h"

namespace daw
{
namespace tg = gen::tuning;

namespace
{
constexpr double pi = juce::MathConstants<double>::pi;
constexpr int toneMs = 3000;

void drawLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, float size, juce::Colour colour,
                juce::Justification just = juce::Justification::centred, bool bold = false)
{
    g.setColour (colour);
    g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText (text, r.toNearestInt(), just, 2, 0.8f);
}

void drawArrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, juce::Colour colour)
{
    const auto angle = (float) std::atan2 (to.y - from.y, to.x - from.x);
    g.setColour (colour);
    g.drawLine (juce::Line<float> (from, to), 1.8f);
    juce::Path head;
    head.addTriangle (to, to + juce::Point<float> (-8.0f, -5.0f).rotatedAboutOrigin (angle),
                      to + juce::Point<float> (-8.0f, 5.0f).rotatedAboutOrigin (angle));
    g.fillPath (head);
}

juce::String describeHz (double hz)
{
    const auto text = juce::String (hz, 1);
    return text.endsWith (".0") ? text.dropLastCharacters (2) : text;
}

juce::String signedCents (double cents)
{
    return (cents > 0.0 ? "+" : "") + juce::String (juce::roundToInt (cents));
}
} // namespace

//==============================================================================
/** The topic's picture, drawn from the example data, the tuning and reference chosen in the Tuner, and the song's key. */
class TuningExplainer::Picture final : public juce::Component,
                                       public juce::SettableTooltipClient
{
public:
    struct State
    {
        tg::Topic topic = tg::Topic::whatItShows;
        tg::Tuning tuning = tg::Tuning::guitarStandard;
        double referenceHz = 440.0;
        MusicalKey key;
        juce::String keyName;
        std::vector<tg::LastReading> last;
    };

    /** Called when a string is clicked in the tunings picture. */
    std::function<void (tg::Tuning, int)> onPlayString;

    void setState (const State& s)
    {
        state = s;
        setMouseCursor (state.topic == tg::Topic::tunings ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        setTooltip (state.topic == tg::Topic::tunings ? "The strings of the chosen tuning. Click a string to hear its note."
                                                      : "The picture of the topic. The tuning, reference pitch and key come from the Tuner and your song.");
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        stringRows.clear();

        switch (state.topic)
        {
            case tg::Topic::whatItShows:    paintTuner (g); break;
            case tg::Topic::centsAndBeats:  paintBeats (g); break;
            case tg::Topic::tunings:        paintTunings (g); break;
            case tg::Topic::reference:      paintReference (g); break;
            case tg::Topic::howToTune:      paintPath (g); break;
            case tg::Topic::drift:          paintDrift (g); break;
            case tg::Topic::byEar:          paintNeck (g); break;
            case tg::Topic::harmonics:      paintHarmonics (g); break;
            case tg::Topic::bassAndUkulele: paintWindows (g); break;
            case tg::Topic::yourTuning:     paintSong (g); break;
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (state.topic != tg::Topic::tunings || ! onPlayString)
            return;

        for (size_t i = 0; i < stringRows.size(); ++i)
            if (stringRows[i].contains (e.position))
            {
                onPlayString (state.tuning, (int) i);
                return;
            }
    }

private:
    juce::Rectangle<float> getArea() const { return getLocalBounds().toFloat().reduced (16.0f, 12.0f); }

    void paintTuner (juce::Graphics& g)
    {
        const auto reading = tg::getReading (tg::exampleHz, 440.0, tg::Tuning::guitarStandard);
        tunepics::TunerPicture p;
        p.has = true;
        p.note = reading.note;
        p.cents = reading.cents;
        p.greenCents = tg::inTuneCents;
        p.yellowCents = tg::closeCents;
        p.line = reading.text;
        p.extraLine = (state.key.contains (reading.midi) ? "in " : "not in ") + state.keyName;
        p.maxNoteSize = 60.0f;
        tunepics::paintTuner (g, getArea().withTrimmedBottom (22.0f), p);
        drawLabel (g, getArea().removeFromBottom (20.0f), "An example: the A string, 12 cents flat (109.24 Hz).", 12.0f, theme::textDim);
    }

    void paintBeats (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "440 Hz plus 442 Hz, over 1.5 seconds: the sound swells twice a second", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);
        auto plot = area.withTrimmedBottom (40.0f).withTrimmedLeft (30.0f).withTrimmedRight (10.0f);
        const float mid = plot.getCentreY(), half = plot.getHeight() * 0.5f - 2.0f;

        // The swell: loud where the two waves add up, silent where they cancel.
        juce::Path envelope;
        const int n = juce::jmax (2, (int) plot.getWidth());

        for (int i = 0; i < n; ++i)
        {
            const double t = 1.5 * i / (n - 1);
            const float y = mid - (float) (tg::getBeatEnvelope (t) / 2.0) * half;
            i == 0 ? envelope.startNewSubPath (plot.getX() + (float) i, y) : envelope.lineTo (plot.getX() + (float) i, y);
        }

        for (int i = n - 1; i >= 0; --i)
        {
            const double t = 1.5 * i / (n - 1);
            envelope.lineTo (plot.getX() + (float) i, mid + (float) (tg::getBeatEnvelope (t) / 2.0) * half);
        }

        envelope.closeSubPath();
        g.setColour (theme::accent.withAlpha (0.28f));
        g.fillPath (envelope);

        // The wave itself, thinned to a visible handful of cycles (the real one has 660).
        juce::Path wave;

        for (int i = 0; i < n; ++i)
        {
            const double t = 1.5 * i / (n - 1);
            const float y = mid - (float) (tg::getBeatEnvelope (t) / 2.0 * std::cos (2.0 * pi * 40.0 * t)) * half;
            i == 0 ? wave.startNewSubPath (plot.getX() + (float) i, y) : wave.lineTo (plot.getX() + (float) i, y);
        }

        g.setColour (theme::accent);
        g.strokePath (wave, juce::PathStrokeType (1.2f));
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), mid, plot.getWidth(), 1.0f);

        for (int k = 0; k <= 3; ++k)
        {
            const float x = plot.getX() + plot.getWidth() * (float) (k * 0.5 / 1.5);
            g.setColour (theme::meterGreen.withAlpha (0.8f));
            g.fillRect (x - 0.5f, plot.getY(), 1.0f, plot.getHeight());
            drawLabel (g, { x - 40.0f, plot.getBottom() + 2.0f, 80.0f, 16.0f }, juce::String (k * 0.5, 1) + " s", 11.5f, theme::textDim);
        }

        drawLabel (g, area.removeFromBottom (20.0f), "2 beats a second (drawn with fewer wiggles than the real 440 a second, so you can see the swells)", 12.5f, theme::text);
    }

    void paintTunings (juce::Graphics& g)
    {
        auto area = getArea();
        const auto strings = tg::getStrings (state.tuning);
        drawLabel (g, area.removeFromTop (22.0f), tg::getTuningName (state.tuning) + ": click a string to hear it", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);

        if (strings.empty())
        {
            drawLabel (g, area, "Chromatic has no strings: the tuner names any note, for any instrument.\nChoose another tuning in the Tuner to see its strings here.", 14.0f, theme::textDim);
            return;
        }

        const auto standard = tg::getStrings (tg::Tuning::guitarStandard);
        const bool guitar = tg::isFrettedTuning (state.tuning) && state.tuning != tg::Tuning::ukulele;
        const float rowHeight = juce::jmin (44.0f, (area.getHeight() - 24.0f) / (float) strings.size());
        const float lineLeft = area.getX() + 150.0f, lineRight = area.getRight() - 170.0f;

        for (size_t i = 0; i < strings.size(); ++i)
        {
            const auto row = juce::Rectangle<float> (area.getX(), area.getY() + (float) i * rowHeight, area.getWidth(), rowHeight);
            stringRows.push_back (row);
            const float y = row.getCentreY();
            const float thickness = juce::jmax (1.0f, 4.5f - 0.6f * (float) i);
            g.setColour (theme::textDim.withAlpha (0.9f));
            g.fillRect (lineLeft, y - thickness * 0.5f, lineRight - lineLeft, thickness);

            const auto& s = strings[i];
            const double hz = tg::getHz ((double) s.midi, state.referenceHz);
            drawLabel (g, { area.getX(), row.getY(), 140.0f, rowHeight }, s.note + "  " + s.label, 13.5f, theme::text, juce::Justification::centredLeft, true);
            drawLabel (g, { lineRight + 10.0f, row.getY(), 70.0f, rowHeight }, describeHz (hz) + " Hz", 12.5f, theme::text, juce::Justification::centredLeft);

            if (guitar && i < standard.size() && s.midi != standard[i].midi)
                drawLabel (g, { lineRight + 80.0f, row.getY(), 90.0f, rowHeight }, signedCents (100.0 * (s.midi - standard[i].midi)) + " cents", 12.0f, theme::meterYellow, juce::Justification::centredLeft, true);
        }

        drawLabel (g, area.removeFromBottom (20.0f), guitar ? "Marked in yellow: the strings that differ from standard tuning."
                                                          : "The strings in the order you see them on the instrument, the top one first.",
                   12.0f, theme::textDim, juce::Justification::centredLeft);
    }

    void paintReference (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "A4 from 415 to 466 Hz: where the usual references sit", 14.0f, theme::text, juce::Justification::centredLeft, true);
        const auto scale = area.withTrimmedLeft (20.0f).withTrimmedRight (20.0f).withTrimmedTop (80.0f).withHeight (4.0f);
        auto xOf = [&] (double hz) { return scale.getX() + scale.getWidth() * (float) ((hz - 415.0) / (466.0 - 415.0)); };
        g.setColour (theme::textDim);
        g.fillRoundedRectangle (scale, 2.0f);

        struct Mark { double hz; const char* label; };
        const Mark marks[] { { 415.0, "415\nbaroque" }, { 432.0, "432" }, { 440.0, "440\nstandard" }, { 442.0, "442" }, { 443.0, "443" } };
        int index = 0;

        for (auto& m : marks)
        {
            const float x = xOf (m.hz);
            g.setColour (theme::text);
            g.fillRect (x - 1.0f, scale.getY() - 10.0f, 2.0f, 24.0f);
            const bool above = (index++ % 2) == 0;
            drawLabel (g, { x - 45.0f, above ? scale.getY() - 52.0f : scale.getY() + 18.0f, 90.0f, 36.0f }, m.label, 12.5f, theme::text);
        }

        const float x = xOf (state.referenceHz);
        g.setColour (theme::accent);
        g.fillEllipse (x - 8.0f, scale.getCentreY() - 8.0f, 16.0f, 16.0f);
        const double cents = tg::getReferenceCents (state.referenceHz);
        drawLabel (g, area.removeFromBottom (60.0f),
                   "Your tuner is set to A4 = " + describeHz (state.referenceHz) + " Hz"
                       + (std::abs (cents) < 0.05 ? ", the standard." : ": " + juce::String (std::abs (cents), 1) + " cents " + (cents > 0.0 ? "above" : "below") + " 440 Hz."),
                   14.0f, theme::accent, juce::Justification::centred, true);
    }

    /** A plot's frame: time along the bottom, cents up the side, a zero line and the green band. */
    juce::Rectangle<float> paintCentsFrame (juce::Graphics& g, juce::Rectangle<float> area, double seconds, double maxCents)
    {
        auto plot = area.withTrimmedLeft (46.0f).withTrimmedRight (10.0f).withTrimmedBottom (34.0f);
        const float mid = plot.getCentreY();
        const float scaleY = plot.getHeight() * 0.5f / (float) maxCents;
        g.setColour (theme::meterGreen.withAlpha (0.18f));
        g.fillRect (plot.getX(), mid - (float) tg::inTuneCents * scaleY, plot.getWidth(), 2.0f * (float) tg::inTuneCents * scaleY);
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), mid, plot.getWidth(), 1.0f);

        for (const int c : { -(int) maxCents, 0, (int) maxCents })
            drawLabel (g, { area.getX(), mid - (float) c * scaleY - 8.0f, 42.0f, 16.0f }, signedCents (c) + " c", 11.5f, theme::textDim, juce::Justification::centredRight);

        for (int s = 0; s <= (int) seconds; ++s)
            drawLabel (g, { plot.getX() + plot.getWidth() * (float) s / (float) seconds - 20.0f, plot.getBottom() + 2.0f, 40.0f, 16.0f }, juce::String (s) + " s", 11.5f, theme::textDim);

        return plot;
    }

    void paintPath (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "One string, tuned: loosened past the note, then brought up to it", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (4.0f);
        const auto plot = paintCentsFrame (g, area, 6.0, 40.0);
        const float mid = plot.getCentreY(), scaleY = plot.getHeight() * 0.5f / 40.0f;
        auto at = [&] (double t) { return juce::Point<float> (plot.getX() + plot.getWidth() * (float) (t / 6.0), mid - (float) tg::getTuningPath (t) * scaleY); };

        auto stroke = [&] (double from, double to, juce::Colour colour)
        {
            juce::Path p;

            for (double t = from; t <= to + 1.0e-9; t += 0.02)
                t <= from + 1.0e-9 ? p.startNewSubPath (at (t)) : p.lineTo (at (t));

            g.setColour (colour);
            g.strokePath (p, juce::PathStrokeType (2.6f));
        };

        stroke (0.0, 3.5, theme::meterYellow);
        stroke (3.5, 6.0, theme::meterGreen);
        drawLabel (g, { at (0.2).x, at (0.2).y - 30.0f, 200.0f, 18.0f }, "starts 30 cents sharp", 12.0f, theme::meterYellow, juce::Justification::centredLeft);
        drawLabel (g, { at (2.5).x - 60.0f, at (2.5).y + 8.0f, 200.0f, 18.0f }, "loosened to 20 flat", 12.0f, theme::meterYellow, juce::Justification::centredLeft);
        drawArrow (g, { at (3.7).x, at (3.7).y - 22.0f }, { at (4.9).x, at (4.9).y - 12.0f }, theme::meterGreen);
        drawLabel (g, { at (3.6).x, at (3.6).y - 52.0f, 200.0f, 18.0f }, "up to the note", 12.5f, theme::meterGreen, juce::Justification::centredLeft, true);
    }

    void paintDrift (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "A hard pluck: sharp at first, gliding down to the note", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (4.0f);
        const auto plot = paintCentsFrame (g, area.withTrimmedBottom (20.0f), 2.0, 10.0);
        const float mid = plot.getCentreY(), scaleY = plot.getHeight() * 0.5f / 10.0f;
        juce::Path p;
        const int n = juce::jmax (2, (int) plot.getWidth());

        for (int i = 0; i < n; ++i)
        {
            const double t = 2.0 * i / (n - 1);
            const juce::Point<float> pt (plot.getX() + (float) i, mid - (float) tg::getDriftGlide (t) * scaleY);
            i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
        }

        g.setColour (theme::accent);
        g.strokePath (p, juce::PathStrokeType (2.6f));
        drawLabel (g, area.removeFromBottom (20.0f), "drawn to show the shape, not measured", 12.5f, theme::textDim);
    }

    void paintNeck (juce::Graphics& g)
    {
        auto area = getArea();
        const auto strings = tg::getStrings (tg::Tuning::guitarStandard);
        const auto steps = tg::getFifthFretSteps();
        drawLabel (g, area.removeFromTop (22.0f), "Press each string where the ring is, and match the next open string to it", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);
        const float labelW = 44.0f, left = area.getX() + labelW, right = area.getRight() - 120.0f;
        const int frets = 6;
        const float top = area.getY() + 14.0f, rowH = (area.getHeight() - 60.0f) / 5.0f;
        auto xOfFret = [&] (double f) { return left + (right - left) * (float) (f / frets); };
        auto yOfString = [&] (int s) { return top + (float) (5 - s) * rowH; };   // the low E at the bottom

        for (int f = 0; f <= frets; ++f)
        {
            g.setColour (f == 0 ? theme::text : theme::textDim);
            g.fillRect (xOfFret (f) - (f == 0 ? 2.0f : 0.5f), top, f == 0 ? 4.0f : 1.0f, rowH * 5.0f);
            drawLabel (g, { xOfFret (f) - 15.0f, top - 16.0f, 30.0f, 14.0f }, f == 0 ? "" : juce::String (f), 11.0f, theme::textDim);
        }

        for (int s = 0; s < 6; ++s)
        {
            g.setColour (theme::textDim);
            g.fillRect (left, yOfString (s) - 0.5f * (3.0f - 0.4f * (float) s), right - left, 3.0f - 0.4f * (float) s);
            drawLabel (g, { area.getX(), yOfString (s) - 9.0f, labelW - 6.0f, 18.0f }, strings[(size_t) s].note, 12.5f, theme::text, juce::Justification::centredRight, true);
        }

        for (int s = 0; s < 5; ++s)
        {
            const double fret = steps[(size_t) s];
            const juce::Point<float> dot (xOfFret (fret - 0.5), yOfString (s));
            const juce::Point<float> next (left - 3.0f, yOfString (s + 1));
            g.setColour (theme::accent);
            g.drawEllipse (dot.x - 9.0f, dot.y - 9.0f, 18.0f, 18.0f, 2.2f);
            drawArrow (g, { dot.x - 10.0f, dot.y - 4.0f }, { left + 4.0f, next.y + 6.0f }, theme::accent.withAlpha (0.55f));
            drawLabel (g, { right + 10.0f, yOfString (s) - 9.0f, 110.0f, 18.0f },
                       "fret " + juce::String (steps[(size_t) s]) + " = " + strings[(size_t) s + 1].note, 12.5f, theme::text, juce::Justification::centredLeft);
        }
    }

    void paintHarmonics (juce::Graphics& g)
    {
        auto area = getArea();
        const auto hz = tg::getHarmonicHz (110.0);
        drawLabel (g, area.removeFromTop (22.0f), "The A string (110 Hz) held still at one point: it can only vibrate in equal parts", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);
        const int parts[] { 2, 3, 4 };
        const char* frets[] { "12th fret", "7th fret", "5th fret" };
        const float rowH = area.getHeight() / 3.0f, left = area.getX() + 20.0f, right = area.getRight() - 160.0f;

        for (int r = 0; r < 3; ++r)
        {
            const float mid = area.getY() + rowH * ((float) r + 0.5f), amp = rowH * 0.34f;
            juce::Path p;
            const int n = juce::jmax (2, (int) (right - left));

            for (int i = 0; i < n; ++i)
            {
                const double u = (double) i / (n - 1);
                const juce::Point<float> pt (left + (float) i, mid - amp * (float) std::sin (pi * parts[r] * u));
                i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
            }

            g.setColour (theme::accent);
            g.strokePath (p, juce::PathStrokeType (2.0f));
            g.setColour (theme::textDim);
            g.fillRect (left, mid - 0.5f, right - left, 1.0f);

            for (int k = 0; k <= parts[r]; ++k)
            {
                const float x = left + (right - left) * (float) k / (float) parts[r];
                g.setColour (theme::meterYellow);
                g.fillEllipse (x - 4.0f, mid - 4.0f, 8.0f, 8.0f);
            }

            drawLabel (g, { right + 14.0f, mid - 20.0f, 150.0f, 40.0f }, juce::String (frets[r]) + "\n" + describeHz (hz[(size_t) r]) + " Hz", 13.0f, theme::text, juce::Justification::centredLeft, true);
        }
    }

    void paintWindows (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "What the tuner hears in 85 milliseconds", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);
        const double ms = 85.33;
        const double hzs[] { 41.20, 329.63 };
        const char* names[] { "Bass low E (41.2 Hz)", "Guitar high E (329.6 Hz)" };
        const float rowH = area.getHeight() / 2.0f;

        for (int r = 0; r < 2; ++r)
        {
            auto row = juce::Rectangle<float> (area.getX(), area.getY() + rowH * (float) r, area.getWidth(), rowH - 6.0f);
            const double cycles = tg::getCyclesInWindow (hzs[r], ms);
            drawLabel (g, row.removeFromTop (18.0f), juce::String (names[r]) + ": " + juce::String (cycles, 1) + " cycles", 13.0f, theme::text, juce::Justification::centredLeft, true);
            g.setColour (theme::panel);
            g.fillRoundedRectangle (row, 4.0f);
            juce::Path p;
            const int n = juce::jmax (2, (int) row.getWidth());

            for (int i = 0; i < n; ++i)
            {
                const double u = (double) i / (n - 1);
                const juce::Point<float> pt (row.getX() + (float) i, row.getCentreY() - (float) std::sin (2.0 * pi * cycles * u) * (row.getHeight() * 0.4f));
                i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
            }

            g.setColour (r == 0 ? theme::meterYellow : theme::accent);
            g.strokePath (p, juce::PathStrokeType (1.6f));
        }
    }

    void paintSong (juce::Graphics& g)
    {
        auto area = getArea();
        const auto strings = tg::getStrings (state.tuning);
        drawLabel (g, area.removeFromTop (22.0f), tg::getTuningName (state.tuning) + " in " + state.keyName + ": green strings are in the key", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);

        if (strings.empty())
        {
            drawLabel (g, area, "Chromatic has no strings. Choose a tuning in the Tuner to compare it with your song.", 14.0f, theme::textDim);
            return;
        }

        const auto fit = tg::getKeyFit (state.tuning, state.key);
        const float boxW = juce::jmin (110.0f, (area.getWidth() - 10.0f * (float) strings.size()) / (float) strings.size());
        const float gap = 10.0f, total = (boxW + gap) * (float) strings.size() - gap;
        float x = area.getCentreX() - total * 0.5f;

        for (size_t i = 0; i < strings.size(); ++i)
        {
            const bool in = std::find (fit.inKey.begin(), fit.inKey.end(), (int) i) != fit.inKey.end();
            const auto box = juce::Rectangle<float> (x, area.getY() + 10.0f, boxW, 96.0f);
            g.setColour (in ? theme::meterGreen.withAlpha (0.25f) : theme::panel);
            g.fillRoundedRectangle (box, 6.0f);
            g.setColour (in ? theme::meterGreen : theme::textDim);
            g.drawRoundedRectangle (box.reduced (0.5f), 6.0f, 1.6f);
            drawLabel (g, box.withHeight (34.0f).translated (0.0f, 6.0f), strings[i].note, 18.0f, theme::text, juce::Justification::centred, true);
            drawLabel (g, box.withHeight (18.0f).translated (0.0f, 40.0f), strings[i].label, 11.5f, theme::textDim);

            juce::String last = "not heard";

            for (auto& l : state.last)
                if (l.stringIndex == (int) i)
                    last = tg::describeCentsShort (l.cents);

            drawLabel (g, box.withTrimmedTop (62.0f).reduced (4.0f, 2.0f), last, 11.5f, theme::text);
            x += boxW + gap;
        }

        area.removeFromTop (124.0f);
        const int best = tg::getBestCapoFret (state.tuning, state.key);
        const int m = (int) strings.size(), n = (int) fit.inKey.size();

        if (tg::isFrettedTuning (state.tuning) && n <= m / 2 && best > 0 && (int) tg::getKeyFit (state.tuning, state.key, best).inKey.size() >= m - 1)
            drawLabel (g, area.removeFromTop (22.0f), "A capo on fret " + juce::String (best) + " puts " + juce::String (tg::getKeyFit (state.tuning, state.key, best).inKey.size())
                                                          + " of the " + juce::String (m) + " open strings in " + state.keyName + ".",
                       13.5f, theme::meterYellow, juce::Justification::centredLeft, true);
        else
            drawLabel (g, area.removeFromTop (22.0f), "The last reading of each string comes from the Tuner (View > Tuner...).", 12.5f, theme::textDim, juce::Justification::centredLeft);
    }

    State state;
    std::vector<juce::Rectangle<float>> stringRows;
};

//==============================================================================
TuningExplainer::TuningExplainer (Engine& e) : engine (e)
{
    title.setText ("How to tune a guitar", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    picture->onPlayString = [this] (tg::Tuning t, int i) { playString (t, i); };
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
    list.selectRow (0);
    setSize (width, height);
    startTimerHz (10);
}

TuningExplainer::~TuningExplainer()
{
    stopTone();
    list.setModel (nullptr);
}

tg::Tuning TuningExplainer::getSavedTuning() const
{
    if (auto* settings = engine.properties.getUserSettings())
        return tg::getTuningFromId (settings->getValue ("tuner.tuning", "guitarStandard"));

    return tg::Tuning::guitarStandard;
}

double TuningExplainer::getSavedReference() const
{
    if (auto* settings = engine.properties.getUserSettings())
        return juce::jlimit (415.0, 466.0, settings->getDoubleValue ("tuner.referenceHz", 440.0));

    return 440.0;
}

void TuningExplainer::playString (tg::Tuning t, int stringIndex)
{
    const auto strings = tg::getStrings (t);

    if (! juce::isPositiveAndBelow (stringIndex, (int) strings.size()))
        return;

    Engine::Tone tone;
    tone.on = true;
    tone.shape = soundguide::Shape::saw;
    tone.frequency = (float) tg::getHz ((double) strings[(size_t) stringIndex].midi, getSavedReference());
    tone.levelDb = -20.0f;
    engine.setTone (tone);
    toneEndsAt = juce::Time::getMillisecondCounter() + (juce::uint32) toneMs;
}

void TuningExplainer::stopTone()
{
    if (toneEndsAt != 0)
    {
        engine.setTone ({});
        toneEndsAt = 0;
    }
}

juce::String TuningExplainer::getPrintSource() const
{
    return "the " + tg::getTuningName (getSavedTuning()) + " tuning";
}

int TuningExplainer::getNumPrintTopics() const
{
    return (int) std::size (tg::allTopics);
}

booklet::Topic TuningExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    return { tg::getTopicName (tg::allTopics[index]), tg::getTopicSummary (tg::allTopics[index]), explanation.getText(),
             { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image TuningExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void TuningExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void TuningExplainer::showTopic (tg::Topic topic)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (tg::allTopics[i] == topic)
            list.selectRow (i);
}

int TuningExplainer::getNumRows()
{
    return (int) std::size (tg::allTopics);
}

void TuningExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = tg::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (tg::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (tg::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void TuningExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        show (row);
}

void TuningExplainer::timerCallback()
{
    if (toneEndsAt != 0 && (int) (juce::Time::getMillisecondCounter() - toneEndsAt) >= 0)
        stopTone();

    refreshPicture();

    // "Your tuning and your song" follows the tuner's settings, the key and the readings.
    if (tg::allTopics[shownRow] == tg::Topic::yourTuning)
        refreshText();
}

void TuningExplainer::refreshPicture()
{
    Picture::State s;
    s.topic = tg::allTopics[shownRow];
    s.tuning = getSavedTuning();
    s.referenceHz = getSavedReference();
    s.key = engine.getKey();
    s.keyName = engine.getKey().getName();
    s.last = getTunerLastReadings (s.tuning);
    picture->setState (s);
}

void TuningExplainer::refreshText()
{
    const auto topic = tg::allTopics[shownRow];
    auto text = tg::explain (topic).replace ("*", "");   // the texts mark bold and italics with * for the plan; the editor shows plain text

    if (topic == tg::Topic::yourTuning)
    {
        std::vector<tg::TrackLow> lows;

        for (auto& track : engine.getTracks())
        {
            if (! track->isMidi())
                continue;

            tg::TrackLow low { track->name, -1 };

            for (auto& clip : track->clips)
                if (clip->kind == Clip::Kind::midi)
                    for (auto& note : static_cast<MidiClip&> (*clip).notes)
                        low.lowestMidi = low.lowestMidi < 0 ? note.pitch : juce::jmin (low.lowestMidi, note.pitch);

            lows.push_back (low);
        }

        const auto t = getSavedTuning();
        const auto advice = tg::analyse (t, getSavedReference(), engine.getKey(), lows, getTunerLastReadings (t));

        text << "\n\nTuning: " << tg::getTuningName (t) << ", A4 = " << describeHz (getSavedReference()) << " Hz (set in the Tuner).";

        for (auto& a : advice)
            text << "\n\n" << a;
    }

    if (text != explanation.getText())
    {
        explanation.setText (text, false);

        if (topic != tg::Topic::yourTuning)
            explanation.moveCaretToTop (false);
    }
}

void TuningExplainer::show (int index)
{
    const bool changed = index != shownRow || explanation.getText().isEmpty();
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    refreshPicture();
    refreshText();

    if (changed)
        explanation.moveCaretToTop (false);
}

void TuningExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void TuningExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    picture->setBounds (r.removeFromTop (pictureHeight));
    r.removeFromTop (10);
    explanation.setBounds (r);
}
} // namespace daw
