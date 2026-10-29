# Song templates: a whole song structure per style

Status: Built, not committed
Idea: [Song templates: a whole song structure per style](../FEATURE_IDEAS.html#song-templates-a-whole-song-structure-per-style)

## Goal
*File > New Project from Style...* makes a whole song in one of 14 styles (pop ballad to film score) in one click: tempo,
key, sections, chords per section, drums, bass, chords, a pad, empty melody clips, a Drums bus with glue compression and a
shared reverb return, so a beginner hears a complete song shape at once and starts changing it. A new guide, *Styles of
music*, says what makes each style (tempo, groove, chords, sounds, structure, mix), plays its groove, and compares the
user's song with the styles.

## Decisions for you

**Answered by the user, all as recommended:** new project only for the first release; empty melody clips, DawIt Drums with a kit per style and the two extra effects (Compressor Bass, EQ Make room for a vocal); all 14 styles, the guide in Start here after Song structure, and the producer-guide facts kept as worded and checked by dawTheory. Build: complete, not committed.
1. **How much in this first release.** Options: (a) the whole idea: insert into an existing song, AI fill, per-part
   regenerate, mood and length; (b) rule-based only: the *New Project from Style* window (always a new project) and the
   guide; inserting into a song, AI fill, mood and length later; (c) like (b) but no window, only a *New song in this
   style* button in the guide. **Recommended:** (b). DawIt has no undo yet, so "one undo removes it all" can't hold for an
   insert into a song; a new project is safe because *New Project*'s ask-to-save already exists. The window is small,
   and it is the idea's own entry for "I want a lo-fi track"; (c) loses the key and tempo choice for little saved work.
   Answer:
2. **The melody part.** Options: an empty clip per section where the melody belongs, for the user or *Generate*; a
   rule-based melody on chord tones (as the example song's vocal); no melody track. **Recommended:** empty clips: a
   made-up tune on chord tones teaches a bad melody, and *Generate* already writes one per clip.
   Answer:
3. **Drum sounds.** Options: DawIt Drums on each drum track at the style's kit (808-style, 909-style, Boom bap, Techno,
   Tight pop); or the built-in synth's Drums sound, as in the example song. **Recommended:** DawIt Drums: the kits are
   made per style, and its window teaches each drum.
   Answer:
4. **Effects beyond the two required ones** (the Drums bus compressor and the Reverb return). Options: also DawIt
   Compressor *Bass* on the bass and DawIt EQ *Make room for a vocal* on the chords, as the idea asks; or only the two.
   **Recommended:** both, so "each track gets the starting point that suits it", with its reason in *Your song*.
   Answer:
5. **All 14 styles now, or 8 first** (ballad, dance pop, rock, hip-hop, trap, house, lo-fi, blues). **Recommended:** all
   14: the data is a table and the texts are written below; the cost is the 14 tests and pictures, which are generic.
   Answer:
6. **On *Start here*.** **Recommended:** yes, right after *Song structure*: a beginner with no idea yet starts here.
   Answer:
7. **Facts from producers' guides rather than standards** (worded as typical, with ranges): the tempo ranges of lo-fi,
   reggaeton, afrobeats and blues; the dembow's sixteenth positions (derived from the tresillo plus a kick on every beat);
   the afrobeats, hip-hop, lo-fi and film drum patterns, given as "one common pattern", not as *the* pattern.
   **Recommended:** keep them as worded, and let dawTheory check them.
   Answer:

**Reverb:** the templates use DawIt Reverb's *Return: plate*, *Return: short room* and *Return: hall* starting points by
name. Since the reverb kinds (commit efa3b18) they carry the types Plate, Room and Hall; the Type default (Digital) is
never used by a template. The website's reverb pages were still uncommitted when this was planned: leave them alone.

## What the user does and sees
- **File > New Project from Style...** (below *New Project*, Shift+Cmd+N) opens a window, 860 × 560, fixed:

  ```
  Styles (220 px list)      | Trap                                    [What is trap?]
   Pop ballad    60-80      | 808s, hi-hat rolls and a half-time clap, written at about 140 BPM.
   Dance pop    100-130     | [picture: the style card, as in the guide]
   ...                      | Key [From the style v]  Tempo [----o---- 140 BPM]  [x] Write the notes
                            | Made Trap in F minor at 140 BPM: 7 sections, 8 tracks, 1:50. Press Space to play.
                            | [Explain this song]                         [Shuffle]  [Create]
  ```
- **List:** click a style to show it; double-click creates it. Row tooltip: "Trap · 130 to 150 BPM · Hip-hop and trap
  structure. Click to see it, double-click to make a song in it."
- **Key:** *From the style* (the style's default key at first, a key from its list after *Shuffle*) or any of the 12
  tonics in the style's mode (the mode can't change: the progressions are written for it). Tooltip: "The song's key. From
  the style uses one the style often uses; any key works: pick one that suits your voice."
- **Tempo:** a slider over the style's range, at its default; a new style resets it. Tooltip: "Trap usually runs from
  130 to 150 BPM."
- **Write the notes** (on): off makes the same tracks, bus, return and clips, with no notes. Tooltip: "Off: empty clips
  per section, for you to fill."
- **Create:** asks to save changes first, as *New Project* does, then makes the song. The project is *Untitled* and
  unsaved. While the song is exactly what this window last made, *Create* and *Shuffle* replace it without asking. The
  status line says what was made; Space plays (transport keys work in the window).
- **Shuffle:** a new random seed and *Create* again: another key from the list and other progressions where the style
  has alternatives. The first *Create* of a style uses seed 0, its defaults. Tooltip: "Another song in the same style."
- **What is <style>?** opens the guide at the style's topic; **Explain this song** at *Your song*.
- **Picture tooltips:** a section "Hook 1 · bars 5 to 12 · energy 5"; a part's block "Bass · plays in Verse 1"; a drum
  step "Clap · beat 3 · bar 1".
- **Remembered:** `newFromStyle.style` (style id), `newFromStyle.writeNotes`.

## Data and saving
Nothing new in the project: a template makes ordinary tracks, clips, plugins, a bus, a return, sends, sections, chords,
key and tempo, saved as today. No format change. Test: a built template saves and loads back to the same project text.

## What a template makes (the design of the song)
The 14 styles, their texts and their numbers are in *Theory*. Every template follows these rules:

- **Tracks, in edit order:** the drum tracks (each a MIDI track with DawIt Drums at the style's kit), *Bass*, *Chords*,
  *Pad* (only where the style has one), *Melody*, then the return. Fader and pan starting points: Kick -2 dB, Snare or
  Clap or Rim -4, Hats or Shaker -9 (pan +0.25), Bass -4, Chords -7 (pan -0.2), Pad -12, Melody -3; film: Low drum -4,
  High drum -8, Cymbal -12. Bass, chords and pad play the built-in sound of their chord style; Melody plays *Lead*.
- **Clips:** one clip per section where a part plays, named after the section ("Chorus 2"); the melody's clips are
  empty ("Verse 1 melody", decision 2). Bass, chords and pad notes come from `styles::play` (ChordStyles) on that
  section's chords; drums from the style's pattern, repeated per bar.
- **Chords:** each section type takes one of four progressions of the style: *verse* (verse, intro, interlude, the
  phrases "A" and "Groove", the solo), *chorus* (chorus, post-chorus, drop, the phrase "Peak", outro, breakdown), *lift*
  (pre-chorus, build-up) and *contrast* (bridge, the phrase "B"; rock and blues solos use *verse*); a role the style
  leaves out ("-" in the table) uses the verse's progression. A progression loops
  from its first chord, `barsPerChord` bars each, cut off at the section's end. The song's last bar is the home chord.
  Every chord comes from `guide::getCommonProgressions` for the key.
- **The Drums bus (required):** after the tracks exist,
  `engine.createBus ({ drum tracks }, "Drums")`, then `engine.addPlugin (*bus, BuiltInCompressor::getDescription(), error)`
  and `applyPreset` with DawIt Compressor's *Drum bus* starting point, as `examples::buildPopSong` does. The bus holds all
  and only the drum tracks (2 or 3), at 0 dB.
- **The Reverb return (required):** `engine.addReturn ("Reverb")`, then `engine.addPlugin (*ret, BuiltInReverb::getDescription(), error)`
  and `setSettings` with the style's return starting point (*Return: plate*, *Return: short room* or *Return: hall*, all
  at 100% mix). Then `engine.setSend (track, *ret, level)`, post-fader (the default): Chords -12 dB, Pad -8 dB, Melody
  -10 dB, and the Snare, Clap or Rim track -14 dB (not in the film score). Kick, hats, bass and the Drums bus never send.
  Every style has at least 2 senders (Chords and Melody).
- **Other effects (decision 4):** Bass gets DawIt Compressor *Bass*; Chords get DawIt EQ *Make room for a vocal*.
- **Who plays where:** the energy table in *Theory*, by the style's family (song, club, film). The build-up's snare roll
  and the club family's phrase rule are there too.
- **Seed:** seed 0 gives the defaults (first key, first progression of each role). Another seed picks the key and each
  role's progression from the style's lists with a fixed generator, so the same style, options and seed always give the
  same song.

## The guide
New guide **Styles of music** (`gen::stylesguide`), window 1000 × 760 in ui-design.md's guide layout, *Export PDF...*.
Topics in reading order (texts in *Theory*):

| # | Topic | Picture |
|---|---|---|
| 1 | Styles of music | the tempo map: a row per style, its range as a bar from 50 to 190 BPM, the default as a tick, the song's tempo as a vertical line in `theme::playhead`. Click a row: that style's topic |
| 2 | The parts of a song | the Dance pop template as a mini arrangement: the section strip in `SectionLane::getColour`, the energy curve over it, a row per part with a block per section where it plays |
| 3 | Drum bus and reverb return | the Dance pop template's routing as boxes and dashed sends, drawn like the routing guide's example (its `routingguide::Node` list and layout, made from the template) |
| 4-17 | one per style (Pop ballad ... Film score) | the style card: the tempo range bar; the mini arrangement; a one-bar drum grid (16 steps, 12 for the blues shuffle; two bars for trap), accents brighter. *Play the groove* loops two bars at the default tempo through `Engine::previewNote` with the step lit, as the rhythm guide plays its examples; a click on a drum row plays that drum. Button: *New song in this style...* opens the window at the style |
| 18 | Your song | the song's sections with a row per track and a block where it has notes, and its tempo on a small tempo map |

- **"Your song"** follows the song (a `ChangeListener`) and gives the rules in *Theory*.
- **Opens from:** the window's *What is <style>?* and *Explain this song*; the Theory Guides window; the Song structure
  guide (each typical structure's text ends with the styles that have a template on it, and *File > New Project from
  Style...*).
- **Theory Guides entry:** area *Writing songs* (`Area::songwriting`), name "Styles of music", summary "Fourteen styles from
  ballad to drum & bass: their tempo, groove, chords, sounds and structure, a song to start from in each, and how your
  song compares.", opens from "File > New Project from Style... and the Song structure guide", keywords "style genre
  template pop ballad dance rock hip-hop hiphop boom bap trap house techno trance drum bass dnb jungle lofi lo-fi
  chillhop blues shuffle reggaeton dembow afrobeats afrobeat film score cinematic groove four-on-the-floor backbeat
  half-time 808 909 new project", topics from `stylesguide::allTopics`. On *Start here* after Song structure (decision 6).

## Code
Recipes: *A theory guide* (copy the lyrics guide), *A command or menu item*, *A window*, *Checking in the real app*.

- **Namespaces:** `gen::styles` is taken by ChordStyles. The style table, the plan and the build go in
  `src/generate/SongTemplates.{h,cpp}` (`gen::templates`); the guide's content in `src/generate/StylesGuide.{h,cpp}`.
- **Concepts:** a *style* is one row of data (name, structure name, tempo range and default, mode and three keys,
  progressions per role with alternatives, bars per chord, chord, bass and pad style ids, drum kit, drum parts with their
  patterns, family, return preset, send levels). A *song plan* is the pure result of (style, key, tempo, seed, write
  notes): tempo, key, sections, chords, parts with clips and notes, levels, sends, effects by starting point name. It is
  plain data, so the tests check it without an engine. *Build* turns a plan into an empty edit through engine functions
  only.
- **Rules:** the build starts from `resetEdit()`. It calls only engine functions (`addTrack`, `setBuiltInSound`,
  `addMidiClip`, `addPlugin`, `createBus`, `addReturn`, `setSend`, `setTempo`, `setKey`, `setSections`, `setChords`),
  writes notes under `getLock()` and calls `notesChanged()` once, as `ExampleSong.cpp` does. A starting point that can't
  be found by name is a test failure, never a silent default.
- **Threading:** all on the message thread; rule-based building takes milliseconds, so no background job.
- **New project:** `ProjectManager` gets a "new project made by this function" path: ask to save, `replaceEdit` (reset,
  mark it saved as an empty untitled project, build), so the result is unsaved. The window keeps the project text it last
  made, to replace it without asking while it is unchanged.
- **One painter** for the style card, the tempo map and the arrangement rows, shared by the guide, the window and the PDF.
- **Risky:** the summed level of up to eight tracks. A test renders each style's loudest section offline and checks the
  master's peak; tune the fader starting points if it fails (record it in *Changes while building*).

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | Typical tempos: ballads 60-80 BPM, hip-hop 80-100, pop and rock 100-130, house 120-128, techno 125-135, trance 136-140, D&B 170-175, trap written at about 140 with half-time drums | DawIt's rhythm guide (`RhythmGuide.cpp`), which the templates agree with | typical |
| 2 | Drum & bass is usually 160-180 BPM, mostly 170-180 since about 1996 | Wikipedia, *Drum and bass* | typical |
| 3 | Trap is programmed at about 130-150 BPM, 140 the classic; the clap on beat 3 makes it feel half as fast; hi-hat rolls in 16ths, 32nds and triplets | samplefocus.com *Everything you need to know about 140 BPM*; RouteNote *Exploring trap BPMs* | typical |
| 4 | Lo-fi hip-hop: about 70-90 BPM, many tracks 75-85 | violetrecording.com *What is lofi music*; modeaudio.com *LoFi Hip Hop: 5 production essentials* | typical |
| 5 | Reggaeton: about 85-100 BPM | orphiq.com *What is reggaeton*; beatstorapon.com reggaeton | typical |
| 6 | Afrobeats: average 114 BPM in 2025 (Spotify); median 110, common 99-115 (Mixgraph) | Spotify Newsroom (tempo); mixgraph.io *BPM for afrobeats* | typical |
| 7 | Blues: slow blues around 55-75 BPM, shuffles mostly 100-130 | soundbrenner.com *Slow blues shuffle*; audiotool.com genre lexicon *Blues* | typical |
| 8 | Four on the floor: the kick on every beat; from 1970s disco, used across dance music | Wikipedia, *Four on the floor (music)* | exact (definition) |
| 9 | House: clap on 2 and 4, open hi-hats on the off-beats | Attack Magazine *Beat dissected* series | typical |
| 10 | Backbeat: the snare on beats 2 and 4 | common knowledge (rock and pop drumming) | exact (definition) |
| 11 | Boom bap: named after the kick (boom) and snare (bap); late 1980s to early 1990s East Coast; kick on 1 and 3, snare on 2 and 4, swung on samplers such as the Akai MPC | Wikipedia, *Boom bap* | typical |
| 12 | D&B two-step: kick on 1 and the "and" of 3, snare on 2 and 4 | MusicRadar *How to program 6 jungle and drum 'n' bass grooves*; bonedo.de *Drum & Bass Basics* | typical |
| 13 | D&B came from the UK jungle scene in the early 1990s; the Amen break is its most-used break | Wikipedia, *Drum and bass* | exact (history) |
| 14 | Dembow: a 3+3+2 (tresillo) rhythm; named after Shabba Ranks's "Dem Bow" (1990) | Wikipedia, *Dembow beat* | exact (history) |
| 15 | Tresillo in sixteenths (onsets 0, 3, 6 of 8) twice per bar plus a kick on every beat gives the reggaeton rhythm, so the snare's own hits fall on sixteenths 3, 6, 11, 14 (0-15) | arXiv 2109.10256 (tresillo in popular music); derived: 8 + 3 = 11, 8 + 6 = 14 | exact (derived) |
| 16 | Afrobeats: West African pop from Nigeria, Ghana and the UK in the 2000s-2010s; not Afrobeat (Fela Kuti, 1960s-70s); often uses a 3-2 or 2-3 clave | Wikipedia, *Afrobeats* | exact (history), typical (clave) |
| 17 | 3-2 son clave in one bar of sixteenths: onsets 0, 3, 6, 10, 12 | Wikipedia, *Clave (rhythm)* | exact |
| 18 | House began in Chicago in the early 1980s, its name usually traced to the club the Warehouse | Wikipedia, *House music* | exact (history) |
| 19 | Techno began in Detroit in the mid-1980s | Wikipedia, *Techno* | exact (history) |
| 20 | Trance took shape in the early 1990s, notably in Germany | Wikipedia, *Trance music* | exact (history) |
| 21 | Trap began in Southern US hip-hop (Atlanta) in the early 2000s | Wikipedia, *Trap music* | exact (history) |
| 22 | Reggaeton took shape in Puerto Rico in the 1990s, from dancehall, hip-hop and Latin music | Wikipedia, *Reggaeton* | exact (history) |
| 23 | Roland TR-808 (1980), TR-909 (1983) | DawIt Drums' starting points (`BuiltInDrums.cpp`) | exact |
| 24 | Guitar standard tuning is E A D G B E, so E, A, D and G keys let open strings ring | derived from the tuning | typical |
| 25 | A shuffle splits each beat in three and plays the first and third: long-short | common knowledge (Wikipedia, *Shuffle rhythm*) | exact (definition) |
| 26 | One bar of 4/4 lasts 240 / BPM seconds: 3.43 s at 70, 1.71 s at 140 | derived | exact |
| 27 | Song lengths: bars x 240 / BPM, rounded to the second (table below) | derived | exact |
| 28 | Drum bus glue: DawIt Compressor's Drum bus, 2:1, 30 ms attack, 150 ms release; a send at -10 dB is about a 20% insert mix (20 log10 tan 18 degrees = -9.76 dB) | DawIt's routing guide and compressor (`RoutingGuide.cpp`, `BuiltInCompressor.cpp`) | exact |
| 29 | The return starting points cut the reverb's lows at 200 Hz (room, hall) and 250 Hz (plate) | `BuiltInReverb.cpp` | exact |
| 30 | Which parts play in which section, send levels, faders, keys, patterns of afrobeats, hip-hop, lo-fi and film | DawIt's own choices | opinion |

### Topic texts
Each style topic's summary is its one-line description in the window too. Chord names are in the style's default key.

**1. Styles of music.** Summary: "What makes a pop song sound different from a techno track."

A style of music (a genre) is a set of habits that musicians share. Nobody enforces them, but listeners recognise them within seconds. Six things set most styles apart:

- Tempo: how fast the beat is. A ballad moves at 60 to 80 BPM, house at 120 to 128, drum & bass at 170 to 175.
- Groove: what the drums play. A kick on every beat is house and techno. A snare on beats 2 and 4 is rock and pop. One clap on beat 3 of every bar is trap.
- Harmony: the key and the chords. Trap, techno and trance are often in minor keys; many pop songs and the blues are in major. Some styles change chords every bar, others ride one loop for the whole song.
- Sounds: an organ and a walking bass say blues; chords chopped into fast sixteenths say trance.
- Structure: verses and choruses in pop, long 16- and 32-bar phrases in club music, one 12-bar pattern in the blues.
- The mix: how loud the drums are, how much reverb, how wide.

The picture shows each style's usual tempo range, with your song's tempo as a red line. The ranges overlap: a song at 96 BPM could be hip-hop, reggaeton or afrobeats. What tells them apart is the groove and the sounds, not the tempo alone.

These are habits, not rules. Most songs mix styles, and many famous songs break a habit of their own style on purpose. Use a template to hear the habits, then change whatever you like.

Every style below has a template. File > New Project from Style... makes a whole song in it: tempo, key, sections, chords, drums, bass and effects. A template is a sketch to start from, not a finished song: the melody is yours to write.

**2. The parts of a song.** Summary: "Drums, bass, chords, pad and melody, and who plays where."

Almost every template has the same five parts, and each has a job:

- Drums keep time and set the groove. Each drum has its own track (Kick, Snare, Hats), so you can see, mute and mix each one.
- Bass joins the drums to the harmony. It usually plays the root of each chord (the note the chord is named after), low down.
- Chords fill the middle. The same chords played another way change the style: held, broken into an arpeggio, or chopped into short stabs.
- Pad: long, soft chords behind everything. They make a section sound fuller without adding rhythm.
- Melody: the tune, sung or played. A template leaves its clips empty for you: write it in the piano roll, or let Generate write one.

Not every part plays all the time. Most of a song's energy comes from how many parts play. In the picture each row is a part, and each block a section where it plays:

- The intro and outro are thin: the chords alone, or in club music the kick, hi-hats and bass, so a DJ can mix the track in and out.
- The verse brings in the drums and the bass.
- The chorus or drop has everything.
- A breakdown takes the drums and bass out, so the next drop hits harder.
- A build-up drops the kick and plays a snare roll that gets faster: quarter notes, then eighths, then sixteenths.

This follows the energy curve of the Song structure guide, from 1 for a calm outro to 5 for a chorus or drop. If your chorus doesn't feel bigger than your verse, count the parts playing in each.

**3. Drum bus and reverb return.** Summary: "Why every template groups the drums and shares one reverb."

Every template sets up two things a mix engineer often adds first.

The Drums bus. The drum tracks play into one bus called Drums instead of straight into the master. One fader turns the whole kit up or down without changing its balance. One compressor, at DawIt Compressor's Drum bus starting point, makes the drums move as one instrument: when the snare hits, everything dips a little together. This is glue compression, and it's gentle: 2:1, with a 30 ms attack so every hit keeps its click.

The Reverb return. A return track called Reverb holds one DawIt Reverb at 100% mix. The chords, the pad, the melody and the snare send a copy of themselves to it. Because they share one reverb, they sound as if they were played in one room, and changing the reverb once changes it for all of them. The send levels set how much each gets: the pad -8 dB (a lot, so it floats behind), the melody -10 dB, the chords -12 dB and the snare -14 dB (a touch). A send at -10 dB gives about as much reverb as a 20% mix on the track itself.

The kick, the hi-hats and the bass don't send. Reverb on the lowest sounds makes the low end muddy, and the return's own low cut (200 Hz on a room or hall, 250 Hz on the plate) keeps the lows of the rest out too.

Which reverb: a plate for most styles, because it is bright and dense and suits vocals, snare and keys. A hall for the ballad, techno, trance and the film score, whose long notes can carry a long tail. A short room for rock, hip-hop, lo-fi and blues, which sound best close and dry. These are starting points: change the return's reverb and every part follows.

The bass also gets DawIt Compressor's Bass starting point, which evens out its notes, and the chords get DawIt EQ's Make room for a vocal: a low cut at 150 Hz leaves the lows to the bass and kick, and a 3 dB dip at 3 kHz leaves room for the melody. The Buses, sends and routing guide explains all of this in more detail.

**4. Pop ballad.** Summary: "Slow, open and emotional: piano and strings at 60 to 80 BPM."

A ballad is a slow song that leaves room for the voice and the words: a piano and a singer, with strings joining for the chorus.

Tempo: 60 to 80 BPM; this template uses 70. A bar then lasts 3.4 seconds, so a long note can really ring.

Structure: the pop song (verse, pre-chorus, chorus, a bridge, a last chorus). 64 bars, about 3:39 at 70 BPM.

Chords, in C major: the verse plays the fifties progression, C Am F G. The pre-chorus plays Am F C G, which ends on G and pulls into the chorus. The chorus plays the pop progression, C G Am F: the same four chords in another order, so the chorus feels like an arrival. The bridge plays the Canon, C G Am Em F C F G, eight chords whose bass walks down: something new before the last chorus.

Groove: soft and sparse. The kick on beats 1 and 3, a quiet snare on 2 and 4, and soft hi-hats on every eighth note.

Sounds: piano playing broken chords (Ballad arpeggio), one held bass note per chord, and strings (Sustained) as the pad in the pre-chorus and chorus.

Mix: a hall reverb, because slow, long notes can carry a long tail.

Try: mute the drums in verse 1, as many ballads start, and write the chorus melody higher than the verse's.

**5. Dance pop.** Summary: "Pop made for dancing: a kick on every beat at about 120 BPM."

Dance pop is a pop song with a club beat: verses and a chorus to sing, over a kick on every beat.

Tempo: pop runs from about 100 to 130 BPM, and dance pop sits near the top. This template uses 120: a beat every half second.

Structure: the pop song. 64 bars, about 2:08 at 120 BPM.

Chords, in A minor: the verse plays Am Dm F G (the minor loop). The pre-chorus climbs F G Am. The chorus plays Am F C G (epic minor): three of its four chords are major, so it sounds hopeful though the key is minor. The bridge walks down Am G F E (the Andalusian cadence); E major borrows a note from outside the key and pulls hard back to Am.

Groove: four on the floor, the kick on every beat. A clap on 2 and 4, and an open hi-hat on every off-beat (the "and" between the beats): the "tss" that makes it bounce.

Sounds: chords as short off-beat plucks, a bass jumping octaves on the eighths (Octave disco), and a pad in the pre-chorus and chorus.

Mix: a plate on the clap, chords, pad and melody.

Try: make the chorus melody higher and simpler than the verse's, with fewer words.

**6. Rock.** Summary: "Guitars, a backbeat and a solo, at 100 to 130 BPM."

Rock is built on guitars, bass and drums playing together, often around one strong riff.

Tempo: most rock songs run at about 100 to 130 BPM; punk goes much faster. This template uses 120.

Structure: the rock song: verse and chorus twice, an 8-bar solo where pop has its bridge, and a last chorus. 56 bars, about 1:52 at 120 BPM. Rock songs often play the last chorus twice: copy it if you want more.

Chords, in E major: guitar bands like E, A, D and G, because the guitar's open strings are E, A, D, G, B and E, and in those keys they ring. The verse plays E A B E (the three-chord song) and the chorus E B C#m A (the pop progression). The solo plays over the verse chords, so it sounds connected to the song.

Groove: the backbeat, the snare on beats 2 and 4. The kick on 1, 3 and the "and" of 3, and the hi-hats on every eighth note, the beats a little louder.

Sounds: power chords on the eighths (the root, the fifth and the octave, no third) and the bass on the root in eighths, locked to the guitar. DawIt has no guitar sound yet, so the guitar part plays on the Pluck sound: treat it as a sketch, or put a guitar plugin on the Chords track.

Mix: a short room, so the band sounds close and played in one space.

Try: write a riff for the intro and bring it back between the sections.

**7. Hip-hop.** Summary: "Boom bap: a laid-back beat for rapping, 80 to 100 BPM."

Hip-hop is built for the voice: a beat that loops, with room for rapped verses and a hook to sing along to. This template is boom bap, the sound of East Coast hip-hop in the late 1980s and early 1990s. The name copies the sound of the kick (boom) and the snare (bap).

Tempo: 80 to 100 BPM; this template uses 90.

Structure: hooks and verses: an intro, the hook, then 16-bar verses between hooks. 64 bars, about 2:51 at 90 BPM.

Chords, in C minor: one loop for the whole song, Cm Fm Ab Bb (the minor loop). That's normal in hip-hop: the sections differ in what plays, not in their chords.

Groove: the snare on 2 and 4; the kick on 1, on the last sixteenth of beat 2 and on the "and" of 3; hi-hats on the eighths. This is one common pattern. Real boom bap swings: the off-beat notes come a little late, as the samplers it was made on (such as the Akai MPC) could do. DawIt's patterns are straight for now: drag the off-beat hi-hats a little later in the piano roll and hear the difference.

Sounds: DawIt Drums' Boom bap kit, jazz piano chords (Jazz comping) where a producer would use a sample from a record, and one held bass note per chord.

Mix: a short room keeps it dry and close.

Try: mute the drums for the last bar before each hook, so the hook hits harder.

**8. Trap.** Summary: "808s, hi-hat rolls and a half-time clap, written at about 140 BPM."

Trap began in the hip-hop of Atlanta and the American South in the early 2000s and has spread into pop. Its sound: a deep bass, a clap once a bar, and fast hi-hats with rolls.

Tempo: written at 130 to 150 BPM, here 140, but it feels half as fast, 70, because the clap plays only once per bar, on beat 3. This is half time (see the rhythm guide). The fast tempo gives the hi-hats room for quick rolls.

Structure: hooks and verses, as in hip-hop. At 140 BPM a bar goes by in 1.7 seconds, so the 64 bars last only about 1:50: make the verses longer if you need more room.

Chords, in F minor: one dark loop, two bars per chord: Fm Eb Db C (the Andalusian cadence). The last chord, C major, borrows a note from outside the key, which pulls it strongly back to Fm.

Groove: the kick on 1 and on an off-beat or two, the clap on 3, the hi-hats on the eighths. At the end of every second bar the hi-hats roll: 8 hits in one beat (32nd notes).

Sounds: DawIt Drums' 808-style kit, a pad for the chords, and a bass holding each root. In trap the bass is often a long 808 kick tuned to the song's notes (DawIt Drums' Trap: a long 808 kit). DawIt Drums plays one pitch per drum, so this template keeps the bass on its own track.

Mix: a plate on the clap, the chords and the melody.

Try: change the hi-hat rolls every few bars: triplets, sixteenths, 32nds.

**9. House.** Summary: "Four on the floor at 120 to 128 BPM, from Chicago."

House began in Chicago clubs in the early 1980s; its name is usually traced to a club called the Warehouse. It's music for DJs and dancing: a steady groove where parts come and go.

Tempo: 120 to 128 BPM; this template uses 124.

Structure: phrases of 16 and 32 bars: an intro, the groove, a breakdown, the peak, an outro. 112 bars, about 3:37 at 124 BPM.

Chords, in A minor: one loop all the way, Am Dm F G (the minor loop).

Groove: four on the floor, a clap on 2 and 4, and an open hi-hat on every off-beat. The intro and outro have only the kick, hi-hats and bass, so a DJ can mix them with the records before and after.

Sounds: DawIt Drums' 909-style kit, after the Roland TR-909 drum machine (1983); house piano stabs; a bass jumping octaves; a pad in the breakdown and the peak.

Mix: a plate on the clap, chords, pad and melody.

Try: every 8 bars, add or take away one thing; never change everything at once.

**10. Techno.** Summary: "Hypnotic machine music from Detroit, 125 to 135 BPM."

Techno began in Detroit in the mid-1980s. It's hypnotic: a few sounds repeated, changing slowly over minutes.

Tempo: 125 to 135 BPM; this template uses 130.

Structure: as house: intro, groove, breakdown, peak, outro. 112 bars, about 3:27 at 130 BPM.

Chords, in F minor: few chords, two bars each: Db Eb Fm Fm (rising to home). Many techno tracks use one chord or none at all: the groove and the sounds matter more than the harmony.

Groove: four on the floor; a quieter clap on 2 and 4; closed hi-hats on the beats and open ones on the off-beats.

Sounds: DawIt Drums' Techno kit, an arpeggio going up and down, a bass on the eighths, and a pad in the breakdown and the peak.

Mix: a hall, a big dark space around the arpeggio and the pad.

Try: take the kick out for the last 8 bars before the peak.

**11. Trance.** Summary: "Euphoric chords and a long breakdown, 136 to 140 BPM."

Trance took shape in the club scene of the early 1990s, notably in Germany. It's about emotion: big chords, a soaring melody, and a long breakdown before the drop.

Tempo: 136 to 140 BPM; this template uses 138.

Structure: intro, groove, a 32-bar breakdown without drums, an 8-bar build-up, a 32-bar drop, outro. 120 bars, about 3:29 at 138 BPM.

Chords, in A minor: Am F C G (epic minor), two bars each. These are the four chords of C major's pop progression (C G Am F) in another order: starting on Am makes them bittersweet.

Groove: four on the floor, a clap on 2 and 4, closed hi-hats on the beats and open ones on the off-beats. The build-up drops the kick and plays a clap roll that speeds up: quarter notes for 4 bars, eighths for 2, sixteenths for the last 2.

Sounds: chords chopped into sixteenths (Trance gate), a bass on the eighths, a pad that carries the breakdown. The melody has clips in the breakdown, build-up and drop: the breakdown is where a trance melody is usually introduced.

Mix: a hall: long, wide tails are part of the sound.

Try: play the melody in the breakdown over the pad alone, then an octave higher in the drop.

**12. Drum & bass.** Summary: "Fast broken beats over a deep, slow bass, about 174 BPM."

Drum & bass came out of the UK's jungle scene in the early 1990s: fast drums over deep bass lines that move slowly.

Tempo: about 160 to 180 BPM, most tracks at 170 to 175; this template uses 174. The bass and chords change slowly, so the music feels fast and calm at once.

Structure: long sections: a 32-bar intro for DJs, a 32-bar drop, a breakdown, a build-up and a second drop. 136 bars, about 3:08 at 174 BPM.

Chords, in E minor: Em Am C D (the minor loop), two bars per chord.

Groove: the two-step: the kick on 1 and the "and" of 3, the snare on 2 and 4, hi-hats on the eighths. Jungle and much drum & bass chop up sampled breakbeats instead, most famously the Amen break; the two-step is the simple place to start.

Sounds: a pad for the chords and a deep held bass.

Mix: a plate on the snare, the chords and the melody.

Try: make the second drop different from the first: a new bass rhythm over the same drums.

**13. Lo-fi.** Summary: "Relaxed, dusty beats with jazzy chords, 70 to 90 BPM."

Lo-fi (low fidelity) beats are relaxed instrumental tracks made to sound a little old and worn: soft drums, jazzy chords, the crackle of a record.

Tempo: 70 to 90 BPM; this template uses 80.

Structure: two 8-bar loops, A and B, taking turns. 40 bars: exactly 2:00 at 80 BPM.

Chords, in F major: A plays Gm7 C7 Fmaj7 (the ii-V-I of jazz) and B plays F Dm Bb C (the fifties progression). A seventh chord adds a fourth note on top of a chord, which gives lo-fi its soft, jazzy colour.

Groove: like boom bap but softer: the kick on 1 and around 3, a quiet snare on 2 and 4, soft hi-hats on the eighths. Lo-fi usually swings and sits a little behind the beat; DawIt's patterns are straight for now.

Sounds: DawIt Drums' Boom bap kit, piano chords comping like a jazz pianist, and a held bass.

Mix: a short room, so it stays close and intimate.

Try: mute the drums for two bars before A comes back.

**14. Blues.** Summary: "Twelve bars, three chords and a shuffle."

The blues grew out of African American music in the southern United States, and jazz, rock 'n' roll and rock all borrow from it.

Tempo: from slow blues around 60 to 75 BPM to shuffles of about 100 to 130. This template is a medium shuffle at 100.

Structure: one 12-bar pattern again and again, as sung verses and played solos. 68 bars, about 2:43 at 100 BPM.

Chords, in A major: A A A A, D D A A, E D A E: only the I, IV and V chords. Blues players often make every chord a seventh chord (A7, D7, E7) for a grittier sound.

Groove: the shuffle. Each beat splits in three, and the hi-hat plays the first and the third part: long-short, long-short. The kick on 1 and 3, the snare on 2 and 4.

Sounds: organ chords held for each bar, and a walking bass that steps through the chord on every beat. Both play on the beat, so they fit the shuffle.

Mix: a short room.

Try: sing a line, sing it again, then answer it: the classic plan of a blues verse.

**15. Reggaeton.** Summary: "The dembow beat at about 95 BPM, from Puerto Rico."

Reggaeton took shape in Puerto Rico in the 1990s, from Jamaican dancehall, hip-hop and Latin music.

Tempo: about 85 to 100 BPM; this template uses 95.

Structure: the pop song. 64 bars, about 2:42 at 95 BPM.

Chords, in D minor: the verse plays Dm Gm Bb C (the minor loop), the pre-chorus climbs Bb C Dm, the chorus plays Dm Bb F C (epic minor), and the bridge walks down Dm C Bb A.

Groove: the dembow, named after Shabba Ranks's 1990 song "Dem Bow". The kick plays every beat. The snare plays on the "a" of 1, the "and" of 2, the "a" of 3 and the "and" of 4. With the kick on 1 and 3, each half bar is 3 + 3 + 2 sixteenths: the tresillo rhythm, heard all over Latin music.

Sounds: DawIt Drums' 808-style kit, off-beat plucked chords, a held bass, and a pad in the pre-chorus and chorus.

Mix: a plate on the snare, chords, pad and melody.

Try: keep the dembow all the way through and change only what plays around it.

**16. Afrobeats.** Summary: "West African pop with a bouncing groove, 95 to 120 BPM."

Afrobeats is the pop music of West Africa, grown in Nigeria, Ghana and the UK in the 2000s and 2010s. It isn't the same as Afrobeat (without the s), the big-band style of Fela Kuti in the 1960s and 70s.

Tempo: about 95 to 120 BPM, most songs around 100 to 115. This template uses 108.

Structure: the pop song. 64 bars, about 2:22 at 108 BPM.

Chords, in G major: the verse plays Em C G D, the pre-chorus G Em C D, the chorus G D Em C (the pop progression) and the bridge G C D G.

Groove: the drums are the heart of afrobeats, and grooves vary a lot; this is one common pattern. A rim (a soft snare here) plays a clave, 3 + 2 hits: on 1, the "a" of 1, the "and" of 2, the "and" of 3 and on 4. The kick avoids four on the floor: on 1, the "and" of 2 and the "and" of 3. A shaker (a closed hi-hat here) plays every sixteenth, louder on the off-beats.

Sounds: funky sixteenth-note guitar chords (on the Pluck sound), a bass on the root and the fifth.

Mix: a plate on the rim, chords and melody.

Try: add a percussion track that answers the clave; afrobeats songs often layer several.

**17. Film score.** Summary: "One growing arc for strings, drums and a theme."

Film music serves the picture: it has no fixed tempo or form, and it grows and falls with the story. This template is one big arc: a quiet start, a theme, a development that adds layers, a climax, and rest.

Tempo: whatever the scene needs. This template uses 80, slow enough for long notes.

Structure: one arc. 64 bars, about 3:12 at 80 BPM.

Chords, in D minor, two bars each: the theme and the climax play Dm Bb F C (epic minor). The development plays Bb C Dm (rising to home): two major chords climbing to the minor home chord, which sounds heroic.

Groove: low and high drums, like the big drum ensembles of action scenes, only in the development and the climax, with a cymbal at the start of the climax. The theme has no drums at all.

Sounds: held strings for the chords, a held bass, and the melody track for the theme.

Mix: a hall: orchestras record in big rooms.

Try: play the theme simply on one instrument first, then again with more layers each time.

**18. Your song.** Summary: "Your tempo, chords and parts against the styles."

Static opening: "How your song compares with the styles' habits. They are habits, not rules: use this to decide on purpose." Then the rules below, in order.

**Tooltips and the window's texts** are in *What the user does and sees*; the style summaries above are also the window's line under the style's name.

### Example data
Patterns, one bar of 16 sixteenths (0-15) unless said; X = velocity 0.9, x = 0.6, o = 0.4; O = open hi-hat (note 46) on
the Hats track; notes: kick 36, snare 38, clap 39, closed hat 42, open hat 46, low tom 45, high tom 50, cymbal 49.

| Style | Drum tracks | Pattern |
|---|---|---|
| Pop ballad | Kick, Snare, Hats | `X.......X.......` / `....x.......x...` / `o.o.o.o.o.o.o.o.` |
| Dance pop | Kick, Clap, Hats | `X...X...X...X...` / `....X.......X...` / `..O...O...O...O.` |
| Rock | Kick, Snare, Hats | `X.......X.x.....` / `....X.......X...` / `X.x.X.x.X.x.X.x.` |
| Hip-hop | Kick, Snare, Hats | `X......x..X.....` / `....X.......X...` / `x.x.x.x.x.x.x.x.` |
| Trap (2 bars) | Kick, Clap, Hats | `X......X........ X.........X.....` / `........X.......` both bars / `x.x.x.x.x.x.x.x.` then bar 2 `x.x.x.x.x.x.` + 8 hits at beats 3.0, 3.125 ... 3.875, velocity 0.4 rising to 0.8 |
| House | Kick, Clap, Hats | `X...X...X...X...` / `....X.......X...` / `..O...O...O...O.` |
| Techno | Kick, Clap, Hats | `X...X...X...X...` / `....x.......x...` / `x.O.x.O.x.O.x.O.` |
| Trance | Kick, Clap, Hats | `X...X...X...X...` / `....X.......X...` / `x.O.x.O.x.O.x.O.` |
| Drum & bass | Kick, Snare, Hats | `X.........X.....` / `....X.......X...` / `x.x.x.x.x.x.x.x.` |
| Lo-fi | Kick, Snare, Hats | `X.......x.x.....` / `....x.......x...` / `o.o.o.o.o.o.o.o.` |
| Blues (12 triplet steps) | Kick, Snare, Hats | `X.....X.....` / `...X.....X..` / `X.xX.xX.xX.x` |
| Reggaeton | Kick, Snare, Hats | `X...X...X...X...` / `...X..X....X..X.` / `o.o.o.o.o.o.o.o.` |
| Afrobeats | Kick, Rim (38), Shaker (42) | `X.....X...X.....` / `x..x..x...x.x...` / `ooxoooxoooxoooxo` |
| Film score | Low drum, High drum, Cymbal | `X.....x.X.....x.` / `....x.......x.xx` / X on step 0 of the climax's first bar only |

**The style table** (keys: the first is the default; progressions: verse / chorus / lift / contrast, "or" an alternative
the seed may pick; chords, bass and pad are ChordStyles names; return: plate, room = short room, hall):

| Style | Structure | BPM min/default/max | Keys | Progressions | Bars per chord | Chords / bass / pad | Kit | Family | Return |
|---|---|---|---|---|---|---|---|---|---|
| Pop ballad | Pop song (verse-chorus) | 60/70/80 | C, G, D major | Fifties / Pop / Sad pop / Canon | 1 | Ballad arpeggio / Held root / Sustained | Tight pop | song | hall |
| Dance pop | Pop song | 100/120/130 | A, C, E minor | Minor loop / Epic minor / Rising to home / Andalusian | 1 | Off-beat pluck / Octave disco / Pad | Tight pop | song | plate |
| Rock | Rock song (with a solo) | 100/120/130 | E, A, G major | Three-chord / Pop / - / (solo: verse) | 1 | Power chords / Root eighths / - | Tight pop | song | room |
| Hip-hop | Hip-hop and trap | 80/90/100 | C, D, A minor | Minor loop or Epic minor, all roles | 1 | Jazz comping / Held root / - | Boom bap | song | room |
| Trap | Hip-hop and trap | 130/140/150 | F, C, G minor | Andalusian or Minor loop, all roles | 2 | Pad / Held root / - | Init: an 808-style kit | song | plate |
| House | Techno and house | 120/124/128 | A, D, G minor | Minor loop or Epic minor, all roles | 1 | House stabs / Octave disco / Pad | 909-style | club | plate |
| Techno | Techno and house | 125/130/135 | F, A, D minor | Rising to home or Minor loop, all roles | 2 | Arpeggio up and down / Root eighths / Pad | Techno | club | hall |
| Trance | Trance | 136/138/140 | A, D, E minor | Epic minor or Minor loop, all roles | 2 | Trance gate / Root eighths / Pad | 909-style | club | hall |
| Drum & bass | Drum & bass | 160/174/180 | E, F, A minor | Minor loop or Epic minor, all roles | 2 | Pad / Held root / - | Tight pop | club | plate |
| Lo-fi | Lo-fi and beats | 70/80/90 | F, C, Eb major | Jazz turnaround / - / - / Fifties or Sad pop | 1 | Jazz comping / Held root / - | Boom bap | song | room |
| Blues | Twelve-bar blues | 60/100/130 | A, E, G major | Twelve-bar blues, all roles | 1 | Organ chords / Walking bass / - | Tight pop | song, shuffle | room |
| Reggaeton | Pop song | 85/95/100 | D, A, E minor | Minor loop / Epic minor / Rising to home / Andalusian | 1 | Off-beat pluck / Held root / Pad | Init: an 808-style kit | song | plate |
| Afrobeats | Pop song | 95/108/120 | G, C, D major | Sad pop / Pop / Fifties / Three-chord | 1 | Funk sixteenths / Root and fifth / - | Tight pop | song | plate |
| Film score | Film and ambient | 60/80/120 | D, C, E minor | Epic minor / Epic minor / - / Rising to home | 2 | Sustained / Held root / - | Init: an 808-style kit | film | hall |

**Who plays where** (P = plays; R = the build-up roll on the snare or clap; M = an empty melody clip):

| Family: section types | Kick | Snare/clap | Hats | Bass | Chords | Pad | Melody |
|---|---|---|---|---|---|---|---|
| song: intro, interlude, outro | - | - | - | - | P | - | - |
| song: verse, phrase, bridge, solo | P | P | P | P | P | - | M |
| song: pre-chorus, post-chorus, chorus, drop | P | P | P | P | P | P | M |
| song and club: breakdown | - | - | - | - | P | P | M (song: -) |
| song and club: build-up | - | R | P | - | P | P | M (club only) |
| club: intro, outro | P | - | P | P | - | - | - |
| club: phrase or verse before the first breakdown | P | P | P | P | P | - | - |
| club: phrase after a breakdown, drop, chorus | P | P | P | P | P | P | M |
| film: intro, outro | - | - | - | - | P | - | - |
| film: verse | - | - | - | P | P | - | M |
| film: bridge | low, high | | | P | P | - | M |
| film: chorus | low, high, cymbal | | | P | P | - | M |

The roll, over a build-up of N bars: quarter notes in the first half, eighths in the next quarter, sixteenths in the last
quarter; velocity rising evenly from 0.4 to 1.0. For 8 bars: 4 x 4 + 2 x 8 + 2 x 16 = 64 hits.

**Song lengths** (derived, `structure::formatDuration`): pop song 64 bars at 70 = 3:39, at 120 = 2:08, at 95 = 2:42, at
108 = 2:22; rock 56 at 120 = 1:52; hip-hop 64 at 90 = 2:51, at 140 = 1:50; techno and house 112 at 124 = 3:37, at
130 = 3:27; trance 120 at 138 = 3:29; drum & bass 136 at 174 = 3:08; lo-fi 40 at 80 = 2:00; blues 68 at 100 = 2:43;
film 64 at 80 = 3:12.

### "Your song" rules
In this order; each says something or stays quiet.
1. **Tempo** (always): the styles whose min-max holds the tempo. "At 96 BPM your song is in the usual range of hip-hop
   (80 to 100), blues (60 to 130), reggaeton (85 to 100), afrobeats (95 to 120) and the film score (60 to 120)." None:
   try half and double the tempo. "No style here usually sits at 155 BPM. Half of it, 77.5, is in the range of the pop
   ballad, lo-fi, blues and the film score: if your snare or clap plays only once a bar, listeners feel it at that half
   tempo." Neither: "No style here usually sits at
   N BPM. That's fine: these are habits, not rules."
2. **Structure:** sections matching a known structure (`structure::findStructureName`) that templates use: "Your sections
   follow the Pop song structure, which the pop ballad, dance pop, reggaeton and afrobeats templates use." No sections:
   "No sections yet: File > New Project from Style... makes a song with them, or mark them on the Sections row." Quiet
   when they match no template's structure.
3. **Verse and chorus chords** (the first verse and the first chorus or drop, each with chords; repeats merged): the same
   chords in the same order: "Your verse and chorus play the same chords (Am F C G). That's normal in hip-hop and club
   music, where one loop runs through the song. In a pop song, try the chorus on the same chords in another order, such
   as F C G Am, so it feels like an arrival." The same chords in another order: "Your verse plays vi-IV-I-V and the
   chorus I-V-vi-IV: the same chords in another order, so the chorus feels like an arrival." Different chords: "Your
   chorus moves to other chords than the verse, so it sounds like a new place." Quiet when either has no chords.
4. **Energy:** count the tracks (not buses or returns) with notes or audio in the first verse or phrase, and in the
   first chorus or drop after it. Chorus fewer: "Your chorus has 3 parts playing and the verse 5. A chorus usually has
   the most energy: try bringing a part in at the chorus, or leaving one out of the verse." Chorus more: "Your chorus has
   6 parts and the verse 4: it lifts." Equal or missing: quiet.
5. **Drums:** 2 or more drum tracks (`routingguide::getFamily` = drums) not all in one bus: "Your 3 drum tracks play
   straight into the master. Put them on a Drums bus with DawIt Compressor's Drum bus starting point: the routing guide's
   Your song's routing does it in one click." In one bus with no compressor on it: "Your Drums bus has no compressor:
   try DawIt Compressor's Drum bus starting point for glue." Otherwise quiet.
6. **Reverb:** no return and 3 or more tracks with notes: "Your song has no shared reverb. Track > New Return Track >
   Reverb makes one; send the chords, pad and melody to it, and the snare a little." A return with DawIt Reverb that no
   track sends to: "Nothing sends to Reverb yet: turn up the Reverb knob of a track in the mixer." Otherwise quiet.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Every style: min <= default <= max | as the style table | exact |
| Defaults inside the rhythm guide's ranges | ballad 70 in 60-80, hip-hop 90 in 80-100, rock 120 and dance pop 120 in 100-130, house 124 in 120-128, techno 130 in 125-135, trance 138 in 136-140, D&B 174 in 170-175 | exact |
| `formatDuration` of each template | the song lengths above | exact string |
| House kick per bar | beats 0, 1, 2, 3 | 1e-6 |
| Reggaeton snare per bar | beats 0.75, 1.5, 2.75, 3.5 | 1e-6 |
| Afrobeats rim per bar | beats 0, 0.75, 1.5, 2.5, 3.0 | 1e-6 |
| Trap clap | beat 2.0 of every bar only | 1e-6 |
| Trap roll in bar 2 | 8 hits at 3.0 + k x 0.125 | 1e-6 |
| Blues hats per beat | offsets 0 and 2/3 | 1e-6 |
| Drum & bass | kick 0 and 2.5, snare 1 and 3 | 1e-6 |
| 8-bar build-up roll (trance) | 64 hits; velocity never falls; first 0.4, last 1.0 | 0.01 |
| Trance drop chords, seed 0 | Am Am F F C C G G, repeating | exact |
| Ballad chorus, seed 0 | C G Am F | exact |
| Lo-fi phrase A, seed 0 | Gm7 C7 Fmaj7 Fmaj7 | exact |
| Last bar of every template | the key's home triad | exact |
| Bar of 4/4 | 240/70 = 3.43 s, 240/140 = 1.71 s | 0.01 s |
| Send levels | Chords -12, Pad -8, Melody -10, Snare/Clap/Rim -14 dB | exact |
| Loudest section rendered offline, every style | master peak below -1 dBFS | - |

## Steps
**Group A, engine and generate** (`./build.sh --test` at step 3):
- [x] 1. `SongTemplates.{h,cpp}`: the style table, drum patterns, role and energy rules, and the plan from (style, key,
  tempo, seed, write notes). Add both new generate files to `DawIt` and `DawItTests`. Tests: `runSongTemplateTests` (data
  and plan rows of *Tests*). Check: the tests list 14 styles. Done when every starting point, structure, progression and
  chord style named in the table is found.
- [x] 2. Build a plan into the engine: tracks, clips, DawIt Drums kits, the Drums bus, the Reverb return and sends, the
  other effects. Tests: bus, return, sends, effects, energy, write-notes-off, save-and-load, the offline render. Done
  when all 14 styles build and pass.
- [x] 3. `StylesGuide.{h,cpp}`: topics, texts, example data for the pictures, the "your song" analysis; `gen::guides`
  entry, `Guide::styles`, *Start here*; the structure guide's line naming the templates per structure. Tests: every
  topic has text and example data, the six rules on made-up songs, search finds "dembow", "lofi", "808", "trap".

**Group B, UI** (`./build.sh --test` at step 7):
- [x] 4. The shared painter (tempo map, mini arrangement, drum grid, the routing boxes from a plan), in `src/ui`.
- [x] 5. `StylesExplainer`: the guide window, *Play the groove*, *New song in this style...*, PDF export; owned by
  `MainComponent` like the routing guide, with cases in `openGuide` and `getPrintGuide`. Check: `--export-guides` passes.
- [x] 6. `NewFromStyleWindow` and the command `newFromStyleCommand` (File menu, Shift+Cmd+N); `ProjectManager`'s
  new-project-from-a-function path; the settings keys. Check: by hand in step 8.
- [x] 7. `ScreenshotRunner`: a song `"style:<id>"` (builds that template, seed 0) and a scene `newFromStyle` (the window
  open, a style chosen by `"style"`).
- [x] 8. Real-app check, one run: (a) `song "style:trap"`, the arrangement with the Drums bus open and chords open;
  (b) same song, mixer panel (the Reverb sends); (c) `style:house`, Signal Flow scene; (d) scene `newFromStyle`, style
  lofi; (e) guide `styles` at *Trap* and at *Reggaeton* (drum grids), at *Styles of music* and at *Your song* on the pop
  example song. Look for: names cut off, the bus holding exactly the drums, sends on the right strips, blocks matching
  the energy table, 12 steps on blues.
- [x] 9. Docs: README *Features* bullet (File > New Project from Style..., the window, the guide), *Tests* line, *Code
  layout* lines for the four new file pairs; the README's Theory Guides count; FEATURE_IDEAS status *Partly done* with
  what's left (below); docs/recipes.md: a line under *A command or menu item* or a short *A song template* note saying a
  template builds only through engine functions; docs/architecture.md: `gen::templates` and the guide in the guides list.

## Tests
`runSongTemplateTests` and `runStylesGuideTests` in `tests/EngineTests.cpp`:
- Data: 14 styles; every structure name in `structure::getStructures`; every progression in `getCommonProgressions` of
  the style's mode; every chord style in `styles::findStyle`; every kit in `drums::getPresets`; every return preset in
  `verb::getPresets`; DawIt Compressor *Drum bus* and *Bass*, DawIt EQ *Make room for a vocal* exist.
- Every row of *Theory > Test numbers*; blues chord and bass notes all start on whole beats.
- Plan: sections equal `structure.toSections (0)`; a chord on every bar, its root in the key; the same seed twice gives
  identical plans; seeds 0-19 give at least 2 different keys per style.
- Energy: for each style and section, each part has notes exactly where the table says; melody clips there, empty.
- **Drum bus and reverb send, every style (the required parts):** one bus named Drums holding exactly the drum tracks
  (2 or 3), each with DawIt Drums at the kit's settings, and on the bus one DawIt Compressor at the *Drum bus* settings;
  one return named Reverb with DawIt Reverb at the style's return settings (mix 100%); sends at the listed levels,
  post-fader, from at least 2 tracks; none from kick, hats, bass or the bus. Bass and Chords effects at their starting
  points. Write notes off: the same tracks, bus, return and sends, every clip empty.
- Save and load: a built template's project text loads back to the same text. Offline render as in *Test numbers*.
- Guide: 18 topics with name, summary and text; example data per style; rules 1-6 on made-up songs (96 BPM; 155 BPM;
  same verse and chorus chords; rotated chords; 5 parts in the verse and 3 in the chorus; 3 drum tracks outside a bus;
  no return; a return nobody sends to); the Theory Guides entry lists the topics; the search words of step 3.

## Website hand-off
- Features page: a section *Start from a style*: the window, a template's arrangement, and the guide's style card.
- Main page: worth a step for beginners ("Pick a style, hear a whole song"); the guide picker's *Writing songs* area
  gains *Styles of music*.
- Screenshots: the window with Lo-fi chosen; the Trap template's arrangement with the Drums bus and chords open; the
  mixer showing the Reverb sends; the guide at *Reggaeton* (dembow grid) and at *Styles of music* (tempo map).
- How it's made: no change.

## Left for later
- Insert a template into an existing song or after its last section: needs *Undo and redo*.
- AI fill and *Shuffle* per part with the part generators; mood and length choices in the window.
- More styles: jazz (AABA), folk (strophic), big-room EDM, dubstep; modes such as dorian for lo-fi (the progressions are
  written for major and minor).
- Swing and a laid-back feel; fills and a crash at section starts; per-section drum variations; the step editor's
  pattern library sharing these patterns.
- A guitar sound for rock and afrobeats; an 808 bass tuned per note.
- Remembering in the project which style a song came from.

## Changes while building
- The compressor's logarithmic parameters now snap to 0.01 units. The template save-and-load check exposed a
  few-millionths drift in a 20 ms value after deserialisation; snapping keeps project text byte-for-byte stable.
