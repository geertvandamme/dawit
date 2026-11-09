#include "SamplerGuide.h"

#include "BuiltInSampler.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace daw::samplerguide
{
namespace
{
    juce::String utf8 (const char* text) { return juce::String::fromUTF8 (text); }

    struct PageText
    {
        const char* name;
        const char* summary;
        const char* text;
    };

    // The texts are the ones in the plan's Theory section; each ends with what the picture shows.
    const char* const textRecordings =
        "A sampler is an instrument that plays recordings. Someone recorded a real piano, note by note and at several strengths. "
        "When you press middle C, the sampler plays the recording of middle C. That's why a sampled piano sounds like a piano: it is one, recorded.\n\n"
        "A synthesiser such as the DawIt Synth or DawIt Subtractive builds its sound from scratch with oscillators and filters. That is light and flexible, "
        "but an acoustic instrument's hammer noise, ringing strings and breath are very hard to build that way. A sampler sidesteps the problem by playing back the real thing.\n\n"
        "Samplers began with tape: the Mellotron (1963) had a strip of tape behind each key. The first polyphonic digital sampler on sale was the Fairlight CMI (1979), "
        "at 8 bits and about US$25,000; the E-mu Emulator (1981) had 128 kB of memory. Today a sampled instrument is a set of recordings plus a map that says which "
        "recording plays for which key, and for how hard you play.\n\n"
        "DawIt Sampler plays such sets in the SoundFont format (.sf2 and .sf3). Use it for pianos, guitars, strings, brass and drum kits when a part should sound "
        "like real players, for example before you bounce it to a WAV.\n\n"
        "What it doesn't do yet: load a single recording of your own and play it across the keys, or play SFZ libraries.\n\n"
        "The picture shows the recording that played for the last note, with the keys it covers and the note it was recorded at.";

    const char* const textPitch =
        "Every recording in a sampled instrument has a root note: the note it was recorded at. To play another note, the sampler plays the recording faster or slower, "
        "the way a record sounds higher when it spins faster. One semitone up is 1.0595 times faster (2 to the power 1/12). Twelve semitones, an octave, is exactly "
        "twice as fast: A3 at 220 Hz becomes A4 at 440 Hz, and the recording lasts half as long.\n\n"
        "That's the catch: speed changes more than pitch. A recording of A3 played as E4, 7 semitones up, runs 1.498 times faster. Its attack, its fade and any vibrato "
        "all go 1.5 times faster, and it is over in two thirds of the time. The body resonances that give an instrument or a voice its character, its formants, move up "
        "with it. A voice pushed up an octave sounds like a chipmunk, and pushed down, like a giant. A step or two is hard to hear; big steps sound unnatural.\n\n"
        "Some samplers can change the pitch without the speed (time-stretching). A SoundFont player such as DawIt Sampler changes the speed, as samplers have done from the start.\n\n"
        "The picture shows one recording at its root, and the same recording played higher or lower: the same wave, squeezed or stretched. Each click on Hear both plays the note and then the same note moved by the interval "
        "shown, and the next click moves on to the next interval, so you can see and hear how much faster and shorter it gets.";

    const char* const textZones =
        "Because a recording sounds less natural the further it's moved, good sampled instruments are multisampled: recorded every few notes, each recording covering "
        "a small range of keys, its key zone (also called a keygroup or split). A recording every 3 or 4 semitones is common, so no key is more than a semitone or two "
        "from its root; a zone often reaches up to about 5 semitones either side of its root. Some piano libraries record every key.\n\n"
        "When you play a key, the sampler finds the zone it falls in and plays that zone's recording, sped up or slowed down by the distance to its root. Where two zones "
        "meet, the change of recording can be heard as a small jump in tone. Good libraries hide it with careful recording; some overlap neighbouring zones and crossfade them.\n\n"
        "A key outside every zone plays nothing. A violin preset may stop at G3, the violin's lowest note, so a bass line played on it stays silent below that.\n\n"
        "The picture is the keyboard with the zones of your preset, one colour each, a dot on each root. The last key you played is lit, with the zone it used.";

    const char* const textVelocity =
        "Play a piano key softly and the note is dark and round; play it hard and it is bright, with more hammer. That isn't only a change of level: the timbre changes. "
        "A sampler captures this with velocity layers, recordings of the same note at different strengths, each played over a range of MIDI velocities (1 to 127). "
        "A preset might use one recording for velocities 1 to 63 and another for 64 to 127; detailed piano libraries can have 8 layers or more, at the cost of memory.\n\n"
        "So in a layered preset, velocity picks the recording as well as the level. A part drawn with every note at velocity 100 always plays the same layer, one reason "
        "programmed parts can sound mechanical. Vary the velocities and the soft notes really sound soft.\n\n"
        "Many presets, including many in free SoundFonts, have only one layer. Velocity then changes only the level.\n\n"
        "Some libraries go further with round robin: several recordings of the same note at the same strength, played in turn, so a fast repeated snare doesn't sound "
        "like a machine gun. SoundFonts have no round robin; many SFZ libraries do.\n\n"
        "The picture shows the layers under the last key you played: velocity 1 at the bottom, 127 at the top, each layer a band, and a line at the last note's velocity.";

    const char* const textLoops =
        "An organ note lasts as long as you hold the key, but a recording has an end. Samplers solve this with a loop: two points in the recording, the loop start and end, "
        "between which the sampler plays round and round while the note sounds. Placed where the wave matches, a loop can't be heard, and 2 seconds of recording can hold "
        "a note for a minute.\n\n"
        "In a SoundFont each zone says what its recording does: play once to its end (a piano or a drum, which fade by themselves); loop for as long as the note sounds; "
        "or loop while the key is held and play on to the end when you let go, so the natural release is heard.\n\n"
        "On top of the recording sits a volume envelope: the same attack, decay, sustain and release as on the synths (see What is sound?), with a hold at full level between the attack and the decay. It fades a looped note out after "
        "you let go, so it ends smoothly instead of stopping dead. Each zone has its own envelope, so a piano's low notes can ring longer than its high ones.\n\n"
        "The picture shows the recording for the last key, with its loop marked if it has one and the play position running through it. Under it is the zone's envelope, "
        "with its times in seconds.";

    const char* const textSoundFont =
        "A SoundFont is one file that holds the recordings and the map. E-mu and Creative made the format for the Sound Blaster AWE32 sound card (1994). SoundFont 2.0 (1996) "
        "was published for anyone to use, and its files end in .sf2. Version 2.04 (2005) added 24-bit samples.\n\n"
        "Inside, from the top down:\n"
        "- A preset is what you choose, such as Acoustic Grand Piano. Presets are numbered within banks of up to 128, and bank 128 is kept for drum kits.\n"
        "- A preset is made of one or more instruments, and an instrument of zones. Each zone has a key range, a velocity range, a recording, and settings such as tuning, "
        "loop, envelope and filter.\n"
        "- The samples are the recordings themselves, always mono: a stereo sound is a left and a right sample played together. One sample can serve many zones and presets.\n\n"
        "An .sf3 is the same thing with its recordings compressed with Ogg Vorbis, the way an MP3 is compressed. MuseScore made it, and the files are about a fifth of the size: "
        "MuseScore General is 40 MB as .sf3 and 216 MB as .sf2. The compression throws a little detail away, usually too little to matter in a mix. DawIt decodes an .sf3 "
        "when it loads it.\n\n"
        "SFZ is another format: a plain text file that lists the zones and points to separate audio files (WAV or FLAC). Many free libraries come as SFZ. DawIt Sampler "
        "doesn't play SFZ yet.\n\n"
        "DawIt Sampler reads the zones, loops, envelopes and filter. It doesn't read a SoundFont's modulators, so the mod wheel does nothing in it, or its reverb and "
        "chorus amounts: add DawIt Reverb for space.\n\n"
        "The picture shows the file you chose, top to bottom: how many presets it holds, how many zones the chosen preset has, and how many recordings those zones use.";

    const char* const textGeneralMidi =
        "General MIDI (1991) fixed a numbered list of 128 sounds, so a MIDI file plays with roughly the right instruments on any device. They come in 16 families of 8: "
        "Piano 1-8, Chromatic Percussion 9-16, Organ 17-24, Guitar 25-32, Bass 33-40, Strings 41-48, Ensemble 49-56, Brass 57-64, Reed 65-72, Pipe 73-80, Synth Lead 81-88, "
        "Synth Pad 89-96, Synth Effects 97-104, Ethnic 105-112, Percussive 113-120 and Sound Effects 121-128. So 1 is the Acoustic Grand Piano, 25 the nylon guitar, "
        "34 the fingered electric bass and 49 the string ensemble.\n\n"
        "The numbers are shown from 1 to 128, as here and in most software. Inside a MIDI message they run from 0 to 127, so program 1 is sent as 0. Some SoundFont editors "
        "show 0 to 127: the same sounds, counted from 0.\n\n"
        "Drums have a map of their own: each note is a drum, not a pitch. General MIDI puts drums on channel 10 and names 47 sounds on notes 35 to 81: 36 is the kick, "
        "38 the snare, 42 the closed hi-hat, 46 the open hi-hat, 49 a crash and 51 a ride. In a SoundFont the drum kits live in bank 128. DawIt's drum patterns write this "
        "map, so they play on a sampled kit unchanged.\n\n"
        "General MIDI fixes the names, not the sound. Program 1 is a piano on every device, but each SoundFont's piano is its own recording. What each instrument can play, and how to write for it, is in Instruments in MIDI.\n\n"
        "The picture shows the 128 sounds as 16 rows of 8, with your preset lit; for a drum kit, the drum map from note 35 to 81, with the notes the kit plays lit and the six main drums named.";

    const char* const textMemory =
        "Recordings are big. One minute of mono CD-quality audio (16 bits, 44,100 samples a second) is 44,100 \xC3\x97 2 bytes \xC3\x97 60 = 5.3 MB; in stereo, 10.6 MB. "
        "A piano recorded on every key, at several strengths, each note ringing for seconds, adds up fast: big commercial piano libraries fill many gigabytes.\n\n"
        "A SoundFont is loaded into memory whole, so its notes start instantly. DawIt Sampler keeps each sample as a 32-bit number, twice the 16 bits in the file, so a "
        "SoundFont takes about twice its .sf2 size in memory: MuseScore General, 216 MB as an .sf2, takes about 430 MB. An .sf3 is decoded to full size first, which takes "
        "a moment; until it is ready the track plays the DawIt Synth.\n\n"
        "A SoundFont is loaded once, however many tracks use it: ten tracks on MuseScore General presets cost the memory of one. It is freed when no track uses it any more.\n\n"
        "The DawIt Synth needs no recordings, which is why it starts instantly and stays the default.\n\n"
        "The picture compares, for the SoundFont you chose, the file on disk, the decoded samples and the memory used.";

    const char* const textSampledOrSynth =
        "Neither is better; they are good at different things, and engineers choose differently.\n\n"
        "A sampled sound suits a part that should sound like real players: a piano ballad, a nylon guitar, a string section, a drum kit for a rock or pop song. It brings "
        "detail a synth can't fake. It costs memory and a moment to load, and it plays as recorded: you choose a preset more than you shape it.\n\n"
        "A synth suits a sound you want to shape, a sound that is electronic anyway (basses, leads and pads in dance music), or a part you want light and instant. "
        "DawIt Subtractive, Additive, FM, Drums and Physical let you turn every knob.\n\n"
        "A sampled part still has to be played like the real instrument to convince: notes in the instrument's range, varied velocities, a sustain pedal for a piano, "
        "chords a guitarist could finger. A string section playing a fast bass line won't sound like strings, however good the recordings.\n\n"
        "Many songs mix both: a sampled piano and drums with a synth bass and pad. Try a part on each: Use a sampled sound in the DawIt Synth window switches a track to the "
        "matching sampled preset, and choosing DawIt Synth in the instrument menu switches back.\n\n"
        "The button plays a short tune of four notes through your track's own sound, so you can hear it on the DawIt Synth and then on a sampled sound.";

    const PageText& getPage (Page page)
    {
        static const PageText pages[] {
            { "A sampler plays recordings", "What a sampler is, and what it's for", textRecordings },
            { "Pitch from playback speed", "Root notes, and why higher means shorter", textPitch },
            { "Key zones", "Multisampling: a recording every few notes", textZones },
            { "Velocity layers", "Why a hard hit sounds brighter, not only louder", textVelocity },
            { "Loops and the envelope", "How a short recording holds a long note", textLoops },
            { "Inside a SoundFont", "Presets, instruments, samples and banks", textSoundFont },
            { "General MIDI", "Why program 1 is a piano everywhere", textGeneralMidi },
            { "Memory and loading", "Why a sampled sound takes a moment and a lot of memory", textMemory },
            { "Sampled or synthesised?", "Which to pick for a part", textSampledOrSynth },
            { "Your track", "What this track's sampler is doing", "" },
            { "Sources", "Where the facts in this guide come from", "" },
        };
        return pages[(size_t) page];
    }

    /** The first zone of the preset that plays this key at this velocity: its index, or -1. */
    int findZone (const std::vector<soundfonts::Zone>& zones, int key, int velocity)
    {
        for (size_t i = 0; i < zones.size(); ++i)
            if (zones[i].containsKey (key) && zones[i].containsVelocity (velocity))
                return (int) i;

        return -1;
    }

    bool keyHasZone (const std::vector<soundfonts::Zone>& zones, int key)
    {
        return std::any_of (zones.begin(), zones.end(), [key] (const soundfonts::Zone& z) { return z.containsKey (key); });
    }

    using Layer = std::pair<int, int>;

    /** The distinct velocity ranges of the zones that cover a key (a stereo pair counts once). */
    std::set<Layer> getLayers (const std::vector<soundfonts::Zone>& zones, int key)
    {
        std::set<Layer> layers;

        for (auto& z : zones)
            if (z.containsKey (key))
                layers.insert ({ z.lowVelocity, z.highVelocity });

        return layers;
    }
} // namespace

juce::String getPageName (Page page) { return getPage (page).name; }
juce::String getPageSummary (Page page) { return getPage (page).summary; }

juce::String getNoteName (int key)
{
    return juce::MidiMessage::getMidiNoteName (key, true, true, 4);
}

juce::StringArray analyse (const TrackView& view)
{
    juce::StringArray lines;

    if (view.font == TrackView::Font::missing)
    {
        lines.add ("This track's SoundFont, \"" + view.fontName + "\", isn't on this computer, so the DawIt Synth's "
                   + builtInSounds::getName (view.fallbackSound).toLowerCase() + " plays instead. Add it with Add SoundFont... (or download MuseScore General) "
                   "and the sampled sound comes back; the project still remembers it.");
        return lines;
    }

    if (view.font == TrackView::Font::loading)
    {
        lines.add ("Loading " + view.fontName + ": the DawIt Synth plays until it's ready.");
        return lines;
    }

    if (view.notes.empty())
    {
        lines.add ("There are no notes on this track yet. Click Play a note in DawIt Sampler's window or draw notes in the piano roll, and this page shows which zones they use.");
        return lines;
    }

    if (view.zones.empty())
    {
        lines.add (view.presetName + " has no zones to play, so this track makes no sound.");
        return lines;
    }

    const int m = (int) view.notes.size();
    int low = 127, high = 0;

    for (auto& z : view.zones)
    {
        low = juce::jmin (low, z.lowKey);
        high = juce::jmax (high, z.highKey);
    }

    // Rule 4: notes outside every zone.
    std::vector<Note> inside;
    int outside = 0, lowestOut = 127, highestOut = 0;

    for (auto& n : view.notes)
    {
        if (keyHasZone (view.zones, n.key))
        {
            inside.push_back (n);
            continue;
        }

        ++outside;
        lowestOut = juce::jmin (lowestOut, n.key);
        highestOut = juce::jmax (highestOut, n.key);
    }

    const auto rangeText = view.presetName + " plays (" + getNoteName (low) + " to " + getNoteName (high) + ")";

    if (outside == 1)
        lines.add ("1 of " + juce::String (m) + " notes (" + getNoteName (lowestOut) + ") is outside the keys " + rangeText + ", so it makes no sound. "
                   "Move it an octave towards the middle, or pick a preset that reaches it.");
    else if (outside > 1)
        lines.add (juce::String (outside) + " of " + juce::String (m) + " notes (lowest " + getNoteName (lowestOut) + ", highest " + getNoteName (highestOut)
                   + ") are outside the keys " + rangeText + ", so they make no sound. Move them an octave towards the middle, or pick a preset that reaches them.");

    if (! view.isDrumKit && ! inside.empty())
    {
        // Rule 5: every velocity in one layer of a layered preset.
        const int s = (int) inside.size();
        int layerCount = 0;
        std::set<Layer> candidates;

        for (auto& n : inside)
        {
            const auto layers = getLayers (view.zones, n.key);
            layerCount = juce::jmax (layerCount, (int) layers.size());

            if (layers.size() >= 2)
                candidates.insert (layers.begin(), layers.end());
        }

        if (s >= 8 && layerCount >= 2)
        {
            const Layer* best = nullptr;

            for (auto& c : candidates)
            {
                const bool holdsAll = std::all_of (inside.begin(), inside.end(), [&c] (const Note& n) { return n.velocity >= c.first && n.velocity <= c.second; });

                if (holdsAll && (best == nullptr || c.second - c.first < best->second - best->first))
                    best = &c;
            }

            if (best != nullptr)
                lines.add ("All " + juce::String (s) + " notes fall in the velocity layer for " + juce::String (best->first) + "-" + juce::String (best->second)
                           + ", so you hear only one of this preset's " + juce::String (layerCount) + " recordings per key. Vary the velocities (the piano roll's velocity mode) "
                           "and soft notes will use the soft recordings, not only play quieter.");
        }

        // Rule 6: a note far from its zone's root.
        int worstDistance = 0, worstKey = 0, worstRoot = 0;

        for (auto& n : inside)
        {
            int index = findZone (view.zones, n.key, n.velocity);

            if (index < 0)
                for (size_t i = 0; i < view.zones.size(); ++i)
                    if (view.zones[i].containsKey (n.key))
                    {
                        index = (int) i;
                        break;
                    }

            const int distance = n.key - view.zones[(size_t) index].rootKey;

            if (std::abs (distance) > std::abs (worstDistance))
            {
                worstDistance = distance;
                worstKey = n.key;
                worstRoot = view.zones[(size_t) index].rootKey;
            }
        }

        if (std::abs (worstDistance) >= 12)
            lines.add (getNoteName (worstKey) + " plays a recording made at " + getNoteName (worstRoot) + ", " + juce::String (std::abs (worstDistance))
                       + " semitones away, at " + juce::String (getSpeed (worstDistance), 3) + utf8 ("\xC3\x97 its speed; stretched that far a note can sound thin or unnatural. "
                       "That is how this preset was made; another preset may cover that range better."));
    }

    if (lines.isEmpty())
    {
        std::set<int> used;

        for (auto& n : inside)
            for (size_t i = 0; i < view.zones.size(); ++i)
                if (view.zones[i].containsKey (n.key) && view.zones[i].containsVelocity (n.velocity))
                    used.insert ((int) i);

        lines.add ("All " + juce::String (m) + " notes are inside " + view.presetName + "'s range (" + getNoteName (low) + " to " + getNoteName (high) + ") and use "
                   + juce::String ((int) used.size()) + " of its " + juce::String ((int) view.zones.size()) + " zones. Nothing to change.");
    }

    return lines;
}

juce::String explain (Page page, const TrackView* track)
{
    switch (page)
    {
        case Page::yourTrack:
            if (track == nullptr)
                return "Choose a track that plays DawIt Sampler and this page looks at its notes against the zones of its sound.";

            return analyse (*track).joinIntoString ("\n\n");

        case Page::sources:
            return {};

        case Page::recordings:
        case Page::pitch:
        case Page::zones:
        case Page::velocity:
        case Page::loops:
        case Page::soundFont:
        case Page::generalMidi:
        case Page::memory:
        case Page::sampledOrSynth:
            break;
    }

    return utf8 (getPage (page).text);
}

//==============================================================================
juce::String getFamilyName (int family)
{
    static const char* const names[numFamilies] { "Piano", "Chromatic Percussion", "Organ", "Guitar", "Bass", "Strings", "Ensemble", "Brass",
                                                  "Reed", "Pipe", "Synth Lead", "Synth Pad", "Synth Effects", "Ethnic", "Percussive", "Sound Effects" };
    return names[juce::jlimit (0, numFamilies - 1, family)];
}

int getFamilyFirstProgram (int family) { return juce::jlimit (0, numFamilies - 1, family) * 8 + 1; }
int getFamilyLastProgram (int family) { return getFamilyFirstProgram (family) + 7; }

juce::String getFamilyRange (int family)
{
    return getFamilyName (family) + " " + juce::String (getFamilyFirstProgram (family)) + "-" + juce::String (getFamilyLastProgram (family));
}

int getFamilyOfShownProgram (int shownProgram)
{
    return sampler::getFamily (shownProgram - 1);
}

SampledPreset getSampledPreset (BuiltInSound sound)
{
    const auto p = sampler::getSampledPreset (sound);
    return { p.bank, p.program + 1 };
}

BuiltInSound getFallbackSound (const SampledPreset& preset)
{
    return sampler::getFallbackSound (preset.bank, preset.shownProgram - 1);
}

const std::vector<StartingPoint>& getStartingPoints()
{
    static const std::vector<StartingPoint> points {
        { "Grand piano for chords", 0, 1,
          "The most used sampled sound: it carries chords and melody alike, and its recordings change from soft to bright with velocity." },
        { "Fingered bass", 0, 34,
          "A round electric bass played with the fingers that sits under chords without covering them; the usual choice for pop." },
        { "String section pad", 0, 49,
          "A string ensemble that swells in slowly; hold whole-bar chords under a chorus." },
        { "Nylon guitar", 0, 25,
          "Plucked strings for arpeggios and soft chords; keep chords to shapes a guitarist could finger." },
        { "Drawbar organ", 0, 17,
          "A held organ tone that loops for as long as you hold the keys; good for gospel, rock and soul chords." },
        { "Standard drum kit", soundfonts::drumBank, 1,
          "A kit that follows the General MIDI drum map, so DawIt's drum patterns play on real drums unchanged." },
    };
    return points;
}

double getSpeed (int semitones)
{
    return std::pow (2.0, (double) semitones / 12.0);
}

double getLengthFactor (int semitones)
{
    return 1.0 / getSpeed (semitones);
}

const std::vector<int>& getPictureIntervals()
{
    static const std::vector<int> intervals { -12, -7, -5, 5, 7, 12 };
    return intervals;
}

juce::int64 getBytesPerMinuteMono16 (int sampleRate)
{
    return (juce::int64) sampleRate * 2 * 60;
}

juce::int64 getMemoryBytes (juce::int64 sf2Bytes)
{
    return sf2Bytes * 2;
}
} // namespace daw::samplerguide
