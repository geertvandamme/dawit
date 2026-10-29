#include "AdditiveGuide.h"
#include "SubtractiveGuide.h"

namespace daw::additext
{
using additive::Knob;
using additive::numPartials;
using additive::offDb;

namespace
{
    constexpr float topDb = 6.0f;

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    juce::String formatDb (double db)
    {
        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    juce::String formatCents (double cents)
    {
        const int c = juce::roundToInt (cents);
        return (c > 0 ? "+" : "") + juce::String (c) + " ct";
    }

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

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Colour c = theme::textDim,
                    juce::Justification justification = juce::Justification::centredLeft)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (text, r, justification, true);
    }

    /** Samples across an area, with 1 at `scale` pixels from the middle. */
    void drawLine (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<float>& values, float scale, juce::Colour c, float thickness)
    {
        if (values.size() < 2)
            return;

        juce::Path path;

        for (size_t i = 0; i < values.size(); ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / (float) (values.size() - 1);
            const float y = juce::jlimit (r.getY(), r.getBottom(), r.getCentreY() - values[i] * scale);
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }

        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    double now()
    {
        return juce::Time::getMillisecondCounterHiRes() / 1000.0;
    }

    juce::Colour getBarColour (int n)
    {
        return n % 2 == 0 ? evenColour : colour;
    }

    /** One and a half cycles of a wave from its first `count` harmonics, with their true signs (from the subtractive oscillators' formulas). */
    std::vector<float> getShapeWave (sub::Wave wave, int count, int points)
    {
        std::vector<sub::Partial> partials;

        for (int n = 1; n <= count; ++n)
        {
            sub::Partial p;
            p.hz = n;
            p.before = sub::getHarmonic (wave, 50.0f, n);
            partials.push_back (p);
        }

        return sub::getWave (partials, 1.5, points, false);
    }

    float getPeak (const std::vector<float>& wave)
    {
        float peak = 0.0f;

        for (auto v : wave)
            peak = juce::jmax (peak, std::abs (v));

        return peak;
    }

    /** The test note's wave over a few cycles, as the voice plays it before its gain. */
    std::vector<float> getTestWave (const additive::Settings& s, int points)
    {
        return additive::getWave (additive::getPartials (s, sub::testNoteHz), 4.0 / sub::testNoteHz, points);
    }

    /** Shows something on settings that have little to show: the window's sound, or a saw when only the 1st harmonic is on. */
    additive::Settings withSomethingToShow (additive::Settings s)
    {
        int on = 0;

        for (auto db : s.levelsDb)
            on += db > offDb + 0.05f ? 1 : 0;

        if (on < 2)
            s.levelsDb = additive::getShapeLevels (additive::Shape::saw);

        return s;
    }
}

//==============================================================================
juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::brightness: return "Brightness";
        case (int) Knob::even:       return "Even harmonics";
        case (int) Knob::stretch:    return "Stretch";
        case (int) Knob::phases:     return "Phases";
        case (int) Knob::swell:      return "Swell: the highs come in later";
        case (int) Knob::fade:       return "Fade: the highs die sooner";
        case (int) Knob::ampAttack:  return "Amp envelope: attack";
        case (int) Knob::ampDecay:   return "Amp envelope: decay";
        case (int) Knob::ampSustain: return "Amp envelope: sustain";
        case (int) Knob::ampRelease: return "Amp envelope: release";
        case (int) Knob::lfoRate:    return "LFO rate";
        case (int) Knob::lfoDepth:   return "LFO depth";
        case (int) Knob::lfoTarget:  return "LFO to";
        case (int) Knob::glide:      return "Glide";
        case (int) Knob::mode:       return "Voices: poly or mono";
        case (int) Knob::velocity:   return "Velocity";
        case (int) Knob::output:     return "Output";
        case partials:               return "The harmonics";
        case wave:                   return "The wave: the harmonics added up";
        case overTime:               return "A note over time";
        case fxui::overview:         return "DawIt Additive";
        case fxui::presets:          return "Starting points";
        case fxui::clap:             return "Play a note";
        case fxui::listen:           return "Hear only the 1st harmonic";
        case fxui::guide:            return "What is additive synthesis?";
        case fxui::history:          return "The last 4 seconds";
        default:                     return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::brightness:
            return "Tilts all the harmonics at once. Below 0, every octave up is this many dB quieter on top of what the bars say; "
                   "above 0, louder. It works like a gentle filter that moves with the note: the sound gets darker or brighter "
                   "without redrawing a single bar.\n\n"
                   "Harmonics drawn all at the same level with a tilt of -6 dB per octave are exactly a saw's: the 2nd an octave up "
                   "at -6 dB, the 4th two octaves up at -12.";

        case (int) Knob::even:
            return "How much of the even harmonics (the 2nd, 4th, 6th...) you hear. The odd ones alone sound hollow and woody, "
                   "like a clarinet or a square wave. The even ones fill the sound out: the 2nd is an octave above the note, the 4th "
                   "two octaves, so they add strength without adding new notes.\n\n"
                   "At 0%, a saw's bars become a square's.";

        case (int) Knob::stretch:
            return "Pulls the harmonics sharp, the higher ones more, as a stiff string does. A piano string isn't perfectly "
                   "flexible, so its high partials sit a little above whole-number multiples of the note (inharmonicity). A little "
                   "stretch makes a sound more like a piano or a guitar; a lot makes it metallic and bell-like, because the partials "
                   "no longer line up into one clear note. The 1st harmonic, the note itself, stays where it is.";

        case (int) Knob::phases:
            return "Where each harmonic starts its cycle. Aligned: they all start together, so a saw's harmonics add up to the "
                   "saw's ramp, as in a textbook. Spread: each starts at the point that keeps the wave's peaks lowest, found with "
                   "Manfred Schroeder's recipe (1970) and a few other tries, and the same harmonics make a very different-looking "
                   "wave.\n\n"
                   "The ear hears how loud each harmonic is, hardly how they line up, so the two sound almost the same. What changes "
                   "is the peak: when many harmonics are about as loud, as in an organ, a choir or a bell, spread peaks lower and "
                   "leaves more headroom for chords. A saw or a square already peaks low lined up (a square as low as any wave can), "
                   "so for them spread stays lined up. It applies from the next note played.";

        case (int) Knob::swell:
            return "Makes the higher harmonics come in after the note starts, so each note gets brighter as it begins, as on brass "
                   "and bowed strings, whose high harmonics take a moment to build up. The time is for the highest, the 32nd; the "
                   "others come in sooner, in proportion: the 2nd in 1/31 of the time, the 16th in about half. Off: every harmonic "
                   "starts with the note.";

        case (int) Knob::fade:
            return "Makes the higher harmonics die away sooner, as in every plucked or struck sound: a guitar, a piano or a bell is "
                   "brightest the moment it's hit and gets rounder as it rings. The time is how long the 2nd harmonic takes to fall "
                   "to half its level; the 3rd falls twice as fast, the 4th three times, and so on. Off: they last as long as the "
                   "note.\n\n"
                   "Moving each harmonic on its own like this is the strength of additive synthesis.";

        case (int) Knob::ampAttack:
            return "How long a note takes to rise from nothing to full when a key is pressed. Short (1-5 ms): the note starts at "
                   "once, a hit or a pluck. Medium (20-100 ms): a soft start, like breath or a bow. Long (0.5 s and up): the note "
                   "swells in, as for pads.";

        case (int) Knob::ampDecay:
            return "After the attack, how long the note takes to fall from full to the sustain level while the key is still held. "
                   "With a low sustain, the decay is how long the note lasts: short for a blip, seconds for a piano or a bell.";

        case (int) Knob::ampSustain:
            return "The level a note holds while the key stays down, after the decay. A level, not a time. 100%: as loud as at its "
                   "peak, like an organ. 0%: it dies away even with the key held, like a struck or plucked note.";

        case (int) Knob::ampRelease:
            return "How long a note takes to fade to nothing after the key is let go. Short: it stops when you lift your finger. "
                   "Long: it rings on, like a bell or a sound in a big room, and chords blur into each other.";

        case (int) Knob::lfoRate:
            return "The LFO (low-frequency oscillator) is a wave too slow to hear as a note, used to move something up and down "
                   "over and over. Rate is how many times a second: 5-7 Hz is the speed of a singer's vibrato; 0.1-1 Hz a slow "
                   "change you feel more than hear.";

        case (int) Knob::lfoDepth:
            return "How far the LFO moves its target: at 100%, the pitch by 2 semitones up and down, the brightness by 6 dB per "
                   "octave either way, or the even harmonics all the way out and back. A little goes a long way on the pitch.";

        case (int) Knob::lfoTarget:
            return "What the LFO moves. Pitch: vibrato. Brightness: the tilt goes up and down, so the sound gets brighter and "
                   "darker, like a slow filter sweep. Even: the even harmonics fade out and back, so the sound moves between full "
                   "and hollow.";

        case (int) Knob::glide:
            return "Glide (portamento): instead of jumping to the next note, the pitch slides there, taking about this long. In "
                   "mono only overlapping (legato) notes slide; in poly every new note slides from the last one played.";

        case (int) Knob::mode:
            return "Poly: each note gets its own voice (up to 16), for chords and pads. Mono: one note at a time; a note played "
                   "while another is held takes over without starting the envelopes again (legato), and glides if Glide is on. "
                   "Basses, leads and wind instruments usually play mono.";

        case (int) Knob::velocity:
            return "How much playing softer makes a note quieter and darker. At 100%, a note played half as hard is 6 dB quieter and "
                   "its harmonics tilt down a further 3 dB per octave, as on an acoustic instrument played gently. At 0% every note "
                   "is the same, as on an organ.";

        case (int) Knob::output:
            return "The synth's level going into the track's effects and fader. DawIt Additive turns the sum of the harmonics down "
                   "as you add them, so its power is never more than one full sine's: adding harmonics makes it brighter, not "
                   "louder. Still leave headroom: chords add up.";

        case partials:
            return "The 32 harmonics of the note as bars, the 1st (the note itself) on the left. The outline is the level you drew; "
                   "the colour is what you hear after the brightness, the even harmonics, the velocity and the swell and fade of the "
                   "newest note, so it moves while a note plays. Odd harmonics are teal, even ones pink.\n\n"
                   "Drag over the bars to draw them: up is louder, the bottom is off. Double-click a bar to turn it off; right-click "
                   "to fill them all with a shape.";

        case wave:
            return "A few cycles of the test note: all the harmonics added up (coloured) and the 1st harmonic alone (grey). Every "
                   "bump and corner in the wave comes from a harmonic: take them away and it turns back into the grey sine.";

        case overTime:
            return "A note over time, left to right. Each row is a harmonic, the 1st at the bottom, brighter where it's louder; the "
                   "green line is the amp envelope. With a swell, the top rows start later; with a fade, they die sooner. The key "
                   "is let go at the dashed line, and the white line is the newest note.";

        case fxui::clap:
            return "Plays the test note, A3, for 0.8 seconds through the synth, even while the song is stopped. Or play the track's "
                   "MIDI notes, or a MIDI keyboard on this track.";

        case fxui::listen:
            return "Turns every harmonic but the 1st off for a moment, at the level it has in the sound. The pitch stays the same; "
                   "only the colour goes. Switch back and forth: everything you hear in between is the harmonics.";

        case fxui::presets:
            return "Sounds to start from, each telling why it's made the way it is. Pick one and read the notes below, then change "
                   "a bar or a knob at a time and listen.";

        case fxui::guide:
            return "A guide for beginners: what additive synthesis is, the harmonic series, how the classic waves are built from "
                   "sines, why phase hardly matters, harmonics that move, stretched partials, the Hammond's drawbars, and how to "
                   "build a sound from scratch.";

        case fxui::history:
            return "The last 4 seconds: grey how loud the 1st harmonic alone was, coloured the whole sound. The gap between them is "
                   "what the other harmonics add.";

        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::brightness: return "0 when the bars say it all; -2 to -4 dB/oct to soften a bright set of bars; -6 to turn equal bars into a saw.";
        case (int) Knob::even:       return "100% for most sounds; 0-20% for hollow, reedy and clarinet-like ones.";
        case (int) Knob::stretch:    return "0 for organs and winds; 5-15% for strings and keys; 20-35% for piano-like; 60-100% for bells and metal.";
        case (int) Knob::phases:     return "Aligned for saws and squares, and to see the shapes; spread for organs, bells and voices, where many harmonics are about as loud.";
        case (int) Knob::swell:      return "Off for plucks and keys; 50-300 ms for brass and bowed strings; seconds for a slow bloom.";
        case (int) Knob::fade:       return "0.2-0.5 s for plucks and keys; 1-2 s for bells; off for held sounds.";
        case (int) Knob::ampAttack:  return "1-5 ms for keys, plucks and organs; 20-80 ms for winds and brass; 0.3-1 s for pads.";
        case (int) Knob::ampDecay:   return "Matters when sustain is low: 1-3 s for plucks and keys, longer for bells.";
        case (int) Knob::ampSustain: return "100% for organs and pads; 0% for plucks, keys and bells; in between for brass.";
        case (int) Knob::ampRelease: return "20-100 ms for organs and basses; 0.3-0.5 s for keys; 1-3 s for pads and bells.";
        case (int) Knob::lfoRate:    return "Vibrato 5-7 Hz; slow changes in colour 0.1-0.5 Hz.";
        case (int) Knob::lfoDepth:   return "Vibrato 3-6%; brightness 20-50%; even harmonics 60-100%.";
        case (int) Knob::lfoTarget:  return "Pitch for vibrato, brightness or even for pads that change colour.";
        case (int) Knob::glide:      return "Off for most sounds; 30-80 ms for winds, leads and basses.";
        case (int) Knob::mode:       return "Mono for basses, leads and winds; poly for keys, organs and pads.";
        case (int) Knob::velocity:   return "50-70% for keys and plucks; 0 for organs; 20-40% for pads.";
        case (int) Knob::output:     return "0 dB for one-note parts; -3 to -6 dB for chords.";
        default:                     return {};
    }
}

double getPeakDb (const additive::Settings& s)
{
    const float peak = getPeak (getTestWave (s, 2000));
    return juce::Decibels::gainToDecibels (peak * additive::voiceGain, -100.0f) + s.outputDb;
}

juce::String getNoteName (int note)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1);
}

juce::String describeSettings (int t, const additive::Settings& s)
{
    const double note = sub::testNoteHz;

    switch (t)
    {
        case (int) Knob::brightness:
        {
            if (std::abs (s.brightnessDb) < 0.05f)
                return "No tilt: you hear the bars as they're drawn.";

            auto at = [&s] (int n) { return formatDb (s.brightnessDb * std::log2 ((double) n)); };
            return additive::formatKnob (Knob::brightness, s) + ": against the bars, the 2nd harmonic is " + at (2) + ", the 4th "
                   + at (4) + ", the 8th " + at (8) + " and the 32nd " + at (32) + ".";
        }

        case (int) Knob::even:
            if (s.evenPercent >= 99.5f)
                return "100%: all of the even harmonics, as drawn.";

            if (s.evenPercent < 0.5f)
                return "0%: only the odd harmonics, a hollow sound.";

            return additive::formatKnob (Knob::even, s) + ": the even harmonics are "
                   + formatDb (juce::Decibels::gainToDecibels (s.evenPercent / 100.0)) + " against their bars.";

        case (int) Knob::stretch:
        {
            if (s.stretchPercent < 0.5f)
                return "0%: every harmonic exactly at a whole-number multiple of the note, as in an organ or a perfect string.";

            return additive::formatKnob (Knob::stretch, s) + ": the 4th harmonic is " + formatCents (additive::getStretchCents (4, s.stretchPercent))
                   + " sharp, the 8th " + formatCents (additive::getStretchCents (8, s.stretchPercent)) + ", the 16th "
                   + formatCents (additive::getStretchCents (16, s.stretchPercent)) + ". At A3 the 8th is at "
                   + formatHz (note * additive::getRatio (8, s.stretchPercent)) + " instead of " + formatHz (note * 8.0) + ".";
        }

        case (int) Knob::phases:
        {
            auto other = s;
            other.phases = s.phases == additive::Phases::aligned ? additive::Phases::spread : additive::Phases::aligned;
            return additive::formatKnob (Knob::phases, s) + ": A3 peaks at " + formatDb (getPeakDb (s)) + "; "
                   + additive::formatKnob (Knob::phases, other).toLowerCase() + ", the same harmonics would peak at " + formatDb (getPeakDb (other)) + ".";
        }

        case (int) Knob::swell:
            if (s.swellSeconds < 0.0005f)
                return "Off: every harmonic starts with the note.";

            return "The 32nd harmonic comes in over " + additive::formatKnob (Knob::swell, s) + ", the 8th over "
                   + fxui::formatSeconds (s.swellSeconds * 7.0 / 31.0) + ", the 2nd over " + fxui::formatSeconds (s.swellSeconds / 31.0) + ".";

        case (int) Knob::fade:
        {
            if (additive::isFadeOff (s.fadeSeconds))
                return "Off: the harmonics last as long as the note.";

            auto after = [&s] (int n)
            {
                const double db = juce::Decibels::gainToDecibels (additive::getPartialEnvelope (s, n, 1.0), -200.0);
                return db < -90.0 ? juce::String ("gone (under -90 dB)") : formatDb (db);
            };
            return "The 2nd harmonic halves in " + additive::formatKnob (Knob::fade, s) + ", the 4th in " + fxui::formatSeconds (s.fadeSeconds / 3.0)
                   + ", the 8th in " + fxui::formatSeconds (s.fadeSeconds / 7.0) + ". A second into a note, the 2nd is " + after (2)
                   + ", the 8th " + after (8) + ".";
        }

        case (int) Knob::ampAttack:
        case (int) Knob::ampDecay:
        case (int) Knob::ampSustain:
        case (int) Knob::ampRelease:
            return "A note rises in " + additive::formatKnob (Knob::ampAttack, s) + ", falls to " + additive::formatKnob (Knob::ampSustain, s)
                   + " in " + additive::formatKnob (Knob::ampDecay, s) + (s.ampEnv.sustain < 0.005f ? " and is gone, even if held" : " and holds there")
                   + ", and fades out in " + additive::formatKnob (Knob::ampRelease, s) + " after the key is let go.";

        case (int) Knob::lfoRate:
            return additive::formatKnob (Knob::lfoRate, s) + ": a cycle every " + juce::String (1000.0 / s.lfoRateHz, 0) + " ms"
                   + (s.lfoDepthPercent < 0.5f ? ", but the depth is 0, so it moves nothing." : ".");

        case (int) Knob::lfoDepth:
        case (int) Knob::lfoTarget:
        {
            const float d = s.lfoDepthPercent / 100.0f;

            if (d < 0.005f)
                return "Depth 0: the LFO moves nothing.";

            switch (s.lfoTarget)
            {
                case additive::LfoTarget::pitch:      return "Vibrato: the pitch goes up and down by " + juce::String (d * additive::lfoPitchSemitones, 2) + " semitones.";
                case additive::LfoTarget::brightness: return "The brightness swings " + juce::String (d * additive::lfoBrightnessDb, 1) + " dB per octave each way.";
                case additive::LfoTarget::even:       return "The even harmonics dip to " + juce::String (juce::roundToInt (100.0f - d * 100.0f)) + "% of their level and back.";
            }

            return {};
        }

        case (int) Knob::glide:
            return s.glideSeconds < 0.0005f ? juce::String ("Off: notes jump straight to their pitch.")
                                            : "Notes slide to their pitch in about " + additive::formatKnob (Knob::glide, s)
                                                  + (s.mode == sub::Mode::mono ? ", when you play them legato." : ", from the last note played.");

        case (int) Knob::mode:
            return s.mode == sub::Mode::mono ? "Mono: one note at a time, legato notes don't restart the envelope."
                                             : "Poly: up to " + juce::String (BuiltInAdditive::numVoices) + " notes at once.";

        case (int) Knob::velocity:
        {
            const float half = additive::getVelocityScale (s, 0.5f);
            return "A note played half as hard is " + formatDb (juce::Decibels::gainToDecibels (half)) + ", with its harmonics tilted "
                   + formatDb (-3.0 * s.velocityPercent / 100.0) + " more per octave.";
        }

        case (int) Knob::output:
            return "A3 at full velocity peaks at about " + formatDb (getPeakDb (s)) + "; one sine at 0 dB would peak at "
                   + formatDb (additive::sinePeakDb + s.outputDb) + ".";

        default:
            return {};
    }
}

//==============================================================================
juce::Rectangle<float> getPartialsPlot (juce::Rectangle<float> r)
{
    return r.withTrimmedLeft (34.0f).withTrimmedBottom (18.0f).withTrimmedTop (2.0f);
}

int getPartialAt (juce::Rectangle<float> plot, float x)
{
    return juce::jlimit (1, numPartials, 1 + (int) std::floor ((x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()) * numPartials));
}

float getDbAt (juce::Rectangle<float> plot, float y)
{
    const float db = juce::jmap (y, plot.getBottom(), plot.getY(), offDb, topDb);
    return db < offDb + 1.0f ? offDb : juce::jlimit (offDb, 0.0f, std::round (db * 2.0f) / 2.0f);
}

void drawPartials (juce::Graphics& g, juce::Rectangle<float> r, const additive::Settings& s, const std::array<float, numPartials>* envelope,
                   float velocity, float lfo, std::function<bool (int)> lit)
{
    const auto plot = getPartialsPlot (r);
    auto toY = [&plot] (double db) { return juce::jmap ((float) juce::jlimit ((double) offDb, (double) topDb, db), offDb, topDb, plot.getBottom(), plot.getY()); };

    for (const double db : { 0.0, -20.0, -40.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
        drawLabel (g, juce::String ((int) db), { r.getX(), toY (db) - 7.0f, 30.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
    }

    const float w = plot.getWidth() / (float) numPartials;

    for (int n = 1; n <= numPartials; ++n)
    {
        const auto bar = juce::Rectangle<float> (plot.getX() + w * (float) (n - 1), plot.getY(), w, plot.getHeight()).reduced (w > 12.0f ? 2.0f : 1.0f, 0.0f);
        const auto c = getBarColour (n);
        const bool isLit = lit != nullptr && lit (n);

        if (isLit)
        {
            g.setColour (c.withAlpha (0.12f));
            g.fillRect (bar);
        }

        const float drawn = s.levelsDb[(size_t) n - 1];
        double heard = additive::getPartialGain (s, n, velocity, lfo);

        if (envelope != nullptr)
            heard *= (*envelope)[(size_t) n - 1];

        const double heardDb = juce::Decibels::gainToDecibels (heard, -100.0);

        if (heardDb > offDb)
        {
            g.setColour (c.withAlpha (isLit || lit == nullptr ? 0.9f : 0.45f));
            g.fillRect (bar.withTop (toY (heardDb)));
        }

        if (drawn > offDb + 0.05f)
        {
            g.setColour (theme::text.withAlpha (0.55f));
            g.drawRect (bar.withTop (toY (drawn)), 1.0f);
        }

        if (w >= 11.0f || n == 1 || n % 8 == 0)
            drawLabel (g, juce::String (n), juce::Rectangle<float> (24.0f, 13.0f).withCentre ({ bar.getCentreX(), plot.getBottom() + 9.5f }),
                       isLit ? c : theme::textDim, juce::Justification::centred);
    }
}

void drawWave (juce::Graphics& g, juce::Rectangle<float> r, const additive::Settings& s, double noteHz, double seconds, double cycles)
{
    auto all = additive::getPartials (s, noteHz, seconds, 1.0f, 12000.0);
    std::vector<additive::Partial> first;

    for (auto& p : all)
        if (p.n == 1)
            first.push_back (p);

    const auto sum = additive::getWave (all, cycles / noteHz, (int) r.getWidth());
    const auto alone = additive::getWave (first, cycles / noteHz, (int) r.getWidth());
    const float peak = juce::jmax (0.2f, getPeak (sum), getPeak (alone));

    g.setColour (theme::text.withAlpha (0.15f));
    g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.0f);

    const float scale = r.getHeight() * 0.45f / peak;
    drawLine (g, r, alone, scale, theme::textDim.withAlpha (0.6f), 1.2f);
    drawLine (g, r, sum, scale, colour, 2.0f);
}

void drawOverTime (juce::Graphics& g, juce::Rectangle<float> r, const additive::Settings& s, double age, double releasedAt)
{
    const auto& e = s.ampEnv;
    double hold = juce::jlimit (0.8, 3.0, (double) (e.attack + e.decay) + 0.6);

    if (releasedAt >= 0.0)
        hold = releasedAt;
    else if (age >= 0.0)
        hold = juce::jmax (hold, juce::jmin (age + 0.2, 6.0));

    const double total = hold + juce::jlimit (0.2, 3.0, (double) e.release);
    auto plot = r.withTrimmedLeft (34.0f).withTrimmedBottom (14.0f);
    auto toX = [&plot, total] (double t) { return plot.getX() + plot.getWidth() * (float) (t / total); };

    // As many rows as the highest harmonic that's on, at least 8, so a few harmonics aren't squeezed into the bottom.
    int rows = 8;

    for (int n = numPartials; n > rows; --n)
        if (s.levelsDb[(size_t) n - 1] > offDb + 0.05f)
            rows = n;

    const double norm = additive::getNormalisation (s);
    const float rowH = plot.getHeight() / (float) rows;
    constexpr float columnW = 3.0f;

    for (float x = 0.0f; x < plot.getWidth(); x += columnW)
    {
        const double t = total * (x + columnW * 0.5f) / plot.getWidth();
        const double amp = sub::getEnvelopeLevel (e, t, hold);

        for (int n = 1; n <= rows; ++n)
        {
            const double level = additive::getPartialGain (s, n) * norm * additive::getPartialEnvelope (s, n, t) * amp;
            const float alpha = juce::jlimit (0.0f, 1.0f, (float) (juce::Decibels::gainToDecibels (level, -100.0) + 60.0) / 60.0f);

            if (alpha > 0.01f)
            {
                g.setColour (getBarColour (n).withAlpha (alpha));
                g.fillRect (plot.getX() + x, plot.getBottom() - rowH * (float) n, columnW, juce::jmax (1.0f, rowH - 0.5f));
            }
        }
    }

    // The amp envelope over it.
    juce::Path ampLine;

    for (float x = 0.0f; x <= plot.getWidth(); x += 2.0f)
    {
        const float y = plot.getBottom() - plot.getHeight() * sub::getEnvelopeLevel (e, total * x / plot.getWidth(), hold);
        x <= 0.0f ? ampLine.startNewSubPath (plot.getX() + x, y) : ampLine.lineTo (plot.getX() + x, y);
    }

    g.setColour (ampColour);
    g.strokePath (ampLine, juce::PathStrokeType (1.6f));

    const float upX = toX (hold);
    const float dashes[] = { 4.0f, 3.0f };
    g.setColour (theme::textDim);
    g.drawDashedLine ({ upX, plot.getY(), upX, plot.getBottom() }, dashes, 2, 1.0f);

    if (age >= 0.0 && age <= total)
    {
        g.setColour (juce::Colours::white);
        g.fillRect (toX (age), plot.getY(), 1.5f, plot.getHeight());
    }

    drawLabel (g, additive::getOrdinal (rows), { r.getX(), plot.getY(), 32.0f, 12.0f }, theme::textDim, juce::Justification::centredRight);
    drawLabel (g, "1st", { r.getX(), plot.getBottom() - 12.0f, 32.0f, 12.0f }, theme::textDim, juce::Justification::centredRight);
    drawLabel (g, "0 s", { plot.getX(), plot.getBottom() + 1.0f, 40.0f, 13.0f });
    drawLabel (g, "key up", { upX - 63.0f, plot.getY(), 60.0f, 13.0f }, theme::textDim, juce::Justification::centredRight);
    drawLabel (g, fxui::formatSeconds (total), { plot.getRight() - 60.0f, plot.getBottom() + 1.0f, 60.0f, 13.0f }, theme::textDim,
               juce::Justification::centredRight);
}

void drawStretch (juce::Graphics& g, juce::Rectangle<float> r, float stretchPercent)
{
    constexpr int shown = 16;
    const double top = additive::getRatio (shown, stretchPercent) * 1.04;
    auto plot = r.reduced (10.0f, 0.0f).withTrimmedBottom (20.0f);
    auto toX = [&plot, top] (double ratio) { return plot.getX() + plot.getWidth() * (float) (ratio / top); };
    const float middle = plot.getCentreY();

    g.setColour (theme::grid);
    g.fillRect (plot.getX(), middle, plot.getWidth(), 1.0f);
    drawLabel (g, "perfect harmonics: whole-number multiples of 220 Hz", plot.withHeight (14.0f));
    drawLabel (g, "with the stretch at " + juce::String (juce::roundToInt (stretchPercent)) + "%", plot.withTop (plot.getBottom() - 14.0f), colour);

    for (int n = 1; n <= shown; ++n)
    {
        const float perfect = toX (n), stretched = toX (additive::getRatio (n, stretchPercent));
        g.setColour (theme::textDim);
        g.fillRect (perfect - 1.0f, middle - 50.0f, 2.0f, 46.0f);
        g.setColour (getBarColour (n));
        g.fillRect (stretched - 1.0f, middle + 4.0f, 2.0f, 46.0f);
        g.setColour (theme::textDim.withAlpha (0.4f));
        g.drawLine (perfect, middle - 4.0f, stretched, middle + 4.0f, 1.0f);

        if (n == 1 || n == 2 || n == 4 || n == 8 || n == 12 || n == 16)
        {
            drawLabel (g, juce::String (n), { perfect - 15.0f, middle - 68.0f, 30.0f, 14.0f }, theme::textDim, juce::Justification::centred);
            drawLabel (g, n == 1 ? juce::String ("0 ct") : formatCents (additive::getStretchCents (n, stretchPercent)),
                       { stretched - 30.0f, middle + 54.0f, 60.0f, 14.0f }, getBarColour (n), juce::Justification::centred);
        }
    }
}

void drawDrawbars (juce::Graphics& g, juce::Rectangle<float> r, const additive::Settings& s)
{
    struct Bar { const char* footage; int harmonic; juce::Colour c; };
    const Bar bars[] = { { "16'", 0, juce::Colour (0xff8b5a2b) }, { "5 1/3'", 0, juce::Colour (0xff8b5a2b) },
                         { "8'", 1, juce::Colour (0xffeeeeee) }, { "4'", 2, juce::Colour (0xffeeeeee) }, { "2 2/3'", 3, juce::Colour (0xff2a2a2a) },
                         { "2'", 4, juce::Colour (0xffeeeeee) }, { "1 3/5'", 5, juce::Colour (0xff2a2a2a) }, { "1 1/3'", 6, juce::Colour (0xff2a2a2a) },
                         { "1'", 8, juce::Colour (0xffeeeeee) } };

    const float w = r.getWidth() / 9.0f;
    const float railY = r.getY() + 22.0f, maxLength = r.getHeight() - 70.0f;
    g.setColour (theme::panel.brighter (0.2f));
    g.fillRect (r.getX(), railY - 6.0f, r.getWidth(), 6.0f);

    for (int i = 0; i < 9; ++i)
    {
        const auto& b = bars[i];
        const float x = r.getX() + w * (float) i + w * 0.3f, barW = w * 0.4f;
        int step = 0;

        if (b.harmonic > 0)
        {
            const double db = juce::Decibels::gainToDecibels (additive::getPartialGain (s, b.harmonic), -100.0);
            step = db <= offDb ? 0 : juce::jlimit (0, 8, juce::roundToInt (8.0 + db / 3.0));
        }

        const float length = 14.0f + maxLength * (float) step / 8.0f;
        auto bar = juce::Rectangle<float> (x, railY, barW, length);
        g.setColour (b.harmonic > 0 ? b.c : b.c.withAlpha (0.35f));
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (theme::textDim);
        g.drawRoundedRectangle (bar, 3.0f, 1.0f);

        drawLabel (g, b.footage, { x - 20.0f, r.getY(), barW + 40.0f, 14.0f }, theme::text, juce::Justification::centred);
        drawLabel (g, b.harmonic > 0 ? juce::String (step) : juce::String ("-"), { x - 20.0f, bar.getBottom() + 2.0f, barW + 40.0f, 14.0f },
                   theme::text, juce::Justification::centred);
        drawLabel (g, b.harmonic > 0 ? additive::getOrdinal (b.harmonic) : juce::String ("below"),
                   { x - 20.0f, r.getBottom() - 16.0f, barW + 40.0f, 14.0f }, b.harmonic > 0 ? getBarColour (b.harmonic) : theme::textDim,
                   juce::Justification::centred);
    }
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInAdditive& synth)
{
    BuiltInAdditive* processor = &synth;
    std::vector<fxui::Guide::Page> pages;

    pages.push_back ({ "Build it from sines", "What additive synthesis is",
        "Every steady sound is a mix of sines: one at the note's frequency, the fundamental, and others at 2, 3, 4... times it, the "
        "harmonics. How loud each one is gives the sound its colour, its timbre: it's why a flute and a violin playing the same A "
        "sound different. Joseph Fourier showed in 1822 that any repeating wave can be taken apart this way (see What is sound? "
        "in the wave editor).\n\n"
        "An additive synthesiser goes the other way: it builds a sound by adding sines together, each at its own level. Where a "
        "subtractive synth starts with a rich wave and takes away what it doesn't want, additive starts from nothing and adds what "
        "it does. Nothing is hidden: every harmonic is a bar you can see and set.\n\n"
        "The idea is older than electronics. Thaddeus Cahill's Telharmonium (1897) and the Hammond organ (1935) made their notes "
        "from spinning tonewheels, a near-sine each, mixed by drawbars. Computers made it practical to control many sines at once: "
        "the Synclavier and the Kawai K5 (1987) gave each sound dozens of harmonics to set, and software synths today can rebuild "
        "a recording from hundreds of them.\n\n"
        "The picture builds a saw one harmonic at a time: watch the grey sine get its edge as harmonics are added.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const int count = juce::jlimit (1, numPartials, 1 + (int) std::fmod (now() * 3.0, 40.0));
            additive::Settings s;
            const auto saw = additive::getShapeLevels (additive::Shape::saw);

            for (int n = 1; n <= numPartials; ++n)
                s.levelsDb[(size_t) n - 1] = n <= count ? saw[(size_t) n - 1] : offDb;

            drawCaption (g, r, "A saw from its first " + juce::String (count) + (count == 1 ? " harmonic" : " harmonics"),
                         "grey: the 1st alone; teal: all of them added up");
            auto bars = r.removeFromBottom (r.getHeight() * 0.4f);
            drawWave (g, r.withTrimmedBottom (10.0f), s, 1.0, -1.0, 2.0);
            drawPartials (g, bars, s, nullptr, 1.0f, 0.0f, [count] (int n) { return n == count; });
        } });

    pages.push_back ({ "The harmonic series", "Which notes are in a note",
        "The harmonics of a note aren't random: they sit at whole-number multiples of its frequency, and each one is a note of its "
        "own. For A3 at 220 Hz, the 2nd harmonic is A4 (440 Hz), an octave up; the 3rd is E5 (660 Hz), a fifth above that; the 4th "
        "is A5, two octaves up; the 5th is C#6, a major third above; then E6, and a G6 that is about a third of a semitone flat of "
        "the piano's.\n\n"
        "So every note already holds a major chord. The low harmonics are far apart (an octave, a fifth, a fourth) and the high ones "
        "ever closer, until from the 16th up they're less than a semitone apart. That's why the low harmonics change a sound's body "
        "and strength, and the high ones its brightness and edge.\n\n"
        "It's also why some chords sound smooth: in a fifth, the 3rd harmonic of the lower note is the 2nd of the upper one, so "
        "their harmonics meet instead of rubbing (see the chord guide).\n\n"
        "The picture lists the first 16 harmonics of A3 as notes, with how far each is from the nearest piano key in cents "
        "(hundredths of a semitone). The 7th, 11th and 13th are far from any key: the notes between the notes.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The first 16 harmonics of A3", "cents: how far from the nearest piano key");
            const float columnW = r.getWidth() / 2.0f, rowH = r.getHeight() / 8.0f;

            for (int n = 1; n <= 16; ++n)
            {
                const double exact = 57.0 + 12.0 * std::log2 ((double) n);
                const int nearest = juce::roundToInt (exact);
                const double cents = (exact - nearest) * 100.0;
                auto row = juce::Rectangle<float> (r.getX() + columnW * (float) ((n - 1) / 8), r.getY() + rowH * (float) ((n - 1) % 8), columnW - 20.0f, rowH - 4.0f);
                const auto c = getBarColour (n);

                g.setColour (c.withAlpha (0.12f));
                g.fillRoundedRectangle (row, 4.0f);
                g.setColour (c);
                g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
                g.drawText (additive::getOrdinal (n), row.withWidth (52.0f).withTrimmedLeft (8.0f), juce::Justification::centredLeft);
                g.setColour (theme::text);
                g.drawText (getNoteName (nearest), row.withTrimmedLeft (64.0f).withWidth (60.0f), juce::Justification::centredLeft);
                g.setFont (juce::FontOptions (12.0f));
                g.setColour (theme::textDim);
                g.drawText (formatHz (220.0 * n), row.withTrimmedLeft (128.0f).withWidth (80.0f), juce::Justification::centredLeft);
                g.setColour (std::abs (cents) > 25.0 ? evenColour : theme::textDim);
                g.drawText (std::abs (cents) < 0.5 ? juce::String ("on the key") : formatCents (cents), row.withTrimmedRight (10.0f),
                            juce::Justification::centredRight);
            }
        } });

    pages.push_back ({ "Saw, square and triangle, from sines", "The classic waves as recipes",
        "The waves of a subtractive synth are additive recipes too.\n\n"
        "Saw: every harmonic, the nth at 1/n of the first's level (-6 dB each time the harmonic number doubles). Bright and buzzy.\n"
        "Square: only the odd harmonics, also at 1/n. Hollow.\n"
        "Triangle: the odd harmonics at 1/n squared, so they fade fast, and every other one upside down (half a cycle out), which is "
        "what folds the wave into a triangle. Soft.\n"
        "Sine: the 1st harmonic alone. Pure.\n\n"
        "The picture adds up the harmonics of each, one more at a time, up to 32: watch the corners sharpen. Even with 32, the "
        "square has small ripples at its corners that overshoot by about 9% of the jump (the Gibbs phenomenon); more harmonics only "
        "squeeze them closer to the corner.\n\n"
        "Right-click the bars in the window to fill them with any of these shapes. The window's sines can't be turned upside down, "
        "so its triangle has the right harmonics but a rounder shape: it sounds the same (see Phase).",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const int count = juce::jlimit (1, numPartials, 1 + (int) std::fmod (now() * 3.0, 40.0));
            drawCaption (g, r, "Harmonics 1 to " + juce::String (count) + " added up", "with their true signs");
            const float w = (r.getWidth() - 3.0f * 16.0f) / 4.0f;
            const std::pair<sub::Wave, additive::Shape> shapes[] = { { sub::Wave::saw, additive::Shape::saw }, { sub::Wave::square, additive::Shape::square },
                                                                     { sub::Wave::triangle, additive::Shape::triangle }, { sub::Wave::sine, additive::Shape::sine } };

            for (auto& [waveShape, levels] : shapes)
            {
                auto box = r.removeFromLeft (w);
                r.removeFromLeft (16.0f);
                drawLabel (g, sub::getWaveName (waveShape), box.removeFromTop (15.0f), colour);
                auto top = box.removeFromTop (box.getHeight() * 0.5f);
                const auto values = getShapeWave (waveShape, count, (int) top.getWidth());
                g.setColour (theme::text.withAlpha (0.15f));
                g.fillRect (top.getX(), top.getCentreY(), top.getWidth(), 1.0f);
                drawLine (g, top.reduced (0.0f, 4.0f), values, top.getHeight() * 0.36f, colour, 1.8f);

                additive::Settings s;
                const auto all = additive::getShapeLevels (levels);

                for (int n = 1; n <= numPartials; ++n)
                    s.levelsDb[(size_t) n - 1] = n <= count ? all[(size_t) n - 1] : offDb;

                drawPartials (g, box.withTrimmedTop (8.0f), s, nullptr, 1.0f, 0.0f);
            }
        } });

    pages.push_back ({ "Brightness, odd and even", "Changing all the harmonics at once",
        "Drawing 32 bars one at a time is slow, so two knobs change them together.\n\n"
        "Brightness tilts them: every octave up, this many dB quieter (or louder). At -3 dB per octave the 2nd harmonic loses 3 dB, "
        "the 4th 6, the 8th 9 and the 32nd 15. It works like a gentle filter that moves with the note, and it's the quickest way to "
        "make a sound darker or brighter. With the LFO on it, the sound breathes.\n\n"
        "Even sets how much of the even harmonics you hear. Without them a sound is hollow: a square wave, or a clarinet in its low "
        "notes, whose tube is closed at one end and so makes mostly odd harmonics. The even ones are octaves of lower harmonics (the "
        "2nd is an octave of the 1st, the 6th of the 3rd), so they fill the sound out without adding new notes.\n\n"
        "Velocity tilts the harmonics too: softer notes are darker as well as quieter, as on most acoustic instruments.\n\n"
        "The picture swings the brightness of the window's sound slowly from dark to bright.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = withSomethingToShow (processor->getSettings());
            s.brightnessDb = juce::jlimit (additive::minBrightness, additive::maxBrightness, (float) (-3.0 + 6.0 * std::sin (now() * 0.9)));
            drawCaption (g, r, "Brightness " + additive::formatKnob (Knob::brightness, s), "outline: the bars; colour: what you hear");
            drawPartials (g, r, s, nullptr, 1.0f, 0.0f);
        } });

    pages.push_back ({ "Phase: same sound, different shape", "What the ear hears, and what it doesn't",
        "Each harmonic has a level and a phase: where in its cycle it starts. Change the phases and the wave can look completely "
        "different, yet for a steady note the ear hears almost the same sound: it listens to how loud each harmonic is, hardly to "
        "how they line up. Georg Ohm said so in 1843 and Hermann von Helmholtz showed it in the 1860s (Ohm's acoustic law).\n\n"
        "Phases do change the peaks. When many harmonics are about as loud, as in an organ, lined up they all reach their top "
        "together now and then, so the wave peaks high. Spread them the way Manfred Schroeder worked out in 1970 and the peaks "
        "stay low: the same sound with more headroom, which helps when big chords add up. A saw and a square are the other way "
        "round: lined up, they already peak low (a square, which is always at its peak, as low as any wave can), so there's "
        "nothing to gain, and spread leaves them lined up.\n\n"
        "Phase matters again when two copies of a sound are mixed, because then they can cancel (see Phase in What is sound?).\n\n"
        "The picture shows the window's sound both ways: the same harmonics, aligned above and spread below, with their peaks.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = withSomethingToShow (processor->getSettings());
            drawCaption (g, r, "The same harmonics at A3, two ways", "the peaks at full velocity, before the output knob");
            const float h = (r.getHeight() - 20.0f) / 2.0f;
            std::array<std::vector<float>, 2> waves;

            for (int i = 0; i < 2; ++i)
            {
                s.phases = i == 0 ? additive::Phases::aligned : additive::Phases::spread;
                waves[(size_t) i] = getTestWave (s, (int) r.getWidth());
            }

            // Both at the same scale, so the lower peaks show.
            const float loudest = juce::jmax (0.2f, getPeak (waves[0]), getPeak (waves[1]));

            for (int i = 0; i < 2; ++i)
            {
                auto box = r.removeFromTop (h);
                r.removeFromTop (20.0f);
                const float peak = getPeak (waves[(size_t) i]);
                drawLabel (g, juce::String (i == 0 ? "Aligned" : "Spread") + ": peaks at " + formatDb (juce::Decibels::gainToDecibels (peak * additive::voiceGain)),
                           box.removeFromTop (15.0f), colour);
                g.setColour (theme::text.withAlpha (0.15f));
                g.fillRect (box.getX(), box.getCentreY(), box.getWidth(), 1.0f);
                drawLine (g, box, waves[(size_t) i], box.getHeight() * 0.48f / loudest, colour, 1.6f);
            }
        } });

    pages.push_back ({ "Harmonics that move", "Swell and fade: an envelope for each",
        "Real instruments don't only get louder and softer: their colour changes during every note, harmonic by harmonic. Strike a "
        "piano key or pluck a string and every harmonic starts at once, but the high ones die away fastest, so the note turns rounder "
        "as it rings. A trumpet does the opposite: the low harmonics speak first and the high ones build up over the first tenth of a "
        "second or so, the \"bwaah\".\n\n"
        "In an additive synth each harmonic can have an envelope of its own. Here two knobs set them all at once:\n"
        "Swell: the high harmonics come in later. The 32nd takes the whole swell time, the others less in proportion.\n"
        "Fade: the high harmonics die sooner. The 2nd halves in the fade time, the 3rd twice as fast, the 4th three times...\n\n"
        "That's the difference between a sound that's alive and one that's a still picture. A subtractive synth gets something "
        "like it from its filter envelope; additive can move every harmonic on its own.\n\n"
        "The picture shows a note over time with the window's settings: each row is a harmonic, the 1st at the bottom, brighter "
        "where louder; the green line is the amp envelope.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = withSomethingToShow (processor->getSettings());
            juce::String note = "with the window's swell and fade";

            if (s.swellSeconds < 0.0005f && additive::isFadeOff (s.fadeSeconds))
            {
                s.fadeSeconds = 0.4f;
                note = "with a fade of 0.4 s, as the window's are off";
            }

            const double length = juce::jlimit (0.8, 3.0, (double) (s.ampEnv.attack + s.ampEnv.decay) + 0.6) + juce::jlimit (0.2, 3.0, (double) s.ampEnv.release);
            drawCaption (g, r, "A note over time", note);
            drawOverTime (g, r, s, std::fmod (now(), length + 0.5), -1.0);
        } });

    pages.push_back ({ "Stretched partials: pianos and bells", "When the partials aren't whole numbers",
        "So far every partial (every sine in the sound) has been a harmonic: an exact whole-number multiple of the note. Many real "
        "sounds aren't quite like that.\n\n"
        "A piano string is stiff, and a stiff string bends less easily into the short waves of its high partials, so they run sharp, "
        "more the higher they are: inharmonicity. It's part of why a piano sounds like a piano, and why tuners stretch its octaves "
        "a little wider than 2 to 1, so the high notes agree with the low notes' sharp harmonics (the Railsback curve).\n\n"
        "Bars, plates and bells are stiffer still: their partials are far from whole numbers, so they ring with a metallic clang "
        "instead of one clear note. Real bells have their own pattern (a hum an octave below the note, a minor third, a fifth...), "
        "which a stretch only hints at.\n\n"
        "Stretch pulls the partials sharp the way a stiff string does: the nth sits at n f sqrt((1 + B n^2) / (1 + B)), where B is "
        "the stiffness; the 1st stays on the note. A few percent sounds like a real string; past 50% it turns to metal.\n\n"
        "The picture shows the partials of A3: grey where perfect harmonics would be, coloured where the stretch puts them.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const float stretch = processor->getSettings().stretchPercent;
            drawCaption (g, r, stretch < 0.5f ? juce::String ("The window's stretch is 0, so this shows 60%") : "The window's stretch",
                         "cents: how far sharp of the perfect harmonic");
            drawStretch (g, r, stretch < 0.5f ? 60.0f : stretch);
        } });

    pages.push_back ({ "Drawbars: the Hammond organ", "Additive synthesis you play by hand",
        "The Hammond organ (1935) is the best-known additive instrument. Under the keys, 91 tonewheels (on a B-3) spin next to "
        "magnetic pickups, each making a near-sine; for every key, nine of them are mixed by nine drawbars. Pulling a drawbar out "
        "turns its harmonic up, in steps from 0 to 8, about 3 dB a step.\n\n"
        "The drawbars are named in feet, after the pipe lengths of church organs: 8' is the note itself, 4' an octave up (the 2nd "
        "harmonic), 2 2/3' the 3rd, 2' the 4th, 1 3/5' the 5th, 1 1/3' the 6th and 1' the 8th. The white ones are octaves of the "
        "note, the black ones the fifths and thirds in between. The two brown ones, 16' and 5 1/3', sound an octave below and a "
        "fifth above that, under the note, so this synth, which builds up from the note, leaves them out.\n\n"
        "Organists write a sound (a registration) as nine numbers, one per drawbar: the Drawbar organ starting point is "
        "00 8888 005, the four middle drawbars full and the 1' at 5.\n\n"
        "The picture shows the drawbars for the window's sound: how far each would be pulled out.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The window's sound as drawbars", "0 in, 8 all the way out");
            drawDrawbars (g, r, processor->getSettings());
        } });

    pages.push_back ({ "Shaping the note", "The amp envelope, vibrato and voices",
        "The harmonics set a note's colour; the amp envelope sets how its level runs: attack (how long it takes to rise), decay "
        "(how long to fall to the sustain level), sustain (the level held while the key is down) and release (how long it fades "
        "after the key comes up). An organ: a few milliseconds of attack and release, sustain at 100%. A struck or plucked sound: "
        "attack near 0, a long decay, sustain at 0. A pad: slow attack and release.\n\n"
        "The LFO adds movement: on the pitch it's vibrato (5-7 Hz, small), on the brightness a slow breathing, on the even "
        "harmonics a slow change from full to hollow.\n\n"
        "Poly plays chords, a voice per note. Mono plays one note at a time, with legato notes taking over without a new attack, "
        "and Glide slides between them, as for winds, basses and leads. Velocity makes soft notes quieter and darker.\n\n"
        "The picture shows the window's amp envelope, a note running along it, and the LFO.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double hold = juce::jlimit (0.8, 3.0, (double) (s.ampEnv.attack + s.ampEnv.decay) + 0.6);
            const double t = std::fmod (now(), hold + s.ampEnv.release + 0.4);
            drawCaption (g, r, "The amp envelope", "A " + additive::formatKnob (Knob::ampAttack, s) + ", D " + additive::formatKnob (Knob::ampDecay, s)
                                                      + ", S " + additive::formatKnob (Knob::ampSustain, s) + ", R " + additive::formatKnob (Knob::ampRelease, s));
            auto lfoArea = r.removeFromBottom (r.getHeight() * 0.3f);
            subtext::drawEnvelope (g, r.withTrimmedBottom (12.0f), s.ampEnv, ampColour, "a note held, then let go", t, hold, true);

            std::vector<float> lfo ((size_t) juce::jmax (2, (int) lfoArea.getWidth()));
            const double rateHz = juce::jmax (0.2, (double) s.lfoRateHz);

            for (size_t i = 0; i < lfo.size(); ++i)
                lfo[i] = (float) std::sin (juce::MathConstants<double>::twoPi * rateHz * (now() + 3.0 * (double) i / (double) (lfo.size() - 1)));

            g.setColour (theme::text.withAlpha (0.15f));
            g.fillRect (lfoArea.getX(), lfoArea.getCentreY(), lfoArea.getWidth(), 1.0f);
            drawLine (g, lfoArea, lfo, lfoArea.getHeight() * 0.42f * juce::jmax (0.15f, s.lfoDepthPercent / 100.0f), lfoColour, 2.0f);
            drawLabel (g, "3 seconds of the LFO at " + additive::formatKnob (Knob::lfoRate, s) + ", moving the "
                              + additive::getLfoTargetName (s.lfoTarget).toLowerCase(), lfoArea.withTop (lfoArea.getBottom() - 14.0f));
        } });

    pages.push_back ({ "Building a sound, step by step", "From one sine to your own sound",
        "Start from Init (one sine) and work up the harmonics, listening after every step. Play a note or a phrase over and over "
        "while you draw.\n\n"
        "1. The body: the 2nd to 4th harmonics. An octave, a fifth above it, two octaves: they make the sound fuller and stronger "
        "without making it brighter. An organ or a flute is mostly these.\n"
        "2. The brightness: the 5th harmonic and up. A few soft ones for warmth; many, falling slowly, for a bright, buzzy sound. "
        "Use Brightness to tilt them all.\n"
        "3. Odd or even: turn the even harmonics down for a hollow, reedy sound.\n"
        "4. In tune or stretched: 0 for organs and winds, a little for strings and keys, a lot for bells.\n"
        "5. Movement within the note: a swell for blown and bowed sounds, a fade for struck and plucked ones.\n"
        "6. The amp envelope: how the note starts and ends.\n"
        "7. Vibrato, voices and the level.\n\n"
        "When a sound isn't right: too thin? More of the low harmonics. Too dull? More high ones, or more brightness. Too buzzy or "
        "harsh? Less brightness, or turn down the harmonics from the 7th up. Lifeless? A fade or a swell, a touch of vibrato, or a "
        "little stretch.\n\n"
        "The picture lights the harmonics each step is about, on the window's sound.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const int step = (int) std::fmod (now() / 2.0, 4.0);
            const juce::StringArray names { "1. the body: the 2nd to 4th", "2. the brightness: the 5th and up", "3. odd or even: the even ones",
                                            "4. and the 1st, the note itself, under them all" };
            drawCaption (g, r, "Building a sound up the harmonics", names[step]);
            drawPartials (g, r, withSomethingToShow (processor->getSettings()), nullptr, 1.0f, 0.0f, [step] (int n)
            {
                switch (step)
                {
                    case 0:  return n >= 2 && n <= 4;
                    case 1:  return n >= 5;
                    case 2:  return n % 2 == 0;
                    default: return n == 1;
                }
            });
        } });

    pages.push_back ({ "Recipes for common sounds", "Organ, flute, reed, keys, bell, pluck, brass, voice",
        "Every one of these is in the starting points, with notes on why each part is set the way it is. Load one, play it, and "
        "change one thing at a time.\n\n"
        "Organ: the 1st to 4th harmonics full, maybe the 8th; sustain 100%, short attack and release, a little vibrato, spread "
        "phases.\n\n"
        "Flute: the 1st strong and a few soft harmonics fading fast; a short swell for the breath, an attack of 50-100 ms, vibrato.\n\n"
        "Reed (clarinet-like): the odd harmonics only, a little darker; mono with a short glide.\n\n"
        "Electric keys: a few strong low harmonics, a fade of about half a second, sustain 0 with a long decay, some velocity.\n\n"
        "Bell: many harmonics, falling gently, a big stretch, a fade of a second or so and a long decay and release.\n\n"
        "Plucked string: every harmonic at 1/n, a little darker, a fade of a quarter of a second, a touch of stretch.\n\n"
        "Brass: every harmonic at 1/n with a swell of a quarter of a second, an attack of about 30 ms, vibrato.\n\n"
        "A sung vowel: peaks in the harmonics near the vowel's formants, for \"ah\" around 700 Hz, 1.2 kHz and 2.5 kHz.\n\n"
        "The picture shows the window's sound over a note.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const double age = processor->noteAge.load();
            drawCaption (g, r, "The sound in the window now, over a note", "Play a note to see it run");
            drawOverTime (g, r, processor->getSettings(), age, age >= 0.0 ? (double) processor->releaseAge.load() : -1.0);
        } });

    return pages;
}
} // namespace daw::additext
