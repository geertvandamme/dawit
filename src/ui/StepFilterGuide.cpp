#include "StepFilterGuide.h"

namespace daw::stftext
{
using stf::Knob;

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

    juce::String formatDb (double db, int decimals = 1)
    {
        return (db > 0.05 ? "+" : "") + fx::formatNumber (db, decimals) + " dB";
    }

    const char* const stepNouns[] = { "thirty-second", "sixteenth triplet", "sixteenth", "eighth triplet", "eighth", "quarter" };

    juce::String getOrdinal (int n)
    {
        return juce::String (n) + (n % 100 >= 11 && n % 100 <= 13 ? "th" : n % 10 == 1 ? "st" : n % 10 == 2 ? "nd" : n % 10 == 3 ? "rd" : "th");
    }

    juce::String dot() { return juce::String::fromUTF8 (" \xc2\xb7 "); }

    /** The cutoff the settings give a step, kept to what the filter can do at 44.1 kHz. */
    double getShownCutoff (const stf::Settings& s, int step) { return stf::clampCutoff (stf::getStepCutoffHz (s, step), 44100.0); }

    /** The lower and higher of the two cutoff knobs. */
    std::pair<double, double> getRange (const stf::Settings& s)
    {
        return { (double) juce::jmin (s.lowHz, s.highHz), (double) juce::jmax (s.lowHz, s.highHz) };
    }

    double getOctaves (const stf::Settings& s)
    {
        const auto [low, high] = getRange (s);
        return std::log2 (juce::jmax (1.0, high) / juce::jmax (1.0, low));
    }

    /** How much the filter changes a level at `hz`, in dB. */
    double getDb (const stf::Settings& s, double cutoff, double hz) { return filt::getFilterDb (s.type, s.steep, cutoff, s.resonancePercent, hz); }

    //==============================================================================
    /** A frequency (log) against level (dB) plot. */
    struct Plot
    {
        juce::Rectangle<float> area;
        double minDb = -48.0, maxDb = 30.0, minHz = 20.0, maxHz = 20000.0;

        float x (double hz) const
        {
            return area.getX() + area.getWidth() * (float) (std::log (juce::jlimit (minHz, maxHz, hz) / minHz) / std::log (maxHz / minHz));
        }

        float y (double db) const { return area.getBottom() - area.getHeight() * (float) (juce::jlimit (minDb, maxDb, db) - minDb) / (float) (maxDb - minDb); }
    };

    void drawPlotGrid (juce::Graphics& g, const Plot& plot, double dbStep)
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot.area);
        g.setFont (juce::FontOptions (10.0f));

        for (double db = std::ceil (plot.minDb / dbStep) * dbStep; db <= plot.maxDb + 1.0e-6; db += dbStep)
        {
            if (db < plot.minDb + 6.0)
                continue;   // keeps the lowest label out of the frequency labels' way

            g.setColour (juce::approximatelyEqual (db, 0.0) ? theme::gridBar : theme::grid);
            g.fillRect (plot.area.getX(), plot.y (db) - 0.5f, plot.area.getWidth(), 1.0f);
            g.setColour (theme::textDim);
            g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (plot.area.getX() + 2.0f, plot.y (db) - 12.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
        }

        for (const double hz : { 100.0, 1000.0, 10000.0 })
        {
            g.setColour (theme::grid);
            g.fillRect (plot.x (hz) - 0.5f, plot.area.getY(), 1.0f, plot.area.getHeight());
            g.setColour (theme::textDim);
            g.drawText (hz >= 1000.0 ? juce::String (juce::roundToInt (hz / 1000.0)) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz",
                        juce::Rectangle<float> (plot.x (hz) - 24.0f, plot.area.getBottom() - 13.0f, 48.0f, 12.0f), juce::Justification::centred);
        }
    }

    juce::Path makeCurve (const Plot& plot, const std::function<double (double)>& db)
    {
        juce::Path p;
        const int width = juce::jmax (2, (int) plot.area.getWidth());

        for (int i = 0; i <= width; ++i)
        {
            const double hz = plot.minHz * std::pow (plot.maxHz / plot.minHz, (double) i / width);
            const juce::Point<float> pt { plot.area.getX() + (float) i * plot.area.getWidth() / (float) width, plot.y (db (hz)) };
            i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
        }

        return p;
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

    /** The harmonics of the test chord: saws at A2, E3, A3, C4 and E4, each harmonic n at 1/n. */
    struct Harmonic
    {
        double hz, amplitude;
    };

    const std::vector<Harmonic>& getChordHarmonics()
    {
        static const std::vector<Harmonic> list = []
        {
            std::vector<Harmonic> h;

            for (const double f : { 110.0, 164.8, 220.0, 261.6, 329.6 })
                for (int n = 1; n * f <= 20000.0; ++n)
                    h.push_back ({ n * f, 1.0 / n });

            return h;
        }();

        return list;
    }

    /** What the chord's power does through the filter at `cutoff`, in dB against the unfiltered chord. */
    double getChordLevelDb (const stf::Settings& s, double cutoff)
    {
        double in = 0.0, out = 0.0;

        for (auto& h : getChordHarmonics())
        {
            const double p = h.amplitude * h.amplitude;
            in += p;
            out += p * std::pow (10.0, getDb (s, cutoff, h.hz) / 10.0);
        }

        return 10.0 * std::log10 (juce::jmax (1.0e-12, out / in));
    }

    /** "50%". */
    juce::String getHeightText (float percent) { return juce::String (juce::roundToInt (percent)) + "%"; }
}

//==============================================================================
juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength: return "Step length";
        case (int) Knob::steps:      return "Steps";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:     return "Tempo";
        case (int) Knob::type:       return "Type";
        case (int) Knob::slope:      return "Slope";
        case (int) Knob::lowCutoff:  return "Low cutoff";
        case (int) Knob::highCutoff: return "High cutoff";
        case (int) Knob::resonance:  return "Resonance";
        case (int) Knob::glide:      return "Glide";
        case (int) Knob::mix:        return "Mix";
        case (int) Knob::output:     return "Output";
        case pattern:                return "The pattern";
        case filterNow:              return "The filter now";
        case fxui::overview:         return "DawIt Step Filter";
        case fxui::presets:          return "Starting points";
        case fxui::clap:             return "Play a bar";
        case fxui::listen:           return "Hear only what it removes";
        case fxui::guide:            return "What is a step filter?";
        case fxui::history:          return "The last 4 seconds";
        default:                     return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength:
            return "How long each step lasts, as a note value of the song's tempo, 1/32 to 1/4 with triplets. At 120 BPM a 1/16 step is 125 ms.";
        case (int) Knob::steps:
            return "How many steps the pattern has before it starts again, counted from the start of the song. 16 steps of 1/16 make one bar.";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:
            return "Song: the steps follow the song's tempo and position. Own: they follow the BPM set here, on the step filter's own clock "
                   "(also used as a plugin when the program gives no tempo).";
        case (int) Knob::type:
            return "Low-pass keeps the lows: low steps are dark. High-pass keeps the highs: high steps are thin. Band-pass keeps a band around the cutoff: nasal and vocal.";
        case (int) Knob::slope:
            return "How steeply the filter turns things down past the cutoff: 12 or 24 dB per octave. 24 makes a stronger movement.";
        case (int) Knob::lowCutoff:
            return "The cutoff of a step at the bottom (0%). Turn it to move the low end of the whole pattern.";
        case (int) Knob::highCutoff:
            return "The cutoff of a step at the top (100%). Steps in between are spaced evenly in octaves.";
        case (int) Knob::resonance:
            return "A peak at the cutoff. It makes each step's cutoff sing; at 50% a 12 dB low-pass is 11.5 dB louder at the cutoff, at 100% 26 dB.";
        case (int) Knob::glide:
            return "How long the cutoff takes to reach each step's value, as a share of the step (ms below). 0% jumps (still over 2 ms, so it doesn't click); 100% slides the whole step.";
        case (int) Knob::mix:
            return "How much of what you hear is the filtered sound. Below 100% the filtered sound partly cancels the original near the cutoff, a moving dip.";
        case (int) Knob::output:
            return juce::String::fromUTF8 ("The level after the filter, from \xe2\x88\x92" "24 to +6 dB. Turn it down when resonance makes the track louder.");
        case pattern:
            return "Each bar is a step; its height sets its cutoff between Low and High. Click or drag to set steps, Alt for exact values, double-click for the top. The lit step is playing now.";
        case filterNow:
            return "The filter at the cutoff playing now: what passes (above the 0 dB line is boosted by resonance). The shaded band is Low to High.";
        case fxui::clap:
            return "Plays one bar of a held A minor chord of saw waves through the filter, from step 1, at the tempo it uses. It works while the song is stopped.";
        case fxui::listen:
            return "Plays what the filter takes away, so you hear which frequencies each step removes.";
        case fxui::guide:
            return "A guide to step filters, for beginners: a pattern of tone, cutoff and the three filters, resonance, steps and heights, glide, "
                   "how it differs from a slicer and an LFO, mix and level, how it works, step filters in a mix, and your track. The pictures "
                   "follow the controls here.";
        case fxui::history:
            return "The last 4 seconds, newest on the right: the track coming in (grey) and the part of the sound the filter changed (colour), with the beats of the song.";
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength:
            return "1/16 for acid and fast pulses, 1/8 for chords and pads, 1/4 for slow movements and rises.";
        case (int) Knob::steps:
            return "16 for a bar of 1/16 or two of 1/8; 8 for a bar of 1/8.";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:
            return "Song, nearly always. Own for a part recorded at another tempo.";
        case (int) Knob::type:
            return "Low-pass nearly always; high-pass for drums and rises; band-pass for talking effects.";
        case (int) Knob::slope:
            return "24 dB for basses, acid and obvious pulses; 12 dB for pads and gentle movement.";
        case (int) Knob::lowCutoff:
            return "Basses 150-500 Hz, pads 400-800 Hz, high-pass rises 30 Hz.";
        case (int) Knob::highCutoff:
            return "2-5 kHz for an obvious movement; 20 kHz for steps that open fully.";
        case (int) Knob::resonance:
            return "0-20% for natural movement, 30-50% for character, 60-80% for acid. Lower Output as you raise it.";
        case (int) Knob::glide:
            return "0-10% for crisp steps, 20-50% for a softer pulse, 100% for sweeps.";
        case (int) Knob::mix:
            return "100% nearly always.";
        case (int) Knob::output:
            return juce::String::fromUTF8 ("0 dB without resonance; \xe2\x88\x92" "6 to \xe2\x88\x92" "12 dB with 50% or more.");
        default:
            return {};
    }
}

juce::String getTooltip (int t)
{
    switch (t)
    {
        case (int) Knob::stepLength: return "How long a step lasts, as a note value";
        case (int) Knob::steps:      return "How many steps before the pattern repeats";
        case (int) Knob::tempo:
        case (int) Knob::ownBpm:     return "Follow the song's tempo, or a tempo of its own";
        case (int) Knob::type:       return "Which frequencies the filter keeps";
        case (int) Knob::slope:      return "12 or 24 dB per octave";
        case (int) Knob::lowCutoff:  return "The cutoff of the lowest steps";
        case (int) Knob::highCutoff: return "The cutoff of the highest steps";
        case (int) Knob::resonance:  return "A peak at the cutoff: makes the steps sing";
        case (int) Knob::glide:      return "Jump or slide between steps";
        case (int) Knob::mix:        return "The filtered sound against the untouched track";
        case (int) Knob::output:     return "The level after the filter";
        case fxui::clap:             return "Play a held chord through the step filter";
        case fxui::listen:           return "Hear only what the filter takes away (not saved)";
        default:                     return {};
    }
}

juce::String describeSettings (int t, const stf::Settings& s, double hostBpm)
{
    const double bpm = s.ownTempo || hostBpm <= 0.0 ? stf::clampBpm (s.ownBpm) : stf::clampBpm (hostBpm);
    const double stepBeats = stf::getStepBeats (s.stepLength), stepMs = stf::getStepMs (s.stepLength, bpm);
    const juce::String length = stf::formatKnob (Knob::stepLength, s);
    const auto [low, high] = getRange (s);

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
                return "Song, but no tempo has reached the step filter yet: it uses its own " + formatShort (s.ownBpm) + " BPM until one does.";

            return "Song: the steps follow the song's " + formatShort (hostBpm) + " BPM.";

        case (int) Knob::type:
            return stf::getTypeName (s.type) + (s.type == filt::FilterType::lowPass ? ": keeps what is below each step's cutoff."
                                                : s.type == filt::FilterType::highPass ? ": keeps what is above each step's cutoff."
                                                                                       : ": keeps a band around each step's cutoff.");

        case (int) Knob::slope:
        {
            // One octave past the cutoff on the side the filter turns down (above for low- and band-pass, below for high-pass).
            const double hz = s.type == filt::FilterType::highPass ? 500.0 : 2000.0;
            const double db = filt::getFilterDb (s.type, s.steep, 1000.0, 0.0f, hz);
            return juce::String (s.steep ? 24 : 12) + " dB per octave: with no resonance, one octave past the cutoff is " + fx::formatNumber (-db, 1) + " dB down.";
        }

        case (int) Knob::lowCutoff:
            return stf::formatHz (low) + ": the lowest steps' cutoff. " + stf::formatHz (high) + " is the top, " + formatShort (std::round (getOctaves (s) * 10.0) / 10.0) + " octaves up.";

        case (int) Knob::highCutoff:
            return stf::formatHz (high) + ": the highest steps' cutoff. 25% is " + stf::formatHz (stf::getHeightCutoffHz (low, high, 25.0)) + ", 50% "
                   + stf::formatHz (stf::getHeightCutoffHz (low, high, 50.0)) + ", 75% " + stf::formatHz (stf::getHeightCutoffHz (low, high, 75.0)) + ".";

        case (int) Knob::resonance:
        {
            const double q = filt::getQ (s.resonancePercent);

            if (s.type == filt::FilterType::bandPass)
                return juce::String (juce::roundToInt (s.resonancePercent)) + "%: Q " + fx::formatNumber (q, 2) + ", so at 1 kHz the band is " + formatShort (std::round (filt::getBandWidthHz (s.steep, 1000.0, s.resonancePercent))) + " Hz wide ("
                       + formatShort (std::round (filt::getBandWidthHz (s.steep, 1000.0, s.resonancePercent) / 10.0)) + "% of its cutoff).";

            return juce::String (juce::roundToInt (s.resonancePercent)) + "%: " + formatDb (filt::getFilterDb (s.type, s.steep, 1000.0, s.resonancePercent, 1000.0))
                   + " at the cutoff itself (Q " + fx::formatNumber (q, 2) + ").";
        }

        case (int) Knob::glide:
        {
            const double ms = stf::getGlideMs (s, bpm);
            return juce::String (juce::roundToInt (s.glidePercent)) + "% of a " + formatMs (stepMs) + " step: " + formatMs (ms)
                   + (ms <= stf::minGlideMs + 1.0e-6 ? ", the least it ever takes." : ".");
        }

        case (int) Knob::mix:
            if (s.mixPercent >= 99.5f)
                return "100%: only the filtered sound is heard.";

            if (s.mixPercent < 0.5f)
                return "0%: the filter is not heard at all.";

            return formatShort (s.mixPercent) + "%: the filtered sound with the untouched track under it.";

        case (int) Knob::output:
            return fx::formatNumber (s.outputDb, 1) + " dB: the sound after the filter is x" + fx::formatNumber (juce::Decibels::decibelsToGain (s.outputDb), 3) + ".";

        default:
            return {};
    }
}

//==============================================================================
int PatternGrid::getStepAt (juce::Point<float> p) const
{
    if (! cells.expanded (0.0f, 6.0f).contains (p))
        return -1;

    return juce::jlimit (0, stf::maxSteps - 1, (int) ((p.x - cells.getX()) / cellWidth));
}

float PatternGrid::getHeightAt (float y) const
{
    return 100.0f * juce::jlimit (0.0f, 1.0f, (cells.getBottom() - y) / juce::jmax (1.0f, cells.getHeight()));
}

PatternGrid layoutPattern (juce::Rectangle<float> bounds)
{
    PatternGrid grid;
    grid.cells = bounds.withTrimmedTop (26.0f).withTrimmedBottom (32.0f).withTrimmedLeft (50.0f);
    grid.cellWidth = grid.cells.getWidth() / (float) stf::maxSteps;
    return grid;
}

void drawPattern (juce::Graphics& g, const PatternGrid& grid, const stf::Settings& s, double bpm, int playingStep, int hovered)
{
    const double sb = stf::getStepBeats (s.stepLength);
    const auto row = grid.cells;
    const auto [low, high] = getRange (s);

    // The beat and bar lines: a line per beat, stronger at each bar.
    for (int beat = 0; beat * (1.0 / sb) <= stf::maxSteps + 1.0e-6; ++beat)
    {
        const float x = row.getX() + grid.cellWidth * (float) (beat / sb);

        if (x > row.getRight() + 1.0f)
            break;

        g.setColour (beat % 4 == 0 ? theme::gridBar : theme::grid);
        g.fillRect (x - 0.5f, row.getY() - 6.0f, 1.0f, row.getHeight() + 26.0f);
    }

    // The Hz at both ends of the scale.
    g.setFont (juce::FontOptions (10.5f));
    g.setColour (theme::textDim);
    g.drawText (stf::formatHz (high), juce::Rectangle<float> (row.getX() - 52.0f, row.getY() - 7.0f, 48.0f, 14.0f), juce::Justification::centredRight);
    g.drawText (stf::formatHz (low), juce::Rectangle<float> (row.getX() - 52.0f, row.getBottom() - 7.0f, 48.0f, 14.0f), juce::Justification::centredRight);
    g.setColour (theme::grid);
    g.fillRect (row.getX(), row.getY() - 0.5f, row.getWidth(), 1.0f);
    g.fillRect (row.getX(), row.getBottom() - 0.5f, row.getWidth(), 1.0f);

    auto yOf = [&] (float percent) { return row.getBottom() - row.getHeight() * percent / 100.0f; };

    for (int i = 0; i < stf::maxSteps; ++i)
    {
        const auto cell = grid.getCell (i).reduced (2.0f, 0.0f);
        const bool used = i < s.steps;
        const float alpha = used ? 1.0f : 0.25f;
        const float height = s.heights[(size_t) i];
        const auto bar = cell.withTop (yOf (height)).withHeight (juce::jmax (3.0f, row.getBottom() - yOf (height)));

        g.setColour (colour.withAlpha (0.55f * alpha));
        g.fillRoundedRectangle (bar.withBottom (row.getBottom()), 3.0f);
        g.setColour (colour.withAlpha (0.9f * alpha));
        g.fillRect (bar.withHeight (2.0f).withBottom (bar.getY() + 2.0f));

        // The step's number and where it falls in the bar.
        g.setFont (juce::FontOptions (10.0f));
        g.setColour (theme::text.withMultipliedAlpha (used ? 0.8f : 0.3f));
        g.drawText (juce::String (i + 1), grid.getCell (i).withY (row.getBottom() + 3.0f).withHeight (13.0f), juce::Justification::centred);

        const double beats = i * sb, frac = beats - std::floor (beats);
        juce::String label;

        if (frac < 1.0e-6 || frac > 1.0 - 1.0e-6)
            label = juce::String ((int) (std::floor (beats / 4.0 + 1.0e-6)) + 1) + "." + juce::String ((int) std::floor (beats + 1.0e-6) % 4 + 1);
        else if (std::abs (frac - 0.5) < 1.0e-6)
            label = "&";

        g.setColour (theme::textDim.withMultipliedAlpha (used ? 1.0f : 0.4f));
        g.drawText (label, grid.getCell (i).withY (row.getBottom() + 16.0f).withHeight (13.0f), juce::Justification::centred);
    }

    // The cutoff's path through the bars: a slope where Glide slides, flat after.
    const double glideShare = juce::jmin (1.0, stf::getGlideMs (s, bpm) / stf::getStepMs (s.stepLength, bpm));
    juce::Path path;

    for (int i = 0; i < s.steps; ++i)
    {
        const float previous = s.heights[(size_t) ((i + s.steps - 1) % s.steps)], now = s.heights[(size_t) i];
        const float x0 = row.getX() + grid.cellWidth * (float) i, x1 = x0 + grid.cellWidth * (float) glideShare, x2 = x0 + grid.cellWidth;

        if (i == 0)
            path.startNewSubPath (x0, yOf (previous));
        else
            path.lineTo (x0, yOf (previous));

        path.lineTo (x1, yOf (now));
        path.lineTo (x2, yOf (now));
    }

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));

    for (const int step : { hovered, playingStep })
    {
        if (! juce::isPositiveAndBelow (step, stf::maxSteps))
            continue;

        const auto cell = grid.getCell (step).reduced (1.0f, 0.0f);
        g.setColour (step == playingStep ? juce::Colours::white : theme::text.withAlpha (0.7f));
        g.drawRoundedRectangle (cell.expanded (0.0f, 4.0f), 4.0f, step == playingStep ? 2.5f : 1.5f);
    }
}

juce::String getCellDescription (const stf::Settings& s, int step)
{
    if (! juce::isPositiveAndBelow (step, stf::maxSteps))
        return {};

    if (step >= s.steps)
        return "Step " + juce::String (step + 1) + " is past Steps (" + juce::String (s.steps) + "), so it isn't used. Raise Steps to use it.";

    const double sb = stf::getStepBeats (s.stepLength), beats = step * sb;
    juce::String where = (s.steps * sb > 4.0 + 1.0e-9 ? "bar " + juce::String ((int) std::floor (beats / 4.0 + 1.0e-6) + 1) + ", " : juce::String())
                         + "beat " + juce::String ((int) std::floor (beats + 1.0e-6) % 4 + 1);

    if (sb < 1.0)
        where << ", " << getOrdinal ((int) std::floor ((beats - std::floor (beats + 1.0e-6)) / sb + 1.0e-6) + 1) << " " << stepNouns[juce::jlimit (0, stf::numStepLengths - 1, s.stepLength)];

    return "Step " + juce::String (step + 1) + " of " + juce::String (s.steps) + dot() + where + dot() + stf::formatHz (getShownCutoff (s, step))
           + " (" + getHeightText (s.heights[(size_t) step]) + ")";
}

void drawFilterNow (juce::Graphics& g, juce::Rectangle<float> r, const stf::Settings& s, double cutoffHz)
{
    Plot plot;
    plot.area = r;
    plot.minDb = -48.0;
    plot.maxDb = 30.0;
    drawPlotGrid (g, plot, 12.0);

    // Low to High, shaded.
    const auto [low, high] = getRange (s);
    g.setColour (colour.withAlpha (0.12f));
    g.fillRect (juce::Rectangle<float> (plot.x (low), r.getY(), juce::jmax (1.0f, plot.x (high) - plot.x (low)), r.getHeight()));

    const auto curve = makeCurve (plot, [&] (double hz) { return getDb (s, cutoffHz, hz); });
    auto fill = curve;
    fill.lineTo (r.getRight(), r.getBottom());
    fill.lineTo (r.getX(), r.getBottom());
    fill.closeSubPath();
    g.setColour (colour.withAlpha (0.2f));
    g.fillPath (fill);
    g.setColour (colour);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    const auto at = juce::Point<float> (plot.x (cutoffHz), plot.y (getDb (s, cutoffHz, cutoffHz)));
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (at));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (stf::formatHz (cutoffHz), juce::Rectangle<float> (at.x - 40.0f, at.y - 24.0f, 80.0f, 14.0f).withX (juce::jlimit (r.getX() + 2.0f, r.getRight() - 82.0f, at.x - 40.0f)),
                juce::Justification::centred);
}

//==============================================================================
std::vector<stf::StepRecord> getLastBars (BuiltInStepFilter& processor)
{
    const int n = juce::jlimit (1, 64, juce::roundToInt (8.0 / stf::getStepBeats (processor.getSettings().stepLength)));
    std::vector<stf::StepRecord> records ((size_t) n);
    records.resize ((size_t) processor.history.getRecent (records.data(), n));
    return records;
}

void drawLastBars (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<stf::StepRecord>& records, const stf::Settings& s)
{
    const double sb = stf::getStepBeats (s.stepLength);
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
    const auto lane = r.withTrimmedBottom (r.getHeight() * 0.32f).withTrimmedTop (14.0f);
    const auto cutoffLane = r.withTop (lane.getBottom() + 6.0f).withTrimmedBottom (4.0f);
    auto toY = [&] (float peak) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, juce::Decibels::gainToDecibels (peak, -60.0f)), -60.0f, 0.0f, lane.getBottom(), lane.getY() + 2.0f); };
    auto cutoffY = [&] (double hz) { return juce::jmap ((float) std::log2 (juce::jlimit (20.0, 20000.0, hz) / 20.0), 0.0f, (float) std::log2 (1000.0), cutoffLane.getBottom(), cutoffLane.getY()); };

    for (int beat = (int) std::ceil (start - 1.0e-6); beat <= (int) std::floor (end + 1.0e-6); ++beat)
    {
        g.setColour (beat % 4 == 0 ? theme::gridBar : theme::grid);
        g.fillRect (toX (beat) - 0.5f, r.getY(), 1.0f, r.getHeight());
    }

    juce::Path cutoffPath;
    bool started = false;

    for (auto& rec : records)
    {
        if (rec.beat < start - 1.0e-6)
            continue;

        const float x0 = toX (rec.beat), x1 = toX (rec.beat + sb);
        const auto cell = juce::Rectangle<float> (x0 + 1.0f, lane.getY(), juce::jmax (1.0f, x1 - x0 - 2.0f), lane.getHeight());

        g.setColour (juce::Colour (0xff8a8f9a).withAlpha (0.45f));
        g.fillRect (cell.withTop (toY (rec.inPeak)));
        g.setColour (colour.withAlpha (0.9f));
        g.fillRect (cell.reduced (cell.getWidth() * 0.2f, 0.0f).withTop (toY (rec.outPeak)));

        const float y = cutoffY (rec.cutoffHz);

        if (! started)
        {
            cutoffPath.startNewSubPath (x0, y);
            started = true;
        }
        else
        {
            cutoffPath.lineTo (x0, y);
        }

        cutoffPath.lineTo (x1, y);
    }

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.strokePath (cutoffPath, juce::PathStrokeType (1.6f));
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (theme::textDim);
    g.drawText ("grey: in, colour: out", r.withTrimmedLeft (4.0f).withHeight (13.0f), juce::Justification::centredLeft);
    g.drawText ("the white line is each step's cutoff", cutoffLane.withTrimmedLeft (4.0f).withHeight (13.0f), juce::Justification::centredLeft);
}

//==============================================================================
namespace
{
    /** The settings the pictures use: a small example pattern, so a picture shows the idea whatever the pattern in the window. */
    stf::Settings getDemoSettings (const stf::Settings& yours)
    {
        stf::Settings s = yours;
        s.lowHz = 250.0f;
        s.highHz = 4000.0f;
        return s;
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInStepFilter& filter)
{
    BuiltInStepFilter* processor = &filter;
    std::vector<fxui::Guide::Page> pages;
    auto bpmOf = [processor] { return stf::clampBpm (processor->getUsedBpm()); };

    struct PageText { const char* name; const char* summary; const char* text; };
    static const PageText texts[] = {
        { "A pattern of tone", "Moving a filter in a rhythm", R"stf(A step filter changes how bright a sound is, step by step, in time with the song. Each step of a repeating pattern has its own filter setting: dark on one eighth note, bright on the next. A held chord that would just sit there starts to pulse and move, while it keeps playing all the time.

A filter takes frequencies away. A low-pass, the most used, lets the low ones through and turns down the high ones above its cutoff. Every pitched sound is made of harmonics: a saw wave at A2 (110 Hz) has one at 110 Hz, 220, 330, 440 and so on up. With the cutoff at 440 Hz the first three pass almost untouched, the fourth (right at the cutoff) is turned down a little and those above more and more, so the sound is dark and round; at 4 kHz dozens pass and it is bright and buzzy.

That's why a step filter changes tone much more than level. Most of a saw's power is in its lowest harmonics: keeping only the first four keeps 87% of it, just 0.6 dB less. You hear the change as darker or brighter, not quieter. DawIt Slicer's Mute steps work on level, leaving gaps.

Like a slicer, a step filter works on the sound itself and doesn't know where the notes are: each step starts on the grid, even in the middle of a note.

The picture shows the harmonics of the test chord, with each step's cutoff and the harmonics it lets through.)stf" },
        { "Cutoff and the three filters", "Low-pass, band-pass, high-pass", R"stf(The cutoff is where the filter starts working. A filter doesn't stop at its cutoff like a wall: it turns things down more and more past it, along a slope. With no resonance the level at the cutoff itself is 3 dB down, and one octave past it a 12 dB per octave filter is about 12 dB down, a 24 dB one about 25 dB.

- Low-pass keeps what's below the cutoff. Low steps sound dark and muffled, high steps bright. The usual choice for a step filter.
- High-pass keeps what's above. Low steps leave the sound whole, high steps thin it out to fizz. Good for drums and for build-ups.
- Band-pass keeps a band around the cutoff and takes away both sides: nasal and vocal. Moving it from step to step sounds like a voice changing its vowel, or a wah pedal played in rhythm.

Slope: 24 dB per octave is the stronger, more obvious movement; 12 dB is gentler and keeps more of the sound at every step. On a band-pass, 24 dB makes the band's sides steeper.

The same filter is in DawIt Subtractive: What is subtractive synthesis? > The filter shows it on a synth's harmonics.

The picture shows the three types with the cutoff at 1 kHz.)stf" },
        { "Resonance", "The peak at the cutoff", R"stf(Resonance boosts the frequencies right at the cutoff while those past it are still cut: the filter gets a peak. At 0% there is none; at 50% a low-pass at 12 dB per octave is 11.5 dB louder at the cutoff; at 100% 26 dB, and it nearly rings on its own, a whistle at the cutoff.

On a step filter, resonance is what makes the pattern sing. Each step moves the peak to a new frequency, so the harmonic nearest the cutoff stands out on every step and the pattern sounds almost like a melody. That is the squelchy, talking sound of acid house basslines: high resonance, usually 60 to 80%, and a cutoff that moves on every sixteenth.

Two cautions. Resonance makes the track louder, by up to 26 dB at the peak, so turn Output down as you turn Resonance up, or the next effect or the master can clip. And a peak low down, under about 150 Hz, booms on a bass.

On a band-pass, Resonance makes the band narrower instead: at 1 kHz and 50% it is 266 Hz wide at 12 dB per octave.

The picture shows a low-pass at 1 kHz at 0, 25, 50, 75 and 100%.)stf" },
        { "Steps, Low and High", "When each step plays, and where its cutoff is", R"stf(Step length is a note value: at 120 BPM a 1/16 step lasts 125 ms and a 1/8 step 250 ms. Steps sets how many steps the pattern has, from 1 to 16; 16 steps of 1/16 make a bar of 4/4, 8 of 1/8 too. The pattern is counted from the start of the song, so step 1 is always on the first beat of bar 1. These work exactly as in DawIt Slicer, so the two line up when you use both.

Each step's height sets its cutoff between two knobs: Low cutoff at the bottom, High cutoff at the top. The heights are spaced in octaves, the way we hear pitch: with Low at 250 Hz and High at 4 kHz (4 octaves apart), 0% is 250 Hz, 25% 500 Hz, 50% 1 kHz, 75% 2 kHz and 100% 4 kHz. Equal steps up the bars are equal musical steps.

Turning Low and High moves the whole pattern at once. Move them apart for a bigger movement, close together for a subtle one. If they are equal, every step has the same cutoff and nothing moves.

Typical ranges: basses 150 to 500 Hz at the bottom; pads and chords from about 500 Hz to 2 to 5 kHz; drums with a high-pass from about 30 Hz to 2 kHz.

The picture shows your pattern with each step's cutoff in Hz and how long each step lasts.)stf" },
        { "Glide: jump or slide", "How the cutoff gets to the next step", R"stf(Glide sets how long the cutoff takes to move to the next step's value, as a share of a step. At 0% it jumps; at 50% it slides through the first half of the step and holds; at 100% it slides the whole step, so a pattern turns into a smooth curve.

A jump is crisp and rhythmic, but a sudden change of cutoff can make a small click, most of all with high resonance. Step filters in other programs have an attack or glide control for the same reason. DawIt Step Filter always takes at least 2 ms, which keeps even 0% from clicking. With high resonance a jump can still start with a short ping at the new cutoff; a little Glide, 5 to 20%, softens the edges further.

The slide is even in octaves, as we hear it: from 250 Hz to 4 kHz it passes 1 kHz halfway through the glide.

Longer glides make the filter sound like a sweep. A pattern of 16 quarter-note steps rising from 0% to 100% with Glide 100% is a smooth four-bar rise.

The picture shows the cutoff over two steps at Glide 0, 25, 50 and 100%.)stf" },
        { "Step filter, slicer and LFO", "Three ways to make a sound move", R"stf(DawIt Slicer and DawIt Step Filter both follow a pattern on the beat, but they change different things. A Mute step on the slicer takes the sound away: silence, a trance gate. A low step on the step filter takes away only the high frequencies: the sound keeps playing, darker. A trance gate chops; a step filter pulses. They work well together: Slicer first for the gaps, the step filter after it for the tone.

An LFO (low-frequency oscillator), as on DawIt Subtractive's filter, moves the cutoff along a repeating smooth wave, such as a sine. That's a wobble or a sweep, always the same shape. A step filter can make any shape: two steps high, one low, a jump at the end of the bar. Old analogue synths had a stepped version too, sample and hold, which set the cutoff to a random value on every step.

An auto-wah follows how hard the part is played rather than a pattern. Automation draws one movement across a whole song. Neither is in DawIt yet.

The picture shows a held chord's level through Slicer's Trance gate and through Eighth-note pulse, and an LFO's sine against a step pattern.)stf" },
        { "Mix, level and clipping", "What happens around the filter", R"stf(Mix blends the filtered sound with the untouched track. 100% is the filter alone. Lower values keep some of the original under the movement, but not exactly half a filter: the filtered sound lags behind the original near the cutoff (by half a cycle at the cutoff of a 24 dB low-pass), so the two partly cancel there. A 24 dB low-pass at Mix 50% is 12 dB down right at its cutoff, deeper than at 100% (6 dB down). It sounds like a soft notch that moves with the pattern. If that's not what you want, keep Mix at 100% and move Low and High closer together for a gentler effect.

Output sets the level after the filter, from -24 to +6 dB. Lower it when you add resonance: the peak can make the track up to 26 dB louder at the cutoff.

A low-pass that goes very low on a step, say 50 Hz on a pad, leaves almost nothing; that's a gap like a slicer's Mute, with a muffled edge.

Hear only what it removes plays the part the filter takes away, so you can hear which frequencies each step removes.

The picture shows a low-pass at 1 kHz at Mix 100% and 50%, and the resonance peak against Output.)stf" },
        { "How DawIt Step Filter works", "From the song's clock to the sound", R"stf(1. It follows the song's clock. DawIt tells every effect the tempo and where the playhead is. From that, DawIt Step Filter works out which step it's in, counted from the start of the song, so step 1 is on the first beat of bar 1 wherever you start, loop or change the tempo.

2. It finds the step's cutoff from its height between Low and High.

3. It glides there over Glide's share of the step, at least 2 ms, evenly in octaves.

4. It filters the sound with the type, slope and resonance you set, each side of a stereo track on its own, both at the same cutoff. The cutoff stays below 45% of the sample rate: 19.8 kHz at 44.1 kHz.

5. It mixes the result with the untouched track by Mix, then sets the level with Output.

6. When the song is stopped, it keeps its own clock at the song's tempo, so you hear it on Play a bar and on what you play live. Tempo Own uses the BPM you set instead. As a plugin in another program, it follows that program's tempo and position when it gives them, and its own tempo when not.

7. It holds nothing back: no latency, so the track stays in time whether it's on or bypassed.

The picture follows the sound through these parts.)stf" },
        { "Step filters in a mix", "Where it works", R"stf(A step filter works best on steady, bright sounds, where there are harmonics to take away and time to hear them change: pads, held chords, saw basses, noise, drum loops. On a sine or a soft flute there is little above the first harmonic, so a low-pass has little to do. That's an opinion with a reason behind it.

- Pads and chords: an eighth-note pulse between dark and bright, the starting point Eighth-note pulse.
- Basslines and leads: high resonance and a different cutoff on every sixteenth, the acid sound (Acid steps).
- Drum loops: a high-pass on some steps thins the loop out without leaving gaps. Producers of French and filter house in the 1990s built whole tracks on disco loops under a moving low-pass filter.
- Build-ups: a high-pass rising over the bars before a drop takes the bass away, so the drop lands harder when it comes back (Build-up rise).
- Vocals: a band-pass pattern on a copy of a vocal for a talking, radio-like effect, with the main vocal left clear.

How to use it:
- Put it on one part, not the whole mix, so the rest keeps the song steady.
- DawIt can't yet switch an effect on at a point in the song. To filter one section, put that clip on a track of its own with the step filter.
- Watch the level when you add resonance; Your track says when it gets loud.

The picture shows the starting points' patterns side by side.)stf" },
        { "Your track", "What the pattern does to this track", R"stf(This topic looks at the last 2 bars of the track DawIt Step Filter is on, step by step: each step's cutoff and how loud the track came in and went out. It says whether the pattern is heard, whether resonance makes it too loud and whether a step leaves almost nothing.

Play the track, or click Play a bar, and the picture follows.)stf" },
    };

    for (auto& t : texts)
        pages.push_back ({ t.name, t.summary, t.text, nullptr });

    // 1. A pattern of tone: the chord's harmonics, each step's cutoff, and what the playing step keeps.
    pages[0].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const int playing = juce::jlimit (0, s.steps - 1, processor->playingStep.load() < 0 ? 0 : processor->playingStep.load());
        drawCaption (g, r, "The test chord's harmonics, and the cutoff of each step",
                     "step " + juce::String (playing + 1) + ": " + stf::formatHz (getShownCutoff (s, playing)) + ", " + stf::getTypeName (s.type).toLowerCase());
        Plot plot;
        plot.area = r.withTrimmedBottom (40.0f);
        plot.minHz = 80.0;
        plot.minDb = 0.0;
        plot.maxDb = 1.0;
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot.area);

        const double kept = getShownCutoff (s, playing);

        for (auto& h : getChordHarmonics())
        {
            const bool passes = getDb (s, kept, h.hz) > -6.0;
            const float height = (float) h.amplitude * (plot.area.getHeight() - 18.0f);
            g.setColour (passes ? colour : theme::textDim.withAlpha (0.4f));
            g.fillRect (plot.x (h.hz) - 0.75f, plot.area.getBottom() - 16.0f - height, 1.5f, height);
        }

        for (int i = 0; i < s.steps; ++i)
        {
            const float x = plot.x (getShownCutoff (s, i));
            g.setColour (i == playing ? juce::Colours::white : juce::Colours::white.withAlpha (0.28f));
            g.fillRect (x - (i == playing ? 1.0f : 0.5f), plot.area.getY(), i == playing ? 2.0f : 1.0f, plot.area.getHeight() - 16.0f);

            if (i == playing)
            {
                g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
                g.drawText (juce::String (i + 1), juce::Rectangle<float> (x - 12.0f, plot.area.getY() + 2.0f, 24.0f, 13.0f), juce::Justification::centred);
            }
        }

        g.setFont (juce::FontOptions (10.0f));
        g.setColour (theme::textDim);

        for (const double hz : { 100.0, 1000.0, 10000.0 })
            g.drawText (hz >= 1000.0 ? juce::String (juce::roundToInt (hz / 1000.0)) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz",
                        juce::Rectangle<float> (plot.x (hz) - 24.0f, plot.area.getBottom() - 14.0f, 48.0f, 12.0f), juce::Justification::centred);

        drawNote (g, r.withTrimmedTop (r.getHeight() - 34.0f), "Each bar is a harmonic of one of the five saws, as tall as it is loud; lit ones pass the playing step's filter (within 6 dB). "
                  "The white lines are the cutoffs of the steps: the nearer the right, the brighter the step.");
    };

    // 2. The three filters at 1 kHz.
    pages[1].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "Low-pass, band-pass and high-pass at 1 kHz", juce::String (s.steep ? 24 : 12) + " dB per octave, no resonance");
        Plot plot;
        plot.area = r.withTrimmedBottom (36.0f);
        plot.minDb = -48.0;
        plot.maxDb = 6.0;
        drawPlotGrid (g, plot, 12.0);
        const juce::Colour colours[] = { colour, juce::Colour (0xfff2994a), juce::Colour (0xff5ec2ff) };
        const char* const names[] = { "Low-pass: keeps the lows", "Band-pass: keeps a band", "High-pass: keeps the highs" };
        const filt::FilterType types[] = { filt::FilterType::lowPass, filt::FilterType::bandPass, filt::FilterType::highPass };

        for (int i = 0; i < 3; ++i)
        {
            g.setColour (colours[i]);
            g.strokePath (makeCurve (plot, [&] (double hz) { return filt::getFilterDb (types[i], s.steep, 1000.0, 0.0f, hz); }), juce::PathStrokeType (2.2f));
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (names[i], juce::Rectangle<float> (plot.area.getX() + 40.0f + 190.0f * (float) i, plot.area.getY() + 4.0f, 190.0f, 16.0f), juce::Justification::centredLeft);
        }

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillRect (plot.x (1000.0) - 0.5f, plot.area.getY(), 1.0f, plot.area.getHeight());
        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "The white line is the cutoff, 1 kHz. The low- and high-pass are 3 dB down there; the band-pass is at 0 dB. "
                  "One octave past it the " + juce::String (s.steep ? "24" : "12") + " dB slope is " + fx::formatNumber (-filt::getFilterDb (filt::FilterType::lowPass, s.steep, 1000.0, 0.0f, 2000.0), 1) + " dB down.");
    };

    // 3. Resonance: five low-passes at 1 kHz.
    pages[2].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "A low-pass at 1 kHz at five resonances", juce::String (s.steep ? 24 : 12) + " dB per octave");
        Plot plot;
        plot.area = r.withTrimmedBottom (34.0f);
        plot.minDb = -36.0;
        plot.maxDb = 30.0;
        plot.minHz = 100.0;
        plot.maxHz = 10000.0;
        drawPlotGrid (g, plot, 12.0);

        for (int i = 0; i < 5; ++i)
        {
            const float res = 25.0f * (float) i;
            const double peak = filt::getFilterDb (filt::FilterType::lowPass, s.steep, 1000.0, res, 1000.0);
            const auto c = colour.interpolatedWith (juce::Colour (0xffff7a7a), (float) i / 4.0f);
            g.setColour (c);
            g.strokePath (makeCurve (plot, [&] (double hz) { return filt::getFilterDb (filt::FilterType::lowPass, s.steep, 1000.0, res, hz); }), juce::PathStrokeType (2.0f));
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText (juce::String (juce::roundToInt (res)) + "%: " + formatDb (peak), juce::Rectangle<float> (plot.x (1000.0) + 90.0f, plot.area.getY() + 8.0f + 16.0f * (float) (4 - i), 110.0f, 14.0f), juce::Justification::centredLeft);   // a column to the right, clear of the curves
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "The level right at the cutoff, against a track with no filter. "
                  + juce::String (s.steep ? "24 dB makes every peak 3.0 dB lower than the 12 dB filter's." : "At 100% the peak is 26 dB."));
    };

    // 4. Steps, Low and High: your pattern with each step's Hz.
    pages[3].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const double bpm = bpmOf();
        drawCaption (g, r, "Your pattern, with each step's cutoff", stf::formatKnob (Knob::stepLength, s) + " at " + juce::String (juce::roundToInt (bpm)) + " BPM: " + formatMs (stf::getStepMs (s.stepLength, bpm)) + " a step");
        const auto grid = layoutPattern (r.withTrimmedBottom (30.0f));
        drawPattern (g, grid, s, bpm, processor->playingStep.load(), -1);
        g.setFont (juce::FontOptions (9.5f));
        g.setColour (theme::text);

        for (int i = 0; i < s.steps; ++i)
        {
            const float y = grid.cells.getBottom() - grid.cells.getHeight() * s.heights[(size_t) i] / 100.0f;
            g.drawText (stf::formatHz (getShownCutoff (s, i)), grid.getCell (i).withY (y - 15.0f).withHeight (13.0f).expanded (6.0f, 0.0f), juce::Justification::centred);
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "Bars are as high as their cutoff, between " + stf::formatHz (juce::jmin (s.lowHz, s.highHz)) + " and "
                  + stf::formatHz (juce::jmax (s.lowHz, s.highHz)) + ", " + formatShort (std::round (getOctaves (s) * 10.0) / 10.0) + " octaves apart.");
    };

    // 5. Glide: the cutoff over two steps at four glides.
    pages[4].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        auto s = getDemoSettings (processor->getSettings());
        const double bpm = bpmOf(), stepMs = stf::getStepMs (s.stepLength, bpm);
        drawCaption (g, r, "The cutoff over two steps: 250 Hz, then 4 kHz", stf::formatKnob (Knob::stepLength, s) + " at " + juce::String (juce::roundToInt (bpm)) + " BPM: " + formatMs (stepMs) + " a step");
        const float cellHeight = (r.getHeight() - 8.0f) / 2.0f, cellWidth = (r.getWidth() - 10.0f) / 2.0f;

        for (int i = 0; i < 4; ++i)
        {
            const float glide = i == 0 ? 0.0f : i == 1 ? 25.0f : i == 2 ? 50.0f : 100.0f;
            s.glidePercent = glide;
            const double glideMs = stf::getGlideMs (s, bpm);
            const auto cell = juce::Rectangle<float> (r.getX() + (cellWidth + 10.0f) * (float) (i % 2), r.getY() + (cellHeight + 8.0f) * (float) (i / 2), cellWidth, cellHeight);
            const auto plot = cell.withTrimmedLeft (44.0f).withTrimmedBottom (14.0f).withTrimmedTop (14.0f);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (cell);
            auto toX = [&] (double ms) { return plot.getX() + plot.getWidth() * (float) (ms / (2.0 * stepMs)); };
            auto toY = [&] (double octaves) { return plot.getBottom() - plot.getHeight() * (float) (octaves / 4.0); };

            g.setFont (juce::FontOptions (10.0f));

            for (int o = 0; o <= 4; ++o)
            {
                g.setColour (o == 2 ? theme::gridBar : theme::grid);
                g.fillRect (plot.getX(), toY (o) - 0.5f, plot.getWidth(), 1.0f);
                g.setColour (theme::textDim);
                g.drawText (stf::formatHz (250.0 * std::pow (2.0, o)), juce::Rectangle<float> (cell.getX() + 2.0f, toY (o) - 7.0f, 40.0f, 13.0f), juce::Justification::centredRight);
            }

            g.setColour (theme::gridBar);
            g.fillRect (toX (stepMs) - 0.5f, plot.getY(), 1.0f, plot.getHeight());

            juce::Path p;
            p.startNewSubPath (plot.getX(), toY (0.0));
            p.lineTo (toX (stepMs), toY (0.0));
            p.lineTo (toX (stepMs + glideMs), toY (4.0));
            p.lineTo (plot.getRight(), toY (4.0));
            g.setColour (colour);
            g.strokePath (p, juce::PathStrokeType (2.2f));
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText ("Glide " + juce::String (juce::roundToInt (glide)) + "%: " + formatMs (glideMs) + (glide < 1.0f ? " (the least)" : ""), cell.withHeight (14.0f).reduced (6.0f, 0.0f), juce::Justification::centredRight);
            g.setFont (juce::FontOptions (10.0f));
            g.setColour (theme::textDim);
            g.drawText ("step 1", plot.withY (plot.getBottom() + 1.0f).withHeight (13.0f).withWidth (plot.getWidth() / 2.0f), juce::Justification::centred);
            g.drawText ("step 2", plot.withY (plot.getBottom() + 1.0f).withHeight (13.0f).withTrimmedLeft (plot.getWidth() / 2.0f), juce::Justification::centred);
        }
    };

    // 6. Step filter, slicer and LFO.
    pages[5].paint = [bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const double bpm = bpmOf();
        drawCaption (g, r, "Three ways to make a held chord move", "one bar at " + juce::String (juce::roundToInt (bpm)) + " BPM");
        const float laneHeight = (r.getHeight() - 24.0f) / 3.0f;
        const stf::Settings pulse = [&]
        {
            for (auto& p : stf::getPresets())
                if (p.name == "Eighth-note pulse")
                    return p.settings;

            return stf::Settings();
        }();

        for (int lane = 0; lane < 3; ++lane)
        {
            const auto area = juce::Rectangle<float> (r.getX(), r.getY() + (laneHeight + 12.0f) * (float) lane, r.getWidth(), laneHeight);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (area);

            for (int i = 0; i <= 8; ++i)
            {
                g.setColour (i % 2 == 0 ? theme::gridBar : theme::grid);
                g.fillRect (area.getX() + area.getWidth() * (float) i / 8.0f - 0.5f, area.getY(), 1.0f, area.getHeight());
            }

            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (lane == 0 ? "DawIt Slicer, Trance gate: the level (every other 1/16 step is Mute)"
                                  : lane == 1 ? "DawIt Step Filter, Eighth-note pulse: the level of the chord (grey) and the cutoff (white)"
                                              : "An LFO's sine (grey) against a step pattern (white): the cutoff",
                        area.reduced (6.0f, 2.0f), juce::Justification::topLeft);
            const auto plot = area.withTrimmedTop (18.0f).reduced (0.0f, 4.0f);

            if (lane == 0)
            {
                g.setColour (juce::Colour (0xfff2994a).withAlpha (0.85f));

                for (int i = 0; i < 16; i += 2)
                    g.fillRect (plot.getX() + plot.getWidth() * (float) i / 16.0f, plot.getY(), plot.getWidth() / 16.0f - 1.0f, plot.getHeight());
            }
            else if (lane == 1)
            {
                const double base = getChordLevelDb (pulse, 20000.0);

                for (int i = 0; i < 8; ++i)
                {
                    const double cutoff = getShownCutoff (pulse, i);
                    const double db = juce::jmax (-12.0, getChordLevelDb (pulse, cutoff) - base);
                    const float h = plot.getHeight() * (float) (1.0 + db / 12.0) * 0.95f;
                    const auto cell = juce::Rectangle<float> (plot.getX() + plot.getWidth() * (float) i / 8.0f, plot.getBottom() - h, plot.getWidth() / 8.0f - 1.0f, h);
                    g.setColour (theme::textDim.withAlpha (0.5f));
                    g.fillRect (cell);
                    const float y = plot.getBottom() - plot.getHeight() * (float) (std::log2 (cutoff / 400.0) / std::log2 (4000.0 / 400.0)) * 0.9f - 2.0f;
                    g.setColour (juce::Colours::white);
                    g.fillRect (cell.withTop (y).withHeight (2.5f));
                }
            }
            else
            {
                juce::Path sine;

                for (int x = 0; x <= (int) plot.getWidth(); ++x)
                {
                    const float v = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) x / plot.getWidth() * 2.0f);
                    const juce::Point<float> pt { plot.getX() + (float) x, plot.getBottom() - 3.0f - v * (plot.getHeight() - 6.0f) };
                    x == 0 ? sine.startNewSubPath (pt) : sine.lineTo (pt);
                }

                g.setColour (theme::textDim);
                g.strokePath (sine, juce::PathStrokeType (1.8f));
                const float levels[] = { 0.0f, 1.0f, 0.2f, 0.7f, 0.0f, 1.0f, 0.2f, 0.7f };
                juce::Path steps;

                for (int i = 0; i < 8; ++i)
                {
                    const float y = plot.getBottom() - 3.0f - levels[i] * (plot.getHeight() - 6.0f);
                    i == 0 ? steps.startNewSubPath (plot.getX(), y) : steps.lineTo (plot.getX() + plot.getWidth() * (float) i / 8.0f, y);
                    steps.lineTo (plot.getX() + plot.getWidth() * (float) (i + 1) / 8.0f, y);
                }

                g.setColour (juce::Colours::white);
                g.strokePath (steps, juce::PathStrokeType (2.0f));
            }
        }
    };

    // 7. Mix, level and clipping.
    pages[6].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "What Mix and Output do", "left: a 24 dB low-pass at 1 kHz; right: the peak at the cutoff");
        auto left = r.removeFromLeft (r.getWidth() * 0.56f);
        r.removeFromLeft (12.0f);
        Plot plot;
        plot.area = left.withTrimmedBottom (34.0f);
        plot.minDb = -36.0;
        plot.maxDb = 6.0;
        plot.minHz = 100.0;
        plot.maxHz = 10000.0;
        drawPlotGrid (g, plot, 12.0);

        for (const double mix : { 1.0, 0.5 })
        {
            auto db = [mix] (double hz)
            {
                const auto h = filt::getFilterResponse (filt::FilterType::lowPass, true, 1000.0, 0.0f, hz);
                return 20.0 * std::log10 (juce::jmax (1.0e-10, std::abs ((1.0 - mix) + mix * h)));
            };

            g.setColour (mix > 0.99 ? colour : juce::Colour (0xfff2994a));
            g.strokePath (makeCurve (plot, db), juce::PathStrokeType (2.0f));
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.drawText ("Mix " + juce::String (juce::roundToInt (mix * 100.0)) + "%: " + formatDb (db (1000.0)), juce::Rectangle<float> (plot.x (1000.0) + 8.0f, plot.y (db (1000.0)) + (mix > 0.99 ? -16.0f : 4.0f), 130.0f, 14.0f), juce::Justification::centredLeft);
        }

        drawNote (g, left.withTrimmedTop (left.getHeight() - 30.0f), "At 50% the two sounds partly cancel at the cutoff: a dip, deeper than at 100%.");

        // The peak at the cutoff against resonance, and after Output.
        auto right = r.withTrimmedBottom (34.0f);
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (right);
        const float zero = right.getY() + right.getHeight() * 0.72f, perDb = right.getHeight() * 0.7f / 30.0f;
        g.setColour (theme::gridBar);
        g.fillRect (right.getX(), zero - 0.5f, right.getWidth(), 1.0f);
        g.setFont (juce::FontOptions (10.5f));

        for (int i = 0; i < 5; ++i)
        {
            const float res = 25.0f * (float) i;
            const double peak = filt::getFilterDb (filt::FilterType::lowPass, s.steep, 1000.0, res, 1000.0);
            const float x = right.getX() + right.getWidth() * ((float) i + 0.5f) / 5.0f;
            const float h = (float) peak * perDb;
            g.setColour (colour.interpolatedWith (juce::Colour (0xffff7a7a), (float) i / 4.0f).withAlpha (0.85f));
            g.fillRect (juce::Rectangle<float> (x - 16.0f, juce::jmin (zero, zero - h), 14.0f, std::abs (h)));
            const double after = peak + s.outputDb;
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.drawRect (juce::Rectangle<float> (x + 2.0f, juce::jmin (zero, zero - (float) after * perDb), 14.0f, juce::jmax (1.0f, std::abs ((float) after * perDb))), 1.0f);
            g.setColour (theme::text);
            g.drawText (formatDb (peak, 0), juce::Rectangle<float> (x - 30.0f, zero - h - (h >= 0.0f ? 15.0f : -2.0f), 60.0f, 13.0f), juce::Justification::centred);
            g.setColour (theme::textDim);
            g.drawText (juce::String (juce::roundToInt (res)) + "%", juce::Rectangle<float> (x - 20.0f, right.getBottom() - 14.0f, 40.0f, 13.0f), juce::Justification::centred);
        }

        g.setColour (theme::textDim);
        g.drawText ("Resonance", right.withHeight (14.0f).reduced (4.0f, 0.0f), juce::Justification::topLeft);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "Colour: the filter's peak. Outline: after Output (" + stf::formatKnob (Knob::output, s) + ").");
    };

    // 8. How it works: a block diagram.
    pages[7].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "From the song's clock to the sound", "the cutoff, then the filter");
        const float col = (r.getWidth() - 3.0f * 26.0f) / 4.0f, topY = r.getY() + 16.0f, bottomY = r.getCentreY() + 28.0f, h = 58.0f;
        auto box = [&] (int column, float y) { return juce::Rectangle<float> (r.getX() + (col + 26.0f) * (float) column, y, col, h); };
        const auto clock = drawBox (g, box (0, topY), "The song's clock", "tempo and position", theme::accent);
        const auto height = drawBox (g, box (1, topY), "Step height", "Low to High, in octaves", colour);
        const auto glide = drawBox (g, box (2, topY), "Glide", "evenly in octaves, 2 ms at least", colour);
        const auto input = drawBox (g, box (0, bottomY), "Track in", "every block", theme::textDim);
        const auto filterBox = drawBox (g, box (1, bottomY), "Filter", "type, slope, resonance", colour);
        const auto mix = drawBox (g, box (2, bottomY), "Mix", "with the dry track", theme::accent);
        const auto output = drawBox (g, box (3, bottomY), "Output", "the level, then out", theme::accent);

        arrow (g, { clock.getRight(), clock.getCentreY() }, { height.getX(), height.getCentreY() });
        arrow (g, { height.getRight(), height.getCentreY() }, { glide.getX(), glide.getCentreY() });

        // The cutoff comes down from the glide into the filter.
        juce::Path cutoff;
        cutoff.startNewSubPath (glide.getRight(), glide.getCentreY());
        cutoff.lineTo (glide.getRight() + 13.0f, glide.getCentreY());
        cutoff.lineTo (glide.getRight() + 13.0f, (topY + bottomY + h) * 0.5f - 6.0f);
        cutoff.lineTo (filterBox.getCentreX(), (topY + bottomY + h) * 0.5f - 6.0f);
        g.setColour (theme::textDim);
        g.strokePath (cutoff, juce::PathStrokeType (1.2f));
        arrow (g, { filterBox.getCentreX(), (topY + bottomY + h) * 0.5f - 6.0f }, { filterBox.getCentreX(), filterBox.getY() });
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("the cutoff", juce::Rectangle<float> (filterBox.getCentreX() + 8.0f, (topY + bottomY + h) * 0.5f - 22.0f, 90.0f, 14.0f), juce::Justification::centredLeft);

        arrow (g, { input.getRight(), input.getCentreY() }, { filterBox.getX(), filterBox.getCentreY() });
        arrow (g, { filterBox.getRight(), filterBox.getCentreY() }, { mix.getX(), mix.getCentreY() });
        arrow (g, { mix.getRight(), mix.getCentreY() }, { output.getX(), output.getCentreY() });

        // The dry track around the filter, to the mix.
        juce::Path dry;
        const float under = bottomY + h + 22.0f;
        dry.startNewSubPath (input.getCentreX(), input.getBottom());
        dry.lineTo (input.getCentreX(), under);
        dry.lineTo (mix.getCentreX(), under);
        g.setColour (theme::textDim);
        g.strokePath (dry, juce::PathStrokeType (1.2f));
        g.drawArrow ({ mix.getCentreX(), under, mix.getCentreX(), mix.getBottom() }, 1.2f, 7.0f, 7.0f);
        g.drawText ("the dry track", juce::Rectangle<float> (mix.getCentreX() - input.getCentreX(), 16.0f).withPosition (input.getCentreX(), under + 3.0f), juce::Justification::centred);
    };

    // 9. Step filters in a mix: the starting points' patterns.
    pages[8].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "The starting points' patterns", "height of each step, 1 to 16 (or 8)");
        const auto& list = stf::getPresets();
        const float cellWidth = (r.getWidth() - 20.0f) / 3.0f, cellHeight = (r.getHeight() - 12.0f) / 2.0f;

        for (size_t k = 0; k < list.size() && k < 6; ++k)
        {
            const auto& p = list[k];
            const auto cell = juce::Rectangle<float> (r.getX() + (cellWidth + 10.0f) * (float) (k % 3), r.getY() + (cellHeight + 12.0f) * (float) (k / 3), cellWidth, cellHeight);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (cell);
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (p.name, cell.withHeight (16.0f).reduced (6.0f, 0.0f), juce::Justification::centredLeft, true);
            g.setFont (juce::FontOptions (10.0f));
            g.setColour (theme::textDim);
            g.drawText (stf::getTypeName (p.settings.type) + " " + stf::formatKnob (Knob::slope, p.settings) + ", " + stf::formatHz (juce::jmin (p.settings.lowHz, p.settings.highHz)) + " to "
                            + stf::formatHz (juce::jmax (p.settings.lowHz, p.settings.highHz)),
                        cell.withTrimmedTop (16.0f).withHeight (13.0f).reduced (6.0f, 0.0f), juce::Justification::centredLeft, true);
            const auto plot = cell.withTrimmedTop (34.0f).reduced (6.0f, 4.0f);
            const float step = plot.getWidth() / (float) p.settings.steps;

            for (int i = 0; i < p.settings.steps; ++i)
            {
                const float height = plot.getHeight() * p.settings.heights[(size_t) i] / 100.0f;
                g.setColour (colour.withAlpha (0.8f));
                g.fillRect (juce::Rectangle<float> (plot.getX() + step * (float) i + 1.0f, plot.getBottom() - juce::jmax (2.0f, height), step - 2.0f, juce::jmax (2.0f, height)));
            }
        }
    };

    // 10. Your track: the last 2 bars, and the advice.
    pages[9].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto host = fxui::getHostTrack (*processor);
        const auto s = processor->getSettings();
        drawCaption (g, r, (host.name.isEmpty() ? "Stand-in step filter" : host.name) + ": the last 2 bars", stf::formatKnob (Knob::stepLength, s) + ", " + juce::String (s.steps) + " steps");
        drawLastBars (g, r, getLastBars (*processor), s);
    };
    const auto introduction = pages[9].text;
    pages[9].liveText = [processor, introduction]
    {
        stf::AnalysisInput input;
        input.settings = processor->getSettings();
        input.records = getLastBars (*processor);
        input.bpm = processor->getUsedBpm();
        input.hostHasTempo = processor->tempoBpm.load() > 0.0;
        juce::StringArray lines;

        for (auto& line : stf::analyse (input))
            lines.add (line);

        return introduction + "\n\n" + lines.joinIntoString ("\n\n");
    };

    return pages;
}
} // namespace daw::stftext
