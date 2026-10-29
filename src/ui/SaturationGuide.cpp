#include "SaturationGuide.h"

namespace daw::sattext
{
using sat::Knob;
using sat::Type;

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
}

juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::drive:  return "Drive";
        case (int) Knob::type:   return "Type: tape, tube or clip";
        case (int) Knob::tone:   return "Tone";
        case (int) Knob::output: return "Output";
        case (int) Knob::mix:    return "Mix";
        case curve:              return "The curve";
        case harmonics:          return "The harmonics it adds";
        case match:              return "Match level";
        case fxui::overview:     return "DawIt Saturation";
        case fxui::presets:      return "Starting points";
        case fxui::clap:         return "Play a note";
        case fxui::listen:       return "Hear only what it adds";
        case fxui::guide:        return "What is saturation?";
        case fxui::history:      return "The last 4 seconds";
        default:                 return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::drive:
            return "How hard the sound is pushed into the curve: it is turned up by this much before it goes in. The curve passes "
                   "small values unchanged and rounds off big ones, so more drive means more of the wave gets rounded: a little "
                   "warmth at first, then audible grit, then a buzzing fuzz.\n\n"
                   "How much happens also depends on how loud the track already is: the same drive does more to a loud track than "
                   "to a quiet one. That's why a saturator early in a chain behaves differently from one after a compressor, and "
                   "why turning a track up before it changes its sound.";
        case (int) Knob::type:
            return "The shape of the curve.\n\n"
                   "Tape rounds off both halves of the wave the same way, softly, like magnetic tape that is recorded too hot. A "
                   "symmetric curve adds only odd harmonics (3rd, 5th, 7th...), quieter the higher they go: smooth and dense.\n\n"
                   "Tube rounds off one half sooner than the other, like a tube amplifier. A lopsided curve adds even harmonics "
                   "too, the octave above the note and the octave and fifth: notes of the chord itself, which is why it sounds "
                   "warm and full rather than distorted.\n\n"
                   "Clip cuts the peaks off flat, like a transistor or a converter running out of room. The sharp corners make "
                   "many odd harmonics that stay loud high up: bright, aggressive, fuzzy.";
        case (int) Knob::tone:
            return "A high cut after the curve. Saturation adds harmonics up to the very top of the range we hear, and those can "
                   "sound fizzy and harsh, especially with hard clipping. Lowering the tone keeps the warmth of the low harmonics "
                   "and takes the fizz away; guitar amplifiers do this too, as their speakers hardly play above 5 kHz.";
        case (int) Knob::output:
            return "The level after everything else. With match level on, it is a trim on top of the matched level; off, use it to "
                   "turn the saturated sound back down, because driving the curve makes quiet sounds a lot louder.";
        case (int) Knob::mix:
            return "How much of the saturated sound you hear against the clean one. Below 100% it's parallel saturation: the clean "
                   "sound keeps its attack, its low end and its clarity, and the saturated sound underneath adds harmonics and "
                   "density. Drums and bass often sound best this way: driven hard, but blended in.";
        case match:
            return "Keeps the sound as loud coming out as going in, by turning it down by as much as the saturation turned it up, "
                   "measured over the last half second. Louder almost always sounds better to the ear, so without this you can't "
                   "tell whether the saturation makes the sound better or just louder. Switch the effect off and on with match "
                   "level on to compare honestly.";
        case curve:
            return "What comes out (up) for every value that goes in (left to right), over the whole range a sample can have, -1 "
                   "to 1. The dashed line is no change. Near the middle the curve follows it: small values pass unchanged. Further "
                   "out it bends away: that's where the peaks are rounded off. The thick part is the stretch the track uses, up to "
                   "its loudest peak, so you can see whether the sound reaches the bend.\n\n"
                   "Under it, a sine at the track's level before (grey) and after (orange) the curve, made as loud, so only the "
                   "change in shape shows. Drag up and down on the curve for the drive.";
        case harmonics:
            return "What the curve turns one pure note into: the note itself (1) and the harmonics it adds, at whole-number "
                   "multiples of its frequency, as loud as they come out against the note. Yellow ones are even (an octave, two "
                   "octaves, the octave and fifth above that), orange ones odd. Under each, what it is musically.\n\n"
                   "THD (total harmonic distortion) adds all the harmonics up against the note: below 1% you hardly hear it, "
                   "around 3-10% is clear warmth, and fuzz is far over 30%.";
        case fxui::presets:
            return "Settings for common uses, each explaining why every knob is set the way it is. They are a start, not an "
                   "answer: play the track and adjust by ear, with match level on.";
        case fxui::clap:
            return "Plays a plain A (220 Hz, a sine with no harmonics of its own) through the saturation, so everything you hear "
                   "besides that one pure note is what the saturation adds. It works while the song is stopped.";
        case fxui::listen:
            return "Plays only what the saturation adds: the output minus a clean copy of the input at the same level. What's left "
                   "is the harmonics (and the change in dynamics), on their own. Turn up the drive and listen to them grow; switch "
                   "the type and hear their character change.";
        case fxui::guide:
            return "A guide to saturation and distortion, for beginners: why pushing a sound too far changes its shape, harmonics, "
                   "even and odd, soft and hard, tape, tubes and transistors, chords, loudness, aliasing, using it in a mix, and "
                   "how DawIt Saturation works. The pictures follow the knobs here.";
        case fxui::history:
            return "The last 4 seconds while the track plays: grey the track, orange what the saturation adds, as loud as it is "
                   "mixed. The closer the orange comes to the grey, the more distorted the sound. Loud notes and hits add the "
                   "most, because they reach furthest into the curve.";
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::drive:
            return "2-6 dB: subtle density on a mix or a bus.\n"
                   "6-12 dB: warmth on vocals, keys and synths.\n"
                   "12-24 dB: audible grit on bass, drums, guitars.\n"
                   "24 dB and more: distortion and fuzz.";
        case (int) Knob::type:
            return "Tape: mixes, buses, drums, anything that should just sound denser.\n"
                   "Tube: vocals, bass, keys, synths: warmth.\n"
                   "Clip: drums in parallel, guitars, effects.";
        case (int) Knob::tone:
            return "Off: subtle saturation.\n"
                   "8-12 kHz: vocals and synths, to smooth the top.\n"
                   "3-6 kHz: guitar fuzz, lo-fi, bass harmonics.";
        case (int) Knob::output:
            return "0 dB with match level on.\n"
                   "Down by about as much as the drive with it off, then by ear.";
        case (int) Knob::mix:
            return "100%: most uses.\n"
                   "30-60%: parallel saturation on drums and bass.";
        default:
            return {};
    }
}

juce::String describeSettings (int t, const sat::Settings& s, float peak)
{
    const auto levels = sat::getHarmonicsDb (s, peak, 12);
    const float thd = sat::getThdPercent (levels);
    const auto thdText = fx::formatNumber (thd, thd < 10.0f ? 1 : 0) + "%";

    switch (t)
    {
        case (int) Knob::drive:
        {
            const float pushed = juce::Decibels::gainToDecibels (peak) + s.driveDb;
            return sat::formatKnob (Knob::drive, s) + ": the loudest peak (" + formatPeak (peak) + ") goes into the curve at "
                   + formatDb (pushed) + (pushed < -6.0f ? ", where the curve is still nearly straight." : pushed < 3.0f ? ", where it starts to bend."
                                                                                                         : ", well into the bend.")
                   + " The harmonics add up to " + thdText + " of the note (THD).";
        }

        case (int) Knob::type:
        {
            const float second = levels[1] - levels[0], third = levels[2] - levels[0];
            return sat::formatKnob (Knob::type, s) + ": on a sine at " + formatPeak (peak) + ", the octave (2nd harmonic) comes out "
                   + (second < -100.0f ? juce::String ("not at all") : formatDb (second)) + " and the octave and fifth (3rd) " + formatDb (third)
                   + " against the note.";
        }

        case (int) Knob::tone:
            if (! sat::isToneOn (s.toneHz))
                return "Off: every harmonic the curve makes is heard.";

            return sat::formatKnob (Knob::tone, s) + ": harmonics above it are turned down, 12 dB for every octave further up.";

        case (int) Knob::output:
            return sat::formatKnob (Knob::output, s) + ".";

        case (int) Knob::mix:
            return sat::formatKnob (Knob::mix, s) + (s.mixPercent > 99.5f ? ": only the saturated sound." : ": the clean sound with the saturated one blended in.");

        case curve:
            return "The track peaks at " + formatPeak (peak) + "; driven " + sat::formatKnob (Knob::drive, s) + ", it reaches "
                   + formatDb (juce::Decibels::gainToDecibels (peak) + s.driveDb) + " into the curve.";

        case harmonics:
            return "THD " + thdText + " on a sine at " + formatPeak (peak) + ".";

        default:
            return {};
    }
}

//==============================================================================
juce::Rectangle<float> drawCurve (juce::Graphics& g, juce::Rectangle<float> bounds, const sat::Settings& s, float peak, bool labels)
{
    const float range = s.type == Type::tube ? 1.6f : 1.2f;
    auto area = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()), juce::jmin (bounds.getWidth(), bounds.getHeight()));
    auto toX = [&] (float x) { return juce::jmap (x, -1.0f, 1.0f, area.getX(), area.getRight()); };
    auto toY = [&] (float y) { return juce::jmap (juce::jlimit (-range, range, y), -range, range, area.getBottom(), area.getY()); };
    const float drive = juce::Decibels::decibelsToGain (s.driveDb);

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

    auto path = [&] (float from, float to)
    {
        juce::Path p;
        const int steps = 300;

        for (int i = 0; i <= steps; ++i)
        {
            const float x = from + (to - from) * (float) i / (float) steps;
            const float y = toY (sat::shape (s.type, drive * x));
            i == 0 ? p.startNewSubPath (toX (x), y) : p.lineTo (toX (x), y);
        }

        return p;
    };

    g.setColour (colour.withAlpha (0.5f));
    g.strokePath (path (-1.0f, 1.0f), juce::PathStrokeType (1.5f));
    g.setColour (colour);
    g.strokePath (path (-juce::jmin (1.0f, peak), juce::jmin (1.0f, peak)), juce::PathStrokeType (3.5f));

    for (const float x : { -peak, peak })
        if (std::abs (x) <= 1.0f)
            g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ toX (x), toY (sat::shape (s.type, drive * x)) }));

    if (labels)
    {
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (theme::textDim);
        g.drawText ("in", area.withTop (toY (0.0f) + 2.0f).withHeight (12.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
        g.drawText ("out", area.withX (toX (0.0f) + 4.0f).withHeight (12.0f), juce::Justification::centredLeft);
        g.drawText ("no change", juce::Rectangle<float> (70.0f, 12.0f).withPosition (toX (0.55f), toY (0.55f) - 2.0f), juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("the track: up to " + formatPeak (peak), area.withTop (area.getBottom() - 14.0f).reduced (4.0f, 0.0f), juce::Justification::centredRight);
    }

    return area;
}

void drawBeforeAfter (juce::Graphics& g, juce::Rectangle<float> area, const sat::Settings& s, float peak, double cycles)
{
    constexpr int steps = 400;
    const float drive = juce::Decibels::decibelsToGain (s.driveDb);
    std::vector<float> before (steps + 1), after (steps + 1);
    double powerBefore = 0.0, powerAfter = 0.0, mean = 0.0;

    for (int i = 0; i <= steps; ++i)
    {
        before[(size_t) i] = peak * (float) std::sin (juce::MathConstants<double>::twoPi * cycles * i / steps);
        after[(size_t) i] = sat::shape (s.type, drive * before[(size_t) i]);
        mean += after[(size_t) i] / (steps + 1);
    }

    for (int i = 0; i <= steps; ++i)
    {
        after[(size_t) i] -= (float) mean;   // without its DC, as DawIt Saturation removes it
        powerBefore += before[(size_t) i] * before[(size_t) i];
        powerAfter += after[(size_t) i] * after[(size_t) i];
    }

    const float sameLoudness = powerAfter > 0.0 ? (float) std::sqrt (powerBefore / powerAfter) : 1.0f;
    const float scale = area.getHeight() * 0.42f / juce::jmax (0.05f, peak * 1.4f);
    auto line = [&] (const std::vector<float>& v, float gain, juce::Colour c, float thickness)
    {
        juce::Path p;

        for (int i = 0; i <= steps; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / steps, y = area.getCentreY() - v[(size_t) i] * gain * scale;
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

void drawHarmonics (juce::Graphics& g, juce::Rectangle<float> bounds, const sat::Settings& s, float peak, int count, bool labels)
{
    const auto levels = sat::getHarmonicsDb (s, peak, count);
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
        const float db = levels[(size_t) k - 1] - levels[0];
        const auto cell = juce::Rectangle<float> (area.getX() + column * (float) (k - 1), area.getY(), column, area.getHeight()).reduced (column * 0.18f, 0.0f);
        const auto c = k == 1 ? theme::text : k % 2 == 0 ? evenColour : colour;

        if (db > bottomDb)
        {
            g.setColour (c);
            g.fillRect (cell.withTop (toY (db)));
        }

        g.setColour (theme::textDim);
        g.drawText (juce::String (k), juce::Rectangle<float> (column, 12.0f).withPosition (cell.getCentreX() - column / 2.0f, area.getBottom() + 2.0f),
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
        const float thd = sat::getThdPercent (levels);
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::text);
        g.drawText ("THD " + fx::formatNumber (thd, thd < 10.0f ? 1 : 0) + "%, on a sine at " + formatPeak (peak), bounds.withHeight (14.0f), juce::Justification::centredRight);
        g.setColour (evenColour);
        g.drawText ("even", bounds.withHeight (14.0f).withTrimmedLeft (30.0f), juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("odd", bounds.withHeight (14.0f).withTrimmedLeft (66.0f), juce::Justification::centredLeft);
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
            g.drawFittedText (detail, box.withTrimmedTop (box.getHeight() / 2.0f - 2.0f).reduced (4.0f, 2.0f).toNearestInt(), juce::Justification::centredTop, 2);
        }

        return box;
    }

    void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow ({ from, to }, 1.2f, 7.0f, 7.0f);
    }

    /** The spectrum of two notes, A2 and one at `high`/`low` times its frequency, through the curve. Both
        are whole multiples of a base frequency, so the result repeats exactly and its FFT has no smear. */
    std::vector<float> computeTwoNotes (const sat::Settings& s, int low, int high, int maxMultiple)
    {
        constexpr int order = 14, n = 1 << order;
        std::vector<float> data (2 * n, 0.0f);
        const float drive = juce::Decibels::decibelsToGain (s.driveDb);

        for (int i = 0; i < n; ++i)
        {
            const double phase = juce::MathConstants<double>::twoPi * i / n;
            data[(size_t) i] = sat::shape (s.type, drive * 0.35f * (float) (std::sin (low * phase) + std::sin (high * phase)));
        }

        juce::dsp::FFT (order).performFrequencyOnlyForwardTransform (data.data(), true);
        std::vector<float> levels;

        for (int k = 0; k <= maxMultiple; ++k)
            levels.push_back (juce::Decibels::gainToDecibels (2.0f * data[(size_t) k] / n, -150.0f));

        return levels;
    }

    /** A spectrum from 0 to `topHz`, with lines for every frequency `levels` holds (dB), coloured by what they are. */
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
            g.fillRect (x - 1.0f, toY (db), 2.5f, area.getBottom() - toY (db));
        }
    }

    struct Pictures
    {
        sat::Settings twoNotesFor, aliasingFor;
        bool hasTwoNotes = false, hasAliasing = false;
        std::array<std::vector<float>, 3> twoNotes;
        std::vector<std::pair<double, float>> naive, oversampled;
    };

    /** A sine at 9.1 kHz through the curve at 48 kHz: once straight, once through DawIt Saturation's oversampling. */
    void computeAliasing (Pictures& p, const sat::Settings& settings)
    {
        constexpr double rate = 48000.0, hz = 9100.0;
        constexpr int order = 13, n = 1 << order, total = 3 * n;
        auto s = settings;
        s.driveDb = juce::jmax (18.0f, s.driveDb);
        s.toneHz = sat::maxTone;
        s.mixPercent = 100.0f;
        s.outputDb = 0.0f;

        juce::AudioBuffer<float> naive (1, total), through (2, total);

        for (int i = 0; i < total; ++i)
        {
            const float x = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / rate);
            naive.setSample (0, i, sat::shape (s.type, juce::Decibels::decibelsToGain (s.driveDb) * x));
            through.setSample (0, i, x);
            through.setSample (1, i, x);
        }

        BuiltInSaturation plugin;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        plugin.prepareToPlay (rate, total);
        juce::MidiBuffer midi;
        plugin.processBlock (through, midi);

        auto spectrum = [&] (const float* samples)
        {
            std::vector<float> data (2 * n, 0.0f);
            std::copy (samples + total - n, samples + total, data.begin());
            juce::dsp::WindowingFunction<float> (n, juce::dsp::WindowingFunction<float>::hann, false).multiplyWithWindowingTable (data.data(), n);
            juce::dsp::FFT (order).performFrequencyOnlyForwardTransform (data.data(), true);
            std::vector<std::pair<double, float>> lines;
            float loudest = -200.0f;

            for (int k = 1; k < n / 2; ++k)
                loudest = juce::jmax (loudest, juce::Decibels::gainToDecibels (data[(size_t) k] + 1.0e-9f));

            // The peaks only, as lines.
            for (int k = 2; k < n / 2 - 1; ++k)
                if (data[(size_t) k] > data[(size_t) k - 1] && data[(size_t) k] >= data[(size_t) k + 1])
                    lines.push_back ({ k * rate / n, juce::Decibels::gainToDecibels (data[(size_t) k] + 1.0e-9f) - loudest });

            return lines;
        };

        p.naive = spectrum (naive.getReadPointer (0));
        p.oversampled = spectrum (through.getReadPointer (0));
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInSaturation& saturation)
{
    BuiltInSaturation* processor = &saturation;
    auto pictures = std::make_shared<Pictures>();
    std::vector<fxui::Guide::Page> pages;
    auto peakOf = [processor] { return getShownPeak (processor->recentPeak.load()); };
    auto sixDb = [] { return 0.5f; };   // the pages that compare the types use one level, so they compare alike

    pages.push_back ({ "Pushed too far", "When the gear runs out of room",
        "Every piece of audio gear has a limit to how big a signal it can pass: a tape can only hold so much magnetism, a tube or "
        "transistor amplifier can only swing its voltage so far, and a digital file can't store a number above full scale (0 dB). "
        "Below the limit the sound passes unchanged. Push it past, and the peaks of the wave can't follow: they get rounded off, or "
        "cut off flat.\n\n"
        "That is saturation, and pushed further, distortion. The word saturation comes from tape: when it's recorded too loud, the "
        "magnetic particles are saturated and can't take more. It is the same thing as the overdrive of a guitar amp and the fuzz of "
        "a fuzz pedal, only gentler.\n\n"
        "The picture shows it with a curve: what comes out (up) for what goes in (across). Where the curve follows the dashed line, "
        "nothing changes. Where it bends, the peaks are squashed. The drive turns the sound up into the bend, so the more drive, the "
        "more of the wave gets squashed. Watch the sine under it change shape as the drive goes up and down, kept at the same "
        "loudness so only the shape changes.\n\n"
        "Engineers have done this on purpose since the 1950s: recording drums onto tape a little too hot, pushing a console's "
        "preamps, or turning a guitar amp all the way up. A little makes a sound fuller and warmer; a lot makes it aggressive.",
        [processor, peakOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            const double t = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            s.driveDb = (float) (15.0 + 15.0 * std::sin (t * 0.8));
            drawCaption (g, r, "The drive going up and down: " + sat::formatKnob (Knob::drive, s), sat::getTypeName (s.type));
            auto left = r.removeFromLeft (r.getHeight());
            drawCurve (g, left.reduced (6.0f), s, peakOf(), true);
            r.removeFromLeft (20.0f);
            drawBeforeAfter (g, r, s, peakOf(), 2.5);
            drawLabel (g, "grey: before; orange: after, as loud", r.withTop (r.getBottom() - 16.0f));
        } });

    pages.push_back ({ "Harmonics", "A new shape is new frequencies",
        "Why does squashing a wave change its sound, and not just its level? Because a wave's shape is its timbre. The sound guide's "
        "topic 'The shape is the timbre' shows how every repeating wave is made of sine waves at whole-number multiples of its "
        "frequency: the harmonics. A pure sine has only the first. Round off its peaks, and the new shape needs other harmonics to "
        "make it: the curve adds them.\n\n"
        "The added harmonics are all notes related to the original: twice the frequency is an octave up, three times an octave and a "
        "fifth, four times two octaves, five times two octaves and a major third, then the fifth again, then a slightly flat minor "
        "seventh. The low ones are notes of the chord itself, so they blend in and make the sound richer and fuller rather than "
        "out of tune. Higher up they crowd together and get more dissonant, which is where saturation starts to sound harsh.\n\n"
        "That's why saturation makes things sound louder, closer and more present at the same level: the extra harmonics fill in "
        "the spectrum, especially in the range the ear is most sensitive to (2-5 kHz).\n\n"
        "The picture shows the harmonics the curve makes of a sine at the track's level (or -6 dB while it's quiet). Change the drive "
        "and type in the window and watch them. THD (total harmonic distortion) adds them up against the note.",
        [processor, peakOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "What a pure note turns into", sat::formatKnob (Knob::drive, s) + ", " + sat::getTypeName (s.type));
            auto wave = r.removeFromBottom (r.getHeight() * 0.3f);
            drawHarmonics (g, r, s, peakOf(), 12, true);
            drawBeforeAfter (g, wave.reduced (30.0f, 4.0f), s, peakOf(), 3.0);
        } });

    pages.push_back ({ "Even and odd", "Why tubes sound warm",
        "Which harmonics a curve adds depends on its shape.\n\n"
        "A symmetric curve treats the top and bottom of the wave the same: both are rounded alike. The result is still symmetric, "
        "and a symmetric wave can only contain odd harmonics: the 3rd (an octave and a fifth up), the 5th (two octaves and a major "
        "third), the 7th... Tape and most transistor circuits work this way. Odd harmonics sound denser, edgier, more 'hollow' or "
        "'reedy', like a clarinet, whose sound is mostly odd harmonics.\n\n"
        "A lopsided (asymmetric) curve treats them differently: one half is squashed sooner than the other. That adds even "
        "harmonics too: the 2nd (an octave up) and the 4th (two octaves up) are the note itself, higher, and the 6th is its "
        "fifth. Single-ended tube circuits are lopsided like this, which is why tube gear is called warm: its main addition is the "
        "octave, which makes a sound fuller without making it sound distorted.\n\n"
        "A lopsided curve also shifts the middle of the wave up or down a little (a DC offset). DawIt Saturation filters that out.\n\n"
        "The picture shows the same note through Tape and through Tube at the window's drive: Tape's even bars are missing.",
        [processor, sixDb] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.driveDb = juce::jmax (12.0f, s.driveDb);
            drawCaption (g, r, "Tape (symmetric) and Tube (lopsided), a sine at -6 dB, " + sat::formatKnob (Knob::drive, s), "yellow: even harmonics");
            auto left = r.removeFromLeft (r.getWidth() / 2.0f).reduced (6.0f, 0.0f);
            auto right = r.reduced (6.0f, 0.0f);

            for (auto [area, type] : { std::pair { left, Type::tape }, std::pair { right, Type::tube } })
            {
                s.type = type;
                auto top = area.removeFromTop (18.0f);
                drawLabel (g, sat::getTypeName (type), top, theme::text);
                auto wave = area.removeFromBottom (70.0f);
                drawHarmonics (g, area, s, sixDb(), 8, false);
                drawBeforeAfter (g, wave.reduced (30.0f, 4.0f), s, sixDb(), 2.0);
            }
        } });

    pages.push_back ({ "Soft and hard", "Rounded off or cut off",
        "How sharply a curve bends matters as much as its symmetry.\n\n"
        "A soft curve (tape, tube) bends gradually: quiet parts pass clean, louder ones are squashed a little, the loudest more. The "
        "harmonics it adds fall away quickly the higher they go, so the result is smooth. Because the curve is gentle, it also "
        "behaves like a fast compressor on the peaks.\n\n"
        "A hard curve (clip) does nothing until the limit, then cuts everything above it off flat. The corners where it cuts are "
        "sharp, and sharp corners need many high harmonics to make: they fall off slowly, as 1/n, so even the 15th or 21st "
        "harmonic is loud. That sounds bright, buzzy and aggressive: a fuzz pedal, or digital clipping.\n\n"
        "Digital clipping is the hardest of all: a file can't hold a value above 0 dB, so anything over is cut off flat. Unlike "
        "analog clipping it also folds its high harmonics back into the audible range (see Aliasing), which is why a clipping "
        "master sounds so nasty and why gain staging keeps the levels below 0 dB.\n\n"
        "The picture shows the three curves at the window's drive, and how loud each one's harmonics are, up to the 15th. The hard "
        "clip's stay high.",
        [processor, sixDb] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.driveDb = juce::jmax (12.0f, s.driveDb);
            drawCaption (g, r, "Three curves at " + sat::formatKnob (Knob::drive, s), "the harmonics of a sine at -6 dB, 1 to 15");
            auto curves = r.removeFromLeft (r.getHeight());
            const juce::Colour colours[] = { colour, evenColour, juce::Colour (0xffff5c8a) };
            const float range = 1.6f;
            auto plot = curves.reduced (8.0f);
            auto toX = [&] (float x) { return juce::jmap (x, -1.0f, 1.0f, plot.getX(), plot.getRight()); };
            auto toY = [&] (float y) { return juce::jmap (juce::jlimit (-range, range, y), -range, range, plot.getBottom(), plot.getY()); };
            g.setColour (theme::grid);
            g.fillRect (plot.getX(), toY (0.0f), plot.getWidth(), 1.0f);
            g.fillRect (toX (0.0f), plot.getY(), 1.0f, plot.getHeight());

            auto graph = r.withTrimmedLeft (50.0f).withTrimmedBottom (16.0f);
            auto gx = [&] (int k) { return graph.getX() + graph.getWidth() * (float) (k - 1) / 14.0f; };
            auto gy = [&] (float db) { return juce::jmap (juce::jlimit (-80.0f, 0.0f, db), -80.0f, 0.0f, graph.getBottom(), graph.getY()); };

            for (float db = 0.0f; db >= -80.0f; db -= 20.0f)
            {
                g.setColour (theme::grid);
                g.fillRect (graph.getX(), gy (db), graph.getWidth(), 1.0f);
                drawLabel (g, juce::String ((int) db), juce::Rectangle<float> (40.0f, 12.0f).withPosition (graph.getX() - 44.0f, gy (db) - 6.0f),
                           theme::textDim, juce::Justification::centredRight);
            }

            for (int k : { 1, 3, 5, 7, 9, 11, 13, 15 })
                drawLabel (g, juce::String (k), juce::Rectangle<float> (20.0f, 12.0f).withCentre ({ gx (k), graph.getBottom() + 8.0f }), theme::textDim,
                           juce::Justification::centred);

            for (int t = 0; t < sat::numTypes; ++t)
            {
                s.type = (Type) t;
                const float drive = juce::Decibels::decibelsToGain (s.driveDb);
                juce::Path curvePath, levelsPath;

                for (int i = 0; i <= 200; ++i)
                {
                    const float x = -1.0f + 2.0f * (float) i / 200.0f;
                    i == 0 ? curvePath.startNewSubPath (toX (x), toY (sat::shape (s.type, drive * x))) : curvePath.lineTo (toX (x), toY (sat::shape (s.type, drive * x)));
                }

                g.setColour (colours[t]);
                g.strokePath (curvePath, juce::PathStrokeType (2.0f));

                const auto levels = sat::getHarmonicsDb (s, sixDb(), 15);
                bool started = false;

                for (int k = 1; k <= 15; ++k)
                {
                    const float db = levels[(size_t) k - 1] - levels[0];

                    if (db < -100.0f)
                        continue;

                    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ gx (k), gy (db) }));
                    started ? levelsPath.lineTo (gx (k), gy (db)) : levelsPath.startNewSubPath (gx (k), gy (db));
                    started = true;
                }

                g.strokePath (levelsPath, juce::PathStrokeType (1.2f));
                drawLabel (g, sat::getTypeName (s.type), juce::Rectangle<float> (60.0f, 14.0f).withPosition (graph.getRight() - 60.0f, graph.getY() + 16.0f * (float) t),
                           colours[t], juce::Justification::centredRight);
            }
        } });

    pages.push_back ({ "Tape, tubes and transistors", "Where the sounds come from",
        "Each kind of saturation has a history, and a sound people go looking for.\n\n"
        "Tape. Until the 1990s most music was recorded on magnetic tape. Recorded hotter than it was meant for, tape rounds the "
        "peaks off softly, a little at a time, and loses a little of its highest treble. Drums recorded into the red sound fatter "
        "and more glued together: 'tape compression'. Tape saturation is symmetric, so it adds mostly odd harmonics, gently.\n\n"
        "Tubes. Vacuum tubes amplified everything until transistors replaced them in the 1960s, and they stayed in guitar amps, "
        "microphone preamps and studio compressors. Pushed hard, many tube circuits clip lopsidedly and softly, adding even "
        "harmonics: the warm, singing overdrive of a cranked Fender, Vox or Marshall amp.\n\n"
        "Transistors and diodes. Transistor circuits clip much harder when pushed. The first fuzz pedals did this on purpose: the "
        "Maestro Fuzz-Tone (1962) gave the Rolling Stones the riff of 'Satisfaction' (1965), and the Fuzz Face (1966) was one of Jimi "
        "Hendrix's sounds. Before pedals, guitarists got the sound by damaging their gear: Link Wray poked holes in his amp's "
        "speaker for 'Rumble' (1958), and Dave Davies of the Kinks slashed his with a razor blade for 'You Really Got Me' (1964).\n\n"
        "Digital. Digital audio has no gentle limit at all: past 0 dB it clips hard. That's why digital saturation plugins, like "
        "this one, model the curves of the old gear.\n\n"
        "The picture shows a note through each of DawIt Saturation's three types at the window's drive, made as loud.",
        [processor, sixDb] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.driveDb = juce::jmax (12.0f, s.driveDb);
            drawCaption (g, r, "A sine at -6 dB through each type, at " + sat::formatKnob (Knob::drive, s), "made as loud as the note (grey)");
            const char* about[] = { "Tape: soft, symmetric", "Tube: soft, lopsided", "Clip: hard, flat-topped" };
            const float row = r.getHeight() / 3.0f;

            for (int t = 0; t < sat::numTypes; ++t)
            {
                s.type = (Type) t;
                auto line = r.withHeight (row).withY (r.getY() + row * (float) t).reduced (0.0f, 4.0f);
                drawLabel (g, about[t], line.removeFromLeft (170.0f), theme::text);
                drawBeforeAfter (g, line, s, sixDb(), 3.0);
            }
        } });

    pages.push_back ({ "Chords and intermodulation", "Why distortion likes power chords",
        "With one note, saturation adds only harmonics of that note, which blend in. With two notes at once, it does more: it also "
        "adds tones at the sum and difference of their frequencies, and of their harmonics. That's intermodulation distortion.\n\n"
        "Whether those extra tones fit depends on the interval. Take A at 110 Hz:\n\n"
        "- With the E a fifth above (165 Hz, 3:2), the difference is 55 Hz, the A an octave below, and every new tone is a "
        "multiple of 55 Hz: the chord turns into one big, low A. That's why distorted guitars play power chords (root and fifth): "
        "they sound huge, not messy.\n"
        "- With the C sharp a major third above (137.5 Hz, 5:4), the new tones are multiples of 27.5 Hz, two octaves down: many "
        "more, closer together, and some clash with the chord. A distorted major chord sounds muddy.\n"
        "- With the B flat a semitone above (117.3 Hz, 16:15), they are multiples of 7.3 Hz: a dense cluster of tones that sounds "
        "rough and out of tune.\n\n"
        "This is also why saturation on a whole mix should be gentle, and why a bus with many instruments gets cloudy when driven: "
        "every pair of notes makes new tones.\n\n"
        "The picture shows what comes out of the curve at the window's settings for each pair, up to 1 kHz: the two notes in white, "
        "their own harmonics in orange, the new intermodulation tones in red.",
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.driveDb = juce::jmax (12.0f, s.driveDb);
            constexpr double base = 110.0 / 60.0;   // A2 is the 60th multiple; E3 the 90th, C#3 the 75th, Bb2 the 64th
            constexpr int maxMultiple = 545;
            const std::pair<const char*, int> pairs[] = { { "A and E: a fifth", 90 }, { "A and C#: a major third", 75 }, { "A and Bb: a semitone", 64 } };

            if (! pictures->hasTwoNotes || std::memcmp (&s, &pictures->twoNotesFor, sizeof (s)) != 0)
            {
                for (size_t i = 0; i < 3; ++i)
                    pictures->twoNotes[i] = computeTwoNotes (s, 60, pairs[i].second, maxMultiple);

                pictures->twoNotesFor = s;
                pictures->hasTwoNotes = true;
            }

            drawCaption (g, r, "Two notes through the curve, " + sat::formatKnob (Knob::drive, s) + " " + sat::getTypeName (s.type),
                         "white: the notes; orange: their harmonics; red: new tones");
            auto axis = r.removeFromBottom (14.0f).withTrimmedLeft (170.0f);
            const float row = r.getHeight() / 3.0f;

            for (size_t i = 0; i < 3; ++i)
            {
                auto line = r.withHeight (row).withY (r.getY() + row * (float) i).reduced (0.0f, 5.0f);
                const int high = pairs[i].second;
                const int common = std::gcd (60, high);
                drawLabel (g, pairs[i].first, line.withWidth (165.0f).withHeight (line.getHeight() / 2.0f), theme::text);
                drawLabel (g, "new tones every " + juce::String (common * base, 1) + " Hz",
                           line.withWidth (165.0f).withTrimmedTop (line.getHeight() / 2.0f));
                line.removeFromLeft (170.0f);

                std::vector<std::pair<double, float>> lines;
                const auto& levels = pictures->twoNotes[i];
                const float loudest = juce::jmax (levels[60], levels[(size_t) high]);

                for (int k = 1; k <= maxMultiple; ++k)
                    lines.push_back ({ k * base, levels[(size_t) k] - loudest });

                drawLines (g, line, lines, 1000.0, [high] (double hz)
                {
                    const int k = juce::roundToInt (hz / base);
                    if (k == 60 || k == high) return theme::text;
                    if (k % 60 == 0 || k % high == 0) return colour;
                    return juce::Colour (0xffff5c5c);
                });
            }

            for (const int hz : { 0, 250, 500, 750, 1000 })
                drawLabel (g, juce::String (hz) + " Hz", juce::Rectangle<float> (60.0f, 14.0f).withCentre ({ axis.getX() + axis.getWidth() * (float) hz / 1000.0f, axis.getCentreY() }),
                           theme::textDim, juce::Justification::centred);
        } });

    pages.push_back ({ "Louder at the same peak", "Saturation and dynamics",
        "Rounding off the peaks of a sound lowers its peaks more than its body. So after saturation, the gap between the peaks "
        "and the average level (the crest factor) is smaller: at the same peak level, the sound is louder. A soft curve works like "
        "a very fast compressor on the peaks, faster than any compressor's attack.\n\n"
        "That's why engineers use saturation on drums, bass and whole mixes: the sound gets denser and louder without its peaks "
        "getting any higher, so it can be turned up further before it clips. Mastering limiters and 'clippers' use the same idea.\n\n"
        "It is also the trap: louder almost always sounds better to the ear, so a saturator can seem to improve everything while it "
        "only makes it louder. Match level keeps the loudness the same, so you hear only the change in tone.\n\n"
        "A second trick hides in the harmonics: small speakers (phones, laptops) can't play the lowest notes of a bass or kick, but "
        "they can play their harmonics an octave and more up. The ear fills in the missing low note from its harmonics (the "
        "'missing fundamental'), so a saturated bass is heard on a phone where a clean one disappears.\n\n"
        "The picture shows a kick drum before and after the curve, made to peak at the same level: the after is fuller, and louder.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            s.driveDb = juce::jmax (9.0f, s.driveDb);
            constexpr int n = 600;
            std::vector<float> before (n), after (n);
            const float drive = juce::Decibels::decibelsToGain (s.driveDb);
            float peakBefore = 0.0f, peakAfter = 0.0f;

            for (int i = 0; i < n; ++i)
            {
                const double t = i / (double) n * 0.4;
                const double phase = juce::MathConstants<double>::twoPi * (50.0 * t + 120.0 * 0.03 * (1.0 - std::exp (-t / 0.03)));
                before[(size_t) i] = 0.9f * (float) (std::sin (phase) * std::exp (-t / 0.12));
                after[(size_t) i] = sat::shape (s.type, drive * before[(size_t) i]);
                peakBefore = juce::jmax (peakBefore, std::abs (before[(size_t) i]));
                peakAfter = juce::jmax (peakAfter, std::abs (after[(size_t) i]));
            }

            double sumBefore = 0.0, sumAfter = 0.0;

            for (int i = 0; i < n; ++i)
            {
                after[(size_t) i] *= peakBefore / peakAfter;
                sumBefore += before[(size_t) i] * before[(size_t) i];
                sumAfter += after[(size_t) i] * after[(size_t) i];
            }

            const double louder = 10.0 * std::log10 (sumAfter / sumBefore);
            drawCaption (g, r, "A kick drum at the same peak, " + sat::formatKnob (Knob::drive, s) + " " + sat::getTypeName (s.type),
                         "after: " + formatDb (louder) + " louder");
            const float half = r.getHeight() / 2.0f;

            for (int row = 0; row < 2; ++row)
            {
                auto line = r.withHeight (half).withY (r.getY() + half * (float) row).reduced (0.0f, 4.0f);
                const auto& v = row == 0 ? before : after;
                const double sum = row == 0 ? sumBefore : sumAfter;
                drawLabel (g, row == 0 ? "Before" : "After", line.removeFromLeft (70.0f), theme::text);
                const float scale = line.getHeight() * 0.48f / peakBefore;
                juce::Path p;

                for (int i = 0; i < n; ++i)
                {
                    const float x = line.getX() + line.getWidth() * (float) i / n, y = line.getCentreY() - v[(size_t) i] * scale;
                    i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
                }

                g.setColour (row == 0 ? theme::textDim : colour);
                g.strokePath (p, juce::PathStrokeType (1.4f));
                const float rms = (float) std::sqrt (sum / n);
                g.setColour (theme::text.withAlpha (0.6f));
                const float dashes[] = { 4.0f, 4.0f };

                for (const float level : { rms, -rms })
                    g.drawDashedLine ({ line.getX(), line.getCentreY() - level * scale, line.getRight(), line.getCentreY() - level * scale }, dashes, 2, 1.0f);

                drawLabel (g, "average level (RMS)", juce::Rectangle<float> (140.0f, 12.0f).withPosition (line.getRight() - 140.0f, line.getCentreY() - rms * scale - 14.0f),
                           theme::textDim, juce::Justification::centredRight);
            }
        } });

    pages.push_back ({ "Aliasing", "Harmonics that fold back",
        "Digital audio has a ceiling on frequencies too: a sample rate can only hold frequencies up to half of itself (the Nyquist "
        "frequency): 24 kHz at 48 kHz. That's fine for what goes in, because we hear only up to about 20 kHz. But saturation makes "
        "new harmonics, and of a high note they go far past 24 kHz: the 5th harmonic of 9.1 kHz is 45.5 kHz.\n\n"
        "A digital curve can't make a frequency the sample rate can't hold. Instead it folds back, mirrored at the Nyquist "
        "frequency, and lands somewhere else: that 45.5 kHz comes out at 2.5 kHz. Unlike a harmonic, a folded tone has nothing to "
        "do with the note's pitch, so it sounds like a dissonant, metallic whine. This is aliasing, the same effect that makes "
        "wagon wheels seem to turn backwards on film.\n\n"
        "The usual cure is oversampling: run the curve at a higher sample rate, where there's room for the harmonics, then filter "
        "away everything above 24 kHz before going back to the original rate. DawIt Saturation runs the curve at four times the "
        "sample rate (192 kHz at 48 kHz), with filters that keep the saturated sound in time with the dry one.\n\n"
        "The picture shows a 9.1 kHz sine through the curve at 48 kHz: straight, and through DawIt Saturation. Every line besides "
        "the note is folded back: none of its harmonics fit below 24 kHz.",
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();

            if (! pictures->hasAliasing || std::memcmp (&s, &pictures->aliasingFor, sizeof (s)) != 0)
            {
                computeAliasing (*pictures, s);
                pictures->aliasingFor = s;
                pictures->hasAliasing = true;
            }

            drawCaption (g, r, "A 9.1 kHz sine through the curve at 48 kHz, " + juce::String (juce::jmax (18.0f, s.driveDb), 0) + " dB drive",
                         "white: the note; red: folded back");
            auto axis = r.removeFromBottom (14.0f).withTrimmedLeft (170.0f);
            const float row = r.getHeight() / 2.0f;

            for (int i = 0; i < 2; ++i)
            {
                auto line = r.withHeight (row).withY (r.getY() + row * (float) i).reduced (0.0f, 5.0f);
                drawLabel (g, i == 0 ? "Without oversampling" : "DawIt Saturation (4x)", line.withWidth (165.0f), theme::text);
                line.removeFromLeft (170.0f);
                drawLines (g, line, i == 0 ? pictures->naive : pictures->oversampled, 24000.0,
                           [] (double hz) { return std::abs (hz - 9100.0) < 30.0 ? theme::text : juce::Colour (0xffff5c5c); });
            }

            for (const int khz : { 0, 4, 8, 12, 16, 20, 24 })
                drawLabel (g, juce::String (khz) + " kHz", juce::Rectangle<float> (60.0f, 14.0f).withCentre ({ axis.getX() + axis.getWidth() * (float) khz / 24.0f, axis.getCentreY() }),
                           theme::textDim, juce::Justification::centred);
        } });

    pages.push_back ({ "Saturation in a mix", "Warmth, presence, glue",
        "Saturation is one of the most used tools in mixing, mostly in small amounts you don't notice as distortion.\n\n"
        "- Vocals: a little tube makes a voice thicker and more present, so it sits on top of the mix without being louder.\n"
        "- Bass: harmonics make it heard on small speakers; blend it in parallel so the clean low end stays.\n"
        "- Drums: tape or clipping rounds the peaks of the hits, making them denser and punchier; in parallel for crunch.\n"
        "- Synths and keys: digital sounds get some of the warmth and movement of analog gear.\n"
        "- Buses and the mix: a touch of tape glues the parts together.\n"
        "- Guitars and effects: lots of drive for overdrive and fuzz; with a tone control, because fuzz without one is harsh.\n\n"
        "Some habits:\n\n"
        "- Compare at the same loudness (match level), or louder will always win.\n"
        "- Where it sits in the chain matters: before a compressor it reacts to every peak; after one, the level is steadier and so "
        "is the saturation. Before an EQ, the EQ can shape the harmonics; after an EQ, boosted frequencies get driven harder.\n"
        "- The level going in sets how much happens: turning a track up before the saturator gives more saturation, not just "
        "more level. This is gain staging again.\n"
        "- Little by little: a bit on several tracks adds up more musically than a lot on one. And watch for chords (see "
        "Chords and intermodulation).\n"
        "- Tame the top: if it sounds fizzy, lower the tone before lowering the drive.",
        [peakOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto& presets = sat::getPresets();
            drawCaption (g, r, "Four starting points", "what each adds to a note at the track's level");
            const char* names[] = { "Warm vocal", "Bass on small speakers", "Drum crunch", "Fuzz guitar" };
            const float w = r.getWidth() / 2.0f, h = r.getHeight() / 2.0f;

            for (int i = 0; i < 4; ++i)
            {
                const auto preset = std::find_if (presets.begin(), presets.end(), [&] (auto& p) { return p.name == names[i]; });

                if (preset == presets.end())
                    continue;

                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 2), r.getY() + h * (float) (i / 2), w, h).reduced (8.0f, 4.0f);
                drawLabel (g, preset->name + ": " + sat::formatKnob (Knob::drive, preset->settings) + " " + sat::getTypeName (preset->settings.type),
                           cell.removeFromTop (16.0f), theme::text);
                drawHarmonics (g, cell, preset->settings, peakOf(), 10, false);
            }
        } });

    pages.push_back ({ "How DawIt Saturation works", "A curve, four times faster",
        "1. Oversampling: the sound is raised to four times the sample rate with linear-phase filters, so there is room for the "
        "harmonics the curve makes.\n\n"
        "2. Drive and curve: every sample is turned up by the drive and put through the curve: tanh (the hyperbolic tangent, a "
        "smooth S) for tape, a tanh shifted off-centre for tube, and a flat limit at 1 for clip. All three pass small values "
        "unchanged.\n\n"
        "3. Back down: the filters remove everything above the original half sample rate, then the sound goes back to the "
        "original rate. The filters take 61 samples at 48 kHz (about 1.3 ms); the dry sound is held back as long, so the two "
        "stay in step when mixed.\n\n"
        "4. Clean up: a 10 Hz high-pass removes the DC offset of the tube's lopsided curve, then the tone's high cut (12 dB per "
        "octave).\n\n"
        "5. What it adds: a clean copy of the input, put through the same filters and fitted to the level of the output, is "
        "subtracted from it. What's left is what the saturation added, for Hear only what it adds and the graph.\n\n"
        "6. Mix, output and match level: the clean and saturated sound are blended, the output gain applied, and match level "
        "turns the result up or down to the loudness of the input over the last half second.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "From the dry sound to the saturated one", {});
            const float col = (r.getWidth() - 5.0f * 14.0f) / 6.0f, midY = r.getCentreY() - 10.0f;
            auto box = [&] (int column, float height = 54.0f) { return juce::Rectangle<float> (r.getX() + (col + 14.0f) * (float) column, midY - height / 2.0f, col, height); };

            const auto in = drawBox (g, box (0), "Sound in", "dry", theme::textDim);
            const auto up = drawBox (g, box (1), "Up 4x", "room for harmonics", theme::accent);
            const auto curveBox = drawBox (g, box (2), "Drive + curve", "tape, tube or clip", colour);
            const auto down = drawBox (g, box (3), "Down 4x", "filter above 24 kHz", theme::accent);
            const auto tone = drawBox (g, box (4), "DC + tone", "high-pass, high cut", colour);
            const auto out = drawBox (g, box (5), "Mix, output", "and match level", theme::accent);

            arrow (g, { in.getRight(), midY }, { up.getX(), midY });
            arrow (g, { up.getRight(), midY }, { curveBox.getX(), midY });
            arrow (g, { curveBox.getRight(), midY }, { down.getX(), midY });
            arrow (g, { down.getRight(), midY }, { tone.getX(), midY });
            arrow (g, { tone.getRight(), midY }, { out.getX(), midY });

            juce::Path dry;
            const float top = midY - 66.0f;
            dry.startNewSubPath (in.getCentreX(), in.getY());
            dry.lineTo (in.getCentreX(), top);
            dry.lineTo (out.getCentreX(), top);
            g.setColour (theme::textDim);
            g.strokePath (dry, juce::PathStrokeType (1.2f));
            g.drawArrow ({ out.getCentreX(), top, out.getCentreX(), out.getY() }, 1.2f, 7.0f, 7.0f);
            drawLabel (g, "the dry sound, held back as long as the filters", juce::Rectangle<float> (out.getCentreX() - in.getCentreX(), 14.0f)
                                                                                .withPosition (in.getCentreX(), top - 16.0f), theme::textDim, juce::Justification::centred);
        } });

    return pages;
}
} // namespace daw::sattext
