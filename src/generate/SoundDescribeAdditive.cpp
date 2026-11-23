#include "SoundDescribeAdditive.h"

namespace daw::gen::sounddescribe
{
using additive::Knob;

namespace
{
    constexpr int firstBar = additive::firstBarIndex;
    constexpr int allControls = additive::numControls;

    additive::Settings toSettings (const Values& v)
    {
        additive::Settings s;

        for (int k = 0; k < allControls && (size_t) k < v.size(); ++k)
            additive::setKnobValue (k, s, v[(size_t) k]);

        return s;
    }

    Values toValues (const additive::Settings& s)
    {
        Values v;

        for (int k = 0; k < allControls; ++k)
            v.push_back (additive::getKnobValue (k, s));

        return v;
    }

    /** The knob's name with its direction, as the prompt shows it. */
    juce::String getKnobTitle (Knob k)
    {
        switch (k)
        {
            case Knob::brightness: return "Brightness: tilts all harmonics; up (+) is brighter, down (-) is darker";
            case Knob::even:       return "Even: how much of the even harmonics (2nd, 4th, 6th...) there is; 0% leaves only the odd ones, a hollow sound";
            case Knob::stretch:    return "Stretch: pulls the harmonics sharp; more is more metallic and bell-like";
            case Knob::phases:     return "Phases: Aligned or Spread; changes the wave's shape, hardly the colour";
            case Knob::swell:      return "Swell: the high harmonics come in over this time after a note starts";
            case Knob::fade:       return "Fade: the high harmonics die away sooner than the low ones; Off for held sounds";
            case Knob::ampAttack:  return "Amp envelope attack: how long a note takes to reach full level";
            case Knob::ampDecay:   return "Amp envelope decay: how long it takes to fall to the sustain level";
            case Knob::ampSustain: return "Amp envelope sustain: the level while the key is held";
            case Knob::ampRelease: return "Amp envelope release: how long a note fades after the key is let go";
            case Knob::lfoRate:    return "LFO rate";
            case Knob::lfoDepth:   return "LFO depth";
            case Knob::lfoTarget:  return "LFO target (Pitch, Brightness or Even)";
            case Knob::glide:      return "Glide time";
            case Knob::mode:       return "Voices (Poly or Mono)";
            case Knob::velocity:   return "Velocity sensitivity";
            case Knob::output:     return "Output level";
        }

        return {};
    }

    int getOrder (Knob k)
    {
        switch (k)
        {
            case Knob::brightness: return 1;
            case Knob::even:       return 2;
            case Knob::stretch:    return 3;
            case Knob::phases:     return 4;
            case Knob::swell:      return 5;
            case Knob::fade:       return 6;
            case Knob::lfoRate:    return 7;
            case Knob::lfoDepth:   return 8;
            case Knob::lfoTarget:  return 9;
            case Knob::ampAttack:  return 10;
            case Knob::ampDecay:   return 11;
            case Knob::ampSustain: return 12;
            case Knob::ampRelease: return 13;
            case Knob::glide:      return 14;
            case Knob::mode:       return 15;
            case Knob::velocity:   return 16;
            case Knob::output:     return 17;
        }

        return 99;
    }

    juce::StringArray getChoiceNames (Knob k)
    {
        juce::StringArray names;

        if (k == Knob::phases)
            names = { "Aligned", "Spread" };
        else if (k == Knob::lfoTarget)
            for (int i = 0; i < additive::numLfoTargets; ++i)
                names.add (additive::getLfoTargetName ((additive::LfoTarget) i));
        else if (k == Knob::mode)
            names = { "Poly", "Mono" };

        return names;
    }

    int readChoiceText (int k, const juce::String& text)
    {
        switch ((Knob) k)
        {
            case Knob::phases:
                if (text.contains ("spread") || text.contains ("schroeder")) return 1;
                if (text.contains ("align")) return 0;
                return -1;

            case Knob::lfoTarget:
                if (text.contains ("pitch") || text.contains ("vibrato")) return 0;
                if (text.contains ("bright")) return 1;
                if (text.contains ("even")) return 2;
                return -1;

            case Knob::mode:
                if (text.contains ("mono") || text.contains ("legato")) return 1;
                if (text.contains ("poly")) return 0;
                return -1;

            case Knob::brightness:
            case Knob::even:
            case Knob::stretch:
            case Knob::swell:
            case Knob::fade:
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

    juce::String formatControl (int k, float value)
    {
        if (k >= firstBar)
            return additive::formatLevel (value);

        additive::Settings s;
        additive::setKnobValue (k, s, value);
        return additive::formatKnob ((Knob) k, s);
    }

    bool isBarOn (float db)
    {
        return db > additive::offDb + 0.05f;
    }

    /** The shape the bars make, if they equal one of the five within 0.05 dB. */
    juce::String getShapeText (const additive::Settings& s)
    {
        for (int i = 0; i < additive::numShapes; ++i)
        {
            const auto levels = additive::getShapeLevels ((additive::Shape) i);
            bool same = true;

            for (int n = 0; n < additive::numPartials && same; ++n)
                same = std::abs (levels[(size_t) n] - s.levelsDb[(size_t) n]) <= 0.05f;

            if (same)
                return additive::getShapeName ((additive::Shape) i) + " shape";
        }

        return {};
    }

    juce::String describeValues (const Values& v, const Values* skipEqualTo)
    {
        const auto s = toSettings (v);
        juce::StringArray parts;

        for (int k = 0; k < additive::numKnobs; ++k)
        {
            if (skipEqualTo != nullptr && nearlyEqual (v[(size_t) k], (*skipEqualTo)[(size_t) k]))
                continue;

            parts.add (juce::String (daw::BuiltInAdditive::getParameterId ((Knob) k)) + "=" + tidy (formatControl (k, v[(size_t) k])));
        }

        bool barsSame = skipEqualTo != nullptr;

        for (int k = firstBar; k < allControls && barsSame; ++k)
            barsSame = nearlyEqual (v[(size_t) k], (*skipEqualTo)[(size_t) k]);

        auto line = parts.joinIntoString (", ");

        if (! barsSame)
            line += juce::String (line.isEmpty() ? "" : "; ") + "bars: " + describeAdditiveBars (s);

        return line;
    }

    //==============================================================================
    juce::StringArray wordNotesFor (const juce::String& words, bool followUp, const Values& baseValues, const Values& resultValues)
    {
        juce::StringArray notes;
        const auto w = padWords (words);
        const auto base = toSettings (baseValues), result = toSettings (resultValues);

        if (followUp)
        {
            const double before = additive::getCentroidHz (base, sub::testNoteHz);
            const double after = additive::getCentroidHz (result, sub::testNoteHz);

            if (hasAny (w, { "bright", "brighter", "brightness" }) && after <= 0.95 * before)
                notes.add ("You asked for brighter, but on A3 the answer is darker: its centre of brightness moves from "
                           + formatCentroid (before) + " to " + formatCentroid (after) + ". Try Brightness up, or taller high bars, or ask again.");

            if (hasAny (w, { "dark", "darker", "dull", "duller", "muffled" }) && after >= 1.05 * before)
                notes.add ("You asked for darker, but on A3 the answer is brighter (" + formatCentroid (before) + " to "
                           + formatCentroid (after) + "). Try Brightness down, or ask again.");

            if (hasAny (w, { "softer start", "less attack", "softer attack" }) && result.ampEnv.attack <= base.ampEnv.attack)
                notes.add ("You asked for a softer start, but Amp attack didn't get longer. Attack is a time: a longer attack starts more softly.");
        }

        if (hasAny (w, { "pluck", "plucky", "plucked", "stab" }) && result.ampEnv.sustain > 0.3f)
            notes.add ("A pluck usually fades even while the key is held: try Amp sustain at 0%, Decay between 1 and 3 s, and Fade between 0.2 and 0.5 s so the high harmonics go first.");

        if (hasAny (w, { "pad", "pads", "fade in", "fades in", "slow attack" }) && result.ampEnv.attack < 0.1f)
            notes.add ("A pad usually fades in: try Amp attack between 0.3 and 1 s.");

        if (hasWord (w, "bass") && ! hasAny (w, { "chord", "chords" }) && result.mode == sub::Mode::poly)
            notes.add ("A bass usually plays one note at a time: try Voices: Mono, so overlapping notes slide.");

        if (hasAny (w, { "hollow", "woody", "reedy", "clarinet" }) && result.evenPercent > 20.0f)
        {
            bool evenOn = false;

            for (int n = 2; n <= additive::numPartials; n += 2)
                evenOn = evenOn || isBarOn (result.levelsDb[(size_t) n - 1]);

            if (evenOn)
                notes.add ("A hollow sound comes from the odd harmonics: try Even at 0 to 20%, so the 2nd, 4th, 6th... are quiet.");
        }

        if (hasAny (w, { "metal", "metallic", "bell", "bells", "chime", "chimes", "gong" }) && result.stretchPercent < 50.0f)
            notes.add ("Bells and metal have partials that aren't whole-number multiples of the note: try Stretch between 60 and 100%.");

        return notes;
    }

    juce::StringArray limitNotesFor (const juce::String& words)
    {
        juce::StringArray notes;
        const auto w = padWords (words);

        if (hasAny (w, { "808", "kick", "snare", "hihat", "hi-hat", "drum", "drums" }))
            notes.add ("An 808 kick drops in pitch at the start of each note, and DawIt Additive has no pitch envelope. "
                       "DawIt Drums makes 808-style drums; for a deep bass here, start from Sub bass.");

        if (hasAny (w, { "noise", "breath", "breathy", "airy", "air", "whisper" }))
            notes.add ("DawIt Additive makes only harmonics, so it has no noise for breath or air. DawIt Subtractive has a Noise knob; "
                       "here, a slower attack and a softer top sound gentler.");

        if (hasAny (w, { "squelch", "squelchy", "acid", "resonance", "resonant", "303" }))
            notes.add ("DawIt Additive has no filter, so there is no resonance to squelch. DawIt Subtractive makes acid sounds; "
                       "here, the LFO on Brightness moves the colour.");

        if (hasAny (w, { "fat", "phat", "thick", "detune", "detuned", "supersaw", "unison" }))
            notes.add ("DawIt Additive has one set of harmonics per note, with no second oscillator to detune. DawIt Subtractive's Detune makes a fat sound; "
                       "here, stronger low bars (the 2nd to 4th) make it fuller.");

        if (hasAny (w, { "guitar", "harp", "cello", "violin", "clarinet" }))
            notes.add ("A real guitar, harp, cello or clarinet comes from a vibrating string or tube, and DawIt Physical models those. "
                       "Here you get an additive version.");

        if (hasAny (w, { "orchestra", "orchestral" }) || hasWord (w.replace (" electric piano ", " "), "piano"))
            notes.add ("For a real piano or orchestra, DawIt Sampler plays recordings of real instruments.");

        if (hasAny (w, { "reverb", "echo", "delay", "distortion", "distorted" }))
            notes.add ("Reverb, echo and distortion are effects: add DawIt Reverb, DawIt Delay or DawIt Distortion after the synth in the track's chain.");

        if (hasWord (w, "chorus"))
            notes.add ("DawIt has no chorus effect, and DawIt Additive has no second oscillator to detune. DawIt Subtractive's Detune gives a similar thickness.");

        if (hasAny (w, { "wide", "stereo", "spread" }))
            notes.add ("DawIt Additive plays the same sound in both speakers; Phases: Spread changes the wave's shape, not its width. A reverb after it adds space.");

        return notes;
    }

    //==============================================================================
    std::vector<Example> getExamples()
    {
        const auto bar = [] (int n) { return additive::firstBarIndex + n - 1; };
        const auto k = [] (Knob knob) { return (int) knob; };

        return {
            { "Drawbar organ", "organ jazz drawbar hammond church bright top", "a jazz organ with a bright top",
              "Starts from Drawbar organ and pulls the 8th harmonic up for a brighter top, with a slight wobble.",
              { bar (8), "0 dB", "Raising the 8th harmonic to full level adds a bright, airy top to the organ." },
              { k (Knob::lfoDepth), "6%", "A little LFO depth makes the pitch waver slightly, as a rotary speaker does." } },
            { "Hollow reed", "hollow woody clarinet reed reedy oboe woodwind", "a hollow woody clarinet",
              "Starts from Hollow reed and softens the top for a rounder, woodier tone.",
              { k (Knob::brightness), "-4.0 dB/oct", "A lower Brightness turns down the high harmonics, which sounds rounder and woodier." },
              { k (Knob::ampAttack), "50 ms", "A 50 ms attack lets each note start softly, as a breath into a reed does." } },
            { "Metal bell", "bell bells metallic metal chime gong bright ring", "a bright metallic bell",
              "Starts from Metal bell and lets it ring longer and brighter.",
              { k (Knob::fade), "2.00 s", "A longer Fade keeps the high harmonics ringing for about two seconds before they die away." },
              { k (Knob::brightness), "+2.0 dB/oct", "A little more Brightness lifts the high harmonics, which sounds brighter." } },
            { "Plucked string", "pluck plucked harp guitar soft string short", "a soft harp pluck",
              "Starts from Plucked string and makes the high harmonics die away faster for a softer sound.",
              { k (Knob::fade), "150 ms", "A shorter Fade makes the high harmonics die away within 150 ms, so the pluck sounds softer and rounder." },
              { k (Knob::brightness), "-5.0 dB/oct", "A lower Brightness takes the edge off the high harmonics." } },
            { "Sub bass", "sub bass deep clean low boom", "a deep clean sub bass",
              "Starts from Sub bass and leaves only the note and a little of the 2nd harmonic.",
              { bar (3), "Off", "Turning the 3rd harmonic off leaves a cleaner, deeper sound." },
              { k (Knob::ampRelease), "150 ms", "A slightly longer release lets each note fade out instead of stopping short." } },
            { "Shimmer pad", "pad slowly shifting evolving moving slow shimmer", "a slowly shifting pad",
              "Starts from Shimmer pad and slows the colour change down so the pad drifts.",
              { k (Knob::lfoRate), "0.10 Hz", "A slower LFO takes ten seconds for one change in colour." },
              { k (Knob::ampAttack), "1.50 s", "A longer attack fades each chord in over a second and a half." } },
            { "Shimmer pad", "warm soft pad lush mellow", "a warm soft pad",
              "Starts from Shimmer pad and softens the top so it sounds warmer.",
              { k (Knob::brightness), "-4.0 dB/oct", "A lower Brightness turns down the high harmonics, which sounds warmer." },
              { k (Knob::lfoDepth), "50%", "A smaller LFO depth keeps the change in colour gentle." } },
            { "Brass swell", "brass stab punchy horn trumpet fanfare swell", "a punchy brass stab",
              "Starts from Brass swell and makes it quicker and shorter for a punchy stab.",
              { k (Knob::swell), "80 ms", "A shorter Swell brings the high harmonics in faster, so the stab sounds punchier." },
              { k (Knob::ampSustain), "60%", "A lower sustain lets the note drop back after the first hit, like a stab." } },
            { "Flute", "flute breathy airy soft wind", "a breathy soft flute",
              "Starts from Flute. DawIt Additive has no noise for breath, so this softens the start and the tone instead.",
              { k (Knob::ampAttack), "150 ms", "A slower attack imitates a breath starting the note." },
              { bar (2), "-12 dB", "A quieter 2nd harmonic makes the tone softer and closer to a sine." } },
            { "Electric keys", "electric piano keys mellow rhodes", "mellow electric piano",
              "Starts from Electric keys and takes the sharp edge off the tine.",
              { bar (8), "-30 dB", "A quieter 8th harmonic takes the sharp, bell-like edge off the tine." },
              { k (Knob::velocity), "50%", "At 50% velocity, softer notes are quieter and duller, as on a real keyboard." } },
            { "Ah pad", "choir ah voice vocal singing vowel", "a choir singing ah",
              "Starts from Ah pad and slows the start so the voices swell in together.",
              { k (Knob::ampAttack), "1.00 s", "A slow attack lets the voices swell in together." },
              { k (Knob::lfoDepth), "5%", "A little vibrato depth makes the voices sound alive." } },
            { "A saw, from sines", "lead buzzy bright saw solo mono", "a bright buzzy lead",
              "Starts from A saw, from sines and makes it play one note at a time with a slide, as a lead does.",
              { k (Knob::mode), "Mono", "Mono plays one note at a time, as a lead does." },
              { k (Knob::glide), "50 ms", "A 50 ms glide slides from one note to the next." } }
        };
    }

    Table buildTable()
    {
        Table t;
        t.synthName = "DawIt Additive";

        for (int k = 0; k < allControls; ++k)
        {
            KnobDesc d;
            d.info = additive::getKnobInfo (k);
            d.hasOff = ! d.info.isChoice && d.info.min <= 0.0f;

            if (k >= firstBar)
            {
                const int n = k - firstBar + 1;
                d.id = daw::BuiltInAdditive::getPartialId (n);
                d.title = "Harmonic " + juce::String (n) + " bar";
                d.group = 0;
                d.order = 0;
                d.offValue = additive::offDb;
                d.offExact = true;
                d.inPrompt = false;
                d.noun = "bar";
            }
            else
            {
                const auto knob = (Knob) k;
                d.id = daw::BuiltInAdditive::getParameterId (knob);
                d.title = getKnobTitle (knob);
                d.typical = additive::getTypical (k);
                d.choices = getChoiceNames (knob);
                d.order = getOrder (knob);

                if (knob == Knob::fade)
                {
                    d.hasOff = true;
                    d.offValue = additive::maxFade;
                }
            }

            t.knobs.push_back (d);
        }

        for (auto& p : additive::getPresets())
        {
            StartingPoint s;
            s.displayName = p.name;
            s.name = p.name.removeCharacters ("\"");   // a quote in the schema's choices misleads the model
            s.why = p.why;
            s.values = toValues (p.settings);
            t.starts.push_back (s);
        }

        t.examples = getExamples();
        t.fallbackExamples = { "Drawbar organ", "Plucked string", "Shimmer pad" };

        t.promptHead = juce::String ("You set the knobs and harmonic bars of DawIt Additive, an additive synthesiser, for a beginner who describes a sound in words. "
                                     "Each note is 32 sines at the harmonics of its pitch: h1 is the note itself, h2 the octave above, h3 an octave and a fifth above, "
                                     "and so on. Each bar runs from 0 dB down to -60 dB, which is Off. You do not hear sound: use what is known about how "
                                     "synthesisers are made.\n\n"
                                     "Rules:\n"
                                     "- Use only the knobs and bars in the table below, named by their id.\n"
                                     "- Give every value with its unit, as the examples show: \"-3.0 dB/oct\", \"35%\", \"350 ms\", \"1.50 s\", \"4.0 Hz\", "
                                     "\"-12 dB\" or \"Off\" for a bar, or for a choice its name (\"Spread\", \"Mono\", \"Brightness\"). Stay inside each range.\n"
                                     "- Pick the starting point by the kind of sound the words name (pad, bass, pluck, organ, bell, lead), not by one knob's name, "
                                     "and set startFrom to its name. When asked to change the current sound, set startFrom to \"") + currentSoundName
                       + "\".\n- Change as few as the sound needs, at most " + juce::String (maxChanges) + " changes, and only values that differ from the starting point.\n"
                         "- Change single harmonics only when the colour needs it, such as a missing octave or a vowel; most changes are knobs. "
                         "Harmonic changes count as one change, at most " + juce::String (maxGroupMembers) + " bars.\n"
                         "- Give one sentence per change for a beginner: what they will hear and why.\n"
                         "- If the request needs something this synth lacks (noise or breath, a filter with resonance, a second detuned oscillator, a pitch "
                         "envelope, stereo spread, effects, realistic acoustic instruments), say so in the summary.\n"
                         "- Answer with JSON only, in the form given.\n\n"
                         "Knobs and bars (id | name | range | how it is usually set):\n";

        t.extraKnobRows.add ("h1 to h32 | The harmonic bars: h1 is the note, h2 the octave above, h3 the octave and a fifth, h4 two octaves | Off (-60 dB) to 0 dB "
                             "| A saw has every bar at -20 log10 n dB; a square only the odd ones; an organ h1 to h4 at 0 dB; a hollow sound loses the even ones.");

        t.followUpExamples = "Follow-up examples, which change the current sound and touch only what they need:\n"
                             "- \"brighter\", from Plucked string: brightness -3.0 dB/oct to +0.0 dB/oct. Reason: \"Less tilt lets the high harmonics come through louder.\"\n"
                             "- \"softer start\", from Drawbar organ: ampAttack 3.0 ms to 60 ms. Reason: \"A longer attack fades each note in over 60 ms instead of starting with a click.\"\n\n";

        t.groupTitles = { "Harmonics" };
        t.memberLabel = [] (int knob) { return additive::getOrdinal (knob - firstBar + 1); };
        t.format = formatControl;
        t.readChoice = readChoiceText;
        t.describeValues = describeValues;
        t.wordNotes = wordNotesFor;
        t.limitNotes = limitNotesFor;
        return t;
    }
}

//==============================================================================
juce::String describeAdditiveBars (const additive::Settings& s)
{
    if (const auto shape = getShapeText (s); shape.isNotEmpty())
        return shape;

    juce::StringArray on;

    for (int n = 1; n <= additive::numPartials; ++n)
        if (isBarOn (s.levelsDb[(size_t) n - 1]))
            on.add ("h" + juce::String (n) + " " + tidy (additive::formatLevel (s.levelsDb[(size_t) n - 1])));

    return on.isEmpty() ? juce::String ("all Off") : on.joinIntoString (", ") + ", the rest Off";
}

const Table& getAdditiveTable()
{
    static const Table table = buildTable();
    return table;
}

Values getAdditiveValues (const additive::Settings& s)
{
    return toValues (s);
}

additive::Settings getAdditiveSettings (const Values& v)
{
    return toSettings (v);
}
} // namespace daw::gen::sounddescribe
