#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <vector>

#include "Tune.h"

/**
    The rules of DawIt Harmoniser with no audio in them, so the processor, the guide's pictures and the
    tests all use the same numbers: how far a voice moves a sung note (a fixed number of semitones, or a
    number of steps along the key's scale), the names of intervals, the mix and pan gains, the starting
    points, and what the voices made of a take ("Your track").
*/
namespace daw::harm
{
constexpr int numVoices = 2;

/** Fixed moves every note by the same number of semitones; Key moves it a number of steps along the scale. */
enum class Mode { fixed, key };

/** The scale of a key chosen in the effect (with Song, the song's own scale is used). */
enum class Scale { major, minor };

/** The Key choice: 0 follows the song; 1 to 12 are C to B. */
constexpr int keyFromSong = 0;

/** The range of Level in dB: the bottom of the range is "off" (no sound at all). */
constexpr float minLevelDb = -30.0f, maxLevelDb = 6.0f;

/** One of the two voices. */
struct Voice
{
    bool on = false;
    int semitones = 4;          // the interval in Fixed, -12 to +12
    int steps = 2;              // the interval in Key, -7 to +7 scale steps
    float levelDb = -6.0f;
    float panPercent = 0.0f;    // -100 left to +100 right
    float detuneCents = 0.0f;   // -50 to +50
    float delayMs = 0.0f;       // 0 to 60
};

/** The controls of DawIt Harmoniser. The defaults are the starting point Third above. */
struct Settings
{
    Mode mode = Mode::key;
    int key = keyFromSong;
    Scale scale = Scale::major;
    float mixPercent = 50.0f;
    std::array<Voice, numVoices> voices { Voice { true, 4, 2, -6.0f, 0.0f, 0.0f, 0.0f }, Voice { false, 7, -2, -6.0f, 0.0f, 0.0f, 0.0f } };
};

/** A starting point and why. */
struct Preset
{
    juce::String name, why;
    Settings settings;
};

/** The six starting points; the first, Third above, is the default. */
const std::vector<Preset>& getPresets();

//==============================================================================
// Keys and shifts

/** The notes Key harmony follows, as pitch-class bits (bit 0 = C). A key chosen in the effect wins over the song's `songMask`.
    A mask without exactly 7 notes (no key, or a scale with another size) gives C major. */
int getKeyMask (int keyChoice, Scale, int songMask) noexcept;

/** The notes of a mask from C upwards (pitch classes 0 to 11). */
std::vector<int> getMaskNotes (int mask);

/** The MIDI note `steps` scale steps from `note` (which should be in the mask: 7 steps is an octave). */
int stepInKey (int note, int steps, int mask);

/** The scale note a sung pitch is measured from: the nearest note of the mask, ties to the lower, kept until
    the pitch is 15 cents past the halfway point (tune::chooseTarget); `previous` is -1 for none. */
int getReference (double sungMidi, int mask, int previous);

/** How far a voice moves a sung note, in semitones: its interval in Fixed, in Key the distance from the reference
    note to the note `steps` along the scale. Never more than an octave either way. The shift is added to the sung
    pitch, so the voice keeps the lead's cents. */
double getShift (const Voice&, Mode, int reference, int mask);

/** The same from the sung pitch (the reference is found with `previous`, and given back for the next call). */
double getShift (double sungMidi, const Voice&, Mode, int mask, int& previousReference);

/** The factor the voice's frequency is multiplied by: the shift in semitones plus the voice's detune. */
double getRatio (double shiftSemitones, float detuneCents) noexcept;

//==============================================================================
// Names and gains

/** "major 3rd up", "octave down", "unison" for Fixed (semitones); "3rd up", "6th down", "octave up" for Key (steps). */
juce::String getIntervalName (Mode, int value);

/** The size of an interval alone, 0 to 12 semitones: "minor 3rd", "tritone", "octave". */
juce::String getSizeName (int semitones);

/** "C", "C#" ... "B". */
juce::String getNoteName (int pitchClass);

/** The linear gain of a level in dB; the bottom of the range, -30 dB, is silence. */
float getLevelGain (float levelDb) noexcept;

/** How loud the lead (dry) and the voices (wet) are at a Mix in percent: at 50% both are at full level, at 0% only
    the lead, at 100% only the voices. */
std::pair<float, float> getMixGains (float mixPercent) noexcept;

/** The left and right gains of a constant-power pan, -100 (left) to +100 (right): 0.7071 each at the centre. */
std::pair<float, float> getPanGains (float panPercent) noexcept;

//==============================================================================
// What was heard

/** What the processor heard at one moment, for the picture and for "Your track". */
struct Frame
{
    float beat = -1.0f;                  // where in the song, or -1 while the song isn't playing
    float lead = -1.0f;                  // the lead's pitch as a fractional MIDI note, or -1 for no clear pitch
    std::array<float, numVoices> voice { -1.0f, -1.0f };   // each voice's note (the lead's plus the shift), or -1: off or quiet
    float rmsDb = -100.0f;               // how loud the sound was
};

/** How the song's key is known to "Your track": whether, its notes as bits, and its name ("C major"). */
struct KeyInfo
{
    bool known = false;
    int mask = 0;
    juce::String name;
};

/** What the voices made of a take: a few short paragraphs, as the plan's rules give them. Voices need two seconds
    of clear notes before they are judged. `framesPerSecond` is the pace of the frames (172 at 44.1 kHz).
    `latencyLine` is the lead line about a bus, return or instrument track, or empty. */
std::vector<juce::String> getAdvice (const std::vector<Frame>&, const Settings&, const KeyInfo&, double framesPerSecond, const juce::String& latencyLine);

/** The sentence about being late on a bus, return or instrument track; `trackKind` is "bus", "return" or "instrument track". `lateMs` is the
    latency at the sample rate in use (29 ms at 44.1 kHz, 27 at 48 kHz); 0 when the rate isn't known says "about 27 ms". */
juce::String getLatencyLine (const juce::String& trackKind, double lateMs = 0.0);

//==============================================================================
// Made-up takes

/** The singing guide's phrase "hold on, hold on to-night" in C major: E4 E4 G4 E4 D4 C4 at 90 BPM, beat times in seconds. */
std::vector<tune::TakeNote> getPhraseNotes();
constexpr double phraseSeconds = 8.0 * 60.0 / 90.0;
}
