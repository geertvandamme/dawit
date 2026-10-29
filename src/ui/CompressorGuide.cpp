#include "CompressorGuide.h"

namespace daw::comptext
{
namespace
{
    constexpr double levelRate = 1000.0;   // the made-up levels: one per millisecond
    const juce::Colour inColour { 0xff8a8f9a }, outColour { theme::accent }, reductionColour { theme::meterRed };

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

    /** "6", "2.5": whole numbers without decimals. (juce::String with 0 decimals would print every digit.) */
    juce::String formatNumber (double value)
    {
        return std::abs (value - std::round (value)) < 0.05 ? juce::String (juce::roundToInt (value)) : juce::String (value, 1);
    }

    juce::String formatDb (double db, int decimals = 1)
    {
        return (decimals == 0 ? juce::String (juce::roundToInt (db)) : juce::String (db, decimals)) + " dB";
    }

    /** Time left to right, level up, in dB. */
    struct TimePlot
    {
        juce::Rectangle<float> area;
        double seconds = 4.0, lowDb = -48.0, highDb = 0.0;

        float x (double t) const { return area.getX() + area.getWidth() * (float) juce::jlimit (0.0, 1.0, t / seconds); }
        float y (double db) const { return juce::jmap ((float) juce::jlimit (lowDb, highDb, db), (float) lowDb, (float) highDb, area.getBottom(), area.getY()); }
    };

    TimePlot drawTimeGrid (juce::Graphics& g, juce::Rectangle<float> r, double seconds, double lowDb, double highDb, double tick)
    {
        TimePlot p { r.withTrimmedLeft (34.0f).withTrimmedBottom (16.0f), seconds, lowDb, highDb };
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (p.area);
        g.setFont (juce::FontOptions (10.5f));

        const int dbStep = highDb - lowDb > 24.0 ? 12 : 3;

        for (int db = (int) highDb; db >= (int) lowDb; db -= dbStep)
        {
            g.setColour (theme::grid);
            g.fillRect (p.area.getX(), p.y (db), p.area.getWidth(), 1.0f);
            g.setColour (theme::textDim);
            g.drawText (juce::String (db), juce::Rectangle<float> (r.getX(), p.y (db) - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
        }

        for (double t = 0.0; t <= seconds + 1.0e-6; t += tick)
        {
            g.setColour (theme::grid);
            g.fillRect (p.x (t), p.area.getY(), 1.0f, p.area.getHeight());
            g.setColour (theme::textDim);
            g.drawText (t < 1.0 && tick < 1.0 ? juce::String (juce::roundToInt (t * 1000.0)) + " ms" : juce::String (t, tick < 1.0 ? 1 : 0) + " s",
                        juce::Rectangle<float> (p.x (t) - 30.0f, p.area.getBottom() + 2.0f, 60.0f, 13.0f), juce::Justification::centred);
        }

        return p;
    }

    void drawLevels (juce::Graphics& g, const TimePlot& p, const std::vector<float>& levels, juce::Colour c, float thickness, bool fill = false,
                     double offsetDb = 0.0)
    {
        juce::Path path;
        const int count = juce::jmin ((int) levels.size(), (int) (p.seconds * levelRate));

        for (int i = 0; i < count; i += 2)
        {
            const auto pt = juce::Point<float> (p.x (i / levelRate), p.y (levels[(size_t) i] + offsetDb));

            if (i == 0)
                path.startNewSubPath (pt);
            else
                path.lineTo (pt);
        }

        if (fill && count > 0)
        {
            juce::Path area (path);
            area.lineTo (p.x ((count - 1) / levelRate), p.area.getBottom());
            area.lineTo (p.area.getX(), p.area.getBottom());
            area.closeSubPath();
            g.setColour (c.withAlpha (0.2f));
            g.fillPath (area);
        }

        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (thickness));
    }

    void drawThreshold (juce::Graphics& g, const TimePlot& p, double thresholdDb)
    {
        juce::Path line, dashed;
        line.startNewSubPath (p.area.getX(), p.y (thresholdDb));
        line.lineTo (p.area.getRight(), p.y (thresholdDb));
        const float dashes[] { 5.0f, 4.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, line, dashes, 2);
        g.setColour (curveColour);
        g.fillPath (dashed);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("threshold " + formatDb (thresholdDb), juce::Rectangle<float> (p.area.getRight() - 130.0f, p.y (thresholdDb) - 15.0f, 126.0f, 13.0f),
                    juce::Justification::centredRight);
    }

    void drawLegend (juce::Graphics& g, juce::Rectangle<float> r, std::initializer_list<std::pair<juce::String, juce::Colour>> items)
    {
        g.setFont (juce::FontOptions (11.5f));
        float x = r.getX() + 40.0f;

        for (auto& [name, c] : items)
        {
            g.setColour (c);
            g.fillRect (x, r.getY() + 5.0f, 14.0f, 3.0f);
            const float w = juce::GlyphArrangement::getStringWidth (juce::Font (juce::FontOptions (11.5f)), name) + 10.0f;
            g.drawText (name, juce::Rectangle<float> (x + 18.0f, r.getY(), w, 13.0f), juce::Justification::centredLeft);
            x += w + 30.0f;
        }
    }

    /** The static curve: what comes out for what goes in, from -60 to 0 dB. */
    struct Curve
    {
        comp::Settings settings;
        juce::Colour colour;
        juce::String label;
    };

    void drawCurve (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<Curve>& curves, double lowDb, double highDb)
    {
        auto area = r.withTrimmedLeft (34.0f).withTrimmedBottom (16.0f);
        area = area.withSizeKeepingCentre (juce::jmin (area.getWidth(), area.getHeight()), juce::jmin (area.getWidth(), area.getHeight()));
        const auto x = [&] (double db) { return juce::jmap ((float) db, (float) lowDb, (float) highDb, area.getX(), area.getRight()); };
        const auto y = [&] (double db) { return juce::jmap ((float) juce::jlimit (lowDb, highDb, db), (float) lowDb, (float) highDb, area.getBottom(), area.getY()); };

        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (area);
        g.setFont (juce::FontOptions (10.5f));
        const double step = highDb - lowDb > 30.0 ? 12.0 : 3.0;

        for (double db = highDb; db >= lowDb - 1.0e-6; db -= step)
        {
            g.setColour (theme::grid);
            g.fillRect (area.getX(), y (db), area.getWidth(), 1.0f);
            g.fillRect (x (db), area.getY(), 1.0f, area.getHeight());
            g.setColour (theme::textDim);
            g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (area.getX() - 34.0f, y (db) - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
            g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (x (db) - 20.0f, area.getBottom() + 2.0f, 40.0f, 13.0f), juce::Justification::centred);
        }

        g.setColour (theme::textDim.withAlpha (0.6f));
        g.drawLine (x (lowDb), y (lowDb), x (highDb), y (highDb), 1.0f);

        for (auto& curve : curves)
        {
            juce::Path path;

            for (int i = 0; i <= 200; ++i)
            {
                const double in = lowDb + (highDb - lowDb) * i / 200.0;
                const auto pt = juce::Point<float> (x (in), y (in - comp::getReductionDb (curve.settings, in)));

                if (i == 0)
                    path.startNewSubPath (pt);
                else
                    path.lineTo (pt);
            }

            g.setColour (curve.colour);
            g.strokePath (path, juce::PathStrokeType (2.2f));
            g.setFont (juce::FontOptions (11.5f));
            const double end = highDb - comp::getReductionDb (curve.settings, highDb);
            g.drawText (curve.label, juce::Rectangle<float> (area.getRight() + 6.0f, y (end) - 7.0f, 120.0f, 14.0f), juce::Justification::centredLeft);
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("in (dB)", juce::Rectangle<float> (area.getRight() + 6.0f, area.getBottom() - 14.0f, 60.0f, 14.0f), juce::Justification::centredLeft);
        g.drawText ("out", juce::Rectangle<float> (area.getX() + 4.0f, area.getY() + 2.0f, 40.0f, 14.0f), juce::Justification::centredLeft);
    }

    /** One hit for the timing pictures: 2 seconds of a steady loud note that starts at 0.1 s and stops at 1.1 s. */
    std::vector<float> makeNoteLevels()
    {
        std::vector<float> levels (2000);

        for (size_t i = 0; i < levels.size(); ++i)
            levels[i] = i >= 100 && i < 1100 ? -6.0f : -60.0f;

        return levels;
    }

    std::vector<float> getReductions (const comp::Settings& s, const std::vector<float>& levels)
    {
        return comp::simulateReduction (s, levels, levelRate);
    }

    /** How far apart the loudest and the quietest syllable of the vocal line peak in `levels` (each a quarter of a
        second), leaving out the rests of the vocal line `in`: how even the words are. */
    double getSpreadDb (const std::vector<float>& levels, const std::vector<float>& in)
    {
        float loudest = -200.0f, quietest = 200.0f;

        for (size_t start = 0; start + 250 <= juce::jmin (levels.size(), in.size()); start += 250)
        {
            const auto from = (long) start, to = (long) start + 250;

            if (*std::max_element (in.begin() + from, in.begin() + to) < -50.0f)
                continue;   // a rest

            const float peak = *std::max_element (levels.begin() + from, levels.begin() + to);
            loudest = juce::jmax (loudest, peak);
            quietest = juce::jmin (quietest, peak);
        }

        return loudest > quietest ? loudest - quietest : 0.0;
    }

    void drawBox (juce::Graphics& g, juce::Rectangle<float> box, const juce::String& name, const juce::String& detail, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.18f));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (c);
        g.drawRoundedRectangle (box, 5.0f, 1.2f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawFittedText (name, box.reduced (4.0f).removeFromTop (box.getHeight() * 0.45f).toNearestInt(), juce::Justification::centredBottom, 1, 0.8f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText (detail, box.reduced (4.0f).withTrimmedTop (box.getHeight() * 0.48f).toNearestInt(), juce::Justification::centredTop, 2, 0.8f);
    }

    void drawArrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        juce::Path arrow;
        arrow.addArrow ({ from, to }, 1.5f, 8.0f, 7.0f);
        g.setColour (theme::textDim);
        g.fillPath (arrow);
    }
} // namespace

std::vector<float> makeVocalLevels()
{
    // Syllables of a quarter of a second each, at the levels of a singer who isn't quite even.
    const float syllables[] { -14, -8, -18, -6, -12, -60, -9, -16, -4, -13, -22, -10, -60, -7, -15, -11 };
    std::vector<float> levels (4000, -60.0f);

    for (size_t s = 0; s < std::size (syllables); ++s)
    {
        for (int i = 0; i < 250; ++i)
        {
            const float base = syllables[s];
            float level = -60.0f;

            if (i < 20)
                level = juce::jmap ((float) i, 0.0f, 20.0f, -40.0f, base);
            else if (i < 190)
                level = base - 3.0f * (float) (i - 20) / 170.0f;
            else
                level = juce::jmap ((float) i, 190.0f, 250.0f, base - 3.0f, -45.0f);

            levels[s * 250 + (size_t) i] = juce::jmax (-60.0f, base <= -60.0f ? -60.0f : level);
        }
    }

    return levels;
}

std::vector<float> makeDrumLevels (double seconds)
{
    std::vector<float> levels ((size_t) (seconds * levelRate));

    for (size_t i = 0; i < levels.size(); ++i)
    {
        const int sinceHit = (int) (i % 500);
        levels[i] = juce::jmax (-60.0f, -3.0f - 0.4f * (float) sinceHit);
    }

    return levels;
}

std::vector<float> getOutputLevels (const comp::Settings& s, const std::vector<float>& levelsDb)
{
    const auto reductions = getReductions (s, levelsDb);
    const double wet = s.mixPercent / 100.0, makeup = juce::Decibels::decibelsToGain ((double) s.makeupDb);
    std::vector<float> out (levelsDb.size());

    for (size_t i = 0; i < levelsDb.size(); ++i)
    {
        const double dry = juce::Decibels::decibelsToGain ((double) levelsDb[i], -200.0);
        const double compressed = dry * juce::Decibels::decibelsToGain (-(double) reductions[i], -200.0) * makeup;
        out[i] = (float) juce::Decibels::gainToDecibels ((1.0 - wet) * dry + wet * compressed, -100.0);
    }

    return out;
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInCompressor& compressor)
{
    BuiltInCompressor* processor = &compressor;
    std::vector<fxui::Guide::Page> pages;

    // Numbers for the text, from the compressor's own curve.
    comp::Settings hard;
    hard.thresholdDb = -18.0f;
    hard.ratio = 4.0f;
    hard.kneeDb = 0.0f;
    comp::Settings soft = hard;
    soft.kneeDb = 6.0f;
    const double loudOut = -6.0 - comp::getReductionDb (hard, -6.0), quietOut = -20.0 - comp::getReductionDb (hard, -20.0);
    const double makeup = -6.0 - loudOut;

    pages.push_back ({ "Turning down the loud parts", "What a compressor does",
        "A compressor turns a sound down while it's loud, and leaves it alone while it's quiet. It's like an engineer with a "
        "hand on the fader who pulls it down whenever the singer gets loud, only much faster and always the same.\n\n"
        "Why: a singer is louder on some words than others, a bass player hits some notes harder, a snare varies from hit to "
        "hit. In a mix, the quiet parts get lost and the loud ones jump out. A compressor makes the level more even, so the "
        "track can sit at one place in the mix and stay there. Turned up afterwards (makeup), the quiet details come forward: "
        "breaths, the tail of a note, the room around a drum.\n\n"
        "Compressors were first built for radio and records in the 1930s, to keep the level within what a transmitter or a "
        "record groove could take. The famous studio ones came later: the Teletronix LA-2A of the early 1960s, which turns down "
        "with a light shining on a photocell and is still loved on vocals and bass, and the Universal Audio 1176 of 1967, with "
        "transistors and very fast timing, a classic on vocals and drums.\n\n"
        "The picture shows a made-up vocal line through the compressor with the settings in its window: in grey what comes in, "
        "in blue what comes out, and in red how much it turned down. The loud syllables come down, the quiet ones stay.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto in = makeVocalLevels();
            drawCaption (g, r, "A made-up vocal line, through the settings in the window", "4 seconds");
            drawLegend (g, r.removeFromTop (14.0f), { { "in", inColour }, { "out", outColour }, { "turned down (dB under the top line)", reductionColour } });
            r.removeFromTop (4.0f);
            const auto p = drawTimeGrid (g, r, 4.0, -48.0, 0.0, 1.0);
            const auto reductions = getReductions (s, in);
            std::vector<float> reductionLine (reductions.size());

            for (size_t i = 0; i < reductions.size(); ++i)
                reductionLine[i] = -reductions[i];

            drawLevels (g, p, in, inColour, 1.4f, true);
            drawLevels (g, p, getOutputLevels (s, in), outColour, 2.0f);
            drawLevels (g, p, reductionLine, reductionColour, 1.6f);
            drawThreshold (g, p, s.thresholdDb);
        } });

    pages.push_back ({ "Threshold and ratio", "When, and how much",
        "Two knobs decide what a compressor does with a level.\n\n"
        "The threshold is where it starts: anything louder than the threshold gets turned down, anything quieter passes "
        "unchanged. Lower the threshold, and more of the sound is compressed.\n\n"
        "The ratio is how much: how many dB that go in over the threshold give one dB out over it. At 4:1, a level 8 dB over the "
        "threshold comes out " + formatDb (8.0 - comp::getReductionDb (hard, hard.thresholdDb + 8.0), 0) + " over it: "
        "the compressor turned it down by " + formatDb (comp::getReductionDb (hard, hard.thresholdDb + 8.0), 0) + ". At "
        "2:1 the same level comes out 4 dB over; at 20:1 almost nothing gets over, which is called limiting. 1:1 does nothing.\n\n"
        "The picture shows that as a curve: what goes in along the bottom, what comes out up the side. Under the threshold the "
        "curve follows the diagonal (nothing changes); above it, it bends flatter, the flatter the higher the ratio. The orange "
        "curve is the window's.\n\n"
        "How to set them: the threshold depends on how loud the track is, so set it by ear and by the meter, not by a number. "
        "Lower it until the loudest parts are turned down by the amount you want (3-6 dB is common on a vocal). Gentle ratios "
        "(1.5:1 to 2:1) glue a mix or a piano; 3:1 to 4:1 is the usual for vocals, bass and drums; 8:1 and up holds a part "
        "firmly in place.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "What comes out for what goes in", "the window: " + formatKnob (comp::Knob::threshold, s) + ", " + formatKnob (comp::Knob::ratio, s));
            auto t2 = s, t8 = s;
            t2.ratio = 2.0f;
            t8.ratio = 8.0f;
            drawCurve (g, r, { { t2, theme::textDim, "2:1" }, { t8, theme::textDim, "8:1" }, { s, curveColour, "the window, " + formatKnob (comp::Knob::ratio, s) } },
                       -60.0, 0.0);
        } });

    pages.push_back ({ "The knee", "A sharp or a gentle start",
        "With a hard knee the compressor does nothing below the threshold and the full ratio right above it: a sharp corner in "
        "the curve. With a soft knee the ratio fades in over a range around the threshold, so the compression starts gently and "
        "it's harder to hear where it begins.\n\n"
        "The knee is measured in dB: a 6 dB knee fades in from 3 dB under the threshold to 3 dB over it. Right at the threshold, "
        "a 6 dB knee at 4:1 already turns down " + juce::String (comp::getReductionDb (soft, soft.thresholdDb), 1) + " dB, where a "
        "hard knee turns down nothing.\n\n"
        "A hard knee (0-2 dB) suits drums and limiting, where you want the compressor to grab. A soft knee (6-12 dB) suits "
        "vocals, bass, piano, pads and buses: wherever the compression should go unnoticed.\n\n"
        "The picture zooms in on the corner: a hard knee, 6 dB and 12 dB, with the window's ratio, and the window's own knee in "
        "orange.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The corner of the curve, 12 dB around the threshold", "the window: " + formatKnob (comp::Knob::knee, s) + " knee");
            auto k0 = s, k6 = s, k12 = s;
            k0.kneeDb = 0.0f;
            k6.kneeDb = 6.0f;
            k12.kneeDb = 12.0f;
            const double t = s.thresholdDb;
            drawCurve (g, r, { { k0, theme::textDim, {} }, { k6, theme::textDim.brighter (0.3f), {} }, { k12, theme::text.withAlpha (0.6f), {} },
                               { s, curveColour, "the window" } },
                       juce::jmax (-60.0, t - 12.0), juce::jmin (0.0, t + 12.0));
            g.setFont (juce::FontOptions (11.5f));
            g.setColour (theme::textDim);
            g.drawText ("grey: a hard knee, 6 dB and 12 dB", r.removeFromBottom (14.0f), juce::Justification::centredRight);
        } });

    pages.push_back ({ "Attack: how fast it grabs", "Letting the start through, or not",
        "A compressor doesn't turn down all at once. The attack time is how fast it gets there: after the attack time it has done "
        "about two thirds (63%) of the turning down, and after three times as long 95%.\n\n"
        "That matters most for the start of a sound, its transient: the click of a kick's beater, the crack of a snare, the pick "
        "on a string. With a slow attack (20-30 ms on a kick) the start passes before the compressor reacts, and only the body "
        "after it is turned down: the hit sounds punchier than before. With a fast attack (0.1-2 ms) the compressor catches "
        "the start too: smoother, softer, further away.\n\n"
        "Typical attacks: kick 20-30 ms and snare 5-20 ms, so the click and crack get through (a kick's click lasts 5-10 ms); "
        "vocals 2-10 ms; bass 10-30 ms; piano 20-30 ms; pads 30-80 ms; buses 10-30 ms.\n\n"
        "The picture shows the start of a loud note with three attacks, 1, 10 and 30 ms, and the window's in orange: how much "
        "each has turned down over the first 120 ms.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "Turning down at the start of a loud note", "the window: " + formatKnob (comp::Knob::attack, s) + " attack");
            const auto note = makeNoteLevels();
            const double most = comp::getReductionDb (s, -6.0);
            const auto p = drawTimeGrid (g, r, 0.12, -juce::jmax (6.0, std::ceil (most + 2.0)), 0.0, 0.02);
            g.setFont (juce::FontOptions (11.5f));

            const auto draw = [&] (float attackMs, juce::Colour c, bool label)
            {
                auto t = s;
                t.attackMs = attackMs;
                const auto reductions = getReductions (t, note);
                std::vector<float> line;

                for (size_t i = 100; i < reductions.size(); ++i)
                    line.push_back (-reductions[i]);

                drawLevels (g, p, line, c, 2.0f);

                if (label)
                {
                    const float y = p.y (-reductions[100 + (size_t) juce::jmin (110, juce::roundToInt (attackMs * 3.0))]);
                    g.drawText (formatKnob (comp::Knob::attack, t), juce::Rectangle<float> (p.x (juce::jmin (0.1, attackMs * 0.003)) + 6.0f, y - 14.0f, 60.0f, 13.0f),
                                juce::Justification::centredLeft);
                }
            };

            for (float ms : { 1.0f, 10.0f, 30.0f })
                draw (ms, theme::textDim, true);

            draw (s.attackMs, curveColour, false);
        } });

    pages.push_back ({ "Release: how fast it lets go", "Back to normal after a loud part",
        "The release time is how fast the compressor lets go once the level drops back under the threshold: after the release "
        "time it has given back about two thirds of what it took.\n\n"
        "Too fast, and the level jumps up and down with every syllable or the waves of a low note: it can sound grainy or even "
        "distort bass. Too slow, and the compressor is still turning down when the next quiet part comes, which gets lost; on a "
        "drum bus or a mix you can hear the level being pulled down and creeping back up, which is called pumping or breathing. "
        "Sometimes that's wanted: in dance music the pumping is part of the groove.\n\n"
        "Typical releases: kick 40-80 ms, snare 80-150 ms, vocals 40-120 ms, bass 100-300 ms, piano 200-300 ms, pads 300-500 ms, "
        "buses 100-300 ms. A release that ends in time with the beat breathes with the song: at 120 BPM an eighth note is 250 ms.\n\n"
        "The picture shows a loud note that stops after a second, with releases of 50, 150 and 500 ms, and the window's in orange: "
        "how long each takes to let go.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "Letting go after a loud note stops (at 1 s)", "the window: " + formatKnob (comp::Knob::release, s) + " release");
            const auto note = makeNoteLevels();
            const double most = comp::getReductionDb (s, -6.0);
            const auto p = drawTimeGrid (g, r, 1.8, -juce::jmax (6.0, std::ceil (most + 2.0)), 0.0, 0.2);
            g.setFont (juce::FontOptions (11.5f));

            const auto draw = [&] (float releaseMs, juce::Colour c, bool label)
            {
                auto t = s;
                t.releaseMs = releaseMs;
                const auto reductions = getReductions (t, note);
                std::vector<float> line;

                for (size_t i = 100; i < reductions.size(); ++i)
                    line.push_back (-reductions[i]);

                drawLevels (g, p, line, c, 2.0f);

                if (label)
                {
                    const size_t at = juce::jmin (line.size() - 1, (size_t) (1000.0 + releaseMs));
                    g.drawText (formatKnob (comp::Knob::release, t), juce::Rectangle<float> (p.x (at / levelRate) + 4.0f, p.y (line[at]) - 6.0f, 70.0f, 13.0f),
                                juce::Justification::centredLeft);
                }
            };

            for (float ms : { 50.0f, 150.0f, 500.0f })
                draw (ms, theme::textDim, true);

            draw (s.releaseMs, curveColour, false);
        } });

    pages.push_back ({ "Makeup: louder again, but more even", "What compression really changes",
        "A compressor only turns down, so the track comes out quieter. Makeup gain turns everything back up afterwards. The loud "
        "parts end up about as loud as before, and the quiet parts louder: the difference between them, the dynamic range, is "
        "smaller.\n\n"
        "An example with a hard knee, a threshold of -18 dB and 4:1: a loud word at -6 dB is 12 dB over the threshold and comes "
        "out " + formatDb (loudOut, 0) + ". A quiet word at -20 dB is under the threshold and comes out unchanged at "
        + formatDb (quietOut, 0) + ". Before, they were 14 dB apart; after, " + formatDb (loudOut - quietOut, 0) + ". With "
        + formatDb (makeup, 0) + " of makeup the loud word is back at -6 dB and the quiet one at " + formatDb (quietOut + makeup, 0)
        + ": the quiet word is " + formatDb (makeup, 0) + " easier to hear.\n\n"
        "That's why compressed sounds seem louder and closer at the same peak level, and why it's easy to fool yourself: louder "
        "sounds better. Match level in the window sets the makeup for you so the track is as loud with the compressor as "
        "without; switch it on and off to hear only what the compression does.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto in = makeVocalLevels();
            const auto out = getOutputLevels (s, in);
            drawCaption (g, r, "The vocal line in, and out with the window's makeup", "how far apart loud and quiet are: "
                             + formatDb (getSpreadDb (in, in), 0) + " in, " + formatDb (getSpreadDb (out, in), 0) + " out");
            drawLegend (g, r.removeFromTop (14.0f), { { "in", inColour }, { "out, with " + formatKnob (comp::Knob::makeup, s) + " makeup", outColour } });
            r.removeFromTop (4.0f);
            const auto p = drawTimeGrid (g, r, 4.0, -48.0, 0.0, 1.0);
            drawLevels (g, p, in, inColour, 1.4f, true);
            drawLevels (g, p, out, outColour, 2.0f);
        } });

    pages.push_back ({ "Peak or RMS", "What the compressor listens to",
        "A compressor has to decide how loud the sound is right now, and there are two ways.\n\n"
        "Peak listens to the highest point of the wave at each moment. It reacts to every short hit and catches transients: "
        "right for drums, plucked and picked parts, and limiting.\n\n"
        "RMS (root mean square) listens to the average power over a short time: DawIt Compressor averages over "
        + juce::String (juce::roundToInt (comp::rmsWindowMs)) + " ms. That's closer to how loud we hear something, so it "
        "follows the loudness of a voice or a bass more smoothly and lets short peaks through. A steady sine wave measures 3 dB "
        "lower as RMS than as peak; a short drum hit, whose peak is much higher than its average, far lower.\n\n"
        "So with RMS, the same threshold compresses less, especially on hits. RMS suits bass, voice, piano, pads and buses.\n\n"
        "The picture shows the made-up drum hits, and what comes out when the compressor listens to their peak and to their "
        "RMS, with the window's other settings.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto peak = processor->getSettings(), rms = peak;
            peak.detection = comp::Detection::peak;
            rms.detection = comp::Detection::rms;
            const auto hits = makeDrumLevels (2.0);
            drawCaption (g, r, "Drum hits through the compressor, listening to peak or to RMS", "threshold " + formatKnob (comp::Knob::threshold, peak));
            drawLegend (g, r.removeFromTop (14.0f), { { "the hits", inColour }, { "out, peak", reductionColour }, { "out, RMS", curveColour } });
            r.removeFromTop (4.0f);
            const auto p = drawTimeGrid (g, r, 1.0, -36.0, 0.0, 0.25);
            drawLevels (g, p, hits, inColour, 1.4f, true);
            drawLevels (g, p, getOutputLevels (rms, hits), curveColour, 2.0f);
            drawLevels (g, p, getOutputLevels (peak, hits), reductionColour, 2.0f);
            drawThreshold (g, p, peak.thresholdDb);
        } });

    const eq::Band sidechain100 { true, eq::BandType::lowCut, 100.0f, 0.0f, eq::flatQ, 12 };

    pages.push_back ({ "The sidechain filter", "Not letting the bass decide",
        "What the compressor listens to (its sidechain) doesn't have to be exactly what it turns down. DawIt Compressor can "
        "take the lows out of what it listens to with a low cut, 12 dB per octave: the sidechain filter.\n\n"
        "Why: low notes carry the most energy. On a drum bus or a whole mix, every kick would make the compressor turn "
        "everything down, so the cymbals and the vocal duck on every beat (pumping). On a vocal, a 'p' blowing into the "
        "microphone (a plosive) would do the same. With the sidechain filter at 80-150 Hz, the compressor reacts to the rest "
        "of the sound and ignores the low thumps, while the sound itself keeps all its lows.\n\n"
        "With the filter at 100 Hz, a 50 Hz kick counts " + formatDb (-eq::getResponseDb (sidechain100, 50.0, 48000.0), 0)
        + " quieter to the compressor, and 25 Hz " + formatDb (-eq::getResponseDb (sidechain100, 25.0, 48000.0), 0) + ".\n\n"
        "Keep it off on kick, bass and limiters: there the lows should drive the compressor.\n\n"
        "In dance music, 'sidechain compression' usually means something else: a compressor on the bass or the pads that "
        "listens to the kick on another track, so they duck on every beat. DawIt Compressor can't listen to another track yet.\n\n"
        "The picture shows the window's sidechain filter: what the compressor hears of each frequency.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const bool on = comp::isSidechainFilterOn (s.sidechainHz);
            drawCaption (g, r, "What the compressor hears of each frequency", on ? "sidechain filter at " + formatKnob (comp::Knob::sidechain, s)
                                                                                : "sidechain filter off: it hears everything");
            auto area = r.withTrimmedLeft (34.0f).withTrimmedBottom (16.0f);
            const auto x = [&] (double hz) { return area.getX() + area.getWidth() * (float) (std::log (hz / 20.0) / std::log (1000.0)); };
            const auto y = [&] (double db) { return juce::jmap ((float) juce::jlimit (-36.0, 3.0, db), -36.0f, 3.0f, area.getBottom(), area.getY()); };
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.fillRect (area);
            g.setFont (juce::FontOptions (10.5f));

            for (double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (x (hz), area.getY(), 1.0f, area.getHeight());
                g.setColour (theme::textDim);
                g.drawText (hz >= 1000.0 ? juce::String (hz / 1000.0) + "k" : juce::String (hz), juce::Rectangle<float> (x (hz) - 20.0f, area.getBottom() + 2.0f, 40.0f, 13.0f),
                            juce::Justification::centred);
            }

            for (int db = 0; db >= -36; db -= 12)
            {
                g.setColour (db == 0 ? theme::gridBar : theme::grid);
                g.fillRect (area.getX(), y (db), area.getWidth(), 1.0f);
                g.setColour (theme::textDim);
                g.drawText (juce::String (db), juce::Rectangle<float> (r.getX(), y (db) - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
            }

            const eq::Band filter { on, eq::BandType::lowCut, s.sidechainHz, 0.0f, eq::flatQ, 12 };
            juce::Path path;

            for (float px = area.getX(); px <= area.getRight(); px += 1.0f)
            {
                const double hz = 20.0 * std::pow (1000.0, (px - area.getX()) / area.getWidth());
                const auto pt = juce::Point<float> (px, y (eq::getResponseDb (filter, hz, 48000.0)));

                if (px <= area.getX())
                    path.startNewSubPath (pt);
                else
                    path.lineTo (pt);
            }

            g.setColour (curveColour);
            g.strokePath (path, juce::PathStrokeType (2.4f));
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawText ("kick and bass", juce::Rectangle<float> (x (40.0), area.getBottom() - 30.0f, x (120.0) - x (40.0), 13.0f), juce::Justification::centred);
            g.drawText ("vocal, guitars, cymbals", juce::Rectangle<float> (x (300.0), area.getBottom() - 30.0f, x (8000.0) - x (300.0), 13.0f), juce::Justification::centred);
        } });

    pages.push_back ({ "Parallel compression: the mix knob", "Squashed and natural together",
        "The mix knob blends the compressed sound with the original. At 100% you hear only the compressed sound; at 30% you "
        "hear the original with 30% of a compressed copy added.\n\n"
        "That's parallel compression, also called New York compression: compress a copy hard (a low threshold, a high ratio, a "
        "fast attack), then mix it in under the original. The original keeps its transients and life, the squashed copy brings "
        "up the quiet details and sustain. The result sounds fuller and denser without sounding compressed. It's common on drums, "
        "and used on vocals and bass too.\n\n"
        "Typical: 100% for most uses; 20-50% for parallel compression with 10-15 dB of turning down on the copy; 70-90% to keep "
        "some life in a vocal.\n\n"
        "The picture shows the made-up drum hits: the original, the copy fully compressed with the window's settings, and the "
        "blend at the window's mix (40% while the window is at 100%).",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            const bool fromWindow = s.mixPercent < 99.0f;

            if (! fromWindow)
                s.mixPercent = 40.0f;   // at 100% the blend is the compressed sound: show a typical parallel blend instead

            auto full = s;
            full.mixPercent = 100.0f;
            const auto hits = makeDrumLevels (2.0);
            drawCaption (g, r, "Drum hits: the original, fully compressed, and the blend",
                         fromWindow ? "the window: " + formatKnob (comp::Knob::mix, s) + " mix" : "the window is at 100%: a blend of 40% shown");
            drawLegend (g, r.removeFromTop (14.0f), { { "original", inColour }, { "100% compressed", reductionColour }, { formatKnob (comp::Knob::mix, s) + " blend", outColour } });
            r.removeFromTop (4.0f);
            const auto p = drawTimeGrid (g, r, 2.0, -48.0, 0.0, 0.5);
            drawLevels (g, p, hits, inColour, 1.4f, true);
            drawLevels (g, p, getOutputLevels (full, hits), reductionColour, 1.6f);
            drawLevels (g, p, getOutputLevels (s, hits), outColour, 2.2f);
        } });

    juce::String amounts;

    for (auto& preset : comp::getPresets())
        if (preset.targetReductionDb > 0.0f)
            amounts << "- " << preset.name << ": about " << formatNumber (preset.targetReductionDb) << " dB, at " << formatKnob (comp::Knob::ratio, preset.settings) << "\n";

    pages.push_back ({ "How much, for what", "Typical amounts of compression",
        "How much a compressor turns down is shown by its gain reduction meter: how many dB it takes off the loudest parts. "
        "Typical amounts for the loudest parts, from DawIt Compressor's starting points:\n\n" + amounts +
        "\nIn general: 1-2 dB on a whole mix is glue you feel more than hear; 3-6 dB on a vocal or bass makes it sit; 10 dB "
        "and more is an effect, or parallel compression.\n\n"
        "Pick a starting point in the window for the instrument, then use Set it up step by step: it plays the track, sets the "
        "threshold so the loudest parts come down by the right amount, then walks you through attack, release and level.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "How much the loudest parts are turned down", "the starting points");
            std::vector<const comp::Preset*> presets;

            for (auto& preset : comp::getPresets())
                if (preset.targetReductionDb > 0.0f)
                    presets.push_back (&preset);

            const float rowHeight = r.getHeight() / (float) juce::jmax (1, (int) presets.size());
            auto bars = r.withTrimmedLeft (150.0f).withTrimmedRight (90.0f);
            g.setFont (juce::FontOptions (12.0f));

            for (size_t i = 0; i < presets.size(); ++i)
            {
                const auto row = r.withHeight (rowHeight).withY (r.getY() + rowHeight * (float) i);
                const float w = bars.getWidth() * presets[i]->targetReductionDb / 15.0f;
                g.setColour (theme::text);
                g.drawText (presets[i]->name, row.withWidth (146.0f), juce::Justification::centredLeft, true);
                g.setColour (reductionColour.withAlpha (0.7f));
                g.fillRect (juce::Rectangle<float> (bars.getX(), row.getCentreY() - rowHeight * 0.3f, w, rowHeight * 0.6f));
                g.setColour (theme::textDim);
                g.drawText (formatNumber (presets[i]->targetReductionDb) + " dB, " + formatKnob (comp::Knob::ratio, presets[i]->settings),
                            juce::Rectangle<float> (bars.getX() + w + 6.0f, row.getY(), 100.0f, row.getHeight()), juce::Justification::centredLeft);
            }
        } });

    pages.push_back ({ "Too much", "Flat, pumping, lifeless",
        "Compression is easy to overdo, because a compressed sound is louder and seems better at first. Signs that it's too "
        "much:\n\n"
        "- Flat: everything is equally loud, so nothing stands out; the music stops breathing and gets tiring to listen to.\n"
        "- No punch: a fast attack has taken the start off every hit.\n"
        "- Pumping and breathing: the level audibly ducks and comes back, and noise or room sound swells up in the gaps.\n"
        "- Up-front noise: breaths, clicks and room sound are as loud as the notes.\n"
        "- Distortion on low notes: a release so fast that it follows the waves of the bass.\n\n"
        "To check: switch the compressor off and on with Match level on, so both are equally loud. If the version without "
        "sounds more alive and you don't miss anything, use less: raise the threshold, lower the ratio, or blend it with the mix "
        "knob.\n\n"
        "The picture shows the made-up vocal line with the window's settings, and with heavy settings (a threshold of -36 dB at "
        "10:1 and a fast release): how far apart its loud and quiet parts are with each.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            auto heavy = s;
            heavy.thresholdDb = -36.0f;
            heavy.ratio = 10.0f;
            heavy.releaseMs = 30.0f;
            heavy.attackMs = 1.0f;
            heavy.mixPercent = 100.0f;
            const auto in = makeVocalLevels();
            auto window = getOutputLevels (s, in), squashed = getOutputLevels (heavy, in);

            // Both at the same peak, as match level would compare them.
            const auto peakOf = [] (const std::vector<float>& v) { return *std::max_element (v.begin(), v.end()); };
            const double windowShift = peakOf (in) - peakOf (window), squashedShift = peakOf (in) - peakOf (squashed);

            for (auto& v : window)
                v += (float) windowShift;

            for (auto& v : squashed)
                v += (float) squashedShift;

            drawCaption (g, r, "The vocal line, at the same peak", "loud to quiet: " + formatDb (getSpreadDb (in, in), 0) + " in, "
                             + formatDb (getSpreadDb (window, in), 0) + " with the window, " + formatDb (getSpreadDb (squashed, in), 0) + " heavy");
            drawLegend (g, r.removeFromTop (14.0f), { { "in", inColour }, { "the window's settings", outColour }, { "too much", reductionColour } });
            r.removeFromTop (4.0f);
            const auto p = drawTimeGrid (g, r, 4.0, -48.0, 0.0, 1.0);
            drawLevels (g, p, in, inColour, 1.4f, true);
            drawLevels (g, p, window, outColour, 1.8f);
            drawLevels (g, p, squashed, reductionColour, 1.8f);
        } });

    pages.push_back ({ "How DawIt Compressor works", "From level to gain",
        "For every sample, DawIt Compressor goes through the same steps:\n\n"
        "1. Listen: it takes the louder of the left and right channel, so both sides are turned down together and the stereo "
        "image doesn't shift. With the sidechain filter on, it listens through a low cut.\n"
        "2. Measure: the peak of the wave right now, or the RMS over " + juce::String (juce::roundToInt (comp::rmsWindowMs))
        + " ms, in dB.\n"
        "3. The curve: how many dB that level should be turned down, from the threshold, ratio and knee (the soft knee follows "
        "Giannoulis, Massberg and Reiss, 2012).\n"
        "4. Timing: the turning down moves towards that value with the attack time while it needs to turn down more, and with "
        "the release time while it can let go. Smoothing the dB like this is what makes the compressor sound musical.\n"
        "5. Turn down: the sound is multiplied by the gain, then the makeup, and blended with the original by the mix knob.\n\n"
        "For learning, it also keeps the last 4 seconds of levels for the window's graph, can play only what it removed (the "
        "difference between in and out), and measures the level in and out over half a second for Match level.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The path of the sound, and of the level", "with the window's settings");
            const float boxW = (r.getWidth() - 4.0f * 22.0f) / 5.0f, boxH = 66.0f;
            const float top = r.getY() + 20.0f, bottom = r.getBottom() - boxH - 30.0f;
            const juce::String names[] { "Sidechain filter", "Detector", "Curve", "Attack and release", "Gain" };
            const juce::String details[] { comp::isSidechainFilterOn (s.sidechainHz) ? "low cut at " + formatKnob (comp::Knob::sidechain, s) : "off: hears everything",
                                           s.detection == comp::Detection::rms ? "RMS over 10 ms" : "peak",
                                           formatKnob (comp::Knob::threshold, s) + ", " + formatKnob (comp::Knob::ratio, s) + ", knee " + formatKnob (comp::Knob::knee, s),
                                           formatKnob (comp::Knob::attack, s) + " / " + formatKnob (comp::Knob::release, s),
                                           "turned down by the result" };
            std::vector<juce::Rectangle<float>> boxes;

            for (int i = 0; i < 5; ++i)
            {
                boxes.push_back ({ r.getX() + (boxW + 22.0f) * (float) i, top + (i == 4 ? bottom - top : 0.0f), boxW, boxH });
                drawBox (g, boxes.back(), names[i], details[i], i == 4 ? outColour : curveColour);
            }

            for (int i = 0; i < 3; ++i)
                drawArrow (g, { boxes[(size_t) i].getRight() + 2.0f, boxes[(size_t) i].getCentreY() }, { boxes[(size_t) i + 1].getX() - 2.0f, boxes[(size_t) i + 1].getCentreY() });

            drawArrow (g, { boxes[3].getCentreX(), boxes[3].getBottom() + 2.0f }, { boxes[4].getX() - 2.0f, boxes[4].getCentreY() });

            // The sound's own path, along the bottom.
            const auto sound = juce::Rectangle<float> (r.getX(), bottom, boxW, boxH);
            drawBox (g, sound, "In", "the track's sound", inColour);
            drawArrow (g, { sound.getRight() + 2.0f, sound.getCentreY() }, { boxes[4].getX() - 2.0f, boxes[4].getCentreY() });
            drawArrow (g, { sound.getCentreX(), sound.getY() - 2.0f }, { boxes[0].getCentreX(), boxes[0].getBottom() + 2.0f });
            const auto out = juce::Rectangle<float> (r.getX() + (boxW + 22.0f) * 4.0f, bottom + boxH + 6.0f, boxW, 22.0f);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText ("then makeup " + formatKnob (comp::Knob::makeup, s) + ", mix " + formatKnob (comp::Knob::mix, s), out.toNearestInt(),
                              juce::Justification::centred, 1, 0.8f);
        } });

    return pages;
}
} // namespace daw::comptext
