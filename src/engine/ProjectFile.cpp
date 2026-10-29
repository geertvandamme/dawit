#include "ProjectFile.h"

#include "Routing.h"

namespace daw::project
{
namespace
{
    constexpr const char* formatName = "DawIt project";
    constexpr const char* oldFormatName = "MiniDAW project";

    juce::var makeObject (std::initializer_list<std::pair<const char*, juce::var>> properties)
    {
        auto* object = new juce::DynamicObject();

        for (auto& [name, value] : properties)
            object->setProperty (name, value);

        return object;
    }

    /** Indented JSON that keeps short objects and arrays (a note, a chord, a key) on one line, so a
        clip's notes read as a list instead of six lines each. */
    void writeJson (juce::String& out, const juce::var& value, int indent)
    {
        const auto compact = juce::JSON::toString (value, juce::JSON::FormatOptions{}
                                                              .withSpacing (juce::JSON::Spacing::singleLine)
                                                              .withMaxDecimalPlaces (6));
        auto* array = value.getArray();
        auto* object = value.getDynamicObject();

        if ((array == nullptr && object == nullptr) || indent + compact.length() <= 100)
        {
            out << compact;
            return;
        }

        const auto pad = juce::String::repeatedString (" ", indent + 2);

        if (array != nullptr)
        {
            out << "[\n";

            for (int i = 0; i < array->size(); ++i)
            {
                out << pad;
                writeJson (out, array->getReference (i), indent + 2);
                out << (i + 1 < array->size() ? ",\n" : "\n");
            }

            out << juce::String::repeatedString (" ", indent) << "]";
            return;
        }

        const auto& properties = object->getProperties();
        out << "{\n";

        for (int i = 0; i < properties.size(); ++i)
        {
            out << pad << juce::JSON::toString (properties.getName (i).toString()) << ": ";
            writeJson (out, properties.getValueAt (i), indent + 2);
            out << (i + 1 < properties.size() ? ",\n" : "\n");
        }

        out << juce::String::repeatedString (" ", indent) << "}";
    }

    juce::String toPath (const juce::File& file, const juce::File& projectFile)
    {
        if (projectFile == juce::File())
            return file.getFullPathName();

        return file.getRelativePathFrom (projectFile.getParentDirectory()).replaceCharacter ('\\', '/');
    }

    //==============================================================================
    juce::var pluginToJson (const PluginSlot& slot, bool includeState)
    {
        const auto description = slot.instance->getPluginDescription();
        juce::String state;

        if (includeState)
        {
            juce::MemoryBlock block;
            slot.instance->getStateInformation (block);
            state = block.toBase64Encoding();
        }

        return makeObject ({ { "name", description.name },
                             { "manufacturer", description.manufacturerName },
                             { "format", description.pluginFormatName },
                             { "id", description.createIdentifierString() },
                             { "file", description.fileOrIdentifier },
                             { "bypassed", slot.bypassed.load() },
                             { "state", state } });
    }

    juce::var clipToJson (const Clip& clip, const juce::File& projectFile)
    {
        if (clip.kind == Clip::Kind::audio)
        {
            auto& audio = static_cast<const AudioClip&> (clip);
            return makeObject ({ { "type", "audio" },
                                 { "name", audio.name },
                                 { "start", audio.startBeat },
                                 { "muted", audio.muted },
                                 { "gainDb", audio.gainDb },
                                 { "file", toPath (audio.file, projectFile) } });
        }

        auto& midi = static_cast<const MidiClip&> (clip);
        juce::Array<juce::var> notes;

        for (auto& n : midi.notes)
            notes.add (makeObject ({ { "pitch", n.pitch },
                                     { "start", n.start },
                                     { "length", n.length },
                                     { "velocity", juce::jlimit (1, 127, juce::roundToInt (n.velocity * 127.0f)) } }));

        return makeObject ({ { "type", "midi" },
                             { "name", midi.name },
                             { "start", midi.startBeat },
                             { "muted", midi.muted },
                             { "length", midi.lengthBeats },
                             { "notes", notes } });
    }

    /** The index of a return among the returns, in file order (the order of the tracks list). */
    int getReturnIndex (const Track& ret, const Tracks& all)
    {
        int index = 0;

        for (auto& t : all)
        {
            if (t.get() == &ret)
                return index;

            if (t->isReturn())
                ++index;
        }

        return -1;
    }

    const char* getScoreClefId (ScoreClef clef)
    {
        switch (clef)
        {
            case ScoreClef::automatic:  return "auto";
            case ScoreClef::treble:     return "treble";
            case ScoreClef::bass:       return "bass";
            case ScoreClef::grand:      return "grand";
            case ScoreClef::guitar:     return "guitar";
            case ScoreClef::bassGuitar: return "bassGuitar";
        }

        return "auto";
    }

    /** The clef saved under this id; Auto for a missing or unknown one. */
    ScoreClef getScoreClefFromId (const juce::String& id)
    {
        for (auto clef : { ScoreClef::treble, ScoreClef::bass, ScoreClef::grand, ScoreClef::guitar, ScoreClef::bassGuitar })
            if (id == getScoreClefId (clef))
                return clef;

        return ScoreClef::automatic;
    }

    juce::var trackToJson (const Track& track, const Tracks& all, const juce::File& projectFile, bool includePluginState)
    {
        juce::Array<juce::var> plugins, clips, members;

        for (auto& slot : track.plugins)
            plugins.add (pluginToJson (*slot, includePluginState));

        for (auto& clip : track.clips)
            clips.add (clipToJson (*clip, projectFile));

        for (auto& other : all)
            if (other->bus == &track)
                members.add (trackToJson (*other, all, projectFile, includePluginState));

        auto json = makeObject ({ { "type", track.isReturn() ? "return" : track.isBus() ? "bus" : track.isMidi() ? "midi" : "audio" },
                                  { "name", track.name },
                                  { "colour", "#" + track.colour.toDisplayString (false).toLowerCase() },
                                  { "volumeDb", track.volumeDb.load() },
                                  { "pan", track.pan.load() },
                                  { "muted", track.muted.load() },
                                  { "soloed", track.soloed.load() } });

        if (track.isMidi())
        {
            json.getDynamicObject()->setProperty ("scaleFilter", track.scaleFilter.load());
            json.getDynamicObject()->setProperty ("builtInSound", builtInSounds::getId (track.builtInSound.load()));

            // The Score view's clef, left out for Auto.
            if (track.scoreClef != ScoreClef::automatic)
                json.getDynamicObject()->setProperty ("scoreClef", getScoreClefId (track.scoreClef));
        }

        // The input an audio track records, left out for the default (Input 1, mono). Arm and Monitor aren't saved.
        if (track.isAudio() && (track.inputChannel.load() != 0 || track.inputStereo.load()))
        {
            json.getDynamicObject()->setProperty ("inputChannel", track.inputChannel.load());

            if (track.inputStereo.load())
                json.getDynamicObject()->setProperty ("inputStereo", true);
        }

        if (track.isBus() && track.folded)
            json.getDynamicObject()->setProperty ("folded", true);

        json.getDynamicObject()->setProperty ("plugins", plugins);

        if (! track.hasInput())
            json.getDynamicObject()->setProperty ("clips", clips);

        juce::Array<juce::var> sendList;

        for (auto& send : track.sends)
        {
            if (! send->isOn())
                continue;

            auto entry = makeObject ({ { "return", getReturnIndex (*send->target, all) }, { "levelDb", send->levelDb.load() } });

            if (send->preFader.load())
                entry.getDynamicObject()->setProperty ("preFader", true);

            sendList.add (entry);
        }

        if (! sendList.isEmpty())
            json.getDynamicObject()->setProperty ("sends", sendList);

        if (! members.isEmpty())
            json.getDynamicObject()->setProperty ("tracks", members);

        return json;
    }

    //==============================================================================
    std::optional<int> parsePitchClass (const juce::String& name)
    {
        if (auto chord = Chord::fromName (name))
            return chord->root;

        return std::nullopt;
    }

    MusicalKey parseKey (const juce::var& json)
    {
        MusicalKey key;
        key.tonic = parsePitchClass (json["tonic"].toString()).value_or (0);

        for (int i = 0; i < MusicalKey::numScales; ++i)
            if (MusicalKey::getScaleName ((MusicalKey::Scale) i) == json["scale"].toString())
                key.scale = (MusicalKey::Scale) i;

        return key;
    }

    std::optional<Chord> parseChord (const juce::var& json)
    {
        // The exact notes when present, so any chord survives; otherwise the chord symbol.
        if (auto* intervals = json["intervals"].getArray(); intervals != nullptr && ! intervals->isEmpty())
        {
            Chord chord;
            chord.root = ((int) json["root"] % 12 + 12) % 12;
            chord.intervals.clear();

            for (auto& interval : *intervals)
                chord.intervals.add ((int) interval);

            return chord;
        }

        return Chord::fromName (json["chord"].toString());
    }

    /** The plugin's description from the scanned list, or else from the plugin file itself. */
    std::unique_ptr<juce::PluginDescription> findPlugin (Engine& engine, const juce::var& json)
    {
        if (auto known = engine.knownPlugins.getTypeForIdentifierString (json["id"].toString()))
            return known;

        for (auto* format : engine.pluginFormats.getFormats())
        {
            if (format->getName() != json["format"].toString())
                continue;

            juce::OwnedArray<juce::PluginDescription> found;
            format->findAllTypesForFile (found, json["file"].toString());

            for (auto* description : found)
                if (description->name == json["name"].toString() || found.size() == 1)
                    return std::make_unique<juce::PluginDescription> (*description);
        }

        return nullptr;
    }

    void loadPlugin (Engine& engine, Track& track, const juce::var& json, juce::StringArray& warnings)
    {
        const auto name = json["name"].toString();
        auto description = findPlugin (engine, json);

        if (description == nullptr)
        {
            warnings.add ("\"" + name + "\" on " + track.name + " isn't installed or hasn't been scanned.");
            return;
        }

        juce::String error;
        auto* slot = engine.addPlugin (track, *description, error);

        if (slot == nullptr)
        {
            warnings.add ("\"" + name + "\" on " + track.name + " couldn't be loaded: " + error);
            return;
        }

        if (juce::MemoryBlock state; state.fromBase64Encoding (json["state"].toString()) && state.getSize() > 0)
            slot->instance->setStateInformation (state.getData(), (int) state.getSize());

        if ((bool) json["bypassed"])
            engine.setPluginBypassed (*slot, true);
    }

    void loadClip (Engine& engine, Track& track, const juce::var& json, const juce::File& projectFile, juce::StringArray& warnings)
    {
        const double start = juce::jmax (0.0, (double) json["start"]);
        const bool isAudio = json["type"].toString() == "audio";

        if (isAudio == track.isMidi())
        {
            warnings.add ("Skipped " + json["type"].toString() + " clip \"" + json["name"].toString() + "\" on " + track.name
                          + ", which is a " + (track.isMidi() ? "MIDI" : "audio") + " track.");
            return;
        }

        if (isAudio)
        {
            const auto file = projectFile.getParentDirectory().getChildFile (json["file"].toString());
            juce::String error;

            if (! file.existsAsFile())
                warnings.add ("The audio file \"" + json["file"].toString() + "\" on " + track.name + " is missing.");
            else if (auto* clip = engine.addAudioClip (track, file, start, error))
            {
                clip->name = json["name"].toString();
                clip->muted = (bool) json["muted"];
                clip->gainDb = juce::jlimit (AudioClip::minGainDb, AudioClip::maxGainDb, (float) json.getProperty ("gainDb", 0.0));
            }
            else
                warnings.add (error);

            return;
        }

        auto* clip = engine.addMidiClip (track, start, (double) json["length"]);
        clip->name = json["name"].toString();
        clip->muted = (bool) json["muted"];

        if (auto* notes = json["notes"].getArray())
        {
            const juce::ScopedLock sl (engine.getLock());

            for (auto& n : *notes)
                clip->notes.push_back ({ juce::jlimit (0, 127, (int) n["pitch"]),
                                         juce::jmax (0.0, (double) n["start"]),
                                         juce::jmax (1.0 / 64.0, (double) n["length"]),
                                         juce::jlimit (1, 127, (int) n["velocity"]) / 127.0f });
        }
    }

    /** A send read from the file, resolved once every track exists (`returnIndex` counts the returns in file order). */
    struct PendingSend
    {
        Track* from;
        int returnIndex;
        float levelDb;
        bool preFader;
    };

    /** Makes a track from its JSON, inside `parent` (null: the top level), then the tracks inside it. */
    void loadTrack (Engine& engine, const juce::var& t, Track* parent, const juce::File& projectFile, juce::StringArray& warnings,
                    std::vector<PendingSend>& pendingSends)
    {
        const auto type = t["type"].toString();
        auto* track = type == "return" ? engine.addReturn ("Return") : type == "bus" ? engine.addBus() : engine.addTrack (type == "audio" ? Track::Type::audio : Track::Type::midi);

        if (auto* sendList = t["sends"].getArray(); sendList != nullptr && ! track->isReturn())
            for (auto& entry : *sendList)
                pendingSends.push_back ({ track, (int) entry.getProperty ("return", -1), (float) entry.getProperty ("levelDb", -10.0), (bool) entry["preFader"] });

        if (parent != nullptr)
            engine.moveTrack (*track, parent);

        track->name = t["name"].toString();

        if (const auto colour = t["colour"].toString().trimCharactersAtStart ("#"); colour.length() == 6)
            track->colour = juce::Colour::fromString ("ff" + colour);

        track->volumeDb = juce::jlimit (-60.0f, 6.0f, (float) t.getProperty ("volumeDb", 0.0));
        track->pan = juce::jlimit (-1.0f, 1.0f, (float) t.getProperty ("pan", 0.0));
        track->muted = (bool) t["muted"];
        track->soloed = (bool) t["soloed"];

        if (track->isAudio())
            engine.setInput (*track, juce::jlimit (0, 63, (int) t.getProperty ("inputChannel", 0)), (bool) t["inputStereo"]);

        engine.setScaleFilter (*track, track->isMidi() && (bool) t["scaleFilter"]);

        if (const auto sound = builtInSounds::fromId (t["builtInSound"].toString()))
            engine.setBuiltInSound (*track, *sound);

        if (track->isMidi())
            engine.setScoreClef (*track, getScoreClefFromId (t["scoreClef"].toString()));

        if (auto* plugins = t["plugins"].getArray())
            for (auto& plugin : *plugins)
                loadPlugin (engine, *track, plugin, warnings);

        if (! track->hasInput())
            if (auto* clips = t["clips"].getArray())
                for (auto& clip : *clips)
                    loadClip (engine, *track, clip, projectFile, warnings);

        if (auto* members = t["tracks"].getArray())
        {
            if (! track->isBus())
                warnings.add (track->name + " isn't a bus, so the tracks listed inside it were put at the top level.");

            for (auto& member : *members)
                loadTrack (engine, member, track->isBus() ? track : nullptr, projectFile, warnings, pendingSends);
        }

        if (track->isBus())
            engine.setFolded (*track, (bool) t["folded"]);
    }

    bool writeAudio (const AudioClip& clip, const juce::File& target)
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (target.createOutputStream());

        if (stream == nullptr)
            return false;

        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                       .withSampleRate (clip.sourceSampleRate)
                                                       .withNumChannels (clip.audio.getNumChannels())
                                                       .withBitsPerSample (24));

        return writer != nullptr && writer->writeFromAudioSampleBuffer (clip.audio, 0, clip.audio.getNumSamples());
    }

    /** Puts the clip's audio in the project's audio folder, unless it is there already. */
    bool collectAudio (AudioClip& clip, const juce::File& folder, juce::String& error)
    {
        if (clip.file.isAChildOf (folder) && clip.file.existsAsFile())
            return true;

        if (! folder.createDirectory())
        {
            error = "Couldn't create " + folder.getFullPathName();
            return false;
        }

        auto target = folder.getChildFile (clip.file.getFileName());

        if (clip.file.existsAsFile())
        {
            // A file of the same name that is a different recording gets a numbered name.
            if (target.existsAsFile() && ! target.hasIdenticalContentTo (clip.file))
                target = folder.getNonexistentChildFile (clip.file.getFileNameWithoutExtension(), clip.file.getFileExtension(), false);

            if (! target.existsAsFile() && ! clip.file.copyFileTo (target))
            {
                error = "Couldn't copy " + clip.file.getFullPathName() + " to " + folder.getFullPathName();
                return false;
            }
        }
        else
        {
            // The original file is gone, but the clip still holds its audio.
            const auto name = clip.file.getFileNameWithoutExtension().isNotEmpty() ? clip.file.getFileNameWithoutExtension() : clip.name;
            target = folder.getNonexistentChildFile (juce::File::createLegalFileName (name), ".wav", false);

            if (! writeAudio (clip, target))
            {
                error = "Couldn't write " + target.getFullPathName();
                return false;
            }
        }

        clip.file = target;
        return true;
    }
}

//==============================================================================
juce::String toText (const Engine& engine, const juce::File& projectFile, bool includePluginState)
{
    juce::Array<juce::var> chords, tracks;

    for (auto& [bar, chord] : engine.getChords())
    {
        juce::Array<juce::var> intervals;
        for (int interval : chord.intervals)
            intervals.add (interval);

        chords.add (makeObject ({ { "bar", bar }, { "chord", chord.getName() }, { "root", chord.root }, { "intervals", intervals } }));
    }

    juce::Array<juce::var> sections;

    for (auto& section : engine.getSections())
        sections.add (makeObject ({ { "start", section.startBar }, { "bars", section.bars },
                                    { "type", sectionTypes::getId (section.type) }, { "name", section.name } }));

    // Lyrics: a line per sung line; a syllable has a beat only when the user placed it, the others are spread out.
    juce::Array<juce::var> lyricLines;

    for (auto& line : engine.getLyrics().lines)
    {
        juce::Array<juce::var> syllables;

        for (auto& s : line.syllables)
        {
            auto* syllable = new juce::DynamicObject();
            syllable->setProperty ("text", s.text);

            if (! s.endsWord)
                syllable->setProperty ("joined", true);

            if (s.pinned)
                syllable->setProperty ("beat", s.beat);

            syllables.add (syllable);
        }

        auto* object = new juce::DynamicObject();
        object->setProperty ("start", line.startBeat);
        object->setProperty ("end", line.endBeat);

        if (line.startsVerse)
            object->setProperty ("verse", true);

        if (line.heading.isNotEmpty())
            object->setProperty ("heading", line.heading);

        object->setProperty ("syllables", syllables);
        lyricLines.add (object);
    }

    bool anyBus = false, anyReturn = false;

    for (auto& track : engine.getTracks())
    {
        anyBus = anyBus || track->isBus();
        anyReturn = anyReturn || track->isReturn();

        if (track->bus == nullptr)
            tracks.add (trackToJson (*track, engine.getTracks(), projectFile, includePluginState));
    }

    const auto& key = engine.getKey();
    const auto& tempo = engine.getTempoMap();

    // A song that follows a band: [beat, BPM] for each change after the first tempo.
    juce::Array<juce::var> tempoChanges;
    for (size_t i = 1; i < tempo.getChanges().size(); ++i)
        tempoChanges.add (juce::Array<juce::var> { tempo.getChanges()[i].beat, tempo.getChanges()[i].bpm });

    auto json = makeObject ({
        { "format", formatName },
        { "version", anyReturn ? formatVersion : anyBus ? formatVersionWithoutReturns : formatVersionWithoutBuses },
        { "tempo", tempo.getChanges().front().bpm },
        { "tempoChanges", tempoChanges },
        { "key", makeObject ({ { "tonic", MusicalKey::getTonicName (key.tonic) }, { "scale", MusicalKey::getScaleName (key.scale) } }) },
        { "masterVolumeDb", engine.masterVolumeDb.load() },
        { "loop", makeObject ({ { "enabled", engine.loopEnabled.load() },
                                { "start", engine.loopStartBeat.load() },
                                { "end", engine.loopEndBeat.load() } }) },
        { "chords", chords },
        { "sections", sections },
        { "lyrics", lyricLines },
        { "tracks", tracks } });

    if (const auto& brief = engine.getLyricsBrief(); ! brief.isEmpty())
    {
        auto* object = new juce::DynamicObject();

        if (brief.about.isNotEmpty())       object->setProperty ("about", brief.about);
        if (brief.mood.isNotEmpty())        object->setProperty ("mood", brief.mood);
        if (brief.pointOfView.isNotEmpty()) object->setProperty ("pointOfView", brief.pointOfView);

        json.getDynamicObject()->setProperty ("lyricsBrief", juce::var (object));
    }

    if (tempoChanges.isEmpty())
        json.getDynamicObject()->removeProperty ("tempoChanges");   // one tempo, as in projects from before tempo changes

    juce::String text;
    writeJson (text, json, 0);
    return text + "\n";
}

bool save (Engine& engine, const juce::File& projectFile, juce::String& savedText, juce::String& error)
{
    const auto audioFolder = projectFile.getSiblingFile (audioFolderName);

    for (auto& track : engine.getTracks())
        for (auto& clip : track->clips)
            if (clip->kind == Clip::Kind::audio && ! collectAudio (static_cast<AudioClip&> (*clip), audioFolder, error))
                return false;

    const auto text = toText (engine, projectFile);
    juce::TemporaryFile temp (projectFile);

    if (! temp.getFile().replaceWithText (text, false, false, "\n") || ! temp.overwriteTargetFileWithTemporary())
    {
        error = "Couldn't write " + projectFile.getFullPathName();
        return false;
    }

    savedText = text;
    return true;
}

bool load (Engine& engine, const juce::File& projectFile, juce::String& error, juce::StringArray& warnings)
{
    juce::var json;

    if (const auto result = juce::JSON::parse (projectFile.loadFileAsString(), json); result.failed())
    {
        error = projectFile.getFileName() + " isn't valid JSON: " + result.getErrorMessage();
        return false;
    }

    if (const auto format = json["format"].toString(); format != formatName && format != oldFormatName)
    {
        error = projectFile.getFileName() + " isn't a DawIt project.";
        return false;
    }

    if ((int) json["version"] > formatVersion)
    {
        error = projectFile.getFileName() + " was saved by a newer version of DawIt.";
        return false;
    }

    engine.resetEdit();

    std::vector<TempoChange> tempoChanges { { 0.0, (double) json.getProperty ("tempo", 120.0) } };

    if (auto* list = json["tempoChanges"].getArray())
        for (auto& change : *list)
            if (auto* pair = change.getArray(); pair != nullptr && pair->size() == 2)
                tempoChanges.push_back ({ (double) (*pair)[0], (double) (*pair)[1] });

    engine.setTempoMap (TempoMap (std::move (tempoChanges)));
    engine.setKey (parseKey (json["key"]));
    engine.masterVolumeDb = (float) json.getProperty ("masterVolumeDb", 0.0);

    const auto& loop = json["loop"];
    engine.loopStartBeat = juce::jmax (0.0, (double) loop.getProperty ("start", 0.0));
    engine.loopEndBeat = juce::jmax (engine.loopStartBeat.load(), (double) loop.getProperty ("end", 16.0));
    engine.loopEnabled = (bool) loop["enabled"];

    ChordMap chords;
    if (auto* list = json["chords"].getArray())
        for (auto& entry : *list)
            if (auto chord = parseChord (entry); chord.has_value() && (int) entry["bar"] >= 0)
                chords[(int) entry["bar"]] = *chord;

    engine.setChords (std::move (chords));

    SectionList sections;
    if (auto* list = json["sections"].getArray())
        for (auto& entry : *list)
            sections.push_back ({ (int) entry["start"], (int) entry["bars"],
                                  sectionTypes::fromId (entry["type"].toString()).value_or (SectionType::verse), entry["name"].toString() });

    engine.setSections (std::move (sections));

    Lyrics lyrics;
    if (auto* list = json["lyrics"].getArray())
    {
        for (auto& entry : *list)
        {
            LyricLine line;
            line.startBeat = (double) entry.getProperty ("start", 0.0);
            line.endBeat = (double) entry.getProperty ("end", line.startBeat + 1.0);
            line.startsVerse = (bool) entry["verse"];
            line.heading = entry["heading"].toString();

            if (auto* syllables = entry["syllables"].getArray())
                for (auto& s : *syllables)
                    if (const auto text = s["text"].toString().trim(); text.isNotEmpty())
                        line.syllables.push_back ({ text, ! (bool) s["joined"], s.hasProperty ("beat"), (double) s.getProperty ("beat", 0.0) });

            lyrics.lines.push_back (std::move (line));
        }
    }

    engine.setLyrics (std::move (lyrics));

    // The song brief for the lyrics AI; projects from before it have none.
    {
        LyricsBrief brief;
        const auto& saved = json["lyricsBrief"];
        brief.about = saved["about"].toString();
        brief.mood = saved["mood"].toString();
        brief.pointOfView = saved["pointOfView"].toString();
        engine.setLyricsBrief (std::move (brief));
    }

    std::vector<PendingSend> pendingSends;

    if (auto* tracks = json["tracks"].getArray())
        for (auto& t : *tracks)
            loadTrack (engine, t, nullptr, projectFile, warnings, pendingSends);

    const auto returns = routing::getReturns (engine.getTracks());

    for (auto& pending : pendingSends)
    {
        if (pending.returnIndex < 0 || pending.returnIndex >= (int) returns.size())
        {
            warnings.add (pending.from->name + " sends to a return that isn't in the project, so that send was left out.");
            continue;
        }

        engine.setSend (*pending.from, *returns[(size_t) pending.returnIndex], pending.levelDb);

        if (auto* send = routing::findSend (*pending.from, *returns[(size_t) pending.returnIndex]))
            send->preFader = pending.preFader;
    }

    engine.notesChanged();
    return true;
}
} // namespace daw::project
