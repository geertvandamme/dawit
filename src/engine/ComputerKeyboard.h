#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace daw
{
/**
    The state of playing notes with the computer's letter keys, when there is no MIDI keyboard: no UI
    and no sound, only the rules. A S D F G H J K L are the white keys (C D E F G A B C D), W E T Y U O P
    the black ones, Z and X move the octave and C and V the velocity. It turns key presses into the
    MIDI messages a keyboard would send (channel 1), and remembers which note each held key started, so
    a key that comes up always stops its own note, even if the octave changed meanwhile.

    Only the message thread uses it.
*/
class ComputerKeyboard
{
public:
    /** The sixteen playing keys, from C up to the D an octave above. */
    static constexpr int numKeys = 16;

    /** The octave: the note the A key plays, from C0 (12) to C8 (108) in steps of 12. */
    static constexpr int lowestOctaveNote = 12, highestOctaveNote = 108, defaultOctaveNote = 60;

    /** Velocity steps (as in Ableton Live, in twenties), and where it starts. */
    static constexpr std::array<int, 7> velocitySteps { 20, 40, 60, 80, 100, 120, 127 };
    static constexpr int defaultVelocity = 100;

    /** What pressing a key did: `handled` is true for every key in the map (so the caller can use it
        up), and `noteOn` holds the message to send when a note should start. */
    struct Press
    {
        bool handled = false;
        std::optional<juce::MidiMessage> noteOn;
    };

    //==============================================================================
    /** The letter of key `index` (0 to 15, in semitones above the A key): "awsedftgyhujkolp". */
    static char getLetter (int index);

    /** The index (0 to 15) of a letter, in either case, or -1 when it plays no note. */
    static int getKeyIndex (char letter);

    /** True for the black keys (W E T Y U O P). */
    static bool isBlackKey (int index);

    /** True for a letter that plays a note or changes the octave or velocity: a to l, w e t y u o p, z x c v. */
    static bool isMapped (char letter);

    //==============================================================================
    /** Presses a letter (either case). A key that is already held (auto-repeat) is handled but starts
        nothing; so is a note above 127. Z, X, C and V change the octave and velocity. */
    Press press (char letter);

    /** Releases a letter: the Note Off for the note that key started, or nothing when it wasn't held. */
    std::optional<juce::MidiMessage> release (char letter);

    /** Note Offs for every held key, and forgets them (turning off, losing focus, changing track). */
    std::vector<juce::MidiMessage> releaseAll();

    /** True while the letter's note is sounding. */
    bool isHeld (char letter) const;
    bool isKeyHeld (int index) const;

    /** The letters held now. */
    std::string getHeldLetters() const;

    //==============================================================================
    int getOctaveNote() const noexcept     { return octaveNote; }
    int getVelocity() const noexcept       { return velocity; }

    /** Sets the octave, clamped to C0..C8 and snapped to the nearest octave (a tie goes up). */
    void setOctaveNote (int note) noexcept;

    /** Moves the octave by `steps` octaves, stopping at the ends. Returns true when it moved. */
    bool moveOctave (int steps) noexcept;

    /** Sets the velocity, snapped to the nearest step (a tie goes up). */
    void setVelocity (int newVelocity) noexcept;

    /** Moves the velocity by `steps` steps, stopping at 20 and 127. Returns true when it moved. */
    bool moveVelocity (int steps) noexcept;

    /** The note key `index` plays now, or -1 when it would be above 127. */
    int getNote (int index) const noexcept;

    /** The letters, in key order, that the scale filter keeps silent: those whose pitch class isn't in
        `keyPitchClasses` (bit n set when pitch class n is in the key, as `Track` takes it). */
    std::string getSilencedLetters (int keyPitchClasses) const;

private:
    int octaveNote = defaultOctaveNote, velocity = defaultVelocity;
    std::map<char, int> held;   // letter -> the note it started
};
} // namespace daw
