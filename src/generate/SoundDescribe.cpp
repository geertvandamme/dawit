#include "SoundDescribe.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace daw::gen::sounddescribe
{
namespace
{
    constexpr float reachedFraction = 0.02f;

    juce::String normaliseId (const juce::String& s)
    {
        return s.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789");
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

    /** The normalised position of a value on the knob's travel, 0 to 1 (the log scale for the cutoff, times and rate). */
    double getTravel (const sub::KnobInfo& info, double value)
    {
        if (info.isLog)
            return std::log (juce::jmax (1.0e-9, value / info.min)) / std::log (info.max / info.min);

        return (value - info.min) / juce::jmax (1.0e-9f, info.max - info.min);
    }

    //==============================================================================
    juce::String valuesText (const Table& t, const Values& v, const Values* skipEqualTo)
    {
        return t.describeValues (v, skipEqualTo);
    }

    const StartingPoint* findStart (const Table& t, const juce::String& name)
    {
        for (auto& p : t.starts)
            if (p.name.equalsIgnoreCase (name) || (p.displayName.isNotEmpty() && p.displayName.equalsIgnoreCase (name)))
                return &p;

        return nullptr;
    }

    /** "Init" or "Bass": a name the model gave, matched to a starting point. */
    const StartingPoint* matchStart (const Table& t, const juce::String& name)
    {
        const auto wanted = name.trim();

        if (wanted.isEmpty())
            return nullptr;

        if (auto* p = findStart (t, wanted))
            return p;

        if (wanted.length() >= 3)
            for (auto& p : t.starts)
                if (p.name.startsWithIgnoreCase (wanted) || wanted.startsWithIgnoreCase (p.name.upToFirstOccurrenceOf (":", false, false)))
                    return &p;

        return nullptr;
    }

    juce::String exampleJson (const Table& t, const Example& e)
    {
        auto* answer = new juce::DynamicObject();
        answer->setProperty ("startFrom", e.preset);
        answer->setProperty ("summary", e.summary);
        juce::Array<juce::var> changes;

        for (auto& c : { e.first, e.second })
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("knob", getKnobId (t, c.knob));
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

    /** The lower-case words of a description; hyphens stay inside a word ("hi-hat"). */
    juce::StringArray splitWords (const juce::String& text)
    {
        juce::String cleaned;

        for (auto c : text.toLowerCase())
            cleaned << ((juce::CharacterFunctions::isLetterOrDigit (c) || c == '-') ? juce::String::charToString (c) : juce::String (" "));

        return juce::StringArray::fromTokens (cleaned, " ", "");
    }
}

//==============================================================================
juce::String padWords (const juce::String& words)
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

/** "20.0 kHz" becomes "20 kHz", "-24.0 dB" "-24 dB": the way a sentence says it. */
juce::String tidy (juce::String s)
{
    return s.replace (".00 ", " ").replace (".0 ", " ");
}

juce::String firstSentence (const juce::String& s)
{
    const int dot = s.indexOf (". ");
    return dot > 0 ? s.substring (0, dot + 1) : s;
}

bool nearlyEqual (float a, float b)
{
    return std::abs (a - b) <= 1.0e-4f + 1.0e-4f * std::abs (a);
}

//==============================================================================
juce::String getKnobId (const Table& t, int knob)
{
    return juce::isPositiveAndBelow (knob, t.getNumKnobs()) ? t.knobs[(size_t) knob].id : juce::String();
}

bool getKnobFromId (const Table& t, const juce::String& id, int& result)
{
    const auto wanted = normaliseId (id);

    if (wanted.isEmpty())
        return false;

    for (int k = 0; k < t.getNumKnobs(); ++k)
        if (normaliseId (t.knobs[(size_t) k].id) == wanted)
        {
            result = k;
            return true;
        }

    return false;
}

//==============================================================================
juce::String formatKnobValue (const Table& t, int knob, float value)
{
    return t.format (knob, value);
}

ValueRead readValue (const Table& t, int knob, const juce::String& textIn)
{
    ValueRead r;

    if (! juce::isPositiveAndBelow (knob, t.getNumKnobs()))
        return r;

    const auto& desc = t.knobs[(size_t) knob];
    const auto& info = desc.info;
    const auto original = textIn.trim();
    const auto text = original.toLowerCase();

    if (text.isEmpty())
        return r;

    if (info.isChoice)
    {
        const int index = t.readChoice (knob, text);

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
        if ((text == "off" || text == "none" || text == "zero") && desc.hasOff)
        {
            r.ok = true;
            r.value = desc.offValue;
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
        r.note = "The model asked for " + original + "; the " + desc.noun + " stops at " + tidy (formatKnobValue (t, knob, (float) value)) + ".";
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

ValueRead readValue (const Table& t, int knob, const juce::var& v)
{
    if (v.isString())
        return readValue (t, knob, v.toString());

    if ((v.isInt() || v.isInt64() || v.isDouble()) && juce::isPositiveAndBelow (knob, t.getNumKnobs()))
    {
        const auto& info = t.knobs[(size_t) knob].info;

        if (info.isChoice)
        {
            ValueRead r;
            r.ok = true;
            r.value = (float) juce::jlimit (0, info.numChoices - 1, (int) v);
            return r;
        }

        return readValue (t, knob, juce::String ((double) v) + " " + (info.unit == "dB/oct" ? juce::String ("dB/oct") : info.unit));
    }

    return {};
}

//==============================================================================
juce::String buildSystemPrompt (const Table& t)
{
    juce::String s = t.promptHead;

    for (int k = 0; k < t.getNumKnobs(); ++k)
    {
        const auto& desc = t.knobs[(size_t) k];

        if (! desc.inPrompt)
            continue;

        juce::String range;

        if (desc.info.isChoice)
            range = desc.choices.joinIntoString (" | ");
        else
            range = tidy (formatKnobValue (t, k, desc.info.min)) + " to " + tidy (formatKnobValue (t, k, desc.info.max));

        s << desc.id << " | " << desc.title << " | " << range << " | " << desc.typical << "\n";
    }

    for (auto& row : t.extraKnobRows)
        s << row << "\n";

    return s;
}

std::vector<int> pickExampleIndices (const Table& t, const juce::String& words)
{
    const auto& examples = t.examples;
    const auto tokens = splitWords (words);
    std::vector<std::pair<int, int>> scored;   // score, index

    for (int i = 0; i < (int) examples.size(); ++i)
    {
        const auto keys = padWords (juce::String (examples[(size_t) i].keywords) + " " + examples[(size_t) i].preset);
        int score = 0;

        for (auto& tok : tokens)
            if (tok.length() >= 3 && keys.contains (" " + tok + " "))
                ++score;

        if (score > 0)
            scored.push_back ({ score, i });
    }

    std::stable_sort (scored.begin(), scored.end(), [] (auto& a, auto& b) { return a.first > b.first; });

    std::vector<int> picked;

    for (auto& [score, i] : scored)
    {
        juce::ignoreUnused (score);

        if (picked.size() < 3)
            picked.push_back (i);
    }

    for (auto& fallback : t.fallbackExamples)
        for (int i = 0; i < (int) examples.size() && picked.size() < 3; ++i)
            if (fallback == examples[(size_t) i].preset && std::find (picked.begin(), picked.end(), i) == picked.end())
            {
                picked.push_back (i);
                break;
            }

    return picked;
}

juce::StringArray pickExamples (const Table& t, const juce::String& words)
{
    juce::StringArray names;

    for (auto i : pickExampleIndices (t, words))
        names.add (t.examples[(size_t) i].preset);

    return names;
}

juce::String buildUserPrompt (const Table& t, const Request& r)
{
    const auto& init = t.getInit();
    juce::String s;

    s << "Starting points (name: what it is | the knobs that differ from Init):\n";

    for (auto& p : t.starts)
    {
        const bool isInit = &p == &t.starts.front();
        s << "- " << p.name << ": " << firstSentence (p.why) << " | " << (isInit ? juce::String ("nothing differs") : valuesText (t, p.values, &init)) << "\n";
    }

    s << "\nWorked examples, each from a starting point:\n";

    // The closest example goes last, nearest the question, where the model weighs it most.
    const auto picks = pickExampleIndices (t, r.words);

    for (auto i = picks.size(); i-- > 0;)
    {
        const auto& e = t.examples[(size_t) picks[i]];

        if (auto* start = findStart (t, e.preset))
            s << "Description: \"" << e.description << "\"\nStarting point values: " << valuesText (t, start->values, &init)
              << "\nAnswer: " << exampleJson (t, e) << "\n\n";
    }

    s << t.followUpExamples;
    s << "The current sound: " << valuesText (t, r.current, nullptr) << "\n\n";

    if (r.followUp)
        s << "Change the current sound as described. Set startFrom to \"" << currentSoundName << "\" and give only the changes.\n";
    else
        s << "Pick the closest starting point for the description, change as little as the sound needs, and set startFrom to its name.\n";

    s << "Description: \"" << r.words.trim() << "\"\n";
    return s;
}

juce::var buildSchema (const Table& t)
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

    for (auto& p : t.starts)
        names.add (p.name);

    names.add (currentSoundName);

    for (auto& k : t.knobs)
        ids.add (k.id);

    juce::NamedValueSet changeProps;
    changeProps.set ("knob", enumOf (ids));
    changeProps.set ("value", text());
    changeProps.set ("reason", text());

    auto* changes = new juce::DynamicObject();
    changes->setProperty ("type", "array");
    std::set<int> groups;

    for (auto& k : t.knobs)
        if (k.group >= 0)
            groups.insert (k.group);

    changes->setProperty ("maxItems", maxChanges + (int) groups.size() * (maxGroupMembers - 1));
    changes->setProperty ("items", object (changeProps, { "knob", "value", "reason" }));

    juce::NamedValueSet props;
    props.set ("startFrom", enumOf (names));
    props.set ("summary", text());
    props.set ("changes", juce::var (changes));
    return object (props, { "startFrom", "summary", "changes" });
}

//==============================================================================
Result readReply (const Table& t, const Request& request, const juce::String& reply)
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
    sound.baseValues = request.current;

    if (! request.followUp)
    {
        const auto named = json["startFrom"].toString().trim();

        if (named.equalsIgnoreCase (currentSoundName))
        {}
        else if (auto* start = matchStart (t, named))
        {
            sound.base = (start->displayName.isNotEmpty() ? start->displayName : start->name).upToFirstOccurrenceOf (":", false, false);
            sound.baseValues = start->values;
        }
        else
            sound.notes.add ("The model named no starting point DawIt knows, so it starts from the current sound.");
    }

    // The changes: the last mention of a knob wins, equal values and unknown knobs go, then the caps.
    std::vector<DescribedChange> found;
    int dropped = 0;

    for (auto& item : *list)
    {
        int knob = 0;

        if (! getKnobFromId (t, item["knob"].toString(), knob))
        {
            ++dropped;
            continue;
        }

        const auto value = readValue (t, knob, item["value"]);

        if (! value.ok)
        {
            ++dropped;
            continue;
        }

        DescribedChange c;
        c.knob = knob;
        c.before = (size_t) knob < sound.baseValues.size() ? sound.baseValues[(size_t) knob] : 0.0f;
        c.after = value.value;
        c.reason = item["reason"].toString().trim();
        c.note = value.note;

        auto same = std::find_if (found.begin(), found.end(), [knob] (auto& f) { return f.knob == knob; });

        if (same != found.end())
            found.erase (same);

        found.push_back (c);
    }

    result.unknownKnobs = dropped;
    std::vector<DescribedChange> candidates;

    for (auto& c : found)
    {
        if (nearlyEqual (c.before, c.after))
            ++result.droppedEqual;
        else
            candidates.push_back (c);
    }

    // A group (the harmonic bars) is one change against the cap, whatever its size, and holds at most 12 members.
    std::set<int> groups;

    for (auto& c : candidates)
        if (t.knobs[(size_t) c.knob].group >= 0)
            groups.insert (t.knobs[(size_t) c.knob].group);

    const int knobCap = juce::jmax (0, maxChanges - (int) groups.size());
    int knobsKept = 0;
    std::map<int, int> groupKept;
    std::vector<DescribedChange> kept;

    for (auto& c : candidates)
    {
        const int group = t.knobs[(size_t) c.knob].group;

        if (group >= 0)
        {
            if (groupKept[group] >= maxGroupMembers)
            {
                ++result.droppedGroup;
                continue;
            }

            ++groupKept[group];
        }
        else
        {
            if (knobsKept >= knobCap)
            {
                ++result.droppedExtra;
                continue;
            }

            ++knobsKept;
        }

        kept.push_back (c);
    }

    std::stable_sort (kept.begin(), kept.end(), [&t] (auto& a, auto& b)
    {
        const auto oa = t.knobs[(size_t) a.knob].order, ob = t.knobs[(size_t) b.knob].order;
        return oa != ob ? oa < ob : a.knob < b.knob;
    });

    sound.changes = kept;

    if (dropped > 0)
        sound.notes.add (dropped == 1 ? "1 suggestion named a knob this synth doesn't have and was left out."
                                      : juce::String (dropped) + " suggestions named knobs this synth doesn't have and were left out.");

    for (auto& c : sound.changes)
        if (c.note.isNotEmpty())
            sound.notes.add (c.note);

    if (result.droppedGroup > 0)
        sound.notes.add (juce::String (result.droppedGroup) + (result.droppedGroup == 1 ? " bar change" : " bar changes")
                         + " beyond " + juce::String (maxGroupMembers) + " " + (result.droppedGroup == 1 ? "was" : "were") + " left out.");

    sound.notes.addArray (t.wordNotes (sound.words, sound.followUp, sound.baseValues, getResultValues (sound)));
    sound.notes.addArray (t.limitNotes (sound.words));
    result.ok = true;
    return result;
}

int countChanges (const Table& t, const DescribedSound& sound)
{
    std::set<int> groups;
    int count = 0;

    for (auto& c : sound.changes)
    {
        const int group = t.knobs[(size_t) c.knob].group;

        if (group < 0)
            ++count;
        else
            groups.insert (group);
    }

    return count + (int) groups.size();
}

Values getResultValues (const DescribedSound& sound)
{
    auto v = sound.baseValues;

    for (auto& c : sound.changes)
        if ((size_t) c.knob < v.size())
            v[(size_t) c.knob] = c.after;

    return v;
}

//==============================================================================
bool areSame (const Table& t, const Values& a, const Values& b)
{
    // A starting point's value may sit between two steps of the knob (a bar of -9.54 dB, a step of 0.1), and the synth then
    // holds the rounded value: within half a step is the same sound.
    for (int k = 0; k < t.getNumKnobs(); ++k)
    {
        if ((size_t) k >= a.size() || (size_t) k >= b.size())
            return false;

        const float tolerance = t.knobs[(size_t) k].info.step * 0.5f + 1.0e-4f;

        if (std::abs (a[(size_t) k] - b[(size_t) k]) > tolerance && ! nearlyEqual (a[(size_t) k], b[(size_t) k]))
            return false;
    }

    return true;
}

bool Walkthrough::isReached (const Table& t, int knob, float value, float target)
{
    const auto& desc = t.knobs[(size_t) knob];

    if (desc.info.isChoice)
        return juce::roundToInt (value) == juce::roundToInt (target);

    if (desc.offExact && desc.hasOff && std::abs (target - desc.offValue) < 0.05f)
        return std::abs (value - desc.offValue) < 0.05f;

    return std::abs (getTravel (desc.info, value) - getTravel (desc.info, target)) <= reachedFraction + 1.0e-6;
}

Walkthrough::Walkthrough (const Table& t, const DescribedSound& sound, const Values& live)
    : table (t), baseValues (sound.baseValues)
{
    if (! sound.followUp && ! areSame (t, sound.baseValues, live))
    {
        Step s;
        s.loadsBase = true;
        s.reason = "Load the starting point " + sound.base;
        steps.push_back (s);
    }

    for (auto& c : sound.changes)
    {
        const int group = t.knobs[(size_t) c.knob].group;

        if (group >= 0 && ! steps.empty() && steps.back().group == group)
        {
            auto& s = steps.back();
            s.members.push_back (c);

            if (s.reason.isEmpty())
                s.reason = c.reason;

            if (s.note.isEmpty())
                s.note = c.note;

            continue;
        }

        Step s;
        s.knob = c.knob;
        s.before = c.before;
        s.target = c.after;
        s.group = group;
        s.members.push_back (c);
        s.reason = c.reason;
        s.note = c.note;
        steps.push_back (s);
    }

    begin (live);
}

void Walkthrough::begin (const Values& live)
{
    if (index < getNumSteps())
    {
        steps[(size_t) index].snapshot = live;
        steps[(size_t) index].taken = true;
    }
}

void Walkthrough::advance (const Values& live)
{
    ++index;
    begin (live);
}

bool Walkthrough::isCurrentReached (const Values& live) const
{
    if (isFinished() || steps[(size_t) index].loadsBase)
        return false;

    for (auto& m : steps[(size_t) index].members)
        if ((size_t) m.knob >= live.size() || ! isReached (table, m.knob, live[(size_t) m.knob], m.after))
            return false;

    return true;
}

void Walkthrough::markDone (const Values& live)
{
    if (! isFinished())
        advance (live);
}

void Walkthrough::setForUser (Values& live)
{
    if (isFinished())
        return;

    auto& s = steps[(size_t) index];

    if (s.loadsBase)
        live = baseValues;
    else
        for (auto& m : s.members)
            if ((size_t) m.knob < live.size())
                live[(size_t) m.knob] = m.after;

    advance (live);
}

void Walkthrough::skip (const Values& live)
{
    if (! isFinished())
        advance (live);
}

void Walkthrough::back (Values& live)
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
            for (auto& m : s.members)
                if ((size_t) m.knob < live.size() && (size_t) m.knob < s.snapshot.size())
                    live[(size_t) m.knob] = s.snapshot[(size_t) m.knob];
    }

    s.snapshot = live;
    s.taken = true;
}

void Walkthrough::finishAll (Values& live)
{
    while (! isFinished())
        setForUser (live);
}

//==============================================================================
void ValuesUndo::push (const Values& v)
{
    list.push_back (v);

    if ((int) list.size() > maxSteps)
        list.erase (list.begin());
}

bool ValuesUndo::pop (Values& result)
{
    if (list.empty())
        return false;

    result = list.back();
    list.pop_back();
    return true;
}
} // namespace daw::gen::sounddescribe
