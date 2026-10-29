# Buses and routing: the theory

Status: Checked, with open points
Plan: [buses.md](buses.md)

Written after the feature was built (there was no pass 1), as the record of what the *Buses and routing* guide, the gain staging guide's *Through a bus* topic and hint, the *Mix bus* and *Drum bus* starting points and the bus tooltips state, where each fact comes from, and what the check changed.

## 1. Facts and sources

**Exact** facts are definitions or calculations a test can check. **Typical** values are usual ranges, given in the text as typical. **Opinion** marks practices engineers differ on; the text says so.

### Adding levels

| # | Fact | Kind | Source |
|---|---|---|---|
| 1 | N equal, coherent signals (the same sound in step) add their amplitudes: 20 log10 N dB. 2 → +6.02 dB, 4 → +12.04 dB. | exact | Sengpiel, [Adding coherent sources](https://sengpielaudio.com/calculator-coherentsources.htm) ("20 × log10 2 = +6.02 dB"); [Adding amplitudes and levels (PDF)](https://www.SengpielAudio.com/AddingAmplitudesAndLevels.pdf) |
| 2 | N equal, uncorrelated signals (different sounds) add their powers, on average: 10 log10 N dB. 2 → +3.01, 4 → +6.02, 8 → +9.03 dB. | exact (for long-term RMS) | Sengpiel (as above); derived: 10 log10 2 = 3.0103, 10 log10 4 = 6.0206, 10 log10 8 = 9.0309 |
| 3 | Two at -6 dB: different -6 + 3.01 = **-2.99 dB**; the same -6 + 6.02 = **+0.02 dB**. | exact | derived |
| 4 | Four at -10 dB: different -10 + 6.02 = **-3.98 dB**; the same (or peaks lined up) -10 + 12.04 = **+2.04 dB**. | exact | derived |
| 5 | The coherent sum is the most peaks can add up to (|a + b| <= |a| + |b|). Hits that never coincide don't add at all (the bus's peak is then the loudest track's). | exact | derived (triangle inequality) |
| 6 | Signals opposite in polarity cancel (x + (-x) = 0); partly correlated ones partly cancel (two microphones on one snare, top and bottom). | exact | derived; the snare example is common practice |
| 7 | A bus at 0 dB, centre pan and no effects changes nothing: summing is linear, so the tracks add up the same in the master either way. | exact | derived; DawIt's balance pan is 1.0 on both sides in the middle (`Track::process`), a 0 dB fader is a gain of 1 |
| 8 | Two identical 440 Hz sines peaking at 0.25 → 0.50 (+6.02 dB). | exact | derived, tested |
| 9 | 440 Hz and 660 Hz sines, each peak 0.25, RMS 0.25/√2 = 0.1768 → RMS √2 × 0.1768 = 0.250 (+3.01 dB). | exact | derived, tested |
| 10 | -6 dB is a gain of 10^(-6/20) = 0.501; 0.50 × 0.501 = 0.251; nested 0.25 × 0.501² = 0.0628. | exact | derived, tested |
| 11 | A +6 dB bell at 440 Hz on a 0.50 sum → 0.50 × 1.995 = 0.998. | exact | derived, tested |

### Clipping and headroom

| # | Fact | Kind | Source |
|---|---|---|---|
| 12 | DawIt calculates in 32-bit float, which has far more room above 0 dBFS than any mix needs (largest value about 3.4 × 10^38); a level over 0 dB between steps doesn't distort by itself. | exact | IEEE 754; DawIt's engine has no clip stage (`Engine::renderSegment`) |
| 13 | A 16- or 24-bit WAV can't hold values beyond full scale: JUCE's float-to-int conversion clamps them (hard clipping). | exact | `juce_AudioDataConverters.h` (`jlimit` in `setAsFloatLE` for Int16 and Int24) |
| 14 | Bounce to WAV offers 16-bit, 24-bit (the default) and 32-bit float. A 32-bit float file keeps peaks over 0 dB. The bounce report warns of clipping above 0 dB for 16 and 24-bit, and says a float file keeps it. | exact | `BounceDialog.cpp`, `Bounce.cpp` |
| 15 | A sound card's converter has a fixed full scale; what goes over is cut off on the way to the speakers. | exact (in principle) | general; *unconfirmed*: the exact clipping point inside Core Audio for float streams |
| 16 | DawIt's master meter reads after the master fader. | exact | `Engine::renderSegment` (peaks of the output samples) |
| 17 | Headroom = 0 dBFS minus the peak (6 dB for a peak at -6 dBFS). | exact (definition) | definition; tested |
| 18 | Before mastering, mixes commonly peak around -6 dBFS, with -6 to -3 dBFS the usual range. | typical | [Audient: The Beginners Guide to Headroom](https://audient.com/tutorial/the-beginners-guide-to-headroom) (3 to 6 dB); [Waves: How loud should a mix be before mastering](https://www.waves.com/how-loud-mix-before-mastering) (-6 to -3 dB); the gain staging guide (*Over 0 dB*: -6 to -3; *Step by step*: below about -6); FEATURE_IDEAS *Gain staging* and *The mastering chain* (-6 dBFS) |
| 19 | No limiter on the mix that goes to mastering. | opinion (common advice) | [MasteringBOX: headroom for mastering](https://www.masteringbox.com/learn/headroom-for-mastering); Audient (as above). Some engineers in loud genres mix into a limiter and send a copy without it. |

### Bus compression

| # | Fact | Kind | Source |
|---|---|---|---|
| 20 | SSL G-series bus compressor: ratios 2:1, 4:1, 10:1; attack 0.1, 0.3, 1, 3, 10, 30 ms; release 0.1, 0.3, 0.6, 1.2 s or auto. 2:1 subtle, 10:1 close to limiting. | exact (the unit's switches) | [Sound On Sound: SSL XLogic G-Series Compressor](https://www.soundonsound.com/reviews/ssl-xlogic-g-series-compressor); [UA SSL 4000 G Bus Compressor manual](https://help.uaudio.com/hc/en-us/articles/30847649785748-SSL-4000-G-Bus-Compressor-Manual). (The 500-series G Comp module adds 1.5, 3 and 5:1; not stated.) |
| 21 | Mix bus: 1 to 2 dB of gain reduction, ratio 1.5:1 to 2:1. | typical | the compressor guide (*How much, for what*: "1-2 dB on a whole mix"; ratios "1.5:1 to 2:1 glue a mix"); [MusicRadar: compress your mix bus](https://musicradar.com/how-to/compress-mix-bus); UA blog says 2 to 4 dB, so the range varies |
| 22 | Mix bus attack 10 to 30 ms, release 100 to 300 ms, auto, or fitted to the tempo. | typical / opinion | sources differ: [Music Guy Mixing](https://www.musicguymixing.com/bus-compression/) prefers 3 to 10 ms attack and auto release; SSL users often 30 ms. The text calls them a place to start. |
| 23 | Drum bus glue: 2:1 (up to 4:1), 2 to 3 dB (up to about 4), attack slow enough to keep the hits (about 10 to 30 ms). | typical / opinion | [Native Instruments blog: glue compressor](https://blog.native-instruments.com/glue-compressor/), [eMastered: glue compressor](https://emastered.com/blog/glue-compressor); the text says many use 4:1 or more reduction |
| 24 | The release should come back to almost 0 before the next beat; at 120 BPM a beat is 60 / 120 = 0.5 s = 500 ms. | exact (the arithmetic) / typical (the advice) | derived; matches the Drum bus preset's own text |
| 25 | A sidechain filter is a low cut (12 dB per octave in DawIt) in what the compressor listens to, so the kick and bass don't drive all the compression. 80 to 150 Hz is the usual corner. | exact (DawIt) / typical (corner) | `BuiltInCompressor.cpp` (`eq::BandType::lowCut`, 12); the compressor guide's *The sidechain filter* |
| 26 | DawIt Compressor's *Drum bus*: -16 dB, 2.0:1, knee 6 dB, 30 ms, 150 ms, makeup +2 dB, mix 100%, sidechain 120 Hz, peak, target 2.5 dB → "2 to 3 dB". | exact | `comp::getPresets()`; the text is built from it (`describeGlueSettings`), tested |
| 27 | DawIt Compressor's *Mix bus*: -12 dB, 1.5:1, knee 8 dB, 30 ms, 200 ms, makeup +1 dB, mix 100%, sidechain 80 Hz, RMS, target 1.5 dB → "1 to 2 dB". | exact | `comp::getPresets()`; built into the text (`describeMixBusSettings`), tested |
| 28 | Parallel compression in one plugin: DawIt Compressor's *Parallel drums* uses Mix 35%. | exact | `comp::getPresets()` |
| 29 | Tracks play into a bus after their own faders (post-fader), so a track's fader changes what the bus's compressor hears. | exact | `Track::process` into `busInput`; tested (the bus's input is the sum after the tracks' faders) |

### Words, practice and DawIt's behaviour

| # | Fact | Kind | Source |
|---|---|---|---|
| 30 | A bus that sums a group is also called a group bus, subgroup or submix; Ableton Live calls it a group track. A stem is a group rendered to its own audio file. A VCA group links faders without summing the audio, so it takes no effects. | typical (usage varies between programs) | [iZotope: VCA and sub-groups](https://www.izotope.com/en/learn/the-difference-between-vca-and-sub-groups-in-mixing.html); [Wikipedia: Stem mixing and mastering](https://en.wikipedia.org/wiki/Stem_mixing_and_mastering); [Wikipedia: Audio bus](https://en.wikipedia.org/wiki/Audio_bus) |
| 31 | The mix bus is also called the master bus, the stereo bus or the 2-bus. | typical | common usage (UA blog, SOS) |
| 32 | Solo: a soloed track keeps its buses open (it is heard through them); a soloed bus plays its tracks; mute always wins. | exact (DawIt) | `routing::isOnSoloPath`, `Track::process` (`audible = ! muted && (! anySolo || onSoloPath)`), tested |
| 33 | A muted bus's tracks still meter, because their meters are before the bus. | exact (DawIt) | tested |
| 34 | Typical groups: drums, bass (often alone), guitars, keys, vocals with backing vocals as a bus inside. | typical | common practice |
| 35 | Two levels of buses is plenty for most songs. | opinion | common advice for small projects; large productions nest more. The text says so. |
| 36 | Track peaks around -18 to -12 dB when several go into one bus; the bus fader near 0 dB. | typical / opinion | consistent with the gain staging guide (*Step by step*: -18 to -10 dB; *Effects, fader and master*: faders within about 10 dB of 0) |
| 37 | DawIt has no automation, no sends, no master effects chain and no plugin delay compensation yet. | exact | README *Known limitations*; `Engine` |

## 2. The text

The shipped text is in `src/engine/RoutingGuide.cpp` (`explain`, `getTopicName`, `getTopicSummary`, `describeGlueSettings`, `describeMixBusSettings`, `describeMasterLevel`, `analyse`), `src/engine/GainStaging.cpp` (`Topic::bus`, the bus hint) and the window's labels in `src/ui/RoutingExplainer.cpp`. It is not repeated here, to keep one copy; section 7 lists what this check changed. The topics, in order:

1. What is routing? (Where each track's sound goes)
2. What a bus is for (Treat a group of tracks as one)
3. Grouping tracks in DawIt (Select, right-click, Create bus)
4. Levels add up (Why a bus is usually louder than its tracks)
5. Glue: one compressor for the group (Make the drums move as one)
6. The mix bus (the master) (The last bus: headroom and a gentle glue)
7. Typical buses in a song (The groups most mixes use)
8. Solo and mute with buses (What you hear when a track or a bus is soloed)
9. Your song's routing (Your tracks and buses, and what to try)

Gain staging: *Through a bus* (Why a bus gets louder than its tracks).

## 3. Completeness

| A beginner expects | Where |
|---|---|
| What a bus is, and the signal path | What is routing?, What a bus is for |
| Why use one (effects, one fader, one mute) | What a bus is for |
| How to make one in DawIt, and undo it | Grouping tracks in DawIt (Ungroup, the x, Delete bus and its tracks...) |
| Grouping doesn't change a track's sound | Grouping tracks in DawIt (added) |
| Why levels add up, with numbers | Levels add up, Through a bus |
| Misconception: "a bus makes things louder" | Levels add up, Through a bus (added) |
| Peaks vs average when summing; polarity cancelling | Levels add up (added) |
| Glue compression, with settings and why | Glue |
| Track faders feed the bus compressor | Glue (added) |
| Serial vs parallel processing on a bus | Glue (added: effects in series; parallel via the Mix knob) |
| The master as the mix bus; headroom; clipping | The mix bus |
| Limiter on the master, order of work | The mix bus (now marked as advice engineers differ on) |
| The words: group, submix, stem, VCA | What a bus is for (rewritten) |
| Typical buses and how deep to nest | Typical buses in a song |
| Solo and mute rules, mute wins | Solo and mute with buses (mute wins added) |
| Sends and return tracks | What is routing? says DawIt has none yet |

**Left out on purpose:**

- Sends, returns, pre and post fader: not built (the plan's *Left for later*); the guide says so.
- Plugin delay compensation: not built; a bus with DawIt Saturation or Distortion delays its group by a fraction of a millisecond. Without sends no sound reaches the master by two paths, so nothing comb-filters; not worth a beginner's paragraph yet. Belongs in the sends plan.
- LUFS, limiters and loudness targets: the mastering ideas in FEATURE_IDEAS.html.
- Pan laws on a bus (DawIt's pan is a balance): the bus's pan works like a track's, which the guide says.
- Automation ("riding" a bus fader through the song): DawIt has none, so the text no longer says "ride".

## 4. Examples and data

- **Levels add up (bars, examples):** two tracks at -6 dB → different -2.99, the same +0.02; four drum tracks at -10 dB → different -3.98, peaks lined up +2.04 (formatted "+0.0 dB", "-3.0 dB", "-4.0 dB", "+2.0 dB").
- **Levels add up (live):** the selected bus's tracks' peaks, the measured sum (`busInputLevel`), the power sum of those peaks (labelled "Powers added (a rough guide)") and the amplitude sum ("All peaks lined up (the most)", a true upper bound).
- **Trees:** the Drums example (Kick -12, Snare -14, Hats -18 into Drums), the first example (Drums, Vocals with Backing vocals, Bass, Guitar), the pop song (18 boxes). Levels are only for the meters' look.
- **Mix bus picture:** the master's live peak against the -6 dB mark; -8 dB for an empty song ("8.0 dB of headroom, enough").

## 5. The "your song" rules

| Condition | Text | Quiet when |
|---|---|---|
| 2+ tracks of one family playing into the master or into a bus of another family | "Kick, Snare and Hats play straight into the master. Put them in a Drums bus: one fader for the kit, and one compressor to glue it." | they share a bus of their family, or a bus with no family name ("Band") |
| A bus with no tracks | "Fx is empty: drag tracks onto it, or delete it." | |
| A bus with one track | "Lead Vox has a bus to itself (Lead). A bus does most for a group: add other tracks to it, or ungroup it." | |
| A bus named for drums, 2+ tracks, no effects | "Drums has no effects yet. A gentle compressor (2.0:1, 30 ms attack and 150 ms release, ...) makes the kit move as one." | it has any effect |
| A bus inside 2+ other buses | "... is inside 2 other buses. Two levels of buses is plenty for most songs; ..." | |
| Playing, a bus's input over +0.05 dB | "The tracks in Drums add up to +2.1 dB peaks. Nothing distorts inside DawIt, but turn them down a little, together, so any effect on the bus gets a healthy level." | stopped |
| Playing, master peak over 0 dB | "The master peaks at +1.5 dB: it clips at your speakers and in a 16- or 24-bit bounce. Turn the loudest tracks and buses down (or the master fader) until the peaks stay below about -6 dB." | stopped |
| Playing, master peak between -6 and 0 dB | "The master peaks at -3.0 dB, which leaves 3.0 dB of headroom. Before mastering, about 6 dB is the usual aim: ..." | stopped, or at -6 dB and below |
| No buses, nothing else to say | "Every track goes straight to the master. That's fine for a small song; ..." | |
| Buses and no advice | "Nothing stands out: no bus is empty or holds just one track, and no group of tracks of one kind is waiting for a bus." | |

## 6. Numbers for the tests

| What is checked | Expected | Tolerance | In the tests |
|---|---|---|---|
| `sumDb ({-6, -6}, false)` | -2.99 dB | 0.01 | yes |
| `sumDb ({-6, -6}, true)` | +0.02 dB | 0.01 | yes |
| `sumDb (4 × -10, false / true)` | -3.98 / +2.04 dB | 0.01 | yes |
| `sumDb ({0, 0}, false / true)`, 8 × 0 different | +3.01 / +6.02, +9.03 dB | 0.01 | yes |
| `sumDb ({-6, -100}, false)` | -6 dB | 0.01 | yes |
| Two identical sines 0.25 through a bus | peak 0.50 | 0.01 | yes |
| 440 + 660 Hz, RMS 0.177 each | RMS 0.250 | 0.005 | yes |
| Bus fader -6 dB | 0.251 | 0.01 | yes |
| +6 dB bell at 440 Hz on the bus | 1.0 (0.998); input 0.50 | 0.05; 0.02 | yes |
| Nested buses at -6 dB each | 0.0628 | 0.005 | yes |
| Bounce with the bus at -6 dB | 0.251 | 0.01 | yes |
| Headroom of -6 / -0.5 dB | 6 / 0.5 dB | exact | yes |
| `mixBusTargetPeakDb` | -6 | exact | yes |
| Glue text has "2.0:1", "30 ms", "150 ms" from the Drum bus preset | | | yes |
| Mix bus text has "1.5:1", "30 ms", "200 ms", "1 to 2 dB", "-6 dBFS", "limiter", "no effects chain" | | | yes |
| Mix bus settings contain "80 Hz"; the mix bus text contains "24-bit"; the *Levels add up* text contains "doesn't make the mix any louder"; the *What a bus is for* text contains "VCA" and "stem"; the solo text contains "Mute always wins" ("VCA" and "stem" are in *What a bus is for*) | | | yes |

## 7. Changes made in this check

`src/engine/RoutingGuide.cpp`:

- *Levels add up*: summary "Why a bus is usually louder than its tracks" (it isn't always: hits that don't coincide don't add, opposite polarity cancels). "On average" for the power sum. Replaced "about -3.98 dB in loudness" (a power sum of peak levels isn't a loudness) with "about 6 dB louder together (-3.98 dB)". Added: peaks are less predictable, polarity can cancel, and a bus doesn't make the mix louder by itself (same sound at 0 dB, centre, no effects).
- *What a bus is for*: "ride a section: the backing vocals 2 dB down in the verse" implied automation, which DawIt doesn't have; now "all the backing vocals 2 dB down in one move". "Mixers call it a group, a submix or a stem" was wrong for *stem* (a rendered file) and ambiguous for *group* (a VCA group doesn't sum); rewritten with group bus, subgroup, submix, Ableton's group track, stem, and VCA group.
- *Grouping tracks in DawIt*: the x and *Delete bus and its tracks...*; grouping never changes a track's sound.
- *Glue*: the release in numbers (500 ms at 120 BPM), the sidechain filter, "starting points, not rules" (many use 4:1), SSL's auto release, tracks feed the bus after their faders, effects in series, parallel compression through the Mix knob.
- *The mix bus*: the clipping sentence said "a 16-bit file"; DawIt's default bounce is 24-bit, which clips too, and 32-bit float doesn't. Now "a 16- or 24-bit WAV file", with the float case. "The master cannot [go over 0 dB]" was wrong (the sum into the master is float too; it's the output after the fader that clips). Headroom: "-6 to -3 dB is the usual range", as the gain staging guide says, and why headroom helps without implying mastering needs to "raise" the level. "The sidechain low cut stops the kick and bass ducking" overclaimed a 12 dB/octave filter; now it explains what the filter does. *What not to do* became *What to watch out for*: the limiter and order-of-work items now say engineers differ. "2-bus" added. "(here in the mixer ...)" dropped.
- `describeMixBusSettings`: now names the 80 Hz sidechain filter (from the preset), so the text no longer mentions a filter it never introduced.
- `describeMasterLevel`: "(aim for 6 dB)" → "(about 6 dB is the usual aim)".
- *Typical buses*: the two-level advice is marked as advice for most songs.
- *Solo and mute*: "Mute always wins".
- *Your song's routing* intro: lists what the analysis really looks at (a drum bus without effects, empty buses, deep nesting, the master's headroom).
- Advice: the hot-bus text no longer assumes the bus has effects; the master-clipping text says where it clips and no longer calls turning tracks down "better" than the master fader as a rule; the headroom text "about 6 dB is the usual aim".

`src/engine/GainStaging.cpp`:

- *Through a bus*: the same "-4 dB in loudness" fix, hits that don't coincide, and a bus doesn't make the mix louder.
- The bus hint: "the bus's effects get a hot level ... or the first effect's input" assumed effects and an input control; now "any effect on the bus gets a hot level. Turn the tracks down a little, together."
- The very-quiet hint: "(for a bus, the tracks that play into it)", since a bus has no instrument.
- *Over 0 dB*: "an exported file" → "an exported 16- or 24-bit file", with the 32-bit float case (this guide disagreed with the bounce dialog, which offers float).
- *What is gain staging?*: the chain includes "any bus it plays into".

`src/ui/RoutingExplainer.cpp` (picture labels and the "your song" fallback text only):

- Live bars: "If they play different sounds" (a power sum of peaks, not a prediction of the bus's peak) → "Powers added (a rough guide)"; "If they play the same sound" → "All peaks lined up (the most)".
- Example bar: "4 drum tracks, peaks at -10 dB" → "4 drum tracks at -10 dB" (the -3.98 dB power sum applies to average levels).
- "Nothing stands out: every bus has a few tracks and an effect or two" could show for buses without effects; now "no bus is empty or holds just one track, and no group of tracks of one kind is waiting for a bus".

No test expected value had to change. The headless tests end with `All checks passed`.

## Open points (for dawImplement)

All done in the follow-up build (see *Changes while building* in plans/buses.md):

- Advice that would nest a family bus inside another now puts the new bus next to the other one, and says so ("a Drums bus of their own, next to Vocals rather than inside it"). Text changed: that clause.
- A second group of a family whose bus exists is told "Move them into Drums, the bus you already have for this kind of track: they then share its fader and its effects." (a `moveTrack` action). New text.
- The hot-bus hint and advice stay quiet for a bus without effects. No text changed.
- The five suggested text checks are in `runRoutingGuideTests`.

## Decisions for you

- **How much headroom to ask for.** The guide says peaks around -6 dBFS, with -6 to -3 dB the usual range, and the advice speaks up from -6 dB. Some mastering engineers say that with 32-bit float files only "no clipping and no limiter" matters. Recommended: keep -6 dB, as the gain staging guide and FEATURE_IDEAS.html do; the advice calls it "the usual aim", not a rule.
- **Mix into a bus compressor, or add it last.** Engineers differ; the text now gives both. Recommended: keep both, beginners first balancing without it.
