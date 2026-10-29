#include "MidiGuide.h"
#include "MidiExport.h"

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
        case Topic::time:        return "Time: ticks and clock";
        case Topic::files:       return "MIDI files";
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
        case Topic::time:        return "Ticks per quarter note, tempo";
        case Topic::files:       return "Chunks of tracks, events and times";
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
    std::vector<Event> events;

    for (auto& note : clip.notes)
    {
        if (note.start >= clip.lengthBeats || note.pitch < 0 || note.pitch > 127)
            continue;

        const auto velocity = (juce::uint8) toMidiVelocity (note.velocity);
        events.push_back ({ note.start, juce::MidiMessage::noteOn (1, note.pitch, velocity) });
        events.push_back ({ juce::jmin (note.getEnd(), clip.lengthBeats), juce::MidiMessage::noteOff (1, note.pitch) });
    }

    std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
    {
        if (! juce::approximatelyEqual (a.beat, b.beat))
            return a.beat < b.beat;

        return a.message.isNoteOff() && ! b.message.isNoteOff();   // offs first, so a re-struck note stops before it starts again
    });

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
    const auto file = midiexport::createClipFile (clip, trackName, { TempoMap (bpm), key });
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
    juce::String explainClip (const MidiClip* clip, double bpm, const MusicalKey& key)
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

        // As bytes, on a cable and in a file.
        const int bytes = countBytes (events, false), running = countBytes (events, true);
        text << "\n\nAs MIDI it's " << events.size() << " messages: a Note On and a Note Off for each note, " << bytes << " bytes";

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

        return text;
    }
}

juce::String explain (Topic topic, const MidiClip* clip, double bpm, const MusicalKey& key)
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
                   "The picture shows your clip's notes by their velocity: taller and brighter is harder.";

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
            return "Besides notes, MIDI carries what the pedals, wheels and knobs do.\n\n"
                   "Control Change messages (status B0 for channel 1) carry a controller number and a value from 0 to 127. Some numbers "
                   "have a fixed meaning everywhere: 1 is the modulation wheel, 7 the volume, 10 the pan, 64 the sustain pedal (64 or "
                   "more is down). Others are free for a synth to use for its knobs.\n\n"
                   "Pitch Bend has its own message and twice the detail: 14 bits, 0 to 16,383, with 8,192 in the middle for no bend. "
                   "How far the full bend goes (often 2 semitones) is set on the instrument.\n\n"
                   "Program Change picks a sound (in General MIDI, 1 is the grand piano and 25 the nylon guitar). Aftertouch says how "
                   "hard a key is pressed while it's held, for keyboards that can feel it, and System Exclusive lets a maker send "
                   "anything, such as a whole synth patch.\n\n"
                   "DawIt's clips hold notes; controllers drawn as lanes under the notes are still to come.";

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

        case Topic::yourClip:
            return explainClip (clip, bpm, key);
    }

    return {};
}
} // namespace daw::midiguide
