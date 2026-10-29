#include "DistortionGuide.h"
#include "engine/BuiltInSaturation.h"

namespace daw::disttext
{
using dist::Knob;
using dist::Type;

namespace
{
    juce::String formatDb (double db)
    {
        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    /** "-6.0 dB" for a peak given as a gain. */
    juce::String formatPeak (float peak)
    {
        return juce::String (juce::Decibels::gainToDecibels (peak, -100.0f), 1) + " dB";
    }

    float getLoudest (const std::vector<float>& levels)
    {
        return levels.empty() ? -150.0f : *std::max_element (levels.begin(), levels.end());
    }

    /** Whether the note itself is far quieter than what the curve made of it, as with rectify. */
    bool isNoteGone (const std::vector<float>& levels)
    {
        return ! levels.empty() && levels[0] < getLoudest (levels) - 20.0f;
    }

    /** "THD 34%", or that the note is gone. */
    juce::String describeThd (const std::vector<float>& levels)
    {
        if (isNoteGone (levels))
            return "the note itself is gone; the loudest is its octave";

        const float thd = dist::getThdPercent (levels);
        return "THD " + fx::formatNumber (thd, thd < 10.0f ? 1 : 0) + "%";
    }

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }
}

juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::tight:   return "Tight";
        case (int) Knob::drive:   return "Drive";
        case (int) Knob::type:    return "Type: the five curves";
        case (int) Knob::tone:    return "Tone";
        case (int) Knob::speaker: return "Speaker";
        case (int) Knob::output:  return "Output";
        case (int) Knob::mix:     return "Mix";
        case curve:               return "The curve";
        case harmonics:           return "The harmonics it ends up with";
        case filters:             return "Before and after the curve";
        case match:               return "Match level";
        case fxui::overview:      return "DawIt Distortion";
        case fxui::presets:       return "Starting points";
        case fxui::clap:          return "Play a note";
        case fxui::listen:        return "Hear only what it adds";
        case fxui::guide:         return "What is distortion?";
        case fxui::history:       return "The last 4 seconds";
        default:                  return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::tight:
            return "A low cut before the curve: the lows are turned down before they're distorted. Low notes carry the most energy, "
                   "so they push hardest into the curve; distorted, they turn flubby and muddy, and every pair of low notes makes "
                   "new tones even lower (intermodulation, see What is saturation?). Cutting them first keeps the distortion tight "
                   "and clear; the speaker and the harmonics give the weight back.\n\n"
                   "Many classic pedals do this: the Tube Screamer cuts below about 720 Hz before its diodes, which is why metal "
                   "players put one in front of an already distorted amp. With the mix below 100%, the clean sound keeps all its lows.";
        case (int) Knob::drive:
            return "How hard the sound is pushed into the curve: it is turned up by this much before it goes in. A little reaches "
                   "only the bend, so loud notes break up and soft ones stay clean; a lot pushes nearly the whole wave past the "
                   "limit, so it becomes a square that is equally loud whatever you play.\n\n"
                   "Distortion has more drive than saturation: up to 48 dB, as much as a fuzz pedal or a high-gain amp. So it "
                   "also turns up the noise, hum and string squeak between the notes by as much: that's why high-gain players use "
                   "a noise gate, such as DawIt Gate, before or after it.";
        case (int) Knob::type:
            return "The shape of the curve, each modelled on a kind of gear.\n\n"
                   "Overdrive: soft and a little lopsided, like the clipping diodes of an overdrive pedal or a tube amp turned up. "
                   "It follows the playing: soft notes stay clean-ish, hard ones growl. Blues, rock, edge-of-breakup.\n\n"
                   "Distortion: nearly flat-topped and symmetric, like the diodes to ground of a distortion pedal (ProCo RAT, Boss "
                   "DS-1). Even, compressed and thick: rock and metal rhythm guitar.\n\n"
                   "Fuzz: hard and lopsided, with a bias that chokes off quiet sounds, like the starved transistors of a fuzz "
                   "pedal. A buzzing square that sputters as the note dies: 1960s rock, stoner, garage.\n\n"
                   "Rectify: the bottom of the wave is flipped up, which doubles its frequency: an octave up, like Hendrix's "
                   "Octavia. Best on single notes.\n\n"
                   "Fold: what goes over the top folds back down instead of being cut off, like a synth's wavefolder. Every bit "
                   "more drive adds another fold and brighter, shifting harmonics: metallic, vocal, alive. Synths and effects.";
        case (int) Knob::tone:
            return "A high cut after the curve. Distortion adds harmonics up to the very top of the range we hear, and those sound "
                   "fizzy and harsh. Lowering the tone keeps the body of the distortion and takes the fizz away. Almost every "
                   "distortion pedal has one, because hard clipping without it is too bright to listen to.";
        case (int) Knob::speaker:
            return "The sound of a guitar speaker in its cabinet after the distortion. A 12-inch guitar speaker can't move enough "
                   "air below about 90 Hz, rings a little in the upper mids where its cone breaks up (around 2.5 kHz), and hardly "
                   "plays anything above 5 kHz. That's why a distorted guitar from an amp sounds warm and a distortion pedal "
                   "straight into a mixing desk sounds like a swarm of bees: the speaker takes away the fizz.\n\n"
                   "Turn it on for guitars and anything that should sound like an amp; leave it off for bass, drums, vocals and "
                   "synths, whose lows and highs you want to keep.";
        case (int) Knob::output:
            return "The level after everything else. With match level on, it is a trim on top of the matched level; off, use it to "
                   "turn the distorted sound back down, because driving the curve makes quiet sounds enormously louder.";
        case (int) Knob::mix:
            return "How much of the distorted sound you hear against the clean one. Below 100% it's parallel distortion: the clean "
                   "sound keeps its attack, its low end and its clarity, and the distorted copy underneath adds growl, density and "
                   "loudness. It's how bass and drums are usually distorted: driven hard, but blended in.";
        case match:
            return "Keeps the sound as loud coming out as going in, by turning it down by as much as the distortion turned it up, "
                   "measured over the last half second. Distortion can make a sound 20 dB and more louder, and louder always sounds "
                   "more exciting, so without this you can't tell what the distortion itself does. Switch the effect off and on "
                   "with match level on to compare honestly.";
        case curve:
            return "What comes out (up) for every value that goes in (left to right), over the whole range a sample can have, -1 "
                   "to 1, with the drive included. The dashed line is no change. The thick part is the stretch the test note (A3) "
                   "at the track's level uses, after the tight low cut, so you can see how far into the curve it goes.\n\n"
                   "Under it, that note before (grey) and after (red) the curve, made as loud, so only the change in shape shows. "
                   "Drag up and down on the curve for the drive.";
        case harmonics:
            return "What one pure note (A3, 220 Hz) at the track's level turns into, with the filters before and after the curve: "
                   "the note itself (1) and the harmonics the curve adds, at whole-number multiples of its frequency, against the "
                   "loudest one. Yellow ones are even (an octave, two octaves...), red ones odd. With the speaker or the tone on, "
                   "you see the high ones drop away.\n\n"
                   "THD (total harmonic distortion) adds all the harmonics up against the note: a crunchy guitar is far over 30%, "
                   "a fuzz near 50%. Rectify takes the note itself away, leaving its octave.";
        case filters:
            return "What the filters do to each frequency, from 20 Hz (left) to 20 kHz (right): blue is the tight low cut before "
                   "the curve, red the tone and the speaker after it. 0 dB is unchanged.\n\n"
                   "Where a filter sits matters. Before the curve it changes what gets distorted: lows cut there are distorted "
                   "less. After it, it changes what you hear of the distortion: highs cut there take the fizz away.";
        case fxui::presets:
            return "Settings for the sounds distortion is usually used for, each explaining why every knob is set the way it is. "
                   "They are a start, not an answer: play the track and adjust by ear, with match level on.";
        case fxui::clap:
            return "Plays a plain A (220 Hz, a sine with no harmonics of its own) through the distortion, so everything you hear "
                   "besides that one pure note is what the distortion adds. It works while the song is stopped.";
        case fxui::listen:
            return "Plays only what the distortion adds: the output minus a clean copy of the input, through the same filters, at "
                   "the same level. What's left is the harmonics on their own. Switch the type and hear their character change.";
        case fxui::guide:
            return "A guide to distortion for beginners: how it differs from saturation, the five curves and the gear they come "
                   "from, overdrive, distortion and fuzz pedals, sustain and noise, octaves and folds, why the lows are cut first, "
                   "the guitar speaker, when and how to use it in a mix, and how DawIt Distortion works. The pictures follow the "
                   "knobs here.";
        case fxui::history:
            return "The last 4 seconds while the track plays: grey the track, red what the distortion adds, as loud as it is mixed. "
                   "With a lot of drive the red stays high even as the notes die away: that's the sustain, and the reason noise "
                   "between notes gets louder too.";
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::tight:
            return "Off: fuzz, synths, drums, when the lows should be distorted too.\n"
                   "100-200 Hz: guitars, to keep the low strings tight.\n"
                   "250-400 Hz: high-gain metal; bass in parallel.\n"
                   "500-1000 Hz: telephone and megaphone voices.";
        case (int) Knob::drive:
            return "6-15 dB: edge of breakup, blues, electric piano.\n"
                   "15-30 dB: crunch and rock rhythm, distorted bass, vocal effects.\n"
                   "30-48 dB: high gain, lead sustain, fuzz.";
        case (int) Knob::type:
            return "Overdrive: blues, rock and lead guitar, electric piano, organ.\n"
                   "Distortion: rock and metal rhythm guitar, bass, smashed drums, vocal effects.\n"
                   "Fuzz: riffs and single notes, 60s and stoner rock, fuzz bass.\n"
                   "Rectify: octave leads, synth basses.\n"
                   "Fold: synths and sound design.";
        case (int) Knob::tone:
            return "Off: synths and parallel drums, when the top should stay.\n"
                   "5-8 kHz: guitars and bass.\n"
                   "2-4 kHz: lo-fi, telephone, fuzz without a speaker.";
        case (int) Knob::speaker:
            return "On: electric guitar, and anything that should sound like it comes from an amp.\n"
                   "Off: bass, drums, vocals, synths.";
        case (int) Knob::output:
            return "0 dB with match level on.\n"
                   "Down by about as much as the drive with it off, then by ear.";
        case (int) Knob::mix:
            return "100%: guitars, effects, synths.\n"
                   "20-50%: bass and drums in parallel, so the clean sound stays on top.";
        default:
            return {};
    }
}

juce::String describeSettings (int t, const dist::Settings& s, float peak)
{
    const auto levels = dist::getHarmonicsDb (s, peak, 12);
    const float into = dist::getPeakIntoCurve (s, peak);
    const float pushed = juce::Decibels::gainToDecibels (into, -100.0f) + s.driveDb;

    switch (t)
    {
        case (int) Knob::tight:
            if (! dist::isTightOn (s.tightHz))
                return "Off: the whole sound, lows and all, goes into the curve.";

            return dist::formatKnob (Knob::tight, s) + ": below it, the sound going into the curve is turned down by 12 dB more "
                   "for every octave further down. The A3 test note (220 Hz) goes in "
                   + formatDb (juce::Decibels::gainToDecibels (dist::getGainBefore (s, dist::noteHz))) + ", a low E (82 Hz) "
                   + formatDb (juce::Decibels::gainToDecibels (dist::getGainBefore (s, 82.4))) + ".";

        case (int) Knob::drive:
            return dist::formatKnob (Knob::drive, s) + ": the test note at the track's level (" + formatPeak (peak) + ") goes into the curve at "
                   + formatDb (pushed) + (pushed < -6.0f ? ", where the curve is still nearly straight." : pushed < 6.0f ? ", where it bends."
                                                                                                        : ", far past the bend.")
                   + " What comes out: " + describeThd (levels) + ".";

        case (int) Knob::type:
        {
            const float top = getLoudest (levels);
            return dist::formatKnob (Knob::type, s) + ": on the test note at " + formatPeak (peak) + ", the note comes out at "
                   + formatDb (levels[0] - top) + ", the octave (2nd harmonic) " + (levels[1] < -100.0f ? juce::String ("not at all") : formatDb (levels[1] - top))
                   + " and the octave and fifth (3rd) " + formatDb (levels[2] - top) + ", against the loudest.";
        }

        case (int) Knob::tone:
            if (! dist::isToneOn (s.toneHz))
                return "Off: every harmonic the curve makes is heard" + juce::String (s.speaker ? ", up to where the speaker stops." : ".");

            return dist::formatKnob (Knob::tone, s) + ": harmonics above it are turned down, 12 dB for every octave further up.";

        case (int) Knob::speaker:
            if (! s.speaker)
                return "Off: the raw distortion, as a pedal sounds straight into a mixing desk.";

            return "On: " + formatDb (dist::getSpeakerDb (60.0)) + " at 60 Hz, " + formatDb (dist::getSpeakerDb (2500.0)) + " at 2.5 kHz, "
                   + formatDb (dist::getSpeakerDb (8000.0)) + " at 8 kHz.";

        case (int) Knob::output:
            return dist::formatKnob (Knob::output, s) + ".";

        case (int) Knob::mix:
            return dist::formatKnob (Knob::mix, s) + (s.mixPercent > 99.5f ? ": only the distorted sound." : ": the clean sound with the distorted one blended in.");

        case curve:
            return "The track peaks at " + formatPeak (peak) + "; the test note at that level goes in at " + formatPeak (into)
                   + " after the tight low cut, and driven " + dist::formatKnob (Knob::drive, s) + ", it reaches " + formatDb (pushed) + " into the curve.";

        case harmonics:
            return describeThd (levels).replace ("THD", "THD of") + ", on the test note at " + formatPeak (peak) + ".";

        case filters:
        {
            juce::StringArray parts;
            parts.add (dist::isTightOn (s.tightHz) ? "a low cut at " + dist::formatKnob (Knob::tight, s) + " before the curve" : "nothing before the curve");
            parts.add (dist::isToneOn (s.toneHz) ? "a high cut at " + dist::formatKnob (Knob::tone, s) + " after it" : "no tone after it");
            parts.add (s.speaker ? "and the speaker" : "and no speaker");
            const auto text = parts.joinIntoString (", ") + ".";
            return text.substring (0, 1).toUpperCase() + text.substring (1);
        }

        default:
            return {};
    }
}

//==============================================================================
juce::Rectangle<float> drawCurve (juce::Graphics& g, juce::Rectangle<float> bounds, const dist::Settings& s, float peak, bool labels)
{
    const float range = s.type == Type::fuzz ? 1.6f : 1.2f;
    auto area = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()), juce::jmin (bounds.getWidth(), bounds.getHeight()));
    auto toX = [&] (float x) { return juce::jmap (x, -1.0f, 1.0f, area.getX(), area.getRight()); };
    auto toY = [&] (float y) { return juce::jmap (juce::jlimit (-range, range, y), -range, range, area.getBottom(), area.getY()); };
    const float drive = juce::Decibels::decibelsToGain (s.driveDb), into = dist::getPeakIntoCurve (s, peak);

    g.setColour (theme::grid);
    g.fillRect (area.getX(), toY (0.0f), area.getWidth(), 1.0f);
    g.fillRect (toX (0.0f), area.getY(), 1.0f, area.getHeight());

    for (const float level : { 1.0f, -1.0f })
    {
        g.setColour (theme::grid.brighter (0.2f));
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine ({ area.getX(), toY (level), area.getRight(), toY (level) }, dashes, 2, 1.0f);
    }

    const float dashes[] = { 5.0f, 4.0f };
    g.setColour (theme::gridBar);
    g.drawDashedLine ({ toX (-1.0f), toY (-1.0f), toX (1.0f), toY (1.0f) }, dashes, 2, 1.0f);

    // Many steps: a fold at full drive turns many times.
    auto path = [&] (float from, float to)
    {
        juce::Path p;
        const int steps = 1200;

        for (int i = 0; i <= steps; ++i)
        {
            const float x = from + (to - from) * (float) i / (float) steps;
            const float y = toY (dist::distort (s.type, drive, x));
            i == 0 ? p.startNewSubPath (toX (x), y) : p.lineTo (toX (x), y);
        }

        return p;
    };

    g.setColour (colour.withAlpha (0.5f));
    g.strokePath (path (-1.0f, 1.0f), juce::PathStrokeType (1.5f));
    g.setColour (colour);
    g.strokePath (path (-juce::jmin (1.0f, into), juce::jmin (1.0f, into)), juce::PathStrokeType (3.5f));

    for (const float x : { -into, into })
        if (std::abs (x) <= 1.0f)
            g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ toX (x), toY (dist::distort (s.type, drive, x)) }));

    if (labels)
    {
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (theme::textDim);
        g.drawText ("in", area.withTop (toY (0.0f) + 2.0f).withHeight (12.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
        g.drawText ("out", area.withX (toX (0.0f) + 4.0f).withHeight (12.0f), juce::Justification::centredLeft);
        g.drawText ("no change", juce::Rectangle<float> (70.0f, 12.0f).withPosition (toX (0.55f), toY (0.55f) - 2.0f), juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("the note: up to " + formatPeak (into), area.withTop (area.getBottom() - 14.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
    }

    return area;
}

void drawBeforeAfter (juce::Graphics& g, juce::Rectangle<float> area, const dist::Settings& s, float peak, double cycles)
{
    constexpr int steps = 600;
    const float drive = juce::Decibels::decibelsToGain (s.driveDb), into = dist::getPeakIntoCurve (s, peak);
    std::vector<float> before (steps + 1), after (steps + 1);
    double powerBefore = 0.0, powerAfter = 0.0, mean = 0.0;

    for (int i = 0; i <= steps; ++i)
    {
        before[(size_t) i] = into * (float) std::sin (juce::MathConstants<double>::twoPi * cycles * i / steps);
        after[(size_t) i] = dist::distort (s.type, drive, before[(size_t) i]);
        mean += after[(size_t) i] / (steps + 1);
    }

    for (int i = 0; i <= steps; ++i)
    {
        after[(size_t) i] -= (float) mean;   // without its DC, as DawIt Distortion removes it
        powerBefore += before[(size_t) i] * before[(size_t) i];
        powerAfter += after[(size_t) i] * after[(size_t) i];
    }

    const float sameLoudness = powerAfter > 0.0 ? (float) std::sqrt (powerBefore / powerAfter) : 1.0f;
    const float scale = area.getHeight() * 0.42f / juce::jmax (0.02f, into * 1.5f);
    auto line = [&] (const std::vector<float>& v, float gain, juce::Colour c, float thickness)
    {
        juce::Path p;

        for (int i = 0; i <= steps; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / steps;
            const float y = juce::jlimit (area.getY(), area.getBottom(), area.getCentreY() - v[(size_t) i] * gain * scale);
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }

        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (thickness));
    };

    g.setColour (theme::grid);
    g.fillRect (area.getX(), area.getCentreY(), area.getWidth(), 1.0f);
    line (before, 1.0f, theme::textDim, 1.5f);
    line (after, sameLoudness, colour, 2.0f);
}

void drawHarmonics (juce::Graphics& g, juce::Rectangle<float> bounds, const dist::Settings& s, float peak, int count, bool labels)
{
    const auto levels = dist::getHarmonicsDb (s, peak, count);
    const float top = getLoudest (levels);
    auto area = bounds.withTrimmedLeft (30.0f).withTrimmedBottom (labels ? 38.0f : 14.0f).withTrimmedTop (labels ? 18.0f : 2.0f);
    constexpr float bottomDb = -80.0f;
    auto toY = [&] (float db) { return juce::jmap (juce::jlimit (bottomDb, 0.0f, db), bottomDb, 0.0f, area.getBottom(), area.getY()); };

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= bottomDb; db -= 20.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (26.0f, 12.0f).withCentre ({ area.getX() - 15.0f, toY (db) }), juce::Justification::centredRight);
    }

    const float column = area.getWidth() / (float) count;

    for (int k = 1; k <= count; ++k)
    {
        const float db = levels[(size_t) k - 1] - top;
        const auto cell = juce::Rectangle<float> (area.getX() + column * (float) (k - 1), area.getY(), column, area.getHeight()).reduced (column * 0.18f, 0.0f);
        const auto c = k == 1 ? theme::text : k % 2 == 0 ? evenColour : colour;

        if (db > bottomDb)
        {
            g.setColour (c);
            g.fillRect (cell.withTop (toY (db)));
        }

        g.setColour (theme::textDim);

        if (column > 14.0f || k % 4 == 1)
            g.drawText (juce::String (k), juce::Rectangle<float> (juce::jmax (column, 20.0f), 12.0f).withCentre ({ cell.getCentreX(), area.getBottom() + 8.0f }),
                        juce::Justification::centred);

        if (labels && column > 40.0f)
        {
            // "2 oct + 3rd" on two lines, so neighbours don't run into each other.
            g.setFont (juce::FontOptions (9.5f));
            g.drawFittedText (sat::getHarmonicInterval (k).replace (" + ", "\n+ "),
                              juce::Rectangle<float> (column, 24.0f).withPosition (cell.getCentreX() - column / 2.0f, area.getBottom() + 13.0f).toNearestInt(),
                              juce::Justification::centredTop, 2, 1.0f);
            g.setFont (juce::FontOptions (10.0f));
        }
    }

    if (labels)
    {
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::text);
        g.drawText (describeThd (levels) + ", on A3 at " + formatPeak (peak), bounds.withHeight (14.0f), juce::Justification::centredRight);
        g.setColour (evenColour);
        g.drawText ("even", bounds.withHeight (14.0f).withTrimmedLeft (30.0f), juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("odd", bounds.withHeight (14.0f).withTrimmedLeft (66.0f), juce::Justification::centredLeft);
    }
}

void drawFilters (juce::Graphics& g, juce::Rectangle<float> bounds, const dist::Settings& s, bool labels)
{
    auto area = bounds.withTrimmedLeft (labels ? 30.0f : 4.0f).withTrimmedBottom (14.0f).withTrimmedTop (labels ? 38.0f : 2.0f);
    constexpr float topDb = 6.0f, bottomDb = -36.0f;
    auto toX = [&] (double hz) { return area.getX() + area.getWidth() * (float) (std::log (hz / 20.0) / std::log (1000.0)); };
    auto toY = [&] (double db) { return juce::jmap (juce::jlimit (bottomDb, topDb, (float) db), bottomDb, topDb, area.getBottom(), area.getY()); };

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= bottomDb; db -= 12.0f)
    {
        g.setColour (db > -0.5f ? theme::gridBar : theme::grid);
        g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);

        if (labels)
        {
            g.setColour (theme::textDim);
            g.drawText (juce::String ((int) db), juce::Rectangle<float> (26.0f, 12.0f).withCentre ({ area.getX() - 15.0f, toY (db) }), juce::Justification::centredRight);
        }
    }

    for (const double hz : { 100.0, 1000.0, 10000.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (toX (hz), area.getY(), 1.0f, area.getHeight());
        g.setColour (theme::textDim);
        g.drawText (formatHz (hz), juce::Rectangle<float> (50.0f, 12.0f).withCentre ({ toX (hz), area.getBottom() + 8.0f }), juce::Justification::centred);
    }

    auto curveOf = [&] (const std::function<double (double)>& gainAt, juce::Colour c)
    {
        juce::Path p;
        const int steps = juce::jmax (40, (int) area.getWidth());

        for (int i = 0; i <= steps; ++i)
        {
            const double hz = 20.0 * std::pow (1000.0, i / (double) steps);
            const float y = toY (juce::Decibels::gainToDecibels (gainAt (hz), -100.0));
            i == 0 ? p.startNewSubPath (toX (hz), y) : p.lineTo (toX (hz), y);
        }

        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (2.0f));
    };

    curveOf ([&s] (double hz) { return dist::getGainAfter (s, hz); }, colour);
    curveOf ([&s] (double hz) { return dist::getGainBefore (s, hz); }, beforeColour);

    if (labels)
    {
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.setColour (theme::textDim);
        g.drawText ("WHAT THE FILTERS DO", bounds.withHeight (14.0f), juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.5f));
        auto legend = bounds.withTrimmedTop (16.0f).withHeight (14.0f);
        g.setColour (beforeColour);
        g.drawText ("before the curve", legend.removeFromLeft (100.0f), juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("after it", legend, juce::Justification::centredLeft);
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

    /** A spectrum from 0 to `topHz`, with a line for each frequency (dB), coloured by what it is. */
    void drawLines (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<std::pair<double, float>>& lines, double topHz,
                    const std::function<juce::Colour (double)>& colourFor)
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (area);
        auto toY = [&] (float db) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, area.getBottom(), area.getY() + 2.0f); };

        for (auto& [hz, db] : lines)
        {
            if (db < -60.0f || hz > topHz)
                continue;

            const float x = area.getX() + area.getWidth() * (float) (hz / topHz);
            g.setColour (colourFor (hz));
            g.fillRect (x - 1.25f, toY (db), 2.5f, area.getBottom() - toY (db));
        }
    }

    /** The settings with the filters off, so pictures that compare curves compare only the curves. */
    dist::Settings withoutFilters (dist::Settings s)
    {
        s.tightHz = dist::minTight;
        s.toneHz = dist::maxTone;
        s.speaker = false;
        return s;
    }

    /** A plucked note: A2 (110 Hz), dying away like a guitar string, then nothing; with a little noise under it all (-60 dB). */
    constexpr double pluckRate = 8000.0, pluckSeconds = 3.0, pluckEnds = 2.0;

    std::vector<float> makePluck()
    {
        const int n = (int) (pluckRate * pluckSeconds);
        std::vector<float> v ((size_t) n);
        juce::Random random (11);

        for (int i = 0; i < n; ++i)
        {
            const double t = i / pluckRate;
            const double note = t < pluckEnds ? 0.5 * juce::jmin (1.0, t / 0.003) * std::exp (-t / 0.45) * std::sin (juce::MathConstants<double>::twoPi * 110.0 * t) : 0.0;
            v[(size_t) i] = (float) note + 0.001f * (random.nextFloat() * 2.0f - 1.0f);
        }

        return v;
    }

    /** Through the curve, then made to peak as high as the pluck. */
    std::vector<float> distortPluck (const std::vector<float>& in, Type type, float driveDb)
    {
        const float drive = juce::Decibels::decibelsToGain (driveDb);
        std::vector<float> out (in.size());
        float peakIn = 0.0f, peakOut = 0.0f, dcIn = 0.0f, dcOut = 0.0f;
        const float pole = 1.0f - juce::MathConstants<float>::twoPi * 10.0f / (float) pluckRate;

        for (size_t i = 0; i < in.size(); ++i)
        {
            const float v = dist::distort (type, drive, in[i]);
            const float dc = v - dcIn + pole * dcOut;   // without the DC, as DawIt Distortion
            dcIn = v;
            dcOut = dc;
            out[i] = dc;
            peakIn = juce::jmax (peakIn, std::abs (in[i]));
            peakOut = juce::jmax (peakOut, std::abs (dc));
        }

        for (auto& v : out)
            v *= peakIn / juce::jmax (1.0e-9f, peakOut);

        return out;
    }

    /** The level in dB of every 20 ms. */
    std::vector<float> getEnvelopeDb (const std::vector<float>& v)
    {
        const int block = (int) (0.02 * pluckRate);
        std::vector<float> db;

        for (size_t start = 0; start + (size_t) block <= v.size(); start += (size_t) block)
        {
            double sum = 0.0;

            for (int i = 0; i < block; ++i)
                sum += (double) v[start + (size_t) i] * v[start + (size_t) i];

            db.push_back ((float) juce::Decibels::gainToDecibels (std::sqrt (sum / block), -100.0));
        }

        return db;
    }

    /** How long the note stays within 12 dB of its loudest, in seconds. */
    double getSustainSeconds (const std::vector<float>& envelopeDb)
    {
        const float loudest = getLoudest (envelopeDb);
        int last = 0;

        for (int i = 0; i < (int) envelopeDb.size(); ++i)
            if (envelopeDb[(size_t) i] > loudest - 12.0f)
                last = i;

        return (last + 1) * 0.02;
    }

    /** The level after the note ends: the noise. */
    float getNoiseDb (const std::vector<float>& envelopeDb)
    {
        const size_t from = (size_t) ((pluckEnds + 0.3) / 0.02);
        double sum = 0.0;
        int count = 0;

        for (size_t i = from; i < envelopeDb.size(); ++i, ++count)
            sum += std::pow (10.0, envelopeDb[i] / 10.0);

        return count > 0 ? (float) (10.0 * std::log10 (juce::jmax (1.0e-12, sum / count))) : -100.0f;
    }

    /** A wave drawn as its outline: the highest and lowest value in every column. */
    void drawOutline (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<float>& v, juce::Colour c)
    {
        const int columns = juce::jmax (1, (int) area.getWidth());
        const float scale = area.getHeight() * 0.48f / 0.5f;
        g.setColour (c);

        for (int x = 0; x < columns; ++x)
        {
            const size_t from = v.size() * (size_t) x / (size_t) columns, to = juce::jmax (from + 1, v.size() * (size_t) (x + 1) / (size_t) columns);
            float low = 0.0f, high = 0.0f;

            for (size_t i = from; i < to; ++i)
            {
                low = juce::jmin (low, v[i]);
                high = juce::jmax (high, v[i]);
            }

            g.fillRect (area.getX() + (float) x, area.getCentreY() - high * scale, 1.0f, juce::jmax (1.0f, (high - low) * scale));
        }
    }

    struct Pictures
    {
        std::vector<float> pluck = makePluck();
        Type pluckType = Type::overdrive;
        float pluckDrive = -1.0f;
        std::array<std::vector<float>, 3> pedals;   // overdrive, distortion, fuzz
        std::vector<float> sustained;
    };

    /** A low power chord, E2 and B2, through the curve: both are whole multiples of 41.2 Hz (2 and 3),
        so one cycle of that repeats exactly and its FFT has no smear. The levels of each multiple, in dB, after the filters. */
    std::vector<float> computePowerChord (const dist::Settings& s, int maxMultiple)
    {
        constexpr int order = 13, n = 1 << order;
        constexpr double base = 82.4 / 2.0;
        std::vector<float> data (2 * n, 0.0f);
        const float drive = juce::Decibels::decibelsToGain (s.driveDb);
        const float low = 0.35f * (float) dist::getGainBefore (s, 2.0 * base), high = 0.35f * (float) dist::getGainBefore (s, 3.0 * base);

        for (int i = 0; i < n; ++i)
        {
            const double phase = juce::MathConstants<double>::twoPi * i / n;
            data[(size_t) i] = dist::distort (s.type, drive, low * (float) std::sin (2.0 * phase) + high * (float) std::sin (3.0 * phase));
        }

        juce::dsp::FFT (order).performFrequencyOnlyForwardTransform (data.data(), true);
        std::vector<float> levels;

        for (int k = 0; k <= maxMultiple; ++k)
            levels.push_back (juce::Decibels::gainToDecibels (2.0f * data[(size_t) k] / n * (float) dist::getGainAfter (s, k * base), -150.0f));

        return levels;
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInDistortion& distortion)
{
    BuiltInDistortion* processor = &distortion;
    auto pictures = std::make_shared<Pictures>();
    std::vector<fxui::Guide::Page> pages;
    auto peakOf = [processor] { return getShownPeak (processor->recentPeak.load()); };

    pages.push_back ({ "More than saturation", "The same idea, pushed all the way",
        "Distortion is saturation pushed further. Both send the sound into a curve that can't follow its peaks (see What is "
        "saturation? for how that makes harmonics). Saturation stays near the bend of the curve and adds warmth you barely "
        "notice; distortion drives the sound far past it, until most of the wave is cut flat. Then you don't just hear warmth: "
        "you hear the sound itself change, into a growl, a crunch, a buzz.\n\n"
        "Distortion grew up with the electric guitar. In the 1950s, players turned small amps all the way up and liked what "
        "they heard; by the 1960s they bought boxes that did it on purpose: the fuzz pedal, then the overdrive and distortion "
        "pedals of the 1970s, and amps built for more gain. Today it is used on almost anything: bass, drums, vocals, synths, "
        "and as an effect on a whole mix.\n\n"
        "Because it goes so far, distortion needs things saturation doesn't. More drive (up to 48 dB here). A low cut before "
        "the curve (tight), so the bass doesn't turn into mud. A tone control and a speaker after it, because the harmonics "
        "of a hard-driven curve reach up into the harsh, fizzy top. And match level, because a distorted sound is so much "
        "louder.\n\n"
        "The picture shows the curve with the drive sweeping from clean to the most it goes, and a note before and after, made "
        "as loud: from a rounded sine to nearly a square.",
        [processor, peakOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = withoutFilters (processor->getSettings());
            const double t = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            s.driveDb = (float) (24.0 + 24.0 * std::sin (t * 0.6));
            drawCaption (g, r, "The drive going up and down: " + dist::formatKnob (Knob::drive, s), dist::getTypeName (s.type));
            auto left = r.removeFromLeft (r.getHeight());
            drawCurve (g, left.reduced (6.0f), s, peakOf(), true);
            r.removeFromLeft (20.0f);
            drawBeforeAfter (g, r, s, peakOf(), 2.5);
            drawLabel (g, "grey: before; red: after, as loud", r.withTop (r.getBottom() - 16.0f));
        } });

    pages.push_back ({ "Five curves", "Overdrive, distortion, fuzz, rectify, fold",
        "DawIt Distortion has five curves, each modelled on a kind of gear.\n\n"
        "Overdrive bends softly, and its bottom half gives in a little sooner than its top. It is modelled on an overdrive "
        "pedal, whose clipping diodes sit in the feedback of an amplifier, so the sound is never fully cut off. Soft and "
        "lopsided, it adds mostly low harmonics, odd and some even, and it follows the playing.\n\n"
        "Distortion is almost flat-topped and symmetric, like a distortion pedal whose diodes clip the signal straight to "
        "ground. It adds strong odd harmonics far up, and every note comes out about as loud.\n\n"
        "Fuzz is hard and lopsided, and biased off-centre like the starved transistors of a fuzz pedal: the wave crosses zero "
        "off-centre, which adds even harmonics, and quiet sounds can't get past the bias at all, so dying notes choke off.\n\n"
        "Rectify flips the bottom half of the wave up. Two bumps per cycle instead of one up and one down: the frequency "
        "doubles, the note goes up an octave.\n\n"
        "Fold doesn't stop at the top: what goes over folds back down, and with more drive it folds again and again. Each "
        "fold adds harmonics, so turning the drive sweeps the sound through changing, vocal timbres.\n\n"
        "The picture shows each curve with 12 dB of drive, a little past the bend, where their shapes differ most, and a note "
        "at -6 dB through it.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = withoutFilters (processor->getSettings());
            s.driveDb = 12.0f;
            drawCaption (g, r, "Each curve at " + dist::formatKnob (Knob::drive, s) + ", a note at -6 dB", "made as loud as the note (grey)");
            const char* about[] = { "soft, lopsided", "hard, even", "hard, biased", "an octave up", "folds back" };
            const float row = r.getHeight() / (float) dist::numTypes;

            for (int t = 0; t < dist::numTypes; ++t)
            {
                s.type = (Type) t;
                auto line = r.withHeight (row).withY (r.getY() + row * (float) t).reduced (0.0f, 3.0f);
                auto text = line.removeFromLeft (120.0f);
                drawLabel (g, dist::getTypeName (s.type), text.removeFromTop (text.getHeight() / 2.0f).withTrimmedTop (2.0f), theme::text);
                drawLabel (g, about[t], text.withTrimmedBottom (2.0f));
                drawCurve (g, line.removeFromLeft (line.getHeight()), s, 0.5f, false);
                line.removeFromLeft (14.0f);
                drawBeforeAfter (g, line, s, 0.5f, 3.0);
            }
        } });

    pages.push_back ({ "Overdrive, distortion, fuzz", "The three pedals, and how they feel",
        "Guitarists sort their pedals into three families, and the difference is as much about feel as about sound.\n\n"
        "Overdrive (the Ibanez Tube Screamer, 1979, and the Klon Centaur) sounds like a tube amp turned up just past clean. It "
        "follows your hands: play softly and it cleans up, dig in and it growls. The soft curve leaves the dynamics mostly "
        "in place. Players often use it as a boost in front of an amp that's already distorting.\n\n"
        "Distortion (the MXR Distortion+ and ProCo RAT around 1978, the Boss DS-1) clips hard, so loud and soft notes come "
        "out nearly the same. That makes it thick, even and compressed: the wall of rock and metal rhythm guitar.\n\n"
        "Fuzz (the Maestro Fuzz-Tone, 1962, the Fuzz Face, 1966, and the Electro-Harmonix Big Muff, 1969) is the oldest and "
        "the most extreme: the note becomes a buzzing square, and as it dies it sputters and chokes off instead of fading. "
        "The Rolling Stones' 'Satisfaction', Hendrix, and later stoner and garage rock.\n\n"
        "The picture shows the same plucked note (A2, dying away like a string) through each, at a drive typical for it, made "
        "to peak at the same height. Watch how much each keeps of the note fading out, and the fuzz choking off at the end.",
        [pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            constexpr float drives[] = { 18.0f, 30.0f, 40.0f };

            if (pictures->pedals[0].empty())
                for (int t = 0; t < 3; ++t)
                    pictures->pedals[(size_t) t] = distortPluck (pictures->pluck, (Type) t, drives[t]);

            drawCaption (g, r, "A plucked note through each, at a typical drive", "made to peak as high");
            const float row = r.getHeight() / 4.0f;
            const std::vector<float>* rows[] = { &pictures->pluck, &pictures->pedals[0], &pictures->pedals[1], &pictures->pedals[2] };
            const juce::String names[] = { "The note", "Overdrive +18", "Distortion +30", "Fuzz +40" };

            for (int i = 0; i < 4; ++i)
            {
                auto line = r.withHeight (row).withY (r.getY() + row * (float) i).reduced (0.0f, 3.0f);
                const auto env = getEnvelopeDb (*rows[i]);
                drawLabel (g, names[i], line.removeFromLeft (110.0f), theme::text);
                auto info = line.removeFromRight (150.0f);
                drawOutline (g, line, *rows[i], i == 0 ? theme::textDim : colour);
                drawLabel (g, "holds " + fx::formatNumber (getSustainSeconds (env), 1) + " s within 12 dB", info.reduced (8.0f, 0.0f));
            }
        } });

    pages.push_back ({ "Sustain and noise", "Why leads sing, and why amps hiss",
        "A plucked string dies away quickly: its level falls by about 20 dB a second. Distortion squashes that: as long as the "
        "string is loud enough to reach the bend of the curve, it comes out nearly as loud as at the start. With 40 dB of drive, "
        "the note has to fall about 40 dB before the output starts to fall. That is sustain: the long, singing notes of a lead "
        "guitar (Carlos Santana, David Gilmour, Gary Moore) come from the distortion holding them up.\n\n"
        "The same thing happens to everything else. Distortion turns up the quiet parts by as much as the drive: the hiss of "
        "the pickups, the hum of the mains, the squeak of fingers on strings. Between the notes, they come out loud. That's "
        "why high-gain guitarists put a noise gate (a switch that closes when the sound falls below a threshold) before or "
        "after the distortion, and why they mute the strings with their hands between riffs.\n\n"
        "For mixing it means: a distorted track has almost no dynamics left, so it fills its space all the time. Leave room for "
        "it with EQ, and don't expect a compressor to do much after it.\n\n"
        "The picture shows the level of a plucked note with a little noise under it, before and after the window's curve and "
        "drive, made to peak as high: the note holds longer, and the noise after it comes up.",
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();

            if (pictures->pluckType != s.type || ! juce::exactlyEqual (pictures->pluckDrive, s.driveDb) || pictures->sustained.empty())
            {
                pictures->sustained = distortPluck (pictures->pluck, s.type, s.driveDb);
                pictures->pluckType = s.type;
                pictures->pluckDrive = s.driveDb;
            }

            const auto before = getEnvelopeDb (pictures->pluck), after = getEnvelopeDb (pictures->sustained);
            drawCaption (g, r, "A plucked note and noise, " + dist::formatKnob (Knob::drive, s) + " " + dist::getTypeName (s.type),
                         "held within 12 dB: " + fx::formatNumber (getSustainSeconds (after), 1) + " s instead of " + fx::formatNumber (getSustainSeconds (before), 1) + " s");
            auto axis = r.removeFromBottom (14.0f).withTrimmedLeft (36.0f);
            auto area = r.withTrimmedLeft (36.0f);
            auto toX = [&] (double seconds) { return area.getX() + area.getWidth() * (float) (seconds / pluckSeconds); };
            auto toY = [&] (float db) { return juce::jmap (juce::jlimit (-80.0f, 0.0f, db), -80.0f, 0.0f, area.getBottom(), area.getY()); };

            for (float db = 0.0f; db >= -80.0f; db -= 20.0f)
            {
                g.setColour (theme::grid);
                g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);

                if (db > -70.0f)   // the bottom line's label would run into the seconds
                    drawLabel (g, juce::String ((int) db) + " dB", juce::Rectangle<float> (34.0f, 12.0f).withPosition (area.getX() - 38.0f, toY (db) - 6.0f),
                               theme::textDim, juce::Justification::centredRight);
            }

            for (int second = 0; second <= 3; ++second)
                drawLabel (g, juce::String (second) + " s", juce::Rectangle<float> (40.0f, 14.0f).withCentre ({ toX (second), axis.getCentreY() }), theme::textDim,
                           juce::Justification::centred);

            auto line = [&] (const std::vector<float>& env, juce::Colour c)
            {
                juce::Path p;

                for (size_t i = 0; i < env.size(); ++i)
                    i == 0 ? p.startNewSubPath (toX (0.01), toY (env[i])) : p.lineTo (toX (0.01 + 0.02 * (double) i), toY (env[i]));

                g.setColour (c);
                g.strokePath (p, juce::PathStrokeType (2.0f));
            };

            line (before, theme::textDim);
            line (after, colour);
            g.setColour (theme::textDim.withAlpha (0.5f));
            g.fillRect (toX (pluckEnds), area.getY(), 1.0f, area.getHeight());
            drawLabel (g, "the note stops", juce::Rectangle<float> (120.0f, 14.0f).withPosition (toX (pluckEnds) + 4.0f, area.getY()));
            const auto noiseText = "noise: " + fx::formatNumber (getNoiseDb (before), 0) + " dB before, " + fx::formatNumber (getNoiseDb (after), 0) + " dB after";
            drawLabel (g, noiseText, juce::Rectangle<float> (area.getRight() - toX (pluckEnds) - 4.0f, 14.0f).withPosition (toX (pluckEnds) + 4.0f, toY (getNoiseDb (after)) - 18.0f),
                       colour);
        } });

    pages.push_back ({ "Octaves and folds", "Rectify and the wavefolder",
        "Two curves don't only cut the peaks off: they change the wave into a different one.\n\n"
        "Rectify takes the bottom half of the wave and flips it up, like the rectifier that turns AC into DC in a power "
        "supply. Where a sine went up once and down once, it now goes up twice: twice the frequency, an octave higher, and the "
        "original note almost disappears (only even harmonics remain). Guitar pedals did this from the 1960s: Roger Mayer "
        "built the Octavia for Jimi Hendrix, heard on the solo of 'Purple Haze' (1967). With chords it gets messy fast, so it "
        "is for single notes, high on the neck; blended with the mix, you hear the note and its octave together.\n\n"
        "Fold lets the wave go past the top and folds it back down, like a sheet of paper folded at the edge. Push harder and "
        "it folds again, and again: every fold makes new harmonics, and they shift as the drive changes, which sounds metallic, "
        "hollow, almost like a voice. Wavefolders come from synthesizers: Don Buchla's West Coast synths of the early 1970s "
        "shaped their sound this way instead of with filters, and modular synths still do.\n\n"
        "The picture shows rectify at the window's drive, and fold with its drive sweeping, each with a note at -6 dB and "
        "its harmonics.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = withoutFilters (processor->getSettings());
            drawCaption (g, r, "Rectify and fold, a note at -6 dB", "white: the note; yellow: even harmonics; red: odd");
            auto left = r.removeFromLeft (r.getWidth() / 2.0f).reduced (6.0f, 0.0f);
            auto right = r.reduced (6.0f, 0.0f);
            const double t = juce::Time::getMillisecondCounterHiRes() / 1000.0;

            for (auto [area, type] : { std::pair { left, Type::rectify }, std::pair { right, Type::fold } })
            {
                s.type = type;
                s.driveDb = type == Type::fold ? (float) (14.0 + 14.0 * std::sin (t * 0.5)) : s.driveDb;
                drawLabel (g, dist::getTypeName (type) + ", " + dist::formatKnob (Knob::drive, s), area.removeFromTop (18.0f), theme::text);
                auto wave = area.removeFromTop (area.getHeight() * 0.4f);
                drawBeforeAfter (g, wave.reduced (30.0f, 4.0f), s, 0.5f, 2.0);
                drawHarmonics (g, area.withTrimmedTop (12.0f), s, 0.5f, 12, false);
            }
        } });

    pages.push_back ({ "Tight: cut the lows first", "Why metal players cut bass before the distortion",
        "Low notes carry far more energy than high ones, so in a full sound they push hardest into the curve. Distorted, they "
        "do two bad things. Their big, slow swings decide when the curve clips, so everything above them is chopped in their "
        "rhythm: the sound gets flubby and loose. And two low notes together make difference tones below them "
        "(intermodulation, see What is saturation?): a low E power chord, E and B, makes a tone an octave below the E, where it "
        "rumbles and muddies the bass.\n\n"
        "Cutting the lows before the curve fixes both: the curve works on the mids, where the guitar's character is, and the "
        "low end stays clean and solid. The Tube Screamer does this (it cuts below about 720 Hz before its diodes), which is "
        "why so many metal players put one in front of an amp that already has plenty of gain: not for more distortion, but "
        "for tighter distortion. The weight comes back from the speaker's low bump and from the harmonics.\n\n"
        "The same idea works on bass: distort only a copy, with its lows cut, and blend it with the clean bass (mix below "
        "100%). Many bass players run a clean and a dirty amp side by side for this.\n\n"
        "The picture shows a low E power chord through the window's curve, without tight and with it: the notes and their "
        "harmonics in white and yellow, the new difference tones in red.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.driveDb = juce::jmax (24.0f, s.driveDb);
            const float tight = dist::isTightOn (s.tightHz) ? s.tightHz : 300.0f;
            constexpr double base = 82.4 / 2.0;
            constexpr int maxMultiple = 48;   // up to 2 kHz
            drawCaption (g, r, "A low E power chord (E2 and B2) through the curve, " + dist::formatKnob (Knob::drive, s) + " " + dist::getTypeName (s.type),
                         "white: the notes; yellow: their harmonics; red: new tones");
            auto axis = r.removeFromBottom (14.0f).withTrimmedLeft (170.0f);
            const float row = r.getHeight() / 2.0f;

            for (int i = 0; i < 2; ++i)
            {
                s.tightHz = i == 0 ? dist::minTight : tight;
                const auto levels = computePowerChord (s, maxMultiple);
                const float loudest = juce::jmax (levels[2], levels[3]);
                auto line = r.withHeight (row).withY (r.getY() + row * (float) i).reduced (0.0f, 5.0f);
                drawLabel (g, i == 0 ? "Tight off" : "Tight at " + dist::formatKnob (Knob::tight, s), line.withWidth (165.0f).withHeight (line.getHeight() / 2.0f), theme::text);
                drawLabel (g, "41 Hz below the chord: " + fx::formatNumber (levels[1] - loudest, 0) + " dB", line.withWidth (165.0f).withTrimmedTop (line.getHeight() / 2.0f));
                line.removeFromLeft (170.0f);

                std::vector<std::pair<double, float>> lines;

                for (int k = 1; k <= maxMultiple; ++k)
                    lines.push_back ({ k * base, levels[(size_t) k] - loudest });

                drawLines (g, line, lines, 2000.0, [] (double hz)
                {
                    const int k = juce::roundToInt (hz / base);
                    if (k == 2 || k == 3) return theme::text;
                    if (k % 2 == 0 || k % 3 == 0) return evenColour;
                    return colour;
                });
            }

            for (const int hz : { 0, 500, 1000, 1500, 2000 })
                drawLabel (g, juce::String (hz) + " Hz", juce::Rectangle<float> (60.0f, 14.0f).withCentre ({ juce::jmin (axis.getRight() - 30.0f, axis.getX() + axis.getWidth() * (float) hz / 2000.0f),
                                                                                                          axis.getCentreY() }),
                           theme::textDim, hz == 2000 ? juce::Justification::centredRight : juce::Justification::centred);
        } });

    pages.push_back ({ "Tone and the speaker", "Where the fizz goes",
        "Plug a distortion pedal straight into a mixing desk and it sounds thin and fizzy, like a swarm of bees. Through a guitar "
        "amp it sounds warm and full. The difference is mostly the speaker.\n\n"
        "A guitar speaker (usually 12 inches, in a closed or open wooden cabinet) is a poor speaker on purpose: it can't move "
        "enough air below about 90 Hz, its cone rings in the upper mids where it starts to break up (around 2-3 kHz), and above "
        "about 5 kHz it plays almost nothing. All the harsh high harmonics the distortion makes are simply not played. When "
        "guitars are recorded, a microphone is put in front of that speaker (a Shure SM57, close to the cone, is the classic), "
        "so the recording has the speaker's sound in it.\n\n"
        "When a guitar is recorded straight into the computer (DI), an amp simulator or an impulse response of a cabinet does "
        "the same job afterwards. DawIt Distortion's speaker is a simple version: a low cut with a bump, a dip in the low mids, "
        "the cone's bump, and a steep high cut.\n\n"
        "The tone knob is the pedal's own answer: a high cut after the curve. Pedals have one because hard clipping without it "
        "is too bright; the Big Muff's tone even crossfades between a low-pass and a high-pass, leaving a dip in the mids.\n\n"
        "The picture shows the filters at the window's settings with the speaker on, and the harmonics of a note at -6 dB "
        "without any filter after the curve and with them.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.speaker = true;
            drawCaption (g, r, "The speaker on, tone " + dist::formatKnob (Knob::tone, s), dist::formatKnob (Knob::drive, s) + " " + dist::getTypeName (s.type));
            auto left = r.removeFromLeft (r.getWidth() * 0.42f);
            drawFilters (g, left.reduced (4.0f, 0.0f), s, true);
            r.removeFromLeft (16.0f);
            const float half = r.getHeight() / 2.0f;

            for (int i = 0; i < 2; ++i)
            {
                auto cell = r.withHeight (half).withY (r.getY() + half * (float) i).reduced (0.0f, 4.0f);
                auto shown = s;

                if (i == 0)
                {
                    shown.toneHz = dist::maxTone;
                    shown.speaker = false;
                }

                drawLabel (g, i == 0 ? "A note at -6 dB without the tone and speaker: 64 harmonics, up to 14 kHz" : "With them", cell.removeFromTop (16.0f), theme::text);
                drawHarmonics (g, cell, shown, 0.5f, 64, false);
            }
        } });

    pages.push_back ({ "Distortion in a mix", "When and how it's used",
        "Distortion is a sound of its own more than a fix, so the first question is whether the part should sound distorted.\n\n"
        "When it's used:\n\n"
        "- Rhythm guitar: crunch for rock, high gain for metal, with tight and the speaker on. Use less gain than feels right "
        "alone: in a mix, a cleaner sound is bigger and clearer. Recording the part twice and panning the takes left and right "
        "makes the wall.\n"
        "- Lead guitar: more gain for sustain, overdrive for a smoother, more vocal sound.\n"
        "- Bass: in parallel, with the lows cut before the curve, so the clean bass keeps the low end and the distortion adds "
        "growl that cuts through the guitars and is heard on small speakers.\n"
        "- Drums: a smashed copy blended under the clean kit (or on the room mics) for size and aggression.\n"
        "- Vocals: telephone, megaphone or radio voices for an intro, one line or a backing part; aggressive vocals in rock, "
        "industrial and hip-hop.\n"
        "- Synths and 808s: fold and rectify for new timbres; a distorted 808 bass is heard on phones, where a clean one "
        "disappears.\n"
        "- As an effect: on a break, a build-up, or the whole mix for a moment; automate the mix knob.\n\n"
        "How it's used:\n\n"
        "- Compare at the same loudness (match level): distortion makes everything louder, and louder always wins.\n"
        "- Order: EQ or tight before it shapes what gets distorted; EQ after shapes the result. Distortion goes before delay "
        "and reverb, as on a pedalboard: after them, the echoes and the reverb tail get distorted into mush (unless that's "
        "the point).\n"
        "- A noise gate (DawIt Gate) before or after it, or tidy edits, to keep the hiss between notes down.\n"
        "- Power chords and single notes distort cleanly; full chords turn muddy (intermodulation).\n"
        "- Two moderate stages (an overdrive into a crunchy amp) often sound better than one extreme one.\n"
        "- Distorted parts have no dynamics left: make room for them with EQ instead of turning them up.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto& presets = dist::getPresets();
            drawCaption (g, r, "Four starting points", "what each makes of a note at -6 dB, filters included");
            const char* names[] = { "Rock rhythm", "Bass grit", "Vintage fuzz", "Synth folds" };
            const float w = r.getWidth() / 2.0f, h = r.getHeight() / 2.0f;

            for (int i = 0; i < 4; ++i)
            {
                const auto preset = std::find_if (presets.begin(), presets.end(), [&] (auto& p) { return p.name == names[i]; });

                if (preset == presets.end())
                    continue;

                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 2), r.getY() + h * (float) (i / 2), w, h).reduced (8.0f, 4.0f);
                drawLabel (g, preset->name + ": " + dist::formatKnob (Knob::drive, preset->settings) + " " + dist::getTypeName (preset->settings.type),
                           cell.removeFromTop (16.0f), theme::text);
                drawHarmonics (g, cell, preset->settings, 0.5f, 16, false);
            }
        } });

    pages.push_back ({ "How DawIt Distortion works", "From the clean sound to the distorted one",
        "1. Tight: a low cut (12 dB per octave) on the sound going into the curve. The clean sound keeps its lows.\n\n"
        "2. Oversampling: the sound is raised to four times the sample rate with linear-phase filters, so there is room for the "
        "harmonics the curve makes. Distortion makes far more of them than saturation, and fold and fuzz make very high ones, "
        "which would otherwise fold back down as tones that don't belong (aliasing, see What is saturation?).\n\n"
        "3. Drive and curve: every sample is turned up by the drive and put through the curve: x / (1 + |x|^p)^(1/p), a curve "
        "that bends more sharply the higher p is (2 for overdrive, 6 for distortion, 4 for fuzz), with the bottom half "
        "flattening sooner for overdrive and fuzz; the fuzz's input is shifted by its bias first. Rectify uses the overdrive "
        "curve on the wave flipped up; fold is a sine, which goes back down past its top.\n\n"
        "4. Back down: the filters remove everything above the original half sample rate, then the sound goes back to the "
        "original rate. The dry sound is held back as long, so the two stay in step when mixed.\n\n"
        "5. Clean up and colour: a 10 Hz high-pass removes the DC offset of the lopsided curves, then the tone's high cut (12 dB "
        "per octave) and the speaker (a low cut at 85 Hz with a bump, a dip at 450 Hz, a bump at 2.5 kHz and a 24 dB per "
        "octave high cut at 5 kHz).\n\n"
        "6. What it adds: a clean copy of the input, through the same filters, fitted to the level of the output, is "
        "subtracted from it, for Hear only what it adds and the graph.\n\n"
        "7. Mix, output and match level: the clean and distorted sound are blended, the output gain applied, and match level "
        "turns the result to the loudness of the input over the last half second.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "From the clean sound to the distorted one", {});
            constexpr int columns = 8;
            const float gap = 10.0f, col = (r.getWidth() - gap * (columns - 1)) / (float) columns, midY = r.getCentreY() - 10.0f;
            auto box = [&] (int column) { return juce::Rectangle<float> (r.getX() + (col + gap) * (float) column, midY - 30.0f, col, 60.0f); };

            const juce::Rectangle<float> boxes[] = {
                drawBox (g, box (0), "In", "clean", theme::textDim),
                drawBox (g, box (1), "Tight", "low cut", beforeColour),
                drawBox (g, box (2), "Up 4x", "room", theme::accent),
                drawBox (g, box (3), "Curve", "drive, type", colour),
                drawBox (g, box (4), "Down 4x", "filter", theme::accent),
                drawBox (g, box (5), "Tone", "DC, high cut", colour),
                drawBox (g, box (6), "Speaker", "if on", colour),
                drawBox (g, box (7), "Mix", "output, match", theme::accent)
            };

            for (int i = 0; i + 1 < columns; ++i)
                arrow (g, { boxes[i].getRight(), midY }, { boxes[i + 1].getX(), midY });

            juce::Path dry;
            const float top = midY - 70.0f;
            dry.startNewSubPath (boxes[0].getCentreX(), boxes[0].getY());
            dry.lineTo (boxes[0].getCentreX(), top);
            dry.lineTo (boxes[7].getCentreX(), top);
            g.setColour (theme::textDim);
            g.strokePath (dry, juce::PathStrokeType (1.2f));
            g.drawArrow ({ boxes[7].getCentreX(), top, boxes[7].getCentreX(), boxes[7].getY() }, 1.2f, 7.0f, 7.0f);
            drawLabel (g, "the clean sound, lows and all, held back as long as the filters", juce::Rectangle<float> (boxes[7].getCentreX() - boxes[0].getCentreX(), 14.0f)
                                                                                                    .withPosition (boxes[0].getCentreX(), top - 16.0f), theme::textDim, juce::Justification::centred);
        } });

    return pages;
}
} // namespace daw::disttext
