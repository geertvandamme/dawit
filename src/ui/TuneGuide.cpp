#include "TuneGuide.h"

#include "SectionLane.h"
#include "Theme.h"
#include "TunePictures.h"
#include "engine/PitchTracker.h"
#include "engine/SungVoice.h"
#include "generate/TuneAnalysis.h"
#include "generate/TuneGuideText.h"

namespace daw::tunetext
{
namespace
{
    SongAccess access;

    constexpr int aMinorMask = (1 << 9) | (1 << 11) | (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7);
    constexpr int cMajorMask = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11);

    //==============================================================================
    void caption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (text, r.removeFromTop (22.0f), juce::Justification::centredLeft);
    }

    /** A moving line over a picture for a few seconds after the test sound, so the picture can be captured at a repeatable moment. */
    void drawCursor (juce::Graphics& g, juce::Rectangle<float> area, double sinceClap, double total)
    {
        if (sinceClap < 0.0 || sinceClap > total)
            return;

        g.setColour (theme::playhead.withAlpha (0.8f));
        g.fillRect (area.getX() + (float) (sinceClap / total) * area.getWidth(), area.getY(), 1.5f, area.getHeight());
    }

    tune::Settings getSettingsFor (double speedMs, bool keepVibrato, double tolerance = 10.0)
    {
        tune::Settings s;
        s.mode = tune::Mode::correct;
        s.speedMs = (float) speedMs;
        s.toleranceCents = (float) tolerance;
        s.keepVibrato = keepVibrato;
        return s;
    }

    std::vector<tune::Frame> framesFor (const std::vector<tune::TakeNote>& notes, double seconds, const tune::Settings& settings, int mask)
    {
        const auto sung = tune::makeFrames (notes, seconds, tune::exampleFrameSeconds);
        return tunepics::makeFrames (sung, tune::simulate (sung, settings, mask, 1.0 / tune::exampleFrameSeconds));
    }

    void paintFrames (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<tune::Frame>& frames, int mask, int fixedLow = -1)
    {
        tunepics::Trace trace;
        trace.frames = &frames;
        trace.framesPerSecond = 1.0 / tune::exampleFrameSeconds;
        trace.seconds = (double) frames.size() * tune::exampleFrameSeconds;
        trace.keyMask = mask;
        trace.showBars = false;
        trace.fixedLow = fixedLow;
        tunepics::paintTrace (g, area, trace);
    }

    void legend (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setFont (juce::FontOptions (11.5f));
        float x = area.getX();
        const struct { juce::Colour c; const char* text; } items[] = {
            { theme::meterGreen, "within 10 cents" }, { theme::meterYellow, "within 25" }, { theme::meterRed, "beyond" }, { theme::accent, "after DawIt Tune" } };

        for (const auto& item : items)
        {
            g.setColour (item.c);
            g.fillRect (x, area.getCentreY() - 1.5f, 16.0f, 3.0f);
            g.setColour (theme::textDim);
            g.drawText (item.text, juce::Rectangle<float> (x + 20.0f, area.getY(), 120.0f, area.getHeight()), juce::Justification::centredLeft);
            x += 20.0f + (float) juce::GlyphArrangement::getStringWidthInt (juce::Font (juce::FontOptions (11.5f)), item.text) + 18.0f;
        }
    }

    //==============================================================================
    void paintWhat (juce::Graphics& g, juce::Rectangle<float> r, double sinceClap)
    {
        caption (g, r, "A short line sung in A minor, and what DawIt Tune makes of it (Natural fix)");
        auto legendArea = r.removeFromBottom (20.0f);
        const auto frames = framesFor (tune::getExamplePhraseNotes(), 3.0, tune::getPresets()[1].settings, aMinorMask);
        paintFrames (g, r, frames, aMinorMask, 60);
        drawCursor (g, r.reduced (4.0f).withTrimmedLeft (tunepics::traceMargin), sinceClap, 3.0);
        legend (g, legendArea);
    }

    void paintCents (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A4 to A#4 is 100 cents; the coloured bands are DawIt's: 10 and 25 cents either side of a note");
        auto ruler = r.reduced (30.0f, 0.0f);
        ruler.removeFromBottom (70.0f);
        ruler = ruler.withTrimmedTop (30.0f);
        const auto line = ruler.withHeight (46.0f).withY (ruler.getCentreY() - 40.0f);
        auto xFor = [&] (double cents) { return line.getX() + (float) ((cents + 60.0) / 220.0) * line.getWidth(); };

        for (const double centre : { 0.0, 100.0 })
        {
            g.setColour (tunepics::getZoneColour (tune::Zone::close).withAlpha (0.3f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (centre - 25.0), line.getY(), xFor (centre + 25.0), line.getBottom()));
            g.setColour (tunepics::getZoneColour (tune::Zone::inTune).withAlpha (0.45f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (centre - 10.0), line.getY(), xFor (centre + 10.0), line.getBottom()));
            g.setColour (theme::text);
            g.fillRect (xFor (centre) - 1.0f, line.getY() - 6.0f, 2.0f, line.getHeight() + 12.0f);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (centre < 50.0 ? "A4   440.00 Hz" : "A#4   466.16 Hz", juce::Rectangle<float> (xFor (centre) - 70.0f, line.getBottom() + 8.0f, 140.0f, 18.0f), juce::Justification::centred);
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));

        for (int c = -60; c <= 160; c += 10)
        {
            g.fillRect (xFor (c) - 0.5f, line.getBottom() - (c % 50 == 0 ? 8.0f : 4.0f), 1.0f, c % 50 == 0 ? 8.0f : 4.0f);

            if (c % 50 == 0)
                g.drawText (c > 0 ? "+" + juce::String (c) : juce::String (c), juce::Rectangle<float> (xFor (c) - 18.0f, line.getBottom() + 28.0f, 36.0f, 14.0f), juce::Justification::centred);
        }

        // Three notes, with their frequencies.
        const struct { double cents; const char* text; } marks[] = { { -30.0, "-30 cents\n432.44 Hz" }, { -20.0, "-20 cents\n434.95 Hz" }, { 50.0, "+50 cents\n452.89 Hz" } };
        int row = 0;

        for (const auto& m : marks)
        {
            const float x = xFor (m.cents);
            const float top = line.getY() - 8.0f - (float) (row % 2) * 4.0f;
            g.setColour (tunepics::getZoneColour (tune::getZone (m.cents)));
            g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ x, line.getCentreY() }));
            g.setFont (juce::FontOptions (11.5f));
            g.setColour (theme::text);
            // The two flat notes are close together, so their labels lean away from each other.
            const auto box = row == 0 ? juce::Rectangle<float> (x - 90.0f, top - 32.0f, 100.0f, 30.0f) : row == 1 ? juce::Rectangle<float> (x - 10.0f, top - 32.0f, 100.0f, 30.0f)
                                                                                                                  : juce::Rectangle<float> (x - 46.0f, top - 32.0f, 92.0f, 30.0f);
            g.drawFittedText (m.text, box.toNearestInt(), row == 0 ? juce::Justification::centredRight : row == 1 ? juce::Justification::centredLeft : juce::Justification::centred, 2);
            ++row;
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText ("Cents measure a ratio, so a cent is the same small step at any pitch. At A2 (110 Hz) the same 20 cents is only 1.26 Hz: 110.00 Hz becomes 108.74 Hz.",
                          r.removeFromBottom (60.0f).reduced (30.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, 3);
    }

    struct MatchData
    {
        std::vector<float> wave, buzzMatch, noiseMatch;
        double rate = 44100.0;
    };

    const MatchData& getMatchData()
    {
        static const MatchData data = []
        {
            MatchData d;
            tune::SungVoice voice;
            voice.prepare (d.rate);
            const double midi = 57.0;   // A3
            std::vector<float> buzz (2048);

            for (auto& v : buzz)
                v = voice.next (midi);

            pitch::Tracker tracker;
            tracker.prepare (d.rate, 2048);
            tracker.analyse (buzz.data(), 2048);
            d.buzzMatch = tracker.getMatch();
            d.wave.assign (buzz.begin(), buzz.begin() + 1200);

            juce::Random random (4);
            std::vector<float> noise (2048);
            double energy = 0.0;

            for (auto& v : noise)
            {
                v = random.nextFloat() - 0.5f;
                energy += v * v;
            }

            const double target = std::sqrt (std::inner_product (buzz.begin(), buzz.end(), buzz.begin(), 0.0) / 2048.0), have = std::sqrt (energy / 2048.0);

            for (auto& v : noise)
                v = (float) (v * target / have);

            tracker.analyse (noise.data(), 2048);
            d.noiseMatch = tracker.getMatch();
            return d;
        }();

        return data;
    }

    void paintFinding (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A sung A3 repeats every 4.5 ms; DawIt Tune looks for the delay where the sound matches itself");
        const auto& d = getMatchData();
        auto left = r.removeFromLeft (r.getWidth() * 0.42f).reduced (6.0f);
        auto right = r.reduced (6.0f);

        // The wave, with one period marked.
        tunepics::paintTrace (g, juce::Rectangle<float>(), {});   // no-op: keeps the include honest
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (left, 4.0f);
        auto plot = left.reduced (10.0f, 26.0f);
        const int shown = 1100;
        juce::Path wave;

        for (int i = 0; i < shown; ++i)
        {
            const juce::Point<float> p { plot.getX() + plot.getWidth() * (float) i / shown, plot.getCentreY() - d.wave[(size_t) i] * plot.getHeight() * 1.4f };
            i == 0 ? wave.startNewSubPath (p) : wave.lineTo (p);
        }

        g.setColour (tunepics::colour);
        g.strokePath (wave, juce::PathStrokeType (1.3f));
        const float period = plot.getWidth() * (float) (d.rate / 220.0) / shown;
        g.setColour (theme::text);
        const float bracketY = plot.getBottom() + 6.0f;
        g.drawHorizontalLine ((int) bracketY, plot.getX() + period, plot.getX() + 2.0f * period);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("one period, 4.5 ms", juce::Rectangle<float> (plot.getX(), bracketY + 2.0f, plot.getWidth(), 14.0f), juce::Justification::centred);
        g.setColour (theme::textDim);
        g.drawText ("the sung A3 (25 ms)", juce::Rectangle<float> (left.getX(), left.getY() + 2.0f, left.getWidth(), 18.0f), juce::Justification::centred);

        // The match at each delay.
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (right, 4.0f);
        auto curves = right.reduced (14.0f, 10.0f);
        curves.removeFromTop (18.0f);
        auto buzzArea = curves.removeFromTop (curves.getHeight() * 0.56f);
        curves.removeFromTop (18.0f);   // room for the delays under the peaks
        auto noiseArea = curves.reduced (0.0f, 4.0f);
        const int maxLag = (int) (d.rate / 50.0);
        auto draw = [&] (juce::Rectangle<float> area, const std::vector<float>& match, juce::Colour c)
        {
            g.setColour (theme::gridBar);
            g.fillRect (area.getX(), area.getBottom() - 0.5f, area.getWidth(), 1.0f);
            g.fillRect (area.getX(), area.getY() + area.getHeight() * 0.5f, area.getWidth(), 1.0f);
            juce::Path p;

            for (int lag = 0; lag <= maxLag; ++lag)
            {
                const float v = juce::jlimit (0.0f, 1.0f, match[(size_t) lag]);
                const juce::Point<float> pt { area.getX() + area.getWidth() * (float) lag / maxLag, area.getBottom() - v * area.getHeight() };
                lag == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
            }

            g.setColour (c);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        };

        draw (buzzArea, d.buzzMatch, tunepics::colour);
        draw (noiseArea, d.noiseMatch, theme::textDim);
        g.setFont (juce::FontOptions (11.5f));

        for (const double ms : { 4.545, 9.091, 13.636 })
        {
            const float x = buzzArea.getX() + buzzArea.getWidth() * (float) (ms * 0.001 * d.rate / maxLag);
            g.setColour (tunepics::getZoneColour (tune::Zone::inTune));
            g.fillRect (x - 0.5f, buzzArea.getY(), 1.0f, buzzArea.getHeight());
            g.drawText (juce::String (ms, ms < 5.0 ? 3 : 3) + " ms", juce::Rectangle<float> (x - 30.0f, buzzArea.getBottom() + 1.0f, 60.0f, 14.0f), juce::Justification::centred);
        }

        g.setColour (theme::textDim);
        g.drawText ("match, from 0 to 20 ms: peaks at 1, 2 and 3 periods", juce::Rectangle<float> (right.getX(), right.getY() + 3.0f, right.getWidth(), 16.0f), juce::Justification::centred);
        g.drawText ("white noise at the same level: no peak above 0.5", noiseArea.removeFromTop (14.0f), juce::Justification::centredLeft);
    }

    void paintTargets (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "Target notes in C major: an F# is pulled to F or G, whichever is nearer");
        const int low = 60, high = 72;
        auto rows = r.reduced (6.0f);
        const float rowHeight = rows.getHeight() / (float) (high - low + 1);
        auto yFor = [&] (double pitch) { return rows.getBottom() - (float) (pitch - low + 0.5) * rowHeight; };
        const float left = rows.getX() + 38.0f;

        for (int note = low; note <= high; ++note)
        {
            const bool in = (cMajorMask & (1 << (note % 12))) != 0;
            g.setColour (in ? theme::gridBar.withAlpha (0.55f) : theme::background.withAlpha (0.55f));
            g.fillRect (juce::Rectangle<float> (left, yFor (note) - rowHeight * 0.5f + 0.5f, rows.getWidth() * 0.55f - 38.0f, rowHeight - 1.0f));
            g.setColour (in ? theme::text : theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawText (tunepics::getNoteName (note), juce::Rectangle<float> (rows.getX(), yFor (note) - rowHeight * 0.5f, 34.0f, rowHeight), juce::Justification::centredRight);
        }

        const float rowsRight = rows.getX() + rows.getWidth() * 0.55f;
        const float x1 = left + (rowsRight - left) * 0.3f, x2 = left + (rowsRight - left) * 0.7f;

        // The band around the halfway point.
        g.setColour (theme::accent.withAlpha (0.25f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (left, yFor (66.15), rowsRight, yFor (65.85)));
        g.setColour (theme::accent);
        g.drawHorizontalLine ((int) yFor (66.0), left, rowsRight);

        auto arrow = [&] (float x, double from, double to, juce::Colour c)
        {
            g.setColour (c);
            g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ x, yFor (from) }));
            g.drawArrow ({ x, yFor (from), x, yFor (to) }, 2.0f, 9.0f, 9.0f);
        };

        arrow (x1, 66.10, 67.0, tunepics::getZoneColour (tune::Zone::inTune));
        arrow (x2, 65.90, 65.0, tunepics::getZoneColour (tune::Zone::inTune));
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.5f));
        auto notes = rows.withLeft (rowsRight + 14.0f);
        g.drawFittedText ("F# sung 10 cents sharp (66.10)\n goes up to G", notes.removeFromTop (rowHeight * 3.2f).toNearestInt(), juce::Justification::centredLeft, 3);
        notes.removeFromTop (rowHeight * 1.0f);
        g.drawFittedText ("F# sung 10 cents flat (65.90)\ngoes down to F", notes.removeFromTop (rowHeight * 3.2f).toNearestInt(), juce::Justification::centredLeft, 3);
        g.setColour (theme::accent);
        g.drawFittedText ("The halfway point is 66.00. A note keeps its target until it is 15 cents past it: between 65.85 and 66.15 nothing flips.", notes.removeFromBottom (rowHeight * 3.6f).toNearestInt(), juce::Justification::centredLeft, 3);
    }

    void paintSpeed (juce::Graphics& g, juce::Rectangle<float> r, double sinceClap)
    {
        caption (g, r, "The same scooped E4 corrected at three speeds (Key: A minor, Amount 100%, Tolerance 10 cents)");
        auto legendArea = r.removeFromBottom (20.0f);
        const double speeds[] = { 0.0, 50.0, 200.0 };
        const float width = r.getWidth() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            auto panel = r.withWidth (width).withX (r.getX() + (float) i * width).reduced (3.0f, 0.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (juce::String ((int) speeds[i]) + " ms", panel.removeFromTop (20.0f), juce::Justification::centred);
            const auto frames = framesFor (tune::getScoopNotes(), 1.0, getSettingsFor (speeds[i], false), aMinorMask);
            paintFrames (g, panel, frames, aMinorMask, 57);
            drawCursor (g, panel.reduced (4.0f).withTrimmedLeft (tunepics::traceMargin), sinceClap, 1.0);
        }

        legend (g, legendArea);
    }

    void paintVibrato (juce::Graphics& g, juce::Rectangle<float> r, double sinceClap)
    {
        caption (g, r, "A note 30 cents flat with a 40-cent vibrato: Keep vibrato on (left) and off (right)");
        auto legendArea = r.removeFromBottom (20.0f);
        const float width = r.getWidth() * 0.5f;
        const struct { const char* title; double speed; bool keep; } cases[] = { { "Keep vibrato on, 50 ms", 50.0, true }, { "Keep vibrato off, 0 ms", 0.0, false } };

        for (int i = 0; i < 2; ++i)
        {
            auto panel = r.withWidth (width).withX (r.getX() + (float) i * width).reduced (3.0f, 0.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (cases[i].title, panel.removeFromTop (20.0f), juce::Justification::centred);
            const auto frames = framesFor (tune::getVibratoNotes(), 2.0, getSettingsFor (cases[i].speed, cases[i].keep), cMajorMask);
            paintFrames (g, panel, frames, cMajorMask, 62);
            drawCursor (g, panel.reduced (4.0f).withTrimmedLeft (tunepics::traceMargin), sinceClap, 2.0);
        }

        legend (g, legendArea);
    }

    void paintFormants (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "Harmonics (lines) under the shape of an \"ah\" (curve): the voice's colour is the curve");
        const struct { const char* title; double f0; double envelopeScale; } rows[] = {
            { "A vowel on 220 Hz", 220.0, 1.0 }, { "Played faster by an octave: everything moves, the curve too (peaks 1,460 and 2,180 Hz)", 440.0, 2.0 },
            { "PSOLA: the pitch moves, the curve stays (peaks 730 and 1,090 Hz)", 440.0, 1.0 } };
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
                g.setColour (tunepics::colour.brighter (0.3f));
                g.fillRect (xFor (hz) - 1.0f, plot.getBottom() - juce::jlimit (0.02f, 1.0f, level (hz)) * plot.getHeight(), 2.0f, juce::jlimit (0.02f, 1.0f, level (hz)) * plot.getHeight());
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

    void paintHard (juce::Graphics& g, juce::Rectangle<float> r, double sinceClap)
    {
        caption (g, r, "G4 with vibrato, a slide up to A4, A4: Hard tune (0 ms, Amount 100%, Tolerance 0, Keep vibrato off) in A minor");
        auto legendArea = r.removeFromBottom (20.0f);
        const auto frames = framesFor (tune::getHardTuneNotes(), 1.35, tune::getPresets()[3].settings, aMinorMask);
        paintFrames (g, r, frames, aMinorMask, 60);
        drawCursor (g, r.reduced (4.0f).withTrimmedLeft (tunepics::traceMargin), sinceClap, 1.35);
        legend (g, legendArea);
    }

    void paintChain (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A vocal track's chain: correction first, on the dry sound");
        const char* names[] = { "Vocal take", "DawIt Tune", "DawIt EQ", "DawIt Compressor", "Reverb send" };
        const char* notes[] = { "dry", "measure, then fix lightly", "shape the tone", "even out the level", "reverb after the tuner" };
        const float boxWidth = r.getWidth() / 5.0f - 20.0f;
        auto row = r.withSizeKeepingCentre (r.getWidth(), 120.0f).reduced (8.0f, 0.0f);

        for (int i = 0; i < 5; ++i)
        {
            auto box = row.withX (row.getX() + (float) i * (boxWidth + 20.0f)).withWidth (boxWidth).withHeight (56.0f).withY (row.getY() + 10.0f);
            g.setColour (i == 1 ? tunepics::colour.withAlpha (0.35f) : theme::panelLight);
            g.fillRoundedRectangle (box, 5.0f);
            g.setColour (i == 1 ? tunepics::colour : theme::gridBar);
            g.drawRoundedRectangle (box, 5.0f, 1.5f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawFittedText (names[i], box.toNearestInt(), juce::Justification::centred, 2);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText (notes[i], box.translated (0.0f, 62.0f).withHeight (32.0f).toNearestInt(), juce::Justification::centredTop, 2);

            if (i < 4)
                g.drawArrow ({ box.getRight() + 2.0f, box.getCentreY(), box.getRight() + 18.0f, box.getCentreY() }, 1.6f, 8.0f, 8.0f);
        }
    }

    //==============================================================================
    struct MadeUp
    {
        std::vector<tune::Frame> log;
        SongInfo song;
    };

    /** A made-up take for the picture and the text when nothing has been heard yet: a flat verse and a chorus with two F sharps. */
    MadeUp makeMadeUpTake()
    {
        MadeUp m;
        m.song.key = { 0, MusicalKey::Scale::major };
        m.song.sections = { { 0, 8, SectionType::verse, {} }, { 8, 8, SectionType::chorus, {} } };
        m.song.numBars = 16;
        const int pitches[] = { 60, 64, 62, 65, 67, 64, 62, 60 };

        auto add = [&m] (double beat, int midi, double cents)
        {
            for (double b = beat; b < beat + 3.0; b += 0.0125)
            {
                tune::Frame f;
                f.beat = (float) b;
                f.sung = (float) (midi + cents / 100.0 + 0.1 * std::sin (b * 20.0));
                f.corrected = f.sung;
                f.rmsDb = -20.0f;
                m.log.push_back (f);
            }
        };

        for (int i = 0; i < 8; ++i)
            add (i * 4.0, pitches[i], -18.0 + (i % 3) * 4.0 - 4.0);

        for (int i = 0; i < 8; ++i)
            add (32.0 + i * 3.8, (i == 2 || i == 5) ? 66 : pitches[i], i % 2 == 0 ? 6.0 : -4.0);

        return m;
    }

    void paintYourTrack (juce::Graphics& g, juce::Rectangle<float> r, BuiltInTune& processor)
    {
        auto song = getSong();
        auto log = processor.getTakeLog();
        auto analysis = gen::tunecheck::analyse (log, song.tempo, song.key, song.sections);
        const bool madeUp = analysis.notes.size() < 8;
        const auto host = fxui::getHostTrack (processor);

        if (madeUp)
        {
            auto m = makeMadeUpTake();
            song = m.song;
            analysis = gen::tunecheck::analyse (m.log, song.tempo, song.key, song.sections);
        }

        caption (g, r, (madeUp ? juce::String ("A made-up take, until this track has played through DawIt Tune") : (host.name.isEmpty() ? juce::String ("The take DawIt Tune heard") : host.name + ": the take DawIt Tune heard")));
        auto legendArea = r.removeFromBottom (20.0f);
        legend (g, legendArea);

        const int bars = juce::jmax (song.numBars, analysis.notes.empty() ? 8 : analysis.notes.back().bar + 2);
        auto plotArea = r.reduced (8.0f, 0.0f);
        auto margin = plotArea.removeFromLeft (34.0f);
        auto bands = plotArea.removeFromTop (22.0f);
        const auto bottomBars = plotArea.removeFromBottom (plotArea.getHeight() * 0.32f);
        auto xFor = [&] (double beat) { return plotArea.getX() + (float) (beat / 4.0 / bars) * plotArea.getWidth(); };

        // The sections.
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

        // The notes on a pitch axis, coloured by how far out they are; an outline for a note outside the key.
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plotArea.expanded (0.0f, 2.0f), 4.0f);
        int low = 127, high = 0;

        for (const auto& n : analysis.notes)
        {
            low = juce::jmin (low, n.midi);
            high = juce::jmax (high, n.midi);
        }

        low = juce::jmax (0, low - 2);
        high = juce::jmax (low + 8, high + 2);
        const float rowHeight = plotArea.getHeight() / (float) (high - low + 1);
        auto yFor = [&] (double pitch) { return plotArea.getBottom() - (float) (pitch - low + 0.5) * rowHeight; };

        for (int note = low; note <= high; ++note)
        {
            const bool in = (song.key.contains (note));
            g.setColour (in ? theme::panelLight.withAlpha (0.5f) : theme::background.withAlpha (0.3f));
            g.fillRect (juce::Rectangle<float> (plotArea.getX(), yFor (note) - rowHeight * 0.5f + 0.5f, plotArea.getWidth(), rowHeight - 1.0f));

            if ((high - low) < 24 || note % 12 == 0)
            {
                g.setColour (in ? theme::text : theme::textDim.withAlpha (0.5f));
                g.setFont (juce::FontOptions (9.5f));
                g.drawText (tunepics::getNoteName (note), juce::Rectangle<float> (margin.getX(), yFor (note) - 7.0f, margin.getWidth() - 3.0f, 14.0f), juce::Justification::centredRight);
            }
        }

        for (const auto& n : analysis.notes)
        {
            const auto box = juce::Rectangle<float>::leftTopRightBottom (xFor (n.startBeat), yFor (n.midi + n.errorCents / 100.0) - 2.5f, juce::jmax (xFor (n.endBeat), xFor (n.startBeat) + 3.0f), yFor (n.midi + n.errorCents / 100.0) + 2.5f);
            g.setColour (tunepics::getZoneColour (tune::getZone (n.errorCents)));
            g.fillRoundedRectangle (box, 2.0f);

            if (! n.inKey)
            {
                g.setColour (theme::text);
                g.drawRoundedRectangle (box.expanded (2.0f), 3.0f, 1.2f);
            }
        }

        // Each section's average, as a bar from the middle: sharp up, flat down (the scale runs to 40 cents).
        auto bars2 = bottomBars.reduced (0.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (bars2.expanded (0.0f, 4.0f), 4.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("average", juce::Rectangle<float> (margin.getX() - 4.0f, bars2.getCentreY() - 7.0f, margin.getWidth() + 2.0f, 14.0f), juce::Justification::centredRight);
        g.setColour (theme::gridBar);
        g.fillRect (bars2.getX(), bars2.getCentreY() - 0.5f, bars2.getWidth(), 1.0f);

        for (const auto& s : analysis.sections)
        {
            const auto& section = song.sections[(size_t) s.section];
            const float x0 = xFor (section.startBar * 4.0), x1 = xFor (section.getEndBar() * 4.0);
            const float h = juce::jlimit (-1.0f, 1.0f, (float) (s.meanCents / 40.0)) * bars2.getHeight() * 0.5f;
            g.setColour (tunepics::getZoneColour (tune::getZone (s.meanCents)));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0 + (x1 - x0) * 0.3f, h < 0.0f ? bars2.getCentreY() : bars2.getCentreY() - h, x0 + (x1 - x0) * 0.7f, h < 0.0f ? bars2.getCentreY() - h : bars2.getCentreY()));
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.drawText ((s.meanCents > 0.0 ? "+" : "") + juce::String (juce::roundToInt (s.meanCents)) + " cents", juce::Rectangle<float> (x0, h < 0.0f ? bars2.getY() : bars2.getBottom() - 14.0f, x1 - x0, 14.0f), juce::Justification::centred);
        }
    }

    //==============================================================================
    juce::String getYourTrackText (BuiltInTune& processor)
    {
        const auto song = getSong();
        const auto lines = gen::tunecheck::getAdvice (processor.getTakeLog(), song.tempo, song.key, song.sections, processor.getSettings().mode == tune::Mode::correct);
        juce::StringArray all;

        for (const auto& l : lines)
            all.add (l);

        return gen::tuneguide::getPageText (9) + "\n\n" + all.joinIntoString ("\n\n");
    }
} // namespace

void setSongAccess (SongAccess a) { access = std::move (a); }

SongInfo getSong() { return access.getSong ? access.getSong() : SongInfo(); }

void seekToBeat (double beat)
{
    if (access.seekToBeat)
        access.seekToBeat (beat);
}

juce::PropertiesFile* getSettings() { return access.getSettings ? access.getSettings() : nullptr; }

std::vector<fxui::Guide::Page> makeGuide (BuiltInTune& processor)
{
    auto* p = &processor;
    std::vector<fxui::Guide::Page> pages;

    for (int i = 0; i < gen::tuneguide::numPages; ++i)
        pages.push_back ({ gen::tuneguide::getPageName (i), gen::tuneguide::getPageSummary (i), gen::tuneguide::getPageText (i), {} });

    pages[0].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double t) { paintWhat (g, r, t); };
    pages[1].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintCents (g, r); };
    pages[2].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintFinding (g, r); };
    pages[3].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintTargets (g, r); };
    pages[4].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double t) { paintSpeed (g, r, t); };
    pages[5].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double t) { paintVibrato (g, r, t); };
    pages[6].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintFormants (g, r); };
    pages[7].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double t) { paintHard (g, r, t); };
    pages[8].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintChain (g, r); };
    pages[9].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintYourTrack (g, r, *p); };
    pages[9].liveText = [p] { return getYourTrackText (*p); };
    return pages;
}
} // namespace daw::tunetext
