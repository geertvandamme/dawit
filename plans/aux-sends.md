# Aux sends and buses (part 2: sends and return tracks)

Status: Built, not committed
Idea: [Aux sends and buses](../FEATURE_IDEAS.html#aux-sends-and-buses)

## Goal

*Track > New Return Track > Reverb* adds a **return track**, and every mixer strip gets a *Reverb* knob: turn it up and a copy of the track, at its own level, goes to one shared reverb. Tracks then sound like one room, the reverb is set once, and the send level becomes a distance knob. The *Buses and routing* guide (renamed *Buses, sends and routing*) gets five topics, and its "your song" topic offers to share reverbs that sit on several tracks. Builds on part 1 ([buses.md](buses.md)): `Track::bus` stays the one output, and sends are a list beside it.

## Decisions for you

1. **What a return is.** (a) A kind of track of its own that only sends feed (Ableton Live, Cubase); (b) any bus can also receive sends (Reaper). **Recommended:** (a). A bus takes the whole sound and a return takes a copy, and two kinds keep that difference visible.
   Answer: (a) Own kind of track (confirmed by the user).
2. **Where returns play.** (a) Into the master only, with no sends of their own; (b) into buses too, and sending to other returns, with loop checks. **Recommended:** (a). It allows no loops, and the render order is simply "everything, then the returns".
   Answer: (a) Into the master only (confirmed by the user).
3. **Where a post-fader send is taken.** (a) After the fader, before the pan (Logic's *Post Fader*); (b) after the pan too (Logic's *Post Pan*). **Recommended:** (a). DawIt Reverb listens to (L+R)/2, so with (b) a hard-panned track would feed it 6.02 dB less. The cost: a panned track's echoes on a delay return come from the middle (the text says so).
   Answer: (a) After the fader, before the pan (confirmed by the user).
4. **Mute and pre-fader sends.** (a) Muting a track, or a bus it's in, silences all its sends; (b) pre-fader sends keep playing (Ableton Live, for headphone mixes). **Recommended:** (a). DawIt has no headphone mixes, and a beginner expects mute to silence the reverb too.
   Answer: (a) Muting silences all sends (confirmed by the user).
5. **Send knobs in the mixer.** (a) A knob per return on every strip, at *Off* until turned up (Ableton Live); (b) only the strip's own sends, with a *+ Send* button. **Recommended:** (a). It takes one gesture and nothing to set up. A strip shows knobs for the first 4 returns.
   Answer: (a) A knob per return, first 4 shown (confirmed by the user).
6. **The guide's name.** Keep *Buses and routing*, or rename it *Buses, sends and routing* (part 1's decision 6 foresaw this). **Recommended:** rename the title and the menu items. `Guide::routing` and the code names stay.
   Answer: Rename to *Buses, sends and routing* (confirmed by the user).
7. **The "share one reverb" action.** (a) A button that makes a return with the first track's reverb at 100% wet, adds sends at the levels that keep each track's amount of reverb, and **bypasses** (doesn't delete) their own reverbs; (b) advice text only. **Recommended:** (a). It is the lesson in one click, and bypassing keeps it recoverable without undo.
   Answer: (a) Offer the button; it bypasses the tracks' own reverbs (confirmed by the user).

## What the user does and sees

**Adding a return:** *Track > New Return Track >* *Reverb* / *Delay* / *Empty*, or a **+ Return** button after *+ Bus* in the transport bar, which opens the same three items. *Reverb* makes "Reverb" with DawIt Reverb at *Return: plate*; *Delay* makes "Delay" with DawIt Delay at *Return: quarter echo*; *Empty* makes "Return 1". A repeated name gets "Reverb 2". The return goes at the end of the song, at the top level, and is selected. There's no shortcut, because a song needs only a few. DawIt Reverb or Delay added to a return with *+ Effect* also starts at its first *Return:* starting point. Button tooltip: "Add a return track: a shared reverb or delay that tracks send a copy of themselves to, each at its own level. Then turn up the send knobs in the mixer."

**The return row:**
- **Header:** as a bus's, without the fold arrow; "RETURN · 3 sends"; the **?** opens *Sends and returns*. Tooltip: "Reverb is a return track: Vocal and Guitar send a copy of themselves to it, and it plays into the master. Turn up a track's Reverb knob in the mixer to send it here. Right-click for more."
- **Lane:** the senders' clips as thin bars, drawn like a bus lane. When empty, dimmed: "Turn up the Reverb knob on a mixer strip to send a track here". Tooltip: "Bar 9 · Reverb · Vocal and Guitar send here". Dropped files are refused.
- **Dragging:** a return moves at the top level only, and nothing drops into it.
- **Right-click:**
  - the header "Reverb (return, 3 sends)";
  - *Output* (*Master*, greyed);
  - *Delete return track...*, which asks when tracks send to it: "Delete the return Reverb? Vocal, Guitar and Piano send to it; their sends go too. The tracks keep playing, without this reverb.";
  - *About buses, sends and routing...*.

  A selection that includes a return makes its bus without the return.

**Sends on a track:**
- **Track menu:** **Sends >** after *Output*. One item per return ("Reverb (-10.0 dB)"), ticked when on; a click turns it on at -10 dB, or off, for every selected track. Then *New return track >* (Reverb, Delay, Empty), which also turns on the sends from the selection at -10 dB.
- **Mixer strip** (now one row per send with a Sends button in the header: see [sends-button-mixer-layout.md](sends-button-mixer-layout.md)): a sends area between the pan and the fader, shown only when the song has a return. Knobs sit two per row: the return's name above (10.5, in its colour) and a 30-px dial, 44 px per row. Return and master strips leave the space empty, so the faders line up.
  - Range Off (-60) to +6 dB, with the middle at -12 dB. Double-click: Off. A pre-fader send shows "PRE" in `meterYellow`.
  - Right-click: *Post-fader* / *Pre-fader* (ticked), *Off*, *Show Reverb's chain*, *About sends and returns...*.
  - Tooltip: "Send to Reverb: how much of this track goes to the Reverb return, now -10.0 dB, taken after the fader. Drag up for more; double-click to turn it off. Right-click for pre- or post-fader." When off: "Send to Reverb: off. Drag up to send some of this track to the Reverb return."
  - A return's strip says "RETURN", and its output button reads "→ Master", disabled, with the tooltip "A return plays into the master."
  - The mixer's hint gets: "Turn up a strip's Reverb knob to send it to the shared reverb."
- **Device bar:** sends that are on are half-height chips (22 px) in the lower half of the row. They sit before the fader chip (pre-fader) or after it (post-fader); the main arrow runs over them and a short branch drops into each:
  ```
  [Instrument] → [EQ] → [Fader] ──────────────→ [BUS Drums] → [Master]
                           └→[→ Reverb -10.0 dB ▮]
  ```
  - Each chip shows the return's colour, "→ Reverb", the level, PRE, and a meter of what it sends.
  - Click: selects the return. Drag up or down: the level, 0.25 dB per pixel. Right-click: the knob's menu.
  - Tooltip: "Send to Reverb: a copy of this track, taken after its fader, at -10.0 dB, goes to the Reverb return. Drag up or down to change the level; click to see the return's chain; right-click for pre- or post-fader."
- **A return's chain:** `[Sends from Vocal, Guitar +1] → [DawIt Reverb] → [Fader] → [Master]`, with "RETURN" in the title and no *+ Instrument*. The first chip's menu lists *Show Vocal's chain (-10.0 dB)*, ..., then the guide. Its tooltip: "What comes into Reverb: the sends of 3 tracks, added up. Click for a list of them."

**Solo and mute (DawIt's rules):**
- Muting a track, or a bus it plays into, silences its sends, pre-fader ones too.
- While anything is soloed, a return is heard when it is soloed, or when a track on the solo path sends to it.
- Soloing a return silences the other tracks' own outputs, but their sends keep feeding it.
- Muting a return removes its effect; the tracks play on, dry.

**Remembered:** nothing in the app settings.

## Data and saving

- `Track::Type::fxReturn`, `isReturn()`, and `hasInput()` (a bus or a return: no clips, no instrument, plays what comes in). `isBus()` keeps meaning a bus that holds tracks.
- `struct Send` (`Track.h`):
  - `Track* target`, structural;
  - `std::atomic<float> levelDb { -10 }`;
  - `std::atomic<bool> preFader`;
  - `std::atomic<float> peak` (since the UI last read it);
  - an audio-thread `juce::SmoothedValue<float> gain`.
- `Track::sends` (`std::vector<std::unique_ptr<Send>>`, structural); `sendOffDb = -60` (at or below it, the send is off). The UI writes `levelDb` and `preFader` directly, as it does `volumeDb`. `Track::mutedAbove` is audio-thread only, like `soloAbove`.
- **Project:**
  - a return is `"type": "return"` with the track's usual fields and `"plugins"`, but no clips;
  - a track or bus writes `"sends": [ { "return": 0, "levelDb": -10.0, "preFader": true } ]`;
  - `"return"` is the index among the returns in file order; `"preFader"` is written only when true; sends that are off aren't saved.
- **Version:** as part 1's decision 4. It's `3` when the song has a return, otherwise 2 with a bus and 1 without, because today's build would read a return as a MIDI track and drop the sends. `formatVersion = 3` and `formatVersionWithoutReturns = 2`.
- **Loading:** sends are collected and resolved after every track exists. An index with no return is skipped with a warning, and older projects load unchanged.

## The guide

New topics in `routingguide`, in this order: what, why, howTo, levels, glue, mixBus, typical, **sends, sendHowTo, insertOrSend, depth, preFader**, solo, yourSong (14). The texts are in *Theory*; *what*, *solo* and *yourSong* get revised text.

| Topic | Name — summary | Picture |
|---|---|---|
| `sends` | Sends and returns — *One reverb shared by many tracks* | Tree: Vocal, Guitar, Pad, Bass → Master. Dashed sends to a Reverb return: Vocal -12.4, Guitar -8.9, Pad -1.4; Bass none |
| `sendHowTo` | Returns in DawIt — *Add a return, turn up the send* | Two small strips (Vocal, Guitar) with Reverb knobs at -12 and -9 dB, then the return's chain `[Sends from Vocal, Guitar] → [DawIt Reverb 100%] → [Fader] → [Master]` |
| `insertOrSend` | Insert or send? — *Which effects go on the track, which on a return* | Insert: `[EQ] → [Compressor] → [Reverb 20%] → [Fader]` ("the whole sound goes through"). Send: `[EQ] → [Compressor] → [Fader]`, branching at -9.8 dB to `[Reverb 100%]` ("a copy, added"). Both say "reverb 9.8 dB under the dry sound" |
| `depth` | Depth with send levels — *More reverb sounds further away* | The Reverb guide's stage, each part placed by its send: lead vocal -12.4, drums -9.8, guitar -8.9, piano -7.7, strings -4.3, pad -1.4 |
| `preFader` | Pre-fader and post-fader — *Does the send follow the fader?* | Two chains with the tap marked; fader at -6 dB, send at -10 dB; "reverb -10 dB under the dry sound" (post), "-4 dB" (pre) |

- **Song pictures** (*what*, *solo*, *yourSong*): returns are boxes tagged RETURN, in the column before the master and below the other rows. Sends that are on are dashed curves labelled "-10" or "PRE -10".
- **Opens from:** the *?* in a return's header and the send knob and chip menus, at `sends`.
- **`gen::guides` entry:**
  - name: "Buses, sends and routing";
  - summary: "Where each track's sound goes: buses that group tracks, sends to a shared reverb or delay, levels that add up, glue compression, solo and mute, and your song's routing.";
  - `opensFrom` adds "the ? in a return's header and a send knob's right-click menu";
  - new search words: "send sends aux aux-send return returns return-track fx-channel effect-return pre-fader post-fader prefader postfader insert inserts wet dry shared-reverb reverb-bus depth distance";
  - not on *Start here*.

## Code

Follows recipes.md *Topics in an existing guide* and *A command or menu item*, and part 1's bus wiring.

- **`Track.{h,cpp}`:**
  - the data above;
  - `prepare` sizes `busInput` when `hasInput()`;
  - `process` taps each send: pre-fader right after the plugins, post-fader the same signal times the track's smoothed volume gain (no pan), then times the send's smoothed gain, into `target->busInput` at offset 0, with the peak into `Send::peak`;
  - a send is silent when `muted || mutedAbove || (anySolo && ! onSoloPath && ! target->soloed)`.
- **`Routing.{h,cpp}`:**
  - new: `getReturns`, `getSenders (ret, tracks)` (sends that are on), `findSend (track, ret)`, `isSendOn`;
  - `getRenderOrder`: returns last;
  - `canMoveInto`: nothing goes into a return, and a return doesn't go into a bus;
  - `isOnSoloPath` and `getSilence` follow the rules above;
  - `getOutermost` drops returns;
  - `describePath` adds "(+ sends to Reverb)".
- **`Engine.{h,cpp}`:**
  - `addReturn (name = {})`;
  - `setSend (Track&, Track& ret, float levelDb)`: creates the send under the lock, or sets its level;
  - `removeSend`;
  - `removeTrack` removes the sends to a return;
  - `createBus` and `createBusBeside` skip returns;
  - `addPlugin` refuses instruments when `hasInput()`;
  - `renderSegment` works out `mutedAbove` (in edit order), clears every input buffer, and after each track ors its solo path into the `soloBelow` of the returns it sends to.
- **`ProjectFile.{h,cpp}`:** as above; update the header comment.
- **`BuiltInReverb.cpp`, `BuiltInDelay.cpp`:** the *Return:* presets at the end of `getPresets()`; `verb::getReturnPreset()` and `dly::getReturnPreset()`.
- **`RoutingGuide.{h,cpp}`:**
  - the topics, with `makeSendsExample` and `makeDepthExample`;
  - `Node::isReturn` and `Node::sends` (`Link { int target; float levelDb; bool preFader; }`);
  - `fromSong` and `layOut` for returns;
  - `getSendDbForMix (mixPercent)` and `getDryChangeDb (mixPercent)`;
  - `explainSong` and `analyse` per *Theory*;
  - `Advice::Action` gets `shareEffect`, `addReturnEffect` and `makeWet`, with `juce::String effect` (the identifier), and `bus` holds the return;
  - `Levels::busInputDb` includes returns (`RoutingExplainer` fills it).
- **`GainStaging.{h,cpp}`:** `StopKind::send` and `Stop::isReturn`. `findHints` skips send stops, which are off the line. The return's `busInput` hint text is in *Theory*.
- **`TheoryGuides.cpp`:** the entry.
- **UI:** audit the ~85 `isBus()` calls and use `hasInput()` where a return acts like a bus: no clips, no instrument, the FX menu, the first chip, strip and lane drawing. Keep `isBus()` for folding, members, *Ungroup*, drop targets and families.
  - `TrackHeader`, `TrackLanes`, `ArrangementView`: the return variant.
  - `TrackActions`:
    - `addReturn (ReturnKind { reverb, delay, empty }, sendFrom)`;
    - `buildSendsMenu`;
    - `showSendMenu (Track&, Track& ret, juce::Component*)`;
    - `shareOnReturn (tracks, identifier, existingReturn)`: copies the first track's plugin state, sets the mix to 100%, sends at `getSendDbForMix`, and bypasses the old slots;
    - `addReturnEffect`, `makeReturnWet`, and the return's own menu.
  - `MixerView`: a private `SendKnob`, and the return strip.
  - `DeviceBar`: `Chip::Kind::send` (a `Track&` and a `Send*`), the half-height layout, the branches in `paintArrows`, and the return's title and first chip.
  - `GainStagingExplainer::ChainPicture`: a send stop as a small box below the line, joined by a branch.
  - `RoutingExplainer`: return boxes, send links, the new pictures, and the three action buttons.
  - `MainComponent`: `newReverbReturnCommand`, `newDelayReturnCommand`, `newReturnCommand`.
  - `TransportBar`: `addReturnButton` and `onAddReturn`.
  - `PluginActions`: the return starting point.
  - The rename: grep "uses and routing" across `src/` and the tests.
  - `ReverbGuide.cpp` and `DelayGuide.cpp`: the texts in *Theory*.
- **Threading:** `sends` and `target` are structural (message thread, under the lock). Level and pre-fader are atomics. Send gains, `mutedAbove` and the input buffers belong to the audio thread. No allocation on the audio thread.

## Libraries and models

None.

## Theory

### Facts

| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | A send (aux send) sends a copy of a channel at its own level to an effect; the effect comes back on a return and is mixed in, while the channel still plays as before | Paul White, *Making The Most Of Your Mixer*, Sound On Sound, Sept 1996; Avid, Pro Tools help *Send and Return Submixing for Effects Processing* | exact |
| 2 | Pre-fade sends don't follow the channel fader (used for musicians' monitor/foldback mixes); post-fade sends do, so reverb and echo stay in proportion to the dry sound | White, SOS 1996; Ableton Live manual, *Mixing* (pre-fader example: a separate monitor mix) | exact (definition), typical (uses) |
| 3 | An effect on a return is set to 100% wet, so that the send alone sets how much effect each channel gets | White, SOS 1996; Avid Pro Tools help ("Set the effect to 100% wet") | typical |
| 4 | Processors (EQ, compressors, gates) normally go on inserts; reverb, delay, chorus can go on either; an effect for one channel only can go on an insert | White, SOS 1996 | typical / opinion |
| 5 | Names: return track (Ableton Live), Aux Input track (Pro Tools), aux (Logic Pro), FX channel (Cubase) | manufacturers' manuals (Ableton, Avid sources above; Logic and Cubase common knowledge) | exact |
| 6 | Logic Pro offers sends Pre Fader, Post Fader (follows volume, not pan) and Post Pan | Apple, Logic Remote user guide, sends | exact |
| 7 | Ableton Live: the Pre/Post switch taps before or after the mixer stage (pan, volume, track activator), so its pre-fader sends ignore the track activator; soloing a return mutes the other tracks' main outputs but what reaches the return through sends is still heard | Ableton Live manual, *Mixing* | exact |
| 8 | DawIt's mix law: dry = cos(mix·90°), wet = sin(mix·90°) | `fx::getMixGains`, `EffectTools.h` | exact |
| 9 | The send level that gives the same reverb-to-dry balance as an insert at mix m: 20·log10(tan(m·90°)). 15% → -12.39 dB; 18% → -10.74; 20% → -9.76; 22% → -8.87; 25% → -7.66; 35% → -4.25; 45% → -1.37; 50% → 0.00 | derived from 8 (e.g. tan 18° = 0.3249, 20·log10 0.3249 = -9.76) | exact |
| 10 | Moving from an insert at mix m to a send makes the dry sound louder by -20·log10(cos(m·90°)): 20% → +0.44 dB, 45% → +2.38 dB | derived from 8 (cos 18° = 0.9511) | exact |
| 11 | At 100% mix the dry gain is cos 90°, below -140 dB in floats | derived | exact |
| 12 | Sends into one return add up like tracks in a bus: powers for different sounds (4 at -10 dB → -3.98 dB), amplitudes for the same sound in step (worst-case peaks +2.04 dB) | Sengpiel, *Adding amplitudes and levels* (as in the Levels topic) | exact |
| 13 | A return 10 dB under the dry sound adds 10·log10(1 + 0.1) = +0.41 dB to the total (different sounds) | derived, power sum | exact |
| 14 | Post-fader: the reverb-to-dry balance doesn't change when the fader moves. Pre-fader: fader -6 dB → reverb 6 dB more compared with the dry sound (-10 dB send → -4 dB) | derived | exact |
| 15 | DawIt Reverb listens to (L+R)/2, so a hard-panned track sent after DawIt's pan (a balance: the far side to 0) would feed it 20·log10 0.5 = -6.02 dB less | `BuiltInReverb.cpp` (input sum), `Track::process` (pan) | exact |
| 16 | The direct sound falls 20·log10 2 = 6.02 dB per doubling of distance (inverse square law); the reverberant sound in a room stays roughly as loud, so the direct-to-reverberant ratio falls with distance and is a main distance cue | inverse square law; Zahorik, *Assessing auditory distance perception using virtual acoustics*, JASA 111 (2002) | exact (6.02) / typical (room) |
| 17 | Far sounds are also quieter and duller (less attack, fewer highs) | common mixing practice; air absorption | typical |
| 18 | Mixes often use several reverb returns with different jobs: a short ambience to blend, a longer plate or hall for size and tone | Mike Senior, *Mixing Secrets for the Small Studio*, ch. 16, and his SOS columns | typical / opinion |
| 19 | Low cut on a reverb 150-300 Hz; pre-delay 30-100 ms on vocals; bass and kick little or no reverb | DawIt Reverb guide (existing, agrees) | typical |
| 20 | DawIt Saturation and Distortion report a latency (`setLatencySamples`) and DawIt has no delay compensation, so a late parallel copy can comb-filter | code; part 1 decision 3 | exact (that it's late), typical (heard as hollow) |
| 21 | Room size: 5·8^(size/100) m: 25% → 8.4 m, 45% → 12.7 m, 75% → 23.8 m | `verb::getRoomMetres` | exact |
| 22 | DawIt Reverb mixes in the Reverb guide's depth picture: lead vocal 15, drums 20, guitar 22, piano 25, strings 35, pad 45% | `ReverbGuide.cpp` | exact |

None of this contradicts an existing guide. Three texts that say "In DawIt, put a reverb (delay) on each track" become out of date, and step 9 replaces them.

### Topic texts (`routingguide::explain`)

**What is routing?** (replace the last paragraph):
"Mixers also have sends: a copy of a track, at its own level, that goes to a return track with a shared effect, such as one reverb for the whole song. The track still plays to its own place; the send only adds to it. The picture draws sends as dashed lines, and the topics from Sends and returns on explain them."

**Sends and returns** — *One reverb shared by many tracks*
"A send is a copy of a track's sound, at a level of its own, that goes to a return track. The return holds an effect, usually a reverb or a delay, and plays into the master next to the tracks. The track itself still plays as before: the send only adds to it.

Why it's done:
- One room for the whole band. When the vocal, the guitar and the pad send to the same reverb, they sound as if they were played in the same space. Five tracks with five different reverbs can sound like five different rooms.
- Set it once. Change the reverb's decay on the return and every track that sends to it follows.
- Less work for the computer. One reverb runs once, however many tracks send to it.
- More or less per track. Each track has its own send: a lot of reverb on the pad, a little on the vocal, none on the bass.

In the picture, Vocal sends to Reverb at -12 dB, Guitar at -9 dB and Pad at -1 dB, and Bass doesn't send at all. Sends are the dashed lines; the solid lines are where each track's own sound goes.

A return is not a bus. Tracks in a bus play into it instead of the master: the bus takes their whole sound. A return takes a copy, and the tracks still go their own way.

The words vary. A send is also called an aux send (short for auxiliary). The track it goes to is a return track in Ableton Live and DawIt, an aux in Logic Pro, an Aux Input track in Pro Tools and an FX channel in Cubase. On a mixing desk, every channel has send knobs, and the effect comes back on a return input."

**Returns in DawIt** — *Add a return, turn up the send*
"1. Choose Track > New Return Track > Reverb, or click + Return in the transport bar. A return track called Reverb appears at the bottom of the song, with DawIt Reverb on it at 100% mix.

2. Open the mixer (Cmd+2). Every strip now has a Reverb knob under its pan. Drag it up on the tracks that should have reverb; double-click it to turn it off. A send you switch on from a menu starts at -10 dB, about as much reverb as a 20% mix on the track itself.

3. Play the song and set the amounts by ear. Then shape the reverb once, on the return: select it, and open DawIt Reverb in the device bar.

A track's right-click menu has Sends, to turn the send to each return on or off, and New return track. In the device bar, a track's sends branch off its chain with their levels: drag one up or down to change it, or click it to see the return's chain. A return's chain starts with the tracks that send to it.

A return plays into the master. It can't hold tracks, but you can drag it up or down. Deleting a return removes the sends to it. Adding a return doesn't change any track's sound until you turn up its send."

**Insert or send?** — *Which effects go on the track, which on a return*
"An insert effect sits in the track's own chain, so the whole sound goes through it: what comes out replaces what went in. A send effect sits on a return, so it gets a copy, and its result is added to the untouched sound.

That gives a rule of thumb:
- Effects that change the sound itself go on the track: EQ, compression, gates, saturation and distortion. You want all of the vocal compressed, not a compressed copy next to the original.
- Effects that add something around the sound go on a return: reverb and delay, and sometimes chorus.

On a return, set the effect to 100% wet: DawIt Reverb's or DawIt Delay's Mix at 100%, as their Return starting points do. Then the return carries only the effect, and each send alone decides how much of it a track gets. At a lower mix, the return also adds a dry copy of every track that sends to it, which makes them louder and changes the balance.

For one track, an insert at a low mix and a send do the same job. DawIt Reverb at 20% mix puts its reverb 9.8 dB under the dry sound. A send at -9.8 dB to the same reverb at 100% on a return, with the return's fader at 0 dB, gives the same balance. Use an insert when only one track needs the effect, or for something special: a huge reverb on one word, a slapback on one guitar. Use a return when several tracks should share it. Engineers work both ways, but a return is the usual home of a song's main reverb.

EQ and compression can also go on a return, after the reverb. A low cut at 150-300 Hz on the reverb return keeps the low end clear for every track at once.

Parallel compression (a squashed copy blended under the dry sound) can also be done with a send to a return with a compressor. In DawIt, the Mix knob of DawIt Compressor, Saturation or Distortion is simpler. For Saturation and Distortion it is also safer: their oversampling delays the sound slightly, DawIt doesn't line up such delays yet, and a late copy blended with the dry sound can sound hollow."

**Depth with send levels** — *More reverb sounds further away*
"In a room, the sound that comes straight from an instrument gets quieter the further away you are: about 6 dB quieter each time the distance doubles. The reverb, the sound bouncing around the room, stays roughly as loud wherever you stand. So the further away a sound is, the more reverb you hear compared with its direct sound. The ear uses this to judge distance, and a mix can use it too.

With one shared reverb, the send level works as a distance knob. The picture places the parts by their sends:
- lead vocal -12.4 dB: up front;
- drums -9.8 dB, guitar -8.9 dB, piano -7.7 dB: the band, behind the singer;
- strings -4.3 dB and pad -1.4 dB: far back.
These are the Reverb guide's mixes (15%, 20%, 22%, 25%, 35% and 45%) turned into the send levels that give the same amount of reverb.

Reverb isn't the only clue. Closer sounds are also louder, brighter and sharper at the start, so a part far back in the mix is usually a little quieter and darker too. Pre-delay works the other way: a longer pre-delay keeps a sound close even with plenty of reverb, because its start is heard dry first (see the Reverb guide).

When everything is far away, the mix gets muddy. Pick what should be up front, usually the lead vocal and one main instrument, keep their sends low, and let the rest sit behind. To check, mute the return: the mix should get closer and drier, not fall apart."

**Pre-fader and post-fader** — *Does the send follow the fader?*
"A send can take its copy at two places in the track's chain:
- Post-fader, after the track's fader: the usual choice, and DawIt's default. Turn the track down 6 dB and its send goes down 6 dB too. The track keeps the same amount of reverb, just quieter. That's almost always what you want: when you turn a vocal up or down, its reverb follows.
- Pre-fader, before the fader. The send ignores the fader. Turn the track down 6 dB and the reverb stays as loud, so a send of -10 dB now gives reverb only 4 dB under the dry sound. Pull the fader all the way down and only the reverb is left.

What pre-fader is for: in studios, the musicians' headphone mixes, which must not change when the engineer moves a fader. In a mix it's an effect: a sound that fades into its own reverb, or a reverb that keeps one level while the track moves.

In DawIt, both kinds are taken after the track's effects and before its pan. A guitar panned hard left still sends its whole sound, so its reverb fills the space around it. DawIt Reverb listens to left and right added together, so a send taken after the pan would give that guitar 6 dB less reverb than a centred one. The price is that its echoes on a delay return come from the middle. Programs differ here; Logic Pro, for one, also offers a send after the pan.

Right-click a send knob in the mixer, or a send in the device bar, and choose Pre-fader or Post-fader. A pre-fader send shows PRE in yellow.

In DawIt, muting a track also silences its sends, pre-fader ones included. Some programs, such as Ableton Live, keep pre-fader sends playing while a track is off, for those headphone mixes."

**Solo and mute** (add after the second paragraph):
"Returns follow the same idea. While a track is soloed, the returns it sends to stay on, so you hear a soloed vocal with its reverb. Soloing a return plays only the return: the other tracks go quiet but keep sending, so you hear the reverb on its own, which is a good way to hear what it does. Muting a return takes its effect out of the mix, and the tracks play on, dry. Muting a track also silences its sends."

**Your song's routing** (replace the first paragraph):
"This page looks at your song's tracks, buses and returns, and says what could help. For buses: tracks of one kind that could share one, a drum bus with no effects yet, a bus with only one track or none, and buses nested deeper than two levels. For returns: the same reverb or delay on several tracks that could share one return, a return with no sends or no effect, an effect on a return that isn't at 100% wet, and a lot of reverb on the bass. And while the song plays: a bus or return whose input adds up to over 0 dB, or a master with less than about 6 dB of headroom."

### Other texts

- **Gain staging hint, return** (`findHints`, a `busInput` stop with `isReturn`): "The sends into Reverb add up to +1.2 dB peaks. Nothing distorts inside DawIt, but the return's effects get a hot level. Turn the sends down a little, together, or the first effect's input."
- **`ReverbGuide.cpp`, the Mix knob's last paragraph:** "In most mixes one reverb sits on a return track and every track sends a part of itself to it, so all instruments share one space. On a return, set the mix to 100% (the Return starting points do) and set each track's amount with its send. See Buses, sends and routing."
- **`ReverbGuide.cpp`, the last paragraph of "How it's used":** "In most mixes one reverb sits on a return track and every track sends some of itself to it, so all share one room and the computer runs only one reverb. In DawIt: Track > New Return Track > Reverb, then turn up the Reverb knob on each mixer strip. A send of -10 dB gives about as much reverb as a 20% mix on the track. Put a reverb on the track itself when only that track needs it, or for a special effect."
- **`DelayGuide.cpp`, the last paragraph:** "A delay that several tracks share sits on a return track: Track > New Return Track > Delay, then turn up the Delay knob on each mixer strip. On a return the mix is 100%, and each send sets how much a track gets. A delay on the track itself is fine when only that track needs it, such as a slapback on one guitar."

### Starting points (settings in the knobs' order; notes by knob)

DawIt Reverb (predelay, size, decay, damping, low cut, early, width, mix):
- **Return: plate** — "For a return track: one bright, smooth plate that vocals, snare and keys can share. The mix is at 100%, so the return carries only reverb and each track's send sets how much it gets." {40, 45, 1.6, 9000, 250, 0, 100, 100}. Notes: "40 ms: each sound starts dry, then the reverb follows, so words and hits stay clear." / "45%: a plate has no walls, but its tail is dense like a mid-sized room's." / "1.6 s: carries the end of a phrase and clears before the next at most tempos." / "9 kHz: a plate is bright; softening only the very top keeps s and t sounds from hissing in the tail." / "250 Hz: the shared reverb takes no lows from any track, so the low end stays clear for all of them." / "0%: a plate has no walls, so no early reflections: just the tail." / "100%: the reverb spreads around every sound that sends to it." / "100%: on a return, only the reverb. The dry sound comes from the tracks themselves, and the sends set the amounts."
- **Return: short room** — "For a return track: a small, short room that puts dry tracks in one space without a tail you notice. Send a little of the drums, guitars and keys to it, so they sound played together." {5, 25, 0.7, 8000, 200, 75, 100, 100}. Notes: "5 ms: in a small room the walls are close, so the reflections come almost at once." / "25%: a room about 8 m across." / "0.7 s: short enough that it isn't heard as a tail, only as a space." / "8 kHz: a little softer in the highs, like a furnished room." / "200 Hz: keeps the kick and bass out of the room." / "75%: in a small room, the early reflections are most of what you hear." / "100%." / "100%: on a return, only the reverb."
- **Return: hall** — "For a return track: a warm concert hall for ballads, strings, pads and choirs, shared so they sit in one big space." {30, 75, 2.4, 5500, 200, 40, 100, 100}. Notes: "30 ms: keeps the start of each note clear." / "75%: a hall about 24 m across." / "2.4 s: a concert hall's tail; slower songs can take 3 s." / "5.5 kHz: a hall's air and seats soften the highs, so the tail sounds warm and far away." / "200 Hz: keeps the long tail out of the low end." / "40%: enough early reflections to hear the walls of a hall." / "100%: a wide, enveloping space." / "100%: on a return, only the reverb."

DawIt Delay (time, feedback, high cut, low cut, mode, mix):
- **Return: quarter echo** — "For a return track: quarter-note echoes that several tracks can share, heard mostly in the gaps between phrases." synced (1/4, 30, 4500, 300, stereo, 100). Notes: "1/4: the echo of a word lands on the next beat." / "30%: two or three repeats." / "4.5 kHz: dark echoes stay behind the sounds they repeat." / "300 Hz: thin echoes don't muddy the low end." / "Stereo: the echoes keep the left and right of what is sent." / "100%: on a return, only the echoes; each send sets how much a track gets."
- **Return: dotted ping-pong** — "For a return track: dotted-eighth echoes that bounce from left to right, shared by the parts that should move: synths, keys, guitar lines." synced (dotted 1/8, 35, 5000, 250, ping-pong, 100). Notes: "Dotted 1/8: three sixteenths, so the echoes fall between the notes played." / "35%: three or four repeats." / "5 kHz: the echoes stay behind the sound." / "250 Hz." / "Ping-pong: the first echo comes from the left, the next from the right, and so on." / "100%: on a return, only the echoes."

### Example data
- `makeSendsExample`: Vocal (send to Reverb at -12.4), Guitar (-8.9), Pad (-1.4), Bass (none), and the Reverb return; all play into the master.
- `makeDepthExample`: Lead vocal -12.4, Drums -9.8, Guitar -8.9, Piano -7.7, Strings -4.3, Pad -1.4 → Reverb (fact 9 applied to fact 22).
- `insertOrSend` picture: 20% mix → reverb 9.76 dB under the dry sound; the send at -9.76 dB. `preFader` picture: fader -6 dB and send -10 dB give reverb 10 dB under the dry sound (post) and 4 dB under it (pre).

### "Your song" rules (`analyse`, after the bus rules)
1. **Share an effect:** 2 or more tracks (not returns), each with an un-bypassed DawIt Reverb (or DawIt Delay) at a mix under 100%. "Vocal, Guitar and Piano each have their own DawIt Reverb. Share one instead: a Reverb return with Vocal's settings at 100% wet, and a send from each at the level that keeps its amount of reverb (Vocal -9.8 dB for its 20% mix). Their own reverbs are switched off, not deleted." Action: *Share one reverb on a return* (`shareEffect`). When a return already has that effect, the text starts "Send them to Reverb instead" and the action uses that return. It stays quiet for a single track. (`insertOrSend`)
2. **No sends:** "Reverb has no sends yet. Turn up the Reverb knob on the mixer strips of the tracks that need space." (`sendHowTo`)
3. **No effect:** "Reverb is a return with no effect on it. A return usually holds a reverb or a delay, at 100% wet." Action: *Put DawIt Reverb on Reverb*. It's DawIt Delay instead when the name has the word "delay" or "echo"; either starts at its *Return:* starting point. (`sendHowTo`)
4. **Not 100% wet:** DawIt Reverb or Delay on a return with the mix under 99.5%. "DawIt Reverb on Reverb is at 25% mix, so the return also adds a dry copy of every track that sends to it. On a return, set it to 100%." Action: *Set DawIt Reverb on Reverb to 100% wet* (`makeWet`). (`insertOrSend`)
5. **Reverb on bass:** a Bass-family track, or one with the name word "kick", sending above -20 dB to a return with DawIt Reverb. "Bass sends to Reverb at -10.0 dB. Reverb on bass usually makes the low end muddy: try a lower send, or none." It stays quiet at -20 dB or below. (`depth`)
6. **Pre-fader**, for each pre-fader send that is on: "Vocal's send to Reverb is pre-fader, so turning Vocal down leaves its reverb as loud. For a reverb that follows the track, right-click the send and choose Post-fader." (`preFader`)
7. **Hot return** (while playing, the return has effects, its input is over +0.05 dB): "The sends into Reverb add up to +1.2 dB peaks. Nothing distorts inside DawIt, but turn the sends down a little, together, so the effect on the return gets a healthy level." (`levels`)
- **`explainSong`** counts returns ("7 tracks, 2 buses and 1 return") and adds "Reverb is a return: Vocal (-10.0 dB) and Guitar (-12.0 dB, pre-fader) send to it." or "Reverb is a return with no sends."

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| `getSendDbForMix` 15 / 18 / 20 / 22 / 25 / 35 / 45 / 50 | -12.39 / -10.74 / -9.76 / -8.87 / -7.66 / -4.25 / -1.37 / 0.00 | 0.01 dB |
| `getSendDbForMix` 0 / 80 | -60 (off) / +6 (clamped) | exact |
| `getDryChangeDb` 20 / 45 | +0.44 / +2.38 | 0.01 dB |
| `fx::getMixGains (100).first` | abs < 1e-6 | |
| 440 Hz sine, peak 0.5, post send -6.02 dB, empty return at 0 dB: return peak | 0.25 | 1% |
| same: master peak (track + return, same sound) | 0.75 | 1% |
| track fader -6.02 dB: return peak post / pre | 0.125 / 0.25 | 1% |
| track muted (pre or post), or in a muted bus | return < 1e-6 | |
| track panned hard left, post send: return input, right channel | 0.25 | 1% |
| 440 and 660 Hz sines, RMS 0.177 each, both sending at 0 dB | return RMS 0.250 | 1% |
| A and B send; solo A: return carries A only. Solo the return: master = return only | as stated | 1% |
| send at -60 dB | return < 1e-6 | |
| a bus that sends: its send carries its tracks' sum | = the bus's fader input | 1% |

## Steps

Group A, engine (`./build.sh --test` at step 5):
- [x] 1. Track and routing data and functions (*Code*). Files: `Track.{h,cpp}`, `Routing.{h,cpp}`. Tests: `runRoutingTests` (the routing checks under *Tests*).
- [x] 2. Engine and sound: `addReturn`, `setSend`, `removeSend`, the `removeTrack` clean-up, `createBus` skipping returns, and the sends in `renderSegment` and `Track::process` with the solo and mute rules. Files: `Engine.{h,cpp}`, `Track.cpp`. Tests: new `runSendAudioTests` (the audio rows).
- [x] 3. Saving: returns, sends, version 3. Files: `ProjectFile.{h,cpp}`. Tests: `runProjectFileTests`.
- [x] 4. The *Return:* starting points and `getReturnPreset`. Files: `BuiltInReverb.{h,cpp}`, `BuiltInDelay.{h,cpp}`. Tests: the reverb and delay tests.
- [x] 5. Guide content, advice, the gain staging send stop and hint, and the Theory Guides entry. Files: `RoutingGuide.{h,cpp}`, `GainStaging.{h,cpp}`, `TheoryGuides.cpp`. Tests: `runRoutingGuideTests`, `runGainStagingTests`, `runTheoryGuidesTests`. Done when: the build passes.

Group B, UI (`./build.sh --test` at step 10):
- [x] 6. The `isBus()` audit; the return's header, lane and drag rules. Files: `TrackHeader`, `TrackLanes`, `ArrangementView`, `PluginActions`. Check: a return row shows its senders' clips and can't be dropped into a bus.
- [x] 7. *New Return Track >*, **+ Return**, the track menu's *Sends >*, the return's menu, and `TrackActions`. Files: `TrackActions`, `MainComponent`, `TransportBar`.
- [x] 8. Mixer send knobs and the return strip. Files: `MixerView.{h,cpp}`. Check: knobs show only with a return; PRE shows; double-click gives Off.
- [x] 9. Device bar send chips and the return's chain; the `ChainPicture` branch; the Reverb and Delay guide texts. Files: `DeviceBar.{h,cpp}`, `GainStagingExplainer.cpp`, `ReverbGuide.cpp`, `DelayGuide.cpp`.
- [x] 10. Guide window pictures and action buttons, and the rename. Files: `RoutingExplainer.{h,cpp}` and the renamed strings. Check: `--export-guides` passes. Done when: the build passes.
- [x] 11. Real-app check (recipes.md). Set up: `--demo`, Reverb and Delay returns, three tracks sending to Reverb (one pre-fader), and DawIt Reverb at 20% on two other tracks. Shots:
  1. the return rows;
  2. the mixer with knobs and PRE;
  3. a device bar with two send chips;
  4. a return's chain;
  5. the guide at *Sends and returns*, *Insert or send?*, *Depth* and *Pre-fader*;
  6. *Your song's routing* with *Share one reverb*;
  7. the same after clicking it.

  Look for cut-off text, numbers that don't match and send links that aren't drawn.
- [x] 12. Docs:
  - README: extend the buses bullet with returns and sends; the guide bullet gets 14 topics and the new name; *Tests* lines for sends, saving, presets and the guide; *Code layout*.
  - FEATURE_IDEAS: the *Partly done* line.
  - `docs/architecture.md`, *The edit*: returns, sends, and the render order.
  - `docs/ui-design.md`: the send knob and the branch chips.

## Tests

- **Routing:**
  - a track can't go into a return, and a return can't go into a bus;
  - `getSenders` ignores sends that are off;
  - the render order has the returns after every track and bus;
  - a return is `solo`-silent when another track is soloed and none of its senders is, and audible when a soloed track sends to it;
  - `createBus` with a return in the selection leaves the return at the top level.
- **Engine:**
  - removing a return removes the sends to it, and the next render is safe;
  - an instrument is refused on a return;
  - `setSend` called twice keeps one send.
- **Audio:** the *Test numbers* rows.
- **Project:**
  - two returns, a pre-fader send, a bus that sends and a send that's off save, load and save to the same text, at version 3, with no entry for the off send;
  - a song with a bus but no return saves at version 2, and one with neither at version 1;
  - a send to a missing return loads with a warning;
  - version 4 is refused.
- **Presets:**
  - three Reverb and two Delay *Return:* starting points, each at mix 100, with every note filled in;
  - `getReturnPreset` gives *Return: plate* and *Return: quarter echo*;
  - `getRoomMetres (25)` is within 0.5 of 8 (the "about 8 m" note).
- **Guide:**
  - 14 topics, each with a name, a summary, a text over 200 characters and an example of at least 3 boxes;
  - the sends example has a return and 3 links;
  - `layOut` puts a return one column before the master;
  - the pop example still has 18 boxes;
  - the *Insert or send?* text contains "9.8 dB";
  - searching "aux send", "return track" and "pre-fader" finds `routing`, now named "Buses, sends and routing".
- **Advice** (a made-up song):
  - Vocal and Guitar with DawIt Reverb at 20% give `shareEffect`, with "-9.8 dB" in the text;
  - an empty return gives the no-sends text;
  - a return without effects gives `addReturnEffect` with DawIt Reverb, or DawIt Delay when it's named "Delay";
  - a return with its reverb at 25% gives `makeWet`;
  - Bass sending at -10 dB gives the bass advice, and at -25 dB nothing;
  - a pre-fader send gives the pre-fader advice;
  - a return input of +1.2 dB while playing gives the hot text, and nothing when the return has no effects.
- **Gain staging:** a send stop between the fader and the master doesn't change the chain's hints; a return's hint starts "The sends into".

## Website hand-off

- **Features page:** a "Sends and return tracks" part in the buses section: one reverb set once, send knobs, branches in the device bar, the five topics, and *Share one reverb*. Rename the guide everywhere on the site, including the guide picker.
- **Main page:** one line in the mixing step, if buses are mentioned there.
- **Screenshots:**
  - the mixer of a 6-track song with two returns and a PRE send;
  - Vocal's device bar with two send chips;
  - the guide at *Depth with send levels*;
  - *Your song's routing* offering *Share one reverb*.
- **How it's made:** no change.

## Left for later

- Returns that play into buses, sends from returns, and loops (decision 2).
- A send taken after the pan, or with its own pan; pre-fader sends that stay live while muted; headphone mixes (decisions 3 and 4).
- Send automation and reverb "throws" (DawIt has no automation).
- Knobs for more than 4 returns per strip; send knobs in the track headers.
- Delay compensation on parallel paths (part 1, decision 3).
- Side-chains fed by a send.
- Stems per return.

## Changes while building
- Returns always sit in one block at the bottom (a new track goes above them, a return can only be reordered among the returns, and a project loads them last), so the order on the file and the rows agree; the plan let them be dragged anywhere at the top level.
- `Engine::setSend` at Off keeps a send that exists (at -60) instead of removing it, so the pre-fader choice survives turning a knob down to Off; such a send isn't saved. `removeSend` removes it.
- The device bar grows by 26 px (`DeviceBar::getPreferredHeight`) while the shown track has sends, because the chain row is only 34 px high.
- `Stop::preFader` and `Stop::tapAfter` were added to the gain staging stop for the send boxes; `ScreenshotRunner` got `"song": "sends"`.
- The depth topic is drawn as parts placed along a close-to-far line by send level (not the Reverb guide's stage itself).
- "Send them to Reverb instead" is the second sentence of the advice text, after the "each have their own" sentence.
