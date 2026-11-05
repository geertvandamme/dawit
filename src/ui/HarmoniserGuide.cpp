#include "HarmoniserGuide.h"

#include "SectionLane.h"
#include "Theme.h"
#include "TuneGuide.h"
#include "TunePictures.h"
#include "engine/SungVoice.h"
#include "generate/HarmoniserGuideText.h"

namespace daw::harmtext
{
namespace
{
    constexpr int cMajorMask = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11);

    void caption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (text, r.removeFromTop (22.0f), juce::Justification::centredLeft);
    }

    void legend (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<std::pair<juce::Colour, juce::String>>& items)
    {
        g.setFont (juce::FontOptions (11.5f));
        float x = area.getX();

        for (const auto& [colour, text] : items)
        {
            g.setColour (colour);
            g.fillRect (x, area.getCentreY() - 1.5f, 16.0f, 3.0f);
            g.setColour (theme::textDim);
            g.drawText (text, juce::Rectangle<float> (x + 20.0f, area.getY(), 260.0f, area.getHeight()), juce::Justification::centredLeft);
            x += 20.0f + (float) juce::GlyphArrangement::getStringWidthInt (juce::Font (juce::FontOptions (11.5f)), text) + 18.0f;
        }
    }

    //==============================================================================
    /** A made-up line and what its voices sing, one value per frame. */
    struct Example
    {
        std::vector<tune::Frame> frames;
        std::vector<std::vector<float>> voices;
        std::vector<juce::Colour> colours;
    };

    /** The singing guide's phrase with voices: each is (voice settings, mode) and sings the lead's note plus its shift in the key. */
    Example makeExample (const std::vector<std::pair<harm::Voice, harm::Mode>>& voices, int mask = cMajorMask)
    {
        Example e;
        const auto sung = tune::makeFrames (harm::getPhraseNotes(), harm::phraseSeconds, tune::exampleFrameSeconds);
        e.frames.resize (sung.size());

        for (size_t i = 0; i < sung.size(); ++i)
            e.frames[i].sung = e.frames[i].corrected = sung[i];

        for (size_t v = 0; v < voices.size(); ++v)
        {
            std::vector<float> line (sung.size(), -1.0f);
            int reference = -1;

            for (size_t i = 0; i < sung.size(); ++i)
                if (sung[i] >= 0.0f)
                    line[i] = (float) (sung[i] + harm::getShift (sung[i], voices[v].first, voices[v].second, mask, reference));
                else
                    reference = -1;

            e.voices.push_back (std::move (line));
            e.colours.push_back (tunepics::getVoiceColour ((int) v));
        }

        return e;
    }

    harm::Voice makeVoice (int semitones, int steps)
    {
        harm::Voice v;
        v.on = true;
        v.semitones = semitones;
        v.steps = steps;
        return v;
    }

    void paintExample (juce::Graphics& g, juce::Rectangle<float> area, const Example& e, int mask = cMajorMask)
    {
        tunepics::Trace trace;
        trace.frames = &e.frames;
        trace.framesPerSecond = 1.0 / tune::exampleFrameSeconds;
        trace.seconds = (double) e.frames.size() * tune::exampleFrameSeconds;
        trace.keyMask = mask;
        trace.showBars = false;
        trace.showCorrected = false;
        trace.fixedLow = 55;

        for (size_t v = 0; v < e.voices.size(); ++v)
            trace.extraLines.push_back ({ &e.voices[v], e.colours[v] });

        tunepics::paintTrace (g, area, trace);
    }

    //==============================================================================
    void paintWhat (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "\"Hold on, hold on to-night\" in C major, with voice 1 set to a 3rd up in the key");
        auto legendArea = r.removeFromBottom (20.0f);
        auto notes = r.removeFromBottom (44.0f);
        const auto example = makeExample ({ { makeVoice (4, 2), harm::Mode::key } });
        paintExample (g, r, example);
        legend (g, legendArea, { { theme::text, "lead" }, { tunepics::getVoiceColour (0), "voice 1, 3rd up in C major" } });

        g.setFont (juce::FontOptions (12.5f));
        g.setColour (theme::text);
        g.drawText ("Lead     E4  E4  G4  E4  D4  C4", notes.removeFromTop (20.0f).reduced (8.0f, 0.0f), juce::Justification::centredLeft);
        g.setColour (tunepics::getVoiceColour (0));
        g.drawText ("Voice 1  G4  G4  B4  G4  F4  E4     (+3  +3  +4  +3  +3  +4 semitones)", notes.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
    }

    void paintIntervals (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "From C4 up to C5: each interval's size in semitones, its name and the number its frequency is multiplied by");
        const float rowHeight = r.getHeight() / 13.0f;
        const float barLeft = r.getX() + 330.0f;
        const float barWidth = r.getRight() - barLeft - 16.0f;

        for (int n = 0; n <= 12; ++n)
        {
            auto row = juce::Rectangle<float> (r.getX(), r.getY() + (float) n * rowHeight, r.getWidth(), rowHeight).reduced (4.0f, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (n % 2 == 0 ? 0.28f : 0.18f));
            g.fillRoundedRectangle (row, 3.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawText (juce::String (n), row.withWidth (36.0f), juce::Justification::centred);
            g.setFont (juce::FontOptions (12.5f));
            g.drawText (harm::getSizeName (n), row.withX (row.getX() + 40.0f).withWidth (110.0f), juce::Justification::centredLeft);
            g.setColour (theme::textDim);
            g.drawText ("C4 " + juce::String::fromUTF8 ("\xe2\x86\x92") + " " + tunepics::getNoteName (60 + n), row.withX (row.getX() + 150.0f).withWidth (90.0f), juce::Justification::centredLeft);
            g.drawText (juce::String::fromUTF8 ("\xc3\x97") + juce::String (std::pow (2.0, n / 12.0), 4), row.withX (row.getX() + 240.0f).withWidth (80.0f), juce::Justification::centredLeft);
            const bool perfect = n == 0 || n == 5 || n == 7 || n == 12;
            g.setColour ((perfect ? tunepics::harmoniserColour : tunepics::getVoiceColour (0)).withAlpha (0.8f));
            g.fillRoundedRectangle (juce::Rectangle<float> (barLeft, row.getY() + 3.0f, juce::jmax (3.0f, barWidth * (float) n / 12.0f), row.getHeight() - 6.0f), 2.0f);
        }
    }

    void paintFixedOrKey (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "The C major scale from C4, moved up a 3rd two ways. A ringed note is outside C major.");
        const int scale[] = { 60, 62, 64, 65, 67, 69, 71, 72 };
        const float labelWidth = 150.0f;
        const float cell = (r.getWidth() - labelWidth) / 8.0f;
        const float rowHeight = juce::jmin (70.0f, r.getHeight() / 3.0f);
        const char* titles[] = { "The scale", "Fixed: +4 semitones", "Key: +2 steps" };

        for (int row = 0; row < 3; ++row)
        {
            const float y = r.getY() + (float) row * rowHeight + 10.0f;
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawText (titles[row], juce::Rectangle<float> (r.getX(), y, labelWidth - 6.0f, 36.0f), juce::Justification::centredLeft);

            for (int i = 0; i < 8; ++i)
            {
                const int note = row == 0 ? scale[i] : row == 1 ? scale[i] + 4 : harm::stepInKey (scale[i], 2, cMajorMask);
                const bool in = (cMajorMask & (1 << (note % 12))) != 0;
                const auto box = juce::Rectangle<float> (r.getX() + labelWidth + (float) i * cell, y, cell - 8.0f, 36.0f);
                const auto colour = row == 0 ? theme::text : tunepics::getVoiceColour (row == 1 ? 1 : 0);
                g.setColour (colour.withAlpha (0.22f));
                g.fillRoundedRectangle (box, 5.0f);
                g.setColour (colour);
                g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
                g.drawText (tunepics::getNoteName (note), box, juce::Justification::centred);

                if (! in)
                {
                    g.setColour (theme::text);
                    g.drawRoundedRectangle (box.expanded (2.0f), 6.0f, 1.6f);
                }
            }
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.5f));
        g.drawText ("Fixed leaves the key on 4 of 8 notes (F#, G#, C#, D#); Key stays in it, with major and minor thirds.",
                    juce::Rectangle<float> (r.getX(), r.getY() + 3.0f * rowHeight + 14.0f, r.getWidth(), 20.0f), juce::Justification::centredLeft);
    }

    void paintHow (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "The same line with a 3rd above, a 6th below, a 5th above and an octave below");
        struct Row { const char* title; const char* kind; harm::Voice voice; harm::Mode mode; };
        const Row rows[] = { { "3rd above", "imperfect consonance", makeVoice (4, 2), harm::Mode::key },
                             { "6th below", "imperfect consonance", makeVoice (-9, -5), harm::Mode::key },
                             { "5th above", "perfect consonance", makeVoice (7, 4), harm::Mode::fixed },
                             { "octave below", "perfect consonance", makeVoice (-12, -7), harm::Mode::fixed } };
        const float rowHeight = r.getHeight() / 4.0f;

        for (int i = 0; i < 4; ++i)
        {
            auto row = r.withHeight (rowHeight).withY (r.getY() + (float) i * rowHeight).reduced (0.0f, 2.0f);
            auto label = row.removeFromLeft (170.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
            g.drawText (rows[i].title, label.removeFromTop (label.getHeight() * 0.5f).translated (0.0f, 8.0f), juce::Justification::centredLeft);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (rows[i].kind, label.translated (0.0f, 4.0f).withHeight (20.0f), juce::Justification::topLeft);
            const auto example = makeExample ({ { rows[i].voice, rows[i].mode } });
            paintExample (g, row, example);
        }
    }

    void drawNoteBlock (juce::Graphics& g, juce::Rectangle<float> plot, int low, int rows, double beatStart, double beatEnd, double totalBeats, int note, juce::Colour c, const juce::String& text)
    {
        const float rowHeight = plot.getHeight() / (float) rows;
        const auto box = juce::Rectangle<float> (plot.getX() + (float) (beatStart / totalBeats) * plot.getWidth(), plot.getBottom() - (float) (note - low + 1) * rowHeight,
                                                 (float) ((beatEnd - beatStart) / totalBeats) * plot.getWidth() - 3.0f, rowHeight - 2.0f);
        g.setColour (c);
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.setFont (juce::FontOptions (juce::jmin (12.0f, rowHeight), juce::Font::bold));
        g.drawText (text, box, juce::Justification::centred);
    }

    void paintWhere (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A C major bar: the lead's E, D, C with a 3rd up in the key (left), and the one fifth that isn't perfect (right)");
        auto left = r.removeFromLeft (r.getWidth() * 0.62f).reduced (0.0f, 4.0f);
        r.removeFromLeft (14.0f);
        constexpr int low = 59, rows = 12;
        const auto panel = left.withTrimmedBottom (30.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (panel, 4.0f);
        auto plot = panel.reduced (34.0f, 6.0f).withTrimmedRight (0.0f);
        const float rowHeight = plot.getHeight() / (float) rows;

        for (int n = low; n < low + rows; ++n)
        {
            const bool chord = n % 12 == 0 || n % 12 == 4 || n % 12 == 7;
            const bool in = (cMajorMask & (1 << (n % 12))) != 0;
            const auto row = juce::Rectangle<float> (plot.getX(), plot.getBottom() - (float) (n - low + 1) * rowHeight, plot.getWidth(), rowHeight);
            g.setColour (chord ? tunepics::harmoniserColour.withAlpha (0.28f) : (in ? theme::panelLight.withAlpha (0.6f) : theme::background.withAlpha (0.4f)));
            g.fillRect (row.reduced (0.0f, 0.5f));
            g.setColour (chord ? theme::text : theme::textDim);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (tunepics::getNoteName (n), juce::Rectangle<float> (panel.getX(), row.getY(), 30.0f, rowHeight), juce::Justification::centredRight);
        }

        const int lead[] = { 64, 62, 60 }, voice[] = { 67, 65, 64 };
        const char* leadNames[] = { "E4", "D4", "C4" };
        const char* voiceNames[] = { "G4", "F4", "E4" };

        for (int i = 0; i < 3; ++i)
        {
            drawNoteBlock (g, plot, low, rows, i * 2.0, i * 2.0 + 2.0, 6.0, lead[i], theme::text, leadNames[i]);
            drawNoteBlock (g, plot, low, rows, i * 2.0, i * 2.0 + 2.0, 6.0, voice[i], tunepics::getVoiceColour (0), voiceNames[i]);
        }

        // The F against the chord's E.
        const float rowH = plot.getHeight() / (float) rows;
        const auto f = juce::Rectangle<float> (plot.getX() + plot.getWidth() / 3.0f, plot.getBottom() - (float) (65 - low + 1) * rowH, plot.getWidth() / 3.0f - 3.0f, rowH - 2.0f);
        g.setColour (theme::meterRed);
        g.drawRoundedRectangle (f.expanded (2.0f), 4.0f, 1.8f);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("F4: 1 semitone above the chord's E", juce::Rectangle<float> (f.getX() + 6.0f, f.getY() - rowH * 2.5f, plot.getRight() - f.getX() - 6.0f, 18.0f), juce::Justification::centredLeft);
        g.setColour (theme::textDim);
        g.drawText ("shaded rows: the chord C, E, G", left.removeFromBottom (24.0f), juce::Justification::centredLeft);

        // B to F: six semitones.
        auto box = r.reduced (0.0f, 4.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (box.withTrimmedBottom (30.0f), 4.0f);
        auto inner = box.withTrimmedBottom (30.0f).reduced (20.0f, 14.0f);
        const float x1 = inner.getX() + inner.getWidth() * 0.25f, x2 = inner.getX() + inner.getWidth() * 0.75f;
        const float yLow = inner.getBottom() - 20.0f, yHigh = inner.getY() + 30.0f;
        g.setColour (theme::text);
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ x1, yLow }));
        g.setColour (tunepics::getVoiceColour (0));
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ x2, yHigh }));
        g.setColour (theme::meterRed);
        g.drawArrow ({ x1 + 10.0f, yLow - 4.0f, x2 - 10.0f, yHigh + 4.0f }, 2.0f, 9.0f, 9.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.5f));
        g.drawText ("B4  493.88 Hz", juce::Rectangle<float> (x1 - 50.0f, yLow + 8.0f, 100.0f, 16.0f), juce::Justification::centred);
        g.drawText ("F5  698.46 Hz", juce::Rectangle<float> (x2 - 50.0f, yHigh - 26.0f, 100.0f, 16.0f), juce::Justification::centred);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("6 semitones", juce::Rectangle<float> (inner.getCentreX() - 50.0f, inner.getCentreY() - 8.0f, 100.0f, 16.0f), juce::Justification::centred);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("a 5th up in the key, on B", box.removeFromBottom (24.0f), juce::Justification::centredLeft);
    }

    void paintDouble (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "Lead in the centre; two copies a few cents off, a little late and panned apart (the Wide double starting point)");
        auto plot = r.reduced (60.0f, 10.0f).withTrimmedBottom (26.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot.expanded (10.0f, 8.0f), 4.0f);
        auto xFor = [&] (double ms) { return plot.getX() + (float) (ms / 60.0) * plot.getWidth(); };
        auto yFor = [&] (double pan) { return plot.getCentreY() + (float) (pan / 100.0) * plot.getHeight() * 0.5f; };

        g.setColour (theme::gridBar);
        g.fillRect (plot.getX(), plot.getCentreY() - 0.5f, plot.getWidth(), 1.0f);

        for (const double ms : { 30.0, 50.0 })
        {
            g.setColour (ms < 40.0 ? tunepics::harmoniserColour : theme::meterRed);
            g.fillRect (xFor (ms) - 0.5f, plot.getY(), 1.0f, plot.getHeight());
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText (ms < 40.0 ? "30 ms: still one sound" : "50 ms: an echo", juce::Rectangle<float> (xFor (ms) + 4.0f, plot.getY() + 2.0f, 130.0f, 30.0f).toNearestInt(), juce::Justification::topLeft, 2);
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("left", juce::Rectangle<float> (r.getX(), plot.getY(), 52.0f, 16.0f), juce::Justification::centredRight);
        g.drawText ("right", juce::Rectangle<float> (r.getX(), plot.getBottom() - 16.0f, 52.0f, 16.0f), juce::Justification::centredRight);

        for (int ms = 0; ms <= 60; ms += 10)
            g.drawText (juce::String (ms) + (ms == 60 ? " ms later" : ""), juce::Rectangle<float> (xFor (ms) - 40.0f, plot.getBottom() + 12.0f, 80.0f, 14.0f), juce::Justification::centred);

        struct Copy { double ms, pan; float size; juce::Colour c; const char* text; };
        const Copy copies[] = { { 0.0, 0.0, 22.0f, theme::text, "lead  0 ms, centre" },
                                { 12.0, -70.0, 16.0f, tunepics::getVoiceColour (0), "+7 cents, 12 ms, 70% left, -8 dB" },
                                { 20.0, 70.0, 16.0f, tunepics::getVoiceColour (1), "-7 cents, 20 ms, 70% right, -8 dB" } };

        for (const auto& c : copies)
        {
            const juce::Point<float> p { xFor (c.ms), yFor (c.pan) };
            g.setColour (c.c);
            g.fillEllipse (juce::Rectangle<float> (c.size, c.size).withCentre (p));
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (c.text, juce::Rectangle<float> (p.x + c.size * 0.5f + 6.0f, p.y - 9.0f, 230.0f, 18.0f), juce::Justification::centredLeft);
        }
    }

    void paintFormants (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "Harmonics (lines) under the shape of an \"ah\" (curve): the voice's colour is the curve");
        const struct { const char* title; double f0; double envelopeScale; } rows[] = {
            { "A vowel on 220 Hz (peaks 730 and 1,090 Hz)", 220.0, 1.0 },
            { "Up a 5th by speeding up: 329.6 Hz, and the curve moves too (peaks 1,094 and 1,633 Hz)", 329.6, 1.498 },
            { "Up a 5th by PSOLA: 329.6 Hz, the curve stays (peaks 730 and 1,090 Hz)", 329.6, 1.0 } };
        const float rowHeight = r.getHeight() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            auto row = r.withHeight (rowHeight).withY (r.getY() + (float) i * rowHeight).reduced (8.0f, 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (row, 4.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (rows[i].title, row.removeFromTop (18.0f).reduced (8.0f, 0.0f), juce::Justification::centredLeft);
            auto plot = row.reduced (10.0f, 6.0f);
            plot.removeFromBottom (12.0f);
            auto xFor = [&] (double hz) { return plot.getX() + (float) (hz / 4000.0) * plot.getWidth(); };
            auto level = [&] (double hz) { return (float) (tune::SungVoice::getEnvelope (hz / rows[i].envelopeScale) / 1.4); };

            for (int n = 1; n * rows[i].f0 <= 4000.0; ++n)
            {
                const double hz = n * rows[i].f0;
                const float h = juce::jlimit (0.02f, 1.0f, level (hz)) * plot.getHeight();
                g.setColour (tunepics::harmoniserColour.brighter (0.2f));
                g.fillRect (xFor (hz) - 1.0f, plot.getBottom() - h, 2.0f, h);
            }

            juce::Path envelope;

            for (int x = 0; x <= (int) plot.getWidth(); ++x)
            {
                const double hz = 4000.0 * x / plot.getWidth();
                const juce::Point<float> p { plot.getX() + (float) x, plot.getBottom() - juce::jlimit (0.0f, 1.0f, level (hz)) * plot.getHeight() };
                x == 0 ? envelope.startNewSubPath (p) : envelope.lineTo (p);
            }

            g.setColour (theme::accent);
            g.strokePath (envelope, juce::PathStrokeType (1.6f));
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (10.0f));

            for (const int hz : { 0, 1000, 2000, 3000, 4000 })
                g.drawText (hz == 4000 ? "4 kHz" : juce::String (hz), juce::Rectangle<float> (xFor (hz) - 20.0f, plot.getBottom() + 1.0f, 40.0f, 11.0f), juce::Justification::centred);
        }
    }

    void paintHears (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A sung \"sun\" and then a chord: where the voices sing, and where they fade out");
        struct Part { const char* title; const char* what; bool clear; double share; };
        const Part parts[] = { { "s", "noise, no clear note: voices off", false, 0.25 }, { "un", "one clear note: voices on", true, 0.40 }, { "C major chord", "three notes at once: voices off", false, 0.35 } };
        auto area = r.reduced (0.0f, 8.0f).withTrimmedBottom (20.0f);
        const float labelWidth = 80.0f;
        auto labels = area.removeFromLeft (labelWidth);
        const float rowHeight = area.getHeight() / 2.0f;
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("lead", labels.removeFromTop (rowHeight), juce::Justification::centredRight);
        g.setColour (tunepics::getVoiceColour (0));
        g.drawText ("voices", labels, juce::Justification::centredRight);
        float x = area.getX() + 8.0f;
        const float total = area.getWidth() - 8.0f;

        for (const auto& p : parts)
        {
            const float w = (float) p.share * total;
            auto column = juce::Rectangle<float> (x, area.getY(), w - 6.0f, area.getHeight());
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (column, 4.0f);
            auto lead = column.removeFromTop (rowHeight).reduced (8.0f, 14.0f);
            auto voice = column.reduced (8.0f, 14.0f);

            if (p.clear)
            {
                g.setColour (theme::text);
                g.fillRoundedRectangle (lead.withSizeKeepingCentre (lead.getWidth(), 8.0f), 3.0f);
                g.setColour (tunepics::getVoiceColour (0));
                g.fillRoundedRectangle (voice.withSizeKeepingCentre (voice.getWidth(), 8.0f).translated (0.0f, -12.0f), 3.0f);
            }
            else
            {
                g.setColour (theme::textDim);
                juce::Random random (7);

                if (p.title[0] == 's')
                {
                    for (float px = lead.getX(); px < lead.getRight(); px += 3.0f)
                        g.fillRect (px, lead.getCentreY() + (random.nextFloat() - 0.5f) * 14.0f, 1.5f, 1.5f);
                }
                else
                {
                    for (const float dy : { -12.0f, 0.0f, 12.0f })
                        g.fillRoundedRectangle (lead.withSizeKeepingCentre (lead.getWidth(), 4.0f).translated (0.0f, dy), 2.0f);
                }

                g.setColour (theme::textDim.withAlpha (0.6f));
                g.drawText (juce::String::fromUTF8 ("\xe2\x80\x93"), voice, juce::Justification::centred);
            }

            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawText (p.title, column.withY (area.getY() + 2.0f).withHeight (16.0f).reduced (8.0f, 0.0f), juce::Justification::centredLeft);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (p.what, juce::Rectangle<float> (x, area.getBottom() + 2.0f, w - 6.0f, 18.0f), juce::Justification::centredLeft);
            x += w;
        }
    }

    void paintChain (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A vocal track's chain: tune the lead first, then add the voices");
        const char* names[] = { "Vocal take", "DawIt Tune", "DawIt Harmoniser", "DawIt EQ", "DawIt Compressor", "Reverb send" };
        const char* notes[] = { "dry", "tune the lead", "voices under the lead", "shape the tone", "even out the level", "reverb on a send" };
        const float gap = 18.0f;
        const float boxWidth = (r.getWidth() - 16.0f - gap * 5.0f) / 6.0f;
        auto row = r.withSizeKeepingCentre (r.getWidth(), 120.0f).reduced (8.0f, 0.0f);

        for (int i = 0; i < 6; ++i)
        {
            auto box = row.withX (row.getX() + (float) i * (boxWidth + gap)).withWidth (boxWidth).withHeight (56.0f).withY (row.getY() + 10.0f);
            g.setColour (i == 2 ? tunepics::harmoniserColour.withAlpha (0.35f) : theme::panelLight);
            g.fillRoundedRectangle (box, 5.0f);
            g.setColour (i == 2 ? tunepics::harmoniserColour : theme::gridBar);
            g.drawRoundedRectangle (box, 5.0f, 1.5f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawFittedText (names[i], box.toNearestInt(), juce::Justification::centred, 2);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText (notes[i], box.translated (0.0f, 62.0f).withHeight (32.0f).toNearestInt(), juce::Justification::centredTop, 2);

            if (i < 5)
                g.drawArrow ({ box.getRight() + 2.0f, box.getCentreY(), box.getRight() + gap - 2.0f, box.getCentreY() }, 1.6f, 8.0f, 8.0f);
        }
    }

    //==============================================================================
    harm::KeyInfo getKeyInfo (BuiltInHarmoniser& p)
    {
        const auto s = p.getSettings();
        harm::KeyInfo info;
        const bool songKey = juce::BigInteger (p.getSongKeyMask()).countNumberOfSetBits() == 7;
        info.known = s.key != harm::keyFromSong || songKey;
        info.mask = p.getEffectiveKeyMask();
        info.name = s.key != harm::keyFromSong ? harm::getNoteName (s.key - 1) + (s.scale == harm::Scale::minor ? " minor" : " major")
                                               : (songKey ? tunetext::getSong().key.getName() : juce::String());
        return info;
    }

    /** The latency in milliseconds at the sample rate the effect is running at, or 0 when it hasn't been prepared. */
    double getLateMs (const BuiltInHarmoniser& p)
    {
        const double rate = p.getSampleRate();
        return rate > 0.0 ? 1000.0 * BuiltInHarmoniser::getLatencyFor (rate) / rate : 0.0;
    }

    juce::String getLatencyLine (BuiltInHarmoniser& p)
    {
        const auto host = fxui::getHostTrack (p);
        return host.isBus || host.isReturn || host.isInstrument ? harm::getLatencyLine (host.isReturn ? "return" : host.isBus ? "bus" : "instrument track", getLateMs (p)) : juce::String();
    }

    bool hasEnoughNotes (const std::vector<harm::Frame>& log)
    {
        int clear = 0;

        for (const auto& f : log)
            clear += f.lead >= 0.0f ? 1 : 0;

        return clear >= 100;
    }

    void paintYourTrack (juce::Graphics& g, juce::Rectangle<float> r, BuiltInHarmoniser& processor)
    {
        const auto log = processor.getTakeLog();

        if (! hasEnoughNotes (log))
        {
            caption (g, r, "A made-up take, until this track has played through DawIt Harmoniser");
            auto legendArea = r.removeFromBottom (20.0f);
            paintExample (g, r, makeExample ({ { makeVoice (4, 2), harm::Mode::key } }));
            legend (g, legendArea, { { theme::text, "lead" }, { tunepics::getVoiceColour (0), "voice 1" } });
            return;
        }

        const auto song = tunetext::getSong();
        const auto host = fxui::getHostTrack (processor);
        caption (g, r, host.name.isEmpty() ? juce::String ("What DawIt Harmoniser heard") : host.name + ": what DawIt Harmoniser heard");
        auto legendArea = r.removeFromBottom (20.0f);
        legend (g, legendArea, { { theme::text, "lead" }, { tunepics::getVoiceColour (0), "voice 1" }, { tunepics::getVoiceColour (1), "voice 2" } });

        float lastBeat = 0.0f;
        float lo = 127.0f, hi = 0.0f;

        for (const auto& f : log)
        {
            lastBeat = juce::jmax (lastBeat, f.beat);

            for (const float v : { f.lead, f.voice[0], f.voice[1] })
                if (v >= 0.0f)
                {
                    lo = juce::jmin (lo, v);
                    hi = juce::jmax (hi, v);
                }
        }

        const int bars = juce::jmax (song.numBars, (int) std::ceil (lastBeat / 4.0f) + 1);
        auto plotArea = r.reduced (8.0f, 0.0f);
        auto margin = plotArea.removeFromLeft (34.0f);
        auto bands = plotArea.removeFromTop (22.0f);
        auto xFor = [&] (double beat) { return plotArea.getX() + (float) (beat / 4.0 / bars) * plotArea.getWidth(); };

        for (size_t i = 0; i < song.sections.size(); ++i)
        {
            const auto& s = song.sections[i];
            const auto box = juce::Rectangle<float>::leftTopRightBottom (xFor (s.startBar * 4.0), bands.getY(), xFor (s.getEndBar() * 4.0), bands.getBottom());
            g.setColour (SectionLane::getColour (s.type).withAlpha (0.8f));
            g.fillRect (box.reduced (1.0f, 0.0f));
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (s.getDisplayName(), box.reduced (4.0f, 0.0f), juce::Justification::centredLeft);
        }

        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plotArea.expanded (0.0f, 2.0f), 4.0f);
        const int low = juce::jmax (0, (int) lo - 2), high = juce::jmax (low + 8, (int) hi + 2);
        const float rowHeight = plotArea.getHeight() / (float) (high - low + 1);
        auto yFor = [&] (double pitch) { return plotArea.getBottom() - (float) (pitch - low + 0.5) * rowHeight; };
        const int mask = processor.getEffectiveKeyMask();

        for (int note = low; note <= high; ++note)
        {
            const bool in = (mask & (1 << (note % 12))) != 0;
            g.setColour (in ? theme::panelLight.withAlpha (0.5f) : theme::background.withAlpha (0.3f));
            g.fillRect (juce::Rectangle<float> (plotArea.getX(), yFor (note) - rowHeight * 0.5f + 0.5f, plotArea.getWidth(), rowHeight - 1.0f));

            if ((high - low) < 24 || note % 12 == 0)
            {
                g.setColour (in ? theme::text : theme::textDim.withAlpha (0.5f));
                g.setFont (juce::FontOptions (9.5f));
                g.drawText (tunepics::getNoteName (note), juce::Rectangle<float> (margin.getX(), yFor (note) - 7.0f, margin.getWidth() - 3.0f, 14.0f), juce::Justification::centredRight);
            }
        }

        for (const auto& f : log)
        {
            if (f.beat < 0.0f)
                continue;

            const float x = xFor (f.beat);

            for (int v = 0; v < harm::numVoices; ++v)
                if (f.voice[(size_t) v] >= 0.0f)
                {
                    g.setColour (tunepics::getVoiceColour (v));
                    g.fillRect (x, yFor (f.voice[(size_t) v]) - 1.0f, 1.5f, 2.0f);
                }

            if (f.lead >= 0.0f)
            {
                g.setColour (theme::text);
                g.fillRect (x, yFor (f.lead) - 1.0f, 1.5f, 2.0f);
            }
        }
    }

    juce::String getYourTrackText_ (BuiltInHarmoniser& p)
    {
        const auto lines = harm::getAdvice (p.getTakeLog(), p.getSettings(), getKeyInfo (p), p.getFramesPerSecond(), getLatencyLine (p));
        juce::StringArray all;

        for (const auto& l : lines)
            all.add (l);

        return gen::harmoniserguide::getPageText (9) + "\n\n" + all.joinIntoString ("\n\n");
    }
} // namespace

juce::String getYourTrackText (BuiltInHarmoniser& p) { return getYourTrackText_ (p); }

std::vector<fxui::Guide::Page> makeGuide (BuiltInHarmoniser& processor)
{
    auto* p = &processor;
    std::vector<fxui::Guide::Page> pages;

    for (int i = 0; i < gen::harmoniserguide::numPages; ++i)
        pages.push_back ({ gen::harmoniserguide::getPageName (i), gen::harmoniserguide::getPageSummary (i), gen::harmoniserguide::getPageText (i), {} });

    pages[0].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintWhat (g, r); };
    pages[1].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintIntervals (g, r); };
    pages[2].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintFixedOrKey (g, r); };
    pages[3].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintHow (g, r); };
    pages[4].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintWhere (g, r); };
    pages[5].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintDouble (g, r); };
    pages[6].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintFormants (g, r); };
    pages[7].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintHears (g, r); };
    pages[8].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintChain (g, r); };
    pages[9].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintYourTrack (g, r, *p); };
    pages[9].liveText = [p] { return getYourTrackText_ (*p); };

    // The pages that only draw an idea have no test sound; the others play the sung line through the effect.
    for (const int i : { 1, 2, 4, 5, 6, 8 })
        pages[(size_t) i].noClap = true;

    return pages;
}
} // namespace daw::harmtext
