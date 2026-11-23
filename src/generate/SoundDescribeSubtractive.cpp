#include "SoundDescribeSubtractive.h"

#include "engine/BuiltInSubtractive.h"

namespace daw::gen::sounddescribe
{
using sub::Knob;

namespace
{
    Knob knobAt (int i) { return (Knob) i; }

    sub::Settings toSettings (const Values& v)
    {
        sub::Settings s;

        for (int k = 0; k < sub::numKnobs && (size_t) k < v.size(); ++k)
            sub::setKnobValue (knobAt (k), s, v[(size_t) k]);

        return s;
    }

    Values toValues (const sub::Settings& s)
    {
        Values v;

        for (int k = 0; k < sub::numKnobs; ++k)
            v.push_back (sub::getKnobValue (knobAt (k), s));

        return v;
    }

    juce::StringArray getChoiceNames (Knob k)
    {
        juce::StringArray names;

        switch (k)
        {
            case Knob::wave1:
            case Knob::wave2:      for (int i = 0; i < sub::numWaves; ++i) names.add (sub::getWaveName ((sub::Wave) i)); break;
            case Knob::filterType: for (int i = 0; i < sub::numFilterTypes; ++i) names.add (sub::getFilterTypeName ((sub::FilterType) i)); break;
            case Knob::slope:      names.add ("12 dB/oct"); names.add ("24 dB/oct"); break;
            case Knob::lfoTarget:  for (int i = 0; i < sub::numLfoTargets; ++i) names.add (sub::getLfoTargetName ((sub::LfoTarget) i)); break;
            case Knob::mode:       names.add ("Poly"); names.add ("Mono"); break;
            case Knob::width:
            case Knob::pitch2:
            case Knob::detune:
            case Knob::mix:
            case Knob::noise:
            case Knob::cutoff:
            case Knob::resonance:
            case Knob::keyTrack:
            case Knob::envAmount:
            case Knob::filterAttack:
            case Knob::filterDecay:
            case Knob::filterSustain:
            case Knob::filterRelease:
            case Knob::ampAttack:
            case Knob::ampDecay:
            case Knob::ampSustain:
            case Knob::ampRelease:
            case Knob::lfoRate:
            case Knob::lfoDepth:
            case Knob::glide:
            case Knob::velocity:
            case Knob::output:     break;
        }

        return names;
    }

    int readChoiceText (Knob k, const juce::String& text)
    {
        switch (k)
        {
            case Knob::wave1:
            case Knob::wave2:
                if (text.contains ("saw")) return 0;
                if (text.contains ("square") || text.contains ("pulse")) return 1;
                if (text.contains ("tri")) return 2;
                if (text.contains ("sin")) return 3;
                return -1;

            case Knob::filterType:
                if (text.contains ("low")) return 0;
                if (text.contains ("band")) return 1;
                if (text.contains ("high")) return 2;
                return -1;

            case Knob::slope:
                if (text.contains ("24") || text.contains ("steep")) return 1;
                if (text.contains ("12") || text.contains ("gentle")) return 0;
                return -1;

            case Knob::lfoTarget:
                if (text.contains ("pitch") || text.contains ("vibrato")) return 0;
                if (text.contains ("filter") || text.contains ("cutoff")) return 1;
                if (text.contains ("volume") || text.contains ("amp") || text.contains ("tremolo")) return 2;
                if (text.contains ("width") || text.contains ("pwm") || text.contains ("pulse")) return 3;
                return -1;

            case Knob::mode:
                if (text.contains ("mono") || text.contains ("legato")) return 1;
                if (text.contains ("poly")) return 0;
                return -1;

            case Knob::width:
            case Knob::pitch2:
            case Knob::detune:
            case Knob::mix:
            case Knob::noise:
            case Knob::cutoff:
            case Knob::resonance:
            case Knob::keyTrack:
            case Knob::envAmount:
            case Knob::filterAttack:
            case Knob::filterDecay:
            case Knob::filterSustain:
            case Knob::filterRelease:
            case Knob::ampAttack:
            case Knob::ampDecay:
            case Knob::ampSustain:
            case Knob::ampRelease:
            case Knob::lfoRate:
            case Knob::lfoDepth:
            case Knob::glide:
            case Knob::velocity:
            case Knob::output:
                break;
        }

        return -1;
    }

    /** The knob's name with its part of the window: "Oscillator 1 wave", "Filter envelope attack". */
    juce::String getKnobTitle (Knob k)
{
    switch (k)
    {
        case Knob::wave1:         return "Oscillator 1 wave";
        case Knob::width:         return "Pulse width (square only)";
        case Knob::wave2:         return "Oscillator 2 wave";
        case Knob::pitch2:        return "Oscillator 2 pitch";
        case Knob::detune:        return "Oscillator 2 detune";
        case Knob::mix:           return "Oscillator mix (0% oscillator 1 only, 100% oscillator 2 only)";
        case Knob::noise:         return "Noise";
        case Knob::filterType:    return "Filter type";
        case Knob::slope:         return "Filter slope";
        case Knob::cutoff:        return "Filter cutoff";
        case Knob::resonance:     return "Filter resonance";
        case Knob::keyTrack:      return "Filter key tracking";
        case Knob::envAmount:     return "Filter envelope amount";
        case Knob::filterAttack:  return "Filter envelope attack";
        case Knob::filterDecay:   return "Filter envelope decay";
        case Knob::filterSustain: return "Filter envelope sustain";
        case Knob::filterRelease: return "Filter envelope release";
        case Knob::ampAttack:     return "Amp envelope attack";
        case Knob::ampDecay:      return "Amp envelope decay";
        case Knob::ampSustain:    return "Amp envelope sustain";
        case Knob::ampRelease:    return "Amp envelope release";
        case Knob::lfoRate:       return "LFO rate";
        case Knob::lfoDepth:      return "LFO depth";
        case Knob::lfoTarget:     return "LFO target";
        case Knob::glide:         return "Glide time";
        case Knob::mode:          return "Voices";
        case Knob::velocity:      return "Velocity sensitivity";
        case Knob::output:        return "Output level";
    }

    return {};
}

    /** A sound as "id=value" for the knobs that differ from `skipEqualTo` (all when null). */
    juce::String describeValues (const Values& v, const Values* skipEqualTo)
    {
        juce::StringArray parts;
        const auto s = toSettings (v);

        for (int k = 0; k < sub::numKnobs; ++k)
        {
            if (skipEqualTo != nullptr && nearlyEqual (v[(size_t) k], (*skipEqualTo)[(size_t) k]))
                continue;

            parts.add (juce::String (daw::BuiltInSubtractive::getParameterId (knobAt (k))) + "=" + tidy (sub::formatKnob (knobAt (k), s)));
        }

        return parts.joinIntoString (", ");
    }

    juce::StringArray wordNotesFor (const juce::String& words, bool followUp, const sub::Settings& base, const sub::Settings& result)
    {
        juce::StringArray notes;
        const auto w = padWords (words);

        if (followUp)
        {
            const double before = sub::getCentroidHz (base, sub::testNoteHz, true);
            const double after = sub::getCentroidHz (result, sub::testNoteHz, true);

            if (hasAny (w, { "bright", "brighter", "brightness" }) && after <= 0.95 * before)
                notes.add ("You asked for brighter, but on A3 the answer is darker: its centre of brightness moves from "
                           + formatCentroid (before) + " to " + formatCentroid (after) + ". Try Cutoff up, or ask again.");

            if (hasAny (w, { "dark", "darker", "dull", "duller", "muffled" }) && after >= 1.05 * before)
                notes.add ("You asked for darker, but on A3 the answer is brighter (" + formatCentroid (before) + " to "
                           + formatCentroid (after) + "). Try Cutoff down, or ask again.");

            if (hasAny (w, { "softer start", "less attack", "softer attack" }) && result.ampEnv.attack <= base.ampEnv.attack)
                notes.add ("You asked for a softer start, but Amp attack didn't get longer. Attack is a time: a longer attack starts more softly.");
        }

        if (hasAny (w, { "pluck", "plucky", "plucked", "stab" }) && result.ampEnv.sustain > 0.3f)
            notes.add ("A pluck usually fades even while the key is held: try Amp sustain at 0% and Amp decay between 0.2 and 1 s.");

        if (hasAny (w, { "pad", "swell", "swells", "swelling", "fade in", "slow attack" }) && result.ampEnv.attack < 0.1f)
            notes.add ("A pad usually fades in: try Amp attack between 0.3 and 1 s.");

        if (hasWord (w, "bass") && ! hasAny (w, { "chord", "chords" }) && result.mode == sub::Mode::poly)
            notes.add ("A bass usually plays one note at a time: try Voices: Mono, so overlapping notes slide.");

        return notes;
    }

    juce::StringArray limitNotesFor (const juce::String& words)
    {
        juce::StringArray notes;
        const auto w = padWords (words);

        if (hasAny (w, { "808", "kick", "snare", "hihat", "hi-hat", "drum", "drums" }))
            notes.add ("An 808 kick drops in pitch at the start of each note, and DawIt Subtractive has no pitch envelope. "
                       "DawIt Drums makes 808-style drums; for an 808-style bass here, start from Sub bass with a long Amp decay.");

        if (hasAny (w, { "bell", "bells", "chime", "chimes", "glass", "glassy", "marimba", "electric piano" }))
            notes.add ("Bells and chimes have partials that aren't whole-number multiples of the note, and this synth's oscillators make "
                       "whole-number harmonics only. DawIt FM makes bells, electric pianos and marimbas, and DawIt Additive's Stretch makes bells too.");

        if (hasAny (w, { "guitar", "harp", "cello", "violin", "clarinet" }))
            notes.add ("A real guitar, harp, cello or clarinet comes from a vibrating string or tube, and DawIt Physical models those. "
                       "Here you get a synth version.");

        if (hasAny (w, { "orchestra", "orchestral", "choir" }) || hasWord (w.replace (" electric piano ", " "), "piano"))
            notes.add ("For a real piano or orchestra, DawIt Sampler plays recordings of real instruments.");

        if (hasAny (w, { "reverb", "echo", "delay", "distortion", "distorted" }))
            notes.add ("Reverb, echo and distortion are effects: add DawIt Reverb, DawIt Delay or DawIt Distortion after the synth in the track's chain.");

        if (hasWord (w, "chorus"))
            notes.add ("DawIt has no chorus effect. For a chorus-like movement here, try a square with the LFO on Width.");

        if (hasAny (w, { "wide", "stereo", "spread" }))
            notes.add ("DawIt Subtractive plays the same sound in both speakers. Detune makes it sound fuller; a reverb after it adds space.");

        return notes;
    }

    std::vector<Example> getExamples()
    {
        const std::vector<Example> examples
        {
            { "Bass", "bass fat analog low deep thick", "a fat analog bass",
              "Starts from Bass and lets more of the saw through for a fatter sound.",
              { (int) Knob::cutoff, "300 Hz", "A higher cutoff lets more of the saw's upper harmonics through, so the bass sounds fatter." },
              { (int) Knob::filterDecay, "400 ms", "A longer filter decay keeps each note bright for a little longer after it starts." } },
            { "Acid bass", "acid squelch squelchy 303 resonant resonance techno house", "an acid bass with lots of squelch",
              "Starts from Acid bass and pushes the resonance and the filter sweep for more squelch.",
              { (int) Knob::resonance, "85%", "More resonance boosts the frequencies at the cutoff, which makes the squelch stronger." },
              { (int) Knob::envAmount, "+4.0 oct", "A bigger filter envelope sweeps the cutoff further at the start of each note." } },
            { "Sub bass", "sub deep bass low boom clean", "a deep clean sub bass",
              "Starts from Sub bass and darkens it a little so only the low body is left.",
              { (int) Knob::cutoff, "1.20 kHz", "A lower cutoff removes more of the upper harmonics and leaves the low body." },
              { (int) Knob::ampRelease, "200 ms", "A slightly longer release lets each note fade out instead of stopping short." } },
            { "Warm pad", "warm pad soft chords swell swells swelling evolving slow lush", "a warm pad that swells in slowly",
              "Starts from Warm pad and lengthens the attack so each chord fades in.",
              { (int) Knob::ampAttack, "1.20 s", "A longer attack fades each chord in over 1.2 seconds instead of 0.8." },
              { (int) Knob::detune, "14 ct", "A wider detune makes the two saws beat faster, which sounds thicker." } },
            { "Pluck", "pluck short plucky bright arpeggio riff staccato", "a short bright pluck",
              "Starts from Pluck and makes it shorter and a little brighter.",
              { (int) Knob::ampDecay, "400 ms", "A shorter decay makes the note die away faster, which sounds more plucked." },
              { (int) Knob::cutoff, "600 Hz", "A higher starting cutoff makes the pluck brighter." } },
            { "Synth brass", "brass horn horns trumpet fanfare eighties bwaah", "a punchy 80s synth brass",
              "Starts from Synth brass and makes the swell quicker with a sharper edge.",
              { (int) Knob::filterAttack, "40 ms", "A shorter filter attack makes the brass open faster, so it sounds punchier." },
              { (int) Knob::resonance, "20%", "A little more resonance gives the brass a sharper edge." } },
            { "Lead", "lead solo melody cutting vibrato glide", "a cutting solo lead",
              "Starts from Lead and makes it brighter so it cuts through.",
              { (int) Knob::cutoff, "3.50 kHz", "A higher cutoff makes the lead brighter, so it cuts through a mix." },
              { (int) Knob::lfoDepth, "15%", "More vibrato depth makes the pitch wave a little more." } },
            { "String machine", "strings string ensemble orchestra polysynth", "lush synth strings",
              "Starts from String machine and slows the attack so the strings swell in.",
              { (int) Knob::ampAttack, "500 ms", "A slower attack makes the strings swell in like a bowed section." },
              { (int) Knob::detune, "18 ct", "More detune makes the ensemble wider and richer." } },
            { "PWM strings", "pwm pulse moving lush shimmer", "a moving pulse-width sound",
              "Starts from PWM strings and makes the pulse width move faster and further.",
              { (int) Knob::lfoRate, "0.9 Hz", "A faster LFO makes the pulse width change quicker, so the sound moves more." },
              { (int) Knob::lfoDepth, "50%", "A deeper LFO swings the pulse width further." } },
            { "Wobble bass", "wobble dubstep wub wobbly bass lfo", "a dubstep wobble bass",
              "Starts from Wobble bass and speeds up the wobble with more bite.",
              { (int) Knob::lfoRate, "4.0 Hz", "At 120 BPM, 4 Hz gives a wobble on every eighth note." },
              { (int) Knob::resonance, "60%", "More resonance makes the wobble sound sharper." } },
            { "Soft keys", "keys keyboard piano soft gentle electric", "soft electric keys",
              "Starts from Soft keys and makes them a little darker and shorter.",
              { (int) Knob::ampDecay, "900 ms", "A shorter decay lets the note fade a little sooner." },
              { (int) Knob::cutoff, "1.20 kHz", "A lower cutoff takes off some brightness, which sounds softer." } },
            { "Hollow organ", "organ hollow steady church held", "a hollow organ",
              "Starts from Hollow organ and makes it a little darker with a softer ending.",
              { (int) Knob::ampRelease, "150 ms", "A slightly longer release lets each note fade out softly." },
              { (int) Knob::cutoff, "3.00 kHz", "A lower cutoff makes the hollow sound a little darker." } },
            { "Bright stab", "stab bright short chord accent rhythmic", "a bright short chord stab",
              "Starts from Bright stab and makes it brighter and shorter.",
              { (int) Knob::cutoff, "2.00 kHz", "A higher cutoff makes the stab brighter." },
              { (int) Knob::ampDecay, "200 ms", "A shorter decay makes the chord fade away quicker." } },
            { "Tremolo keys", "tremolo pulsing pulse keys", "pulsing keys",
              "Starts from Tremolo keys and makes the pulsing faster and deeper.",
              { (int) Knob::lfoRate, "6.0 Hz", "A faster LFO makes the level rise and fall quicker." },
              { (int) Knob::lfoDepth, "50%", "A deeper LFO makes the level rise and fall further." } },
            { "Airy sweep", "airy breathy breath noise sweep wind", "an airy breathy sweep",
              "Starts from Airy sweep and adds more breath and a slower start.",
              { (int) Knob::noise, "30%", "More noise adds a breathier edge." },
              { (int) Knob::ampAttack, "900 ms", "A longer attack makes the sweep fade in more slowly." } },
            { "Resonant reed", "reed clarinet oboe woody nasal bandpass woodwind", "a reedy woodwind tone",
              "Starts from Resonant reed and makes it more focused and a little brighter.",
              { (int) Knob::resonance, "40%", "More resonance makes the tone more focused." },
              { (int) Knob::cutoff, "1.60 kHz", "A higher cutoff moves the band up, which sounds brighter." } }
        };

        return examples;
    }

    Table buildTable()
    {
        Table t;
        t.synthName = "DawIt Subtractive";

        for (int k = 0; k < sub::numKnobs; ++k)
        {
            const auto knob = knobAt (k);
            KnobDesc d;
            d.id = daw::BuiltInSubtractive::getParameterId (knob);
            d.title = getKnobTitle (knob);
            d.typical = sub::getTypical (k);
            d.info = sub::getKnobInfo (knob);
            d.choices = getChoiceNames (knob);
            d.order = k;
            d.hasOff = ! d.info.isChoice && d.info.min <= 0.0f;
            t.knobs.push_back (d);
        }

        for (auto& p : sub::getPresets())
            t.starts.push_back ({ p.name, p.why, toValues (p.settings), {} });

        t.examples = getExamples();
        t.fallbackExamples = { "Bass", "Warm pad", "Pluck" };

        t.promptHead = juce::String ("You set the knobs of DawIt Subtractive, a subtractive synthesiser, for a beginner who describes a sound in words. "
                                     "You do not hear sound: use what is known about how synthesisers are made.\n\n"
                                     "Rules:\n"
                                     "- Use only the knobs in the table below, named by their id.\n"
                                     "- Give every value with its unit, as the examples show: \"1.80 kHz\", \"60 ms\", \"35%\", \"+7 st\", \"12 ct\", "
                                     "\"+2.5 oct\", \"-3.0 dB\", or for a choice its name (\"Saw\", \"Low-pass\", \"Mono\"). Stay inside each range.\n"
                                     "- Start from the starting point closest to the words and set startFrom to its name. When asked to change the "
                                     "current sound, set startFrom to \"") + currentSoundName
                       + "\".\n- Change as few knobs as the sound needs, at most " + juce::String (maxChanges) + ", and only knobs whose value differs from the starting point.\n"
                         "- Give one sentence per change for a beginner: what they will hear and why.\n"
                         "- If the request needs something this synth lacks (a pitch envelope, stereo spread, effects, bell-like tones whose "
                         "partials are not whole multiples of the note, realistic acoustic instruments), say so in the summary.\n"
                         "- Answer with JSON only, in the form given.\n\n"
                         "Knobs (id | name | range | how it is usually set):\n";

        t.followUpExamples = "Follow-up examples, which change the current sound and touch only the knobs they need:\n"
                             "- \"brighter\", from Warm pad: cutoff 900 Hz to 1.80 kHz. Reason: \"Twice the cutoff, one octave up, lets more of the saws' upper harmonics through.\"\n"
                             "- \"softer start\", from Lead: ampAttack 5.0 ms to 60 ms. Reason: \"A longer attack fades each note in over 60 ms instead of starting with a click.\"\n\n";

        t.format = [] (int k, float value)
        {
            sub::Settings s;
            sub::setKnobValue (knobAt (k), s, value);
            return sub::formatKnob (knobAt (k), s);
        };

        t.readChoice = [] (int k, const juce::String& text) { return readChoiceText (knobAt (k), text); };
        t.describeValues = describeValues;
        t.wordNotes = [] (const juce::String& words, bool followUp, const Values& base, const Values& result)
        {
            return wordNotesFor (words, followUp, toSettings (base), toSettings (result));
        };
        t.limitNotes = [] (const juce::String& words) { return limitNotesFor (words); };
        return t;
    }
}

//==============================================================================
const Table& getSubtractiveTable()
{
    static const Table table = buildTable();
    return table;
}

Values getSubtractiveValues (const sub::Settings& s)
{
    return toValues (s);
}

sub::Settings getSubtractiveSettings (const Values& v)
{
    return toSettings (v);
}
} // namespace daw::gen::sounddescribe
