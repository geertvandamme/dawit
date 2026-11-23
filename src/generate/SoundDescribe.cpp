#include "SoundDescribe.h"

#include <algorithm>
#include <cmath>

namespace daw::gen::sounddescribe
{
using sub::Knob;

namespace
{
    constexpr float reachedFraction = 0.02f;

    Knob knobAt (int i) { return (Knob) i; }

    /** "20.0 kHz" becomes "20 kHz", "-24.0 dB" "-24 dB": the way a sentence says it. */
    juce::String tidy (juce::String s)
    {
        return s.replace (".00 ", " ").replace (".0 ", " ");
    }

    juce::String normaliseId (const juce::String& s)
    {
        return s.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789");
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

    /** The first number in `text` (sign, digits, a decimal point), and where it ends. */
    bool findNumber (const juce::String& text, double& value, int& end)
    {
        const int n = text.length();

        for (int i = 0; i < n; ++i)
        {
            const auto c = text[i];
            const bool digit = c >= '0' && c <= '9';
            const bool sign = c == '-' || c == '+' || c == (juce::juce_wchar) 0x2212;
            const auto next = i + 1 < n ? text[i + 1] : (juce::juce_wchar) 0;
            const bool nextStarts = (next >= '0' && next <= '9') || (next == '.' && i + 2 < n && text[i + 2] >= '0' && text[i + 2] <= '9');

            if (! (digit || (sign && nextStarts) || (c == '.' && nextStarts == false && next >= '0' && next <= '9')))
                continue;

            int j = i;
            juce::String number;

            if (sign)
            {
                number << (c == '+' ? juce::String() : juce::String ("-"));
                ++j;
            }

            bool dot = false;

            while (j < n)
            {
                const auto d = text[j];

                if (d >= '0' && d <= '9')
                    number << juce::String::charToString (d);
                else if (d == '.' && ! dot && j + 1 < n && text[j + 1] >= '0' && text[j + 1] <= '9')
                {
                    dot = true;
                    number << ".";
                }
                else if (d == ',' && j + 1 < n && text[j + 1] >= '0' && text[j + 1] <= '9')
                { /* a thousands separator */ }
                else
                    break;

                ++j;
            }

            value = number.getDoubleValue();
            end = j;
            return true;
        }

        return false;
    }

    int readChoice (Knob k, const juce::String& text)
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

    /** The normalised position of a value on the knob's travel, 0 to 1 (the log scale for the cutoff, times and rate). */
    double getTravel (Knob k, double value)
    {
        const auto info = sub::getKnobInfo (k);

        if (info.isLog)
            return std::log (juce::jmax (1.0e-9, value / info.min)) / std::log (info.max / info.min);

        return (value - info.min) / juce::jmax (1.0e-9f, info.max - info.min);
    }

    bool nearlyEqual (float a, float b)
    {
        return std::abs (a - b) <= 1.0e-4f + 1.0e-4f * std::abs (a);
    }

    //==============================================================================
    /** The lower-case words of a description; hyphens stay inside a word ("hi-hat"). */
    juce::StringArray splitWords (const juce::String& text)
    {
        juce::String cleaned;

        for (auto c : text.toLowerCase())
            cleaned << ((juce::CharacterFunctions::isLetterOrDigit (c) || c == '-') ? juce::String::charToString (c) : juce::String (" "));

        return juce::StringArray::fromTokens (cleaned, " ", "");
    }

    /** " a warm pad " for matching whole words and phrases. */
    juce::String padded (const juce::String& words)
    {
        return " " + splitWords (words).joinIntoString (" ") + " ";
    }

    bool hasWord (const juce::String& paddedWords, const juce::String& phrase)
    {
        return paddedWords.contains (" " + phrase + " ");
    }

    bool hasAny (const juce::String& paddedWords, std::initializer_list<const char*> phrases)
    {
        for (auto* p : phrases)
            if (hasWord (paddedWords, p))
                return true;

        return false;
    }

    juce::String formatCentroid (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    //==============================================================================
    struct Example
    {
        const char* preset;
        const char* keywords;
        const char* description;
        const char* summary;
        struct Change { Knob knob; const char* value; const char* reason; };
        Change first, second;
    };

    const std::vector<Example>& getExamples()
    {
        static const std::vector<Example> examples
        {
            { "Bass", "bass fat analog low deep thick", "a fat analog bass",
              "Starts from Bass and lets more of the saw through for a fatter sound.",
              { Knob::cutoff, "300 Hz", "A higher cutoff lets more of the saw's upper harmonics through, so the bass sounds fatter." },
              { Knob::filterDecay, "400 ms", "A longer filter decay keeps each note bright for a little longer after it starts." } },
            { "Acid bass", "acid squelch squelchy 303 resonant resonance techno house", "an acid bass with lots of squelch",
              "Starts from Acid bass and pushes the resonance and the filter sweep for more squelch.",
              { Knob::resonance, "85%", "More resonance boosts the frequencies at the cutoff, which makes the squelch stronger." },
              { Knob::envAmount, "+4.0 oct", "A bigger filter envelope sweeps the cutoff further at the start of each note." } },
            { "Sub bass", "sub deep bass low boom clean", "a deep clean sub bass",
              "Starts from Sub bass and darkens it a little so only the low body is left.",
              { Knob::cutoff, "1.20 kHz", "A lower cutoff removes more of the upper harmonics and leaves the low body." },
              { Knob::ampRelease, "200 ms", "A slightly longer release lets each note fade out instead of stopping short." } },
            { "Warm pad", "warm pad soft chords swell swells swelling evolving slow lush", "a warm pad that swells in slowly",
              "Starts from Warm pad and lengthens the attack so each chord fades in.",
              { Knob::ampAttack, "1.20 s", "A longer attack fades each chord in over 1.2 seconds instead of 0.8." },
              { Knob::detune, "14 ct", "A wider detune makes the two saws beat faster, which sounds thicker." } },
            { "Pluck", "pluck short plucky bright arpeggio riff staccato", "a short bright pluck",
              "Starts from Pluck and makes it shorter and a little brighter.",
              { Knob::ampDecay, "400 ms", "A shorter decay makes the note die away faster, which sounds more plucked." },
              { Knob::cutoff, "600 Hz", "A higher starting cutoff makes the pluck brighter." } },
            { "Synth brass", "brass horn horns trumpet fanfare eighties bwaah", "a punchy 80s synth brass",
              "Starts from Synth brass and makes the swell quicker with a sharper edge.",
              { Knob::filterAttack, "40 ms", "A shorter filter attack makes the brass open faster, so it sounds punchier." },
              { Knob::resonance, "20%", "A little more resonance gives the brass a sharper edge." } },
            { "Lead", "lead solo melody cutting vibrato glide", "a cutting solo lead",
              "Starts from Lead and makes it brighter so it cuts through.",
              { Knob::cutoff, "3.50 kHz", "A higher cutoff makes the lead brighter, so it cuts through a mix." },
              { Knob::lfoDepth, "15%", "More vibrato depth makes the pitch wave a little more." } },
            { "String machine", "strings string ensemble orchestra polysynth", "lush synth strings",
              "Starts from String machine and slows the attack so the strings swell in.",
              { Knob::ampAttack, "500 ms", "A slower attack makes the strings swell in like a bowed section." },
              { Knob::detune, "18 ct", "More detune makes the ensemble wider and richer." } },
            { "PWM strings", "pwm pulse moving lush shimmer", "a moving pulse-width sound",
              "Starts from PWM strings and makes the pulse width move faster and further.",
              { Knob::lfoRate, "0.9 Hz", "A faster LFO makes the pulse width change quicker, so the sound moves more." },
              { Knob::lfoDepth, "50%", "A deeper LFO swings the pulse width further." } },
            { "Wobble bass", "wobble dubstep wub wobbly bass lfo", "a dubstep wobble bass",
              "Starts from Wobble bass and speeds up the wobble with more bite.",
              { Knob::lfoRate, "4.0 Hz", "At 120 BPM, 4 Hz gives a wobble on every eighth note." },
              { Knob::resonance, "60%", "More resonance makes the wobble sound sharper." } },
            { "Soft keys", "keys keyboard piano soft gentle electric", "soft electric keys",
              "Starts from Soft keys and makes them a little darker and shorter.",
              { Knob::ampDecay, "900 ms", "A shorter decay lets the note fade a little sooner." },
              { Knob::cutoff, "1.20 kHz", "A lower cutoff takes off some brightness, which sounds softer." } },
            { "Hollow organ", "organ hollow steady church held", "a hollow organ",
              "Starts from Hollow organ and makes it a little darker with a softer ending.",
              { Knob::ampRelease, "150 ms", "A slightly longer release lets each note fade out softly." },
              { Knob::cutoff, "3.00 kHz", "A lower cutoff makes the hollow sound a little darker." } },
            { "Bright stab", "stab bright short chord accent rhythmic", "a bright short chord stab",
              "Starts from Bright stab and makes it brighter and shorter.",
              { Knob::cutoff, "2.00 kHz", "A higher cutoff makes the stab brighter." },
              { Knob::ampDecay, "200 ms", "A shorter decay makes the chord fade away quicker." } },
            { "Tremolo keys", "tremolo pulsing pulse keys", "pulsing keys",
              "Starts from Tremolo keys and makes the pulsing faster and deeper.",
              { Knob::lfoRate, "6.0 Hz", "A faster LFO makes the level rise and fall quicker." },
              { Knob::lfoDepth, "50%", "A deeper LFO makes the level rise and fall further." } },
            { "Airy sweep", "airy breathy breath noise sweep wind", "an airy breathy sweep",
              "Starts from Airy sweep and adds more breath and a slower start.",
              { Knob::noise, "30%", "More noise adds a breathier edge." },
              { Knob::ampAttack, "900 ms", "A longer attack makes the sweep fade in more slowly." } },
            { "Resonant reed", "reed clarinet oboe woody nasal bandpass woodwind", "a reedy woodwind tone",
              "Starts from Resonant reed and makes it more focused and a little brighter.",
              { Knob::resonance, "40%", "More resonance makes the tone more focused." },
              { Knob::cutoff, "1.60 kHz", "A higher cutoff moves the band up, which sounds brighter." } }
        };

        return examples;
    }

    const sub::Preset* findPreset (const juce::String& name)
    {
        for (auto& p : sub::getPresets())
            if (p.name.equalsIgnoreCase (name))
                return &p;

        return nullptr;
    }

    /** "Init" or "Bass": a name the model gave, matched to a starting point. */
    const sub::Preset* matchPreset (const juce::String& name)
    {
        const auto wanted = name.trim();

        if (wanted.isEmpty())
            return nullptr;

        if (auto* p = findPreset (wanted))
            return p;

        if (wanted.length() >= 3)
            for (auto& p : sub::getPresets())
                if (p.name.startsWithIgnoreCase (wanted) || wanted.startsWithIgnoreCase (p.name.upToFirstOccurrenceOf (":", false, false)))
                    return &p;

        return nullptr;
    }

    juce::String valuesLine (const sub::Settings& s, const sub::Settings* skipEqualTo)
    {
        juce::StringArray parts;

        for (int k = 0; k < sub::numKnobs; ++k)
        {
            const auto knob = knobAt (k);

            if (skipEqualTo != nullptr && nearlyEqual (sub::getKnobValue (knob, s), sub::getKnobValue (knob, *skipEqualTo)))
                continue;

            parts.add (getKnobId (knob) + "=" + tidy (sub::formatKnob (knob, s)));
        }

        return parts.joinIntoString (", ");
    }

    juce::String firstSentence (const juce::String& s)
    {
        const int dot = s.indexOf (". ");
        return dot > 0 ? s.substring (0, dot + 1) : s;
    }

    juce::String exampleJson (const Example& e)
    {
        auto* answer = new juce::DynamicObject();
        answer->setProperty ("startFrom", e.preset);
        answer->setProperty ("summary", e.summary);
        juce::Array<juce::var> changes;

        for (auto& c : { e.first, e.second })
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("knob", getKnobId (c.knob));
            o->setProperty ("value", c.value);
            o->setProperty ("reason", c.reason);
            changes.add (juce::var (o));
        }

        answer->setProperty ("changes", changes);
        return juce::JSON::toString (juce::var (answer), true);
    }

    juce::var parseJson (const juce::String& reply)
    {
        const int a = reply.indexOfChar ('{'), b = reply.lastIndexOfChar ('}');

        if (a < 0 || b <= a)
            return {};

        return juce::JSON::parse (reply.substring (a, b + 1));
    }
}

//==============================================================================
juce::String getKnobId (Knob k)
{
    return daw::BuiltInSubtractive::getParameterId (k);
}

bool getKnobFromId (const juce::String& id, Knob& result)
{
    const auto wanted = normaliseId (id);

    if (wanted.isEmpty())
        return false;

    for (int k = 0; k < sub::numKnobs; ++k)
        if (normaliseId (getKnobId (knobAt (k))) == wanted)
        {
            result = knobAt (k);
            return true;
        }

    return false;
}

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

//==============================================================================
juce::String formatKnobValue (Knob k, float value)
{
    sub::Settings s;
    sub::setKnobValue (k, s, value);
    return sub::formatKnob (k, s);
}

ValueRead readValue (Knob k, const juce::String& textIn)
{
    ValueRead r;
    const auto info = sub::getKnobInfo (k);
    const auto original = textIn.trim();
    const auto text = original.toLowerCase();

    if (text.isEmpty())
        return r;

    if (info.isChoice)
    {
        const int index = readChoice (k, text);

        if (index < 0)
            return r;

        r.ok = true;
        r.value = (float) index;
        return r;
    }

    double number = 0.0;
    int end = 0;

    if (! findNumber (text, number, end))
    {
        if ((text == "off" || text == "none" || text == "zero") && info.min <= 0.0f)
        {
            r.ok = true;
            r.value = 0.0f;
            return r;
        }

        return r;
    }

    // The unit after the number: letters and % and the slash of dB/oct.
    const auto rest = text.substring (end).trimStart();
    juce::String unit;

    for (auto c : rest)
    {
        if (juce::CharacterFunctions::isLetter (c) || c == '%')
            unit << juce::String::charToString (c);
        else
            break;
    }

    if (info.unit == "Hz" && (unit == "khz" || unit == "k"))
        number *= 1000.0;
    else if (info.unit == "s" && (unit == "ms" || unit == "msec" || unit == "millisecond" || unit == "milliseconds"))
        number /= 1000.0;

    double value = number;
    const double slack = 1.0e-4 * (info.max - info.min);

    if (value < info.min - slack || value > info.max + slack)
    {
        value = juce::jlimit ((double) info.min, (double) info.max, value);
        r.note = "The model asked for " + original + "; the knob stops at " + tidy (formatKnobValue (k, (float) value)) + ".";
    }
    else if (! info.isLog && info.step > 0.0f)
    {
        value = info.min + std::round ((value - info.min) / info.step) * info.step;
        value = juce::jlimit ((double) info.min, (double) info.max, value);
    }

    r.ok = true;
    r.value = (float) value;
    return r;
}

ValueRead readValue (Knob k, const juce::var& v)
{
    if (v.isString())
        return readValue (k, v.toString());

    if (v.isInt() || v.isInt64() || v.isDouble())
    {
        const auto info = sub::getKnobInfo (k);

        if (info.isChoice)
        {
            ValueRead r;
            r.ok = true;
            r.value = (float) juce::jlimit (0, info.numChoices - 1, (int) v);
            return r;
        }

        return readValue (k, juce::String ((double) v) + " " + info.unit);
    }

    return {};
}

//==============================================================================
juce::String buildSystemPrompt()
{
    juce::String s;
    s << "You set the knobs of DawIt Subtractive, a subtractive synthesiser, for a beginner who describes a sound in words. "
         "You do not hear sound: use what is known about how synthesisers are made.\n\n"
         "Rules:\n"
         "- Use only the knobs in the table below, named by their id.\n"
         "- Give every value with its unit, as the examples show: \"1.80 kHz\", \"60 ms\", \"35%\", \"+7 st\", \"12 ct\", "
         "\"+2.5 oct\", \"-3.0 dB\", or for a choice its name (\"Saw\", \"Low-pass\", \"Mono\"). Stay inside each range.\n"
         "- Start from the starting point closest to the words and set startFrom to its name. When asked to change the "
         "current sound, set startFrom to \"" << currentSoundName << "\".\n"
         "- Change as few knobs as the sound needs, at most " << maxChanges << ", and only knobs whose value differs from the starting point.\n"
         "- Give one sentence per change for a beginner: what they will hear and why.\n"
         "- If the request needs something this synth lacks (a pitch envelope, stereo spread, effects, bell-like tones whose "
         "partials are not whole multiples of the note, realistic acoustic instruments), say so in the summary.\n"
         "- Answer with JSON only, in the form given.\n\n"
         "Knobs (id | name | range | how it is usually set):\n";

    for (int k = 0; k < sub::numKnobs; ++k)
    {
        const auto knob = knobAt (k);
        const auto info = sub::getKnobInfo (knob);
        juce::String range;

        if (info.isChoice)
            range = getChoiceNames (knob).joinIntoString (" | ");
        else
            range = tidy (formatKnobValue (knob, info.min)) + " to " + tidy (formatKnobValue (knob, info.max));

        s << getKnobId (knob) << " | " << getKnobTitle (knob) << " | " << range << " | " << sub::getTypical (k) << "\n";
    }

    return s;
}

juce::StringArray pickExamples (const juce::String& words)
{
    const auto& examples = getExamples();
    const auto tokens = splitWords (words);
    std::vector<std::pair<int, int>> scored;   // score, index

    for (int i = 0; i < (int) examples.size(); ++i)
    {
        const auto keys = padded (juce::String (examples[(size_t) i].keywords) + " " + examples[(size_t) i].preset);
        int score = 0;

        for (auto& t : tokens)
            if (t.length() >= 3 && keys.contains (" " + t + " "))
                ++score;

        if (score > 0)
            scored.push_back ({ score, i });
    }

    std::stable_sort (scored.begin(), scored.end(), [] (auto& a, auto& b) { return a.first > b.first; });

    juce::StringArray picked;

    for (auto& [score, i] : scored)
    {
        juce::ignoreUnused (score);

        if (picked.size() < 3)
            picked.add (examples[(size_t) i].preset);
    }

    for (auto* fallback : { "Bass", "Warm pad", "Pluck" })
        if (picked.size() < 3 && ! picked.contains (fallback))
            picked.add (fallback);

    return picked;
}

juce::String buildUserPrompt (const Request& r)
{
    const auto& presets = sub::getPresets();
    const sub::Settings init = presets.front().settings;
    juce::String s;

    s << "Starting points (name: what it is | the knobs that differ from Init):\n";

    for (auto& p : presets)
    {
        const bool isInit = &p == &presets.front();
        s << "- " << p.name << ": " << firstSentence (p.why) << " | " << (isInit ? juce::String ("nothing differs") : valuesLine (p.settings, &init)) << "\n";
    }

    s << "\nWorked examples, each from a starting point:\n";

    for (auto& name : pickExamples (r.words))
        for (auto& e : getExamples())
            if (name == e.preset)
            {
                if (auto* preset = findPreset (e.preset))
                    s << "Description: \"" << e.description << "\"\nStarting point values: " << valuesLine (preset->settings, &init) << "\nAnswer: " << exampleJson (e) << "\n\n";
            }

    s << "Follow-up examples, which change the current sound and touch only the knobs they need:\n"
         "- \"brighter\", from Warm pad: cutoff 900 Hz to 1.80 kHz. Reason: \"Twice the cutoff, one octave up, lets more of the saws' upper harmonics through.\"\n"
         "- \"softer start\", from Lead: ampAttack 5.0 ms to 60 ms. Reason: \"A longer attack fades each note in over 60 ms instead of starting with a click.\"\n\n";

    s << "The current sound: " << valuesLine (r.current, nullptr) << "\n\n";

    if (r.followUp)
        s << "Change the current sound as described. Set startFrom to \"" << currentSoundName << "\" and give only the changes.\n";
    else
        s << "Pick the closest starting point for the description, change as little as the sound needs, and set startFrom to its name.\n";

    s << "Description: \"" << r.words.trim() << "\"\n";
    return s;
}

juce::var buildSchema()
{
    auto object = [] (juce::NamedValueSet props, juce::StringArray required)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("type", "object");
        auto* p = new juce::DynamicObject();

        for (auto& v : props)
            p->setProperty (v.name, v.value);

        o->setProperty ("properties", juce::var (p));
        juce::Array<juce::var> req;

        for (auto& name : required)
            req.add (name);

        o->setProperty ("required", req);
        return juce::var (o);
    };

    auto text = [] { auto* o = new juce::DynamicObject(); o->setProperty ("type", "string"); return juce::var (o); };

    auto enumOf = [] (const juce::StringArray& values)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("type", "string");
        juce::Array<juce::var> list;

        for (auto& v : values)
            list.add (v);

        o->setProperty ("enum", list);
        return juce::var (o);
    };

    juce::StringArray names, ids;

    for (auto& p : sub::getPresets())
        names.add (p.name);

    names.add (currentSoundName);

    for (int k = 0; k < sub::numKnobs; ++k)
        ids.add (getKnobId (knobAt (k)));

    juce::NamedValueSet changeProps;
    changeProps.set ("knob", enumOf (ids));
    changeProps.set ("value", text());
    changeProps.set ("reason", text());

    auto* changes = new juce::DynamicObject();
    changes->setProperty ("type", "array");
    changes->setProperty ("maxItems", maxChanges);
    changes->setProperty ("items", object (changeProps, { "knob", "value", "reason" }));

    juce::NamedValueSet props;
    props.set ("startFrom", enumOf (names));
    props.set ("summary", text());
    props.set ("changes", juce::var (changes));
    return object (props, { "startFrom", "summary", "changes" });
}

//==============================================================================
Result readReply (const Request& request, const juce::String& reply)
{
    Result result;
    const auto json = parseJson (reply);
    auto* list = json.isObject() ? json["changes"].getArray() : nullptr;

    if (list == nullptr)
    {
        result.error = "The model's answer couldn't be read. Ask again, or pick another model.";
        return result;
    }

    auto& sound = result.sound;
    sound.words = request.words.trim();
    sound.model = request.model;
    sound.summary = json["summary"].toString().trim();
    sound.followUp = request.followUp;

    // The base: the sound in the window, or the starting point the model named.
    sound.base = currentSoundName;
    sound.baseSettings = request.current;

    if (! request.followUp)
    {
        const auto named = json["startFrom"].toString().trim();

        if (named.equalsIgnoreCase (currentSoundName))
        {}
        else if (auto* preset = matchPreset (named))
        {
            sound.base = preset->name.upToFirstOccurrenceOf (":", false, false);
            sound.baseSettings = preset->settings;
        }
        else
            sound.notes.add ("The model named no starting point DawIt knows, so it starts from the current sound.");
    }

    // The changes: the last mention of a knob wins, equal values and unknown knobs go, then the cap.
    std::vector<sub::DescribedChange> found;
    int dropped = 0;

    for (auto& item : *list)
    {
        Knob knob = Knob::cutoff;

        if (! getKnobFromId (item["knob"].toString(), knob))
        {
            ++dropped;
            continue;
        }

        const auto value = readValue (knob, item["value"]);

        if (! value.ok)
        {
            ++dropped;
            continue;
        }

        sub::DescribedChange c;
        c.knob = knob;
        c.before = sub::getKnobValue (knob, sound.baseSettings);
        c.after = value.value;
        c.reason = item["reason"].toString().trim();
        c.note = value.note;

        auto same = std::find_if (found.begin(), found.end(), [knob] (auto& f) { return f.knob == knob; });

        if (same != found.end())
            found.erase (same);

        found.push_back (c);
    }

    result.unknownKnobs = dropped;
    std::vector<sub::DescribedChange> kept;

    for (auto& c : found)
    {
        if (nearlyEqual (c.before, c.after))
            ++result.droppedEqual;
        else if ((int) kept.size() >= maxChanges)
            ++result.droppedExtra;
        else
            kept.push_back (c);
    }

    std::sort (kept.begin(), kept.end(), [] (auto& a, auto& b) { return (int) a.knob < (int) b.knob; });
    sound.changes = kept;

    if (dropped > 0)
        sound.notes.add (dropped == 1 ? "1 suggestion named a knob this synth doesn't have and was left out."
                                      : juce::String (dropped) + " suggestions named knobs this synth doesn't have and were left out.");

    for (auto& c : sound.changes)
        if (c.note.isNotEmpty())
            sound.notes.add (c.note);

    sound.notes.addArray (getWordNotes (sound.words, sound.followUp, sound.baseSettings, getResultSettings (sound)));
    sound.notes.addArray (getLimitNotes (sound.words));
    result.ok = true;
    return result;
}

sub::Settings getResultSettings (const sub::DescribedSound& sound)
{
    auto s = sound.baseSettings;

    for (auto& c : sound.changes)
        sub::setKnobValue (c.knob, s, c.after);

    return s;
}

//==============================================================================
juce::StringArray getWordNotes (const juce::String& words, bool followUp, const sub::Settings& base, const sub::Settings& result)
{
    juce::StringArray notes;
    const auto w = padded (words);

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

juce::StringArray getLimitNotes (const juce::String& words)
{
    juce::StringArray notes;
    const auto w = padded (words);

    if (hasAny (w, { "808", "kick", "snare", "hihat", "hi-hat", "drum", "drums" }))
        notes.add ("An 808 kick drops in pitch at the start of each note, and DawIt Subtractive has no pitch envelope. "
                   "DawIt Drums makes 808-style drums; for an 808-style bass here, start from Sub bass with a long Amp decay.");

    if (hasAny (w, { "bell", "bells", "chime", "chimes", "glass", "glassy", "marimba", "electric piano" }))
        notes.add ("Bells and chimes have partials that aren't whole-number multiples of the note, and this synth's oscillators make "
                   "whole-number harmonics only. DawIt FM makes bells, electric pianos and marimbas.");

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

//==============================================================================
bool areSame (const sub::Settings& a, const sub::Settings& b)
{
    for (int k = 0; k < sub::numKnobs; ++k)
        if (! nearlyEqual (sub::getKnobValue (knobAt (k), a), sub::getKnobValue (knobAt (k), b)))
            return false;

    return true;
}

bool Walkthrough::isReached (Knob k, float value, float target)
{
    if (sub::getKnobInfo (k).isChoice)
        return juce::roundToInt (value) == juce::roundToInt (target);

    return std::abs (getTravel (k, value) - getTravel (k, target)) <= reachedFraction + 1.0e-6;
}

Walkthrough::Walkthrough (const sub::DescribedSound& sound, const sub::Settings& live)
    : baseSettings (sound.baseSettings)
{
    if (! sound.followUp && ! areSame (sound.baseSettings, live))
    {
        Step s;
        s.loadsBase = true;
        s.reason = "Load the starting point " + sound.base;
        steps.push_back (s);
    }

    for (auto& c : sound.changes)
    {
        Step s;
        s.knob = c.knob;
        s.before = c.before;
        s.target = c.after;
        s.reason = c.reason;
        s.note = c.note;
        steps.push_back (s);
    }

    begin (live);
}

void Walkthrough::begin (const sub::Settings& live)
{
    if (index < getNumSteps())
    {
        steps[(size_t) index].snapshot = live;
        steps[(size_t) index].taken = true;
    }
}

void Walkthrough::advance (const sub::Settings& live)
{
    ++index;
    begin (live);
}

bool Walkthrough::isCurrentReached (const sub::Settings& live) const
{
    if (isFinished() || steps[(size_t) index].loadsBase)
        return false;

    auto& s = steps[(size_t) index];
    return isReached (s.knob, sub::getKnobValue (s.knob, live), s.target);
}

void Walkthrough::markDone (const sub::Settings& live)
{
    if (! isFinished())
        advance (live);
}

void Walkthrough::setForUser (sub::Settings& live)
{
    if (isFinished())
        return;

    auto& s = steps[(size_t) index];

    if (s.loadsBase)
        live = baseSettings;
    else
        sub::setKnobValue (s.knob, live, s.target);

    advance (live);
}

void Walkthrough::skip (const sub::Settings& live)
{
    if (! isFinished())
        advance (live);
}

void Walkthrough::back (sub::Settings& live)
{
    if (index <= 0)
        return;

    --index;
    auto& s = steps[(size_t) index];

    if (s.taken)
    {
        if (s.loadsBase)
            live = s.snapshot;
        else
            sub::setKnobValue (s.knob, live, sub::getKnobValue (s.knob, s.snapshot));
    }

    s.snapshot = live;
    s.taken = true;
}

void Walkthrough::finishAll (sub::Settings& live)
{
    while (! isFinished())
        setForUser (live);
}

//==============================================================================
void SettingsUndo::push (const sub::Settings& s)
{
    list.push_back (s);

    if ((int) list.size() > maxSteps)
        list.erase (list.begin());
}

bool SettingsUndo::pop (sub::Settings& result)
{
    if (list.empty())
        return false;

    result = list.back();
    list.pop_back();
    return true;
}
} // namespace daw::gen::sounddescribe
