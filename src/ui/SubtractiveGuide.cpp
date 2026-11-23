#include "SubtractiveGuide.h"

namespace daw::subtext
{
using sub::Knob;
using sub::Wave;

namespace
{
    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    juce::String formatDb (double db)
    {
        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    /** "a fifth up", "an octave down", or the semitones. */
    juce::String describeInterval (int semitones)
    {
        const int size = std::abs (semitones);
        const juce::String direction = semitones > 0 ? " up" : " down";
        static const std::map<int, juce::String> names { { 3, "a minor third" }, { 4, "a major third" }, { 5, "a fourth" }, { 7, "a fifth" },
                                                         { 12, "an octave" }, { 19, "an octave and a fifth" }, { 24, "two octaves" } };

        if (size == 0)
            return "in unison with oscillator 1";

        if (auto it = names.find (size); it != names.end())
            return it->second + direction;

        return juce::String (size) + " semitones" + direction;
    }

    float getX (juce::Rectangle<float> r, double hz)
    {
        return r.getX() + r.getWidth() * (float) (std::log (hz / 20.0) / std::log (1000.0));
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

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Colour labelColour = theme::textDim,
                    juce::Justification justification = juce::Justification::centredLeft)
    {
        g.setColour (labelColour);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (text, r, justification, true);
    }

    /** Samples across an area, with 1 at `scale` pixels from the middle. */
    void drawLine (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<float>& values, float scale, juce::Colour lineColour, float thickness)
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

        g.setColour (lineColour);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    /** The part of the path of the sound (drawFlow's numbering) a knob belongs to. */
    int getFlowPart (Knob k)
    {
        switch (k)
        {
            case Knob::wave1: case Knob::width: case Knob::wave2: case Knob::pitch2: case Knob::detune:
            case Knob::glide: case Knob::mode:
                return 0;
            case Knob::mix: case Knob::noise:
                return 1;
            case Knob::filterType: case Knob::slope: case Knob::cutoff: case Knob::resonance: case Knob::keyTrack:
                return 2;
            case Knob::envAmount: case Knob::filterAttack: case Knob::filterDecay: case Knob::filterSustain: case Knob::filterRelease:
            case Knob::ampAttack: case Knob::ampDecay: case Knob::ampSustain: case Knob::ampRelease:
                return 4;
            case Knob::lfoRate: case Knob::lfoDepth: case Knob::lfoTarget:
                return 5;
            case Knob::velocity: case Knob::output:
                return 3;
        }

        return -1;
    }

    double now()
    {
        return juce::Time::getMillisecondCounterHiRes() / 1000.0;
    }
}

//==============================================================================
juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::wave1:         return "Oscillator 1: wave";
        case (int) Knob::width:         return "Pulse width";
        case (int) Knob::wave2:         return "Oscillator 2: wave";
        case (int) Knob::pitch2:        return "Oscillator 2: pitch";
        case (int) Knob::detune:        return "Oscillator 2: detune";
        case (int) Knob::mix:           return "Mix";
        case (int) Knob::noise:         return "Noise";
        case (int) Knob::filterType:    return "Filter type";
        case (int) Knob::slope:         return "Filter slope";
        case (int) Knob::cutoff:        return "Cutoff";
        case (int) Knob::resonance:     return "Resonance";
        case (int) Knob::keyTrack:      return "Key tracking";
        case (int) Knob::envAmount:     return "Filter envelope: amount";
        case (int) Knob::filterAttack:  return "Filter envelope: attack";
        case (int) Knob::filterDecay:   return "Filter envelope: decay";
        case (int) Knob::filterSustain: return "Filter envelope: sustain";
        case (int) Knob::filterRelease: return "Filter envelope: release";
        case (int) Knob::ampAttack:     return "Amp envelope: attack";
        case (int) Knob::ampDecay:      return "Amp envelope: decay";
        case (int) Knob::ampSustain:    return "Amp envelope: sustain";
        case (int) Knob::ampRelease:    return "Amp envelope: release";
        case (int) Knob::lfoRate:       return "LFO rate";
        case (int) Knob::lfoDepth:      return "LFO depth";
        case (int) Knob::lfoTarget:     return "LFO to";
        case (int) Knob::glide:         return "Glide";
        case (int) Knob::mode:          return "Voices: poly or mono";
        case (int) Knob::velocity:      return "Velocity";
        case (int) Knob::output:        return "Output";
        case spectrum:                  return "The harmonics and the filter";
        case wave:                      return "The wave, before and after the filter";
        case envelopes:                 return "The envelopes";
        case fxui::overview:            return "DawIt Subtractive";
        case fxui::presets:             return "Starting points";
        case fxui::clap:                return "Play a note";
        case fxui::listen:              return "Hear it without the filter";
        case fxui::guide:               return "What is subtractive synthesis?";
        case fxui::history:             return "The last 4 seconds";
        default:                        return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::wave1:
        case (int) Knob::wave2:
            return "The oscillator makes the raw sound: a wave repeating at the note's pitch. Its shape decides which harmonics "
                   "it has, and that is what the filter will work on.\n\n"
                   "Saw: every harmonic, each softer than the one before (the 2nd at half the level, the 3rd at a third...). "
                   "Bright and buzzy, the most used, because it has the most to take away: strings, brass, basses, leads.\n"
                   "Square: only the odd harmonics (1st, 3rd, 5th...). Hollow and woody, like a clarinet's low notes. Narrow it with Width "
                   "for a pulse.\n"
                   "Triangle: odd harmonics too, but falling away fast. Soft and round, like a flute or an ocarina.\n"
                   "Sine: the note alone, no harmonics at all. Pure; the filter can change its level but not its colour. For sub basses and whistles.";

        case (int) Knob::width:
            return "How much of each cycle a square spends up, against down. At 50% it's a square: up and down for as long, only "
                   "odd harmonics. Narrower, it becomes a pulse: the even harmonics come in and the sound gets thinner and more "
                   "nasal, like an oboe or a reed. At a width of 1 in n, every nth harmonic is missing: at 25%, the 4th, 8th, "
                   "12th...\n\n"
                   "Moving the width slowly with the LFO (pulse-width modulation, PWM) makes one oscillator sound like a "
                   "chorus of several. Only the square uses it.";

        case (int) Knob::pitch2:
            return "How far oscillator 2 is from oscillator 1, in semitones. An octave down (-12) adds weight under a bass; an "
                   "octave up adds brightness; a fifth (+7) gives the thick, organ-like sound of two notes in one, used in leads "
                   "and in chords of power. Other intervals make a chord out of every key you press, as on some 1980s riffs.";

        case (int) Knob::detune:
            return "Oscillator 2 a little higher still, in cents (hundredths of a semitone). Two oscillators slightly out of tune "
                   "drift in and out of step: you hear the sound swell and sway as they do, a beating that makes it wide and "
                   "alive. The bigger the difference, the faster it beats: the beat rate is the difference between their "
                   "frequencies. It's the sound of an ensemble, where no two players are ever exactly in tune.";

        case (int) Knob::mix:
            return "How much of each oscillator you hear: at 0% only oscillator 1, at 100% only oscillator 2, at 50% both as "
                   "loud. Bring oscillator 2 in under oscillator 1 for weight or width, rather than as loud.";

        case (int) Knob::noise:
            return "Noise is every frequency at once, at random: a hiss. On its own through the filter it makes wind, surf, "
                   "breath or a snare's rattle; a little under the oscillators adds breath to a flute or air to a pad, and a "
                   "short burst at the start of a note adds a chiff or a pick.";

        case (int) Knob::filterType:
            return "The filter is what makes this subtractive: it takes harmonics away.\n\n"
                   "Low-pass lets the lows through and turns down everything above the cutoff: closed, the sound is dark and "
                   "muffled; open, bright. By far the most used, because natural sounds get darker as they fade, and so does a "
                   "low-pass closing.\n"
                   "High-pass does the opposite: it takes the lows and the body away and keeps the fizz. Thin, for hi-hats from "
                   "noise or for a part that must stay out of the bass's way.\n"
                   "Band-pass keeps a band around the cutoff and takes away both sides: nasal and vocal, like a telephone or a "
                   "talking \"wah\".";

        case (int) Knob::slope:
            return "How steeply the filter turns things down past the cutoff: 12 dB per octave (2 poles), or 24 (4 poles). "
                   "24 dB is the Moog ladder's slope: strong, dark and fat, the classic bass and lead filter. 12 dB is gentler and "
                   "more open, the sound of the Oberheim SEM: more of the harmonics are still heard, so it sounds brighter and "
                   "more natural for brass, strings and pads.";

        case (int) Knob::cutoff:
            return "Where the filter starts working. For a low-pass, the harmonics below the cutoff pass, those above it are turned "
                   "down, more and more the higher they are. Turning the cutoff down is the most important move on a subtractive "
                   "synth: the sound goes from bright and buzzy to warm, then to dark and muffled.\n\n"
                   "Drag the picture of the harmonics left and right to move it, and watch which bars stay.";

        case (int) Knob::resonance:
            return "A peak at the cutoff: the harmonics right at the cutoff are boosted while those above are cut. A little gives "
                   "the filter character; more makes it sing and whistle, and when the cutoff moves (with the envelope or the "
                   "LFO), the peak sweeps through the harmonics and you hear the famous \"wow\" and squelch. At the top it nearly "
                   "rings on its own, a pure tone at the cutoff.\n\n"
                   "Drag the picture of the harmonics up and down to change it.";

        case (int) Knob::keyTrack:
            return "How much the cutoff follows the note you play. At 0% the cutoff stays put, so high notes have fewer harmonics "
                   "under it and sound duller than low notes. At 100% it moves with the pitch: an octave up the keyboard, an "
                   "octave up the cutoff, so every note has the same brightness. Acoustic instruments are in between: higher notes "
                   "a bit brighter.";

        case (int) Knob::envAmount:
            return "How far the filter envelope moves the cutoff, in octaves, at the envelope's peak. Positive opens the filter "
                   "(brighter) as the envelope rises; negative closes it. This is what makes a synth sound move within each note: "
                   "the bright start and dark tail of a pluck, the swell of a brass note, the \"yow\" of an acid bass. Softer "
                   "notes move it less, as set by Velocity.";

        case (int) Knob::filterAttack:
        case (int) Knob::ampAttack:
            return "How long the envelope takes to rise from nothing to its peak when a key is pressed. Short (1-5 ms): the note "
                   "starts at once, a hit or a pluck. Medium (20-100 ms): a soft start, like breath or a bow. Long (0.5 s and up): "
                   "the note swells in, as for pads. On the filter envelope, the attack is how long the sound takes to get "
                   "bright.";

        case (int) Knob::filterDecay:
        case (int) Knob::ampDecay:
            return "After the attack, how long the envelope takes to fall from its peak to the sustain level, while the key is "
                   "still held. With a low sustain, the decay is how long the note (or its brightness, on the filter) lasts: "
                   "short for a pluck or a blip, longer for a piano-like fade.";

        case (int) Knob::filterSustain:
        case (int) Knob::ampSustain:
            return "The level the envelope holds while the key stays down, after the decay. It's a level, not a time. 100%: the "
                   "note stays as loud (or the filter as open) as at its peak, like an organ. 0%: it dies away even with the key "
                   "held, like a plucked string. In between: a bright start that settles.";

        case (int) Knob::filterRelease:
        case (int) Knob::ampRelease:
            return "How long the envelope takes to fall to nothing after the key is let go. Short: the note stops when you lift "
                   "your finger, tight and rhythmic. Long: it rings on and fades, like a sound in a big room, and chords blur into "
                   "each other.";

        case (int) Knob::lfoRate:
            return "The LFO (low-frequency oscillator) is a wave too slow to hear as a note, used to move something up and down "
                   "over and over. Rate is how many times a second: 5-7 Hz is the speed of a singer's vibrato; 0.1-1 Hz a slow "
                   "sweep you feel more than hear; 2-8 Hz a rhythmic wobble (at 120 BPM, 2 Hz is once a beat).";

        case (int) Knob::lfoDepth:
            return "How far the LFO moves its target: at 100%, the pitch by 2 semitones up and down, the cutoff by 2 octaves, the "
                   "volume all the way down, or the pulse width by 45 points. A little goes a long way on the pitch.";

        case (int) Knob::lfoTarget:
            return "What the LFO moves. Pitch: vibrato. Filter: a sweep or a wobble in brightness. Volume: tremolo, a pulsing "
                   "level like an old amp's. Width: pulse-width modulation, which makes a square sound like a chorus of them.";

        case (int) Knob::glide:
            return "Glide (portamento): instead of jumping to the next note, the pitch slides there, taking about this long. In "
                   "mono only overlapping (legato) notes slide, so you choose which notes glide by how you play; in poly every new "
                   "note slides from the last one played.";

        case (int) Knob::mode:
            return "Poly: each note gets its own voice (up to 16), for chords and pads. Mono: one note at a time, as on the "
                   "Minimoog or the TB-303; a note played while another is held takes over without starting the envelopes again "
                   "(legato), and glides if Glide is on. Basses and leads usually play mono: it's tighter, and the slides and "
                   "legato are part of the sound.";

        case (int) Knob::velocity:
            return "How much playing softer makes a note quieter and darker (the filter envelope moves less). At 0% every note "
                   "is the same however hard you play; at 100% a soft note is much quieter and duller than a hard one, like an "
                   "acoustic instrument.";

        case (int) Knob::output:
            return "The synth's level going into the track's effects and fader. Leave headroom: chords add up, and resonance "
                   "makes a sound louder than it looks.";

        case spectrum:
            return "The harmonics of the test note (A3, 220 Hz) as bars on a scale from 20 Hz to 20 kHz: grey what the oscillators "
                   "make, coloured what's left after the filter. The line is the filter: what it does to each frequency, at the "
                   "cutoff the newest note has right now, so you see it move with the envelope and the LFO. Oscillator 2's "
                   "harmonics are drawn in pink.\n\n"
                   "Drag left and right for the cutoff, up and down for the resonance; double-click to open it fully.";

        case wave:
            return "A few cycles of the test note: grey the wave the oscillators make, coloured what the filter leaves of it. "
                   "Taking harmonics away rounds the corners: a saw through a closed low-pass turns into something close to a "
                   "sine.";

        case envelopes:
            return "The two envelopes' shapes: how the filter (amber) and the volume (green) rise, fall, hold and fade over a "
                   "note: attack, decay, sustain while held, release after. The dot is where the newest note is now. Play a note "
                   "to see it run.";

        case fxui::clap:
            return "Plays the test note, A3, for 0.8 seconds through the synth, even while the song is stopped. Or play the track's "
                   "MIDI notes, or a MIDI keyboard on this track.";

        case fxui::listen:
            return "Turns the filter off for a moment, so you hear the oscillators as they are, raw. Switch back and forth to hear "
                   "what the filter takes away: that's subtractive synthesis.";

        case fxui::presets:
            return "Sounds to start from, each telling why it's made the way it is. Pick one and read the notes below, then change "
                   "a knob at a time and listen.";

        case fxui::guide:
            return "A guide for beginners: what subtractive synthesis is, how each part works, and how to build a sound from "
                   "scratch, with pictures that follow the knobs.";

        case fxui::history:
            return "The last 4 seconds: grey how loud the oscillators were (what you'd hear without the filter), coloured what came "
                   "out after the filter and the amp. The gap between them is what the filter took away.";

        default:
            return {};
    }
}

juce::String describeSettings (int t, const sub::Settings& s)
{
    const double note = sub::testNoteHz;

    switch (t)
    {
        case (int) Knob::wave1:
        case (int) Knob::wave2:
        {
            const auto w = t == (int) Knob::wave1 ? s.wave1 : s.wave2;
            juce::String text = sub::getWaveName (w) + ": ";

            switch (w)
            {
                case Wave::saw:      text << "every harmonic, the 2nd at -6 dB, the 4th at -12 dB, the 10th at -20 dB."; break;
                case Wave::square:   text << (s.widthPercent >= 49.5f ? "only the odd harmonics, the 3rd at -9.5 dB, the 5th at -14 dB."
                                                                       : "a pulse of " + juce::String (juce::roundToInt (s.widthPercent)) + "%, see Width."); break;
                case Wave::triangle: text << "odd harmonics falling fast: the 3rd at -19 dB, the 5th at -28 dB."; break;
                case Wave::sine:     text << "the note alone, " << formatHz (note) << " for A3, nothing above it."; break;
            }

            return text;
        }

        case (int) Knob::width:
        {
            if (s.widthPercent >= 49.5f)
                return "50%: a square, every even harmonic missing.";

            const double every = 100.0 / s.widthPercent;
            juce::String text = juce::String (juce::roundToInt (s.widthPercent)) + "%: a pulse, thinner than a square.";

            if (std::abs (every - std::round (every)) < 0.05)
                text << " Every " << juce::roundToInt (every) << "th harmonic is missing.";

            if (s.wave1 != Wave::square && s.wave2 != Wave::square)
                text << " Neither oscillator is a square, so it does nothing now.";

            return text;
        }

        case (int) Knob::pitch2:
        {
            const double hz = note * std::pow (2.0, s.pitch2 / 12.0);
            return sub::formatKnob (Knob::pitch2, s) + ": " + describeInterval (juce::roundToInt (s.pitch2)) + ". For A3, oscillator 2 plays "
                   + formatHz (hz) + (s.mixPercent < 1.0f ? ", but the mix is at 0%, so you don't hear it." : ".");
        }

        case (int) Knob::detune:
        {
            if (s.detuneCents < 0.05f)
                return "In tune: the oscillators don't beat.";

            const double beat = sub::getBeatHz (note * std::pow (2.0, s.pitch2 / 12.0), s.detuneCents);
            return juce::String (s.detuneCents, 1) + " cents: at A3 they beat " + juce::String (beat, beat < 1.0 ? 2 : 1)
                   + " times a second, faster on higher notes (twice as fast an octave up).";
        }

        case (int) Knob::mix:
            return "Oscillator 1 at " + juce::String (juce::roundToInt (100.0f - s.mixPercent)) + "%, oscillator 2 at "
                   + juce::String (juce::roundToInt (s.mixPercent)) + "%.";

        case (int) Knob::noise:
            return s.noisePercent < 0.5f ? juce::String ("Off.") : juce::String (juce::roundToInt (s.noisePercent)) + "% noise on top of the oscillators.";

        case (int) Knob::filterType:
            return sub::getFilterTypeName (s.filterType) + ", " + sub::formatKnob (Knob::slope, s) + ".";

        case (int) Knob::slope:
        {
            const double octaveUp = sub::getFilterDb (s.filterType, s.steep, 1000.0, 0.0f, s.filterType == sub::FilterType::highPass ? 500.0 : 2000.0);
            return sub::formatKnob (Knob::slope, s) + ": an octave past the cutoff, the sound is " + formatDb (octaveUp) + " (without resonance).";
        }

        case (int) Knob::cutoff:
        {
            const double cutoff = sub::getCutoffAt (s, sub::testNote, 1.0f, 0.0f, 0.0f);
            const int below = (int) std::floor (cutoff / note + 1.0e-6);
            juce::String text = sub::formatKnob (Knob::cutoff, s) + ".";

            if (s.filterType == sub::FilterType::lowPass)
                text << (cutoff >= 19000.0 ? " Wide open: every harmonic passes."
                                           : below < 1 ? " Below the note itself: A3's fundamental is turned down a little and every harmonic above it a lot, a dark, round sound."
                                                       : " For A3, the first " + juce::String (below) + (below == 1 ? " harmonic passes" : " harmonics pass")
                                                             + ", those above are turned down.");

            if (std::abs (s.envOctaves) > 0.05f)
                text << " The envelope moves it from here (see Amount).";

            return text;
        }

        case (int) Knob::resonance:
        {
            const double peak = sub::getFilterDb (s.filterType, s.steep, 1000.0, s.resonancePercent, 1000.0);
            return "Q " + juce::String (sub::getQ (s.resonancePercent), 1) + ": right at the cutoff the filter is " + formatDb (peak) + ".";
        }

        case (int) Knob::keyTrack:
            return juce::String (juce::roundToInt (s.keyTrackPercent)) + "%: an octave up the keyboard moves the cutoff "
                   + juce::String (s.keyTrackPercent / 100.0f, 2) + " octaves. From middle C, A3 moves it "
                   + juce::String (s.keyTrackPercent / 100.0f * -0.25f, 2) + " octaves.";

        case (int) Knob::envAmount:
        case (int) Knob::filterAttack:
        case (int) Knob::filterDecay:
        case (int) Knob::filterSustain:
        case (int) Knob::filterRelease:
        {
            if (std::abs (s.envOctaves) < 0.05f)
                return "The amount is 0, so the filter envelope does nothing yet: turn Amount up to hear it.";

            const double base = sub::getCutoffAt (s, sub::testNote, 1.0f, 0.0f, 0.0f);
            const double top = sub::getCutoffAt (s, sub::testNote, 1.0f, 1.0f, 0.0f);
            const double held = sub::getCutoffAt (s, sub::testNote, 1.0f, s.filterEnv.sustain, 0.0f);
            return "For A3 at full velocity: from " + formatHz (base) + " the filter " + (s.envOctaves > 0.0f ? "opens" : "closes") + " to "
                   + formatHz (top) + " in " + sub::formatKnob (Knob::filterAttack, s) + ", settles at " + formatHz (held) + " after "
                   + sub::formatKnob (Knob::filterDecay, s) + " while held, and goes back to " + formatHz (base) + " in "
                   + sub::formatKnob (Knob::filterRelease, s) + " after the key is let go.";
        }

        case (int) Knob::ampAttack:
        case (int) Knob::ampDecay:
        case (int) Knob::ampSustain:
        case (int) Knob::ampRelease:
            return "A note rises in " + sub::formatKnob (Knob::ampAttack, s) + ", falls to " + sub::formatKnob (Knob::ampSustain, s)
                   + " in " + sub::formatKnob (Knob::ampDecay, s) + (s.ampEnv.sustain < 0.005f ? " and is gone, even if held" : " and holds there")
                   + ", and fades out in " + sub::formatKnob (Knob::ampRelease, s) + " after the key is let go.";

        case (int) Knob::lfoRate:
            return sub::formatKnob (Knob::lfoRate, s) + ": a cycle every " + juce::String (1000.0 / s.lfoRateHz, 0) + " ms"
                   + (s.lfoDepthPercent < 0.5f ? ", but the depth is 0, so it moves nothing." : ".");

        case (int) Knob::lfoDepth:
        case (int) Knob::lfoTarget:
        {
            const float d = s.lfoDepthPercent / 100.0f;

            if (d < 0.005f)
                return "Depth 0: the LFO moves nothing.";

            switch (s.lfoTarget)
            {
                case sub::LfoTarget::pitch:  return "Vibrato: the pitch goes up and down by " + juce::String (d * sub::lfoPitchSemitones, 2) + " semitones.";
                case sub::LfoTarget::filter: return "The cutoff goes up and down by " + juce::String (d * sub::lfoFilterOctaves, 2) + " octaves.";
                case sub::LfoTarget::volume: return "Tremolo: the level dips to " + juce::String (juce::roundToInt (100.0f - d * 100.0f)) + "% and back.";
                case sub::LfoTarget::width:  return "The pulse width swings " + juce::String (d * sub::lfoWidthPoints, 0) + " points each way"
                                                    + (s.wave1 != Wave::square && s.wave2 != Wave::square ? ", but no oscillator is a square." : ".");
            }

            return {};
        }

        case (int) Knob::glide:
            return s.glideSeconds < 0.0005f ? juce::String ("Off: notes jump straight to their pitch.")
                                            : "Notes slide to their pitch in about " + sub::formatKnob (Knob::glide, s)
                                                  + (s.mode == sub::Mode::mono ? ", when you play them legato." : ", from the last note played.");

        case (int) Knob::mode:
            return s.mode == sub::Mode::mono ? "Mono: one note at a time, legato notes don't restart the envelopes."
                                             : "Poly: up to " + juce::String (BuiltInSubtractive::numVoices) + " notes at once.";

        case (int) Knob::velocity:
        {
            const float half = sub::getVelocityScale (s, 0.5f);
            return "A note played half as hard is " + formatDb (juce::Decibels::gainToDecibels (half)) + " and moves the filter "
                   + juce::String (juce::roundToInt (half * 100.0f)) + "% as far.";
        }

        case (int) Knob::output:
            return "A single saw note at full velocity peaks at about " + formatDb (sub::sawPeakDb + s.outputDb) + ".";

        default:
            return {};
    }
}

//==============================================================================
void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r, const sub::Settings& s, double noteHz, double cutoffHz, bool labels)
{
    auto plot = r.withTrimmedLeft (labels ? 34.0f : 0.0f).withTrimmedBottom (labels ? 16.0f : 0.0f);
    constexpr float topDb = 12.0f, bottomDb = -60.0f;
    auto toY = [&plot] (double db) { return juce::jmap ((float) juce::jlimit ((double) bottomDb, (double) topDb, db), bottomDb, topDb, plot.getBottom(), plot.getY()); };

    for (const double db : { 0.0, -24.0, -48.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);

        if (labels)
            drawLabel (g, juce::String ((int) db), { r.getX(), toY (db) - 7.0f, 30.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
    }

    for (const double hz : { 100.0, 1000.0, 10000.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (getX (plot, hz), plot.getY(), 1.0f, plot.getHeight());

        if (labels)
            drawLabel (g, hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + " kHz" : juce::String ((int) hz) + " Hz",
                       juce::Rectangle<float> (60.0f, 14.0f).withCentre ({ getX (plot, hz), plot.getBottom() + 8.0f }), theme::textDim,
                       juce::Justification::centred);
    }

    // The harmonics, against a saw's fundamental at full level (0 dB).
    const auto partials = sub::getPartials (s, noteHz, cutoffHz);
    const double reference = 2.0 / juce::MathConstants<double>::pi;

    for (auto& p : partials)
    {
        const float x = getX (plot, p.hz);

        if (x < plot.getX() || x > plot.getRight())
            continue;

        const double before = juce::Decibels::gainToDecibels (p.getBefore() / reference, -100.0);
        const double after = juce::Decibels::gainToDecibels (p.getAfter() / reference, -100.0);
        const auto c = p.oscillator == 2 ? secondColour : colour;
        g.setColour (theme::textDim.withAlpha (0.35f));
        g.fillRect (x - 1.0f, toY (before), 2.5f, plot.getBottom() - toY (before));
        g.setColour (c);
        g.fillRect (x - 1.0f, toY (after), 2.5f, plot.getBottom() - toY (after));
    }

    // The filter.
    juce::Path curve;

    for (int i = 0; i <= (int) plot.getWidth(); i += 2)
    {
        const double hz = 20.0 * std::pow (1000.0, i / (double) plot.getWidth());
        const float y = toY (sub::getFilterDb (s.filterType, s.steep, cutoffHz, s.resonancePercent, hz));
        i == 0 ? curve.startNewSubPath (plot.getX() + (float) i, y) : curve.lineTo (plot.getX() + (float) i, y);
    }

    g.setColour (filterColour);
    g.strokePath (curve, juce::PathStrokeType (1.8f));

    const float cx = getX (plot, cutoffHz);
    g.setColour (filterColour.withAlpha (0.5f));
    g.fillRect (cx, plot.getY(), 1.0f, plot.getHeight());

    if (labels)
        drawLabel (g, formatHz (cutoffHz), { juce::jlimit (plot.getX(), plot.getRight() - 70.0f, cx + 4.0f), plot.getY(), 70.0f, 14.0f }, filterColour);
}

void drawWave (juce::Graphics& g, juce::Rectangle<float> r, const sub::Settings& s, double noteHz, double cutoffHz, double cycles)
{
    const auto partials = sub::getPartials (s, noteHz, cutoffHz, 12000.0);
    const auto before = sub::getWave (partials, cycles / noteHz, (int) r.getWidth(), false);
    const auto after = sub::getWave (partials, cycles / noteHz, (int) r.getWidth(), true);

    float peak = 0.2f;
    for (size_t i = 0; i < before.size(); ++i)
        peak = juce::jmax (peak, std::abs (before[i]), std::abs (after[i]));

    g.setColour (theme::text.withAlpha (0.15f));
    g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.0f);

    const float scale = r.getHeight() * 0.45f / peak;
    drawLine (g, r, before, scale, theme::textDim.withAlpha (0.6f), 1.2f);
    drawLine (g, r, after, scale, colour, 2.0f);
}

void drawEnvelope (juce::Graphics& g, juce::Rectangle<float> r, const sub::Envelope& e, juce::Colour c, const juce::String& name,
                   double age, double releasedAt, bool labels)
{
    if (labels)
    {
        auto top = r.removeFromTop (15.0f);
        drawLabel (g, name, top, c);
    }

    auto plot = r.reduced (2.0f, 3.0f);

    // Time is drawn on a square-root scale, so 2 ms and 2 s both have room; the held part is a fixed stretch.
    const double attackDecay = e.attack + e.decay;
    const float held = plot.getWidth() * 0.2f;
    const float shaped = plot.getWidth() - held;
    const double total = attackDecay + e.release;
    auto scaleT = [&] (double t) { return (float) std::sqrt (juce::jmax (0.0, t) / juce::jmax (1.0e-6, total)); };
    const float aX = plot.getX() + shaped * scaleT (e.attack) / juce::jmax (1.0e-6f, scaleT (attackDecay) + scaleT (e.release));
    const float dX = plot.getX() + shaped * scaleT (attackDecay) / juce::jmax (1.0e-6f, scaleT (attackDecay) + scaleT (e.release));
    const float sEnd = dX + held;
    const float rEnd = plot.getRight();
    auto y = [&plot] (double level) { return plot.getBottom() - plot.getHeight() * (float) level; };

    juce::Path shape;
    shape.startNewSubPath (plot.getX(), plot.getBottom());
    shape.lineTo (aX, y (1.0));
    shape.lineTo (dX, y (e.sustain));
    shape.lineTo (sEnd, y (e.sustain));
    shape.lineTo (rEnd, plot.getBottom());

    juce::Path filled (shape);
    filled.closeSubPath();
    g.setColour (c.withAlpha (0.15f));
    g.fillPath (filled);
    g.setColour (c);
    g.strokePath (shape, juce::PathStrokeType (1.8f));

    if (labels)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        const float h = 14.0f;
        g.drawText ("A", juce::Rectangle<float> (plot.getX(), plot.getBottom() - h, juce::jmax (12.0f, aX - plot.getX()), h), juce::Justification::centred);
        g.drawText ("D", juce::Rectangle<float> (aX, plot.getBottom() - h, juce::jmax (12.0f, dX - aX), h), juce::Justification::centred);
        g.drawText ("S", juce::Rectangle<float> (dX, plot.getBottom() - h, held, h), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (sEnd, plot.getBottom() - h, rEnd - sEnd, h), juce::Justification::centred);
    }

    // Where the newest note is.
    if (age >= 0.0)
    {
        float x = plot.getX();
        const double level = sub::getEnvelopeLevel (e, age, releasedAt);

        if (releasedAt >= 0.0 && age >= releasedAt)
            x = sEnd + (rEnd - sEnd) * juce::jlimit (0.0f, 1.0f, scaleT (age - releasedAt) / juce::jmax (1.0e-6f, scaleT (e.release)));
        else if (age < attackDecay)
            x = plot.getX() + shaped * scaleT (age) / juce::jmax (1.0e-6f, scaleT (attackDecay) + scaleT (e.release));
        else
            x = dX + held * 0.5f;

        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ x, y (level) }));
    }
}

void drawFlow (juce::Graphics& g, juce::Rectangle<float> r, const sub::Settings& s, int highlight)
{
    const float boxH = juce::jmin (54.0f, r.getHeight() * 0.3f);
    auto row = r.withSizeKeepingCentre (r.getWidth(), boxH).translated (0.0f, -r.getHeight() * 0.12f);
    const float gap = 26.0f;
    const float boxW = (row.getWidth() - gap * 4.0f) / 5.0f;

    struct Box { juce::String name, detail; juce::Colour c; int part; };
    const Box boxes[] = {
        { "Oscillators", sub::getWaveName (s.wave1) + (s.mixPercent > 0.5f ? " + " + sub::getWaveName (s.wave2) : juce::String()), colour, 0 },
        { "Mixer", "levels + noise", colour, 1 },
        { "Filter", sub::getFilterTypeName (s.filterType) + " " + sub::formatKnob (Knob::cutoff, s), filterColour, 2 },
        { "Amp", "volume", ampColour, 3 },
        { "Out", "to the track", theme::text, -2 } };

    std::array<juce::Rectangle<float>, 5> areas;

    for (int i = 0; i < 5; ++i)
    {
        areas[(size_t) i] = row.removeFromLeft (boxW);
        row.removeFromLeft (gap);
        const auto& b = boxes[i];
        const bool lit = b.part == highlight;
        g.setColour (b.c.withAlpha (lit ? 0.45f : 0.15f));
        g.fillRoundedRectangle (areas[(size_t) i], 6.0f);
        g.setColour (b.c.withAlpha (lit ? 1.0f : 0.6f));
        g.drawRoundedRectangle (areas[(size_t) i], 6.0f, lit ? 2.5f : 1.2f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawFittedText (b.name, areas[(size_t) i].withTrimmedBottom (boxH * 0.45f).toNearestInt(), juce::Justification::centredBottom, 1);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText (b.detail, areas[(size_t) i].withTrimmedTop (boxH * 0.55f).toNearestInt().reduced (3, 0), juce::Justification::centredTop, 1);

        if (i < 4)
        {
            juce::Path arrow;
            const float y = areas[(size_t) i].getCentreY();
            arrow.addArrow ({ areas[(size_t) i].getRight() + 3.0f, y, areas[(size_t) i].getRight() + gap - 3.0f, y }, 2.0f, 9.0f, 8.0f);
            g.setColour (theme::textDim);
            g.fillPath (arrow);
        }
    }

    // What moves them, underneath.
    auto below = [&] (int box, const juce::String& name, juce::Colour c, bool lit, float offset)
    {
        const auto target = areas[(size_t) box];
        const auto label = juce::Rectangle<float> (target.getWidth() + gap, 30.0f).withCentre ({ target.getCentreX() + offset, target.getBottom() + 52.0f });
        g.setColour (c.withAlpha (lit ? 0.4f : 0.12f));
        g.fillRoundedRectangle (label, 5.0f);
        g.setColour (c.withAlpha (lit ? 1.0f : 0.7f));
        g.drawRoundedRectangle (label, 5.0f, lit ? 2.0f : 1.0f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawFittedText (name, label.toNearestInt(), juce::Justification::centred, 1);

        juce::Path arrow;
        arrow.addArrow ({ label.getCentreX(), label.getY() - 2.0f, target.getCentreX(), target.getBottom() + 3.0f }, 1.6f, 8.0f, 7.0f);
        g.fillPath (arrow);
    };

    below (2, "Filter envelope", filterColour, highlight == 4, 0.0f);
    below (3, "Amp envelope", ampColour, highlight == 4, 0.0f);

    const int lfoBox = s.lfoTarget == sub::LfoTarget::filter ? 2 : s.lfoTarget == sub::LfoTarget::volume ? 3 : 0;
    const auto target = areas[(size_t) lfoBox];
    const auto lfo = juce::Rectangle<float> (target.getWidth(), 28.0f).withCentre ({ target.getCentreX(), target.getY() - 40.0f });
    g.setColour (lfoColour.withAlpha (highlight == 5 ? 0.4f : 0.12f));
    g.fillRoundedRectangle (lfo, 5.0f);
    g.setColour (lfoColour.withAlpha (highlight == 5 ? 1.0f : 0.7f));
    g.drawRoundedRectangle (lfo, 5.0f, highlight == 5 ? 2.0f : 1.0f);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawFittedText ("LFO: " + sub::getLfoTargetName (s.lfoTarget).toLowerCase(), lfo.toNearestInt(), juce::Justification::centred, 1);
    juce::Path arrow;
    arrow.addArrow ({ lfo.getCentreX(), lfo.getBottom() + 2.0f, target.getCentreX(), target.getY() - 3.0f }, 1.6f, 8.0f, 7.0f);
    g.fillPath (arrow);
}

void drawOscillator (juce::Graphics& g, juce::Rectangle<float> r, Wave w, float widthPercent, juce::Colour c)
{
    auto top = r.removeFromTop (r.getHeight() * 0.5f);
    drawLabel (g, sub::getWaveName (w) + (w == Wave::square && widthPercent < 49.5f ? " " + juce::String (juce::roundToInt (widthPercent)) + "%" : juce::String()),
               top.removeFromTop (15.0f), c);

    // One and a half cycles, drawn from the oscillator's own formula.
    std::vector<float> cycle ((size_t) juce::jmax (2, (int) top.getWidth()));
    const double width = juce::jlimit (0.05, 0.95, widthPercent / 100.0);

    for (size_t i = 0; i < cycle.size(); ++i)
    {
        const double p = std::fmod (1.5 * (double) i / (double) (cycle.size() - 1), 1.0);

        switch (w)
        {
            case Wave::saw:      cycle[i] = (float) (2.0 * p - 1.0); break;
            case Wave::square:   cycle[i] = (float) ((p < width ? 1.0 : -1.0) - (2.0 * width - 1.0)) * 0.7f; break;
            case Wave::triangle: cycle[i] = (float) (p < 0.25 ? 4.0 * p : p < 0.75 ? 2.0 - 4.0 * p : 4.0 * p - 4.0); break;
            case Wave::sine:     cycle[i] = (float) std::sin (juce::MathConstants<double>::twoPi * p); break;
        }
    }

    g.setColour (theme::text.withAlpha (0.15f));
    g.fillRect (top.getX(), top.getCentreY(), top.getWidth(), 1.0f);
    drawLine (g, top.reduced (0.0f, 4.0f), cycle, top.getHeight() * 0.38f, c, 1.8f);

    // The first 12 harmonics.
    r.removeFromTop (6.0f);
    const float barW = r.getWidth() / 12.0f;
    const double first = std::abs (sub::getHarmonic (w, widthPercent, 1));

    for (int n = 1; n <= 12; ++n)
    {
        const double level = std::abs (sub::getHarmonic (w, widthPercent, n)) / juce::jmax (1.0e-6, first);
        const float db = juce::Decibels::gainToDecibels ((float) level, -40.0f);
        const float h = r.getHeight() * (db + 40.0f) / 40.0f;
        g.setColour (c.withAlpha (0.85f));
        g.fillRect (r.getX() + barW * (float) (n - 1) + 1.0f, r.getBottom() - h, barW - 2.0f, h);
    }
}

void drawLfo (juce::Graphics& g, juce::Rectangle<float> r, const sub::Settings& s, double seconds)
{
    constexpr double span = 3.0;
    std::vector<float> wave ((size_t) juce::jmax (2, (int) r.getWidth()));
    const double rateHz = juce::jmax (0.2, (double) s.lfoRateHz);

    for (size_t i = 0; i < wave.size(); ++i)
        wave[i] = (float) std::sin (juce::MathConstants<double>::twoPi * rateHz * (seconds + span * (double) i / (double) (wave.size() - 1)));

    g.setColour (theme::text.withAlpha (0.15f));
    g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.0f);
    const float depth = juce::jmax (0.15f, s.lfoDepthPercent / 100.0f);
    drawLine (g, r, wave, r.getHeight() * 0.45f * depth, lfoColour, 2.0f);
    drawLabel (g, "3 seconds of the LFO at " + sub::formatKnob (Knob::lfoRate, s) + " moving the " + sub::getLfoTargetName (s.lfoTarget).toLowerCase(),
               r.withTop (r.getBottom() - 14.0f));
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInSubtractive& synth)
{
    BuiltInSubtractive* processor = &synth;
    std::vector<fxui::Guide::Page> pages;

    pages.push_back ({ "Start rich, take away", "What subtractive synthesis is",
        "A subtractive synthesiser starts with a sound full of harmonics and takes away the ones you don't want, the way a sculptor "
        "starts with a block of stone. An oscillator makes a bright, buzzy wave; a filter takes harmonics away to shape the timbre; "
        "an amplifier shapes how loud each note is over time; and envelopes and an LFO move the filter and the level while the note "
        "plays, so the sound lives.\n\n"
        "It is one of the oldest kinds of synthesis, and for decades it was nearly the only one. Robert Moog showed his modular synthesiser in 1964, and Wendy Carlos's "
        "Switched-On Bach (1968) made it famous. The Minimoog (1970) put oscillators, a filter and envelopes in one keyboard and "
        "became the sound of synth basses and leads; the Prophet-5 (1978), the first polyphonic synth that could store its sounds, "
        "and the Roland Juno series (from 1982) brought it to chords; the Roland TB-303 (1981) gave acid house its squelch. Many software synths today still work this way at heart, and the knobs "
        "on DawIt Subtractive are the same ones you'll find on almost any of them.\n\n"
        "The picture shows the path the sound takes, left to right: the oscillators make it, the mixer blends them, the filter "
        "shapes it, the amp sets its level, and the envelopes and the LFO (moving the part they point at) make it change over "
        "time. Each page that follows looks at one part.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const int part = (int) std::fmod (now() / 1.5, 6.0);
            drawCaption (g, r, "The path of the sound", "now lit: " + juce::StringArray { "oscillators", "mixer", "filter", "amp", "envelopes", "LFO" }[part]);
            drawFlow (g, r.withTrimmedTop (30.0f), processor->getSettings(), part);
        } });

    pages.push_back ({ "The oscillators", "Saw, square, triangle, sine",
        "The oscillator makes the raw material: a wave that repeats as many times a second as the note's frequency (A3 is 220 "
        "times). Any repeating wave is a sum of sines at whole-number multiples of that frequency, the harmonics (see What is "
        "sound? in the wave editor), and the wave's shape decides which harmonics it has and how loud.\n\n"
        "The saw ramps up and drops: it has every harmonic, the nth at 1/n of the first's level. Bright and buzzy, it's the "
        "favourite starting point, because there's the most for the filter to work with: strings, brass, basses, leads.\n\n"
        "The square jumps between up and down: only the odd harmonics (1, 3, 5...), also at 1/n. Hollow and woody, like the "
        "low notes of a clarinet, whose tube, closed at one end, makes mostly odd harmonics there. Squeeze it into a narrow pulse with Width and it turns thin "
        "and nasal.\n\n"
        "The triangle has the odd harmonics too, but at 1/n squared, so they fade fast: soft and round, like a flute.\n\n"
        "The sine is the fundamental alone. Pure, and nothing to filter: for sub basses, whistles and soft tones.\n\n"
        "The picture shows each wave and its first twelve harmonics as bars.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Four waves and their harmonics", "bars: harmonics 1 to 12, as loud as against the first");
            const float w = (r.getWidth() - 3.0f * 16.0f) / 4.0f;
            const Wave waves[] = { Wave::saw, Wave::square, Wave::triangle, Wave::sine };

            for (int i = 0; i < 4; ++i)
            {
                auto box = r.removeFromLeft (w);
                r.removeFromLeft (16.0f);
                drawOscillator (g, box, waves[i], waves[i] == Wave::square ? processor->getSettings().widthPercent : 50.0f,
                                waves[i] == processor->getSettings().wave1 ? colour : theme::textDim);
            }
        } });

    pages.push_back ({ "Two oscillators and noise", "Detune, octaves, fifths, breath",
        "One oscillator can sound thin. A second one, mixed in under it, can add weight, width or a second note.\n\n"
        "Detuned by a few cents (hundredths of a semitone), the two waves slide in and out of step: when their peaks line up they "
        "add up, half a beat later they partly cancel (see Phase in What is sound?). You hear the level swell and fall, a beating "
        "as many times a second as the difference between their frequencies: at A3, 7 cents apart, about once a second. That slow "
        "movement is what makes detuned synths sound wide and alive, like an ensemble that's never quite in tune.\n\n"
        "Tuned an octave below, the second oscillator adds weight to a bass; an octave above, brightness; a fifth above, the thick "
        "sound of two notes in one, used on leads.\n\n"
        "Noise is every frequency at once, at random. A little adds breath or air; on its own, through the filter, it makes wind, "
        "surf or the hiss of a hi-hat.\n\n"
        "The picture shows two sines a few cents apart, added up: the level of the sum swells and falls with the beating.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const float cents = s.detuneCents > 0.05f ? s.detuneCents : 7.0f;
            const double beat = sub::getBeatHz (sub::testNoteHz, cents);
            drawCaption (g, r, "Two sines at A3, " + juce::String (cents, 1) + " cents apart, added up over 2 seconds",
                         "they beat " + juce::String (beat, 2) + " times a second");

            std::vector<float> sum ((size_t) juce::jmax (2, (int) r.getWidth() * 2));
            for (size_t i = 0; i < sum.size(); ++i)
            {
                // The sum's level: 2 |cos (pi * beat * t)|, drawn as the wave would fill it.
                const double t = 2.0 * (double) i / (double) (sum.size() - 1);
                const double envelope = 2.0 * std::abs (std::cos (juce::MathConstants<double>::pi * beat * t));
                sum[i] = (float) (envelope * ((i % 2 == 0) ? 1.0 : -1.0));
            }

            g.setColour (colour.withAlpha (0.7f));
            for (size_t i = 0; i < sum.size(); i += 2)
            {
                const float x = r.getX() + r.getWidth() * (float) i / (float) (sum.size() - 1);
                const float h = std::abs (sum[i]) * r.getHeight() * 0.22f;
                g.fillRect (x, r.getCentreY() - h, 1.0f, h * 2.0f);
            }

            drawLabel (g, "loud where the two are in step, quiet where they cancel", r.withTop (r.getBottom() - 14.0f));
        } });

    pages.push_back ({ "The filter", "Low-pass, high-pass, band-pass, cutoff, slope",
        "The filter is the heart of a subtractive synth: it decides which harmonics stay.\n\n"
        "A low-pass filter lets everything below its cutoff through and turns down what's above, more and more the higher it is. "
        "Turn the cutoff down and a saw goes from bright and buzzy to warm, to dark, to a muffled thud. Most synth sounds use a "
        "low-pass, because it sounds natural: real instruments are darker when played softly and get darker as they fade.\n\n"
        "A high-pass does the opposite: it takes the low end and the body away, for thin sounds, hi-hats made from noise, or a pad "
        "that must stay out of the bass's way. A band-pass keeps only a band around the cutoff: nasal and vocal, like a telephone.\n\n"
        "The slope is how fast the filter turns things down past the cutoff: 12 dB per octave (2 poles), the gentler sound of the "
        "Oberheim SEM, or 24 dB (4 poles), the steeper and fatter Moog ladder. With 24 dB, a harmonic an octave above the cutoff "
        "is about 24 dB quieter.\n\n"
        "The picture sweeps the cutoff of your filter up and down over a saw at A3: grey bars are the harmonics the saw makes, "
        "coloured what's left.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            const double cutoff = 100.0 * std::pow (60.0, 0.5 + 0.5 * std::sin (now() * 0.7));
            s.mixPercent = 0.0f;
            drawCaption (g, r, sub::getFilterTypeName (s.filterType) + ", " + sub::formatKnob (Knob::slope, s) + ", the cutoff at " + formatHz (cutoff),
                         "orange: the filter");
            drawSpectrum (g, r, s, sub::testNoteHz, cutoff, true);
        } });

    pages.push_back ({ "Resonance", "The peak, the squelch, the whistle",
        "Resonance feeds some of the filter's output back into it, so the frequencies right at the cutoff are boosted while those "
        "above are still cut. The filter gets a peak.\n\n"
        "A little resonance gives the sound an edge. More makes the filter sing: the harmonic nearest the cutoff stands out like a "
        "whistle. And when the cutoff moves, with the filter envelope or the LFO, that peak sweeps across the harmonics one by one: "
        "the \"wow\" of a synth brass swell, the squelch of an acid bass, the \"yoy\" of a wobble. Near the top the filter almost "
        "rings on its own, a pure tone at the cutoff even with little going in.\n\n"
        "Resonance makes a sound louder at the cutoff, so turn the output down as you turn it up, and be careful with it on basses: "
        "a peak in the low end can boom.\n\n"
        "The picture shows the low-pass filter at 1 kHz with no resonance, then more and more: the peak grows at the cutoff.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "A 24 dB low-pass at 1 kHz with resonance 0%, 30%, 60% and 90%", "the peak at the cutoff grows");
            auto plot = r.withTrimmedLeft (34.0f).withTrimmedBottom (16.0f);
            auto toY = [&plot] (double db) { return juce::jmap ((float) juce::jlimit (-48.0, 30.0, db), -48.0f, 30.0f, plot.getBottom(), plot.getY()); };

            for (const double db : { 24.0, 0.0, -24.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
                drawLabel (g, (db > 0 ? "+" : "") + juce::String ((int) db), { r.getX(), toY (db) - 7.0f, 30.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
            }

            for (const float res : { 0.0f, 30.0f, 60.0f, 90.0f })
            {
                juce::Path curve;

                for (int i = 0; i <= (int) plot.getWidth(); i += 2)
                {
                    const double hz = 20.0 * std::pow (1000.0, i / (double) plot.getWidth());
                    const float y = toY (sub::getFilterDb (sub::FilterType::lowPass, true, 1000.0, res, hz));
                    i == 0 ? curve.startNewSubPath (plot.getX() + (float) i, y) : curve.lineTo (plot.getX() + (float) i, y);
                }

                g.setColour (filterColour.withAlpha (0.35f + res / 150.0f));
                g.strokePath (curve, juce::PathStrokeType (1.6f));
            }

            const double peak = sub::getFilterDb (sub::FilterType::lowPass, true, 1000.0, 90.0f, 1000.0);
            drawLabel (g, "at 90%: " + formatDb (peak) + " at the cutoff", { getX (plot, 1000.0) + 6.0f, toY (peak) - 6.0f, 160.0f, 14.0f }, filterColour);
        } });

    pages.push_back ({ "Envelopes: attack, decay, sustain, release", "How a note starts, holds and ends",
        "A sound isn't the same from start to end. A piano note hits hard and fades; an organ note stays the same while the key is "
        "down; a violin swells in. An envelope draws that shape, in four parts:\n\n"
        "Attack: how long it takes to rise to full when the key goes down.\n"
        "Decay: how long it then takes to fall to the sustain level.\n"
        "Sustain: the level it holds while the key stays down. A level, not a time.\n"
        "Release: how long it takes to fade to nothing after the key is let go.\n\n"
        "On the amp, the envelope shapes the volume. A pluck: attack 1 ms, decay half a second, sustain 0, so it dies even when "
        "held. An organ: attack and release a few ms, sustain 100%. A pad: attack around a second, sustain high, release a couple "
        "of seconds, so it fades in and out. Brass: an attack of 30-50 ms, like breath, and a high sustain.\n\n"
        "What is sound? > The shape is the timbre measures the envelope of a note in your own clip. The picture shows these four shapes. Your amp envelope is in the window, with a dot following the newest note.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Four shapes of a note: the same knobs, different settings");
            const std::pair<const char*, sub::Envelope> shapes[] = { { "Pluck: A 1 ms, D 0.5 s, S 0%, R 0.3 s", { 0.001f, 0.5f, 0.0f, 0.3f } },
                                                                     { "Organ: A 5 ms, S 100%, R 50 ms", { 0.005f, 0.05f, 1.0f, 0.05f } },
                                                                     { "Pad: A 1 s, D 1 s, S 80%, R 2 s", { 1.0f, 1.0f, 0.8f, 2.0f } },
                                                                     { "Brass: A 40 ms, D 0.2 s, S 90%, R 0.25 s", { 0.04f, 0.2f, 0.9f, 0.25f } } };
            const float w = (r.getWidth() - 20.0f) / 2.0f, h = (r.getHeight() - 16.0f) / 2.0f;

            for (int i = 0; i < 4; ++i)
            {
                const juce::Rectangle<float> box (r.getX() + (float) (i % 2) * (w + 20.0f), r.getY() + (float) (i / 2) * (h + 16.0f), w, h);
                drawEnvelope (g, box, shapes[i].second, ampColour, shapes[i].first, -1.0, -1.0, true);
            }
        } });

    pages.push_back ({ "The filter envelope and key tracking", "Making the timbre move within each note",
        "The second envelope moves the filter's cutoff, so each note's brightness changes as it plays. Amount says how far, in "
        "octaves, at the envelope's peak.\n\n"
        "This is what makes a synth sound like something. A pluck: the filter snaps open at the start and closes over a few hundred "
        "milliseconds, so the note starts bright and turns dark, like a plucked string. Brass: the filter's attack is slower than "
        "the amp's, so the note swells brighter, the \"bwaah\". An acid bass: a short decay and lots of resonance, so every note "
        "goes \"yow\".\n\n"
        "Velocity makes softer notes move the filter less, so they're duller as well as quieter, as on an acoustic instrument.\n\n"
        "Key tracking moves the cutoff with the note you play. Without it, the filter stays put, so high notes have fewer "
        "harmonics below the cutoff and sound duller than low ones; at 100%, every note is as bright. Most sounds sit around half.\n\n"
        "The picture plays a note over and over with your filter envelope (with an amount of at least 3 octaves, so it shows): "
        "watch the harmonics open and close.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();

            if (std::abs (s.envOctaves) < 3.0f)
                s.envOctaves = 3.0f;

            if (s.cutoffHz > 5000.0f)
                s.cutoffHz = 300.0f;

            const double length = juce::jlimit (0.6, 3.0, (double) (s.filterEnv.attack + s.filterEnv.decay) * 1.5 + 0.3);
            const double t = std::fmod (now(), length + s.filterEnv.release + 0.4);
            const double releasedAt = length;
            const float level = sub::getEnvelopeLevel (s.filterEnv, t, releasedAt);
            const double cutoff = sub::getCutoffAt (s, sub::testNote, 1.0f, level, 0.0f);

            drawCaption (g, r, "A note at A3: the filter envelope moves the cutoff to " + formatHz (cutoff),
                         t < releasedAt ? "key held" : "key let go");
            auto envelopeArea = r.removeFromBottom (r.getHeight() * 0.3f);
            drawSpectrum (g, r.withTrimmedBottom (8.0f), s, sub::testNoteHz, cutoff, true);
            drawEnvelope (g, envelopeArea, s.filterEnv, filterColour, "the filter envelope", t, releasedAt, true);
        } });

    pages.push_back ({ "The LFO", "Vibrato, sweeps, wobbles, PWM",
        "An LFO, a low-frequency oscillator, is a wave too slow to hear as a note, from a few cycles a minute to about 20 a second. "
        "Instead of being heard, it moves something else up and down, over and over.\n\n"
        "On the pitch it's vibrato: 5-7 times a second and a small depth, like a singer or a violinist. On the filter it's a sweep "
        "(slow) or a wobble (fast, and in time with the song: at 120 BPM, 2 Hz is once a beat, the dubstep wub). On the volume it's "
        "tremolo, the pulse of an old guitar amp. On a square's width it's pulse-width modulation: the harmonics keep changing, "
        "which sounds like several oscillators detuned, part of the lush sound of the Roland Juno, along with its chorus.\n\n"
        "Keep the depth small at first: a little vibrato sounds alive, a lot sounds seasick.\n\n"
        "The picture shows your LFO over 3 seconds, moving along.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The LFO: " + sub::formatKnob (Knob::lfoRate, s) + ", depth " + sub::formatKnob (Knob::lfoDepth, s),
                         "to the " + sub::getLfoTargetName (s.lfoTarget).toLowerCase());
            drawLfo (g, r, s, now());
        } });

    pages.push_back ({ "Voices, glide and velocity", "Poly and mono, legato, portamento",
        "Poly lets the synth play chords: each note gets its own voice, with its own envelopes and filter (up to 16 here). Mono "
        "plays one note at a time, like the Minimoog and the TB-303. That isn't only a limit: a new note played while the last one "
        "is still held takes over without restarting the envelopes (legato), so a fast line sounds smooth and connected, and with "
        "glide it slides from one pitch to the next. Bass lines and solo leads usually play mono.\n\n"
        "Glide (portamento) makes the pitch slide instead of jumping. A short glide (30-100 ms) gives leads their singing slides "
        "and acid basses their slurs; long glides are an effect.\n\n"
        "Velocity is how hard a key is played. Here it can make soft notes quieter and darker, so the part can be played with "
        "feeling.\n\n"
        "The picture shows a line of notes as steps, and the pitch with glide sliding between them.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto shown = processor->getSettings();

            if (shown.glideSeconds <= 0.0005f)
                shown.glideSeconds = 0.08f;

            const double glide = shown.glideSeconds;
            drawCaption (g, r, "Four notes, half a second each, with a glide of " + sub::formatKnob (Knob::glide, shown),
                         "grey: the notes; colour: the pitch you hear");
            const int notes[] = { 45, 52, 57, 50 };
            auto y = [&r] (double note) { return r.getBottom() - r.getHeight() * (float) ((note - 42.0) / 18.0); };
            juce::Path steps, pitch;
            double current = notes[0];

            for (int i = 0; i <= (int) r.getWidth(); ++i)
            {
                const double t = 2.0 * i / r.getWidth();
                const int index = juce::jmin (3, (int) (t / 0.5));
                const float x = r.getX() + (float) i;
                current += (notes[index] - current) * (1.0 - std::exp (-3.0 / (glide * r.getWidth() / 2.0)));
                i == 0 ? steps.startNewSubPath (x, y (notes[index])) : steps.lineTo (x, y (notes[index]));
                i == 0 ? pitch.startNewSubPath (x, y (current)) : pitch.lineTo (x, y (current));
            }

            g.setColour (theme::textDim.withAlpha (0.6f));
            g.strokePath (steps, juce::PathStrokeType (1.2f));
            g.setColour (colour);
            g.strokePath (pitch, juce::PathStrokeType (2.2f));
        } });

    pages.push_back ({ "Building a sound, step by step", "From Init to your own sound",
        "Start from Init (a plain saw, filter open, nothing moving) and work along the path of the sound, listening after every "
        "step. Play a note or a phrase over and over while you turn the knobs.\n\n"
        "1. The oscillators: what is the sound like? Bright and full: a saw. Hollow or woody: a square. Soft: a triangle. Then "
        "decide on oscillator 2: a few cents of detune for width, an octave down for weight, or leave it out.\n"
        "2. The filter: turn the cutoff down until the sound is as dark as it should be at its darkest, usually lower than you "
        "think. Add a little resonance for character.\n"
        "3. The amp envelope: how does a note start and end? A hit (attack near 0), a swell (longer attack), a pluck (sustain 0, "
        "the decay sets the length), a held tone (sustain high). Then the release: tight or ringing.\n"
        "4. The filter envelope: should the sound move within each note? Turn Amount up and set its attack and decay: short for a "
        "pluck or a bass, a slower attack for a brass swell.\n"
        "5. Movement: an LFO, small. Vibrato on a lead, a slow filter sweep on a pad.\n"
        "6. Playing: mono and glide for basses and leads, poly for chords; velocity for feel.\n"
        "7. The level: set the output so the part sits with the others, with room for chords and resonance.\n\n"
        "When a sound isn't right, go back along the path: too thin? oscillators. Too bright or too dull? the filter. Wrong at the "
        "start or end of notes? the envelopes. Too static? the filter envelope or the LFO.\n\n"
        "The picture lights each step in turn on the path of the sound.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const int step = (int) std::fmod (now() / 1.8, 7.0);
            const int parts[] = { 0, 2, 3, 4, 5, 1, 3 };
            const juce::StringArray names { "1. the oscillators", "2. the filter", "3. the amp envelope", "4. the filter envelope",
                                            "5. movement: the LFO", "6. playing: voices and glide", "7. the level" };
            drawCaption (g, r, "Building a sound along the path", names[step]);
            drawFlow (g, r.withTrimmedTop (30.0f), processor->getSettings(), parts[step]);
        } });

    pages.push_back ({ "Recipes for common sounds", "Bass, pad, pluck, brass, lead",
        "Every one of these is in the starting points, with notes on why each knob is set the way it is. Load one, play it, and "
        "change one thing at a time.\n\n"
        "Bass: a saw with a square an octave down, mono. Low-pass 24 dB at 150-300 Hz with half key tracking, a filter envelope of "
        "2-3 octaves with a quick decay (200-300 ms) and low sustain for the attack's bite, the amp at attack 0 and a short release.\n\n"
        "Pad: two saws detuned by 8-15 cents, poly. Low-pass at 500 Hz-1 kHz, slow attacks on both envelopes (0.5-1.5 s), a long "
        "release (1-2 s), a slow LFO (0.2-0.5 Hz) gently on the filter.\n\n"
        "Pluck: a saw (maybe a square an octave up), low-pass low with 3-4 octaves of envelope and a decay of about 200 ms, the "
        "amp's sustain at 0 so the note dies away, some velocity.\n\n"
        "Brass: two saws slightly detuned, a 12 dB low-pass at about 600 Hz, a filter envelope with an attack of 50-100 ms for the "
        "swell, the amp at about 40 ms attack, a touch of vibrato.\n\n"
        "Lead: a square and a saw, mono with 50-100 ms of glide, a brighter filter (2-4 kHz) with some resonance, vibrato at about "
        "5.5 Hz.\n\n"
        "Acid bass: one saw, mono with a little glide, a 24 dB low-pass low with high resonance (60-80%), a big filter envelope "
        "with a short decay and no sustain.\n\n"
        "The picture shows the harmonics and filter of the sound in the window now, for the test note.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double cutoff = processor->noteAge.load() >= 0.0f ? (double) processor->shownCutoffHz.load()
                                                                     : sub::getCutoffAt (s, sub::testNote, 1.0f, 0.0f, 0.0f);
            drawCaption (g, r, "The sound in the window now, for A3", "Play a note to see the filter move");
            drawSpectrum (g, r, s, sub::testNoteHz, cutoff, true);
        } });

    pages.push_back ({ "Words for sounds", "Bright, warm, fat, punchy: what they mean on the knobs",
        "People describe sounds with borrowed words: bright, dark, warm, fat, thin, punchy, soft. They aren't measurements, but most of them point at a few knobs. "
        "Knowing which ones turns \"make it warmer\" into something you can do. A study of English and Greek listeners found the same three families of words in "
        "both languages: how bright a sound is, its texture (smooth or rough) and its mass (thick or thin).\n\n"
        "Bright and dark: how much of the sound's energy sits high up. Listening tests find brightness follows the spectral centroid, the average frequency of the "
        "harmonics, weighted by how loud each one is. A plain saw on A3 (220 Hz) has its centroid near 3.9 kHz. A sine on the same note has it at 220 Hz, as dark as "
        "a note can be. To go brighter, turn Cutoff up (doubling it, say from 900 Hz to 1.8 kHz, moves it up an octave), use a saw or a square instead of a triangle "
        "or a sine, or add filter-envelope Amount. Darker is the opposite. The picture marks the centroid of the sound in the window now.\n\n"
        "Warm: a full lower middle and a soft top. Sound engineers mostly mean that, but people agree on \"warm\" less than on \"bright\", and \"warm\" and \"round\" "
        "are often used for each other. On this synth, try a low-pass at about 500 Hz to 2 kHz, little resonance and two saws.\n\n"
        "Fat, thick or phat: more than one oscillator at once. A second oscillator detuned by 5 to 20 cents beats against the first, which sounds bigger and alive. "
        "At 12 cents on A3 it beats about 1.5 times a second. An octave below adds weight. Thin is the opposite: one oscillator, a narrow pulse or a high-pass.\n\n"
        "Punchy, snappy, plucky: how a note starts and ends. Use a fast attack (1 to 5 ms), a short decay, and a filter envelope that opens for the first 100 to "
        "300 ms. A pluck also has Amp sustain at 0%, so it fades even while you hold the key.\n\n"
        "Soft, slow or swelling: a longer attack. Attack is a time, so \"less attack\" usually means a longer attack: 0.3 to 1 s for a pad. A long tail is the release.\n\n"
        "Squelchy: high resonance (60 to 80%) with a filter envelope, as in acid bass. Wobbly: the LFO on the filter, on the beat (2 or 4 Hz at 120 BPM). Vibrato: "
        "the LFO on the pitch, at about 5 to 6 Hz and small. Airy or breathy: 5 to 15% noise. Hollow or woody: a square wave.\n\n"
        "What these words can't mean here: DawIt Subtractive plays the same sound in both speakers, so \"wide\" means fuller, not wider in stereo. A reverb after it "
        "adds space. Reverb, echo and distortion are effects that go after the synth in the track's chain. All of this is common practice among synth programmers, "
        "not a set of rules: try the knob, then trust your ears.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double cutoff = processor->noteAge.load() >= 0.0f ? (double) processor->shownCutoffHz.load()
                                                                     : sub::getCutoffAt (s, sub::testNote, 1.0f, 0.0f, 0.0f);
            const double centroid = sub::getCentroidHz (s, sub::testNoteHz, true);
            const auto hz = centroid >= 1000.0 ? juce::String (centroid / 1000.0, centroid >= 10000.0 ? 0 : 1) + " kHz"
                                               : juce::String ((int) std::round (centroid)) + " Hz";
            drawCaption (g, r, "The sound in the window now, for A3", "Centre of brightness " + hz);
            drawSpectrum (g, r, s, sub::testNoteHz, cutoff, true);

            // The same plot area as drawSpectrum's, to put the line in the right place.
            const auto plot = r.withTrimmedLeft (34.0f).withTrimmedBottom (16.0f);
            const float x = getX (plot, juce::jlimit (20.0, 20000.0, centroid));
            g.setColour (secondColour);
            g.fillRect (x - 0.75f, plot.getY(), 1.5f, plot.getHeight());
            const bool left = x > plot.getCentreX();
            drawLabel (g, "Centre of brightness " + hz,
                       { left ? x - 186.0f : x + 6.0f, plot.getY() + 2.0f, 180.0f, 14.0f }, secondColour,
                       left ? juce::Justification::centredRight : juce::Justification::centredLeft);
        } });

    pages.push_back ({ describePageName, "What the AI does, what DawIt checks, and the walkthrough",
        "Describe a sound, at the top of DawIt Subtractive's window, lets you type what you want to hear, such as \"a warm pad that swells in slowly\". A language "
        "model then suggests the knob settings, with a reason for each. It's for getting started on a sound and learning which knobs make it; your ears still decide.\n\n"
        "How it works: DawIt sends the model the synth's 28 knobs with their ranges and usual settings, the starting points with their settings, and your words. "
        "The model runs on your own computer through Ollama, so nothing leaves it. It picks the starting point closest to your words and answers with only the "
        "knobs to change, each with a value and a reason. A follow-up such as \"a bit brighter\" or \"softer start\" changes the current sound and touches only the "
        "knobs it needs.\n\n"
        "What DawIt checks: a language model writes likely text; it doesn't hear sound. So DawIt reads every value itself and keeps it inside the knob's range: a "
        "cutoff of 40 kHz becomes 20 kHz, and the step says so. It leaves out knobs the synth doesn't have, and it measures the result: if you asked for brighter "
        "and the answer is darker on A3, it tells you. It also says when a sound needs something this synth doesn't have: an 808's falling pitch (DawIt Drums), a "
        "bell (DawIt FM), a real guitar or clarinet (DawIt Physical) or a piano (DawIt Sampler).\n\n"
        "The reasons are the model's own words. Most are sensible, but a model can be wrong and sound sure of itself. Hover a knob to read DawIt's own explanation, "
        "and check what you hear.\n\n"
        "Walk me through it shows the changes one knob at a time, in the window's order. It lights the knob, says the value and why, and waits for you to turn it "
        "(close is fine) or to click Set it for me. Back, Skip and Finish all are there too. Set it all now changes every knob at once. Undo in the bar takes back "
        "the last change, whether a whole answer or one step. The app's Undo doesn't cover synth knobs.\n\n"
        "Getting the best from it: describe the sound, not the song (\"a short bright pluck\" rather than \"something for my chorus\"). Ask for one or two changes "
        "per follow-up. Try the same words twice, because each answer can differ. Small models answer faster but miss more often; qwen2.5:7b is a good start.\n\n"
        "What it doesn't do: it doesn't listen to a recording to copy its sound, and it only sets DawIt Subtractive so far.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto& d = processor->lastDescription;
            std::vector<int> parts;

            for (auto& c : d.changes)
            {
                const int part = getFlowPart (c.knob);

                if (std::find (parts.begin(), parts.end(), part) == parts.end())
                    parts.push_back (part);
            }

            const int lit = parts.empty() ? -1 : parts[(size_t) std::fmod (now() / 1.5, (double) parts.size())];
            const auto right = d.isEmpty() ? juce::String() : juce::String (d.changes.size()) + (d.changes.size() == 1 ? " change" : " changes");
            drawCaption (g, r, d.isEmpty() ? juce::String ("Describe a sound to see its changes here")
                                           : "Your last description: \"" + d.words + "\"", right);
            drawFlow (g, r.withTrimmedTop (30.0f), processor->getSettings(), lit);
        },
        {}, {}, {},
        [processor]
        {
            const auto& d = processor->lastDescription;
            const auto arrow = juce::String::fromUTF8 (" \xe2\x86\x92 ");

            if (fxui::isPlugin (*processor))
                return juce::String ("Describe a sound works in the DawIt app, with Ollama on your computer.");

            if (d.isEmpty())
                return juce::String ("Your last description: none yet. Click Describe a sound at the top of DawIt Subtractive's window.");

            auto text = "Your last description: \"" + d.words + "\", from " + d.base + ": " + juce::String (d.changes.size())
                        + (d.changes.size() == 1 ? " change, " : " changes, ") + juce::String (d.made) + " made so far.";

            for (auto& c : d.changes)
            {
                sub::Settings before, after;
                sub::setKnobValue (c.knob, before, c.before);
                sub::setKnobValue (c.knob, after, c.after);
                text += "\n" + getTitle ((int) c.knob) + ": " + sub::formatKnob (c.knob, before) + arrow + sub::formatKnob (c.knob, after);
            }

            for (auto& note : d.notes)
                if (std::none_of (d.changes.begin(), d.changes.end(), [&note] (const auto& c) { return c.note == note; }))
                    text += "\n" + note;

            return text;
        } });

    // The live part follows the explanation, as in the guides that look at your song.
    const auto introduction = pages.back().text;
    auto live = pages.back().liveText;
    pages.back().liveText = [introduction, live] { return introduction + "\n\n" + live(); };

    return pages;
}
} // namespace daw::subtext
