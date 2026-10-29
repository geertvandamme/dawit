# Aux sends and buses (part 1: buses)

Status: Built, not committed
Idea: [Aux sends and buses](../FEATURE_IDEAS.html#aux-sends-and-buses)

## Goal

Select a few tracks, right-click and choose **Create bus**: a new bus track appears that holds them, like a folder. The bus has its own effects (device bar), fader, pan, mute, solo and meter, hears its tracks added together, and plays into the master. Buses can sit inside buses, and tracks move in and out by dragging or with an *Output* menu. A beginner can then do what every mix does: one compressor that makes the drums move as one, one fader for all the backing vocals, one mute for a whole section. A new guide, *Buses and routing*, explains where each track's sound goes, why the levels add up in a bus, and looks at the user's own song.

This plan covers **buses only**. Aux sends and return tracks stay open for a later plan. The design leaves room for them: a track's output stays a single `Track::bus` pointer, the render order and summing work for any tree, and the guide's topics, the device bar's chips and the mixer strip all have a place where sends will go (see *Left for later*).

## Decisions for you

1. **The order of the strips in the mixer.** Options: (a) the arrangement's order: a bus strip comes right before its tracks, a coloured bracket joins them, and folding a bus folds its strips too (as Ableton Live's group tracks do); (b) a console layout: every track on the left, every bus at the right before the master (as the idea suggested and as large studio consoles do). **Recommended:** (a). The user asked for a folder, and one order in both views is easier for a beginner to follow. With (b) a folded bus has no obvious meaning in the mixer.
   Answer: (a) strips follow the arrangement (chosen by the user).
2. **What the x on a bus does.** Options: (a) it removes the bus only: its tracks stay, at the bus's place, and play where the bus played (the same as *Ungroup*); the right-click menu also has *Delete bus and its 4 tracks...*; (b) it deletes the bus with everything in it, as deleting a folder does. **Recommended:** (a). Losing a whole drum kit to one click is the worse mistake, and DawIt has no undo. It asks first when the bus has effects ("Remove the bus Drums and its 2 effects? Its 4 tracks stay and play into the master.").
   Answer: (a) the x removes the bus only (chosen by the user).
3. **Plugin delay compensation.** DawIt Saturation and DawIt Distortion report a latency from their oversampling filters, and other plugins can too. No track is delay-compensated today, so a track with such an effect already plays a little late against the others. Buses don't make this worse: a bus with a delaying effect delays its whole group. Options: (a) leave delay compensation for its own plan, for tracks and buses together, and say so in *Left for later*; (b) build it now. **Recommended:** (a). It touches every track's processing, it is a separate idea, and the delays are a fraction of a millisecond (step 3 prints the real number). The render order in this plan is where it would go.
   Answer: (a) leave delay compensation for its own plan (recommendation, accepted by the user).
4. **Opening a project with buses in an older DawIt.** An older DawIt reads an unknown `"type": "bus"` as a MIDI track and ignores the tracks inside it, so they would be lost without a word. Options: (a) save `"version": 2` only when the song has a bus, so an older DawIt refuses it ("saved by a newer version") and projects without buses stay at version 1 and open everywhere; (b) always save version 2; (c) save a flat list that an older DawIt reads with the buses as empty MIDI tracks. **Recommended:** (a): nothing is lost silently, and nothing changes for songs without buses.
   Answer: (a) save version 2 only when the song has a bus (recommendation, accepted).
5. **Colours.** Options: (a) the bus gets its own colour (the next one in the palette), its tracks keep theirs, and the bus's colour runs down a gutter at the left of its tracks' headers, as a bracket over their mixer strips and as a tint behind their lanes; (b) creating a bus recolours its tracks in shades of the bus's colour. **Recommended:** (a). Track colours are the user's choice and other views use them (clips, the device bar). (b) can come later as a menu item (*Colour tracks like the bus*).
   Answer: (a) the bus gets its own colour; its tracks keep theirs (recommendation, accepted).
6. **The guide's name and place.** Options: "Buses and routing", "Routing", or "Signal flow". **Recommended:** *Buses and routing*, in the *Mixing* area after *What is compression?*, not on *Start here* (beginners meet buses after levels, EQ and compression). When sends come, the guide can become *Buses, sends and routing*.
   Answer: *Buses and routing*, in Mixing after compression (recommendation, accepted).
7. **The shortcut for Create bus.** Cmd+G, the usual one for grouping (Ableton Live), opens the Theory Guides in DawIt. Options: Cmd+Shift+G (G for group), or Shift+Cmd+D (Logic's track stacks). **Recommended:** Cmd+Shift+G. The menu item uses the user's words: *Create bus*.
   Answer: Cmd+Shift+G (recommendation, accepted).
8. **Where a bus's folded state is kept.** Options: (a) in the project, so a song opens the way it was left; (b) in neither: buses always open unfolded. **Recommended:** (a), as Logic and Reaper do. Folding marks the project as changed, which is the price.
   Answer: (a) saved in the project (recommendation, accepted).

## What the user does and sees

### What exists today

- `Selection` holds one track and one clip. There is no multi-selection of tracks, no way to move a track up or down, and no *Track* menu.
- Every track plays straight into the master (`Engine::renderSegment` adds each track into `mixBuffer`). The master has a volume and a meter, no effects.
- The mixer (`MixerView`, Cmd+2) shows a strip per track in edit order and the master. The device bar shows the selected track's chain, then its fader, then the master, with the gain staging arrows and badges.

### Selecting several tracks

- **Click** a track header: only that track is selected, as now.
- **Shift-click** a header: selects every track from the last clicked one to this one, in the order the rows show.
- **Cmd-click** a header: adds the track to the selection, or takes it out.
- The selected headers and lanes are highlighted, as the selected track is now. The device bar, piano roll and keyboard input follow the track clicked last.
- The same works on mixer strips.
- Right-clicking a selected track keeps the selection, so the menu acts on all of them. Right-clicking an unselected track selects only that one.

### Create bus

- **Right-click** a selected track header (or strip) > **Create bus** (for several: *Create bus from 3 tracks*), or *Track > Create Bus* (**Cmd+Shift+G**).
- A new bus row appears where the first of the tracks was. The tracks move into it, in their order, indented under it. The bus's name opens for typing:
  - If every track is of the same kind by its name or sound, the name is filled in ("Drums" for Kick, Snare and Hats; "Vocals" for Lead Vox, BV 1 and BV 2; see *Theory and numbers*).
  - Otherwise it is "Bus 1", "Bus 2" and so on.
- The bus is selected, so the device bar shows its chain, ready for *+ Effect*.
- Tracks keep their instruments, effects, clips, faders and colours. Only where their sound goes changes.
- If tracks come from different buses, the new bus goes into the closest bus they all share (or the master), at the place of the first of them. A track inside a selected bus stays inside that bus: selecting Drums and Kick and creating a bus moves Drums (with Kick in it).
- **New Bus** (empty) is in the *Track* menu and on a **+ Bus** button next to *+ MIDI Track* in the transport bar. Tooltip: "Add a bus: a track without clips that other tracks play into, so you can add effects to them and turn them up or down together. To put tracks in a new bus at once, select them and press Cmd+Shift+G."

### The arrangement

```
| Drums                    ?  [M][S][x] ▮|  ▒▒▒▒ thin bars: the tracks' clips ▒▒▒▒   <- bus row
|  [====volume====] -2.0 dB   (pan)      ▮|
| BUS · 3 tracks        [FX: DawIt Compressor] ▮|
|▌| Kick              [R][M][S][x]      ▮|  [clip][clip][clip]                       <- member, indented
|▌|  ...                                     |
|▌| Snare             ...                    |
|▌| Hats              ...                    |
| > Vocals                 ?  [M][S][x] ▮|  ▒▒ thin bars ▒▒                         <- folded bus
| Bass                      ...              |
```

- **A bus row** has the same height as a track (84 px). Its header:
  - **Fold arrow** (▾ open, ▸ folded) left of the name. Tooltip: "Hide this bus's tracks (or show them again). While they're hidden, the bus's row still shows where they play."
  - **Name** (double-click to rename), **?** (opens *Buses and routing* at *What a bus is for*), **M**, **S**, **x**, volume, pan and meter, as on a track.
  - **Bottom line:** "BUS · 3 tracks" and the **FX** button. A bus has effects only: the FX menu has no *Add instrument*.
  - **Tooltip** on the header: "Drums is a bus: Kick, Snare and Hats play into it, and it plays into the master. Drag a track's header onto it to add the track. Right-click for more."
- **A bus's lane** has no clips. It shows its tracks' clips as thin bars in their colours, one line per track (up to 10 px each, stacked). So a folded bus still shows where its tracks play.
  - An empty bus says, dimmed: "Drag tracks here to put them in this bus".
  - **Double-click** the bus's lane: folds or unfolds it.
  - **Tooltip** over the lane: "Bar 9 · Drums · Kick and Snare play here".
  - **Audio files dropped** on a bus's lane become a new audio track inside the bus.
- **Tracks in a bus** are indented by a 10-px gutter per level, in the bus's colour (25% alpha with a 2-px line), up to 4 levels. Their lanes get a faint tint of the bus's colour (4% alpha), so the group reads as a block.
  - The header's bottom-left text says "MIDI", "AUDIO", or "BUS MUTED" / "SILENT" when the track can't be heard (see *Solo and mute* below).
  - **Tooltip** on the gutter: "In the Drums bus".

### Moving tracks in and out

- **Drag a track header** by its name or its background (not its controls) up or down. An accent line shows where it will land, indented to the level it will land at.
  - Between two rows whose levels differ (the last track of a bus and the next row outside it), the mouse's left-right position picks the level: further right stays in the bus, further left goes out.
  - Over the middle of a bus's row, the whole row is outlined instead: dropping puts the track at the end of that bus.
  - A bus can't be dropped into itself or into a bus inside it. The line doesn't show there.
  - Several selected tracks move together, in their order.
  - The view scrolls near its top and bottom edges. Escape cancels.
- This also lets any track be moved up or down, which wasn't possible before.
- **Right-click > Output** lists *Master* and every bus (indented by level, the current one ticked, the bus itself and its own buses left out), then *New bus...*.
- **Right-click > Remove from bus "Drums"**: the track goes to where the bus plays, just below the bus.

### The right-click menu of a track or a bus

```
Kick                                    (or "3 tracks", or "Drums (bus, 3 tracks)")
Create bus                 Cmd+Shift+G  ("Create bus from 3 tracks")
Output                   >  Master ✓ / Drums / Vocals / -- / New bus...
Remove from bus "Drums"                 (when in a bus)
--
Show tracks / Hide tracks               (a bus)
Export track as MIDI file...            (MIDI tracks, as now)
Show only notes in A minor              (MIDI tracks, as now)
--
Delete track                            (a track; "Delete 3 tracks" for several)
Ungroup (keep its tracks)               (a bus, instead of Delete track: its tracks take its place; the bus and its effects go; the x does the same)
Delete bus and its 3 tracks...          (a bus, asks first)
--
About buses and routing...              (opens the guide: the last item, as everywhere)
```

### Solo and mute

- **Mute a bus:** every track in it is silent. Their own meters still move, because they still play into the bus; their headers say "BUS MUTED".
- **Solo a track in a bus:** you hear it through its bus (with the bus's effects and fader). The bus counts as soloed for that track, but its other tracks are silent.
- **Solo a bus:** you hear all its tracks.
- The rule: while anything is soloed, a track is heard when it, a bus it plays into, or a track that plays into it is soloed. The mixer's *SILENT* and the device bar's *FADER - SILENT* follow the same rule (`routing::getSilence`).

### The mixer

```
| Drums  | Kick  | Snare | Hats  | Vocals | Bass  |  | Master |
|========= bracket in Drums' colour =|                   |        |
| ▾ BUS  | MIDI  | MIDI  | MIDI  | ▸ BUS  | MIDI  |  | MIX    |
|  M  S  | M  S  | ...                                 |        |
|  pan   | pan   |                                     |        |
| fader  | fader |                                     |        |
| ▮meter | ▮     |                                     |        |
|-> Master|-> Drums|-> Drums|-> Drums|-> Master|-> Master| | -> Out |
```

- Strips follow the arrangement's rows (decision 1a): a bus's strip, then its tracks; a folded bus hides its tracks' strips; the fold arrow sits in the bus strip's top left.
- A bracket in the bus's colour runs over the strips of its tracks (3 px, under the colour bar).
- **Output button** at the bottom of every strip: "→ Master", "→ Drums". Click: the *Output* menu. Tooltip: "Where this track's sound goes: straight to the master, or into a bus. Click to change it."
- Shift-click and Cmd-click select several strips; right-click opens the same menu as on a track header.
- The master strip's output says "→ Out" and doesn't open a menu. Tooltip: "The master goes to your speakers, and into a bounced WAV."
- The hint at the top gets: "Shift- or Cmd-click strips to select several; right-click to put them in a bus."

### The device bar

- **A track in a bus:** after its fader come the buses it plays into, then the master:
  `[Instrument] → [EQ] → [Fader] → [BUS Drums] → [Master]`
  - The bus chip shows the bus's name, what comes in (all its tracks together) and what goes out (after its effects and fader).
  - Tooltip: "Drums: this track plays into the Drums bus, together with Snare and Hats. In: all of them added up; out: after the bus's effects and fader. Click to see the bus's own chain."
  - Click: selects the bus, so its chain shows.
  - The arrow into a bus chip is drawn dashed in the gain staging guide, as into the master, since other tracks join there.
- **A bus:** its chain starts with what feeds it instead of an instrument:
  `[From Kick, Snare, Hats] → [DawIt Compressor] → [Fader] → [Master]`
  - The first chip shows the tracks' colours as a small stack and their names ("Kick, Snare, Hats +2" when long), with the level of their sum going out.
  - Tooltip: "What comes into Drums: its 3 tracks added up. Click for a list of them."
  - Click: a menu *Show Kick's chain*, *Show Snare's chain*, ..., then *About buses and routing...*.
  - The title says "BUS" and the name. *+ Instrument* is hidden; *+ Effect* and *Gain staging?* stay.
- **Gain staging** follows (see *Theory and numbers*): the arrows carry the levels; a ! points at a bus whose tracks add up to over 0 dB going into its effects, with the topic *Through a bus* in the gain staging guide.

### The Theory Guides window and the guide

*Buses and routing* is listed under *Mixing* and opens from: the last item of every track's right-click menu, the **?** in a bus's header, the *Output* menu's last item in the mixer, and the menu of a bus chain's first chip. See *The guide*.

### Remembered between sessions

Nothing new in the app settings. Whether each bus is folded belongs to the song and is saved in the project (decision 8).

## Data and saving

### In the edit

- `Track::Type` gets `bus`. A bus has no clips and no instrument, only effects.
- `Track::bus` (a `Track*`, null for the master) is where the track plays into. `Track::folded` says whether a bus's tracks are hidden.
- `Engine::tracks` stays one flat list in the order the rows show, with the rule: **a bus is followed by everything in it**, its own tracks and their tracks (a depth-first order). Every engine function that changes the structure keeps this rule, and `routing::isTidy` checks it in the tests. Code that walks `getTracks()` (saving, the song's end, plugin lookups) keeps working.

### In the `.dawit` project

A bus holds its tracks, so the file reads like the folders it shows:

```json
"tracks": [
  { "type": "bus", "name": "Drums", "colour": "#ff8a4f", "volumeDb": -2.0, "pan": 0.0,
    "muted": false, "soloed": false, "folded": true,
    "plugins": [ { "name": "DawIt Compressor", ... } ],
    "tracks": [
      { "type": "midi", "name": "Kick", ... "clips": [ ... ] },
      { "type": "midi", "name": "Snare", ... }
    ] },
  { "type": "midi", "name": "Bass", ... }
]
```

- A bus has no `clips`, `scaleFilter` or `builtInSound`. `"folded"` is written only when true. `"tracks"` is left out for an empty bus.
- **Version (decision 4a):** `"version": 2` when the song has a bus, otherwise 1, as today. `project::formatVersion` becomes 2 (the newest this build reads), with a new `formatVersionWithoutBuses = 1`.
- **Loading** reads the tracks recursively: `"type": "bus"` makes a bus, and the tracks in its `"tracks"` go into it in order. A track of any other type with `"tracks"` keeps them at the top level, with a warning. Older projects have no buses and load as before.
- **Test:** a song with a nested bus, plugins on a bus, a folded bus and a muted bus saves, loads, and saves to the same text; a song without buses saves exactly as before (version 1, no `"folded"`, no nested `"tracks"`); a hand-written version-1 project loads unchanged; version 3 is refused.

## Theory and numbers

### Levels add up in a bus

A bus adds its tracks sample by sample. How much louder the sum is depends on whether the tracks play the same thing:

- **Different sounds** (incoherent): powers add. N equal tracks are 10·log10(N) dB louder than one: 2 tracks **+3.01 dB**, 4 tracks **+6.02 dB**, 8 tracks **+9.03 dB**. Two tracks at -6 dB make **-2.99 dB**.
- **The same sound in step** (coherent): amplitudes add. N equal tracks are 20·log10(N) dB louder: 2 tracks **+6.02 dB**, 4 tracks **+12.04 dB**. This is also the worst case for peaks: when the peaks of different tracks happen to line up, they add like this.
- Example for the guide and the hint: 4 drum tracks peaking at -10 dB each add up to peaks of up to **+2.04 dB** (over 0 dB), and to about **-3.98 dB** in loudness.
- Source: Eberhard Sengpiel, *Adding amplitudes and levels* ([sengpielaudio.com, PDF](https://www.SengpielAudio.com/AddingAmplitudesAndLevels.pdf); [calculator for coherent sources](https://sengpielaudio.com/calculator-coherentsources.htm)): "n = 2 coherent sources of equal level result in a higher level of 20 × log10 2 = +6.02 dB"; "n = 2 equally loud non-coherent sound sources result in a higher level of 10 × log10 2 = +3.01 dB".
- *Tested:* `routingguide::sumDb` gives these numbers (within 0.01 dB). The engine is tested on real sound: two identical 440 Hz sines peaking at 0.25 come out of a bus peaking at **0.50** (+6.02 dB); a 440 Hz and a 660 Hz sine with RMS 0.177 each come out with RMS **0.250** (+3.01 dB).

### The bus's fader, pan, mute and solo

- The fader multiplies the sum: -6 dB is a gain of **0.501**, so every track in the bus comes down by the same 6 dB and their balance stays. *Tested* on rendered sound.
- DawIt's pan is a balance: in the middle both sides pass at full level; turning it left turns the right side down by cos(p·π/2), to silence at hard left (`Track::process`). On a bus it works on the group's stereo mix. *Tested:* a bus panned hard left leaves the right channel silent.
- The solo rule (see *What the user does and sees*) is the "solo in place" behaviour of DAWs, where soloing a track keeps the path to the speakers open. *Tested* on rendered sound and on `routing::isOnSoloPath`.

### Glue: one compressor on a bus

- DawIt Compressor's **Drum bus** starting point: threshold -16 dB, **2:1**, knee 6 dB, attack **30 ms**, release **150 ms**, makeup 2 dB, sidechain low cut at 120 Hz, for **2 to 3 dB** of reduction on the loudest hits. Its **Mix bus** starting point: **1.5:1**, attack 30 ms, release 200 ms, 1 to 2 dB of reduction (`comp::getPresets()` in `BuiltInCompressor.cpp`).
- These match the best-known bus compressor, the SSL G-series: ratios **2:1, 4:1 and 10:1** ("2:1 ... subtle, transparent control", "10:1 ... essentially peak limiting"), attack **0.1, 0.3, 1, 3, 10 or 30 ms**, release **0.1, 0.3, 0.6 or 1.2 s or auto** ([Sound On Sound, SSL XLogic G-Series Compressor review](https://www.soundonsound.com/reviews/ssl-xlogic-g-series-compressor)).
- *Tested:* the guide's *Glue* text is built from the Drum bus preset itself (`comp::formatKnob`), so it shows "2.0:1", "30 ms" and "150 ms" and can't drift from the compressor.

### Which tracks belong together (for the suggested names and the advice)

A track's **family** comes from the words in its name (whole words or word starts, any case), checked in this order (the first family that matches wins), then from its sound:

| Family (bus name) | Name words | Or |
|---|---|---|
| Drums | drum, drums, kick, snare, hat, hats, hihat, hi-hat, tom, toms, cymbal, crash, ride, overhead, overheads, oh, room, perc, percussion, clap, shaker, 808, beat | a MIDI track on the DawIt Synth's *Drums* or *Percussion* sound, or with DawIt Drums as its instrument |
| Bass | bass, sub | (after Drums, so "Bass drum" is Drums; before Guitars, so "Bass guitar" is Bass) |
| Vocals | vocal, vocals, vox, voc, voice, bv, bvs, backing, harmony, harmonies, choir, adlib | |
| Guitars | guitar, guitars, gtr, gtrs | |
| Keys | keys, piano, organ, rhodes, wurli, pad, pads | |

- A bus's own family comes from its name the same way ("Drums" is Drums, "Band" has none).
- *Tested:* "Kick" and "Bass drum" are Drums; "Bass" and "Bass guitar" are Bass; "BV 2" and "Lead Vox" are Vocals; "Gtr L" is Guitars; a MIDI track named "MIDI 3" on the *Drums* sound is Drums; "Strings" has no family.
- **Advice thresholds** (tested on a made-up song):
  - 2 or more tracks of one family that play into the same place, when that place is the master or a bus of another family (tracks in a bus the user named without a family, such as "Band", are left alone);
  - a bus with 1 track; an empty bus;
  - a Drums bus (by its name) of 2 or more tracks with no effects;
  - buses nested more than 2 levels deep;
  - a bus whose tracks add up to peaks over 0 dB (from live levels, while playing).

### Latency

DawIt Saturation and DawIt Distortion call `setLatencySamples` with their oversampling filters' delay. No track or bus is delay-compensated (decision 3). Step 3's test prints the number of samples so it is on record, and checks only that it is above 0.

## The guide

A new guide, **Buses and routing**: content in `src/engine/RoutingGuide.{h,cpp}` (namespace `daw::routingguide`, next to `gainstaging`), window in `src/ui/RoutingExplainer.{h,cpp}`. Window 980 x 720, as *Gain staging*: the title row with *Export PDF...*, the topic list on the left (250 px), the picture at the top (320 px), the text below.

### The picture

One drawing style for every topic: the **routing tree**.

- Boxes in the tracks' colours, joined by arrows that run to the right: tracks on the left, buses further right, and the master box at the far right.
- A bus's box is wider, sits at the middle of its tracks' rows, and shows a thin bar for its incoming sum on its left edge.
- Every box has a small live level meter (`meterProportion`, the app's meter colours).
- Muted or silenced boxes are dimmed; S and M marks show solo and mute; a folded bus is one box, "Drums (4)".
- The selected track's path (track → buses → master) is drawn in the accent colour.
- Click a box to select that track in the app. Tooltip: "Kick → Drums → Master · -12 dB".

The layout is a pure function (`routingguide::layOut`), so it can be tested. For topics about the general idea, the picture shows example trees (plain data, `routingguide::getExample`); for *Your song's routing* and *Solo and mute* it shows the song live. With no tracks, every topic shows its example.

### Topics

1. **What is routing?** — *Where each track's sound goes.*
   - Picture: the song's tree, or the example: Kick, Snare and Hats into Drums; Lead vocal and a Backing vocals bus (BV 1, BV 2) into Vocals; Bass and Guitar straight to the master.
   - Text: every sound takes a path. Instrument, effects, fader and pan, then out: straight into the master, or into a bus first. The master adds everything up and goes to the speakers. In DawIt every track goes to the master until you put it in a bus. Mixers also have sends (a copy of a track at its own level, to a shared effect such as one reverb); DawIt doesn't have those yet.
2. **What a bus is for** — *Treat a group of tracks as one.*
   - Picture: the Drums example with three labels: "one compressor for all", "one fader", "one mute".
   - Text: a bus is a track without clips that other tracks play into. What it's for:
     - effects on the group: one compressor that makes a kit move together (glue), one EQ for all the backing vocals;
     - one fader to ride a section: the backing vocals 2 dB down in the verse;
     - one mute or solo for a group.
     It also keeps a big song tidy: fold the bus and the 8 drum tracks become one row. A bus is sometimes called a group, a submix or a stem, and the master is itself the last bus (the mix bus).
3. **Grouping tracks in DawIt** — *Select, right-click, Create bus.*
   - Picture: three track headers drawn as in the arrangement, selected, an arrow, then the bus row with the three indented under it.
   - Text, step by step: click Kick's header, Shift-click Hats' header; right-click > *Create bus* (Cmd+Shift+G); type a name. Drag a header onto a bus to add it, drag it out to take it out, or use *Output* in the right-click menu or under a mixer strip. *Ungroup* removes the bus and keeps its tracks. The arrow folds the bus.
4. **Levels add up** — *A bus is louder than any of its tracks.*
   - Picture: bars. Two tracks at -6 dB → the bus at about -3 dB when they play different things, 0 dB when they play the same thing. Four drums at -10 dB peaks → up to +2.0 dB peaks. When a bus is selected and the song plays, its tracks' live levels and the measured sum are drawn instead.
   - Text: why (adding samples, power for different sounds, amplitudes for the same sound), the numbers from `sumDb` (+3, +6, 10·log10 N), and what to do: keep the tracks' peaks lower (around -18 to -12 dB) when several go into one bus; leave the bus fader near 0 dB and balance with the tracks' faders; turn the bus down to move the whole group in the mix. Over 0 dB inside DawIt doesn't distort, but the bus's effects react to the hotter level (see *Gain staging*).
5. **Glue: one compressor for the group** — *Make the drums move as one.*
   - Picture: a chain like the device bar's: Kick, Snare, Hats → [DawIt Compressor: 2.0:1, 30 ms, 150 ms, about -2.5 dB] → Fader → Master, with the numbers read from the *Drum bus* starting point.
   - Text: one compressor hears the whole kit. When the snare hits hard, everything dips a little together, so the drums breathe as one instrument. Gentle settings (from the preset), with why each. The SSL bus compressor's ratios and times as the classic example. Use 1 to 2 dB on the whole mix (the *Mix bus* starting point). EQ on a bus shapes the group: a low cut on all the backing vocals at once.
   - Action, when a bus of 2 or more tracks is selected and has no DawIt Compressor: **Put DawIt Compressor on Drums** (adds it with the *Drum bus* starting point and opens its window).
6. **Typical buses in a song** — *The groups most mixes use.*
   - Picture: the example tree of a pop song: Drums (Kick, Snare, Hats, Overheads), Bass, Guitars (Guitar L, Guitar R), Keys (Piano, Pad), Vocals (Lead vocal, Backing vocals: BV 1, BV 2, BV 3).
   - Text: what goes in each and why: drums for glue; vocals for one shared tone; backing vocals as a bus inside the vocals, to sit under the lead with one fader; doubled guitars left and right on one fader. Not every track needs a bus: one bass track goes straight to the master. Two levels of buses is plenty for most songs.
7. **Solo and mute with buses** — *What you hear when a track or a bus is soloed.*
   - Picture: the song's tree with its real solo and mute states; silenced boxes dimmed.
   - Text: the rule (soloing a track keeps its bus open; soloing a bus plays its tracks; muting a bus silences them, while their meters still move). Why: you hear a soloed track as it sounds in the mix, through its bus's effects.
8. **Your song's routing** — *Your tracks and buses, and what to try.*
   - Picture: the song's tree, live.
   - Text: `routingguide::explainSong`, then the advice (`routingguide::analyse`), for example:
     - "9 tracks: 3 in Drums, 3 in Vocals (2 of them in Backing vocals), 3 straight to the master."
     - "Kick, Snare and Hats go straight to the master. Put them in a Drums bus: one fader for the kit, and one compressor to glue it." with the action **Create a Drums bus from Kick, Snare and Hats**.
     - "Drums has no effects yet. A gentle compressor (2.0:1, 30 ms attack, 2 to 3 dB of reduction) makes the kit move as one." with the action **Put DawIt Compressor on Drums**.
     - "Lead has a bus to itself. A bus does most for a group: add the other vocal tracks to it, or ungroup it."
     - "Fx is empty: drag tracks onto it, or delete it."
     - "The tracks in Drums add up to +2.1 dB peaks. Turn them down a little, together, so the bus's compressor gets a healthy level." (while playing)
     - With no buses and no family of 2 or more: "Every track goes straight to the master. That's fine for a small song; buses help once there are several drum or vocal tracks."
   - It follows the song (a `juce::ChangeListener` on the engine and the selection, and a 15 Hz timer for the levels).

### The gain staging guide

`gainstaging::Topic` gets **`bus`**, "Through a bus" — *Why a bus gets louder than its tracks.* A short version of topic 4 (the +3 and +6 dB, the 4 drums at -10 dB, what to do), ending "The *Buses and routing* guide shows your song's buses." Its picture is the existing chain picture, where the arrows into a bus are dashed, as into the master.

### In the Theory Guides window

- `Guide::routing`, placed after `Guide::compressor` in `allGuides`.
- **Area:** *Mixing*, whose summary becomes "Levels, EQ, compression and buses: making the parts fit".
- **Name:** "Buses and routing".
- **Summary:** "Where each track's sound goes: buses that group tracks, levels that add up, glue compression, solo and mute, and your song's routing."
- **Opens from:** "A track's right-click menu (About buses and routing...), the ? in a bus's header, and the mixer's Output menu".
- **Search words:** "bus buses group grouping groups folder submix sub-mix stem routing route output master signal flow glue mix-bus drum-bus".
- **Topics:** from `routingguide::getTopicName` over `routingguide::allTopics`.
- **Not on *Start here*** (decision 6); the path stays at 7 guides.
- `MainComponent::openGuide` opens it at a topic (`RoutingExplainer::showTopic`). `MainComponent::getPrintGuide` gives the open window or a new one for the PDF export.

## Code

### New: `src/engine/Routing.{h,cpp}`, namespace `daw::routing` (both targets, no UI)

Pure functions on the tree of tracks, for the engine, the views and the tests. `Tracks` is `std::vector<std::unique_ptr<Track>>`, as `Engine::getTracks()` returns.

```cpp
namespace daw::routing
{
int getDepth (const Track&);                                   // 0 at the top level, 1 in a bus, ...
std::vector<Track*> getMembers (const Track& bus, const Tracks&);   // its own tracks, in order
int getBlockEnd (const Tracks&, int index);                    // the index after the track and everything in it
bool playsInto (const Track& track, const Track& bus);         // directly or through other buses
bool canMoveInto (const Track& track, const Track* bus);       // null (the master) or a bus that isn't the track or inside it
bool isTidy (const Tracks&);                                   // every bus is followed by everything in it

/** Every track before the buses it plays into, otherwise in edit order: the order the engine renders in. */
std::vector<Track*> getRenderOrder (const Tracks&);

/** The rows the arrangement and the mixer show: everything but the tracks inside folded buses. */
std::vector<Track*> getVisibleTracks (const Tracks&);

/** The selected tracks without those inside another selected bus, in edit order. */
std::vector<Track*> getOutermost (const std::vector<Track*>&, const Tracks&);

/** The closest bus every one of them plays into (null: the master). */
Track* findCommonBus (const std::vector<Track*>&);

/** Solo and mute (see the plan's rule). */
bool isOnSoloPath (const Track&, const Tracks&);
enum class Silence { none, muted, busMuted, solo };
Silence getSilence (const Track&, const Tracks&);

/** "Kick → Drums → Master". */
juce::String describePath (const Track&);

/** Where a dragged track lands. In `bus` (null: the master), before `before` (null: at the end). */
struct Place { Track* bus = nullptr; const Track* before = nullptr; };

/** The levels (depths) a track can land at in the gap above visible row `gap` (rows.size(): below the last
    row): the deepest is the row above's level (one deeper when it is an open bus); the shallowest the row
    below's level (0 below the last row). */
std::pair<int, int> getGapLevels (const std::vector<Track*>& rows, int gap);   // { shallowest, deepest }
Place getPlace (const std::vector<Track*>& rows, int gap, int level);
}
```

### Changed: `src/engine/Track.{h,cpp}`

- `enum class Type { audio, midi, bus }`; `isBus()`, `isAudio()`.
- `Track* bus = nullptr;` and `bool folded = false;`: structural, changed only by the engine on the message thread under the lock.
- `std::atomic<float> busInputPeak { 0.0f }` (since the device bar last read it, as `faderInputPeak`) and `std::atomic<float> busInputLevel { 0.0f }` (with `updateMeterLevel`'s fall-back, for the guide's picture): the sum coming into a bus, before its effects.
- Audio thread only: `juce::AudioBuffer<float> busInput` (2 x the block size, sized in `prepare` for buses), and `bool soloAbove = false, soloBelow = false`, worked out by the engine per segment.
- `void process (const RenderContext&, juce::AudioBuffer<float>& destination, int destinationOffset)`: adds the output into `destination` (a bus's `busInput` at offset 0, or the mix buffer at the segment's offset). `RenderContext::bufferOffset` goes. A bus starts from its `busInput` (copied into `scratch`, peak into `busInputPeak` and `busInputLevel`) instead of clips and MIDI.
- `RenderContext` gets `bool onSoloPath`, set per track by the engine. The audible test becomes `! muted && (! ctx.anySolo || ctx.onSoloPath)`.
- `void clearBusInput (int numSamples)`.

### Changed: `src/engine/Engine.{h,cpp}`

New and changed functions (message thread; each takes the lock for the change, destroys plugins outside it, calls `updateRouting()` under the lock and `sendChangeMessage()`):

```cpp
/** Adds a track right below the track at `index - 1`, in the same bus as that track (below everything in it
    when it is a bus); at index 0 first, and with -1 last at the top level. */
Track* addTrack (Track::Type, int index = -1);

/** An empty bus, placed as addTrack places a track, named "Bus 1", "Bus 2"... */
Track* addBus (int index = -1);

/** Create bus: a new bus at the place of the first of `tracks` in their closest common bus, holding the outermost of
    them in edit order. `name` empty: "Bus N". Returns null for an empty list. Never changes a track's sound. */
Track* createBus (const std::vector<Track*>& tracks, const juce::String& name = {});

/** Moves a track (with everything in it) into `bus` (null: the master), before `before` (null: at the end).
    False, changing nothing, when routing::canMoveInto says no or `before` isn't in that bus. */
bool moveTrack (Track&, Track* bus, const Track* before = nullptr);

/** The track goes to where its bus plays, right below the bus. */
bool removeFromBus (Track&);

/** The bus's tracks take its place and output, in order; the bus and its effects are removed. */
void ungroup (Track& bus);

void setFolded (Track& bus, bool);

/** Removes the track and, for a bus, everything in it. */
void removeTrack (Track*);
```

- `addPlugin` refuses an instrument on a bus, with the error "A bus has no instrument: it plays what its tracks send into it."
- `resetEdit` clears the render order.
- Private: `std::vector<Track*> renderOrder` (rebuilt under the lock by `updateRouting()` from `routing::getRenderOrder`), and `void insertTrack (std::unique_ptr<Track>, Track* bus, const Track* before)`, used by `addTrack`, `addBus` and the loader.
- `renderSegment`:
  1. Works out `anySolo`. Then `soloAbove` in edit order (soloed, or the bus's `soloAbove`) and `soloBelow` in render order (soloed, or-ed into the bus after each member).
  2. Clears every bus's `busInput` for the segment.
  3. Processes `renderOrder`: each track into `bus->busInput` (offset 0) or into `mixBuffer` at `offset`, with `ctx.onSoloPath = soloAbove || soloBelow`. Live MIDI and previews go to the tracks as now; a bus ignores them.
- `isOwnPreviewSoundNeeded` already treats a non-MIDI live track as needing the engine's own synth, so a selected bus behaves like an audio track for previews.
- No allocation on the audio thread: `renderOrder` and the bus buffers are made on the message thread.

### Changed: `src/engine/ProjectFile.{h,cpp}`

- `trackToJson` writes `"type": "bus"`, `"folded"` when true, and a bus's `"tracks"` recursively (no `"clips"`). `toText` writes only the top-level tracks and sets `"version"` to 2 when there is a bus.
- `load` becomes recursive: `loadTrack (engine, json, parentBus, ...)` makes the track with `engine.addBus()` for `"type": "bus"` or `engine.addTrack (type)` otherwise (both last at the top level), then, when `parentBus` isn't null, `engine.moveTrack (*track, parentBus)` (to the end of that bus), and fills in its settings, plugins and clips as now. A bus then loads its `"tracks"` with itself as the parent, and `setFolded` last.
- The header comment describes buses and the versions.

### New: `src/engine/RoutingGuide.{h,cpp}`, namespace `daw::routingguide` (both targets, no UI)

```cpp
namespace daw::routingguide
{
enum class Topic { what, why, howTo, levels, glue, typical, solo, yourSong };
inline constexpr Topic allTopics[] { ... };
juce::String getTopicName (Topic);
juce::String getTopicSummary (Topic);
juce::String explain (Topic);        // yourSong: the general part; the song's part comes from explainSong

/** The sum of levels in dB: powers for different sounds, amplitudes for the same sound (and worst-case peaks). */
float sumDb (const std::vector<float>& levelsDb, bool sameSound);

enum class Family { none, drums, bass, guitars, keys, vocals };
Family getFamily (const Track&);
juce::String getBusName (Family);    // "Drums"...
/** "Drums" when every track is of one family and no bus has that name yet; otherwise "Bus N". */
juce::String suggestBusName (const std::vector<Track*>&, const Tracks&);

/** A box of the picture, as plain data: the song's tracks or an example. */
struct Node
{
    juce::String name;
    juce::Colour colour;
    bool isBus = false, folded = false, muted = false, soloed = false, silent = false, onSelectedPath = false;
    int parent = -1;                 // index of its bus in the list, -1: the master
    int numInside = 0;               // a folded bus: how many tracks it holds
    float levelDb = -100.0f, inputDb = -100.0f;   // live levels; a bus's input is its sum
    const Track* track = nullptr;    // the song's track, for clicks
};
std::vector<Node> getExample (Topic);
std::vector<Node> fromSong (const Tracks&, const Track* selected);   // levels from peakLeft/Right and busInputLevel

/** Positions in units: x = column (0 left, the master in the last), y = row (tracks one per row, a bus at the middle
    of its tracks). Index nodes.size() is the master. */
std::vector<juce::Point<float>> layOut (const std::vector<Node>&);

/** What the "your song" topic says, and what it suggests. */
struct Advice
{
    juce::String text;
    Topic topic = Topic::yourSong;
    enum class Action { none, createBus, addGlue } action = Action::none;
    std::vector<Track*> tracks;      // createBus: the tracks; addGlue: the bus
    juce::String busName;
};
struct Levels { std::map<const Track*, float> busInputDb; bool playing = false; };
juce::String explainSong (const Tracks&);
std::vector<Advice> analyse (const Tracks&, const Levels&);

/** The Glue topic's numbers, from DawIt Compressor's Drum bus starting point. */
juce::String describeGlueSettings();
}
```

### Changed: `src/engine/GainStaging.{h,cpp}`

- `StopKind` gets `busInput` (a bus's chain starts here: no input, out = the sum) and `bus` (a bus on the way: in = the sum of its tracks, out = after its fader). `Stop` gets `int numTracks = 0` for both.
- `findHints`: the source is the first stop's `outDb` for `instrument` and `busInput`. New hint (topic `bus`, not serious) on a `busInput` or `bus` stop whose sum is over 0 dB: "The 4 tracks in Drums add up to +2.1 dB peaks. Nothing distorts inside DawIt, but the bus's effects get a hot level. Turn the tracks down a little, together, or the first effect's input."
- `Topic::bus` with its name, summary and text (see *The guide*).

### Changed: `src/generate/TheoryGuides.{h,cpp}`

`Guide::routing` in the enum and `allGuides` (after `compressor`), its `getEntry`, and the *Mixing* summary.

### New: `src/ui/TrackActions.{h,cpp}`, class `daw::TrackActions`

The track menu and the bus commands, shared by the track headers, the mixer strips, the device bar and the *Track* menu. Owned by `ArrangementView` (like `StemSplitter`) and reached through `ArrangementView::getTrackActions()`.

```cpp
class TrackActions final
{
public:
    TrackActions (Engine&, Selection&, PluginWindowManager&, juce::Component& dialogParent);

    std::function<void (Track&)> onEditName;              // the arrangement opens the new bus's name for typing
    std::function<void (routingguide::Topic)> onShowGuide;   // Buses and routing, at a topic
    std::function<void (Track&, bool withItsTracks)> onDelete;   // the arrangement asks and deletes

    void showTrackMenu (Track& clicked, juce::Component* target);
    juce::PopupMenu makeOutputMenu (const Track&, int firstId) const;   // Master, the buses by level, New bus...
    void showOutputMenu (Track&, juce::Component* target);

    bool canCreateBus() const;                 // something selected
    Track* createBus();                        // from the selection, with routingguide::suggestBusName
    Track* createBus (const std::vector<Track*>&, const juce::String& name);   // the guide's action
    void removeFromBus (Track&);
    void ungroup (Track& bus);                 // asks first when it has effects; closes their windows
    void addGlueCompressor (Track& bus);       // DawIt Compressor with the Drum bus starting point, window opened
};
```

### Changed: `src/ui/Selection.h`

- `std::vector<Track*> tracks` (the selected tracks) next to `track` (the one clicked last, as now).
- `const std::vector<Track*>& getTracks() const`, `bool isSelected (const Track*) const`, `void toggle (Track*)` (Cmd-click), `void extendTo (Track*, const std::vector<Track*>& rows)` (Shift-click, from the one clicked last).
- `select` selects one track, as now. `forget` also takes the track out of the set.

### Changed: `src/ui/TrackHeader.{h,cpp}`

- Shift- and Cmd-clicks; the selection highlight from `isSelected`.
- The bus variant: fold arrow, **?**, no arm or scale button, "BUS · N tracks", the FX menu without instruments.
- The indent gutter for tracks in buses (controls laid out in what's left), and "BUS MUTED" / "SILENT" from `routing::getSilence`.
- Dragging the header: `onDragMove (TrackHeader&, const juce::MouseEvent&)`, `onDragEnd`, `onDragCancel` (Escape) for the arrangement.
- `showTrackMenu` goes to `TrackActions`. `editName()` opens the name for typing.

### Changed: `src/ui/ArrangementView.{h,cpp}` and `src/ui/TrackLanes.{h,cpp}`

- Rows come from `routing::getVisibleTracks`: `layoutContent` places only visible headers. `TrackLanes` keeps a cached row list instead of `engine.indexOf` for rows (in `getClipBounds`, `trackAtY`, `findClipAt`, `paint`, the copy drag's original row, `fileDragMove`).
- `TrackLanes::paint`: the bus lane's summary bars and empty-bus text, the tint behind tracks in a bus, the highlight of every selected track.
- `TrackLanes` becomes a `juce::TooltipClient` for bus rows. A double-click on a bus lane folds it. Files dropped on a bus go into a new audio track inside it.
- Guards: `importAudio()` takes the selected track only when `isAudio()`; double-click and file drops treat a bus as above.
- **The header drag:** a `TrackDropMarker` child of `content` (no mouse clicks, in front) draws the line or the bus outline. `ArrangementView::dragTrack` works out the gap (from y), the level (from x in the header column, between `getGapLevels`), or the bus under the middle third of a row, then `routing::getPlace`, checked by `canMoveInto` for every dragged track. `viewport.autoScroll` near the edges. The drop calls `engine.moveTrack` for each outermost selected track, in order.
- `deleteTrack (Track&, bool withItsTracks)`: closes the plugin windows and forgets every track that goes (the whole block for a bus with its tracks), or for a bus alone calls `TrackActions::ungroup`.
- `getTrackActions()`, `createBusFromSelection()` (for the command), `onShowRoutingGuide`.
- The `switch`es over `gen::guides::Guide` in `getPrintGuide` and `showGuide` get `case G::routing:` with the other main-window guides.

### Changed: `src/ui/MixerView.{h,cpp}`

- Constructed with `TrackActions&`. Strips in `getVisibleTracks` order; the bus strip (fold arrow, "BUS"); the bracket painted over `stripHolder`; the output button under the fader (20 px); Shift/Cmd-click; right-click menu; *SILENT* from `routing::getSilence`; the master's "→ Out".

### Changed: `src/ui/DeviceBar.{h,cpp}`

- Constructed with `TrackActions&`.
- `Chip::Kind` gets `busInput` and `bus`. `bus` chips are made per bus on the way: a `Chip` for a bus on the way keeps a `Track&` for that bus.
- Their meters: `busInputPeak` in and `faderOutputPeak` out for `bus`; `busInputPeak` out for `busInput`. Their stops map to the new `StopKind`s.
- The bus title, *+ Instrument* hidden for buses, the click actions.
- `dragChip` finds the fader chip by kind instead of `chips[chips.size() - 2]`, since bus chips can come after it.

### Changed: `src/ui/GainStagingExplainer.cpp`

`ChainPicture`: dashed lines into `bus` stops too; details "N tracks add up here" (busInput) and "with N other tracks, volume x dB" (bus).

### New: `src/ui/RoutingExplainer.{h,cpp}`, class `daw::RoutingExplainer`

```cpp
class RoutingExplainer final : public juce::Component, public guideprint::Source, private juce::ListBoxModel,
                               private juce::ChangeListener, private juce::Timer
{
public:
    RoutingExplainer (Engine&, Selection&);
    std::function<void (std::vector<Track*>, juce::String name)> onCreateBus;
    std::function<void (Track& bus)> onAddGlue;
    void showTopic (routingguide::Topic);
    // guideprint::Source: title "Buses and routing", source "the song's 9 tracks and 2 buses" or "an example song"
};
```

- Its `RoutingPicture` draws `routingguide::layOut` of the topic's nodes (see *The guide*), with clicks and tooltips.
- An action button under the text, shown when the advice has an action (*Create a Drums bus from Kick, Snare and Hats*, *Put DawIt Compressor on Drums*), as the structure guide offers *Set it up step by step*.
- The topic list and text as `GainStagingExplainer`; an `ExportButton` for `Guide::routing`.

### Changed: `src/ui/MainComponent.{h,cpp}`, `src/ui/TransportBar.{h,cpp}`

- A **Track** menu between *File* and *View* (`getMenuBarNames`: File, Track, View, Help; `getMenuForIndex`'s indexes shift by one): *New Audio Track* (`newAudioTrackCommand`), *New MIDI Track* (`newMidiTrackCommand`), *New Bus* (`newBusCommand`: an empty bus below the selected track, as `addTrack`'s rule places it, then selected), a separator, then *Create Bus* (`createBusCommand`, Cmd+Shift+G, "Put the selected tracks in a new bus", enabled when tracks are selected; `MainComponent::changeListenerCallback` calls `commandStatusChanged()` when the selection changes). The new IDs go at the end of `CommandIDs` and into `getAllCommands`, `getCommandInfo` and `perform`.
- `showRoutingGuide (routingguide::Topic)` with `juce::Component::SafePointer<RoutingExplainer> routingGuide`, opened through `pluginWindows.show`. Its `onCreateBus` and `onAddGlue` go to `arrangement.getTrackActions()`.
- `openGuide` and `getPrintGuide` get `case G::routing`.
- `mixer` and `deviceBar` take `arrangement.getTrackActions()` (they are declared after `arrangement`).
- TransportBar: an `addBusButton` "+ Bus" with `onAddBus`, in the `buttons[]` that scale to fit.

### Other callers

- `ChordStyles`, `Stems` and `LyricsActions` call `addTrack (type, indexOf (t) + 1)` meaning "below t": the new rule keeps that meaning, and puts the new track in t's bus.
- `BounceDialog.cpp`'s hint: "every track, bus, instrument and effect".
- `PluginActions`: `showAddMenu` and `add` refuse instruments on a bus.

### Threading

- Every structure change happens on the message thread under `Engine::getLock()`: `bus`, `folded`, the order of `tracks`, `renderOrder`, the bus buffers (sized in `prepare` and in `addBus`/`createBus`, as `addTrack` prepares a track).
- The audio thread only try-locks, as now, and reads `bus` and `renderOrder` under that lock. `soloAbove` / `soloBelow` and `busInput` are touched only by the audio thread.
- Mute, solo, volume and pan stay atomics. The views compute `getSilence` and `getVisibleTracks` on the message thread from the same data.
- Plugins of a removed bus are destroyed outside the lock, as `removeTrack` does now.

### Files

- **New:** `src/engine/Routing.{h,cpp}`, `src/engine/RoutingGuide.{h,cpp}` (both in `DawIt` and `DawItTests` in `CMakeLists.txt`); `src/ui/TrackActions.{h,cpp}`, `src/ui/RoutingExplainer.{h,cpp}` (`DawIt`).
- **Changed:** `Track.{h,cpp}`, `Engine.{h,cpp}`, `ProjectFile.{h,cpp}`, `GainStaging.{h,cpp}`, `TheoryGuides.{h,cpp}`, `Selection.h`, `TrackHeader.{h,cpp}`, `ArrangementView.{h,cpp}`, `TrackLanes.{h,cpp}`, `MixerView.{h,cpp}`, `DeviceBar.{h,cpp}`, `GainStagingExplainer.cpp`, `MainComponent.{h,cpp}`, `TransportBar.{h,cpp}`, `PluginActions.cpp`, `BounceDialog.cpp`, `tests/EngineTests.cpp`.

## Libraries and models

None.

## Steps

**Group A: the engine, the saving and the guide's content** (steps 1 to 5; build and test at the end of step 5).

- [x] 1. **Buses in `Track` and the routing helpers.**
  - Add `Type::bus`, `isBus`, `isAudio`, `bus`, `folded`, `busInputPeak`, `busInputLevel`, `busInput`, `soloAbove` / `soloBelow` and `clearBusInput` to `Track`; size `busInput` in `prepare` for buses.
  - Write `src/engine/Routing.{h,cpp}`.

  Files: `src/engine/Track.{h,cpp}`, `src/engine/Routing.{h,cpp}` (new), `CMakeLists.txt` (both targets), `tests/EngineTests.cpp`. Tests: the helper checks of `runRoutingTests()` on hand-built track lists (see *Tests*). Check: the tests print the render order and the visible rows of the example tree. Done when: the code compiles in the group's build.
- [x] 2. **The engine's structure functions.** `addTrack`'s rule, `addBus`, `createBus`, `moveTrack`, `removeFromBus`, `ungroup`, `setFolded`, `removeTrack` of a block, `insertTrack`, `updateRouting` and `renderOrder`, `addPlugin` refusing an instrument on a bus. Files: `src/engine/Engine.{h,cpp}`, `tests/EngineTests.cpp`. Tests: the structure checks of `runRoutingTests()`. Check: every check also asserts `routing::isTidy`. Done when: it compiles.
- [x] 3. **Rendering through buses.** `Track::process` with a destination; the bus starting from `busInput`; `RenderContext::onSoloPath`; `renderSegment`'s solo pass, bus clearing and render order. Files: `src/engine/Track.{h,cpp}`, `src/engine/Engine.cpp`, `tests/EngineTests.cpp`. Tests: `runBusAudioTests()` (summing, fader, pan, mute, solo, effects on a bus, nesting, moving while playing, bounce, the latency print). Check: the existing audio checks in `main` pass unchanged (a song without buses renders as before). Done when: it compiles.
- [x] 4. **Saving buses.** Nested JSON, `"folded"`, version 1 or 2, recursive loading. Files: `src/engine/ProjectFile.{h,cpp}`, `tests/EngineTests.cpp`. Tests: the bus checks added to `runProjectFileTests()`. Check: write a project with a bus to a temporary file in a test and read the JSON by eye once (then remove that line). Done when: it compiles.
- [x] 5. **The guide's content and gain staging.**
  - Write `src/engine/RoutingGuide.{h,cpp}`: topics, texts, `sumDb`, families, `suggestBusName`, examples, `fromSong`, `layOut`, `explainSong`, `analyse`, `describeGlueSettings`.
  - Gain staging: `StopKind::busInput` and `bus`, `Stop::numTracks`, the bus hint, `Topic::bus`.
  - `Guide::routing` and its entry in `TheoryGuides.cpp`; the *Mixing* summary. Add `case G::routing:` to the `switch`es in `ArrangementView.cpp` (with the main-window guides) and to `MainComponent::openGuide` / `getPrintGuide` (doing nothing until step 12), so the build stays free of `-Wswitch-enum` warnings.

  Files: `src/engine/RoutingGuide.{h,cpp}` (new), `CMakeLists.txt`, `src/engine/GainStaging.{h,cpp}`, `src/generate/TheoryGuides.{h,cpp}`, `src/ui/ArrangementView.cpp`, `src/ui/MainComponent.cpp`, `tests/EngineTests.cpp`. Tests: `runRoutingGuideTests()`, the additions to `runGainStagingTests()` and `runTheoryGuidesTests()`. Check: the tests print the analysis of the made-up song. Done when: `./build.sh --test` passes without warnings (end of group A).

**Group B: the arrangement, the mixer and the device bar** (steps 6 to 11; build and test at the end of step 11).

- [x] 6. **Selecting several tracks.** `Selection`'s set; Shift- and Cmd-click on headers and strips; the highlight in `TrackHeader`, `TrackLanes` and `ChannelStrip`; a right-click on a selected track keeps the set. Files: `src/ui/Selection.h`, `src/ui/TrackHeader.cpp`, `src/ui/TrackLanes.cpp`, `src/ui/MixerView.cpp`. Check: `./run.sh -- --demo`, Shift-click Lead then Drums: three headers highlighted; Cmd-click Bass: two. The device bar shows the last clicked.
- [x] 7. **Buses and folders in the arrangement.**
  - Rows from `getVisibleTracks`, `TrackLanes`' cached rows, the bus header, the indent gutter, the bus lane's summary and tooltip, the tint.
  - Folding by the arrow and by double-click; files dropped on a bus; the `importAudio` guard; "BUS MUTED" / "SILENT".

  Files: `src/ui/TrackHeader.{h,cpp}`, `src/ui/ArrangementView.{h,cpp}`, `src/ui/TrackLanes.{h,cpp}`. Check: in the demo, a temporary line in `MainComponent::createDemoContent` (removed after this step) makes a bus of Lead and Bass. It shows indented, folds, its lane shows their clips, and clips still drag between tracks but never onto the bus.
- [x] 8. **The track menu, Create bus and the Track menu.**
  - `TrackActions` with the menu (ending with the guide), the *Output* submenu, *Create bus* (suggested name, the name opened for typing, the bus selected), *Remove from bus*, *Ungroup* (asking when it has effects), *Delete bus and its N tracks...*, *Delete 3 tracks*.
  - `ArrangementView::deleteTrack` for blocks; `createBusCommand` (Cmd+Shift+G) and the *Track* menu; the **+ Bus** button.

  Files: `src/ui/TrackActions.{h,cpp}` (new), `CMakeLists.txt` (`DawIt`), `src/ui/TrackHeader.{h,cpp}`, `src/ui/ArrangementView.{h,cpp}`, `src/ui/MainComponent.{h,cpp}`, `src/ui/TransportBar.{h,cpp}`, `src/ui/PluginActions.cpp`. Check: select Lead and Bass, Cmd+Shift+G: a bus "Bus 1" with its name open; rename to "Band"; *Output > Master* on Bass takes it out; *Ungroup* removes the bus; the menu's last item is *About buses and routing...* (opens nothing until step 12).
- [x] 9. **Dragging tracks in and out.** `TrackHeader`'s drag callbacks, `TrackDropMarker`, the gap and level from the mouse, dropping onto a bus row, several tracks at once, auto-scroll, Escape. Files: `src/ui/TrackHeader.{h,cpp}`, `src/ui/ArrangementView.{h,cpp}`. Check: drag Drums onto the bus row (outlined), out again below the bus by moving left at the gap, and a bus into itself (no line).
- [x] 10. **The mixer.** Strips in row order, the bus strip and fold arrow, the bracket, the output buttons and menu, multi-select, right-click menu, *SILENT* from `getSilence`, the hint text. Files: `src/ui/MixerView.{h,cpp}`, `src/ui/MainComponent.h` (the constructor argument). Check: Cmd+2 with a bus: the bracket over its tracks, "→ Band" under them; folding the bus in the arrangement hides their strips; solo a track in the bus: the bus stays lit, the other tracks say *SILENT*.
- [x] 11. **The device bar.** The `bus` and `busInput` chips, their meters, stops, clicks, menus and tooltips; the bus title; *+ Instrument* hidden; `dragChip`'s fader lookup; `GainStagingExplainer`'s dashed lines and details; the *Track* menu's *Create Bus* enabled state. Files: `src/ui/DeviceBar.{h,cpp}`, `src/ui/GainStagingExplainer.cpp`, `src/ui/MainComponent.h`. Check: play the demo with Lead and Bass in a bus; Lead's chain shows → Band → Master with levels; click Band: its chain starts with "From Lead, Bass"; put DawIt EQ on it with +12 dB: a ! about the hot level, and *Gain staging?* opens *Through a bus*. Done when: `./build.sh --test` passes without warnings (end of group B).

**Group C: the guide window** (step 12; build and test at its end).

- [x] 12. **The Buses and routing guide.**
  - `RoutingExplainer` with `RoutingPicture`, the topics, the action button, the guideprint `Source` and `ExportButton`.
  - `MainComponent::showRoutingGuide`, `openGuide`, `getPrintGuide`; opening from the track menu, the bus header's **?**, the mixer's output menu and the bus chain's first chip.

  Files: `src/ui/RoutingExplainer.{h,cpp}` (new), `CMakeLists.txt` (`DawIt`), `src/ui/MainComponent.{h,cpp}`, `src/ui/ArrangementView.{h,cpp}`, `src/ui/TrackActions.cpp`, `src/ui/MixerView.cpp`, `src/ui/DeviceBar.cpp`. Check: open it from each place; every topic shows its picture; *Your song's routing* offers *Create a Drums bus...* for three drum-named tracks and does it; *Put DawIt Compressor on Drums* adds it with the Drum bus settings; Cmd+G > search "bus" lists it first; `--export-guides /tmp/dawit-guides` with `--demo` prints a line for *Buses and routing* and returns 0. Done when: `./build.sh --test` passes without warnings and the export run returns 0.

- [x] 13. **Real-app check.**
  - Copy `~/Library/Application Support/DawIt/DawIt.settings` aside first and put it back afterwards (never read it).
  - Patch `src/Main.cpp` temporarily with a hook behind `DAWIT_HOOK`. It sets up the A minor pop song (sections, chords, lyrics, as for the website's lyrics shots) with MIDI tracks Kick, Snare and Hats on the DawIt Synth's *Drums* sound (notes 36, 38, 42), grouped into a **Drums** bus with DawIt Compressor on the *Drum bus* starting point; Lead vocal, BV 1 and BV 2 on the *Lead* sound in a **Vocals** bus, with BV 1 and BV 2 in a **Backing vocals** bus inside it, Vocals folded; Bass and Keys at the top level. It plays from bar 9.
  - Save 2x PNGs with `createComponentSnapshot (..., 2.0f)` after `juce::Timer::callAfterDelay`:
    1. the main window: the arrangement with Drums open and Vocals folded, Kick selected, the device bar showing Kick → Drums → Master;
    2. the right-click menu on three selected tracks (*Create bus from 3 tracks*);
    3. the mixer with the bus strips, the bracket and the output buttons;
    4. the device bar with Drums selected (From Kick, Snare, Hats → DawIt Compressor → Fader → Master);
    5. *Buses and routing* at *Your song's routing*, *Levels add up* (Drums selected, playing) and *Glue*;
    6. the gain staging guide at *Through a bus*.
  - Look at them for: no clipped or overlapping text at the deepest indent; the gutter and bracket in the bus's colour; folded Vocals showing its tracks' clips as bars; meters moving on buses; *SILENT* / *BUS MUTED* right after soloing Kick and muting Vocals; the picture's tree readable with 11 boxes.
  - Also run without the hook: open a project saved before this feature (version 1) and check it plays and saves as before.
  - Then `git checkout -- src/Main.cpp` (restore a saved copy if the file has other changes) and rebuild cleanly.

  Done when: every screenshot looks right and `./build.sh --test` passes on the clean tree.
- [x] 14. **Docs.**
  - **README:**
    - a *Features* bullet "**Buses**": select tracks (Shift-click, Cmd-click), right-click > *Create bus* or Cmd+Shift+G, the bus's effects, fader, pan, mute, solo and meter, folding, dragging in and out, *Output*, *Ungroup*, the solo rule, the mixer and device bar;
    - a bullet for the *Buses and routing* guide;
    - update the *Mixer* bullets (lines 9 and 26) for bus strips and the output buttons;
    - *Tests* lines for `runRoutingTests`, `runBusAudioTests`, `runRoutingGuideTests` and the additions to the project, gain staging and Theory Guides tests;
    - *Code layout* lines: `Routing.{h,cpp}` ("the tree of tracks and buses: render order, visible rows, solo, where a dragged track lands"), `RoutingGuide.{h,cpp}` ("the Buses and routing guide's topics, the sums, families and the advice on your song"), `TrackActions.{h,cpp}` ("the track menu, Create bus, Output, Ungroup"), `RoutingExplainer.{h,cpp}` ("the Buses and routing guide window: the routing tree drawn live");
    - *Known limitations*: no plugin delay compensation for tracks or buses (as today), no sends yet.
  - **FEATURE_IDEAS.html:** under *Aux sends and buses*, a `<p class="status partly"><strong>Partly done:</strong> ...</p>` saying what's built (buses, the guide) and what's open (see *Left for later*). Under *Mixer view*, update the status: group/bus tracks done. Under *What is mixing?*'s *Done* line, add the *Buses and routing* guide.
  - **CLAUDE.md:**
    - the guide table: "Buses and routing | a track's right-click menu, the ? in a bus's header, the mixer's Output menu";
    - in *Engine and data*: tracks form a tree (`Track::bus`). `getTracks()` is the edit order, with each bus followed by everything in it. The views use `routing::getVisibleTracks` for rows and `routing::getSilence` for silent. `addTrack (type, index)` puts a track below the one at `index - 1`, in its bus. Structure changes go through the engine's bus functions;
    - in *UI > Interaction*: Shift-click and Cmd-click select several tracks.
  - Tick the steps here and set `Status: Built, not committed`.

## Tests

### `runRoutingTests()` (new; steps 1 and 2)

On a tree built through the engine: Kick, Snare, Hats in **Drums**; **Vocals** holding Lead and **Backing** (BV 1, BV 2); Bass at the top level.

- **Order and tidiness:** `getTracks()` is Drums, Kick, Snare, Hats, Vocals, Lead, Backing, BV 1, BV 2, Bass, and `isTidy` is true. `getRenderOrder` is Kick, Snare, Hats, Drums, Lead, BV 1, BV 2, Backing, Vocals, Bass: every track before its buses. A song without buses renders in its edit order.
- **Depth and paths:** BV 1 has depth 2, and `describePath` gives "BV 1 → Backing → Vocals → Master" (with the UTF-8 arrow). `playsInto (BV 1, Vocals)` is true; `playsInto (Kick, Vocals)` is false. `getMembers (Vocals)` is Lead, Backing.
- **Folding:** with Backing folded, `getVisibleTracks` leaves out BV 1 and BV 2; with Vocals folded, it leaves out Lead, Backing, BV 1 and BV 2 whatever Backing's state.
- **createBus:**
  - from Kick and Hats at the top level: the bus takes Kick's place, holds Kick and Hats in order, and Snare stays below it;
  - from Snare (in Drums) and Bass: the new bus is at the top level, before Drums, holding Snare then Bass, and Drums still holds Kick and Hats;
  - from Drums and Kick: only Drums moves, and Kick stays inside it;
  - the name is "Bus 1", then "Bus 2";
  - an empty list returns null;
  - the tracks' plugins, built-in sounds and clips are unchanged (generating and grouping never change a sound).
- **moveTrack:**
  - BV 1 to the master before Bass, then into Drums at the end, then into Vocals before Lead;
  - Vocals into Backing is refused and changes nothing (`canMoveInto` is false);
  - a track into itself is refused;
  - `before` from another bus is refused.
  - Every check also asserts `isTidy`.
- **removeFromBus:** BV 2 goes into Vocals right below Backing.
- **ungroup:** Backing's BV 1 and BV 2 take its place inside Vocals, in order. Backing and its DawIt EQ are gone. Ungrouping Drums puts Kick, Snare and Hats at the top level where Drums was.
- **addTrack's rule** ("right below the track at `index - 1`, in its bus; below everything in it when it is a bus"):
  - at `indexOf (Kick) + 1` the new track is in Drums, right after Kick;
  - at `indexOf (Hats) + 1` (the last in Drums) it is in Drums, after Hats;
  - at `indexOf (Drums) + 1` it is at the top level, after Hats and before Vocals (below the bus and everything in it);
  - at `indexOf (BV 2) + 1` it is in Backing, after BV 2;
  - at 0 it is first at the top level, and at -1 last at the top level.
- **removeTrack** of Vocals removes Lead, Backing, BV 1 and BV 2 too.
- **Plugins on a bus:** DawIt EQ goes on Drums; DawIt Subtractive is refused with the bus error, and Drums' plugin list is unchanged.
- **Solo and silence:**
  - with Kick soloed, `isOnSoloPath` is true for Kick and Drums and false for Snare and Bass; `getSilence (Snare)` is `solo`;
  - with Drums soloed, Kick, Snare and Hats are on the path;
  - with Drums muted and nothing soloed, `getSilence (Kick)` is `busMuted` and `getSilence (Drums)` is `muted`.
- **Dropping between rows** (on visible rows Drums, Kick, Snare, Hats, Vocals, Lead, Backing, BV 1, BV 2, Bass):
  - in the gap below Hats, `getGapLevels` is {0, 1}; level 1 gives { Drums, end } and level 0 gives { master, before Vocals };
  - below the Drums row (open), it is {1, 1}: inside Drums before Kick;
  - with Drums folded, the gap below its row is {0, 0}: { master, before Vocals };
  - below BV 2, it is {0, 2}: level 2 is { Backing, end }, level 1 { Vocals, end } and level 0 { master, before Bass };
  - below the last row (Bass), it is {0, 0}: { master, end }.

### `runBusAudioTests()` (new; step 3)

With a `FakeDevice`-driven engine at 48 kHz and in-memory audio clips (`addAudioClip` with a buffer): A and B each a 440 Hz sine peaking at 0.25, C a 660 Hz sine peaking at 0.25. A and B go into a bus. Each check renders past the 20 ms smoothing first.

- **Summing:**
  - A and B (the same sine) come out of the bus and the master peaking at **0.50 ± 0.01**;
  - the bus's `busInputPeak` is 0.50 ± 0.01;
  - with B swapped for C, the master's RMS is **0.250 ± 0.005**, against 0.177 for one sine (+3.01 dB).
- **Fader:** the bus at -6 dB gives a master peak of 0.50 × 0.501 = **0.251 ± 0.01**; A's and B's own meters still read about 0.25.
- **Pan:** the bus hard left leaves the right channel's RMS below 1e-4.
- **Mute:** the bus muted gives silence (RMS below 1e-4), while A's `peakLeft` stays above 0.2.
- **Solo:**
  - A soloed: the master peaks at 0.25 ± 0.01 (A through the bus, B silent);
  - with a top-level track D also playing, D is silent;
  - the bus soloed: A and B (0.50) and not D.
- **An effect on the bus:** DawIt EQ with a +6 dB bell at 440 Hz (Q 1) on the bus raises the master's peak to **1.0 ± 0.05**, and its slot's `inputPeak` is the sum (0.50 ± 0.02).
- **Nesting:** A in an inner bus inside an outer bus, both at -6 dB: the master peaks at 0.25 × 0.501 × 0.501 = **0.063 ± 0.005**.
- **Changing while playing:** moving B out of the bus while playing, then ungrouping, renders without a crash and with the expected peaks after (0.25 from the bus, 0.50 in total).
- **An empty bus** is silent and costs nothing (its meters stay at 0).
- **Bounce:** `bounce::bounceToFile` with the bus at -6 dB gives a file peaking at 0.251 ± 0.01; with the bus muted, a silent file.
- **Latency:** DawIt Saturation added to a track reports `getLatencySamples() > 0`; the message prints the number ("not compensated yet").

### Project files (added to `runProjectFileTests()`; step 4)

- A song with Drums (folded, muted, -2 dB, DawIt Compressor with changed settings) holding Kick and a nested Sub bus holding Snare, plus Bass, saves with `"version": 2`, `"type": "bus"`, `"folded": true`, Kick inside Drums' `"tracks"`, and no `"clips"` on the buses.
- Loading gives the same text again (`toText` equal). The tree is the same: Kick's `bus` is the loaded Drums, and Snare's `bus` is Sub. The compressor's settings come back. `isTidy` is true.
- The same song without buses saves `"version": 1`, with no `"folded"` and no `"tracks"` inside a track. The existing checks of the test still pass unchanged.
- A hand-written version-1 project string, as saved before this feature, loads with its tracks in order and saves back to the same text.
- `"version": 3` is refused with "newer".

### `runRoutingGuideTests()` (new; step 5)

- Every topic has a name, a summary, text longer than 200 characters, and an example of at least 3 nodes; *Your song's routing*'s example is the pop song.
- **sumDb:**
  - {-6, -6} different: **-2.99**; the same: **+0.02**;
  - four at -10: **-3.98** different and **+2.04** the same;
  - {0} is 0;
  - {-6, -100} is -6 (silence adds nothing).
  - Each within 0.01.
- **Families:**
  - "Kick" and "Bass drum": Drums; "Bass", "Bass guitar" and "Sub": Bass;
  - "BV 2" and "Lead Vox": Vocals; "Gtr L": Guitars; "Rhodes": Keys;
  - "Strings": none;
  - a MIDI track "MIDI 3" on the *Drums* sound: Drums.
- **suggestBusName:**
  - Kick, Snare, Hats: "Drums";
  - with a bus already called Drums: "Bus 1";
  - Kick and Bass: "Bus 1".
- **layOut** on the pop-song example:
  - every bus is further right than its tracks, and the master is right of everything;
  - every bus's y is the middle of its tracks' ys;
  - a top-level track has a row of its own;
  - no two boxes share a position.
- **analyse**, on a made-up song: Kick, Snare, Hats, Bass, Lead Vox, BV 1, BV 2 at the top level:
  - two `createBus` advices, for Drums (Kick, Snare, Hats) and Vocals (Lead Vox, BV 1, BV 2), in that order, each text naming the tracks;
  - no advice for Bass alone.
  - After `createBus` of the drums: an `addGlue` advice for Drums whose text contains "2.0:1" and "30 ms", and no more Drums `createBus` advice.
  - A bus with only Lead Vox: the "a bus to itself" advice. An empty bus: the empty advice.
  - Buses three levels deep: the nesting advice.
  - With `Levels` saying Drums' input is +2.1 dB while playing: the "add up" advice with "+2.1 dB"; not while stopped.
  - A song with no buses and one track of each family: only the "every track goes straight to the master" text.
- **explainSong** on the tree of `runRoutingTests` says "7 tracks and 3 buses" and names Drums, Vocals and Backing with their counts.
- **describeGlueSettings** contains `comp::formatKnob`'s ratio, attack and release of the *Drum bus* preset (so "2.0:1", "30 ms", "150 ms").

### Gain staging (added to `runGainStagingTests()`; step 5)

- *Through a bus* has its name, summary and text, like the other topics (the loop over `allTopics` already checks them).
- A bus chain (busInput out at +2 dB, an effect, fader, master at -6 dB) gives one hint, on stop 0, with topic `bus`, not serious, its text containing "+2.0 dB".
- A track's chain with a `bus` stop whose in is +1 dB gives the `bus` hint on that stop.
- A busInput at -45 dB gives the "very quiet from the start" hint (the source is its out level).

### Theory Guides (added to `runTheoryGuidesTests()`; step 5)

- *Buses and routing* has an entry in *Mixing*, with its topics from `routingguide::allTopics` (same count, same first name).
- Searches: `first ("bus")`, `first ("folder")` and `first ("submix")` are "Buses and routing"; "group tracks" finds it.
- The existing searches still pass ("Compress" → *What is compression?*, "muddy" → *What is EQ?*).
- The *Start here* path is unchanged (7 guides).

## Website hand-off

For dawWebsite, which asks the user first:

- **Features page:** a section `id="buses"` after `gain-staging`, with the step label "Buses" and the h2 "Put tracks in a bus, and mix them as one".
  - The intro: a bus is a track that other tracks play into, so a group gets one set of effects and one fader; in DawIt you make one like a folder.
  - The list:
    - **Create bus** from selected tracks (Shift-click, Cmd-click, right-click or Cmd+Shift+G), with a suggested name;
    - its own effects, fader, pan, mute, solo and meter;
    - fold it to one row that still shows where its tracks play;
    - drag tracks in and out, or pick an *Output*;
    - buses inside buses;
    - solo a track and hear it through its bus;
    - the mixer's bus strips and output buttons;
    - the device bar's chain into the bus, with the levels adding up;
    - the *Buses and routing* guide.
  - A wide screenshot of the arrangement, and a figure of the mixer.
  - Update the page's `meta description` list.
- **Screenshots** (the A minor pop song set up as in step 13, at 2x, from the real app):
  1. the arrangement with Drums open and Vocals folded, the device bar showing Kick → Drums → Master (wide shot);
  2. the mixer with the bus strips, the bracket and the output buttons;
  3. the device bar of the Drums bus, starting from "From Kick, Snare, Hats";
  4. *Buses and routing* at *Your song's routing*, with the tree and the advice;
  5. optionally the right-click menu with *Create bus from 3 tracks*.
- **Main page:**
  - in *3 Take it further*, a list item: "Group tracks into buses, like folders, with their own effects and fader";
  - in the guide picker, *Buses and routing* in the *Mixing* area (a tab button with its description, an entry in `guides` with screenshot 4, its `more` link to `features.html#buses`, and `guide-routing` added to the Mixing `areas` entry), not on *Start here*.
- **How it's made:** no change (no new library, model or tool).

## Left for later

- **Aux sends and return tracks:**
  - a send knob per track (pre- or post-fader) to a return track;
  - send knobs in the mixer strips and branches in the device bar;
  - reverb and delay starting points "for a return track" at 100% wet;
  - the guide's send topics (insert against send effects, pre and post fader, depth by send level).
  The `Track::bus` output, the render order (returns render after the tracks that send to them) and the mixer strip's output area are where they go.
- **Plugin delay compensation** for tracks and buses (decision 3).
- **Bouncing a bus on its own** (stems per bus) and *Export each bus as a WAV*.
- **Colour tracks like their bus** as a menu item (decision 5b).
- **Deleting and duplicating tracks with keys:** Delete removing the selected tracks, Cmd+D duplicating a track or a bus with its tracks, copy and paste of tracks.
- **VCA-style groups:** faders linked without summing the audio.
- A **side-chain from a bus** (ducking the music under a vocal bus).
- **The master track with effects** (its own idea), which would make the master the last bus in the device bar and the guide.
- **Compare the levels of the buses and advise on the balance** (its own idea), built on these buses.

## Changes while building

- `TrackActions::onDelete` takes a list of tracks (`std::vector<Track*>, bool withItsTracks`) so *Delete 3 tracks* asks once; `ArrangementView::deleteTracks` replaces `deleteTrack`.
- Output > *New bus...* and the menus' *Create bus* share one path (`TrackActions::createBus`). Output > *Master* puts the track right below the top-level bus it was in (not at the end of the song).
- `RenderContext::bufferOffset` went; `Track::process` takes the destination and an offset, as planned. `TrackLanes::getRows()` recomputes the visible rows on every call instead of keeping a cache that could point at deleted tracks.
- A bus's `busInput` stop in the gain staging takes the bus's name, and the first effect after a hot bus does not repeat the "over 0 dB going in" hint (the bus's hint covers it), so the planned "one hint" holds.
- The routing guide's *Levels add up* picture is a bar chart (examples, or the selected bus's tracks and measured sum while playing); the other topics draw the tree.
- Real-app check: the right-click menu is a native macOS menu that can't be snapshotted from inside the app, so shot 2 (the menu) was not taken; the other shots were. Tests: the final run once ended with a crash at exit (`recursive_mutex lock failed`) after `All checks passed`; it did not repeat in 12 more runs and the cause was not found.
- Added after the plan: a Track menu item set (*New Audio Track*, *New MIDI Track*, *New Bus*, *Create Bus*), as planned; the guide's own text says the suggestion button is above the text.
- Added after the plan: a ninth topic, *The mix bus (the master)* (`Topic::mixBus`, after *Glue*), on the master as the last bus: headroom (peaks about -6 dBFS, `getHeadroomDb`, `describeMasterLevel`), the *Mix bus* starting point's numbers (`describeMixBusSettings`), what not to do, and that the master has only a fader and a meter today. Its picture is the routing tree with the master highlighted and a live peak bar with the -6 dB mark; the *Your song's routing* advice names a master that clips or has under 6 dB of headroom while playing (`Levels::masterPeakDb`). It opens from a ? on the mixer's master strip and that strip's right-click menu (through `TrackActions::onShowGuide`); the device bar's master chip and the transport bar's master slider have no link. The Theory Guides entry has the search words *mix bus*, *master bus*, *stereo bus*, *bus compressor*, *glue the mix* and *headroom*.
- Advice for tracks that sit in a bus of another kind (drums in Vocals): the *Create a Drums bus* action now puts the new bus next to that bus (in the bus it is in, right after it) with `Engine::createBusBeside`, taking the tracks out of it; the advice says "next to Vocals rather than inside it". `Engine::createBus` is unchanged (closest common bus), so the Output menu and Cmd+G-style grouping of a selection behave as before. The advice groups tracks by the place they play into, so drum tracks in the master and drum tracks in Vocals are two pieces of advice, each with its own action; nothing groups tracks from different buses into one bus.
- When a bus of the same kind exists elsewhere (a Drums bus, found by `getFamily` on the bus), two or more further tracks get `Advice::Action::moveIntoBus` ("Move them into Drums, ..."), whose button calls `Engine::moveTrack` for each (`TrackActions::moveIntoBus`), instead of making "Bus 1".
- The hot-bus hint (`gainstaging::findHints`, via the new `Stop::busHasEffects`, set by `DeviceBar`) and the hot-bus advice in the routing guide stay quiet for a bus with no effects.
