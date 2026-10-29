#include "RoutingGuide.h"

#include "BuiltInCompressor.h"
#include "BuiltInDelay.h"
#include "BuiltInDistortion.h"
#include "BuiltInDrums.h"
#include "BuiltInGate.h"
#include "BuiltInReverb.h"
#include "BuiltInSaturation.h"
#include "GainStaging.h"
#include "Routing.h"
#include "TrackNameWords.h"

namespace daw::routingguide
{
namespace
{
    /** "Kick", "Kick and Snare", "Kick, Snare and Hats". */
    juce::String joinNames (const juce::StringArray& names)
    {
        if (names.size() <= 1)
            return names.joinIntoString ("");

        return names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1];
    }

    using trackname::getWords;
    using trackname::nameHasWord;

    Family getFamilyFromName (const juce::String& name)
    {
        const auto words = getWords (name);

        if (nameHasWord (words, { "drum", "drums", "kick", "snare", "hat", "hats", "hihat", "tom", "toms", "cymbal", "crash", "ride",
                                  "overhead", "overheads", "oh", "room", "perc", "percussion", "clap", "shaker", "808", "beat" }))
            return Family::drums;

        if (nameHasWord (words, { "bass", "sub" }))
            return Family::bass;

        if (nameHasWord (words, { "vocal", "vocals", "vox", "voc", "voice", "bv", "bvs", "backing", "harmony", "harmonies", "choir", "adlib" }))
            return Family::vocals;

        if (nameHasWord (words, { "guitar", "guitars", "gtr", "gtrs" }))
            return Family::guitars;

        if (nameHasWord (words, { "keys", "piano", "organ", "rhodes", "wurli", "pad", "pads" }))
            return Family::keys;

        return Family::none;
    }

    juce::String nextBusName (const Tracks& tracks)
    {
        for (int n = 1;; ++n)
        {
            const auto name = "Bus " + juce::String (n);

            if (std::none_of (tracks.begin(), tracks.end(), [&name] (auto& t) { return t->isBus() && t->name == name; }))
                return name;
        }
    }

    juce::String plural (int count, const juce::String& one, const juce::String& many)
    {
        return juce::String (count) + " " + (count == 1 ? one : many);
    }

    float toDb (float linear) { return juce::Decibels::gainToDecibels (linear, -100.0f); }

    juce::Colour palette (int i)
    {
        static const juce::uint32 colours[] = { 0xff4fa3ff, 0xffff8a4f, 0xff5fd38d, 0xffc57bff,
                                                0xffffd24f, 0xff4fe0e0, 0xffff5c8a, 0xffa3c94f };
        return juce::Colour (colours[(size_t) i % std::size (colours)]);
    }

    /** Builds an example tree from a short description. */
    struct ExampleBuilder
    {
        std::vector<Node> nodes;

        int bus (const juce::String& name, int parent, juce::Colour colour, float levelDb, float inputDb)
        {
            Node n;
            n.name = name;
            n.colour = colour;
            n.isBus = true;
            n.parent = parent;
            n.levelDb = levelDb;
            n.inputDb = inputDb;
            nodes.push_back (n);
            return (int) nodes.size() - 1;
        }

        int track (const juce::String& name, int parent, juce::Colour colour, float levelDb)
        {
            Node n;
            n.name = name;
            n.colour = colour;
            n.parent = parent;
            n.levelDb = levelDb;
            nodes.push_back (n);
            return (int) nodes.size() - 1;
        }

        int returnTrack (const juce::String& name, juce::Colour colour, float levelDb)
        {
            Node n;
            n.name = name;
            n.colour = colour;
            n.isReturn = true;
            n.levelDb = levelDb;
            n.inputDb = levelDb - 2.0f;
            nodes.push_back (n);
            return (int) nodes.size() - 1;
        }

        void send (int from, int to, float levelDb, bool preFader = false)
        {
            nodes[(size_t) from].sends.push_back ({ to, levelDb, preFader });
        }
    };

    /** Vocal, Guitar and Pad send to one Reverb at the levels of fact 9; Bass doesn't send. */
    std::vector<Node> makeSendsExample()
    {
        ExampleBuilder b;
        const int vocal = b.track ("Vocal", -1, palette (5), -9.0f);
        const int guitar = b.track ("Guitar", -1, palette (3), -13.0f);
        const int pad = b.track ("Pad", -1, palette (4), -18.0f);
        b.track ("Bass", -1, palette (2), -10.0f);
        const int reverb = b.returnTrack ("Reverb", palette (6), -14.0f);
        b.send (vocal, reverb, -12.4f);
        b.send (guitar, reverb, -8.9f);
        b.send (pad, reverb, -1.4f);
        return b.nodes;
    }

    /** The Reverb guide's depth picture (mixes 15 to 45%) as send levels (fact 9 applied to fact 22). */
    std::vector<Node> makeDepthExample()
    {
        ExampleBuilder b;
        const int lead = b.track ("Lead vocal", -1, palette (5), -9.0f);
        const int drums = b.track ("Drums", -1, palette (1), -8.0f);
        const int guitar = b.track ("Guitar", -1, palette (3), -13.0f);
        const int piano = b.track ("Piano", -1, palette (0), -15.0f);
        const int strings = b.track ("Strings", -1, palette (7), -17.0f);
        const int pad = b.track ("Pad", -1, palette (4), -18.0f);
        const int reverb = b.returnTrack ("Reverb", palette (6), -10.0f);
        b.send (lead, reverb, -12.4f);
        b.send (drums, reverb, -9.8f);
        b.send (guitar, reverb, -8.9f);
        b.send (piano, reverb, -7.7f);
        b.send (strings, reverb, -4.3f);
        b.send (pad, reverb, -1.4f);
        return b.nodes;
    }

    std::vector<Node> makeDrumsExample()
    {
        ExampleBuilder b;
        const int drums = b.bus ("Drums", -1, palette (1), -8.0f, -6.0f);
        b.track ("Kick", drums, palette (0), -12.0f);
        b.track ("Snare", drums, palette (2), -14.0f);
        b.track ("Hats", drums, palette (3), -18.0f);
        return b.nodes;
    }

    std::vector<Node> makeFirstExample()
    {
        ExampleBuilder b;
        const int drums = b.bus ("Drums", -1, palette (1), -8.0f, -6.0f);
        b.track ("Kick", drums, palette (0), -12.0f);
        b.track ("Snare", drums, palette (2), -14.0f);
        b.track ("Hats", drums, palette (3), -18.0f);
        const int vocals = b.bus ("Vocals", -1, palette (6), -7.0f, -5.0f);
        const int lead = b.track ("Lead vocal", vocals, palette (5), -9.0f);
        const int backing = b.bus ("Backing vocals", vocals, palette (4), -13.0f, -11.0f);
        b.track ("BV 1", backing, palette (7), -16.0f);
        b.track ("BV 2", backing, palette (0), -16.0f);
        b.track ("Bass", -1, palette (2), -10.0f);
        const int guitar = b.track ("Guitar", -1, palette (3), -13.0f);
        const int reverb = b.returnTrack ("Reverb", palette (6), -14.0f);
        b.send (lead, reverb, -12.0f);
        b.send (guitar, reverb, -9.0f);
        return b.nodes;
    }

    std::vector<Node> makePopExample()
    {
        ExampleBuilder b;
        const int drums = b.bus ("Drums", -1, palette (1), -8.0f, -6.0f);
        b.track ("Kick", drums, palette (0), -12.0f);
        b.track ("Snare", drums, palette (2), -14.0f);
        b.track ("Hats", drums, palette (3), -18.0f);
        b.track ("Overheads", drums, palette (4), -17.0f);
        b.track ("Bass", -1, palette (5), -10.0f);
        const int guitars = b.bus ("Guitars", -1, palette (2), -11.0f, -9.0f);
        b.track ("Guitar L", guitars, palette (6), -14.0f);
        b.track ("Guitar R", guitars, palette (7), -14.0f);
        const int keys = b.bus ("Keys", -1, palette (3), -12.0f, -10.0f);
        b.track ("Piano", keys, palette (0), -15.0f);
        b.track ("Pad", keys, palette (4), -18.0f);
        const int vocals = b.bus ("Vocals", -1, palette (6), -7.0f, -5.0f);
        b.track ("Lead vocal", vocals, palette (5), -9.0f);
        const int backing = b.bus ("Backing vocals", vocals, palette (4), -13.0f, -11.0f);
        b.track ("BV 1", backing, palette (7), -16.0f);
        b.track ("BV 2", backing, palette (0), -16.0f);
        b.track ("BV 3", backing, palette (1), -16.0f);
        return b.nodes;
    }
} // namespace

//==============================================================================
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::what:     return "What is routing?";
        case Topic::why:      return "What a bus is for";
        case Topic::howTo:    return "Grouping tracks in DawIt";
        case Topic::levels:   return "Levels add up";
        case Topic::glue:     return "Glue: one compressor for the group";
        case Topic::mixBus:   return "The mix bus (the master)";
        case Topic::typical:  return "Typical buses in a song";
        case Topic::sends:    return "Sends and returns";
        case Topic::sendHowTo:    return "Returns in DawIt";
        case Topic::inspector:    return "The inspector";
        case Topic::insertOrSend: return "Insert or send?";
        case Topic::depth:    return "Depth with send levels";
        case Topic::preFader: return "Pre-fader and post-fader";
        case Topic::solo:     return "Solo and mute with buses";
        case Topic::signalFlow: return "Signal flow";
        case Topic::bounce:   return "Bouncing to audio";
        case Topic::inPlace:  return "Bounce in place";
        case Topic::yourSong: return "Your song's routing";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::what:     return "Where each track's sound goes";
        case Topic::why:      return "Treat a group of tracks as one";
        case Topic::howTo:    return "Select, right-click, Create bus";
        case Topic::levels:   return "Why a bus is usually louder than its tracks";
        case Topic::glue:     return "Make the drums move as one";
        case Topic::mixBus:   return "The last bus: headroom and a gentle glue";
        case Topic::typical:  return "The groups most mixes use";
        case Topic::sends:    return "One reverb shared by many tracks";
        case Topic::sendHowTo:    return "Add a return, turn up the send";
        case Topic::inspector:    return "One track's effects, sends, pan and volume, beside the tracks";
        case Topic::insertOrSend: return "Which effects go on the track, which on a return";
        case Topic::depth:    return "More reverb sounds further away";
        case Topic::preFader: return "Does the send follow the fader?";
        case Topic::solo:     return "What you hear when a track or a bus is soloed";
        case Topic::signalFlow: return "Where the sound goes, step by step, and why the order matters";
        case Topic::bounce:   return "Turn the mix, or a few tracks, into a WAV file";
        case Topic::inPlace:  return "Print a track's instrument and effects to audio, and keep the original";
        case Topic::yourSong: return "Your tracks and buses, and what to try";
    }

    return {};
}

juce::String describeGlueSettings()
{
    for (auto& preset : comp::getPresets())
    {
        if (preset.name != "Drum bus")
            continue;

        using K = comp::Knob;
        const auto& s = preset.settings;
        const int low = juce::roundToInt (preset.targetReductionDb - 0.5f), high = juce::roundToInt (preset.targetReductionDb + 0.5f);
        return comp::formatKnob (K::ratio, s) + ", " + comp::formatKnob (K::attack, s) + " attack and " + comp::formatKnob (K::release, s)
               + " release, with the threshold at " + comp::formatKnob (K::threshold, s) + ", for " + juce::String (low) + " to "
               + juce::String (high) + " dB of reduction on the loudest hits";
    }

    return {};
}

float getHeadroomDb (float peakDb)
{
    return -peakDb;
}

juce::String describeMasterLevel (float peakDb)
{
    if (peakDb <= -99.0f)
        return "No sound is reaching the master. Play the song to see its peak.";

    const auto peak = "Peak " + gainstaging::formatDb (peakDb);

    if (peakDb > 0.05f)
        return peak + ": over 0 dB, so it clips.";

    const auto headroom = juce::String (getHeadroomDb (peakDb), 1) + " dB of headroom";

    if (peakDb > mixBusTargetPeakDb)
        return peak + ": " + headroom + ", a bit hot before mastering (about 6 dB is the usual aim).";

    return peak + ": " + headroom + ", enough.";
}

juce::String describeMixBusSettings()
{
    for (auto& preset : comp::getPresets())
    {
        if (preset.name != "Mix bus")
            continue;

        using K = comp::Knob;
        const auto& s = preset.settings;
        const int low = juce::roundToInt (preset.targetReductionDb - 0.5f), high = juce::roundToInt (preset.targetReductionDb + 0.5f);
        return comp::formatKnob (K::ratio, s) + ", " + comp::formatKnob (K::attack, s) + " attack and " + comp::formatKnob (K::release, s)
               + " release, with the sidechain filter at " + comp::formatKnob (K::sidechain, s) + ", for " + juce::String (low) + " to "
               + juce::String (high) + " dB of reduction on the loudest parts (DawIt Compressor's Mix bus starting point)";
    }

    return {};
}

juce::String explain (Topic topic)
{
    switch (topic)
    {
        case Topic::what:
            return "Every sound takes a path. It starts at an instrument or a recording, goes through the track's effects, its fader "
                   "and its pan, and then goes out: straight into the master, or into a bus first, which has its own effects, fader "
                   "and pan. The master adds everything up, sets the final volume and sends it to your speakers.\n\n"
                   "In DawIt every track goes straight to the master until you put it in a bus. The picture above shows the paths as "
                   "a tree: the tracks on the left, the buses to their right and the master at the far right. The arrows run the way "
                   "the sound travels, and each box has a small meter.\n\n"
                   "Mixers also have sends: a copy of a track, at its own level, that goes to a return track with a shared effect, such "
                   "as one reverb for the whole song. The track still plays to its own place; the send only adds to it. The picture "
                   "draws sends as dashed lines, and the topics from Sends and returns on explain them.";

        case Topic::why:
            return "A bus is a track without clips that other tracks play into. Think of it as a folder that also mixes: everything "
                   "in it is added together, and the bus has its own effects, fader, pan, mute and solo.\n\n"
                   "What that gives you:\n"
                   "- Effects on the group: one compressor that makes a drum kit move together (glue), or one EQ for all the "
                   "backing vocals.\n"
                   "- One fader for a whole section: all the backing vocals 2 dB down in one move, with their balance kept.\n"
                   "- One mute or solo for a whole group.\n\n"
                   "A bus also keeps a big song tidy: fold the bus and eight drum tracks become one row.\n\n"
                   "The words vary. A bus like this is also called a group bus, a subgroup or a submix; Ableton Live calls it a group "
                   "track. A stem is a bus saved as its own audio file, such as the drums alone. In some programs a \"group\" only "
                   "links the faders without adding the sound together (a VCA group), so it can't take effects. The master is itself "
                   "the last bus: the mix bus.";

        case Topic::howTo:
            return "1. Click Kick's header, then Shift-click Hats' header: the tracks between them are selected as well. "
                   "Cmd-click adds or takes out a single track.\n\n"
                   "2. Right-click one of them and choose Create bus (or press Cmd+Shift+G). A bus row appears with the tracks indented "
                   "under it, and its name is open for typing. When the tracks have names like Kick, Snare and Hats it suggests "
                   "\"Drums\".\n\n"
                   "3. Put effects on the bus in the device bar, and turn it up or down with its fader.\n\n"
                   "Drag a track's header onto a bus to add it, and drag it out to take it out. The right-click menu has Output, "
                   "which lists the master and every bus, and so has the button under each mixer strip. Ungroup removes the bus and "
                   "keeps its tracks, and so does the x on a bus's header; Delete bus and its tracks... in the right-click menu removes "
                   "them too. The arrow in a bus's header folds it to a single row.\n\n"
                   "Grouping never changes a track's sound: its instrument, effects, fader and clips stay as they were. Only where "
                   "its sound goes changes.";

        case Topic::levels:
            return "A bus adds its tracks together, sample by sample, so it is usually louder than any one of them. How much louder "
                   "depends on whether the tracks play the same thing.\n\n"
                   "Different sounds add their power: on average, two equal tracks make +3.01 dB, four make +6.02 dB, eight +9.03 dB. "
                   "Two tracks at -6 dB come out at about -3 dB.\n\n"
                   "The same sound in step, such as two copies of one drum hit, add their amplitudes: two tracks make +6.02 dB, four "
                   "+12.04 dB. This is also the worst case for peaks. Four drum tracks at -10 dB each are about 6 dB louder together "
                   "(-3.98 dB), but when their hits line up exactly, the peaks can reach +2.04 dB.\n\n"
                   "The meters show peaks, and peaks are less predictable: hits that never happen at the same moment don't add up at "
                   "all, and the worst case is the one above. Sounds that are opposite in polarity, such as a snare's top and bottom "
                   "microphones, can even cancel each other out.\n\n"
                   "A bus doesn't make the mix any louder by itself: the same tracks add up in the master just the same. With the bus's "
                   "fader at 0 dB, its pan in the middle and no effects, the song sounds exactly as it did before you grouped it.\n\n"
                   "What to do: keep the peaks of tracks that go into one bus lower, around -18 to -12 dB. Leave the bus's fader near "
                   "0 dB and balance with the tracks' faders; use the bus's fader to move the whole group. Over 0 dB inside DawIt "
                   "doesn't distort, but the bus's effects react to the hotter level (see the Gain staging guide).";

        case Topic::glue:
            return "Glue compression is a gentle compressor on a whole group. It hears everything in the bus at once, so when the "
                   "snare hits hard, everything dips a little together. The drums then breathe as one instrument instead of "
                   "sounding like separate recordings.\n\n"
                   "The settings are gentle: " + describeGlueSettings() + ". A ratio of 2:1 is glue, not squashing; the slow attack "
                   "lets every hit keep its click; and the release should bring the reduction back to almost nothing just before the "
                   "next beat (at 120 BPM the beats are 500 ms apart). The sidechain filter keeps the kick, which has the most low end, "
                   "from deciding all the compression on its own. These are starting points, not rules: many engineers use 4:1 on "
                   "drums, or a little more reduction.\n\n"
                   "The classic example is the SSL bus compressor, with ratios of 2:1, 4:1 and 10:1, attacks from 0.1 to 30 ms and "
                   "releases from 0.1 to 1.2 seconds, or auto. For the whole mix use even less: 1 to 2 dB of reduction (see The mix "
                   "bus).\n\n"
                   "The tracks play into the bus after their faders, so turning one track up also makes the bus's compressor work "
                   "harder on it. Set the balance first, then the glue.\n\n"
                   "Effects on a bus run one after the other, on the group's sum, as on a track. EQ there shapes the group: one low cut "
                   "on all the backing vocals at once. Parallel compression, a squashed copy blended under the untouched drums, needs "
                   "no second bus in DawIt: DawIt Compressor's Parallel drums starting point does it with its Mix knob.";

        case Topic::mixBus:
            return "The mix bus is the master: the last bus in the song. Every track and every bus ends up in it, and what comes "
                   "out of it is what you hear and what Bounce to WAV saves. Its other names are the master bus, the stereo bus and "
                   "the 2-bus.\n\n"
                   "Why it matters: it is the one place where you can treat the whole song at once. A gentle compressor glues the "
                   "mix together, a final EQ tilts the whole sound, and a limiter at the very end catches the last peaks. It is also "
                   "the one level that must not clip. A peak over 0 dBFS is cut off on the way to your speakers, and in a 16- or "
                   "24-bit WAV file, however clean each track was. Tracks, buses and even the sum coming into the master can go "
                   "over 0 dB inside DawIt without distorting; what comes out of the master, after its fader, cannot. (A 32-bit "
                   "float WAV keeps peaks over 0 dB, so they can still be turned down later; any 16- or 24-bit copy of it clips.)\n\n"
                   "Headroom is the distance between the loudest peak and 0 dBFS. A common aim before mastering is peaks around "
                   "-6 dBFS on the master; -6 to -3 dB is the usual range, as in the Gain staging guide. A mix that peaks at -6 dB has "
                   "6 dB of headroom: the mastering step has room for its own EQ and compression, and nothing has clipped on the way. "
                   "A mix that already peaks at -0.5 dB has almost nothing left.\n\n"
                   "Typical settings for a bus compressor on the mix bus: " + describeMixBusSettings() + ". A ratio of 1.5:1 to 2:1 "
                   "is glue, not squashing. An attack of 10 to 30 ms lets the drums keep their punch. A release of 100 to 300 ms, or "
                   "fitted to the tempo, lets the reduction fall back between the beats. The sidechain filter takes the lows out of "
                   "what the compressor listens to, so the kick and bass don't make the whole mix duck on every beat. Engineers "
                   "differ on all of these; treat them as a place to start.\n\n"
                   "What to watch out for:\n"
                   "- A limiter on the master while you mix hides a bad balance and uses up the headroom that mastering needs. Most "
                   "advice is to mix without it and add it at the end. (Some engineers, in loud styles, mix into one to hear the "
                   "final sound, and bounce a copy without it for mastering.)\n"
                   "- Don't fix a bad balance with the mix bus. If the vocal is buried, turn up the vocal. Compression on the whole "
                   "mix only glues what is already balanced.\n"
                   "- The order of work: balance the tracks first, group them in buses, then add a little glue or EQ to the whole. "
                   "Some engineers put a gentle bus compressor on early and mix into it instead, so they judge the balance with it; "
                   "either way, keep it gentle.\n\n"
                   "In DawIt today the master has a volume fader (in the mixer and in the transport bar) and a peak meter. It has "
                   "no effects chain yet, so the glue and the limiter can't go on the master itself; put a gentle compressor on "
                   "your buses, and keep the master's peaks below about -6 dB with the tracks' and buses' faders (or, if you must, "
                   "the master fader). A master with effects, like a track, is an idea on the list.";

        case Topic::typical:
            return "Most mixes use the same few groups.\n\n"
                   "Drums: the kit in one bus, for glue and one fader. Vocals: for one shared tone and level. Backing vocals as a bus "
                   "inside the vocals, so they sit under the lead with one fader. Doubled guitars, left and right, on one fader. "
                   "Keys and pads together.\n\n"
                   "Not every track needs a bus: one bass track goes straight to the master. And two levels of buses (backing "
                   "vocals inside vocals) is plenty for most songs; more makes it hard to see where a sound goes. Big productions "
                   "use more, but they have many more tracks to sort.\n\n"
                   "The picture shows a pop song set up this way, with the tracks coloured as in the arrangement.";

        case Topic::sends:
            return "A send is a copy of a track's sound, at a level of its own, that goes to a return track. The return holds an effect, "
                   "usually a reverb or a delay, and plays into the master next to the tracks. The track itself still plays as before: "
                   "the send only adds to it.\n\n"
                   "Why it's done:\n"
                   "- One room for the whole band. When the vocal, the guitar and the pad send to the same reverb, they sound as if they "
                   "were played in the same space. Five tracks with five different reverbs can sound like five different rooms.\n"
                   "- Set it once. Change the reverb's decay on the return and every track that sends to it follows.\n"
                   "- Less work for the computer. One reverb runs once, however many tracks send to it.\n"
                   "- More or less per track. Each track has its own send: a lot of reverb on the pad, a little on the vocal, none on "
                   "the bass.\n\n"
                   "In the picture, Vocal sends to Reverb at -12 dB, Guitar at -9 dB and Pad at -1 dB, and Bass doesn't send at all. "
                   "Sends are the dashed lines; the solid lines are where each track's own sound goes.\n\n"
                   "A return is not a bus. Tracks in a bus play into it instead of the master: the bus takes their whole sound. A "
                   "return takes a copy, and the tracks still go their own way.\n\n"
                   "The words vary. A send is also called an aux send (short for auxiliary). The track it goes to is a return track in "
                   "Ableton Live and DawIt, an aux in Logic Pro, an Aux Input track in Pro Tools and an FX channel in Cubase. On a "
                   "mixing desk, every channel has send knobs, and the effect comes back on a return input.";

        case Topic::sendHowTo:
            return "1. Choose Track > New Return Track > Reverb, or click + Return in the transport bar. A return track called Reverb "
                   "appears at the bottom of the song, with DawIt Reverb on it at 100% mix.\n\n"
                   "2. Open the mixer (Cmd+2). Every strip now has a Reverb knob under its pan. Drag it up on the tracks that should "
                   "have reverb; double-click it to turn it off. A send you switch on from a menu starts at -10 dB, about as much "
                   "reverb as a 20% mix on the track itself. The inspector (View > Inspector, I) shows the same knobs for the "
                   "selected track, one for every return, with their full names.\n\n"
                   "3. Play the song and set the amounts by ear. Then shape the reverb once, on the return: select it, and open DawIt "
                   "Reverb in the device bar.\n\n"
                   "A track's right-click menu has Sends, to turn the send to each return on or off, and New return track. In the "
                   "device bar, a track's sends branch off its chain with their levels: drag one up or down to change it, or click it "
                   "to see the return's chain. A return's chain starts with the tracks that send to it.\n\n"
                   "A return plays into the master. It can't hold tracks, but you can drag it up or down. Deleting a return removes the "
                   "sends to it. Adding a return doesn't change any track's sound until you turn up its send.";

        case Topic::inspector:
            return "The inspector is the panel on the left of the track headers (View > Inspector, or press I). It shows one track at a "
                   "time: the one you clicked last. Click another header, or a strip in the mixer, and it follows. With nothing "
                   "selected, it shows the master.\n\n"
                   "From top to bottom it follows the sound:\n"
                   "- Effects: the instrument first on a MIDI track, then the effects in the order the sound goes through them. Click one "
                   "to open it, click its dot to switch it off (bypass), and drag it up or down to change the order. + Effect adds one at "
                   "the end.\n"
                   "- Sends: a row for every return in the song, with the return's full name. Drag a knob up to send a copy of the track "
                   "to that return, and double-click it to turn the send off. PRE means the copy is taken before the fader.\n"
                   "- Pan: where the track sits between the left and the right speaker.\n"
                   "- Volume: the track's fader, with its meter beside it.\n\n"
                   "It shows the track itself, not a copy. Turn the Reverb send up in the inspector and the Reverb knob in the mixer turns "
                   "with it. Use whichever is closer: the inspector while you work on the arrangement, the mixer (Cmd+2) to compare all "
                   "the tracks side by side.\n\n"
                   "Why are the sends a list of their own, and not with the effects? Everything in the Effects list is an insert: the "
                   "whole sound goes through it. A send leaves the track's sound alone and adds a copy to a return, where a shared reverb "
                   "or delay usually sits (see Insert or send?). So a vocal with an EQ and a compressor in its Effects list and a send of "
                   "-12 dB to Reverb stays fully compressed, with the reverb added beside it.\n\n"
                   "Things to know:\n"
                   "- It changes one track. With several tracks selected, it shows the one you clicked last, and only that one changes. "
                   "To change several at once, use the mixer or the track menu's Sends.\n"
                   "- A return has no sends: it plays into the master only. Its inspector lists the tracks that send to it instead.\n"
                   "- The master has a volume and a meter here, but no pan, effects or sends.\n"
                   "- Muting a track with M silences its sends too.\n"
                   "- Other programs have the same panel. Logic Pro and Cubase call it the inspector, and in Logic Pro the I key opens it "
                   "too. Logic Pro also shows, beside the track, the strip it plays into.";

        case Topic::insertOrSend:
            return "An insert effect sits in the track's own chain, so the whole sound goes through it: what comes out replaces what "
                   "went in. A send effect sits on a return, so it gets a copy, and its result is added to the untouched sound.\n\n"
                   "That gives a rule of thumb:\n"
                   "- Effects that change the sound itself go on the track: EQ, compression, gates, saturation and distortion. You want "
                   "all of the vocal compressed, not a compressed copy next to the original.\n"
                   "- Effects that add something around the sound go on a return: reverb and delay, and sometimes chorus.\n\n"
                   "On a return, set the effect to 100% wet: DawIt Reverb's or DawIt Delay's Mix at 100%, as their Return starting "
                   "points do. Then the return carries only the effect, and each send alone decides how much of it a track gets. At a "
                   "lower mix, the return also adds a dry copy of every track that sends to it, which makes them louder and changes the "
                   "balance.\n\n"
                   "For one track, an insert at a low mix and a send do the same job. DawIt Reverb at 20% mix puts its reverb 9.8 dB "
                   "under the dry sound. A send at -9.8 dB to the same reverb at 100% on a return, with the return's fader at 0 dB, "
                   "gives the same balance. Use an insert when only one track needs the effect, or for something special: a huge reverb "
                   "on one word, a slapback on one guitar. Use a return when several tracks should share it. Engineers work both ways, "
                   "but a return is the usual home of a song's main reverb.\n\n"
                   "EQ and compression can also go on a return, after the reverb. A low cut at 150-300 Hz on the reverb return keeps "
                   "the low end clear for every track at once.\n\n"
                   "Parallel compression (a squashed copy blended under the dry sound) can also be done with a send to a return with a "
                   "compressor. In DawIt, the Mix knob of DawIt Compressor, Saturation or Distortion is simpler. For Saturation and "
                   "Distortion it is also safer: their oversampling delays the sound slightly, DawIt doesn't line up such delays yet, "
                   "and a late copy blended with the dry sound can sound hollow.";

        case Topic::depth:
            return "In a room, the sound that comes straight from an instrument gets quieter the further away you are: about 6 dB "
                   "quieter each time the distance doubles. The reverb, the sound bouncing around the room, stays roughly as loud "
                   "wherever you stand. So the further away a sound is, the more reverb you hear compared with its direct sound. The "
                   "ear uses this to judge distance, and a mix can use it too.\n\n"
                   "With one shared reverb, the send level works as a distance knob. The picture places the parts by their sends:\n"
                   "- lead vocal -12.4 dB: up front;\n"
                   "- drums -9.8 dB, guitar -8.9 dB, piano -7.7 dB: the band, behind the singer;\n"
                   "- strings -4.3 dB and pad -1.4 dB: far back.\n"
                   "These are the Reverb guide's mixes (15%, 20%, 22%, 25%, 35% and 45%) turned into the send levels that give the same "
                   "amount of reverb.\n\n"
                   "Reverb isn't the only clue. Closer sounds are also louder, brighter and sharper at the start, so a part far back "
                   "in the mix is usually a little quieter and darker too. Pre-delay works the other way: a longer pre-delay keeps a "
                   "sound close even with plenty of reverb, because its start is heard dry first (see the Reverb guide).\n\n"
                   "When everything is far away, the mix gets muddy. Pick what should be up front, usually the lead vocal and one main "
                   "instrument, keep their sends low, and let the rest sit behind. To check, mute the return: the mix should get closer "
                   "and drier, not fall apart.";

        case Topic::preFader:
            return "A send can take its copy at two places in the track's chain:\n"
                   "- Post-fader, after the track's fader: the usual choice, and DawIt's default. Turn the track down 6 dB and its send "
                   "goes down 6 dB too. The track keeps the same amount of reverb, just quieter. That's almost always what you want: "
                   "when you turn a vocal up or down, its reverb follows.\n"
                   "- Pre-fader, before the fader. The send ignores the fader. Turn the track down 6 dB and the reverb stays as loud, so "
                   "a send of -10 dB now gives reverb only 4 dB under the dry sound. Pull the fader all the way down and only the "
                   "reverb is left.\n\n"
                   "What pre-fader is for: in studios, the musicians' headphone mixes, which must not change when the engineer moves a "
                   "fader. In a mix it's an effect: a sound that fades into its own reverb, or a reverb that keeps one level while the "
                   "track moves.\n\n"
                   "In DawIt, both kinds are taken after the track's effects and before its pan. A guitar panned hard left still sends "
                   "its whole sound, so its reverb fills the space around it. DawIt Reverb listens to left and right added together, so "
                   "a send taken after the pan would give that guitar 6 dB less reverb than a centred one. The price is that its echoes "
                   "on a delay return come from the middle. Programs differ here; Logic Pro, for one, also offers a send after the pan.\n\n"
                   "Right-click a send knob in the mixer or the inspector, or a send in the device bar, and choose Pre-fader or Post-fader. A pre-fader "
                   "send shows PRE in yellow.\n\n"
                   "In DawIt, muting a track also silences its sends, pre-fader ones included. Some programs, such as Ableton Live, "
                   "keep pre-fader sends playing while a track is off, for those headphone mixes.";

        case Topic::solo:
            return "Soloing a track in a bus keeps the way to the speakers open: you hear the track through its bus, with the bus's "
                   "effects and fader, but the bus's other tracks are silent. Soloing a bus plays all its tracks. Muting a bus "
                   "silences every track in it; their own meters still move, because they still play into the bus, and their "
                   "headers say BUS MUTED.\n\n"
                   "The rule: while anything is soloed, a track is heard when it, a bus it plays into, or a track that plays into it "
                   "is soloed. Mute always wins: a soloed track in a muted bus stays silent.\n\n"
                   "Returns follow the same idea. While a track is soloed, the returns it sends to stay on, so you hear a soloed vocal "
                   "with its reverb. Soloing a return plays only the return: the other tracks go quiet but keep sending, so you hear the "
                   "reverb on its own, which is a good way to hear what it does. Muting a return takes its effect out of the mix, and "
                   "the tracks play on, dry. Muting a track also silences its sends.\n\n"
                   "Why: you hear a soloed track as it sounds in the mix, through its bus's compressor, not on its own. The picture "
                   "dims the boxes you can't hear.";

        case Topic::signalFlow:
            return "Signal flow is the route a sound takes from where it starts to your speakers, and the order of the steps on the way. The Signal Flow window (View > Signal Flow..., Cmd+6) draws it for your whole song; the picture above is a small example.\n\n"
                   "Read it from left to right. Each track starts with its source: its audio clips, or the instrument that plays its MIDI notes. Then come its effects, one after the other, then its fader. From there the line goes into a bus, where it meets the other tracks of the group, or straight into the master on the right. A send branches off as a dashed line to a return.\n\n"
                   "Each effect hears what the one before it made. An EQ that cuts 6 dB at 200 Hz hands the compressor after it a sound with less low end, and the compressor reacts to that. Swap them, and the compressor reacts to the full low end while the EQ only shapes what comes out. Two EQs, or an EQ and a fader, give the same sound in either order, because neither reacts to the level. Order matters most for the effects that do: compressors, gates, saturation and distortion.\n\n"
                   "A worked example: a compressor at 4:1 with its threshold at -30 dB. Turn a -10 dB sound up 6 dB before it, and it comes out at -23.5 dB. Turn it up 6 dB after the compressor instead, and it comes out at -19 dB. The same 6 dB makes a 4.5 dB difference, just from where it sits.\n\n"
                   "Orders most engineers start from:\n"
                   "- A gate first, before compression. A compressor narrows the gap between the notes and the quiet noise between them, which makes the gate harder to set. Gated reverb is the exception: the gate comes after the reverb to cut its tail short.\n"
                   "- EQ and compression in either order. EQ before the compressor changes what it reacts to: cut the boomy lows and it stops pumping on them. EQ after it changes the tone without changing how it works. Many engineers do both, a cut before and a gentle boost after. Neither is wrong.\n"
                   "- Saturation and distortion before delay and reverb, as on a guitarist's pedalboard. After them, the echoes and the reverb's tail are distorted too and smear together, unless that's the sound you want.\n"
                   "- Reverb last, after EQ, compression, distortion and delay. A compressor after a reverb squeezes the reverb too: between the notes the tail comes up, and the sound can pump.\n"
                   "These are starting points, not rules. Try the other order and listen.\n\n"
                   "A send is a branch, not a detour. The track's own line carries on unchanged to its bus or the master; the send adds a copy to a return. In DawIt it's taken after the effects, after the fader (post-fader) or before it (pre-fader), and always before the pan.\n\n"
                   "A bus adds up its lines, so the line coming out of a bus is usually thicker than the lines going in. Two different sounds at -12 dB and -14 dB add up to about -9.9 dB; two copies of the same sound, in step, come out 6 dB louder than one. The master is the last sum of all.\n\n"
                   "In the Signal Flow window:\n"
                   "- A line's thickness and colour follow the level passing through it, like the meters: grey when quiet (below -24 dB), green when healthy, yellow when hot (above -6 dB), red over 0 dB.\n"
                   "- Red on a line inside the song is a warning, not damage. DawIt calculates with floating-point numbers, so a track or a bus over 0 dB doesn't distort by itself, but effects after it react to the hot level. Only the master's output clips, at your speakers and in a 16- or 24-bit file; its box then shows CLIP.\n"
                   "- Dots run along the lines while the song plays, one step per beat. They show which way the sound goes, not how fast: the computer works out the whole path for one small block of audio at a time, for example 512 samples, which is 10.7 ms at 48 kHz.\n"
                   "- A switched-off (bypassed) effect is grey, and the line goes round it: the sound passes unchanged.\n"
                   "- A muted track's line goes dark after its fader. Its instrument and effects still work, but the sound stops there, and its sends stop too. While a track is soloed, everything that isn't on its path goes dark.\n"
                   "- Click a box to select its track and light up its path.\n"
                   "- The window only shows the song; it doesn't change it. Change things where you always do (the device bar, the mixer, a track's right-click menu), and the picture follows straight away: switch an effect off in the device bar and its box goes grey.";

        case Topic::bounce:
            return "Bouncing turns the sound DawIt makes while it plays into a WAV file: a recording of it. The instruments, effects and faders do their work once, and the result is saved. The word comes from tape, where engineers mixed several tracks down onto a free one: they bounced them.\n\n"
                   "What goes in depends on what you bounce:\n"
                   "- The mix (File > Bounce to WAV..., Cmd+B): every track through its bus and its sends, then the master volume. This is the file you play to other people.\n"
                   "- Selected tracks: select them in the track headers (Shift-click, Cmd-click), then pick Selected tracks in the same window. You get what you hear when you solo them: each one through its buses and the reverb it sends to, then the master volume. Your solo buttons aren't touched.\n"
                   "- One track, as audio in the song: see Bounce in place.\n\n"
                   "The picture shows a bounce of Kick and Snare. The lit boxes are in the file. Kick and Snare still go through the Drums bus, so its compressor and fader shape the file, but Hats and Overheads are left out.\n\n"
                   "A bounce runs offline: DawIt renders as fast as your computer can, usually much faster than the song plays, and plugins are told they may use their best quality. It doesn't change your song. Muted tracks stay silent, and the metronome is left out.\n\n"
                   "Every bounce starts exactly at the start of its range. Two files bounced from the same range line up sample for sample when you place them side by side in another program.\n\n"
                   "The format choices, 44.1 or 48 kHz and 16, 24 or 32-bit float, are in What is sound?, under From air to numbers. One rule matters here: a 16- or 24-bit file can't hold a peak over 0 dB. The loudest moments are cut off (clipped), which sounds harsh. A 32-bit float file keeps them, so they can still be turned down later.";

        case Topic::inPlace:
            return "Bounce in place turns a track, or one clip, into audio on a new track right below it, and mutes the original. Right-click the track's header or a clip and choose Bounce in place, or press Cmd+Shift+B.\n\n"
                   "Why you'd want it:\n"
                   "- To work with the sound as audio: reverse it, split it into stems, convert it to MIDI for another instrument, or cut and move it like a recording.\n"
                   "- To keep a sound you like, such as a synth patch and its effects, safe from a change by accident later.\n"
                   "- To save computer power. Instruments and effects work all the time the song plays; an audio clip only has to be read. Six tracks with heavy synths and reverbs can make playback stutter; bounced, they play without effort.\n\n"
                   "What goes in: the track's clips through its instrument and its effects, up to its fader. What stays live: the fader, the pan, the sends and the bus. The new track gets the original's fader level, pan, sends and bus, and no effects (they are in the audio now), so the song sounds the same. Logic Pro's Bounce in Place works the same way by default: the volume and pan are copied, not printed.\n\n"
                   "The original stays in the song, muted, with its instrument, notes and effects as they were. To go back, unmute it and delete the bounced track. Other programs call this kind of reversible bounce freezing.\n\n"
                   "Things to know:\n"
                   "- The audio starts where the first clip starts and runs on for up to 4 seconds after the last one ends, while a reverb or a released note dies away. It is cut where the sound has fallen below -80 dB.\n"
                   "- A bounce takes the sound as it is now. Later changes to the notes or the effects don't reach the audio; bounce again after changing them.\n"
                   "- Bouncing one clip plays only that clip: a note still ringing from the clip before it is left out.\n"
                   "- A bounce that peaks over 0 dB is turned down to peak at -1 dB, and its clip gain is raised by the same amount: a peak of 2.0 (+6 dB) gets +7.0 dB of clip gain. It sounds the same and saves without clipping.\n"
                   "- Effects that hold the sound back a little, such as DawIt Tune, are allowed for, so the audio lands in time.";

        case Topic::yourSong:
            return "This page looks at your song's tracks, buses and returns, and says what could help. For buses: tracks of one "
                   "kind that could share one, a drum bus with no effects yet, a bus with only one track or none, and buses nested "
                   "deeper than two levels. For returns: the same reverb or delay on several tracks that could share one return, a "
                   "return with no sends or no effect, an effect on a return that isn't at 100% wet, and a lot of reverb on the bass. "
                   "For the order of effects on a track: a compressor, distortion or saturation after a reverb or delay, a gate after "
                   "a compressor or a reverb, and a track with its own reverb that also sends to one. "
                   "And while the song plays: a bus or return whose input adds up to over 0 dB, or a master with less than about 6 dB "
                   "of headroom.\n\n"
                   "It follows your changes, so try a suggestion with the button above the text and see the picture change.";
    }

    return {};
}

juce::String describeSends (const Track* track, const Tracks& tracks)
{
    if (tracks.empty())
        return {};

    auto join = [] (const juce::StringArray& parts)
    {
        return parts.size() <= 1 ? parts.joinIntoString ("") : parts.joinIntoString (", ", 0, parts.size() - 1) + " and " + parts[parts.size() - 1];
    };

    if (track == nullptr)
        return "The master gets every track, bus and return. It has no sends.";

    if (track->isReturn())
    {
        const auto senders = routing::getSenders (*track, tracks);

        if (senders.empty())
            return "Nothing sends to " + track->name + " yet.";

        juce::StringArray parts;

        for (auto* s : senders)
            if (auto* send = routing::findSend (*s, *track))
                parts.add (s->name + " at " + juce::String (send->levelDb.load(), 1) + " dB");

        return track->name + " is fed by " + join (parts) + ".";
    }

    if (routing::getReturns (tracks).empty())
        return "Your song has no return yet. Track > New Return Track > Reverb adds one, and every track gets a knob for it.";

    juce::StringArray parts;

    for (auto& send : track->sends)
        if (send->isOn())
            parts.add (send->target->name + " at " + juce::String (send->levelDb.load(), 1) + " dB, "
                       + (send->preFader.load() ? "before" : "after") + " the fader");

    if (parts.isEmpty())
        return track->name + " doesn't send to a return yet. Drag up a knob under Sends in the inspector to add some reverb.";

    // One return: no list; more: commas, and "and to" before the last.
    juce::String text = track->name + " sends to ";

    for (int i = 0; i < parts.size(); ++i)
        text << (i == 0 ? "" : (i == parts.size() - 1 ? ", and to " : ", to ")) << parts[i];

    return text + ".";
}

float getSendDbForMix (float mixPercent)
{
    if (mixPercent <= 0.0f)
        return sendOffDb;

    const double angle = juce::jlimit (0.0, 1.0, (double) mixPercent / 100.0) * juce::MathConstants<double>::halfPi;
    const double db = 20.0 * std::log10 (std::tan (angle));
    return (float) juce::jlimit ((double) sendOffDb, 6.0, db);
}

float getDryChangeDb (float mixPercent)
{
    const double angle = juce::jlimit (0.0, 1.0, (double) mixPercent / 100.0) * juce::MathConstants<double>::halfPi;
    return (float) (-20.0 * std::log10 (juce::jmax (1.0e-9, std::cos (angle))));
}

float sumDb (const std::vector<float>& levelsDb, bool sameSound)
{
    double sum = 0.0;

    for (auto level : levelsDb)
        sum += std::pow (10.0, (double) level / (sameSound ? 20.0 : 10.0));

    if (sum <= 0.0)
        return -100.0f;

    return (float) ((sameSound ? 20.0 : 10.0) * std::log10 (sum));
}

//==============================================================================
Family getFamily (const Track& track)
{
    if (const auto family = getFamilyFromName (track.name); family != Family::none)
        return family;

    if (track.isMidi())
    {
        if (track.usesBuiltInSynth() && builtInSounds::isPercussive (track.builtInSound.load()))
            return Family::drums;

        for (auto& slot : track.plugins)
            if (slot->isInstrument && slot->instance != nullptr && slot->instance->getName() == BuiltInDrums::identifier)
                return Family::drums;
    }

    return Family::none;
}

juce::String getFamilyName (Family family)
{
    switch (family)
    {
        case Family::none:    return {};
        case Family::drums:   return "Drums";
        case Family::bass:    return "Bass";
        case Family::guitars: return "Guitars";
        case Family::keys:    return "Keys";
        case Family::vocals:  return "Vocals";
    }

    return {};
}

juce::String suggestBusName (const std::vector<Track*>& selected, const Tracks& tracks)
{
    if (! selected.empty())
    {
        const auto family = getFamily (*selected.front());
        const bool same = family != Family::none && std::all_of (selected.begin(), selected.end(), [family] (Track* t) { return getFamily (*t) == family; });
        const auto name = getFamilyName (family);

        if (same && std::none_of (tracks.begin(), tracks.end(), [&name] (auto& t) { return t->isBus() && t->name == name; }))
            return name;
    }

    return nextBusName (tracks);
}

//==============================================================================
std::vector<Node> getExample (Topic topic)
{
    switch (topic)
    {
        case Topic::what:     return makeFirstExample();
        case Topic::why:
        case Topic::howTo:
        case Topic::levels:
        case Topic::glue:     return makeDrumsExample();
        case Topic::mixBus:
        case Topic::typical:
        case Topic::yourSong: return makePopExample();
        case Topic::bounce:
        {
            // Kick and Snare bounced: their path (the Drums bus, the master) is lit and everything else dimmed.
            auto nodes = makePopExample();

            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const bool inFile = nodes[i].name == "Kick" || nodes[i].name == "Snare" || nodes[i].name == "Drums";
                nodes[i].onSelectedPath = inFile;
                nodes[i].silent = ! inFile;
            }

            return nodes;
        }
        case Topic::inPlace:
        {
            // Lead, muted and dimmed, and its bounced copy below it, in one bus, both sending to the same reverb.
            ExampleBuilder b;
            const int synths = b.bus ("Synths", -1, palette (3), -10.0f, -8.0f);
            const int lead = b.track ("Lead", synths, palette (4), -8.0f);
            const int bounced = b.track ("Lead (bounced)", synths, palette (4), -8.0f);
            const int reverb = b.returnTrack ("Reverb", palette (6), -14.0f);
            b.send (lead, reverb, -12.0f);
            b.send (bounced, reverb, -12.0f);
            auto nodes = b.nodes;
            nodes[(size_t) lead].muted = nodes[(size_t) lead].silent = true;
            return nodes;
        }
        case Topic::inspector:
        {
            // Vocal and the Reverb it sends to are the selected path; the other tracks are dimmed.
            auto nodes = makeSendsExample();

            for (auto& n : nodes)
            {
                n.onSelectedPath = n.name == "Vocal" || n.name == "Reverb";
                n.silent = ! n.onSelectedPath;
            }

            return nodes;
        }
        case Topic::sends:
        case Topic::sendHowTo:
        case Topic::insertOrSend:
        case Topic::preFader:
        case Topic::signalFlow: return makeSendsExample();
        case Topic::depth:    return makeDepthExample();
        case Topic::solo:
        {
            // Kick soloed, and a Reverb return that the Snare sends to: it is heard because the soloed track sends to it.
            ExampleBuilder b;
            const int drums = b.bus ("Drums", -1, palette (1), -8.0f, -6.0f);
            const int kick = b.track ("Kick", drums, palette (0), -12.0f);
            const int snare = b.track ("Snare", drums, palette (2), -14.0f);
            b.track ("Hats", drums, palette (3), -18.0f);
            const int reverb = b.returnTrack ("Reverb", palette (6), -14.0f);
            b.send (kick, reverb, -14.0f);
            auto nodes = b.nodes;
            nodes[(size_t) kick].soloed = true;
            nodes[(size_t) snare].silent = nodes[3].silent = true;
            nodes[(size_t) kick].onSelectedPath = nodes[(size_t) drums].onSelectedPath = true;
            nodes[(size_t) reverb].onSelectedPath = true;
            return nodes;
        }
    }

    return {};
}

juce::String getExampleFileTag (Topic topic)
{
    return topic == Topic::bounce ? "Kick + Snare.wav" : juce::String();
}

std::vector<Node> fromSong (const Tracks& tracks, const Track* selected)
{
    const auto rows = routing::getVisibleTracks (tracks);
    std::vector<Node> nodes;

    auto indexInRows = [&rows] (const Track* t)
    {
        for (size_t i = 0; i < rows.size(); ++i)
            if (rows[i] == t)
                return (int) i;

        return -1;
    };

    for (auto* t : rows)
    {
        Node n;
        n.name = t->name;
        n.colour = t->colour;
        n.isBus = t->isBus();
        n.isReturn = t->isReturn();
        n.folded = t->isBus() && t->folded;
        n.muted = t->muted.load();
        n.soloed = t->soloed.load();
        n.silent = routing::getSilence (*t, tracks) != routing::Silence::none;
        n.parent = t->bus != nullptr ? indexInRows (t->bus) : -1;
        n.levelDb = toDb (juce::jmax (t->peakLeft.load(), t->peakRight.load()));
        n.inputDb = t->hasInput() ? toDb (t->busInputLevel.load()) : -100.0f;
        n.track = t;

        for (auto& send : t->sends)
            if (send->isOn())
                if (const int target = indexInRows (send->target); target >= 0)
                    n.sends.push_back ({ target, send->levelDb.load(), send->preFader.load() });

        if (n.folded)
            for (auto& other : tracks)
                if (routing::playsInto (*other, *t))
                    ++n.numInside;

        if (selected != nullptr)
            n.onSelectedPath = t == selected || routing::playsInto (*selected, *t);

        nodes.push_back (std::move (n));
    }

    return nodes;
}

std::vector<juce::Point<float>> layOut (const std::vector<Node>& nodes)
{
    const int count = (int) nodes.size();
    std::vector<std::vector<int>> members ((size_t) count);

    for (int i = 0; i < count; ++i)
        if (nodes[(size_t) i].parent >= 0 && nodes[(size_t) i].parent < count)
            members[(size_t) nodes[(size_t) i].parent].push_back (i);

    std::vector<float> column ((size_t) count, 0.0f), row ((size_t) count, 0.0f);
    float nextRow = 0.0f;

    // Edit order puts a bus before its tracks, so walking backwards sees members first for the columns;
    // the rows are given forwards to the leaves, in order.
    for (int i = 0; i < count; ++i)
        if (members[(size_t) i].empty())
            row[(size_t) i] = nextRow++;

    float lastColumn = 0.0f;

    for (int i = count - 1; i >= 0; --i)
    {
        auto& kids = members[(size_t) i];

        if (nodes[(size_t) i].isBus)
        {
            float maxColumn = 0.0f, minRow = 1.0e9f, maxRow = -1.0e9f;

            for (int k : kids)
            {
                maxColumn = juce::jmax (maxColumn, column[(size_t) k]);
                minRow = juce::jmin (minRow, row[(size_t) k]);
                maxRow = juce::jmax (maxRow, row[(size_t) k]);
            }

            column[(size_t) i] = maxColumn + 1.0f;

            if (! kids.empty())
                row[(size_t) i] = (minRow + maxRow) * 0.5f;
        }

        if (! nodes[(size_t) i].isReturn)
            lastColumn = juce::jmax (lastColumn, column[(size_t) i]);
    }

    // Returns sit in the column before the master, below the other rows (they are last in edit order).
    bool anyReturn = false;

    for (int i = 0; i < count; ++i)
        if (nodes[(size_t) i].isReturn)
        {
            column[(size_t) i] = lastColumn + 1.0f;
            anyReturn = true;
        }

    std::vector<juce::Point<float>> points;

    for (int i = 0; i < count; ++i)
        points.push_back ({ column[(size_t) i], row[(size_t) i] });

    points.push_back ({ lastColumn + (anyReturn ? 2.0f : 1.0f), juce::jmax (0.0f, nextRow - 1.0f) * 0.5f });
    return points;
}

//==============================================================================
juce::String explainSong (const Tracks& tracks)
{
    int numTracks = 0, numBuses = 0, numReturns = 0;

    for (auto& t : tracks)
        (t->isReturn() ? numReturns : t->isBus() ? numBuses : numTracks) += 1;

    if (tracks.empty())
        return "The song has no tracks yet.";

    const auto buses = numBuses == 0 ? juce::String ("no buses") : plural (numBuses, "bus", "buses");
    juce::String text = "The song has " + plural (numTracks, "track", "tracks") + (numReturns == 0 ? " and " : ", ") + buses
                        + (numReturns == 0 ? juce::String() : " and " + plural (numReturns, "return", "returns")) + ".";

    for (auto& t : tracks)
    {
        if (! t->isBus())
            continue;

        juce::StringArray names;

        for (auto* m : routing::getMembers (*t, tracks))
            names.add (m->name);

        text << " " << t->name << (names.isEmpty() ? " is empty." : " takes " + juce::String (names.size()) + " (" + joinNames (names) + ").");
    }

    juce::StringArray straight;

    for (auto& t : tracks)
        if (t->bus == nullptr && ! t->hasInput())
            straight.add (t->name);

    if (! straight.isEmpty())
        text << " " << plural (straight.size(), "track plays", "tracks play") << " straight into the master (" << joinNames (straight) << ").";

    for (auto& t : tracks)
    {
        if (! t->isReturn())
            continue;

        juce::StringArray senders;

        for (auto* sender : routing::getSenders (*t, tracks))
        {
            auto* send = routing::findSend (*sender, *t);
            senders.add (sender->name + " (" + juce::String (send->levelDb.load(), 1) + " dB" + (send->preFader.load() ? ", pre-fader" : "") + ")");
        }

        text << " " << t->name << (senders.isEmpty() ? " is a return with no sends."
                                                     : " is a return: " + joinNames (senders) + (senders.size() == 1 ? " sends" : " send") + " to it.");
    }

    return text;
}

std::vector<Advice> analyse (const Tracks& tracks, const Levels& levels)
{
    std::vector<Advice> advice;

    // Tracks of one kind that play into the same place, and could share a bus.
    struct Group
    {
        Family family = Family::none;
        Track* place = nullptr;
        std::vector<Track*> members;
    };

    std::vector<Group> groups;

    for (auto& t : tracks)
    {
        if (t->hasInput())
            continue;

        const auto family = getFamily (*t);

        if (family == Family::none)
            continue;

        auto it = std::find_if (groups.begin(), groups.end(), [&] (const Group& g) { return g.family == family && g.place == t->bus; });

        if (it == groups.end())
        {
            groups.push_back ({ family, t->bus, {} });
            it = groups.end() - 1;
        }

        it->members.push_back (t.get());
    }

    for (auto& g : groups)
    {
        if (g.members.size() < 2)
            continue;

        // Already in a bus of the same kind, or in a bus the user named for something else: leave it alone.
        if (g.place != nullptr && (getFamily (*g.place) == Family::none || getFamily (*g.place) == g.family))
            continue;

        juce::StringArray names;

        for (auto* m : g.members)
            names.add (m->name);

        // A bus for this kind already exists somewhere else: join it rather than start Bus 1.
        Track* existing = nullptr;

        for (auto& t : tracks)
            if (t->isBus() && t.get() != g.place && getFamily (*t) == g.family)
            {
                existing = t.get();
                break;
            }

        Advice a;
        const juce::String where = g.place == nullptr ? juce::String ("straight into the master") : "into " + g.place->name;

        if (existing != nullptr)
        {
            a.text = joinNames (names) + (g.members.size() == 1 ? " plays " : " play ") + where + ". Move them into " + existing->name + ", the bus you already have for this kind of track: "
                     "they then share its fader and its effects.";
            a.topic = Topic::howTo;
            a.action = Advice::Action::moveIntoBus;
            a.tracks = g.members;
            a.bus = existing;
            a.busName = existing->name;
            advice.push_back (std::move (a));
            continue;
        }

        const auto name = suggestBusName (g.members, tracks);
        const juce::String why = g.family == Family::drums    ? "one fader for the kit, and one compressor to glue it."
                                 : g.family == Family::vocals ? "one fader for the voices, and one EQ for all of them."
                                                              : "one fader and one set of effects for the group.";
        const juce::String busWords = name.startsWith ("Bus ") ? juce::String ("a bus") : "a " + name + " bus";

        // In a bus of another kind: the new bus goes next to that bus, not inside it (a Drums bus inside Vocals would be odd).
        a.text = joinNames (names) + (g.members.size() == 1 ? " plays " : " play ") + where + ". Put them in "
                 + busWords + (g.place != nullptr ? " of their own, next to " + g.place->name + " rather than inside it" : juce::String()) + ": " + why;
        a.topic = Topic::howTo;
        a.action = Advice::Action::createBus;
        a.tracks = g.members;
        a.busName = name;
        a.bus = g.place;
        advice.push_back (std::move (a));
    }

    for (auto& t : tracks)
    {
        if (! t->isBus())
            continue;

        const auto members = routing::getMembers (*t, tracks);

        if (members.empty())
        {
            advice.push_back ({ t->name + " is empty: drag tracks onto it, or delete it.", Topic::howTo, Advice::Action::none, {}, {} });
            continue;
        }

        if (members.size() == 1)
            advice.push_back ({ members.front()->name + " has a bus to itself (" + t->name + "). A bus does most for a group: add other tracks "
                                "to it, or ungroup it.", Topic::why, Advice::Action::none, {}, {} });

        if (getFamilyFromName (t->name) == Family::drums && members.size() >= 2 && t->plugins.empty())
        {
            Advice a;
            a.text = t->name + " has no effects yet. A gentle compressor (" + describeGlueSettings() + ") makes the kit move as one.";
            a.topic = Topic::glue;
            a.action = Advice::Action::addGlue;
            a.tracks = { t.get() };
            advice.push_back (std::move (a));
        }

        if (routing::getDepth (*t) >= 2)
            advice.push_back ({ t->name + " is inside " + plural (routing::getDepth (*t), "other bus", "other buses") + ". Two levels of buses is plenty for most "
                                "songs; more makes it hard to see where a sound goes.", Topic::typical, Advice::Action::none, {}, {} });

        if (levels.playing)
        {
            // Quiet for a bus without effects: nothing on it cares about the level.
            if (const auto it = levels.busInputDb.find (t.get()); it != levels.busInputDb.end() && it->second > 0.05f && ! t->plugins.empty())
                advice.push_back ({ "The tracks in " + t->name + " add up to " + gainstaging::formatDb (it->second) + " peaks. Nothing distorts inside "
                                    "DawIt, but turn them down a little, together, so any effect on the bus gets a healthy level.", Topic::levels, Advice::Action::none, {}, {} });
        }
    }

    // Returns and sends.
    auto findEffect = [] (const Track& t, const juce::String& identifier) -> const PluginSlot*
    {
        for (auto& slot : t.plugins)
            if (slot->instance != nullptr && slot->instance->getName() == identifier && ! slot->bypassed.load())
                return slot.get();

        return nullptr;
    };

    auto getMix = [] (const PluginSlot& slot, const juce::String& identifier) -> float
    {
        if (identifier == BuiltInReverb::identifier)
            if (auto* r = dynamic_cast<BuiltInReverb*> (slot.instance.get()))
                return r->getSettings().mixPercent;

        if (identifier == BuiltInDelay::identifier)
            if (auto* d = dynamic_cast<BuiltInDelay*> (slot.instance.get()))
                return d->getSettings().mixPercent;

        return 100.0f;
    };

    const juce::String reverbId (BuiltInReverb::identifier), delayId (BuiltInDelay::identifier);
    const auto returns = routing::getReturns (tracks);

    // 1. The same effect on several tracks: share it on a return.
    for (auto& identifier : { reverbId, delayId })
    {
        std::vector<Track*> own;

        for (auto& t : tracks)
            if (! t->isReturn())
                if (auto* slot = findEffect (*t, identifier); slot != nullptr && getMix (*slot, identifier) < 99.5f)
                    own.push_back (t.get());

        if (own.size() < 2)
            continue;

        juce::StringArray names;

        for (auto* t : own)
            names.add (t->name);

        Track* existing = nullptr;

        for (auto* r : returns)
            if (findEffect (*r, identifier) != nullptr)
            {
                existing = r;
                break;
            }

        const auto mix = getMix (*findEffect (*own.front(), identifier), identifier);
        const auto detail = own.front()->name + " " + juce::String (getSendDbForMix (mix), 1) + " dB for its " + juce::String (juce::roundToInt (mix)) + "% mix";
        const auto kind = identifier == reverbId ? juce::String ("Reverb") : juce::String ("Delay");

        Advice a;
        a.text = joinNames (names) + " each have their own " + identifier + ". "
                 + (existing != nullptr ? "Send them to " + existing->name + " instead: a send from each at the level that keeps its amount of " + kind.toLowerCase()
                                        : "Share one instead: a " + kind + " return with " + own.front()->name + "'s settings at 100% wet, and a send from each at the level that keeps its amount of "
                                              + kind.toLowerCase())
                 + " (" + detail + "). Their own " + (identifier == reverbId ? "reverbs" : "delays") + " are switched off, not deleted.";
        a.topic = Topic::insertOrSend;
        a.action = Advice::Action::shareEffect;
        a.tracks = own;
        a.bus = existing;
        a.effect = identifier;
        advice.push_back (std::move (a));
    }

    for (auto* r : returns)
    {
        const auto senders = routing::getSenders (*r, tracks);

        // 2. A return nothing sends to.
        if (senders.empty())
            advice.push_back ({ r->name + " has no sends yet. Turn up the " + r->name + " knob on the mixer strips of the tracks that need space.",
                                Topic::sendHowTo, Advice::Action::none, {}, {} });

        // 3. A return with no effect.
        if (r->plugins.empty())
        {
            const auto words = getWords (r->name);
            const bool delay = nameHasWord (words, { "delay", "echo" });
            Advice a;
            a.text = r->name + " is a return with no effect on it. A return usually holds a reverb or a delay, at 100% wet.";
            a.topic = Topic::sendHowTo;
            a.action = Advice::Action::addReturnEffect;
            a.bus = r;
            a.effect = delay ? delayId : reverbId;
            advice.push_back (std::move (a));
        }

        // 4. An effect on a return that isn't all wet.
        for (auto& identifier : { reverbId, delayId })
            if (auto* slot = findEffect (*r, identifier))
                if (const auto mix = getMix (*slot, identifier); mix < 99.5f)
                {
                    Advice a;
                    a.text = identifier + " on " + r->name + " is at " + juce::String (juce::roundToInt (mix)) + "% mix, so the return also adds a dry copy of every track that "
                             "sends to it. On a return, set it to 100%.";
                    a.topic = Topic::insertOrSend;
                    a.action = Advice::Action::makeWet;
                    a.bus = r;
                    a.effect = identifier;
                    advice.push_back (std::move (a));
                }
    }

    for (auto& t : tracks)
    {
        const bool isBassy = getFamily (*t) == Family::bass || getWords (t->name).contains ("kick");

        for (auto& send : t->sends)
        {
            if (! send->isOn())
                continue;

            // 5. Reverb on the bass.
            if (isBassy && send->levelDb.load() > -20.0f && findEffect (*send->target, reverbId) != nullptr)
                advice.push_back ({ t->name + " sends to " + send->target->name + " at " + juce::String (send->levelDb.load(), 1) + " dB. Reverb on bass usually makes "
                                    "the low end muddy: try a lower send, or none.", Topic::depth, Advice::Action::none, {}, {} });

            // 6. A send that ignores the fader.
            if (send->preFader.load())
                advice.push_back ({ t->name + "'s send to " + send->target->name + " is pre-fader, so turning " + t->name + " down leaves its reverb as loud. For a reverb "
                                    "that follows the track, right-click the send and choose Post-fader.", Topic::preFader, Advice::Action::none, {}, {} });
        }
    }

    // 8. The order of effects on a track (only DawIt's own effects that are on; a reverb or delay only at 1% mix or more).
    {
        const juce::String compressorId (BuiltInCompressor::identifier), gateId (BuiltInGate::identifier), distortionId (BuiltInDistortion::identifier),
                           saturationId (BuiltInSaturation::identifier);

        struct Stage
        {
            int index = 0;
            juce::String id;
            const PluginSlot* slot = nullptr;
        };

        auto getStages = [&] (const Track& t)
        {
            std::vector<Stage> stages;

            for (size_t i = 0; i < t.plugins.size(); ++i)
            {
                auto& slot = *t.plugins[i];

                if (slot.instance == nullptr || slot.isInstrument || slot.bypassed.load())
                    continue;

                const auto id = slot.instance->getName();
                const bool timeEffect = id == reverbId || id == delayId;

                if (timeEffect && getMix (slot, id) < 1.0f)
                    continue;

                if (timeEffect || id == compressorId || id == gateId || id == distortionId || id == saturationId)
                    stages.push_back ({ (int) i, id, &slot });
            }

            return stages;
        };

        auto firstEffectIndex = [] (const Track& t)
        {
            for (size_t i = 0; i < t.plugins.size(); ++i)
                if (! t.plugins[i]->isInstrument)
                    return (int) i;

            return 0;
        };

        auto moveAdvice = [&advice] (const Track& t, const juce::String& text, const Stage& moved, int toIndex, const juce::String& buttonText)
        {
            Advice a;
            a.text = text;
            a.topic = Topic::signalFlow;
            a.action = Advice::Action::moveEffect;
            a.tracks = { const_cast<Track*> (&t) };
            a.slot = moved.slot;
            a.toIndex = toIndex;
            a.effect = moved.id;
            a.buttonText = buttonText;
            advice.push_back (std::move (a));
        };

        for (auto& tp : tracks)
        {
            const auto& t = *tp;
            const auto stages = getStages (t);
            const auto name = t.name;

            // The earliest reverb or delay before stage `i` (or null).
            auto timeBefore = [&stages, &reverbId, &delayId] (size_t i, bool reverbOnly) -> const Stage*
            {
                for (size_t k = 0; k < i; ++k)
                    if (stages[k].id == reverbId || (! reverbOnly && stages[k].id == delayId))
                        return &stages[k];

                return nullptr;
            };

            auto compressorBefore = [&stages, &compressorId] (size_t i)
            {
                for (size_t k = 0; k < i; ++k)
                    if (stages[k].id == compressorId)
                        return true;

                return false;
            };

            if (! t.isReturn())
            {
                // 1. A compressor after a reverb or delay.
                for (size_t i = 0; i < stages.size(); ++i)
                    if (stages[i].id == compressorId)
                        if (auto* time = timeBefore (i, false))
                        {
                            const bool isReverb = time->id == reverbId;
                            moveAdvice (t, "On " + name + ", " + compressorId + " comes after " + time->id + ", so it squeezes the "
                                               + (isReverb ? "reverb too: between the notes the tail comes up, and the sound can pump. Usually the reverb goes last. Try the compressor before it."
                                                           : "echoes too: they come up between the notes, and the sound can pump. Usually the delay goes after the compressor."),
                                        stages[i], time->index, "Put " + compressorId + " before " + time->id);
                            break;
                        }

                // 2. A gate after a reverb.
                for (size_t i = 0; i < stages.size(); ++i)
                    if (stages[i].id == gateId)
                        if (auto* time = timeBefore (i, true))
                        {
                            moveAdvice (t, "On " + name + ", " + gateId + " comes after " + time->id + ", so it cuts the reverb's tail short: the gated reverb sound of 1980s drums. "
                                               "Keep it if that's the sound you want; for a natural tail, put the gate first.",
                                        stages[i], firstEffectIndex (t), "Put " + gateId + " first");
                            break;
                        }
            }

            // 3. A gate after a compressor, with no reverb before the gate.
            for (size_t i = 0; i < stages.size(); ++i)
                if (stages[i].id == gateId && compressorBefore (i) && timeBefore (i, true) == nullptr)
                {
                    moveAdvice (t, "On " + name + ", " + gateId + " comes after " + compressorId + ". The compressor narrows the gap between the notes and the noise between them, "
                                       "so the gate is harder to set. A gate usually goes first.",
                                stages[i], firstEffectIndex (t), "Put " + gateId + " first");
                    break;
                }

            if (! t.isReturn())
            {
                // 4. Distortion or saturation after a reverb or delay.
                for (size_t i = 0; i < stages.size(); ++i)
                    if (stages[i].id == distortionId || stages[i].id == saturationId)
                        if (auto* time = timeBefore (i, false))
                        {
                            const bool isReverb = time->id == reverbId, isDistortion = stages[i].id == distortionId;
                            const juce::String what = isReverb ? "reverb's tail" : "echoes";
                            const juce::String effectWord = isDistortion ? "distorted" : "saturated";
                            juce::String text = "On " + name + ", " + stages[i].id + " comes after " + time->id + ", so the " + what + (isReverb ? " is " : " are ") + effectWord + " too";

                            if (isDistortion)
                                text << ", and it all smears together. On a guitarist's pedalboard the distortion comes first. Unless that's the sound you want, try it before the "
                                     << (isReverb ? "reverb." : "delay.");
                            else
                                text << ", which thickens " << (isReverb ? "it" : "them") << " and can blur " << (isReverb ? "it" : "them")
                                     << ". Saturation usually goes before the " << (isReverb ? "reverb." : "delay.");

                            moveAdvice (t, text, stages[i], time->index, "Put " + stages[i].id + " before " + time->id);
                            break;
                        }

                // 5. Two reverbs: its own, and a send to a return that has one.
                const PluginSlot* own = nullptr;

                for (auto& st : stages)
                    if (st.id == reverbId)
                        own = st.slot;

                if (own != nullptr)
                    for (auto& send : t.sends)
                    {
                        if (! send->isOn() || ! send->target->isReturn())
                            continue;

                        const auto returnStages = getStages (*send->target);

                        if (std::any_of (returnStages.begin(), returnStages.end(), [&reverbId] (auto& st) { return st.id == reverbId; }))
                        {
                            advice.push_back ({ name + " has its own " + reverbId + " and also sends to " + send->target->name + ", so it gets two reverbs. That can be on purpose "
                                                "(a short room on the track, a long hall on the return), but usually one is enough: switch one off and compare.",
                                                Topic::signalFlow, Advice::Action::none, {}, {} });
                            break;
                        }
                    }
            }
        }
    }

    // 7. A hot return, while playing.
    if (levels.playing)
        for (auto* r : returns)
            if (const auto it = levels.busInputDb.find (r); it != levels.busInputDb.end() && it->second > 0.05f && ! r->plugins.empty())
                advice.push_back ({ "The sends into " + r->name + " add up to " + gainstaging::formatDb (it->second) + " peaks. Nothing distorts inside DawIt, but turn the "
                                    "sends down a little, together, so the effect on the return gets a healthy level.", Topic::levels, Advice::Action::none, {}, {} });

    const bool anyBus = std::any_of (tracks.begin(), tracks.end(), [] (auto& t) { return t->hasInput(); });

    if (advice.empty() && ! anyBus && ! tracks.empty())
        advice.push_back ({ "Every track goes straight to the master. That's fine for a small song; buses help once there are several "
                            "drum or vocal tracks.", Topic::what, Advice::Action::none, {}, {} });

    if (levels.playing && ! tracks.empty() && levels.masterPeakDb > -99.0f && levels.masterPeakDb > mixBusTargetPeakDb)
    {
        const bool clips = levels.masterPeakDb > 0.05f;
        advice.push_back ({ clips ? "The master peaks at " + gainstaging::formatDb (levels.masterPeakDb) + ": it clips at your speakers and in a 16- or 24-bit "
                                    "bounce. Turn the loudest tracks and buses down (or the master fader) until the peaks stay below about -6 dB."
                                  : "The master peaks at " + gainstaging::formatDb (levels.masterPeakDb) + ", which leaves "
                                    + juce::String (getHeadroomDb (levels.masterPeakDb), 1) + " dB of headroom. Before mastering, about 6 dB is the usual aim: "
                                    "turn the loudest tracks and buses down a little.",
                            Topic::mixBus, Advice::Action::none, {}, {} });
    }

    return advice;
}
} // namespace daw::routingguide
