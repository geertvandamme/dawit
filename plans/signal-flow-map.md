# A live map of the sound flow

Status: Built, not committed. Theory checked
Idea: [A live map of the sound flow](../FEATURE_IDEAS.html#a-live-map-of-the-sound-flow)

## Goal
A *Signal Flow* window (View > Signal Flow..., Cmd+6) draws the whole song as one flow diagram: every track's source, its effects and fader, the buses and returns, the sends and the master, with the real levels moving along the lines while the song plays. A beginner sees where each sound starts, what changes it on the way and where it ends, and why order, buses and sends matter. A new *Signal flow* topic in the *Buses, sends and routing* guide explains it, and the guide's *Your song* advice learns to spot effects in an unusual order.

## Decisions for you
1. **Scope.** In: a **read-only** window with live levels, beat pulses, over-0 dB warnings, the mute, solo and bypass states shown, the selected track's path, the small EQ, Compressor and Gate pictures in their boxes, the *Signal flow* topic and five effect-order advice rules. Out, for later: anything in the window that edits the song (bypass, mute, solo, outputs, faders, sends, dragging lines), a compact path in the device bar, and pictures or advice for other effects and third-party plugins.
   Answer: the window is read-only; it only views (zoom, scroll, fit, tooltips, live display, legend, moving-dots toggle).
2. **Where the graph code lives.** Options: `src/generate/SignalFlow` (as the idea says), or `src/engine/SignalFlow`, next to `routing::` and `routingguide::`. **Recommended:** `src/engine`. It is routing, not music theory, and engine code can't include generate code, so the routing guide could use it later only from there. It is still a pure function tested in `EngineTests`.
   Answer: `src/engine/SignalFlow`, as recommended.
3. **Shortcut.** Cmd+3 (the idea's example) already folds the song rows, and Cmd+1, 2, 4 and 5 are taken. **Recommended:** Cmd+6, the next one in the View menu's row.
   Answer: Cmd+6, as recommended.
4. **Folded buses.** Options: the map shows every track always, or it folds the buses the arrangement has folded. **Recommended:** every track always. The user asked for all tracks, and zoom, Fit and the overview mode handle big songs.
   Answer: every track always, as recommended.
5. **What a click does in the read-only window.** Options:
   - (a) a click on any box selects its track (the app's selection: the path lights up, and the device bar and mixer show that track), and nothing else;
   - (b) as (a), and a click on an effect or instrument box also opens its window;
   - (c) neither: clicks do nothing, hover only, and the lit path follows the selection made elsewhere.

   **Recommended: (a).** Selecting is a view action, it changes nothing in the song, and it is how the lit path is reached from inside the map. An effect window is an editor full of knobs, so opening it from a read-only view blurs the line the user drew.
   Answer: (a), a click selects the track and does nothing else.
6. **The advice buttons in the guide.** The four reorder buttons for rules 1 to 4 sit in the routing guide's *Your song's routing*, not in the Signal Flow window, as do the guide's existing buttons (*Create bus*, *Share one reverb*). Options: keep them, or give the advice as text only. **Recommended:** keep them. The read-only rule is about the map window, and the guide already acts on the song this way.
   Answer: keep the four reorder buttons in the guide.
7. **Moving dots.** They can be always on, or on by default with a toggle that is remembered. **Recommended:** a toggle, on by default. Some people find motion distracting, and the lines still show the level without it.
   Answer: a toggle, on by default, remembered.

## What the user does and sees
- **Opening:** *View > Signal Flow...* (Cmd+6, after *Mixer*); *Show in Signal Flow...* in a track's right-click menu (opens it and scrolls to that track); the *Show my song's signal flow* button on the guide's *Signal flow* topic. One window (`PluginWindowManager::show` with `minimumSize` 640 × 420, resizable, default 1200 × 760). It comes to the front when opened again.
- **Layout:**
  ```
  [Signal flow]  [-] [100%] [+] [Fit]  [Moving dots]                 [How sound flows?]   36 px toolbar
  ┌ canvas (viewport, scrolls both ways) ─────────────────────────────────────────────┐
  │ ▌Kick  ─────────────▶[Fader]─┐                                                    │
  │ ▌Snare ──▶[Gate]───▶[Fader]──┴▶(Σ Drums 2)─▶[Compressor]─▶[Fader]──┐              │
  │ ▌Bass  ─▶[EQ]─▶[Saturation]─▶[Fader]───────────────────────────────┼─▶[ Master ▮▮]│
  │ ▌Vocal ─▶[EQ]─▶[Compressor]─▶[Fader]┬──────────────────────────────┤              │
  │                                     ┆ -12 dB                       │              │
  │                          (Σ Reverb 1)─▶[Reverb]─▶[Fader]───────────┘              │
  └───────────────────────────────────────────────────────────────────────────────────┘
  Lines: thicker and brighter is louder · grey quiet, green healthy, ...              22 px legend
  ```
- **Graph rules (drawn left to right):** each track is one row: its source box, its effects in order, then its fader. Each bus and return starts with a Σ box (its sum) followed by its effects and fader. The master is one box on the far right. A bus sits at the middle of its tracks' rows, to the right of their faders; returns are rows at the bottom. Lines are orthogonal, and lines into one box share their last vertical piece, so tracks visibly merge into a bus. Sends are dashed lines in the return's colour, branching just before the fader box (pre-fader, with PRE in `meterYellow`) or just after it (post-fader), labelled with the send level.
- **Live:** a line's thickness (1.5 to 6 px at 100%, by `meterProportion`) and colour (`gainstaging::getZoneColour`: grey quiet, green healthy, yellow hot, red over) follow the level at that point. A red over-0 dB mark holds for 2 s. The master shows a stereo meter and a red CLIP badge, held 3 s, when its output goes over 0 dB. While the song plays, dots run along the lines that carry sound, one dot spacing (24 px at 100%) per beat of `positionBeats`, with size and alpha by level. They stand still when the song is stopped.
- **States:** a bypassed effect box is drawn at 40% with a grey name, and the line arcs over it. A muted track's line is dark after its fader (`theme::gridBar`, no dots), and so are its sends, while its chain before the fader still shows the levels. Everything that `routing::getSilence` says isn't heard goes dark the same way. With a track selected, its path is drawn normally and the rest at 30% alpha. A track's path is its chain, the buses it plays into, the returns it sends to and the master. A bus's or a return's path also includes the tracks feeding it.
- **Zoom and scroll:** zoom runs from 25% to 200%, in steps of x1.25 for the buttons and Cmd+= / Cmd+-. Cmd+scroll or a pinch zooms around the mouse. Cmd+0 or *Fit* fits the whole graph with a 24 px margin, between 25% and 100%, and it stays fitted when the window or the song changes until the user zooms. Plain scrolling scrolls (Shift for sideways, or a trackpad in both directions), and dragging the empty canvas pans it. Clicking the zoom label sets 100%. Below 60% is the overview mode: effect boxes become 10 px squares without names, the track names stay (shrunk to fit), and there are no mini pictures. When the selection changes and its row is out of view, the map scrolls to it.
- **Read-only:** nothing in this window changes the song. There are no bypass dots, no menus of the track, effect or send, no double-click actions, no dragging of lines or boxes, and no fader or send controls. Changes are made where they are made now (device bar, mixer, track menus), and the map follows them live.
- **Clicks** (decision 5, recommended (a)): a click on any box or line selects its track through `Selection`, which lights up its path, and nothing else. A click on empty canvas leaves the selection as it is (dragging there pans).
- **Right-click** anywhere opens the view's own menu: *Fit to Window*, *Zoom to 100%*, *Show the Selected Track*, *Moving Dots* (ticked) and, last, *About signal flow...* (the guide at the *Signal flow* topic). Delete, copy, paste and double-click do nothing here. Space plays, as in every window.
- **Tooltips:** on every box, line, button and the empty canvas, saying what the thing is, its levels and its state. They don't promise actions the window doesn't have. The texts are in *Theory*.
- **Remembered** (user settings): `view.signalFlowZoom` (0 means Fit), `view.signalFlowScroll` (the top-left point in graph units, so it survives a zoom change), `view.signalFlowPulses` (default true) and `view.signalFlowSize`.

## Data and saving
No project data. Only the four settings keys above.

## The guide
- **New topic** in *Buses, sends and routing* (`routingguide`), between *Solo and mute with buses* and *Your song's routing* (15 topics):
  - **Signal flow**: "Where the sound goes, step by step, and why the order matters". Its picture is the example map (`signalflow::getExample`), drawn by the same painter as the window, fitted, with live dots when playing, and printed the same way in the PDF. The topic's action button, *Show my song's signal flow*, opens the window.
- **Your song's routing:** five new advice rules about effect order, each linking to *Signal flow*. Four of them have a button that reorders the chain. The topic's text gets a sentence for them (see *Theory*).
- **The `gen::guides` entry** (routing): the new summary, the new *opensFrom* and the extra search words in *Theory*. The topics come from `allTopics` by themselves. It stays off *Start here*, as now.
- **Opens from:** the window's *How sound flows?* button and the last item of its canvas menu, both at *Signal flow*.

## Code
- Recipes: *A window*, *A command or menu item*, *Topics in an existing guide*, and *Checking in the real app* (a scene and a song).
- **`signalflow::` (`src/engine/SignalFlow.{h,cpp}`), pure:** a `Graph` of nodes and edges. A node is a source (audio clips, DawIt Synth or an instrument slot), an effect, a fader, a sum (bus or return) or the master. Each node carries its name, colour, track, slot, bypassed, silenced, lit and level in dB. An edge is a flow edge or a send edge (level, pre-fader). Functions: build from `Tracks` and the selected track, reading the atomics; the example graph; a layout in grid units (column and row).
- **Invariants:** every node but the master has exactly one outgoing flow edge, and the master has none. Only tracks and buses send; returns don't. Every edge goes from a lower column to a higher one, so there are never loops. Columns come from the longest path from column 0, with the master one column past the highest. Rows: tracks in edit order as integer rows, a bus at the mean of its members' rows (its own row when it is empty), returns in rows after all others, and the master at the middle of the rows. A send edge leaves from the sender's fader node and enters the return's Σ.
- **Threading:** the graph is rebuilt on the message thread on every frame (about 30 Hz), from the live edit and its atomics. It is cheap, and it never holds pointers across a structural change. The layout is recomputed only when the structure signature (the nodes and edges) changes, so the picture doesn't jump. Click handling uses the latest graph. **Audio-thread change (risky but small):** the stage peaks (`PluginSlot::inputPeak/outputPeak`, `faderInputPeak`, `Send::peak`, `masterInputPeak`) are swapped to 0 by the device bar when it reads them, so a second reader would steal them. Add falling levels (`updateMeterLevel`) that any number of views can read: the level going into the chain (after the clips or DawIt Synth), each slot's output, and each send's output. The peaks are already computed there, and no allocation is added. Existing falling levels cover the rest: `peakLeft/Right`, `busInputLevel` and `masterPeakLeft/Right`.
- **Repaints:** only the viewport's visible area is painted. The 30 Hz timer runs while the window is showing and something moves (playing, a level above -60 dB, or a hold still on); otherwise it runs at 4 Hz.
- **One painter** draws a graph for the window, the guide's picture and the PDF.
- **Read-only invariant:** the window's code calls no engine function that changes the edit. It reads `Engine::getTracks()` and the atomics, and writes only `Selection` and its own settings keys.
- **Advice:** the new rules go in `routingguide::analyse` (only DawIt's own effects, identified by `identifier`), with a new action, *move effect*, that `RoutingExplainer` carries out with `Engine::movePlugin`.
- **Mini pictures:** move the drawing of the setting curves out of `DeviceBar::Chip::paintPicture` into a shared UI helper, and use it in the device bar and the map. The device bar's spectrum layer stays its own.

## Libraries and models
None.

## Theory

### Facts
1. A track's effects run in series: each hears the output of the one before. **Exact** (DawIt `Track::process`).
2. An effect that reacts to the level (compressor, gate, saturation, distortion) behaves differently depending on what comes before it. **Exact** (definition).
3. Linear filters with fixed settings (EQs, a fader) give the same result in either order. **Exact** (linear time-invariant systems commute). Tested with two DawIt EQs.
4. A hard-knee compressor above its threshold: 6 dB of gain before it comes out 6 x (1 - 1/ratio) dB louder than the same 6 dB after it, which is 4.5 dB less at 4:1. **Exact**, derived: with threshold -30 dB, ratio 4 and knee 6 dB, a -10 dB input loses 15 dB and a -4 dB input loses 19.5 dB, so -23.5 vs -10 - 15 + 6 = -19. Tested with `comp::getReductionDb`.
5. EQ before a compressor changes what it reacts to, while EQ after it changes only the tone. **Typical**: Sound On Sound, H. Robjohns, *Q. In what order should I compress and EQ?* (2004); Waves, *What plugin goes first in a mix chain*.
6. Whether EQ goes first or second is a matter of taste. SOS calls EQ after compression the more usual way, iZotope calls EQ first "slightly more conventional", and many engineers use both. **Opinion**: iZotope, *Signal chain: order of operations*; mastering.com (R. Mayzes).
7. A gate usually goes first, before compression, which narrows the gap between notes and noise; gated reverb is the exception (gate after reverb). **Typical**, as the Gate guide already says (`GateGuide.cpp`) and in agreement with it.
8. Distortion and saturation usually go before delay and reverb, as on a pedalboard; after them, the echoes and tails get distorted. **Typical/opinion**, as the Distortion guide already says; iZotope's order is dynamics, gain, modulation, time.
9. Reverb usually goes last on a track, after EQ, compression, distortion and delay. **Typical**, as DawIt Reverb's explainer already says (`ReverbEditor.cpp`).
10. A compressor after a reverb also compresses the tail, which brings it up between notes and can pump. **Derived/opinion**: a compressor narrows the range between the dry hits and the quieter tail.
11. Gated reverb is the 1980s drum sound. **Typical**, as `BuiltInGate.cpp` and the Gate guide already say.
12. A send is a copy. The track's own line carries on unchanged, and DawIt takes the send after the effects, before or after the fader, and always before the pan. **Exact** (DawIt; the routing guide's *Pre-fader* topic).
13. Different sounds add as powers: -12 dB and -14 dB give 10 log10 (10^-1.2 + 10^-1.4) = -9.88 dB. Two copies in step give +6.02 dB. **Exact**, derived (`routingguide::sumDb`, the *Levels add up* topic).
14. DawIt's floating-point maths doesn't distort over 0 dB inside the song. Only the master's output and 16- or 24-bit files clip. **Exact** (the Gain staging guide's *Over 0 dB* topic).
15. The level zones: quiet below -24 dB, healthy from -24 to -6, hot from -6 to 0, over above 0. **Typical** (DawIt's `gainstaging` zones).
16. Audio is worked out in blocks: 512 samples at 48 kHz is 512 / 48000 = 10.7 ms. **Exact**, derived. The dots show direction and the beat, not speed.
17. A bypassed effect passes the sound through unchanged. **Exact** (`Track::process`).
18. Muting a track silences its sends too; while something is soloed, tracks off its path are silent. **Exact** (DawIt; the routing guide's *Solo* topic).

No disagreement with the existing guides. Facts 7 to 9 and 11 repeat them.

### Topic text
**Signal flow** · *Where the sound goes, step by step, and why the order matters*

> Signal flow is the route a sound takes from where it starts to your speakers, and the order of the steps on the way. The Signal Flow window (View > Signal Flow..., Cmd+6) draws it for your whole song; the picture above is a small example.
>
> Read it from left to right. Each track starts with its source: its audio clips, or the instrument that plays its MIDI notes. Then come its effects, one after the other, then its fader. From there the line goes into a bus, where it meets the other tracks of the group, or straight into the master on the right. A send branches off as a dashed line to a return.
>
> Each effect hears what the one before it made. An EQ that cuts 6 dB at 200 Hz hands the compressor after it a sound with less low end, and the compressor reacts to that. Swap them, and the compressor reacts to the full low end while the EQ only shapes what comes out. Two EQs, or an EQ and a fader, give the same sound in either order, because neither reacts to the level. Order matters most for the effects that do: compressors, gates, saturation and distortion.
>
> A worked example: a compressor at 4:1 with its threshold at -30 dB. Turn a -10 dB sound up 6 dB before it, and it comes out at -23.5 dB. Turn it up 6 dB after the compressor instead, and it comes out at -19 dB. The same 6 dB makes a 4.5 dB difference, just from where it sits.
>
> Orders most engineers start from:
> - A gate first, before compression. A compressor narrows the gap between the notes and the quiet noise between them, which makes the gate harder to set. Gated reverb is the exception: the gate comes after the reverb to cut its tail short.
> - EQ and compression in either order. EQ before the compressor changes what it reacts to: cut the boomy lows and it stops pumping on them. EQ after it changes the tone without changing how it works. Many engineers do both, a cut before and a gentle boost after. Neither is wrong.
> - Saturation and distortion before delay and reverb, as on a guitarist's pedalboard. After them, the echoes and the reverb's tail are distorted too and smear together, unless that's the sound you want.
> - Reverb last, after EQ, compression, distortion and delay. A compressor after a reverb squeezes the reverb too: between the notes the tail comes up, and the sound can pump.
> These are starting points, not rules. Try the other order and listen.
>
> A send is a branch, not a detour. The track's own line carries on unchanged to its bus or the master; the send adds a copy to a return. In DawIt it's taken after the effects, after the fader (post-fader) or before it (pre-fader), and always before the pan.
>
> A bus adds up its lines, so the line coming out of a bus is usually thicker than the lines going in. Two different sounds at -12 dB and -14 dB add up to about -9.9 dB; two copies of the same sound, in step, come out 6 dB louder than one. The master is the last sum of all.
>
> In the Signal Flow window:
> - A line's thickness and colour follow the level passing through it, like the meters: grey when quiet (below -24 dB), green when healthy, yellow when hot (above -6 dB), red over 0 dB.
> - Red on a line inside the song is a warning, not damage. DawIt calculates with floating-point numbers, so a track or a bus over 0 dB doesn't distort by itself, but effects after it react to the hot level. Only the master's output clips, at your speakers and in a 16- or 24-bit file; its box then shows CLIP.
> - Dots run along the lines while the song plays, one step per beat. They show which way the sound goes, not how fast: the computer works out the whole path for one small block of audio at a time, for example 512 samples, which is 10.7 ms at 48 kHz.
> - A switched-off (bypassed) effect is grey, and the line goes round it: the sound passes unchanged.
> - A muted track's line goes dark after its fader. Its instrument and effects still work, but the sound stops there, and its sends stop too. While a track is soloed, everything that isn't on its path goes dark.
> - Click a box to select its track and light up its path.
> - The window only shows the song; it doesn't change it. Change things where you always do (the device bar, the mixer, a track's right-click menu), and the picture follows straight away: switch an effect off in the device bar and its box goes grey.

**Your song's routing** (replaces the text that doesn't depend on the song):

> This page looks at your song's tracks, buses and returns, and says what could help. For buses: tracks of one kind that could share one, a drum bus with no effects yet, a bus with only one track or none, and buses nested deeper than two levels. For returns: the same reverb or delay on several tracks that could share one return, a return with no sends or no effect, an effect on a return that isn't at 100% wet, and a lot of reverb on the bass. For the order of effects on a track: a compressor, distortion or saturation after a reverb or delay, a gate after a compressor or a reverb, and a track with its own reverb that also sends to one. And while the song plays: a bus or return whose input adds up to over 0 dB, or a master with less than about 6 dB of headroom.
>
> It follows your changes, so try a suggestion with the button above the text and see the picture change.

### Texts in the window, menus and guide entry
- Command *Signal Flow...*: "A live map of how the sound travels through the whole song: every track, effect, bus, send and return on its way to the master". Track menu: *Show in Signal Flow...*. Guide button: *Show my song's signal flow*.
- Toolbar: **-** "Zoom out (Cmd+minus, Cmd+scroll or pinch)" · **zoom label** "The zoom. Click for 100%" · **+** "Zoom in (Cmd+plus, Cmd+scroll or pinch)" · **Fit** "Fit the whole song in the window (Cmd+0). It stays fitted as the window or the song changes, until you zoom" · **Moving dots** "Dots that run along the lines to the beat while the song plays. Switch them off for a still picture: the lines still show the levels" · **How sound flows?** "The Signal flow topic of the Buses, sends and routing guide: what this picture shows, and why the order of effects matters".
- Empty canvas: "The whole song's signal flow, live. It only shows: change things in the device bar, the mixer or a track's menu. Drag to move around; scroll to scroll; Cmd+scroll or pinch to zoom; click a box to select its track. Right-click for the view's menu".
- Legend: "Lines: thicker and brighter is louder · grey quiet, green healthy, yellow hot, red over 0 dB · dashed: a send · grey box: switched off · dark line: muted, or not heard while another track is soloed · this window only shows the song: change it in the device bar or the mixer".
Every box tooltip ends with " Click to select the track." (with decision 5 (a); drop it for (c), and for (b) effect and instrument boxes end with " Click to select the track and open it.").
- Source box: "Kick: its audio clips, where its sound starts. Level -12.3 dB." For DawIt Synth: "Bass: DawIt Synth (Bass) turns the track's MIDI notes into sound. Level -10.0 dB." For an instrument plugin: "<name> on Bass: the instrument that turns the track's MIDI notes into sound. Level -10.0 dB."
- Effect box: "DawIt Compressor on Vocal: turns the loud moments down, so the level is more even. In -9.0 dB, out -12.0 dB." Bypassed: "DawIt Saturation on Bass is switched off: the sound goes past it unchanged. Switch it on in the device bar." What each effect does: EQ "turns ranges of frequencies up or down"; Compressor "turns the loud moments down, so the level is more even"; Gate "silences the sound between the notes, when it falls below the threshold"; Reverb "adds the sound of a room"; Delay "adds echoes"; Saturation "adds warmth and harmonics by rounding off the peaks"; Distortion "adds grit by clipping the peaks hard"; any other plugin "a VST3 effect" or "an AU effect".
- Fader box: "Vocal's fader: -3.0 dB, pan centre (or 30% left). Out -12.0 dB." Plus one of: "Muted: the sound stops here, and so do its sends." / "Its bus Drums is muted, so nothing from here is heard." / "Not heard: another track is soloed."
- Σ box: bus "Drums: 2 tracks add up here (Kick, Snare). Level -9.9 dB. A bus is usually louder than any one of its tracks"; return "Reverb: the sends of Vocal and Keys add up here. Level -24.0 dB".
- Master: "The master: everything adds up here, then the master volume (0.0 dB), out to your speakers. Peak -7.5 dB". When clipping: "It went over 0 dB in the last 3 seconds: that clips at your speakers and in a 16- or 24-bit bounce".
- Line: "Kick → Drums: -12.0 dB peak, healthy". Over: "... over 0 dB. Nothing distorts inside DawIt, but effects after this point react to the hot level; only the master's output clips". Send: "Vocal sends to Reverb at -12.0 dB, post-fader: -24.0 dB peak now. Change it with the send knob in the mixer".
- `gen::guides` routing entry: summary "Where each track's sound goes: buses that group tracks, sends to a shared reverb or delay, levels that add up, glue compression, solo and mute, the order of effects, and your song's routing."; *opensFrom* ends with ", and How sound flows? in the Signal Flow window"; add the search words "signal-flow signal-chain chain effect-order plugin-order order-of-effects flow map".

### Example data (the topic's picture, `signalflow::getExample`)
Levels are as meters would show them, and sums are powers (`sumDb`, different sounds).

| Row | Chain (level out) |
|---|---|
| Kick (audio) | clips -12 → fader 0 dB: -12 → Drums |
| Snare (audio) | clips -14 → DawIt Gate -14 → fader 0 dB: -14 → Drums |
| Drums (bus) | Σ -9.88 → DawIt Compressor -12 → fader 0 dB: -12 → Master |
| Bass (MIDI) | DawIt Synth -10 → DawIt EQ -11 → DawIt Saturation (bypassed) -11 → fader -2 dB: -13 → Master |
| Vocal (audio) | clips -8 → DawIt EQ -9 → DawIt Compressor -12 → fader 0 dB: -12 → Master; send post-fader -12 dB: -24 → Reverb |
| Reverb (return) | Σ -24 → DawIt Reverb (100% wet) -26 → fader 0 dB: -26 → Master |
| Master | 10 log10 (10^-1.2 + 10^-1.3 + 10^-1.2 + 10^-2.6) = -7.48 dB |

This gives 20 nodes, 19 flow edges and 1 send edge. Vocal is lit (selected), so Vocal, Reverb and the master are drawn normally.

### "Your song" rules (in `routingguide::analyse`, topic *Signal flow*)
These rules count only DawIt's own effects that are on (not bypassed), and a reverb or delay only when its mix is at least 1%. Rules 1, 2 and 4 skip returns. At most one rule fires per track per rule.
1. **A compressor after a reverb or delay:** "On Vocal, DawIt Compressor comes after DawIt Reverb, so it squeezes the reverb too: between the notes the tail comes up, and the sound can pump. Usually the reverb goes last. Try the compressor before it." For a delay: "...after DawIt Delay, so it squeezes the echoes too: they come up between the notes, and the sound can pump. Usually the delay goes after the compressor." Button: *Put DawIt Compressor before DawIt Reverb* (moves it to just before that effect).
2. **A gate after a reverb:** "On Snare, DawIt Gate comes after DawIt Reverb, so it cuts the reverb's tail short: the gated reverb sound of 1980s drums. Keep it if that's the sound you want; for a natural tail, put the gate first." Button: *Put DawIt Gate first* (moves it to the first effect, after any instrument).
3. **A gate after a compressor** (with no reverb before the gate): "On Guitar, DawIt Gate comes after DawIt Compressor. The compressor narrows the gap between the notes and the noise between them, so the gate is harder to set. A gate usually goes first." Button: *Put DawIt Gate first*.
4. **Distortion or saturation after a reverb or delay:** "On Guitar, DawIt Distortion comes after DawIt Delay, so the echoes are distorted too and smear together. On a guitarist's pedalboard the distortion comes first. Unless that's the sound you want, try it before the delay." For saturation: "...DawIt Saturation comes after DawIt Reverb, so the reverb's tail is saturated too, which thickens it and can blur it. Saturation usually goes before the reverb." Button: *Put DawIt Distortion before DawIt Delay* (or the matching names).
5. **Two reverbs:** a track with its own DawIt Reverb that also has a send that is on to a return with DawIt Reverb: "Vocal has its own DawIt Reverb and also sends to Reverb, so it gets two reverbs. That can be on purpose (a short room on the track, a long hall on the return), but usually one is enough: switch one off and compare." There is no button.

These rules stay quiet for EQ and compressor in either order, any chain whose reverb or delay is last, an effect on a return after its reverb, third-party plugins and bypassed effects. In the advice list they come after the sends' advice and before the level advice.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| `sumDb ({-12, -14}, false)` (the example's Drums Σ) | -9.875 dB | 0.01 |
| `sumDb ({-12, -13, -12, -26}, false)` (the example's master) | -7.48 dB | 0.01 |
| The example's send: -12 dB out, send -12 dB | -24 dB | exact |
| `comp::getReductionDb` at threshold -30, ratio 4, knee 6: reduction at -10 and -4 | 15.0 and 19.5 dB, difference in output 4.5 dB | 0.01 |
| Two DawIt EQs (low cut 100 Hz; bell +6 dB at 3 kHz) in either order on 1 s of noise | max sample difference < 1e-3 | — |
| Level atomic 0.5 / 0.25 shown as | -6.02 / -12.04 dB | 0.01 |
| A falling level at 0.5 after 1 s of silence | < 0.001 (0.5 x 0.8^30 = 0.00062) | — |
| `getZone` at -30, -12, -3, +1 | quiet, healthy, hot, over | exact |
| The text says "10.7 ms" and "4.5 dB" | present | — |

## Steps
Group A (engine, build at step 3):
- [x] 1. Falling levels on the audio thread: the level into the chain on `Track`, an output level on `PluginSlot` and a level on `Send`, written with `updateMeterLevel` next to the existing peaks (`Track.{h,cpp}`). Tests: render a clip through the fake device, check the three are above 0, still unchanged after the device bar's `exchange (0)` reads, and below 0.001 after 1 s of silence. Done when the existing meter and send tests still pass.
- [x] 2. `src/engine/SignalFlow.{h,cpp}` (both CMake targets): the graph, build from the song, the example and the layout, under the invariants in *Code*. Tests: the *Graph* checks in *Tests*. Check: the tests print the made-up song's columns.
- [x] 3. The routing guide: the *Signal flow* topic (name, summary, text; `getExample`'s switch returns the sends example for it, since the picture uses `signalflow::getExample`), the new *Your song's routing* text, rules 1 to 5 with the *move effect* action, and the `gen::guides` summary, *opensFrom* and words (`RoutingGuide.cpp`, `TheoryGuides.cpp`). Tests: the *Guide and advice* checks; change "14 topics" to 15 in `runSendGuideTests`. Done when `./build.sh --test` passes without warnings.

Group B (UI, build at step 7):
- [x] 4. `src/ui/SignalFlowView.{h,cpp}`: the painter (boxes, orthogonal lines, sends, levels, holds, dots, dimming, overview mode) and the window content (toolbar, viewport canvas, legend), with zoom, Fit, scroll, pan, the settings keys and the repaint policy. Check: open it on the demo song and zoom from 25% to 200%.
- [x] 5. Viewing interaction only: tooltips, click to select (decision 5), the view menu, scroll-to-selection and Cmd+= / Cmd+- / Cmd+0 in the window. No call that changes the edit. Check: bypass an effect, mute a bus and move a fader in the device bar and the mixer, and see the map follow within a frame. Clicking boxes changes only the selection.
- [x] 6. Wiring: `signalFlowCommand` (Cmd+6, View menu after *Mixer*) and the window's `SafePointer` in `MainComponent`; *Show in Signal Flow...* (a view item) through a new `TrackActions` callback; in `RoutingExplainer`, the *Signal flow* picture with the shared painter (and in the PDF), the *Show my song's signal flow* button and the guide's *move effect* button (`Engine::movePlugin`, decision 6).
- [x] 7. Mini pictures: move the curve drawing out of `DeviceBar` into a shared helper (for example `src/ui/ChainPictures.{h,cpp}`), and use it in the device bar and in the map's EQ, Compressor and Gate boxes at 100% zoom or more. Check: the device bar's chips look the same as before. Done when `./build.sh --test` passes and `--export-guides` succeeds.
- [x] 8. Real-app check: add a song `"flow"` to `ScreenshotRunner`, the *sends* song plus DawIt EQ and Compressor on Lead vocal, DawIt EQ and a bypassed DawIt Saturation on Bass, and DawIt Delay (20%) then DawIt Compressor on Keys. Add a scene `"signalFlow"` with an optional `"flowZoom"` (percent or `"fit"`), and document both in `DevScreenshots.h`. Take these shots:
  - the window fitted, playing at beat 32, with Lead vocal selected: lit path, dots, sends dashed, the bypassed Saturation grey;
  - 150% on the Drums bus: the merge, the Σ, the compressor's mini curve, readable names;
  - 30% overview;
  - Drums muted: dark lines after the faders, the chain still lit before them;
  - the routing guide at *Signal flow* and at *Your song's routing*, showing the Keys advice and its button.
- [x] 9. Docs:
  - README: a Features bullet (where, Cmd+6, what the lines and dots mean, that it only shows and a click selects), *Tests* lines and *Code layout* for `SignalFlow` and `SignalFlowView`;
  - FEATURE_IDEAS: the idea's status (*Partly done*: the read-only map is done; editing in the map, drag editing and the device-bar version are open), with its index label and counts;
  - `docs/architecture.md`: the stage peaks are swapped to 0 by the device bar, while the falling levels are for any view (the doc doesn't say this now);
  - `docs/ui-design.md`: a *Signal Flow window* section (read-only, where a click only selects; sizes, zoom range, line thickness and colours, overview below 60%);
  - `docs/recipes.md` *A window*: a zoomable map window copies `SignalFlowView`.

## Tests
- **Levels:** step 1's checks.
- **Graph** (`runSignalFlowTests`). The made-up song: Kick (audio) and Snare (audio, DawIt Gate) in Drums (DawIt Compressor); Bass (MIDI, DawIt Synth, DawIt EQ, DawIt Saturation bypassed); Vocal (audio, DawIt EQ, DawIt Compressor, a -12 dB post-fader send to Reverb); Reverb (return, DawIt Reverb). Checks:
  - 20 nodes, 19 flow edges and 1 send edge (level -12, not pre-fader, from Vocal's fader to Reverb's Σ);
  - every node but the master has exactly one outgoing flow edge, and every edge goes to a higher column;
  - columns: Kick 0-1; Snare 0-2; Drums Σ 3, compressor 4, fader 5; Bass and Vocal 0-3; Reverb Σ 4, effect 5, fader 6; master 7;
  - rows: Kick 0, Snare 1, Drums 0.5, Bass 2, Vocal 3, Reverb 4, master 2;
  - only Bass's Saturation is bypassed;
  - with Kick selected, 6 nodes are lit; with Vocal, 8; with Drums, 9;
  - with Drums muted, the chains of Kick, Snare and Drums are silenced; with Vocal soloed, Kick, Snare, Drums and Bass are silenced, and Vocal and Reverb are not;
  - setting Kick's source level to 0.5 shows -6.02 dB;
  - the example has 20 nodes, its Drums Σ is -9.875 dB and its master -7.48 dB;
  - read-only: the build takes the tracks as `const`, and after it the track count, every chain's order, every bypass, mute and solo state, and every send level are the same as before.
- **Order** (pure maths): the compressor's 4.5 dB, and the two EQs in either order (from *Test numbers*).
- **Guide and advice:**
  - 15 topics; *Signal flow* has text containing "10.7 ms" and "4.5 dB";
  - a search for "signal chain", "effect order" and "plugin order" first finds *Buses, sends and routing*;
  - on a made-up song, rules 1 to 5 each fire once with topic *Signal flow*, and rules 1 to 4 carry *move effect*;
  - they stay quiet on: Compressor then Reverb; a return with Reverb then Compressor; a bypassed Reverb before a Compressor; EQ after Compressor; a reverb at 0% mix;
  - after doing rule 1's action, the chain is Compressor, Reverb, and the advice is gone.

## Website hand-off
- Features page: a *Signal flow* section, the window fitted on the `flow` song while it plays with Lead vocal selected, and the 150% Drums close-up. Text: what the lines, colours and dots mean, that it only shows the song (changes are made in the device bar and the mixer and show up live), and that a click selects a track to light up its path.
- Main page: worth a place in the mixing step, as one picture of the whole mix.
- Guide picker (Mixing area): the routing guide's *Signal flow* topic shot.
- How it's made: no change.

## Left for later
- Editing in the map (the window is read-only by the user's choice): drag a line's end to another bus, or drag from a track to a return to make a send, with loops refused and a reason given (the idea's *Edit by dragging*); also bypass, mute, solo and the track, effect and send menus from the map's boxes.
- Opening an effect's window from its box (decision 5 (b)), if wanted later.
- A compact version of the selected track's path in the device bar.
- Mini pictures for Reverb, Delay, Saturation and Distortion, and for third-party plugins.
- Effect-order advice for third-party plugins, by their plugin category.
- A master effects chain on the map, once the master has one (*A master track* idea).
- Plugin latency drawn on the lines, and an image or PDF export of the map.

## Changes while building
- Step 2: `Graph` keeps a `tracks` list (`TrackInfo`) next to the nodes, so the layout and the example share the same code; the layout is deterministic, so it is recomputed on every frame instead of only when a structure signature changes (the picture can't jump). `Node::detail` carries DawIt Synth's sound name for the tooltip.
- Test numbers: the two-EQ check turns match level off and compares the second half of 2 s of noise, after the bands' parameter glide has settled (the first second differs by up to 0.4); the settled difference is below 1e-3.
- Advice texts: the plan gives distortion with delay and saturation with reverb; the other two combinations (distortion after reverb, saturation after delay) are built from the same wording ("the reverb's tail is distorted too and smear together", "the echoes are saturated too, which thickens them and can blur them"). dawTheory may want to check them. Advice `topic` is *Signal flow* as planned, but the guide window doesn't show a link from the advice line itself.
- The guide's picture and its PDF use the normal (not overview) layout, shrunk to fit, so the effect names stay; they are small. The picture has no per-box tooltips, only one for the whole example.
- Step 8: added the shot-list key `"mute"` / `"unmute"` and the song `"flow"` and scene `"signalFlow"` (with `"flowZoom"`), documented in `DevScreenshots.h`.
- *Show in Signal Flow...* is in the track menu of tracks, buses and returns; the `PluginSlot` pointer in `Advice` (for *move effect*) is checked against the track's chain when the button is pressed.
- dawTheory: rule 4's distortion text said "the reverb's tail is distorted too and smear together" (singular subject); it now reads "..., and it all smears together", for both the reverb and the delay case (`RoutingGuide.cpp`).
- dawTheory: the map's effect descriptions now cover every curve type: Saturation "rounding or cutting off the peaks" (tape, tube, clip), Distortion "driving the sound hard into a curve that clips or folds its peaks" (overdrive to fold) (`SignalFlowView.cpp`).
- dawTheory: the two unplanned advice texts (distortion after reverb, saturation after delay) agree with the Distortion guide and fact 8; the two-EQ test with match level off is right, as off is DawIt EQ's default and match level is a level-following gain, not a fixed filter.
