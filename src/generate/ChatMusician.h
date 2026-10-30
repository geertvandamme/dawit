#pragma once

#include "engine/Clip.h"
#include "engine/Chord.h"

namespace daw::gen::chatmusician
{
/** A section of a clip to generate in one model request, in beats relative to the clip. */
struct Chunk
{
    double start = 0.0;
    double length = 0.0;
};

/** Divide a whole-bar clip into requests of at most four bars; reject partial bars. */
std::vector<Chunk> splitRange (double lengthBeats);

/** Join a chunk's notes to a take. Reject notes outside the chunk without modifying the take. */
bool appendChunk (std::vector<MidiNote>& destination, const std::vector<MidiNote>& notes,
                  const Chunk& chunk, double clipLength);

/** Build the constrained response schema for a whole-bar request. */
juce::var buildSchema (int bars);

/** Build the model's raw composition prompt with the supplied chords. */
juce::String buildPrompt (const MusicalKey& key, const std::vector<Chord>& chords);

/** Validate a complete response and produce four quarter notes per bar at the given beat offset. */
bool parseReply (const juce::String& reply, int bars, double offset, std::vector<MidiNote>& notes, juce::String& error);

/** Select the chord names available in a key from explicitly enabled harmonic choices. */
juce::StringArray chordVocabulary (const MusicalKey& key, bool sevenths, bool borrowed, bool majorDominant);

/** Build a schema with a separate allowed chord list for each bar. */
juce::var buildChordSchema (const std::vector<juce::StringArray>& allowed);

/** Ask for a progression with the given allowed names and any fixed first or last chord. */
juce::String buildChordPrompt (const MusicalKey& key, const std::vector<juce::StringArray>& allowed,
                               const juce::String& previous);

/** Check a complete chord response against every bar's allowed names and adjacent repetition. */
bool parseChords (const juce::String& reply, const std::vector<juce::StringArray>& allowed,
                  const juce::String& previous, std::vector<Chord>& chords, juce::String& error);

/** Voice each chord as a four-beat MIDI chord at the requested offset. */
bool voiceChords (const std::vector<Chord>& chords, double offset, std::vector<MidiNote>& notes);

/** Request and validate consecutive chord chunks, retrying invalid replies at most twice. */
bool generateChords (const MusicalKey& key, const std::vector<juce::StringArray>& allowed,
                     const std::function<juce::String (const juce::String&, const juce::var&, int, juce::String&)>& request,
                     const std::function<bool()>& cancelled, std::vector<Chord>& chords, juce::String& error);

/** Request a whole-bar melody in chunks; leave notes unchanged if any request fails or is cancelled. */
bool generateMelody (const MusicalKey& key, const std::vector<Chord>& chords,
                     const std::function<juce::String (const juce::String&, const juce::var&, juce::String&)>& request,
                     const std::function<bool()>& cancelled, std::vector<MidiNote>& notes, juce::String& error);
} // namespace daw::gen::chatmusician
