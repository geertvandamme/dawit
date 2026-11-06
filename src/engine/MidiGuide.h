#pragma once

#include "Clip.h"
#include "MusicalKey.h"

#include <juce_audio_basics/juce_audio_basics.h>

/**
    What MIDI is, for the "What is MIDI?" guide in the piano roll: instructions, not sound, from the
    cable to the bytes and the file. The topics and their texts, the numbers the pictures show (bytes,
    note numbers and their pitch, the time a message takes on the cable), the clip's notes as the
    messages they're sent as, and a look at the clip in the piano roll.
*/
namespace daw::midiguide
{
enum class Topic { whatItIs, history, cable, messages, noteNumbers, velocity, computerKeyboard, controllers, sustainPedal,
                   pitchBend, modExpressionVolume, time, files, retrograde, inversion, newNotes, yourClip };

inline constexpr Topic allTopics[] = { Topic::whatItIs, Topic::history, Topic::cable, Topic::messages, Topic::noteNumbers,
                                       Topic::velocity, Topic::computerKeyboard, Topic::controllers, Topic::sustainPedal,
                                       Topic::pitchBend, Topic::modExpressionVolume, Topic::time, Topic::files,
                                       Topic::retrograde, Topic::inversion, Topic::newNotes, Topic::yourClip };

juce::String getTopicName (Topic);
juce::String getTopicSummary (Topic);

/** What "Your clip" needs to know about the song around the clip, for its paragraph on controllers: where the clip
    sits, the song's chord changes under it, and what the track's instrument does with controllers. */
struct ControllerContext
{
    /** What plays the clip: DawIt's built-in synth or one of the four DawIt synths (pedal, bend, volume and
        expression), the built-in synth's drum sounds (volume and expression only), DawIt Drums (none of them),
        a plugin (up to it), or not known. */
    enum class Instrument { unknown, builtIn, builtInPercussion, dawItSynth, drums, plugin };

    /** A chord starting `beat` beats from the clip's start (negative or 0 for the one playing at its start). */
    struct Chord
    {
        double beat = 0.0;
        juce::String name;
    };

    double clipStartBeat = 0.0;       // where the clip starts in the song, so bars are the song's
    std::vector<Chord> chords;        // in time order; empty when the song has no chords here
    Instrument instrument = Instrument::unknown;
    juce::String instrumentName;      // "DawIt Subtractive", or the plugin's name
};

/** The text of a topic. "Your clip" looks at `clip` (null when no MIDI clip is in the piano roll)
    at the song's tempo and in its key, and says only what fits when it plays a drum sound (`drumSound`),
    with the song's chords and instrument for what its controllers do (`context`);
    "Time" uses the tempo for its numbers. */
juce::String explain (Topic, const MidiClip* clip, double bpm, const MusicalKey& = {}, bool drumSound = false,
                      const ControllerContext& context = {});

/** The advice on the clip's controllers, a paragraph for "Your clip" (it starts with "Controllers: "); empty for a
    clip without notes. Says there are none, how often and for how long the pedal is down, the bend's range, a pedal
    held through a chord change, a pedal or bend still away from rest at the end, a volume or expression at 0 under a
    note, and the lanes the instrument ignores. */
juce::String explainControllers (const MidiClip&, const ControllerContext&);

/** The three examples of the controller topics, as clips: four chords with the pedal changed at each (16 beats), a
    guitar-style bend on A4 (4 beats) and a string chord with an expression swell (8 beats). */
MidiClip getSustainExample();
MidiClip getBendExample();
MidiClip getSwellExample();

/** The semitones a pitch bend value means at the usual range of 2: (value - 8192) / 8192 * 2. */
double getBendSemitones (int value);

/** The level, in dB, that volume and expression together give by the General MIDI curve: 40 log10 (volume / 127) plus
    40 log10 (expression / 127); -100 for 0. */
double getGainDb (int volume, int expression);

/** The example of the three topics on turning a melody around: the start of Frère Jacques (bars 1 and 3,
    public domain), as notes of an 8-beat clip in C major. */
std::vector<MidiNote> getTurnAroundExample();
inline constexpr double turnAroundExampleLength = 8.0;

//==============================================================================
// The numbers

/** Bits per second on a MIDI cable (the 5-pin DIN socket). */
inline constexpr int cableBitsPerSecond = 31250;

/** Each byte goes over the cable as 10 bits: a start bit, its 8 bits and a stop bit. */
inline constexpr int bitsPerByteOnCable = 10;

/** How long `numBytes` take on a MIDI cable: a three-byte Note On takes 0.96 ms. */
double getCableSeconds (int numBytes);

/** Clock messages per quarter note, for keeping drum machines and sequencers in time. */
inline constexpr int clockPulsesPerQuarter = 24;

/** The centre of pitch bend's 14 bits (0 to 16383): no bend. */
inline constexpr int pitchBendCentre = 8192;

/** The lowest and highest key of an 88-key piano: A0 and C8. */
inline constexpr int pianoLowest = 21, pianoHighest = 108;

/** The pitch of a note number in Hz, with note 69 (A4) at 440 Hz: f = 440 * 2^((n - 69) / 12). */
double getFrequency (int noteNumber);

/** "C4", with note 60 as C4 (some makers call it C3). */
juce::String getNoteName (int noteNumber);

/** The well-known controller numbers and what they do, for the controllers topic. */
struct Controller
{
    int number = 0;
    juce::String name, what;
};

const std::vector<Controller>& getCommonControllers();

//==============================================================================
// Messages

/** "90 3C 64": the bytes in hexadecimal. */
juce::String toHex (const juce::MidiMessage&);

/** "1001 0000": a byte in binary, in two groups of four. */
juce::String toBinary (juce::uint8);

/** The message in words: "Note On, channel 1, C4 (note 60), velocity 100". */
juce::String describe (const juce::MidiMessage&);

/** A message of a clip, at its beat from the clip's start. */
struct Event
{
    double beat = 0.0;
    juce::MidiMessage message;
};

/** The clip's notes and controller events as the messages they're played as, on channel 1, in time order:
    notes past the clip's end are left out and notes ringing past it end there, as in playback; events past the
    end are left out, and the pedal, bend, mod wheel, expression or pressure still away from rest at the end
    gets a reset message there. At the same moment the Note Offs go first, then controllers, then Note Ons. */
std::vector<Event> getEvents (const MidiClip&);

/** How many bytes the events take on a cable: 3 a message, or with running status 2 for a message
    with the same status byte as the one before. */
int countBytes (const std::vector<Event>&, bool runningStatus);

//==============================================================================
// Files

/** A part of a Standard MIDI File, for drawing it: where it is, how long, and what it says. */
struct FilePart
{
    int offset = 0, length = 0;
    juce::String name, value;
};

/** The bytes of the file DawIt exports for the clip (a type 1 file, as dragging it out writes), at
    the song's tempo and in its key. */
juce::MemoryBlock getClipFile (const MidiClip&, double bpm, const MusicalKey&, const juce::String& trackName);

/** The header chunk and the start of the first track of a Standard MIDI File, named part by part:
    "MThd", its length, the format (0, 1 or 2), the number of tracks, the ticks per quarter note,
    then "MTrk" and its length. */
std::vector<FilePart> describeFile (const juce::MemoryBlock&);

//==============================================================================
// The clip

/** What the clip's velocities are: how many notes, the lowest, highest and average (1 to 127). */
struct VelocityStats
{
    int count = 0, lowest = 0, highest = 0;
    double average = 0.0;
    int distinct = 0;   // how many different velocities
};

VelocityStats getVelocityStats (const MidiClip&);

/** The MIDI velocity (1 to 127) of a note's velocity (0 to 1), as playback sends it. */
int toMidiVelocity (float velocity);
} // namespace daw::midiguide
