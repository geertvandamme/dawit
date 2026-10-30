#include "ChatMusician.h"
#include "Harmony.h"

#include <cmath>

namespace daw::gen::chatmusician
{
juce::StringArray chordVocabulary (const MusicalKey& key, bool sevenths, bool borrowed, bool majorDominant)
{
    juce::StringArray result;
    for (int degree = 1; degree <= 7; ++degree)
    {
        const auto triad = harmony::triad (key, degree);
        result.addIfNotAlreadyThere (triad.getName());
        if (sevenths)
            result.addIfNotAlreadyThere (Chord { triad.root, getChordIntervals (key, degree, "seventh") }.getName());
    }
    if (borrowed)
    {
        auto parallel = key;
        parallel.scale = harmony::isMinorKey (key) ? MusicalKey::Scale::major : MusicalKey::Scale::minor;
        for (int degree = 1; degree <= 7; ++degree)
            result.addIfNotAlreadyThere (harmony::triad (parallel, degree).getName());
    }
    if (majorDominant)
        if (const auto dominant = harmony::getBorrowedDominant (key))
        {
            result.addIfNotAlreadyThere (dominant->getName());
            if (sevenths)
                result.addIfNotAlreadyThere (Chord { dominant->root, { 0, 4, 7, 10 } }.getName());
        }
    for (int index = result.size(); --index >= 0;)
        if (! Chord::fromName (result[index]).has_value())
            result.remove (index);
    return result;
}

juce::var buildChordSchema (const std::vector<juce::StringArray>& allowed)
{
    auto* properties = new juce::DynamicObject();
    juce::Array<juce::var> required;
    for (int bar = 0; bar < (int) allowed.size(); ++bar)
    {
        auto* field = new juce::DynamicObject();
        juce::Array<juce::var> choices;
        for (const auto& chord : allowed[(size_t) bar]) choices.add (chord);
        field->setProperty ("type", "string");
        field->setProperty ("enum", juce::var (choices));
        const auto name = "bar" + juce::String (bar + 1);
        properties->setProperty (juce::Identifier (name), juce::var (field));
        required.add (name);
    }
    auto* schema = new juce::DynamicObject();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (properties));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema);
}

juce::String buildChordPrompt (const MusicalKey& key, const std::vector<juce::StringArray>& allowed,
                               const juce::String& previous)
{
    juce::String prompt = "Human: Compose a coherent " + juce::String ((int) allowed.size())
                          + "-bar chord progression in " + key.getName() + ". ";
    if (! allowed.empty() && allowed.front().size() == 1)
        prompt << "Start on " << allowed.front()[0] << ". ";
    if (allowed.size() > 1 && allowed.back().size() == 1)
        prompt << "End on " << allowed.back()[0] << ". ";
    if (previous.isNotEmpty())
        prompt << "The previous chord is " << previous << ". ";
    prompt << "Do not repeat adjacent chords. Return only JSON with ";
    juce::StringArray names;
    for (int bar = 0; bar < (int) allowed.size(); ++bar) names.add ("bar" + juce::String (bar + 1));
    prompt << names.joinIntoString (", ") << " as chord names. </s> Assistant:";
    return prompt;
}

bool parseChords (const juce::String& reply, const std::vector<juce::StringArray>& allowed,
                  const juce::String& previous, std::vector<Chord>& chords, juce::String& error)
{
    error.clear();
    const int bars = (int) allowed.size();
    if (bars < 1 || bars > 4) { error = "Invalid chord range."; return false; }
    int pos = 0;
    auto space = [&] { while (pos < reply.length() && juce::CharacterFunctions::isWhitespace (reply[pos])) ++pos; };
    auto string = [&] (juce::String& out)
    {
        if (pos >= reply.length() || reply[pos++] != '"') return false;
        const int begin = pos;
        while (pos < reply.length() && reply[pos] != '"')
        {
            if (reply[pos] == '\\' || reply[pos] < 32) return false;
            ++pos;
        }
        if (pos >= reply.length()) return false;
        out = reply.substring (begin, pos++);
        return true;
    };
    auto fail = [&] { error = "The model returned an incomplete or invalid chord progression."; return false; };
    juce::StringArray keys, values;
    space();
    if (pos >= reply.length() || reply[pos++] != '{') return fail();
    for (int i = 0; i < bars; ++i)
    {
        space(); juce::String name, value;
        if (! string (name) || keys.contains (name)) return fail();
        space(); if (pos >= reply.length() || reply[pos++] != ':') return fail();
        space(); if (! string (value)) return fail();
        keys.add (name); values.add (value);
        space(); if (pos >= reply.length() || reply[pos++] != (i == bars - 1 ? '}' : ',')) return fail();
    }
    space(); if (pos != reply.length()) return fail();
    std::vector<Chord> staged;
    juce::String last = previous;
    for (int bar = 0; bar < bars; ++bar)
    {
        const int index = keys.indexOf ("bar" + juce::String (bar + 1));
        if (index < 0 || ! allowed[(size_t) bar].contains (values[index]) || values[index] == last) return fail();
        const auto chord = Chord::fromName (values[index]);
        if (! chord.has_value()) return fail();
        staged.push_back (*chord);
        last = values[index];
    }
    chords = std::move (staged);
    return true;
}

bool voiceChords (const std::vector<Chord>& chords, double offset, std::vector<MidiNote>& notes)
{
    if (! std::isfinite (offset) || offset < 0.0) return false;
    std::vector<int> previous;
    std::vector<MidiNote> staged;
    for (size_t bar = 0; bar < chords.size(); ++bar)
    {
        const auto voiced = voiceChord (chords[bar].getPitchClasses(), previous);
        if (voiced.empty()) return false;
        for (int pitch : voiced) staged.push_back ({ pitch, offset + 4.0 * (double) bar, 4.0, 0.8f });
        previous = voiced;
    }
    notes = std::move (staged);
    return true;
}

bool generateChords (const MusicalKey& key, const std::vector<juce::StringArray>& allowed,
                     const std::function<juce::String (const juce::String&, const juce::var&, int, juce::String&)>& request,
                     const std::function<bool()>& cancelled, std::vector<Chord>& chords, juce::String& error)
{
    error.clear();
    if (allowed.empty() || ! request) { error = "No chords or model were selected."; return false; }
    std::vector<Chord> staged;
    juce::String previous;
    for (size_t start = 0; start < allowed.size(); start += 4)
    {
        const auto end = juce::jmin (start + 4, allowed.size());
        std::vector<juce::StringArray> choices (allowed.begin() + (std::ptrdiff_t) start,
                                                allowed.begin() + (std::ptrdiff_t) end);
        std::vector<Chord> chunk;
        bool valid = false;
        for (int attempt = 0; attempt < 3 && ! valid; ++attempt)
        {
            if (cancelled && cancelled()) { error = "Cancelled."; return false; }
            error.clear();
            const auto reply = request (buildChordPrompt (key, choices, previous), buildChordSchema (choices), attempt, error);
            if (cancelled && cancelled()) { error = "Cancelled."; return false; }
            if (error.isNotEmpty()) return false;
            valid = parseChords (reply, choices, previous, chunk, error);
        }
        if (! valid) { error = "Could not get a valid chord progression after three attempts: " + error; return false; }
        staged.insert (staged.end(), chunk.begin(), chunk.end());
        previous = chunk.back().getName();
    }
    chords = std::move (staged);
    return true;
}

bool generateMelody (const MusicalKey& key, const std::vector<Chord>& chords,
                     const std::function<juce::String (const juce::String&, const juce::var&, juce::String&)>& request,
                     const std::function<bool()>& cancelled, std::vector<MidiNote>& notes, juce::String& error)
{
    error.clear();
    if (chords.empty() || ! request) { error = "Choose chords for the whole range and a model."; return false; }
    std::vector<MidiNote> staged;
    for (size_t start = 0; start < chords.size(); start += 4)
    {
        if (cancelled && cancelled()) { error = "Cancelled."; return false; }
        const auto end = juce::jmin (start + 4, chords.size());
        const std::vector<Chord> chunkChords (chords.begin() + (std::ptrdiff_t) start, chords.begin() + (std::ptrdiff_t) end);
        const auto reply = request (buildPrompt (key, chunkChords), buildSchema ((int) chunkChords.size()), error);
        if (cancelled && cancelled()) { error = "Cancelled."; return false; }
        if (error.isNotEmpty()) return false;
        std::vector<MidiNote> chunk;
        if (! parseReply (reply, (int) chunkChords.size(), 4.0 * (double) start, chunk, error)) return false;
        staged.insert (staged.end(), chunk.begin(), chunk.end());
    }
    notes = std::move (staged);
    return true;
}

juce::var buildSchema (int bars)
{
    auto* properties = new juce::DynamicObject();
    juce::Array<juce::var> required;
    for (int bar = 1; bar <= bars; ++bar)
    {
        const auto name = "bar" + juce::String (bar);
        auto* value = new juce::DynamicObject();
        value->setProperty ("type", "string");
        value->setProperty ("pattern", "^[A-Ga-g][,']? [A-Ga-g][,']? [A-Ga-g][,']? [A-Ga-g][,']?$");
        properties->setProperty (juce::Identifier (name), juce::var (value));
        required.add (name);
    }
    auto* schema = new juce::DynamicObject();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (properties));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema);
}

juce::String buildPrompt (const MusicalKey& key, const std::vector<Chord>& chords)
{
    juce::String prompt = "Human: Compose a simple " + juce::String ((int) chords.size()) + "-bar melody in "
                          + key.getName() + ", in 4/4.\n";
    juce::StringArray names;
    for (int bar = 0; bar < (int) chords.size(); ++bar)
    {
        prompt << "Bar " << bar + 1 << ": " << chords[(size_t) bar].getName() << ". ";
        names.add ("bar" + juce::String (bar + 1));
    }
    prompt << "\nReturn JSON with " << names.joinIntoString (", ")
           << ". Each value contains exactly four ABC quarter-note pitches separated by spaces. "
              "Use chord tones and smooth melodic motion. Do not include chord names or bar lines inside the values. </s> Assistant:";
    return prompt;
}

bool parseReply (const juce::String& reply, int bars, double offset, std::vector<MidiNote>& notes, juce::String& error)
{
    error.clear();
    if (bars <= 0 || bars > 4 || ! std::isfinite (offset) || offset < 0.0)
    {
        error = "Invalid range for this request.";
        return false;
    }
    // JUCE's JSON reader accepts trailing text and duplicate keys. Check the small expected
    // object grammar first so neither can silently turn an incomplete take into a valid one.
    int position = 0;
    auto skipSpace = [&] { while (position < reply.length() && juce::CharacterFunctions::isWhitespace (reply[position])) ++position; };
    auto readString = [&] (juce::String& value)
    {
        if (position >= reply.length() || reply[position++] != '"') return false;
        const int start = position;
        while (position < reply.length() && reply[position] != '"')
        {
            if (reply[position] == '\\' || reply[position] < 32) return false;
            ++position;
        }
        if (position >= reply.length()) return false;
        value = reply.substring (start, position++);
        return true;
    };
    auto failJson = [&] { error = "The model did not return a complete JSON object."; return false; };
    skipSpace();
    if (position >= reply.length() || reply[position++] != '{') return failJson();
    juce::StringArray keys, values;
    for (int index = 0; index < bars; ++index)
    {
        skipSpace();
        juce::String name, value;
        if (! readString (name) || keys.contains (name)) return failJson();
        skipSpace();
        if (position >= reply.length() || reply[position++] != ':') return failJson();
        skipSpace();
        if (! readString (value)) return failJson();
        keys.add (name);
        values.add (value);
        skipSpace();
        if (position >= reply.length() || reply[position++] != (index == bars - 1 ? '}' : ',')) return failJson();
    }
    skipSpace();
    if (position != reply.length()) return failJson();

    if (keys.size() != bars)
    {
        error = "The model did not return a complete JSON object.";
        return false;
    }
    std::vector<MidiNote> staged;
    constexpr int pitchClasses[] = { 9, 11, 0, 2, 4, 5, 7 };
    for (int bar = 0; bar < bars; ++bar)
    {
        const auto name = "bar" + juce::String (bar + 1);
        const int index = keys.indexOf (name);
        if (index < 0)
        {
            error = "The reply is missing " + name + ".";
            return false;
        }
        const auto value = values[index];
        juce::StringArray tokens;
        tokens.addTokens (value, " ", "");
        if (tokens.size() != 4 || tokens.joinIntoString (" ") != value)
        {
            error = name + " must contain four pitches separated by single spaces.";
            return false;
        }
        for (int beat = 0; beat < 4; ++beat)
        {
            const auto token = tokens[beat];
            const auto letter = token[0];
            const auto upper = juce::CharacterFunctions::toUpperCase (letter);
            if (upper < 'A' || upper > 'G' || token.length() < 1 || token.length() > 2
                || (token.length() == 2 && token[1] != ',' && token[1] != '\''))
            {
                error = "Unsupported pitch in " + name + ": " + token;
                return false;
            }
            int pitch = 60 + pitchClasses[(int) (upper - 'A')];
            if (letter >= 'a' && letter <= 'g') pitch += 12;
            if (token.length() == 2) pitch += token[1] == '\'' ? 12 : -12;
            if (pitch < 0 || pitch > 127)
            {
                error = "A pitch is outside the MIDI range.";
                return false;
            }
            staged.push_back ({ pitch, offset + 4.0 * bar + beat, 1.0, 0.8f });
        }
    }
    notes = std::move (staged);
    return true;
}

std::vector<Chunk> splitRange (double lengthBeats)
{
    std::vector<Chunk> chunks;

    if (! std::isfinite (lengthBeats) || lengthBeats <= 0.0 || std::abs (lengthBeats / 4.0 - std::round (lengthBeats / 4.0)) > 1.0e-9)
        return chunks;

    for (double start = 0.0; start < lengthBeats;)
    {
        const double length = juce::jmin (16.0, lengthBeats - start);
        if (length <= 0.0 || start + length <= start)
            return {};

        chunks.push_back ({ start, length });
        start += length;
    }

    return chunks;
}

bool appendChunk (std::vector<MidiNote>& destination, const std::vector<MidiNote>& notes,
                  const Chunk& chunk, double clipLength)
{
    if (! std::isfinite (chunk.start) || ! std::isfinite (chunk.length) || ! std::isfinite (clipLength)
        || chunk.start < 0.0 || chunk.length <= 0.0 || clipLength <= 0.0 || chunk.start >= clipLength)
        return false;

    std::vector<MidiNote> staged;
    const double end = juce::jmin (chunk.length, clipLength - chunk.start);

    for (auto note : notes)
    {
        if (note.pitch < 0 || note.pitch > 127 || ! std::isfinite (note.start) || ! std::isfinite (note.length)
            || ! std::isfinite (note.velocity) || note.start < 0.0 || note.length <= 0.0)
            return false;

        if (note.start >= end || note.length > end - note.start + 1.0e-9)
            return false;
        note.start += chunk.start;
        staged.push_back (note);
    }

    destination.insert (destination.end(), staged.begin(), staged.end());
    return true;
}
} // namespace daw::gen::chatmusician
