#include "DrumTriggerExplainer.h"

#include <cmath>

#include "engine/BuiltInDrums.h"
#include "engine/BuiltInSynth.h"
#include "engine/HitsToMidi.h"
#include "HitsToMidiWindow.h"

namespace daw
{
namespace tg = triggerguide;
namespace ah = analysis::hits;

namespace
{
constexpr double exampleRate = 44100.0;

/** One hit of the example loop in seconds. */
struct Stroke
{
    double time = 0.0;
    ah::Drum drum = ah::Drum::kick;
    double levelDb = 0.0;
    bool bounce = false;   // the kick bounce that the minimum gap merges
};

/** The example loop with what the pictures need from it, made once. */
struct Example
{
    Example()
    {
        audio = hitstomidi::renderExampleLoop (exampleRate);

        for (const auto& h : hitstomidi::getExampleLoop())
        {
            Stroke s;
            s.time = h.beat * 0.5 + h.lateMs / 1000.0;
            s.drum = h.drum;
            s.levelDb = h.levelDb;
            s.bounce = h.drum == ah::Drum::kick && h.levelDb < -10.0 && h.levelDb > -14.0;
            strokes.push_back (s);
        }

        // The loop's level every 5 ms (RMS over 10 ms) in dB against its loudest part.
        const int hop = (int) (0.005 * exampleRate), window = (int) (0.010 * exampleRate), n = audio.getNumSamples();
        std::vector<float> rms;

        for (int start = 0; start + window <= n; start += hop)
            rms.push_back (audio.getRMSLevel (0, start, window));

        const float loudest = rms.empty() ? 1.0f : juce::jmax (1.0e-6f, *std::max_element (rms.begin(), rms.end()));

        for (auto r : rms)
            levelDb.push_back (juce::jmax (-60.0f, juce::Decibels::gainToDecibels (r / loudest, -60.0f)));

        // The spectra of the first kick, snare and hi-hat: the 2048 samples from the hit minus the 2048 before it.
        for (auto drum : { ah::Drum::kick, ah::Drum::snare, ah::Drum::hihat })
            for (auto& s : strokes)
                if (s.drum == drum && ! s.bounce)
                {
                    spectra[(size_t) drum] = getSpectrum ((int) (s.time * exampleRate));
                    break;
                }

        for (auto& sp : spectra)
        {
            float top = 1.0e-12f;

            for (auto v : sp)
                top = juce::jmax (top, v);

            for (auto& v : sp)
                v = juce::jmax (-50.0f, juce::Decibels::gainToDecibels (v / top, -50.0f));
        }

        for (size_t d = 0; d < 3; ++d)
        {
            double low = 0.0, mid = 0.0, high = 0.0;

            for (int i = 0; i < spectrumPoints; ++i)
            {
                const double f = getFrequency (i), p = std::pow (10.0, spectra[d][(size_t) i] / 10.0) * f;
                (f < 150.0 ? low : f < 6000.0 ? mid : high) += p;
            }

            const double total = juce::jmax (1.0e-12, low + mid + high);
            shares[d] = { (int) std::lround (100.0 * low / total), (int) std::lround (100.0 * mid / total), (int) std::lround (100.0 * high / total) };
        }
    }

    static constexpr int spectrumPoints = 120;

    static double getFrequency (int i) { return 30.0 * std::pow (11000.0 / 30.0, (double) i / (spectrumPoints - 1)); }

    /** Power at the log-spaced frequencies of the rise at `at`: the window from there minus the one before it. */
    std::vector<float> getSpectrum (int at) const
    {
        const int size = 2048;
        auto power = [&] (int start)
        {
            std::vector<float> out ((size_t) spectrumPoints, 0.0f);

            if (start < 0 || start + size > audio.getNumSamples())
                return out;

            const auto* x = audio.getReadPointer (0);

            for (int i = 0; i < spectrumPoints; ++i)
            {
                const double w = juce::MathConstants<double>::twoPi * getFrequency (i) / exampleRate;
                double re = 0.0, im = 0.0;

                for (int k = 0; k < size; ++k)
                {
                    const double hann = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * k / (size - 1));
                    re += x[start + k] * hann * std::cos (w * k);
                    im -= x[start + k] * hann * std::sin (w * k);
                }

                out[(size_t) i] = (float) (re * re + im * im);
            }

            return out;
        };

        auto after = power (at);
        const auto before = power (at - size);

        for (size_t i = 0; i < after.size(); ++i)
            after[i] = juce::jmax (0.0f, after[i] - before[i]);

        return after;
    }

    juce::AudioBuffer<float> audio;
    std::vector<Stroke> strokes;
    std::vector<float> levelDb;
    std::array<std::vector<float>, 3> spectra;
    std::array<std::array<int, 3>, 3> shares {};
};

const Example& getExample()
{
    static const Example example;
    return example;
}

void drawLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, float size, juce::Colour colour,
                juce::Justification just = juce::Justification::centred, bool bold = false)
{
    g.setColour (colour);
    g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText (text, r.toNearestInt(), just, 2, 0.8f);
}

juce::String drumName (ah::Drum d)
{
    switch (d)
    {
        case ah::Drum::kick:  return "Kick";
        case ah::Drum::snare: return "Snare";
        case ah::Drum::hihat: return "Hi-hat";
    }

    return {};
}

juce::String dbText (double db)
{
    return (db < 0.0 ? juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")) : juce::String()) + juce::String (std::abs (db), 0) + " dB";
}

/** Draws the three drum lanes of a piece: lane names on the left, a bar for each hit (beats from the start). */
void drawLanes (juce::Graphics& g, juce::Rectangle<float> r, double lengthBeats, const std::vector<std::pair<double, ah::Drum>>& hits)
{
    const float labelWidth = 56.0f;
    auto lanes = r.withTrimmedLeft (labelWidth);
    const float laneHeight = r.getHeight() / 3.0f;
    const ah::Drum order[] { ah::Drum::hihat, ah::Drum::snare, ah::Drum::kick };

    for (int i = 0; i < 3; ++i)
    {
        const auto lane = juce::Rectangle<float> (lanes.getX(), r.getY() + (float) i * laneHeight, lanes.getWidth(), laneHeight);
        g.setColour (i % 2 == 0 ? theme::lane : theme::laneAlt);
        g.fillRect (lane.reduced (0.0f, 0.5f));
        drawLabel (g, { r.getX(), lane.getY(), labelWidth - 6.0f, laneHeight }, drumName (order[i]), 11.5f, HitsToMidiWindow::getDrumColour (order[i]),
                   juce::Justification::centredRight, true);
    }

    for (int beat = 0; beat <= (int) lengthBeats; ++beat)
    {
        const float x = lanes.getX() + (float) (beat / lengthBeats) * lanes.getWidth();
        g.setColour (beat % 4 == 0 ? theme::gridBar : theme::grid);
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    }

    for (auto& [beat, drum] : hits)
    {
        int row = 0;

        for (int i = 0; i < 3; ++i)
            if (order[i] == drum)
                row = i;

        const float x = lanes.getX() + (float) (beat / lengthBeats) * lanes.getWidth();
        g.setColour (HitsToMidiWindow::getDrumColour (drum));
        g.fillRoundedRectangle (x - 1.5f, r.getY() + (float) row * laneHeight + 3.0f, 3.0f, laneHeight - 6.0f, 1.0f);
    }
}
} // namespace

//==============================================================================
/** The picture of a topic. */
class DrumTriggerExplainer::Picture final : public juce::Component,
                                            public juce::SettableTooltipClient
{
public:
    /** What the pictures take from the user: their Hits to MIDI settings and the part they are looking at. */
    struct State
    {
        double sensitivityDb = -30.0;
        double gapMs = 30.0;
        int curve = 0;
        std::optional<tg::Part> part;
    };

    std::function<void (float)> onClick;   // x as a fraction of the picture, for what it plays

    void setState (tg::Topic t, State s)
    {
        topic = t;
        state = std::move (s);
        setMouseCursor (isPlayable() ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }

    bool isPlayable() const
    {
        return topic == tg::Topic::whatItIs || topic == tg::Topic::transients || topic == tg::Topic::findingHits || topic == tg::Topic::replaceOrLayer;
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (isPlayable() && onClick != nullptr && getLocalBounds().contains (e.getPosition()))
            onClick ((float) e.x / (float) juce::jmax (1, getWidth()));
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        switch (topic)
        {
            case tg::Topic::whatItIs:        paintWhatItIs (g); break;
            case tg::Topic::transients:      paintTransient (g); break;
            case tg::Topic::findingHits:     paintFinding (g); break;
            case tg::Topic::velocity:        paintVelocity (g); break;
            case tg::Topic::whichDrum:       paintSpectra (g); break;
            case tg::Topic::falseHits:       paintFalseHits (g); break;
            case tg::Topic::grid:            paintGrid (g); break;
            case tg::Topic::replaceOrLayer:  paintPhase (g); break;
            case tg::Topic::yourClip:        paintYourClip (g); break;
        }
    }

private:
    juce::Rectangle<float> getArea() const { return getLocalBounds().toFloat().reduced (16.0f, 12.0f); }

    void paintWhatItIs (juce::Graphics& g) const
    {
        const auto& ex = getExample();
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "Two bars of drums at 120 BPM: the recording, a marker on every hit, and the notes they become (click to listen)",
                   12.0f, theme::textDim, juce::Justification::centredLeft);
        area.removeFromTop (4.0f);
        auto wave = area.removeFromTop (area.getHeight() * 0.48f);
        auto lanes = area.reduced (0.0f, 6.0f);
        const float labelWidth = 56.0f;
        const auto waveArea = wave.withTrimmedLeft (labelWidth);
        g.setColour (theme::lane);
        g.fillRect (waveArea);

        // The waveform: the loudest sample in each column.
        const int columns = (int) waveArea.getWidth(), n = ex.audio.getNumSamples();
        g.setColour (theme::accent.withAlpha (0.7f));

        for (int c = 0; c < columns; ++c)
        {
            const int start = (int) ((double) c / columns * n), count = juce::jmax (1, (int) ((double) n / columns));
            const float peak = juce::jmin (1.0f, ex.audio.getMagnitude (0, start, juce::jmin (count, n - start)) * 1.4f);
            const float h = peak * waveArea.getHeight() * 0.5f;
            g.drawVerticalLine ((int) waveArea.getX() + c, waveArea.getCentreY() - h, waveArea.getCentreY() + h);
        }

        std::vector<std::pair<double, ah::Drum>> notes;

        for (auto& s : ex.strokes)
            if (! s.bounce)
            {
                const float x = waveArea.getX() + (float) (s.time / 4.0) * waveArea.getWidth();
                g.setColour (HitsToMidiWindow::getDrumColour (s.drum));
                g.drawVerticalLine ((int) x, waveArea.getY(), waveArea.getBottom());
                notes.push_back ({ s.time * 2.0, s.drum });
            }

        drawLabel (g, { wave.getX(), wave.getY(), labelWidth - 6.0f, wave.getHeight() }, "Audio", 11.5f, theme::textDim, juce::Justification::centredRight, true);
        drawLanes (g, lanes, 8.0, notes);
    }

    /** The first 60 ms of the example snare hit, the peak marked, the marker at the start of the rise. */
    void paintTransient (juce::Graphics& g) const
    {
        const auto& ex = getExample();
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "The first 60 ms of a snare hit (click to listen): a sudden rise, then a slower fall", 12.0f, theme::textDim,
                   juce::Justification::centredLeft);
        area.removeFromTop (6.0f);

        double hitTime = 0.5;

        for (auto& s : ex.strokes)
            if (s.drum == ah::Drum::snare)
            {
                hitTime = s.time;
                break;
            }

        const int begin = (int) ((hitTime - 0.005) * exampleRate), length = (int) (0.060 * exampleRate);
        const auto* x = ex.audio.getReadPointer (0);
        float peak = 0.001f, maxDiff = 0.0001f;
        int peakAt = 0;

        for (int i = 0; i < length; ++i)
        {
            if (std::abs (x[begin + i]) > peak)
            {
                peak = std::abs (x[begin + i]);
                peakAt = i;
            }

            if (i > 0)
                maxDiff = juce::jmax (maxDiff, std::abs (x[begin + i] - x[begin + i - 1]));
        }

        int riseAt = 0;

        for (int i = 1; i < length; ++i)
            if (std::abs (x[begin + i] - x[begin + i - 1]) >= 0.1f * maxDiff)
            {
                riseAt = i;
                break;
            }

        auto plot = area.withTrimmedBottom (22.0f);
        g.setColour (theme::lane);
        g.fillRect (plot);
        g.setColour (theme::gridBar);
        g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

        juce::Path wave;
        const int columns = (int) plot.getWidth();

        for (int c = 0; c < columns; ++c)
        {
            const int a = (int) ((double) c / columns * length), b = juce::jmax (a + 1, (int) ((double) (c + 1) / columns * length));
            float lo = 0.0f, hi = 0.0f;

            for (int i = a; i < b && i < length; ++i)
            {
                lo = juce::jmin (lo, x[begin + i]);
                hi = juce::jmax (hi, x[begin + i]);
            }

            const float px = plot.getX() + (float) c;
            wave.addLineSegment (juce::Line<float> (px, plot.getCentreY() - hi / peak * plot.getHeight() * 0.46f, px, plot.getCentreY() - lo / peak * plot.getHeight() * 0.46f), 1.0f);
        }

        g.setColour (HitsToMidiWindow::getDrumColour (ah::Drum::snare));
        g.fillPath (wave);

        auto xAt = [&] (int sample) { return plot.getX() + (float) sample / (float) length * plot.getWidth(); };
        g.setColour (theme::playhead);
        g.drawVerticalLine ((int) xAt (riseAt), plot.getY(), plot.getBottom());
        drawLabel (g, { xAt (riseAt) - 150.0f, plot.getY() + 4.0f, 144.0f, 32.0f }, "marker: the start of the rise", 12.0f, theme::playhead, juce::Justification::centredRight);

        const float peakY = plot.getCentreY() - x[begin + peakAt] / peak * plot.getHeight() * 0.46f;
        g.setColour (theme::meterYellow);
        g.fillEllipse (xAt (peakAt) - 4.0f, peakY - 4.0f, 8.0f, 8.0f);
        drawLabel (g, { xAt (peakAt) + 8.0f, peakY - 26.0f, 120.0f, 20.0f }, "the peak", 12.0f, theme::meterYellow, juce::Justification::centredLeft);

        const float riseMs = (float) (peakAt - riseAt) / (float) exampleRate * 1000.0f;
        drawLabel (g, area.removeFromBottom (20.0f), "0 ms at the left edge, 60 ms at the right. The peak comes " + juce::String (riseMs, 1) + " ms after the marker.",
                   12.0f, theme::textDim, juce::Justification::centredLeft);
    }

    /** The loop's level in dB with the user's Sensitivity line, the hits as dots, the gap after the first kick of bar 2. */
    void paintFinding (juce::Graphics& g) const
    {
        const auto& ex = getExample();
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "The loop's level, hit by hit (click to listen). Dots above the line are found; hollow ones are too quiet.", 12.0f, theme::textDim,
                   juce::Justification::centredLeft);
        area.removeFromTop (4.0f);
        const float axisWidth = 40.0f;
        auto plot = area.withTrimmedLeft (axisWidth).withTrimmedBottom (20.0f);
        g.setColour (theme::lane);
        g.fillRect (plot);

        auto yAt = [&] (double db) { return plot.getY() + (float) (-juce::jlimit (-60.0, 0.0, db) / 60.0) * plot.getHeight(); };
        auto xAt = [&] (double seconds) { return plot.getX() + (float) (seconds / 4.0) * plot.getWidth(); };

        for (int db = 0; db >= -60; db -= 12)
        {
            g.setColour (theme::grid);
            g.drawHorizontalLine ((int) yAt (db), plot.getX(), plot.getRight());
            drawLabel (g, { area.getX(), yAt (db) - 8.0f, axisWidth - 4.0f, 16.0f }, dbText (db), 10.5f, theme::textDim, juce::Justification::centredRight);
        }

        // The gap after the kick that the bounce follows (the second bar's first beat, 2.0 s).
        const double gapStart = 2.0;
        g.setColour (theme::meterYellow.withAlpha (0.25f));
        g.fillRect (xAt (gapStart), plot.getY(), juce::jmax (2.0f, xAt (gapStart + state.gapMs / 1000.0) - xAt (gapStart)), plot.getHeight());
        drawLabel (g, { xAt (gapStart) + 2.0f, plot.getBottom() - 46.0f, 190.0f, 40.0f },
                   "gap " + juce::String ((int) state.gapMs) + " ms: a bounce 12 ms after this kick is not a new hit", 10.5f, theme::meterYellow, juce::Justification::centredLeft);

        juce::Path curve;
        curve.startNewSubPath (plot.getX(), plot.getBottom());

        for (size_t i = 0; i < ex.levelDb.size(); ++i)
            curve.lineTo (xAt ((double) i * 0.005), yAt (ex.levelDb[i]));

        curve.lineTo (plot.getRight(), plot.getBottom());
        curve.closeSubPath();
        g.setColour (theme::accent.withAlpha (0.28f));
        g.fillPath (curve);
        g.setColour (theme::accent.withAlpha (0.8f));
        g.strokePath (curve, juce::PathStrokeType (1.0f));

        for (auto& s : ex.strokes)
        {
            const bool counted = s.levelDb >= state.sensitivityDb && ! (s.bounce && state.gapMs > 12.0);
            const auto colour = HitsToMidiWindow::getDrumColour (s.drum);
            const float px = xAt (s.time), py = yAt (s.levelDb);
            g.setColour (colour);

            if (counted)
                g.fillEllipse (px - 4.5f, py - 4.5f, 9.0f, 9.0f);
            else
                g.drawEllipse (px - 4.0f, py - 4.0f, 8.0f, 8.0f, 1.5f);
        }

        g.setColour (theme::playhead);
        g.drawHorizontalLine ((int) yAt (state.sensitivityDb), plot.getX(), plot.getRight());
        drawLabel (g, { plot.getRight() - 300.0f, yAt (state.sensitivityDb) - 18.0f, 296.0f, 16.0f },
                   "Sensitivity " + dbText (state.sensitivityDb) + " (your setting in Hits to MIDI)", 11.5f, theme::playhead, juce::Justification::centredRight);
        drawLabel (g, area.removeFromBottom (18.0f), "0 s at the left edge, 4 s at the right. Kick, snare and hi-hat dots use the drum colours.", 11.5f, theme::textDim,
                   juce::Justification::centredLeft);
    }

    /** The four velocity curves from 0 to -40 dB, the chosen one bright, with the example points. */
    void paintVelocity (juce::Graphics& g) const
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "How loud a hit was (left to right: 0 to 40 dB below the loudest) and the velocity it gets", 12.0f, theme::textDim,
                   juce::Justification::centredLeft);
        area.removeFromTop (6.0f);
        const float axisWidth = 40.0f;
        auto plot = area.withTrimmedLeft (axisWidth).withTrimmedBottom (22.0f).withTrimmedRight (110.0f);
        g.setColour (theme::lane);
        g.fillRect (plot);

        auto xAt = [&] (double db) { return plot.getX() + (float) (db / 40.0) * plot.getWidth(); };
        auto yAt = [&] (double v) { return plot.getBottom() - (float) (v / 127.0) * plot.getHeight(); };

        for (int v : { 1, 32, 64, 96, 127 })
        {
            g.setColour (theme::grid);
            g.drawHorizontalLine ((int) yAt (v), plot.getX(), plot.getRight());
            drawLabel (g, { area.getX(), yAt (v) - 8.0f, axisWidth - 4.0f, 16.0f }, juce::String (v), 10.5f, theme::textDim, juce::Justification::centredRight);
        }

        for (int db = 0; db <= 40; db += 10)
            drawLabel (g, { xAt (db) - 30.0f, plot.getBottom() + 3.0f, 60.0f, 16.0f }, db == 0 ? "0 dB" : dbText (-db), 10.5f, theme::textDim);

        const ah::VelocityCurve curves[] { ah::VelocityCurve::natural, ah::VelocityCurve::gentle, ah::VelocityCurve::strong, ah::VelocityCurve::same };
        const char* names[] { "Natural", "Gentle", "Strong", "Same for all" };
        const juce::Colour colours[] { theme::accent, theme::meterGreen, juce::Colour (0xfff5a04a), theme::textDim };
        const int chosen = juce::jlimit (0, 3, state.curve);

        for (int pass = 0; pass < 2; ++pass)
            for (int c = 0; c < 4; ++c)
            {
                if ((c == chosen) != (pass == 1))
                    continue;

                juce::Path path;

                for (int db = 0; db <= 40; ++db)
                {
                    const auto p = juce::Point<float> (xAt (db), yAt (tg::getCurveVelocity (curves[c], db)));

                    if (db == 0)
                        path.startNewSubPath (p);
                    else
                        path.lineTo (p);
                }

                g.setColour (colours[c].withAlpha (c == chosen ? 1.0f : 0.4f));
                g.strokePath (path, juce::PathStrokeType (c == chosen ? 3.0f : 1.5f));
            }

        for (int c = 0; c < 4; ++c)
        {
            auto row = juce::Rectangle<float> (plot.getRight() + 12.0f, plot.getY() + 6.0f + (float) c * 22.0f, 100.0f, 18.0f);
            g.setColour (colours[c].withAlpha (c == chosen ? 1.0f : 0.5f));
            g.fillRect (row.removeFromLeft (14.0f).withSizeKeepingCentre (14.0f, 4.0f));
            drawLabel (g, row.withTrimmedLeft (4.0f), names[c], 11.5f, c == chosen ? theme::text : theme::textDim, juce::Justification::centredLeft, c == chosen);
        }

        for (int db : { 0, 6, 12, 20, 30, 40 })
        {
            const int v = tg::getCurveVelocity (curves[chosen], db);
            const float px = xAt (db), py = yAt (v);
            g.setColour (colours[chosen]);
            g.fillEllipse (px - 4.0f, py - 4.0f, 8.0f, 8.0f);
            drawLabel (g, { px - 18.0f, py - 24.0f, 36.0f, 16.0f }, juce::String (v), 11.0f, theme::text, juce::Justification::centred, true);
        }
    }

    /** The spectra of the example kick, snare and hi-hat with the borders at 150 Hz and 6 kHz and each band's share. */
    void paintSpectra (juce::Graphics& g) const
    {
        const auto& ex = getExample();
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "Where the energy each hit adds sits, 30 Hz to 11 kHz (the borders are at 150 Hz and 6 kHz)", 12.0f, theme::textDim,
                   juce::Justification::centredLeft);
        area.removeFromTop (4.0f);
        auto shareArea = area.removeFromBottom (54.0f);
        auto plot = area.withTrimmedBottom (20.0f);
        g.setColour (theme::lane);
        g.fillRect (plot);

        auto xAt = [&] (double hz) { return plot.getX() + (float) (std::log (hz / 30.0) / std::log (11000.0 / 30.0)) * plot.getWidth(); };
        auto yAt = [&] (double db) { return plot.getY() + (float) (-juce::jlimit (-50.0, 0.0, db) / 50.0) * plot.getHeight(); };

        for (double hz : { 50.0, 100.0, 500.0, 1000.0, 5000.0, 10000.0 })
        {
            g.setColour (theme::grid);
            g.drawVerticalLine ((int) xAt (hz), plot.getY(), plot.getBottom());
            drawLabel (g, { xAt (hz) - 25.0f, plot.getBottom() + 2.0f, 50.0f, 16.0f }, hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + " kHz" : juce::String ((int) hz) + " Hz",
                       10.5f, theme::textDim);
        }

        for (size_t d = 0; d < 3; ++d)
        {
            juce::Path path;

            for (int i = 0; i < Example::spectrumPoints; ++i)
            {
                const auto p = juce::Point<float> (xAt (Example::getFrequency (i)), yAt (ex.spectra[d][(size_t) i]));

                if (i == 0)
                    path.startNewSubPath (p);
                else
                    path.lineTo (p);
            }

            g.setColour (HitsToMidiWindow::getDrumColour ((ah::Drum) d));
            g.strokePath (path, juce::PathStrokeType (2.0f));
        }

        for (double border : { 150.0, 6000.0 })
        {
            g.setColour (theme::text.withAlpha (0.7f));
            const float x = xAt (border);

            for (float y = plot.getY(); y < plot.getBottom(); y += 8.0f)
                g.drawVerticalLine ((int) x, y, juce::jmin (y + 4.0f, plot.getBottom()));
        }

        const float w = shareArea.getWidth() / 3.0f;

        for (size_t d = 0; d < 3; ++d)
        {
            const auto cell = juce::Rectangle<float> (shareArea.getX() + (float) d * w, shareArea.getY(), w, shareArea.getHeight()).reduced (4.0f, 2.0f);
            const auto& s = ex.shares[d];
            drawLabel (g, cell.withHeight (18.0f), drumName ((ah::Drum) d), 12.5f, HitsToMidiWindow::getDrumColour ((ah::Drum) d), juce::Justification::centredLeft, true);
            drawLabel (g, cell.withTrimmedTop (18.0f), juce::String (s[0]) + "% below 150 Hz, " + juce::String (s[1]) + "% from 150 Hz to 6 kHz, " + juce::String (s[2]) + "% above 6 kHz",
                       11.5f, theme::text, juce::Justification::topLeft);
        }
    }

    /** The snare track with bleed and ghost notes, and which strokes each Sensitivity line keeps. */
    void paintFalseHits (juce::Graphics& g) const
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "A snare recording with ghost notes and kick bleed: the taller the stroke, the louder", 12.0f, theme::textDim,
                   juce::Justification::centredLeft);
        area.removeFromTop (4.0f);
        const float labelWidth = 110.0f;
        auto keepArea = area.removeFromBottom (78.0f);
        auto plot = area.withTrimmedLeft (labelWidth).withTrimmedBottom (4.0f);
        g.setColour (theme::lane);
        g.fillRect (plot);

        auto xAt = [&] (double beat) { return plot.getX() + 16.0f + (float) (beat / 4.0) * (plot.getWidth() - 32.0f); };
        auto yAt = [&] (double db) { return plot.getY() + 6.0f + (float) (-juce::jlimit (-40.0, 0.0, db) / 40.0) * (plot.getHeight() - 12.0f); };
        const double lines[] { -30.0, -24.0, -18.0 };
        const juce::Colour lineColours[] { theme::meterGreen, theme::meterYellow, theme::meterRed };

        for (int i = 0; i < 3; ++i)
        {
            g.setColour (lineColours[i].withAlpha (0.8f));
            g.drawHorizontalLine ((int) yAt (lines[i]), plot.getX(), plot.getRight());
            drawLabel (g, { area.getX(), yAt (lines[i]) - 9.0f, labelWidth - 6.0f, 18.0f }, dbText (lines[i]) + ": " + juce::String (tg::countKept (lines[i])) + " kept", 11.5f,
                       lineColours[i], juce::Justification::centredRight, true);
        }

        const auto& track = tg::getSnareTrack();

        for (auto& s : track)
        {
            const juce::Colour colour = s.kind == tg::SnareStroke::Kind::backbeat ? HitsToMidiWindow::getDrumColour (ah::Drum::snare)
                                      : s.kind == tg::SnareStroke::Kind::ghost    ? HitsToMidiWindow::getDrumColour (ah::Drum::snare).withAlpha (0.55f)
                                                                                  : HitsToMidiWindow::getDrumColour (ah::Drum::kick);
            g.setColour (colour);
            g.fillRect (xAt (s.beat) - 3.0f, yAt (s.levelDb), 6.0f, plot.getBottom() - yAt (s.levelDb));

            // Under it, a dot for each line that keeps it.
            for (int i = 0; i < 3; ++i)
            {
                const float cy = keepArea.getY() + 12.0f + (float) i * 22.0f;
                g.setColour (lineColours[i].withAlpha (s.levelDb >= lines[i] ? 1.0f : 0.18f));
                g.fillEllipse (xAt (s.beat) - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            }
        }

        for (int i = 0; i < 3; ++i)
            drawLabel (g, { keepArea.getX(), keepArea.getY() + 3.0f + (float) i * 22.0f, labelWidth - 6.0f, 18.0f }, "kept at " + dbText (lines[i]), 11.0f, theme::textDim,
                       juce::Justification::centredRight);

        g.setColour (HitsToMidiWindow::getDrumColour (ah::Drum::snare));
        g.fillRect (plot.getX() + 8.0f, keepArea.getBottom() - 12.0f, 10.0f, 8.0f);
        drawLabel (g, { plot.getX() + 22.0f, keepArea.getBottom() - 16.0f, 90.0f, 16.0f }, "backbeat", 11.0f, theme::textDim, juce::Justification::centredLeft);
        g.setColour (HitsToMidiWindow::getDrumColour (ah::Drum::snare).withAlpha (0.55f));
        g.fillRect (plot.getX() + 120.0f, keepArea.getBottom() - 12.0f, 10.0f, 8.0f);
        drawLabel (g, { plot.getX() + 134.0f, keepArea.getBottom() - 16.0f, 120.0f, 16.0f }, "ghost note", 11.0f, theme::textDim, juce::Justification::centredLeft);
        g.setColour (HitsToMidiWindow::getDrumColour (ah::Drum::kick));
        g.fillRect (plot.getX() + 260.0f, keepArea.getBottom() - 12.0f, 10.0f, 8.0f);
        drawLabel (g, { plot.getX() + 274.0f, keepArea.getBottom() - 16.0f, 160.0f, 16.0f }, "kick bleeding through", 11.0f, theme::textDim, juce::Justification::centredLeft);
    }

    /** Eight played hits on a 1/16 grid, at 0%, 50% and 100%. */
    void paintGrid (juce::Graphics& g) const
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "Eight hits played a little off the 1/16 grid (120 BPM): how Strength moves them", 12.0f, theme::textDim, juce::Justification::centredLeft);
        area.removeFromTop (6.0f);
        const float labelWidth = 120.0f;
        const auto offsets = tg::getGridOffsetsMs();
        const double stepMs = 125.0, spanMs = stepMs * 8.0;
        const double strengths[] { 0.0, 0.5, 1.0 };
        const char* names[] { "0% (as played)", "50%", "100% (on the grid)" };
        const float rowHeight = (area.getHeight() - 22.0f) / 3.0f;

        for (int row = 0; row < 3; ++row)
        {
            const auto rowArea = juce::Rectangle<float> (area.getX(), area.getY() + (float) row * rowHeight, area.getWidth(), rowHeight);
            const auto plot = rowArea.withTrimmedLeft (labelWidth).reduced (0.0f, 3.0f);
            auto xAt = [&] (double ms) { return plot.getX() + 10.0f + (float) (ms / spanMs) * (plot.getWidth() - 20.0f); };
            g.setColour (theme::lane);
            g.fillRect (plot);
            drawLabel (g, rowArea.withWidth (labelWidth - 8.0f), juce::String ("Strength ") + names[row], 11.5f, theme::text, juce::Justification::centredRight, true);

            for (int k = 0; k <= 8; ++k)
            {
                g.setColour (k % 4 == 0 ? theme::gridBar : theme::grid);
                g.drawVerticalLine ((int) xAt (k * stepMs), plot.getY(), plot.getBottom());
            }

            for (size_t k = 0; k < offsets.size(); ++k)
            {
                const double ms = (double) k * stepMs + 0.0 + tg::getSnappedOffsetMs (offsets[k], strengths[row]);
                g.setColour (HitsToMidiWindow::getDrumColour (ah::Drum::snare));
                g.fillRoundedRectangle (xAt (ms) - 2.0f, plot.getY() + 4.0f, 4.0f, plot.getHeight() - 8.0f, 1.5f);

                if (row == 0)
                    drawLabel (g, { xAt (ms) - 20.0f, plot.getY() - 1.0f, 40.0f, 12.0f }, (offsets[k] >= 0.0 ? "+" : "") + juce::String ((int) offsets[k]), 9.5f, theme::textDim);
            }
        }

        drawLabel (g, area.removeFromBottom (20.0f), "Grid lines are 1/16 notes (125 ms). The numbers above the first row are how many ms each hit was off.", 11.5f, theme::textDim,
                   juce::Justification::centredLeft);
    }

    /** Two 60 Hz waves in line and 8.3 ms apart, with their sums. */
    void paintPhase (juce::Graphics& g) const
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (18.0f), "Two copies of a 60 Hz kick (click a half to hear it)", 12.0f, theme::textDim, juce::Justification::centredLeft);
        area.removeFromTop (4.0f);
        const double delays[] { 0.0, 8.333 };
        const double spanMs = 50.0;

        for (int half = 0; half < 2; ++half)
        {
            auto panel = half == 0 ? area.removeFromLeft (area.getWidth() / 2.0f - 6.0f) : area;

            if (half == 0)
                area.removeFromLeft (12.0f);

            drawLabel (g, panel.removeFromTop (20.0f),
                       half == 0 ? "In line" : juce::String ("8.3 ms apart (half a cycle)"), 13.0f, theme::text, juce::Justification::centredLeft, true);
            const float rowHeight = (panel.getHeight() - 24.0f) / 3.0f;
            const juce::Colour colours[] { theme::accent, HitsToMidiWindow::getDrumColour (ah::Drum::kick), theme::meterGreen };
            const char* names[] { "Original", "Layer", "Sum" };

            for (int row = 0; row < 3; ++row)
            {
                const auto plot = juce::Rectangle<float> (panel.getX(), panel.getY() + (float) row * rowHeight, panel.getWidth(), rowHeight).reduced (0.0f, 2.0f);
                g.setColour (theme::lane);
                g.fillRect (plot);
                g.setColour (theme::gridBar);
                g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

                juce::Path path;
                const int columns = (int) plot.getWidth();

                for (int c = 0; c <= columns; ++c)
                {
                    const double ms = (double) c / columns * spanMs;
                    const double a = std::sin (juce::MathConstants<double>::twoPi * 60.0 * ms / 1000.0);
                    const double b = ms >= delays[half] ? std::sin (juce::MathConstants<double>::twoPi * 60.0 * (ms - delays[half]) / 1000.0) : 0.0;
                    const double v = row == 0 ? a : row == 1 ? b : (a + b) / 2.0 * (half == 0 ? 1.0 : 1.0);
                    const auto p = juce::Point<float> (plot.getX() + (float) c, plot.getCentreY() - (float) v * plot.getHeight() * 0.42f);

                    if (c == 0)
                        path.startNewSubPath (p);
                    else
                        path.lineTo (p);
                }

                g.setColour (colours[row]);
                g.strokePath (path, juce::PathStrokeType (row == 2 ? 2.2f : 1.4f));
                drawLabel (g, plot.reduced (4.0f, 1.0f).withHeight (14.0f), names[row], 10.5f, theme::textDim, juce::Justification::topLeft);
            }

            const double gain = tg::getSumGainDb (60.0, delays[half]);
            drawLabel (g, panel.removeFromBottom (22.0f),
                       half == 0 ? juce::String ("Sum: twice the swing, +") + juce::String (gain, 2) + " dB" : juce::String ("Sum: they cancel, below ") + dbText (-40.0),
                       12.0f, half == 0 ? theme::meterGreen : theme::meterRed, juce::Justification::centredLeft, true);
        }
    }

    /** The user's hits on the bar grid in drum lanes, with the counts. */
    void paintYourClip (juce::Graphics& g) const
    {
        auto area = getArea();

        if (! state.part.has_value() || state.part->hits.empty())
        {
            drawLabel (g, area, "Nothing to look at yet.\nRight-click an audio clip with drums and choose Hits to MIDI..., or select a drum part.", 14.0f, theme::textDim);
            return;
        }

        const auto& part = *state.part;
        drawLabel (g, area.removeFromTop (18.0f), "Your hits on the bar grid", 12.0f, theme::textDim, juce::Justification::centredLeft);
        area.removeFromTop (4.0f);
        auto footer = area.removeFromBottom (42.0f);
        const double lengthBeats = juce::jmax (4.0, part.lengthBars * 4.0);
        std::vector<std::pair<double, ah::Drum>> hits;

        for (auto& h : part.hits)
            hits.push_back ({ h.beat, h.drum });

        drawLanes (g, area.withTrimmedTop (4.0f), lengthBeats, hits);
        drawLabel (g, footer, tg::describeCounts (part), 12.5f, theme::text, juce::Justification::centredLeft);
    }

    tg::Topic topic = tg::Topic::whatItIs;
    State state;
};

//==============================================================================
DrumTriggerExplainer::DrumTriggerExplainer (Engine& e, PartProvider provider) : engine (e), getPart (std::move (provider))
{
    title.setText ("How drum triggers work", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    picture->setTooltip ("The picture of the topic. Where it shows your settings or your hits, it follows them. Click the loop, the snare hit or the two kicks to hear them.");
    picture->onClick = [this] (float x) { play ((int) (x * 100.0f)); };
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

DrumTriggerExplainer::~DrumTriggerExplainer()
{
    list.setModel (nullptr);
    engine.stopAudition();
}

std::optional<tg::Part> DrumTriggerExplainer::getPartForClip (const Engine& eng, const Track& track, const MidiClip& clip)
{
    bool drums = track.usesBuiltInSynth() && builtInSounds::isPercussive (track.builtInSound.load());

    for (auto& slot : track.plugins)
        if (slot->instance != nullptr && slot->instance->getName() == BuiltInDrums::identifier)
            drums = true;

    if (! drums || clip.notes.empty())
        return std::nullopt;

    std::vector<tg::DrumNote> notes;

    for (auto& n : clip.notes)
        notes.push_back ({ n.pitch, n.start, (int) std::lround (n.velocity * 127.0f) });

    const auto& tempo = eng.getTempoMap();
    const double lengthBeats = clip.getLengthBeats (tempo);
    auto part = tg::getPartFromNotes (notes, lengthBeats, tempo.getAverageBpm (clip.startBeat, clip.startBeat + lengthBeats));

    if (part.hits.empty())
        return std::nullopt;

    return part;
}

juce::String DrumTriggerExplainer::getPrintSource() const
{
    return "the example drum loop";
}

int DrumTriggerExplainer::getNumPrintTopics() const
{
    return (int) std::size (tg::allTopics);
}

booklet::Topic DrumTriggerExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    return { tg::getTopicName (tg::allTopics[index]), tg::getTopicSummary (tg::allTopics[index]), explanation.getText(),
             { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image DrumTriggerExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void DrumTriggerExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void DrumTriggerExplainer::showTopic (tg::Topic topic)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (tg::allTopics[i] == topic)
            list.selectRow (i);
}

int DrumTriggerExplainer::getNumRows()
{
    return (int) std::size (tg::allTopics);
}

void DrumTriggerExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
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

void DrumTriggerExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        show (row);
}

void DrumTriggerExplainer::timerCallback()
{
    refreshPicture();

    if (tg::allTopics[shownRow] == tg::Topic::yourClip)
        refreshText();
}

void DrumTriggerExplainer::refreshPicture()
{
    Picture::State state;
    auto& settings = *engine.properties.getUserSettings();
    state.sensitivityDb = settings.getDoubleValue ("hits.sensitivityDb", -30.0);
    state.gapMs = settings.getDoubleValue ("hits.gapMs", 30.0);
    state.curve = settings.getIntValue ("hits.velocity", 0);

    if (tg::allTopics[shownRow] == tg::Topic::yourClip && getPart != nullptr)
        state.part = getPart();

    picture->setState (tg::allTopics[shownRow], std::move (state));
}

void DrumTriggerExplainer::refreshText()
{
    const auto topic = tg::allTopics[shownRow];
    juce::String text;

    if (topic == tg::Topic::yourClip)
    {
        const auto part = getPart != nullptr ? getPart() : std::nullopt;
        text = tg::explain (topic, part.has_value() ? &*part : nullptr);
    }
    else
    {
        text = tg::explain (topic);
    }

    if (text != explanation.getText())
    {
        explanation.setText (text, false);

        if (topic != tg::Topic::yourClip)
            explanation.moveCaretToTop (false);
    }
}

void DrumTriggerExplainer::show (int index)
{
    const bool changed = index != shownRow || explanation.getText().isEmpty();
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    refreshPicture();
    refreshText();

    if (changed)
        explanation.moveCaretToTop (false);
}

void DrumTriggerExplainer::play (int what)
{
    const double rate = engine.getSampleRate();

    if (rate <= 0.0)
        return;

    const auto topic = tg::allTopics[shownRow];
    juce::AudioBuffer<float> buffer;

    if (topic == tg::Topic::whatItIs || topic == tg::Topic::findingHits)
    {
        buffer = hitstomidi::renderExampleLoop (rate);
    }
    else if (topic == tg::Topic::transients)
    {
        const auto loop = hitstomidi::renderExampleLoop (rate);
        double hitTime = 0.5;

        for (auto& s : hitstomidi::getExampleLoop())
            if (s.drum == ah::Drum::snare)
            {
                hitTime = s.beat * 0.5 + s.lateMs / 1000.0;
                break;
            }

        const int start = juce::jmax (0, (int) ((hitTime - 0.005) * rate)), length = juce::jmin (loop.getNumSamples() - start, (int) (0.4 * rate));
        buffer.setSize (1, length);
        buffer.copyFrom (0, 0, loop, 0, start, length);
    }
    else if (topic == tg::Topic::replaceOrLayer)
    {
        // Two 60 Hz kicks, the second one 0 or 8.33 ms late: left half in line, right half apart.
        const double delayMs = what < 50 ? 0.0 : 8.333;
        const int length = (int) (0.6 * rate);
        buffer.setSize (1, length);

        for (int i = 0; i < length; ++i)
        {
            auto kick = [] (double t)
            {
                return t < 0.0 ? 0.0 : std::sin (juce::MathConstants<double>::twoPi * 60.0 * t) * std::exp (-t / 0.15) * (1.0 - std::exp (-t / 0.002));
            };

            const double t = i / rate;
            buffer.setSample (0, i, (float) (0.4 * (kick (t) + kick (t - delayMs / 1000.0))));
        }
    }
    else
    {
        return;
    }

    engine.stop();
    engine.startAudition (std::move (buffer));
}

void DrumTriggerExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void DrumTriggerExplainer::resized()
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
