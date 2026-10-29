#include "GateGuide.h"

namespace daw::gatetext
{
using gate::Knob;

namespace
{
    juce::String formatPercent (double percent)
    {
        return percent >= 10.0 ? juce::String (juce::roundToInt (percent)) + "%" : juce::String (percent, percent >= 1.0 ? 1 : 2) + "%";
    }

    juce::String formatMs (double ms)
    {
        return ms >= 1000.0 ? juce::String (ms / 1000.0, 2) + " s" : juce::String (juce::roundToInt (ms)) + " ms";
    }
}

juce::String formatLevel (double db)
{
    return juce::String (juce::roundToInt (db)) + " dB";
}

juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::threshold:  return "Threshold: when it opens";
        case (int) Knob::hysteresis: return "Hysteresis: open high, close low";
        case (int) Knob::range:      return "Range: how far it closes";
        case (int) Knob::ratio:      return "Ratio: gate or expander";
        case (int) Knob::attack:     return "Attack: how fast it opens";
        case (int) Knob::hold:       return "Hold: open a little longer";
        case (int) Knob::release:    return "Release: how fast it closes";
        case (int) Knob::keyLow:     return "Key low cut: ignore the lows";
        case (int) Knob::keyHigh:    return "Key high cut: ignore the highs";
        case curve:                  return "The curve";
        case key:                    return "The key filters";
        case match:                  return "Match level";
        case keyListen:              return "Key listen";
        case fxui::overview:         return "DawIt Gate";
        case fxui::presets:          return "Starting points";
        case fxui::clap:             return "Hits and hiss";
        case fxui::listen:           return "Hear what it removes";
        case fxui::guide:            return "What is a gate?";
        case fxui::history:          return "The last 4 seconds";
        default:                     return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::threshold:
            return "The level the sound must reach to open the gate. Louder passes; when it falls back under the threshold minus the "
                   "hysteresis, the gate closes. Drag the threshold dot on the curve sideways.";
        case (int) Knob::hysteresis:
            return "How far under the threshold the sound must fall before the gate closes. It stops the gate flickering open and shut "
                   "(chatter) when a fading note wobbles around the threshold.";
        case (int) Knob::range:
            return "How much the gate turns the sound down while it's closed. -80 dB is as good as silence, -20 dB leaves the gaps faint, "
                   "0 dB does nothing.";
        case (int) Knob::ratio:
            return "At its far end, Gate: closed means turned down by the whole range at once. Below that it's an expander: under the "
                   "threshold, every dB the sound falls becomes more in the output, so quiet sounds are turned down gently instead of cut.";
        case (int) Knob::attack:
            return "How fast the gate opens once the sound crosses the threshold: after the attack time it's two thirds open, after three "
                   "times as long 95%.";
        case (int) Knob::hold:
            return "How long the gate stays fully open after the sound falls under the closing level, before it starts to close. It keeps "
                   "the gate from closing between the waves of a low note, and lets a tail ring a moment longer.";
        case (int) Knob::release:
            return "How fast the gate closes after the hold. After the release time it's two thirds closed: the sound falls about 8.7 dB "
                   "every release time, so it fades instead of stopping.";
        case (int) Knob::keyLow:
            return "Takes the lows out of what the gate listens to (the key), 12 dB per octave, without changing the sound. Off at its lowest.";
        case (int) Knob::keyHigh:
            return "Takes the highs out of what the gate listens to, 12 dB per octave. Off at its highest. It makes the gate open a little "
                   "later, so set it no lower than needed.";
        case curve:
            return "Level in across, out up, from -80 to 0 dB. Above the threshold the line follows the diagonal: nothing changes. The lower "
                   "line is the gate closed (the range, or the ratio). The dashed lines are where it opens (the threshold) and where it "
                   "closes (minus the hysteresis): it jumps up at one and down at the other. The dot is the track as it plays. Drag the "
                   "threshold and closing dots sideways and the range dot (on the lower line) up or down; with no hysteresis the first two "
                   "sit together, and the left half picks the closing dot. Scroll for the ratio, double-click a dot to reset it, "
                   "right-click for more.";
        case key:
            return "What the gate hears of each frequency. Drag the left handle for the low cut and the right one for the high cut; "
                   "double-click one to turn it off.";
        case match:
            return "Keeps the output as loud as the input. A gate that only removes noise hardly changes the loudness, so match level adds "
                   "less than 1 dB; when it adds more, the gate is cutting into the notes.";
        case keyListen:
            return "Plays what the gate listens to, through the key filters, instead of the track. Set the filters so you hear the drum "
                   "you want with the others small. Not saved with the song.";
        case fxui::listen:
            return "Plays only what the gate takes away. You should hear the noise and bleed in the gaps; if you hear the starts or ends of "
                   "notes, the gate is cutting into them.";
        case fxui::clap:
            return "Four claps a quarter of a second apart over steady hiss at about -50 dB, through the gate with the window's settings, "
                   "even while the song is stopped. With the threshold at -40 dB the gate closes after each clap and the hiss fades: with a 100 ms "
                   "release it's about 15 dB down by the next clap, and with a 30 ms release it's gone well before it.";
        case fxui::history:
            return "The level coming in (grey), what comes out (colour), how much it turned down (red, from the top), the threshold and "
                   "closing level, and a bar along the bottom while the gate is open.";
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::threshold:
            return "Above the loudest noise, below the quietest note you want to keep. Start low, play a part with gaps and raise it until "
                   "the gaps are quiet, then check the soft notes still come through. Right-click the curve to set it from the track.";
        case (int) Knob::hysteresis:
            return "A few dB: 3-6 dB suits most sounds. More if the gate chatters at the ends of notes; 0-2 dB for gated reverb, where it "
                   "should close exactly.";
        case (int) Knob::range:
            return "The full range (-60 to -80 dB) for hiss between riffs and for gated reverb. 10-20 dB on vocals and on drums with room "
                   "sound, so the background is turned down rather than switched off.";
        case (int) Knob::ratio:
            return "Gate for drums, distorted guitars and effects. 2:1 to 4:1 for vocals, speech and anything with soft endings.";
        case (int) Knob::attack:
            return "0.01-0.1 ms for drums, so the first click gets through. 0.5-2 ms if a fast attack clicks on low notes. 1-10 ms for "
                   "voices, strings and pads.";
        case (int) Knob::hold:
            return "10-50 ms for most sounds, 50-150 ms for toms, 250-500 ms for gated reverb, where hold sets much of the length.";
        case (int) Knob::release:
            return "20-80 ms for tight kicks and snares, 200-500 ms for toms and ringing sounds, 100-300 ms for vocals, 10-30 ms to cut "
                   "gated reverb off.";
        case (int) Knob::keyLow:
            return "About 150 Hz on a snare, so the kick doesn't open it; 80-100 Hz on vocals, against rumble and plosives. Off on kick, "
                   "bass and toms.";
        case (int) Knob::keyHigh:
            return "About 150 Hz on a kick, so the snare doesn't open it; 2-5 kHz on snare and toms, against hi-hat and cymbals. Off on "
                   "most other sounds.";
        default:
            return {};
    }
}

juce::String describeSettings (int t, const gate::Settings& s, const std::vector<gate::Moment>& recent, double bpm)
{
    const double closing = (double) s.thresholdDb - (double) s.hysteresisDb;

    switch (t)
    {
        case (int) Knob::threshold:
        {
            float peak = -100.0f;

            for (auto& m : recent)
                peak = juce::jmax (peak, m.inputDb);

            auto lead = "Opens at " + formatLevel (s.thresholdDb) + ", closes at " + formatLevel (closing) + ".";

            if (peak > -90.0f)
                lead << " The track reached " + formatLevel (peak) + " in the last 4 seconds.";

            return lead;
        }

        case (int) Knob::hysteresis:
            return "Opens at " + formatLevel (s.thresholdDb) + ", closes at " + formatLevel (closing) + ".";

        case (int) Knob::range:
            return "Closed, it turns the sound down by " + juce::String (juce::roundToInt (s.rangeDb)) + " dB, to "
                   + formatPercent (100.0 * juce::Decibels::decibelsToGain (-(double) s.rangeDb)) + " of its level.";

        case (int) Knob::ratio:
        {
            if (gate::isGate (s.ratio))
                return "A sound under the threshold is turned down by the whole range, " + formatLevel (s.rangeDb) + ".";

            return "A sound 10 dB under the threshold is turned down by " + formatLevel (juce::jmin ((double) s.rangeDb, 10.0 * ((double) s.ratio - 1.0)))
                   + ".";
        }

        case (int) Knob::attack:
        {
            const double g = juce::Decibels::decibelsToGain (-(double) s.rangeDb), x = -20.0 * std::log10 (1.0 - (1.0 - g) * std::exp (-1.0 / (double) s.attackMs));
            return x < 0.1 ? juce::String ("A millisecond into a hit, the gate is fully open: the hit comes through whole.")
                           : "A millisecond into a hit, it still comes out " + juce::String (x, 1) + " dB quieter.";
        }

        case (int) Knob::hold:
        {
            auto lead = formatMs (s.holdMs) + ".";

            if (bpm > 0.0)
                lead << " At " + juce::String (juce::roundToInt (bpm)) + " BPM a sixteenth note is " + formatMs (15000.0 / bpm) + ".";

            return lead;
        }

        case (int) Knob::release:
        {
            const double range = s.rangeDb;

            if (range < 1.0)
                return "The range is " + formatLevel (range) + ": there is nothing to close.";

            if (range < 20.0)
                return "After the hold, it stops at the range, " + formatLevel (range) + " down.";

            auto lead = "After the hold, 20 dB down in " + formatMs (2.3026 * s.releaseMs);

            if (range >= 40.0)
                lead << " and 40 dB down in " + formatMs (4.6052 * s.releaseMs) + ".";
            else
                lead << ", then it stops at the range, " + formatLevel (range) + " down.";

            return lead;
        }

        case (int) Knob::keyLow:
            return gate::isKeyLowOn (s.keyLowHz) ? "A 60 Hz kick counts " + juce::String (-gate::getKeyResponseDb (s, 60.0), 1) + " dB quieter to the gate."
                                                 : juce::String ("Off: it hears all the lows.");

        case (int) Knob::keyHigh:
            return gate::isKeyHighOn (s.keyHighHz) ? "An 8 kHz hi-hat counts " + juce::String (-gate::getKeyResponseDb (s, 8000.0), 1) + " dB quieter to the gate."
                                                   : juce::String ("Off: it hears all the highs.");

        case fxui::history:
        {
            int open = 0, heard = 0, flickers = 0;

            for (auto& m : recent)
            {
                open += m.open ? 1 : 0;
                flickers += m.flickers;

                if (m.detectorDb > -90.0f)
                    ++heard;
            }

            if (heard < 100)
                return "Play the track, or click Hits and hiss, to see it here.";

            return "Open " + juce::String (juce::roundToInt (100.0 * open / (double) recent.size())) + "% of the time; " + juce::String (flickers)
                   + (flickers == 1 ? " flicker." : " flickers.");
        }

        default:
            return {};
    }
}

//==============================================================================
namespace
{
    constexpr float lowestDb = -80.0f;

    /** Where the range dot sits on the curve: on the closed line where it reaches the range. A gate: at the threshold. */
    juce::Point<float> getRangeDotLevels (const gate::Settings& s)
    {
        const float inDb = gate::isGate (s.ratio) ? s.thresholdDb : juce::jmax (lowestDb, s.thresholdDb - s.rangeDb / juce::jmax (0.05f, s.ratio - 1.0f));
        return { inDb, juce::jlimit (lowestDb, 0.0f, inDb - s.rangeDb) };
    }

    void drawNumber (juce::Graphics& g, const juce::String& text, juce::Point<float> centre, juce::Justification j, float width = 30.0f)
    {
        g.drawText (text, juce::Rectangle<float> (width, 12.0f).withCentre (centre), j);
    }
}

juce::Rectangle<float> getCurveSquare (juce::Rectangle<float> bounds, bool labels)
{
    auto area = labels ? bounds.withTrimmedLeft (30.0f).withTrimmedBottom (18.0f).withTrimmedTop (6.0f).withTrimmedRight (8.0f) : bounds.reduced (2.0f);
    const float side = juce::jmin (area.getWidth(), area.getHeight());
    return area.withSizeKeepingCentre (side, side);
}

CurveDots getCurveDots (juce::Rectangle<float> square, const gate::Settings& s)
{
    auto toX = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, square.getX(), square.getRight()); };
    auto toY = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, square.getBottom(), square.getY()); };
    const float closing = s.thresholdDb - s.hysteresisDb;
    const auto range = getRangeDotLevels (s);
    return { { toX (s.thresholdDb), toY (s.thresholdDb) }, { toX (closing), toY (closing) }, { toX (range.x), toY (range.y) } };
}

float curvePositionToDb (juce::Rectangle<float> square, float position, bool vertical)
{
    return vertical ? juce::jmap (position, square.getBottom(), square.getY(), lowestDb, 0.0f) : juce::jmap (position, square.getX(), square.getRight(), lowestDb, 0.0f);
}

juce::Rectangle<float> drawCurve (juce::Graphics& g, juce::Rectangle<float> bounds, const gate::Settings& s, bool labels,
                                  std::optional<std::pair<float, float>> dot, const std::vector<std::pair<float, float>>& trail)
{
    const auto square = getCurveSquare (bounds, labels);
    auto toX = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, square.getX(), square.getRight()); };
    auto toY = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, square.getBottom(), square.getY()); };
    const float closing = juce::jmax (lowestDb, s.thresholdDb - s.hysteresisDb);

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRect (square);
    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= lowestDb; db -= 20.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (square.getX(), toY (db), square.getWidth(), 1.0f);
        g.fillRect (toX (db), square.getY(), 1.0f, square.getHeight());

        if (labels)
        {
            g.setColour (theme::textDim);
            drawNumber (g, juce::String ((int) db), { square.getX() - 17.0f, toY (db) }, juce::Justification::centredRight);
            drawNumber (g, juce::String ((int) db), { toX (db), square.getBottom() + 9.0f }, juce::Justification::centred);
        }
    }

    // No change: the diagonal.
    const float dashes[] = { 5.0f, 4.0f };
    g.setColour (theme::gridBar);
    g.drawDashedLine ({ square.getX(), square.getBottom(), square.getRight(), square.getY() }, dashes, 2, 1.0f);

    // The closing level and the threshold.
    g.setColour (closingColour.withAlpha (0.8f));
    g.drawDashedLine ({ toX (closing), square.getY(), toX (closing), square.getBottom() }, dashes, 2, 1.0f);
    g.setColour (colour.withAlpha (0.9f));
    g.drawDashedLine ({ toX (s.thresholdDb), square.getY(), toX (s.thresholdDb), square.getBottom() }, dashes, 2, 1.0f);

    // Closed: the range, or the ratio. Open: nothing changes.
    juce::Path closed;

    for (float in = lowestDb; in <= s.thresholdDb + 0.01f; in += 0.5f)
    {
        const float out = in - (float) gate::getReductionDb (s, in, false);
        closed.isEmpty() ? closed.startNewSubPath (toX (in), toY (out)) : closed.lineTo (toX (in), toY (out));
    }

    g.setColour (removedColour);
    g.strokePath (closed, juce::PathStrokeType (2.2f));
    g.setColour (colour);
    g.drawLine (toX (closing), toY (closing), toX (0.0f), toY (0.0f), 3.0f);

    // The loop: up at the threshold, down at the closing level.
    const float closedAtThreshold = toY (s.thresholdDb - (float) gate::getReductionDb (s, s.thresholdDb, false));
    const float closedAtClosing = toY (closing - (float) gate::getReductionDb (s, closing, false));
    g.setColour (colour);

    if (closedAtThreshold - toY (s.thresholdDb) > 10.0f)
        g.drawArrow ({ toX (s.thresholdDb) + 4.0f, closedAtThreshold, toX (s.thresholdDb) + 4.0f, toY (s.thresholdDb) + 2.0f }, 1.4f, 7.0f, 7.0f);

    g.setColour (closingColour);

    if (closedAtClosing - toY (closing) > 10.0f)
        g.drawArrow ({ toX (closing) - 4.0f, toY (closing), toX (closing) - 4.0f, closedAtClosing - 2.0f }, 1.4f, 7.0f, 7.0f);

    if (labels)
    {
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (colour);
        g.drawText ("opens", juce::Rectangle<float> (60.0f, 12.0f).withPosition (toX (s.thresholdDb) + 4.0f, square.getY() + 2.0f), juce::Justification::centredLeft);
        g.setColour (closingColour);

        if (s.hysteresisDb >= 1.5f)
            g.drawText ("closes", juce::Rectangle<float> (60.0f, 12.0f).withPosition (toX (closing) - 64.0f, square.getY() + 2.0f), juce::Justification::centredRight);

        g.setColour (theme::textDim);
        g.drawText ("in", square.withTop (square.getBottom() - 12.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
        g.drawText ("out", square.withHeight (12.0f).withTrimmedLeft (4.0f), juce::Justification::centredLeft);
    }

    // The track.
    for (size_t i = 0; i < trail.size(); ++i)
    {
        if (trail[i].first < lowestDb)
            continue;

        g.setColour (theme::text.withAlpha (0.08f + 0.3f * (float) (i + 1) / (float) trail.size()));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ toX (trail[i].first), toY (trail[i].second) }));
    }

    if (dot && dot->first >= lowestDb)
    {
        const auto centre = juce::Point<float> (toX (dot->first), toY (dot->second));
        g.setColour (theme::text);
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (centre));
        g.setColour (theme::background);
        g.drawEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (centre), 1.5f);
    }

    if (labels)
    {
        const auto dots = getCurveDots (square, s);
        auto handle = [&] (juce::Point<float> p, juce::Colour c)
        {
            g.setColour (c);
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (p));
            g.setColour (theme::background);
            g.drawEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (p), 1.5f);
        };

        handle (dots.range, removedColour);
        handle (dots.closing, closingColour);
        handle (dots.threshold, colour);
    }

    return square;
}

//==============================================================================
void drawMoments (juce::Graphics& g, juce::Rectangle<float> bounds, const std::vector<gate::Moment>& moments, const gate::Settings& s, bool labels,
                  double bpm, const gate::Analysis* analysis)
{
    auto area = labels ? bounds.withTrimmedLeft (30.0f).withTrimmedRight (8.0f).reduced (0.0f, 3.0f) : bounds;
    auto toY = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, area.getBottom(), area.getY()); };

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= lowestDb; db -= 20.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);

        if (labels)
        {
            g.setColour (theme::textDim);
            drawNumber (g, juce::String ((int) db), { area.getX() - 17.0f, toY (db) }, juce::Justification::centredRight);
        }
    }

    // The beats, as far apart as the song's.
    if (bpm > 0.0)
    {
        const double beat = 60.0 / bpm;
        g.setColour (theme::gridBar.withAlpha (0.5f));

        for (double t = beat; t < gate::recentSeconds; t += beat)
            g.fillRect (area.getRight() - (float) (t / gate::recentSeconds) * area.getWidth(), area.getY(), 1.0f, area.getHeight());
    }

    const int total = gate::numMoments, count = (int) moments.size(), missing = total - count;
    const int columns = juce::jmax (1, (int) area.getWidth());
    juce::Path out;
    bool started = false;

    for (int c = 0; c < columns; ++c)
    {
        const int from = c * total / columns - missing, to = (c + 1) * total / columns - missing;

        if (to <= 0)
            continue;

        float in = -100.0f, outLevel = -100.0f, reduction = 0.0f;
        bool open = false;

        for (int i = juce::jmax (0, from); i < juce::jmin (count, to); ++i)
        {
            in = juce::jmax (in, moments[(size_t) i].inputDb);
            outLevel = juce::jmax (outLevel, moments[(size_t) i].outputDb);
            reduction = juce::jmax (reduction, moments[(size_t) i].reductionDb);
            open = open || moments[(size_t) i].open;
        }

        const float x = area.getX() + (float) c;
        g.setColour (theme::textDim.withAlpha (0.35f));
        g.fillRect (x, toY (in), 1.0f, area.getBottom() - toY (in));
        if (in > -90.0f)
        {
            g.setColour (removedColour.withAlpha (0.3f));
            g.fillRect (x, area.getY(), 1.0f, juce::jmin (area.getHeight(), area.getHeight() * reduction / 80.0f));
        }

        if (open)
        {
            g.setColour (colour);
            g.fillRect (x, area.getBottom() - 5.0f, 1.0f, 5.0f);
        }

        if (! started)
        {
            out.startNewSubPath (x, toY (outLevel));
            started = true;
        }
        else
        {
            out.lineTo (x, toY (outLevel));
        }
    }

    g.setColour (colour);
    g.strokePath (out, juce::PathStrokeType (1.6f));

    const float dashes[] = { 4.0f, 3.0f };
    g.setColour (colour.withAlpha (0.9f));
    g.drawDashedLine ({ area.getX(), toY (s.thresholdDb), area.getRight(), toY (s.thresholdDb) }, dashes, 2, 1.0f);
    g.setColour (closingColour.withAlpha (0.9f));
    g.drawDashedLine ({ area.getX(), toY (s.thresholdDb - s.hysteresisDb), area.getRight(), toY (s.thresholdDb - s.hysteresisDb) }, dashes, 2, 1.0f);

    if (analysis != nullptr && analysis->heard >= 500 && ! analysis->silentGaps)
    {
        g.setFont (juce::FontOptions (10.5f));

        for (auto [level, name] : { std::pair<float, juce::String> { analysis->floorDb, "noise floor " }, { analysis->loudDb, "loud parts " } })
        {
            g.setColour (theme::text.withAlpha (0.7f));
            g.fillRect (area.getX(), toY (level), area.getWidth(), 1.0f);
            g.drawText (name + formatLevel (level), area.withTop (toY (level) - 13.0f).withHeight (12.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
        }
    }
}

//==============================================================================
juce::Rectangle<float> getKeyPlot (juce::Rectangle<float> bounds, bool labels)
{
    return bounds.withTrimmedLeft (labels ? 30.0f : 4.0f).withTrimmedRight (8.0f).withTrimmedBottom (16.0f).withTrimmedTop (labels ? 22.0f : 4.0f);
}

double keyPositionToHz (juce::Rectangle<float> plot, float x)
{
    return 20.0 * std::pow (1000.0, juce::jlimit (0.0, 1.0, (double) ((x - plot.getX()) / plot.getWidth())));
}

float keyHzToPosition (juce::Rectangle<float> plot, double hz)
{
    return plot.getX() + plot.getWidth() * (float) (std::log (juce::jlimit (20.0, 20000.0, hz) / 20.0) / std::log (1000.0));
}

void drawKeyFilters (juce::Graphics& g, juce::Rectangle<float> bounds, const gate::Settings& s, bool labels, bool handles)
{
    const auto plot = getKeyPlot (bounds, labels);
    constexpr float topDb = 6.0f, bottomDb = -48.0f;
    auto toX = [&] (double hz) { return keyHzToPosition (plot, hz); };
    auto toY = [&] (double db) { return juce::jmap (juce::jlimit (bottomDb, topDb, (float) db), bottomDb, topDb, plot.getBottom(), plot.getY()); };

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRect (plot);
    g.setFont (juce::FontOptions (10.0f));

    // The ranges of the drums, so the picture says what each filter is for.
    struct Range { double from, to; const char* name; };

    for (const Range& range : { Range { 40.0, 120.0, "kick and bass" }, Range { 150.0, 300.0, "snare body" }, Range { 6000.0, 10000.0, "cymbals and hi-hat" } })
    {
        const auto band = juce::Rectangle<float> (toX (range.from), plot.getY(), toX (range.to) - toX (range.from), plot.getHeight());
        g.setColour (colour.withAlpha (0.12f));
        g.fillRect (band);

        if (labels)
        {
            g.setColour (theme::textDim);
            g.drawFittedText (range.name, juce::Rectangle<float> (band.getCentreX() - 38.0f, plot.getY() - 20.0f, 76.0f, 20.0f).toNearestInt(),
                              juce::Justification::centred, 2, 0.8f);
        }
    }

    for (float db = 0.0f; db >= bottomDb; db -= 12.0f)
    {
        g.setColour (db > -0.5f ? theme::gridBar : theme::grid);
        g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);

        if (labels)
        {
            g.setColour (theme::textDim);
            drawNumber (g, juce::String ((int) db), { plot.getX() - 17.0f, toY (db) }, juce::Justification::centredRight);
        }
    }

    for (const double hz : { 100.0, 1000.0, 10000.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (toX (hz), plot.getY(), 1.0f, plot.getHeight());
        g.setColour (theme::textDim);
        drawNumber (g, hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + " kHz" : juce::String ((int) hz) + " Hz", { toX (hz), plot.getBottom() + 9.0f },
                    juce::Justification::centred, 50.0f);
    }

    juce::Path response;
    const int steps = juce::jmax (40, (int) plot.getWidth());

    for (int i = 0; i <= steps; ++i)
    {
        const double hz = 20.0 * std::pow (1000.0, i / (double) steps);
        const float x = toX (hz), y = toY (gate::getKeyResponseDb (s, hz));
        i == 0 ? response.startNewSubPath (x, y) : response.lineTo (x, y);
    }

    g.setColour (colour);
    g.strokePath (response, juce::PathStrokeType (2.2f));

    if (handles)
    {
        for (const bool low : { true, false })
        {
            const double hz = low ? s.keyLowHz : s.keyHighHz;
            const bool on = low ? gate::isKeyLowOn (s.keyLowHz) : gate::isKeyHighOn (s.keyHighHz);
            const auto centre = juce::Point<float> (toX (hz), toY (on ? -3.0 : 0.0));
            g.setColour (on ? colour : theme::panelLight);
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (centre));
            g.setColour (on ? theme::background : colour);
            g.drawEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (centre), 1.5f);
        }
    }
}

//==============================================================================
namespace
{
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

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Colour c = theme::textDim,
                    juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (text, r, j, true);
    }

    juce::Rectangle<float> drawBox (juce::Graphics& g, juce::Rectangle<float> box, const juce::String& name, const juce::String& detail, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.18f));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (c);
        g.drawRoundedRectangle (box, 5.0f, 1.2f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (theme::text);
        g.drawText (name, box.withTrimmedBottom (detail.isNotEmpty() ? box.getHeight() / 2.0f - 2.0f : 0.0f), juce::Justification::centred);

        if (detail.isNotEmpty())
        {
            g.setFont (juce::FontOptions (10.5f));
            g.setColour (theme::textDim);
            g.drawFittedText (detail, box.withTrimmedTop (box.getHeight() / 2.0f - 2.0f).reduced (3.0f, 2.0f).toNearestInt(), juce::Justification::centredTop, 2);
        }

        return box;
    }

    void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow ({ from, to }, 1.2f, 7.0f, 7.0f);
    }

    /** A level against time: where the lines of dB and seconds fall inside some bounds. */
    struct TimePlot
    {
        TimePlot (juce::Rectangle<float> bounds, float low, float high, double length, bool timeLabels = true)
            : area (bounds.withTrimmedLeft (32.0f).withTrimmedBottom (timeLabels ? 14.0f : 2.0f).withTrimmedTop (2.0f)),
              lowDb (low), highDb (high), seconds (length), labelled (timeLabels) {}

        float x (double t) const { return area.getX() + area.getWidth() * (float) (t / seconds); }
        float y (float db) const { return juce::jmap (juce::jlimit (lowDb, highDb, db), lowDb, highDb, area.getBottom(), area.getY()); }

        void drawGrid (juce::Graphics& g, float dbStep, double timeStep) const
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillRect (area);
            g.setFont (juce::FontOptions (10.0f));

            for (float db = std::floor (highDb / dbStep) * dbStep; db >= lowDb; db -= dbStep)
            {
                g.setColour (theme::grid);
                g.fillRect (area.getX(), y (db), area.getWidth(), 1.0f);
                g.setColour (theme::textDim);
                g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (28.0f, 12.0f).withCentre ({ area.getX() - 16.0f, y (db) }),
                            juce::Justification::centredRight);
            }

            for (double t = 0.0; t <= seconds + 1.0e-9; t += timeStep)
            {
                g.setColour (theme::grid);
                g.fillRect (x (t), area.getY(), 1.0f, area.getHeight());

                if (labelled)
                {
                    g.setColour (theme::textDim);
                    g.drawText (fxui::formatSeconds (t), juce::Rectangle<float> (50.0f, 12.0f).withCentre ({ x (t), area.getBottom() + 8.0f }), juce::Justification::centred);
                }
            }
        }

        void hLine (juce::Graphics& g, float db, juce::Colour c) const
        {
            const float dashes[] = { 4.0f, 3.0f };
            g.setColour (c);
            g.drawDashedLine ({ area.getX(), y (db), area.getRight(), y (db) }, dashes, 2, 1.0f);
        }

        /** The values (one per sample at `rate`, from `first`) as a line, the highest of each column. */
        void series (juce::Graphics& g, const std::vector<float>& values, double rate, size_t first, juce::Colour c, float thickness, bool fill = false) const
        {
            const size_t n = juce::jmin (values.size() - juce::jmin (values.size(), first), (size_t) (seconds * rate));
            const int columns = juce::jmax (1, (int) area.getWidth());
            juce::Path path;
            bool started = false;

            for (int col = 0; col < columns && n > 0; ++col)
            {
                const size_t from = n * (size_t) col / (size_t) columns, to = juce::jmax (from + 1, n * (size_t) (col + 1) / (size_t) columns);

                if (from >= n)
                    break;

                float most = -1000.0f;

                for (size_t i = from; i < juce::jmin (n, to); ++i)
                    most = juce::jmax (most, values[first + i]);

                const float px = area.getX() + (float) col;

                if (fill)
                {
                    g.setColour (c);
                    g.fillRect (px, y (most), 1.0f, area.getBottom() - y (most));
                }
                else if (! started)
                {
                    path.startNewSubPath (px, y (most));
                    started = true;
                }
                else
                {
                    path.lineTo (px, y (most));
                }
            }

            if (! fill)
            {
                g.setColour (c);
                g.strokePath (path, juce::PathStrokeType (thickness));
            }
        }

        /** A bar along the bottom wherever the gate was open. */
        void openBar (juce::Graphics& g, const std::vector<char>& open, double rate) const
        {
            const int columns = juce::jmax (1, (int) area.getWidth());
            const size_t n = juce::jmin (open.size(), (size_t) (seconds * rate));
            g.setColour (colour);

            for (int col = 0; col < columns && n > 0; ++col)
            {
                const size_t from = n * (size_t) col / (size_t) columns, to = juce::jmax (from + 1, n * (size_t) (col + 1) / (size_t) columns);
                bool any = false;

                for (size_t i = from; i < juce::jmin (n, to); ++i)
                    any = any || open[i] != 0;

                if (any)
                    g.fillRect (area.getX() + (float) col, area.getBottom() - 5.0f, 1.0f, 5.0f);
            }
        }

        juce::Rectangle<float> area;
        float lowDb, highDb;
        double seconds;
        bool labelled;
    };

    constexpr double rate = gate::exampleRate;

    std::vector<float> plus (const std::vector<float>& level, const std::vector<float>& gain)
    {
        std::vector<float> out (level.size());

        for (size_t i = 0; i < out.size(); ++i)
            out[i] = level[i] + gain[i];

        return out;
    }

    /** What comes out of the gate for each level going in: the open diagonal, and under the threshold the closed line. */
    struct Transfer
    {
        gate::Settings settings;
        juce::Colour colour;
        float thickness;
    };

    void drawTransfers (juce::Graphics& g, juce::Rectangle<float> square, const std::vector<Transfer>& transfers)
    {
        auto toX = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, square.getX(), square.getRight()); };
        auto toY = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, square.getBottom(), square.getY()); };
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRect (square);
        g.setFont (juce::FontOptions (10.0f));

        for (float db = 0.0f; db >= lowestDb; db -= 20.0f)
        {
            g.setColour (theme::grid);
            g.fillRect (square.getX(), toY (db), square.getWidth(), 1.0f);
            g.fillRect (toX (db), square.getY(), 1.0f, square.getHeight());
            g.setColour (theme::textDim);
            g.drawText (juce::String ((int) db), juce::Rectangle<float> (28.0f, 12.0f).withCentre ({ square.getX() - 16.0f, toY (db) }), juce::Justification::centredRight);
            g.drawText (juce::String ((int) db), juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ toX (db), square.getBottom() + 9.0f }), juce::Justification::centred);
        }

        const float dashes[] = { 5.0f, 4.0f };
        g.setColour (theme::gridBar);
        g.drawDashedLine ({ square.getX(), square.getBottom(), square.getRight(), square.getY() }, dashes, 2, 1.0f);

        for (auto& t : transfers)
        {
            juce::Path path;

            for (float in = lowestDb; in <= 0.0f; in += 0.5f)
            {
                const float out = in - (float) gate::getReductionDb (t.settings, in, in >= t.settings.thresholdDb);
                path.isEmpty() ? path.startNewSubPath (toX (in), toY (out)) : path.lineTo (toX (in), toY (out));
            }

            g.setColour (t.colour);
            g.strokePath (path, juce::PathStrokeType (t.thickness));
        }
    }

    gate::Run runOn (const gate::Settings& s, const std::vector<float>& levels)
    {
        return gate::simulate (s, levels, rate);
    }

    gate::Preset getPreset (const juce::String& name)
    {
        for (auto& p : gate::getPresets())
            if (p.name == name)
                return p;

        return gate::getPresets().front();
    }

    /** The index at which the gate last closed, or -1. */
    int getLastClose (const gate::Run& run)
    {
        int last = -1;

        for (size_t i = 1; i < run.open.size(); ++i)
            if (run.open[i - 1] != 0 && run.open[i] == 0)
                last = (int) i;

        return last;
    }

    /** Explains a level with a line and a name at the right edge of a plot. */
    void tag (juce::Graphics& g, const TimePlot& plot, float db, const juce::String& text, juce::Colour c)
    {
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (c);
        g.drawText (text, plot.area.withTop (plot.y (db) - 13.0f).withHeight (12.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInGate& gateProcessor)
{
    BuiltInGate* processor = &gateProcessor;
    std::vector<fxui::Guide::Page> pages;

    pages.push_back ({ "Closing the gaps", "What a gate does",
        "A gate lets a sound through while it's loud and shuts while it's quiet, like a door that opens when someone knocks hard enough. "
        "Between the notes the door is closed, and whatever quietly goes on there is gone.\n\n"
        "What goes on between the notes is usually something you didn't want: the hiss of an amp, the hum of a guitar's pickups, a "
        "fridge in the next room, or the other drums leaking into a drum's microphone (called bleed or spill). Under the notes you "
        "don't hear it, because the notes are much louder. In the gaps you do. A gate removes it there.\n\n"
        "A gate can also shape a sound on purpose: it can make a drum shorter and tighter, or cut a big reverb off abruptly, the gated "
        "drum sound of the 1980s.\n\n"
        "What a gate can't do is take noise out from under the notes: while it's open, the hiss plays along. That needs a noise "
        "reduction tool, which DawIt doesn't have yet.\n\n"
        "The picture shows made-up drum hits over steady hiss at -55 dB, through the gate with the settings in its window: in grey what "
        "comes in, in colour what comes out, and along the bottom when the gate is open. The hits pass. Once a hit has died away the "
        "gate closes and the hiss fades out: with a 100 ms release it's about 24 dB down by the next hit, and with a 30 ms release "
        "it's gone well before it.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto hits = gate::makeDrumHits (3.0);
            const auto run = runOn (s, hits);
            drawCaption (g, r, "Drum hits over hiss at -55 dB, through the gate", "grey: in; colour: out; bar: open");
            TimePlot plot (r, -80.0f, 0.0f, 3.0);
            plot.drawGrid (g, 20.0f, 0.5);
            plot.series (g, hits, rate, 0, theme::textDim.withAlpha (0.35f), 1.0f, true);
            plot.hLine (g, s.thresholdDb, colour.withAlpha (0.8f));
            plot.series (g, plus (hits, run.gainDb), rate, 0, colour, 1.8f);
            plot.openBar (g, run.open, rate);
        } });

    pages.push_back ({ "Threshold: when it opens", "Above the noise, below the notes",
        "The threshold is the level the sound must reach to open the gate. Anything louder passes. When the sound falls back under it, "
        "the gate closes (a little lower, with hysteresis: see the next topic).\n\n"
        "Set it between two levels: above the loudest noise, so the noise alone never opens it, and below the quietest note you want to "
        "keep, so every note does. In the picture the hiss sits at -55 dB and the hits start at -6 dB: anything from about -50 to -20 dB "
        "works, and -40 dB leaves room on both sides.\n\n"
        "The threshold depends on how loud the track is, so set it by ear and by the picture, not by a number. A good way: start with it "
        "at the bottom, play a part with gaps, and raise it until the gaps are quiet. Then check that the softest notes still come "
        "through. The Your track topic suggests a threshold about 6 dB above your track's noise.\n\n"
        "When the noise and the quietest notes are close together, less than about 12 dB apart, no threshold separates them well. Then a "
        "gentle expander works better than a gate (see Gate or expander).\n\n"
        "The picture shows the hits with the window's threshold and closing level: the gate opens where a hit crosses the threshold.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto hits = gate::makeDrumHits (3.0);
            const auto run = runOn (s, hits);
            drawCaption (g, r, "Where the hits cross the threshold: " + formatLevel (s.thresholdDb), "grey: what comes in");
            TimePlot plot (r, -80.0f, 0.0f, 3.0);
            plot.drawGrid (g, 20.0f, 0.5);
            plot.series (g, hits, rate, 0, theme::textDim.withAlpha (0.4f), 1.0f, true);
            plot.hLine (g, s.thresholdDb, colour);
            plot.hLine (g, s.thresholdDb - s.hysteresisDb, closingColour);
            plot.openBar (g, run.open, rate);
            tag (g, plot, -55.0f, "the hiss, -55 dB", theme::text);
            tag (g, plot, -6.0f, "the hits start at -6 dB", theme::text);
            tag (g, plot, s.thresholdDb, "opens at " + formatLevel (s.thresholdDb), colour);

            if (s.hysteresisDb >= 1.5f)
                tag (g, plot, s.thresholdDb - s.hysteresisDb - 4.0f, "closes at " + formatLevel (s.thresholdDb - s.hysteresisDb), closingColour);
        } });

    pages.push_back ({ "Hysteresis: open high, close low", "Why a gate chatters, and the cure",
        "A note doesn't fade smoothly: its level wobbles a little as it dies away. If the gate opened and closed at exactly the same "
        "level, a note fading past the threshold would cross it several times, and the gate would flick open and shut. You hear it as a "
        "buzz or a stutter, called chatter.\n\n"
        "Hysteresis is the cure: the gate opens at the threshold but closes only once the sound has fallen a few dB under it. With a "
        "threshold of -40 dB and 4 dB of hysteresis, it opens at -40 dB and closes at -44 dB. A wobble smaller than 4 dB can't open it "
        "again.\n\n"
        "A thermostat works the same way: it might switch the heating on at 19 degrees and off at 21, instead of on and off around 20 "
        "all day.\n\n"
        "A few dB is usual; DawIt starts at 4 dB. More hysteresis also keeps the gate open longer as a note dies away, a little like a "
        "longer hold. Hold is the other cure for chatter (see Hold and release).\n\n"
        "The picture shows a note fading from -20 dB to -60 dB over two seconds with a slight wobble, through a threshold of -40 dB. "
        "Top: no hysteresis and no hold, and the gate closes, opens again and closes. Bottom: the window's hysteresis; with 4 dB or "
        "more, it closes once.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.thresholdDb = -40.0f;
            s.holdMs = 0.0f;
            auto bare = s;
            bare.hysteresisDb = 0.0f;
            const auto note = gate::makeFadingNote();
            const auto top = runOn (bare, note), bottom = runOn (s, note);
            drawCaption (g, r, "A fading note, with a threshold of -40 dB", "colour: the level; bar: open");
            const float half = r.getHeight() / 2.0f;
            const std::pair<const gate::Run*, juce::String> rows[] = { { &top, "No hysteresis, no hold" }, { &bottom, "Hysteresis " + formatLevel (s.hysteresisDb) } };

            for (int i = 0; i < 2; ++i)
            {
                auto cell = r.withHeight (half).withY (r.getY() + half * (float) i);
                auto head = cell.removeFromTop (14.0f);
                drawLabel (g, rows[i].second, head, theme::text);
                drawLabel (g, "closes: " + juce::String (rows[i].first->closes), head, rows[i].first->closes > 1 ? removedColour : colour, juce::Justification::centredRight);
                TimePlot plot (cell, -70.0f, -10.0f, 2.0, i == 1);
                plot.drawGrid (g, 20.0f, 0.5);
                plot.series (g, note, rate, 0, theme::text, 1.6f);
                plot.hLine (g, -40.0f, colour);

                if (i == 1)
                    plot.hLine (g, -40.0f - s.hysteresisDb, closingColour);

                plot.openBar (g, rows[i].first->open, rate);
            }
        } });

    pages.push_back ({ "Range: how far it closes", "Silence, or just quieter",
        "The range is how much the gate turns the sound down while it's closed. At -80 dB the gaps are as good as silent. At -20 dB "
        "they're at a tenth of their level, which sounds about a quarter as loud: faint, but there. At 0 dB the gate does nothing.\n\n"
        "A full close sounds clean on its own, but in a mix it can sound unnatural: the room and the bleed vanish between hits and come "
        "back with every note, so you hear the background switch on and off. A smaller range turns the gaps down far enough that they "
        "stop being a problem, while the sound still has air around it. Many engineers use 10-20 dB on vocals and on drums with a lot of "
        "room sound, and the full range for the hiss between riffs on a distorted guitar.\n\n"
        "The picture shows what comes out for each level that goes in, with ranges of 10, 20, 40 and 80 dB in grey and the window's in "
        "colour.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "Out for each level in, for different ranges", "grey: 10, 20, 40 and 80 dB; colour: " + formatLevel (-s.rangeDb));
            auto square = r.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f);
            square = square.withSizeKeepingCentre (juce::jmin (square.getWidth(), square.getHeight()), juce::jmin (square.getWidth(), square.getHeight()));
            std::vector<Transfer> transfers;

            for (const float range : { 10.0f, 20.0f, 40.0f, 80.0f })
            {
                auto t = s;
                t.rangeDb = range;
                transfers.push_back ({ t, theme::textDim.withAlpha (0.8f), 1.3f });
            }

            transfers.push_back ({ s, colour, 2.8f });
            drawTransfers (g, square, transfers);
        } });

    pages.push_back ({ "Gate or expander: the ratio", "Cut the gaps, or turn them down gently",
        "A gate is open or closed. An expander is gentler: under the threshold it turns the sound down more the quieter it gets, by a "
        "ratio.\n\n"
        "At 2:1, every dB the sound falls under the threshold becomes 2 dB in the output. A sound 10 dB under the threshold comes out 20 "
        "dB under it: turned down by 10 dB. At 4:1 the same sound is turned down by 30 dB. The range is still the most it turns down.\n\n"
        "It's the mirror image of a compressor, which turns down what's above its threshold. That's why it's also called a downward "
        "expander: it makes the difference between loud and quiet bigger, and pushes the quiet parts further down.\n\n"
        "Why use it: quiet sounds near the threshold, like a breath, a soft word or the end of a note, are turned down a little instead "
        "of cut off. So an expander sounds natural where a gate would chop. 2:1 to 4:1 is a common choice on vocals and speech. Above "
        "about 5:1 an expander already behaves much like a gate.\n\n"
        "In DawIt Gate the Ratio knob runs from 1:1 (nothing happens) to 10:1, and its far end is Gate: closed means turned down by the "
        "whole range at once.\n\n"
        "The picture shows what comes out for each level that goes in, from -80 to 0 dB, for 2:1, 4:1 and a gate, and the window's "
        "settings in colour. Above the threshold every line follows the diagonal: nothing changes there.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "Out for each level in, for different ratios", "grey: 2:1, 4:1 and Gate; colour: " + gate::formatKnob (Knob::ratio, s));
            auto square = r.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f);
            square = square.withSizeKeepingCentre (juce::jmin (square.getWidth(), square.getHeight()), juce::jmin (square.getWidth(), square.getHeight()));
            std::vector<Transfer> transfers;

            for (const float ratio : { 2.0f, 4.0f, 10.0f })
            {
                auto t = s;
                t.ratio = ratio;
                transfers.push_back ({ t, theme::textDim.withAlpha (0.8f), 1.3f });
            }

            transfers.push_back ({ s, colour, 2.8f });
            drawTransfers (g, square, transfers);
        } });

    pages.push_back ({ "Attack: how fast it opens", "Keeping the first click",
        "When the sound crosses the threshold the gate opens, but not all at once. The attack time is how fast: after the attack time "
        "it's about two thirds of the way open (63%), and after three times as long 95%.\n\n"
        "For drums the attack must be fast, because the most important part of a hit is its first few milliseconds: the click of a "
        "kick's beater, the crack of a snare. A gate with a slow attack is still opening while they pass. With a 5 ms attack the "
        "gate is less than a fifth open a millisecond into a hit: the hit comes out about 15 dB quieter there, and quieter still "
        "before it, so it loses its edge. The fastest gates open in a "
        "few hundredths of a millisecond for this reason.\n\n"
        "Faster isn't always better: a gate that jumps open in the middle of a wave can click, mostly on low notes. If you hear a click "
        "at the start of every note, try 0.5-2 ms. Sounds that start softly, like a voice, strings or a pad, are fine with 1-10 ms or "
        "more.\n\n"
        "Makers measure attack in different ways, so the same number can behave differently in another gate.\n\n"
        "The picture shows the start of a hit and how far the gate is open over the first 20 ms, with attacks of 0.05, 1 and 5 ms and "
        "the window's in colour.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto start = gate::makeHitStart();
            drawCaption (g, r, "How far open the gate is over the first 20 ms of a hit", "grey: 0.05, 1 and 5 ms; colour: " + gate::formatKnob (Knob::attack, s));
            auto plot = r.withTrimmedLeft (32.0f).withTrimmedBottom (14.0f);
            auto toX = [&] (double ms) { return plot.getX() + plot.getWidth() * (float) (ms / 20.0); };
            auto toY = [&] (double open) { return plot.getBottom() - plot.getHeight() * (float) open; };
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillRect (plot);
            g.setFont (juce::FontOptions (10.0f));

            for (const double open : { 0.0, 0.25, 0.5, 0.75, 1.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (plot.getX(), toY (open), plot.getWidth(), 1.0f);
                g.setColour (theme::textDim);
                g.drawText (juce::String (juce::roundToInt (open * 100.0)) + "%", juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ plot.getX() - 17.0f, toY (open) }),
                            juce::Justification::centredRight);
            }

            for (const double ms : { 0.0, 5.0, 10.0, 15.0, 20.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (toX (ms), plot.getY(), 1.0f, plot.getHeight());
                g.setColour (theme::textDim);
                g.drawText (juce::String (juce::roundToInt (ms)) + " ms", juce::Rectangle<float> (50.0f, 12.0f).withCentre ({ toX (ms), plot.getBottom() + 8.0f }), juce::Justification::centred);
            }

            g.setColour (theme::gridBar);
            const float dashes[] = { 4.0f, 3.0f };
            g.drawDashedLine ({ plot.getX(), toY (0.632), plot.getRight(), toY (0.632) }, dashes, 2, 1.0f);
            drawLabel (g, "63% after one attack time", plot.withTop (toY (0.632) - 14.0f).withHeight (12.0f).reduced (4.0f, 0.0f), theme::textDim, juce::Justification::centredRight);

            auto line = [&] (gate::Settings t, juce::Colour c, float thickness, const juce::String& name, float labelY)
            {
                const auto run = runOn (t, start);
                juce::Path path;
                const size_t first = (size_t) (0.01 * rate), count = (size_t) (0.02 * rate);

                for (size_t i = 0; i < count; ++i)
                {
                    const double ms = (double) i / rate * 1000.0, open = juce::Decibels::decibelsToGain ((double) run.gainDb[first + i]);
                    i == 0 ? path.startNewSubPath (toX (ms), toY (open)) : path.lineTo (toX (ms), toY (open));
                }

                g.setColour (c);
                g.strokePath (path, juce::PathStrokeType (thickness));
                drawLabel (g, name, juce::Rectangle<float> (120.0f, 12.0f).withPosition (plot.getRight () - 130.0f, labelY), c, juce::Justification::centredRight);
            };

            auto t = s;
            float labelY = plot.getBottom() - 90.0f;

            for (const float attack : { 0.05f, 1.0f, 5.0f })
            {
                t.attackMs = attack;
                line (t, theme::textDim.withAlpha (0.9f), 1.4f, juce::String (attack) + " ms", labelY);
                labelY += 14.0f;
            }

            line (s, colour, 2.8f, "the window's: " + gate::formatKnob (Knob::attack, s), labelY);
        } });

    pages.push_back ({ "Hold and release: how it closes", "The end of every note",
        "When the sound falls under the closing level, two times decide what happens.\n\n"
        "Hold keeps the gate fully open a little longer. Low notes need it: a bass note or a floor tom swings slowly, and between the "
        "peaks of one wave the level can dip under the threshold. Without hold, the gate could try to close inside a single note. Many "
        "gates without a hold knob have a fixed hold of about 20-30 ms for this reason. DawIt starts at 20 ms.\n\n"
        "Release is how fast the gate then closes. After the release time it has closed about two thirds of the way; with a large range, "
        "the sound falls about 8.7 dB every release time. With a 100 ms release the tail is 8.7 dB down after 100 ms, 26 dB after 300 ms "
        "and 43 dB after half a second: it fades rather than stops.\n\n"
        "Release shapes the end of every note. A short release (20-80 ms) makes drums short and tight. A longer one (200-500 ms) lets "
        "toms ring out. Too short, and notes are chopped off, with the reverb on them; too long, and the noise fades out in every gap "
        "instead of disappearing.\n\n"
        "Some engineers set hold and release in time with the song, so every hit lasts a note value: at 120 BPM a sixteenth note is 125 "
        "ms.\n\n"
        "Makers define these differently too: in some gates release is the time to close completely, and hold counts from the moment the "
        "gate opens.\n\n"
        "The picture shows one tom hit dying away over bleed at -50 dB, through a short release (30 ms), a long one (400 ms) and the "
        "window's settings in colour.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto tom = gate::makeTomHit();
            drawCaption (g, r, "A tom hit over bleed at -50 dB, closing", "grey: release 30 ms and 400 ms; colour: the window's");
            TimePlot plot (r, -100.0f, 0.0f, 1.5);
            plot.drawGrid (g, 20.0f, 0.5);
            plot.series (g, tom, rate, 0, theme::textDim.withAlpha (0.3f), 1.0f, true);
            auto t = s;

            for (const float release : { 30.0f, 400.0f })
            {
                t.releaseMs = release;
                plot.series (g, plus (tom, runOn (t, tom).gainDb), rate, 0, theme::textDim.withAlpha (0.9f), 1.4f);
            }

            const auto run = runOn (s, tom);
            plot.series (g, plus (tom, run.gainDb), rate, 0, colour, 2.4f);
            plot.hLine (g, s.thresholdDb - s.hysteresisDb, closingColour);

            if (const int close = getLastClose (run); close > 0 && s.holdMs > 0.0f)
            {
                const float from = plot.x ((double) close / rate - s.holdMs / 1000.0), to = plot.x ((double) close / rate);
                g.setColour (closingColour);
                g.fillRect (from, plot.area.getY() + 6.0f, juce::jmax (2.0f, to - from), 4.0f);
                drawLabel (g, "hold " + formatMs (s.holdMs), juce::Rectangle<float> (140.0f, 12.0f).withPosition (to + 6.0f, plot.area.getY() + 2.0f), closingColour);
            }
        } });

    pages.push_back ({ "The key filter: what it listens to", "Not letting the kick open the snare",
        "The gate decides when to open by listening to a copy of the sound, called the key or sidechain. That copy can be filtered "
        "without changing the sound itself.\n\n"
        "Why: a snare microphone also picks up the kick and the hi-hat. The kick is often loud enough in it to open the gate, so the "
        "snare's gate opens on every kick, bleed and all. The kick's weight is around 60 Hz and the snare's body around 200 Hz, so a "
        "low cut on the key at about 150 Hz makes the kick count far less: at 60 Hz it's 16 dB quieter to the gate. A high cut does "
        "the same for the hi-hat and cymbals, which are mostly above 6 kHz.\n\n"
        "It works the other way round too: a high cut on the key of a kick drum's gate, at about 150 Hz, keeps the snare from opening "
        "it.\n\n"
        "Key listen lets you hear what the gate listens to, so you can set the filters by ear: you should hear the drum you want, with "
        "the others small.\n\n"
        "Don't overdo it. The sharp start of a hit is mostly highs, so cutting the highs out of the key makes the gate open a little "
        "later.\n\n"
        "DawIt Gate listens to its own track. Opening it from another track, so a kick opens the gate on a bass, isn't possible yet.\n\n"
        "The picture shows what the gate hears of each frequency with the window's key filters, 12 dB per octave each.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "What the gate hears of each frequency", "low cut " + gate::formatKnob (Knob::keyLow, s) + ", high cut " + gate::formatKnob (Knob::keyHigh, s));
            drawKeyFilters (g, r, s, true, false);
        } });

    pages.push_back ({ "Where gates are used", "Drums, guitars, voices, and when not to",
        "- Drums with a microphone each: toms especially, because they play least and pick up the whole kit the rest of the time; kick "
        "and snare often too. The overhead and room microphones usually stay ungated, because they hold the kit together.\n"
        "- Distorted guitars: high gain turns up the hiss and hum between riffs. A gate before the distortion follows the playing; "
        "after it, it also catches the hiss the distortion adds. Many players use one in each place.\n"
        "- Vocals and speech: an expander (2:1 to 4:1, a range of 10-20 dB) turns down the room, headphone spill and noise between "
        "phrases. Leave the breaths audible: a vocal without any sounds edited.\n"
        "- Bass and keyboards with hum: a gentle expander between the notes.\n"
        "- Effects: gated reverb (see that topic).\n\n"
        "When not to:\n"
        "- Tracks from software instruments, such as DawIt's own synths, are already silent between the notes: there's nothing to "
        "remove, and a gate would only cut the notes' tails.\n"
        "- Sounds that fade slowly, like piano and pads: the gate cuts the fade. Use an expander, or edit the gaps.\n"
        "- Noise under the notes: a gate only helps in the gaps.\n\n"
        "In the chain a gate usually goes first: before compression, whose makeup turns the quiet parts up and so brings the noise "
        "closer to the notes, and before delay and reverb. The exception is gated reverb, where the gate comes after the reverb.\n\n"
        "The picture shows how four starting points close after a hit: Kick drum, Toms, Lead vocal and Gated reverb.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The gain after one hit, for four starting points", "the hit: -6 dB falling 40 dB/s, over bleed at -50 dB");
            const auto tom = gate::makeTomHit();
            const char* names[] = { "Kick drum", "Toms", "Lead vocal", "Gated reverb" };
            const float w = r.getWidth() / 2.0f, h = r.getHeight() / 2.0f;

            for (int i = 0; i < 4; ++i)
            {
                const auto preset = getPreset (names[i]);
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 2), r.getY() + h * (float) (i / 2), w, h).reduced (6.0f, 3.0f);
                auto head = cell.removeFromTop (14.0f);
                drawLabel (g, preset.name, head, theme::text);
                drawLabel (g, "hold " + gate::formatKnob (Knob::hold, preset.settings) + ", release " + gate::formatKnob (Knob::release, preset.settings),
                           head, theme::textDim, juce::Justification::centredRight);
                TimePlot plot (cell, -60.0f, 3.0f, 1.5, i >= 2);
                plot.drawGrid (g, 20.0f, 0.5);
                plot.series (g, runOn (preset.settings, tom).gainDb, rate, 0, colour, 2.0f);
            }
        } });

    pages.push_back ({ "Gated reverb", "The big drum sound of the 1980s",
        "Gated reverb is a big reverb or room sound that stops abruptly instead of fading: a snare that sounds huge for a moment and "
        "then is gone, leaving space before the next hit.\n\n"
        "It was found around 1979 by producer Steve Lillywhite and engineer Hugh Padgham, recording Phil Collins' drums for Peter "
        "Gabriel's third album (1980). It became the sound of Collins' own 'In the Air Tonight' and of much of the 1980s. One way "
        "studios made it: the room microphones went through a gate that the close microphones opened, held for about half a second and "
        "then closed fast.\n\n"
        "To make it in DawIt: put DawIt Reverb on the drum track with a large room or hall and plenty of reverb, and DawIt Gate after "
        "it with the Gated reverb starting point. Its threshold sits high, just under the hits, so only the hit and the loudest part "
        "of its reverb keep the gate open. Once the reverb falls under the closing level, the hold (350 ms) keeps it open a moment "
        "longer, and a very short release (20 ms) cuts it off. In the picture that's about 0.7 seconds after the hit.\n\n"
        "Because DawIt Gate listens to its own track, the length also depends on how loud the reverb is. For a shorter sound, raise "
        "the threshold or shorten the hold: at 120 BPM a beat is 500 ms, and for the tail to be gone before the next beat, the whole "
        "sound must fit in that.\n\n"
        "The picture shows a snare hit with a long reverb tail, and what's left after the gate.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto preset = getPreset ("Gated reverb");
            const auto snare = gate::makeReverbSnare();
            const auto run = runOn (preset.settings, snare);
            drawCaption (g, r, "A snare with a long reverb tail, through the Gated reverb starting point", "grey: in; colour: out");
            TimePlot plot (r, -80.0f, 0.0f, 1.5);
            plot.drawGrid (g, 20.0f, 0.5);
            plot.series (g, snare, rate, 0, theme::textDim.withAlpha (0.4f), 1.0f, true);
            plot.hLine (g, preset.settings.thresholdDb, colour.withAlpha (0.8f));
            plot.series (g, plus (snare, run.gainDb), rate, 0, colour, 2.4f);
            plot.openBar (g, run.open, rate);
            tag (g, plot, preset.settings.thresholdDb, "threshold " + formatLevel (preset.settings.thresholdDb), colour);

            if (const int close = getLastClose (run); close > 0)
                drawLabel (g, "cut off " + formatMs (1000.0 * ((double) close / rate - 0.1)) + " after the hit", juce::Rectangle<float> (200.0f, 12.0f).withPosition (plot.x ((double) close / rate) + 16.0f, plot.y (-40.0f)), colour);
        } });

    pages.push_back ({ "Too much", "Chopped, choppy, clicking",
        "Signs a gate is doing harm, and what to try:\n"
        "- Notes cut short: the ends of notes, cymbal tails or the reverb vanish. Lower the threshold, or lengthen the hold or the "
        "release.\n"
        "- Quiet notes missing: ghost notes on a snare, soft words. Lower the threshold, or use an expander so they're turned down "
        "rather than lost.\n"
        "- Chatter: a buzz as the gate flickers at the end of notes. More hysteresis or hold.\n"
        "- Clicks at the start of notes: the attack is too fast for the sound. Try 0.5-2 ms.\n"
        "- The background switching on and off: the room or the noise comes back with every note. A smaller range (10-20 dB) usually "
        "sounds more natural than silence.\n"
        "- Breaths gone from a vocal: it sounds edited. An expander, or a lower threshold.\n\n"
        "Match level helps you check. A gate that only removes noise hardly changes how loud a track is: noise 40 dB under the notes "
        "adds only 0.0004 dB. If match level has to add more than about 1 dB, the gate is taking away parts of the notes.\n\n"
        "The picture shows a tom hit through the window's settings, and with the threshold 15 dB higher: its ending is gone.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            auto high = s;
            high.thresholdDb = juce::jmin (0.0f, s.thresholdDb + 15.0f);
            const auto tom = gate::makeTomHit();
            drawCaption (g, r, "A tom hit through the window's settings, and with the threshold 15 dB higher", "grey: in");
            TimePlot plot (r, -100.0f, 0.0f, 1.5);
            plot.drawGrid (g, 20.0f, 0.5);
            plot.series (g, tom, rate, 0, theme::textDim.withAlpha (0.4f), 1.0f, true);
            plot.series (g, plus (tom, runOn (s, tom).gainDb), rate, 0, colour, 2.4f);
            plot.series (g, plus (tom, runOn (high, tom).gainDb), rate, 0, removedColour, 2.0f);
            plot.hLine (g, s.thresholdDb, colour.withAlpha (0.7f));
            plot.hLine (g, high.thresholdDb, removedColour.withAlpha (0.7f));
            tag (g, plot, s.thresholdDb, "threshold " + formatLevel (s.thresholdDb), colour);
            tag (g, plot, high.thresholdDb, "threshold " + formatLevel (high.thresholdDb) + ": the ending is gone", removedColour);
        } });

    pages.push_back ({ "How DawIt Gate works", "From key to gain",
        "For every sample:\n"
        "1. The key: a copy of the sound through the key filters (a low cut and a high cut, 12 dB per octave), with both channels "
        "together, so the stereo image doesn't shift.\n"
        "2. The detector follows the key's peaks. It rises at once and falls back by 8.7 dB every 10 ms, so the moments a wave passes "
        "through zero don't count as silence.\n"
        "3. Open or closed: the gate opens when the detector reaches the threshold. It closes when the detector has stayed under the "
        "threshold minus the hysteresis for longer than the hold.\n"
        "4. How far: open, nothing is turned down. Closed, the sound is turned down by the whole range (Gate), or by the ratio for how "
        "far it is under the threshold, at most the range.\n"
        "5. Smoothing: the gain moves there with the attack time while opening and the release time while closing; after one of them, "
        "it's two thirds of the way.\n"
        "6. The sound is turned by that gain, and match level makes it as loud as it came in.\n\n"
        "DawIt Gate has no lookahead: it reacts to what is playing now, so it adds no delay. A gate with lookahead listens a few "
        "milliseconds ahead and opens before a hit arrives, at the cost of delaying the sound by as much.\n\n"
        "Hear what it removes plays only what the gate takes away. You should hear the noise and bleed in the gaps; if you hear the "
        "starts or ends of notes, the gate is cutting into them.\n\n"
        "The picture shows these steps as a chain.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "From the key to the gain", {});
            constexpr int columns = 7;
            const float gap = 10.0f, col = (r.getWidth() - gap * (columns - 1)) / (float) columns, midY = r.getCentreY() - 10.0f;
            auto box = [&] (int column) { return juce::Rectangle<float> (r.getX() + (col + gap) * (float) column, midY - 30.0f, col, 60.0f); };

            const juce::Rectangle<float> boxes[] = {
                drawBox (g, box (0), "Key", "low cut, high cut", closingColour),
                drawBox (g, box (1), "Detector", "peaks, falls 8.7 dB / 10 ms", theme::accent),
                drawBox (g, box (2), "Open or closed", "threshold, hysteresis, hold", colour),
                drawBox (g, box (3), "How far", "range, ratio", colour),
                drawBox (g, box (4), "Smoothing", "attack, release", colour),
                drawBox (g, box (5), "Gain", "the sound times the gain", theme::accent),
                drawBox (g, box (6), "Match", "as loud as it came in", theme::accent)
            };

            for (int i = 0; i + 1 < columns; ++i)
                arrow (g, { boxes[i].getRight(), midY }, { boxes[i + 1].getX(), midY });

            juce::Path sound;
            const float top = midY - 70.0f;
            sound.startNewSubPath (boxes[0].getX() + 8.0f, boxes[0].getY());
            sound.lineTo (boxes[0].getX() + 8.0f, top);
            sound.lineTo (boxes[5].getCentreX(), top);
            g.setColour (theme::textDim);
            g.strokePath (sound, juce::PathStrokeType (1.2f));
            g.drawArrow ({ boxes[5].getCentreX(), top, boxes[5].getCentreX(), boxes[5].getY() }, 1.2f, 7.0f, 7.0f);
            drawLabel (g, "the sound itself, unfiltered, waits for the gain", juce::Rectangle<float> (boxes[5].getCentreX() - boxes[0].getX(), 14.0f).withPosition (boxes[0].getX() + 8.0f, top - 16.0f),
                       theme::textDim, juce::Justification::centred);
        } });

    pages.push_back ({ "Your track", "What the gate makes of your track",
        "This topic looks at the last 4 seconds of the track DawIt Gate is on, as the gate heard them, and says what to try.\n\n"
        "It measures two levels: the noise floor, the level the track stays under for its quietest tenth of the time it is audible (the gaps; "
        "digital silence, such as space between clips, isn't counted), and the loud parts, the level it reaches in its loudest twentieth. A threshold about 6 dB above the floor keeps the noise out and "
        "lets the notes through, as long as the notes are far enough above it.\n\n"
        "The picture shows the track's level in grey with the floor and the loud parts marked, the threshold and the closing level, "
        "when the gate was open, and what that suggests.\n\n"
        "Play the track, and the picture follows. Hits and hiss shows here too, but it lasts only a second, so it's a thin basis "
        "for advice: play the track for a few seconds for a better picture.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto moments = processor->getRecent();
            const auto analysis = gate::analyse (moments, s, processor->matchGainDb.load());
            drawCaption (g, r, "The last 4 seconds of the track, as the gate heard them", "grey: in; colour: out; red: turned down; bar: open");
            drawMoments (g, r.removeFromTop (juce::jmin (170.0f, r.getHeight() * 0.55f)), moments, s, true, processor->tempoBpm.load(), &analysis);
            r.removeFromTop (8.0f);
            g.setFont (juce::FontOptions (13.0f));
            g.setColour (theme::text);

            for (auto& line : analysis.advice)
            {
                const int lines = juce::jmax (1, (int) std::ceil (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), line) / (double) r.getWidth()) + 1);
                auto box = r.removeFromTop (16.0f * (float) lines + 4.0f);
                g.drawFittedText (line, box.toNearestInt(), juce::Justification::topLeft, lines + 1);
            }
        } });

    return pages;
}
} // namespace daw::gatetext
