#pragma once

#include "ChordStyles.h"
#include "StructureGuide.h"

#include <array>
#include <optional>

namespace daw::gen::templates
{
/** The broad arrangement rule a style follows. */
enum class Family { song, club, film };

/** One hit in a repeating drum pattern. */
struct DrumHit
{
    double beat = 0.0;
    float velocity = 0.8f;
};

/** One drum track and the hits it repeats. */
struct DrumPart
{
    juce::String name;
    int pitch = 36;
    double patternBeats = 4.0;
    std::vector<DrumHit> hits;
};

/** Everything that distinguishes one of the fourteen built-in song styles. */
struct Style
{
    juce::String id, name, summary, structureName;
    int minimumBpm = 60, defaultBpm = 120, maximumBpm = 140;
    MusicalKey::Scale scale = MusicalKey::Scale::major;
    std::array<int, 3> tonics {};
    std::array<std::vector<juce::String>, 4> progressions; // verse, chorus, lift, contrast
    int barsPerChord = 1;
    juce::String chordStyle, bassStyle, padStyle, drumKit, returnPreset;
    Family family = Family::song;
    std::vector<DrumPart> drums;
};

/** One ordinary MIDI clip that a template will put on a track. */
struct ClipPlan
{
    juce::String name;
    double startBeat = 0.0, lengthBeats = 0.0;
    std::vector<MidiNote> notes;
};

/** One ordinary MIDI track that a template will build. */
struct PartPlan
{
    juce::String name;
    BuiltInSound sound = BuiltInSound::piano;
    float volumeDb = 0.0f, pan = 0.0f;
    bool drum = false;
    juce::String chordStyle;
    std::vector<ClipPlan> clips;
};

/** The pure, deterministic result of choosing a style, key, tempo and seed. */
struct SongPlan
{
    juce::String styleId, styleName, drumKit, returnPreset;
    double tempo = 120.0;
    MusicalKey key;
    SectionList sections;
    ChordMap chords;
    std::vector<PartPlan> parts;

    int getTotalBars() const;
};

/** The fourteen styles shown by File > New Project from Style. */
const std::vector<Style>& getStyles();

/** Finds a style by its stable id, or null. */
const Style* findStyle (const juce::String& id);

/** Finds the song structure named by a style, or null when the table is wrong. */
const structure::Structure* findStructure (const Style&);

/** Makes a song without touching the engine. A missing tonic chooses one of the style's three keys
    from the seed; tempo is kept inside the style's range. Seed zero is the documented default. */
SongPlan makePlan (const Style&, std::optional<int> tonic, double tempo, uint32_t seed, bool writeNotes);

/** Replaces the edit with the ordinary tracks, clips, effects, buses, return and sends in a plan.
    Returns false, with a concrete error, if a named kit or starting point cannot be found. */
bool build (Engine&, const SongPlan&, juce::String& error);

/** True when this part has a clip in this section according to the style's energy rules. */
bool playsInSection (const Style&, const juce::String& partName, int sectionIndex, const SectionList&);
} // namespace daw::gen::templates
