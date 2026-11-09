# Stems for a mixing or mastering engineer: what to export and how to hand it over

Status: Released
Idea: [#16 — Stems for a mixing or mastering engineer: what to export and how to hand it over](https://github.com/geertvandamme/dawit/issues/16)

## Goal
A guide, *Sending stems to an engineer*, that walks a beginner through handing a song to someone else: stems, multitracks and the stereo mix, what a mixing and a mastering engineer each want, preparing the files (same start and length, 24-bit, the project's rate, no dither, no normalising, headroom, no loudness limiter), what to print, checking the files add up, what to send with them, and revisions. Its *Your song's stems* topic shows the open song as the files Bounce to WAV would make, with sizes, what's left out and what to fix first.

**Can *Your song* rely on stems export?** Yes. #88 is closed and `plans/bounce-selected-in-place-stems.md` is Released with both goes built (multitracks as heard or dry, stems, *About these files.txt*, *Tempo map.mid*). `bounce::planFiles` gives the files, names and what's skipped and why, without rendering. The issue's "still open" note is out of date; #160 holds the rest.

## Decisions for you
1. **Name and key.** Options: the issue's title (long); *Handing over your song*; *Sending stems to an engineer*. **Recommended:** *Sending stems to an engineer*, key `handover`: short, has the searched word, still covers the stereo mix. Answer: Accepted: *Sending stems to an engineer*, key `handover`.
2. **New guide or more routing topics.** **Recommended:** a new guide. *Buses, sends and routing* (19 topics) says how DawIt makes each file; this one is the practice (formats, headroom, notes, revisions). Its *Stems and multitracks* gets a pointer sentence. Answer: Accepted: a new guide, plus a pointer sentence in the routing guide.
3. **Where it opens.** The issue's "right-click menu on File > Bounce to WAV..." isn't possible on a menu bar item. **Recommended:** a "?" in the Bounce to WAV window (as the Quantise dialog has), a track right-click item, the Theory Guides window and the routing guide's pointer. Answer: Accepted: the Bounce window "?", the track right-click item, Theory Guides and the routing guide's pointer.
4. **The mix's headroom in *Your song*.** Options: the last *Measure Loudness...* sample peak (whole song, may predate the last edit); the live master peak while playing. **Recommended:** the last measurement, naming its bars, else suggest measuring; a live peak misses bars not played. Answer: Accepted: the last Measure Loudness sample peak, naming its bars, else suggest measuring.
5. **Revision numbers.** **Recommended:** "two or three rounds" for mixing, as typical, with "agree first" (a pricing guide and engineers' listings); the alternative is no number. Answer: Accepted: "two or three rounds", typical, with "agree first".
6. **Stem mastering "from 2 to about 8" stems** is typical from studio pages and search summaries (Abbey Road: two; others 3 to 8), not one authority. **Recommended:** keep, worded "usually a handful". Answer: Accepted: keep, worded "usually a handful".

## What the user does and sees
- **Opens from:** Theory Guides (Mixing); a "?" `GuideButton` top right in the Bounce to WAV window (opens *Preparing the files* when *What* is multitracks or stems, else *Mixing or mastering?*); "Sending stems to an engineer..." in the track right-click menu (`guideicon::addItem`, just before "About buses, sends and routing...", which stays last).
- **Window:** ui-design.md *Guide windows*, about 1000 × 760, 9 topics then *Sources*.
- ***Your song's stems*:** lanes, one per stem, along the whole song (bars below), in the bus or track colour: blocks where it has unmuted clips, grey where its file is silence, all ending together with a shaded "4 s tail"; the tracks inside in small text under the name, the file name at the right; left-out tracks dim below with reasons. At the right a level bar of the mix's peak, lines at 0, -3, -6 dBFS ("Measure Loudness to see the peak" when none). Text: counts, sizes, advice. A **Bounce to WAV...** button above the text opens the window. Follows the song, the last measurement and the remembered rate and bit depth.
- **Tooltips:** "?": "Sending stems to an engineer: what a mixing or mastering engineer wants, how to prepare the files and how to check them. Opens the guide." Lane: "Drums stem · Kick, Snare, Hats, Overheads · 01 Drums.wav · clips in bars 1 to 105". Level bar: "Your mix peaks at -1.2 dBFS (last Measure Loudness, bar 1 to bar 105)." Button: "Open Bounce to WAV (Cmd+B) to make the files."
- **Remembered:** nothing new; reads `export.bounceRate`, `export.bounceBits`.

## Data and saving
None: no project data, format change or settings key.

## The guide
New guide `handoverguide`, area Mixing, not on *Start here*. Topics (texts in *Theory*) and pictures:
1. **Stems, multitracks and the mix**: three columns of file tiles for the pop example: the mix (1), stems (5, bracketed to their tracks), multitracks (13).
2. **Mixing or mastering?**: two folders, "To a mixing engineer" (13 multitracks, Rough mix, About these files.txt, Tempo map.mid, Notes) and "To a mastering engineer" (one premaster, dimmed "only if asked: 4 stems", Reference, Notes).
3. **Preparing the files**: the example's 5 stem lanes over 105 bars plus the tail, "every file starts at bar 1" and "3:34" marked, a format line.
4. **Headroom and the master**: level bars at -0.3 ("too hot", flattened), -6.0 ("the usual target"), -14.0 ("fine"), lines at 0, -3, -6.
5. **Effects: printed or dry**: a Lead vocal chain (clip, DawIt EQ, DawIt Compressor, fader, send to Reverb) twice, *As you hear it* all lit, *Dry* only the clip.
6. **Checking before you send**: stems summed, the mix flipped, their sum flat ("silence"); beside it a sum with a small wave, "a bus compressor: a quiet remainder" (illustration, no numbers).
7. **What to send with them**: the example folder, DawIt's files and yours in two colours.
8. **Working with an engineer**: Files sent, Mix 1, notes, Mix 2, notes, Mix 3, Final; bracket "usually included: 2 or 3 rounds".
9. **Your song's stems**: above.

**Entry:** title "Sending stems to an engineer"; summary "What stems and multitracks are, what a mixing or a mastering engineer wants, preparing and checking the files, headroom, what to send with them, revisions, and your song as files."; opens from "The ? in the Bounce to WAV window, a track's right-click menu (Sending stems to an engineer...), and the routing guide's Stems and multitracks"; keywords "stems stem multitracks multitrack handover hand-over deliver delivery send sending export bounce files folder mixing-engineer mastering-engineer engineer mixer premaster pre-master stem-mastering stereo-mix rough-mix reference null null-test polarity dither normalise normalize headroom limiter 24-bit 32-bit float sample-rate wav revisions revision notes tempo key naming"; topics from `allTopics`.

## Code
- Recipes: *A theory guide*, copying the recording guide (`src/engine/RecordingGuide`, `RecordingExplainer`, `MainComponent::showRecordingGuide`); *A command or menu item*; *Checking in the real app*.
- **Content** in `src/engine/HandoverGuide.{h,cpp}` (`daw::handoverguide`): engine layer, because *Your song* reads the Engine and `bounce::planFiles`.
- **Song view:** one pure function of the Engine plus the UI's optional inputs (remembered rate and bit depth, last sample peak and its bars) returning the lanes (name, colour, tracks inside, merged beat ranges of unmuted clips, file name), skipped list, both counts, seconds per file (song range plus `bounce::filesTailSeconds`), bytes (rate × 2, 3 or 4 bytes × 2 channels × seconds; `recordingguide::describeBytes`) and the advice. Files come from `bounce::planFiles` on `bounce::getSongRange`, so the guide always agrees with Bounce. The adding-up advice is `routingguide::analyse`'s items with `Topic::stems`, reused, not copied.
- **Threading:** message thread only; no audio-thread or lock change. The explainer is a `ChangeListener` that recomputes on a short timer, like `RoutingExplainer`; the UI passes `loudnessui::getLast()` in.
- **Bounce window "?":** an opener set once by MainComponent (as `sampleractions::setGuideOpener`), so every `bounceui::showDialog` caller gets it.
- **Risk:** low. `Guide::handover` needs a case in every `switch` over `Guide` (`-Wswitch-enum`).

## Libraries and models
None, so `LICENSE`, `licenses/` and the README's *License note* don't change.

## Theory

### Facts
1. Stem: a group of tracks mixed to one file; multitracks: one file per track. **exact** (iZotope *Stems and multitracks*).
2. DawIt's stems: each top-level bus and each track outside a bus; returns' shares are inside the files. **exact** (`bounce::planFiles`).
3. Musicians often say "stems" for multitracks. **typical** (iZotope *Stems and multitracks*).
4. Mixing engineers work from multitracks; wet, dry or both differs: ask. **opinion** (iZotope; Sonarworks; bchillmix stem delivery).
5. Mastering works on one stereo premaster; stem mastering uses a few group stems (about 2 to 8) when one group needs a change. **typical** (iZotope *Stem mastering*; search: Abbey Road, others).
6. Files start together (usually bar 1) and are the same length; leading silence is fine. **exact** (bchillmix stem delivery; Dana Nielsen; iZotope *Stem mastering*).
7. DawIt adds 4 s after the range to every file. **exact** (`bounce::filesTailSeconds`).
8. 105 bars of 4/4 at 120 BPM = 105 × 4 × 0.5 = 210 s = 3:30; + 4 s = 3:34. **exact** (derived).
9. WAV, 24-bit or 32-bit float, not MP3; 16-bit for the final master. **typical** (bchillmix both; Splice; routing guide's "at least 24-bit").
10. Keep the project's sample rate; don't convert. **typical** (bchillmix both; Splice; Dana Nielsen).
11. Dither once, at the final bit reduction, by the mastering engineer; none on a 24-bit or float premaster. **typical** (Splice; bchillmix checklist; dither search, iZotope).
12. DawIt's Bounce adds no dither. **exact** (code: no dither stage in `Bounce.cpp`).
13. Normalising gives each file its own gain, so the balance is lost. **exact** (derived); keep it off **typical** (bchillmix both).
14. Mix peaks about -6 dBFS for mastering; many accept -3 to -4 if unclipped; some ask more (-10). **typical** (bchillmix checklist; Splice; masteringbox, Waves; Gain staging's -6).
15. Headroom = 0 dBFS minus the peak. **exact** (`routingguide::getHeadroomDb`).
16. Master volume down x dB lowers the bounced peak by x dB (nothing after it): -1.2 needs 4.8 dB for -6.0. **exact** (derived).
17. 16/24-bit files clip over 0 dBFS; float doesn't. **exact** (routing and sound guides).
18. Bypass a loudness limiter; gentle glue that's part of the sound can stay; a loud copy as reference. **opinion** (bchillmix checklist; Splice).
19. Null test: flip polarity and add; identical cancels, the rest is the difference. **exact** (null test search).
20. Bus dynamics, saturation, distortion and pitch effects stop parts adding up; that bus's stem is exact. **exact** (routing guide; `runBounceFilesTests`).
21. Sounds with randomness differ slightly each bounce; several DawIt built-ins use it. **exact** (code: `juce::Random` in `BuiltInSynth`, `BuiltInSubtractive`, `BuiltInPhysical`).
22. Bytes = rate × bytes per sample × channels × seconds: 48 kHz, 24-bit, stereo, 214 s = 61,632,000. **exact** (derived).
23. Rough mix labelled not to mix from, one to three references, short ranked notes with tempo and key, check in a blank session. **opinion** (bchillmix both).
24. Mixing quotes commonly include two or three rounds, mastering fewer; new takes or parts cost extra. **typical** (Violet Recording; search: SoundBetter listings).

### Topic texts

**1. Stems, multitracks and the mix** — Three ways to hand over a song

When someone else mixes or masters your song, they don't open your DawIt project. Their program can't play your instruments and effects, so you send audio files instead. There are three kinds.

The mix is one stereo WAV file: the whole song as you hear it. It's what a mastering engineer works on, and what you play to friends.

Multitracks are one file per track: Kick, Snare, Bass, Lead vocal, each on its own. The song in the picture has 13 tracks, so it makes 13 files. A mixing engineer works from these, because mixing means setting the level, tone and space of every part.

Stems are one file per group: all the drums in one file, then the bass, the guitars, the keys and the vocals. The song in the picture makes 5. In DawIt a stem is each bus at the top level, plus each track that isn't in a bus, so your buses decide the stems. Backing vocals sits inside the Vocals bus, so it is part of the Vocals stem.

People use the word loosely. Many musicians say "stems" when they mean multitracks. When an engineer asks for stems, ask which they mean: every track, or the groups.

Every file covers the same stretch of the song, from the same start to the same end. Put them side by side at 0 dB in any program and they play back your mix (Checking before you send says when they don't quite).

All three come from File > Bounce to WAV... (Cmd+B): under What, pick The mix, Every track (multitracks) or Every bus and lone track (stems). This guide says which to choose and how to prepare them. How DawIt makes each file is in the routing guide, under Stems and multitracks.

**2. Mixing or mastering?** — Who wants which files

Mixing and mastering are two different jobs, and they want different files.

A mixing engineer balances the parts: levels, EQ, compression, panning, reverb. For that they need every part on its own, so send multitracks, one file per track. Some like the group stems as well, to hear how your groups sound.

Send the files as you hear them (with your effects), dry (only the clips and the instrument), or both. Engineers differ, so ask. Many like your effects when they are part of the sound, such as a distorted guitar or a synth's delay. Some want everything dry so they can choose again.

A mastering engineer works on the finished mix: the overall tone, the loudness and the last polish, so the song sounds right on every system and next to other songs. Send one stereo file, the mix, prepared as in Headroom and the master. This file is often called the premaster.

Sometimes a mastering engineer works from stems instead, which is called stem mastering. It helps when the mix is nearly right but one group needs a change that can't be made on the whole mix, such as a vocal a little too quiet or a bass that booms. They usually want a handful of stems: from 2 (the vocals and everything else) to about 8 (drums, bass, guitars, keys, lead vocal, backing vocals and so on). Send stems only when they ask. Many files turn mastering back into mixing.

Not sure? Ask before you bounce. A one-line email ("Multitracks, wet or dry? 24-bit at 48 kHz?") saves sending everything twice.

In short:
- Mixing: multitracks, as you hear them, dry, or both, with a rough mix (see What to send with them).
- Mastering: the mix, as one stereo WAV file. Stems only when asked.

**3. Preparing the files** — Same start, same length, 24-bit, no dither

An engineer should be able to drop your files into a new session, press play and hear your song. A few rules make that work.

Same start. Every file starts at the same moment, usually bar 1, even a guitar that only plays in the chorus: its file is silent until the chorus comes. The engineer lines them all up at the start and nothing is out of time. A file that begins where its part begins has to be placed by hand, at exactly the right spot. Silence at the start of a file is fine; a file in the wrong place is not.

Same length. Every file also ends at the same moment, with room after the last note for a reverb or a held note to die away. DawIt adds 4 seconds after the end of the range to every file. The song in the picture is 105 bars at 120 BPM, 3:30, so every file is 3:34 long.

WAV, 24-bit. A WAV file keeps every sample as it is. An MP3 throws some of the sound away, so it isn't used for mixing or mastering. 24-bit is what most engineers ask for: its noise floor is so far down you never hear it. 32-bit float is fine too, and keeps peaks over 0 dB that a 24-bit file would cut off. Leave 16-bit for the finished master.

The project's sample rate. Send the files at the rate your song was made at: if your recordings are 48 kHz, send 48 kHz. Converting on the way out changes every sample and gains nothing. Bounce to WAV offers 44.1 kHz and 48 kHz; Your song's stems shows which rate your recordings use.

No dither. Dither is a very quiet noise added when a file loses bits, such as 24-bit to 16-bit, to hide the rounding. It belongs to the very last step, once, when the mastering engineer makes the final file. A 24-bit or 32-bit float file for an engineer needs none, and DawIt's Bounce to WAV never adds it.

Not normalised. Normalising turns a file up until its loudest peak reaches a set level. Done to stems, it turns each file up by a different amount, and your balance is gone: the quiet shaker comes back as loud as the snare. Send the files as they come out.

Nothing extra on the master. DawIt has no effects on the master, so the files hold your tracks, buses, returns and the master volume, and nothing more. A compressor or limiter on a bus that holds the whole song is in the files; see Headroom and the master.

**4. Headroom and the master** — Peaks near -6 dBFS, no limiter

Headroom is the space between your loudest peak and 0 dBFS, the top of a digital file. A mix that peaks at -6 dBFS has 6 dB of headroom.

For a mastering engineer, leave some. Mastering EQ and compression change the peaks, and the engineer wants to set the final loudness themselves. Peaks around -6 dBFS are the usual target, as in the Gain staging guide. Many engineers are happy with -3 or -4 dBFS as long as nothing clips; a few ask for more, such as -10. A quieter mix is fine too: a 24-bit file has plenty of room below.

No limiter for loudness. A limiter that makes your mix loud on the way out leaves the mastering engineer little to work with: the peaks are already flattened and can't be brought back. Bypass it before you bounce the premaster. If you like how loud it sounds with it, bounce a second copy with the limiter on and send it as a reference: "this is roughly how loud I hear it". A gentle compressor that is part of your mix's sound, holding it together with 1 or 2 dB of gain reduction, can usually stay. Say in your notes that it's there.

How to get there. File > Measure Loudness... shows the song's peak. If it peaks at -1.2 dBFS, turn the master volume down 4.8 dB. Nothing comes after the master volume in DawIt, so the peak drops by exactly that much, to -6.0 dBFS. The mix sounds the same, only quieter.

Over 0 dB? A 16- or 24-bit file cuts off anything over 0 dBFS, and that clipping is in the file for good. Bounce to WAV's report shows the peak and any file that went over.

Stems and multitracks for a mixing engineer need no headroom target of their own. Each is quieter than the whole mix, and the engineer sets the levels again. Just make sure none of them clips.

**5. Effects: printed or dry** — What goes into the files

Printing an effect means bouncing it into the audio. A printed effect can't be changed later, but it sounds exactly as you meant. Left out, it can be set up again, but maybe not quite as you had it.

As you hear it. DawIt's stems and multitracks are printed unless you choose otherwise: each file is what you hear when you solo that track or bus, with its effects, the bus it plays into, its share of the reverb and delay on the returns, its fader and pan. That's the safe choice: what you send is what you heard.

Dry. Under Sound, Dry gives multitracks with only the clips and the instrument: no effects, the fader at 0 dB, the pan in the middle and no reverb. The engineer makes all those choices again. A synth's sound comes from its instrument, so it is in the dry file; only the effects after it are left out.

Some effects are part of the sound and are best printed, even when the rest goes dry:
- Effects that move with the beat, such as a delay set to an eighth note, a step filter or a slicer. Rebuilt in another program, the pattern may not land the same.
- Reverb and delay on returns. As you hear it, each file carries its own share of the return: the vocal's reverb is in the vocal file. Some engineers like the reverb as a file of its own, so they can set its level; DawIt can't make that yet.
- Parallel processing, where a copy of a track goes through heavy compression or distortion and is blended back in. Print the blend.
- Ducking, or sidechain compression: a bass that dips each time the kick plays. In programs that have it, a compressor on the bass listens to the kick. Print it into the bass, or the engineer has to set it up again.
- Pitch correction and harmonies. DawIt Tune and DawIt Harmoniser change the notes themselves. Send them printed, and the raw vocal as well if the engineer wants to tune it their own way.

When in doubt, send both: Bounce twice, once As you hear it and once Dry, each into its own clearly named folder, such as My song (as heard) and My song (dry). The files keep the same names, so the folder says which is which.

**6. Checking before you send** — Do the files add up to your mix?

Before you send, check that the files really are your song.

Do they add up? Put the files on tracks in a new project, all starting at bar 1, faders at 0 dB, pans in the middle, no effects. Played together they should sound exactly like your mix. Switch between them and a bounce of the mix: you shouldn't hear a difference.

Engineers check this precisely with a null test. Flip the polarity of the mix (turn its wave upside down) and play it with the files. Where the two are the same they cancel to silence; whatever is left is the difference. Files of a song with EQ, faders and pan cancel to silence, or to a hiss far too quiet to hear.

Why the files don't always sound like your mix:
- A compressor, gate, saturation or distortion on a bus or a return. It reacts to everything in it at once. Kick alone pushes the drum bus compressor less than the whole kit does, so the drum multitracks added up are a little louder and less squashed than in your mix. The Drums stem keeps the whole bus in one file, so it is exactly right. Send the stem as well, or say which compressor you used. Your song's stems names any such bus.
- An effect that follows the pitch on a bus or return, such as DawIt Tune or DawIt Harmoniser: it hears all the notes at once.
- A sound with a little chance in it, such as noise in a snare or an oscillator that starts at a random point. Several of DawIt's built-in sounds work this way, as many synths do. Two bounces are never quite the same, so a null test leaves a quiet remainder even though they sound the same.
- Dry files. They aren't meant to add up to your mix: the effects are missing on purpose.
- A file in the wrong place, or faders not at 0 dB in the other program.

Then listen to each file:
- At least the start and the end, for a click where a sound is cut off.
- For clipping: Bounce to WAV's report lists any file that went over 0 dB.
- For anything missing: muted tracks and tracks with no clips are left out, and the report says so.

Last, open the folder (or the zip) the engineer will get. Are all the files there, with clear names? Bounce to WAV writes them into a new folder, with About these files.txt and Tempo map.mid.

**7. What to send with them** — Names, a rough mix, references and notes

The files are only half of a hand-over. The engineer also needs to know what the song is and what you want from it.

Clear names. A file called Audio 12.wav tells nobody anything. DawIt names each file after its track, numbered in the order of your tracks: 01 Kick.wav, 02 Snare.wav. So name your tracks before you bounce: Lead vocal, not Audio 3. Name the folder after the song, with its tempo and key if you like: Summer Song 120bpm Am.

The tempo and key. A WAV file holds no tempo. The engineer needs it to set the grid, time delays to the beat and edit in bars. DawIt writes the tempo, key, sample rate and bit depth into About these files.txt, and the tempo and its changes into Tempo map.mid, which most programs can import.

A rough mix. Bounce The mix as well, over the same range, and put it in the folder with a name like Rough mix - do not mix from.wav. It shows how you hear the balance, the effect moments and where the vocal sits. It doesn't need to be perfect.

One to three reference songs. Finished songs whose sound you'd like to get close to, each with a line on what you like about it: "the dry, close vocal", "how the kick and bass fit". More than three tend to pull in different directions.

Short notes. What matters most to you, what worries you, and what must not change, ranked, in a few lines: "1. The vocal up front. 2. The low end feels muddy from bar 25. 3. Keep the guitar delay in the bridge." Add the song title, the artist and any deadline.

For mastering: the premaster, its sample rate and bit depth, a loud reference if you made one, how the song should end (your fade-out, or one they make), and where it's going (streaming, CD, vinyl).

The MIDI, when it helps. Export track as MIDI file... saves a track's notes. An engineer or producer can use them to replace a drum sound, double a bass line or check a chord.

**8. Working with an engineer** — Revisions, and keeping your own copy

A mix is a conversation. The engineer sends a first version, you listen and send notes, and they send a revision.

How many? A quote for mixing usually includes two or three rounds of changes; more are charged per round. Mastering usually includes fewer. Agree on the number of rounds, the price and the deadline before you start, in writing.

Good notes are specific: the time or bar, what you hear, and what you'd like. "At 1:12 the snare is too loud" helps; "make it pop" doesn't. Before you write them, listen on systems you know, such as your headphones, your car and your phone, and at the same volume each time: a louder version always sounds better at first.

A revision changes the mix, not the song. A new vocal take, an extra instrument or a changed arrangement usually means new files, and often a new price.

Keep your own copy. Keep the DawIt project as it was when you bounced, and the folder you sent. If the engineer asks for one track again, dry or a little longer, you can make it in a minute with the same settings. Use File > Save As... to save a copy with the date in its name before you carry on working.

**9. Your song's stems** — What your song would send (general part)

This page looks at your song as the files Bounce to WAV would make. The picture shows your stems as lanes along the song, with the tracks in each, where they play and where their files are silent. Below, the tracks that would be left out, and why.

The text counts the files for stems and for multitracks and how much room they take, at the sample rate and bit depth you last chose in Bounce to WAV. It checks the mix's headroom (from the last File > Measure Loudness...), the sample rate of your recordings, track names that say nothing, a bus that holds the whole song, and any bus effect that stops the files adding up to your mix.

It follows your changes.

### Other texts
- Tooltips: as in *What the user does and sees*.
- Routing guide, *Stems and multitracks*, after its first paragraph: "What a mixing or mastering engineer wants, and how to prepare and check the files, is in the guide Sending stems to an engineer."

### Example data
- Pop example as `routingguide`'s: Drums (Kick, Snare, Hats, Overheads), Bass, Guitars (Guitar L, Guitar R), Keys (Piano, Pad), Vocals (Lead vocal; Backing vocals: BV 1–3): 5 stems, 13 multitracks.
- Lanes, 105 bars at 120 BPM (bars as shown): Drums 1–105; Bass 9–105; Guitars 25–40, 57–72, 89–105; Keys 1–105; Vocals 9–96. Files 3:34.
- Sizes at 48 kHz, 24-bit, stereo: 288,000 B/s × 214 s = 61,632,000 B a file ("61.63 MB"); 5 files 308,160,000; 13 files 801,216,000.
- Folder: DawIt writes 01 Drums.wav … 05 Vocals.wav, About these files.txt, Tempo map.mid; you add Rough mix - do not mix from.wav, Notes.txt, References.txt.

### "Your song" rules
In this order, each quiet when its condition fails; "audible" as `planFiles` decides.
1. **Summary** (any audible track): "As stems, your song makes 3 files: Drums, Bass and Vocals. As multitracks, 7. Each file is 3:34 long (the song plus 4 seconds), 61.63 MB at 48 kHz and 24-bit: 184.90 MB for the stems, 431.42 MB for the multitracks." None: "Your song has no clips to bounce yet."
2. **Left out**: "Left out: Pad (muted), Strings (no clips)." plus "Unmute a track if it belongs in the song." when one is muted.
3. **Names that say nothing** (audible tracks or buses named exactly "Audio N", "MIDI N", "Bus N"): "Audio 2, Audio 5 and MIDI 1 have names that say nothing. File names come from track names, so rename them before you bounce." Past three: "4 tracks have names that say nothing. ..."
4. **One bus holds everything** (stems = 1 file, a bus with ≥ 2 audible tracks): "Everything plays into the Mix bus, so stems make one file: the whole mix. For group stems, bounce each group with Selected tracks, or take the groups out of Mix."
5. **Its compressor** (that bus has an unbypassed DawIt Compressor, or a plugin named with "limit"): "Mix has DawIt Compressor, which works on the whole song. For a mastering engineer, bypass it if it's there to make the song loud; gentle compression of 1 or 2 dB that holds the mix together can stay. Say in your notes that it's there."
6. **Adding up:** `routingguide::analyse`'s `Topic::stems` advice, as it is.
7. **Headroom** (P = last sample peak; "(last Measure Loudness, bar 1 to bar 105)" after the number): P > 0: "Your mix peaks at +1.4 dBFS (...): over 0 dB, so a 24-bit file would clip. Turn the master volume down 7.4 dB to peak near -6 dBFS." -3 < P ≤ 0: "Your mix peaks at -1.2 dBFS (...). For a mastering engineer, turn the master volume down 4.8 dB to peak near -6 dBFS." -6 < P ≤ -3: "Your mix peaks at -4.5 dBFS (...): enough headroom for most mastering engineers; -6 dBFS gives a little more." P ≤ -6: "Your mix peaks at -8.0 dBFS (...): 8.0 dB of headroom, ready for mastering." None: "Measure Loudness... (File menu) shows your mix's peak, to check its headroom before mastering."
8. **Sample rate** (audio clips' own rates; R the remembered rate): no audio clips: "Your song is all MIDI, so 44.1 or 48 kHz are both fine; ask the engineer which they work at." One rate of 44.1/48 ≠ R: "Your recordings are 48 kHz, but Bounce to WAV is set to 44.1 kHz. Choose 48 kHz so nothing is converted." = R: quiet. One other rate: "Your recordings are 96 kHz, and Bounce to WAV makes 44.1 or 48 kHz files. Choose one, and tell the engineer the recordings were 96 kHz." Mixed: "Your recordings use 44.1 kHz (3 clips) and 48 kHz (5 clips). Choose the rate most of them use, 48 kHz, and say so in your notes." (a tie: 48 kHz).
9. **Bit depth** (remembered 16): "Bounce to WAV is set to 16-bit. Choose 24-bit, or 32-bit float, for files you send to an engineer."
10. **Tempo changes** (more than one tempo in the song): "Your song changes tempo. Bounce to WAV puts the changes in Tempo map.mid; mention them in your notes too."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| 105 bars at 120 BPM: seconds per file | 214.0 | ±0.01 |
| Bytes a file, 48 kHz 24-bit stereo / 44.1 kHz 24-bit / 48 kHz float, 214 s | 61,632,000 / 56,624,400 / 82,176,000 | ± one frame |
| 13 files, 48 kHz 24-bit | 801,216,000 | ± 13 frames |
| Pop-shaped song: stems / multitracks | 5 / 13 | exact |
| Turn-down for peak -1.2 / +1.4 dBFS | 4.8 / 7.4 dB | ±0.05 |
| Generic: "Audio 3", "MIDI 12", "Bus 2" yes; "Audio Lead", "Audio 3b", "Bass" no | as listed | exact |
| Topics | 9 | exact |

## Steps

**Group A, engine** (`./build.sh --test` at step 2)
- [x] 1. `src/engine/HandoverGuide.{h,cpp}` (both targets in `CMakeLists.txt`): topics, names, summaries, texts, example data, the size and turn-down helpers. Tests: `runHandoverGuideTests` with the numbers and texts.
- [x] 2. The song view and the ten rules; the routing guide's pointer sentence. Tests: the rule checks below. Done when `./build.sh --test` passes without warnings.

**Group B, UI** (`./build.sh --test` at step 6)
- [x] 3. `Guide::handover` in `gen::guides` (key, entry, every `switch` over `Guide`); add `Guide key: handover` under the *Handing over* heading in `SOURCES.md` (written without it, since the SOURCES test rejects a key that isn't a guide); `MainComponent::showHandoverGuide`, `openGuide`, `getPrintGuide`. Tests: `runTheoryGuidesTests`, search checks.
- [x] 4. `src/ui/HandoverExplainer.{h,cpp}`: layout, list, `GuideSourcesTopic`, PDF export, pictures of topics 1 to 8. Check: `--export-guides` passes.
- [x] 5. *Your song's stems*: lanes, level bar, advice, *Bounce to WAV...* button, timer-coalesced `ChangeListener`, inputs from `export.bounceRate`, `export.bounceBits`, `loudnessui::getLast()`.
- [x] 6. Openers: the Bounce window's "?" (topic by *What*), the track menu item. Done when `./build.sh --test` passes without warnings.

**Group C, check and docs**
- [x] 7. Real-app check, one `--screenshots` run on the pop song: `guide: handover` at topics 1 to 8 (pictures drawn, nothing cut off); the `loudness` scene then *Your song's stems* (lanes, measured peak, sizes, advice); *Your song's stems* with `"settings": {"export.bounceBits": 16, "export.bounceRate": 48000}` (16-bit advice); the `bounce` scene with `"what": "stems"` (the "?" fits).
- [x] 8. Docs: README *Features* bullet for the guide and the "?" in the Bounce bullet; `docs/tests.md`; `docs/code-layout.md` (`HandoverGuide`, `HandoverExplainer`); `docs/architecture.md` *Guides*: add `handoverguide` to the engine guides; progress on #16, and a note on #160 that this guide explains today's choices.

## Tests
`runHandoverGuideTests`, on songs of audio clips with made-up buffers:
- *Test numbers*; every topic has name, summary, text and example data.
- Pop-shaped song: 5 lanes in edit order with their tracks and "01 Drums.wav" to "05 Vocals.wav"; a chorus-only track's ranges match its clips, merged when they touch.
- Each rule fires on its condition and is quiet otherwise: muted track listed with the unmute line; "Audio 2" fires 3, "Lead" doesn't; one top-level bus with two tracks fires 4, with DawIt Compressor 5, bypassed not; DawIt Compressor on a 2-track drum bus brings the routing stems advice; peaks -1.2, +1.4, -4.5, -8.0 and none give their texts; rates: all 48 kHz with R 44.1 fires, R 48 quiet, 3 × 44.1 + 5 × 48 says 48 kHz; 16-bit fires 9, 24 doesn't; a tempo change fires 10; an empty song gives only the "no clips" text.
- `runTheoryGuidesTests`: the entry's topics; "stems", "premaster", "null test", "dither", "mastering engineer", "revisions" find `handover`. `--export-guides` passes.

## Website hand-off
- `learn.html`: the guide in Mixing, next to *Buses, sends and routing*.
- `features.html`, Bounce/export group: the window's "?" opens a guide to handing over stems, which shows your song as files.
- Screenshot: *Your song's stems* on the example song after Measure Loudness. No `about.html` change; not for Home or `for-you.html`.

## Left for later
- Returns as files, a mix file in the folder, master effects in stems: #160 (topics 5 and 7 then mention them).
- A null test inside DawIt (polarity switch or a "Check files..." command): no polarity control yet.
- Mono files for mono tracks: every file is stereo today.
- Free choice of what each stem holds, beyond top-level buses.
- Fees, contracts and credits: *Releasing music* (#18).

## Changes while building

**Group A hand-off (engine).**
- `src/engine/HandoverGuide.{h,cpp}` (`daw::handoverguide`, in both CMake targets): `Topic` (stems, mixOrMaster, preparing, headroom, effects, checking, sendWith, working, yourSong), `allTopics`, `getTopicName`, `getTopicSummary`, `explain` (texts as in *Theory*, topic 9's general part only), `getBytesPerFile`, `getTurnDownDb`, `isGenericName`, `describeSeconds`, `describeRate`, `getExampleStems()` (bars as shown, inclusive; file names), `getExampleSecondsPerFile()`, `exampleNumBars`, `exampleNumTracks`.
- Song view: `handoverguide::analyse (const Engine&, const Inputs&)` returns `SongView` { `lanes` (name, colour, tracks inside, merged beat `ranges`, `fileName`, `track`), `skipped`, `numStems`, `numMultitracks`, `songBeats`, `secondsPerFile`, byte counts, `advice` (the ten rules as lines, in order) }. `Inputs` is plain data: `bounceRate`, `bounceBits`, `hasPeak`, `peakDb`, `peakScope`. The UI fills `peakScope` in plain words ("bar 1 to bar 105"); the advice adds "(last Measure Loudness, ...)" itself. The level-bar tooltip in the UI needs the same words, so build the scope string once in the UI.
- Texts the plan did not give, made up for grammar: rule 3 with one name ("Audio 2 has a name that says nothing. File names come from track names, so rename it before you bounce.") and with two names it uses "have". Rule 5 only follows rule 4 (the one bus). Rule 6 is `routingguide::analyse`'s `Topic::stems` items unchanged. A "Left out" line lists tracks first, then any bus the stems plan left out.
- Routing guide's *Stems and multitracks* has the pointer sentence after its first paragraph.
- Not done here (Group B): `Guide::handover`, the explainer, pictures, openers.

**Group B hand-off (UI).**
- `Guide::handover` (key `handover`, area Mixing) in `src/generate/TheoryGuides.{h,cpp}`; keyword `test` added so "null test" finds it. `MainComponent::showHandoverGuide`, `openGuide`, `getPrintGuide`; track menu item "Sending stems to an engineer..." (`TrackActions::onShowHandoverGuide`, via `ArrangementView::onShowHandoverGuide`), just above "About buses, sends and routing...".
- `src/ui/HandoverExplainer.{h,cpp}` (in `CMakeLists.txt`, app only): pictures for topics 1 to 8 from `getExampleStems()`, *Your song's stems* drawn from `handoverguide::analyse` (lanes, left-out line, vertical peak bar -30 to +3 dBFS with lines at 0, -3, -6, lane and bar tooltips), a *Bounce to WAV...* button (`onShowBounce`, set in `showHandoverGuide`) shown only on that topic. Recomputes on a 10 Hz timer when the engine changed or the inputs (`export.bounceRate`, `export.bounceBits`, `loudnessui::getLast()` peak and scope) did. `--export-guides` passes (10 topics, 11 pages).
- Bounce window "?": `bounceui::setGuideOpener (std::function<void (bool files)>)`, set in the MainComponent constructor; opens topic 2 (*Preparing the files*) for multitracks or stems, else topic 1 (*Mixing or mastering?*). The button is bottom left of the window, which is unchanged in size (520 x 320): Group C should check it fits.
- Tests changed: two routing-guide search checks assumed "stems" and "multitracks" find the routing guide first; they now find it (not first) because the new guide leads. Added `handover guide:` entry check in `runHandoverGuideTests`.
- SOURCES.md: added `Guide key: handover`, and two searches covering facts the planner cited by name without a URL (Dana Nielsen on same start and sample rate; Waves on headroom, whose own advice is -2 to -4 dBFS peaks). No new pages read, so the Waves difference is only in the searches note; dawTheory may want to check fact 14.
- Group C: shots as in step 7; look at the pictures of topics 1 to 8 and the Bounce "?" first, as no screenshot was taken in Group B.

**Group C hand-off (check and docs).**
- Shots (scale 1, pop song unless noted; list in the scratchpad): `guide: handover` at the eight topics, then `scene: loudness` and *Your song's stems*, again with `export.bounceBits` 16 and `export.bounceRate` 48000, and `scene: bounce` with `what: stems` (`capture: window`).
- Found and fixed: the *Preparing the files* picture scaled the example by seconds / 8 instead of seconds x 2 / 4 (bars at 120 BPM), so bars ran past the lanes over the file names and the axis ended at 29; now bars 1 to 105 plus the tail. Also: the "4 s tail" label no longer overlaps the last axis number; *Checking before you send* labels no longer sit under the waves; the measured peak's number moved under "Mix peak" so it no longer collides with the 0 dB tick.
- Looked at: all eight pictures, *Your song's stems* (4 lanes, -0.6 dBFS bar, 24.87 MB at 44.1 kHz 24-bit; 18.05 MB at 48 kHz 16-bit), and the Bounce "?" (bottom left, fits). The 16-bit advice line is below the fold in the shot; it is covered by the tests.
- Docs done: README (Theory/mixing bullet and the Bounce bullet), `docs/tests.md`, `docs/code-layout.md`, `docs/architecture.md`.
- Website hand-off: as in *Website hand-off* above; screenshot: *Your song's stems* after Measure Loudness (`guide: handover` topic *Your song's stems* after the `loudness` scene). Nothing new for `about.html`. dawTheory may check fact 14 (Waves' headroom advice).

**Theory check.**
- `HandoverGuide.cpp`, *Headroom and the master*: fact 14 now says there is no single rule: -6 dBFS stays the usual target (as in Gain staging and the routing guide), many accept -3 or -4, some online mastering services ask for -2 to -4 (Waves, read), a few want -10; "If yours names a number, use it."
- `HandoverGuide.cpp`: the comment on rule 8's tie said the higher rate wins; it is the lower one, except 48 kHz, which wins any tie (behaviour unchanged, as planned).
- `SOURCES.md`: the Waves and Sage Audio pages moved from search results to *Pages read*, with notes.
- Checked unchanged: topic texts 1 to 9 match *Theory* word for word; ten rules' texts and thresholds; rule 3's one- and two-name wording; test numbers (214 s, 61,632,000 / 56,624,400 / 82,176,000 / 801,216,000 B, 4.8 / 7.4 dB); the routing pointer; picture labels.
- Open for dawImplement: rule 8 with mixed rates can advise a rate Bounce doesn't offer ("Choose the rate most of them use, 96 kHz" when most clips are 96 kHz); the *Stems, multitracks and the mix* picture labels multitracks "1.1 Kick.wav" where Bounce writes "01 Kick.wav" (numbered across all 13).

**Follow-up (dawTheory fixes).** Rule 8 with mixed rates now picks only from 44.1 and 48 kHz (a 96 kHz majority advises the best offered one; with no offered rate it says Bounce makes 44.1 or 48 kHz and to tell the engineer the rates); test added. The multitrack tiles in topic 1 are numbered 01 to 13 with a running counter, as Bounce writes them.
