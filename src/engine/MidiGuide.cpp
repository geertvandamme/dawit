#include "MidiGuide.h"
#include "MidiExport.h"
#include "MidiTransforms.h"

#include <map>
#include <set>

namespace daw::midiguide
{
namespace
{
    juce::String formatMs (double seconds)
    {
        const double ms = seconds * 1000.0;
        return juce::String (ms, ms < 10.0 ? 2 : 1) + " ms";
    }

    /** "1.3.2": bar, beat and sixteenth of a beat from the clip's start, counting from 1. */
    juce::String formatPosition (double beat)
    {
        const int bar = (int) std::floor (beat / 4.0 + 1.0e-9);
        const double inBar = beat - bar * 4.0;
        const int beatInBar = (int) std::floor (inBar + 1.0e-9);
        const int sixteenth = (int) std::floor ((inBar - beatInBar) * 4.0 + 1.0e-9);
        return juce::String (bar + 1) + "." + juce::String (beatInBar + 1) + "." + juce::String (sixteenth + 1);
    }

    juce::String plural (int n, const juce::String& one, const juce::String& many)
    {
        return juce::String (n) + " " + (n == 1 ? one : many);
    }

    int readBigEndian (const juce::uint8* data, int numBytes)
    {
        int value = 0;
        for (int i = 0; i < numBytes; ++i)
            value = (value << 8) | data[i];

        return value;
    }

    /** A variable-length number (7 bits a byte, the top bit set on every byte but the last), as
        delta times are written; `length` gets how many bytes it took. */
    int readVariableLength (const juce::uint8* data, int available, int& length)
    {
        int value = 0;
        length = 0;

        while (length < available && length < 4)
        {
            const auto byte = data[length++];
            value = (value << 7) | (byte & 0x7f);

            if ((byte & 0x80) == 0)
                break;
        }

        return value;
    }

    juce::String bytesToHex (const juce::uint8* data, int length)
    {
        juce::StringArray parts;
        for (int i = 0; i < length; ++i)
            parts.add (juce::String::toHexString ((int) data[i]).paddedLeft ('0', 2).toUpperCase());

        return parts.joinIntoString (" ");
    }
}

//==============================================================================
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItIs:    return "What MIDI is";
        case Topic::history:     return "Where it came from";
        case Topic::cable:       return "The cable and its speed";
        case Topic::messages:    return "Messages, byte by byte";
        case Topic::noteNumbers: return "Note numbers and pitch";
        case Topic::velocity:    return "Velocity";
        case Topic::computerKeyboard: return "The computer keyboard";
        case Topic::controllers: return "Pedals, wheels and knobs";
        case Topic::sustainPedal: return "The sustain pedal";
        case Topic::pitchBend:   return "Pitch bend";
        case Topic::modExpressionVolume: return "Mod wheel, expression, volume";
        case Topic::time:        return "Time: ticks and clock";
        case Topic::files:       return "MIDI files";
        case Topic::retrograde:  return "Backwards: retrograde";
        case Topic::inversion:   return "Upside down: inversion";
        case Topic::newNotes:    return "New notes, same rhythm";
        case Topic::yourClip:    return "Your clip";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItIs:    return "Instructions to play, not sound";
        case Topic::history:     return "1983: one standard for every maker";
        case Topic::cable:       return "5 pins, 31,250 bits a second";
        case Topic::messages:    return "Note On, Note Off: 3 bytes each";
        case Topic::noteNumbers: return "0 to 127: middle C is 60";
        case Topic::velocity:    return "How fast the key went down";
        case Topic::computerKeyboard: return "Play notes with the letter keys when there's no MIDI keyboard";
        case Topic::controllers: return "Sustain, mod wheel, pitch bend";
        case Topic::sustainPedal: return "Controller 64: notes ring on after the keys come up";
        case Topic::pitchBend:   return "Slide the pitch: 8,192 is the middle";
        case Topic::modExpressionVolume: return "Vibrato, swells and the part's level";
        case Topic::time:        return "Ticks per quarter note, tempo";
        case Topic::files:       return "Chunks of tracks, events and times";
        case Topic::retrograde:  return "The last note plays first";
        case Topic::inversion:   return "Up a step becomes down a step";
        case Topic::newNotes:    return "Random pitches from the key";
        case Topic::yourClip:    return "The clip in the piano roll";
    }

    return {};
}

//==============================================================================
double getCableSeconds (int numBytes)
{
    return numBytes * bitsPerByteOnCable / (double) cableBitsPerSecond;
}

double getFrequency (int noteNumber)
{
    return 440.0 * std::pow (2.0, (noteNumber - 69) / 12.0);
}

juce::String getNoteName (int noteNumber)
{
    return juce::MidiMessage::getMidiNoteName (noteNumber, true, true, 4);
}

const std::vector<Controller>& getCommonControllers()
{
    static const std::vector<Controller> controllers {
        { 1, "Modulation wheel", "often vibrato, or opens a filter" },
        { 7, "Volume", "the part's level, 0 to 127" },
        { 10, "Pan", "left (0), middle (64) or right (127)" },
        { 11, "Expression", "a second volume for swells, under the main one" },
        { 64, "Sustain pedal", "64 or more is down: notes ring on" },
        { 74, "Brightness", "often a synth's filter cutoff" },
        { 121, "Reset all controllers", "everything back to where it started" },
        { 123, "All notes off", "the panic button: every note on the channel stops" },
    };

    return controllers;
}

//==============================================================================
juce::String toHex (const juce::MidiMessage& message)
{
    return bytesToHex (message.getRawData(), message.getRawDataSize());
}

juce::String toBinary (juce::uint8 byte)
{
    juce::String bits;
    for (int i = 7; i >= 0; --i)
    {
        bits << ((byte >> i) & 1 ? "1" : "0");

        if (i == 4)
            bits << " ";
    }

    return bits;
}

juce::String describe (const juce::MidiMessage& m)
{
    const auto channel = "channel " + juce::String (m.getChannel());

    if (m.isNoteOn())
        return "Note On, " + channel + ", " + getNoteName (m.getNoteNumber()) + " (note " + juce::String (m.getNoteNumber())
               + "), velocity " + juce::String (m.getVelocity());

    if (m.isNoteOff())
        return "Note Off, " + channel + ", " + getNoteName (m.getNoteNumber()) + " (note " + juce::String (m.getNoteNumber()) + ")"
               + (m.getRawDataSize() >= 3 && m.getRawData()[0] >= 0x90 ? juce::String (", as a Note On at velocity 0") : juce::String());

    if (m.isController())
    {
        const auto& known = getCommonControllers();
        const auto it = std::find_if (known.begin(), known.end(), [&m] (const Controller& c) { return c.number == m.getControllerNumber(); });
        return "Control Change, " + channel + ", controller " + juce::String (m.getControllerNumber())
               + (it != known.end() ? " (" + it->name.toLowerCase() + ")" : juce::String()) + ", value " + juce::String (m.getControllerValue());
    }

    if (m.isPitchWheel())
        return "Pitch Bend, " + channel + ", " + juce::String (m.getPitchWheelValue())
               + (m.getPitchWheelValue() == pitchBendCentre ? " (the middle: no bend)" : juce::String());

    if (m.isProgramChange())
        return "Program Change, " + channel + ", sound " + juce::String (m.getProgramChangeNumber() + 1);

    if (m.isChannelPressure())
        return "Aftertouch, " + channel + ", pressure " + juce::String (m.getChannelPressureValue());

    if (m.isMidiClock())
        return "Clock (24 a quarter note)";

    if (m.isMidiStart())
        return "Start";

    if (m.isMidiStop())
        return "Stop";

    if (m.isTempoMetaEvent())
        return "Tempo: " + juce::String (juce::roundToInt (m.getTempoSecondsPerQuarterNote() * 1.0e6)) + " microseconds a quarter note ("
               + juce::String (60.0 / m.getTempoSecondsPerQuarterNote(), 2) + " BPM)";

    return m.getDescription();
}

std::vector<Event> getEvents (const MidiClip& clip)
{
    std::vector<Event> offs, ons, others;

    for (auto& note : clip.notes)
    {
        if (note.start >= clip.lengthBeats || note.pitch < 0 || note.pitch > 127)
            continue;

        const auto velocity = (juce::uint8) toMidiVelocity (note.velocity);
        ons.push_back ({ note.start, juce::MidiMessage::noteOn (1, note.pitch, velocity) });
        offs.push_back ({ juce::jmin (note.getEnd(), clip.lengthBeats), juce::MidiMessage::noteOff (1, note.pitch) });
    }

    // What playback sends: the resets at the clip's end, then the controllers (events past the end are left out).
    const auto span = controllers::getSpanMessages (clip.controllers, 0.0, clip.lengthBeats, 0.0, clip.lengthBeats + 1.0, false);

    for (auto& m : span.resets)
        others.push_back ({ m.beat, controllers::toMessage (m.kind, m.value) });

    for (auto& m : span.controllers)
        others.push_back ({ m.beat, controllers::toMessage (m.kind, m.value) });

    // At the same moment: Note Offs first (a re-struck note stops before it starts again), then controllers, then Note Ons.
    std::vector<Event> events = std::move (offs);
    events.insert (events.end(), others.begin(), others.end());
    events.insert (events.end(), ons.begin(), ons.end());

    std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b) { return a.beat < b.beat; });
    return events;
}

int countBytes (const std::vector<Event>& events, bool runningStatus)
{
    int bytes = 0, lastStatus = -1;

    for (auto& e : events)
    {
        const int size = e.message.getRawDataSize();
        const int status = e.message.getRawData()[0];
        bytes += runningStatus && status == lastStatus ? size - 1 : size;
        lastStatus = status;
    }

    return bytes;
}

int toMidiVelocity (float velocity)
{
    return juce::MidiMessage::floatValueToMidiByte (juce::jlimit (0.0f, 1.0f, velocity));
}

//==============================================================================
juce::MemoryBlock getClipFile (const MidiClip& clip, double bpm, const MusicalKey& key, const juce::String& trackName)
{
    const auto file = midiexport::createClipFile (clip, trackName, { TempoMap (bpm), key, timesig::fourFour });   // the guide's example is in 4/4
    juce::MemoryOutputStream out;
    file.writeTo (out, 1);
    return out.getMemoryBlock();
}

std::vector<FilePart> describeFile (const juce::MemoryBlock& block)
{
    std::vector<FilePart> parts;
    const auto* data = static_cast<const juce::uint8*> (block.getData());
    const int size = (int) block.getSize();

    auto add = [&] (int offset, int length, const juce::String& name, const juce::String& value)
    {
        if (offset + length <= size)
            parts.push_back ({ offset, length, name, value });
    };

    if (size < 14 || juce::String (reinterpret_cast<const char*> (data), 4) != "MThd")
        return parts;

    const int format = readBigEndian (data + 8, 2), tracks = readBigEndian (data + 10, 2), division = readBigEndian (data + 12, 2);
    add (0, 4, "MThd", "the header chunk starts");
    add (4, 4, "Length", juce::String (readBigEndian (data + 4, 4)) + " bytes follow");
    add (8, 2, "Format", juce::String (format) + (format == 0 ? ": one track" : format == 1 ? ": tracks played together" : ": separate songs"));
    add (10, 2, "Tracks", juce::String (tracks));
    add (12, 2, "Division", juce::String (division) + " ticks a quarter note");

    // The first track (the tempo, time and key) event by event, and the start of the next.
    int at = 14;

    for (int track = 0; track < 2 && at + 8 <= size; ++track)
    {
        const int length = readBigEndian (data + at + 4, 4);
        add (at, 4, "MTrk", "track " + juce::String (track + 1) + (track == 0 ? " (the song: tempo, time, key)" : " (the notes)"));
        add (at + 4, 4, "Length", juce::String (length) + " bytes follow");
        const int end = juce::jmin (size, at + 8 + length);
        int p = at + 8, shown = 0, runningStatus = 0;

        while (p < end && shown < (track == 0 ? 5 : 4))
        {
            int deltaLength = 0;
            const int delta = readVariableLength (data + p, end - p, deltaLength);
            add (p, deltaLength, "Delta", juce::String (delta) + (delta == 1 ? " tick" : " ticks") + " later");
            p += deltaLength;

            if (p >= end)
                break;

            const int status = data[p] >= 0x80 ? data[p] : runningStatus;
            const int start = p;
            juce::String name, value;

            if (status == 0xff && p + 2 < end)
            {
                const int type = data[p + 1];
                int lengthBytes = 0;
                const int metaLength = readVariableLength (data + p + 2, end - p - 2, lengthBytes);
                const auto* meta = data + p + 2 + lengthBytes;
                p += 2 + lengthBytes + metaLength;

                switch (type)
                {
                    case 0x51: name = "Tempo"; value = juce::String (readBigEndian (meta, 3)) + " microseconds a quarter note"; break;
                    case 0x58: name = "Time signature"; value = juce::String (meta[0]) + "/" + juce::String (1 << meta[1]); break;
                    case 0x59: name = "Key signature"; value = (meta[0] >= 0x80 ? juce::String (256 - meta[0]) + " flats" : juce::String (meta[0]) + " sharps")
                                                               + (meta[1] != 0 ? ", minor" : ", major"); break;
                    case 0x03: name = "Track name"; value = "\"" + juce::String (reinterpret_cast<const char*> (meta), (size_t) metaLength) + "\""; break;
                    case 0x2f: name = "End of track"; break;
                    default:   name = "Meta event"; value = "type " + juce::String (type); break;
                }
            }
            else if (status >= 0x80 && status < 0xf0)
            {
                const bool running = data[p] < 0x80;
                const int dataBytes = (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 1 : 2;
                p += (running ? 0 : 1) + dataBytes;
                runningStatus = status;

                const auto message = juce::MidiMessage ((juce::uint8) status, data[p - dataBytes], dataBytes > 1 ? data[p - 1] : 0);
                name = (message.isNoteOn() ? "Note On" : message.isNoteOff() ? "Note Off" : "Event") + juce::String (running ? " (running status)" : "");
                value = describe (message).fromFirstOccurrenceOf (", ", false, false);
            }
            else
            {
                break;   // System Exclusive or something DawIt doesn't write: stop describing here
            }

            add (start, p - start, name, value);
            ++shown;
        }

        at += 8 + length;
    }

    return parts;
}

//==============================================================================
VelocityStats getVelocityStats (const MidiClip& clip)
{
    VelocityStats stats;
    std::set<int> seen;
    double sum = 0.0;

    for (auto& note : clip.notes)
    {
        if (note.start >= clip.lengthBeats)
            continue;

        const int v = toMidiVelocity (note.velocity);
        stats.lowest = stats.count == 0 ? v : juce::jmin (stats.lowest, v);
        stats.highest = stats.count == 0 ? v : juce::jmax (stats.highest, v);
        sum += v;
        seen.insert (v);
        ++stats.count;
    }

    stats.average = stats.count > 0 ? sum / stats.count : 0.0;
    stats.distinct = (int) seen.size();
    return stats;
}

//==============================================================================
namespace
{
    juce::String explainTurningAround (const MidiClip& clip, const MusicalKey& key, bool drumSound)
    {
        // Only notes starting before the clip's end count, as for the transforms.
        miditransform::Notes notes;
        for (auto& n : clip.notes)
            if (n.start < clip.lengthBeats)
                notes.push_back (n);

        if (notes.empty())
            return {};

        if (drumSound)
            return "It plays a drum sound, so only Reverse in time fits: its notes pick drums, not pitches.";

        const miditransform::Selection all;
        const auto range = miditransform::getPitchRange (notes, all, clip.lengthBeats);
        const auto distinct = miditransform::countDistinctPitches (notes, all, clip.lengthBeats);
        const auto random = miditransform::getRandomRange (range.lowest, range.highest);
        const auto keyName = key.getName();
        const auto pick = "Randomise pitch would pick from the " + juce::String ((int) miditransform::getKeyPitches (key, random.lowest, random.highest).size())
                          + " notes of " + keyName + " between " + getNoteName (random.lowest) + " and " + getNoteName (random.highest) + ".";
        juce::String text;

        if (distinct < 2)
        {
            text << "All its notes are " << getNoteName (range.lowest) << ", so there's nothing to mirror. Randomise pitch would give it a tune from the "
                 << (int) miditransform::getKeyPitches (key, random.lowest, random.highest).size() << " notes of " << keyName << " between "
                 << getNoteName (random.lowest) << " and " << getNoteName (random.highest) << ".";
        }
        else
        {
            // The note that ends last would play first when reversed.
            const auto last = std::max_element (notes.begin(), notes.end(), [] (auto& a, auto& b) { return a.getEnd() < b.getEnd(); });
            text << "Reversed in time, its last note, " << getNoteName (last->pitch) << ", would play first. Reversed in pitch, "
                 << getNoteName (range.lowest) << " and " << getNoteName (range.highest) << " swap places. " << pick;
        }

        int outside = 0;
        for (auto& n : notes)
            if (! key.contains (n.pitch))
                ++outside;

        if (outside >= 1)
            text << " " << outside << (outside == 1 ? " of its notes is outside " : " of its notes are outside ") << keyName
                 << ": reversing in the key or randomising moves " << (outside == 1 ? "it" : "them") << " into it.";

        if ((int) notes.size() >= 8 && distinct >= 3)
            text << " Try it on a copy: duplicate the clip and transform the copy, then play them one after the other. "
                    "A phrase answered by its own inversion is an old trick.";

        return text;
    }

    juce::String explainClip (const MidiClip* clip, double bpm, const MusicalKey& key, bool drumSound, const ControllerContext& context)
    {
        if (clip == nullptr)
            return "Select a MIDI clip on a track, and it shows in the piano roll: this topic then looks at its notes as MIDI, "
                   "and the pictures of the other topics use them as their example.";

        const auto events = getEvents (*clip);
        const auto stats = getVelocityStats (*clip);

        if (stats.count == 0)
            return "\"" + clip->name + "\" has no notes yet. Click in the piano roll to draw some, or record them from a MIDI keyboard, "
                   "and this topic shows what they are as MIDI.";

        juce::String text;
        const auto name = clip->name.isNotEmpty() ? "\"" + clip->name + "\"" : juce::String ("This clip");

        // The notes and their range.
        int lowest = 127, highest = 0;
        for (auto& n : clip->notes)
            if (n.start < clip->lengthBeats)
            {
                lowest = juce::jmin (lowest, n.pitch);
                highest = juce::jmax (highest, n.pitch);
            }

        text << name << " has " << plural (stats.count, "note", "notes") << ", from " << getNoteName (lowest) << " (note " << lowest << ", "
             << juce::String (getFrequency (lowest), 1) << " Hz) to " << getNoteName (highest) << " (note " << highest << ", "
             << juce::String (getFrequency (highest), 1) << " Hz).";

        if (lowest < pianoLowest || highest > pianoHighest)
            text << " Some are outside a piano's 88 keys (A0 to C8): MIDI goes from 0 to 127, but a piano sound may not play them.";

        // Velocity.
        text << "\n\nVelocity: ";

        if (stats.distinct == 1)
            text << "every note is at " << stats.lowest << ". That's how a machine plays: nobody strikes every key exactly as hard. "
                 << "Try the notes on the beat a little harder (say 100) and the ones between softer (say 70), or let Humanize vary them.";
        else if (stats.highest - stats.lowest < 12)
            text << "from " << stats.lowest << " to " << stats.highest << ", around " << juce::roundToInt (stats.average)
                 << ": very even. A little more difference between strong and weak notes makes it sound played.";
        else
            text << "from " << stats.lowest << " to " << stats.highest << ", around " << juce::roundToInt (stats.average) << ", with "
                 << stats.distinct << " different values: it has the ups and downs of a player.";

        // Pedal, bend and the other controllers.
        const auto controllerText = explainControllers (*clip, context);

        if (controllerText.isNotEmpty())
            text << "\n\n" << controllerText;

        // As bytes, on a cable and in a file.
        const int bytes = countBytes (events, false), running = countBytes (events, true);
        int controllerMessages = 0;
        for (auto& e : events)
            controllerMessages += e.message.isNoteOnOrOff() ? 0 : 1;

        text << "\n\nAs MIDI it's " << events.size() << " messages: a Note On and a Note Off for each note, "
             << (controllerMessages > 0 ? plural (controllerMessages, "controller message", "controller messages") + ", " : juce::String())
             << bytes << " bytes";

        if (running < bytes)
            text << ", or " << running << " with running status (the status byte left out when it repeats)";
        else
            text << ". Running status saves nothing here, because Note Ons and Note Offs take turns; that's why many keyboards send a Note "
                    "On at velocity 0 instead of a Note Off, so every message after the first can leave its status byte out";

        text << ". On a 5-pin cable they take " << formatMs (getCableSeconds (running)) << " in all.";

        // The worst pile-up: notes starting at the same moment.
        std::map<double, int> starts;
        for (auto& e : events)
            if (e.message.isNoteOn())
                ++starts[std::round (e.beat * 960.0) / 960.0];

        const auto busiest = std::max_element (starts.begin(), starts.end(), [] (auto& a, auto& b) { return a.second < b.second; });

        if (busiest != starts.end() && busiest->second > 1)
            text << " The most notes starting together is " << busiest->second << ", at " << formatPosition (busiest->first)
                 << ": over a cable the last of them sounds " << formatMs (getCableSeconds (3 + (busiest->second - 1) * 2) - getCableSeconds (3))
                 << " after the first. Too little to hear, but it adds up on a busy cable with many parts.";

        // On the grid, or played in.
        int offGrid = 0;
        for (auto& n : clip->notes)
            if (n.start < clip->lengthBeats && std::abs (n.start * 4.0 - std::round (n.start * 4.0)) > 0.02)
                ++offGrid;

        text << "\n\nTiming: " << (offGrid == 0 ? juce::String ("every note starts on a sixteenth note, as when it's drawn or quantized.")
                                                : juce::String (offGrid) + " of the notes start between the sixteenths, as when they're played in by hand.")
             << " In a MIDI file at 960 ticks a quarter note, the clip's "
             << juce::String (clip->lengthBeats, juce::exactlyEqual (clip->lengthBeats, std::round (clip->lengthBeats)) ? 0 : 2) << " beats are "
             << juce::roundToInt (clip->lengthBeats * 960.0) << " ticks.";

        const auto file = getClipFile (*clip, bpm, key, clip->name);
        text << " Dragged out of DawIt as a MIDI file, it's " << (int) file.getSize() << " bytes; the same length as audio, at 44.1 kHz in stereo "
             << "16-bit, would be about " << juce::String (clip->lengthBeats * 60.0 / juce::jmax (1.0, bpm) * 44100.0 * 4.0 / 1.0e6, 1) << " MB.";

        const auto turning = explainTurningAround (*clip, key, drumSound);

        if (turning.isNotEmpty())
            text << "\n\nTurning it around: " << turning;

        return text;
    }
}

//==============================================================================
double getBendSemitones (int value)
{
    return (double) (juce::jlimit (0, controllers::bendMax, value) - pitchBendCentre) / pitchBendCentre * 2.0;
}

double getGainDb (int volume, int expression)
{
    if (volume <= 0 || expression <= 0)
        return -100.0;

    return 40.0 * std::log10 (juce::jmin (127, volume) / 127.0) + 40.0 * std::log10 (juce::jmin (127, expression) / 127.0);
}

MidiClip getSustainExample()
{
    MidiClip clip;
    clip.name = "Sustain example";
    clip.lengthBeats = 16.0;
    const float velocity = 100.0f / 127.0f;
    const int chords[4][3] = { { 60, 64, 67 }, { 57, 60, 64 }, { 53, 57, 60 }, { 55, 59, 62 } };

    for (int c = 0; c < 4; ++c)
    {
        for (int pitch : chords[c])
            clip.notes.push_back ({ pitch, c * 4.0, 1.0, velocity });

        clip.controllers.push_back ({ controllers::sustainPedal, c * 4.0 + 0.25, 127 });
        clip.controllers.push_back ({ controllers::sustainPedal, c * 4.0 + 4.0, 0 });
    }

    controllers::tidy (clip.controllers);
    return clip;
}

MidiClip getBendExample()
{
    MidiClip clip;
    clip.name = "Bend example";
    clip.lengthBeats = 4.0;
    clip.notes.push_back ({ 69, 0.0, 2.0, 100.0f / 127.0f });

    auto up = controllers::getLinePoints (controllers::pitchBend, 0.25, controllers::bendCentre, 0.75, controllers::bendMax);
    auto down = controllers::getLinePoints (controllers::pitchBend, 1.5, controllers::bendMax, 1.75, controllers::bendCentre);
    clip.controllers = up;
    clip.controllers.insert (clip.controllers.end(), down.begin(), down.end());
    controllers::tidy (clip.controllers);
    return clip;
}

MidiClip getSwellExample()
{
    MidiClip clip;
    clip.name = "Swell example";
    clip.lengthBeats = 8.0;

    for (int pitch : { 60, 64, 67 })
        clip.notes.push_back ({ pitch, 0.0, 8.0, 90.0f / 127.0f });

    auto up = controllers::getLinePoints (controllers::expression, 0.0, 40, 4.0, 127, 0.125);
    auto down = controllers::getLinePoints (controllers::expression, 4.0, 127, 8.0, 60, 0.125);
    clip.controllers = up;
    clip.controllers.insert (clip.controllers.end(), down.begin(), down.end());
    controllers::tidy (clip.controllers);
    return clip;
}

namespace
{
    /** "+2", "-1.5", "0": semitones to two decimals with the zeros left off, and a proper minus sign. */
    juce::String formatSemitones (double semitones)
    {
        const double rounded = std::round (semitones * 100.0) / 100.0;

        if (juce::approximatelyEqual (rounded, 0.0))
            return "0";

        auto text = juce::String (std::abs (rounded), 2);

        while (text.endsWith ("0"))
            text = text.dropLastCharacters (1);

        if (text.endsWith ("."))
            text = text.dropLastCharacters (1);

        return juce::String::fromUTF8 (rounded < 0.0 ? "\xe2\x88\x92" : "+") + text;
    }

    juce::String joinWithOr (const juce::StringArray& names)
    {
        if (names.size() < 2)
            return names.joinIntoString (", ");

        auto head = names;
        head.remove (head.size() - 1);
        return head.joinIntoString (", ") + " or " + names[names.size() - 1];
    }

    /** What a pedal press is: the beats it went down and came up (the end of the clip when it never came up). */
    struct Press
    {
        double start = 0.0, end = 0.0;
    };
}

juce::String explainControllers (const MidiClip& clip, const ControllerContext& context)
{
    using namespace controllers;

    std::vector<const MidiNote*> notes;
    for (auto& n : clip.notes)
        if (n.start < clip.lengthBeats)
            notes.push_back (&n);

    if (notes.empty())
        return {};

    // Events at or after the clip's end are never played.
    List audible;
    for (auto& e : clip.controllers)
        if (e.beat < clip.lengthBeats - 1.0e-9)
            audible.push_back (e);

    if (audible.empty())
        return "Controllers: No pedal, bends or controllers in this clip. Press + Lane under the piano roll to draw some.";

    const auto barOf = [&] (double beat) { return (int) std::floor ((context.clipStartBeat + beat) / 4.0 + 1.0e-9) + 1; };
    juce::StringArray sentences;

    // The sustain pedal.
    std::vector<Press> presses;
    bool stillDown = false;
    {
        bool down = false;
        double since = 0.0;

        for (auto& e : getLane (audible, sustainPedal))
        {
            if (e.value >= 64 && ! down)
            {
                down = true;
                since = e.beat;
            }
            else if (e.value < 64 && down)
            {
                down = false;
                presses.push_back ({ since, e.beat });
            }
        }

        if (down)
        {
            presses.push_back ({ since, clip.lengthBeats });
            stillDown = true;
        }
    }

    if (! presses.empty())
    {
        double total = 0.0;
        for (auto& p : presses)
            total += juce::jmin (p.end, clip.lengthBeats) - p.start;

        const int percent = juce::roundToInt (100.0 * total / juce::jmax (1.0e-9, clip.lengthBeats));
        sentences.add ("The sustain pedal goes down " + (presses.size() == 1 ? juce::String ("once") : juce::String ((int) presses.size()) + " times")
                       + ", for " + juce::String (percent) + "% of the clip.");

        juce::String held;

        if (! context.chords.empty())
        {
            for (auto& press : presses)
            {
                for (size_t i = 1; i < context.chords.size() && held.isEmpty(); ++i)
                {
                    const double change = context.chords[i].beat;

                    if (change > press.start + 0.25 + 1.0e-9 && change < press.end - 1.0e-9)
                        held = "The pedal is held through the change from " + context.chords[i - 1].name + " to " + context.chords[i].name
                               + " at bar " + juce::String (barOf (change)) + ", so both chords ring together. Try lifting it as the "
                               + context.chords[i].name + " starts and pressing again just after.";
                }

                if (held.isNotEmpty())
                    break;
            }
        }
        else
        {
            for (auto& press : presses)
                if (press.end - press.start >= 16.0 - 1.0e-9)
                {
                    held = "The pedal stays down for " + plural (juce::roundToInt ((press.end - press.start) / 4.0), "bar", "bars") + " from bar "
                           + juce::String (barOf (press.start)) + ", so the notes pile up. Pianists usually lift it and press again at each chord change.";
                    break;
                }
        }

        if (held.isNotEmpty())
            sentences.add (held);

        if (stillDown)
            sentences.add ("The pedal is still down at the end of the clip. DawIt lifts it there, so notes stop at the clip's end.");
    }

    // Pitch bend.
    const auto bend = getLane (audible, pitchBend);

    if (! bend.empty())
    {
        int lowest = bend.front().value, highest = bend.front().value;
        for (auto& e : bend)
        {
            lowest = juce::jmin (lowest, e.value);
            highest = juce::jmax (highest, e.value);
        }

        if (lowest == highest)
            sentences.add ("Pitch bend stays at " + formatSemitones (getBendSemitones (lowest)) + " semitones (at the usual range of 2).");
        else
            sentences.add ("Pitch bend goes from " + formatSemitones (getBendSemitones (lowest)) + " to " + formatSemitones (getBendSemitones (highest))
                           + " semitones (at the usual range of 2).");

        if (bend.back().value != bendCentre)
            sentences.add ("The bend ends at " + formatSemitones (getBendSemitones (bend.back().value)) + " semitones. DawIt centres it at the clip's end; "
                           "add a point at 0 where the bend should finish.");
    }

    // A level of 0 under a note.
    for (int kind : { volume, expression })
    {
        const auto lane = getLane (audible, kind);

        for (size_t i = 0; i < lane.size(); ++i)
        {
            if (lane[i].value != 0)
                continue;

            double runEnd = clip.lengthBeats;
            for (size_t j = i + 1; j < lane.size(); ++j)
                if (lane[j].value > 0)
                {
                    runEnd = lane[j].beat;
                    break;
                }

            const bool playing = std::any_of (notes.begin(), notes.end(), [&] (const MidiNote* n)
            {
                return n->start < runEnd - 1.0e-9 && n->getEnd() > lane[i].beat + 1.0e-9;
            });

            if (playing)
            {
                sentences.add (getName (kind) + " is at 0 from bar " + juce::String (barOf (lane[i].beat)) + " while notes play, so they are silent there.");
                break;
            }
        }
    }

    // What the instrument does with them.
    const auto used = getKindsUsed (audible);

    switch (context.instrument)
    {
        case ControllerContext::Instrument::builtIn:
        case ControllerContext::Instrument::builtInPercussion:
        case ControllerContext::Instrument::dawItSynth:
        case ControllerContext::Instrument::drums:
        {
            const bool drums = context.instrument == ControllerContext::Instrument::drums;
            // The built-in drum sounds still follow volume and expression, but hits ring out and don't bend.
            const bool percussion = context.instrument == ControllerContext::Instrument::builtInPercussion;
            juce::StringArray ignored;
            std::vector<int> ignoredKinds;

            for (int kind : used)
                if (drums || ! (kind == volume || kind == expression || (! percussion && (kind == sustainPedal || kind == pitchBend))))
                {
                    ignored.add (getName (kind).toLowerCase());
                    ignoredKinds.push_back (kind);
                }

            if (! ignored.isEmpty())
            {
                const auto name = context.instrumentName.isNotEmpty() ? context.instrumentName : juce::String ("This instrument");
                const auto effect = ignored.size() == 1 ? "the " + getName (ignoredKinds.front()) + " lane changes nothing here"
                                                        : juce::String ("those lanes change nothing here");
                sentences.add (name + " doesn't use the " + joinWithOr (ignored) + ", so " + effect + (drums || percussion ? "." : "; a plugin instrument may."));
            }

            break;
        }
        case ControllerContext::Instrument::plugin:
            sentences.add ("What " + (context.instrumentName.isNotEmpty() ? context.instrumentName : juce::String ("the plugin"))
                           + " does with controllers is up to it; its manual lists the ones it uses.");
            break;
        case ControllerContext::Instrument::unknown:
            break;
    }

    if (sentences.isEmpty())
        sentences.add ("It has " + plural ((int) audible.size(), "controller point", "controller points") + " in " + plural ((int) used.size(), "lane", "lanes") + ".");

    return "Controllers: " + sentences.joinIntoString (" ");
}


juce::String explain (Topic topic, const MidiClip* clip, double bpm, const MusicalKey& key, bool drumSound, const ControllerContext& context)
{
    switch (topic)
    {
        case Topic::whatItIs:
            return "MIDI (Musical Instrument Digital Interface) carries no sound at all. It carries instructions: which key went down, "
                   "how hard, when it came up, what the sustain pedal and the wheels did. Like a player piano's roll, or sheet music "
                   "for machines.\n\n"
                   "That's why a MIDI clip can change instrument and an audio clip can't. The same notes can play a piano, a synth or a "
                   "drum kit: the instrument on the track turns the instructions into sound. Change the sound of the track and the same "
                   "melody plays on something else. An audio clip is the sound itself, so it stays what it was recorded as.\n\n"
                   "It's also why MIDI is so easy to edit. A wrong note is one number to change; in a recording it would be part of the "
                   "sound. And it's small: a whole song of MIDI is a few kilobytes, where the same song as audio is tens of megabytes.\n\n"
                   "The picture shows your clip's first notes as what MIDI really sends: a Note On when a key goes down and a Note Off "
                   "when it comes up.";

        case Topic::history:
            return "Before MIDI, every maker connected its instruments its own way. Moog and others used control voltage and gate (a "
                   "voltage for the pitch, another for key down), Roland had its DCB bus, Oberheim its own. A Moog couldn't play a Roland.\n\n"
                   "In 1981 Dave Smith of Sequential Circuits proposed one digital connection for everyone, worked out with Ikutaro "
                   "Kakehashi of Roland and the Japanese makers. In January 1983, at the NAMM show, a Sequential Prophet-600 played a "
                   "Roland Jupiter-6 over a MIDI cable: two makers, one standard. Smith and Kakehashi later got a Technical Grammy for it.\n\n"
                   "Then came the Standard MIDI File (1988), so songs could move between programs, and General MIDI (1991): the same 128 "
                   "sounds and the same drum map on every device, which is why a MIDI file of a song plays with roughly the right "
                   "instruments anywhere. MIDI moved to USB and later to Bluetooth and networks. MPE (2018) gives every note its own "
                   "channel, so controllers like the ROLI Seaboard and the LinnStrument can bend and shape each note on its own. MIDI 2.0 "
                   "(2020) adds far finer steps (32 bits instead of 7) and lets devices tell each other what they can do.\n\n"
                   "More than 40 years on, a keyboard from 1983 can still play a computer from today: MIDI 1.0 never stopped working.";

        case Topic::cable:
            return "The classic MIDI socket is a round 5-pin DIN, though only three pins are used: two carry the signal and one is the "
                   "shield. Devices have an IN, an OUT and often a THRU, which passes on what comes in, so one keyboard can play several "
                   "sound modules in a chain.\n\n"
                   "The signal is a current through a light inside the receiving device (an optocoupler), so the two devices are never "
                   "connected electrically. That keeps hum out: no ground loops between stage gear.\n\n"
                   "It's slow by today's standards: 31,250 bits a second. Each byte takes 10 bits on the wire (a start bit, 8 bits and "
                   "a stop bit), so a three-byte Note On takes 0.96 ms. A chord isn't sent at once: its notes go one after the other, "
                   "about a millisecond apart, as the picture shows for a C major chord. You don't hear that for a chord, but with many "
                   "parts on one cable, a big moment can smear by several milliseconds.\n\n"
                   "Small gear often uses 3.5 mm jacks instead (TRS MIDI, type A or B), and most keyboards today send MIDI over USB, "
                   "which is far faster but carries the same messages.";

        case Topic::messages:
            return "Every MIDI message starts with a status byte, which says what it is and for which of 16 channels, followed by one "
                   "or two data bytes. A status byte always has its first bit 1 (128 to 255), a data byte its first bit 0, which is why "
                   "MIDI values go from 0 to 127.\n\n"
                   "Note On is 9 in the first half of the status byte and the channel in the second: 90 in hexadecimal for channel 1. "
                   "Then the note number and the velocity. So 90 3C 64 means: channel 1, note 60 (middle C), velocity 100. Note Off is "
                   "80 instead of 90. Many devices send a Note On at velocity 0 instead of a Note Off: it means the same.\n\n"
                   "Running status saves a byte: when the status byte would be the same as the one before (a run of Note Ons on the "
                   "same channel), it's left out and only the two data bytes are sent.\n\n"
                   "The 16 channels let one cable carry 16 parts: the bass on channel 2, the drums on 10 (as in General MIDI), and each "
                   "sound module listens to its own.\n\n"
                   "The picture lists your clip's notes as the messages they're sent as, in hexadecimal, with the status byte in binary "
                   "too. While the song plays, the message at the playhead lights up.";

        case Topic::noteNumbers:
            return "A note is a number from 0 to 127, one per semitone. Note 60 is middle C and note 69 is the A above it, the 440 Hz "
                   "that orchestras tune to. Every 12 numbers is an octave: 72 is the next C up, 48 the C below.\n\n"
                   + juce::String::fromUTF8 ("The pitch of any note number is 440 \xc3\x97 2^((n \xe2\x88\x92 69) / 12) Hz: 12 steps up doubles the "
                                             "frequency (see What is sound? in the wave editor).\n\n")
                   + "Makers don't agree on the octave number: DawIt, like most, calls note 60 C4; Yamaha and some DAWs call it C3. It's "
                   "the same note, only the name differs, which is why the number is the safer way to say it.\n\n"
                   "128 notes is more than a piano's 88 keys, which go from A0 (21) to C8 (108). The extra notes are for synths and for "
                   "drum maps, where each number is a drum: in General MIDI 36 is the kick and 38 the snare.\n\n"
                   "Click a key in the picture to hear it and see the message it sends. Click lower on a key to play it harder, as the "
                   "velocity shows.";

        case Topic::velocity:
            return "Velocity is how fast a key went down, measured by the keyboard as the time between two contacts under the key, and "
                   "sent as 1 to 127. It isn't loudness itself: what it does is up to the instrument.\n\n"
                   "A piano sound plays louder and brighter, as a real piano does when struck harder. A sampled instrument may switch to "
                   "another recording (a soft and a hard hit of a snare sound different, not only louder). A synth can open its filter "
                   "or speed up its attack. An organ ignores it completely.\n\n"
                   "Notes drawn with a mouse all get the same velocity, and that's one reason programmed parts sound mechanical. Players "
                   "accent the beat, play the notes between softer, and swell through a phrase. Varying the velocity (by hand, or with "
                   "Humanize) is the quickest way to make a part sound played.\n\n"
                   "The picture shows your clip's notes by their velocity: taller and brighter is harder. In the piano roll, the Velocity lane "
                   "under the notes shows the same bars: drag one up or down to change it.";

        case Topic::computerKeyboard:
            return juce::String::fromUTF8 ("No MIDI keyboard at hand? The letter keys of your computer can play one. Turn it on with the keyboard button in the "
                   "transport bar, or Cmd+K. A strip at the bottom of the window then shows which letter plays which note.\n\n"
                   "The middle row of letters is the white keys: A plays C, S plays D, D plays E, and so on up to L, the D an octave above. "
                   "The row above is the black keys, each sitting between the two white keys it lies between on a piano: W is C♯, between "
                   "A and S, and E is D♯. R and I play nothing, because a piano has no black key between E and F or between B and C. "
                   "That's 16 notes, an octave and a bit: enough for most melodies and bass lines.\n\n"
                   "At first A plays middle C (C4, note 60). Z and X move every key down or up an octave, from C0 to C8, so the same letters "
                   "play a bass line two octaves down or a tune one octave up. C and V make the notes softer or harder in steps: 20, 40, 60, "
                   "80, 100, 120 and 127. It starts at 100.\n\n"
                   "Why steps? A real keyboard measures how fast each key goes down (see Velocity). Most computer keys are plain switches, "
                   "on or off, so they can't tell a soft touch from a hard one. Every note gets the same velocity until you change it. Vary "
                   "it afterwards in the piano roll, or with Humanize, to make the part sound played.\n\n"
                   "The keys play exactly as a MIDI keyboard would. They play the selected track, or the armed tracks when any are armed, "
                   "and R records what you play. With the scale filter on, keys outside the song's key stay silent, and the strip greys them "
                   "out. On a drum track each key is a drum. The kick is note 36 (C2), so press Z twice: then A plays the kick, S the snare "
                   "and T the closed hi-hat.\n\n"
                   "Chords are harder. Many computer keyboards can only be sure of two keys at once. Hold three, and one may not sound, or "
                   "a key you didn't press may play. Keyboards sold with \"n-key rollover\" (often gaming keyboards) don't have this limit. "
                   "If a chord loses a note, draw it in the piano roll or put it on the chord track instead.\n\n"
                   "While it's on, the letters play notes instead of being shortcuts: K plays C, so turn the metronome on and off with its "
                   "button. Space, Return and R still play, go to the start and record, and shortcuts with Cmd work as always. Typing in a "
                   "text box still types. The notes follow the letters printed on the keys, so on a keyboard with another layout, such as a "
                   "French AZERTY one, the two rows aren't straight.\n\n"
                   "The picture shows the keys as they are now: the letter on each, the note it plays, what you're holding down, and which "
                   "track hears it. Click a key to hear it.");

        case Topic::controllers:
            return juce::String::fromUTF8 (
                   "Besides notes, MIDI carries what the pedals, wheels and knobs do.\n\n"
                   "Control Change messages (status B0 for channel 1) carry a controller number and a value from 0 to 127. Some numbers have a fixed meaning everywhere: 1 is the modulation wheel, 7 the volume, 10 the pan, 11 the expression, 64 the sustain pedal (64 or more is down). Others are free for a synth to use for its knobs. Numbers 120 to 127 aren't knobs at all: they are channel messages such as All notes off.\n\n"
                   "Pitch Bend has its own message and twice the detail: 14 bits, 0 to 16,383, with 8,192 in the middle for no bend. How far the full bend goes (often 2 semitones) is set on the instrument.\n\n"
                   "Program Change picks a sound (in General MIDI, 1 is the grand piano and 25 the nylon guitar). Aftertouch, or pressure, says how hard a key is pressed while it's held, for keyboards that can feel it, and System Exclusive lets a maker send anything, such as a whole synth patch.\n\n"
                   "In DawIt a clip keeps its controllers in lanes under the piano roll: press + Lane to add one. Each point is one message, and its value holds until the next point, just as on a cable: a smooth curve is many small steps. When you stop or jump, DawIt centres the bend, lifts the pedal, sets the mod wheel and pressure to 0 and the expression to 127, then sends what the lanes say at the new place. It does the same at the end of each clip. So a part sounds the same wherever you start it, and no note is left bent or held. Volume, pan and the other knobs keep the last value they got, as they do on a synth.");

        case Topic::sustainPedal:
            return juce::String::fromUTF8 (
                   "On a piano, the right-hand pedal lifts every damper off the strings. A note keeps ringing after you let go of its key, until it dies away or the pedal comes up, and the other strings ring along quietly, which makes the sound fuller.\n\n"
                   "On a keyboard the pedal is sent as controller 64: 0 to 63 is up, 64 to 127 is down. Most pedals are switches and send only 0 and 127. Some digital pianos send the values in between for half-pedalling, and some piano instruments use them; others treat anything from 64 up as down.\n\n"
                   "The pedal doesn't make the notes longer in the MIDI: the keys still come up when they do, and it's the instrument that keeps them sounding. So a pedalled note looks short in the piano roll, and the Sustain pedal lane under it shows the press as a bar.\n\n"
                   "Pianists lift the pedal and press it again with each new chord. Lift it as the new chord sounds and press it again just after, while the keys are still down: the old chord stops, the new one is caught, and there's no gap. This is called legato or syncopated pedalling. Hold the pedal through a chord change and both chords ring together, which blurs them.\n\n"
                   "The picture shows four chords, C, Am, F and G, each key held for one beat. The pedal goes down a quarter beat after each chord and comes up as the next one starts, so each chord rings for the whole bar: 2 seconds at 120 BPM, though the keys were down for half a second. When your clip uses the pedal, the picture shows your clip instead.\n\n"
                   "The instrument decides what the pedal does. DawIt's instruments hold notes while it's down; drum sounds ring out as they would anyway, and some synth sounds ignore it.");

        case Topic::pitchBend:
            return juce::String::fromUTF8 (
                   "Pitch bend slides the pitch up or down while a note plays, like a guitarist pushing a string or a singer sliding into a note. On a keyboard it's the wheel or lever left of the keys, and it springs back to the middle when you let go.\n\n"
                   "It has its own message, with 14 bits for its value: 0 to 16,383, with 8,192 in the middle for no bend. The extra detail is for smooth slides: with 7 bits, a bend of 2 semitones each way would move in steps of about 3 cents (a cent is a hundredth of a semitone); with 14 bits a step is 0.02 cents.\n\n"
                   "The message doesn't say how many semitones it means. The instrument decides, with its bend range. General MIDI instruments start at 2 semitones each way, and so do DawIt's. Then 12,288 is 1 semitone up, 4,096 is 1 down and 0 is 2 down; the top, 16,383, is a hair under 2 up. Many synths let you set 12 or 24 for big dives. If a plugin uses another range, the same lane bends further or less, so check the range in the plugin.\n\n"
                   "A semitone up makes the frequency 1.0595 times higher. Bend A4 (440 Hz) up 1 semitone and it plays 466.2 Hz, the B flat; up 2 and it plays 493.9 Hz, the B.\n\n"
                   "A bend moves every note sounding on the channel, so bending a chord bends all of it. For a guitar-style bend, bend one note on its own. Guitarists most often bend by a semitone (one fret) or a whole tone (two frets), sometimes a tone and a half.\n\n"
                   "The picture shows a guitar-style bend on A4: the note starts, rises a whole tone to B4 in half a beat, holds, and comes back down before it ends. The scale on the left is in semitones at a range of 2. When your clip has a pitch bend, the picture shows your clip instead.");

        case Topic::modExpressionVolume:
            return juce::String::fromUTF8 (
                   "Three controllers shape a sound while it plays.\n\n"
                   "The mod wheel (controller 1) sits next to the pitch wheel but stays where you leave it. What it does is up to the sound: on most synths it adds vibrato, and on many orchestral sample libraries it fades between soft and loud recordings. DawIt's own instruments don't use it yet; a plugin's manual says what it does there.\n\n"
                   "Volume (controller 7) is the part's level, and expression (controller 11) a second level inside it, for swells. A common way to work: set the volume once to balance the part, then draw crescendos and fades with expression. Some sample libraries use controllers 1, 7 and 11 differently, so check the manual.\n\n"
                   "In General MIDI both follow the same curve: each lowers the level by 40 × log10 (127 ÷ value) dB, and the two add up. At 127 nothing changes; at 100 the part is 4.2 dB quieter; at 64, 11.9 dB; at 0 it's silent. Volume 100 with expression 64 is 16.1 dB down. Sound modules usually start a part at volume 100; DawIt's own instruments start at 127, so a volume lane can only turn them down.\n\n"
                   "Unlike the mixer's fader, these travel with the notes: a swell drawn in the clip moves when you move the clip, and goes into an exported MIDI file.\n\n"
                   "The picture shows a string chord held for two bars, with expression rising from 40 (20 dB down) to 127 over the first bar and falling to 60 (13 dB down) over the second. When your clip has expression (or volume without expression), the picture shows your clip instead.");

        case Topic::time:
        {
            const double secondsPerBeat = 60.0 / juce::jmax (1.0, bpm);
            return "On a cable, MIDI has no timeline: a message happens the moment it arrives. Time comes from whatever plays it: a "
                   "sequencer, a DAW or a MIDI file.\n\n"
                   "A MIDI file counts time in ticks, a fixed number per quarter note (the division, or PPQ). DawIt writes 960 ticks a "
                   "quarter note, so a sixteenth note is 240 ticks and a triplet eighth 320. The tempo is a separate event, in "
                   "microseconds per quarter note, so the same file can play faster or slower without moving a single note.\n\n"
                   "At your song's " + juce::String (bpm, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " BPM a quarter note is "
                   + juce::String (juce::roundToInt (secondsPerBeat * 1.0e6)) + " microseconds, so one tick lasts "
                   + juce::String (secondsPerBeat / 960.0 * 1000.0, 2) + " ms.\n\n"
                   "To keep devices together, MIDI clock sends 24 pulses per quarter note, plus Start, Stop and Continue, so a drum "
                   "machine follows the DAW's tempo. MIDI Time Code sends hours, minutes, seconds and frames instead, for film.\n\n"
                   "Quantize moves notes onto the nearest grid line in ticks, and swing moves every second one a little later (see the "
                   "guide to note lengths, beats and bars).";
        }

        case Topic::files:
            return "A Standard MIDI File is a series of chunks. The first, MThd, is the header: the format, how many tracks, and the "
                   "ticks per quarter note. Then one MTrk chunk per track, each a list of events.\n\n"
                   "Every event starts with its delta time: the ticks since the event before, written in as few bytes as it needs "
                   "(7 bits a byte). Then the message itself, the same bytes as on a cable, or a meta event that only files have: the "
                   "tempo, the time signature, the key signature, a track's name, the end of a track.\n\n"
                   "Type 0 files hold everything in one track; type 1 files have a track per part, played together, with the tempo in "
                   "the first. DawIt writes type 1: a first track with the song's tempo, time and key, then the notes. That's what you "
                   "get when you drag a clip out of DawIt or export a track.\n\n"
                   "The picture shows the start of your clip's file, byte by byte, with what each part says.";

        case Topic::retrograde:
            return juce::String::fromUTF8 (
                   "Play a melody from its last note to its first and you get its retrograde (Latin for \"going backwards\"). It's one of "
                   "the oldest ways to make a new line out of one you already have.\n\n"
                   "Take the start of Frère Jacques: C D E C, E F G, where the G lasts twice as long. Backwards it's G (long) F E C E D C. "
                   "The same notes with the same lengths, in the opposite order, so the rhythm runs backwards too: the long note now comes "
                   "first.\n\n"
                   "Composers have done this for over 600 years. In Guillaume de Machaut's song Ma fin est mon commencement (\"my end is my "
                   "beginning\") one part sings another part backwards. In Bach's Musical Offering one voice of a canon plays the other "
                   "backwards, and the minuet of Haydn's Symphony No. 47 sounds the same played from either end. In twelve-tone music, the "
                   "retrograde is one of a row's four standard forms.\n\n"
                   "Don't expect to recognise it. Music runs forwards, and a melody backwards rarely sounds like itself: in listening tests "
                   "(Dowling, 1972), people spotted a backwards melody more often than by guessing, but far from always. That's what makes "
                   "it useful. It sounds new, yet it has the same notes, the same length and the same feel as the original, so it still "
                   "fits the song.\n\n"
                   "In DawIt: Transform > Reverse in time, on the piano roll's toolbar or a MIDI clip's right-click menu. With no notes "
                   "selected, the whole clip turns around, rests included: a note 1 beat long at the start of a 4-beat clip moves to beat "
                   "4. With notes selected, only they turn around, between the first one's start and the last one's end. Each note keeps "
                   "its length and velocity; its end becomes its start. Do it again and you're back where you started.\n\n"
                   "One thing to watch: the silence after a note now comes before it. A short note on the beat with a rest after it lands "
                   "after the beat. Notes that each last until the next one starts keep their rhythm exactly. Move short ones back onto the "
                   "beat if you want them there.\n\n"
                   "This reverses the notes, not the sound: every note still starts with its attack. Playing a recording backwards (an "
                   "audio clip's Reverse) is a different effect, where every note swells up and stops dead.\n\n"
                   "The picture shows your clip's first 8 bars (or the start of Frère Jacques) before and after Reverse in time. Click one "
                   "to hear it.");

        case Topic::inversion:
            return juce::String::fromUTF8 (
                   "Turn a melody upside down and every step up becomes a step down, and every step down a step up. That's its "
                   "inversion. The rhythm stays exactly as it was; only the pitches change.\n\n"
                   "Frère Jacques again: C D E C, E F G goes up, up, down, up, up, up. Mirrored around the middle of its range, it becomes "
                   "G F E G, E D C: down, down, up, down, down, down. The highest and lowest notes swap places: the low C becomes a G and "
                   "the high G a C.\n\n"
                   "There are two ways to mirror. Exactly keeps every step's size in semitones: up 4 semitones (C to E) becomes down 4 "
                   "semitones. Mirrored exactly, C D E C, E F G becomes G F E♭ G, E♭ D C. E♭ isn't in C major, so the result sounds darker, "
                   "partly in another key. In the key counts notes of the scale instead: up two scale notes becomes down two scale notes. "
                   "Every note stays in the key, though some steps change size: C up to E is 4 semitones, G down to E is 3. Tonal music "
                   "usually mirrors in the key: in the D minor fugue of Bach's Well-Tempered Clavier, Book 1, the theme comes back upside "
                   "down that way. Twelve-tone music mirrors exactly, and the inversion is one of a row's four standard forms, with the "
                   "retrograde and the retrograde inversion (backwards and upside down).\n\n"
                   "Mirroring exactly turns a major chord into a minor one. C E G is 4 semitones and then 3 up from C. Mirrored around its "
                   "middle it becomes G E♭ C, which from the bottom is C E♭ G: 3 and then 4, C minor. Mirrored in the key, C E G becomes "
                   "G E C, still C major.\n\n"
                   "This isn't a chord inversion. A chord inversion puts another of a chord's notes at the bottom (C/E is C major with E "
                   "lowest). Mirroring a melody is a different thing with the same name.\n\n"
                   "In DawIt: Transform > Reverse in pitch, in the key or exactly. The mirror is the middle of the notes' range, so the line "
                   "stays where it is and its lowest and highest notes swap. It needs at least two different pitches. Do the same one again "
                   "and you're back where you started. In the key, a note outside the key first moves to the nearest note of the key (the "
                   "lower one when it's halfway: C♯ becomes C in C major), so that note doesn't come back exactly.\n\n"
                   "The picture shows your clip before, mirrored in the key and mirrored exactly, each with its mirror line dashed. Notes "
                   "outside the key are outlined. Click one to hear it.");

        case Topic::newNotes:
            return juce::String::fromUTF8 (
                   "A melody is two things at once: when the notes play (the rhythm) and which notes they are (the pitches). Keep the "
                   "rhythm, change the pitches, and you have a new melody that still moves like the old one. Randomise pitch does exactly "
                   "that: every note keeps its start, length and velocity, and gets a new pitch picked at random from the song's key.\n\n"
                   "Why the key? It limits the choice to notes that belong. In C major from C4 to C5 that's 8 notes (C D E F G A B C) "
                   "instead of 13. With 8 choices, a 7-note phrase can come out 8 × 8 × 8 × 8 × 8 × 8 × 8 = 2,097,152 ways, and none has a "
                   "note outside the key. Most won't be good; some will give you an idea you wouldn't have had. Roll again until one does, "
                   "keep the parts you like and fix the rest by hand.\n\n"
                   "Chance in music isn't new. In the 18th century, musical dice games let anyone put together a minuet or a waltz by "
                   "rolling dice to pick each bar from a table of bars that all fitted. The best-known one was printed in 1792 under "
                   "Mozart's name, though that has never been confirmed. The trick is the same as here: the dice choose, but only among "
                   "choices that already fit.\n\n"
                   "What makes random notes sound more like a tune:\n"
                   "• Stay near the original: each note moves at most 1, 2 or 3 notes of the scale from its old pitch, so the shape of the "
                   "line survives.\n"
                   "• Avoid repeating the same pitch: with 8 notes to choose from, about 1 note in 8 would otherwise repeat the one before.\n"
                   "• A narrow range: the notes' own range, widened to an octave when it's smaller.\n"
                   "• Chords move together: notes that start together move by the same number of scale notes, so a three-note chord stays "
                   "a chord of the key. C E G moved up one scale note becomes D F A: it can turn from major to minor.\n\n"
                   "What it doesn't do: it doesn't know your chords. A note can be in the key and still rub against the chord under it: an "
                   "F held over a C chord often sounds unsettled, because F is a semitone above the chord's E. It doesn't shape phrases "
                   "either, so it won't end a line on the home note. Those choices are still yours; the guides to chord progressions and to "
                   "finding the key and chords help with them.\n\n"
                   "In DawIt: Transform > Randomise pitch in (the song's key), then Randomise again to roll another from the same original "
                   "notes until you like one. Undo puts the notes back. Drum sounds are left out: their notes choose a drum, not a pitch.\n\n"
                   "The picture shows your clip before and after one roll with the options at their defaults, with the key's notes as lit "
                   "rows. Click one to hear it.");

        case Topic::yourClip:
            return explainClip (clip, bpm, key, drumSound, context);
    }

    return {};
}

std::vector<MidiNote> getTurnAroundExample()
{
    return { { 60, 0.0, 1.0, 0.8f }, { 62, 1.0, 1.0, 0.8f }, { 64, 2.0, 1.0, 0.8f }, { 60, 3.0, 1.0, 0.8f },
             { 64, 4.0, 1.0, 0.8f }, { 65, 5.0, 1.0, 0.8f }, { 67, 6.0, 2.0, 0.8f } };
}
} // namespace daw::midiguide
