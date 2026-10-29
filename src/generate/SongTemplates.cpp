#include "SongTemplates.h"

#include "ChordGuide.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInDrums.h"
#include "engine/BuiltInEq.h"
#include "engine/BuiltInReverb.h"

#include <algorithm>
#include <random>

namespace daw::gen::templates
{
namespace
{
    using Scale = MusicalKey::Scale;
    using Role = std::array<std::vector<juce::String>, 4>;

    std::vector<DrumHit> pattern (const char* text)
    {
        std::vector<DrumHit> hits;
        const juce::String value (text);

        for (int i = 0; i < value.length(); ++i)
        {
            const auto c = value[i];
            if (c == 'X' || c == 'x' || c == 'o' || c == 'O')
                hits.push_back ({ i * 0.25, c == 'X' || c == 'O' ? 0.9f : c == 'x' ? 0.6f : 0.4f });
        }

        return hits;
    }

    DrumPart drum (const char* name, int pitch, const char* hits, double beats = 4.0)
    {
        return { name, pitch, beats, pattern (hits) };
    }

    Role roles (std::vector<juce::String> verse, std::vector<juce::String> chorus = {},
                std::vector<juce::String> lift = {}, std::vector<juce::String> contrast = {})
    {
        if (chorus.empty()) chorus = verse;
        if (lift.empty()) lift = verse;
        if (contrast.empty()) contrast = verse;
        return { std::move (verse), std::move (chorus), std::move (lift), std::move (contrast) };
    }

    std::vector<DrumPart> straight (const char* secondName, int secondPitch,
                                    const char* kick, const char* second, const char* hats, int hatsPitch = 42)
    {
        return { drum ("Kick", 36, kick), drum (secondName, secondPitch, second), drum (hatsPitch == 42 ? "Hats" : "Shaker", hatsPitch, hats) };
    }

    std::optional<guide::Progression> findProgression (const MusicalKey& key, const juce::String& name)
    {
        for (const auto& progression : guide::getCommonProgressions (key))
            if (progression.name == name)
                return progression;

        return std::nullopt;
    }

    int roleFor (const Style& style, SectionType type)
    {
        if (type == SectionType::preChorus || type == SectionType::buildUp)
            return 2;
        if (type == SectionType::bridge)
            return 3;
        if (type == SectionType::chorus || type == SectionType::postChorus || type == SectionType::drop
            || type == SectionType::outro || type == SectionType::breakdown)
            return 1;
        if ((style.id == "rock" || style.id == "blues") && type == SectionType::solo)
            return 0;
        return 0;
    }

    juce::String pick (const std::vector<juce::String>& choices, uint32_t seed, int role)
    {
        if (choices.empty())
            return {};
        if (seed == 0)
            return choices.front();

        std::mt19937 generator (seed * 2654435761u + (uint32_t) role * 1013904223u);
        return choices[(size_t) (generator() % (uint32_t) choices.size())];
    }

    bool isSecondDrum (const juce::String& name)
    {
        return name == "Snare" || name == "Clap" || name == "Rim" || name == "High drum";
    }

    bool isHat (const juce::String& name)
    {
        return name == "Hats" || name == "Shaker";
    }

    bool isLowDrum (const juce::String& name)
    {
        return name == "Kick" || name == "Low drum";
    }

    bool isDrumName (const Style& style, const juce::String& name)
    {
        return std::any_of (style.drums.begin(), style.drums.end(), [&] (const DrumPart& part) { return part.name == name; });
    }

    int firstBreakdown (const SectionList& sections)
    {
        for (int i = 0; i < (int) sections.size(); ++i)
            if (sections[(size_t) i].type == SectionType::breakdown)
                return i;
        return (int) sections.size();
    }

    std::vector<styles::ChordSpan> chordSpans (const SongPlan& plan, const Section& section)
    {
        std::vector<styles::ChordSpan> spans;

        for (int bar = section.startBar; bar < section.getEndBar(); ++bar)
        {
            const auto found = plan.chords.find (bar);
            if (found == plan.chords.end())
                continue;

            const double start = (bar - section.startBar) * 4.0;
            if (! spans.empty() && spans.back().chord == found->second)
                spans.back().end = start + 4.0;
            else
                spans.push_back ({ start, start + 4.0, found->second });
        }

        return spans;
    }

    float volumeFor (const juce::String& name)
    {
        if (name == "Kick") return -2.0f;
        if (name == "Low drum") return -4.0f;
        if (isSecondDrum (name)) return name == "High drum" ? -8.0f : -4.0f;
        if (isHat (name) || name == "Cymbal") return name == "Cymbal" ? -12.0f : -9.0f;
        if (name == "Bass") return -4.0f;
        if (name == "Chords") return -7.0f;
        if (name == "Pad") return -12.0f;
        if (name == "Melody") return -3.0f;
        return 0.0f;
    }

    float panFor (const juce::String& name)
    {
        if (isHat (name)) return 0.25f;
        if (name == "Chords") return -0.2f;
        return 0.0f;
    }

    BuiltInSound soundFor (const juce::String& name, const juce::String& styleId)
    {
        if (name == "Bass") return BuiltInSound::bass;
        if (name == "Melody") return BuiltInSound::lead;
        if (name == "Pad") return BuiltInSound::pad;
        if (name == "Chords" && (styleId == "rock" || styleId == "afrobeats")) return BuiltInSound::pluck;
        return BuiltInSound::piano;
    }

    const comp::Preset* compressorPreset (const juce::String& name)
    {
        for (const auto& preset : comp::getPresets())
            if (preset.name == name)
                return &preset;
        return nullptr;
    }

    const eq::Preset* eqPreset (const juce::String& name)
    {
        for (const auto& preset : eq::getPresets())
            if (preset.name == name)
                return &preset;
        return nullptr;
    }

    const verb::Preset* reverbPreset (const juce::String& name)
    {
        for (const auto& preset : verb::getPresets())
            if (preset.name == name)
                return &preset;
        return nullptr;
    }

    const drums::Preset* drumPreset (const juce::String& name)
    {
        for (const auto& preset : drums::getPresets())
            if (preset.name == name)
                return &preset;
        return nullptr;
    }

    /** Logarithmic JUCE parameters can move by a few millionths when their saved, denormalised
        value is converted back to a normalised value. Settle that conversion now so a project is
        byte-for-byte stable after it is opened and saved again. */
    void stabiliseState (juce::AudioPluginInstance& plugin)
    {
        juce::MemoryBlock before, after;
        plugin.getStateInformation (before);

        for (int pass = 0; pass < 16; ++pass)
        {
            plugin.setStateInformation (before.getData(), (int) before.getSize());
            plugin.getStateInformation (after);
            if (after == before) return;
            before = after;
        }
    }
} // namespace

const std::vector<Style>& getStyles()
{
    static const std::vector<Style> list {
        { "pop-ballad", "Pop ballad", "Slow, open and emotional: piano and strings at 60 to 80 BPM.", "Pop song (verse-chorus)",
          60, 70, 80, Scale::major, { 0, 7, 2 }, roles ({ "Fifties progression" }, { "Pop progression" }, { "Sad pop" }, { "Canon" }),
          1, "piano-ballad", "bass-held", "strings-sustained", "Tight pop", "Return: hall", Family::song,
          straight ("Snare", 38, "X.......X.......", "....x.......x...", "o.o.o.o.o.o.o.o.") },
        { "dance-pop", "Dance pop", "Pop made for dancing: a kick on every beat at about 120 BPM.", "Pop song (verse-chorus)",
          100, 120, 130, Scale::minor, { 9, 0, 4 }, roles ({ "Minor loop" }, { "Epic minor" }, { "Rising to home" }, { "Andalusian cadence" }),
          1, "synth-offbeat", "bass-octaves", "synth-pad", "Tight pop", "Return: plate", Family::song,
          straight ("Clap", 39, "X...X...X...X...", "....X.......X...", "..O...O...O...O.", 46) },
        { "rock", "Rock", "Guitars, a backbeat and a solo, at 100 to 130 BPM.", "Rock song (with a solo)",
          100, 120, 130, Scale::major, { 4, 9, 7 }, roles ({ "Three-chord song" }, { "Pop progression" }),
          1, "guitar-power", "bass-eighths", {}, "Tight pop", "Return: short room", Family::song,
          straight ("Snare", 38, "X.......X.x.....", "....X.......X...", "X.x.X.x.X.x.X.x.") },
        { "hip-hop", "Hip-hop", "Boom bap: a laid-back beat for rapping, 80 to 100 BPM.", "Hip-hop and trap (hook and verses)",
          80, 90, 100, Scale::minor, { 0, 2, 9 }, roles ({ "Minor loop", "Epic minor" }),
          1, "piano-jazz-comping", "bass-held", {}, "Boom bap", "Return: short room", Family::song,
          straight ("Snare", 38, "X......x..X.....", "....X.......X...", "x.x.x.x.x.x.x.x.") },
        { "trap", "Trap", "808s, hi-hat rolls and a half-time clap, written at about 140 BPM.", "Hip-hop and trap (hook and verses)",
          130, 140, 150, Scale::minor, { 5, 0, 7 }, roles ({ "Andalusian cadence", "Minor loop" }),
          2, "synth-pad", "bass-held", {}, "Init: an 808-style kit", "Return: plate", Family::song,
          { drum ("Kick", 36, "X......X........X.........X.....", 8.0), drum ("Clap", 39, "........X...............X.......", 8.0),
            drum ("Hats", 42, "x.x.x.x.x.x.x.x.x.x.x.x.x.x.....", 8.0) } },
        { "house", "House", "Four on the floor at 120 to 128 BPM, from Chicago.", "Techno and house (phrases)",
          120, 124, 128, Scale::minor, { 9, 2, 7 }, roles ({ "Minor loop", "Epic minor" }),
          1, "piano-house", "bass-octaves", "synth-pad", "909-style", "Return: plate", Family::club,
          straight ("Clap", 39, "X...X...X...X...", "....X.......X...", "..O...O...O...O.", 46) },
        { "techno", "Techno", "Hypnotic machine music from Detroit, 125 to 135 BPM.", "Techno and house (phrases)",
          125, 130, 135, Scale::minor, { 5, 9, 2 }, roles ({ "Rising to home", "Minor loop" }),
          2, "synth-arp-updown", "bass-eighths", "synth-pad", "Techno", "Return: hall", Family::club,
          straight ("Clap", 39, "X...X...X...X...", "....x.......x...", "x.O.x.O.x.O.x.O.", 42) },
        { "trance", "Trance", "Euphoric chords and a long breakdown, 136 to 140 BPM.", "Trance (the long breakdown)",
          136, 138, 140, Scale::minor, { 9, 2, 4 }, roles ({ "Epic minor", "Minor loop" }),
          2, "synth-gate", "bass-eighths", "synth-pad", "909-style", "Return: hall", Family::club,
          straight ("Clap", 39, "X...X...X...X...", "....X.......X...", "x.O.x.O.x.O.x.O.", 42) },
        { "drum-and-bass", "Drum & bass", "Fast broken beats over a deep, slow bass, about 174 BPM.", "Drum & bass",
          160, 174, 180, Scale::minor, { 4, 5, 9 }, roles ({ "Minor loop", "Epic minor" }),
          2, "synth-pad", "bass-held", {}, "Tight pop", "Return: plate", Family::club,
          straight ("Snare", 38, "X.........X.....", "....X.......X...", "x.x.x.x.x.x.x.x.") },
        { "lo-fi", "Lo-fi", "Relaxed, dusty beats with jazzy chords, 70 to 90 BPM.", "Lo-fi and beats (two loops)",
          70, 80, 90, Scale::major, { 5, 0, 3 }, roles ({ "Jazz turnaround (ii-V-I)" }, {}, {}, { "Fifties progression", "Sad pop" }),
          1, "piano-jazz-comping", "bass-held", {}, "Boom bap", "Return: short room", Family::song,
          straight ("Snare", 38, "X.......x.x.....", "....x.......x...", "o.o.o.o.o.o.o.o.") },
        { "blues", "Blues", "Twelve bars, three chords and a shuffle.", "Twelve-bar blues",
          60, 100, 130, Scale::major, { 9, 4, 7 }, roles ({ "Twelve-bar blues" }),
          1, "organ-held", "bass-walking", {}, "Tight pop", "Return: short room", Family::song,
          { { "Kick", 36, 4.0, { { 0.0, 0.9f }, { 2.0, 0.9f } } },
            { "Snare", 38, 4.0, { { 1.0, 0.9f }, { 3.0, 0.9f } } },
            { "Hats", 42, 4.0, { { 0.0, 0.9f }, { 2.0 / 3.0, 0.6f }, { 1.0, 0.9f }, { 1.0 + 2.0 / 3.0, 0.6f },
                                  { 2.0, 0.9f }, { 2.0 + 2.0 / 3.0, 0.6f }, { 3.0, 0.9f }, { 3.0 + 2.0 / 3.0, 0.6f } } } } },
        { "reggaeton", "Reggaeton", "The dembow beat at about 95 BPM, from Puerto Rico.", "Pop song (verse-chorus)",
          85, 95, 100, Scale::minor, { 2, 9, 4 }, roles ({ "Minor loop" }, { "Epic minor" }, { "Rising to home" }, { "Andalusian cadence" }),
          1, "synth-offbeat", "bass-held", "synth-pad", "Init: an 808-style kit", "Return: plate", Family::song,
          straight ("Snare", 38, "X...X...X...X...", "...X..X....X..X.", "o.o.o.o.o.o.o.o.") },
        { "afrobeats", "Afrobeats", "West African pop with a bouncing groove, 95 to 120 BPM.", "Pop song (verse-chorus)",
          95, 108, 120, Scale::major, { 7, 0, 2 }, roles ({ "Sad pop" }, { "Pop progression" }, { "Fifties progression" }, { "Three-chord song" }),
          1, "guitar-funk", "bass-root-fifth", {}, "Tight pop", "Return: plate", Family::song,
          straight ("Rim", 38, "X.....X...X.....", "x..x..x...x.x...", "ooxoooxoooxoooxo") },
        { "film-score", "Film score", "One growing arc for strings, drums and a theme.", "Film and ambient (one arc)",
          60, 80, 120, Scale::minor, { 2, 0, 4 }, roles ({ "Epic minor" }, { "Epic minor" }, {}, { "Rising to home" }),
          2, "strings-sustained", "bass-held", {}, "Init: an 808-style kit", "Return: hall", Family::film,
          { drum ("Low drum", 45, "X.....x.X.....x."), drum ("High drum", 50, "....x.......x.xx"), drum ("Cymbal", 49, "X...............") } }
    };

    return list;
}

const Style* findStyle (const juce::String& id)
{
    for (const auto& style : getStyles())
        if (style.id == id)
            return &style;
    return nullptr;
}

const structure::Structure* findStructure (const Style& style)
{
    for (const auto& candidate : structure::getStructures())
        if (candidate.name == style.structureName)
            return &candidate;
    return nullptr;
}

int SongPlan::getTotalBars() const
{
    return sections.empty() ? 0 : sections.back().getEndBar();
}

bool playsInSection (const Style& style, const juce::String& partName, int sectionIndex, const SectionList& sections)
{
    if (! juce::isPositiveAndBelow (sectionIndex, (int) sections.size()))
        return false;

    const auto type = sections[(size_t) sectionIndex].type;
    const bool drumPart = isDrumName (style, partName);
    const bool low = isLowDrum (partName), second = isSecondDrum (partName), hat = isHat (partName);
    const bool bass = partName == "Bass", chords = partName == "Chords", pad = partName == "Pad", melody = partName == "Melody";

    if (style.family == Family::film)
    {
        if (type == SectionType::intro || type == SectionType::outro) return chords;
        if (type == SectionType::verse) return bass || chords || melody;
        if (type == SectionType::bridge) return low || second || bass || chords || melody;
        if (type == SectionType::chorus) return drumPart || bass || chords || melody;
        return false;
    }

    if (type == SectionType::breakdown)
        return chords || pad || (style.family == Family::club && melody);
    if (type == SectionType::buildUp)
        return second || hat || chords || pad || (style.family == Family::club && melody);

    if (style.family == Family::club)
    {
        if (type == SectionType::intro || type == SectionType::outro)
            return low || hat || bass;

        const bool afterBreakdown = sectionIndex > firstBreakdown (sections);
        if (type == SectionType::drop || type == SectionType::chorus || (type == SectionType::phrase && afterBreakdown))
            return drumPart || bass || chords || pad || melody;
        if (type == SectionType::phrase || type == SectionType::verse)
            return drumPart || bass || chords;
    }

    if (type == SectionType::intro || type == SectionType::interlude || type == SectionType::outro)
        return chords;
    if (type == SectionType::preChorus || type == SectionType::postChorus || type == SectionType::chorus || type == SectionType::drop)
        return drumPart || bass || chords || pad || melody;
    if (type == SectionType::verse || type == SectionType::phrase || type == SectionType::bridge || type == SectionType::solo)
        return drumPart || bass || chords || melody;
    return false;
}

SongPlan makePlan (const Style& style, std::optional<int> tonic, double tempo, uint32_t seed, bool writeNotes)
{
    SongPlan plan;
    plan.styleId = style.id;
    plan.styleName = style.name;
    plan.drumKit = style.drumKit;
    plan.returnPreset = style.returnPreset;
    plan.tempo = juce::jlimit ((double) style.minimumBpm, (double) style.maximumBpm, tempo);
    plan.key = { tonic.has_value() ? ((*tonic % 12) + 12) % 12 : style.tonics[(size_t) (seed == 0 ? 0 : seed % 3)], style.scale };

    if (const auto* structure = findStructure (style))
        plan.sections = structure->toSections (0);

    std::array<std::optional<guide::Progression>, 4> chosen {};
    for (int role = 0; role < 4; ++role)
        chosen[(size_t) role] = findProgression (plan.key, pick (style.progressions[(size_t) role], seed, role));

    for (const auto& section : plan.sections)
    {
        auto* progression = &chosen[(size_t) roleFor (style, section.type)];
        if (! progression->has_value())
            progression = &chosen[0];
        if (! progression->has_value())
            continue;

        const auto sequence = progression->value().getChords (plan.key);
        for (int bar = section.startBar; bar < section.getEndBar(); ++bar)
        {
            const int step = ((bar - section.startBar) / juce::jmax (1, style.barsPerChord)) % (int) sequence.size();
            plan.chords[bar] = sequence[(size_t) step];
        }
    }

    if (! plan.sections.empty())
        plan.chords[plan.getTotalBars() - 1] = guide::buildChord (plan.key, { 1 });

    for (const auto& drumPart : style.drums)
    {
        PartPlan part { drumPart.name, BuiltInSound::drums, volumeFor (drumPart.name), panFor (drumPart.name), true, {}, {} };

        for (int index = 0; index < (int) plan.sections.size(); ++index)
        {
            const auto& section = plan.sections[(size_t) index];
            if (! playsInSection (style, part.name, index, plan.sections))
                continue;

            ClipPlan clip { section.getDisplayName(), section.startBar * 4.0, section.bars * 4.0, {} };
            if (writeNotes)
            {
                if (section.type == SectionType::buildUp && isSecondDrum (part.name))
                {
                    const double total = section.bars * 4.0;
                    std::vector<double> starts;
                    for (double beat = 0.0; beat < total * 0.5 - 1.0e-6; beat += 1.0) starts.push_back (beat);
                    for (double beat = total * 0.5; beat < total * 0.75 - 1.0e-6; beat += 0.5) starts.push_back (beat);
                    for (double beat = total * 0.75; beat < total - 1.0e-6; beat += 0.25) starts.push_back (beat);

                    for (size_t i = 0; i < starts.size(); ++i)
                    {
                        const float velocity = starts.size() > 1 ? 0.4f + 0.6f * (float) i / (float) (starts.size() - 1) : 1.0f;
                        clip.notes.push_back ({ drumPart.pitch, starts[i], 0.12, velocity });
                    }
                }
                else if (style.family == Family::film && part.name == "Cymbal")
                {
                    clip.notes.push_back ({ drumPart.pitch, 0.0, 0.25, 0.9f });
                }
                else
                {
                    for (double repeat = 0.0; repeat < clip.lengthBeats - 1.0e-6; repeat += drumPart.patternBeats)
                        for (const auto& hit : drumPart.hits)
                            if (repeat + hit.beat < clip.lengthBeats - 1.0e-6)
                                clip.notes.push_back ({ drumPart.pitch, repeat + hit.beat, 0.18, hit.velocity });

                    if (style.id == "trap" && part.name == "Hats")
                        for (double bar = 0.0; bar + 8.0 <= clip.lengthBeats + 1.0e-6; bar += 8.0)
                            for (int hit = 0; hit < 8; ++hit)
                                clip.notes.push_back ({ drumPart.pitch, bar + 7.0 + hit * 0.125, 0.08,
                                                        0.4f + 0.4f * (float) hit / 7.0f });
                }
            }
            part.clips.push_back (std::move (clip));
        }
        plan.parts.push_back (std::move (part));
    }

    auto addPitched = [&] (const juce::String& name, const juce::String& styleId)
    {
        PartPlan part { name, soundFor (name, style.id), volumeFor (name), panFor (name), false, styleId, {} };
        const auto* playingStyle = styles::findStyle (styleId);

        for (int index = 0; index < (int) plan.sections.size(); ++index)
        {
            const auto& section = plan.sections[(size_t) index];
            if (! playsInSection (style, name, index, plan.sections))
                continue;

            ClipPlan clip { section.getDisplayName() + (name == "Melody" ? " melody" : ""), section.startBar * 4.0, section.bars * 4.0, {} };
            if (writeNotes && name != "Melody" && playingStyle != nullptr)
                clip.notes = styles::play (*playingStyle, chordSpans (plan, section), clip.lengthBeats, plan.tempo);
            part.clips.push_back (std::move (clip));
        }
        plan.parts.push_back (std::move (part));
    };

    addPitched ("Bass", style.bassStyle);
    addPitched ("Chords", style.chordStyle);
    if (style.padStyle.isNotEmpty()) addPitched ("Pad", style.padStyle);
    addPitched ("Melody", {});
    return plan;
}

bool build (Engine& engine, const SongPlan& plan, juce::String& error)
{
    const auto* style = findStyle (plan.styleId);
    const auto* kit = drumPreset (plan.drumKit);
    const auto* drumBusPreset = compressorPreset ("Drum bus");
    const auto* bassPreset = compressorPreset ("Bass");
    const auto* chordPreset = eqPreset ("Make room for a vocal");
    const auto* returnSettings = reverbPreset (plan.returnPreset);

    if (style == nullptr || kit == nullptr || drumBusPreset == nullptr || bassPreset == nullptr || chordPreset == nullptr || returnSettings == nullptr)
    {
        error = "A song template names a style, kit or starting point that is not available.";
        return false;
    }

    engine.resetEdit();
    engine.setTempo (plan.tempo);
    engine.setKey (plan.key);
    engine.setSections (plan.sections);
    engine.setChords (plan.chords);

    std::vector<Track*> drums;
    std::map<juce::String, Track*> tracks;
    struct Pending { MidiClip* clip; const std::vector<MidiNote>* notes; };
    std::vector<Pending> pending;

    for (const auto& part : plan.parts)
    {
        auto* track = engine.addTrack (Track::Type::midi);
        track->name = part.name;
        track->volumeDb = part.volumeDb;
        track->pan = part.pan;
        engine.setBuiltInSound (*track, part.sound);
        tracks[part.name] = track;

        if (part.drum)
        {
            drums.push_back (track);
            auto* slot = engine.addPlugin (*track, BuiltInDrums::getDescription(), error);
            auto* machine = slot != nullptr ? dynamic_cast<BuiltInDrums*> (slot->instance.get()) : nullptr;
            if (machine == nullptr) return false;
            machine->setSettings (kit->settings);
            stabiliseState (*machine);
        }

        for (const auto& clipPlan : part.clips)
        {
            auto* clip = engine.addMidiClip (*track, clipPlan.startBeat, clipPlan.lengthBeats);
            clip->name = clipPlan.name;
            pending.push_back ({ clip, &clipPlan.notes });
        }
    }

    {
        const juce::ScopedLock lock (engine.getLock());
        for (const auto& item : pending)
            item.clip->notes = *item.notes;
    }
    engine.notesChanged();

    auto* bus = engine.createBus (drums, "Drums");
    if (bus == nullptr) { error = "The Drums bus could not be made."; return false; }
    if (auto* slot = engine.addPlugin (*bus, BuiltInCompressor::getDescription(), error))
    {
        auto* compressor = dynamic_cast<BuiltInCompressor*> (slot->instance.get());
        if (compressor == nullptr) return false;
        compressor->applyPreset (*drumBusPreset);
        stabiliseState (*compressor);
    }
    else return false;

    auto addCompressor = [&] (Track* track, const comp::Preset& preset)
    {
        auto* slot = track != nullptr ? engine.addPlugin (*track, BuiltInCompressor::getDescription(), error) : nullptr;
        auto* processor = slot != nullptr ? dynamic_cast<BuiltInCompressor*> (slot->instance.get()) : nullptr;
        if (processor != nullptr) { processor->applyPreset (preset); stabiliseState (*processor); }
        return processor != nullptr;
    };
    auto addEq = [&] (Track* track, const eq::Preset& preset)
    {
        auto* slot = track != nullptr ? engine.addPlugin (*track, BuiltInEq::getDescription(), error) : nullptr;
        auto* processor = slot != nullptr ? dynamic_cast<BuiltInEq*> (slot->instance.get()) : nullptr;
        if (processor != nullptr) { processor->applyPreset (preset); stabiliseState (*processor); }
        return processor != nullptr;
    };

    if (! addCompressor (tracks["Bass"], *bassPreset) || ! addEq (tracks["Chords"], *chordPreset))
        return false;

    auto* ret = engine.addReturn ("Reverb");
    auto* slot = ret != nullptr ? engine.addPlugin (*ret, BuiltInReverb::getDescription(), error) : nullptr;
    auto* reverb = slot != nullptr ? dynamic_cast<BuiltInReverb*> (slot->instance.get()) : nullptr;
    if (reverb == nullptr) return false;
    reverb->setSettings (returnSettings->settings);
    stabiliseState (*reverb);

    engine.setSend (*tracks["Chords"], *ret, -12.0f);
    if (tracks.contains ("Pad")) engine.setSend (*tracks["Pad"], *ret, -8.0f);
    engine.setSend (*tracks["Melody"], *ret, -10.0f);

    if (style->family != Family::film)
        for (const auto& part : style->drums)
            if (isSecondDrum (part.name))
                engine.setSend (*tracks[part.name], *ret, -14.0f);

    return true;
}
} // namespace daw::gen::templates
