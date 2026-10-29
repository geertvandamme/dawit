#include "Engine.h"
#include "LatencyMeasure.h"
#include "BuiltInFormat.h"
#include "GainStaging.h"
#include "Routing.h"

namespace daw
{
namespace
{
    constexpr const char* knownPluginsKey = "knownPlugins";
    constexpr const char* audioDeviceKey = "audioDevice";
}

/** One armed audio track's recording while it runs (see Engine::captureAudio and drainAudioRecording). The audio thread
    writes the FIFOs, the message thread reads them and owns everything else. */
struct Engine::AudioRecording
{
    /** A pass of a loop starts (or the playhead jumped), or samples were dropped, at a place in the stream. */
    struct Event
    {
        juce::int64 position = 0;   // in samples offered to the FIFO since recording began
        double beat = 0.0;          // pass start: the song position of the first sample
        juce::int64 lost = 0;       // gap: how many samples are missing
        bool isGap = false;
    };

    static constexpr int numEvents = 256;

    AudioRecording (Track& t, int channel, bool isStereo, double sampleRateHz, int shift)
        : track (&t), firstChannel (channel), stereo (isStereo), numChannels (isStereo ? 2 : 1), rate (sampleRateHz),
          latencyShift (shift), fifoData (isStereo ? 2 : 1, juce::jmax (1, (int) (sampleRateHz * 10.0))),
          fifo (fifoData.getNumSamples()), stream (isStereo ? 2 : 1, (int) sampleRateHz * 30)
    {
    }

    ~AudioRecording()
    {
        raw.reset();            // flushes and closes the crash-safety file
        rawFile.deleteFile();   // what it was for is in the take files by now (after a crash it stays)
    }

    Track* track;
    const int firstChannel;
    const bool stereo;
    const int numChannels;
    const double rate;
    const int latencyShift;

    // Shared: the audio thread writes, the message thread reads.
    juce::AudioBuffer<float> fifoData;
    juce::AbstractFifo fifo;
    std::array<Event, numEvents> eventData;
    juce::AbstractFifo events { numEvents };
    std::atomic<juce::int64> dropped { 0 };
    std::atomic<juce::int64> offeredTotal { 0 };   // `offered`, for the message thread when recording ends

    // Audio thread.
    juce::int64 offered = 0;
    juce::int64 gapStart = 0, gapLength = 0;   // samples dropped in a row, pushed as one event when they end

    // Message thread: the take so far, as the stream of offered samples (a dropped stretch is silence).
    juce::AudioBuffer<float> stream;
    juce::int64 length = 0;
    std::vector<Event> passStarts, gaps;
    size_t nextGap = 0;
    std::vector<float> peaks;                  // per LiveTake::peakBlockSize samples
    juce::File rawFile;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> raw;
};

Engine::Engine (bool scratchSettings)
{
    juce::PropertiesFile::Options options;
    options.applicationName = "DawIt";
    options.filenameSuffix = ".settings";
    options.folderName = scratchSettings ? "DawIt/Scratch" : "DawIt";
    options.osxLibrarySubFolder = "Application Support";

    if (scratchSettings)
        options.getDefaultFile().deleteFile();

    // The app used to be called MiniDAW: bring its settings (scanned plugins, recent projects,
    // devices) along the first time.
    if (const auto settingsFile = options.getDefaultFile(); ! scratchSettings && ! settingsFile.exists())
    {
        auto old = options;
        old.applicationName = old.folderName = "MiniDAW";

        if (const auto oldFile = old.getDefaultFile(); oldFile.existsAsFile() && settingsFile.getParentDirectory().createDirectory())
            oldFile.copyFileTo (settingsFile);
    }

    properties.setStorageParameters (options);
    defaultRecordingFolder = options.getDefaultFile().getParentDirectory().getChildFile ("Recordings");

    audioFormats.registerBasicFormats();
    pluginFormats.addDefaultFormats();
    pluginFormats.addFormat (new BuiltInFormat());   // DawIt EQ, Compressor, Gate, Reverb, Delay, Saturation, Distortion, Subtractive, Additive, FM, Drums and Physical

    auto* settings = properties.getUserSettings();

    if (auto xml = settings->getXmlValue (knownPluginsKey))
        knownPlugins.recreateFromXml (*xml);

    knownPlugins.addChangeListener (this);

    auto savedDevice = settings->getXmlValue (audioDeviceKey);

    // The inputs open only while an audio track is armed (see updateInputs()), so a saved device starts without them.
    if (savedDevice != nullptr)
    {
        savedDevice->removeAttribute ("audioDeviceInChans");
        savedDevice->removeAttribute ("audioInputDeviceName");   // an input on another device makes JUCE join two devices, which then barely calls back
    }

    deviceManager.initialise (0, 2, savedDevice.get(), true);

    if (savedDevice == nullptr)
        for (auto& device : juce::MidiInput::getAvailableDevices())
            deviceManager.setMidiInputDeviceEnabled (device.identifier, true);

    deviceManager.addMidiInputDeviceCallback ({}, &midiCollector);
    deviceManager.addAudioCallback (this);
    deviceManager.addChangeListener (this);
    loadMeasuredLatency();
}

Engine::~Engine()
{
    deviceManager.removeChangeListener (this);
    knownPlugins.removeChangeListener (this);
    deviceManager.removeAudioCallback (this);
    deviceManager.removeMidiInputDeviceCallback ({}, &midiCollector);
    deviceManager.closeAudioDevice();

    audioRecordings.clear();
    writerThread.stopThread (2000);
    tracks.clear();
}

juce::File Engine::getDeadMansPedalFile()
{
    return properties.getUserSettings()->getFile().getSiblingFile ("RecentlyCrashedPluginsList");
}

void Engine::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    auto* settings = properties.getUserSettings();

    if (source == &knownPlugins)
    {
        if (auto xml = knownPlugins.createXml())
            settings->setValue (knownPluginsKey, xml.get());
    }
    else if (source == &deviceManager)
    {
        if (auto xml = deviceManager.createStateXml())
            settings->setValue (audioDeviceKey, xml.get());

        loadMeasuredLatency();
    }

    settings->saveIfNeeded();
}

//==============================================================================
juce::Colour Engine::nextTrackColour()
{
    static const juce::uint32 palette[] = { 0xff4fa3ff, 0xffff8a4f, 0xff5fd38d, 0xffc57bff,
                                            0xffffd24f, 0xff4fe0e0, 0xffff5c8a, 0xffa3c94f };
    return juce::Colour (palette[(size_t) colourIndex++ % std::size (palette)]);
}

juce::String Engine::nextBusName() const
{
    for (int n = 1;; ++n)
    {
        const auto name = "Bus " + juce::String (n);

        if (std::none_of (tracks.begin(), tracks.end(), [&name] (auto& t) { return t->isBus() && t->name == name; }))
            return name;
    }
}

juce::String Engine::uniqueReturnName (const juce::String& wanted) const
{
    auto isTaken = [this] (const juce::String& name)
    {
        return std::any_of (tracks.begin(), tracks.end(), [&name] (auto& t) { return t->isReturn() && t->name == name; });
    };

    if (wanted.isEmpty())
    {
        for (int n = 1;; ++n)
            if (! isTaken ("Return " + juce::String (n)))
                return "Return " + juce::String (n);
    }

    if (! isTaken (wanted))
        return wanted;

    for (int n = 2;; ++n)
        if (! isTaken (wanted + " " + juce::String (n)))
            return wanted + " " + juce::String (n);
}

std::pair<Track*, const Track*> Engine::findInsertion (int index) const
{
    const int n = (int) tracks.size();

    if (index < 0 || index > n)
        return { nullptr, nullptr };

    if (index == 0)
        return { nullptr, n > 0 ? tracks[0].get() : nullptr };

    auto* reference = tracks[(size_t) index - 1].get();
    auto* parent = reference->bus;
    const int position = reference->isBus() ? routing::getBlockEnd (tracks, index - 1) : index;
    const Track* before = position < n && tracks[(size_t) position]->bus == parent ? tracks[(size_t) position].get() : nullptr;
    return { parent, before };
}

void Engine::insertTrack (std::unique_ptr<Track> track, Track* bus, const Track* before)
{
    int position = (int) tracks.size();

    if (before != nullptr)
        position = indexOf (before);
    else if (bus != nullptr)
        position = routing::getBlockEnd (tracks, indexOf (bus));
    else if (! track->isReturn())
        while (position > 0 && tracks[(size_t) position - 1]->isReturn()) // a new track goes above the returns
            --position;

    if (position < 0)
        position = (int) tracks.size();

    track->bus = bus;
    tracks.insert (tracks.begin() + position, std::move (track));
    updateRouting();
}

void Engine::updateRouting()
{
    renderOrder = routing::getRenderOrder (tracks);
}

Track* Engine::addTrack (Track::Type type, int index)
{
    if (type == Track::Type::bus)
        return addBus (index);

    if (type == Track::Type::fxReturn)
        return addReturn();

    int count = 1;
    for (auto& t : tracks)
        if (t->type == type)
            ++count;

    const auto name = (type == Track::Type::midi ? "MIDI " : "Audio ") + juce::String (count);
    auto track = std::make_unique<Track> (type, name, nextTrackColour());

    if (sampleRate > 0.0)
        track->prepare (sampleRate, maxBlockSize);

    auto* raw = track.get();
    {
        const juce::ScopedLock sl (lock);
        const auto [bus, before] = findInsertion (index);
        insertTrack (std::move (track), bus, before);
    }

    sendChangeMessage();
    return raw;
}

Track* Engine::addReturn (const juce::String& name)
{
    std::unique_ptr<Track> track;
    Track* raw = nullptr;
    {
        const juce::ScopedLock sl (lock);
        track = std::make_unique<Track> (Track::Type::fxReturn, uniqueReturnName (name), nextTrackColour());
        raw = track.get();

        if (sampleRate > 0.0)
            track->prepare (sampleRate, maxBlockSize);

        insertTrack (std::move (track), nullptr, nullptr);
    }

    sendChangeMessage();
    return raw;
}

bool Engine::setSend (Track& from, Track& ret, float levelDb)
{
    if (! ret.isReturn() || from.isReturn() || indexOf (&ret) < 0 || indexOf (&from) < 0)
        return false;

    levelDb = juce::jlimit (sendOffDb, 6.0f, levelDb);

    // At Off a send that exists stays (it keeps its pre-fader choice, and isn't saved); one that doesn't isn't made.
    if (levelDb <= sendOffDb && routing::findSend (from, ret) == nullptr)
        return true;

    bool changed = false;
    {
        const juce::ScopedLock sl (lock);

        if (auto* existing = routing::findSend (from, ret))
        {
            changed = ! juce::exactlyEqual (existing->levelDb.load(), levelDb);
            existing->levelDb = levelDb;
        }
        else
        {
            auto send = std::make_unique<Send> (ret);
            send->levelDb = levelDb;

            if (sampleRate > 0.0)
                send->gain.reset (sampleRate, 0.02);

            from.sends.push_back (std::move (send));
            changed = true;
        }
    }

    if (changed)
        sendChangeMessage();

    return true;
}

void Engine::removeSend (Track& from, Track& ret)
{
    std::unique_ptr<Send> removed;
    {
        const juce::ScopedLock sl (lock);

        for (auto it = from.sends.begin(); it != from.sends.end(); ++it)
        {
            if ((*it)->target == &ret)
            {
                removed = std::move (*it);
                from.sends.erase (it);
                break;
            }
        }
    }

    if (removed != nullptr)
        sendChangeMessage();
}

Track* Engine::addBus (int index)
{
    auto track = std::make_unique<Track> (Track::Type::bus, nextBusName(), nextTrackColour());

    if (sampleRate > 0.0)
        track->prepare (sampleRate, maxBlockSize);

    auto* raw = track.get();
    {
        const juce::ScopedLock sl (lock);
        const auto [bus, before] = findInsertion (index);
        insertTrack (std::move (track), bus, before);
    }

    sendChangeMessage();
    return raw;
}

void Engine::relocate (Track& track, Track* bus, const Track* before)
{
    const int first = indexOf (&track);

    if (first < 0)
        return;

    const int last = routing::getBlockEnd (tracks, first);
    std::vector<std::unique_ptr<Track>> block;

    for (int i = first; i < last; ++i)
        block.push_back (std::move (tracks[(size_t) i]));

    tracks.erase (tracks.begin() + first, tracks.begin() + last);

    int position = (int) tracks.size();

    if (before != nullptr)
        position = indexOf (before);
    else if (bus != nullptr)
        position = routing::getBlockEnd (tracks, indexOf (bus));

    track.bus = bus;
    tracks.insert (tracks.begin() + position, std::make_move_iterator (block.begin()), std::make_move_iterator (block.end()));
    updateRouting();
}

Track* Engine::createBus (const std::vector<Track*>& selected, const juce::String& name)
{
    const auto outer = routing::getOutermost (selected, tracks);

    if (outer.empty())
        return nullptr;

    auto* common = routing::findCommonBus (outer);
    auto* anchor = outer.front();

    while (anchor->bus != common)
        anchor = anchor->bus;

    auto bus = std::make_unique<Track> (Track::Type::bus, name.isNotEmpty() ? name : nextBusName(), nextTrackColour());

    if (sampleRate > 0.0)
        bus->prepare (sampleRate, maxBlockSize);

    auto* raw = bus.get();
    {
        const juce::ScopedLock sl (lock);
        insertTrack (std::move (bus), common, anchor);

        for (auto* t : outer)
            relocate (*t, raw, nullptr);
    }

    sendChangeMessage();
    return raw;
}

Track* Engine::createBusBeside (const std::vector<Track*>& selected, Track& beside, const juce::String& name)
{
    const int at = indexOf (&beside);

    if (at < 0 || ! beside.isBus())
        return createBus (selected, name);

    const auto outer = routing::getOutermost (selected, tracks);

    if (outer.empty())
        return nullptr;

    // A bus can't be made inside something it would hold itself.
    for (auto* t : outer)
        for (auto* up = &beside; up != nullptr; up = up->bus)
            if (up == t)
                return createBus (selected, name);

    // The new bus sits where `beside` sits, right after it; the tracks leave their buses for it.
    auto* parent = beside.bus;
    const int next = routing::getBlockEnd (tracks, at);
    const Track* before = next < (int) tracks.size() && tracks[(size_t) next]->bus == parent ? tracks[(size_t) next].get() : nullptr;

    auto bus = std::make_unique<Track> (Track::Type::bus, name.isNotEmpty() ? name : nextBusName(), nextTrackColour());

    if (sampleRate > 0.0)
        bus->prepare (sampleRate, maxBlockSize);

    auto* raw = bus.get();
    {
        const juce::ScopedLock sl (lock);
        insertTrack (std::move (bus), parent, before);

        for (auto* t : outer)
            relocate (*t, raw, nullptr);
    }

    sendChangeMessage();
    return raw;
}

bool Engine::moveTrack (Track& track, Track* bus, const Track* before)
{
    if (indexOf (&track) < 0 || ! routing::canMoveInto (track, bus) || (bus != nullptr && indexOf (bus) < 0))
        return false;

    if (before == &track)
        return true;

    if (before != nullptr && (indexOf (before) < 0 || before->bus != bus))
        return false;

    {
        const juce::ScopedLock sl (lock);
        relocate (track, bus, before);
    }

    sendChangeMessage();
    return true;
}

bool Engine::removeFromBus (Track& track)
{
    auto* from = track.bus;

    if (from == nullptr || indexOf (&track) < 0)
        return false;

    auto* target = from->bus;
    const int next = routing::getBlockEnd (tracks, indexOf (from));
    const Track* before = next < (int) tracks.size() && tracks[(size_t) next]->bus == target ? tracks[(size_t) next].get() : nullptr;
    return moveTrack (track, target, before);
}

void Engine::ungroup (Track& bus)
{
    std::unique_ptr<Track> removed;
    {
        const juce::ScopedLock sl (lock);

        for (auto& t : tracks)
            if (t->bus == &bus)
                t->bus = bus.bus;

        if (liveMidiTrack.load() == &bus)
            liveMidiTrack = nullptr;

        for (auto it = tracks.begin(); it != tracks.end(); ++it)
        {
            if (it->get() == &bus)
            {
                removed = std::move (*it);
                tracks.erase (it);
                break;
            }
        }

        updateRouting();
    }

    removed.reset(); // plugins are destroyed outside the lock
    sendChangeMessage();
}

void Engine::setFolded (Track& bus, bool shouldBeFolded)
{
    if (! bus.isBus() || bus.folded == shouldBeFolded)
        return;

    {
        const juce::ScopedLock sl (lock);
        bus.folded = shouldBeFolded;
    }

    sendChangeMessage();
}

void Engine::resetEdit()
{
    stopRecording();
    playing = false;

    std::vector<std::unique_ptr<Track>> removed;
    {
        const juce::ScopedLock sl (lock);
        liveMidiTrack = nullptr;
        removed = std::move (tracks);
        tracks.clear();
        renderOrder.clear();
    }

    removed.clear(); // plugins are destroyed outside the lock

    {
        const juce::ScopedLock sl (lock);
        chords.clear();
    }

    sections.clear();
    songLyrics = {};
    lyricsBrief = {};
    key = {};
    keyPitchClasses = getPitchClassBits (key);
    setTempoMap (TempoMap());
    loopEnabled = false;
    loopStartBeat = 0.0;
    loopEndBeat = 16.0;
    masterVolumeDb = 0.0f;
    colourIndex = 0;
    ++editGeneration;
    setPositionBeats (0.0);
    updateInputs();
    notesChanged();
}

void Engine::removeTrack (Track* track)
{
    std::vector<std::unique_ptr<Track>> removed;
    {
        const juce::ScopedLock sl (lock);
        const int first = indexOf (track);

        if (first >= 0)
        {
            const int last = routing::getBlockEnd (tracks, first);

            for (int i = first; i < last; ++i)
            {
                auto* gone = tracks[(size_t) i].get();

                if (liveMidiTrack.load() == gone)
                    liveMidiTrack = nullptr;

                takes.erase (std::remove_if (takes.begin(), takes.end(), [gone] (const Take& t) { return t.track == gone; }), takes.end());
                audioRecordings.erase (std::remove_if (audioRecordings.begin(), audioRecordings.end(),
                                                       [gone] (auto& r) { return r->track == gone; }), audioRecordings.end());

                if (gone->isReturn())
                    for (auto& t : tracks)
                        t->sends.erase (std::remove_if (t->sends.begin(), t->sends.end(), [gone] (auto& s) { return s->target == gone; }), t->sends.end());

                removed.push_back (std::move (tracks[(size_t) i]));
            }

            tracks.erase (tracks.begin() + first, tracks.begin() + last);
            updateRouting();
        }
    }

    removed.clear(); // plugins are destroyed outside the lock
    updateInputs();
    sendChangeMessage();
}

int Engine::indexOf (const Track* track) const
{
    for (size_t i = 0; i < tracks.size(); ++i)
        if (tracks[i].get() == track)
            return (int) i;

    return -1;
}

Track* Engine::findTrackFor (const Clip* clip) const
{
    for (auto& t : tracks)
        for (auto& c : t->clips)
            if (c.get() == clip)
                return t.get();

    return nullptr;
}

AudioClip* Engine::addAudioClip (Track& track, const juce::File& file, double startBeat, juce::String& error)
{
    std::unique_ptr<juce::AudioFormatReader> reader (audioFormats.createReaderFor (file));

    if (reader == nullptr)
    {
        error = "Couldn't read \"" + file.getFileName() + "\" as an audio file.";
        return nullptr;
    }

    const auto maxLength = (juce::int64) (reader->sampleRate * 60.0 * 30.0);

    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > maxLength)
    {
        error = "\"" + file.getFileName() + "\" is empty or longer than 30 minutes.";
        return nullptr;
    }

    auto clip = std::make_unique<AudioClip>();
    clip->file = file;
    clip->name = file.getFileNameWithoutExtension();
    clip->startBeat = juce::jmax (0.0, startBeat);
    clip->sourceSampleRate = reader->sampleRate;

    const int numChannels = juce::jlimit (1, 2, (int) reader->numChannels);
    const int length = (int) reader->lengthInSamples;
    clip->audio.setSize (numChannels, length);
    reader->read (&clip->audio, 0, length, 0, true, numChannels > 1);
    clip->buildPeaks();

    auto* raw = clip.get();
    {
        const juce::ScopedLock sl (lock);
        track.clips.push_back (std::move (clip));
    }

    sendChangeMessage();
    return raw;
}

AudioClip* Engine::addAudioClip (Track& track, juce::AudioBuffer<float> audio, double rate, double startBeat, const juce::String& name)
{
    auto clip = std::make_unique<AudioClip>();
    clip->name = name;
    clip->startBeat = juce::jmax (0.0, startBeat);
    clip->sourceSampleRate = rate;
    clip->audio = std::move (audio);
    clip->buildPeaks();

    auto* raw = clip.get();
    {
        const juce::ScopedLock sl (lock);
        track.clips.push_back (std::move (clip));
    }

    sendChangeMessage();
    return raw;
}

MidiClip* Engine::addMidiClip (Track& track, double startBeat, double lengthBeats)
{
    auto clip = std::make_unique<MidiClip>();
    clip->name = track.name + " clip";
    clip->startBeat = juce::jmax (0.0, startBeat);
    clip->lengthBeats = juce::jmax (1.0, lengthBeats);

    auto* raw = clip.get();
    {
        const juce::ScopedLock sl (lock);
        track.clips.push_back (std::move (clip));
    }

    sendChangeMessage();
    return raw;
}

Clip* Engine::duplicateClip (Track& track, const Clip& source)
{
    std::unique_ptr<Clip> copy;

    if (source.kind == Clip::Kind::midi)
        copy = std::make_unique<MidiClip> (static_cast<const MidiClip&> (source));
    else
        copy = std::make_unique<AudioClip> (static_cast<const AudioClip&> (source));

    copy->startBeat = source.getEndBeat (tempoMap);

    auto* raw = copy.get();
    {
        const juce::ScopedLock sl (lock);
        track.clips.push_back (std::move (copy));
    }

    sendChangeMessage();
    return raw;
}

void Engine::removeClip (Track& track, const Clip* clip)
{
    std::unique_ptr<Clip> removed;
    {
        const juce::ScopedLock sl (lock);

        for (auto it = track.clips.begin(); it != track.clips.end(); ++it)
        {
            if (it->get() == clip)
            {
                removed = std::move (*it);
                track.clips.erase (it);
                ++midiEditVersion;
                break;
            }
        }

        takes.erase (std::remove_if (takes.begin(), takes.end(), [clip] (const Take& t) { return t.clip == clip; }), takes.end());
    }

    sendChangeMessage();
}

void Engine::moveClip (Track& from, const Clip* clip, Track& to)
{
    if (&from == &to || from.type != to.type)
        return;

    {
        const juce::ScopedLock sl (lock);

        for (auto it = from.clips.begin(); it != from.clips.end(); ++it)
        {
            if (it->get() == clip)
            {
                to.clips.push_back (std::move (*it));
                from.clips.erase (it);
                ++midiEditVersion;
                break;
            }
        }
    }

    sendChangeMessage();
}

bool Engine::canSplit (const Clip& clip, double beat) const
{
    // An audio clip must keep at least a sample on each side.
    const double minBeats = clip.kind == Clip::Kind::audio ? tempoMap.getBpmAt (beat) / 60.0 / static_cast<const AudioClip&> (clip).sourceSampleRate : 0.0;
    return beat > clip.startBeat + minBeats && beat < clip.getEndBeat (tempoMap) - minBeats;
}

Clip* Engine::splitClip (Track& track, Clip& clip, double beat)
{
    if (! canSplit (clip, beat))
        return nullptr;

    const double cut = beat - clip.startBeat;
    std::unique_ptr<Clip> right;

    if (clip.kind == Clip::Kind::midi)
    {
        auto& left = static_cast<MidiClip&> (clip);
        auto part = std::make_unique<MidiClip>();
        part->lengthBeats = left.lengthBeats - cut;
        std::vector<MidiNote> kept;

        for (auto note : left.notes)
        {
            if (note.start < cut)
            {
                note.length = juce::jmin (note.length, cut - note.start);
                kept.push_back (note);
            }
            else
            {
                note.start -= cut;
                part->notes.push_back (note);
            }
        }

        const juce::ScopedLock sl (lock);
        left.notes = std::move (kept);
        left.lengthBeats = cut;
        right = std::move (part);
    }
    else
    {
        auto& left = static_cast<AudioClip&> (clip);
        const int numChannels = left.audio.getNumChannels();
        const int total = left.audio.getNumSamples();
        const int split = juce::jlimit (1, total - 1, juce::roundToInt (tempoMap.getSecondsBetween (clip.startBeat, beat) * left.sourceSampleRate));

        auto part = std::make_unique<AudioClip>();
        part->sourceSampleRate = left.sourceSampleRate;
        part->gainDb = left.gainDb;
        part->audio.setSize (numChannels, total - split);
        juce::AudioBuffer<float> kept (numChannels, split);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            part->audio.copyFrom (ch, 0, left.audio, ch, split, total - split);
            kept.copyFrom (ch, 0, left.audio, ch, 0, split);
        }

        part->buildPeaks();
        {
            const juce::ScopedLock sl (lock);
            left.audio = std::move (kept);
        }

        // The audio no longer matches the files; saving writes each part to a new file.
        left.file = part->file = juce::File();
        left.buildPeaks();
        right = std::move (part);
    }

    right->name = clip.name;
    right->startBeat = beat;
    right->muted = clip.muted;
    auto* raw = right.get();
    {
        const juce::ScopedLock sl (lock);
        track.clips.push_back (std::move (right));
    }

    notesChanged();
    return raw;
}

Clip* Engine::findNeighbourClip (const Track& track, const Clip& clip, bool next)
{
    std::vector<Clip*> ordered;

    for (auto& c : track.clips)
        if (c->kind == clip.kind)
            ordered.push_back (c.get());

    std::stable_sort (ordered.begin(), ordered.end(), [] (const Clip* a, const Clip* b) { return a->startBeat < b->startBeat; });

    const auto it = std::find (ordered.begin(), ordered.end(), &clip);

    if (it == ordered.end())
        return nullptr;

    if (next)
        return it + 1 != ordered.end() ? *(it + 1) : nullptr;

    return it != ordered.begin() ? *(it - 1) : nullptr;
}

Clip* Engine::joinClips (Track& track, Clip& a, Clip& b)
{
    if (&a == &b || a.kind != b.kind)
        return nullptr;

    auto& first = b.startBeat < a.startBeat ? b : a;
    auto& second = &first == &a ? b : a;
    const double offset = second.startBeat - first.startBeat;

    if (first.kind == Clip::Kind::midi)
    {
        auto& firstMidi = static_cast<MidiClip&> (first);
        auto& secondMidi = static_cast<MidiClip&> (second);

        // Playback ignores notes past a clip's end and cuts notes off at it; do the same here.
        auto audible = [] (const MidiClip& clip, double shift, std::vector<MidiNote>& out)
        {
            for (auto note : clip.notes)
            {
                if (note.start >= clip.lengthBeats)
                    continue;

                note.length = juce::jmin (note.length, clip.lengthBeats - note.start);
                note.start += shift;
                out.push_back (note);
            }
        };

        std::vector<MidiNote> notes;
        audible (firstMidi, 0.0, notes);
        audible (secondMidi, offset, notes);
        std::stable_sort (notes.begin(), notes.end(), [] (auto& x, auto& y) { return x.start < y.start; });

        const juce::ScopedLock sl (lock);
        firstMidi.notes = std::move (notes);
        firstMidi.lengthBeats = juce::jmax (firstMidi.lengthBeats, offset + secondMidi.lengthBeats);
    }
    else
    {
        auto& firstAudio = static_cast<AudioClip&> (first);
        auto& secondAudio = static_cast<AudioClip&> (second);

        // The second clip goes in at the first one's sample rate, after silence for any gap and
        // mixed in where the two overlap, so the result sounds as the two did.
        const double rate = firstAudio.sourceSampleRate;
        const double ratio = secondAudio.sourceSampleRate / rate;
        const int start = juce::roundToInt (tempoMap.getSecondsBetween (first.startBeat, second.startBeat) * rate);
        const int secondLength = juce::roundToInt (secondAudio.audio.getNumSamples() / ratio);
        const int numChannels = juce::jmax (firstAudio.audio.getNumChannels(), secondAudio.audio.getNumChannels());
        const float relativeGain = secondAudio.getGain() / firstAudio.getGain();   // the joined clip keeps the first one's gain

        juce::AudioBuffer<float> joined (numChannels, juce::jmax (firstAudio.audio.getNumSamples(), start + secondLength));
        joined.clear();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            joined.copyFrom (ch, 0, firstAudio.audio, juce::jmin (ch, firstAudio.audio.getNumChannels() - 1), 0, firstAudio.audio.getNumSamples());
            const float* src = secondAudio.audio.getReadPointer (juce::jmin (ch, secondAudio.audio.getNumChannels() - 1));

            if (juce::exactlyEqual (ratio, 1.0))
            {
                joined.addFrom (ch, start, src, secondLength, relativeGain);
            }
            else
            {
                juce::LagrangeInterpolator resampler;
                juce::HeapBlock<float> resampled ((size_t) secondLength);
                resampler.process (ratio, src, resampled.get(), secondLength, secondAudio.audio.getNumSamples(), 0);
                joined.addFrom (ch, start, resampled.get(), secondLength, relativeGain);
            }
        }

        {
            const juce::ScopedLock sl (lock);
            firstAudio.audio = std::move (joined);
        }

        firstAudio.file = juce::File(); // saving writes the joined audio to a new file
        firstAudio.buildPeaks();
    }

    removeClip (track, &second);
    notesChanged();
    return &first;
}

void Engine::reverseClip (AudioClip& clip)
{
    {
        const juce::ScopedLock sl (lock);
        clip.audio.reverse (0, clip.audio.getNumSamples());
    }

    clip.file = juce::File(); // saving writes the reversed audio to a new file
    clip.buildPeaks();
    sendChangeMessage();
}

void Engine::setClipGain (AudioClip& clip, float gainDb)
{
    gainDb = juce::jlimit (AudioClip::minGainDb, AudioClip::maxGainDb, gainDb);

    if (juce::exactlyEqual (gainDb, clip.gainDb))
        return;

    {
        const juce::ScopedLock sl (lock);
        clip.gainDb = gainDb;
    }

    sendChangeMessage();
}

void Engine::setClipMuted (Clip& clip, bool muted)
{
    {
        const juce::ScopedLock sl (lock);
        clip.muted = muted;
    }

    notesChanged();   // releases notes the clip was holding
}

void Engine::notesChanged()
{
    ++midiEditVersion;
    sendChangeMessage();
}

bool Engine::canTranspose (const MidiClip& clip, int semitones)
{
    return ! clip.notes.empty()
        && std::all_of (clip.notes.begin(), clip.notes.end(), [semitones] (auto& n)
                        { return juce::isPositiveAndBelow (n.pitch + semitones, 128); });
}

bool Engine::transposeClip (MidiClip& clip, int semitones)
{
    if (! canTranspose (clip, semitones))
        return false;

    {
        const juce::ScopedLock sl (lock);
        for (auto& n : clip.notes)
            n.pitch += semitones;
    }

    notesChanged();
    return true;
}

//==============================================================================
PluginSlot* Engine::addPlugin (Track& track, const juce::PluginDescription& description, juce::String& error, int insertIndex)
{
    if (track.hasInput() && description.isInstrument)
    {
        error = track.isReturn() ? "A return has no instrument: it plays what tracks send to it."
                                 : "A bus has no instrument: it plays what its tracks send into it.";
        return nullptr;
    }

    const double rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    auto instance = pluginFormats.createPluginInstance (description, rate, maxBlockSize, error);

    if (instance == nullptr)
    {
        if (error.isEmpty())
            error = "Couldn't create " + description.name;

        return nullptr;
    }

    // Prefer a plain stereo main bus, with side-chains and extra outputs switched off: the track
    // only uses the main stereo signal. Fall back to whatever part of that the plugin accepts.
    auto layout = instance->getBusesLayout();

    if (! layout.outputBuses.isEmpty())
        layout.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();

    if (! layout.inputBuses.isEmpty())
        layout.inputBuses.getReference (0) = juce::AudioChannelSet::stereo();

    auto mainOnly = layout;

    for (auto* buses : { &mainOnly.inputBuses, &mainOnly.outputBuses })
        for (int i = 1; i < buses->size(); ++i)
            buses->getReference (i) = juce::AudioChannelSet::disabled();

    if (instance->checkBusesLayoutSupported (mainOnly))
        instance->setBusesLayout (mainOnly);
    else if (instance->checkBusesLayoutSupported (layout))
        instance->setBusesLayout (layout);

    instance->setPlayHead (this);
    instance->setRateAndBufferSizeDetails (rate, maxBlockSize);
    instance->prepareToPlay (rate, maxBlockSize);

    auto slot = std::make_unique<PluginSlot>();
    slot->isInstrument = description.isInstrument;
    slot->instance = std::move (instance);
    slot->songContext = dynamic_cast<SongContextReceiver*> (slot->instance.get());
    slot->keepsLatencyWhenBypassed = dynamic_cast<KeepsLatencyWhenBypassed*> (slot->instance.get()) != nullptr;

    auto* raw = slot.get();
    {
        const juce::ScopedLock sl (lock);
        const auto index = juce::isPositiveAndBelow (insertIndex, (int) track.plugins.size()) ? (size_t) insertIndex
                                                                                               : track.plugins.size();
        track.plugins.insert (track.plugins.begin() + (std::ptrdiff_t) index, std::move (slot));
        track.fitPluginChannels();
        ++midiEditVersion; // the built-in synth may stop being used: release its notes
    }

    sendChangeMessage();
    return raw;
}

void Engine::removePlugin (Track& track, const PluginSlot* slot)
{
    std::unique_ptr<PluginSlot> removed;
    {
        const juce::ScopedLock sl (lock);

        for (auto it = track.plugins.begin(); it != track.plugins.end(); ++it)
        {
            if (it->get() == slot)
            {
                removed = std::move (*it);
                track.plugins.erase (it);
                break;
            }
        }
    }

    if (removed != nullptr)
        removed->instance->releaseResources();

    sendChangeMessage();
}

void Engine::movePlugin (Track& track, const PluginSlot* slot, int newIndex)
{
    {
        const juce::ScopedLock sl (lock);
        auto& plugins = track.plugins;
        const auto it = std::find_if (plugins.begin(), plugins.end(), [slot] (auto& p) { return p.get() == slot; });

        if (it == plugins.end())
            return;

        const auto from = (int) std::distance (plugins.begin(), it);
        const auto to = juce::jlimit (0, (int) plugins.size() - 1, newIndex);

        if (from == to)
            return;

        auto moved = std::move (*it);
        plugins.erase (it);
        plugins.insert (plugins.begin() + to, std::move (moved));
        ++midiEditVersion;
    }

    sendChangeMessage();
}

void Engine::setPluginBypassed (PluginSlot& slot, bool shouldBeBypassed)
{
    slot.bypassed = shouldBeBypassed;
    sendChangeMessage();
}

void Engine::previewNote (int pitch, float velocity, int durationMs)
{
    if (! deviceRunning.load() || pitch < 0 || pitch > 127)
        return;

    // Previews have their own queue, so they reach the live track only and are never recorded.
    auto send = [this] (juce::MidiMessage m)
    {
        m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
        previewCollector.addMessageToQueue (m);
    };

    send (juce::MidiMessage::noteOn (1, pitch, velocity));

    juce::Timer::callAfterDelay (durationMs, [weakThis = juce::WeakReference<Engine> (this), pitch, send]
    {
        if (weakThis != nullptr && weakThis->deviceRunning.load())
            send (juce::MidiMessage::noteOff (1, pitch));
    });
}

void Engine::setArmed (Track& track, bool shouldBeArmed)
{
    if (track.armed.load() == shouldBeArmed)
        return;

    if (track.isAudio())
    {
        if (recording.load())
            return;

        track.armed = shouldBeArmed;

        if (! shouldBeArmed)
            track.inputMeterLevel = 0.0f;

        updateInputs();
        sendChangeMessage();
        return;
    }

    if (! track.isMidi())
        return;

    track.armed = shouldBeArmed;
    notesChanged(); // input moves to other tracks: release notes held on the old ones
}

void Engine::setInput (Track& track, int firstChannel, bool stereo)
{
    firstChannel = juce::jlimit (0, 63, firstChannel);

    if (! track.isAudio() || recording.load() || (track.inputChannel.load() == firstChannel && track.inputStereo.load() == stereo))
        return;

    track.inputChannel = firstChannel;
    track.inputStereo = stereo;
    sendChangeMessage();
}

void Engine::setMonitor (Track& track, bool on)
{
    if (! track.isAudio() || track.monitor.load() == on)
        return;

    track.monitor = on;
    sendChangeMessage();
}

//==============================================================================
bool Engine::areInputsWanted() const
{
    if (inputMeterWanted || tunerTap.isListening())
        return true;

    return std::any_of (tracks.begin(), tracks.end(), [] (auto& t) { return t->isAudio() && t->armed.load(); });
}

void Engine::setInputMeterWanted (bool wanted)
{
    if (inputMeterWanted != wanted)
    {
        inputMeterWanted = wanted;
        updateInputs();
    }
}

void Engine::setTunerListening (bool on)
{
    if (tunerTap.isListening() != on)
    {
        tunerTap.setListening (on);
        updateInputs();
    }
}

void Engine::updateInputs()
{
    const bool wanted = areInputsWanted();

    // Only the device the engine plays through: a test with a fake device, or a bounce, never opens a microphone.
    auto* device = deviceManager.getCurrentAudioDevice();

    if (wanted == inputsRequested || device == nullptr || device != activeDevice.load())
        return;

    inputsRequested = wanted;
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.useDefaultInputChannels = false;
    setup.inputChannels.clear();

    if (! wanted)
        setup.inputDeviceName = {};   // output only: no second device to join to the output

    if (wanted)
    {
        setup.inputChannels.setRange (0, 8, true);   // the device keeps the channels it has

        if (setup.inputDeviceName.isEmpty())
            if (auto* type = deviceManager.getCurrentDeviceTypeObject())
            {
                const auto names = type->getDeviceNames (true);
                const int index = type->getDefaultDeviceIndex (true);

                if (names.size() > 0)
                    setup.inputDeviceName = names[juce::jlimit (0, names.size() - 1, index)];
            }
    }

    deviceManager.setAudioDeviceSetup (setup, true);
}

void Engine::setRecordingFolder (const juce::File& folder)
{
    recordingFolder = folder;
}

juce::File Engine::getRecordingFolder() const
{
    if (recordingFolder != juce::File())
        return recordingFolder;

    return defaultRecordingFolder;
}

juce::String Engine::getLatencySettingsKey() const
{
    const auto setup = deviceManager.getAudioDeviceSetup();

    if (setup.inputDeviceName.isEmpty() && setup.outputDeviceName.isEmpty())
        return {};

    return "recording.extraLatencyMs." + setup.inputDeviceName + "|" + setup.outputDeviceName;
}

void Engine::loadMeasuredLatency()
{
    const auto settingsKey = getLatencySettingsKey();
    auto* settings = properties.getUserSettings();

    if (settingsKey.isNotEmpty() && settings != nullptr && settings->containsKey (settingsKey))
    {
        const double rate = sampleRate > 0.0 ? sampleRate : 48000.0;
        measuredLatencyExtra = (int) std::lround (settings->getDoubleValue (settingsKey) * rate / 1000.0);
        latencyMeasured = true;
    }
    else
    {
        measuredLatencyExtra = 0;
        latencyMeasured = false;
    }
}

bool Engine::startLatencyMeasurement()
{
    if (offline.load() || recording.load() || measureState.load() != 0 || numInputChannels.load() <= 0
        || activeDevice.load() == nullptr)
        return false;

    const juce::ScopedLock sl (lock);
    measureRate = sampleRate;

    if (measureRate <= 0.0)
        return false;

    const int total = latency::getTotalSamples (measureRate);
    measureOutput.assign ((size_t) total, 0.0f);
    measureHeard.assign ((size_t) total, 0.0f);
    const auto click = latency::makeClick (measureRate);

    for (int i = 0; i < latency::numClicks; ++i)
    {
        const int start = latency::getClickStart (i, measureRate);

        for (size_t n = 0; n < click.size() && start + (int) n < total; ++n)
            measureOutput[(size_t) start + n] = click[n];
    }

    measurePos = 0;
    lastMeasurement = {};
    measureState = 1;
    return true;
}

double Engine::getLatencyMeasurementProgress() const noexcept
{
    if (measureState.load() == 2)
        return 1.0;

    const int total = (int) measureOutput.size();
    return total > 0 ? juce::jlimit (0.0, 1.0, (double) measurePos.load() / total) : 0.0;
}

void Engine::cancelLatencyMeasurement()
{
    const juce::ScopedLock sl (lock);
    measureState = 0;
}

void Engine::runLatencyMeasurement (const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples)
{
    const int total = (int) measureOutput.size();
    int pos = measurePos.load();

    for (int i = 0; i < numSamples && pos < total; ++i, ++pos)
    {
        float heard = 0.0f;

        // The loudest of the inputs, so it works whichever socket the cable or the microphone is on.
        if (inputs != nullptr)
            for (int ch = 0; ch < numInputs; ++ch)
                if (inputs[ch] != nullptr && std::abs (inputs[ch][i]) > std::abs (heard))
                    heard = inputs[ch][i];

        measureHeard[(size_t) pos] = heard;

        for (int ch = 0; ch < numOutputs; ++ch)
            if (outputs[ch] != nullptr)
                outputs[ch][i] = measureOutput[(size_t) pos];
    }

    measurePos = pos;

    if (pos >= total)
        measureState = 2;
}

bool Engine::serviceLatencyMeasurement()
{
    if (measureState.load() != 2)
        return false;

    const juce::ScopedLock sl (lock);

    if (measureState.load() != 2)
        return false;

    const auto found = latency::findRoundTrip (measureHeard, measureRate);
    measureState = 0;

    LatencyMeasurement m;
    m.ok = found.ok;
    m.clicksFound = found.clicksFound;
    m.roundTripSamples = found.roundTripSamples;
    m.sampleRate = measureRate;

    if (auto* device = activeDevice.load())
        m.reportedSamples = device->getInputLatencyInSamples() + device->getOutputLatencyInSamples();

    if (found.ok)
    {
        m.extraSamples = m.roundTripSamples - m.reportedSamples;
        measuredLatencyExtra = (int) std::lround (m.extraSamples);
        latencyMeasured = true;

        if (const auto settingsKey = getLatencySettingsKey(); settingsKey.isNotEmpty())
        {
            auto* settings = properties.getUserSettings();
            settings->setValue (settingsKey, m.extraSamples * 1000.0 / measureRate);
            settings->saveIfNeeded();
        }
    }
    else
    {
        m.problem = "DawIt couldn't hear the clicks. Connect an output to an input with a cable, or put the microphone "
                    "close to a speaker, turn the input up a little, and try again.";
    }

    lastMeasurement = m;
    return true;
}

int Engine::getLatencyShiftSamples() const
{
    int shift = measuredLatencyExtra.load();

    if (auto* device = activeDevice.load())
        shift += device->getInputLatencyInSamples() + device->getOutputLatencyInSamples();

    return juce::jmax (0, shift);
}

std::vector<Engine::LiveTake> Engine::getLiveTakes() const
{
    std::vector<LiveTake> live;

    for (auto& r : audioRecordings)
    {
        LiveTake take;
        take.track = r->track;
        take.startBeat = r->passStarts.empty() ? recordingStartBeat : r->passStarts.front().beat;
        take.seconds = juce::jmax (0.0, (double) (r->length - r->latencyShift) / r->rate);
        take.peaks = &r->peaks;
        live.push_back (take);
    }

    return live;
}

void Engine::chooseTake (Track& track, AudioClip& chosen)
{
    bool changed = false;
    {
        const juce::ScopedLock sl (lock);
        changed = ! daw::takes::choose (track.clips, chosen, tempoMap).empty();
    }

    if (changed)
        notesChanged();
}

void Engine::deleteOtherTakes (Track& track, AudioClip& chosen)
{
    std::vector<AudioClip*> others;
    {
        const juce::ScopedLock sl (lock);
        others = daw::takes::findOthers (track.clips, chosen, tempoMap);
    }

    for (auto* other : others)
        removeClip (track, other);

    setClipMuted (chosen, false);
}

void Engine::addMidiInput (const juce::MidiMessage& message)
{
    auto m = message;
    m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    midiCollector.addMessageToQueue (m);
}

//==============================================================================
bool Engine::startRecording()
{
    if (recording.load())
        return true;

    std::vector<Track*> targets, audioTargets;

    for (auto& t : tracks)
    {
        if (t->isMidi() && t->armed.load())
            targets.push_back (t.get());
        else if (t->isAudio() && t->armed.load() && sampleRate > 0.0)
            audioTargets.push_back (t.get());
    }

    if (targets.empty() && audioTargets.empty())
    {
        auto* live = liveMidiTrack.load();

        if (live == nullptr || ! live->isMidi())
            return false;

        setArmed (*live, true);
        targets.push_back (live);
    }

    // Drop anything a previous take left in the queue.
    recordQueue.read (recordQueue.getNumReady());

    heldNotes.fill (std::nullopt);
    sustainedNotes.reset();
    pedalDown = false;

    // Each take starts as one bar, at the bar of the playhead, and grows while recording.
    // Past the end of an active loop, playback jumps straight to the loop start, and so does the take.
    double start = getPositionBeats();

    if (loopEnabled.load() && loopEndBeat.load() - loopStartBeat.load() > 0.01 && start >= loopEndBeat.load())
        start = loopStartBeat.load();

    recordedUntil = start;
    recordingStartBeat = start;
    takes.clear();

    for (auto* track : targets)
    {
        auto* clip = addMidiClip (*track, std::floor (start / 4.0) * 4.0, 4.0);
        clip->name = track->name + " recording";
        takes.push_back ({ track, clip });
    }

    // Audio: a FIFO for each armed track, and a raw file that keeps what comes in even if the app crashes.
    std::vector<std::unique_ptr<AudioRecording>> newRecordings;

    if (! audioTargets.empty())
    {
        if (! writerThread.isThreadRunning())
            writerThread.startThread();

        const auto folder = getRecordingFolder();
        const bool haveFolder = folder.createDirectory().wasOk();
        const int shift = getLatencyShiftSamples();

        for (auto* track : audioTargets)
        {
            auto r = std::make_unique<AudioRecording> (*track, track->inputChannel.load(), track->inputStereo.load(), sampleRate, shift);

            if (haveFolder)
            {
                r->rawFile = folder.getNonexistentChildFile (juce::File::createLegalFileName (track->name + " raw"), ".wav", false);
                std::unique_ptr<juce::OutputStream> stream (r->rawFile.createOutputStream());

                if (stream != nullptr)
                {
                    juce::WavAudioFormat wav;
                    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                                   .withSampleRate (sampleRate)
                                                                   .withNumChannels (r->numChannels)
                                                                   .withBitsPerSample (32)
                                                                   .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));

                    if (writer != nullptr)
                        r->raw = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (writer.release(), writerThread, 1 << 16);
                }
            }

            newRecordings.push_back (std::move (r));
        }
    }

    {
        const juce::ScopedLock sl (lock);
        audioRecordings = std::move (newRecordings);
        captureStarted = false;
    }

    deviceChangedWhileRecording = false;
    recordingRate = sampleRate;

    if (countInEnabled.load())
        countInRequest = 4.0; // one 4/4 bar

    recording = true;
    playing = true;
    startTimerHz (30);
    sendChangeMessage();
    return true;
}

void Engine::stopRecording()
{
    if (! recording.load())
        return;

    recording = false;
    stopTimer();

    {
        const juce::ScopedLock sl (lock);   // a block still capturing finishes first
    }

    collectRecordedMidi();

    // Keys still held end where recording stopped.
    const double end = juce::jmax (recordedUntil, getPositionBeats());

    for (int pitch = 0; pitch < 128; ++pitch)
        if (heldNotes[(size_t) pitch].has_value())
            finishNote (pitch, end);

    for (auto& take : std::exchange (takes, {}))
        if (take.clip->notes.empty())
            removeClip (*take.track, take.clip);

    finishAudioRecordings();
    sendChangeMessage();
}

void Engine::timerCallback()
{
    if (! playing.load() || deviceChangedWhileRecording.load())
    {
        stopRecording();
    }
    else
    {
        serviceRecording();
    }
}

void Engine::serviceRecording()
{
    collectRecordedMidi();

    for (auto& r : audioRecordings)
        drainAudioRecording (*r);
}

void Engine::captureInput (double startBeat, double samplesPerBeat, int liveMidiOffset, int numSamples)
{
    for (const auto metadata : liveMidi)
    {
        if (metadata.samplePosition < liveMidiOffset || metadata.samplePosition >= liveMidiOffset + numSamples)
            continue;

        const auto m = metadata.getMessage();

        if (! m.isNoteOnOrOff() && ! m.isSustainPedalOn() && ! m.isSustainPedalOff())
            continue;

        RecordedEvent event;
        event.beat = startBeat + (metadata.samplePosition - liveMidiOffset) / samplesPerBeat;
        std::copy_n (m.getRawData(), juce::jmin (3, m.getRawDataSize()), event.bytes);

        // A full queue (the message thread stalled for a long time) drops the event.
        const auto scope = recordQueue.write (1);

        if (scope.blockSize1 > 0)
            recordBuffer[(size_t) scope.startIndex1] = event;
    }
}

void Engine::captureAudio (double startBeat, double samplesPerBeat, int offset, int numSamples)
{
    // A block that doesn't continue the last one (the loop wrapped, the playhead was moved) starts a new pass.
    const bool newPass = ! captureStarted || std::abs (startBeat - captureExpectedBeat) > 1.0e-6;
    captureStarted = true;
    captureExpectedBeat = startBeat + numSamples / samplesPerBeat;

    for (auto& rp : audioRecordings)
    {
        auto& r = *rp;
        auto push = [&r] (const AudioRecording::Event& event)
        {
            const auto scope = r.events.write (1);

            if (scope.blockSize1 > 0)
                r.eventData[(size_t) scope.startIndex1] = event;
        };

        const int free = r.fifo.getFreeSpace();
        const int written = juce::jmin (numSamples, free);

        // A stretch of dropped samples ends with the first block that fits again: it becomes one event, before the data after it.
        if (written > 0 && r.gapLength > 0)
        {
            AudioRecording::Event gap;
            gap.isGap = true;
            gap.position = r.gapStart;
            gap.lost = r.gapLength;
            push (gap);
            r.gapLength = 0;
        }

        if (newPass)
        {
            AudioRecording::Event pass;
            pass.position = r.offered;
            pass.beat = startBeat;
            push (pass);
        }

        if (written > 0)
        {
            const auto scope = r.fifo.write (written);
            const auto channels = resolveInputChannels (r.firstChannel, r.stereo, blockNumInputs);

            for (int ch = 0; ch < r.numChannels; ++ch)
            {
                const int source = channels.first < 0 ? -1 : (ch == 0 ? channels.first : channels.second);
                const float* data = source >= 0 && blockInputs != nullptr && blockInputs[source] != nullptr ? blockInputs[source] + offset : nullptr;

                for (int part = 0; part < 2; ++part)
                {
                    const int start = part == 0 ? scope.startIndex1 : scope.startIndex2;
                    const int size = part == 0 ? scope.blockSize1 : scope.blockSize2;
                    const int from = part == 0 ? 0 : scope.blockSize1;

                    if (size <= 0)
                        continue;

                    if (data != nullptr)
                        juce::FloatVectorOperations::copy (r.fifoData.getWritePointer (ch, start), data + from, size);
                    else
                        juce::FloatVectorOperations::clear (r.fifoData.getWritePointer (ch, start), size);
                }
            }
        }

        if (written < numSamples)
        {
            if (r.gapLength == 0)
                r.gapStart = r.offered + written;

            r.gapLength += numSamples - written;
            r.dropped += numSamples - written;
        }

        r.offered += numSamples;
        r.offeredTotal = r.offered;
    }
}

void Engine::meterInputs (int numSamples)
{
    bool anyArmed = false, signal = false;

    // The loudest input channel, for the settings panel's meter (nothing is computed when the device has no inputs).
    {
        float loudest = 0.0f;

        for (int c = 0; c < blockNumInputs && blockInputs != nullptr; ++c)
            if (blockInputs[c] != nullptr)
            {
                const auto range = juce::FloatVectorOperations::findMinAndMax (blockInputs[c], numSamples);
                loudest = juce::jmax (loudest, std::abs (range.getStart()), std::abs (range.getEnd()));
            }

        updateMeterLevel (deviceInputLevel, loudest, numSamples, sampleRate);
    }

    for (auto& t : tracks)
    {
        if (! t->isAudio())
            continue;

        if (! t->armed.load())
        {
            if (t->inputMeterLevel.load() > 0.0f)
                t->inputMeterLevel = 0.0f;

            continue;
        }

        anyArmed = true;
        const auto [left, right] = resolveInputChannels (t->inputChannel.load(), t->inputStereo.load(), blockNumInputs);
        float peak = 0.0f;

        for (const int channel : { left, right })
        {
            if (channel < 0 || blockInputs == nullptr || blockInputs[channel] == nullptr)
                continue;

            const auto range = juce::FloatVectorOperations::findMinAndMax (blockInputs[channel], numSamples);
            peak = juce::jmax (peak, std::abs (range.getStart()), std::abs (range.getEnd()));
        }

        signal = signal || peak > 0.0f;
        updateMeterLevel (t->inputMeterLevel, peak, numSamples, sampleRate);
    }

    if (anyArmed && blockNumInputs > 0 && ! signal)
    {
        zeroInputRun += numSamples;

        if ((double) zeroInputRun >= sampleRate)
            inputAllZero = true;
    }
    else
    {
        zeroInputRun = 0;
        inputAllZero = false;
    }
}

void Engine::drainAudioRecording (AudioRecording& r)
{
    // Read how much data there is before the events: an event is always written before the data that follows it.
    int ready = r.fifo.getNumReady();

    {
        const auto scope = r.events.read (r.events.getNumReady());

        for (int part = 0; part < 2; ++part)
        {
            const int start = part == 0 ? scope.startIndex1 : scope.startIndex2;
            const int size = part == 0 ? scope.blockSize1 : scope.blockSize2;

            for (int i = 0; i < size; ++i)
            {
                const auto& event = r.eventData[(size_t) (start + i)];
                (event.isGap ? r.gaps : r.passStarts).push_back (event);
            }
        }
    }

    auto grow = [&r] (juce::int64 needed)
    {
        if (needed > r.stream.getNumSamples())
            r.stream.setSize (r.numChannels, (int) juce::jmin ((juce::int64) 1 << 30, juce::jmax (needed, (juce::int64) r.stream.getNumSamples() * 2)), true, false, true);
    };

    auto added = [&r] (juce::int64 from, int count)
    {
        // The peaks the lane draws, and the raw file's share.
        for (int i = 0; i < count; ++i)
        {
            const auto index = (size_t) ((from + i) / Engine::LiveTake::peakBlockSize);

            if (r.peaks.size() <= index)
                r.peaks.resize (index + 1, 0.0f);

            for (int ch = 0; ch < r.numChannels; ++ch)
                r.peaks[index] = juce::jmax (r.peaks[index], std::abs (r.stream.getSample (ch, (int) (from + i))));
        }

        if (r.raw != nullptr && count > 0)
        {
            const float* pointers[2] = { r.stream.getReadPointer (0, (int) from), r.stream.getReadPointer (r.numChannels - 1, (int) from) };
            r.raw->write (pointers, count);
        }
    };

    for (;;)
    {
        const juce::int64 nextGapPosition = r.nextGap < r.gaps.size() ? r.gaps[r.nextGap].position : std::numeric_limits<juce::int64>::max();

        if (r.length == nextGapPosition)
        {
            // Dropped samples: a stretch of silence, so what follows stays where it was played.
            const int count = (int) juce::jmin ((juce::int64) 1 << 28, r.gaps[r.nextGap].lost);
            grow (r.length + count);

            for (int ch = 0; ch < r.numChannels; ++ch)
                r.stream.clear (ch, (int) r.length, count);

            added (r.length, count);
            r.length += count;
            ++r.nextGap;
            continue;
        }

        if (ready <= 0)
            break;

        const int count = (int) juce::jmin ((juce::int64) ready, nextGapPosition - r.length);
        grow (r.length + count);
        const auto scope = r.fifo.read (count);

        for (int ch = 0; ch < r.numChannels; ++ch)
        {
            if (scope.blockSize1 > 0)
                juce::FloatVectorOperations::copy (r.stream.getWritePointer (ch, (int) r.length), r.fifoData.getReadPointer (ch, scope.startIndex1), scope.blockSize1);

            if (scope.blockSize2 > 0)
                juce::FloatVectorOperations::copy (r.stream.getWritePointer (ch, (int) r.length + scope.blockSize1), r.fifoData.getReadPointer (ch, scope.startIndex2), scope.blockSize2);
        }

        added (r.length, count);
        r.length += count;
        ready -= count;
    }
}

void Engine::finishAudioRecordings()
{
    std::vector<std::unique_ptr<AudioRecording>> finished;
    {
        const juce::ScopedLock sl (lock);
        finished = std::move (audioRecordings);
        audioRecordings.clear();
    }

    if (finished.empty())
        return;

    lastReport.clear();

    for (auto& finishedRecording : finished)
    {
        auto& r = *finishedRecording;
        auto* track = r.track;

        if (indexOf (track) < 0)
            continue;

        drainAudioRecording (r);

        // Samples dropped at the very end never got an event of their own: they are silence too.
        if (r.offeredTotal.load() > r.length)
        {
            AudioRecording::Event gap;
            gap.isGap = true;
            gap.position = r.length;
            gap.lost = r.offeredTotal.load() - r.length;
            r.gaps.push_back (gap);
            drainAudioRecording (r);
        }

        TakeReport report;
        report.trackName = track->name;
        report.inputChannel = r.firstChannel;
        report.stereo = r.stereo;
        report.lostSeconds = (double) r.dropped.load() / r.rate;

        // Each pass of the loop is a take: from where it started, moved back by the latency, to where the next began.
        struct Pass
        {
            double startBeat;
            juce::int64 begin, end;
            bool complete;   // it ran to the loop end
        };

        std::vector<Pass> passes;

        for (size_t i = 0; i < r.passStarts.size(); ++i)
        {
            const bool last = i + 1 == r.passStarts.size();
            const auto begin = r.passStarts[i].position + r.latencyShift;
            const auto end = last ? r.length : juce::jmin (r.length, r.passStarts[i + 1].position + r.latencyShift);

            if (begin < end)
                passes.push_back ({ r.passStarts[i].beat, begin, end, ! last });
        }

        const float silentGain = juce::Decibels::decibelsToGain (gainstaging::silentBelowDb);
        struct Kept
        {
            juce::AudioBuffer<float> audio;
            double startBeat;
            bool complete;
        };

        std::vector<Kept> kept;
        float loudest = 0.0f;

        for (auto& pass : passes)
        {
            juce::AudioBuffer<float> audio (r.numChannels, (int) (pass.end - pass.begin));

            for (int ch = 0; ch < r.numChannels; ++ch)
                audio.copyFrom (ch, 0, r.stream, ch, (int) pass.begin, audio.getNumSamples());

            const float peak = audio.getMagnitude (0, audio.getNumSamples());
            loudest = juce::jmax (loudest, peak);

            if (peak >= silentGain)
                kept.push_back ({ std::move (audio), pass.startBeat, pass.complete });
        }

        report.peakDb = juce::Decibels::gainToDecibels (loudest, -100.0f);
        report.removedAsSilent = ! passes.empty() && kept.empty();

        // The newest pass that ran to the loop end plays. A pass cut short by stopping stays, muted, unless it is the only one.
        int playingIndex = -1;

        for (int i = (int) kept.size() - 1; i >= 0; --i)
            if (kept[(size_t) i].complete) { playingIndex = i; break; }

        if (playingIndex < 0 && kept.size() == 1)
            playingIndex = 0;

        // Clips already on the track where the new takes go are muted, not deleted: they become takes too.
        std::vector<AudioClip*> older;
        {
            const juce::ScopedLock sl (lock);

            for (auto& c : track->clips)
                if (c->kind == Clip::Kind::audio)
                    older.push_back (static_cast<AudioClip*> (c.get()));
        }

        int number = 1;

        for (auto* clip : older)
            if (clip->name.startsWith (track->name + " take "))
                number = juce::jmax (number, clip->name.fromLastOccurrenceOf (" ", false, false).getIntValue() + 1);

        const auto folder = getRecordingFolder();
        juce::WavAudioFormat wav;

        for (size_t i = 0; i < kept.size(); ++i)
        {
            auto& take = kept[i];
            const auto name = track->name + " take " + juce::String (number++);
            const double endBeat = tempoMap.getBeatAfter (take.startBeat, take.audio.getNumSamples() / r.rate);

            for (auto* clip : older)
                if (clip->startBeat < endBeat - 1.0e-9 && take.startBeat < clip->getEndBeat (tempoMap) - 1.0e-9 && ! clip->muted)
                    setClipMuted (*clip, true);

            auto file = folder.getNonexistentChildFile (juce::File::createLegalFileName (name), ".wav", false);
            bool written = false;

            if (folder.createDirectory().wasOk())
            {
                std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());

                if (stream != nullptr)
                {
                    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                                   .withSampleRate (r.rate)
                                                                   .withNumChannels (r.numChannels)
                                                                   .withBitsPerSample (24));
                    written = writer != nullptr && writer->writeFromAudioSampleBuffer (take.audio, 0, take.audio.getNumSamples());
                }
            }

            juce::String error;
            auto* clip = written ? addAudioClip (*track, file, take.startBeat, error) : nullptr;

            if (clip == nullptr)
            {
                report.error = written ? error : "Couldn't write " + file.getFullPathName();
                continue;
            }

            clip->name = name;
            setClipMuted (*clip, (int) i != playingIndex);
            ++report.numTakes;
        }

        lastReport.push_back (report);
    }

    ++reportVersion;
}

void Engine::collectRecordedMidi()
{
    // Takes whose track or clip was deleted while recording are left alone from now on.
    takes.erase (std::remove_if (takes.begin(), takes.end(), [this] (const Take& take)
                 { return findTrackFor (take.clip) != take.track; }),
                 takes.end());

    {
        const auto scope = recordQueue.read (recordQueue.getNumReady());

        for (int i = 0; i < scope.blockSize1; ++i)
            handleRecordedEvent (recordBuffer[(size_t) (scope.startIndex1 + i)]);

        for (int i = 0; i < scope.blockSize2; ++i)
            handleRecordedEvent (recordBuffer[(size_t) (scope.startIndex2 + i)]);
    }

    if (recording.load())
        recordedUntil = juce::jmax (recordedUntil, getPositionBeats());

    // Takes grow bar by bar up to the playhead.
    bool grew = false;
    {
        const juce::ScopedLock sl (lock);

        for (auto& take : takes)
        {
            const double length = std::ceil ((recordedUntil - take.clip->startBeat) / 4.0) * 4.0;

            if (length > take.clip->lengthBeats)
            {
                take.clip->lengthBeats = length;
                grew = true;
            }
        }
    }

    if (grew)
        sendChangeMessage();
}

void Engine::handleRecordedEvent (const RecordedEvent& event)
{
    const juce::MidiMessage m (event.bytes, juce::MidiMessage::getMessageLengthFromFirstByte (event.bytes[0]));
    const int pitch = m.getNoteNumber();

    if (m.isNoteOn())
    {
        if (heldNotes[(size_t) pitch].has_value())
            finishNote (pitch, event.beat);

        heldNotes[(size_t) pitch] = HeldNote { event.beat, m.getFloatVelocity() };
    }
    else if (m.isNoteOff())
    {
        // With the sustain pedal down, a released key keeps sounding until the pedal comes up.
        if (pedalDown)
            sustainedNotes.set ((size_t) pitch);
        else if (heldNotes[(size_t) pitch].has_value())
            finishNote (pitch, event.beat);
    }
    else if (m.isSustainPedalOn())
    {
        pedalDown = true;
    }
    else if (m.isSustainPedalOff())
    {
        pedalDown = false;

        for (int p = 0; p < 128; ++p)
            if (sustainedNotes[(size_t) p] && heldNotes[(size_t) p].has_value())
                finishNote (p, event.beat);

        sustainedNotes.reset();
    }
}

void Engine::finishNote (int pitch, double endBeat)
{
    const auto held = *heldNotes[(size_t) pitch];
    heldNotes[(size_t) pitch].reset();
    sustainedNotes.reset ((size_t) pitch);

    // A note that ends before it starts was held across the loop's wrap: it ends at the loop end.
    if (endBeat < held.beat)
        endBeat = juce::jmax (held.beat, loopEndBeat.load());

    recordedUntil = juce::jmax (recordedUntil, endBeat);
    const double length = juce::jmax (1.0 / 64.0, endBeat - held.beat);

    {
        const juce::ScopedLock sl (lock);

        for (auto& take : takes)
        {
            // A track with the scale filter on only takes notes from the song key.
            if (take.track->scaleFilter.load() && ! key.contains (pitch))
                continue;

            auto& clip = *take.clip;

            // Loop recording can play a note before the take's first bar: the take then starts earlier.
            if (held.beat < clip.startBeat)
            {
                const double shift = clip.startBeat - std::floor (held.beat / 4.0) * 4.0;
                clip.startBeat -= shift;
                clip.lengthBeats += shift;

                for (auto& n : clip.notes)
                    n.start += shift;
            }

            clip.notes.push_back ({ pitch, held.beat - clip.startBeat, length, held.velocity });
            clip.lengthBeats = juce::jmax (clip.lengthBeats, std::ceil ((held.beat + length - clip.startBeat) / 4.0) * 4.0);
        }
    }

    // No notesChanged() here: that would cut off the notes still being played on the keyboard.
    sendChangeMessage();
}

//==============================================================================
void Engine::setPositionBeats (double beats) noexcept
{
    beats = juce::jmax (0.0, beats);
    pendingSeek = beats;

    if (! deviceRunning.load())
        positionBeats = beats;
}

double Engine::getPositionBeats() const noexcept
{
    const double seek = pendingSeek.load();
    return seek >= 0.0 ? seek : positionBeats.load();
}

void Engine::setTempoMap (TempoMap newMap)
{
    if (newMap == tempoMap)
        return;

    {
        const juce::ScopedLock sl (lock);   // the audio thread reads it while rendering
        tempoMap = std::move (newMap);
    }

    sendChangeMessage();
}

double Engine::getHeardPositionBeats() const
{
    const double position = getPositionBeats();
    auto* device = deviceManager.getCurrentAudioDevice();

    if (! isPlaying() || device == nullptr || device->getCurrentSampleRate() <= 0.0)
        return position;

    const double latencySeconds = (device->getOutputLatencyInSamples() + device->getCurrentBufferSizeSamples()) / device->getCurrentSampleRate();
    return juce::jmax (0.0, tempoMap.getBeatAfter (position, -latencySeconds));
}

juce::Optional<juce::AudioPlayHead::PositionInfo> Engine::getPosition() const
{
    PositionInfo info;
    info.setBpm (headBpm);
    info.setTimeSignature (TimeSignature { 4, 4 });
    info.setPpqPosition (headBeat);
    info.setPpqPositionOfLastBarStart (std::floor (headBeat / 4.0) * 4.0);
    info.setTimeInSeconds (headSeconds);
    info.setTimeInSamples ((juce::int64) (headSeconds * sampleRate));
    info.setIsPlaying (headPlaying);
    info.setIsLooping (loopEnabled.load());
    info.setLoopPoints (LoopPoints { loopStartBeat.load(), loopEndBeat.load() });
    return info;
}

void Engine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    activeDevice = device;

    // What was recorded at another rate can't be joined to what comes next: the recording ends (and is kept).
    if (recording.load() && recordingRate > 0.0 && std::abs (device->getCurrentSampleRate() - recordingRate) > 0.5)
        deviceChangedWhileRecording = true;

    // While bouncing, the device's settings are only noted: the edit stays prepared for the bounce.
    if (offline.load())
    {
        const juce::ScopedLock sl (lock);
        deviceWasRunning = true;
        deviceRate = device->getCurrentSampleRate();
        deviceBlockSize = device->getCurrentBufferSizeSamples();
        return;
    }

    prepare (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void Engine::prepare (double rate, int blockSize)
{
    const juce::ScopedLock sl (lock);

    sampleRate = rate;
    maxBlockSize = juce::jmax (64, blockSize);

    mixBuffer.setSize (2, maxBlockSize);
    clickBuffer.setSize (1, maxBlockSize);
    liveMidi.ensureSize (4096);
    previewMidi.ensureSize (1024);
    chunkPreviewMidi.ensureSize (1024);
    previewSynth.setCurrentPlaybackSampleRate (sampleRate);
    midiCollector.reset (sampleRate);
    previewCollector.reset (sampleRate);
    masterGain.reset (sampleRate, 0.02);
    toneGain.reset (sampleRate, 0.02);
    tonePhase.reset (sampleRate, 0.05);
    toneCopy.reset (sampleRate, 0.02);
    toneHz.reset (sampleRate, 0.05);
    sideOnly.reset (sampleRate, 0.05);
    monoOnly.reset (sampleRate, 0.05);

    for (auto& track : tracks)
        track->prepare (sampleRate, maxBlockSize);

    deviceRunning = true;
}

void Engine::audioDeviceStopped()
{
    activeDevice = nullptr;

    if (measureState.load() == 1)
        measureState = 0;   // the device went away: no result

    if (recording.load())
        deviceChangedWhileRecording = true;   // the message thread ends it: the take so far is kept

    const juce::ScopedLock sl (lock);

    if (offline.load())
    {
        deviceWasRunning = false;
        return;
    }

    deviceRunning = false;

    for (auto& track : tracks)
        track->releaseResources();
}

void Engine::beginOffline (double rate, int blockSize)
{
    stopRecording();   // a bounce replaces the device's callback: nothing more would come in

    const juce::ScopedLock sl (lock);

    deviceWasRunning = deviceRunning.load();
    deviceRate = sampleRate;
    deviceBlockSize = maxBlockSize;
    offline = true;
    prepare (rate, blockSize);
    setPluginsOffline (true);

    for (auto& track : tracks)
        track->snapGains = true;
}

void Engine::setBounceFocus (const BounceFocus& focus)
{
    const juce::ScopedLock sl (lock);
    bounceFocus = focus;
}

void Engine::clearBounceFocus()
{
    const juce::ScopedLock sl (lock);
    bounceFocus = {};
}

void Engine::renderOffline (float* const* channels, int numChannels, int numSamples)
{
    const juce::ScopedLock sl (lock);   // waits for the message thread, where playback would skip the block
    processBlock (nullptr, 0, channels, numChannels, numSamples);   // a bounce hears no input
}

void Engine::endOffline()
{
    const juce::ScopedLock sl (lock);

    setPluginsOffline (false);
    offline = false;
    bounceFocus = {};

    if (deviceWasRunning && deviceRate > 0.0)
    {
        prepare (deviceRate, deviceBlockSize);
    }
    else
    {
        deviceRunning = false;

        for (auto& track : tracks)
            track->releaseResources();
    }
}

void Engine::setPluginsOffline (bool isOffline)
{
    for (auto& track : tracks)
        for (auto& slot : track->plugins)
            if (slot->instance != nullptr)
                slot->instance->setNonRealtime (isOffline);
}

void Engine::audioDeviceIOCallbackWithContext (const float* const* inputs, int numInputs,
                                               float* const* outputs, int numOutputs,
                                               int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    numInputChannels = numInputs;

    if (offline.load())
    {
        // Bouncing: renderOffline() plays the song, and the device stays silent meanwhile.
        for (int ch = 0; ch < numOutputs; ++ch)
            if (outputs[ch] != nullptr)
                juce::FloatVectorOperations::clear (outputs[ch], numSamples);

        return;
    }

    processBlock (inputs, numInputs, outputs, numOutputs, numSamples);
}

void Engine::processBlock (const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples)
{
    for (int ch = 0; ch < numOutputs; ++ch)
        if (outputs[ch] != nullptr)
            juce::FloatVectorOperations::clear (outputs[ch], numSamples);

    tunerTap.push (inputs, numInputs, numSamples);   // no lock needed: the tuner hears the input even when a block is skipped

    liveMidi.clear();
    midiCollector.removeNextBlockOfMessages (liveMidi, numSamples);
    previewMidi.clear();
    previewCollector.removeNextBlockOfMessages (previewMidi, numSamples);

    const juce::ScopedTryLock sl (lock);

    if (! sl.isLocked() || sampleRate <= 0.0 || mixBuffer.getNumSamples() == 0)
        return;

    // Measuring the interface's delay plays only its clicks.
    if (measureState.load() == 1)
    {
        runLatencyMeasurement (inputs, numInputs, outputs, numOutputs, numSamples);
        return;
    }

    previewsOnOwnSynth = isOwnPreviewSoundNeeded (liveMidiTrack.load());

    blockInputs = inputs;
    blockNumInputs = inputs != nullptr ? numInputs : 0;
    meterInputs (numSamples);

    const bool nowPlaying = playing.load();
    bool flush = flushPending || nowPlaying != wasPlaying;
    bool restart = restartPending || nowPlaying != wasPlaying;   // clips are read from a new place (but not after note edits)
    wasPlaying = nowPlaying;

    if (const double seek = pendingSeek.exchange (-1.0); seek >= 0.0)
    {
        positionBeats = seek;
        flush = true;
        restart = true;
    }

    if (const int version = midiEditVersion.load(); version != lastMidiEditVersion)
    {
        lastMidiEditVersion = version;
        flush = true;
    }

    const double loopStart = loopStartBeat.load(), loopEnd = loopEndBeat.load();
    const bool looping = nowPlaying && loopEnabled.load() && loopEnd - loopStart > 0.01;
    const bool capturing = nowPlaying && recording.load() && ! liveMidi.isEmpty();
    const bool capturingAudio = nowPlaying && recording.load() && ! audioRecordings.empty();
    const bool clicking = nowPlaying && metronomeEnabled.load() && (recording.load() || ! metronomeOnlyWhileRecording.load());

    if (const double request = countInRequest.exchange (0.0); request > 0.0)
    {
        countInLeft = request;
        flush = true; // clips pause during the count-in: release their notes
    }

    if (! nowPlaying)
        countInLeft = 0.0;

    // A bounce starts at its master volume (no glide), and a single track's chain has none.
    if (offline.load())
        masterGain.setCurrentAndTargetValue (bounceFocus.mode == BounceFocus::Mode::oneChain ? 1.0f
                                                 : juce::Decibels::decibelsToGain (masterVolumeDb.load(), -60.0f));
    else
        masterGain.setTargetValue (juce::Decibels::decibelsToGain (masterVolumeDb.load(), -60.0f));

    headPlaying = nowPlaying;
    double pos = positionBeats.load();
    float peakL = 0.0f, peakR = 0.0f, peakIn = 0.0f;

    // Render in chunks no longer than what the tracks were prepared for, and split each chunk
    // at the loop end so the wrap is sample-accurate, and at tempo changes so each piece has one tempo.
    for (int done = 0; done < numSamples;)
    {
        const int chunk = juce::jmin (numSamples - done, mixBuffer.getNumSamples());
        mixBuffer.clear (0, chunk);
        clickBuffer.clear (0, chunk);

        for (int offset = 0; offset < chunk;)
        {
            int length = chunk - offset;
            const double bpm = tempoMap.getBpmAt (countInLeft > 0.0 ? pos : (looping && pos >= loopEnd ? loopStart : pos));
            const double samplesPerBeat = sampleRate * 60.0 / bpm;

            // Count-in: clicks on beats -4 to -1 while the playhead waits. Clips don't play and
            // nothing is recorded, but the keyboard can still be heard.
            if (countInLeft > 0.0)
            {
                length = juce::jlimit (1, length, (int) std::ceil (countInLeft * samplesPerBeat));
                headPlaying = false;
                renderSegment (pos, bpm, offset, done + offset, length, flush, false);
                headPlaying = nowPlaying;
                renderClicks (-countInLeft, samplesPerBeat, offset, length, flush, true);
                flush = false;

                countInLeft -= length / samplesPerBeat;

                if (countInLeft < 1.0e-9)
                {
                    countInLeft = 0.0;
                    flush = true; // the song starts here
                    restart = true;
                }

                offset += length;
                continue;
            }

            if (looping)
            {
                if (pos >= loopEnd)
                {
                    pos = loopStart;
                    flush = true;
                    restart = true;
                }

                length = juce::jlimit (1, length, (int) std::ceil ((loopEnd - pos) * samplesPerBeat));
            }

            if (const auto change = tempoMap.getNextChangeAfter (pos); change.has_value() && nowPlaying)
                length = juce::jlimit (1, length, (int) std::ceil ((*change - pos) * samplesPerBeat));

            renderSegment (pos, bpm, offset, done + offset, length, flush, restart);
            renderClicks (pos, samplesPerBeat, offset, length, flush, clicking);
            flush = false;
            restart = false;

            if (capturing)
                captureInput (pos, samplesPerBeat, done + offset, length);

            if (capturingAudio)
                captureAudio (pos, samplesPerBeat, done + offset, length);

            if (nowPlaying)
                pos += length / samplesPerBeat;

            if (looping && pos >= loopEnd - 1.0e-9)
            {
                pos = loopStart;
                flush = true;
                restart = true;
            }

            offset += length;
        }

        // Previews with no track to play them: the engine's own synth.
        if (previewsOnOwnSynth)
        {
            chunkPreviewMidi.clear();
            chunkPreviewMidi.addEvents (previewMidi, done, chunk, -done);
            previewSynth.renderNextBlock (mixBuffer, chunkPreviewMidi, 0, chunk);
        }

        renderTone (chunk);
        renderAudition (chunk);

        const float* mixL = mixBuffer.getReadPointer (0);
        const float* mixR = mixBuffer.getReadPointer (1);
        const float* click = clickBuffer.getReadPointer (0);

        if (offline.load())
        {
            // A bounce is always the mix itself, from its first sample.
            sideOnly.setCurrentAndTargetValue (0.0f);
            monoOnly.setCurrentAndTargetValue (0.0f);
        }
        else
        {
            const int mode = speakerMode.load();
            sideOnly.setTargetValue (mode == (int) SpeakerMode::leftMinusRight ? 1.0f : 0.0f);
            monoOnly.setTargetValue (mode == (int) SpeakerMode::mono ? 1.0f : 0.0f);
        }

        for (int i = 0; i < chunk; ++i)
        {
            const float g = masterGain.getNextValue();
            const float l = mixL[i] * g, r = mixR[i] * g;

            // Left minus right on both speakers, scaled so a sound on one side only keeps its level.
            const float side = sideOnly.getNextValue();
            const float difference = (l - r) * juce::MathConstants<float>::sqrt2 * 0.5f;
            // Mono is (L+R)/2 on both speakers, so a sound in the middle keeps its level.
            const float mono = monoOnly.getNextValue();
            const float sum = 0.5f * (l + r);
            const float heardL = l + side * (difference - l) + mono * (sum - l);
            const float heardR = r + side * (difference - r) + mono * (sum - r);

            if (numOutputs == 1)
            {
                if (outputs[0] != nullptr)
                    outputs[0][done + i] = 0.5f * (heardL + heardR) + click[i];
            }
            else
            {
                if (outputs[0] != nullptr) outputs[0][done + i] = heardL + click[i];
                if (outputs[1] != nullptr) outputs[1][done + i] = heardR + click[i];
            }

            peakL = juce::jmax (peakL, std::abs (l));
            peakR = juce::jmax (peakR, std::abs (r));
            peakIn = juce::jmax (peakIn, std::abs (mixL[i]), std::abs (mixR[i]));
        }

        done += chunk;
    }

    positionBeats = pos;
    countInBeatsLeft = countInLeft;
    flushPending = flush; // e.g. a loop wrap on the very last sample of this block
    restartPending = restart;

    updateMeterLevel (masterPeakLeft, peakL, numSamples, sampleRate);
    updateMeterLevel (masterPeakRight, peakR, numSamples, sampleRate);

    if (peakIn > masterInputPeak.load()) masterInputPeak = peakIn;
    if (const float peakOut = juce::jmax (peakL, peakR); peakOut > masterOutputPeak.load()) masterOutputPeak = peakOut;
}

void Engine::setTone (const Tone& tone) noexcept
{
    toneShape = tone.shape;
    toneFrequency = juce::jlimit (soundguide::lowestHeard, soundguide::highestHeard, tone.frequency);
    toneLevel = juce::Decibels::decibelsToGain (tone.levelDb);
    tonePhaseShift = tone.phaseDegrees / 360.0f;
    toneWithCopy = tone.withCopy;
    toneFlipCopy = tone.flipCopy;
    toneOn = tone.on;
}

void Engine::renderTone (int numSamples)
{
    toneGain.setTargetValue (toneOn.load() && ! offline.load() ? toneLevel.load() : 0.0f);
    toneHz.setTargetValue (toneFrequency.load());
    tonePhase.setTargetValue (tonePhaseShift.load());
    toneCopy.setTargetValue (toneWithCopy.load() ? (toneFlipCopy.load() ? -1.0f : 1.0f) : 0.0f);

    if (! toneGain.isSmoothing() && toneGain.getTargetValue() <= 0.0f)
    {
        // Silent: start the next tone at the beginning of a cycle, and at its own pitch.
        toneCycle = 0.0;
        toneHz.setCurrentAndTargetValue (toneFrequency.load());
        tonePhase.setCurrentAndTargetValue (tonePhase.getTargetValue());
        toneCopy.setCurrentAndTargetValue (toneCopy.getTargetValue());
        return;
    }

    const auto shape = toneShape.load();
    float* left = mixBuffer.getWritePointer (0);
    float* right = mixBuffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        const double increment = toneHz.getNextValue() / sampleRate;
        const float shift = tonePhase.getNextValue();
        const float copy = toneCopy.getNextValue();
        float sample = soundguide::getSample (shape, toneCycle, increment);

        if (std::abs (copy) > 0.0f)
            sample += copy * soundguide::getSample (shape, toneCycle - shift, increment);

        sample *= toneGain.getNextValue();
        left[i] += sample;
        right[i] += sample;

        toneCycle += increment;
        toneCycle -= std::floor (toneCycle);
    }
}

void Engine::startAudition (juce::AudioBuffer<float> buffer, int startSample)
{
    juce::AudioBuffer<float> old;
    const int length = buffer.getNumSamples();

    {
        const juce::ScopedLock sl (lock);
        old = std::move (auditionBuffer);
        auditionBuffer = std::move (buffer);
        auditionPosition = juce::jlimit (0, length, startSample);
        auditionOn = length > 0 && startSample < length;
    }
    // `old` is freed here, outside the lock and off the audio thread.
}

void Engine::stopAudition()
{
    juce::AudioBuffer<float> old;

    {
        const juce::ScopedLock sl (lock);
        auditionOn = false;
        old = std::move (auditionBuffer);
        auditionBuffer = {};
    }
}

void Engine::renderAudition (int numSamples)
{
    if (! auditionOn.load())
        return;

    // Pressing Play (or a bounce) ends the audition.
    if (playing.load() || offline.load())
    {
        auditionOn = false;
        return;
    }

    const int length = auditionBuffer.getNumSamples();
    const int position = auditionPosition.load();
    const int count = juce::jmin (numSamples, length - position);

    if (count > 0)
    {
        const int channels = auditionBuffer.getNumChannels();

        if (channels > 0)
        {
            mixBuffer.addFrom (0, 0, auditionBuffer, 0, position, count);
            mixBuffer.addFrom (1, 0, auditionBuffer, channels > 1 ? 1 : 0, position, count);
        }
    }

    const int next = position + juce::jmax (count, 0);
    auditionPosition = next;

    if (next >= length)
        auditionOn = false;
}

void Engine::renderClicks (double startBeat, double samplesPerBeat, int offset, int numSamples, bool restart, bool trigger)
{
    // A click starts on the first sample of each new beat. After a jump (seek, loop wrap, start)
    // only a position right on a beat clicks, so landing mid-beat stays quiet.
    if (restart || ! trigger)
        haveClickBeat = false;

    float* out = clickBuffer.getWritePointer (0, offset);

    for (int i = 0; i < numSamples; ++i)
    {
        if (trigger)
        {
            const double beat = startBeat + i / samplesPerBeat;
            const double index = std::floor (beat + 1.0e-9);
            const bool onBeat = haveClickBeat ? index > clickBeatIndex : beat - index < 1.0 / samplesPerBeat;
            haveClickBeat = true;
            clickBeatIndex = index;

            if (onBeat)
            {
                const bool barStart = std::fmod (index, 4.0) == 0.0;
                clickAge = 0;
                clickLevel = barStart ? 0.5f : 0.3f;
                clickFrequency = barStart ? 1760.0 : 1320.0;
            }
        }

        out[i] += nextClickSample();
    }
}

float Engine::nextClickSample() noexcept
{
    if (clickAge < 0)
        return 0.0f;

    // A short sine blip with a fast exponential decay.
    const double t = clickAge / sampleRate;
    const auto value = (float) (clickLevel * std::sin (juce::MathConstants<double>::twoPi * clickFrequency * t) * std::exp (-t / 0.008));

    if (++clickAge > (int) (0.05 * sampleRate))
        clickAge = -1;

    return value;
}

void Engine::renderSegment (double startBeat, double bpm, int offset, int liveMidiOffset, int numSamples, bool flushNotes, bool restarted)
{
    headBeat = startBeat;
    headBpm = bpm;
    headSeconds = tempoMap.beatToSeconds (startBeat);

    RenderContext ctx;
    ctx.sampleRate = sampleRate;
    ctx.bpm = bpm;
    ctx.tempoMap = &tempoMap;
    ctx.startBeat = startBeat;
    ctx.numSamples = numSamples;
    ctx.playing = headPlaying;
    ctx.flushNotes = flushNotes;
    ctx.liveMidiOffset = liveMidiOffset;
    ctx.inputs = blockInputs;
    ctx.numInputs = blockNumInputs;
    ctx.muteMonitoring = tunerMuteMonitoring.load() && tunerTap.isListening();
    ctx.keyPitchClasses = keyPitchClasses.load();
    ctx.restarted = restarted && headPlaying;

    // The chord of the bar the slice starts in, for plugins that follow the song (the audio thread holds the lock that guards the chords).
    if (const auto chord = chords.find ((int) std::floor (startBeat / 4.0)); chord != chords.end() && startBeat >= 0.0)
        for (const int pitchClass : chord->second.getPitchClasses())
            ctx.chordPitchClasses |= 1 << pitchClass;

    // Solo in place: a track is heard when it, a bus it plays into, or a track that plays into it is soloed.
    // soloAbove follows the edit order (a bus comes before its tracks), soloBelow the render order (a bus after them).
    // A bounce can replace the solo buttons with a set of tracks, or play one track's chain alone.
    const bool focused = offline.load();
    const bool asSoloed = focused && bounceFocus.mode == BounceFocus::Mode::heardAsSoloed;
    const bool chain = focused && bounceFocus.mode == BounceFocus::Mode::oneChain;

    if (chain)
    {
        ctx.unityOutput = true;
        ctx.instrumentOnly = bounceFocus.instrumentOnly;
        ctx.onlyClip = bounceFocus.clip;
    }

    for (auto& track : tracks)
    {
        const bool isSoloed = asSoloed ? std::find (bounceFocus.tracks.begin(), bounceFocus.tracks.end(), track.get()) != bounceFocus.tracks.end()
                                       : ! chain && track->soloed.load();
        track->countsAsSoloed = isSoloed;
        ctx.anySolo = ctx.anySolo || isSoloed;
        track->soloAbove = isSoloed || (track->bus != nullptr && track->bus->soloAbove);
        track->soloBelow = isSoloed;
        track->mutedAbove = track->bus != nullptr && (track->bus->muted.load() || track->bus->mutedAbove);

        if (track->hasInput())
            track->clearBusInput (numSamples);
    }

    // MIDI input plays the armed tracks, or the live (selected) track when none is armed.
    // Note previews always go to the live track.
    auto* live = liveMidiTrack.load();
    const bool anyArmed = std::any_of (tracks.begin(), tracks.end(), [] (auto& t) { return t->isMidi() && t->armed.load(); });

    for (auto* track : renderOrder)
    {
        if (chain)
        {
            // Only the chosen track renders, straight into the mix: no bus, return or live MIDI.
            if (track != bounceFocus.track)
                continue;

            ctx.liveMidi = nullptr;
            ctx.previewMidi = nullptr;
            ctx.onSoloPath = false;
            track->process (ctx, mixBuffer, offset);
            continue;
        }

        const bool hearsInput = anyArmed ? track->isMidi() && track->armed.load() : track == live;
        ctx.liveMidi = hearsInput ? &liveMidi : nullptr;
        ctx.previewMidi = track == live && ! previewsOnOwnSynth ? &previewMidi : nullptr;
        ctx.onSoloPath = track->soloAbove || track->soloBelow;

        if (auto* bus = track->bus)
        {
            if (bus->busInput.getNumSamples() >= numSamples)
                track->process (ctx, bus->busInput, 0);

            bus->soloBelow = bus->soloBelow || track->soloBelow;
        }
        else
        {
            track->process (ctx, mixBuffer, offset);
        }

        // A return is heard while a track on the solo path sends to it.
        if (ctx.anySolo && (track->soloAbove || track->soloBelow))
            for (auto& send : track->sends)
                if (send->isOn())
                    send->target->soloBelow = true;
    }
}
} // namespace daw
