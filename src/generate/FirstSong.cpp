#include "FirstSong.h"
#include "ChordGuide.h"

#include <cmath>

namespace daw::gen::firstsong
{
using guides::Guide;

std::vector<Step> getSteps()
{
    return {
        { "Hear a note", "Play middle C, then write a note",
          "A MIDI note is an instruction, not a sound: which key, how hard and how long. Middle C is note 60. The track's instrument turns the "
          "instruction into sound, which is why you can change the note, or the whole instrument, at any time.",
          { Guide::midi, "The computer keyboard" }, true },
        { "Change a chord", "Pick another chord for one bar",
          "The Chords row is the song's plan. It makes no sound by itself, but it shows what each chord is, guides the generator and fills Play along. "
          "Changing one chord changes the feeling of the whole bar.",
          { Guide::chords, "Your chord track" }, true },
        { "Add a rhythm", "A beat of kick, snare and hi-hat",
          "This is the backbeat that most pop and rock rests on: the snare on beats 2 and 4 gives the bar its push, the kick marks beats 1 and 3, "
          "and the hi-hat keeps time in between. Drummers change it endlessly, but it's a good first beat.",
          { Guide::rhythm, "Bars and beats" }, true },
        { "Save your song", "Keep it as a project file",
          "Saving often means a crash or a closed window costs you little. A project keeps every part separate, so you can change any of them later.",
          {}, false }
    };
}

std::vector<Link> getAllLinks()
{
    std::vector<Link> links;

    for (const auto& s : getSteps())
        if (s.hasLink)
            links.push_back (s.link);

    links.push_back ({ Guide::midi, "What MIDI is" });
    links.push_back ({ Guide::midi, "Note numbers and pitch" });
    links.push_back ({ Guide::recording, "Choosing an input" });
    return links;
}

juce::String getTitle() { return "First song exercise"; }

juce::String getIntro()
{
    return "Four short steps in your own song, about five minutes in all. Each one says what to do and why. "
           "Do it in the main window: the tick appears by itself.";
}

juce::String getCompletionText()
{
    return "Done. You heard a note, changed a chord, added a rhythm and saved your song. Next, try Start from a style for a whole song sketch, "
           "or carry on with Start here in Theory Guides.";
}

juce::String getDoText (int step, const MusicalKey& key, const juce::String& midiTrackName)
{
    switch (step)
    {
        case 0:
        {
            const auto first = midiTrackName.isEmpty()
                ? juce::String ("Press Add a piano and play it. DawIt adds a Piano track with an empty clip, opens it in the piano roll at the bottom and turns on the computer keyboard.")
                : "Press Open " + midiTrackName + " and play it: DawIt opens its first clip in the piano roll and turns on the computer keyboard.";

            return first + " Press A: you hear middle C (C4, note 60). Then click anywhere in the piano roll's grid to write a note. "
                           "You hear it as you add it, and it stays in the song. Press Space to play it back.";
        }

        case 1:
        {
            const auto progressions = guide::getCommonProgressions (key);
            juce::String name = "Pop progression", chordNames = "C, G, Am and F", keyName = "C major", swap = "Dm in place of F";

            if (! progressions.empty())
            {
                const auto chords = getExampleChords (key);
                juce::StringArray names;

                for (const auto& c : chords)
                    names.add (c.second.getName());

                name = progressions[0].name;
                keyName = key.getName();
                chordNames = names.isEmpty() ? juce::String() : names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1];

                if (chords.size() == 4)
                    swap = getSwapSuggestion (key).second.getName() + " in place of " + chords[3].second.getName();
            }

            return "The Chords row above the tracks holds one chord per bar. If it's empty, press Put in four chords: DawIt writes the " + name
                   + " in your key, " + chordNames + " in " + keyName + ", in bars 1 to 4. Click a bar on the Chords row and choose another chord, for example "
                   + swap + ". Open the row with its arrow to see each chord as piano keys: click a chord to hear it.";
        }

        case 2:
            return juce::String::fromUTF8 ("Press Add drums: DawIt adds a Drums track with an empty four-bar clip and opens it in the piano roll. On a drum track each note is a "
                   "drum, not a pitch: C2 (note 36) is the kick, D2 (38) the snare and F\xE2\x99\xAF" "2 (42) the closed hi-hat. Write the kick on beats 1 and 3, "
                   "the snare on 2 and 4 and a hi-hat on every eighth note. Or press Write a basic beat and change what it writes. Press Space to hear it.");

        case 3:
            return "Press Save... (Cmd+S) and choose a name and a folder. DawIt writes a .dawit project with everything in the song: tracks, notes, chords, "
                   "sounds and effects. Audio you record or import is copied into the project's audio folder. Open it again with File > Open Recent.";

        default: break;
    }

    return {};
}

juce::String getActionLabel (int step, const juce::String& midiTrackName, bool chordTrackEmpty, bool hasDrumTrack)
{
    switch (step)
    {
        case 0: return midiTrackName.isEmpty() ? juce::String ("Add a piano and play it") : "Open " + midiTrackName + " and play it";
        case 1: return chordTrackEmpty ? "Put in four chords" : "Open the Chords row";
        case 2: return hasDrumTrack ? "Write a basic beat" : "Add drums";
        case 3: return "Save...";
        default: break;
    }

    return {};
}

juce::String getActionTooltip (int step, bool chordTrackEmpty, bool hasDrumTrack)
{
    switch (step)
    {
        case 0: return "Adds a Piano track (or opens your MIDI track), opens its clip and turns on Play with Computer Keyboard (Cmd+K turns it off again).";
        case 1: return chordTrackEmpty ? "Writes four chords from the key into bars 1 to 4 of the empty Chords row. One undo step."
                                       : "Opens the Chords row above the tracks, where each bar's chord shows as piano keys.";
        case 2: return hasDrumTrack ? "Writes kick, snare and hi-hat into the empty drum clip: one bar, repeated. One undo step."
                                    : "Adds a MIDI track with the built-in Drums sound and an empty four-bar clip, open in the piano roll.";
        case 3: return "Saves the project. The first time it asks for a name and a folder.";
        default: break;
    }

    return {};
}

int clampStep (int stored) noexcept
{
    return stored >= 0 && stored < numSteps ? stored : 0;
}

bool readDone (const juce::var& stored)
{
    return stored.isBool() && (bool) stored;
}

//==============================================================================
bool isDrumTrack (const Track& track)
{
    if (! track.isMidi())
        return false;

    if (track.usesBuiltInSynth())
        return builtInSounds::isPercussive (track.builtInSound.load());

    for (auto& slot : track.plugins)
        if (slot->isInstrument && slot->instance != nullptr && slot->instance->getName() == "DawIt Drums")
            return true;

    return false;
}

static int countNotes (const Track& track)
{
    int n = 0;

    for (auto& c : track.clips)
        if (auto* m = dynamic_cast<const MidiClip*> (c.get()))
            n += (int) m->notes.size();

    return n;
}

bool isStep1Done (const Engine& engine)
{
    for (auto& t : engine.getTracks())
        if (t->isMidi() && ! isDrumTrack (*t) && countNotes (*t) > 0)
            return true;

    return false;
}

bool isStep2Done (const Engine& engine, const ChordMap& baseline)
{
    const auto& chords = engine.getChords();
    return ! chords.empty() && chords != baseline;
}

bool isStep3Done (const Engine& engine)
{
    for (auto& t : engine.getTracks())
        if (isDrumTrack (*t) && countNotes (*t) >= 4)
            return true;

    return false;
}

bool isStep4Done (const SavedState& s) { return s.hasFile && ! s.unsaved; }

bool isStepDone (int step, const Engine& engine, const ChordMap& baseline, const SavedState& saved)
{
    switch (step)
    {
        case 0: return isStep1Done (engine);
        case 1: return isStep2Done (engine, baseline);
        case 2: return isStep3Done (engine);
        case 3: return isStep4Done (saved);
        default: break;
    }

    return false;
}

const Track* findMidiTrack (const Engine& engine)
{
    const Track* firstEmpty = nullptr;

    for (auto& t : engine.getTracks())
    {
        if (! t->isMidi() || isDrumTrack (*t))
            continue;

        if (countNotes (*t) > 0)
            return t.get();

        if (firstEmpty == nullptr)
            firstEmpty = t.get();
    }

    return firstEmpty;
}

//==============================================================================
std::vector<std::pair<int, Chord>> getExampleChords (const MusicalKey& key)
{
    std::vector<std::pair<int, Chord>> result;
    const auto progressions = guide::getCommonProgressions (key);

    if (progressions.empty())
        return result;

    const auto chords = progressions[0].getChords (key);

    for (int i = 0; i < (int) chords.size() && i < 4; ++i)
        result.emplace_back (i, chords[(size_t) i]);

    return result;
}

std::pair<int, Chord> getSwapSuggestion (const MusicalKey& key)
{
    const auto chords = getExampleChords (key);

    if (chords.size() < 4)
        return { 3, Chord() };

    // The progression's home chord says whether it is a major or a minor one.
    const bool major = chords[0].second.intervals.contains (4);
    return { 3, guide::buildChord (key, { major ? 2 : 4, "triad" }) };
}

std::vector<MidiNote> getBasicBeat (int bars)
{
    std::vector<MidiNote> notes;

    for (int bar = 0; bar < bars; ++bar)
    {
        const double origin = bar * 4.0;

        notes.push_back ({ 36, origin + 0.0, 0.25, 0.9f });
        notes.push_back ({ 36, origin + 2.0, 0.25, 0.9f });
        notes.push_back ({ 38, origin + 1.0, 0.25, 0.85f });
        notes.push_back ({ 38, origin + 3.0, 0.25, 0.85f });

        for (int e = 0; e < 8; ++e)
            notes.push_back ({ 42, origin + 0.5 * e, 0.25, e % 2 == 0 ? 0.7f : 0.55f });
    }

    return notes;
}

bool canWriteChords (const Engine& engine) { return engine.getChords().empty(); }

bool canWriteBeat (const Engine& engine, const MidiClip& clip)
{
    if (engine.getTimeSignature().numerator != 4 || engine.getTimeSignature().denominator != 4 || ! clip.notes.empty())
        return false;

    for (auto& t : engine.getTracks())
        for (auto& c : t->clips)
            if (c.get() == &clip)
                return isDrumTrack (*t);

    return false;
}

bool writeExampleChords (Engine& engine)
{
    if (! canWriteChords (engine))
        return false;

    ChordMap map;

    for (const auto& [bar, chord] : getExampleChords (engine.getKey()))
        map[bar] = chord;

    if (map.empty())
        return false;

    engine.commitHistory();   // anything pending is its own step
    engine.setChords (std::move (map));
    engine.nameNextStep ("Put in four chords");
    engine.commitHistory();
    return true;
}

bool writeBasicBeat (Engine& engine, MidiClip& clip)
{
    if (! canWriteBeat (engine, clip))
        return false;

    const int bars = juce::jmax (1, (int) std::lround (clip.lengthBeats / engine.getBarLength()));

    engine.commitHistory();

    {
        const juce::ScopedLock sl (engine.getLock());
        clip.notes = getBasicBeat (bars);
    }

    engine.notesChanged();
    engine.nameNextStep ("Write a basic beat");
    engine.commitHistory();
    return true;
}
} // namespace daw::gen::firstsong
