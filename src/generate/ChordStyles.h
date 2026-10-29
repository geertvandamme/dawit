#pragma once

#include "engine/Engine.h"

#include <array>
#include <optional>

namespace daw::gen::styles
{
/**
    Playing chords in an instrument's style: a ballad piano, a folk strum, an off-beat EDM pluck.

    A style is a short pattern (one or two bars) whose events name a part of the chord instead of
    a pitch: the bass, the fifth, the second chord tone, the whole chord. Each event is voiced on
    the chord sounding at that moment, for the instrument (piano hands, guitar shapes, a synth
    stack), so the part always fits the chords. The built-in styles come from ChordStyles.json,
    in the same format a model could write.
*/
enum class Instrument { piano, guitar, synth, strings, bass };

constexpr std::array<Instrument, 5> allInstruments { Instrument::piano, Instrument::guitar, Instrument::synth,
                                                     Instrument::strings, Instrument::bass };

juce::String getInstrumentName (Instrument);
std::optional<Instrument> instrumentFromId (const juce::String&);

/** What an event plays, from the chord's voicing. */
enum class Role
{
    bass,       // the chord's root, low
    third,      // the third (or the sus note) above the bass
    fifth,      // the fifth above the bass
    octave,     // the bass an octave up
    tone,       // one chord tone, counted from the bottom of the voicing above the bass (index 0 = lowest)
    top,        // the highest note of the voicing
    upper,      // the voicing without the bass
    all,        // the bass and the voicing: the whole chord
    approach,   // a step leading into the next chord's bass (walking bass)
    mute        // the whole chord, short and soft: a muted ghost strum
};

enum class Strum { none, down, up };

struct Event
{
    double start = 0.0;     // beats from the start of the pattern
    double length = 1.0;    // beats
    Role role = Role::all;
    int index = 0;          // for Role::tone
    int octave = 0;         // moves the notes by octaves
    float velocity = 0.75f; // 0-1
    Strum strum = Strum::none;
};

struct Style
{
    juce::String id, name, description;
    juce::StringArray parts;    // the Generate panel parts that offer it: "chords", "arpeggio", "bassline"
    juce::StringArray genres;   // styles of music it is typical for, such as "House" or "Jazz"
    Instrument instrument = Instrument::piano;
    BuiltInSound sound = BuiltInSound::piano;
    bool powerChords = false;   // root, fifth and octave instead of the full chord
    double strumMs = 0.0;       // time between the notes of a strum or a rolled chord
    int bars = 1;
    std::vector<Event> events;

    double getLengthBeats() const { return bars * 4.0; }

    /** True if the style is typical for this style of music (any case). */
    bool suitsGenre (const juce::String& genre) const;
};

/** The role's name in JSON, e.g. "bass". */
juce::String getRoleId (Role);
std::optional<Role> roleFromId (const juce::String&);
juce::StringArray getRoleIds();

/** True if the instrument plays the role: a bass plays one note at a time, so not the chord roles
    (top, upper, all, mute). */
bool playsRole (Instrument, Role);

/** Reads one event of a pattern `patternBeats` long. `velocityScale` is the top of the velocity
    range: 1 in the style library, 127 in a model's reply. Returns nullopt and sets `error` if it
    can't be used. */
std::optional<Event> parseEvent (const juce::var&, double patternBeats, double velocityScale, juce::String& error);

/** The events as compact JSON, one per line, with velocities 1-127: how a model sees a pattern. */
juce::String describeEvents (const std::vector<Event>&);

/** Reads one style from its JSON object; returns nullopt and sets `error` if it can't be used. */
std::optional<Style> parseStyle (const juce::var&, juce::String& error);

/** Reads a list of styles from JSON text ({ "styles": [ ... ] }). */
std::vector<Style> parseStyles (const juce::String& json, juce::String& error);

/** The built-in styles, in library order. */
const std::vector<Style>& getLibrary();
std::vector<const Style*> getStylesFor (Instrument);
const Style* findStyle (const juce::String& id);

//==============================================================================
/** A chord sounding from `start` to `end` (beats, relative to the start of the part). */
struct ChordSpan
{
    double start = 0.0, end = 4.0;
    Chord chord;
};

/** The chord track's bars from `firstBar` up to (not including) `endBar`, one span per bar with
    a chord; bars without one are gaps, where nothing plays. */
std::vector<ChordSpan> spansFromChordTrack (const ChordMap&, int firstBar, int endBar);

/** The notes of a chord as one instrument plays it. */
struct Voicing
{
    int bass = 48;
    std::vector<int> upper;   // ascending, above the bass
    int third = 52, fifth = 55;
    bool wrapsOctaves = true; // tone indexes past the top go up an octave (not on a guitar: they stay on the top string)
    std::vector<int> shape;   // guitar: the fret per string (see findGuitarShape()); empty for other instruments
};

/** Voices the chord for the style's instrument, close to the previous voicing (smooth voice leading). */
Voicing voice (const Style&, const Chord&, const Voicing* previous);

/** A playable guitar shape for the chord in standard tuning: the fret per string from low E to
    high E, -1 for a string that isn't played. Prefers open, low shapes with the root in the bass
    and, with `previous`, shapes near it. Empty if none is found. */
std::vector<int> findGuitarShape (const Chord&, const std::vector<int>* previous = nullptr);

/** Up to `maxShapes` ways to play the chord on different places of the neck, the easiest first
    (as findGuitarShape() picks it), then e.g. a barre shape and one higher up. With `accept`, only
    the shapes it accepts. */
std::vector<std::vector<int>> findGuitarShapes (const Chord&, int maxShapes = 3,
                                                const std::function<bool (const std::vector<int>&)>& accept = nullptr);

/** The pitch of each string of a shape (skipping strings that aren't played), low to high. */
std::vector<int> getShapePitches (const std::vector<int>& shape);

/** Plays the chords in the style: the pattern repeats from beat 0 up to `totalBeats`, and each
    event is voiced on the chord sounding then. An event just before a chord change that rings
    on into the next chord plays that chord (a pushed chord); notes end at the chord's end. */
std::vector<MidiNote> play (const Style&, const std::vector<ChordSpan>&, double totalBeats, double bpm);

/** Puts the notes in one clip on a new MIDI track at `index` (-1: at the end), with the
    style's sound and a name after the instrument and style. Message thread. */
MidiClip* addTrack (Engine&, const Style&, std::vector<MidiNote>, double startBeat, double lengthBeats, int index);
} // namespace daw::gen::styles
