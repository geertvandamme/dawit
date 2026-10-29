#include "FmGuide.h"

namespace daw::fmtext
{
using fm::Knob;
using fm::numOperators;

namespace
{
    constexpr float floorDb = -60.0f;

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    juce::String formatDb (double db)
    {
        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    juce::String times()
    {
        return juce::String (juce::CharPointer_UTF8 ("\xc3\x97"));
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
                    juce::Justification justification = juce::Justification::centredLeft, float size = 11.0f)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size));
        g.drawFittedText (text, r.toNearestInt(), justification, 1, 0.8f);
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

    float getPeak (const std::vector<float>& wave)
    {
        float peak = 0.0f;

        for (auto v : wave)
            peak = juce::jmax (peak, std::abs (v));

        return peak;
    }

    double now()
    {
        return juce::Time::getMillisecondCounterHiRes() / 1000.0;
    }

    std::array<float, numOperators> getFull()
    {
        std::array<float, numOperators> full;
        full.fill (1.0f);
        return full;
    }

    bool isOn (const fm::Settings& s, int op)
    {
        return s.ops[(size_t) op].levelPercent > 0.05f;
    }

    /** Operators as words: "1", "1 and 3", "2, 3 and 4". */
    juce::String listOperators (int bits)
    {
        juce::StringArray names;

        for (int op = 0; op < numOperators; ++op)
            if ((bits & (1 << op)) != 0)
                names.add (juce::String (op + 1));

        if (names.size() <= 1)
            return names.joinIntoString ({});

        const auto last = names[names.size() - 1];
        names.remove (names.size() - 1);
        return names.joinIntoString (", ") + " and " + last;
    }

    /** The window's sound, or the simple 1 : 1 sound when nothing in it modulates anything. */
    fm::Settings withSomethingToShow (const fm::Settings& s, bool& replaced)
    {
        for (int op = 0; op < numOperators; ++op)
        {
            if (! fm::isCarrier (s, op) && isOn (s, op))
            {
                replaced = false;
                return s;
            }
        }

        replaced = true;
        return fm::getPresets()[1].settings;
    }

    /** The cells of each algorithm's picture: column, and row from the bottom, for operators 1 to 4. */
    const std::array<std::array<juce::Point<float>, numOperators>, fm::numAlgorithms>& getCells()
    {
        static const std::array<std::array<juce::Point<float>, numOperators>, fm::numAlgorithms> cells {{
            {{ { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 2.0f }, { 0.0f, 3.0f } }},
            {{ { 0.5f, 0.0f }, { 0.5f, 1.0f }, { 0.0f, 2.0f }, { 1.0f, 2.0f } }},
            {{ { 0.5f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 2.0f } }},
            {{ { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 2.0f, 1.0f } }},
            {{ { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } }},
            {{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 1.0f, 1.0f } }},
            {{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 2.0f, 1.0f } }},
            {{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 3.0f, 0.0f } }} }};

        return cells;
    }

    constexpr float outHeight = 26.0f;

    /** One ADSR's shape across an area, held for `hold` seconds of `total`. */
    juce::Path getEnvelopePath (const sub::Envelope& e, juce::Rectangle<float> r, double hold, double total)
    {
        juce::Path path;

        for (float x = 0.0f; x <= r.getWidth(); x += 1.5f)
        {
            const float y = r.getBottom() - r.getHeight() * sub::getEnvelopeLevel (e, total * x / r.getWidth(), hold);
            x <= 0.0f ? path.startNewSubPath (r.getX() + x, y) : path.lineTo (r.getX() + x, y);
        }

        return path;
    }
}

juce::Colour getOperatorColour (int op)
{
    static const juce::Colour colours[] = { juce::Colour (0xff5cc8ff), juce::Colour (0xffffb454), juce::Colour (0xff5fd38d), juce::Colour (0xffff8fb1) };
    return colours[juce::jlimit (0, numOperators - 1, op)];
}

//==============================================================================
juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::coarse:    return "Ratio";
        case (int) Knob::fine:      return "Fine";
        case (int) Knob::level:     return "Level: loudness or index";
        case (int) Knob::attack:    return "Envelope: attack";
        case (int) Knob::decay:     return "Envelope: decay";
        case (int) Knob::sustain:   return "Envelope: sustain";
        case (int) Knob::release:   return "Envelope: release";
        case (int) Knob::algorithm: return "Algorithm";
        case (int) Knob::feedback:  return "Feedback";
        case (int) Knob::lfoRate:   return "LFO rate";
        case (int) Knob::lfoDepth:  return "LFO depth";
        case (int) Knob::lfoTarget: return "LFO to";
        case (int) Knob::glide:     return "Glide";
        case (int) Knob::mode:      return "Voices: poly or mono";
        case (int) Knob::velocity:  return "Velocity";
        case (int) Knob::output:    return "Output";
        case operators:             return "The operators";
        case spectrum:              return "The spectrum";
        case wave:                  return "The wave";
        case selectOperator:        return "Which operator";
        case fxui::overview:        return "DawIt FM";
        case fxui::presets:         return "Starting points";
        case fxui::clap:            return "Play a note";
        case fxui::listen:          return "Hear the carriers alone";
        case fxui::guide:           return "What is FM synthesis?";
        case fxui::history:         return "The last 4 seconds";
        default:                    return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::coarse:
            return "How fast this operator runs, as a multiple of the note: 1 is the note itself, 2 an octave up, 0.5 an octave down. "
                   "For a modulator, the ratio sets where the new partials (sidebands) go: they sit at the carrier's frequency plus and "
                   "minus whole multiples of the modulator's. Whole-number ratios put them on the note's harmonics, so the sound has a "
                   "clear pitch: 1 : 1 gives every harmonic, 1 : 2 only the odd ones, 1 : 3 all but every third. Ratios in between give "
                   "partials between the harmonics: bells, metal, clangs.";

        case (int) Knob::fine:
            return "Raises the ratio by a percentage of itself, as on a DX7: a ratio of 1 with fine at 41% is 1.41. A modulator off a "
                   "whole number makes inharmonic, bell-like sounds. A carrier a hair off (1 or 2%) beats slowly against another at the "
                   "whole number, like two instruments a little out of tune: a chorus.";

        case (int) Knob::level:
            return "What the level does depends on the operator's place in the algorithm.\n\n"
                   "A carrier is heard: its level is how loud it is, like a mixer's fader.\n\n"
                   "A modulator isn't heard itself: its level is how hard it swings the operator below it, the modulation index. At 0 "
                   "nothing happens; at 10% (an index of 1) the first sidebands appear, about 44% as loud as the carrier; at 24% (2.4) "
                   "the carrier's own frequency vanishes and the sound is all sidebands; higher still, the sidebands spread further "
                   "and the sound gets brighter and harsher. The index is FM's brightness knob.";

        case (int) Knob::attack:
            return "How long this operator's envelope takes to rise from nothing to full when a key is pressed. On a carrier it's how the "
                   "note starts: 1-5 ms for a hit or pluck, 50-100 ms for breath, 0.5 s and up for a pad. On a modulator it's how the "
                   "brightness comes in: a slow modulator attack makes the note bloom, as brass does.";

        case (int) Knob::decay:
            return "After the attack, how long this operator's envelope takes to fall to the sustain level while the key is held. On a "
                   "modulator this is the most-used trick in FM: a short decay makes the note bright for a moment and then mellow, the "
                   "sound of every struck or plucked instrument. On a carrier with a low sustain, it's how long the note lasts.";

        case (int) Knob::sustain:
            return "The level this operator's envelope holds while the key stays down, after the decay. 100%: as at its peak, like an "
                   "organ. 0%: it dies away even with the key held: on a carrier, a struck note; on a modulator, a sound that turns "
                   "into a pure sine as it rings.";

        case (int) Knob::release:
            return "How long this operator's envelope takes to fade to nothing after the key is let go. On the carriers it's how long the "
                   "note rings on; the note ends when every carrier's envelope has finished.";

        case (int) Knob::algorithm:
            return "How the four operators are wired: which ones modulate which, and which are heard (the carriers, at the bottom of "
                   "the picture). A stack of four makes one very rich sound; two pairs make two sounds layered, such as the body and "
                   "the tine of an electric piano; four sines side by side are an organ's drawbars. Yamaha's four-operator synths, such "
                   "as the DX21, DX100 and TX81Z, offered eight; the six-operator DX7 had 32.";

        case (int) Knob::feedback:
            return "Operator 4 modulates itself: its own output is fed back into its phase. A little adds a few harmonics; around half "
                   "it's like a saw wave, every harmonic falling away; at the top it starts to turn into noise. It's how FM makes "
                   "buzzy, saw-like sounds from sines, for leads, basses and brass.";

        case (int) Knob::lfoRate:
            return "The LFO (low-frequency oscillator) is a wave too slow to hear as a note, used to move something up and down over and "
                   "over. Rate is how many times a second: 5-7 Hz is the speed of a singer's vibrato; 0.1-1 Hz a slow change you feel "
                   "more than hear.";

        case (int) Knob::lfoDepth:
            return "How far the LFO moves its target: at 100%, the pitch by 2 semitones up and down, the modulators from nothing to twice "
                   "their level, or the volume all the way down and back. A little goes a long way on the pitch.";

        case (int) Knob::lfoTarget:
            return "What the LFO moves. Pitch: vibrato. Brightness: every modulator's level goes up and down together, so the sound gets "
                   "brighter and duller, like a slow filter sweep. Volume: tremolo.";

        case (int) Knob::glide:
            return "Glide (portamento): instead of jumping to the next note, the pitch slides there, taking about this long. In mono only "
                   "overlapping (legato) notes slide; in poly every new note slides from the last one played.";

        case (int) Knob::mode:
            return "Poly: each note gets its own voice (up to 16), for chords and pads. Mono: one note at a time; a note played while "
                   "another is held takes over without starting the envelopes again (legato), and glides if Glide is on. Basses, leads "
                   "and wind instruments usually play mono.";

        case (int) Knob::velocity:
            return "How much playing softer makes a note quieter and duller. It turns down the carriers and the modulators together, so "
                   "a soft note has a lower index and fewer sidebands: darker, as on an acoustic instrument played gently. At 100% a "
                   "note played half as hard is 6 dB quieter, with half the index. At 0% every note is the same, as on an organ.";

        case (int) Knob::output:
            return "The synth's level going into the track's effects and fader. Modulation doesn't make the sound louder (it moves the "
                   "carrier's power into its sidebands), and several carriers are turned down together so their power is one full "
                   "sine's. Still leave headroom: chords add up.";

        case operators:
            return "The four operators, each a sine at a ratio of the note, wired by the algorithm: a modulator sits above what it "
                   "modulates, and the carriers at the bottom go to the output. Each box shows the ratio and the level (for a "
                   "modulator, its index) and glows as its envelope runs. The loop on operator 4 is its feedback.\n\n"
                   "Click an operator to show its knobs below; drag up or down on it to change its level. Right-click to choose the "
                   "algorithm.";

        case spectrum:
            return "The partials of the test note, A3, as they are now: each bar is one sine in the sound, on a line from 0 Hz up, with "
                   "the note's harmonics numbered. Purple bars are harmonics; amber ones fall between them, which sounds metallic. The "
                   "ticks underneath are where the operators run. Turn a modulator's level up and watch the sidebands spread out from "
                   "its carrier, evenly spaced by the modulator's frequency.";

        case wave:
            return "A few cycles of the test note: the sound (coloured) and the carriers as plain sines, without their modulators (grey). "
                   "Modulation bends the sine's shape, squeezing and stretching its cycles; every bend is a sideband.";

        case selectOperator:
            return "Which operator the knobs below belong to. Click to step through 1 to 4, or click an operator in the picture.";

        case fxui::clap:
            return "Plays the test note, A3, for 0.8 seconds through the synth, even while the song is stopped. Or play the track's MIDI "
                   "notes, or a MIDI keyboard on this track.";

        case fxui::listen:
            return "Turns every modulator off for a moment (and the feedback), so you hear the carriers as plain sines at their levels "
                   "and envelopes. Switch back and forth: everything you hear in between is the modulation.";

        case fxui::presets:
            return "Sounds to start from, each telling why it's made the way it is and what every operator does in it. Pick one, then "
                   "change one thing at a time and listen.";

        case fxui::guide:
            return "A guide for beginners: what FM is and where it came from, carriers and modulators, sidebands and the index, "
                   "harmonic and clangorous ratios, envelopes that move the brightness, algorithms, feedback, and how to build a "
                   "sound.";

        case fxui::history:
            return "The last 4 seconds: grey how loud the carriers would peak without modulation, coloured the whole sound. They stay "
                   "close: FM changes a sound's colour, hardly its level.";

        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::coarse:    return "1 for carriers; 1 or 2 for modulators of warm sounds; 3-7 for bright and nasal ones; 14 for an electric piano's tine.";
        case (int) Knob::fine:      return "0 for most; 1-2% on one carrier for a slow chorus; 20-50% on a modulator for bells.";
        case (int) Knob::level:     return "Carriers 70-100%; modulators 5-30% (an index of 0.5-3) for most sounds, 50% and up for bells and noise.";
        case (int) Knob::attack:    return "1-5 ms for keys, basses and bells; 30-100 ms for winds and brass; 0.5-2 s for pads.";
        case (int) Knob::decay:     return "On modulators 0.1-1 s for plucks and keys; on carriers 1-4 s for keys, 5-15 s for bells.";
        case (int) Knob::sustain:   return "Carriers 100% for organs and pads, 0% for struck sounds; modulators 0-30% for sounds that mellow.";
        case (int) Knob::release:   return "20-100 ms for organs and basses; 0.3-0.5 s for keys; 1-3 s for pads; seconds for bells.";
        case (int) Knob::algorithm: return "Stack for one rich sound; Two pairs for layered sounds like an electric piano; Four sines for organs.";
        case (int) Knob::feedback:  return "Off for clean sounds; 10-30% for warmth; 50-70% for saw-like leads and basses.";
        case (int) Knob::lfoRate:   return "Vibrato 5-7 Hz; slow changes in brightness 0.1-0.5 Hz.";
        case (int) Knob::lfoDepth:  return "Vibrato 3-6%; brightness 20-50%; tremolo 20-40%.";
        case (int) Knob::lfoTarget: return "Pitch for vibrato, brightness for pads that breathe, volume for tremolo.";
        case (int) Knob::glide:     return "Off for most sounds; 30-80 ms for leads, basses and winds.";
        case (int) Knob::mode:      return "Mono for basses, leads and winds; poly for keys, bells, organs and pads.";
        case (int) Knob::velocity:  return "50-70% for keys and plucks; 0 for organs; 20-40% for pads.";
        case (int) Knob::output:    return "0 dB for one-note parts; -3 to -6 dB for chords.";
        default:                    return {};
    }
}

double getPeakDb (const fm::Settings& s)
{
    const auto wave = fm::render (s, sub::testNoteHz, getFull(), 96000.0, 9600);
    return juce::Decibels::gainToDecibels (getPeak (wave) * fm::voiceGain, -100.0f) + s.outputDb;
}

double getHoldSeconds (const fm::Settings& s)
{
    double longest = 0.0;

    for (int op = 0; op < numOperators; ++op)
        if (isOn (s, op))
            longest = juce::jmax (longest, (double) (s.ops[(size_t) op].env.attack + s.ops[(size_t) op].env.decay));

    return juce::jlimit (0.8, 3.0, longest + 0.6);
}

juce::String describeOperator (const fm::Settings& s, int op)
{
    const auto& o = s.ops[(size_t) op];
    const juce::String name = "Operator " + juce::String (op + 1) + ", " + times() + fm::formatRatio (fm::getRatio (o));

    if (! isOn (s, op))
        return name + ": off (level 0)";

    if (fm::isCarrier (s, op))
        return name + ": a carrier at " + fm::formatKnob (Knob::level, s, op) + ", heard at " + formatHz (sub::testNoteHz * fm::getRatio (o)) + " at A3";

    return name + ": modulates " + listOperators (fm::getTargets (s, op)) + ", index " + juce::String (fm::getIndex (s, op), 1);
}

juce::String describeSettings (int t, const fm::Settings& s, int op)
{
    const auto& o = s.ops[(size_t) juce::jlimit (0, numOperators - 1, op)];
    const double hz = sub::testNoteHz * fm::getRatio (o);
    const bool carrier = fm::isCarrier (s, op);
    const juce::String who = "Operator " + juce::String (op + 1);

    switch (t)
    {
        case (int) Knob::coarse:
        case (int) Knob::fine:
        {
            juce::String text = who + " runs at " + fm::formatRatio (fm::getRatio (o)) + " times the note: " + formatHz (hz) + " at A3.";

            if (carrier)
                return text + " It's a carrier, so it's heard at that frequency, with any sidebands around it.";

            // Where the sidebands land on the first operator it modulates, as one sine on another would put them.
            int target = 0;

            while (target < numOperators && (fm::getTargets (s, op) & (1 << target)) == 0)
                ++target;

            if (target >= numOperators)
                return text;

            const double index = juce::jmax (1.0, fm::getIndex (s, op));
            auto peaks = fm::getSidebands (fm::getRatio (s.ops[(size_t) target]), fm::getRatio (o), index);
            std::sort (peaks.begin(), peaks.end(), [] (const fm::Peak& a, const fm::Peak& b) { return a.amplitude > b.amplitude; });
            juce::StringArray loudest;

            for (size_t i = 0; i < peaks.size() && loudest.size() < 4; ++i)
                loudest.add (fm::formatRatio (peaks[i].hz));

            return text + " On operator " + juce::String (target + 1) + ", the sidebands are " + formatHz (hz) + " apart; at an index of "
                   + juce::String (index, 1) + " the loudest are at " + loudest.joinIntoString (", ") + " times the note.";
        }

        case (int) Knob::level:
        {
            if (carrier)
                return who + " is a carrier at " + fm::formatKnob (Knob::level, s, op) + ": "
                       + formatDb (juce::Decibels::gainToDecibels (fm::getAmplitude (s, op), -100.0) + fm::sinePeakDb + s.outputDb)
                       + " at its peak, played alone at full velocity.";

            const double index = fm::getIndex (s, op);

            if (index < 0.005)
                return who + " is a modulator at 0: it changes nothing. Turn it up to add sidebands.";

            return who + " is a modulator with an index of " + juce::String (index, 2) + ". On one sine it would leave the carrier at "
                   + juce::String (juce::roundToInt (std::abs (fm::getBessel (0, index)) * 100.0)) + "% and put the first sidebands at "
                   + juce::String (juce::roundToInt (std::abs (fm::getBessel (1, index)) * 100.0)) + "%; Carson's rule spreads the sound over about "
                   + formatHz (fm::getCarsonHz (index, hz)) + ".";
        }

        case (int) Knob::attack:
        case (int) Knob::decay:
        case (int) Knob::sustain:
        case (int) Knob::release:
            return who + " rises in " + fm::formatKnob (Knob::attack, s, op) + ", falls to " + fm::formatKnob (Knob::sustain, s, op) + " in "
                   + fm::formatKnob (Knob::decay, s, op) + (o.env.sustain < 0.005f ? " and is gone, even if held" : " and holds there")
                   + ", and fades out in " + fm::formatKnob (Knob::release, s, op) + " after the key is let go. "
                   + (carrier ? "It's a carrier, so this is how its loudness runs."
                              : "It's a modulator, so this is how the brightness runs: an index of " + juce::String (fm::getIndex (s, op), 1)
                                    + " at the peak, " + juce::String (fm::getIndex (s, op) * o.env.sustain, 1) + " while held.");

        case (int) Knob::algorithm:
        {
            const auto& a = fm::getAlgorithm (s.algorithm);
            return a.name + " (" + a.wiring + "): " + listOperators (a.carriers) + (juce::isPowerOfTwo (a.carriers) ? " is" : " are")
                   + " heard, the others modulate.";
        }

        case (int) Knob::feedback:
            return s.feedbackPercent < 0.5f ? juce::String ("Off: operator 4 is a plain sine.")
                                            : fm::formatKnob (Knob::feedback, s) + ": operator 4 feeds back " + juce::String (s.feedbackPercent / 100.0 * fm::maxFeedback, 2)
                                                  + " times its output into its own phase" + (isOn (s, 3) ? "." : ", but its level is 0, so it isn't heard.");

        case (int) Knob::lfoRate:
            return fm::formatKnob (Knob::lfoRate, s) + ": a cycle every " + juce::String (1000.0 / s.lfoRateHz, 0) + " ms"
                   + (s.lfoDepthPercent < 0.5f ? ", but the depth is 0, so it moves nothing." : ".");

        case (int) Knob::lfoDepth:
        case (int) Knob::lfoTarget:
        {
            const float d = s.lfoDepthPercent / 100.0f;

            if (d < 0.005f)
                return "Depth 0: the LFO moves nothing.";

            switch (s.lfoTarget)
            {
                case fm::LfoTarget::pitch:      return "Vibrato: the pitch goes up and down by " + juce::String (d * fm::lfoPitchSemitones, 2) + " semitones.";
                case fm::LfoTarget::brightness: return "The modulators swing between " + juce::String (juce::roundToInt (100.0f - d * 100.0f)) + "% and "
                                                       + juce::String (juce::roundToInt (100.0f + d * 100.0f)) + "% of their level.";
                case fm::LfoTarget::volume:     return "Tremolo: the volume dips to " + juce::String (juce::roundToInt (100.0f - d * 100.0f)) + "% and back.";
            }

            return {};
        }

        case (int) Knob::glide:
            return s.glideSeconds < 0.0005f ? juce::String ("Off: notes jump straight to their pitch.")
                                            : "Notes slide to their pitch in about " + fm::formatKnob (Knob::glide, s)
                                                  + (s.mode == sub::Mode::mono ? ", when you play them legato." : ", from the last note played.");

        case (int) Knob::mode:
            return s.mode == sub::Mode::mono ? "Mono: one note at a time, legato notes don't restart the envelopes."
                                             : "Poly: up to " + juce::String (BuiltInFm::numVoices) + " notes at once.";

        case (int) Knob::velocity:
        {
            const float half = fm::getVelocityScale (s, 0.5f);
            return "A note played half as hard is " + formatDb (juce::Decibels::gainToDecibels (half)) + ", with its modulators' index at "
                   + juce::String (juce::roundToInt (half * 100.0f)) + "%.";
        }

        case (int) Knob::output:
            return "A3 at full velocity peaks at about " + formatDb (getPeakDb (s)) + "; one full carrier would peak at "
                   + formatDb (fm::sinePeakDb + s.outputDb) + ".";

        default:
            return {};
    }
}

//==============================================================================
juce::Rectangle<float> getOperatorBox (juce::Rectangle<float> area, int algorithm, int op)
{
    const auto& cells = getCells()[(size_t) juce::jlimit (0, fm::numAlgorithms - 1, algorithm)];
    float maxCol = 0.0f, maxRow = 0.0f;

    for (auto& c : cells)
    {
        maxCol = juce::jmax (maxCol, c.x);
        maxRow = juce::jmax (maxRow, c.y);
    }

    // At least three rows and columns of room, so a small algorithm doesn't get giant boxes.
    const float cellH = (area.getHeight() - outHeight) / juce::jmax (3.0f, maxRow + 1.0f);
    const float cellW = juce::jmin (area.getWidth() / juce::jmax (3.0f, maxCol + 1.0f), cellH * 1.9f);
    const float left = area.getCentreX() - (maxCol + 1.0f) * cellW * 0.5f;
    const float top = area.getY() + (area.getHeight() - outHeight - (maxRow + 1.0f) * cellH) * 0.5f;
    const auto c = cells[(size_t) op];

    return juce::Rectangle<float> (left + c.x * cellW, top + (maxRow - c.y) * cellH, cellW, cellH).reduced (cellW * 0.1f, cellH * 0.16f);
}

int getOperatorAt (juce::Rectangle<float> area, int algorithm, juce::Point<float> point)
{
    for (int op = 0; op < numOperators; ++op)
        if (getOperatorBox (area, algorithm, op).expanded (4.0f).contains (point))
            return op;

    return -1;
}

void drawAlgorithm (juce::Graphics& g, juce::Rectangle<float> r, const fm::Settings& s, int selected, const std::array<float, numOperators>* envelopes,
                    bool detailed)
{
    const auto& a = fm::getAlgorithm (s.algorithm);
    const float outY = r.getBottom() - outHeight * 0.5f;
    std::array<juce::Rectangle<float>, numOperators> boxes;

    for (int op = 0; op < numOperators; ++op)
        boxes[(size_t) op] = getOperatorBox (r, s.algorithm, op);

    // The wiring first, under the boxes: modulators down into what they modulate, carriers into the output.
    float outLeft = r.getRight(), outRight = r.getX();

    for (int op = 0; op < numOperators; ++op)
    {
        const auto& box = boxes[(size_t) op];

        for (int target = 0; target < numOperators; ++target)
        {
            if ((a.modulatedBy[(size_t) target] & (1 << op)) == 0)
                continue;

            const auto& to = boxes[(size_t) target];
            juce::Path arrow;
            arrow.addArrow ({ box.getCentreX(), box.getBottom(), to.getCentreX(), to.getY() - 1.0f }, 1.6f, 9.0f, 8.0f);
            g.setColour (getOperatorColour (op).withAlpha (isOn (s, op) ? 0.9f : 0.3f));
            g.fillPath (arrow);
        }

        if ((a.carriers & (1 << op)) != 0)
        {
            g.setColour (theme::textDim);
            g.drawLine (box.getCentreX(), box.getBottom(), box.getCentreX(), outY, 1.4f);
            outLeft = juce::jmin (outLeft, box.getCentreX());
            outRight = juce::jmax (outRight, box.getCentreX());
        }
    }

    g.setColour (theme::textDim);
    g.fillRect (outLeft - 6.0f, outY - 1.0f, outRight - outLeft + 12.0f, 2.0f);
    drawLabel (g, "out", { outRight + 10.0f, outY - 8.0f, 40.0f, 16.0f });

    // Operator 4's feedback: a loop from its top back into its side.
    if (s.feedbackPercent >= 0.5f)
    {
        const auto& box = boxes[3];
        juce::Path loop;
        const float x = box.getRight() - box.getWidth() * 0.2f, side = box.getRight() + juce::jmin (14.0f, box.getWidth() * 0.18f);
        loop.startNewSubPath (x, box.getY());
        loop.lineTo (x, box.getY() - 8.0f);
        loop.lineTo (side, box.getY() - 8.0f);
        loop.lineTo (side, box.getCentreY());
        loop.lineTo (box.getRight() + 3.0f, box.getCentreY());
        g.setColour (getOperatorColour (3).withAlpha (isOn (s, 3) ? 0.9f : 0.35f));
        g.strokePath (loop, juce::PathStrokeType (1.6f));

        if (detailed)
            drawLabel (g, fm::formatKnob (Knob::feedback, s), { side + 3.0f, box.getY() - 14.0f, 50.0f, 14.0f }, getOperatorColour (3));
    }

    for (int op = 0; op < numOperators; ++op)
    {
        const auto& box = boxes[(size_t) op];
        const auto c = getOperatorColour (op);
        const bool on = isOn (s, op);
        const float glow = on ? (envelopes != nullptr ? (*envelopes)[(size_t) op] : 1.0f) : 0.0f;

        g.setColour (theme::panel);
        g.fillRoundedRectangle (box, 6.0f);
        g.setColour (c.withAlpha (0.08f + 0.32f * glow));
        g.fillRoundedRectangle (box, 6.0f);
        g.setColour (op == selected ? theme::text : c.withAlpha (on ? 0.9f : 0.35f));
        g.drawRoundedRectangle (box, 6.0f, op == selected ? 2.2f : 1.2f);

        auto inside = box.reduced (6.0f, 3.0f);
        const float numberSize = juce::jlimit (11.0f, 22.0f, box.getHeight() * 0.42f);
        g.setColour (c.withAlpha (on ? 1.0f : 0.5f));
        g.setFont (juce::FontOptions (numberSize, juce::Font::bold));
        g.drawText (juce::String (op + 1), inside.withWidth (numberSize * 0.9f).withHeight (numberSize + 2.0f), juce::Justification::centredLeft);

        auto line = inside.withTrimmedLeft (numberSize * 0.9f).withHeight (numberSize + 2.0f);
        drawLabel (g, times() + fm::formatRatio (fm::getRatio (s.ops[(size_t) op])), line, on ? theme::text : theme::textDim,
                   juce::Justification::centredRight, juce::jlimit (9.0f, 13.0f, numberSize * 0.7f));

        if (! detailed)
            continue;

        inside.removeFromTop (numberSize + 2.0f);
        const bool carrier = fm::isCarrier (s, op);
        drawLabel (g, ! on ? juce::String ("off") : carrier ? "level " + fm::formatKnob (Knob::level, s, op) : "index " + juce::String (fm::getIndex (s, op), 1),
                   inside.removeFromTop (14.0f), on ? theme::text : theme::textDim, juce::Justification::centredLeft);

        if (inside.getHeight() > 6.0f && on)
        {
            const auto& e = s.ops[(size_t) op].env;
            const double hold = juce::jlimit (0.3, 3.0, (double) (e.attack + e.decay) * 1.2);
            g.setColour (c.withAlpha (0.8f));
            g.strokePath (getEnvelopePath (e, inside.reduced (0.0f, 1.0f), hold, hold + juce::jlimit (0.1, 3.0, (double) e.release)), juce::PathStrokeType (1.2f));
        }
    }
}

void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<fm::Peak>& peaks, double noteHz, double maxHarmonic,
                   const fm::Settings* operatorsOf)
{
    auto plot = r.withTrimmedLeft (30.0f).withTrimmedBottom (operatorsOf != nullptr ? 30.0f : 16.0f);
    const double topHz = maxHarmonic * noteHz;
    auto toX = [&plot, topHz] (double hz) { return plot.getX() + plot.getWidth() * (float) (hz / topHz); };
    auto toY = [&plot] (double db) { return juce::jmap ((float) juce::jlimit ((double) floorDb, 0.0, db), floorDb, 0.0f, plot.getBottom(), plot.getY()); };

    for (const double db : { 0.0, -20.0, -40.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
        drawLabel (g, juce::String ((int) db), { r.getX(), toY (db) - 7.0f, 26.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
    }

    g.setColour (theme::grid);
    g.fillRect (plot.getX(), plot.getBottom(), plot.getWidth(), 1.0f);

    const int labelStep = maxHarmonic > 24.0 ? 4 : maxHarmonic > 12.0 ? 2 : 1;

    for (int n = 1; n <= (int) maxHarmonic; ++n)
    {
        g.setColour (theme::grid.withAlpha (0.6f));
        g.fillRect (toX (n * noteHz), plot.getBottom(), 1.0f, 4.0f);

        if (n == 1 || n % labelStep == 0)
            drawLabel (g, juce::String (n), juce::Rectangle<float> (24.0f, 12.0f).withCentre ({ toX (n * noteHz), plot.getBottom() + 10.0f }),
                       theme::textDim, juce::Justification::centred);
    }

    const float barW = juce::jlimit (2.0f, 6.0f, plot.getWidth() / (float) maxHarmonic * 0.35f);

    for (auto& p : peaks)
    {
        if (p.hz > topHz)
            continue;

        const double db = juce::Decibels::gainToDecibels (p.amplitude, -200.0);

        if (db <= floorDb)
            continue;

        const double n = p.hz / noteHz;
        const bool harmonic = std::abs (n - std::round (n)) < 0.02 && std::round (n) >= 1.0;
        const float x = toX (p.hz);
        g.setColour (harmonic ? colour : inharmonicColour);
        g.fillRect (juce::Rectangle<float> (x - barW * 0.5f, toY (db), barW, plot.getBottom() - toY (db)));
    }

    if (operatorsOf == nullptr)
        return;

    // A tick under each operator's frequency; operators at the same one share it, their numbers side by side.
    std::array<bool, numOperators> drawn {};

    for (int op = 0; op < numOperators; ++op)
    {
        const double hz = noteHz * fm::getRatio (operatorsOf->ops[(size_t) op]);

        if (drawn[(size_t) op] || ! isOn (*operatorsOf, op) || hz > topHz)
            continue;

        const float x = toX (hz), y = plot.getBottom() + 18.0f;
        juce::Path tick;
        tick.addTriangle (x - 4.0f, y + 6.0f, x + 4.0f, y + 6.0f, x, y);
        g.setColour (getOperatorColour (op));
        g.fillPath (tick);
        float labelX = x + 5.0f;

        for (int other = op; other < numOperators; ++other)
        {
            if (isOn (*operatorsOf, other) && std::abs (noteHz * fm::getRatio (operatorsOf->ops[(size_t) other]) - hz) < hz * 0.005)
            {
                drawn[(size_t) other] = true;
                drawLabel (g, juce::String (other + 1), { labelX, y - 1.0f, 9.0f, 12.0f }, getOperatorColour (other));
                labelX += 9.0f;
            }
        }
    }
}

void drawWave (juce::Graphics& g, juce::Rectangle<float> r, const fm::Settings& s, double noteHz, const std::array<float, numOperators>& envelopes,
               double cycles)
{
    const int points = juce::jmax (8, (int) r.getWidth() * 3);
    const double sampleRate = points / (cycles / noteHz);
    const auto sound = fm::render (s, noteHz, envelopes, sampleRate, points);
    const auto plain = fm::render (s, noteHz, envelopes, sampleRate, points, 1.0f, false);
    const float peak = juce::jmax (0.2f, getPeak (sound), getPeak (plain));

    g.setColour (theme::text.withAlpha (0.15f));
    g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.0f);

    const float scale = r.getHeight() * 0.45f / peak;
    drawLine (g, r, plain, scale, theme::textDim.withAlpha (0.6f), 1.2f);
    drawLine (g, r, sound, scale, colour, 1.8f);
}

void drawBessel (juce::Graphics& g, juce::Rectangle<float> r, double index)
{
    auto plot = r.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f).withTrimmedRight (70.0f);
    constexpr double topIndex = 10.0;
    auto toX = [&plot] (double x) { return plot.getX() + plot.getWidth() * (float) (x / topIndex); };
    auto toY = [&plot] (double y) { return juce::jmap ((float) y, -0.5f, 1.0f, plot.getBottom(), plot.getY()); };

    g.setColour (theme::grid);
    g.fillRect (plot.getX(), toY (0.0), plot.getWidth(), 1.0f);

    for (int i = 0; i <= 10; i += 2)
        drawLabel (g, juce::String (i), juce::Rectangle<float> (20.0f, 12.0f).withCentre ({ toX (i), plot.getBottom() + 9.0f }), theme::textDim,
                   juce::Justification::centred);

    drawLabel (g, "1", { r.getX(), toY (1.0) - 7.0f, 26.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
    drawLabel (g, "0", { r.getX(), toY (0.0) - 7.0f, 26.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);

    const juce::Colour colours[] = { colour, getOperatorColour (0), getOperatorColour (1), getOperatorColour (2) };
    const char* names[] = { "carrier", "1st sidebands", "2nd", "3rd" };

    for (int n = 0; n < 4; ++n)
    {
        juce::Path curve;

        for (float x = 0.0f; x <= plot.getWidth(); x += 2.0f)
        {
            const double at = topIndex * x / plot.getWidth();
            const float y = toY (fm::getBessel (n, at));
            x <= 0.0f ? curve.startNewSubPath (plot.getX() + x, y) : curve.lineTo (plot.getX() + x, y);
        }

        g.setColour (colours[n]);
        g.strokePath (curve, juce::PathStrokeType (n == 0 ? 2.2f : 1.5f));
        drawLabel (g, juce::String ("J") + juce::String (n) + ": " + names[n], { plot.getRight() + 4.0f, plot.getY() + 16.0f * (float) n, 70.0f, 14.0f }, colours[n]);

        const float y = toY (fm::getBessel (n, index));
        g.fillEllipse (toX (index) - 3.5f, y - 3.5f, 7.0f, 7.0f);
    }

    g.setColour (theme::text.withAlpha (0.7f));
    g.fillRect (toX (index), plot.getY(), 1.2f, plot.getHeight());
    drawLabel (g, "index " + juce::String (index, 1), { toX (index) + 4.0f, plot.getY(), 80.0f, 14.0f }, theme::text);
}

void drawEnvelopes (juce::Graphics& g, juce::Rectangle<float> r, const fm::Settings& s, double age, double hold)
{
    double release = 0.2;

    for (int op = 0; op < numOperators; ++op)
        if (isOn (s, op))
            release = juce::jmax (release, (double) s.ops[(size_t) op].env.release);

    const double total = hold + juce::jmin (3.0, release);
    auto plot = r.withTrimmedBottom (14.0f).withTrimmedLeft (4.0f);
    auto toX = [&plot, total] (double t) { return plot.getX() + plot.getWidth() * (float) (t / total); };

    for (int op = numOperators - 1; op >= 0; --op)
    {
        if (! isOn (s, op))
            continue;

        const auto c = getOperatorColour (op);
        g.setColour (c.withAlpha (fm::isCarrier (s, op) ? 1.0f : 0.75f));
        g.strokePath (getEnvelopePath (s.ops[(size_t) op].env, plot, hold, total),
                      juce::PathStrokeType (fm::isCarrier (s, op) ? 2.2f : 1.4f));
        drawLabel (g, juce::String (op + 1) + (fm::isCarrier (s, op) ? " (carrier)" : " (modulator)"),
                   { plot.getRight() - 110.0f, plot.getY() + 14.0f * (float) op, 110.0f, 14.0f }, c, juce::Justification::centredRight);
    }

    const float upX = toX (hold);
    const float dashes[] = { 4.0f, 3.0f };
    g.setColour (theme::textDim);
    g.drawDashedLine ({ upX, plot.getY(), upX, plot.getBottom() }, dashes, 2, 1.0f);
    drawLabel (g, "key up", { upX + 4.0f, plot.getBottom() - 14.0f, 60.0f, 13.0f });
    drawLabel (g, "0 s", { plot.getX(), plot.getBottom() + 1.0f, 40.0f, 13.0f });
    drawLabel (g, fxui::formatSeconds (total), { plot.getRight() - 60.0f, plot.getBottom() + 1.0f, 60.0f, 13.0f }, theme::textDim,
               juce::Justification::centredRight);

    if (age >= 0.0 && age <= total)
    {
        g.setColour (juce::Colours::white);
        g.fillRect (toX (age), plot.getY(), 1.5f, plot.getHeight());
    }
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInFm& synth)
{
    BuiltInFm* processor = &synth;
    std::vector<fxui::Guide::Page> pages;
    const double note = sub::testNoteHz;

    pages.push_back ({ "A sine that wobbles another", "What FM synthesis is",
        "Take a sine and move its pitch up and down a few times a second: that's vibrato, and you hear the pitch wobble. Now move it "
        "faster and faster. Past about 20 times a second you stop hearing a wobble and start hearing a new sound: the movement has "
        "turned into new partials, sidebands, on both sides of the sine. That's frequency modulation, FM: one sine, the modulator, "
        "moving another, the carrier, at audio speed.\n\n"
        "John Chowning found this by accident at Stanford in 1967, while trying out very fast vibrato on a computer. Two sines "
        "could make sounds as rich as a whole bank of oscillators, and changing one number, the modulator's level, made them "
        "brighter or darker. He published it in 1973, Stanford licensed it to Yamaha, and in 1983 the Yamaha DX7 brought it to "
        "everyone: one of the best-selling synthesisers of its time, the sound of the 1980s' electric pianos, basses and bells. "
        "Yamaha's FM chips then went into computer sound cards (the AdLib and Sound Blaster) and game consoles such as the Sega "
        "Mega Drive.\n\n"
        "Where a subtractive synth starts rich and takes away, and an additive one adds sines one by one, FM grows a whole spectrum "
        "from a few sines bending each other.\n\n"
        "The picture speeds a modulator up from a slow wobble to the carrier's own frequency: above, the wave; below, the partials.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            // A carrier with 24 cycles across the picture, modulated ever faster.
            const double t = std::fmod (now() * 0.12, 1.0);
            const double cycles = 24.0, modCycles = 0.5 + 23.5 * t * t, index = 2.0;
            drawCaption (g, r, "The modulator at " + juce::String (modCycles / cycles, 2) + " times the carrier",
                         modCycles < 3.0 ? "slow: a wobble in pitch" : "fast: new partials");
            auto top = r.removeFromTop (r.getHeight() * 0.42f);
            std::vector<float> values ((size_t) juce::jmax (2, (int) top.getWidth() * 2));

            for (size_t i = 0; i < values.size(); ++i)
            {
                const double x = (double) i / (double) (values.size() - 1);
                values[i] = (float) std::sin (juce::MathConstants<double>::twoPi * cycles * x + index * std::sin (juce::MathConstants<double>::twoPi * modCycles * x));
            }

            g.setColour (theme::text.withAlpha (0.15f));
            g.fillRect (top.getX(), top.getCentreY(), top.getWidth(), 1.0f);
            drawLine (g, top.reduced (0.0f, 4.0f), values, top.getHeight() * 0.42f, colour, 1.4f);

            auto peaks = fm::getSidebands (4.0, 4.0 * modCycles / cycles, index);
            r.removeFromTop (10.0f);
            drawSpectrum (g, r, peaks, 1.0, 9.0);
        } });

    pages.push_back ({ "Carrier and modulator", "The two sines of FM, and the index",
        "In FM each sine is called an operator. The one you hear is the carrier. The one that bends it is the modulator: you don't "
        "hear it on its own, you hear what it does to the carrier.\n\n"
        "Two numbers decide the sound. The ratio is how fast the modulator runs against the carrier: it decides where the new "
        "partials go. The modulation index is how hard it swings the carrier: it decides how many partials there are and how loud. "
        "In DawIt FM a modulator's level is its index: 10% is an index of 1, 100% an index of 10.\n\n"
        "Strictly, the DX7 and DawIt FM bend the carrier's phase (where it is in its cycle) rather than its frequency: phase "
        "modulation. The partials are the same, and it keeps the pitch steady however the modulator moves, so everyone calls it "
        "FM anyway.\n\n"
        "The picture shows a modulator, the carrier on its own, and the carrier bent by the modulator, using the window's first "
        "pair of operators: the carrier's cycles squeeze where the modulator is rising and stretch where it's falling.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            bool replaced = false;
            const auto s = withSomethingToShow (processor->getSettings(), replaced);
            int modulator = 1;

            for (int op = 1; op < numOperators; ++op)
            {
                if (! fm::isCarrier (s, op) && isOn (s, op))
                {
                    modulator = op;
                    break;
                }
            }

            int carrier = 0;

            while (carrier < numOperators - 1 && (fm::getTargets (s, modulator) & (1 << carrier)) == 0)
                ++carrier;

            const double carrierRatio = fm::getRatio (s.ops[(size_t) carrier]), modulatorRatio = fm::getRatio (s.ops[(size_t) modulator]);
            const double index = juce::jmax (0.5, fm::getIndex (s, modulator));
            drawCaption (g, r, "Operator " + juce::String (modulator + 1) + " modulating operator " + juce::String (carrier + 1) + ", index " + juce::String (index, 1),
                         replaced ? "the 1 : 1 starting point, as nothing in the window modulates" : "from the window's sound");
            const float h = (r.getHeight() - 30.0f) / 3.0f;
            const double cycles = 3.0;
            const int points = juce::jmax (2, (int) r.getWidth() * 2);
            std::array<std::vector<float>, 3> waves;

            for (auto& w : waves)
                w.resize ((size_t) points);

            for (int i = 0; i < points; ++i)
            {
                const double x = juce::MathConstants<double>::twoPi * cycles * i / (points - 1);
                waves[0][(size_t) i] = (float) std::sin (modulatorRatio * x);
                waves[1][(size_t) i] = (float) std::sin (carrierRatio * x);
                waves[2][(size_t) i] = (float) std::sin (carrierRatio * x + index * std::sin (modulatorRatio * x));
            }

            const juce::String names[] = { "the modulator, " + times() + fm::formatRatio (modulatorRatio) + ": not heard itself",
                                           "the carrier on its own, " + times() + fm::formatRatio (carrierRatio),
                                           "the carrier, modulated: what you hear" };
            const juce::Colour colours[] = { getOperatorColour (modulator), getOperatorColour (carrier).withAlpha (0.6f), colour };

            for (int i = 0; i < 3; ++i)
            {
                auto box = r.removeFromTop (h);
                r.removeFromTop (10.0f);
                drawLabel (g, names[i], box.removeFromTop (14.0f), colours[i]);
                g.setColour (theme::text.withAlpha (0.12f));
                g.fillRect (box.getX(), box.getCentreY(), box.getWidth(), 1.0f);
                drawLine (g, box, waves[(size_t) i], box.getHeight() * 0.42f, colours[i], 1.6f);
            }
        } });

    pages.push_back ({ "Sidebands and the index", "Where the new partials go, and how loud",
        "When one sine modulates another, the new partials (sidebands) sit at the carrier's frequency plus and minus whole multiples "
        "of the modulator's: with a carrier at 1,000 Hz and a modulator at 100 Hz, at 1,100 and 900, 1,200 and 800, and so on, "
        "evenly spaced.\n\n"
        "How loud each one is follows a family of curves called Bessel functions, one per sideband: J0 for the carrier, J1 for the "
        "first pair, J2 for the second... At an index of 0 there's only the carrier. As the index grows, the first sidebands rise, "
        "then the second, then the third, and the sound gets brighter. The curves swing up and down, so the partials don't just grow: "
        "at an index of 2.4 the carrier's own frequency disappears completely, and the sound is all sidebands.\n\n"
        "Two rules of thumb. The sound spreads over about 2 (index + 1) times the modulator's frequency (Carson's rule): an index of 2 "
        "and a modulator at 220 Hz cover about 1.3 kHz. And modulation doesn't add power, it moves it: the carrier gets quieter as the "
        "sidebands get louder, so the sound gets brighter, not louder.\n\n"
        "The picture sweeps the index from 0 to 10: the Bessel curves, and the sidebands of a carrier at 8 times a modulator.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const double index = 5.0 - 5.0 * std::cos (now() * 0.5);
            drawCaption (g, r, "The index at " + juce::String (index, 1), "carrier at 8, modulator at 1");
            auto top = r.removeFromTop (r.getHeight() * 0.5f);
            drawBessel (g, top, index);
            r.removeFromTop (10.0f);
            drawSpectrum (g, r, fm::getSidebands (8.0, 1.0, index), 1.0, 17.0);
        } });

    pages.push_back ({ "Ratios: in tune or clangorous", "Which sidebands land on harmonics",
        "The ratio between carrier and modulator decides whether the sidebands land on the note's harmonics.\n\n"
        "1 : 1: sidebands at 1 + 1, 1 + 2... and 1 - 1, 1 - 2... Sidebands below 0 Hz fold back up as if mirrored, so every harmonic "
        "is there: a bright, saw-like sound.\n"
        "1 : 2: sidebands at 1, 3, 5, 7... only the odd harmonics, the hollow sound of a square wave or a clarinet.\n"
        "1 : 3: at 1, 4, 7... and folded back 2, 5, 8...: every harmonic except each third one.\n"
        "1 : 1.4, or any ratio that isn't a simple fraction: the sidebands fall between the harmonics, and the sound has no clear "
        "pitch any more, like a bell, a gong or struck metal. Chowning's first bells used just this ratio.\n\n"
        "A higher modulator ratio spaces the sidebands further apart: a modulator at 14 times the note puts its partials far up, the "
        "clang of an electric piano's tine.\n\n"
        "The picture shows four ratios at the same index.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const double index = 2.0 + std::sin (now() * 0.7);
            drawCaption (g, r, "Four ratios at an index of " + juce::String (index, 1), "purple: harmonics; amber: between them");
            const std::pair<double, const char*> ratios[] = { { 1.0, "1 : 1, every harmonic" }, { 2.0, "1 : 2, only the odd ones" },
                                                              { 3.0, "1 : 3, no 3rd, 6th, 9th" }, { 1.4, "1 : 1.4, a bell" } };
            const float w = r.getWidth() / 2.0f, h = r.getHeight() / 2.0f;

            for (int i = 0; i < 4; ++i)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 2), r.getY() + h * (float) (i / 2), w, h).reduced (8.0f, 4.0f);
                drawLabel (g, ratios[i].second, cell.removeFromTop (16.0f), theme::text, juce::Justification::centredLeft, 12.0f);
                cell.removeFromTop (10.0f);
                drawSpectrum (g, cell, fm::getSidebands (1.0, ratios[i].first, index), 1.0, 12.0);
            }
        } });

    pages.push_back ({ "Brightness that moves", "Envelopes on the modulators",
        "A modulator's level is the sound's brightness, so an envelope on a modulator moves the brightness through each note, the way "
        "a filter envelope does on a subtractive synth. It's where FM's sounds come alive.\n\n"
        "Struck and plucked sounds: the modulator decays quickly, so the note is bright for a moment and then mellows, while the "
        "carrier rings on. Every electric piano, bass and mallet patch on a DX7 works like this.\n\n"
        "Brass: Jean-Claude Risset found in the 1960s, measuring trumpet tones at Bell Labs, that a trumpet gets brighter as it "
        "gets louder. Chowning gave the modulator the same envelope as the carrier, and FM brass was born: each note blooms from "
        "dark to bright.\n\n"
        "Bells: the modulator dies away with the note, so the clang of the strike fades into a purer hum.\n\n"
        "Velocity works the same way: a soft note has a smaller index, so it's darker as well as quieter.\n\n"
        "The picture plays a note of the window's sound over and over: the partials as they change, and the operators' envelopes.",
        [processor, note] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            bool replaced = false;
            auto s = withSomethingToShow (processor->getSettings(), replaced);

            if (replaced)
                s = fm::getPresets()[2].settings;

            const double hold = getHoldSeconds (s);
            double release = 0.2;

            for (auto& o : s.ops)
                release = juce::jmax (release, (double) o.env.release);

            const double length = hold + juce::jmin (3.0, release);
            const double t = std::fmod (now(), length + 0.4);
            drawCaption (g, r, "A note over time, " + juce::String (t, 1) + " s in",
                         replaced ? "the Electric piano, as nothing in the window modulates" : "the window's sound");
            auto top = r.removeFromTop (r.getHeight() * 0.55f);
            drawSpectrum (g, top, fm::getSpectrum (s, note, fm::getEnvelopes (s, t, hold), 1.0f, note * 24.0), note, 24.0, &s);
            r.removeFromTop (10.0f);
            drawEnvelopes (g, r, s, t, hold);
        } });

    pages.push_back ({ "Algorithms: wiring four operators", "Stacks, pairs and sines side by side",
        "With more than two operators there are many ways to wire them, and each wiring is called an algorithm. DawIt FM has four "
        "operators and eight algorithms, as Yamaha's four-operator synths had; the six-operator DX7 had 32.\n\n"
        "In a stack (4 modulates 3, 3 modulates 2, 2 modulates 1), the sidebands get sidebands of their own: very rich, from warm to "
        "harsh, good for brass, leads and basses.\n"
        "Several modulators into one carrier add their sidebands together: a fuller sound from one note.\n"
        "Two pairs are two FM sounds layered, each with its own envelopes: the round body and the tine of an electric piano, or a "
        "bell over a soft pad.\n"
        "Four sines side by side modulate nothing: additive synthesis, an organ's drawbars.\n\n"
        "The carriers are the operators you hear; their envelopes are the note's loudness. The modulators' envelopes are its "
        "brightness.\n\n"
        "The picture shows the eight, with the window's lit. Right-click the operators in the window to change it.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto current = processor->getSettings();
            drawCaption (g, r, "The eight algorithms", "the carriers go to the line at the bottom");
            const float w = r.getWidth() / 4.0f, h = r.getHeight() / 2.0f;

            for (int a = 0; a < fm::numAlgorithms; ++a)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (a % 4), r.getY() + h * (float) (a / 4), w, h).reduced (5.0f, 4.0f);
                fm::Settings s;
                s.algorithm = a;

                for (auto& o : s.ops)
                    o.levelPercent = 50.0f;

                if (a == current.algorithm)
                {
                    g.setColour (colour.withAlpha (0.12f));
                    g.fillRoundedRectangle (cell, 6.0f);
                }

                drawLabel (g, juce::String (a + 1) + ". " + fm::getAlgorithm (a).name, cell.removeFromTop (15.0f).reduced (4.0f, 0.0f),
                           a == current.algorithm ? colour : theme::text, juce::Justification::centredLeft, 11.5f);
                drawAlgorithm (g, cell.reduced (4.0f, 2.0f), s, -1, nullptr, false);
            }
        } });

    pages.push_back ({ "Feedback", "An operator that modulates itself",
        "Feedback sends an operator's own output back into its phase, so it modulates itself. In DawIt FM it's operator 4, as on "
        "Yamaha's four-operator synths.\n\n"
        "A little feedback adds a few soft harmonics above the sine. More, and the sine leans over into something very like a saw "
        "wave, with every harmonic, each a little quieter than the one below. Turn it all the way and it begins to break up into "
        "noise. One operator can make a saw-like wave this way, which a subtractive synth would start from, so feedback is how FM "
        "gets buzzy leads, basses and brass.\n\n"
        "An operator with feedback can be heard itself (in Four sines) or modulate others, passing its richness down the stack.\n\n"
        "The picture turns the feedback of a single operator slowly up and down: its wave above, its partials below.",
        [note] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            fm::Settings s;
            s.algorithm = 7;
            s.ops[0].levelPercent = 0.0f;
            s.ops[3].levelPercent = 100.0f;
            s.feedbackPercent = (float) (50.0 - 50.0 * std::cos (now() * 0.6));
            drawCaption (g, r, "Operator 4 alone, feedback " + fm::formatKnob (Knob::feedback, s), "the wave, and its partials");
            auto top = r.removeFromTop (r.getHeight() * 0.4f);
            drawWave (g, top, s, note, getFull(), 3.0);
            r.removeFromTop (10.0f);
            drawSpectrum (g, r, fm::getSpectrum (s, note, getFull(), 1.0f, note * 16.0), note, 16.0);
        } });

    pages.push_back ({ "Shaping the note", "Envelopes, vibrato and voices",
        "Each operator has its own envelope: attack (how long it takes to rise), decay (how long to fall to the sustain level), "
        "sustain (the level held while the key is down) and release (how long it fades after the key comes up).\n\n"
        "On the carriers they're the note's loudness: a few milliseconds of attack and sustain at 100% for an organ; attack near 0, a "
        "long decay and sustain at 0 for a struck note; slow attack and release for a pad. The note ends when every carrier has "
        "finished its release.\n\n"
        "On the modulators they're the brightness, as the last pages showed. A modulator that decays faster than its carrier is the "
        "most useful trick in FM.\n\n"
        "The LFO adds movement: on the pitch it's vibrato (5-7 Hz, small), on the brightness a slow breathing, on the volume a "
        "tremolo. Poly plays chords; mono plays one note at a time, with glide between legato notes. Velocity makes soft notes "
        "quieter and darker.\n\n"
        "The picture shows the window's envelopes over a note held and let go.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double hold = getHoldSeconds (s);
            const double age = processor->noteAge.load();
            drawCaption (g, r, "The window's envelopes", "carriers thick, modulators thin; play a note to see it run");
            drawEnvelopes (g, r, s, age, age >= 0.0 && processor->releaseAge.load() >= 0.0f ? (double) processor->releaseAge.load() : hold);
        } });

    pages.push_back ({ "Building a sound, step by step", "From one sine to your own sound",
        "Start from Init (one carrier, its modulator at 0) and change one thing at a time, playing a note or a phrase over and over.\n\n"
        "1. The colour: pick the modulator's ratio. 1 for warm and full, 2 for hollow, 3 or more for nasal and bright, a ratio off a "
        "whole number for bells.\n"
        "2. The brightness: raise the modulator's level until the sound is as bright as you want. An index of 1 to 3 covers most "
        "sounds.\n"
        "3. The movement: give the modulator its own envelope. A fast decay for plucks and keys, a slow attack for brass and pads.\n"
        "4. The loudness: the carrier's envelope, how the note starts and ends.\n"
        "5. More: a second pair for a second layer (Two pairs), or more modulators for a richer sound (a stack). Feedback for buzz.\n"
        "6. Vibrato, voices, velocity and the level.\n\n"
        "When it isn't right: too dull? More index, or a higher modulator ratio. Too harsh or buzzy? Less index or feedback. Too "
        "static? An envelope on the modulator, or the LFO on the brightness. Out of tune or clangy? Check that the ratios are whole "
        "numbers.\n\n"
        "The picture lights the operators each step is about, on the window's sound.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const int step = (int) std::fmod (now() / 2.5, 3.0);
            const juce::StringArray names { "1-3. the modulators: ratio, level and envelope", "4. the carriers: the loudness", "5. operator 4: feedback" };
            drawCaption (g, r, "Building up the window's sound", names[step]);

            int lit = -1;

            for (int op = 0; op < numOperators && lit < 0; ++op)
                if ((step == 0 && ! fm::isCarrier (s, op)) || (step == 1 && fm::isCarrier (s, op)) || (step == 2 && op == 3))
                    lit = op;

            drawAlgorithm (g, r.reduced (r.getWidth() * 0.15f, 0.0f), s, lit, nullptr, true);
        } });

    pages.push_back ({ "Recipes for common sounds", "Keys, bass, bells, brass, reed, organ",
        "Every one of these is in the starting points, with notes on what each operator does. Load one, play it, and change one "
        "thing at a time.\n\n"
        "Electric piano: two pairs. A 1 : 1 pair with a slowly decaying modulator for the body; a pair with its modulator at 14 "
        "times the note, gone in a quarter of a second, for the tine. Velocity on, so soft notes are mellow.\n\n"
        "Bass: a stack, the modulator at 1 : 1 with a high index that falls fast after the attack, a short burst at 3 times for "
        "the click; mono with a little glide.\n\n"
        "Bell: a modulator at 1.4 times the carrier, an index near 10, both dying over many seconds.\n\n"
        "Brass: 1 : 1, the modulator's envelope following the carrier's a little slower, an index of 2 to 4, a little vibrato.\n\n"
        "Reed or clarinet: 1 : 2 for the odd harmonics, a low index, mono.\n\n"
        "Organ: four sines at 1, 2, 3 and 4 times the note, no modulation.\n\n"
        "Marimba: a pure note dying in a second, and a sine at 4 times dying four times as fast.\n\n"
        "The picture shows the window's sound: its operators and its partials as they are now.",
        [processor, note] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            std::array<float, numOperators> envelopes;

            for (size_t i = 0; i < envelopes.size(); ++i)
                envelopes[i] = processor->noteAge.load() >= 0.0f ? processor->shownEnvelopes[i].load() : 1.0f;

            drawCaption (g, r, "The window's sound", "play a note to see it move");
            auto left = r.removeFromLeft (r.getWidth() * 0.4f);
            drawAlgorithm (g, left, s, -1, &envelopes, false);
            r.removeFromLeft (16.0f);
            drawSpectrum (g, r, fm::getSpectrum (s, note, envelopes, 1.0f, note * 24.0), note, 24.0, &s);
        } });

    return pages;
}
} // namespace daw::fmtext
