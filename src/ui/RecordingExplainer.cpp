#include "RecordingExplainer.h"
#include "GainStagingExplainer.h"

namespace daw
{
namespace rg = recordingguide;

namespace
{
/** What the pictures show when there is no device yet: 128 samples at 48 kHz. */
constexpr int exampleBuffer = 128;
constexpr double exampleRate = 48000.0;

void drawBox (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, const juce::String& detail, juce::Colour edge)
{
    g.setColour (theme::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.5f);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawFittedText (name, r.toNearestInt().reduced (6, 6).removeFromTop (20), juce::Justification::centred, 1, 0.8f);

    if (detail.isNotEmpty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText (detail, r.toNearestInt().reduced (6, 6).withTrimmedTop (22), juce::Justification::centredTop, 4, 0.85f);
    }
}

void drawArrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, juce::Colour colour)
{
    g.setColour (colour);
    g.drawLine (juce::Line<float> (from, to), 1.6f);
    juce::Path head;
    head.addTriangle (to, to + juce::Point<float> (-6.0f, -4.0f).rotatedAboutOrigin ((float) std::atan2 (to.y - from.y, to.x - from.x)),
                      to + juce::Point<float> (-6.0f, 4.0f).rotatedAboutOrigin ((float) std::atan2 (to.y - from.y, to.x - from.x)));
    g.fillPath (head);
}

void drawLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, float size, juce::Colour colour,
                juce::Justification just = juce::Justification::centred, bool bold = false)
{
    g.setColour (colour);
    g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText (text, r.toNearestInt(), just, 2, 0.8f);
}

juce::String voltsText (double volts)
{
    return volts < 0.1 ? juce::String (volts * 1000.0, 1) + " mV" : juce::String (volts, 3) + " V";
}

juce::String dbText (double db)
{
    return db < -90.0 ? juce::String ("no signal") : juce::String (db, 1) + " dB";
}
} // namespace

//==============================================================================
/** The topic's picture, drawn from the example data and, where it helps, the user's own device and input level. */
class RecordingExplainer::Picture final : public juce::Component,
                                          public juce::SettableTooltipClient
{
public:
    void setState (rg::Topic t, const rg::Snapshot& s)
    {
        topic = t;
        snapshot = s;
        meters.resize (snapshot.armed.size());

        for (size_t i = 0; i < meters.size(); ++i)
            meters[i].update (juce::Decibels::decibelsToGain (snapshot.armed[i].inputPeakDb, -100.0f));

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        switch (topic)
        {
            case rg::Topic::air:            paintAir (g); break;
            case rg::Topic::audioInterface: paintInterface (g); break;
            case rg::Topic::chooseInput:    paintSockets (g); break;
            case rg::Topic::level:          paintLevel (g); break;
            case rg::Topic::buffers:        paintBuffers (g); break;
            case rg::Topic::monitoring:     paintMonitoring (g); break;
            case rg::Topic::moved:          paintMoved (g); break;
            case rg::Topic::takes:          paintTakes (g); break;
            case rg::Topic::yourSession:    paintSession (g); break;
        }
    }

private:
    juce::Rectangle<float> getArea() const { return getLocalBounds().toFloat().reduced (16.0f, 12.0f); }

    int getBufferSamples() const { return snapshot.bufferSamples > 0 ? snapshot.bufferSamples : exampleBuffer; }
    double getRate() const { return snapshot.sampleRate > 0.0 ? snapshot.sampleRate : exampleRate; }

    /** The round trip of the user's device, or of the example when there is none. */
    double getRoundTrip() const
    {
        return snapshot.sampleRate > 0.0 ? rg::getRoundTripMs (snapshot) : rg::getMinimumRoundTripMs (exampleBuffer, exampleRate);
    }

    /** The first armed track's input level, with its number, or a hint when nothing is armed. */
    void drawLiveMeter (juce::Graphics& g, juce::Rectangle<float> r)
    {
        if (meters.empty())
        {
            recordingui::InputMeter none;
            recordingui::drawInputMeter (g, r.withTrimmedRight (110.0f).reduced (0.0f, 2.0f), none, false);
            drawLabel (g, r.removeFromRight (106.0f), "no track armed", 12.0f, theme::textDim, juce::Justification::centredRight);
            return;
        }

        recordingui::drawInputMeter (g, r.withTrimmedRight (110.0f).reduced (0.0f, 2.0f), meters[0], false);
        drawLabel (g, r.removeFromRight (106.0f), dbText (snapshot.armed[0].inputPeakDb), 12.5f, theme::text, juce::Justification::centredRight, true);
    }

    void paintAir (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "Five stops between your voice and the take", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (10.0f);

        const juce::String names[] { "Microphone", "Interface", "Buffer", "DawIt", "WAV file" };
        const juce::String details[] { "a few thousandths of a volt: about 2.5 mV",
                                       "the preamp makes it bigger, the converter turns it into numbers",
                                       juce::String (getBufferSamples()) + " samples at a time, " + juce::String (rg::getBufferMs (getBufferSamples(), getRate()), 1) + " ms",
                                       "takes a batch in and sends a batch of the song out",
                                       "24 bits, written while you play; a clip when you stop" };
        const float gap = 24.0f, boxWidth = (area.getWidth() - 4.0f * gap) / 5.0f, boxHeight = 132.0f;

        for (int i = 0; i < 5; ++i)
        {
            const auto box = juce::Rectangle<float> (area.getX() + (float) i * (boxWidth + gap), area.getY(), boxWidth, boxHeight);
            drawBox (g, box, names[i], details[i], i == 1 ? theme::accent : theme::textDim);

            if (i < 4)
                drawArrow (g, { box.getRight() + 3.0f, box.getCentreY() }, { box.getRight() + gap - 3.0f, box.getCentreY() }, theme::textDim);
        }

        // The converter box shows the live input and the device.
        auto box = juce::Rectangle<float> (area.getX() + (boxWidth + gap), area.getY(), boxWidth, boxHeight);
        auto bottom = box.reduced (8.0f).removeFromBottom (16.0f);
        recordingui::drawInputMeter (g, bottom, meters.empty() ? recordingui::InputMeter() : meters[0], false);

        area.removeFromTop (boxHeight + 10.0f);
        const auto device = snapshot.inputDevice.isEmpty() ? juce::String ("No input device found") : snapshot.inputDevice;
        drawLabel (g, area.removeFromTop (20.0f), "Interface: " + device + (snapshot.sampleRate > 0.0 ? ", " + juce::String (snapshot.sampleRate / 1000.0, 1) + " kHz" : juce::String()),
                   13.0f, theme::text, juce::Justification::centredLeft);
        drawLabel (g, area.removeFromTop (20.0f), meters.empty() ? juce::String ("Arm an audio track with its R button to see the level coming in.")
                                                                 : "Input level now: " + dbText (snapshot.armed[0].inputPeakDb),
                   13.0f, theme::textDim, juce::Justification::centredLeft);
        area.removeFromTop (8.0f);
        drawLabel (g, area, "What is saved is the input as it came in. The track's effects and fader change what you hear, not the take.",
                   12.5f, theme::textDim, juce::Justification::topLeft);
    }

    void paintInterface (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "How strong each kind of signal is, on a scale where each step is ten times more", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (8.0f);

        const auto levels = rg::getSignalLevels();
        const double lowV = 0.001, highV = 2.0;
        const float labelWidth = 190.0f;
        const auto bars = area.withTrimmedLeft (labelWidth).withTrimmedRight (86.0f);
        auto xOf = [&] (double volts) { return bars.getX() + bars.getWidth() * (float) (std::log10 (volts / lowV) / std::log10 (highV / lowV)); };
        const float rowHeight = 46.0f;

        // Decade lines.
        for (double v = lowV; v <= 1.0001; v *= 10.0)
        {
            g.setColour (theme::grid);
            g.fillRect (xOf (v), bars.getY(), 1.0f, rowHeight * (float) levels.size());
            drawLabel (g, juce::Rectangle<float> (xOf (v) - 30.0f, bars.getY() + rowHeight * (float) levels.size() + 2.0f, 60.0f, 16.0f), v < 0.9 ? juce::String (juce::roundToInt (v * 1000.0)) + " mV" : juce::String ("1 V"), 11.0f, theme::textDim);
        }

        double micVolts = 0.0, lineVolts = 0.0;
        float y = bars.getY();
        const juce::Colour colours[] { theme::accent, theme::meterYellow, theme::meterGreen, theme::meterGreen };
        int index = 0;

        for (auto& level : levels)
        {
            drawLabel (g, juce::Rectangle<float> (area.getX(), y, labelWidth - 8.0f, rowHeight - 8.0f), level.name, 12.5f, theme::text, juce::Justification::centredLeft, true);

            if (level.hasValue)
            {
                if (micVolts <= 0.0)
                    micVolts = level.volts;

                lineVolts = level.volts;
                g.setColour (colours[index % 4]);
                g.fillRoundedRectangle (bars.getX(), y + 6.0f, juce::jmax (3.0f, xOf (level.volts) - bars.getX()), rowHeight - 20.0f, 3.0f);
                drawLabel (g, juce::Rectangle<float> (xOf (level.volts) + 6.0f, y + 4.0f, 80.0f, rowHeight - 16.0f), voltsText (level.volts), 12.0f, theme::text, juce::Justification::centredLeft, true);
            }
            else
            {
                // No figure: it differs for every guitar, so a range between the two neighbours.
                g.setColour (colours[index % 4].withAlpha (0.35f));
                g.fillRoundedRectangle (xOf (0.02), y + 6.0f, xOf (0.3) - xOf (0.02), rowHeight - 20.0f, 3.0f);
                drawLabel (g, juce::Rectangle<float> (xOf (0.3) + 6.0f, y + 4.0f, 100.0f, rowHeight - 16.0f), "differs for every guitar", 11.5f, theme::textDim, juce::Justification::centredLeft);
            }

            y += rowHeight;
            ++index;
        }

        if (micVolts > 0.0 && lineVolts > 0.0)
            drawLabel (g, juce::Rectangle<float> (area.getX(), y + 22.0f, area.getWidth(), 22.0f),
                       "From a microphone to studio line level is " + juce::String (rg::getGainDb (micVolts, rg::getVoltsFromDbu (4.0)), 1) + " dB of gain, about "
                           + juce::String (juce::roundToInt (rg::getVoltsFromDbu (4.0) / micVolts)) + " times.",
                       13.0f, theme::text, juce::Justification::centredLeft);
    }

    void paintSockets (juce::Graphics& g)
    {
        auto area = getArea();
        const int numSockets = juce::jlimit (2, 8, snapshot.numInputs);
        drawLabel (g, area.removeFromTop (22.0f), snapshot.numInputs > 0 ? "Your interface's inputs and the track that records each"
                                                                         : "An interface's inputs and the track that records each (an example)",
                   14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (8.0f);

        const float rowHeight = juce::jmin (36.0f, (area.getHeight() - 40.0f) / (float) numSockets);
        const float socketX = area.getX() + 20.0f, trackX = area.getRight() - 250.0f;
        std::vector<juce::Rectangle<float>> sockets;

        for (int i = 0; i < numSockets; ++i)
        {
            const auto r = juce::Rectangle<float> (socketX, area.getY() + (float) i * rowHeight, 30.0f, rowHeight - 6.0f);
            sockets.push_back (r);
            g.setColour (theme::panel);
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (theme::textDim);
            g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.2f);
            drawLabel (g, r, juce::String (i + 1), 13.0f, theme::text, juce::Justification::centred, true);
            drawLabel (g, r.translated (38.0f, 0.0f).withWidth (80.0f), "Input " + juce::String (i + 1), 12.0f, theme::textDim, juce::Justification::centredLeft);
        }

        // The armed tracks, or two examples when nothing is armed.
        std::vector<rg::ArmedTrack> shown = snapshot.armed;
        const bool example = shown.empty();

        if (example)
        {
            shown.push_back ({ "Vocals", 0, false, false, -100.0f });
            shown.push_back ({ "Guitar", 1, false, false, -100.0f });
        }

        float trackY = area.getY();

        for (auto& t : shown)
        {
            const bool stereo = t.stereo && t.inputChannel + 1 < numSockets;
            const auto box = juce::Rectangle<float> (trackX, trackY, 240.0f, 34.0f);
            const auto colour = example ? theme::textDim : theme::meterRed;
            g.setColour (theme::panel);
            g.fillRoundedRectangle (box, 5.0f);
            g.setColour (colour);
            g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.5f);
            drawLabel (g, box.reduced (8.0f, 2.0f), t.name + " · " + rg::describeInput (t.inputChannel, stereo), 12.5f, theme::text, juce::Justification::centredLeft, true);

            for (int c = t.inputChannel; c <= t.inputChannel + (stereo ? 1 : 0) && c < numSockets; ++c)
                drawArrow (g, { sockets[(size_t) c].getRight() + 90.0f, sockets[(size_t) c].getCentreY() }, { trackX - 3.0f, box.getCentreY() }, colour.withAlpha (0.8f));

            trackY += 44.0f;
        }

        drawLabel (g, juce::Rectangle<float> (area.getX(), area.getBottom() - 34.0f, area.getWidth(), 34.0f),
                   "One microphone or guitar is one channel: record it mono. A stereo pair is for sources that really are two channels.",
                   12.5f, theme::textDim, juce::Justification::centredLeft);
    }

    void paintLevel (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "The same phrase at three levels", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (8.0f);

        const auto examples = rg::getLevelExamples();
        const float gap = 14.0f, panelWidth = (area.getWidth() - 2.0f * gap) / (float) juce::jmax (1, (int) examples.size());
        const float panelHeight = area.getHeight() - 22.0f - 38.0f - 10.0f;

        for (size_t i = 0; i < examples.size(); ++i)
        {
            auto& e = examples[i];
            const auto panel = juce::Rectangle<float> (area.getX() + (float) i * (panelWidth + gap), area.getY(), panelWidth, panelHeight);
            const auto colour = e.clipped ? theme::meterRed : gainstaging::getZoneColour (gainstaging::getZone ((float) e.peakDb));
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (panel, 4.0f);

            // 0 dBFS is the top and bottom edge.
            g.setColour (theme::meterRed.withAlpha (0.5f));
            g.fillRect (panel.getX(), panel.getY(), panel.getWidth(), 1.0f);
            g.fillRect (panel.getX(), panel.getBottom() - 1.0f, panel.getWidth(), 1.0f);
            g.setColour (theme::grid);
            g.fillRect (panel.getX(), panel.getCentreY(), panel.getWidth(), 1.0f);

            const int numPoints = juce::jmax (2, (int) panel.getWidth());
            const auto wave = rg::getPhrase (e, numPoints);
            juce::Path path;

            for (int p = 0; p < numPoints; ++p)
            {
                const float x = panel.getX() + (float) p;
                const float y = panel.getCentreY() - wave[(size_t) p] * (panel.getHeight() * 0.5f - 2.0f);

                if (p == 0)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }

            g.setColour (colour);
            g.strokePath (path, juce::PathStrokeType (1.4f));
            drawLabel (g, panel.translated (0.0f, panelHeight + 2.0f).withHeight (18.0f), e.label, 12.5f, theme::text, juce::Justification::centred, true);
            drawLabel (g, panel.translated (0.0f, panelHeight + 20.0f).withHeight (16.0f), "peak " + juce::String (e.peakDb, 0) + " dB", 12.0f, colour, juce::Justification::centred);
        }

        area.removeFromTop (panelHeight + 42.0f);
        drawLabel (g, area.removeFromLeft (120.0f), "Your input now", 12.5f, theme::textDim, juce::Justification::centredLeft);
        drawLiveMeter (g, area);
    }

    void paintBuffers (juce::Graphics& g)
    {
        auto area = getArea();
        const double bufferMs = rg::getBufferMs (getBufferSamples(), getRate());
        const double roundTrip = getRoundTrip();
        const double extra = juce::jmax (0.0, roundTrip - 2.0 * bufferMs);
        const bool own = snapshot.sampleRate > 0.0;

        drawLabel (g, area.removeFromTop (22.0f), (own ? juce::String ("Your device: ") : juce::String ("An example: ")) + juce::String (getBufferSamples()) + " samples at "
                                                       + juce::String (getRate() / 1000.0, 1) + " kHz",
                   14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);

        // The trip as three stretches of time.
        auto line = area.removeFromTop (54.0f);
        const double total = bufferMs * 2.0 + extra + bufferMs * 0.0;
        const juce::String names[] { "in buffer", "converters and driver", "out buffer" };
        const double parts[] { bufferMs, extra, bufferMs };
        float x = line.getX();

        for (int i = 0; i < 3; ++i)
        {
            if (parts[i] <= 0.0)
                continue;

            const float w = line.getWidth() * (float) (parts[i] / total);
            const auto r = juce::Rectangle<float> (x, line.getY(), w - 2.0f, 34.0f);
            g.setColour (i == 1 ? theme::meterYellow.withAlpha (0.6f) : theme::accent.withAlpha (0.55f));
            g.fillRoundedRectangle (r, 4.0f);
            drawLabel (g, r, juce::String (parts[i], 1) + " ms", 12.5f, theme::text, juce::Justification::centred, true);
            drawLabel (g, juce::Rectangle<float> (x, line.getY() + 35.0f, w - 2.0f, 16.0f), names[i], 11.0f, theme::textDim);
            x += w;
        }

        drawLabel (g, area.removeFromTop (22.0f), "Round trip " + juce::String (roundTrip, 1) + " ms: " + juce::String (rg::getDistanceMetres (roundTrip), 1) + " m of sound travelling in air",
                   12.5f, theme::text, juce::Justification::centredLeft);
        area.removeFromTop (6.0f);

        // The table of buffer sizes at this rate.
        const auto table = rg::getBufferTable (getRate(), extra);
        const float rowHeight = juce::jmin (28.0f, area.getHeight() / (float) juce::jmax (1, (int) table.size()));
        double maxTrip = 1.0;

        for (auto& row : table)
            maxTrip = juce::jmax (maxTrip, row.roundTripMs);

        float y = area.getY();

        for (auto& row : table)
        {
            const bool current = row.samples == getBufferSamples();
            drawLabel (g, juce::Rectangle<float> (area.getX(), y, 90.0f, rowHeight - 4.0f), juce::String (row.samples) + " samples", 12.5f, current ? theme::accent : theme::text,
                       juce::Justification::centredLeft, current);
            const float barWidth = (area.getWidth() - 270.0f) * (float) (row.roundTripMs / maxTrip);
            g.setColour (current ? theme::accent : theme::textDim.withAlpha (0.55f));
            g.fillRoundedRectangle (area.getX() + 96.0f, y + 3.0f, juce::jmax (3.0f, barWidth), rowHeight - 10.0f, 3.0f);
            drawLabel (g, juce::Rectangle<float> (area.getRight() - 164.0f, y, 164.0f, rowHeight - 4.0f),
                       juce::String (row.bufferMs, 1) + " ms buffer, " + juce::String (row.roundTripMs, 1) + " ms trip", 12.0f, theme::text, juce::Justification::centredRight);
            y += rowHeight;
        }
    }

    void paintMonitoring (juce::Graphics& g)
    {
        auto area = getArea();
        drawLabel (g, area.removeFromTop (22.0f), "Two ways to hear yourself, and one to avoid", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (8.0f);

        const float rowHeight = 78.0f, boxWidth = 118.0f, boxHeight = 44.0f;
        const double roundTrip = getRoundTrip();

        auto drawPath = [&] (float y, const juce::StringArray& names, const juce::String& heading, const juce::String& result, juce::Colour colour)
        {
            drawLabel (g, juce::Rectangle<float> (area.getX(), y, area.getWidth(), 18.0f), heading, 12.5f, colour, juce::Justification::centredLeft, true);
            const float bw = juce::jmin (boxWidth, (area.getWidth() - 110.0f) / (1.5f * (float) names.size()));   // narrower for the five-box path
            const float step = (area.getWidth() - 110.0f - bw) / (float) juce::jmax (1, names.size() - 1);

            for (int i = 0; i < names.size(); ++i)
            {
                const auto box = juce::Rectangle<float> (area.getX() + (float) i * step, y + 22.0f, bw, boxHeight);
                drawBox (g, box, names[i], {}, theme::textDim);

                if (i + 1 < names.size())
                    drawArrow (g, { box.getRight() + 3.0f, box.getCentreY() }, { box.getRight() + step - bw - 3.0f, box.getCentreY() }, colour);
            }

            drawLabel (g, juce::Rectangle<float> (area.getRight() - 100.0f, y + 22.0f, 100.0f, boxHeight), result, 13.0f, colour, juce::Justification::centredRight, true);
        };

        drawPath (area.getY(), { "Microphone", "Interface", "Headphones" }, "Direct monitoring: the interface sends the input straight to the headphones", "0 ms", theme::meterGreen);
        drawPath (area.getY() + rowHeight, { "Microphone", "Interface", "DawIt", "Interface", "Headphones" }, "Monitor in DawIt: through the track's effects and fader",
                  juce::String (roundTrip, 1) + " ms late", theme::meterYellow);

        // The loop to avoid: speakers into the microphone.
        const float y = area.getY() + 2.0f * rowHeight;
        drawLabel (g, juce::Rectangle<float> (area.getX(), y, area.getWidth(), 18.0f), "Speakers instead of headphones: the microphone hears itself", 12.5f, theme::meterRed, juce::Justification::centredLeft, true);
        const auto mic = juce::Rectangle<float> (area.getX(), y + 22.0f, boxWidth, boxHeight);
        const auto speaker = juce::Rectangle<float> (area.getX() + 2.0f * (boxWidth + 40.0f), y + 22.0f, boxWidth, boxHeight);
        drawBox (g, mic, "Microphone", {}, theme::textDim);
        drawBox (g, speaker, "Speaker", {}, theme::textDim);
        drawArrow (g, { mic.getRight() + 3.0f, mic.getCentreY() }, { speaker.getX() - 3.0f, speaker.getCentreY() }, theme::meterRed);
        juce::Path back;
        back.startNewSubPath (speaker.getCentreX(), speaker.getBottom() + 2.0f);
        back.lineTo (speaker.getCentreX(), speaker.getBottom() + 14.0f);
        back.lineTo (mic.getCentreX(), speaker.getBottom() + 14.0f);
        back.lineTo (mic.getCentreX(), mic.getBottom() + 3.0f);
        g.setColour (theme::meterRed);
        g.strokePath (back, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
        drawLabel (g, juce::Rectangle<float> (speaker.getRight() + 16.0f, y + 22.0f, 200.0f, boxHeight), "round and round: howl", 13.0f, theme::meterRed, juce::Justification::centredLeft, true);
    }

    void paintMoved (juce::Graphics& g)
    {
        auto area = getArea();
        const double roundTrip = getRoundTrip();
        const bool measured = snapshot.sampleRate > 0.0 && snapshot.measured && std::abs (snapshot.measuredExtraMs) >= 0.05;
        const double extra = measured ? snapshot.measuredExtraMs : 0.0;
        const double reported = roundTrip - extra;

        drawLabel (g, area.removeFromTop (22.0f), "A click played on the beat, and where DawIt puts it: " + juce::String (roundTrip, 1) + " ms earlier",
                   14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (10.0f);

        const float labelWidth = 150.0f;
        const auto axis = area.withTrimmedLeft (labelWidth).withTrimmedRight (20.0f);
        const double spanMs = juce::jmax (12.0, roundTrip * 2.0);
        auto xOf = [&] (double ms) { return axis.getX() + axis.getWidth() * (float) (ms / spanMs); };
        const float rowHeight = 70.0f;

        // The beat.
        g.setColour (theme::gridBar);
        g.fillRect (xOf (0.0) - 0.5f, axis.getY(), 1.5f, rowHeight * 3.0f);
        drawLabel (g, juce::Rectangle<float> (xOf (0.0) + 4.0f, axis.getY() - 2.0f, 60.0f, 14.0f), "the beat", 11.0f, theme::textDim, juce::Justification::centredLeft);

        const juce::String names[] { "You play on the beat", "It comes back in late", "DawIt moves it back" };
        const double clickMs[] { 0.0, roundTrip, 0.0 };

        for (int i = 0; i < 3; ++i)
        {
            const float y = axis.getY() + (float) i * rowHeight + 16.0f;
            drawLabel (g, juce::Rectangle<float> (area.getX(), y, labelWidth - 6.0f, 30.0f), names[i], 12.5f, theme::text, juce::Justification::centredLeft, true);
            g.setColour (theme::grid);
            g.fillRect (axis.getX(), y + 15.0f, axis.getWidth(), 1.0f);
            g.setColour (i == 1 ? theme::meterYellow : theme::meterGreen);
            g.fillRoundedRectangle (xOf (clickMs[i]) - 3.0f, y + 2.0f, 7.0f, 26.0f, 2.0f);
        }

        // The arrow back, split into what the device reports and what was measured.
        const float arrowY = axis.getY() + 2.0f * rowHeight - 6.0f;
        const double reportedPart = juce::jmax (0.0, reported);
        drawArrow (g, { xOf (roundTrip), arrowY }, { xOf (roundTrip - reportedPart) + 2.0f, arrowY }, theme::accent);
        drawLabel (g, juce::Rectangle<float> (xOf (roundTrip - reportedPart), arrowY - 18.0f, xOf (roundTrip) - xOf (roundTrip - reportedPart), 16.0f), "reported " + juce::String (reportedPart, 1) + " ms", 11.5f, theme::accent);

        if (measured)
        {
            const double from = roundTrip - reportedPart;
            drawArrow (g, { xOf (from), arrowY }, { xOf (0.0) + 2.0f, arrowY }, theme::meterYellow);
            drawLabel (g, juce::Rectangle<float> (xOf (0.0), arrowY - 18.0f, xOf (from) - xOf (0.0), 16.0f), "measured " + juce::String (std::abs (extra), 1) + " ms " + (extra < 0.0 ? "less" : "more"),
                       11.5f, theme::meterYellow);
        }

        drawLabel (g, juce::Rectangle<float> (area.getX(), area.getBottom() - 40.0f, area.getWidth(), 40.0f),
                   measured ? juce::String ("The two parts together are the round trip: what the device reports, and what Measure latency found beyond it.")
                            : juce::String ("The device reports this much. Measure latency in Audio / MIDI Settings finds anything it leaves out."),
                   12.5f, theme::textDim, juce::Justification::topLeft);
    }

    void paintTakes (juce::Graphics& g)
    {
        auto area = getArea();
        const auto takes = rg::getTakesExample();
        drawLabel (g, area.removeFromTop (22.0f), "Loop recording over bars " + juce::String (rg::exampleLoopStartBar) + " to " + juce::String (rg::exampleLoopEndBar - 1) + ": every pass is a take",
                   14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (6.0f);

        const float labelWidth = 70.0f;
        const auto lanes = area.withTrimmedLeft (labelWidth).withTrimmedBottom (48.0f);
        const float barWidth = lanes.getWidth() / (float) (rg::exampleLoopEndBar - rg::exampleLoopStartBar);
        auto xOf = [&] (double bar) { return lanes.getX() + (float) (bar - rg::exampleLoopStartBar) * barWidth; };

        // The loop and the ruler.
        g.setColour (theme::accent.withAlpha (0.12f));
        g.fillRect (lanes.withTrimmedTop (0.0f));

        for (int bar = rg::exampleLoopStartBar; bar < rg::exampleLoopEndBar; ++bar)
        {
            g.setColour (theme::gridBar);
            g.fillRect (xOf (bar), lanes.getY(), 1.0f, lanes.getHeight());
            drawLabel (g, juce::Rectangle<float> (xOf (bar) + 3.0f, lanes.getY(), 40.0f, 16.0f), juce::String (bar), 11.0f, theme::textDim, juce::Justification::centredLeft);
        }

        const float rowHeight = juce::jmin (46.0f, (lanes.getHeight() - 20.0f) / (float) juce::jmax (1, (int) takes.size()));
        float y = lanes.getY() + 20.0f;

        for (auto& take : takes)
        {
            drawLabel (g, juce::Rectangle<float> (area.getX(), y, labelWidth - 6.0f, rowHeight - 6.0f), "Take " + juce::String (take.pass), 12.5f, take.playing ? theme::accent : theme::text,
                       juce::Justification::centredLeft, true);
            const auto r = juce::Rectangle<float> (xOf (take.startBar), y, xOf (take.endBar) - xOf (take.startBar) - 2.0f, rowHeight - 6.0f);
            g.setColour (take.playing ? theme::accent : theme::accent.withAlpha (0.22f));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (take.playing ? theme::text : theme::textDim);
            g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
            const juce::String state = take.playing ? "plays" : take.cutShort ? "muted, stopped early" : take.startBar > rg::exampleLoopStartBar ? "muted, started mid-loop" : "muted";
            drawLabel (g, r.reduced (6.0f, 0.0f), state, 12.0f, take.playing ? theme::text : theme::textDim, juce::Justification::centredLeft);
            y += rowHeight;
        }

        drawLabel (g, juce::Rectangle<float> (area.getX(), area.getBottom() - 44.0f, area.getWidth(), 44.0f),
                   "The newest pass that reached the loop end plays. Right-click a take, then Takes, to hear another one; Delete other takes keeps only that one.",
                   12.5f, theme::textDim, juce::Justification::topLeft);
    }

    void paintSession (juce::Graphics& g)
    {
        auto area = getArea();
        const bool has = snapshot.sampleRate > 0.0;
        drawLabel (g, area.removeFromTop (22.0f), has ? "Your session" : "No audio device is running", 14.0f, theme::text, juce::Justification::centredLeft, true);
        area.removeFromTop (8.0f);

        // The chain with this device's numbers.
        const double bufferMs = rg::getBufferMs (getBufferSamples(), getRate());
        const juce::String names[] { "Input", "Buffer", "DawIt", "Output" };
        const juce::String details[] { snapshot.inputDevice.isEmpty() ? juce::String ("no input found") : snapshot.inputDevice + ", " + juce::String (snapshot.numInputs) + " in",
                                       juce::String (getBufferSamples()) + " samples, " + juce::String (bufferMs, 1) + " ms",
                                       "round trip " + juce::String (getRoundTrip(), 1) + " ms",
                                       snapshot.outputDevice.isEmpty() ? juce::String ("no output found") : snapshot.outputDevice };
        const float gap = 26.0f, boxWidth = (area.getWidth() - 3.0f * gap) / 4.0f, boxHeight = 78.0f;

        for (int i = 0; i < 4; ++i)
        {
            const auto box = juce::Rectangle<float> (area.getX() + (float) i * (boxWidth + gap), area.getY(), boxWidth, boxHeight);
            drawBox (g, box, names[i], details[i], i == 2 ? theme::accent : theme::textDim);

            if (i < 3)
                drawArrow (g, { box.getRight() + 3.0f, box.getCentreY() }, { box.getRight() + gap - 3.0f, box.getCentreY() }, theme::textDim);
        }

        area.removeFromTop (boxHeight + 14.0f);

        // Each armed track with its input level.
        if (snapshot.armed.empty())
        {
            drawLabel (g, area.removeFromTop (22.0f), "Nothing is armed. Arm an audio track with its R button to see its level here.", 12.5f, theme::textDim, juce::Justification::centredLeft);
        }
        else
        {
            for (size_t i = 0; i < snapshot.armed.size() && area.getHeight() > 60.0f; ++i)
            {
                auto row = area.removeFromTop (26.0f);
                auto& a = snapshot.armed[i];
                drawLabel (g, row.removeFromLeft (250.0f), a.name + " · " + rg::describeInput (a.inputChannel, a.stereo) + (a.monitor ? " · Monitor" : ""), 12.5f, theme::text, juce::Justification::centredLeft, true);
                recordingui::drawInputMeter (g, row.withTrimmedRight (90.0f).reduced (0.0f, 6.0f), meters[i], false);
                drawLabel (g, row.removeFromRight (86.0f), dbText (a.inputPeakDb), 12.0f, theme::text, juce::Justification::centredRight, true);
                area.removeFromTop (4.0f);
            }
        }

        area.removeFromTop (6.0f);
        drawLabel (g, area.removeFromTop (22.0f),
                   snapshot.hasLastTake ? "Last take: " + snapshot.lastTakeName + ", loudest peak " + dbText (snapshot.lastTakePeakDb) : juce::String ("No take recorded in this session yet."),
                   12.5f, theme::text, juce::Justification::centredLeft);
    }

    rg::Topic topic = rg::Topic::air;
    rg::Snapshot snapshot;
    std::vector<recordingui::InputMeter> meters;
};

//==============================================================================
RecordingExplainer::RecordingExplainer (Engine& e) : engine (e)
{
    title.setText ("How recording works", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    picture->setTooltip ("The picture of the topic. Where it shows your own device or input level, it follows them.");
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
    startTimerHz (15);
}

RecordingExplainer::~RecordingExplainer()
{
    list.setModel (nullptr);
}

juce::String RecordingExplainer::getPrintSource() const
{
    const auto session = recordingui::makeSnapshot (engine);
    return session.inputDevice.isNotEmpty() ? "your device, " + session.inputDevice : juce::String ("an example device");
}

int RecordingExplainer::getNumPrintTopics() const
{
    return (int) std::size (rg::allTopics);
}

booklet::Topic RecordingExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    return { rg::getTopicName (rg::allTopics[index]), rg::getTopicSummary (rg::allTopics[index]), explanation.getText(),
             { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image RecordingExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void RecordingExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void RecordingExplainer::showTopic (rg::Topic topic)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (rg::allTopics[i] == topic)
            list.selectRow (i);
}

int RecordingExplainer::getNumRows()
{
    return (int) std::size (rg::allTopics);
}

void RecordingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = rg::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (rg::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (rg::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void RecordingExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        show (row);
}

void RecordingExplainer::timerCallback()
{
    picture->setState (rg::allTopics[shownRow], recordingui::makeSnapshot (engine));

    // "Your session" follows the device and the last take.
    if (rg::allTopics[shownRow] == rg::Topic::yourSession)
        refreshText();
}

void RecordingExplainer::refreshText()
{
    const auto topic = rg::allTopics[shownRow];
    auto text = rg::explain (topic);

    if (topic == rg::Topic::yourSession)
    {
        const auto session = recordingui::makeSnapshot (engine);
        text << "\n\n" << rg::describeSession (session);
        const auto advice = rg::analyse (session);

        if (advice.empty())
            text << "\n\nNothing needs changing right now.";

        for (auto& a : advice)
            text << "\n\n" << a.text << " (" << rg::getTopicName (a.topic) << ")";
    }

    if (text != explanation.getText())
    {
        explanation.setText (text, false);

        if (topic != rg::Topic::yourSession)
            explanation.moveCaretToTop (false);
    }
}

void RecordingExplainer::show (int index)
{
    const bool changed = index != shownRow || explanation.getText().isEmpty();
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    picture->setState (rg::allTopics[shownRow], recordingui::makeSnapshot (engine));
    refreshText();

    if (changed)
        explanation.moveCaretToTop (false);
}

void RecordingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void RecordingExplainer::resized()
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
