#include "SoundExplainer.h"
#include "WaveEditor.h"

#include <juce_dsp/juce_dsp.h>

namespace daw
{
using namespace soundguide;

namespace
{
    /** How much time the frequency pictures show: between about 2 and 12 cycles of the tone. */
    double getWindowSeconds (float hz)
    {
        for (const double seconds : { 0.1, 0.05, 0.02, 0.01, 0.005, 0.002, 0.001 })
            if (hz * seconds <= 12.0)
                return seconds;

        return 0.0005;
    }

    juce::String formatLevel (float db)
    {
        return db <= -99.0f ? juce::String ("-inf dB") : juce::String (db, 1) + " dB";
    }

    /** The peak and RMS of one cycle of the shape, in dB. */
    std::pair<float, float> measureShape (Shape shape)
    {
        double sum = 0.0;
        float peak = 0.0f;
        constexpr int steps = 4096;

        for (int i = 0; i < steps; ++i)
        {
            const float v = getSample (shape, i / (double) steps);
            sum += (double) v * v;
            peak = juce::jmax (peak, std::abs (v));
        }

        return { juce::Decibels::gainToDecibels (peak), juce::Decibels::gainToDecibels ((float) std::sqrt (sum / steps)) };
    }

    /** "44,100". */
    juce::String withThousands (int number)
    {
        auto text = juce::String (number);

        for (int i = text.length() - 3; i > (number < 0 ? 1 : 0); i -= 3)
            text = text.substring (0, i) + "," + text.substring (i);

        return text;
    }

    constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
    constexpr float spectrumLowHz = 30.0f, spectrumHighHz = 16000.0f, spectrumFloorDb = -90.0f;
}

//==============================================================================
/** The picture of the selected topic. */
class SoundExplainer::Picture final : public juce::Component
{
public:
    struct Settings
    {
        Topic topic = Topic::air;
        Shape shape = Shape::sine;
        float frequency = 440.0f, levelDb = -18.0f, phaseDegrees = 90.0f;
        float delayMs = 1.0f;   // the copy's delay, for phase cancellation
        bool flipped = false;   // the copy upside down
    };

    void setSettings (const Settings& s)
    {
        settings = s;
        repaint();
    }

    void setClipData (const ClipData& data)
    {
        clip = data;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (r, 4.0f);
        r = r.reduced (16.0f, 12.0f);

        switch (settings.topic)
        {
            case Topic::air:       paintAir (g, r); break;
            case Topic::waveform:  paintWaveform (g, r); break;
            case Topic::amplitude: paintAmplitude (g, r); break;
            case Topic::frequency: paintFrequency (g, r); break;
            case Topic::timbre:    paintTimbre (g, r); break;
            case Topic::numbers:   paintNumbers (g, r); break;
            case Topic::phase:     paintPhase (g, r); break;
            case Topic::cancellation: paintCancellation (g, r); break;
            case Topic::mono:      paintMono (g, r); break;
            case Topic::vocals:    paintVocals (g, r); break;
        }
    }

private:
    float getLevel() const { return juce::Decibels::decibelsToGain (settings.levelDb); }

    /** The tone's level against the loudest the slider goes (-6 dB), for pictures that show it. */
    float getRelativeLevel() const { return juce::jlimit (0.0f, 1.0f, getLevel() / juce::Decibels::decibelsToGain (-6.0f)); }

    void drawCaption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& left, const juce::String& right = {})
    {
        auto line = r.removeFromTop (18.0f);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::text);
        g.drawText (left, line, juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.drawText (right, line, juce::Justification::centredRight, true);
        r.removeFromTop (4.0f);
    }

    void drawNote (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText (text, r.toNearestInt(), juce::Justification::centred, 3);
    }

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Justification j, juce::Colour colour = theme::textDim)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (text, r, j, true);
    }

    /** A line through value (t) for t from 0 to 1 across the area, with 1 at `scale` pixels from the middle. */
    void drawLine (juce::Graphics& g, juce::Rectangle<float> area, float scale, const std::function<float (double)>& value,
                   juce::Colour colour, float thickness = 2.0f)
    {
        juce::Path path;
        const int steps = juce::jmax (2, (int) area.getWidth());

        for (int i = 0; i <= steps; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / (float) steps;
            const float y = juce::jlimit (area.getY(), area.getBottom(), area.getCentreY() - value (i / (double) steps) * scale);

            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawCentre (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour (theme::text.withAlpha (0.18f));
        g.fillRect (area.getX(), area.getCentreY(), area.getWidth(), 1.0f);
    }

    /** Mono samples across the area, with 0 dB at `scale` pixels from the middle. */
    void drawSamples (juce::Graphics& g, juce::Rectangle<float> area, float scale, const std::vector<float>& samples, juce::Colour colour)
    {
        if (samples.empty())
            return;

        const int n = (int) samples.size();
        g.setColour (colour);

        if (n > area.getWidth() * 2.0f)
        {
            for (int x = 0; x < (int) area.getWidth(); ++x)
            {
                const int s0 = x * n / (int) area.getWidth(), s1 = juce::jmax (s0 + 1, (x + 1) * n / (int) area.getWidth());
                float lo = 0.0f, hi = 0.0f;

                for (int i = s0; i < juce::jmin (n, s1); ++i)
                {
                    lo = juce::jmin (lo, samples[(size_t) i]);
                    hi = juce::jmax (hi, samples[(size_t) i]);
                }

                g.drawVerticalLine ((int) area.getX() + x, juce::jmax (area.getY(), area.getCentreY() - hi * scale),
                                    juce::jmin (area.getBottom(), area.getCentreY() - lo * scale + 1.0f));
            }

            return;
        }

        drawLine (g, area, scale, [&] (double t) { return samples[(size_t) juce::jlimit (0, n - 1, (int) std::round (t * (n - 1)))]; },
                  colour, 1.5f);
    }

    //==============================================================================
    void paintAir (juce::Graphics& g, juce::Rectangle<float> r)
    {
        // Slowed down to a wave a second or so, with a wavelength that still follows the frequency.
        constexpr double shownHz = 0.8;
        const float hz = settings.frequency;
        const float wavelength = juce::jlimit (40.0f, 420.0f, 240.0f * std::sqrt (220.0f / hz));
        const double t = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        const float k = juce::MathConstants<float>::twoPi / wavelength;
        const float omegaT = (float) std::fmod (juce::MathConstants<double>::twoPi * shownHz * t, juce::MathConstants<double>::twoPi);
        const float level = getRelativeLevel();

        drawCaption (g, r, "Air in front of a speaker, slowed down about " + juce::String (juce::roundToInt (hz / shownHz)) + " times",
                     formatHz (hz) + ": one wave is " + formatWavelength (hz) + " long in the air");

        auto dots = r.removeFromTop (r.getHeight() * 0.52f);
        r.removeFromTop (10.0f);
        const float left = dots.getX() + 44.0f;

        // The speaker: a box and its cone, moving with the air next to it.
        const float coneOffset = wavelength * (0.03f + 0.09f * level) * std::sin (-omegaT);
        g.setColour (theme::textDim.withAlpha (0.5f));
        g.fillRoundedRectangle (dots.getX(), dots.getCentreY() - 40.0f, 20.0f, 80.0f, 3.0f);
        g.setColour (theme::text);
        g.fillRoundedRectangle (left - 18.0f + coneOffset, dots.getCentreY() - 34.0f, 6.0f, 68.0f, 2.0f);

        // The molecules: each wobbles around its own place; the pattern moves away from the speaker.
        if (molecules.empty())
        {
            juce::Random random (7);

            for (int i = 0; i < 900; ++i)
                molecules.push_back ({ random.nextFloat(), random.nextFloat() });
        }

        const float amplitude = wavelength * (0.03f + 0.09f * level);
        g.setColour (theme::text.withAlpha (0.75f));

        for (const auto& m : molecules)
        {
            const float x0 = left + m.x * (dots.getRight() - left);
            const float x = x0 + amplitude * std::sin (k * (x0 - left) - omegaT);
            g.fillEllipse (x - 1.8f, dots.getY() + m.y * dots.getHeight() - 1.8f, 3.6f, 3.6f);
        }

        // The pressure: up where the dots crowd together.
        auto curve = r.withTrimmedLeft (left - r.getX());
        curve.removeFromBottom (16.0f);
        drawCentre (g, curve);
        const float swing = curve.getHeight() * 0.42f * juce::jmax (0.5f, level);
        drawLine (g, curve, swing, [&] (double u) { return -std::cos (k * (float) u * curve.getWidth() - omegaT); }, theme::accent);

        drawLabel (g, "squeezed", { r.getX(), curve.getCentreY() - swing - 7.0f, left - r.getX() + 40.0f, 14.0f }, juce::Justification::centredLeft);
        drawLabel (g, "thinned", { r.getX(), curve.getCentreY() + swing - 7.0f, left - r.getX() + 40.0f, 14.0f }, juce::Justification::centredLeft);
        drawLabel (g, "The pressure: the waveform. Up where the air is squeezed (compression), down where it is thinned out (rarefaction).",
                   r.removeFromBottom (14.0f), juce::Justification::centred);
    }

    void paintWaveform (juce::Graphics& g, juce::Rectangle<float> r)
    {
        if (! clip.present)
        {
            drawCaption (g, r, "The tone: " + getShapeName (settings.shape) + " at " + formatHz (settings.frequency), "time ->");
            auto area = r.withTrimmedBottom (40.0f).withTrimmedLeft (110.0f);
            drawAxisLabels (g, r.withTrimmedBottom (40.0f), area);
            drawCentre (g, area);
            drawLine (g, area, area.getHeight() * 0.45f, [&] (double u) { return getSample (settings.shape, u * 3.0); }, theme::accent);
            drawNote (g, r.removeFromBottom (36.0f), "Select an audio clip in the arrangement to see your own sound here.");
            return;
        }

        drawCaption (g, r, "Your clip: " + clip.name + ", " + formatTime (clip.numSamples / clip.rate), "time ->");
        auto area = r.withTrimmedLeft (110.0f);
        drawAxisLabels (g, r, area);
        drawCentre (g, area);

        const int n = (int) clip.overviewMax.size();
        const float scale = area.getHeight() * 0.48f;
        g.setColour (theme::accent.brighter (0.3f));

        for (int i = 0; i < n; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / (float) n;
            g.fillRect (x, juce::jmax (area.getY(), area.getCentreY() - clip.overviewMax[(size_t) i] * scale), juce::jmax (1.0f, area.getWidth() / (float) n),
                        juce::jmax (1.0f, (clip.overviewMax[(size_t) i] - clip.overviewMin[(size_t) i]) * scale));
        }
    }

    void drawAxisLabels (juce::Graphics& g, juce::Rectangle<float> r, juce::Rectangle<float> area)
    {
        const float w = area.getX() - r.getX() - 8.0f;
        drawLabel (g, "air squeezed", { r.getX(), area.getY(), w, 14.0f }, juce::Justification::centredLeft);
        drawLabel (g, "at rest: silence", { r.getX(), area.getCentreY() - 7.0f, w, 14.0f }, juce::Justification::centredLeft);
        drawLabel (g, "air thinned out", { r.getX(), area.getBottom() - 14.0f, w, 14.0f }, juce::Justification::centredLeft);
    }

    void paintAmplitude (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto [shapePeak, shapeRms] = measureShape (settings.shape);
        const float peakDb = settings.levelDb + shapePeak, rmsDb = settings.levelDb + shapeRms;

        drawCaption (g, r, "The tone: " + getShapeName (settings.shape) + ", peak " + formatLevel (peakDb) + ", RMS " + formatLevel (rmsDb),
                     clip.present ? "Your clip: peak " + formatLevel (clip.peakDb) + ", RMS " + formatLevel (clip.rmsDb) : juce::String());

        auto area = r.withTrimmedRight (150.0f);
        const float scale = area.getHeight() * 0.48f;
        const float dashes[] = { 4.0f, 4.0f };
        drawCentre (g, area);

        // The scale: 0 dB at the edges, and what -6, -12 and -20 dB are.
        for (const auto [db, name] : { std::pair { 0.0f, "0 dB, the most a file holds" }, std::pair { -6.0f, "-6 dB, half" },
                                       std::pair { -12.0f, "-12 dB, a quarter" }, std::pair { -20.0f, "-20 dB, a tenth" } })
        {
            const float y = area.getCentreY() - juce::Decibels::decibelsToGain (db) * scale;
            g.setColour (theme::text.withAlpha (db == 0.0f ? 0.35f : 0.15f));
            g.fillRect (area.getX(), y, area.getWidth(), 1.0f);
            g.fillRect (area.getX(), 2.0f * area.getCentreY() - y, area.getWidth(), 1.0f);
            drawLabel (g, name, { area.getRight() + 8.0f, y - 7.0f, 150.0f, 14.0f }, juce::Justification::centredLeft);
        }

        const float level = getLevel();
        drawLine (g, area, scale * level, [&] (double u) { return getSample (settings.shape, u * 3.0); }, theme::accent);

        // Its peak and RMS.
        const float peakY = area.getCentreY() - juce::Decibels::decibelsToGain (peakDb) * scale;
        const float rmsY = area.getCentreY() - juce::Decibels::decibelsToGain (rmsDb) * scale;
        g.setColour (theme::meterYellow);
        g.fillRect (area.getX(), peakY, area.getWidth(), 1.0f);
        g.drawDashedLine ({ area.getX(), rmsY, area.getRight(), rmsY }, dashes, 2, 1.0f);
        drawLabel (g, "peak " + formatLevel (peakDb), { area.getRight() - 124.0f, peakY - 15.0f, 120.0f, 14.0f }, juce::Justification::centredRight, theme::meterYellow);
        drawLabel (g, "RMS " + formatLevel (rmsDb), { area.getRight() - 124.0f, rmsY + 1.0f, 120.0f, 14.0f }, juce::Justification::centredRight, theme::meterYellow);
    }

    void paintFrequency (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const float hz = settings.frequency;
        const double seconds = getWindowSeconds (hz);
        const double cycles = hz * seconds;

        auto range = r.removeFromBottom (40.0f);
        const float rowHeight = (r.getHeight() - 10.0f) * 0.5f;
        auto toneRow = r.removeFromTop (rowHeight);
        r.removeFromTop (10.0f);
        auto clipRow = r;

        // The tone, with every cycle numbered.
        drawCaption (g, toneRow, "The tone: " + (cycles < 10.0 ? juce::String (cycles, 1) : juce::String (juce::roundToInt (cycles))) + " cycles in " + formatTime (seconds) + " = "
                                     + formatHz (hz) + ", close to " + getNoteName (hz));
        auto toneArea = toneRow.withTrimmedBottom (14.0f);
        drawCentre (g, toneArea);
        drawLine (g, toneArea, toneArea.getHeight() * 0.45f, [&] (double u) { return getSample (settings.shape, u * cycles); }, theme::accent);

        for (int c = 0; c < (int) std::ceil (cycles - 1e-6); ++c)
        {
            const float x = toneArea.getX() + toneArea.getWidth() * (float) (c / cycles);
            g.setColour (theme::text.withAlpha (0.2f));
            g.fillRect (x, toneArea.getY(), 1.0f, toneArea.getHeight());
            drawLabel (g, juce::String (c + 1), { x, toneArea.getBottom(), toneArea.getWidth() / (float) cycles, 14.0f }, juce::Justification::centred);
        }

        // The clip over the same time.
        if (clip.present)
        {
            const juce::String pitch = clip.pitchHz > 0.0f ? "about " + formatHz (clip.pitchHz) + ", close to " + getNoteName (clip.pitchHz)
                                                           : juce::String ("no clear pitch here (silence, a drum, noise, or several notes)");
            drawCaption (g, clipRow, "Your clip from " + formatTime (clip.focusSample / clip.rate) + ", the same " + formatTime (seconds) + ": " + pitch);
            drawCentre (g, clipRow);
            drawSamples (g, clipRow, clipRow.getHeight() * 0.48f, clip.window, theme::accent.brighter (0.3f));
        }
        else
        {
            drawNote (g, clipRow, "Select an audio clip to compare: while the song plays, this follows the playhead.");
        }

        paintHearingRange (g, range.withTrimmedTop (8.0f));
    }

    /** 20 Hz to 20 kHz, with the tone and the clip's pitch on it. */
    void paintHearingRange (juce::Graphics& g, juce::Rectangle<float> r)
    {
        auto bar = r.removeFromTop (8.0f);
        auto xOf = [&] (float hz) { return bar.getX() + bar.getWidth() * std::log (hz / lowestHeard) / std::log (highestHeard / lowestHeard); };

        g.setColour (theme::text.withAlpha (0.12f));
        g.fillRoundedRectangle (bar, 3.0f);

        for (const auto [hz, name] : { std::pair { 20.0f, "20 Hz" }, std::pair { 50.0f, "50" }, std::pair { 100.0f, "100" }, std::pair { 200.0f, "200" },
                                       std::pair { 500.0f, "500" }, std::pair { 1000.0f, "1k" }, std::pair { 2000.0f, "2k" }, std::pair { 5000.0f, "5k" },
                                       std::pair { 10000.0f, "10k" }, std::pair { 20000.0f, "20 kHz" } })
        {
            const float x = xOf (hz);
            const auto j = hz <= 20.0f ? juce::Justification::centredLeft : hz >= 20000.0f ? juce::Justification::centredRight : juce::Justification::centred;
            drawLabel (g, name, { hz <= 20.0f ? x : hz >= 20000.0f ? x - 60.0f : x - 30.0f, r.getY() + 2.0f, 60.0f, 14.0f }, j);
        }

        auto marker = [&] (float hz, juce::Colour colour)
        {
            const float x = xOf (juce::jlimit (lowestHeard, highestHeard, hz));
            g.setColour (colour);
            g.fillRoundedRectangle (x - 2.0f, bar.getY() - 3.0f, 4.0f, bar.getHeight() + 6.0f, 1.5f);
        };

        if (clip.present && clip.pitchHz > 0.0f)
            marker (clip.pitchHz, theme::meterYellow);

        marker (settings.frequency, theme::accent);
        drawLabel (g, "What we hear:", { bar.getX() - 4.0f, bar.getY() - 17.0f, 200.0f, 14.0f }, juce::Justification::centredLeft);
    }

    void paintTimbre (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const float hz = settings.frequency;
        auto top = r.removeFromTop (r.getHeight() * 0.44f);   // the lower half has the spectrum and the envelope
        r.removeFromTop (10.0f);

        drawCaption (g, top, getShapeName (settings.shape) + " at " + formatHz (hz) + ", and its harmonics",
                     "1 = " + formatHz (hz) + ", 2 = " + formatHz (2.0f * hz) + ", 3 = " + formatHz (3.0f * hz) + " ...");
        auto wave = top.removeFromLeft (top.getWidth() * 0.38f);
        top.removeFromLeft (20.0f);
        drawCentre (g, wave);
        drawLine (g, wave, wave.getHeight() * 0.45f, [&] (double u) { return getSample (settings.shape, u * 2.0); }, theme::accent);

        // One bar per harmonic; the ones above our hearing are dim.
        constexpr int harmonics = 16;
        auto bars = top.withTrimmedBottom (14.0f);
        const float step = bars.getWidth() / harmonics;

        for (int n = 1; n <= harmonics; ++n)
        {
            const float level = getHarmonicLevel (settings.shape, n);
            const auto column = juce::Rectangle<float> (bars.getX() + (n - 1) * step, bars.getY(), step, bars.getHeight());
            const bool heard = n * hz <= highestHeard;
            g.setColour (theme::accent.withAlpha (heard ? 1.0f : 0.3f));
            g.fillRect (column.reduced (step * 0.18f, 0.0f).withTrimmedTop (column.getHeight() * (1.0f - level)));
            drawLabel (g, juce::String (n), { column.getX(), bars.getBottom(), step, 14.0f }, juce::Justification::centred);
        }

        if (hz * 2.0f > highestHeard)
            drawLabel (g, "Above 20 kHz the harmonics are too high to hear: up here every shape sounds like a sine.",
                       bars.removeFromTop (14.0f), juce::Justification::centredRight);

        // Below: the clip's spectrum (how loud each frequency is, from the focus on) and its envelope.
        if (! clip.present || clip.spectrumDb.empty())
        {
            drawNote (g, r, "Select an audio clip to see its harmonics and its envelope: while the song plays, this follows the playhead.");
            return;
        }

        auto envelopeArea = r.removeFromRight (r.getWidth() * 0.42f);
        r.removeFromRight (16.0f);
        paintEnvelope (g, envelopeArea);

        drawCaption (g, r, clip.example ? "Example clip (made up): spectrum" : "Your clip: spectrum",
                     clip.pitchHz > 0.0f ? "about " + formatHz (clip.pitchHz) + ", marks: harmonics" : juce::String ("no clear pitch here"));
        auto area = r.withTrimmedBottom (14.0f);
        auto xOf = [&] (float f) { return area.getX() + area.getWidth() * std::log (f / spectrumLowHz) / std::log (spectrumHighHz / spectrumLowHz); };
        auto yOf = [&] (float db) { return area.getBottom() - area.getHeight() * juce::jlimit (0.0f, 1.0f, (db - spectrumFloorDb) / -spectrumFloorDb); };

        for (const float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        {
            g.setColour (theme::text.withAlpha (0.1f));
            g.fillRect (xOf (f), area.getY(), 1.0f, area.getHeight());
            drawLabel (g, f >= 1000.0f ? juce::String (juce::roundToInt (f / 1000.0f)) + "k" : juce::String (juce::roundToInt (f)),
                       { xOf (f) - 20.0f, area.getBottom(), 40.0f, 14.0f }, juce::Justification::centred);
        }

        // The loudest bin under every pixel.
        juce::Path fill;
        fill.startNewSubPath (area.getX(), area.getBottom());
        const double binHz = clip.rate / fftSize;

        for (int x = 0; x <= (int) area.getWidth(); ++x)
        {
            auto freqOf = [&] (float px) { return spectrumLowHz * std::pow (spectrumHighHz / spectrumLowHz, px / area.getWidth()); };
            const int b0 = (int) (freqOf ((float) x) / binHz), b1 = juce::jmax (b0 + 1, (int) (freqOf ((float) x + 1.0f) / binHz));
            float db = spectrumFloorDb;

            for (int b = b0; b < juce::jmin (b1 + 1, (int) clip.spectrumDb.size()); ++b)
                db = juce::jmax (db, clip.spectrumDb[(size_t) b]);

            fill.lineTo (area.getX() + (float) x, yOf (db));
        }

        fill.lineTo (area.getRight(), area.getBottom());
        fill.closeSubPath();
        g.setColour (theme::accent.withAlpha (0.45f));
        g.fillPath (fill);
        g.setColour (theme::accent);
        g.strokePath (fill, juce::PathStrokeType (1.0f));

        if (clip.pitchHz > 0.0f)
            for (int n = 1; n * clip.pitchHz < spectrumHighHz && n <= 8; ++n)
            {
                g.setColour (theme::meterYellow);
                g.fillRect (xOf (n * clip.pitchHz) - 0.5f, area.getY(), 1.5f, 8.0f);
            }
    }

    /** The note at the focus: its level as a grey line, with the attack, decay, sustain and release drawn over it. */
    void paintEnvelope (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto& e = clip.envelope;
        drawCaption (g, r, clip.example ? "Envelope of the example, at " + formatTime (clip.focusSample / clip.rate)
                                        : "Envelope at " + formatTime (clip.focusSample / clip.rate));

        auto text = r.removeFromBottom (58.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.5f));
        g.drawFittedText (clip.envelopeMeasured ? describeEnvelope (e) : juce::String(), text.toNearestInt(), juce::Justification::topLeft, 4);

        if (! clip.envelopeMeasured || e.contour.empty())
            return;

        auto area = r.withTrimmedTop (14.0f).withTrimmedBottom (14.0f);   // room for the segment labels, and for seconds
        const double focusSeconds = clip.focusSample / clip.rate;
        const double contourEnd = e.contourStartSeconds + (double) e.contour.size() * e.blockSeconds;
        double from = e.found ? e.onsetSeconds - 0.5 : focusSeconds - 1.5;
        double seconds = e.found ? juce::jlimit (3.0, 9.0, e.endSeconds - e.onsetSeconds + 0.5) : 3.0;
        from = juce::jmin (from, juce::jmax (e.contourStartSeconds, contourEnd - seconds));   // may start before the clip does
        const double to = from + seconds;
        auto xOf = [&] (double t) { return area.getX() + area.getWidth() * (float) ((t - from) / juce::jmax (0.001, to - from)); };

        float top = 0.001f;

        for (const float v : e.contour)
            top = juce::jmax (top, v);

        if (e.found)
            top = juce::Decibels::decibelsToGain (e.peakDb);

        auto yOf = [&] (float level) { return area.getBottom() - area.getHeight() * juce::jlimit (0.0f, 1.0f, level / top); };

        g.setColour (theme::text.withAlpha (0.1f));
        g.fillRect (area.getX(), area.getBottom(), area.getWidth(), 1.0f);

        for (double t = juce::jmax (0.0, std::ceil (from)); t <= to; t += 1.0)
        {
            g.fillRect (xOf (t), area.getY(), 1.0f, area.getHeight());
            drawLabel (g, juce::String ((int) t) + " s", { xOf (t) - 20.0f, area.getBottom(), 40.0f, 14.0f }, juce::Justification::centred);
        }

        juce::Path line;
        bool started = false;

        for (size_t i = 0; i < e.contour.size(); ++i)
        {
            const double t = e.contourStartSeconds + (double) i * e.blockSeconds;

            if (t < from || t > to)
                continue;

            const juce::Point<float> p (xOf (t), yOf (e.contour[i]));

            if (started)
                line.lineTo (p);
            else
                line.startNewSubPath (p);

            started = true;
        }

        g.setColour (theme::textDim);
        g.strokePath (line, juce::PathStrokeType (1.2f));

        if (! e.found)
            return;

        // The four segments, in the accent colour, each labelled with what DawIt measured.
        const float peak = top, sustain = peak * juce::Decibels::decibelsToGain (e.sustainDb);
        enum class LabelAt { above, left, below };
        auto segment = [&] (double t0, float l0, double t1, float l1, const juce::String& label, LabelAt at)
        {
            const juce::Point<float> a (xOf (t0), yOf (l0)), b (xOf (t1), yOf (l1));
            g.setColour (theme::accent);
            g.drawLine (a.x, a.y, b.x, b.y, 3.0f);

            // The labels sit where they don't meet the caption or each other: the attack's to its left, the decay's under
            // the line it falls to, the rest above their segments.
            if (at == LabelAt::left)
                drawLabel (g, label, { a.x - 62.0f, a.y - 2.0f, 58.0f, 14.0f }, juce::Justification::centredRight, theme::text);
            else if (at == LabelAt::below)
                drawLabel (g, label, { b.x + 4.0f, b.y + 2.0f, 68.0f, 14.0f }, juce::Justification::centredLeft, theme::text);
            else
                drawLabel (g, label, { (a.x + b.x) * 0.5f - 34.0f, juce::jmin (a.y, b.y) - 15.0f, 68.0f, 14.0f }, juce::Justification::centred, theme::text);
        };

        segment (e.attackStart, peak * 0.1f, e.attackEnd, peak * 0.9f, "A " + formatTime (e.attack), LabelAt::left);

        if (e.sustained)
        {
            double holdStart = e.attackEnd;

            if (e.hasDecay)
            {
                segment (e.decayStart, peak - 0.1f * (peak - sustain), e.decayEnd, peak - 0.9f * (peak - sustain), "D " + formatTime (e.decay), LabelAt::below);
                holdStart = e.decayEnd;
            }

            if (e.releaseFound)
            {
                segment (holdStart, sustain, e.releaseStart, sustain, "S " + juce::String (e.sustainDb, 1) + " dB", LabelAt::above);
                segment (e.releaseStart, sustain * 0.9f, e.releaseEnd, sustain * 0.1f, "R " + formatTime (e.release), LabelAt::above);
            }
        }
    }

    void paintMono (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto& m = clip.mono;
        const bool shown = clip.present && clip.monoMeasured;
        drawCaption (g, r, clip.example ? "Example clip (made up) in stereo and in mono" : "Your clip in stereo and in mono",
                     shown && m.stereo && m.measurable ? "correlation " + juce::String (m.correlation >= 0.0f ? "+" : "") + juce::String (m.correlation, 2) : juce::String());

        if (! shown)
        {
            drawNote (g, r, "Select an audio clip in the waveform editor to measure what it loses in mono.");
            return;
        }

        auto advice = r.removeFromBottom (64.0f);
        r.removeFromBottom (6.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.5f));
        g.drawFittedText (describeMono (m), advice.toNearestInt(), juce::Justification::topLeft, 4);

        if (! m.stereo || ! m.measurable)
            return;

        constexpr float floorRange = 60.0f;
        float top = -100.0f;

        for (const auto& band : m.bands)
            top = juce::jmax (top, band.stereoDb);

        top = juce::jmax (top, m.stereoDb);

        auto drawBars = [&] (juce::Rectangle<float> area, float stereoDb, float monoDb, float lossDb, juce::Colour monoColour)
        {
            const float barWidth = juce::jmin (26.0f, area.getWidth() * 0.3f);
            auto plot = area.withTrimmedBottom (18.0f);
            auto heightOf = [&] (float db) { return plot.getHeight() * juce::jlimit (0.0f, 1.0f, (db - (top - floorRange)) / floorRange); };
            const float x0 = area.getCentreX() - barWidth - 2.0f, x1 = area.getCentreX() + 2.0f;

            g.setColour (theme::textDim.withAlpha (0.55f));
            g.fillRect (x0, plot.getBottom() - heightOf (stereoDb), barWidth, heightOf (stereoDb));
            g.setColour (monoColour);
            g.fillRect (x1, plot.getBottom() - heightOf (monoDb), barWidth, heightOf (monoDb));
            drawLabel (g, juce::String (lossDb > -0.05f ? 0.0f : lossDb, 1) + " dB", area.removeFromBottom (16.0f), juce::Justification::centred, theme::text);
        };

        // Left: the whole clip. Right: five bands, the one that loses most in yellow.
        auto left = r.removeFromLeft (r.getWidth() * 0.2f);
        r.removeFromLeft (14.0f);
        auto overall = left.withTrimmedBottom (34.0f);
        drawBars (overall, m.stereoDb, m.monoDb, m.lossDb, theme::accent);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText ("Whole clip (grey: stereo, blue: mono)", left.removeFromBottom (32.0f).toNearestInt(), juce::Justification::centredTop, 3);

        const float step = r.getWidth() / (float) numMonoBands;

        for (int i = 0; i < numMonoBands; ++i)
        {
            auto column = juce::Rectangle<float> (r.getX() + (float) i * step, r.getY(), step, r.getHeight()).reduced (4.0f, 0.0f);
            const auto& band = m.bands[(size_t) i];
            auto names = column.removeFromBottom (32.0f);
            drawBars (column, band.stereoDb, band.monoDb, band.lossDb, i == m.mostLostBand ? theme::meterYellow : theme::accent);
            g.setColour (i == m.mostLostBand ? theme::text : theme::textDim);
            g.setFont (juce::FontOptions (11.0f));
            g.drawFittedText (getMonoBandName (i), names.toNearestInt(), juce::Justification::centredTop, 3);
        }
    }

    void paintNumbers (juce::Graphics& g, juce::Rectangle<float> r)
    {
        // Few samples and steps, so they can be seen: 16 samples per cycle, 8 steps (3 bits).
        constexpr int samplesPerCycle = 16, cyclesShown = 2, steps = 8;
        drawCaption (g, r, "Sampling: " + juce::String (samplesPerCycle) + " measurements per cycle, each rounded to one of "
                               + juce::String (steps) + " steps (3 bits)");

        auto text = r.removeFromRight (250.0f);
        r.removeFromRight (16.0f);
        auto area = r.withTrimmedBottom (14.0f);
        const float scale = area.getHeight() * 0.45f;
        auto yOf = [&] (float v) { return area.getCentreY() - v * scale; };
        auto quantise = [&] (float v) { return std::round ((v + 1.0f) * 0.5f * (steps - 1)) / (steps - 1) * 2.0f - 1.0f; };

        for (int s = 0; s < steps; ++s)
        {
            g.setColour (theme::text.withAlpha (0.1f));
            g.fillRect (area.getX(), yOf ((float) s / (steps - 1) * 2.0f - 1.0f), area.getWidth(), 1.0f);
        }

        drawCentre (g, area);
        drawLine (g, area, scale, [&] (double u) { return getSample (settings.shape, u * cyclesShown); }, theme::textDim, 1.5f);

        // What the file holds: each sample held until the next, and the samples themselves.
        constexpr int numSamples = samplesPerCycle * cyclesShown;
        const float step = area.getWidth() / numSamples;
        juce::Path held;

        for (int i = 0; i < numSamples; ++i)
        {
            const float v = quantise (getSample (settings.shape, i / (double) samplesPerCycle + 1.0e-6));
            const float x = area.getX() + i * step, y = yOf (v);

            if (i == 0)
                held.startNewSubPath (x, y);
            else
                held.lineTo (x, y);

            held.lineTo (x + step, y);
            g.setColour (theme::accent.withAlpha (0.5f));
            g.fillRect (x - 0.5f, juce::jmin (y, area.getCentreY()), 1.0f, std::abs (y - area.getCentreY()));
            g.setColour (theme::accent);
            g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
        }

        g.setColour (theme::meterYellow.withAlpha (0.8f));
        g.strokePath (held, juce::PathStrokeType (1.5f));
        drawLabel (g, "grey: the air    dots: the samples    yellow: what plays back", area.withY (area.getBottom()).withHeight (14.0f),
                   juce::Justification::centredLeft);

        // Real numbers.
        const double rate = clip.present ? clip.rate : 44100.0;
        juce::String facts;

        if (clip.present)
            facts << "Your clip: " << juce::String (rate / 1000.0, 1) << " kHz, " << (clip.channels > 1 ? "stereo" : "mono") << ".\n"
                  << withThousands (juce::roundToInt (rate)) << " samples a second" << (clip.channels > 1 ? " for each speaker" : "")
                  << "; its " << formatTime (clip.numSamples / rate) << " are " << withThousands (clip.numSamples) << " samples.\n\n";
        else
            facts << "A CD: 44.1 kHz, 16-bit, stereo.\n\n";

        facts << "At " << juce::String (rate / 1000.0, 1) << " kHz the tone (" << formatHz (settings.frequency) << ") gets "
              << (rate / settings.frequency < 10.0 ? juce::String (rate / settings.frequency, 1) : juce::String (juce::roundToInt (rate / settings.frequency))) << " samples per cycle; the highest "
              << "frequency it can hold is " << formatHz ((float) rate * 0.5f) << ".\n\n"
              << "16-bit: 65,536 steps.\n24-bit: 16,777,216 steps.";

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText (facts, text.toNearestInt(), juce::Justification::topLeft, 16, 1.0f);
    }

    void paintPhase (juce::Graphics& g, juce::Rectangle<float> r)
    {
        constexpr double cycles = 2.5;
        const float shift = settings.phaseDegrees / 360.0f;
        const auto shape = settings.shape;
        auto sum = [&] (double u) { return getSample (shape, u * cycles) + getSample (shape, u * cycles - shift); };

        float peakOne = 0.0f, peakSum = 0.0f;

        for (int i = 0; i < 2000; ++i)
        {
            peakOne = juce::jmax (peakOne, std::abs (getSample (shape, i / 2000.0)));
            peakSum = juce::jmax (peakSum, std::abs (sum (i / 2000.0 / cycles)));
        }

        const juce::String together = peakSum < 0.01f * peakOne ? juce::String ("they cancel: silence")
                                                                : formatLevel (juce::Decibels::gainToDecibels (peakSum / peakOne)) + " against one wave";
        const float rowHeight = (r.getHeight() - 16.0f) / 3.0f;
        const float scale = rowHeight * 0.24f;
        const float labelWidth = 150.0f;

        auto row = [&] (const juce::String& name, const juce::String& detail, const std::function<float (double)>& value, juce::Colour colour)
        {
            auto line = r.removeFromTop (rowHeight);
            auto label = line.removeFromLeft (labelWidth);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (name, label.removeFromTop (label.getHeight() * 0.5f), juce::Justification::bottomLeft, true);
            drawLabel (g, detail, label.withHeight (16.0f), juce::Justification::topLeft);
            drawCentre (g, line);
            drawLine (g, line, scale, value, colour);
            r.removeFromTop (8.0f);
        };

        row ("A wave", getShapeName (shape) + ", " + formatHz (settings.frequency), [&] (double u) { return getSample (shape, u * cycles); }, theme::accent);
        row ("A copy", juce::String (juce::roundToInt (settings.phaseDegrees)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")) + " later",
             [&] (double u) { return getSample (shape, u * cycles - shift); }, theme::meterYellow);
        row ("Together", together, sum, theme::text);
    }

    void paintCancellation (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const double delay = settings.delayMs / 1000.0, hz = settings.frequency;
        const bool flipped = settings.flipped;
        const auto shape = settings.shape;
        const float sign = flipped ? -1.0f : 1.0f;

        // Which frequencies cancel: a copy half a cycle late, or a flipped copy a whole number of cycles late.
        juce::String what;

        if (delay < 1.0e-6)
        {
            what = flipped ? "everything cancels" : "everything adds up, 6 dB louder";
        }
        else
        {
            const double first = flipped ? 1.0 / delay : 0.5 / delay;
            what = "cancels " + juce::String (flipped ? "the lows, " : "") + formatHz ((float) first) + ", " + formatHz ((float) (first + 1.0 / delay))
                   + ", " + formatHz ((float) (first + 2.0 / delay)) + "...";
        }

        drawCaption (g, r, juce::String (flipped ? "A flipped copy " : "A copy ") + juce::String (settings.delayMs, 2) + " ms later", what);

        // Sound and copy together, against frequency: the comb.
        auto graph = r.removeFromTop (r.getHeight() * 0.56f);
        auto plot = graph.withTrimmedLeft (44.0f).withTrimmedBottom (16.0f).withTrimmedRight (6.0f);
        auto toX = [&] (double f) { return plot.getX() + plot.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
        auto toY = [&] (double db) { return juce::jmap ((float) juce::jlimit (-30.0, 9.0, db), -30.0f, 9.0f, plot.getBottom(), plot.getY()); };

        for (const double db : { 6.0, 0.0, -12.0, -24.0 })
        {
            g.setColour (db == 0.0 ? theme::text.withAlpha (0.25f) : theme::grid);
            g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
            drawLabel (g, (db > 0.0 ? "+" : "") + juce::String ((int) db) + " dB", { graph.getX(), toY (db) - 7.0f, 40.0f, 14.0f }, juce::Justification::centredRight);
        }

        for (const double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        {
            g.setColour (theme::grid);
            g.fillRect (toX (f), plot.getY(), 1.0f, plot.getHeight());
            drawLabel (g, f >= 1000.0 ? juce::String (juce::roundToInt (f / 1000.0)) + "k" : juce::String (juce::roundToInt (f)),
                       juce::Rectangle<float> (40.0f, 14.0f).withCentre ({ toX (f), plot.getBottom() + 8.0f }), juce::Justification::centred);
        }

        juce::Path comb;
        const int steps = (int) plot.getWidth() * 4;

        for (int i = 0; i <= steps; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) steps;
            const double f = 20.0 * std::pow (1000.0, i / (double) steps);
            const float y = toY (getCopySumDb (f, delay, flipped));
            i == 0 ? comb.startNewSubPath (x, y) : comb.lineTo (x, y);
        }

        g.setColour (theme::accent);
        g.strokePath (comb, juce::PathStrokeType (1.6f));

        // The tone's harmonics on the comb: each turned up or down by it.
        for (int n = 1; n * hz < 20000.0 && n <= 40; ++n)
        {
            const float level = getHarmonicLevel (shape, n);

            if (level <= 0.0f)
                continue;

            const auto dot = juce::Point<float> (toX (n * hz), toY (getCopySumDb (n * hz, delay, flipped)));
            const float size = 4.0f + 6.0f * std::sqrt (level);
            g.setColour (n == 1 ? theme::meterYellow : theme::meterYellow.withAlpha (0.6f));
            g.fillEllipse (juce::Rectangle<float> (size, size).withCentre (dot));
        }

        const float toneDb = getCopySumDb (hz, delay, flipped);
        const auto toneText = toneDb <= -40.0f ? juce::String ("cancelled") : (toneDb > 0.05f ? "+" : "") + juce::String (toneDb, 1) + " dB";
        const float labelX = juce::jlimit (plot.getX(), plot.getRight() - 170.0f, toX (hz) + 10.0f);
        const float labelY = juce::jlimit (plot.getY(), plot.getBottom() - 14.0f, toY (toneDb) - 7.0f);
        drawLabel (g, "your tone: " + toneText, { labelX, labelY, 170.0f, 14.0f }, juce::Justification::centredLeft, theme::meterYellow);
        r.removeFromTop (6.0f);

        // The tone, its copy and the two together.
        constexpr double cycles = 2.5;
        const double late = hz * delay;   // how many cycles late the copy is
        auto wave = [&] (double u) { return getSample (shape, u * cycles); };
        auto copy = [&] (double u) { return sign * getSample (shape, u * cycles - late); };
        const float rowHeight = (r.getHeight() - 6.0f) / 2.0f;
        const float scale = rowHeight * 0.22f;

        auto row = [&] (const juce::String& name, const juce::String& detail, const std::function<void (juce::Rectangle<float>)>& draw)
        {
            auto line = r.removeFromTop (rowHeight);
            auto label = line.removeFromLeft (150.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (name, label.removeFromTop (label.getHeight() * 0.5f), juce::Justification::bottomLeft, true);
            drawLabel (g, detail, label.withHeight (16.0f), juce::Justification::topLeft);
            drawCentre (g, line);
            draw (line);
            r.removeFromTop (6.0f);
        };

        row ("Wave and copy", juce::String (late, late < 10.0 ? 2 : 1) + " cycles late" + (flipped ? ", flipped" : ""), [&] (juce::Rectangle<float> line)
        {
            drawLine (g, line, scale, wave, theme::accent);
            drawLine (g, line, scale, copy, theme::meterYellow, 1.5f);
        });

        row ("Together", toneText, [&] (juce::Rectangle<float> line)
        {
            drawLine (g, line, scale, [&] (double u) { return wave (u) + copy (u); }, theme::text);
        });
    }

    void paintVocals (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawCaption (g, r, "A typical mix, from the left speaker to the right", "Grey: gone");

        const auto& parts = getTypicalMix();
        const float rowHeight = 64.0f;

        // Where each part that sits at one point goes: at its pan, pushed apart so the names fit.
        constexpr float chipWidth = 66.0f, chipHeight = 20.0f, gap = 3.0f;
        auto stageOf = [] (juce::Rectangle<float> line) { return line.withTrimmedLeft (150.0f).reduced (6.0f, 0.0f); };
        const auto firstStage = stageOf (r.withHeight (rowHeight));
        std::vector<size_t> points;
        std::vector<float> xs;

        for (size_t i = 0; i < parts.size(); ++i)
            if (parts[i].width <= 0.2f)
                points.push_back (i);

        std::stable_sort (points.begin(), points.end(), [&parts] (size_t a, size_t b) { return parts[a].pan < parts[b].pan; });

        for (auto i : points)
            xs.push_back (firstStage.getCentreX() + parts[i].pan * (firstStage.getWidth() - chipWidth) * 0.5f);

        for (int pass = 0; pass < 100; ++pass)
        {
            for (size_t k = 1; k < xs.size(); ++k)
            {
                const float overlap = xs[k - 1] + chipWidth + gap - xs[k];

                if (overlap > 0.0f)
                {
                    xs[k - 1] -= overlap * 0.5f;
                    xs[k] += overlap * 0.5f;
                }
            }
        }

        // Then the whole row moved back inside the stage, keeping the gaps.
        const float leftX = firstStage.getX() + chipWidth * 0.5f, rightX = firstStage.getRight() - chipWidth * 0.5f;
        const float shift = ! xs.empty() ? juce::jmax (0.0f, leftX - xs.front()) - juce::jmax (0.0f, xs.back() - rightX) : 0.0f;
        for (auto& x : xs)
            x += shift;

        auto row = [&] (const juce::String& name, const juce::String& detail, const juce::String& result, std::function<bool (const MixPart&)> stays)
        {
            auto line = r.removeFromTop (rowHeight);
            r.removeFromTop (6.0f);

            g.setColour (theme::lane);
            g.fillRoundedRectangle (line, 4.0f);

            auto label = line.removeFromLeft (150.0f).reduced (10.0f, 6.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (name, label.removeFromTop (18.0f), juce::Justification::centredLeft, true);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText (detail + "\n" + result, label.toNearestInt(), juce::Justification::topLeft, 3);

            const auto stage = line.reduced (6.0f, 6.0f);

            // The middle, where left minus right cancels.
            g.setColour (theme::text.withAlpha (0.18f));
            g.fillRect (stage.getCentreX(), stage.getY(), 1.0f, stage.getHeight());
            drawLabel (g, "L", stage.withWidth (12.0f).withTrimmedTop (stage.getHeight() - 14.0f), juce::Justification::centredLeft);
            drawLabel (g, "R", stage.withTrimmedLeft (stage.getWidth() - 12.0f).withTrimmedTop (stage.getHeight() - 14.0f), juce::Justification::centredRight);

            auto drawPart = [&] (const MixPart& part, juce::Rectangle<float> box)
            {
                const bool kept = stays (part);
                const auto colour = part.vocals ? theme::accent : (part.low ? theme::meterGreen : theme::textDim);
                g.setColour (kept ? colour.withAlpha (0.3f) : theme::panelLight.withAlpha (0.5f));
                g.fillRoundedRectangle (box, 3.0f);
                g.setColour (kept ? colour : theme::textDim.withAlpha (0.4f));
                g.drawRoundedRectangle (box, 3.0f, 1.0f);
                g.setColour (kept ? theme::text : theme::textDim.withAlpha (0.55f));
                g.setFont (juce::FontOptions (11.5f));
                g.drawFittedText (part.name, box.toNearestInt().reduced (3, 0), juce::Justification::centred, 1);

                if (! kept)
                {
                    g.setColour (theme::textDim.withAlpha (0.6f));
                    g.drawLine (box.getX() + 4.0f, box.getCentreY(), box.getRight() - 4.0f, box.getCentreY(), 1.0f);
                }
            };

            for (size_t k = 0; k < points.size(); ++k)
                drawPart (parts[points[k]], juce::Rectangle<float> (chipWidth, chipHeight).withCentre ({ xs[k], stage.getY() + chipHeight * 0.5f }));

            // The parts spread over both speakers, as bands under the others.
            float y = stage.getY() + chipHeight + 4.0f;

            for (const auto& part : parts)
            {
                if (part.width <= 0.2f)
                    continue;

                const float half = part.width * (stage.getWidth() - 30.0f) * 0.5f;
                drawPart (part, { stage.getCentreX() - half, y, half * 2.0f, 13.0f });
                y += 15.0f;
            }
        };

        row ("The mix", "Everything plays", "", [] (const MixPart&) { return true; });
        row ("Left minus right", "The middle cancels:", "no low end, a ghost, mono", [] (const MixPart& p) { return survivesLeftMinusRight (p); });
        row ("Remove vocals", "Demucs hears what", "a voice is: the rest stays", [] (const MixPart& p) { return ! p.vocals; });

        // The clip in the waveform editor, measured.
        auto bottom = r;
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("Your clip", bottom.removeFromTop (18.0f), juce::Justification::centredLeft, true);

        if (! clip.present || ! clip.leftMinusRightMeasured)
        {
            drawNote (g, bottom, "Open an audio clip in the waveform editor to measure what left minus right leaves of it.");
            return;
        }

        const auto& m = clip.leftMinusRight;

        if (! m.stereo)
        {
            drawNote (g, bottom, clip.name + " is the same on the left and the right (mono), so left minus right leaves silence: "
                                 "everything is in the middle. Remove vocals still works on it.");
            return;
        }

        auto bar = [&] (const juce::String& name, float mixDb, float differenceDb)
        {
            auto line = bottom.removeFromTop (17.0f);
            bottom.removeFromTop (3.0f);
            drawLabel (g, name, line.removeFromLeft (100.0f), juce::Justification::centredLeft, theme::text);
            auto text = line.removeFromRight (250.0f);
            auto toWidth = [&line] (float db) { return line.getWidth() * juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };

            g.setColour (theme::textDim.withAlpha (0.45f));
            g.fillRect (juce::Rectangle<float> (line.getX(), line.getCentreY() - 6.0f, toWidth (mixDb), 6.0f));
            g.setColour (theme::accent);
            g.fillRect (juce::Rectangle<float> (line.getX(), line.getCentreY() + 1.0f, toWidth (differenceDb), 6.0f));

            const float lost = mixDb - differenceDb;
            drawLabel (g, "  " + juce::String (lost, 1) + " dB quieter (" + formatLevel (mixDb) + " to " + formatLevel (differenceDb) + ")",
                       text, juce::Justification::centredLeft, theme::text);
        };

        bar ("Whole clip", m.mixDb, m.differenceDb);
        bar ("Below " + formatHz (lowCutHz), m.mixLowDb, m.differenceLowDb);
        drawLabel (g, "Grey: " + clip.name + " as it is. Blue: left minus right. The low end is where the bass and kick drum are.",
                   bottom.removeFromTop (16.0f), juce::Justification::centredLeft);
    }

    Settings settings;
    ClipData clip;
    std::vector<juce::Point<float>> molecules;   // where each dot rests, from 0 to 1 across and down
};

//==============================================================================
SoundExplainer::SoundExplainer (Engine& e, std::function<ClipView()> clipView) : engine (e), getClipView (std::move (clipView))
{
    title.setText ("What is sound?", juce::dontSendNotification);
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

    picture = std::make_unique<Picture>();
    addAndMakeVisible (*picture);

    playButton.setClickingTogglesState (true);
    playButton.setWantsKeyboardFocus (false);
    playButton.setTooltip ("Play the guide's own plain tone, on top of anything else that plays: the bare shape, with no filter or envelope. "
                           "Change it while you listen.");
    playButton.onClick = [this]
    {
        playButton.setButtonText (playButton.getToggleState() ? "Stop tone" : "Play tone");
        updateTone();
    };
    addAndMakeVisible (playButton);

    for (const auto shape : allShapes)
        shapeBox.addItem (getShapeName (shape), (int) shape + 1);

    shapeBox.setSelectedId ((int) Shape::sine + 1, juce::dontSendNotification);
    shapeBox.setTooltip ("The shape of the wave: the same pitch, a different timbre. DawIt Subtractive plays these shapes as a real synth.");
    shapeBox.onChange = [this] { updateControls(); };
    addAndMakeVisible (shapeBox);

    frequencySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    frequencySlider.setNormalisableRange ({ lowestHeard, highestHeard,
                                            [] (double low, double high, double t) { return low * std::pow (high / low, t); },
                                            [] (double low, double high, double v) { return std::log (v / low) / std::log (high / low); } });
    frequencySlider.textFromValueFunction = [] (double hz) { return formatHz ((float) hz) + " (" + getNoteName ((float) hz) + ")"; };
    frequencySlider.valueFromTextFunction = [] (const juce::String& text)
    {
        const double value = text.retainCharacters ("0123456789.").getDoubleValue();
        return text.containsIgnoreCase ("k") ? value * 1000.0 : value;
    };
    frequencySlider.setValue (440.0, juce::dontSendNotification);
    frequencySlider.setDoubleClickReturnValue (true, 440.0);
    frequencySlider.setTooltip ("How many times a second the air goes back and forth: the pitch. Double-click for 440 Hz, the A above middle C.");

    levelSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    levelSlider.setRange (-60.0, -6.0, 0.5);
    levelSlider.textFromValueFunction = [] (double db) { return juce::String (db, 1) + " dB"; };
    levelSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters ("-.0123456789").getDoubleValue(); };
    levelSlider.setValue (-18.0, juce::dontSendNotification);
    levelSlider.setDoubleClickReturnValue (true, -18.0);
    levelSlider.setTooltip ("How big the swing is: the loudness. The tone goes up to -6 dB, so a copy on top of it still fits under 0 dB.");

    phaseSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    phaseSlider.setRange (0.0, 360.0, 1.0);
    phaseSlider.textFromValueFunction = [] (double degrees) { return juce::String (juce::roundToInt (degrees)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")); };
    phaseSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters ("0123456789.").getDoubleValue(); };
    phaseSlider.setValue (90.0, juce::dontSendNotification);
    phaseSlider.setDoubleClickReturnValue (true, 180.0);
    phaseSlider.setTooltip ("How far the copy is behind the wave, in degrees of a cycle: 180 is half a cycle. Double-click for 180.");

    delaySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    delaySlider.setRange (0.0, 10.0, 0.01);
    delaySlider.setSkewFactorFromMidPoint (2.0);
    delaySlider.textFromValueFunction = [] (double ms) { return juce::String (ms, 2) + " ms"; };
    delaySlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters ("0123456789.").getDoubleValue(); };
    delaySlider.setValue (1.0, juce::dontSendNotification);
    delaySlider.setDoubleClickReturnValue (true, 1.0);
    delaySlider.setTooltip ("How much later the copy comes, in milliseconds. Sound travels 34 cm in a millisecond. Double-click for 1 ms.");

    flipButton.setWantsKeyboardFocus (false);
    flipButton.setTooltip ("Turn the copy upside down: reverse its polarity, as the slashed-circle button on a mixing desk does");
    flipButton.onClick = [this] { updateControls(); };
    addAndMakeVisible (flipButton);

    leftMinusRightButton.setWantsKeyboardFocus (false);
    leftMinusRightButton.setTooltip ("Play only what differs between the left and the right speaker, the old trick for taking the vocals out. "
                                     "Switch it on and play the song: the middle (the voice, but also the kick, snare and bass) cancels. "
                                     "A bounce is never changed.");
    leftMinusRightButton.onClick = [this] { engine.setLeftMinusRight (leftMinusRightButton.getToggleState()); };
    addChildComponent (leftMinusRightButton);

    monoButton.setWantsKeyboardFocus (false);
    monoButton.setTooltip ("Play the song with left and right added together on both speakers, as a phone or a club's subwoofers play it. "
                           "Listen for anything that gets thin or vanishes. A bounce is never changed.");
    monoButton.onClick = [this] { engine.setMono (monoButton.getToggleState()); };
    addChildComponent (monoButton);

    stereoGuideButton.setWantsKeyboardFocus (false);
    stereoGuideButton.setTooltip ("Open What is DawIt Stereo? at Mono: phones, clubs and the sum: the correlation meter, what drops in mono, "
                                  "and keeping the low end in the middle.");
    stereoGuideButton.onClick = [this] { if (onOpenStereoGuide != nullptr) onOpenStereoGuide(); };
    addChildComponent (stereoGuideButton);

    removeVocalsButton.setWantsKeyboardFocus (false);
    removeVocalsButton.setTooltip ("Make a karaoke version of the clip in the waveform editor with Demucs, as Remove vocals in its "
                                   "right-click menu does, to compare with left minus right");
    removeVocalsButton.onClick = [this]
    {
        const auto view = getClipView != nullptr ? getClipView() : ClipView();

        if (view.clip != nullptr && onRemoveVocals != nullptr)
        {
            leftMinusRightButton.setToggleState (false, juce::sendNotification);
            onRemoveVocals (*view.clip);
        }
    };
    addChildComponent (removeVocalsButton);

    for (auto* slider : { &frequencySlider, &levelSlider, &phaseSlider, &delaySlider })
    {
        slider->setScrollWheelEnabled (false);   // scrolling never changes a value, so the page scrolls
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, slider == &frequencySlider ? 120 : 80, 20);
        slider->onValueChange = [this] { updateControls(); };
        slider->updateText();
        addAndMakeVisible (slider);
    }

    for (auto* label : { &frequencyLabel, &levelLabel, &phaseLabel, &delayLabel })
    {
        label->setColour (juce::Label::textColourId, theme::textDim);
        label->setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (label);
    }

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
    showTopic (Topic::air);
    startTimerHz (30);
    setSize (width, height);
}

SoundExplainer::~SoundExplainer()
{
    engine.setTone ({});
    engine.setLeftMinusRight (false);
    engine.setMono (false);
    list.setModel (nullptr);
}

juce::String SoundExplainer::getPrintSource() const
{
    if (clipData.example)
        return "a made-up example clip (no clip in the wave editor)";

    return clipData.present ? "the clip " + clipData.name + " in the wave editor" : juce::String ("a made-up tone (no clip in the wave editor)");
}

int SoundExplainer::getNumPrintTopics() const
{
    return (int) std::size (allTopics);
}

booklet::Topic SoundExplainer::showPrintTopic (int index)
{
    if (! topicBeforePrinting)
        topicBeforePrinting = topic;

    showTopic (allTopics[juce::jlimit (0, getNumPrintTopics() - 1, index)]);
    return { getTopicName (topic), getTopicSummary (topic), explanation.getText(), { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image SoundExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void SoundExplainer::endPrinting()
{
    if (topicBeforePrinting)
        showTopic (*topicBeforePrinting);

    topicBeforePrinting.reset();
}

void SoundExplainer::showTopic (Topic newTopic)
{
    topic = newTopic;
    const auto index = (int) (std::find (std::begin (allTopics), std::end (allTopics), topic) - std::begin (allTopics));

    if (list.getSelectedRow() != index)
        list.selectRow (index);   // comes back through selectedRowsChanged

    explanation.setText (explain (topic), false);
    explanation.moveCaretToTop (false);

    phaseSlider.setVisible (topic == Topic::phase);
    phaseLabel.setVisible (topic == Topic::phase);
    delaySlider.setVisible (topic == Topic::cancellation);
    delayLabel.setVisible (topic == Topic::cancellation);
    flipButton.setVisible (topic == Topic::cancellation);

    // Taking the vocals out and Checking in mono play the song, not the tone.
    const bool vocals = topic == Topic::vocals, mono = topic == Topic::mono, playsSong = vocals || mono;

    if (playsSong && playButton.getToggleState())
        playButton.setToggleState (false, juce::sendNotification);

    if (! vocals && leftMinusRightButton.getToggleState())
        leftMinusRightButton.setToggleState (false, juce::sendNotification);

    if (! mono && monoButton.getToggleState())
        monoButton.setToggleState (false, juce::sendNotification);

    for (auto* c : std::initializer_list<juce::Component*> { &playButton, &shapeBox, &levelLabel, &levelSlider, &frequencyLabel, &frequencySlider })
        c->setVisible (! playsSong);

    leftMinusRightButton.setVisible (vocals);
    removeVocalsButton.setVisible (vocals);
    monoButton.setVisible (mono);
    stereoGuideButton.setVisible (mono);
    resized();
    updateControls();
    updateClip();
}

void SoundExplainer::updateControls()
{
    Picture::Settings settings;
    settings.topic = topic;
    settings.shape = (Shape) juce::jmax (0, shapeBox.getSelectedId() - 1);
    settings.frequency = (float) frequencySlider.getValue();
    settings.levelDb = (float) levelSlider.getValue();
    settings.phaseDegrees = (float) phaseSlider.getValue();
    settings.delayMs = (float) delaySlider.getValue();
    settings.flipped = flipButton.getToggleState();
    picture->setSettings (settings);
    updateTone();
}

void SoundExplainer::updateTone()
{
    Engine::Tone tone;
    tone.on = playButton.getToggleState();
    tone.shape = (Shape) juce::jmax (0, shapeBox.getSelectedId() - 1);
    tone.frequency = (float) frequencySlider.getValue();
    tone.levelDb = (float) levelSlider.getValue();
    tone.withCopy = topic == Topic::phase || topic == Topic::cancellation;

    // A copy a time later is that many of the tone's cycles late (not kept within one cycle, so it glides smoothly).
    tone.phaseDegrees = topic == Topic::cancellation ? (float) (360.0 * frequencySlider.getValue() * delaySlider.getValue() / 1000.0)
                                                     : (float) phaseSlider.getValue();
    tone.flipCopy = topic == Topic::cancellation && flipButton.getToggleState();
    engine.setTone (tone);
}

void SoundExplainer::updateClip()
{
    const auto view = getClipView != nullptr ? getClipView() : ClipView();
    const auto* clip = view.clip;

    removeVocalsButton.setEnabled (clip != nullptr && clip->audio.getNumSamples() > 0 && onRemoveVocals != nullptr);

    // With no clip, Timbre and Checking in mono use the made-up example clip, at 1.0 s (inside its first held note).
    const bool noClip = clip == nullptr || clip->audio.getNumSamples() == 0;
    const bool useExample = noClip && (topic == Topic::timbre || topic == Topic::mono);
    int requestedFocus = view.focusSample;

    if (useExample)
    {
        clip = &getExampleClip();
        requestedFocus = juce::roundToInt (1.0 * clip->sourceSampleRate);
    }

    if (noClip && ! useExample)
    {
        measuredClip = nullptr;
        measuredStereoClip = nullptr;

        if (clipData.present)
        {
            clipData = {};
            picture->setClipData (clipData);
        }

        hintLabel.setText ("Select an audio clip to use your own sound as the example.", juce::dontSendNotification);
        return;
    }

    const auto& audio = clip->audio;
    const int numSamples = audio.getNumSamples();
    const float gain = clip->getGain();
    clipData.present = true;
    clipData.example = useExample;
    clipData.name = clip->name;
    clipData.rate = clip->sourceSampleRate;
    clipData.channels = audio.getNumChannels();
    clipData.numSamples = numSamples;
    hintLabel.setText (useExample ? "Select an audio clip to use your own sound; this is a made-up example clip."
                                  : "The examples use " + clip->name + ", the clip in the waveform editor.", juce::dontSendNotification);

    // The whole clip's peak and RMS, when it changed.
    if (clip != measuredClip || numSamples != measuredSamples || ! juce::approximatelyEqual (clip->gainDb, measuredGain))
    {
        double sum = 0.0;
        float peak = 0.0f;

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            const float* data = audio.getReadPointer (ch);

            for (int i = 0; i < numSamples; ++i)
            {
                sum += (double) data[i] * data[i];
                peak = juce::jmax (peak, std::abs (data[i]));
            }
        }

        clipData.peakDb = juce::Decibels::gainToDecibels (peak * gain, -100.0f);
        clipData.rmsDb = juce::Decibels::gainToDecibels ((float) std::sqrt (sum / ((double) numSamples * audio.getNumChannels())) * gain, -100.0f);
        measuredClip = clip;
        measuredSamples = numSamples;
        measuredGain = clip->gainDb;
    }

    // The outline, from the clip's overview.
    constexpr int columns = 700;
    const int numBlocks = (int) clip->peakMax.size();
    clipData.overviewMin.assign (columns, 0.0f);
    clipData.overviewMax.assign (columns, 0.0f);

    for (int c = 0; c < columns && numBlocks > 0; ++c)
    {
        const int b0 = c * numBlocks / columns, b1 = juce::jmax (b0 + 1, (c + 1) * numBlocks / columns);

        for (int b = b0; b < juce::jmin (b1, numBlocks); ++b)
        {
            clipData.overviewMin[(size_t) c] = juce::jmin (clipData.overviewMin[(size_t) c], clip->peakMin[(size_t) b] * gain);
            clipData.overviewMax[(size_t) c] = juce::jmax (clipData.overviewMax[(size_t) c], clip->peakMax[(size_t) b] * gain);
        }
    }

    // Where the editor looks: the stretch, its pitch and its spectrum, for the topics that show them.
    const int focus = juce::jlimit (0, numSamples - 1, requestedFocus);
    clipData.focusSample = focus;
    clipData.window.clear();
    clipData.spectrumDb.clear();
    clipData.pitchHz = 0.0f;

    if (topic == Topic::frequency || topic == Topic::timbre)
    {
        const auto stretch = WaveformView::readMono (*clip, focus, (int) (0.05 * clipData.rate));
        clipData.pitchHz = estimatePitch (stretch.data(), (int) stretch.size(), clipData.rate);
    }

    // Left minus right over the whole clip, measured once for each clip.
    if (topic == Topic::vocals && (clip != measuredStereoClip || numSamples != measuredStereoSamples))
    {
        clipData.leftMinusRight = measureLeftMinusRight (audio, clipData.rate);
        clipData.leftMinusRightMeasured = true;
        measuredStereoClip = clip;
        measuredStereoSamples = numSamples;
    }

    // Checking in mono: the whole clip, measured once for each clip.
    if (topic == Topic::mono && (clip != measuredMonoClip || numSamples != measuredMonoSamples))
    {
        clipData.mono = measureMono (audio, clipData.rate);
        clipData.monoMeasured = true;
        measuredMonoClip = clip;
        measuredMonoSamples = numSamples;
    }

    // The envelope of the note at the focus, measured again when the focus, the clip or its gain changes.
    if (topic == Topic::timbre && (clip != envelopeClip || numSamples != envelopeSamples || focus != envelopeFocus
                                   || ! juce::approximatelyEqual (clip->gainDb, envelopeGain)))
    {
        clipData.envelope = measureEnvelope (audio, clipData.rate, focus, gain);
        clipData.envelopeMeasured = true;
        envelopeClip = clip;
        envelopeSamples = numSamples;
        envelopeFocus = focus;
        envelopeGain = clip->gainDb;
    }

    if (topic == Topic::frequency)
        clipData.window = WaveformView::readMono (*clip, focus, juce::jmax (2, (int) (getWindowSeconds ((float) frequencySlider.getValue()) * clipData.rate)));

    if (topic == Topic::timbre)
    {
        auto samples = WaveformView::readMono (*clip, focus, fftSize);
        juce::dsp::WindowingFunction<float> (fftSize, juce::dsp::WindowingFunction<float>::hann, false).multiplyWithWindowingTable (samples.data(), fftSize);
        std::vector<float> data (2 * fftSize, 0.0f);
        std::copy (samples.begin(), samples.end(), data.begin());
        juce::dsp::FFT (fftOrder).performFrequencyOnlyForwardTransform (data.data(), true);

        // A full-scale sine through the Hann window comes out at a quarter of the size.
        clipData.spectrumDb.resize (fftSize / 2);

        for (int b = 0; b < fftSize / 2; ++b)
            clipData.spectrumDb[(size_t) b] = juce::Decibels::gainToDecibels (data[(size_t) b] * 4.0f / fftSize, spectrumFloorDb);
    }

    picture->setClipData (clipData);
}

const AudioClip& SoundExplainer::getExampleClip()
{
    if (exampleClip == nullptr)
    {
        exampleClip = std::make_unique<AudioClip>();
        exampleClip->name = "Example clip (made up)";
        exampleClip->sourceSampleRate = 44100.0;
        exampleClip->audio = makeExampleClip (exampleClip->sourceSampleRate);
        exampleClip->buildPeaks();
    }

    return *exampleClip;
}

int SoundExplainer::getNumRows()
{
    return (int) std::size (allTopics);
}

void SoundExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto rowTopic = allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (getTopicName (rowTopic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (getTopicSummary (rowTopic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void SoundExplainer::selectedRowsChanged (int row)
{
    if (row >= 0 && row < getNumRows())
        showTopic (allTopics[row]);
}

void SoundExplainer::timerCallback()
{
    // The air moves all the time; the clip is looked at again a few times a second.
    if (topic == Topic::air)
        picture->repaint();

    if (++ticks % 3 == 0)
        updateClip();
}

void SoundExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void SoundExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    title.setBounds (top.removeFromLeft (250));
    exportButton.setBounds (top.removeFromRight (120));
    top.removeFromRight (10);
    hintLabel.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    picture->setBounds (r.removeFromTop (pictureHeight));
    r.removeFromTop (10);

    auto first = r.removeFromTop (26);

    if (monoButton.isVisible())
    {
        monoButton.setBounds (first.removeFromLeft (150));
        first.removeFromLeft (8);
        stereoGuideButton.setBounds (first.removeFromLeft (256));
        r.removeFromTop (10);
        explanation.setBounds (r);
        return;
    }

    if (leftMinusRightButton.isVisible())
    {
        leftMinusRightButton.setBounds (first.removeFromLeft (190));
        first.removeFromLeft (8);
        removeVocalsButton.setBounds (first.removeFromLeft (230));
        r.removeFromTop (10);
        explanation.setBounds (r);
        return;
    }

    playButton.setBounds (first.removeFromLeft (96));
    first.removeFromLeft (8);
    shapeBox.setBounds (first.removeFromLeft (110));
    first.removeFromLeft (8);

    if (flipButton.isVisible())
    {
        flipButton.setBounds (first.removeFromRight (130));
        first.removeFromRight (8);
    }

    levelLabel.setBounds (first.removeFromLeft (50));
    levelSlider.setBounds (first);
    r.removeFromTop (6);

    auto second = r.removeFromTop (26);
    frequencyLabel.setBounds (second.removeFromLeft (76));

    for (auto [slider, label] : { std::pair { &phaseSlider, &phaseLabel }, std::pair { &delaySlider, &delayLabel } })
    {
        if (slider->isVisible())
        {
            slider->setBounds (second.removeFromRight (240));
            label->setBounds (second.removeFromRight (56));
            second.removeFromRight (8);
        }
    }

    frequencySlider.setBounds (second);
    r.removeFromTop (10);

    explanation.setBounds (r);
}
} // namespace daw
