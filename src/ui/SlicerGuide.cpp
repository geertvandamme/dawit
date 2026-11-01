#include "SlicerGuide.h"

namespace daw::slctext
{
using slc::Knob;
using slc::Mode;

namespace
{
    juce::String formatMs (double ms)
    {
        if (ms >= 1000.0)
            return juce::String (ms / 1000.0, 2) + " s";

        return fx::formatNumber (ms, ms < 10.0 ? 1 : ms < 100.0 && ms - std::floor (ms) > 0.05 ? 1 : 0) + " ms";
    }

    /** 4, 2.5, 0.75: without trailing zeros. */
    juce::String formatShort (double v)
    {
        auto text = juce::String (v, 2);

        if (text.contains ("."))
            text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

        return text;
    }

    double getBpm (double bpm) { return bpm > 0.0 ? bpm : 120.0; }

    bool isSlicing (Mode m) { return m == Mode::repeat || m == Mode::reverse; }

    const char* const stepNouns[] = { "thirty-second", "sixteenth triplet", "sixteenth", "eighth triplet", "eighth", "quarter" };

    juce::String getOrdinal (int n)
    {
        return juce::String (n) + (n % 100 >= 11 && n % 100 <= 13 ? "th" : n % 10 == 1 ? "st" : n % 10 == 2 ? "nd" : n % 10 == 3 ? "rd" : "th");
    }
}

juce::Colour getModeColour (Mode m)
{
    switch (m)
    {
        case Mode::through: return juce::Colour (0xff8a8f9a);
        case Mode::repeat:  return colour;
        case Mode::reverse: return juce::Colour (0xff5ec2ff);
        case Mode::mute:    return juce::Colour (0xff3a3d46);
    }

    return theme::textDim;
}

juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength: return "Step length";
        case (int) Knob::steps:      return "Steps";
        case (int) Knob::roll:       return "Roll";
        case (int) Knob::smooth:     return "Smooth";
        case (int) Knob::mix:        return "Mix";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:     return "Tempo";
        case pattern:                return "The pattern";
        case lastBars:               return "The last 2 bars";
        case modeButton + (int) Mode::through: return "Through";
        case modeButton + (int) Mode::repeat:  return "Repeat";
        case modeButton + (int) Mode::reverse: return "Reverse";
        case modeButton + (int) Mode::mute:    return "Mute";
        case fxui::overview:         return "DawIt Slicer";
        case fxui::presets:          return "Starting points";
        case fxui::clap:             return "Play a bar";
        case fxui::listen:           return "Hear only what it changes";
        case fxui::guide:            return "What is a slicer?";
        case fxui::history:          return "The last 2 bars";
        default:                     return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength:
            return "How long each step lasts, as a note value of the song's tempo: from 1/32 to 1/4, with triplets (1/16T, 1/8T). "
                   "At 120 BPM a 1/16 step is 125 ms.";
        case (int) Knob::steps:
            return "How many steps the pattern has before it starts again, counted from the start of the song. 16 steps of 1/16 make "
                   "one bar; other numbers drift across the bar line.";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:
            return "Song: the steps follow the song's tempo and position. Own: they follow the BPM set here, on the slicer's own clock "
                   "(also used as a plugin when the program gives no tempo).";
        case (int) Knob::roll:
            return "How many times a Repeat step plays its slice: 1 plays it once; 2, 3 or 4 play its first half, third or quarter that "
                   "many times, for rolls. Reverse steps ignore it.";
        case (int) Knob::smooth:
            return "How long each cut fades, in ms: the old sound fades out while the new one fades in, so the jump doesn't click. A fade is never longer than a quarter of its piece.";
        case (int) Knob::mix:
            return "How much of what comes out is the sliced sound. 100%: only the slicer; 50%: Mute steps are 6 dB dips and repeats sit "
                   "on top of the track. Through steps are the same at any mix.";
        case pattern:
            return "The steps over the song's beats. Click a step to give it the chosen mode, drag to paint several, double-click to make "
                   "it Through. The lit step is playing now.";
        case lastBars:
            return "Grey: the track coming in; colour: what comes out, against the steps. Repeats copy an earlier step's shape; gaps are "
                   "Mute steps.";
        case modeButton + (int) Mode::through: return "Plays the track as it is.";
        case modeButton + (int) Mode::repeat:  return "Plays the step before again, at full level.";
        case modeButton + (int) Mode::reverse: return "Plays the step before backwards.";
        case modeButton + (int) Mode::mute:    return "Silence for this step: a trance gate.";
        case fxui::clap:
            return "Plays one bar of drums over a held chord through the slicer, at the tempo it uses, from step 1. It works while the "
                   "song is stopped.";
        case fxui::listen:
            return "Silences the Through steps, so you hear only the repeats and reverses.";
        case fxui::guide:
            return "A guide to slicing, for beginners: cutting sound into steps on the beat, note lengths, repeats and stutters, repeat or "
                   "echo, reverse, trance gates, clicks and smoothing, how the slicer works, slicing in a mix, and your track. The "
                   "pictures follow the controls here.";
        case fxui::history:
            return describe (lastBars);
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength:
            return "1/16: stutters, gates and fills. 1/8: reverses and slower chops. 1/32: rolls and buzzes. Triplets: rolling, swung chops.";
        case (int) Knob::steps:
            return "16 for a bar of 1/16 or two bars of 1/8. 12 with 1/16T, 6 with 1/8T. 3, 5 or 7 for patterns that cross the bar.";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:
            return "Song, nearly always. Own for a part recorded at another tempo.";
        case (int) Knob::roll:
            return "1: plain repeats. 2 or 4: drum rolls. 3: triplet rolls.";
        case (int) Knob::smooth:
            return "1-5 ms: crisp. 10-30 ms: soft, breathing gates. Raise it if a bass clicks.";
        case (int) Knob::mix:
            return "100% for stutters and gates. 30-60% for a gentler pulse.";
        default:
            return {};
    }
}

juce::String describeSettings (int t, const slc::Settings& s, double hostBpm)
{
    const double bpm = s.ownTempo || hostBpm <= 0.0 ? slc::clampBpm (s.ownBpm) : slc::clampBpm (hostBpm);
    const double stepBeats = slc::getStepBeats (s.stepLength), stepMs = slc::getStepMs (s.stepLength, bpm);
    const juce::String length = slc::formatKnob (Knob::stepLength, s);

    switch (t)
    {
        case (int) Knob::stepLength:
            return length + " at " + formatShort (bpm) + " BPM: " + formatMs (stepMs) + " a step.";

        case (int) Knob::steps:
        {
            const double beats = s.steps * stepBeats;
            return juce::String (s.steps) + " steps of " + length + ": " + formatShort (beats) + (juce::approximatelyEqual (beats, 1.0) ? " beat" : " beats")
                   + (juce::approximatelyEqual (beats, 4.0) ? ", one bar." : beats > 4.0 && juce::approximatelyEqual (std::fmod (beats, 4.0), 0.0) ? ", " + formatShort (beats / 4.0) + " bars." : ".");
        }

        case (int) Knob::tempo:
        case (int) Knob::ownBpm:
            if (s.ownTempo)
                return "Own: the steps run at " + formatShort (s.ownBpm) + " BPM.";

            if (hostBpm <= 0.0)
                return "Song, but no tempo has reached the slicer yet: it uses its own " + formatShort (s.ownBpm) + " BPM until one does.";

            return "Song: the steps follow the song's " + formatShort (hostBpm) + " BPM.";

        case (int) Knob::roll:
            if (s.roll == 1)
                return "Roll 1: a Repeat step plays its whole slice, " + formatMs (stepMs) + ", once.";

            return "Roll " + juce::String (s.roll) + ": pieces of " + formatMs (stepMs / s.roll) + ", " + juce::String (juce::roundToInt (1000.0 * s.roll / stepMs))
                   + " a second.";

        case (int) Knob::smooth:
            // The slicer keeps every fade to a quarter of its piece (BuiltInSlicer::startFade).
            if (s.smoothMs > stepMs / 4.0)
                return formatShort (s.smoothMs) + " ms, kept to a quarter of each " + formatMs (stepMs) + " step: " + formatMs (stepMs / 4.0) + ".";

            return formatShort (s.smoothMs) + " ms of each " + formatMs (stepMs) + " step ("
                   + formatShort (100.0 * s.smoothMs / stepMs) + "%).";

        case (int) Knob::mix:
            if (s.mixPercent >= 99.5f)
                return "100%: only the slicer is heard. A Mute step is silent.";

            if (s.mixPercent < 0.5f)
                return "0%: the slicer is not heard at all.";

            return formatShort (s.mixPercent) + "%: a Mute step is " + juce::String (juce::Decibels::gainToDecibels (1.0f - s.mixPercent / 100.0f), 1) + " dB, not silent.";

        default:
            return {};
    }
}

//==============================================================================
int getSliceSource (const slc::Settings& s, int step)
{
    if (! juce::isPositiveAndBelow (step, s.steps) || ! isSlicing (s.modes[(size_t) step]))
        return -1;

    int i = step;

    for (int n = 0; n < s.steps; ++n)
    {
        i = (i + s.steps - 1) % s.steps;

        if (! isSlicing (s.modes[(size_t) i]))
            return i;
    }

    return -1;
}

int PatternGrid::getStepAt (juce::Point<float> p) const
{
    if (! cells.expanded (0.0f, 6.0f).contains (p))
        return -1;

    return juce::jlimit (0, slc::maxSteps - 1, (int) ((p.x - cells.getX()) / cellWidth));
}

PatternGrid layoutPattern (juce::Rectangle<float> bounds)
{
    PatternGrid grid;
    grid.cells = bounds.withTrimmedTop (30.0f).withTrimmedBottom (20.0f);
    grid.cellWidth = grid.cells.getWidth() / (float) slc::maxSteps;
    return grid;
}

void drawPattern (juce::Graphics& g, const PatternGrid& grid, const slc::Settings& s, int playingStep, int hovered, bool labels)
{
    const double sb = slc::getStepBeats (s.stepLength);
    const auto row = grid.cells;
    const auto bandTop = row.getY() - 28.0f, bandBottom = row.getBottom() + 18.0f;

    // The beat and bar lines: a line per beat, stronger at each bar.
    for (int beat = 0; beat * (1.0 / sb) <= slc::maxSteps + 1.0e-6; ++beat)
    {
        const float x = row.getX() + grid.cellWidth * (float) (beat / sb);

        if (x > row.getRight() + 1.0f)
            break;

        g.setColour (beat % 4 == 0 ? theme::gridBar : theme::grid);
        g.fillRect (x - 0.5f, bandTop, 1.0f, bandBottom - bandTop);
    }

    for (int i = 0; i < slc::maxSteps; ++i)
    {
        const auto cell = grid.getCell (i).reduced (1.5f, 0.0f);
        const bool used = i < s.steps;
        const auto mode = s.modes[(size_t) i];
        const auto c = getModeColour (mode);
        const float alpha = used ? 1.0f : 0.28f;

        g.setColour ((mode == Mode::through ? theme::panelLight : mode == Mode::mute ? theme::lane : c.withAlpha (0.8f)).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (cell, 4.0f);
        g.setColour ((mode == Mode::mute ? theme::gridBar : c.withAlpha (0.7f)).withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (cell, 4.0f, 1.0f);

        // The glyph of the mode: a loop, a backwards arrow, nothing for a gap, a thin line for the sound passing.
        const auto centre = cell.getCentre();
        const float r = juce::jmin (cell.getWidth() * 0.22f, 9.0f);
        g.setColour (juce::Colours::white.withMultipliedAlpha (used ? 0.92f : 0.3f));

        if (mode == Mode::repeat)
        {
            juce::Path loop;
            loop.addCentredArc (centre.x, centre.y, r, r, 0.0f, juce::MathConstants<float>::pi * 0.35f, juce::MathConstants<float>::pi * 1.9f, true);
            g.strokePath (loop, juce::PathStrokeType (2.0f));
            juce::Path head;
            head.addTriangle (centre.x + r * 0.5f - 4.0f, centre.y + r * 0.9f + 1.0f, centre.x + r * 0.5f + 5.0f, centre.y + r * 0.9f - 2.0f, centre.x + r * 0.5f + 2.0f, centre.y + r * 0.9f + 6.0f);
            g.fillPath (head);
        }
        else if (mode == Mode::reverse)
        {
            g.drawArrow ({ centre.x + r * 1.1f, centre.y, centre.x - r * 1.2f, centre.y }, 2.0f, 8.0f, 8.0f);
        }
        else if (mode == Mode::through)
        {
            g.setColour (theme::textDim.withMultipliedAlpha (alpha));
            g.fillRect (centre.x - r, centre.y - 0.5f, r * 2.0f, 1.0f);
        }

        if (labels)
        {
            g.setFont (juce::FontOptions (10.0f));
            g.setColour (theme::text.withMultipliedAlpha (used ? 0.8f : 0.3f));
            g.drawText (juce::String (i + 1), cell.withHeight (14.0f).reduced (3.0f, 0.0f), juce::Justification::centredLeft);

            // What beat the step falls on: bar.beat on the beat, "&" on the half.
            const double beats = i * sb, frac = beats - std::floor (beats);
            juce::String label;

            if (frac < 1.0e-6 || frac > 1.0 - 1.0e-6)
                label = juce::String ((int) (std::floor (beats / 4.0 + 1.0e-6)) + 1) + "." + juce::String ((int) std::floor (beats + 1.0e-6) % 4 + 1);
            else if (std::abs (frac - 0.5) < 1.0e-6)
                label = "&";

            g.setFont (juce::FontOptions (10.5f));
            g.setColour (theme::textDim.withMultipliedAlpha (used ? 1.0f : 0.4f));
            g.drawText (label, grid.getCell (i).withY (row.getBottom() + 2.0f).withHeight (14.0f), juce::Justification::centred);
        }

        // From the slice a step plays, an arc to where it plays.
        if (const int source = getSliceSource (s, i); source >= 0 && used)
        {
            const auto from = grid.getCell (source).getCentre().withY (row.getY() - 1.0f), to = centre.withY (row.getY() - 1.0f);
            const float height = juce::jmin (26.0f, 7.0f + 3.5f * (float) std::abs (i - source));
            juce::Path arc;
            arc.startNewSubPath (from);
            arc.quadraticTo ({ (from.x + to.x) * 0.5f, row.getY() - 1.0f - height * 1.6f }, to);
            g.setColour (c.withAlpha (0.65f));
            g.strokePath (arc, juce::PathStrokeType (1.4f));
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (to));
        }
    }

    for (const int step : { hovered, playingStep })
    {
        if (! juce::isPositiveAndBelow (step, slc::maxSteps))
            continue;

        const auto cell = grid.getCell (step).reduced (1.5f, 0.0f);
        g.setColour (step == playingStep ? juce::Colours::white : theme::text.withAlpha (0.7f));
        g.drawRoundedRectangle (cell.expanded (1.0f), 5.0f, step == playingStep ? 2.5f : 1.5f);
    }
}

juce::String getCellDescription (const slc::Settings& s, int step, double bpm, bool sliceMissing)
{
    if (! juce::isPositiveAndBelow (step, slc::maxSteps))
        return {};

    if (step >= s.steps)
        return "Step " + juce::String (step + 1) + " is past Steps (" + juce::String (s.steps) + "), so it isn't used. Raise Steps to use it.";

    const double sb = slc::getStepBeats (s.stepLength), beats = step * sb;
    juce::String where = (s.steps * sb > 4.0 + 1.0e-9 ? "bar " + juce::String ((int) std::floor (beats / 4.0 + 1.0e-6) + 1) + ", " : juce::String())
                         + "beat " + juce::String ((int) std::floor (beats + 1.0e-6) % 4 + 1);

    if (sb < 1.0)
        where << ", " << getOrdinal ((int) std::floor ((beats - std::floor (beats + 1.0e-6)) / sb + 1.0e-6) + 1) << " " << stepNouns[juce::jlimit (0, slc::numStepLengths - 1, s.stepLength)];

    const auto mode = s.modes[(size_t) step];
    const int source = getSliceSource (s, step);
    const auto ms = " (" + formatMs (slc::getStepMs (s.stepLength, getBpm (bpm))) + ")";
    juce::String what;

    switch (mode)
    {
        case Mode::through: what = "Through: plays the track as it is"; break;
        case Mode::mute:    what = "Mute: silent for this step"; break;
        case Mode::repeat:
            what = source < 0 ? "Repeat: nothing to repeat, every step repeats"
                              : s.roll == 1 ? "Repeat: plays step " + juce::String (source + 1) + " again" + ms
                                            : "Repeat: plays the first " + juce::String (s.roll == 2 ? "half" : s.roll == 3 ? "third" : "quarter") + " of step " + juce::String (source + 1)
                                                  + " " + juce::String (s.roll) + " times" + ms;
            break;
        case Mode::reverse:
            what = source < 0 ? "Reverse: nothing to reverse, every step repeats or reverses" : "Reverse: plays step " + juce::String (source + 1) + " backwards" + ms;
            break;
    }

    if (sliceMissing && source >= 0)
        what = juce::String (mode == Mode::repeat ? "Repeat" : "Reverse") + ": plays through, the slicer hasn't heard step " + juce::String (source + 1) + " yet";

    return "Step " + juce::String (step + 1) + " of " + juce::String (s.steps) + juce::String::fromUTF8 (" \xc2\xb7 ") + where + juce::String::fromUTF8 (" \xc2\xb7 ") + what;
}

//==============================================================================
std::vector<slc::StepRecord> getLastBars (BuiltInSlicer& slicer)
{
    const int n = juce::jlimit (1, 64, juce::roundToInt (8.0 / slc::getStepBeats (slicer.getSettings().stepLength)));
    std::vector<slc::StepRecord> records ((size_t) n);
    records.resize ((size_t) slicer.history.getRecent (records.data(), n));
    return records;
}

void drawLastBars (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<slc::StepRecord>& records, const slc::Settings& s)
{
    const double sb = slc::getStepBeats (s.stepLength);
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.fillRect (r);

    if (records.empty())
    {
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::textDim);
        g.drawText ("Play the track, or click Play a bar, to see it here.", r, juce::Justification::centred);
        return;
    }

    const double end = records.back().beat + sb, start = end - 8.0;
    auto toX = [&] (double beat) { return r.getX() + r.getWidth() * (float) ((beat - start) / 8.0); };
    const auto lane = r.withTrimmedBottom (14.0f);
    auto toY = [&] (float peak) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, juce::Decibels::gainToDecibels (peak, -60.0f)), -60.0f, 0.0f, lane.getBottom(), lane.getY() + 2.0f); };

    for (int beat = (int) std::ceil (start - 1.0e-6); beat <= (int) std::floor (end + 1.0e-6); ++beat)
    {
        g.setColour (beat % 4 == 0 ? theme::gridBar : theme::grid);
        g.fillRect (toX (beat) - 0.5f, r.getY(), 1.0f, r.getHeight());
    }

    for (auto& rec : records)
    {
        if (rec.beat < start - 1.0e-6)
            continue;

        const float x0 = toX (rec.beat), x1 = toX (rec.beat + sb);
        const auto cell = juce::Rectangle<float> (x0 + 1.0f, lane.getY(), juce::jmax (1.0f, x1 - x0 - 2.0f), lane.getHeight());

        g.setColour (juce::Colour (0xff8a8f9a).withAlpha (0.45f));
        g.fillRect (cell.withTop (toY (rec.inPeak)));
        g.setColour (getModeColour (rec.mode == Mode::mute ? Mode::through : rec.mode).withAlpha (0.9f));
        g.fillRect (cell.reduced (cell.getWidth() * 0.2f, 0.0f).withTop (toY (rec.outPeak)));
        g.setColour (getModeColour (rec.mode).withAlpha (rec.mode == Mode::through ? 0.5f : 1.0f));
        g.fillRect (cell.withTop (lane.getBottom() + 3.0f).withHeight (6.0f));
    }

    g.setFont (juce::FontOptions (10.0f));
    g.setColour (theme::textDim);
    g.drawText ("grey: in, colour: out; the strip under each step is its mode", r.withTrimmedLeft (4.0f).withHeight (13.0f), juce::Justification::centredLeft);
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
            g.drawFittedText (detail, box.withTrimmedTop (box.getHeight() / 2.0f - 2.0f).reduced (4.0f, 2.0f).toNearestInt(), juce::Justification::centredTop, 2);
        }

        return box;
    }

    void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow ({ from, to }, 1.2f, 7.0f, 7.0f);
    }

    void drawNote (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text)
    {
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::textDim);
        g.drawFittedText (text, r.toNearestInt(), juce::Justification::centredLeft, 2);
    }

    /** The level of the test bar (kick on 1 and 3, snare on 2 and 4, hats on every eighth, over a quiet chord) at a position in
        quarter notes, repeating every bar: a picture of its loudness, 0 to 1. */
    float getTestLevel (double beats, double bpm)
    {
        const double beat = std::fmod (std::fmod (beats, 4.0) + 4.0, 4.0), seconds = 60.0 / bpm;
        double v = 0.07;

        auto hit = [&] (double at, double amplitude, double decay)
        {
            v += amplitude * std::exp (-std::fmod (beat - at + 4.0, 4.0) * seconds / decay);
        };

        hit (0.0, 0.65, 0.12);
        hit (2.0, 0.65, 0.12);
        hit (1.0, 0.5, 0.08);
        hit (3.0, 0.5, 0.08);

        for (int i = 0; i < 8; ++i)
            hit (i * 0.5, 0.15, 0.02);

        return (float) juce::jmin (1.0, v);
    }

    /** What plays at a position in the pattern, as a position in the track: through, a slice forwards or backwards, or silence (nullopt). */
    std::optional<double> getSourceBeat (const slc::Settings& s, int absStep, double phase)
    {
        const double sb = slc::getStepBeats (s.stepLength);
        const int index = ((absStep % s.steps) + s.steps) % s.steps;
        const auto mode = s.modes[(size_t) index];
        const int source = getSliceSource (s, index);

        if (mode == Mode::mute)
            return std::nullopt;

        if (source < 0 || ! isSlicing (mode))
            return (absStep + phase) * sb;

        const int back = ((index - source) + s.steps) % s.steps;
        const double sliceStart = (absStep - back) * sb;

        if (mode == Mode::reverse)
            return sliceStart + (1.0 - phase) * sb * 0.995;

        return sliceStart + std::fmod (phase * s.roll, 1.0) / s.roll * sb;
    }

    /** Two rows of steps: what comes in above, and what comes out below, with an arc from each slice to where it plays. */
    void drawStepRows (juce::Graphics& g, juce::Rectangle<float> r, const slc::Settings& s, double bpm, int numSteps, bool numbers)
    {
        const double sb = slc::getStepBeats (s.stepLength);
        const float laneHeight = (r.getHeight() - 44.0f) / 2.0f;
        const auto top = r.withHeight (laneHeight), bottom = r.withTrimmedTop (r.getHeight() - laneHeight);
        auto toX = [&] (double u) { return r.getX() + r.getWidth() * (float) (u / numSteps); };

        for (auto lane : { top, bottom })
        {
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (lane);
        }

        // The level, per pixel, as a filled shape.
        for (int pass = 0; pass < 2; ++pass)
        {
            const auto lane = pass == 0 ? top : bottom;
            const int width = juce::jmax (2, (int) lane.getWidth());

            for (int x = 0; x < width; ++x)
            {
                const double u = (x + 0.5) / width * numSteps;
                const int absStep = (int) std::floor (u);
                const double phase = u - absStep;
                const auto mode = s.modes[(size_t) (absStep % s.steps)];
                std::optional<double> beat = pass == 0 ? std::optional<double> (u * sb) : getSourceBeat (s, absStep, phase);

                if (! beat.has_value())
                    continue;

                const float level = getTestLevel (*beat, bpm);
                g.setColour (pass == 0 ? juce::Colour (0xff8a8f9a).withAlpha (0.7f) : getModeColour (mode).withAlpha (0.9f));
                g.fillRect (lane.getX() + (float) x, lane.getBottom() - level * (lane.getHeight() - 4.0f), 1.0f, level * (lane.getHeight() - 4.0f));
            }
        }

        for (int i = 0; i <= numSteps; ++i)
        {
            g.setColour (theme::gridBar);
            g.fillRect (toX (i) - 0.5f, r.getY(), 1.0f, r.getHeight());

            if (numbers && i < numSteps)
            {
                g.setFont (juce::FontOptions (10.0f));
                g.setColour (theme::textDim);
                g.drawText (juce::String ((i % s.steps) + 1), juce::Rectangle<float> (toX (i), top.getY() + 1.0f, toX (1) - toX (0), 12.0f), juce::Justification::centred);
            }
        }

        // The arcs from a slice in the track above to where it is played below.
        for (int i = 0; i < numSteps; ++i)
        {
            const int index = i % s.steps, source = getSliceSource (s, index);

            if (source < 0)
                continue;

            const int back = ((index - source) + s.steps) % s.steps;

            if (i - back < 0)
                continue;

            const juce::Point<float> from { toX (i - back + 0.5), top.getBottom() }, to { toX (i + 0.5), bottom.getY() };
            juce::Path arc;
            arc.startNewSubPath (from);
            arc.cubicTo ({ from.x, (from.y + to.y) * 0.5f }, { to.x, (from.y + to.y) * 0.5f }, to);
            g.setColour (getModeColour (s.modes[(size_t) index]).withAlpha (0.8f));
            g.strokePath (arc, juce::PathStrokeType (1.4f));
            g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (to));
        }

        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::textDim);
        g.drawText ("comes in", top.reduced (4.0f, 14.0f), juce::Justification::topRight);
        g.drawText ("comes out", bottom.reduced (4.0f, 2.0f), juce::Justification::topRight);
    }

    /** The settings the picture uses: yours when they have a step of this mode, otherwise a small example, said in the caption. */
    slc::Settings getExample (const slc::Settings& yours, std::initializer_list<Mode> wanted, std::initializer_list<Mode> demo, bool& isDemo)
    {
        for (int i = 0; i < yours.steps; ++i)
            for (auto m : wanted)
                if (yours.modes[(size_t) i] == m)
                {
                    isDemo = false;
                    return yours;
                }

        isDemo = true;
        slc::Settings s = yours;
        s.steps = 8;
        s.modes.fill (Mode::through);
        int i = 0;

        for (auto m : demo)
            s.modes[(size_t) i++] = m;

        return s;
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInSlicer& slicer)
{
    BuiltInSlicer* processor = &slicer;
    std::vector<fxui::Guide::Page> pages;
    auto bpmOf = [processor] { return slc::clampBpm (processor->getUsedBpm()); };

    struct PageText { const char* name; const char* summary; const char* text; };
    static const PageText texts[] = {
        { "Slices of time", "Cutting sound on the beat grid", R"slc(A slicer cuts a track into short pieces of time, called steps, that line up with the song's beats, and does something with each one: plays it as it is, plays the step before it again, plays it backwards, or leaves a gap. Out of one steady part it makes stutters, swells and choppy rhythms that stay in time with the song.

The steps follow the beat. At 120 BPM a beat lasts 500 ms, so with a step length of 1/16 each step is 125 ms and 16 of them make one bar of 4/4. Change the tempo and the steps change with it: at 90 BPM a sixteenth lasts 167 ms.

A slicer works on the sound itself, from a recording, a loop or an instrument. It doesn't know where the notes are: it cuts where the grid says, even in the middle of a note. That is part of its sound, and the reason it matters where a part's notes fall against the grid.

Producers use it for the stutter in the bar before a chorus, the choppy pads of trance, a hit played backwards into the next bar, and drum rolls. The next topics take these one by one.

The picture shows one bar of the test beat cut into DawIt Slicer's steps, with how long each lasts at the tempo.)slc" },
        { "Note lengths and steps", "How long a step lasts", R"slc(The step length is a note value, as in the piano roll. A quarter note (1/4) is one beat, an eighth (1/8) half a beat, a sixteenth (1/16) a quarter of a beat and a thirty-second (1/32) an eighth of a beat. Triplets fit three in the time of two: a sixteenth triplet (1/16T) is a sixth of a beat, an eighth triplet (1/8T) a third.

A beat lasts 60,000 / BPM milliseconds. At 120 BPM:
- 1/4: 500 ms
- 1/8: 250 ms
- 1/8T: 167 ms
- 1/16: 125 ms
- 1/16T: 83 ms
- 1/32: 62.5 ms

Steps sets how many steps the pattern has before it starts again, from 1 to 16. 16 steps of 1/16 make one bar; 16 of 1/8 make two. With triplets, 12 steps of 1/16T or 6 of 1/8T fill two beats, so the pattern meets the bar line again. Other numbers drift across the bar and come back later: a pattern of 3 sixteenths only starts on the bar line again every 3 bars.

The pattern is counted from the start of the song, so step 1 is always on the first beat of bar 1, wherever you start playing.

Shorter steps cut more finely: 1/32 steps sound like a buzz or a drum roll. 1/4 steps repeat whole beats, which sounds less like an effect and more like rearranging the part.

The picture shows each step length against one bar at the tempo the slicer is using.)slc" },
        { "Repeat: a stutter", "Playing the step before again", R"slc(A Repeat step plays the step before it again, instead of what comes in now. Set step 2 to Repeat and you hear step 1 twice; what came in during step 2 is left out.

Several Repeat steps in a row all play the same slice: the one just before the row. Through, Repeat, Repeat, Repeat plays step 1 four times. When the row ends, the next Through step plays the track where the song is by then, so the part stays in time: the repeats replace a stretch of it, they don't push it later.

Roll plays a shorter piece more often within each Repeat step. With Roll 2 a step plays the first half of the slice twice; with 3, its first third three times; with 4, its first quarter four times. On 1/16 steps at 120 BPM, Roll 4 repeats pieces of 31 ms, 32 times a second. Roll 3 gives triplets inside the step.

Very short repeats stop sounding like repeats. Above about 20 a second the ear hears them as a buzz with a low pitch, for the same reason a vibration 20 or more times a second is heard as a note. Glitch music and build-ups use this on purpose.

A Repeat step needs something to repeat. Right after you press play, jump or loop back, the slicer hasn't heard the step before yet, so that step plays the track through as it is.

The picture shows the steps that came in above and what comes out below, with an arc from each repeated slice to where it plays again.)slc" },
        { "Repeat or echo?", "Why a stutter isn't a delay", R"slc(A repeat and an echo both play again something you've just heard, but they work differently and sound different.

An echo, from a delay, comes on top of the sound. The sound carries on, and a copy follows a set time later, usually quieter and fading with every repeat. Nothing is taken away: you hear more.

A repeat, from a slicer, replaces the sound. During a Repeat step you hear the step before at its full level, and what came in during that step is left out (at a Mix of 100%). Nothing fades: the fourth repeat is as loud as the first. When the repeats end, the track comes back where the song is.

So a delay fills the gaps and makes a part bigger; a slicer rearranges it. A delay on a vocal leaves the words whole with echoes after them; a slicer makes the same vocal stammer: "to-to-to-tonight".

Some effects can do both: Ableton Live's Beat Repeat can add its repeats to the sound or play them instead of it. DawIt Slicer always plays them instead, and its Mix knob blends the result with the untouched track.

The picture shows one hit through each: DawIt Delay at 1/16 with 50% feedback, where each echo is 6 dB quieter than the one before, and DawIt Slicer with three Repeat steps, where every repeat is as loud as the hit.)slc" },
        { "Reverse", "Backwards slices", R"slc(A Reverse step plays the step before it backwards. A drum hit, which starts loud and dies away, becomes a sound that swells up from nothing and stops dead: the reverse cymbal or reverse snare that pulls into a new section. A sung word becomes the strange, sucking sound of tape played backwards.

The slicer can't play backwards what hasn't happened yet, so it always reverses the step that has just passed. A row of Reverse steps plays that same slice backwards each time.

Backwards sound began with tape: thread a reel the other way round and the music plays backwards. The Beatles made it famous in 1966, with the backwards guitar solo of I'm Only Sleeping and the backwards vocals at the end of Rain.

A reversed hit ends with what was its start, so its loudest moment comes at the very end of the step, just before the next one. That pull towards the next beat is what makes it useful: a Reverse step on the last step of a bar swells into the downbeat. It works best when the step before starts with a hit; a step that is quiet at its start gives a swell that hardly arrives.

The picture shows a hit as it came in and the same slice played backwards in the next step.)slc" },
        { "Mute: a trance gate", "Rhythm from silence", R"slc(A Mute step lets nothing through: a short gap. A pattern of Through and Mute steps chops a long, steady sound into rhythm. On a held pad chord, Through on every other 1/16 step turns one long chord into a pulse of sixteenth notes.

This is called a trance gate, after the trance music of the 1990s, where it was everywhere on pads and held synth chords, usually at 136 to 140 BPM. It works best on steady sounds: a pad, a held chord, a long note, noise. On drums, a Mute step only takes hits away, which you can do more simply by deleting notes.

DawIt Gate is a different effect with a similar name: it opens and closes by itself, following how loud the track is, to keep noise out between notes. A trance gate ignores the level and follows a pattern on the beat.

Smooth sets how quickly each step fades in and out. 1 to 5 ms gives crisp cuts without clicks; 10 to 30 ms gives a softer pulse that breathes. On short steps a long fade fills the gaps back in: on 1/16 steps at 140 BPM (107 ms each), 30 ms would be more than a quarter of every gap, so DawIt Slicer keeps each fade to a quarter of its step, 27 ms.

A Mix below 100% turns the gaps into dips: at 50% a Mute step is half as loud, 6 dB down, instead of silent.

The picture shows a steady chord before and after the pattern, with the fades drawn at Smooth's length.)slc" },
        { "Clicks and smoothing", "Why a cut needs a fade", R"slc(Sound is a wave that swings smoothly up and down. Cut it at a random moment and jump to another piece of sound, and the wave jumps too, from one level to another between two samples. The speaker follows that jump, and you hear a click. A slicer cuts at every step, so without care it would click up to 16 times a bar.

Audio editors avoid it in two ways: by cutting where the wave crosses zero, or with a short fade. Sound On Sound suggests fades of about 1 to 3 ms at the edges of a clip, or crossfades of about 2 to 4 ms where two pieces join.

A slicer can't wait for the wave to cross zero, because the step must start on the beat, so it fades. At every change (from the track to a repeat, at each start of a repeated piece, into and out of a gap, at both ends of a reverse) DawIt Slicer fades the old sound out while it fades the new one in, over the Smooth time, starting on the step's boundary. The two always add up to 100%, so a join is never louder than the louder of the two sounds.

3 ms, where it starts, keeps a drum's attack sharp and takes the click out of most sounds. Clicks are easiest to hear on low, smooth sounds such as a bass or a sine wave, where nothing else covers them; raise Smooth a little if you still hear them. A fade is never longer than a quarter of its piece, so short pieces (Roll on 1/32 steps, or a long Smooth on short steps) still have sound in them.

The picture shows 10 ms of a low note cut at a step: without a fade the wave jumps; with Smooth it bends over to the new sound.)slc" },
        { "How DawIt Slicer works", "The song's clock and a memory", R"slc(1. It follows the song's clock. DawIt tells every effect on a track the tempo and where the playhead is, for every block of sound. From that, DawIt Slicer works out which step it's in: the steps since the start of the song, counted round the pattern. So step 1 is on the first beat of bar 1 whether you play from the start or from bar 9, loop a section or change the tempo.

2. It records every step. While a step plays, what comes in is written into a memory. When the next step is a Repeat or Reverse, that recording becomes the slice it plays from, and stays so for the whole row. The memory holds two of the longest steps: a 1/4 at 20 BPM, DawIt's slowest tempo, is 3 seconds, which at 48 kHz is 144,000 samples for each side. It is made when playback is prepared, never while the sound runs, and never grows.

3. It starts empty. After you press play, jump or loop back, it hasn't heard the step before, so a Repeat or Reverse step right then plays the track through. From the next step on it has a slice.

4. When the song is stopped, it keeps its own clock at the tempo it uses (the song's, or its own with Own), so you still hear it on the test beat and on what you play live. Play a bar starts that clock on step 1.

5. Tempo: Song follows the song's tempo and position. Own uses the BPM you set, for a part recorded at another tempo, or in a program that doesn't tell plugins its tempo; while the song plays, its steps still count from the start of the song. As a plugin in another program, DawIt Slicer follows that program's tempo and position when it gives them, and its own tempo (120 BPM to start with) when it doesn't.

6. It holds nothing back: there's no latency, so the track stays in time whether DawIt Slicer is on or bypassed. Put it after DawIt Tune in a chain: DawIt Tune holds the sound back a little, and a slicer before it would cut slightly ahead of the beat.

7. The output is mixed with the untouched track by Mix, in straight proportion, so a Through step sounds exactly like the track at any Mix.

The picture follows the sound from the input through the memory, the choice for each step and the fades, to the mix.)slc" },
        { "Slicing in a mix", "Where stutters work", R"slc(Slicing works best as a contrast. A part that stutters all the time stops sounding like an effect; a stutter in the last bar before the chorus makes the chorus land harder. That's an opinion, but a widely shared one.

Where it's used:
- Build-ups and fills: Repeat steps on the last beat of a bar or section (Stutter into the next bar).
- Pads and held chords: a Mute pattern, the trance gate.
- Drums and loops: a Reverse step at the end of a bar for a swell; Repeat with Roll for a drum roll.
- Hi-hats: rolls of 1/32 or 1/64 notes, often just before a snare or at the end of a bar, as in trap.
- Vocals: one word chopped into a stammer, a common hook in dance and pop. Use a copy of the vocal, so the main one stays clear.

How it's used:
- DawIt can't yet switch an effect on and off at a point in the song. To slice just one bar, copy that bar's clip to a track of its own with DawIt Slicer, and leave the bar out of the original track.
- Keep Mix at 100% for clean stutters and gaps; lower it to keep some of the original underneath.
- Check that the slices start on a note. A Repeat after a silent step repeats silence; a Reverse after a quiet step barely swells. Your track points these out.
- Clicks: if you hear them, raise Smooth a millisecond or two.

The picture shows eight bars with slicing only in the bar before the chorus.)slc" },
        { "Your track", "What the pattern does to this track", R"slc(This topic looks at the last 2 bars of the track DawIt Slicer is on, step by step, as the slicer heard them, and says what to try.

For each step it compares how loud the track came in and how loud it came out. It points out repeats and reverses of slices that were almost silent, how much quieter the pattern makes the track on average, and a pattern that changes most of its steps.

Play the track, or click Play a bar, and the picture follows.)slc" },
    };

    for (auto& t : texts)
        pages.push_back ({ t.name, t.summary, t.text, nullptr });

    // 1. Slices of time: one bar of the test beat cut into steps.
    pages[0].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const double bpm = bpmOf(), sb = slc::getStepBeats (s.stepLength);
        const int numSteps = juce::roundToInt (4.0 / sb);
        drawCaption (g, r, "One bar of the test beat, cut into steps", slc::formatKnob (Knob::stepLength, s) + " at " + juce::String (juce::roundToInt (bpm)) + " BPM: " + formatMs (slc::getStepMs (s.stepLength, bpm)) + " a step");
        auto lane = r.withTrimmedBottom (40.0f);
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (lane);

        for (int i = 0; i < numSteps; ++i)
        {
            const float x0 = lane.getX() + lane.getWidth() * (float) i / numSteps, x1 = lane.getX() + lane.getWidth() * (float) (i + 1) / numSteps;
            g.setColour (colour.withAlpha (i % 2 == 0 ? 0.12f : 0.04f));
            g.fillRect (x0, lane.getY(), x1 - x0, lane.getHeight());
        }

        const int width = (int) lane.getWidth();

        for (int x = 0; x < width; ++x)
        {
            const float level = getTestLevel ((x + 0.5) / width * 4.0, bpm) * (lane.getHeight() * 0.5f - 4.0f);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillRect (lane.getX() + (float) x, lane.getCentreY() - level, 1.0f, level * 2.0f);
        }

        for (int i = 0; i <= numSteps; ++i)
        {
            const float x = lane.getX() + lane.getWidth() * (float) i / numSteps;
            g.setColour (i % juce::jmax (1, juce::roundToInt (1.0 / sb)) == 0 ? theme::gridBar : theme::grid);
            g.fillRect (x - 0.5f, lane.getY(), 1.0f, lane.getHeight());
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 34.0f), juce::String (numSteps) + " steps make a bar of 4 beats. Brighter lines are the beats; the shaded steps alternate. "
                  "A step is cut where the grid says, even in the middle of a note.");
    };

    // 2. Note lengths: every length against one bar.
    pages[1].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const double bpm = bpmOf();
        drawCaption (g, r, "Every step length against one bar", "at " + juce::String (juce::roundToInt (bpm)) + " BPM, 1 beat = " + formatMs (60000.0 / bpm));
        const float rowHeight = r.getHeight() / (float) slc::numStepLengths, labelWidth = 120.0f;

        for (int row = 0; row < slc::numStepLengths; ++row)
        {
            const int index = slc::numStepLengths - 1 - row;   // 1/4 first
            const auto line = juce::Rectangle<float> (r.getX(), r.getY() + rowHeight * (float) row, r.getWidth(), rowHeight).reduced (0.0f, 2.0f);
            const bool chosen = index == s.stepLength;
            auto bar = line.withTrimmedLeft (labelWidth);
            g.setColour (chosen ? colour.withAlpha (0.2f) : juce::Colours::black.withAlpha (0.2f));
            g.fillRect (line);
            g.setFont (juce::FontOptions (12.0f, chosen ? juce::Font::bold : juce::Font::plain));
            g.setColour (chosen ? theme::text : theme::textDim);
            g.drawText (juce::String (slc::stepLengths[index].name) + "   " + formatMs (slc::getStepMs (index, bpm)), line.withWidth (labelWidth).withTrimmedLeft (6.0f), juce::Justification::centredLeft);

            const double sb = slc::stepLengths[index].beats;

            for (int i = 0; i * sb <= 4.0 + 1.0e-6; ++i)
            {
                const float x = bar.getX() + bar.getWidth() * (float) (i * sb / 4.0);
                g.setColour (chosen ? colour : theme::gridBar);
                g.fillRect (x - 0.5f, bar.getY() + 3.0f, 1.0f, bar.getHeight() - 6.0f);
            }

            g.setColour (theme::text.withAlpha (0.35f));

            for (int beat = 0; beat <= 4; ++beat)
                g.fillRect (bar.getX() + bar.getWidth() * (float) beat / 4.0f - 0.5f, bar.getY(), 1.0f, 3.0f);
        }
    };

    // 3. Repeat: input slices above, output below.
    pages[2].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        bool demo = false;
        const auto s = getExample (processor->getSettings(), { Mode::repeat }, { Mode::through, Mode::repeat, Mode::repeat, Mode::repeat }, demo);
        drawCaption (g, r, "What comes in, and what comes out", juce::String (demo ? "an example: Through, then three Repeat steps; " : "your pattern; ") + "Roll " + juce::String (s.roll));
        drawStepRows (g, r, s, bpmOf(), s.steps, true);
    };

    // 4. Repeat or echo: one hit through a delay and through three Repeat steps.
    pages[3].paint = [bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const double stepMs = slc::getStepMs (2, bpmOf());
        drawCaption (g, r, "One hit, through a delay and through a slicer", "1/16 steps of " + formatMs (stepMs));
        const float laneHeight = (r.getHeight() - 20.0f) / 2.0f;
        const double span = 4.4 * stepMs;

        for (int pass = 0; pass < 2; ++pass)
        {
            const auto lane = r.withHeight (laneHeight).translated (0.0f, (laneHeight + 12.0f) * (float) pass);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (lane);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (pass == 0 ? "DawIt Delay: 1/16, feedback 50%" : "DawIt Slicer: three Repeat steps", lane.reduced (6.0f, 3.0f), juce::Justification::topLeft);

            for (int n = 0; n < 4; ++n)
            {
                const double db = pass == 0 ? 20.0 * std::log10 (std::pow (0.5, n)) : 0.0;
                const float level = (float) juce::jlimit (0.04, 1.0, 1.0 + db / 30.0);
                const float x = lane.getX() + 20.0f + (lane.getWidth() - 60.0f) * (float) (n * stepMs / span);
                const float h = level * (laneHeight - 52.0f);
                g.setColour (n == 0 ? juce::Colours::white : pass == 0 ? juce::Colour (0xffb79cff) : colour);
                g.fillRoundedRectangle (x, lane.getBottom() - 20.0f - h, 14.0f, h, 3.0f);
                g.setFont (juce::FontOptions (11.0f));
                g.setColour (theme::text);
                g.drawText (juce::String (db, 1) + " dB", juce::Rectangle<float> (x - 22.0f, lane.getBottom() - 20.0f - h - 15.0f, 58.0f, 14.0f), juce::Justification::centred);
                g.setColour (theme::textDim);
                g.drawText (juce::String (juce::roundToInt (n * stepMs)) + " ms", juce::Rectangle<float> (x - 22.0f, lane.getBottom() - 16.0f, 58.0f, 14.0f), juce::Justification::centred);
            }
        }
    };

    // 5. Reverse: a hit forwards, then the same slice backwards.
    pages[4].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        bool demo = false;
        const auto s = getExample (processor->getSettings(), { Mode::reverse }, { Mode::through, Mode::reverse }, demo);
        drawCaption (g, r, "A hit as it came in, and played backwards", demo ? "an example: Through, then Reverse" : "your pattern");
        drawStepRows (g, r, s, bpmOf(), s.steps, true);
    };

    // 6. Mute: a steady chord, and the gain the pattern gives it.
    pages[5].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        bool demo = false;
        const auto s = getExample (processor->getSettings(), { Mode::mute }, { Mode::through, Mode::mute, Mode::through, Mode::mute, Mode::through, Mode::mute, Mode::through, Mode::mute }, demo);
        // Fades are kept to a quarter of a step, as the slicer does.
        const double bpm = bpmOf(), sb = slc::getStepBeats (s.stepLength), stepMs = slc::getStepMs (s.stepLength, bpm);
        const double fadeMs = juce::jmin ((double) s.smoothMs, stepMs / 4.0), fade = fadeMs / 1000.0 * bpm / 60.0;
        drawCaption (g, r, "A steady chord before and after the pattern", juce::String (demo ? "an example: every other step Mute; " : "your pattern; ") + "Smooth " + slc::formatKnob (Knob::smooth, s) + ", Mix " + slc::formatKnob (Knob::mix, s));
        const float laneHeight = (r.getHeight() - 40.0f) / 2.0f;
        const auto top = r.withHeight (laneHeight), bottom = r.withTrimmedTop (r.getHeight() - laneHeight - 22.0f).withHeight (laneHeight);
        const float mix = s.mixPercent / 100.0f;

        for (auto lane : { top, bottom })
        {
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (lane);
        }

        g.setColour (juce::Colour (0xff8a8f9a).withAlpha (0.6f));
        g.fillRect (top.reduced (0.0f, 12.0f));

        auto gainOf = [&] (double beats)
        {
            const int k = (int) std::floor (beats / sb + 1.0e-9), prev = k - 1;
            auto g0 = [&] (int step) { return s.modes[(size_t) (((step % s.steps) + s.steps) % s.steps)] == Mode::mute ? 0.0 : 1.0; };
            const double w = fade > 0.0 ? juce::jlimit (0.0, 1.0, (beats - k * sb) / fade) : 1.0;
            const double gain = g0 (prev) + (g0 (k) - g0 (prev)) * (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * w));
            return (1.0 - mix) + mix * gain;
        };

        juce::Path fill;
        const int width = (int) bottom.getWidth();
        fill.startNewSubPath (bottom.getX(), bottom.getBottom());

        for (int x = 0; x < width; ++x)
            fill.lineTo (bottom.getX() + (float) x, bottom.getBottom() - 12.0f - (float) gainOf ((double) x / width * s.steps * sb + 1.0e-6) * (bottom.getHeight() - 24.0f) + 0.0f);

        fill.lineTo (bottom.getRight(), bottom.getBottom());
        fill.closeSubPath();
        g.setColour (colour.withAlpha (0.85f));
        g.fillPath (fill);

        for (int i = 0; i <= s.steps; ++i)
        {
            g.setColour (theme::gridBar);
            g.fillRect (r.getX() + r.getWidth() * (float) i / s.steps - 0.5f, top.getY(), 1.0f, bottom.getBottom() - top.getY());
        }

        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::textDim);
        g.drawText ("the chord", top.reduced (6.0f, 2.0f), juce::Justification::topLeft);
        g.drawText ("after the pattern", bottom.reduced (6.0f, 2.0f), juce::Justification::topLeft);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 20.0f), "Each edge fades over " + formatMs (fadeMs) + ", " + formatShort (100.0 * fadeMs / stepMs) + "% of a step.");
    };

    // 7. Clicks: 10 ms of a 110 Hz sine cut at a step boundary, without and with the fade.
    pages[6].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const double fadeMs = s.smoothMs;
        drawCaption (g, r, "10 ms of a 110 Hz note, cut at 5 ms", "the new piece starts at the opposite phase");

        for (int pass = 0; pass < 2; ++pass)
        {
            const float laneHeight = (r.getHeight() - 30.0f) / 2.0f;
            const auto lane = r.withHeight (laneHeight).translated (0.0f, (laneHeight + 18.0f) * (float) pass);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (lane);
            g.setColour (theme::grid);
            g.fillRect (lane.getX(), lane.getCentreY(), lane.getWidth(), 1.0f);
            g.setColour (theme::gridBar);
            g.fillRect (lane.getX() + lane.getWidth() * 0.5f - 0.5f, lane.getY(), 1.0f, lane.getHeight());

            auto wave = [] (double ms, bool opposite) { return (opposite ? -1.0 : 1.0) * std::sin (juce::MathConstants<double>::twoPi * 110.0 * ms / 1000.0); };
            juce::Path p;
            const int width = (int) lane.getWidth();

            for (int x = 0; x <= width; ++x)
            {
                const double ms = 10.0 * x / width;
                double v;

                if (ms < 5.0)
                    v = wave (ms, false);
                else if (pass == 0)
                    v = wave (ms, true);
                else
                {
                    const double w = juce::jlimit (0.0, 1.0, (ms - 5.0) / fadeMs), mix = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * w);
                    v = (1.0 - mix) * wave (ms, false) + mix * wave (ms, true);
                }

                const juce::Point<float> pt { lane.getX() + (float) x, lane.getCentreY() - (float) v * (laneHeight * 0.5f - 6.0f) };
                x == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
            }

            g.setColour (pass == 0 ? juce::Colour (0xffff7a7a) : colour);
            g.strokePath (p, juce::PathStrokeType (1.8f));
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (pass == 0 ? "Cut without a fade: the wave jumps, you hear a click" : "With Smooth (" + formatMs (fadeMs) + "): it bends over to the new sound", lane.reduced (8.0f, 3.0f), juce::Justification::topLeft);
        }
    };

    // 8. How it works: a block diagram.
    pages[7].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "From the track to the mix", "the held slice is marked");
        const float col = (r.getWidth() - 4.0f * 22.0f) / 5.0f, midY = r.getCentreY() - 8.0f;
        auto box = [&] (int column, float height = 60.0f) { return juce::Rectangle<float> (r.getX() + (col + 22.0f) * (float) column, midY - height / 2.0f, col, height); };
        const auto in = drawBox (g, box (0), "Track in", "every block", theme::textDim);
        const auto memory = drawBox (g, box (1, 104.0f), "Step memory", "two slices", colour);
        const auto choose = drawBox (g, box (2), "Choose by step", "Through, Repeat, Reverse, Mute", colour);
        const auto fade = drawBox (g, box (3), "Fade", "Smooth, at every cut", colour);
        const auto mix = drawBox (g, box (4), "Mix", "with the dry track", theme::accent);
        arrow (g, { in.getRight(), midY }, { memory.getX(), midY });
        arrow (g, { memory.getRight(), midY }, { choose.getX(), midY });
        arrow (g, { choose.getRight(), midY }, { fade.getX(), midY });
        arrow (g, { fade.getRight(), midY }, { mix.getX(), midY });

        // The two slices in the memory: the one being recorded, and the held one.
        const auto cellA = memory.reduced (10.0f, 0.0f).withTop (memory.getBottom() - 22.0f).withHeight (14.0f).withTrimmedRight (memory.getWidth() * 0.5f - 8.0f);
        g.setColour (theme::textDim.withAlpha (0.5f));
        g.fillRoundedRectangle (cellA, 2.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (cellA.translated (cellA.getWidth() + 4.0f, 0.0f), 2.0f);
        g.setFont (juce::FontOptions (10.0f));
        g.setColour (theme::text);
        g.drawText ("held", cellA.translated (cellA.getWidth() + 4.0f, -14.0f), juce::Justification::centred);
        g.setColour (theme::textDim);
        g.drawText ("recording", cellA.translated (0.0f, -14.0f), juce::Justification::centred);

        // The clock above, choosing the step.
        const auto clock = drawBox (g, juce::Rectangle<float> (choose.getX() - 20.0f, midY - 118.0f, choose.getWidth() + 40.0f, 44.0f), "The song's clock", "tempo and position", theme::accent);
        arrow (g, { clock.getCentreX(), clock.getBottom() }, { choose.getCentreX(), choose.getY() });

        // The dry track around everything, to the mix.
        juce::Path dry;
        const float under = midY + 70.0f;
        dry.startNewSubPath (in.getCentreX(), in.getBottom());
        dry.lineTo (in.getCentreX(), under);
        dry.lineTo (mix.getCentreX(), under);
        g.setColour (theme::textDim);
        g.strokePath (dry, juce::PathStrokeType (1.2f));
        g.drawArrow ({ mix.getCentreX(), under, mix.getCentreX(), mix.getBottom() }, 1.2f, 7.0f, 7.0f);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("the dry track", juce::Rectangle<float> (mix.getCentreX() - in.getCentreX(), 16.0f).withPosition (in.getCentreX(), under + 3.0f), juce::Justification::centred);
    };

    // 9. Slicing in a mix: eight bars, slicing only in the bar before the chorus.
    pages[8].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "Eight bars: slicing only in bar 8", "the bar before the chorus");
        const auto strip = r.withHeight (juce::jmin (150.0f, r.getHeight() - 30.0f));
        const float barWidth = strip.getWidth() / 9.0f;

        for (int bar = 0; bar < 9; ++bar)
        {
            const auto cell = juce::Rectangle<float> (strip.getX() + barWidth * (float) bar, strip.getY(), barWidth, strip.getHeight()).reduced (2.0f, 0.0f);
            g.setColour (bar == 8 ? juce::Colour (0xff5fd38d).withAlpha (0.3f) : bar == 7 ? colour.withAlpha (0.28f) : theme::panelLight);
            g.fillRoundedRectangle (cell, 4.0f);
            g.setFont (juce::FontOptions (12.0f, bar == 7 ? juce::Font::bold : juce::Font::plain));
            g.setColour (theme::text);
            g.drawText (bar == 8 ? "Chorus" : "Bar " + juce::String (bar + 1), cell.withHeight (20.0f), juce::Justification::centred);

            if (bar == 7)
            {
                // The pattern in the bar before the chorus, as small cells.
                const auto inner = cell.reduced (4.0f).withTrimmedTop (22.0f).withHeight (36.0f);

                for (int i = 0; i < s.steps; ++i)
                {
                    g.setColour (getModeColour (s.modes[(size_t) i]).withAlpha (0.9f));
                    g.fillRect (juce::Rectangle<float> (inner.getX() + inner.getWidth() * (float) i / s.steps + 0.5f, inner.getY(), inner.getWidth() / s.steps - 1.0f, inner.getHeight()));
                }
            }
        }

        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::textDim);
        drawNote (g, r.withTrimmedTop (strip.getHeight() + 30.0f).withHeight (30.0f), "Bars 1 to 7 play as they are; the pattern (your settings, one bar of it) is in bar 8 only. In DawIt that means a copy "
                  "of bar 8 on a track of its own with DawIt Slicer.");
    };

    // 10. Your track: the last 2 bars, and the advice.
    pages[9].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto host = fxui::getHostTrack (*processor);
        const auto s = processor->getSettings();
        drawCaption (g, r, (host.name.isEmpty() ? "Stand-in slicer" : host.name) + ": the last 2 bars", slc::formatKnob (Knob::stepLength, s) + ", " + juce::String (s.steps) + " steps");
        drawLastBars (g, r, getLastBars (*processor), s);
    };
    const auto introduction = pages[9].text;
    pages[9].liveText = [processor, introduction]
    {
        slc::AnalysisInput input;
        input.settings = processor->getSettings();
        input.records = getLastBars (*processor);
        input.bpm = processor->getUsedBpm();
        input.hostHasTempo = processor->tempoBpm.load() > 0.0;
        juce::StringArray lines;

        for (auto& line : slc::analyse (input))
            lines.add (line);

        return introduction + "\n\n" + lines.joinIntoString ("\n\n");
    };

    return pages;
}
} // namespace daw::slctext
