# The kinds of reverb, explained and shown: room, hall, chamber, plate, spring

Status: Committed
Idea: [The kinds of reverb, explained and shown: room, hall, chamber, plate, spring](../FEATURE_IDEAS.html#the-kinds-of-reverb-explained-and-shown-room-hall-chamber-plate-spring)

## Goal
*What is reverb?* gets a page per kind (room, hall, chamber, plate, spring, then digital and convolution, and the effects
built on them), each with a moving picture of the real thing, what one clap becomes, and a clap through that kind, so a
beginner can tell a plate from a hall by eye and ear. DawIt Reverb gets a **Type** switch that changes how the sound is
made, and the guide a *Your track* page that says which type suits the track.

## Decisions for you
1. **Does Type change the sound?** Options: (a) each type makes the reverb its own way (Plate: no early reflections,
   dense at once; Spring: repeats and chirps; Hall: a slow swell), knobs kept on a switch; (b) a label that loads typical
   knob values; (c) (a) plus a menu item that also loads typical values. **Recommended:** (a), as the idea asks: with (b)
   a "plate" still has walls, which teaches the wrong thing. (c) later.
   Answer: (a), as recommended: each type makes the reverb its own way.
2. **How older projects and new reverbs start.** Options: a type *Digital* that is today's sound sample for sample, the
   default for both; or *Room* as default (old projects change slightly). **Recommended:** Digital: old projects sound as
   before, and the idea lists Digital as a kind. Six types: Digital, Room, Hall, Chamber, Plate, Spring.
   Answer: *Digital* as the default for old projects and new reverbs, as recommended.
3. **How much moving picture in this slice.** (A) all of the idea, also a decay-per-frequency picture per kind; (B) per
   kind one animated drawing of the thing over its response, with the first 100 ms zoomed for the echo density; *Kinds of
   reverb* side by side with *Hear each in turn*; decay per frequency stays on the existing damping page, which follows
   the type; (C) still responses only. **Recommended:** (B): it shows all the idea's text needs; (A)'s extra pictures
   repeat the damping page.
   Answer: (B), as recommended.
4. **Hearing each kind in the guide.** A second network in the processor, fed only by the guide's clap, plays that kind's
   starting point (the user's settings untouched); or text saying "set Type to Plate and clap". **Recommended:** the
   second network: "by ear" is half the point.
   Answer: A second network fed only by the guide's clap, as recommended.
5. **Starting points get a type; *Lead vocal* becomes a Plate** (Early 25% to 0%). Plus two new: *Echo chamber*, *Spring
   guitar*. **Recommended:** yes: a plate is the usual vocal reverb, and the idea's own example.
   Answer: Yes, as recommended.
6. **Single-source facts**, written carefully: spring repeats "about 40 to 80 ms" (one measured spring, one module); EMT
   140 decay "to about 5 seconds" (sources say 5 or 6); Abbey Road's chamber "from the 1930s" (one source says 1931);
   EMT 250 1976 (a vendor history; Mix agrees it was first). Accept, or leave any out.
   Answer: Accepted as written, as recommended.

**Build order:** the singing techniques guide is being built in this checkout. Steps 12 (TheoryGuides.cpp) and 14
(README, FEATURE_IDEAS, docs) wait until it is committed, or are merged by hand with its changes.

## What the user does and sees
- **DawIt Reverb's window** (size unchanged): a first knob group **KIND** with one `Switch`, *Type*. A click opens a menu
  of the six types (current ticked, each with a short line), ending with *What are the kinds of reverb?...*, which opens
  the guide at *Kinds of reverb*. Hover shows the Type topic. Switching keeps every knob.
- On Plate and Spring the *Early* knob is dimmed and disabled, its value kept, its tooltip saying why.
- The explainer's knob texts follow the type where it matters (Size, Decay on Plate and Spring).
- The picture panel (*THE ROOM, FROM ABOVE*) draws the type's thing, moving after a clap; Digital keeps today's room.
- The starting points menu shows each one's type ("Lead vocal (Plate)"); its explainer lists the type first, with why.
- In the guide, a kind's page has *Clap: plate* (etc.); *Kinds of reverb* has *Hear each in turn* (six claps 3 s apart,
  the playing cell outlined).
- Nothing new is remembered between sessions.

## Data and saving
- A choice parameter **Type** in the reverb's parameter state, order fixed for good: Digital 0, Room 1, Hall 2, Chamber 3,
  Plate 4, Spring 5 (new ones appended). Saved in the plugin state; ProjectFile and the format version don't change.
- A state without a type (every older project) loads as Digital: `setStateInformation` sets it explicitly.
- `verb::Settings` carries the type, so presets, `getReturnPreset()` (a Plate) and `PluginActions`' return set-up do too.
- Tests: `checkThroughProject<BuiltInReverb>` with Spring; a state with its type child removed loads as Digital.

## The guide
Pages of *What is reverb?* in reading order (texts in *Theory*):

| # | Page | Picture |
|---|---|---|
| 1-5 | as now (page 5: one sentence fixed) | as now |
| 6 | Kinds of reverb (rewritten) | six cells (Room, Hall, Chamber, Plate, Spring, Digital): name and starting point, response (`drawResponse`), a 100 ms strip with a line per reflection; *Hear each in turn* |
| 7 | Room | `drawRoom`, 20x slower, over the response and strip |
| 8 | Hall | a 2:1 shoebox from above, stage, listener mid-floor, side reflections as rays with ms |
| 9 | Chamber | an irregular room, speaker facing a wall, microphone far corner, rings bouncing |
| 10 | Plate | plate on corner springs, driver, two pickups, damper; rings at 10 kHz, 1 kHz, 100 Hz speeds, 100x slower |
| 11 | Spring | tank, two coils, driver left, pickup right; a pulse there and back, wider and fainter each trip, 20x slower |
| 12 | Digital and convolution | a timeline |
| 13 | Reverb effects | four level-over-time sketches |
| 14-15 | Reverb in a mix; How DawIt Reverb works (plus a paragraph) | as now |
| 16 | Your track (new) | the response as set, the track and type, then the advice lines (as `GateGuide`'s *Your track*) |

- Pages 7-11 have their response under the drawing, and draw and play the kind's **showcase** starting point (Room: Small
  room; Hall: Strings or choir; Chamber: Echo chamber; Plate: Snare plate; Spring: Spring guitar; Digital: Neutral
  start), the clap at mix 50%. Pages 2-5 keep following the processor's settings; page 1 always draws a room.
- `verbtext::drawKind` (next to `drawRoom`) is shared by the pages, the editor's panel and the PDF.
- **Theory Guides entry** (`Guide::reverb`): summary "How a room answers a sound: early reflections and the tail, decay,
  damping, pre-delay, the kinds of reverb (room, hall, chamber, plate, spring, digital) side by side, reverb in a mix, and
  your track."; add search words "chamber spring plate EMT tank convolution gated shimmer reverse kinds types". Not on
  *Start here*. Topics fill in from the pages as now.

## Code
Follows recipes.md *A built-in effect or instrument* (changing DawIt Reverb) and *Topics in an existing guide*.
- **One `verb::Network`, six set-ups.** Digital is today's path, unchanged (step 1 pins it). Room, Hall, Chamber and Plate
  reuse the taps, diffusers and lines with other early patterns, diffusion and feeding, as *How DawIt Reverb works* says
  (the text must match the code). Spring is its own small structure: per side a loop of the round-trip time with a chain
  of stretched first-order all-passes (Välimäki, Parker, Abel 2010), loop gain from `getLoopGain`, the lines' damping, a
  fixed low-pass near 4.5 kHz; right spring 1.12x the left.
- **Invariants:** the knobs mean the same in every type (Size is metres across for the four room-like ones); no output
  before the pre-delay; `withEarly`/`withTail` choose what is output, never what feeds the tail (Hall feeds its tail from
  its taps); Early does nothing on Plate and Spring; every type within 2 dB of Digital on steady noise (the reverb has no
  match-level switch, Mix is its level, so level-matched types keep A/B fair).
- **Threading:** Type is an atomic parameter, re-applied by `processBlock` as now. Every buffer (spring included) is
  sized in `prepare` for the largest type; `setSettings` never allocates. A type change clears the network and fades the
  wet in over 20 ms.
- **Audition:** a second network, fed only by an audition clap (an atomic type, -1 off), processing only while it rings,
  added at the showcase's mix. The main network never hears it.
- **Pure content in `src/engine/ReverbKinds.{h,cpp}`** (both CMake targets): per kind name, summary, text and showcase;
  the name-to-source matcher, reusing routingguide's whole-word matching (move that helper so both share it); the advice;
  the derived numbers as functions. The guide pages take their text from it, so it is tested.
- **Risky, shared by 12 guides:** `fxui::Guide::Page` gets an optional own clap and button text; without them, as today.
- **Host track for *Your track*:** a static lookup in `fxui` that `MainComponent` sets beside the `createEditorFunction`s:
  for a processor, the name and kind (track, bus, return) of the track whose `plugins` hold it, read under
  `Engine::getLock()` on the message thread; empty for a stand-in.

## Libraries and models
None.

## Theory

### Facts
1. Reflection rate in a room: 4 pi c^3 t^2 / V per second (c 343 m/s). Kuttruff, *Room Acoustics*. **Exact.**
2. Derived: 4 pi c^3 = 5.071e8. 60 m3: 845/s at 10 ms; 1,000/s at sqrt(1000 x 60 / 5.071e8) = 10.9 ms. 15,000 m3: 84.5/s
   at 50 ms, 338/s at 100 ms, 1,000/s at 172 ms. 56.6 m3: 10.6 ms. **Exact.**
3. A smooth tail needs about 1,000 echoes/s or more; sharp clicks about 10,000. Schroeder 1962 (JAES, "Natural sounding
   artificial reverberation"); J. O. Smith, *Physical Audio Signal Processing*. **Typical.**
4. Orchestral halls ring 1.7-2.1 s full; Boston Symphony Hall 1.9 s. Beranek (AES Heyser lecture; *Concert Halls and Opera
   Houses*). **Typical**; Boston **exact**.
5. First-reflection gap mid-floor: Boston about 15 ms; good halls under about 25; many 25-35 ms. Beranek; proav.de. **Typical.**
6. Living room 0.4-0.6 s (existing guide); small and live rooms 0.3-1 s. **Typical.**
7. Bill Putnam, Universal Recording, Chicago: the washroom as a chamber on the Harmonicats' *Peg o' My Heart*, 1947, a
   number one. Sound On Sound, "Golden Age reverb chambers". **Exact.**
8. Abbey Road chamber 1931 (Wikipedia). Single source: "from the 1930s".
9. Capitol: four chambers 1956, about 30 ft (9 m) under the car park, four more later; lacquered concrete, no parallel
   surfaces, about 2,000 ft3 (56.6 m3), up to 5 s; still used. SOS (as 7); HiFi News; Wikipedia. **Exact**; 5 s a maximum.
10. Derived (Sabine): 0.161 x 56.6 / 5 = 1.82 m2. **Exact.**
11. EMT 140: EMT, Germany, 1957, by Walter Kuhl of the Hamburg broadcast institute; stereo 140S 1961. Universal Audio,
    "The Origins of EMT Reverb"; MusicTech, "Vintage: EMT 140S". **Exact.**
12. Plate: steel about 0.5 mm, about 2 m2; a driver, a pair of pickups, an absorbing plate for the decay; no distinct
    reflections, a smooth noise-like response; highs travel faster; mode density about constant (a room's grows as f^2).
    Bilbao, DAFX chapter draft 2010 (ccrma.stanford.edu/~bilbao/dafx10/electro.pdf). **Exact**; sizes **typical**.
13. Damper: a fibreglass pad, closer is shorter; under 1 s to about 5 s (some say 6). SOS review of Waves Abbey Road
    Plates; UA (as 11). **Typical.**
14. Derived bending-wave group velocity 2 sqrt(kappa omega), kappa = sqrt(E H^2 / (12 rho (1 - nu^2))); E 200 GPa, rho
    7850, nu 0.3, H 0.5 mm: kappa 0.764 m2/s; 43.8, 138.5, 438 m/s at 100 Hz, 1 kHz, 10 kHz; 2 m in 45.7, 14.4, 4.6 ms. **Exact.**
15. Spring reverb: Laurens Hammond, US patent 2,230,836 (1941, filed 1939), for organs. Parker et al., EURASIP JASP 2011;
    Premier Guitar, "Lords of the springs". **Exact.**
16. Fender 6G15 Reverb unit 1961; reverb in amps from the early 1960s (which was first is disputed). voxac30.org.uk;
    Premier Guitar. **Exact** / **typical**.
17. A spring's echoes are evenly spaced, at twice the driver-to-pickup time. Parker 2011; US patent 8,391,504. **Exact.**
18. Echoes become chirps; the main ones below a transition frequency, usually 2-5 kHz. Bilbao (as 12); Parker 2011; Abel,
    UA WebZine 2006 (one spring: 55 ms at 1 kHz, about 80 ms at 3 kHz). **Typical.**
19. Repeats: one spring 55 ms; a two-spring module 22 and 27 ms one way. UA 2006; Silicon Chip, Jan 2000. **Typical**:
    "about 40 to 80 ms".
20. Tanks: 2-3 springs; decay 1.75-4 s (Accutronics medium 1.75-3, long 2.75-4). UA 2006; Accutronics listings. **Typical.**
21. EMT 250 1976, the first digital reverb; Lexicon 224 at AES 1978, by David Griesinger. Mix, "1978 Lexicon 224"; Vintage
    Digital, "History of digital reverb". **Exact** (224); 1976 single source.
22. FDN: Jot 1991 (existing guide). Spring models: about 100 stretched all-passes in a loop (Välimäki, Parker, Abel, JAES
    2010). **Exact.**
23. Convolution, reverse, shimmer, gated, modulated: definitions; *In the Air Tonight*: existing guide. **Exact.**
24. Type per source, vocal pre-delay 30-100 ms, low cut 150-300 Hz, mix 10-30%: engineering habit (existing *Reverb in a
    mix*). **Opinion / typical**, said so in the text.
25. 1 ft3 = 0.0283168 m3; 30 ft = 9.1 m. **Exact.**

**Against the existing guide:** page 5 says "In a hall it can be 20-40 ms"; fact 5 gives 15-35 ms, so it changes (below).
The rest agrees (living room 0.4-0.6 s, concert hall 1.8-2.2 s, plate 1957 and 2 m wide). The idea's "20-40 ms" stays.

### Topic texts

**Kinds of reverb** · *Six kinds, side by side*
Every reverb copies one of a few real things that ring: a room, a concert hall, an echo chamber, a steel plate or a spring. Studios used the real things for decades. Digital reverbs copy them, or make spaces no building could have.

They differ in more than their length. Listen to how each one starts:

- a room: the first echoes come almost at once, close together, and the tail is short;
- a hall: a short gap, then a few strong echoes from far walls, and a long tail that swells;
- a chamber: a hard room heard through a loudspeaker and a microphone, dense and even soon after it starts;
- a plate: no walls, so no early echoes, only a dense, bright tail from the very start;
- a spring: the sound comes back in repeats, a few hundredths of a second apart, with a metallic boing;
- Digital: DawIt Reverb's own setting, which you can shape into any of them, or into none.

The picture puts one clap through each, with the starting point DawIt Reverb has for that kind (named in each cell). The top of a cell is the level over time, as on *Direct, early, tail*. The strip under it zooms into the first 100 milliseconds, one line per reflection: sparse and spiky in the room and the hall, a dense haze at once in the plate, evenly spaced bursts in the spring.

Click *Hear each in turn* to hear them one after the other, 3 seconds apart. In DawIt Reverb, *Type* picks the kind; the knobs keep their settings when you switch. The next pages take the kinds one at a time.

**Room** · *Close walls, quick echoes, a short tail*
A room is any small space: a living room, a studio's live room, a garage. Its walls are a few metres away, so the first reflections arrive a few milliseconds after the direct sound, strong and close together. You don't hear them as echoes. They make the sound thicker, and tell your ear that it was played somewhere real.

The tail is short: 0.3 to 1 second in most rooms. Because the walls are close, the reflections multiply fast. In a living room of 60 cubic metres they pass 1,000 a second after only 11 milliseconds, and from about there they blur into a smooth tail. In a concert hall that takes about 170 milliseconds.

What it's for: making a sound that was recorded close, or made by a synth, sound as if it was played in a space. Drums above all: studios record a kit with a microphone close to each drum and a pair further away for the room, and mix in as much room as the song wants. Also guitar amps, percussion, and keys recorded direct.

Typical settings: decay 0.3-1 s, pre-delay 0-10 ms, Early 60-90%, mix 10-25%. Too much room on a busy part sounds boxy rather than big.

The picture shows the room from above, slowed down 20 times, with the first reflections as rays. Under it is what one clap becomes, with the first 100 ms zoomed in. Click *Clap: room* to hear the Small room starting point.

**Hall** · *Far walls, a gap, then a long swell*
A concert hall's walls are far away. After the direct sound there is a gap before the first reflection arrives: at a seat in the middle of the floor about 15 to 35 milliseconds. The acoustician Leo Beranek, who measured dozens of halls, found that the best-liked ones had short gaps, about 15 to 25 ms. Boston's Symphony Hall has about 15 ms.

Then the reflections come in: few and strong at first, then more and more. In a hall of 15,000 cubic metres they reach 1,000 a second only after about 170 milliseconds, so the tail swells over the first tenth of a second or two, then dies away slowly and smoothly. Halls built for orchestras ring for about 1.7 to 2.1 seconds when they are full of people; Boston's for 1.9 s. An empty hall rings longer, because the audience soaks up sound.

Why it suits slow music: a long, smooth tail joins the notes into one sound and makes strings, choirs and piano sound full. It blurs fast parts, so drums and quick riffs usually get something shorter.

Typical settings: decay 1.8-3 s, pre-delay 20-40 ms, Size 70-95%, Early 30-50%, damping 4-6 kHz, because air and seats soften a hall's highs. Strings, choirs, piano ballads and pads, and vocals in slow songs.

The picture shows a hall from above, slowed down 20 times: the stage, a listener in the middle and the first reflections off the side walls, each with its time. Click *Clap: hall* to hear the Strings or choir starting point.

**Chamber** · *A hard room with a speaker and a microphone*
Before there were reverb units, a studio that wanted reverb built a room for it: an echo chamber. A loudspeaker plays the recording into a bare room with hard walls, and a microphone, usually at the far end, picks up how the room answers. That is mixed back in with the dry sound.

An early famous one was a washroom. In 1947 Bill Putnam put a speaker and a microphone in the washroom of his Chicago studio for the Harmonicats' *Peg o' My Heart*, which went to number one. Abbey Road in London had a chamber from the 1930s. In 1956 Capitol Records built four under the car park of its Hollywood tower, about 9 metres down, and four more later. They are still used.

Capitol's chambers are concrete, coated with lacquer, with no two walls parallel, so no echo can bounce back and forth between two of them. Each is about 57 cubic metres and rings for up to 5 seconds. Sabine's rule shows how hard those walls are: 0.161 x 57 / 5 = 1.8 square metres of absorption for the whole room, about as much as one small open window.

How it sounds: like a room, because it is one, but a long-ringing one with no clear direct sound. The microphone hears the reflections already mixed together, so the tail is dense and even soon after it starts. The loudspeaker and the microphone colour it a little, and that is part of its sound.

What it's for: vocals, strings and horns with the sound of 1950s and 60s records, and anything that wants a smooth tail that still sounds like a real place.

Typical settings: decay 1.2-2.5 s, pre-delay 0-30 ms, Size 10-30%, mix 15-25%. Click *Clap: chamber* to hear the Echo chamber starting point.

**Plate** · *A steel sheet: dense and bright from the start*
In 1957 the German company EMT brought out the EMT 140, a reverb with no room at all. It is a thin steel sheet, about 1 by 2 metres and half a millimetre thick, hung by its corners on springs inside a large case. A driver, like the coil of a loudspeaker, shakes the plate with the sound, and pickups on the plate hear how it rings. The stereo 140S of 1961 has two pickups in different places, which give two slightly different signals.

A pad of fibreglass hangs next to the plate without touching it. Move it closer and it soaks up more of the plate's motion, so the reverb dies away sooner: the decay can be set from under a second to about 5 seconds.

Why it sounds the way it does. A plate has no walls for the sound to come back from. Its waves reach the edges within milliseconds and keep crossing, so there are no separate early reflections, only a dense tail from the first millisecond. And waves in a plate are dispersive: high frequencies travel faster than low ones. In a 0.5 mm steel plate a 10 kHz ripple crosses 2 metres in about 5 milliseconds, a 1 kHz ripple in 14 and a 100 Hz ripple in 46. A clap is smeared into a bright, splashy wash rather than a row of echoes. A plate's resonances are also spread evenly over all frequencies, where a room has more and more of them higher up: part of why a plate sounds smooth and a little metallic.

What it's for: the vocal and snare reverb of the 1960s to 80s, and still a favourite. It is bright and dense, and without early reflections the voice doesn't sound recorded in a room, so the words stay in front. Also on horns, keys and drums.

Typical settings: decay 1.2-2.5 s, pre-delay 0-40 ms (more on vocals, for clarity), damping 8 kHz or higher, low cut 200-300 Hz. *Early* does nothing on a plate.

The picture shows the plate in its frame with the driver, the pickups and the damper. After a clap, three ripples spread from the driver, slowed down 100 times: 10 kHz in front, then 1 kHz, then 100 Hz. Click *Clap: plate* to hear the Snare plate starting point.

**Spring** · *Repeats along a coil: the boing of guitar amps*
A spring reverb sends the sound down a coiled metal spring. A small driver at one end twists it, and a pickup at the other end turns the motion back into sound. Laurens Hammond patented it in 1941 to give his electric organs the ring of a church without a church. It was small and cheap, so it went into guitar amplifiers: Fender sold a stand-alone Reverb unit from 1961, and reverb was built into amps in the early 1960s. The tank inside an amp is a metal tray, usually with 2 or 3 springs side by side.

Why it sounds the way it does. A wave runs along the spring to the pickup. Part of it bounces back to the driver's end and returns, again and again, so the tail comes in repeats, evenly spaced by the time of one trip there and back: about 40 to 80 ms in typical tanks. A spring also carries its frequencies at different speeds, so each repeat is smeared into a quick chirp: the boing and the drip you hear on a hard pluck. It carries little above a few kilohertz (its limit is usually between 2 and 5 kHz), so its tail is darker than a plate's. Tanks have springs of different lengths so their repeats blur into each other.

Knock a guitar amp and you hear the springs crash: the same thing with a very loud input.

What it's for: clean electric guitar (the surf guitar of the early 1960s is soaked in it), dub reggae, where the reverb is part of the arrangement, organ, and vintage or lo-fi sounds; and as an effect on drums and vocals. Its repeats and chirps make it the least natural reverb, which is exactly why people choose it.

Typical settings: a real tank's decay is fixed, usually between 1.75 and 4 s. The mix is often higher than other reverbs get: 20-40% on a surf guitar. In DawIt Reverb, *Size* sets the time between repeats, 30 ms at 0% to 80 ms at 100%, and *Early* does nothing.

The picture shows the tank and its springs, with a pulse running from the driver to the pickup and back, wider and fainter on each trip, slowed down 20 times. Click *Clap: spring* to hear the Spring guitar starting point.

**Digital and convolution** · *Delay lines, and recorded rooms*
Digital reverbs do with numbers what rooms do with walls. The first, the EMT 250, came out in 1976. The Lexicon 224, designed by David Griesinger and shown in 1978, became one of the most used studio reverbs ever. Inside, a few delay lines feed back into each other, each pass a little quieter and darker, until the echoes come thousands a second: a tail. Manfred Schroeder worked out in 1962 that a tail needs about 1,000 echoes a second or more to sound smooth rather than fluttery; for sharp clicks, about ten times that.

Because nothing in them is physical, digital reverbs can copy any of the real kinds (that is what DawIt Reverb's *Type* does) or make spaces no building could have: a 12-second tail, a huge room with no early reflections, a hall that stays bright to the end. *Digital* is DawIt Reverb's general setting, the one every new DawIt Reverb starts on; its knobs do just what they say, with nothing added.

Convolution reverbs work another way. They play a click or a slow sweep in a real place (a church, a chamber, a real plate) and record how it answers: its impulse response. Each moment of your sound is then replaced by a copy of that answer at its level, and all the copies are added up: that is convolution. The result is the reverb of that place, from that spot, exactly; but you can't move its walls afterwards. DawIt has no convolution reverb. Its plate and spring are models of how those things behave, not recordings of particular units.

The picture is a timeline of artificial reverb, from the first chambers to the digital designs most reverbs build on.

**Reverb effects** · *Gated, reverse, shimmer, moving tails*
Some reverbs aren't meant to sound like any place.

Gated: a big, bright reverb on a drum, cut off sharply a fraction of a second after each hit by a noise gate. The drum sounds huge, but its tail doesn't wash over the next beat: the 1980s drum sound of Phil Collins' *In the Air Tonight*. In DawIt: put DawIt Gate after DawIt Reverb on the drum and pick the gate's *Gated reverb* starting point.

Reverse: the tail comes before the note, so a word swells up out of nothing. Engineers turned the tape round, recorded the reverb of the backwards sound, then turned the tape back. In a DAW: reverse the clip, record its reverb only, and reverse that.

Shimmer: a reverb whose tail is shifted up an octave each time it goes round, so it rises into a glowing, organ-like cloud. For ambient music and slow guitar.

Modulated: the delay lines' lengths sway slowly by a few milliseconds, so the tail's pitch wobbles a little, like a chorus. It hides the metallic ringing a long digital tail can have.

DawIt Reverb has none of these built in; gated reverb is made with DawIt Gate, as above.

The picture shows what each does to one clap, as the level over time.

**Your track** · *Which kind suits your track*
This page looks at the track DawIt Reverb is on and at how the reverb is set, and says what to try.

It goes by the track's name: words like Vocal, BV, Snare, Kick, Drums, Bass, Guitar, Keys, Piano, Strings, Pad and Choir. From that it says which kinds of reverb are usually chosen for such a sound, and why. Then it compares the settings with the usual ranges: the mix, the low cut, the pre-delay on a vocal, the width on a bass, and the decay against the song's tempo.

These are habits that many engineers share, not rules. If it sounds right in the song, it is right.

On a return track it checks the mix, which should be 100%: the dry sound comes from the tracks that send to it.

**How DawIt Reverb works**, added after step 7:
"8. Type: the same parts, set up differently. Room: the early reflections close together, the tail right after them. Hall: a few far-apart early reflections, and the tail fed from them, so it swells over the first tenth of a second. Chamber: the early reflections smeared by more diffusion, so they melt into the tail. Plate: no early reflections, and more diffusion before the lines, so the tail is dense from the first millisecond. Spring: a different circuit. For each side there is one loop as long as a trip along the spring and back, with a chain of all-pass filters in it that delays some frequencies more than others, which turns each repeat into a chirp (the method of Välimäki, Parker and Abel, 2010). The right spring is 12% longer than the left. Digital: the parts exactly as described above." (Match it to the code as built.)

**Size, distance, pre-delay**: "In a hall it can be 20-40 ms." becomes "In a concert hall it is about 15-35 ms at a seat in the middle of the floor."

**Explainer topic *Type*** (title "Type")
- describe: "What makes the reverb: a room, a hall, a chamber, a plate, a spring, or Digital, DawIt Reverb's own general setting. The kinds differ in how the reverb starts, not only in how long it lasts. A room's first echoes come almost at once and close together. A hall's come later and further apart, and its tail swells. A chamber is a hard room heard through a microphone: dense and even. A plate has no walls, so no early reflections: just a dense, bright tail from the start. A spring repeats, with a boing.\n\nThe knobs mean the same on every type, and keep their settings when you switch. Early does nothing on a plate or a spring, which have no walls. More in What is reverb?, Kinds of reverb."
- typical: "Vocals: Plate, or Hall in a slow song. Snare: Plate; the whole kit: Room. Guitars: Room, or Spring on a clean electric. Piano, strings, choirs and pads: Hall. For a 1950s or 60s sound: Chamber or Plate. Digital when you want to shape every part yourself, or a space no real one could be. These are habits, not rules: try two and keep what sounds right in the song."
- describeSettings: Digital "Digital: DawIt Reverb's own reverb, shaped only by its knobs." · Room "Room: close walls, strong early reflections close together, and a tail that starts at once." · Hall "Hall: a few strong reflections from far walls, then a tail that swells and dies away slowly." · Chamber "Chamber: a hard room heard through a microphone: the reflections blend into a dense, even tail within a few hundredths of a second." · Plate "Plate: no walls, so no early reflections: a dense, bright tail from the first millisecond." · Spring "Spring: the sound comes back every <round trip> ms, each repeat smeared into a chirp."
- Size on Plate: "Size <n>%: on a plate, how big the sheet is. A bigger plate has more resonances, closer together, so its tail sounds smoother and a little deeper." Size on Spring: "Size <n>%: the spring's length: a repeat every <30 + 0.5 x n> ms." Decay on Plate adds "On a real plate the damper pad sets this: from under a second to about 5 s."; on Spring "A real tank's decay is fixed, usually 1.75 to 4 s."

### Tooltips and menu
- Type switch: "Type: what makes the reverb: Digital, Room, Hall, Chamber, Plate or Spring. Click to choose; the knobs keep their settings."
- Menu: "Digital: DawIt's own, any space" · "Room: close walls, quick early echoes" · "Hall: far walls, a gap, a slow swell" · "Chamber: a hard room, a speaker and a microphone" · "Plate: a steel sheet, dense and bright" · "Spring: repeats and a boing, as in guitar amps" · "What are the kinds of reverb?..."
- Early, disabled: "Early reflections: a plate has no walls, so it has no early reflections. This knob works on the other types." (Spring: "a spring has no walls...")
- *Clap: <kind>*: "Plays a clap through a <kind> (the <starting point> starting point), without changing your reverb." *Hear each in turn*: "Plays one clap through each kind, 3 seconds apart, in the order shown."

### Starting points: type and its why
- Neutral start, Digital: "Digital: DawIt Reverb's own, with every knob doing just what it says. Switch Type to hear the same settings as a room, a hall, a chamber, a plate or a spring."
- Lead vocal, **Plate**: "Plate: bright and dense, with no early reflections, so the voice sounds close and the words stay in front. The usual vocal reverb since the 1960s." Early becomes 0%: "0%: a plate has no walls, so no early reflections; this knob does nothing on a plate." Size note: "55%: on a plate, how big the sheet is: big enough for a smooth tail."
- Ballad vocal, Hall: "Hall: a slow song has room for a long, smooth tail that swells after each phrase; the pre-delay keeps the words clear."
- Small room, Room: "Room: close walls and quick early reflections make a dry recording sound played in a real space."
- Drum room, Room: "Room: a kit is usually recorded with room microphones as well; a short room with strong early reflections does the same."
- Snare plate, Plate: "Plate: the dense, bright tail of 1970s and 80s snares, with no early reflections to make it boxy."
- Piano or acoustic guitar, Hall: "Hall: an acoustic instrument sounds natural in the kind of space it's played in, with early reflections that place it there."
- Strings or choir, Hall: "Hall: the space orchestras and choirs play in: a long, smooth swell that ties the voices together."
- Ambient wash, Digital: "Digital: an 8-second tail with almost no walls is no real place, so the general setting fits."
- Just a touch, Room: "Room: a tiny room isn't heard as reverb, only as the sound being somewhere."
- Bass, Room: "Room: if a bass gets any reverb at all, a very short, narrow room; a long tail in the lows turns to mud."
- Return: plate, Plate: "Plate: one bright, dense plate suits most of what is sent to it: vocals, snare, keys."
- Return: short room, Room: "Room: a small shared room puts dry tracks in one space."
- Return: hall, Hall: "Hall: one big shared space for slow, sustained parts."

New **Echo chamber** (Chamber; pre-delay 15 ms, size 15%, decay 1.8 s, damping 8 kHz, low cut 200 Hz, early 60%, width 100%, mix 20%). Why: "The sound of 1950s and 60s studios: a hard room that a loudspeaker plays into and a microphone listens to. A dense, even tail that still sounds like a real place, for vocals, strings and horns." Type: "Chamber: a hard, bare room heard through a microphone: dense soon after it starts, and longer-ringing than its size suggests." Notes: "15 ms: a short gap, so the start of each note stays clear. Some studios fed their chambers through a tape delay for a longer one." · "15%: a small room, about 7 m across." · "1.8 s: hard walls ring long for a room this small. Capitol's chambers can ring up to 5 s, but most songs want less." · "8 kHz: hard walls keep the highs; the speaker and microphone soften the very top." · "200 Hz: keeps a small room's boom out of the mix." · "60%: the chamber's first reflections, already blended by the hard walls." · "100%: the reverb all around the sound." · "20%."

New **Spring guitar** (Spring; pre-delay 0, size 50%, decay 2.5 s, damping 6 kHz, low cut 150 Hz, early 0%, width 60%, mix 30%). Why: "The reverb of a guitar amplifier: a spring tank's repeats and chirps on a clean electric guitar, as in surf music. Also good on organ, and as an effect on drums." Type: "Spring: the reverb built into guitar amps since the early 1960s." Notes: "0 ms: a spring brings its own gap: the first repeat comes after one trip along it." · "50%: a long spring, repeating every 55 ms." · "2.5 s: a long tank; real ones are fixed somewhere between about 1.75 and 4 s." · "6 kHz: a spring carries little above a few kHz anyway; this softens the top of the drip." · "150 Hz: keeps the low strings from booming in the tank." · "0%: a spring has no walls; this knob does nothing here." · "60%: a tank is mono; two springs of different lengths, one per side, give a little width." · "30%: surf guitar is soaked in reverb; on a rhythm part, 15-20%."

### Example data
- Spring: T = 30 + 0.5 x Size% ms (30, 55, 80 ms at 0, 50, 100%); right spring 1.12 T (61.6 ms at 50%); the first
  arrival after T/2.
- Plate rings 100x slower: 10 kHz 4.38 m/s (2 m in 0.46 s), 1 kHz 1.385 m/s (1.44 s), 100 Hz 0.438 m/s (4.57 s).
  Spring pulse 20x slower: a 55 ms round trip takes 1.1 s; each trip 30% wider, 40% fainter.
- Echo-density labels: room 60 m3, 1,000/s at 11 ms; hall 15,000 m3, at 172 ms; chamber 57 m3, at 11 ms.
- Timeline: 1930s Abbey Road chamber · 1941 Hammond's spring patent · 1947 *Peg o' My Heart* · 1956 Capitol's chambers ·
  1957 EMT 140 · 1961 Fender Reverb unit, EMT 140S · 1962 Schroeder · 1976 EMT 250 · 1978 Lexicon 224 · 1991 Jot's FDN.
- Effects (dB over 0-2 s, clap at 0): gated: tail flat at -8 dB to 0.30 s, -60 dB by 0.32 s; reverse: -60 dB at 0
  rising to -6 dB at 1.0 s, the note at 1.0 s, silence after 1.1 s; shimmer: tail falling 30 dB/s, plus an octave-up band
  from -30 dB at 0.2 s rising to -15 dB at 0.8 s, then falling 20 dB/s; modulated: a 30 dB/s tail, its pitch line swaying
  +-0.3% at 0.5 Hz.
- Hall rays at the first reflection time and 1.6x and 2.3x it, labelled in ms.

### "Your track" rules
Source from the name (whole words): vocal (vocal, vox, voice, bv, backing, harmony), snare, kick, drums (other drum
words), bass, guitar, keys (keys, piano, organ, rhodes, wurli), ensemble (strings, violin, viola, cello, pad, choir,
orchestra). Usual types: vocal Plate, Hall, Chamber; snare Plate, Room; kick Room; drums Room, Chamber; bass Room;
guitar Room, Spring, Plate; keys Hall, Room, Plate; ensemble Hall, Digital.
1. Type not usual (and not Digital): "Your reverb is a <Type>. On <a vocal>, a <first usual> is the usual choice: <its type note>. A <Type> works too when you want <its describeSettings character>."
2. Digital, source known (not ensemble): "To try: Type <first usual>, the usual reverb on <a vocal>: <its type note>."
3. Not a return, mix > 40% (ensemble > 50%): "The mix is <m>%: the reverb is nearly as loud as the sound itself. Most tracks sit at 10-30%; this much makes it sound far away, which can be what you want."
4. Return, mix < 95%: "On a return, set Mix to 100%: the dry sound comes from the tracks that send to it, and their sends set how much reverb each gets."
5. Low cut below 100 Hz, source not ensemble or keys: "The low cut is <Off / x Hz>. Try 150-300 Hz: the reverb's lows mostly turn into mud."
6. Bass or kick, low cut below 300 Hz: "On a <bass>, keep the lows dry: a low cut of 300-400 Hz, or no reverb at all." (replaces 5)
7. Bass or kick, width above 30%: "Width <w>%: try 0-30% so the low end stays in the middle."
8. Vocal, pre-delay below 20 ms, decay 1 s or more: "Pre-delay <p> ms: try 30-100 ms, so each word is heard dry first and stays in front."
9. Tempo known, not ensemble, decay over 2 bars: "The tail lasts <d> s, about <n> bars at <bpm> BPM (a bar is <b> s). If the parts blur, try a decay under <2 bars> s."
- Quiet, source known: "Nothing to change: a <Type> set like this is a usual choice on <a vocal>." Unknown name: "DawIt can't tell from the name '<name>' what this track holds. Name it Vocal, Snare, Guitar and so on for advice on the type." (rules 3-5 and 9 still run). No track: "This reverb isn't on a track; it's one to try things out with. Put DawIt Reverb on a track or a return to get advice for it."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Reflection rate, 60 m3 at 10 ms | 845 /s | 1 |
| Time to 1,000/s: 60; 15,000; 56.6 m3 | 10.9; 172.0; 10.6 ms | 0.1; 0.5; 0.1 |
| Plate group velocity at 100 Hz; 1 kHz; 10 kHz | 43.8; 138.5; 438.1 m/s | 0.2; 0.5; 1 |
| Spring round trip at 0, 50, 100% | 30, 55, 80 ms | exact |
| Spring repeats (left, width 100%): envelope autocorrelation peak in 20-120 ms | T | 10% |
| Spring level above 8 kHz vs around 1 kHz | 20 dB or more lower | - |
| `measureDecay`, Room to Plate at 1, 2, 4 s, undamped | the knob | 15% (Spring 20%) |
| Steady noise, each type vs Digital | same RMS | 2 dB |
| Output before the pre-delay, every type | 0 | exact |
| Plate and Spring, Early 0% vs 100% | identical | exact |
| Build-up (to within 3 dB of peak, 2 ms RMS, from pre-delay): Plate; Room size 20%; Hall size 80% | <= 10; <= 25; >= 40 ms | - |
| Hall's build-up longer than Room's at size 50% | true | - |
| Crest factor of the first 30 ms: Plate vs Room and Hall | 3 dB or more lower | - |
| Hall's first reflection | pre-delay + `getFirstReflectionMs` | 1 sample |
| Digital at defaults, 48 kHz, 1 s impulse: energy, first non-zero samples | as pinned in step 1 | 1e-6 relative |
| Type change during noise: peak over the next 50 ms | under 2x the steady peak | - |
| Sabine, 0.161 x 56.6 / 5 | 1.82 m2 | 0.01 |

If a build-up or crest target can't be met while the order holds, keep the order and record the measured numbers under
*Changes while building*.

## Steps
**Group A, engine** (`./build.sh --test` at step 7):
- [x] 1. Before any change, pin Digital: energy and first non-zero samples of a default `renderImpulse` (48 kHz, 1 s)
  from today's build. Files: tests. Done when: it passes on today's code.
- [x] 2. Type parameter, `Settings` type, saving, the old-state rule; Room, Hall, Chamber, Plate set-ups. Files:
  BuiltInReverb.{h,cpp}, tests (decay per type, pre-delay, Early on Plate, build-up, crest, Hall's first reflection, old
  state, project round trip).
- [x] 3. Spring (preallocated, round trip from Size, two springs, all-pass chain, low-pass). Tests: repeats, band, decay.
- [x] 4. Type change on the audio thread (clear, 20 ms fade); level matching. Tests: click, noise RMS per type.
- [x] 5. Audition network and clap request (type, showcase, mix 50%). Tests: main network silent and settings unchanged,
  audible, stops processing after its tail.
- [x] 6. Starting points: a type and type note on each, Lead vocal's change, *Echo chamber*, *Spring guitar*. Tests: the
  "explains each knob" check includes the type; the Plates; `getReturnPreset()` is a Plate.
- [x] 7. `src/engine/ReverbKinds.{h,cpp}` (both targets): kind texts, showcases, matcher, advice, derived numbers. Tests:
  every kind has name, summary, text and an existing showcase; Plate's text has "1957" and "EMT 140", Chamber's "1947",
  Spring's "1961", Hall's "1.9"; the numbers; the advice cases. Done when: build and tests pass without warnings.

**Group B, UI** (`./build.sh --test` and `--export-guides` at step 12):
- [x] 8. `fxui::Guide::Page`: optional own clap and button text (EffectWindow.{h,cpp}). Check: other guides unchanged.
- [x] 9. ReverbEditor: KIND group, Type switch and menu, Early disabled on Plate and Spring, knob texts per type, types in
  the starting points, the panel drawing the kind (`verbtext::drawKind` in ReverbGuide).
- [x] 10. ReverbGuide: rewritten *Kinds of reverb*, pages 7-13 and 16, the paragraph for page 15, the page-5 sentence;
  texts from ReverbKinds; claps through the audition.
- [x] 11. The host-track lookup in `fxui`, set in `MainComponent`'s constructor; used by *Your track*.
- [x] 12. TheoryGuides.cpp: the reverb entry's summary and words; a search check that "plate reverb", "spring reverb" and
  "echo chamber" find *What is reverb?* first. **After the singing guide is committed.**
- [x] 13. Real-app check, song "sends": the reverb window on track "Reverb" (the return, a Plate: KIND group, Early
  dimmed, plate panel); with "BV 1" selected, guide "reverb" at topics "Kinds of reverb" (six readable cells), "Room",
  "Hall", "Chamber", "Plate", "Spring" (`"wait": 2500`, so the drawings have moved), and "Your track" (the "To try: Type
  Plate" line). Look for clipped text, empty cells, still animations.
- [x] 14. Docs (**after the singing guide is committed**): README (Reverb bullet, Tests, Code layout for ReverbKinds);
  FEATURE_IDEAS status (*Partly done*, with *Left for later*); recipes.md *A built-in effect or instrument* (a page's own
  clap; the host-track lookup for *Your track*); ui-design.md *Effect and instrument windows* (a Type switch with many
  choices opens a menu). Fix: architecture.md and recipes.md imply every built-in has match level; Reverb and Delay don't.

## Tests
In `runBuiltInReverbTests` and a new content check, with the numbers of *Theory: Test numbers*, plus:
- Digital pinned; a new reverb and an old state are Digital; Spring survives a project.
- Audition: main network and settings untouched, audible, stops after its tail.
- Advice at 120 BPM: "Lead vocal", Digital, pre-delay 0, mix 20%, 1.6 s gives rules 2 and 8; "Bass", Room, low cut
  100 Hz, width 100% gives 6 and 7; a return at 50% gives 4; "Strings", Hall, 3.2 s no decay line; "Snare" with Snare
  plate's settings "Nothing to change"; "Keys", Hall, 6 s rule 9 ("3 bars", "a bar is 2.0 s"); "Track 3" the name hint;
  no track the stand-in line; "Lead vocal" as a Spring rule 1. No line contains "wrong".
- Search: the three phrases of step 12.

## Website hand-off
- Features page, DawIt Reverb section: the Type switch (six kinds, each made differently) and the kinds pages of *What
  is reverb?*, with hearing each kind and *Your track*. Main page: not needed.
- Screenshots (song "sends"): the reverb window on the Reverb return (Plate); *Kinds of reverb*; *Plate* and *Spring*
  mid-animation; *Your track* on BV 1.
- How it's made: no library; the spring method could be credited (Välimäki, Parker, Abel 2010).

## Left for later
- Convolution with recorded impulse responses (a library, licences for the recordings); models of particular units.
- Shimmer, reverse and modulated tails in DawIt Reverb.
- A decay-per-frequency picture per kind; a menu item that also loads a type's typical values; a picture in the device-bar chip.

## Changes while building

- The shared guide page also has an optional live-text callback, so *Your track* follows settings and track-name changes and exports the current advice. Other guides keep their existing clap and button width.
- Screenshot lists accept `"clap": true` on a guide shot, so animations can be captured at a repeatable point after a clap.
- Advice uses the source's matching starting-point note (for example *Lead vocal* for a vocal), rather than the kind's listening showcase (*Snare plate*), so its explanation fits the track.
- Long track-name prefixes retain routing's existing behaviour (for example Vocals2); short names such as BV and vox match whole words.

## Build verification

- Original Digital at 48 kHz, one-second impulse: stereo energy **1.319954598505594**, first non-zero sample **1356** on both channels. The regression preserves these values.
- Measured undamped decay at 1, 2 and 4 seconds passes for every type. Build-up: Plate **0 ms**, Room (20% Size) **14 ms**, Hall (80% Size) **60 ms**. At equal Size, Hall **44 ms**, Room **20 ms**.
- First 30 ms crest: Plate **20.0 dB**, Room **27.7 dB**, Hall **29.3 dB**. Spring repeats at **55 ms** at 50% Size; its 8–10 kHz band is over **40 dB** below the band around 1 kHz.
- Both undamped and default damped noise comparisons stay within **2 dB** of Digital; default settings: Room -0.03, Hall +0.79, Chamber +0.27, Plate +1.74, Spring +1.84 dB.
- All 22 guides and *Start here* exported to `/tmp/reverb-guide-pdfs`; *What is reverb?* has **16 topics, 17 pages**. The user's settings were copied aside and restored without reading their contents.
- Shot list: `/tmp/reverb-kinds-shots.json`; all eight views in `/tmp/reverb-kinds-shots/`: `reverb-plate-return.png`, `reverb-kinds-of-reverb.png`, `reverb-room.png`, `reverb-hall.png`, `reverb-chamber.png`, `reverb-plate.png`, `reverb-spring.png`, `reverb-your-track.png`. The pictures show the Plate return with Early dimmed, all six comparison cells, the five physical mechanisms, and BV 1's Plate recommendation.
- Theory audit: shipped topic texts, dates, formulae and advice checked against this plan. No additional sourced facts, library, model or tool were introduced. Website hand-off remains as above; website files are unchanged.

- Final `./build.sh --test`: **1,359 checks passed (88 new), no warnings**, ending with `All checks passed`. Final screenshots were regenerated and all eight inspected after fixing compact time-axis labels and extending the Room reflection animation.
