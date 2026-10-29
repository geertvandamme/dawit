#include "ExampleSong.h"

#include "BuiltInCompressor.h"

namespace daw::examples
{
namespace
{
    constexpr int songBars = 36;
    constexpr int bandStartBar = 4;   // the intro is the keys alone

    /** The pitch with this pitch class closest to `around`. */
    int nearest (int pitchClass, int around)
    {
        const int below = around - ((around - pitchClass) % 12 + 12) % 12;
        return around - below <= below + 12 - around ? below : below + 12;
    }

    /** The chord tone closest to `around`, stepping `direction` (-1 or +1) to the next one when it would equal `skip`. */
    int chordTone (const Chord& chord, int around, int skip = -1, int direction = 0)
    {
        int best = -1;

        for (int pitch = around - 12; pitch <= around + 12; ++pitch)
        {
            if (! chord.getPitchClasses().contains (pitch % 12))
                continue;

            if (direction != 0 && (pitch - skip) * direction <= 0)
                continue;

            if (best < 0 || std::abs (pitch - around) < std::abs (best - around))
                best = pitch;
        }

        return best >= 0 ? best : around;
    }

    Track* addMidiTrack (Engine& engine, const juce::String& name, BuiltInSound sound, float volumeDb, float pan = 0.0f)
    {
        auto* track = engine.addTrack (Track::Type::midi);
        track->name = name;
        engine.setBuiltInSound (*track, sound);
        track->volumeDb = volumeDb;
        track->pan = pan;
        return track;
    }

    MidiClip* addClip (Engine& engine, Track& track, int fromBar, int toBar, const juce::String& name)
    {
        auto* clip = engine.addMidiClip (track, fromBar * 4.0, (toBar - fromBar) * 4.0);
        clip->name = name;
        return clip;
    }

    /** Adds a note at a beat of the song to a clip, which keeps its notes from its own start. */
    void addNote (MidiClip& clip, int pitch, double beat, double length, float velocity)
    {
        clip.notes.push_back ({ pitch, beat - clip.startBeat, length, velocity });
    }
} // namespace

Track* buildPopSong (Engine& engine)
{
    engine.setTempo (96.0);
    engine.setKey ({ 9, MusicalKey::Scale::minor });

    SectionList sections;
    sections.push_back ({ 0, 4, SectionType::intro, {} });
    sections.push_back ({ 4, 8, SectionType::verse, {} });
    sections.push_back ({ 12, 8, SectionType::chorus, {} });
    sections.push_back ({ 20, 8, SectionType::verse, {} });
    sections.push_back ({ 28, 8, SectionType::chorus, {} });
    engine.setSections (sections);

    // i-VI-III-VII in the intro and verses; the chorus lifts with VI-VII-III-i.
    const char* names[songBars] = { "Am", "F", "C", "G",
                                    "Am", "F", "C", "G", "Am", "F", "C", "G",
                                    "F", "G", "C", "Am", "F", "G", "C", "C",
                                    "Am", "F", "C", "G", "Am", "F", "C", "G",
                                    "F", "G", "C", "Am", "F", "G", "C", "C" };
    ChordMap chords;

    for (int bar = 0; bar < songBars; ++bar)
        if (auto chord = Chord::fromName (names[bar]))
            chords[bar] = *chord;

    engine.setChords (chords);

    const juce::String text = "[Verse]\nI walked a-long the ri-ver late\nthe ci-ty lights be-hind me\n"
                              "your voice still ring-ing in my head\nthe way it used to find me\n\n"
                              "[Chorus]\nhold on, hold on to-night\nwe could run be-fore the light\n"
                              "hold on, don't let it go\nev-ery-thing we used to know\n\n"
                              "[Verse]\nthe trains go by, the sum-mer ends\nI count the cars a-gain\n"
                              "the plat-form emp-ties one by one\nand still I'm wait-ing then";
    engine.setLyrics (lyrics::place (lyrics::parse (text), sections));

    auto* kick = addMidiTrack (engine, "Kick", BuiltInSound::drums, -2.0f);
    auto* snare = addMidiTrack (engine, "Snare", BuiltInSound::drums, -4.0f);
    auto* hats = addMidiTrack (engine, "Hats", BuiltInSound::drums, -9.0f, 0.25f);
    auto* bass = addMidiTrack (engine, "Bass", BuiltInSound::bass, -4.0f);
    auto* keys = addMidiTrack (engine, "Keys", BuiltInSound::piano, -7.0f, -0.2f);
    auto* lead = addMidiTrack (engine, "Lead vocal", BuiltInSound::lead, 0.0f);
    auto* bv1 = addMidiTrack (engine, "BV 1", BuiltInSound::lead, -9.0f, -0.4f);
    auto* bv2 = addMidiTrack (engine, "BV 2", BuiltInSound::lead, -9.0f, 0.4f);

    auto* kickClip = addClip (engine, *kick, bandStartBar, songBars, "Kick");
    auto* snareClip = addClip (engine, *snare, bandStartBar, songBars, "Snare");
    auto* hatsClip = addClip (engine, *hats, bandStartBar, songBars, "Hats");
    auto* bassClip = addClip (engine, *bass, bandStartBar, songBars, "Bassline");
    auto* keysClip = addClip (engine, *keys, 0, songBars, "Chords");
    auto* leadClip = addClip (engine, *lead, bandStartBar, songBars, "Melody");
    auto* bv1Clip = addClip (engine, *bv1, 12, songBars, "Harmony low");
    auto* bv2Clip = addClip (engine, *bv2, 12, songBars, "Harmony high");

    {
        const juce::ScopedLock sl (engine.getLock());

        for (int bar = 0; bar < songBars; ++bar)
        {
            const auto& chord = chords[bar];
            const double start = bar * 4.0;

            for (int pc : chord.getPitchClasses())
                addNote (*keysClip, nearest (pc, 62), start, 3.9, 0.6f);

            if (bar < bandStartBar)
                continue;

            const bool chorus = engine.getSectionAt (bar) != nullptr && engine.getSectionAt (bar)->type == SectionType::chorus;
            addNote (*bassClip, nearest (chord.root, 36), start, 2.4, 0.9f);
            addNote (*bassClip, nearest (chord.root, 36), start + 2.5, 1.4, 0.8f);

            addNote (*kickClip, 36, start, 0.25, 0.95f);
            addNote (*kickClip, 36, start + 2.0, 0.25, 0.9f);

            if (chorus)
                addNote (*kickClip, 36, start + 2.5, 0.25, 0.75f);

            addNote (*snareClip, 38, start + 1.0, 0.25, 0.9f);
            addNote (*snareClip, 38, start + 3.0, 0.25, 0.9f);

            for (int eighth = 0; eighth < 8; ++eighth)
                addNote (*hatsClip, 42, start + eighth * 0.5, 0.2, eighth % 2 == 0 ? 0.7f : 0.45f);
        }

        // The vocals sing the lyrics: a note per syllable on a chord tone, rising and falling over each line.
        for (auto& line : engine.getLyrics().lines)
        {
            const auto count = line.syllables.size();

            for (size_t i = 0; i < count; ++i)
            {
                const auto& syllable = line.syllables[i];
                const int bar = (int) (syllable.beat / 4.0);
                const auto it = chords.find (bar);

                if (it == chords.end())
                    continue;

                const double arc = count > 1 ? std::sin (juce::MathConstants<double>::pi * (double) i / (double) (count - 1)) : 0.0;
                const int pitch = chordTone (it->second, 64 + juce::roundToInt (5.0 * arc));
                const double length = juce::jlimit (0.2, 2.0, line.getSyllableEnd (i) - syllable.beat - 0.05);
                addNote (*leadClip, pitch, syllable.beat, length, 0.8f);

                if (engine.getSectionAt (bar) != nullptr && engine.getSectionAt (bar)->type == SectionType::chorus)
                {
                    addNote (*bv1Clip, chordTone (it->second, pitch - 3, pitch, -1), syllable.beat, length, 0.7f);
                    addNote (*bv2Clip, chordTone (it->second, pitch + 3, pitch, 1), syllable.beat, length, 0.7f);
                }
            }
        }
    }

    engine.notesChanged();

    auto* drums = engine.createBus ({ kick, snare, hats }, "Drums");
    auto* backing = engine.createBus ({ bv1, bv2 }, "Backing vocals");
    engine.createBus ({ lead, backing }, "Vocals");

    // Light compression on the drum bus, from DawIt Compressor's own starting point for it.
    juce::String error;

    if (auto* slot = drums != nullptr ? engine.addPlugin (*drums, BuiltInCompressor::getDescription(), error) : nullptr)
        if (auto* compressor = dynamic_cast<BuiltInCompressor*> (slot->instance.get()))
            for (auto& preset : comp::getPresets())
                if (preset.name == "Drum bus")
                    compressor->applyPreset (preset);

    return lead;
}
} // namespace daw::examples
