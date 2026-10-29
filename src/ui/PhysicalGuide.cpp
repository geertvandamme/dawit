#include "PhysicalGuide.h"

namespace daw::phystext
{
using phys::Exciter;
using phys::Knob;
using phys::Resonator;

namespace
{
    constexpr double note = sub::testNoteHz, pi = juce::MathConstants<double>::pi, twoPi = juce::MathConstants<double>::twoPi;
    constexpr float topDb = -10.0f, floorDb = -80.0f;
    constexpr double minRing = 0.01, maxRing = 20.0;

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    juce::String formatDb (double db)
    {
        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    juce::String formatTime (double seconds)
    {
        if (seconds >= 100.0)
            return "over a minute";

        // A hammer's touch is a fraction of a millisecond.
        if (seconds < 0.0095)
            return juce::String (seconds * 1000.0, seconds < 0.001 ? 2 : 1) + " ms";

        return fxui::formatSeconds (seconds);
    }

    juce::String getOrdinal (int n)
    {
        const int lastTwo = n % 100, last = n % 10;
        const char* suffix = (lastTwo >= 11 && lastTwo <= 13) ? "th" : last == 1 ? "st" : last == 2 ? "nd" : last == 3 ? "rd" : "th";
        return juce::String (n) + suffix;
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
    void drawLine (juce::Graphics& g, juce::Rectangle<float> r, const float* values, size_t count, float scale, juce::Colour c, float thickness)
    {
        if (count < 2)
            return;

        juce::Path path;

        for (size_t i = 0; i < count; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / (float) (count - 1);
            const float y = juce::jlimit (r.getY(), r.getBottom(), r.getCentreY() - values[i] * scale);
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }

        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    float getPeak (const float* values, size_t count)
    {
        float peak = 0.0f;

        for (size_t i = 0; i < count; ++i)
            peak = juce::jmax (peak, std::abs (values[i]));

        return peak;
    }

    double now()
    {
        return juce::Time::getMillisecondCounterHiRes() / 1000.0;
    }

    bool isSustained (const phys::Settings& s)
    {
        return s.exciter == Exciter::bow || s.exciter == Exciter::blow;
    }

    /** How long the pictures hold a note before letting it go. */
    double getHold (const phys::Settings& s)
    {
        return isSustained (s) ? BuiltInPhysical::testNoteSeconds : juce::jlimit (0.5, BuiltInPhysical::testNoteSeconds, (double) s.decaySeconds);
    }

    /** One of the starting points, by name. */
    phys::Settings getPreset (const juce::String& name)
    {
        for (auto& p : phys::getPresets())
            if (p.name == name)
                return p.settings;

        return {};
    }

    juce::String getEndName (const phys::Settings& s, bool near)
    {
        if (phys::getResonator (s) == Resonator::tube)
            return near ? (s.exciter == Exciter::blow ? "closed end, with the reed" : "closed end") : "open end";

        return near ? "bridge" : "nut";
    }

    /** "1, 2 and 3". */
    juce::String listNumbers (const juce::Array<int>& numbers)
    {
        juce::StringArray names;

        for (int n : numbers)
            names.add (juce::String (n));

        if (names.size() <= 1)
            return names.joinIntoString ({});

        const auto last = names[names.size() - 1];
        names.remove (names.size() - 1);
        return names.joinIntoString (", ") + " and " + last;
    }
}

//==============================================================================
juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::exciter:      return "Exciter: how it's played";
        case (int) Knob::hardness:     return "Hardness";
        case (int) Knob::position:     return "Position: where it's played";
        case (int) Knob::pressure:     return "Pressure";
        case (int) Knob::noise:        return "Noise";
        case (int) Knob::resonator:    return "Resonator: a string or a tube";
        case (int) Knob::decay:        return "Decay";
        case (int) Knob::damping:      return "Damping";
        case (int) Knob::release:      return "Release";
        case (int) Knob::attack:       return "Attack";
        case (int) Knob::vibratoRate:  return "Vibrato rate";
        case (int) Knob::vibratoDepth: return "Vibrato depth";
        case (int) Knob::glide:        return "Glide";
        case (int) Knob::mode:         return "Voices: poly or mono";
        case (int) Knob::velocity:     return "Velocity";
        case (int) Knob::output:       return "Output";
        case model:                    return "The model";
        case harmonics:                return "The harmonics";
        case rings:                    return "How long each harmonic rings";
        case fxui::overview:           return "DawIt Physical";
        case fxui::presets:            return "Starting points";
        case fxui::clap:               return "Play a note";
        case fxui::listen:             return "Hear the exciter alone";
        case fxui::guide:              return "What is physical modelling?";
        case fxui::history:            return "The last 4 seconds";
        default:                       return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::exciter:
            return "What sets the string or tube going.\n\n"
                   "Pluck: a pick or a finger pulls the string aside and lets go. Here, as in Karplus and Strong's 1983 trick, it's a "
                   "burst of noise one period long, which the string turns into a note within a few hundred milliseconds.\n\n"
                   "Strike: a hammer or mallet hits it, a short smooth push, as in a piano or a dulcimer.\n\n"
                   "Bow: the hair grips the string, drags it along, and it slips back, once a period, for as long as the bow moves.\n\n"
                   "Blow: breath through a reed. The reed opens and closes like a valve, letting puffs of air into a tube in time with "
                   "the wave inside it. A reed always plays a tube.";

        case (int) Knob::hardness:
            return "What touches the string, for a pluck or a strike. A hard pick or hammer makes a sharp start with every harmonic; a "
                   "soft fingertip or felt makes a round start with few highs. For a pluck it filters the burst of noise, from about "
                   "200 Hz for a soft finger to 20 kHz for a hard pick. For a strike it's how long the hammer touches the string: from 5 "
                   "ms for soft felt to 0.15 ms for a hard hammer. A hammer touching for a time T hardly makes partials above about 1/T: "
                   "a 1 ms touch, about 1 kHz.";

        case (int) Knob::position:
            return "Where along the string (or tube) it's played, from the end: 50% is the middle.\n\n"
                   "A string vibrates in many shapes at once, one per harmonic: the 2nd in two halves, the 3rd in thirds... Each has "
                   "points that don't move, nodes. Playing at a node can't set that harmonic going, so it's missing: pluck at 1/5 of the "
                   "string and the 5th, 10th, 15th harmonics are gone; in the middle, every even one. Near the end, almost every harmonic "
                   "is there, but the low ones weakly: a thin, bright sound (a guitar near the bridge). Towards the middle: round and "
                   "hollow (over the soundhole, or a harp).";

        case (int) Knob::pressure:
            return "For the bow: how hard it presses on the string. Too light, and the string slips more than once a period: a thin, "
                   "whistling sound. Firm, and it grips once a period and slips once: the clean, buzzy sawtooth of a bowed string. "
                   "Players balance pressure, speed and where they bow; John Schelleng worked out the limits in 1973.\n\n"
                   "For the breath: how hard the player blows. Below a threshold the reed doesn't move and nothing sounds; just above it "
                   "the note is slow to start and soft; harder and it speaks at once.";

        case (int) Knob::noise:
            return "For the bow and the breath: how unsteady they are. A bow's hair scratches and a player's breath hisses; a little of "
                   "it makes the note sound played rather than machined. A lot is a breathy flute-like tone or a scratchy bow.";

        case (int) Knob::resonator:
            return "What rings. A string is fixed at both ends: every harmonic fits along it, 1, 2, 3, 4...\n\n"
                   "A tube closed at one end and open at the other, like a clarinet, is different: the air can't move at the closed end "
                   "and the pressure can't change at the open end. Only waves with a quarter, three quarters, five quarters... of a wave "
                   "along the tube fit, so only the odd harmonics ring: a hollow sound. And a tube is an octave lower than a string as "
                   "long would be. A reed always plays a tube.";

        case (int) Knob::decay:
            return "How long the note rings while the key is held: the time the fundamental takes to fade 60 dB (T60), the usual measure "
                   "of how long a sound lasts. A real string loses its energy to the air, to the body of the instrument and inside itself; "
                   "here the wave loses a little each trip round the loop. A guitar rings 3 to 8 seconds, a piano's low notes over 20, a "
                   "violin's pluck under one. For a bow or a breath, it's how long it rings once they stop.";

        case (int) Knob::damping:
            return "How much sooner the high partials die than the low ones. In real strings and tubes the losses grow fast with the "
                   "frequency, about with its square: an octave up, four times as fast. That's why a plucked note starts bright and "
                   "mellows as it rings, and why nylon (more damping) sounds rounder than steel.\n\n"
                   "Here a low-pass filter in the loop takes a little of the highs on every trip; set so its loss per second is the same "
                   "on every note, as a real string's is. At 0% every harmonic rings as long as the fundamental: a metallic sound.";

        case (int) Knob::release:
            return "After the key is let go, the note rings at most this long: a finger lifting off a fret, a hand stopping a harp "
                   "string, a damper falling on a piano string. Short for basses and violins; long, or as long as the decay, for a harp "
                   "or a piano with its pedal down. A bow or a breath also stops in this time.";

        case (int) Knob::attack:
            return "For the bow and the breath: how long they take to get going. A bow speeding up or a player starting to blow: a few "
                   "tens of milliseconds for a crisp start, a few tenths of a second for a slow swell.";

        case (int) Knob::vibratoRate:
            return "Vibrato is a regular wobble in pitch, as string players make by rocking a finger and singers and wind players with "
                   "their breath. Rate is how many times a second: about 5 to 7.";

        case (int) Knob::vibratoDepth:
            return "How far the vibrato moves the pitch: at 100% a semitone up and down. A violinist's is about 20 to 30 cents (20-30%), "
                   "a singer's often a little more. Here it changes the loop's length while the note plays, as a finger rolling on a "
                   "string does.";

        case (int) Knob::glide:
            return "Glide (portamento): the pitch slides to the next note in about this time instead of jumping. On a ringing string "
                   "that's a slide guitar: the same string, its length changing. In mono only overlapping (legato) notes slide; in poly "
                   "every new note slides from the last one played.";

        case (int) Knob::mode:
            return "Poly: each note gets its own string (up to 16). Playing a note that still rings plays the same string again, as on a "
                   "real instrument. Mono: one string. A note played while another is held moves the ringing string to the new pitch "
                   "without plucking it again: a hammer-on (or a pull-off going back), and for a bow or breath, a legato change.";

        case (int) Knob::velocity:
            return "How much playing softer makes a note quieter. Softer also sounds softer, as on a real instrument: a soft pluck is a "
                   "darker finger, a soft strike keeps the hammer on the string longer, a soft bow moves slower, soft breath is closer to "
                   "where the reed stops. At 100% a note played half as hard is 6 dB quieter.";

        case (int) Knob::output:
            return "The synth's level going into the track's effects and fader. A plucked note is loudest at its very start; leave "
                   "headroom for chords.";

        case model:
            return "The instrument as the synth models it: the string from the bridge to the nut (or the tube from its closed end to "
                   "its open end), and the exciter where it's played. When a note plays, the string moves with the harmonics the "
                   "position sets going, slowed down so the eye can follow; in a tube the curve is the air's pressure.\n\n"
                   "Drag along the string to move where it's played. Right-click to change the exciter or the resonator.\n\n"
                   "Inside, the string is a delay line: a wave runs from where it's played to one end and back, then to the other end "
                   "and back. One trip round is one period, so the line's length is the pitch.";

        case harmonics:
            return "The harmonics of the test note, A3 (220 Hz), measured from the synth's own sound: each bar is one of the note's "
                   "partials, at 1, 2, 3... times 220 Hz. While a note plays it follows it, so you can watch the highs die first. The "
                   "small rings mark the harmonics the position leaves out; in a tube there are no even ones at all.";

        case rings:
            return "How long each harmonic of A3 rings, to -60 dB, from the loop's losses: the decay for the low ones, shorter and "
                   "shorter as the damping takes the highs. Spaced by ratio, so 0.1 s, 1 s and 10 s are as far apart. The dashed line is "
                   "the most any harmonic rings after the key is let go.";

        case fxui::clap:
            return "Plays the test note, A3, for 1.5 seconds through the synth, even while the song is stopped. Or play the track's MIDI "
                   "notes, or a MIDI keyboard on this track.";

        case fxui::listen:
            return "Takes the string or tube away, so you hear only what the exciter puts in. A pluck is a 4.5 ms burst of noise; a strike "
                   "a click. A bow or a breath on its own is a steady push: almost silent. Everything you hear in between is the "
                   "resonator's doing: it turns a burst of noise into a note.";

        case fxui::presets:
            return "Sounds to start from, each telling why it's made the way it is. Pick one, then change one thing at a time and listen.";

        case fxui::guide:
            return "A guide for beginners: modelling the instrument instead of the sound, a wave running round a loop, Karplus and "
                   "Strong's pluck, where it's played, losses, plucks and strikes, the bow's stick and slip, tubes and reeds, and how "
                   "to build a sound.";

        case fxui::history:
            return "The last 4 seconds: grey what the exciter put in, coloured the whole sound. For a pluck the exciter is a short burst "
                   "and the note rings on long after; for a bow the two run side by side.";

        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::exciter:      return "Pluck for guitars, harps and basses; strike for dulcimers and soft keys; bow for strings; blow for reeds.";
        case (int) Knob::hardness:     return "30-50% for fingers and felt; 70-100% for picks and hard hammers.";
        case (int) Knob::position:     return "10-15% near the bridge for bright plucks; 15-20% for bowing; 20-30% for a fuller pluck; 40-50% for harps.";
        case (int) Knob::pressure:     return "Bow 50-70% for a clean tone; breath 30-70%, lower for soft and airy.";
        case (int) Knob::noise:        return "10-20% for a natural bow or breath; 40-60% for breathy sounds.";
        case (int) Knob::resonator:    return "String for nearly everything; tube for hollow, clarinet-like sounds.";
        case (int) Knob::decay:        return "0.5-1 s for pizzicato; 3-6 s for guitars; 6-20 s for harps, dulcimers and bells.";
        case (int) Knob::damping:      return "20-35% for steel; 50-65% for nylon, gut and bass; 70% and up for dull and woody.";
        case (int) Knob::release:      return "0.1-0.3 s for basses and stopped strings; 1-3 s for harps; as long as the decay to let notes ring.";
        case (int) Knob::attack:       return "0.03-0.08 s for a crisp start; 0.1-0.3 s for a slow bow or a soft breath.";
        case (int) Knob::vibratoRate:  return "5-6 Hz for strings and voice; 4-5 Hz for a slow, wide one.";
        case (int) Knob::vibratoDepth: return "15-30% for strings; 5-15% for a slide or a reed.";
        case (int) Knob::glide:        return "Off for most; 0.03-0.08 s for winds and legato bass; 0.1-0.2 s for slide guitar.";
        case (int) Knob::mode:         return "Mono for basses, slides, winds and leads; poly for guitars, harps and keys.";
        case (int) Knob::velocity:     return "50-70% for plucked and struck sounds; 30-50% for bow and breath.";
        case (int) Knob::output:       return "0 dB for one-note parts; -3 to -6 dB for chords.";
        default:                       return {};
    }
}

double getPeakDb (const phys::Settings& s)
{
    const auto sound = phys::render (s, note, 48000.0, 1.0, 0.8);
    return juce::Decibels::gainToDecibels (getPeak (sound.data(), sound.size()), -100.0f) + s.outputDb;
}

juce::String listMissing (const phys::Settings& s)
{
    if (s.exciter == Exciter::blow)
        return {};

    juce::Array<int> missing;

    for (int n = 1; n <= 24; ++n)
        if (phys::hasHarmonic (s, n) && phys::getPositionLevel (s, n) < 0.08)
            missing.add (n);

    return listNumbers (missing);
}

juce::String describeSettings (int t, const phys::Settings& s)
{
    const auto loop = phys::getLoop (s, note, 48000.0);
    const bool tube = phys::getResonator (s) == Resonator::tube;

    switch (t)
    {
        case (int) Knob::exciter:
        case (int) Knob::hardness:
            switch (s.exciter)
            {
                case Exciter::pluck:  return "A pluck: a burst of noise as long as one period of A3 (4.5 ms), filtered at " + formatHz (phys::getPickHz (s.hardnessPercent))
                                             + (s.hardnessPercent < 45.0f ? ": a soft fingertip." : s.hardnessPercent < 75.0f ? ": a firm finger." : ": a hard pick.");
                case Exciter::strike: return "A strike: the hammer touches the string for " + formatTime (phys::getContactSeconds (s.hardnessPercent))
                                             + ", so it hardly makes partials above about " + formatHz (1.0 / phys::getContactSeconds (s.hardnessPercent)) + ".";
                case Exciter::bow:    return "A bow, at " + formatKnob (Knob::pressure, s) + " pressure. The note lasts as long as the key is held.";
                case Exciter::blow:   return "Breath through a reed, into a tube, at " + formatKnob (Knob::pressure, s) + " pressure. The note lasts as long as the key is held.";
            }

            return {};

        case (int) Knob::position:
        {
            if (s.exciter == Exciter::blow)
                return "A reed always sits at the tube's closed end, so the position doesn't change it.";

            const auto missing = listMissing (s);
            return "Played at " + formatKnob (Knob::position, s) + " of the way from the " + getEndName (s, true) + ": the 1st harmonic at "
                   + juce::String (juce::roundToInt (phys::getPositionLevel (s, 1) * 100.0)) + "% of its strongest"
                   + (missing.isEmpty () ? juce::String (", and none of the first 24 missing.") : ", the " + missing + " missing.");
        }

        case (int) Knob::pressure:
            if (s.exciter == Exciter::blow)
                return "The breath at " + juce::String (phys::getBreath (s.pressurePercent, 1.0f), 2) + " of what shuts the reed, at full velocity; the tube "
                       "starts to sound at about 0.55, so it speaks " + (s.pressurePercent < 30.0f ? "slowly and softly." : "quickly.");

            return s.exciter == Exciter::bow ? (s.pressurePercent < 30.0f ? juce::String ("Light: the string may slip more than once a period, a thinner, glassier tone.")
                                                                           : "Firm: the string sticks to the hair once a period and slips back once, the bowed sawtooth.")
                                             : juce::String ("Only for the bow and the breath.");

        case (int) Knob::noise:
            return ! isSustained (s) ? juce::String ("Only for the bow and the breath.")
                                     : s.noisePercent < 0.5f ? juce::String ("Off: a perfectly steady bow or breath.")
                                                             : "The " + juce::String (s.exciter == Exciter::bow ? "bow's speed" : "breath")
                                                                   + " wobbles randomly by up to " + juce::String (juce::roundToInt (s.noisePercent * (s.exciter == Exciter::bow ? 0.3f : 0.2f)))
                                                                   + "%.";

        case (int) Knob::resonator:
            return (tube ? juce::String ("A tube: only the odd harmonics. ") : juce::String ("A string: every harmonic. "))
                   + "A3 runs round a loop of " + juce::String (loop.length, 1) + " samples (" + formatTime (loop.length / 48000.0) + ") at 48 kHz"
                   + (tube ? ", half a period: the closed end turns the wave over, so it takes two trips to come back the same."
                           : ", one period.");

        case (int) Knob::decay:
        {
            const double first = phys::getRingSeconds (s, note, 1), sixth = phys::getRingSeconds (s, note, tube ? 7 : 6);
            juce::String text = "A3's fundamental rings " + formatTime (first) + ", its " + getOrdinal (tube ? 7 : 6) + " harmonic " + formatTime (sixth) + ".";

            if (first < s.decaySeconds * 0.95)
                text << " The damping is so strong at this note that the fundamental can't ring the whole decay.";

            return text + " Each trip round the loop keeps " + juce::String (loop.gain * 100.0, 2) + "% of the wave, besides the damping.";
        }

        case (int) Knob::damping:
            return s.dampingPercent < 0.5f ? "Off: every harmonic rings as long as the fundamental, " + formatTime (phys::getRingSeconds (s, note, 1)) + "."
                                           : "A partial at 1 kHz fades " + juce::String (juce::roundToInt (phys::getDampingDbPerSecond (s.dampingPercent, 1000.0)))
                                                 + " dB a second from the damping alone, at 2 kHz four times as fast. A3's 2nd harmonic rings "
                                                 + formatTime (phys::getRingSeconds (s, note, tube ? 3 : 2)) + ", its " + getOrdinal (tube ? 9 : 10) + " "
                                                 + formatTime (phys::getRingSeconds (s, note, tube ? 9 : 10)) + ".";

        case (int) Knob::release:
            return "After the key is let go, A3 is gone within " + formatTime (juce::jmin (s.decaySeconds, s.releaseSeconds))
                   + (s.releaseSeconds >= s.decaySeconds ? ": as long as the decay, so notes ring out." : ".");

        case (int) Knob::attack:
            return ! isSustained (s) ? juce::String ("Only for the bow and the breath: a pluck or a strike starts at once.")
                                     : "The " + juce::String (s.exciter == Exciter::bow ? "bow reaches its speed" : "breath reaches its pressure") + " in "
                                           + formatKnob (Knob::attack, s) + ".";

        case (int) Knob::vibratoRate:
        case (int) Knob::vibratoDepth:
            return s.vibratoDepthPercent < 0.5f ? juce::String ("Off: a steady pitch.")
                                                : "The pitch moves " + juce::String (juce::roundToInt (s.vibratoDepthPercent * phys::vibratoSemitones)) + " cents up and down, "
                                                      + juce::String (s.vibratoRateHz, 1) + " times a second.";

        case (int) Knob::glide:
            return s.glideSeconds < 0.0005f ? juce::String ("Off: notes jump straight to their pitch.")
                                            : "Notes slide to their pitch in about " + formatKnob (Knob::glide, s)
                                                  + (s.mode == sub::Mode::mono ? ", when you play them legato." : ", from the last note played.");

        case (int) Knob::mode:
            return s.mode == sub::Mode::mono ? "Mono: one string. Legato notes are hammer-ons: the string moves to the new pitch without starting again."
                                             : "Poly: up to " + juce::String (BuiltInPhysical::numVoices) + " notes at once, each its own string.";

        case (int) Knob::velocity:
            return "A note played half as hard is " + formatDb (juce::Decibels::gainToDecibels (phys::getVelocityScale (s, 0.5f))) + ", and softer-sounding.";

        case (int) Knob::output:
            return "A3 at full velocity peaks at about " + formatDb (getPeakDb (s)) + ".";

        default:
            return {};
    }
}

//==============================================================================
const std::vector<float>& NoteCache::get (const phys::Settings& s, double holdSeconds)
{
    if (! phys::isSame (s, settings) || ! juce::exactlyEqual (hold, holdSeconds) || sound.empty())
    {
        settings = s;
        hold = holdSeconds;
        const double after = juce::jmin (s.decaySeconds, s.releaseSeconds) + 0.3;
        sound = phys::render (s, note, phys::getAnalysisRate (note), juce::jmin (8.0, juce::jmax (0.5, holdSeconds) + after), holdSeconds);
    }

    return sound;
}

//==============================================================================
juce::Rectangle<float> getResonatorArea (juce::Rectangle<float> r)
{
    return r.withTrimmedTop (r.getHeight() * 0.2f).withTrimmedBottom (r.getHeight() * 0.28f).reduced (juce::jmin (44.0f, r.getWidth() * 0.1f), 0.0f);
}

float getPositionAt (juce::Rectangle<float> r, float x)
{
    const auto area = getResonatorArea (r);
    return juce::jlimit (phys::minPosition, phys::maxPosition, (x - area.getX()) / juce::jmax (1.0f, area.getWidth()) * 100.0f);
}

void drawModel (juce::Graphics& g, juce::Rectangle<float> r, const phys::Settings& s, double phase, float level, bool detailed)
{
    const auto area = getResonatorArea (r);
    const bool tube = phys::getResonator (s) == Resonator::tube;
    const float y = area.getCentreY(), swing = area.getHeight() * 0.42f * juce::jlimit (0.0f, 1.0f, level);
    const float px = s.exciter == Exciter::blow ? area.getX() : area.getX() + area.getWidth() * s.positionPercent / 100.0f;

    // The shape: each harmonic the position sets going, in its own time, slowed down.
    constexpr int count = 16;
    std::array<double, count + 1> amps {};
    double total = 0.0;

    for (int n = 1; n <= count; ++n)
    {
        if (! phys::hasHarmonic (s, n))
            continue;

        amps[(size_t) n] = phys::getPositionLevel (s, n) / (s.exciter == Exciter::pluck ? (double) n * n : (double) n);
        total += amps[(size_t) n];
    }

    juce::Path shape;

    for (float x = 0.0f; x <= area.getWidth(); x += 2.0f)
    {
        const double u = x / area.getWidth();
        double d = 0.0;

        for (int n = 1; n <= count; ++n)
            if (amps[(size_t) n] > 0.0)
                d += amps[(size_t) n] * (tube ? std::cos (n * pi * u / 2.0) : std::sin (n * pi * u)) * std::cos (twoPi * n * phase);

        const float py = y - (float) (total > 0.0 ? d / total : 0.0) * swing;
        x <= 0.0f ? shape.startNewSubPath (area.getX() + x, py) : shape.lineTo (area.getX() + x, py);
    }

    if (tube)
    {
        // The tube's walls, closed on the left and open on the right; the curve inside is the air's pressure.
        const float half = area.getHeight() * 0.5f;
        g.setColour (colour.withAlpha (0.12f));
        g.fillRect (area.getX(), y - half, area.getWidth(), half * 2.0f);
        g.setColour (colour);
        g.drawLine (area.getX(), y - half, area.getRight(), y - half, 2.0f);
        g.drawLine (area.getX(), y + half, area.getRight(), y + half, 2.0f);
        g.drawLine (area.getX(), y - half, area.getX(), y + half, 3.0f);
        g.setColour (colour.withAlpha (0.9f));
        g.strokePath (shape, juce::PathStrokeType (1.6f));
    }
    else
    {
        // The bridge (a saddle) on the left and the nut on the right.
        g.setColour (theme::textDim);
        juce::Path saddle;
        saddle.addTriangle (area.getX() - 7.0f, y + 12.0f, area.getX() + 7.0f, y + 12.0f, area.getX(), y);
        g.fillPath (saddle);
        g.fillRect (area.getRight() - 2.0f, y - 10.0f, 5.0f, 20.0f);
        g.setColour (colour);
        g.strokePath (shape, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // The exciter, where it plays.
    g.setColour (exciterColour);
    const float top = area.getY() - 4.0f;

    switch (s.exciter)
    {
        case Exciter::pluck:
        {
            juce::Path pick;
            pick.addTriangle (px - 9.0f, top - 16.0f, px + 9.0f, top - 16.0f, px, top + 2.0f);
            g.fillPath (pick);
            break;
        }

        case Exciter::strike:
            g.fillRect (px - 1.5f, r.getY() + 4.0f, 3.0f, top - r.getY() - 14.0f);
            g.fillEllipse (px - 8.0f, top - 14.0f, 16.0f, 16.0f);
            break;

        case Exciter::bow:
        {
            juce::Path bow;
            bow.addRoundedRectangle (px - 4.0f, area.getY() - 6.0f, 8.0f, area.getHeight() + 12.0f, 3.0f);
            g.fillPath (bow, juce::AffineTransform::rotation (0.12f, px, y));
            break;
        }

        case Exciter::blow:
        {
            const float half = area.getHeight() * 0.5f;
            juce::Path reed;
            reed.addTriangle (area.getX() - 26.0f, y - half * 0.7f, area.getX() - 26.0f, y + half * 0.7f, area.getX() - 2.0f, y);
            g.fillPath (reed);
            juce::Path breath;
            breath.addArrow ({ r.getX() + 2.0f, y, area.getX() - 28.0f, y }, 1.6f, 8.0f, 7.0f);
            g.fillPath (breath);
            break;
        }
    }

    if (! detailed)
        return;

    const auto below = area.getBottom() + 6.0f;
    drawLabel (g, getEndName (s, true), { area.getX() - 30.0f, below, 150.0f, 14.0f });
    drawLabel (g, getEndName (s, false), { area.getRight() - 150.0f, below, 150.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);

    if (s.exciter != Exciter::blow)
    {
        const float dashes[] = { 3.0f, 3.0f };
        g.setColour (exciterColour.withAlpha (0.6f));
        g.drawDashedLine ({ px, area.getY(), px, area.getBottom() }, dashes, 2, 1.0f);
        drawLabel (g, phys::getExciterName (s.exciter) + " at " + formatKnob (Knob::position, s), { px - 60.0f, below + 14.0f, 120.0f, 14.0f }, exciterColour,
                   juce::Justification::centred);

        const auto missing = listMissing (s);
        drawLabel (g, missing.isEmpty() ? juce::String ("no harmonic missing") : "missing: " + missing, { area.getX(), r.getBottom() - 16.0f, area.getWidth(), 14.0f },
                   theme::textDim, juce::Justification::centred);
    }
    else
    {
        drawLabel (g, "only the odd harmonics", { area.getX(), r.getBottom() - 16.0f, area.getWidth(), 14.0f }, theme::textDim, juce::Justification::centred);
    }
}

void drawHarmonics (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<double>& levels, const phys::Settings& s, bool missing)
{
    auto plot = r.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f);
    const int count = juce::jmax (1, (int) levels.size());
    auto toX = [&plot, count] (int n) { return plot.getX() + plot.getWidth() * ((float) n - 0.5f) / (float) count; };
    auto toY = [&plot] (double db) { return juce::jmap ((float) juce::jlimit ((double) floorDb, (double) topDb, db), floorDb, topDb, plot.getBottom(), plot.getY()); };

    for (const double db : { -20.0, -40.0, -60.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
        drawLabel (g, juce::String ((int) db), { r.getX(), toY (db) - 7.0f, 26.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
    }

    g.setColour (theme::grid);
    g.fillRect (plot.getX(), plot.getBottom(), plot.getWidth(), 1.0f);
    const float barW = juce::jlimit (2.0f, 12.0f, plot.getWidth() / (float) count * 0.5f);
    const int labelStep = count > 16 ? 2 : 1;

    for (int n = 1; n <= count; ++n)
    {
        const float x = toX (n);

        if (n == 1 || n % labelStep == 0)
            drawLabel (g, juce::String (n), juce::Rectangle<float> (24.0f, 12.0f).withCentre ({ x, plot.getBottom() + 9.0f }), theme::textDim, juce::Justification::centred);

        if (! phys::hasHarmonic (s, n))
            continue;

        if (missing && phys::getPositionLevel (s, n) < 0.08)
        {
            g.setColour (exciterColour);
            g.drawEllipse (x - 3.5f, plot.getBottom() - 9.0f, 7.0f, 7.0f, 1.2f);
        }

        const double db = juce::Decibels::gainToDecibels (levels[(size_t) n - 1], -200.0);

        if (db <= floorDb)
            continue;

        g.setColour (colour);
        g.fillRect (juce::Rectangle<float> (x - barW * 0.5f, toY (db), barW, plot.getBottom() - toY (db)));
    }
}

void drawRingTimes (juce::Graphics& g, juce::Rectangle<float> r, const phys::Settings& s, int count, bool released)
{
    auto plot = r.withTrimmedLeft (36.0f).withTrimmedBottom (16.0f);
    auto toX = [&plot, count] (int n) { return plot.getX() + plot.getWidth() * ((float) n - 0.5f) / (float) count; };
    auto toY = [&plot] (double seconds)
    {
        const double t = std::log (juce::jlimit (minRing, maxRing, seconds) / minRing) / std::log (maxRing / minRing);
        return plot.getBottom() - plot.getHeight() * (float) t;
    };

    for (const double t : { 0.01, 0.1, 1.0, 10.0 })
    {
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (t), plot.getWidth(), 1.0f);
        drawLabel (g, t < 1.0 ? juce::String (juce::roundToInt (t * 1000.0)) + " ms" : juce::String ((int) t) + " s", { r.getX(), toY (t) - 7.0f, 34.0f, 14.0f },
                   theme::textDim, juce::Justification::centredRight);
    }

    const float barW = juce::jlimit (2.0f, 12.0f, plot.getWidth() / (float) count * 0.5f);
    const int labelStep = count > 16 ? 2 : 1;

    for (int n = 1; n <= count; ++n)
    {
        const float x = toX (n);

        if (n == 1 || n % labelStep == 0)
            drawLabel (g, juce::String (n), juce::Rectangle<float> (24.0f, 12.0f).withCentre ({ x, plot.getBottom() + 9.0f }), theme::textDim, juce::Justification::centred);

        if (! phys::hasHarmonic (s, n))
            continue;

        const double seconds = phys::getRingSeconds (s, note, n);

        if (seconds <= 0.0)
            continue;

        g.setColour (colour.withAlpha (0.85f));
        g.fillRect (juce::Rectangle<float> (x - barW * 0.5f, toY (seconds), barW, plot.getBottom() - toY (seconds)));
    }

    if (released)
    {
        const double after = juce::jmin (s.decaySeconds, s.releaseSeconds);
        const float dashes[] = { 4.0f, 3.0f };
        g.setColour (exciterColour);
        g.drawDashedLine ({ plot.getX(), toY (after), plot.getRight(), toY (after) }, dashes, 2, 1.2f);
        const auto text = "let go: at most " + formatTime (after);
        const float width = juce::GlyphArrangement::getStringWidth (juce::FontOptions (11.0f), text) + 8.0f;
        const juce::Rectangle<float> box { plot.getRight() - width, juce::jmax (plot.getY(), toY (after) - 16.0f), width, 14.0f };
        g.setColour (theme::panel.withAlpha (0.9f));
        g.fillRoundedRectangle (box, 3.0f);
        drawLabel (g, text, box, exciterColour, juce::Justification::centred);
    }
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInPhysical& synth)
{
    BuiltInPhysical* processor = &synth;
    auto cache = std::make_shared<NoteCache>();
    std::vector<fxui::Guide::Page> pages;

    pages.push_back ({ "Modelling the instrument", "What physical modelling is",
        "Every other synth builds a sound from waves: a subtractive synth filters a saw, an additive one adds sines, an FM synth bends "
        "sines with sines. Physical modelling builds the instrument instead: a string, a tube, what plays it, and the laws of how a "
        "wave travels and bounces. Play it, and the sound comes out of the model the way it comes out of the real thing, with its "
        "start, its ring, and the way it changes when played harder or in another place.\n\n"
        "It started with a happy accident. In 1983 Kevin Karplus and Alex Strong published a trick for plucked strings so cheap it ran "
        "on the computers of the day: a short loop of noise, played round and round. Julius O. Smith at Stanford turned the idea into a "
        "general method, digital waveguides, in the 1980s and early 1990s. Yamaha's VL1 (1994) brought it to a keyboard, with wind and "
        "string models you could play with a breath controller, and Korg's Prophecy and Z1 followed. Today it's in Logic's Sculpture "
        "(2004), in pianos such as Modartt's Pianoteq (2006), and in many plugins.\n\n"
        "The picture shows the window's model, moving when a note plays, and a few cycles of its sound.",
        [processor, cache] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const float level = processor->noteAge.load() >= 0.0f ? juce::jlimit (0.15f, 1.0f, processor->shownLevel.load() * 6.0f) : 0.6f;
            drawCaption (g, r, phys::getExciterName (s.exciter) + " on a " + phys::getResonatorName (phys::getResonator (s)).toLowerCase(), "the window's sound");
            drawModel (g, r.removeFromTop (r.getHeight() * 0.58f), s, now() * 0.7, level, true);
            r.removeFromTop (10.0f);

            const auto& sound = cache->get (s, getHold (s));
            const int perCycle = (int) (phys::getAnalysisRate (note) / note);
            const size_t first = (size_t) (0.3 * phys::getAnalysisRate (note)), count = (size_t) perCycle * 4;

            if (sound.size() > first + count)
            {
                g.setColour (theme::text.withAlpha (0.15f));
                g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.0f);
                const float peak = juce::jmax (1.0e-4f, getPeak (sound.data() + first, count));
                drawLine (g, r, sound.data() + first, count, r.getHeight() * 0.45f / peak, colour, 1.6f);
                drawLabel (g, "four cycles of A3, 0.3 s in", r.removeFromBottom (14.0f));
            }
        } });

    pages.push_back ({ "A wave running round a loop", "Why the length of a string is its pitch",
        "Pluck a string and a bump runs along it in both directions. At each end it bounces back, upside down, and runs the other way. "
        "Back where it started, it has been to both ends: one round trip. Then it goes round again, and again, hundreds of times a "
        "second. Each round trip is one period of the note, so a shorter string (or a tighter one, where waves run faster) plays a "
        "higher note.\n\n"
        "That's all a computer needs: a delay line, a memory that gives back each sample a set time later, with its output fed back "
        "into its input, turned over and a little quieter. For A3, 220 times a second, the round trip takes 4.55 ms: 218.2 samples at "
        "48 kHz. Shorten the line and the note goes up.\n\n"
        "It's the same as a delay effect with lots of feedback: echoes 4.55 ms apart run together into a tone. DawIt Delay's guide calls "
        "the short version comb filtering; here it rings on its own.\n\n"
        "The picture shows a pluck at the window's position running round the string, slowed down: one round trip every 4 seconds.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            const double p = juce::jlimit (0.1, 0.5, (double) s.positionPercent / 100.0);
            const double t = std::fmod (now() * 0.5, 2.0);
            drawCaption (g, r, "A round trip, slowed down: " + juce::String (juce::roundToInt (t * 50.0)) + "% of the way round", "A3: 4.55 ms, 218.2 samples at 48 kHz");

            // d'Alembert: two halves of the bump running apart, each bouncing off the ends upside down.
            auto bump = [p] (double x)
            {
                x = std::fmod (x, 2.0);

                if (x < 0.0)
                    x += 2.0;

                const auto shape = [p] (double u) { return std::exp (-std::pow ((u - p) / 0.04, 2.0)); };
                return x <= 1.0 ? shape (x) : -shape (2.0 - x);
            };

            auto top = r.removeFromTop (r.getHeight() * 0.55f).reduced (30.0f, 0.0f);
            const float y = top.getCentreY();
            juce::Path string;

            for (float x = 0.0f; x <= top.getWidth(); x += 2.0f)
            {
                const double u = x / top.getWidth();
                const float d = (float) (0.5 * (bump (u - t) + bump (u + t)));
                const float py = y - d * top.getHeight() * 0.42f;
                x <= 0.0f ? string.startNewSubPath (top.getX() + x, py) : string.lineTo (top.getX() + x, py);
            }

            g.setColour (theme::textDim);
            g.fillRect (top.getX() - 3.0f, y - 10.0f, 4.0f, 20.0f);
            g.fillRect (top.getRight() - 1.0f, y - 10.0f, 4.0f, 20.0f);
            g.setColour (colour);
            g.strokePath (string, juce::PathStrokeType (2.0f));

            // The same as a delay line with its output fed back.
            r.removeFromTop (12.0f);
            auto line = r.withSizeKeepingCentre (r.getWidth() * 0.6f, juce::jmin (40.0f, r.getHeight() * 0.35f));
            g.setColour (colour.withAlpha (0.2f));
            g.fillRoundedRectangle (line, 6.0f);
            g.setColour (colour);
            g.drawRoundedRectangle (line, 6.0f, 1.4f);
            drawLabel (g, "a delay line: 218.2 samples", line, theme::text, juce::Justification::centred, 13.0f);

            const float pulseX = line.getX() + line.getWidth() * (float) (t / 2.0);
            g.setColour (exciterColour);
            g.fillEllipse (pulseX - 4.0f, line.getBottom() - 10.0f, 8.0f, 8.0f);

            juce::Path back;
            back.startNewSubPath (line.getRight(), line.getCentreY());
            back.lineTo (line.getRight() + 30.0f, line.getCentreY());
            back.lineTo (line.getRight() + 30.0f, line.getBottom() + 22.0f);
            back.lineTo (line.getX() - 30.0f, line.getBottom() + 22.0f);
            back.lineTo (line.getX() - 30.0f, line.getCentreY());
            g.setColour (theme::textDim);
            g.strokePath (back, juce::PathStrokeType (1.4f));
            juce::Path arrow;
            arrow.addArrow ({ line.getX() - 30.0f, line.getCentreY(), line.getX() - 2.0f, line.getCentreY() }, 1.4f, 9.0f, 8.0f);
            g.fillPath (arrow);
            drawLabel (g, "the ends turn it over, a little quieter", { line.getX(), line.getBottom() + 24.0f, line.getWidth(), 14.0f }, theme::textDim,
                       juce::Justification::centred);
        } });

    pages.push_back ({ "Karplus-Strong: noise into a note", "The pluck that started it",
        "Karplus and Strong's idea fits in two lines. Fill a delay line one period long with random numbers: noise. Then play it round "
        "and round, and each time a sample comes out, replace it with the average of it and the next one.\n\n"
        "Averaging two neighbours smooths the wave a little: the highs die and the lows stay. And because the loop repeats every "
        "period, whatever survives repeats every period too: a note. So the hiss of the first few milliseconds becomes the bright "
        "twang of a pluck, mellowing as it rings, just as a real string does.\n\n"
        "DawIt Physical's Pluck works the same way, with a filter for how hard the pick is, a loop that can be played anywhere along "
        "its length, and losses you can set. The Karplus-Strong starting point comes closest to the original: raw noise, played at the "
        "very end, with losses only in the highs.\n\n"
        "The picture runs a short loop of noise: its samples after 0, 1, 4, 16, 64 and 256 trips.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "A loop of 96 samples of noise, averaged every trip", "each row: the same loop, later");
            constexpr int size = 96;
            std::array<float, size> loop;
            juce::Random random (11);

            for (auto& v : loop)
                v = random.nextFloat() * 2.0f - 1.0f;

            const int shown[] = { 0, 1, 4, 16, 64, 256 };
            const float rowH = r.getHeight() / 6.0f;
            int done = 0;

            for (int row = 0; row < 6; ++row)
            {
                for (; done < shown[row]; ++done)
                {
                    const float first = loop[0];

                    for (int i = 0; i < size; ++i)
                        loop[(size_t) i] = 0.5f * (loop[(size_t) i] + (i + 1 < size ? loop[(size_t) i + 1] : first));
                }

                auto band = r.removeFromTop (rowH).reduced (0.0f, 3.0f);
                drawLabel (g, shown[row] == 0 ? juce::String ("the noise") : "after " + juce::String (shown[row]) + (shown[row] == 1 ? " trip" : " trips"),
                           band.removeFromLeft (110.0f), theme::text);
                g.setColour (theme::text.withAlpha (0.12f));
                g.fillRect (band.getX(), band.getCentreY(), band.getWidth(), 1.0f);

                // Two periods, so the repeat shows.
                std::array<float, size * 2> twice;

                for (int i = 0; i < size * 2; ++i)
                    twice[(size_t) i] = loop[(size_t) (i % size)];

                drawLine (g, band, twice.data(), twice.size(), band.getHeight() * 0.48f, colour, 1.3f);
            }
        } });

    pages.push_back ({ "Where it's played", "Nodes, and the harmonics they leave out",
        "A string doesn't vibrate in one shape but in many at once: the whole string swinging (the fundamental), two halves swinging "
        "opposite ways (the 2nd harmonic, an octave up), three thirds (the 3rd)... Each shape has points that stay still, nodes: the "
        "2nd harmonic's is in the middle, the 3rd's at a third and two thirds.\n\n"
        "Where you pluck, strike or bow the string decides how much of each shape you start. At a node, none: pluck at the middle and "
        "every even harmonic is missing, a round, hollow sound; at a fifth of the way, the 5th, 10th and 15th are. Near the end, every "
        "harmonic starts, but the low ones only weakly: thin and bright. Guitarists know it as playing near the bridge or over the "
        "soundhole. Piano makers place the hammers at about 1/7 to 1/9 of the string, where the harsh 7th to 9th harmonics are weak.\n\n"
        "The picture shows the first six shapes with the window's position, and how strongly each is set going.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();

            if (s.exciter == Exciter::blow)
                s = phys::Settings();

            const bool tube = phys::getResonator (s) == Resonator::tube;
            drawCaption (g, r, "Played at " + formatKnob (Knob::position, s) + " of the " + (tube ? "tube" : "string"),
                         tube ? "a tube: only the odd shapes" : "drag along the string in the window to move it");
            const float rowH = r.getHeight() / 6.0f;
            auto shapes = r.removeFromLeft (r.getWidth() * 0.7f);
            const float px = shapes.getX() + 60.0f + (shapes.getWidth() - 70.0f) * s.positionPercent / 100.0f;
            int n = 0;

            for (int row = 0; row < 6; ++row)
            {
                do { ++n; } while (! phys::hasHarmonic (s, n));

                auto band = shapes.removeFromTop (rowH).reduced (0.0f, 4.0f);
                auto bars = r.removeFromTop (rowH).reduced (8.0f, 6.0f);
                drawLabel (g, getOrdinal (n), band.removeFromLeft (60.0f), theme::text);
                band.removeFromRight (10.0f);
                const double strength = phys::getPositionLevel (s, n);
                juce::Path curve;

                for (float x = 0.0f; x <= band.getWidth(); x += 2.0f)
                {
                    const double u = x / band.getWidth();
                    const float py = band.getCentreY() - (float) (tube ? std::cos (n * pi * u / 2.0) : std::sin (n * pi * u)) * band.getHeight() * 0.42f;
                    x <= 0.0f ? curve.startNewSubPath (band.getX() + x, py) : curve.lineTo (band.getX() + x, py);
                }

                g.setColour (colour.withAlpha (0.25f + 0.75f * (float) strength));
                g.strokePath (curve, juce::PathStrokeType (1.6f));

                g.setColour (theme::panelLight);
                g.fillRect (bars.withHeight (10.0f).withCentre (bars.getCentre()));
                g.setColour (strength < 0.08 ? exciterColour : colour);
                g.fillRect (bars.withHeight (10.0f).withCentre (bars.getCentre()).withWidth (bars.getWidth() * (float) strength));
                drawLabel (g, strength < 0.08 ? juce::String ("missing") : juce::String (juce::roundToInt (strength * 100.0)) + "%",
                           bars.withTrimmedTop (bars.getHeight() * 0.5f + 5.0f), strength < 0.08 ? exciterColour : theme::textDim);
            }

            g.setColour (exciterColour);
            g.fillRect (px - 1.0f, r.getY() - rowH * 6.0f, 2.0f, rowH * 6.0f);
        } });

    pages.push_back ({ "Losses: the highs die first", "Decay and damping",
        "A real string doesn't ring for ever: it pushes the air, shakes the instrument's body, and rubs inside itself. Each round trip "
        "the wave comes back a little smaller. That's the decay: how long the note takes to fade 60 dB, about to silence.\n\n"
        "The losses aren't the same for every harmonic. They grow fast with the frequency, roughly with its square: a partial an octave "
        "higher fades about four times as fast. So a plucked note starts bright and turns mellow, ending as almost a pure fundamental. "
        "Nylon and gut damp more than steel; a felt-covered hammer or a dead string more still.\n\n"
        "In the model, each trip round the loop goes through a gentle low-pass filter (the damping) and a gain a hair under 1 (the "
        "decay). The filter is set for each note so a partial at 1 kHz fades as fast on a low note as on a high one, as on a real "
        "instrument.\n\n"
        "The picture shows how long each harmonic of A3 rings with the window's settings, and the note's harmonics early, after half a "
        "second and after one and a half.",
        [processor, cache] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "How long each harmonic of A3 rings", "damping " + formatKnob (Knob::damping, s) + ", decay " + formatKnob (Knob::decay, s));
            drawRingTimes (g, r.removeFromTop (r.getHeight() * 0.5f), s, 24, false);
            r.removeFromTop (12.0f);

            const auto& sound = cache->get (s, getHold (s));
            const double times[] = { 0.05, 0.5, 1.5 };
            const float w = r.getWidth() / 3.0f;

            for (int i = 0; i < 3; ++i)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (6.0f, 0.0f);
                drawLabel (g, juce::String (times[i], times[i] < 0.1 ? 2 : 1) + " s in", cell.removeFromTop (14.0f), theme::text);
                drawHarmonics (g, cell, phys::measureHarmonics (sound, note, times[i], 16), s, false);
            }
        } });

    pages.push_back ({ "Pluck and strike", "What touches the string",
        "A plucked or struck note starts with a short push and then rings by itself. What does the pushing shapes the colour.\n\n"
        "A pluck pulls the string aside and lets go. A hard pick lets go suddenly, with every harmonic; a soft fingertip rolls off it, "
        "with few highs. Here the pluck is a burst of noise one period long, filtered as soft as the finger: from 200 Hz to 20 kHz.\n\n"
        "A strike is a hammer bouncing off the string. The time it touches decides the highs: a hammer that touches for a time T hardly "
        "makes partials above about 1/T. Hard wood for 0.2 ms reaches 5 kHz; soft felt for 4 ms barely 250 Hz. Piano hammers are felt "
        "that gets harder the faster it's hit, which is why a loud piano note is brighter, not just louder. Velocity does the same "
        "here: a softer hit touches longer.\n\n"
        "The picture shows what the exciter puts in (the first 12 ms), soft and hard, for a pluck and a strike.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "What goes into the string: the first 12 ms", "what Hear the exciter alone plays");
            struct Case { Exciter exciter; float hardness; const char* name; };
            const Case cases[] = { { Exciter::pluck, 20.0f, "pluck, soft finger" }, { Exciter::pluck, 100.0f, "pluck, hard pick" },
                                   { Exciter::strike, 10.0f, "strike, soft felt" }, { Exciter::strike, 90.0f, "strike, hard hammer" } };
            const float w = r.getWidth() / 2.0f, h = r.getHeight() / 2.0f;

            for (int i = 0; i < 4; ++i)
            {
                phys::Settings s;
                s.exciter = cases[i].exciter;
                s.hardnessPercent = cases[i].hardness;
                const auto sound = phys::renderParts (s, note, 48000.0, 0.012, -1.0, 1.0f, true).exciter;
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 2), r.getY() + h * (float) (i / 2), w, h).reduced (8.0f, 4.0f);
                const auto detail = s.exciter == Exciter::pluck ? "filtered at " + formatHz (phys::getPickHz (s.hardnessPercent))
                                                                : "touching " + formatTime (phys::getContactSeconds (s.hardnessPercent));
                drawLabel (g, juce::String (cases[i].name) + ": " + detail, cell.removeFromTop (16.0f), theme::text, juce::Justification::centredLeft, 12.0f);
                g.setColour (theme::text.withAlpha (0.12f));
                g.fillRect (cell.getX(), cell.getCentreY(), cell.getWidth(), 1.0f);
                const float peak = juce::jmax (1.0e-4f, getPeak (sound.data(), sound.size()));
                drawLine (g, cell, sound.data(), sound.size(), cell.getHeight() * 0.45f / peak, exciterColour, 1.2f);
            }
        } });

    pages.push_back ({ "The bow: stick and slip", "How a bow keeps a string going",
        "A bow doesn't push the string once: it keeps it going for as long as it moves. Rosin makes the hair sticky. The string sticks "
        "to the hair and is carried along at the bow's speed, until it has been pulled so far that it breaks free and slips back fast. "
        "Then it sticks again. Stick, slip, stick, slip: once a period, timed by the wave running round the string.\n\n"
        "Hermann von Helmholtz watched a point on a violin string through a vibration microscope in the 1860s and worked out the "
        "motion: a single sharp bend running round the string, which makes a sawtooth wave at the bridge. That's why a bowed string "
        "sounds buzzy and rich, with every harmonic. In 1983 Michael McIntyre, Robert Schumacher and Jim Woodhouse described the kind "
        "of model used here: the bow as friction that grips when the speeds are close and lets go when they aren't.\n\n"
        "Pressure matters: too little and the string slips more than once a period, a glassy whistle; enough and it's the clean "
        "sawtooth. John Schelleng mapped the limits in 1973: the closer to the bridge, the more pressure it takes.\n\n"
        "The picture plays a bowed A3: the string's speed under the bow and the bow's speed. Where they're together, it sticks.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            auto s = processor->getSettings();
            const bool own = s.exciter == Exciter::bow;

            if (! own)
                s = getPreset ("Bowed cello");

            s.vibratoDepthPercent = 0.0f;
            s.noisePercent = 0.0f;
            drawCaption (g, r, "Under the bow, three periods of A3", own ? "the window's bow, at " + formatKnob (Knob::pressure, s) + " pressure" : "the Bowed cello, as the window doesn't bow");
            const auto parts = phys::renderParts (s, note, 48000.0, 0.62);
            const size_t count = (size_t) (3.0 * 48000.0 / note), first = parts.point.size() - count;
            const float peak = juce::jmax (1.0e-4f, getPeak (parts.point.data() + first, count), getPeak (parts.push.data() + first, count));
            auto plot = r.withTrimmedBottom (18.0f);
            g.setColour (theme::text.withAlpha (0.12f));
            g.fillRect (plot.getX(), plot.getCentreY(), plot.getWidth(), 1.0f);
            drawLine (g, plot, parts.push.data() + first, count, plot.getHeight() * 0.45f / peak, exciterColour.withAlpha (0.8f), 1.4f);
            drawLine (g, plot, parts.point.data() + first, count, plot.getHeight() * 0.45f / peak, colour, 2.0f);
            drawLabel (g, "pink: the bow's speed; gold: the string's speed under it. Flat together: it sticks. The fast swings: it slips.",
                       r.removeFromBottom (14.0f));
        } });

    pages.push_back ({ "Tubes and reeds", "Why a clarinet is hollow",
        "In a wind instrument the air in a tube does what the string does: a pressure wave runs down the tube and back. The ends "
        "decide which harmonics fit. A tube open at both ends, like a flute, holds every harmonic, as a string does. A tube closed at "
        "one end, like a clarinet (the player's mouth and the reed close it), holds only the odd ones: the pressure must swing fully at "
        "the closed end and not at all at the open end, so only a quarter of a wave, three quarters, five quarters... fit. That's the "
        "hollow, woody sound of a clarinet's low notes, and why it's an octave lower than a flute as long. Blow harder and it jumps "
        "up a twelfth (the 3rd harmonic), not an octave.\n\n"
        "The reed is the exciter. It's a valve: the breath pushes it shut, the wave coming back up the tube pushes it open again, "
        "once a period. Below a threshold of breath it doesn't move at all; just above it the note starts slowly and softly.\n\n"
        "The picture shows the same pluck on a string and on a tube, and the first three shapes that fit in a closed tube.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The same pluck on a string and on a tube", "a tube: only 1, 3, 5...");
            auto top = r.removeFromTop (r.getHeight() * 0.55f);
            auto left = top.removeFromLeft (top.getWidth() * 0.5f).reduced (6.0f, 0.0f);
            phys::Settings string, tube;
            string.positionPercent = tube.positionPercent = 3.0f;
            tube.resonator = Resonator::tube;
            drawLabel (g, "string", left.removeFromTop (14.0f), theme::text);
            drawHarmonics (g, left, phys::getHarmonics (string, note, 0.2, 12), string, false);
            top.reduce (6.0f, 0.0f);
            drawLabel (g, "tube, closed at one end", top.removeFromTop (14.0f), theme::text);
            drawHarmonics (g, top, phys::getHarmonics (tube, note, 0.2, 12), tube, false);

            r.removeFromTop (12.0f);
            auto pipe = r.reduced (40.0f, 4.0f);
            const float half = pipe.getHeight() * 0.42f, y = pipe.getCentreY();
            g.setColour (colour);
            g.drawLine (pipe.getX(), y - half, pipe.getRight(), y - half, 2.0f);
            g.drawLine (pipe.getX(), y + half, pipe.getRight(), y + half, 2.0f);
            g.drawLine (pipe.getX(), y - half, pipe.getX(), y + half, 3.0f);
            const juce::Colour shades[] = { colour, exciterColour, vibratoColour };

            for (int k = 0; k < 3; ++k)
            {
                const int n = 2 * k + 1;
                juce::Path wave;

                for (float x = 0.0f; x <= pipe.getWidth(); x += 2.0f)
                {
                    const float py = y - (float) std::cos (n * pi * x / pipe.getWidth() / 2.0) * half * 0.85f;
                    x <= 0.0f ? wave.startNewSubPath (pipe.getX() + x, py) : wave.lineTo (pipe.getX() + x, py);
                }

                g.setColour (shades[k].withAlpha (0.85f));
                g.strokePath (wave, juce::PathStrokeType (1.4f));
                drawLabel (g, getOrdinal (n), { pipe.getRight() + 4.0f, y - half + 14.0f * (float) k, 36.0f, 13.0f }, shades[k]);
            }

            drawLabel (g, "closed", { pipe.getX() - 40.0f, y - 7.0f, 38.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
        } });

    pages.push_back ({ "Shaping the note", "Release, vibrato, glide and velocity",
        "A model already has its own start and ring, so the knobs left are about playing it.\n\n"
        "Release: what happens when the key comes up. A guitarist lifts a finger and the string stops at once; a harpist lets it ring. "
        "The note rings at most the release time after the key is let go.\n\n"
        "Vibrato: a finger rocking on the string, or a wavering breath, moves the pitch a few cents, five or six times a second. Here "
        "it changes the loop's length while the note plays.\n\n"
        "Glide and mono: in mono there's one string. A note played while another is held moves the ringing string to the new pitch "
        "without plucking it again, a guitarist's hammer-on. With glide it slides there: a slide guitar.\n\n"
        "Velocity: softer is quieter and softer-sounding, a darker finger, a longer touch of the hammer, a slower bow.\n\n"
        "The picture shows the window's note over time, held and let go.",
        [processor, cache] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double hold = getHold (s);
            const auto& sound = cache->get (s, hold);
            const double sampleRate = phys::getAnalysisRate (note), seconds = sound.size() / sampleRate;
            drawCaption (g, r, "The window's A3 over time, in dB", "held " + formatTime (hold) + ", then let go");
            auto plot = r.withTrimmedBottom (14.0f).withTrimmedLeft (30.0f);
            auto toY = [&plot] (double db) { return juce::jmap ((float) juce::jlimit (-70.0, -10.0, db), -70.0f, -10.0f, plot.getBottom(), plot.getY()); };

            for (const double db : { -20.0, -40.0, -60.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
                drawLabel (g, juce::String ((int) db), { r.getX(), toY (db) - 7.0f, 26.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
            }

            juce::Path level;
            const int step = juce::jmax (1, (int) (sampleRate * 0.01));

            for (int i = 0; i + step <= (int) sound.size(); i += step)
            {
                const float x = plot.getX() + plot.getWidth() * (float) (i / sampleRate / seconds);
                const float y = toY (juce::Decibels::gainToDecibels (getPeak (sound.data() + i, (size_t) step), -100.0f));
                i == 0 ? level.startNewSubPath (x, y) : level.lineTo (x, y);
            }

            g.setColour (colour);
            g.strokePath (level, juce::PathStrokeType (1.8f));
            const float upX = plot.getX() + plot.getWidth() * (float) (hold / seconds);
            const float dashes[] = { 4.0f, 3.0f };
            g.setColour (theme::textDim);
            g.drawDashedLine ({ upX, plot.getY(), upX, plot.getBottom() }, dashes, 2, 1.0f);
            drawLabel (g, "key up", { upX + 4.0f, plot.getY(), 60.0f, 13.0f });
            drawLabel (g, "0 s", { plot.getX(), plot.getBottom() + 1.0f, 40.0f, 13.0f });
            drawLabel (g, formatTime (seconds), { plot.getRight() - 60.0f, plot.getBottom() + 1.0f, 60.0f, 13.0f }, theme::textDim, juce::Justification::centredRight);
        } });

    pages.push_back ({ "Building a sound, step by step", "From Init to your own instrument",
        "Start from Init, a plucked string, and change one thing at a time, playing a note or a phrase over and over.\n\n"
        "1. How it's played: pluck, strike, bow or blow. This decides whether the note dies away or lasts as long as the key.\n"
        "2. What plays it: the hardness of the finger, pick or hammer, or the pressure of the bow or breath.\n"
        "3. Where: the position. Near the end for thin and bright, towards the middle for round and hollow.\n"
        "4. What rings: a string, or a tube for the hollow odd harmonics.\n"
        "5. How long: the decay, and the damping for how fast the highs go. Most of an instrument's character is here: steel, nylon, "
        "wood, felt.\n"
        "6. Playing: release, vibrato, glide, mono for hammer-ons and slides, velocity.\n\n"
        "When it isn't right: too dull? Less damping, a harder pick, or play nearer the end. Too thin? Move towards the middle. Too "
        "metallic or ringing? More damping, a shorter decay. A bow that whistles? More pressure. A reed slow to speak? More breath, or "
        "a shorter attack.\n\n"
        "The picture shows the window's model with each step's part lit in turn.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const int step = (int) std::fmod (now() / 2.5, 5.0);
            const juce::StringArray names { "1-2. the exciter: " + phys::getExciterName (s.exciter).toLowerCase(),
                                            "3. where: " + formatKnob (Knob::position, s), "4. what rings: a " + phys::getResonatorName (phys::getResonator (s)).toLowerCase(),
                                            "5. how long: decay " + formatKnob (Knob::decay, s) + ", damping " + formatKnob (Knob::damping, s),
                                            "6. playing: release " + formatKnob (Knob::release, s) + ", " + formatKnob (Knob::mode, s).toLowerCase() };
            drawCaption (g, r, "Building up the window's sound", names[step]);
            auto modelArea = r.removeFromTop (r.getHeight() * (step == 4 ? 0.6f : 1.0f));
            drawModel (g, modelArea, s, now() * 0.7, 0.7f, true);

            const auto area = getResonatorArea (modelArea);
            g.setColour (exciterColour.withAlpha (0.25f));

            if (step == 0)
                g.fillEllipse (juce::Rectangle<float> (44.0f, 44.0f).withCentre ({ s.exciter == Exciter::blow ? area.getX() - 14.0f
                                                                                                             : area.getX() + area.getWidth() * s.positionPercent / 100.0f,
                                                                                 area.getY() - 12.0f }));
            else if (step == 1)
                g.fillRect (area.getX(), area.getY() - 4.0f, area.getWidth() * s.positionPercent / 100.0f, 4.0f);
            else if (step == 2 || step == 3)
                g.drawRoundedRectangle (area.expanded (8.0f), 8.0f, 3.0f);

            if (step == 4)
            {
                r.removeFromTop (8.0f);
                drawRingTimes (g, r, s, 16, true);
            }
        } });

    pages.push_back ({ "Recipes for common sounds", "Guitar, harp, bass, dulcimer, cello, clarinet",
        "Every one of these is in the starting points, with notes on the knobs that matter. Load one, play it, and change one thing at "
        "a time.\n\n"
        "Nylon guitar: a soft finger (35%) at 18%, damping 55%, 4 s. Steel-string: a hard pick (85%) near the bridge (12%), damping "
        "30%, 6 s.\n\n"
        "Harp: plucked near the middle (45%) for its round, hollow tone; a long release, as harp strings ring on.\n\n"
        "Bass: a soft finger near the bridge, damping 60%, a short release; mono, so legato notes are hammer-ons. Slide guitar: mono "
        "with a 0.15 s glide.\n\n"
        "Hammered dulcimer: a hard strike at 12%, ringing 6 s. Felt hammer: a soft strike, dark and gentle.\n\n"
        "Cello: a bow at 18% with firm pressure (60%), a slow attack, a little noise and vibrato. Play it low.\n\n"
        "Clarinet: a reed, 55% breath, a little noise, mono. Breathy reed: barely any breath and lots of noise.\n\n"
        "The picture shows the window's sound: its model and its harmonics now.",
        [processor, cache] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double age = processor->noteAge.load();
            drawCaption (g, r, "The window's sound", "play a note to see it move");
            auto left = r.removeFromLeft (r.getWidth() * 0.45f);
            const float level = age >= 0.0 ? juce::jlimit (0.15f, 1.0f, processor->shownLevel.load() * 6.0f) : 0.6f;
            drawModel (g, left, s, now() * 0.7, level, false);
            r.removeFromLeft (16.0f);
            const double hold = processor->releaseAge.load() >= 0.0f ? (double) processor->releaseAge.load() : getHold (s);
            drawHarmonics (g, r, phys::measureHarmonics (cache->get (s, hold), note, age >= 0.0 ? age : 0.1, 24), s, true);
        } });

    return pages;
}
} // namespace daw::phystext
