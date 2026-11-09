#include "HandoverGuide.h"

#include <algorithm>
#include <map>
#include <set>

#include "RecordingGuide.h"
#include "RoutingGuide.h"
#include "Routing.h"

namespace daw::handoverguide
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

    /** "+1.4" or "-1.2": the sign always shown, one decimal. */
    juce::String formatPeak (float db)
    {
        return (db > 0.0f ? "+" : "") + juce::String (db, 1);
    }

    /** "44.1", "48", "96": kilohertz without a needless ".0". */
    juce::String formatKhz (double rate)
    {
        auto s = juce::String (rate / 1000.0, 1);
        return s.endsWith (".0") ? s.dropLastCharacters (2) : s;
    }

    juce::String clipCount (int n)
    {
        return juce::String (n) + (n == 1 ? " clip" : " clips");
    }
} // namespace

//==============================================================================
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::stems:       return "Stems, multitracks and the mix";
        case Topic::mixOrMaster: return "Mixing or mastering?";
        case Topic::preparing:   return "Preparing the files";
        case Topic::headroom:    return "Headroom and the master";
        case Topic::effects:     return "Effects: printed or dry";
        case Topic::checking:    return "Checking before you send";
        case Topic::sendWith:    return "What to send with them";
        case Topic::working:     return "Working with an engineer";
        case Topic::yourSong:    return "Your song's stems";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::stems:       return "Three ways to hand over a song";
        case Topic::mixOrMaster: return "Who wants which files";
        case Topic::preparing:   return "Same start, same length, 24-bit, no dither";
        case Topic::headroom:    return "Peaks near -6 dBFS, no limiter";
        case Topic::effects:     return "What goes into the files";
        case Topic::checking:    return "Do the files add up to your mix?";
        case Topic::sendWith:    return "Names, a rough mix, references and notes";
        case Topic::working:     return "Revisions, and keeping your own copy";
        case Topic::yourSong:    return "What your song would send";
    }

    return {};
}

juce::String explain (Topic topic)
{
    switch (topic)
    {
        case Topic::stems:
            return "When someone else mixes or masters your song, they don't open your DawIt project. Their program can't play your instruments and effects, so you send audio files instead. There are three kinds.\n\n"
                   "The mix is one stereo WAV file: the whole song as you hear it. It's what a mastering engineer works on, and what you play to friends.\n\n"
                   "Multitracks are one file per track: Kick, Snare, Bass, Lead vocal, each on its own. The song in the picture has 13 tracks, so it makes 13 files. A mixing engineer works from these, because mixing means setting the level, tone and space of every part.\n\n"
                   "Stems are one file per group: all the drums in one file, then the bass, the guitars, the keys and the vocals. The song in the picture makes 5. In DawIt a stem is each bus at the top level, plus each track that isn't in a bus, so your buses decide the stems. Backing vocals sits inside the Vocals bus, so it is part of the Vocals stem.\n\n"
                   "People use the word loosely. Many musicians say \"stems\" when they mean multitracks. When an engineer asks for stems, ask which they mean: every track, or the groups.\n\n"
                   "Every file covers the same stretch of the song, from the same start to the same end. Put them side by side at 0 dB in any program and they play back your mix (Checking before you send says when they don't quite).\n\n"
                   "All three come from File > Bounce to WAV... (Cmd+B): under What, pick The mix, Every track (multitracks) or Every bus and lone track (stems). This guide says which to choose and how to prepare them. How DawIt makes each file is in the routing guide, under Stems and multitracks.";

        case Topic::mixOrMaster:
            return "Mixing and mastering are two different jobs, and they want different files.\n\n"
                   "A mixing engineer balances the parts: levels, EQ, compression, panning, reverb. For that they need every part on its own, so send multitracks, one file per track. Some like the group stems as well, to hear how your groups sound.\n\n"
                   "Send the files as you hear them (with your effects), dry (only the clips and the instrument), or both. Engineers differ, so ask. Many like your effects when they are part of the sound, such as a distorted guitar or a synth's delay. Some want everything dry so they can choose again.\n\n"
                   "A mastering engineer works on the finished mix: the overall tone, the loudness and the last polish, so the song sounds right on every system and next to other songs. Send one stereo file, the mix, prepared as in Headroom and the master. This file is often called the premaster.\n\n"
                   "Sometimes a mastering engineer works from stems instead, which is called stem mastering. It helps when the mix is nearly right but one group needs a change that can't be made on the whole mix, such as a vocal a little too quiet or a bass that booms. They usually want a handful of stems: from 2 (the vocals and everything else) to about 8 (drums, bass, guitars, keys, lead vocal, backing vocals and so on). Send stems only when they ask. Many files turn mastering back into mixing.\n\n"
                   "Not sure? Ask before you bounce. A one-line email (\"Multitracks, wet or dry? 24-bit at 48 kHz?\") saves sending everything twice.\n\n"
                   "In short:\n"
                   "- Mixing: multitracks, as you hear them, dry, or both, with a rough mix (see What to send with them).\n"
                   "- Mastering: the mix, as one stereo WAV file. Stems only when asked.";

        case Topic::preparing:
            return "An engineer should be able to drop your files into a new session, press play and hear your song. A few rules make that work.\n\n"
                   "Same start. Every file starts at the same moment, usually bar 1, even a guitar that only plays in the chorus: its file is silent until the chorus comes. The engineer lines them all up at the start and nothing is out of time. A file that begins where its part begins has to be placed by hand, at exactly the right spot. Silence at the start of a file is fine; a file in the wrong place is not.\n\n"
                   "Same length. Every file also ends at the same moment, with room after the last note for a reverb or a held note to die away. DawIt adds 4 seconds after the end of the range to every file. The song in the picture is 105 bars at 120 BPM, 3:30, so every file is 3:34 long.\n\n"
                   "WAV, 24-bit. A WAV file keeps every sample as it is. An MP3 throws some of the sound away, so it isn't used for mixing or mastering. 24-bit is what most engineers ask for: its noise floor is so far down you never hear it. 32-bit float is fine too, and keeps peaks over 0 dB that a 24-bit file would cut off. Leave 16-bit for the finished master.\n\n"
                   "The project's sample rate. Send the files at the rate your song was made at: if your recordings are 48 kHz, send 48 kHz. Converting on the way out changes every sample and gains nothing. Bounce to WAV offers 44.1 kHz and 48 kHz; Your song's stems shows which rate your recordings use.\n\n"
                   "No dither. Dither is a very quiet noise added when a file loses bits, such as 24-bit to 16-bit, to hide the rounding. It belongs to the very last step, once, when the mastering engineer makes the final file. A 24-bit or 32-bit float file for an engineer needs none, and DawIt's Bounce to WAV never adds it.\n\n"
                   "Not normalised. Normalising turns a file up until its loudest peak reaches a set level. Done to stems, it turns each file up by a different amount, and your balance is gone: the quiet shaker comes back as loud as the snare. Send the files as they come out.\n\n"
                   "Nothing extra on the master. DawIt has no effects on the master, so the files hold your tracks, buses, returns and the master volume, and nothing more. A compressor or limiter on a bus that holds the whole song is in the files; see Headroom and the master.";

        case Topic::headroom:
            return "Headroom is the space between your loudest peak and 0 dBFS, the top of a digital file. A mix that peaks at -6 dBFS has 6 dB of headroom.\n\n"
                   "For a mastering engineer, leave some. Mastering EQ and compression change the peaks, and the engineer wants to set the final loudness themselves. Peaks around -6 dBFS are the usual target, as in the Gain staging guide. There is no single rule, though: many engineers are happy with -3 or -4 dBFS as long as nothing clips, some online mastering services ask for -2 to -4, and a few engineers want more room, such as -10. If yours names a number, use it. A quieter mix is fine too: a 24-bit file has plenty of room below.\n\n"
                   "No limiter for loudness. A limiter that makes your mix loud on the way out leaves the mastering engineer little to work with: the peaks are already flattened and can't be brought back. Bypass it before you bounce the premaster. If you like how loud it sounds with it, bounce a second copy with the limiter on and send it as a reference: \"this is roughly how loud I hear it\". A gentle compressor that is part of your mix's sound, holding it together with 1 or 2 dB of gain reduction, can usually stay. Say in your notes that it's there.\n\n"
                   "How to get there. File > Measure Loudness... shows the song's peak. If it peaks at -1.2 dBFS, turn the master volume down 4.8 dB. Nothing comes after the master volume in DawIt, so the peak drops by exactly that much, to -6.0 dBFS. The mix sounds the same, only quieter.\n\n"
                   "Over 0 dB? A 16- or 24-bit file cuts off anything over 0 dBFS, and that clipping is in the file for good. Bounce to WAV's report shows the peak and any file that went over.\n\n"
                   "Stems and multitracks for a mixing engineer need no headroom target of their own. Each is quieter than the whole mix, and the engineer sets the levels again. Just make sure none of them clips.";

        case Topic::effects:
            return "Printing an effect means bouncing it into the audio. A printed effect can't be changed later, but it sounds exactly as you meant. Left out, it can be set up again, but maybe not quite as you had it.\n\n"
                   "As you hear it. DawIt's stems and multitracks are printed unless you choose otherwise: each file is what you hear when you solo that track or bus, with its effects, the bus it plays into, its share of the reverb and delay on the returns, its fader and pan. That's the safe choice: what you send is what you heard.\n\n"
                   "Dry. Under Sound, Dry gives multitracks with only the clips and the instrument: no effects, the fader at 0 dB, the pan in the middle and no reverb. The engineer makes all those choices again. A synth's sound comes from its instrument, so it is in the dry file; only the effects after it are left out.\n\n"
                   "Some effects are part of the sound and are best printed, even when the rest goes dry:\n"
                   "- Effects that move with the beat, such as a delay set to an eighth note, a step filter or a slicer. Rebuilt in another program, the pattern may not land the same.\n"
                   "- Reverb and delay on returns. As you hear it, each file carries its own share of the return: the vocal's reverb is in the vocal file. Some engineers like the reverb as a file of its own, so they can set its level; DawIt can't make that yet.\n"
                   "- Parallel processing, where a copy of a track goes through heavy compression or distortion and is blended back in. Print the blend.\n"
                   "- Ducking, or sidechain compression: a bass that dips each time the kick plays. In programs that have it, a compressor on the bass listens to the kick. Print it into the bass, or the engineer has to set it up again.\n"
                   "- Pitch correction and harmonies. DawIt Tune and DawIt Harmoniser change the notes themselves. Send them printed, and the raw vocal as well if the engineer wants to tune it their own way.\n\n"
                   "When in doubt, send both: Bounce twice, once As you hear it and once Dry, each into its own clearly named folder, such as My song (as heard) and My song (dry). The files keep the same names, so the folder says which is which.";

        case Topic::checking:
            return "Before you send, check that the files really are your song.\n\n"
                   "Do they add up? Put the files on tracks in a new project, all starting at bar 1, faders at 0 dB, pans in the middle, no effects. Played together they should sound exactly like your mix. Switch between them and a bounce of the mix: you shouldn't hear a difference.\n\n"
                   "Engineers check this precisely with a null test. Flip the polarity of the mix (turn its wave upside down) and play it with the files. Where the two are the same they cancel to silence; whatever is left is the difference. Files of a song with EQ, faders and pan cancel to silence, or to a hiss far too quiet to hear.\n\n"
                   "Why the files don't always sound like your mix:\n"
                   "- A compressor, gate, saturation or distortion on a bus or a return. It reacts to everything in it at once. Kick alone pushes the drum bus compressor less than the whole kit does, so the drum multitracks added up are a little louder and less squashed than in your mix. The Drums stem keeps the whole bus in one file, so it is exactly right. Send the stem as well, or say which compressor you used. Your song's stems names any such bus.\n"
                   "- An effect that follows the pitch on a bus or return, such as DawIt Tune or DawIt Harmoniser: it hears all the notes at once.\n"
                   "- A sound with a little chance in it, such as noise in a snare or an oscillator that starts at a random point. Several of DawIt's built-in sounds work this way, as many synths do. Two bounces are never quite the same, so a null test leaves a quiet remainder even though they sound the same.\n"
                   "- Dry files. They aren't meant to add up to your mix: the effects are missing on purpose.\n"
                   "- A file in the wrong place, or faders not at 0 dB in the other program.\n\n"
                   "Then listen to each file:\n"
                   "- At least the start and the end, for a click where a sound is cut off.\n"
                   "- For clipping: Bounce to WAV's report lists any file that went over 0 dB.\n"
                   "- For anything missing: muted tracks and tracks with no clips are left out, and the report says so.\n\n"
                   "Last, open the folder (or the zip) the engineer will get. Are all the files there, with clear names? Bounce to WAV writes them into a new folder, with About these files.txt and Tempo map.mid.";

        case Topic::sendWith:
            return "The files are only half of a hand-over. The engineer also needs to know what the song is and what you want from it.\n\n"
                   "Clear names. A file called Audio 12.wav tells nobody anything. DawIt names each file after its track, numbered in the order of your tracks: 01 Kick.wav, 02 Snare.wav. So name your tracks before you bounce: Lead vocal, not Audio 3. Name the folder after the song, with its tempo and key if you like: Summer Song 120bpm Am.\n\n"
                   "The tempo and key. A WAV file holds no tempo. The engineer needs it to set the grid, time delays to the beat and edit in bars. DawIt writes the tempo, key, sample rate and bit depth into About these files.txt, and the tempo and its changes into Tempo map.mid, which most programs can import.\n\n"
                   "A rough mix. Bounce The mix as well, over the same range, and put it in the folder with a name like Rough mix - do not mix from.wav. It shows how you hear the balance, the effect moments and where the vocal sits. It doesn't need to be perfect.\n\n"
                   "One to three reference songs. Finished songs whose sound you'd like to get close to, each with a line on what you like about it: \"the dry, close vocal\", \"how the kick and bass fit\". More than three tend to pull in different directions.\n\n"
                   "Short notes. What matters most to you, what worries you, and what must not change, ranked, in a few lines: \"1. The vocal up front. 2. The low end feels muddy from bar 25. 3. Keep the guitar delay in the bridge.\" Add the song title, the artist and any deadline.\n\n"
                   "For mastering: the premaster, its sample rate and bit depth, a loud reference if you made one, how the song should end (your fade-out, or one they make), and where it's going (streaming, CD, vinyl).\n\n"
                   "The MIDI, when it helps. Export track as MIDI file... saves a track's notes. An engineer or producer can use them to replace a drum sound, double a bass line or check a chord.";

        case Topic::working:
            return "A mix is a conversation. The engineer sends a first version, you listen and send notes, and they send a revision.\n\n"
                   "How many? A quote for mixing usually includes two or three rounds of changes; more are charged per round. Mastering usually includes fewer. Agree on the number of rounds, the price and the deadline before you start, in writing.\n\n"
                   "Good notes are specific: the time or bar, what you hear, and what you'd like. \"At 1:12 the snare is too loud\" helps; \"make it pop\" doesn't. Before you write them, listen on systems you know, such as your headphones, your car and your phone, and at the same volume each time: a louder version always sounds better at first.\n\n"
                   "A revision changes the mix, not the song. A new vocal take, an extra instrument or a changed arrangement usually means new files, and often a new price.\n\n"
                   "Keep your own copy. Keep the DawIt project as it was when you bounced, and the folder you sent. If the engineer asks for one track again, dry or a little longer, you can make it in a minute with the same settings. Use File > Save As... to save a copy with the date in its name before you carry on working.";

        case Topic::yourSong:
            return "This page looks at your song as the files Bounce to WAV would make. The picture shows your stems as lanes along the song, with the tracks in each, where they play and where their files are silent. Below, the tracks that would be left out, and why.\n\n"
                   "The text counts the files for stems and for multitracks and how much room they take, at the sample rate and bit depth you last chose in Bounce to WAV. It checks the mix's headroom (from the last File > Measure Loudness...), the sample rate of your recordings, track names that say nothing, a bus that holds the whole song, and any bus effect that stops the files adding up to your mix.\n\n"
                   "It follows your changes.";
    }

    return {};
}

//==============================================================================
juce::int64 getBytesPerFile (double sampleRate, int bits, double seconds)
{
    const int bytesPerSample = bits == 16 ? 2 : bits == 24 ? 3 : 4;
    const auto frames = (juce::int64) std::llround (sampleRate * seconds);
    return frames * bytesPerSample * 2;
}

float getTurnDownDb (float peakDb)
{
    return peakDb - routingguide::mixBusTargetPeakDb;
}

bool isGenericName (const juce::String& name)
{
    for (auto prefix : { "Audio ", "MIDI ", "Bus " })
        if (name.startsWith (prefix))
        {
            const auto rest = name.substring (juce::String (prefix).length());
            return rest.isNotEmpty() && rest.containsOnly ("0123456789");
        }

    return false;
}

juce::String describeSeconds (double seconds)
{
    const int total = (int) std::llround (seconds);
    return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
}

juce::String describeRate (double sampleRate)
{
    return formatKhz (sampleRate) + " kHz";
}

//==============================================================================
std::vector<ExampleStem> getExampleStems()
{
    using R = juce::Range<int>;
    return {
        { "Drums",   { "Kick", "Snare", "Hats", "Overheads" }, { R (1, 105) }, "01 Drums.wav" },
        { "Bass",    {}, { R (9, 105) }, "02 Bass.wav" },
        { "Guitars", { "Guitar L", "Guitar R" }, { R (25, 40), R (57, 72), R (89, 105) }, "03 Guitars.wav" },
        { "Keys",    { "Piano", "Pad" }, { R (1, 105) }, "04 Keys.wav" },
        { "Vocals",  { "Lead vocal", "BV 1", "BV 2", "BV 3" }, { R (9, 96) }, "05 Vocals.wav" }
    };
}

double getExampleSecondsPerFile()
{
    return exampleNumBars * 4 * 0.5 + bounce::filesTailSeconds;
}

//==============================================================================
namespace
{
    /** The unbypassed effect on a bus that works on the whole song: DawIt Compressor first, then any plugin with
        "limit" in its name. Empty when there is none. */
    juce::String findWholeSongEffect (const Track& bus)
    {
        juce::String limiter;

        for (auto& slot : bus.plugins)
        {
            if (slot->instance == nullptr || ! slot->isAudioEffect() || slot->bypassed.load())
                continue;

            const auto name = slot->instance->getName();

            if (name == "DawIt Compressor")
                return name;

            if (limiter.isEmpty() && name.containsIgnoreCase ("limit"))
                limiter = name;
        }

        return limiter;
    }
} // namespace

SongView analyse (const Engine& engine, const Inputs& in)
{
    SongView view;
    const auto range = bounce::getSongRange (engine);
    const auto stemsPlan = bounce::planFiles (engine, bounce::FilesKind::stems, false, range, {});
    const auto multiPlan = bounce::planFiles (engine, bounce::FilesKind::multitracks, false, range, {});
    const auto& tempoMap = engine.getTempoMap();
    const auto& tracks = engine.getTracks();

    view.songBeats = range.getLength();
    view.numStems = (int) stemsPlan.files.size();
    view.numMultitracks = (int) multiPlan.files.size();
    view.secondsPerFile = tempoMap.getSecondsBetween (range.getStart(), range.getEnd()) + bounce::filesTailSeconds;
    view.bytesPerFile = getBytesPerFile (in.bounceRate, in.bounceBits, view.secondsPerFile);
    view.bytesStems = view.bytesPerFile * view.numStems;
    view.bytesMultitracks = view.bytesPerFile * view.numMultitracks;

    // The audible tracks: those that make a multitrack file.
    std::set<const Track*> audible;

    for (auto& f : multiPlan.files)
        audible.insert (f.track);

    // Lanes: one per stem, with the tracks inside and where an unmuted clip plays.
    std::vector<int> insideCounts;

    for (auto& f : stemsPlan.files)
    {
        Lane lane;
        lane.name = f.name;
        lane.fileName = f.fileName;
        lane.track = f.track;
        lane.colour = f.track->colour;

        std::vector<const Track*> members;

        if (f.track->isBus())
        {
            for (auto& mp : tracks)
                if (audible.count (mp.get()) != 0 && routing::playsInto (*mp, *f.track))
                {
                    members.push_back (mp.get());
                    lane.tracks.add (mp->name);
                }
        }
        else
        {
            members.push_back (f.track);
        }

        insideCounts.push_back ((int) members.size());
        std::vector<juce::Range<double>> found;

        for (auto* m : members)
            for (auto& c : m->clips)
            {
                if (c->muted)
                    continue;

                const juce::Range<double> r (juce::jmax (range.getStart(), c->startBeat), juce::jmin (range.getEnd(), c->getEndBeat (tempoMap)));

                if (r.getLength() > 1.0e-9)
                    found.push_back (r);
            }

        std::sort (found.begin(), found.end(), [] (auto& a, auto& b) { return a.getStart() < b.getStart(); });

        for (auto& r : found)
        {
            if (! lane.ranges.empty() && r.getStart() <= lane.ranges.back().getEnd() + 1.0e-9)
                lane.ranges.back().setEnd (juce::jmax (lane.ranges.back().getEnd(), r.getEnd()));
            else
                lane.ranges.push_back (r);
        }

        view.lanes.push_back (std::move (lane));
    }

    // Left out: the tracks, then any bus the stems plan left out as well.
    view.skipped = multiPlan.skipped;

    for (auto& s : stemsPlan.skipped)
        if (std::none_of (view.skipped.begin(), view.skipped.end(), [&] (auto& o) { return o.name == s.name; }))
            view.skipped.push_back (s);

    // 1. Summary.
    if (multiPlan.files.empty())
    {
        view.advice.add ("Your song has no clips to bounce yet.");
    }
    else
    {
        juce::StringArray stemNames;

        for (auto& l : view.lanes)
            stemNames.add (l.name);

        view.advice.add ("As stems, your song makes " + juce::String (view.numStems) + (view.numStems == 1 ? " file: " : " files: ") + joinNames (stemNames)
                         + ". As multitracks, " + juce::String (view.numMultitracks) + ". Each file is " + describeSeconds (view.secondsPerFile)
                         + " long (the song plus " + juce::String ((int) bounce::filesTailSeconds) + " seconds), " + recordingguide::describeBytes ((double) view.bytesPerFile)
                         + " at " + describeRate (in.bounceRate) + " and " + (in.bounceBits == 32 ? juce::String ("32-bit float") : juce::String (in.bounceBits) + "-bit")
                         + ": " + recordingguide::describeBytes ((double) view.bytesStems) + " for the stems, "
                         + recordingguide::describeBytes ((double) view.bytesMultitracks) + " for the multitracks.");
    }

    // 2. Left out.
    if (! view.skipped.empty())
    {
        juce::StringArray parts;
        bool anyMuted = false;

        for (auto& s : view.skipped)
        {
            parts.add (s.name + " (" + s.reason + ")");
            anyMuted = anyMuted || s.reason == "muted";
        }

        view.advice.add ("Left out: " + parts.joinIntoString (", ") + "." + (anyMuted ? " Unmute a track if it belongs in the song." : ""));
    }

    if (multiPlan.files.empty())
        return view;

    // 3. Names that say nothing: audible tracks, and the buses of the stems.
    {
        juce::StringArray generic;

        for (auto& f : multiPlan.files)
            if (isGenericName (f.name))
                generic.addIfNotAlreadyThere (f.name);

        for (auto& f : stemsPlan.files)
            if (f.track->isBus() && isGenericName (f.name))
                generic.addIfNotAlreadyThere (f.name);

        if (! generic.isEmpty())
        {
            const juce::String tail = "File names come from track names, so rename ";

            if (generic.size() == 1)
                view.advice.add (generic[0] + " has a name that says nothing. " + tail + "it before you bounce.");
            else if (generic.size() <= 3)
                view.advice.add (joinNames (generic) + " have names that say nothing. " + tail + "them before you bounce.");
            else
                view.advice.add (juce::String (generic.size()) + " tracks have names that say nothing. " + tail + "them before you bounce.");
        }
    }

    // 4 and 5. One bus that holds everything, and its effect on the whole song.
    if (view.numStems == 1 && stemsPlan.files[0].track->isBus() && insideCounts[0] >= 2)
    {
        const auto& bus = *stemsPlan.files[0].track;
        view.advice.add ("Everything plays into the " + bus.name + " bus, so stems make one file: the whole mix. For group stems, bounce each group with Selected tracks, or take the groups out of "
                         + bus.name + ".");

        if (const auto effect = findWholeSongEffect (bus); effect.isNotEmpty())
            view.advice.add (bus.name + " has " + effect + ", which works on the whole song. For a mastering engineer, bypass it if it's there to make the song loud; "
                             "gentle compression of 1 or 2 dB that holds the mix together can stay. Say in your notes that it's there.");
    }

    // 6. Files that won't add up: the routing guide's advice about stems, as it is.
    for (auto& a : routingguide::analyse (tracks, {}))
        if (a.topic == routingguide::Topic::stems)
            view.advice.add (a.text);

    // 7. Headroom.
    if (in.hasPeak)
    {
        const auto p = in.peakDb;
        const auto where = " (last Measure Loudness, " + in.peakScope + ")";
        const auto peak = "Your mix peaks at " + formatPeak (p) + " dBFS" + where;

        if (p > 0.0f)
            view.advice.add (peak + ": over 0 dB, so a 24-bit file would clip. Turn the master volume down " + juce::String (getTurnDownDb (p), 1) + " dB to peak near -6 dBFS.");
        else if (p > -3.0f)
            view.advice.add (peak + ". For a mastering engineer, turn the master volume down " + juce::String (getTurnDownDb (p), 1) + " dB to peak near -6 dBFS.");
        else if (p > -6.0f)
            view.advice.add (peak + ": enough headroom for most mastering engineers; -6 dBFS gives a little more.");
        else
            view.advice.add (peak + ": " + juce::String (routingguide::getHeadroomDb (p), 1) + " dB of headroom, ready for mastering.");
    }
    else
    {
        view.advice.add ("Measure Loudness... (File menu) shows your mix's peak, to check its headroom before mastering.");
    }

    // 8. Sample rate of the recordings.
    {
        std::map<int, int> rates;   // rate in Hz, number of clips

        for (auto* t : audible)
            for (auto& c : t->clips)
                if (! c->muted && c->kind == Clip::Kind::audio)
                    ++rates[(int) std::llround (static_cast<const AudioClip&> (*c).sourceSampleRate)];

        const auto remembered = (int) std::llround (in.bounceRate);
        const auto isOffered = [] (int r) { return r == 44100 || r == 48000; };

        if (rates.empty())
        {
            view.advice.add ("Your song is all MIDI, so 44.1 or 48 kHz are both fine; ask the engineer which they work at.");
        }
        else if (rates.size() == 1)
        {
            const int r = rates.begin()->first;

            if (isOffered (r) && r != remembered)
                view.advice.add ("Your recordings are " + describeRate (r) + ", but Bounce to WAV is set to " + describeRate (remembered) + ". Choose " + describeRate (r) + " so nothing is converted.");
            else if (! isOffered (r))
                view.advice.add ("Your recordings are " + describeRate (r) + ", and Bounce to WAV makes 44.1 or 48 kHz files. Choose one, and tell the engineer the recordings were " + describeRate (r) + ".");
        }
        else
        {
            juce::StringArray parts;
            int best = 0, bestCount = -1;

            for (auto& [r, n] : rates)   // ascending, so a tie goes to the lower rate, except that 48 kHz wins any tie
            {
                parts.add (describeRate (r) + " (" + clipCount (n) + ")");

                if (r != 44100 && r != 48000)
                    continue;   // Bounce only makes these two, so only they can be advised

                if (n > bestCount || (n == bestCount && r == 48000))
                {
                    best = r;
                    bestCount = n;
                }
            }

            if (best == 0)
                view.advice.add ("Your recordings use " + joinNames (parts) + ", and Bounce to WAV makes 44.1 or 48 kHz files. Choose one, and tell the engineer the recordings were these rates.");
            else
                view.advice.add ("Your recordings use " + joinNames (parts) + ". Choose the rate most of them use, " + describeRate (best) + ", and say so in your notes.");
        }
    }

    // 9. Bit depth.
    if (in.bounceBits == 16)
        view.advice.add ("Bounce to WAV is set to 16-bit. Choose 24-bit, or 32-bit float, for files you send to an engineer.");

    // 10. Tempo changes.
    if (! tempoMap.isSteady())
        view.advice.add ("Your song changes tempo. Bounce to WAV puts the changes in Tempo map.mid; mention them in your notes too.");

    return view;
}
} // namespace daw::handoverguide
