#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <optional>

namespace daw
{
/** The sounds of the built-in synth, which plays on MIDI tracks that have no instrument plugin. */
enum class BuiltInSound { piano, organ, bass, lead, pluck, pad, drums, percussion };

namespace builtInSounds
{
    constexpr std::array<BuiltInSound, 8> all { BuiltInSound::piano, BuiltInSound::organ, BuiltInSound::bass,
                                                BuiltInSound::lead,  BuiltInSound::pluck, BuiltInSound::pad,
                                                BuiltInSound::drums, BuiltInSound::percussion };

    /** True for the sounds that play the General MIDI drum map (one drum or percussion
        instrument per note) instead of pitches. */
    constexpr bool isPercussive (BuiltInSound s) { return s == BuiltInSound::drums || s == BuiltInSound::percussion; }

    /** Shown in the UI. */
    juce::String getName (BuiltInSound);

    /** One line about the sound, for the synth window. */
    juce::String getDescription (BuiltInSound);

    /** Stable id for project files. */
    juce::String getId (BuiltInSound);
    std::optional<BuiltInSound> fromId (const juce::String&);
} // namespace builtInSounds

/**
    The built-in synth: a small synthesiser (no samples) with one sound at a time, picked from
    BuiltInSound. Notes that are already sounding keep their sound when the choice changes.
    Drums and Percussion both follow the General MIDI drum map, so either plays the kit (notes 35-59)
    and the hand percussion (60-82): the kit sound is for drum parts, the percussion sound for
    percussion parts. They ignore note-offs, and a closed hi-hat cuts off an open one (a muted
    triangle an open one). The sound is read from `sound` at each note-on, so the UI can change it at any time.
*/
class BuiltInSynth final : public juce::Synthesiser
{
public:
    explicit BuiltInSynth (const std::atomic<BuiltInSound>& sound);

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override;

    static constexpr int numVoices = 24;

private:
    const std::atomic<BuiltInSound>& selectedSound;
};
} // namespace daw
