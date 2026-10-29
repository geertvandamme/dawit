# An inspector: the selected track's pan, volume, effects and sends beside the tracks

Status: Built, not committed
Idea: [An inspector: the selected track's pan, volume, effects and sends beside the tracks](../features_done.html#an-inspector-the-selected-tracks-pan-volume-effects-and-sends-beside-the-tracks)

## Goal
A narrow, full-height panel on the left of the track headers, as in Logic Pro, that shows the selected track's effects, sends (one per return, with the return's full name), pan and volume. A beginner shapes one sound without opening the mixer, sees every send a track has (also beyond the mixer's four), and learns from its order and its guide topic how inserts and sends differ.

## Decisions for you
Already answered: sends show the return's full name; the track header's Sends button stays as it is.

1. **Width.** Options: resizable by a drag edge, 200 to 360 px, default 230, remembered; or fixed 220 px. **Recommended:** resizable, because long return names ("Plate - vocals") and long plugin names need room on some songs and not others.
   Answer: resizable 200 to 360 px, default 230, remembered (recommended)
2. **How tall.** Options: from under the transport bar to the bottom of the window, beside the arrangement, the device bar and the bottom panel (Logic Pro's layout); or only beside the arrangement. **Recommended:** the full height, as asked ("full-height"); the mixer and the piano roll get 230 px narrower while it's open. The computer keyboard strip stays full width at the very bottom.
   Answer: full window height, beside the arrangement, device bar and bottom panel (recommended)
3. **Open on first launch.** Options: shown until the user hides it; hidden until turned on. **Recommended:** shown, so a beginner finds it; hiding it is remembered.
   Answer: shown until the user hides it (recommended)
4. **Shortcut.** Options: plain I (Logic Pro's); Cmd+8 (next to the other View items, Cmd+1 to 7). **Recommended:** I, as in Logic. I is not in the computer keyboard's note map, and a text box being typed in keeps its letters. If I turns out to surprise people while playing notes with the letter keys, Cmd+8 is the fallback.
   Answer: plain I (recommended)
5. **Several tracks selected.** Options: show the one clicked last and change only it; change all selected together. **Recommended:** the one clicked last (`Selection::getTrack()`, which the device bar and the piano roll follow too), with a dim line "3 selected; showing Lead vocal". Changing several at once stays the mixer's and the track menu's job.
   Answer: the one clicked last, change only it (recommended)
6. **Nothing selected:** the master (volume and meter only, as the idea says). **Returns:** name, effects, pan and volume, no sends (a return can't send), and a *Fed by* list of the tracks that send to it. **Buses:** as tracks. **Recommended** as written.
   Answer: as recommended: master when nothing is selected, returns show no sends, buses work like tracks
7. **Effects.** Options: a vertical list of slots of its own, built on the `plugins::` helpers the device bar uses; or the device bar's chips (horizontal, with pictures and gain-staging badges, about 160 px each, which don't fit 230 px). **Recommended:** the vertical list, without badges or pictures; the device bar keeps the gain staging.
   Answer: a vertical list of its own (recommended)
8. **More than four returns.** **Recommended:** the inspector lists every return, scrolling with the effects when there are many. The mixer stays at four knobs (that open item stays open, under *Left for later*).
   Answer: every return listed, scrolling; the mixer stays at four knobs (recommended)
9. **Mute, solo and Output.** Not asked for, but they're on every strip. **Recommended:** include one row with M, S and the Output button, copied from the strip, because a beginner looking at one track expects them there.
   Answer: include the M, S and Output row (recommended)

## What the user does and sees
**Opening.** *View > Inspector* (ticked while shown, shortcut I), the × in its title row, and the right-click menu's *Hide Inspector*. Open state in `view.inspectorOpen` (default true), width in `view.inspectorWidth` (default 230, 200 to 360).

**Layout** (230 px wide, on `theme::panel`, a 1 px `gridBar` line on its right edge, then a 4 px drag edge):
```
┌ Inspector                    [?] [×] ┐  28 px title row (13 bold, textDim)
│ ▌Lead vocal                          │  colour bar + name (15 bold, shrinks to fit)
│  AUDIO TRACK · in Vocals             │  type and where it plays (10.5, textDim)
│  3 selected; showing Lead vocal      │  only with several selected
│  [M] [S]   [→ Vocals          ]      │  mute, solo, Output (as on the strip)
│ EFFECTS ─────────────────────────────│  section heading (10 bold, textDim)
│  Instrument: DawIt Synth · Piano     │  MIDI tracks only, 24 px
│  ● DawIt EQ                       ×  │  24 px per effect: power dot, name, x on hover
│  ● DawIt Compressor               ×  │
│  + Effect                            │
│ SENDS ───────────────────────────────│
│  (◔) Reverb               -12.0 dB   │  24 px per return: 20 px dial, full name in the
│  (◔) Plate - vocals   PRE  -6.0 dB   │  return's colour, PRE in meterYellow, level
│  + Send to a new return              │  menu: Reverb, Delay, Empty
│ PAN        ( ◠ )   12 L              │  38 px knob, value
│ VOLUME                               │
│   ┃fader┃ ▌▌meter                    │  the rest, at least 140 px; dB box below
│   -3.0 dB                            │
└──────────────────────────────────────┘
```
Effects and sends sit in one viewport that scrolls when they don't fit; pan and volume stay at the bottom. A return shows *FED BY* instead of *SENDS* (a row per sender: its name in its colour and the send level; click selects that track). The master shows "Master", "THE MIX BUS", the volume (master volume) and meter, and one dim line: "The master has no pan, effects or sends. It's the sum of everything."

**Interaction** (CLAUDE.md's UI rules):
- Effect row: click opens its window (`plugins::open`, the instrument row `plugins::openBuiltInSynth` or the plugin), click the dot to bypass, × removes (as the chip does, after the callback), drag up or down to reorder (`Engine::movePlugin`, an accent line where it lands), right-click `plugins::showSlotMenu`. *+ Effect* opens `plugins::showAddMenu` (effects); the instrument row's right-click opens it for instruments.
- Send row: drag the dial (or the row's text) up for more, double-click for Off, right-click `TrackActions::showSendMenu` (which ends with the guide). *+ Send to a new return* opens Reverb, Delay, Empty and calls `TrackActions::addReturn` with this track as the sender (starts at -10 dB), selecting the new return as it does now.
- Pan and volume: as the mixer strip (double-click centres, resets to 0 dB; no scroll wheel).
- A click on the empty panel does nothing; right-click: *Hide Inspector*, then `guideicon::addItem` "About the inspector..." (the routing guide at *The inspector*). The ? does the same.
- No control takes keyboard focus (ui-design.md *Focus*), so Space and the letter keys keep working.

Tooltips: in *Theory*.

## Data and saving
No project data. Two app settings, `view.inspectorOpen` and `view.inspectorWidth`. Nothing to save-and-load test.

## The guide
New topic in **Buses, sends and routing** (`routingguide`), after *Returns in DawIt* (`sendHowTo`):
- **The inspector** — "One track's effects, sends, pan and volume, beside the tracks". Picture: with a song and a selected track, the song tree (`fromSong` with the selected track, its path lit, as the solo topic does); otherwise the sends example with Vocal's path lit (Vocal, its send to Reverb, Reverb, the master) and the others dimmed. Text: *Theory*, plus the selected track's line from `describeSends` at the end ("In your song: ...").
- **Your song's routing** gets the same line for the selected track at the top of its text.
- Two existing topics get one sentence each about the inspector (*Theory*): *Returns in DawIt* and *Pre-fader and post-fader*.
- `gen::guides` entry: add "the inspector's ? and right-click menu" to `opensFrom` and the search words "inspector", "channel strip", "selected track". Not on *Start here* (the guide isn't now either). The topic list follows `allTopics` by itself.

## Code
Recipes: *Topics in an existing guide*, *A command or menu item*, *Checking in the real app*. No new kind of thing beyond a side panel; the docs get it (Group C).

- **Shared send control.** `ChannelStrip::SendKnob` moves out of `MixerView.cpp` into its own UI file as one class both views use: it takes the engine, `TrackActions`, the sending track and the return, and checks both are still in the edit before any write (as `isAlive` does now). Two looks: *compact* (the mixer's 22 px row, exactly as it looks now) and *wide* (full name, the level in dB on the right, PRE). The name, PRE and level painting move into it, so the strip's `paintOverChildren` goes. Its tooltip and right-click stay.
- **In sync, without new plumbing.** Both views read the same `Send` atomics and track atomics on their timers and write through the same paths (`Engine::setSend` for sends; `volumeDb`, `pan`, `muted`, `soloed` and `masterVolumeDb` straight, as the strip does). Never set a value while the user drags it (the existing `isMouseButtonDown` guard).
- **`InspectorPanel`** (`src/ui`), owned by `MainComponent` beside the arrangement: a `ChangeListener` on the engine and the selection, a 30 Hz timer for meters and values. It shows `selection.getTrack()` or the master when null. It keeps a `Track*` only while `engine.indexOf` finds it, and rebuilds its rows when the shown track, its plugin list, the returns or `getEditGeneration()` change (the device bar's checks are the model). Meters use the falling levels (`peakLeft/Right`, `masterPeakLeft/Right`), never the stage peaks the device bar consumes.
- **Pure functions** (engine, tested): `routing::getSendTargets` (the returns a track can send to, in edit order: all of them for a track or bus, none for a return or the master) and `routingguide::describeSends` (the "In your song" sentence for one track, the master, or a return). The inspector's rows come from the first; the guide's line from the second.
- **Layout** in `MainComponent::resized`: after the transport and the typing strip, the panel takes its width off the left of the rest, then the existing layout runs on what's left. A drag edge component sets the width, clamped and remembered on mouse-up.
- **Threading:** no audio-thread change; structural changes (plugins, sends, returns) go through the engine functions that already take the lock.
- **Risk:** the send-knob extraction touches the mixer; check the mixer looks the same (Group B screenshot).

## Libraries and models
None.

## Theory

**Facts**
1. A send is taken after the track's effects, after the fader (post) or before it (pre), and always before the pan; the default is post. DawIt's own behaviour (`Track::process`), as the guide's *Pre-fader and post-fader* says. Exact.
2. The send dial runs from Off (-60 dB) to +6 dB with the middle at -12 dB; double-click turns it off. DawIt's control (`SendKnob`). Exact.
3. A send turned on from a menu or a new return starts at -10 dB, about the reverb of a 20% insert mix: 20 log10 (tan (0.2 × 90°)) = -9.76 dB. Derived; already in *Returns in DawIt*. Exact.
4. An insert takes the whole sound, a send adds a copy to a return; reverb and delay usually live on returns. Existing guide (*Insert or send?*), Sound On Sound (in SOURCES.md). Opinion for "usually" (the guide says engineers work both ways).
5. A return plays into the master only and has no sends. DawIt's rule (architecture.md). Exact.
6. Muting a track also silences its sends. DawIt's rule, existing guide (*Solo and mute with buses*). Exact.
7. The volume fader runs from -inf (-60) to +6 dB, double-click 0 dB; meters read -60 to +6 dB. DawIt's controls (`ChannelStrip`, ui-design.md). Exact.
8. In DawIt the master has a volume but no pan, effects slot or sends. DawIt's edit (no master chain in `Engine`). Exact.
9. Logic Pro and Cubase both call this panel the inspector; Logic Pro's default key for it is I. Apple's Logic Pro 9.1.6 manual (Hide/Show Inspector, default I) and Steinberg's Cubase help (Inspector in the Project window's left zone). Exact (product facts).
10. Logic Pro's inspector shows the selected track's channel strip and, beside it, the strip it plays into. Apple Logic Pro 10.1 help and the current guide's summary (search results; the current page's full wording not read). Exact, but phrased loosely ("also shows") in the text.

**Topic text**

*The inspector* — summary: "One track's effects, sends, pan and volume, beside the tracks"

The inspector is the panel on the left of the track headers (View > Inspector, or press I). It shows one track at a time: the one you clicked last. Click another header, or a strip in the mixer, and it follows. With nothing selected, it shows the master.

From top to bottom it follows the sound:
- Effects: the instrument first on a MIDI track, then the effects in the order the sound goes through them. Click one to open it, click its dot to switch it off (bypass), and drag it up or down to change the order. + Effect adds one at the end.
- Sends: a row for every return in the song, with the return's full name. Drag a knob up to send a copy of the track to that return, and double-click it to turn the send off. PRE means the copy is taken before the fader.
- Pan: where the track sits between the left and the right speaker.
- Volume: the track's fader, with its meter beside it.

It shows the track itself, not a copy. Turn the Reverb send up in the inspector and the Reverb knob in the mixer turns with it. Use whichever is closer: the inspector while you work on the arrangement, the mixer (Cmd+2) to compare all the tracks side by side.

Why are the sends a list of their own, and not with the effects? Everything in the Effects list is an insert: the whole sound goes through it. A send leaves the track's sound alone and adds a copy to a return, where a shared reverb or delay usually sits (see Insert or send?). So a vocal with an EQ and a compressor in its Effects list and a send of -12 dB to Reverb stays fully compressed, with the reverb added beside it.

Things to know:
- It changes one track. With several tracks selected, it shows the one you clicked last, and only that one changes. To change several at once, use the mixer or the track menu's Sends.
- A return has no sends: it plays into the master only. Its inspector lists the tracks that send to it instead.
- The master has a volume and a meter here, but no pan, effects or sends.
- Muting a track with M silences its sends too.
- Other programs have the same panel. Logic Pro and Cubase call it the inspector, and in Logic Pro the I key opens it too. Logic Pro also shows, beside the track, the strip it plays into.

*Returns in DawIt* — add after its step 2: "The inspector (View > Inspector, I) shows the same knobs for the selected track, one for every return, with their full names."

*Pre-fader and post-fader* — change "Right-click a send knob in the mixer, or a send in the device bar," to "Right-click a send knob in the mixer or the inspector, or a send in the device bar,".

**"In your song" line** (`describeSends`, levels with one decimal):
- Track or bus with sends on: "Lead vocal sends to Reverb at -12.0 dB, after the fader, and to Delay at -18.0 dB, before the fader." (one return: no list; three or more: commas and "and").
- Track or bus, returns exist, none on: "Lead vocal doesn't send to a return yet. Drag up a knob under Sends in the inspector to add some reverb."
- No returns: "Your song has no return yet. Track > New Return Track > Reverb adds one, and every track gets a knob for it."
- Return with senders: "Reverb is fed by Lead vocal at -12.0 dB and Guitar at -8.9 dB." No senders: "Nothing sends to Reverb yet."
- Master: "The master gets every track, bus and return. It has no sends."
- Prefix "In your song: " when appended to the topic. Quiet (no line) when the song has no tracks.

**Tooltips**
- Panel title: "Inspector: the selected track's effects, sends, pan and volume. Click a track to show it here."
- ?: "What the inspector shows, and why sends are separate from effects (opens Buses, sends and routing)"
- ×: "Hide the inspector (View > Inspector, I)"
- Drag edge: "Drag to make the inspector wider or narrower"
- Name area: "Lead vocal, an audio track that plays into Vocals" (type and output follow the track).
- Effect row: "DawIt EQ: click to open, click the dot to switch it off, drag up or down to change the order, right-click for more."
- Instrument row: "The instrument that plays this track's notes: click to open it, right-click to choose another"
- + Effect: "Add an effect at the end of the chain"
- Send row: the knob's existing tooltip ("Send to Reverb: ...").
- + Send to a new return: "Make a return track (Reverb, Delay or empty) and send this track to it at -10 dB, about as much reverb as a 20% mix on the track."
- Fed by row: "Lead vocal sends to this return at -12.0 dB. Click to select it."
- Pan, volume, M, S, Output: the strip's tooltips.
- Master view: the fader uses the strip's master tooltip ("Master volume: the whole mix, ..."); the name area says "The master: the mix bus, where every track, bus and return ends. Right-click for the guide."

**Example data** (the picture with no song): the sends example as it is (Vocal -12.4 dB, Guitar -8.9 dB, Pad -1.4 dB to Reverb; Bass none), with Vocal, Reverb and the master on the selected path and Guitar, Pad and Bass drawn dimmed (`onSelectedPath` false, as the bounce example does).

**Test numbers**

| Check | Expected |
|---|---|
| `allTopics` count | 18 (was 17); *The inspector* directly after *Returns in DawIt* |
| `getSendTargets` with 6 returns, for a track / a bus / a return / the master | 6 / 6 / 0 / 0, in edit order |
| `describeSends`, track with -12.0 post to Reverb and -18.0 pre to Delay | contains "Reverb at -12.0 dB, after the fader" and "Delay at -18.0 dB, before the fader" |
| `describeSends`, a return fed by two tracks at -12.0 and -8.9 | contains both names and "-8.9 dB" |
| `describeSends`, no returns / returns but none on / master | the three sentences above |
| a send at Off (-60) | counted as off: not in the "sends to" list |
| new topic | has a name, summary, text over 400 characters and a non-empty example with one node on the selected path |
| search "inspector" | finds Buses, sends and routing |

## Steps

Group A, engine and guide text (`./build.sh --test` at step 2):
- [x] 1. Routing guide: add *The inspector* after `sendHowTo` (name, summary, text, example), the sentences in *Returns in DawIt* and *Pre-fader and post-fader*; the picture case in `RoutingExplainer` (song tree when a track is selected, else the example); `gen::guides` opensFrom and words. Tests: topic count, text, example, search. Check: `--export-guides` passes.
- [x] 2. `routing::getSendTargets` and `routingguide::describeSends`; `RoutingExplainer` appends the line to *The inspector* and puts it first in *Your song's routing* for the selected track. Tests: the table's rows. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 6):
- [x] 3. Move the send knob out of `MixerView.cpp` into a shared file (compact and wide looks, the name, PRE and level drawn by it); the mixer uses compact and looks the same. Check: a mixer shot on the pop song with a reverb return matches the one before the change.
- [x] 4. `InspectorPanel`: title row, name, M/S/Output, effects list (open, bypass, remove, drag to reorder, menus, + Effect), sends from `getSendTargets` in wide mode, + Send to a new return, *Fed by* for returns, master view, pan, volume with meter; follows selection and edit changes as in *Code*.
- [x] 5. `MainComponent`: the panel on the left at full height, the drag edge, *View > Inspector* with I and its tick, `view.inspectorOpen` / `view.inspectorWidth`, the ? and menu opening the routing guide at the topic (through `TrackActions::onShowGuide`).
- [x] 6. `ScreenshotRunner`: a shot key `"inspector": true/false` and a capture `"inspector"`. Done when: `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 7. Real-app check with `--screenshots` on the pop song: (a) `sendsChoice` "Lead vocal:new reverb", then a second return "Lead vocal:new delay", `select` Lead vocal, `inspector` true, capture main: panel on the left, two send rows with full names and levels, mixer below narrower; (b) the same with capture `bottomPanel`, panel mixer: the mixer's knobs as before and at the same levels; (c) six returns made with `sendsChoice` ... empty, capture `inspector`: all six rows, scrolling; (d) a return selected: *Fed by*; (e) nothing selected (`"select": []`; if the runner can't clear the selection, add that to `ScreenshotRunner` in step 6): the master; (f) `guide` routing, topic "The inspector", with Lead vocal selected. Look for clipped names, overlapping rows, wrong dB.
- [x] 8. Docs: README Features bullet (near the mixer's); docs/tests.md (routing guide topic and the two functions); docs/code-layout.md (InspectorPanel, the shared send knob file); ui-design.md: a section *The inspector* (sizes and layout above) and *Sends in the mixer...* naming the shared class; architecture.md *Who owns what*: InspectorPanel under MainComponent; FEATURE_IDEAS status Built (and the open item "more than four send knobs" under Aux sends still open for the mixer).

## Tests
- Routing guide: 18 topics, *The inspector* after *Returns in DawIt*, its name, summary, text (over 400 characters, mentions "View > Inspector" and "Insert or send?") and example (non-empty, Vocal on the selected path, Bass not).
- `routing::getSendTargets`: 6 returns give 6 targets for a track and a bus, in edit order; 0 for a return and for null (master).
- `routingguide::describeSends`: the five cases in *Theory* with the exact numbers; a send at -60 dB counts as off; three sends join with commas and "and".
- `gen::guides` search "inspector" finds the routing guide; `runTheoryGuidesTests`' topic count follows `allTopics`.

## Website hand-off
- Features page: a short section in the mixing area, "The inspector": one track's effects, sends with full names, pan and volume beside the tracks, in sync with the mixer.
- Screenshot: shot (a) above, cropped to the left half (inspector, headers, a few lanes).
- Main page: not needed. How it's made: nothing new.

## Left for later
- Slimmer track headers (volume and pan only in the inspector): the user kept the headers as they are.
- More than four send knobs in the mixer strip.
- Renaming the track or changing its colour from the inspector; a second strip for where the track plays into (Logic Pro's right-hand strip).
- Changing several selected tracks at once from the inspector.
- The device bar's gain-staging badges in the effects list.

## Changes while building

**Group A hand-off (built, `./build.sh --test` passes, `--export-guides` exits 0):**
- `routing::getSendTargets (const Track*, const Tracks&)` in `src/engine/Routing.h`: the returns a track or bus can send to (edit order); empty for a return or null (master). The inspector's send rows come from it.
- `routingguide::describeSends (const Track*, const Tracks&)` and `Topic::inspector` (after `sendHowTo`) in `src/engine/RoutingGuide.h`. `RoutingExplainer::updateText` appends the line to the topic ("In your song: ...") and puts it first in *Your song's routing*; the topic uses the song tree when the song has tracks. Group B opens the guide with `Topic::inspector` (name "The inspector").
- The example picture lights Vocal and Reverb (two nodes; the master isn't a node) and dims the others via `silent`.
- `gen::guides` opensFrom and words are done. Other tests that counted 17 routing topics now say 18.
- Nothing left half-done.

**Group B hand-off (built, `./build.sh --test` passes without warnings):**
- `SendKnob` (`src/ui/SendKnob.h/.cpp`, `daw::SendKnob`, `Look::compact|wide`) replaces `ChannelStrip::SendKnob`; it now owns its row (name, PRE, level), so the strip's `paintOverChildren` and `sendKnobSize` are gone, and the knob's bounds are the whole 72 x 22 row (in the compact look only the dial takes the mouse, as before). A mixer shot after the change looks the same.
- `InspectorPanel` and `InspectorEdge` in `src/ui/InspectorPanel.h/.cpp`; in `MainComponent` as `inspector` / `inspectorEdge`, `setInspectorOpen (bool)`, command `toggleInspectorCommand` (plain I, View menu, ticked), settings `view.inspectorOpen` / `view.inspectorWidth` (saved when the drag ends). Layout: the panel and a 4 px edge come off the left of the area under the transport bar and above the typing strip, before the arrangement/device bar/bottom panel layout.
- `ScreenshotRunner`: shot key `"inspector": true|false`, capture `"inspector"`, and `"select": []` now clears the selection (the master view). Documented in `DevScreenshots.h`.
- One sanity shot (sends song, Lead vocal selected, mixer open) showed the panel, two send rows with levels and the mixer unchanged. The full real-app check (step 7) and docs (step 8) are still to do.
- Differences from the plan: the ? and menu open `Topic::inspector` for the master too; the effect row's drag leaves the row in place and shows the accent line (the row doesn't follow the mouse). The panel's Content sits on `content.addMouseListener (this, false)` so a right-click on the empty list opens *Hide Inspector* / *About the inspector...*.
- Docs for Group C to name: `InspectorPanel`, `InspectorEdge`, `SendKnob` files in code-layout.md; `CMakeLists.txt` has the two new .cpp files in `DawIt` only (UI).

**Group C (real-app check and docs, done):**
- Shots on the pop and flow songs (scale 1): six returns listed in the inspector (all six rows, 230 px, no clipping), the mixer on the pop song unchanged (four knobs, same levels), effects rows with power dots on Lead vocal (flow song), a return's *Fed by*, the master view, several selected ("2 selected; showing Keys"), and the guide topic with Lead vocal's path lit. No fixes were needed. Note: in a shot, `sendsChoice` selects the new return, so a `select` in the same shot is applied first; give the select its own shot.
- Docs written: README bullet, ui-design.md *The inspector*, code-layout.md, architecture.md, tests.md, idea status Done (the idea's section can move to `features_done.html`). SOURCES.md already had the inspector's searches.

## Website hand-off (Group C)
- Features page, mixing area: a section "The inspector": one track's effects, sends with full names, pan and volume beside the tracks, in sync with the mixer.
- Screenshot: pop song, `"sendsChoice": "Lead vocal:new reverb"` then `"sendsChoice": "Lead vocal:new delay"`, then a shot `"select": "Lead vocal", "capture": "main", "area": [0, 0, 720, 900]` (the inspector, headers and a few lanes). A second, `"song": "flow"`, `"select": "Lead vocal"`, `"capture": "inspector"`, shows effects and sends together.
- No new library or model.
