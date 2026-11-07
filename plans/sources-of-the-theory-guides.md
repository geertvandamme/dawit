# Record the sources of the 13 theory guides that have none

Status: Released. Theory checked for Chords, Chord shapes, Structure, Rhythm, Gain staging, EQ, Compressor, Delay, Saturation, Distortion, Subtractive, Additive, FM, Drums and Physical
Idea: [#157 — Record the sources of the 13 theory guides that have none](https://github.com/geertvandamme/dawit/issues/157)

## Goal
Every guide's *Sources* topic lists the real pages and searches its facts rest on. Thirteen guides (Chords, Chord shapes, Structure, Gain staging, EQ, Compressor, Saturation, Distortion, Subtractive, Additive, FM, Drums, Physical) still say "No sources recorded yet", and Rhythm and Delay are only partly covered. Each is checked against sources, wrong facts are corrected, and the sources are recorded, so a curious or sceptical beginner can check any claim, and what DawIt teaches is right.

## Decisions for you
1. **Who runs the groups.** Options: (a) dawTheory runs Groups A1 to A7 with this plan, dawImplement runs Group C; (b) dawImplement runs every group. **Recommended: (a)**: the issue asks for "a dawTheory pass per guide", the work is research and text correction (dawTheory's scope: text and number literals, tests' expected values, `SOURCES.md`), and dawTheory runs on the stronger model. The plan works either way: each group's steps say what to do.
   Answer: (a) dawTheory runs A1 to A7, dawImplement runs C. (user, 2026-11-06)
2. **How far to correct.** Options: (a) correct what a source contradicts, soften what can't be confirmed, and list missing caveats or topics in the issue as follow-ups; (b) also rewrite topics for completeness. **Recommended: (a)**: it keeps each run small and the guides' voice intact; (b) is a second pass per guide, much larger than this issue.
   Answer: (a) correct and soften; completeness rewrites become follow-ups. (user, 2026-11-06)
3. **A fact no source confirms** after two searches. Options: (a) soften it ("about", "around", "in the 1960s") or remove it; (b) keep it and add a reader note. **Recommended: (a)**: never keep an unsourced exact date or number; record the softening in *Changes while building*.
   Answer: (a) soften or remove. (user, 2026-11-06)
4. **Which sources count.** **Recommended:** standards and primary papers first (AES, EBU, ITU, the original paper), then textbooks and manufacturer pages, then well-known references (Sound On Sound, iZotope, Wikipedia with the sources it cites). Wikipedia alone is fine for a definition or a well-documented date; a disputed number or date needs two independent sources. Forum posts and shops never count alone.
   Answer: As recommended. (user, 2026-11-06)
5. **Echoplex date** (Delay, *History* text says 1961). Sources: first made in 1959 (Wikipedia, Mike Battle's own account), in shops from 1961 (MusicRadar). **Recommended:** "the Echoplex (first made in 1959)".
   Answer: As recommended: "first made in 1959". (user, 2026-11-06)

## What the user does and sees
No new UI. Opening any of the 15 guides at its last topic, *Sources*, shows a reader note and lists of pages read and searches instead of "No sources recorded yet". Some topic texts change where a fact was wrong or too exact. The PDF export's Sources page follows by itself.

## Data and saving
None. `SOURCES.md` (embedded in the app and the plugins) is the only data that changes.

## The guide
No new guide or topic. Per guide the heading in `SOURCES.md` gets, in the existing format (the file's own introduction):
- a short **reader note** replacing "No sources recorded yet." (or the partial note of Rhythm, Delay, EQ, Saturation, Distortion, Additive): what is sourced, what is worked out (derived) or measured by DawIt itself, and any point where sources differ. Example: "The numbers for adding tracks are worked out (10 × log10 2 = 3.01 dB). The typical levels are common studio practice from the pages below; engineers differ by a few dB.";
- **Pages read**: one line per page actually opened, with a bracketed note naming the fact it supports;
- **Searched for**: searches whose results shaped a fact.

## Code
No new code or recipe. Rules each run keeps:
- **Scope of a fix:** text and number literals in the guide's content files, starting-point "why" texts and tooltips that state facts, and the tests' expected values that depend on them. Never structure, logic or UI code (dawTheory's rule). A picture's data that depends on a corrected number is a literal too.
- **What needs a source:** definitions, history (names, years, firsts), typical values and ranges, and any number that isn't derived. A derived number is checked by working it out (write the working in the reader note when it's a headline number); what DawIt measures or shows ("the picture shows ...") needs none. Typical values stay worded as typical, with a range.
- **Agreement between guides:** where a fact repeats one in an already sourced guide (dB sums in Routing, TR-909 1983 in Styles, harmonics in Sound, Haas and ADT in Delay), the guides must agree; reuse that guide's source by listing it under this one too.
- **Budget:** about 8 to 15 searches and page reads per guide, aimed at history and typical values first. Read only the part of a page needed; don't fetch a page twice.
- **Files per guide** (text lives in the `describe`, `explain`, topic and preset functions; find them with Grep, don't read whole files):
  - Chords: `src/generate/ChordGuide.cpp` (its progressions' names and the circle of fifths); Chord shapes: `src/generate/ChordDiagrams.cpp` (`explainPlaying`, the shapes); Structure: `src/generate/StructureGuide.cpp`; Rhythm: `src/generate/RhythmGuide.cpp` (topics other than time signatures); Gain staging: `src/engine/GainStaging.cpp`;
  - the built-ins: `src/ui/<X>Guide.cpp` for EQ, Compressor, Saturation, Distortion, Subtractive, Additive, FM, Drums, Physical and Delay, plus `EqExplainer.cpp` and `CompressorExplainer.cpp` for the texts shown in those windows, and each one's starting points.
- **Tests:** `runGuideSourcesTests` in `tests/EngineTests.cpp` checks "Chords has no sources recorded yet" and that the Delay and Rhythm notes contain "Short delays" and "time-signature". The group that changes those headings updates the check in the same run (Chords: has sources; the notes check: drop it, since the made-up text already tests notes).
- No threading, saving or audio-thread involvement. Risk: low; the widest effect is a wrong correction, which the per-guide *Changes while building* lines make reviewable.

## Libraries and models
None. Reading and citing a page is not a dependency; no `LICENSE` change.

## Theory
This feature adds no topic text: its theory is the existing guides', checked. The facts below were checked while planning; each group finds the rest.

**Facts checked while planning**
- Maestro FZ-1 Fuzz-Tone 1962; used on 'Satisfaction', May 1965; Link Wray's 'Rumble' 1958; Dave Davies's slashed speaker on 'You Really Got Me' 1964. Wikipedia *Distortion (music)*. Exact (history). Saturation and Distortion are right.
- MXR Distortion+ released 1974. Wikipedia *MXR Distortion +* (search). Exact. **Distortion guide is wrong** ("the MXR Distortion+ and ProCo RAT around 1978").
- ProCo RAT: about a dozen made in 1977 and 1978, in the catalogue from 1979. Wikipedia *Pro Co RAT* (search). Exact.
- Ibanez Tube Screamer 1979 (Distortion guide): **unconfirmed** in planning; Group A4 confirms or softens it.
- Echoplex first made 1959, in shops 1961. Wikipedia *Echoplex*, Mike Battle's account, MusicRadar (search). Exact, sources differ on what counts (decision 5).
- Telharmonium patented 1897 (US 580,035), first public demonstration 1906. Wikipedia *Telharmonium* and *Thaddeus Cahill* (search). Exact. Additive's "(1897)" is right but reads as if it played then.
- George Massenburg's parametric EQ paper: 42nd AES Convention, May 1972, paper K-2; designed with Burgess Macneal, first shown in 1971. AES E-Library, Wikipedia (search). Exact. EQ guide is right; crediting Macneal is optional.
- Gain staging sums: 10 × log10 2 = 3.01 dB, 10 × log10 4 = 6.02 dB, 20 × log10 2 = 6.02 dB, 20 × log10 4 = 12.04 dB; four drums at −10 dB peaks lined up: −10 + 12.04 = +2.04 dB (text says +2.0). Derived. Exact. Agrees with Routing.
- TR-909 1983: Drums and Styles agree.

**Corrections, ready to paste**
- Distortion, the kinds-of-distortion text: "Distortion (the MXR Distortion+ and ProCo RAT around 1978, the Boss DS-1)" becomes "Distortion (the MXR Distortion+ of 1974, the ProCo RAT of 1979 and the Boss DS-1)".
- Delay, history: "the Echoplex (1961)" becomes "the Echoplex (first made in 1959)" (decision 5).
- Additive, history: "Thaddeus Cahill's Telharmonium (1897)" becomes "Thaddeus Cahill's Telharmonium (patented in 1897)".

**Claims each group looks up first** (from a scan of years and names in the texts; the group also scans the numbers)
- Chords: diatonic chords and numerals, the circle of fifths and key signatures, the named progressions (I–V–vi–IV, 50s, 12-bar blues and the rest in the list) and their example songs.
- Chord shapes: standard tuning E A D G B E, the open and barre shapes, chord tones.
- Structure: section lengths (8 and 16 bars, rap's 16-bar verse, 32-bar intros and drops at 170 to 175 BPM in drum & bass), AABA's 32 bars and the jazz head.
- Rhythm (older topics): note names (crotchet, quaver...), dotted notes and triplets, counting, tempo ranges.
- Gain staging: −18 to −6 dB peaks into effects, master peaks around −6 to −3 dB while mixing, floating point above 0 dBFS, clipping in 16- and 24-bit files.
- EQ: frequency ranges and their names, Q and bandwidth, slopes in dB per octave, instrument ranges, masking.
- Compressor: radio limiters in the 1930s, LA-2A early 1960s, 1176 in 1967, threshold, ratio, knee, attack and release ranges (Giannoulis, Massberg and Reiss 2012, cited in the text), parallel compression.
- Saturation: odd and even harmonics from symmetric and lopsided curves, tape and tube, Fuzz Face 1966.
- Distortion: Tube Screamer 1979, Klon Centaur, Fuzz Face 1966, Big Muff 1969, Roger Mayer's Octavia and 'Purple Haze' 1967.
- Subtractive: Moog modular 1964, Switched-On Bach 1968, Minimoog 1970, Prophet-5 1978, Juno, TB-303 1981, filter slopes.
- Additive: Fourier 1822, Ohm 1843 and Helmholtz in the 1860s, Schroeder phases 1970, Hammond 1935, Synclavier, Kawai K5 1987, the harmonic recipes of saw, square and triangle.
- FM: Chowning at Stanford 1967, paper 1973, licence to Yamaha, DX7 1983, sidebands and their spacing, the index.
- Drums: Wurlitzer Sideman 1959, CR-78 1978, TR-808 1980, TR-909 1983, LinnDrum 1982, the kick's pitch drop and the snare's noise.
- Physical: Karplus and Strong 1983, Julius O. Smith's waveguides, Yamaha VL1 1994, Korg Prophecy and Z1, Logic's Sculpture 2004, Pianoteq 2006, Helmholtz's bowed string (1860s), Schelleng 1973, McIntyre, Schumacher and Woodhouse 1983.
- Delay (rest): Binson Echorec, RE-201 1974, bucket-brigade chips in the late 1970s, Haas 1949, comb filtering under about 10 ms, tempo-synced times.

**Test numbers**
| Check | Expected |
|---|---|
| Guides with at least one page read | 35 of 35 |
| Guides whose summary is "No sources recorded yet" | 0 |
| Guide notes containing "No sources recorded yet" or "before DawIt began keeping a list" | 0 |
| Links not starting `https://` | 0 (existing check) |
| Gain staging's lined-up drums | +2.0 dB (derived +2.04) |

## Steps
Each group: one guide at a time, as *Code* says: read its texts, list its claims, look them up, correct, record. Each guide's step adds one line per correction (or "no corrections") under *Changes while building*, and its group updates the GitHub issue's checklist. Groups A1 to A7 are independent and can run in any order.

Group A1, harmony and songwriting (`./build.sh --test` at step 5):
- [x] 1. Chords: check and record. Update the "Chords has no sources" test to "Chords has sources". Check: `--screenshots` not needed; the test passes.
- [x] 2. Chord shapes: check and record (the shapes are facts too: compare each guitar shape in `ChordDiagrams.cpp` with a chord reference).
- [x] 3. Structure: check and record.
- [x] 4. Rhythm, the older topics: check and record; rewrite its note for the whole guide, and drop the Rhythm half of the notes test.
- [x] 5. Build. Done when: `./build.sh --test` ends with `All checks passed`, no warnings.

Group A2, levels and EQ (`./build.sh --test` at step 3):
- [x] 6. Gain staging: check and record; work out the derived sums in the note.
- [x] 7. EQ (guide and explainer texts): check and record; replace the planning note.
- [x] 8. Build, as step 5.

Group A3, dynamics and time (`./build.sh --test` at step 11):
- [x] 9. Compressor (guide and explainer texts): check and record.
- [x] 10. Delay, the topics not yet covered: check and record; apply the Echoplex correction; rewrite the note; drop the Delay half of the notes test.
- [x] 11. Build, as step 5.

Group A4, colour (`./build.sh --test` at step 14):
- [x] 12. Saturation: check and record; replace the planning note.
- [x] 13. Distortion: apply the pedal-date correction, confirm or soften the Tube Screamer's 1979, check and record; agree with Saturation on the fuzz dates.
- [x] 14. Build, as step 5.

Group A5, synthesis 1 (`./build.sh --test` at step 17):
- [x] 15. Subtractive: check and record.
- [x] 16. Additive: apply the Telharmonium correction, check and record; agree with *What is sound?* on harmonics.
- [x] 17. Build, as step 5.

Group A6, synthesis 2 (`./build.sh --test` at step 20):
- [x] 18. FM: check and record (Chowning's 1973 paper is the primary source).
- [x] 19. Drums: check and record; agree with Styles on the TR-808 and TR-909.
- [x] 20. Build, as step 5.

Group A7, physical modelling (`./build.sh --test` at step 22):
- [x] 21. Physical: check and record (Karplus and Strong 1983, Smith's online books at CCRMA).
- [x] 22. Build, as step 5.

Group C, check and docs:
- [x] 23. Tests: add the three new checks in *Tests*. `./build.sh --test` passes.
- [x] 24. Real-app check: a shot list with `"scale": 1` opening `{"guide": "<key>", "topic": "Sources"}` for `chords`, `gainStaging`, `distortion` and `physical`, window capture; each shows a reader note and links, nothing cut off. Run `--export-guides <scratch folder>`: exit 0.
- [x] 25. Docs: `SOURCES.md`'s introduction (drop "Guides that say 'No sources recorded yet' were built before that ..."); the theory-guides line in `docs/tests.md` (every guide has a page read, no empty or partial note); no README or `docs/code-layout.md` change. GitHub issue #157: per guide done and the corrections made, open follow-ups (decision 2's caveats), *Built, not committed*.

## Tests
In `runGuideSourcesTests`:
- replace "Chords has no sources recorded yet" with: Chords has sources, summary contains "pages read" (Group A1);
- drop the Delay and Rhythm note checks once their notes change (A1, A3); notes stay covered by the made-up text;
- add (Group C): every one of the 35 guides has at least one page read; no guide's summary is "No sources recorded yet"; no guide's notes contain "No sources recorded yet" or "before DawIt began keeping a list";
- any test whose expected value comes from a corrected fact changes with it, in the same step.

## Website hand-off
- Nothing new to show. `learn.html`'s guides intro ends "A guide whose sources have not been researched yet says so.": once Group C is done, dawWebsite drops that sentence. No library or tool for `about.html`.

## Left for later
- Completeness rewrites (missing caveats, misconceptions, new topics) found during the checks: listed in #157 or new issues, per decision 2.
- A periodic link check (pages move): not in scope; a dead link only shows when opened.

## Changes while building
Group A1 (dawTheory, 2026-11-07):
- Chords, Pop progression: "The most used progression in pop music" became "One of the most used progressions in pop music" (Wikipedia calls I–V–vi–IV common, not the most common); the comment in `ChordGuide.h` follows.
- Chords, Canon: "whose bass walks down step by step" was wrong (the bass is D A B F# G D G A, falling fourths and rising steps); now "the bass falls a fourth and climbs a step, again and again, while the tune on top walks down". "Visits almost every chord of the key" became "visits five of the key's seven chords" (I, V, vi, iii, IV).
- Chords, five to one: "the strongest move in music" (two places) became "the strongest way to close/end a phrase" (the authentic cadence is generally considered the strongest cadence); the five chord's role text now says "A phrase that stops on five leaves the listener waiting" so it doesn't contradict itself. Test expected value changed from "strongest move" to "strongest way to end a phrase".
- Chords, circle of fifths: "a fifth down, the strongest move between two chords" became "the most natural move between two chords" (matches `explainMove`); "V to I ends most songs" became "is the classic ending" (unconfirmed "most").
- Styles (agreement with Chords): the pop ballad's bridge, "eight chords whose bass walks down", became "the eight chords of Pachelbel's Canon"; Pachelbel's Canon listed under Styles in `SOURCES.md`.
- Chord shapes: "every other white key for most chords" was wrong for most roots (D major is D F# A); now "a simple chord skips every other letter name, as C E G and D F# A do". The guitar shapes match a chord reference (C, A, G, E, D, Am, Em, Dm open shapes and the F barre 133211); no shape changed.
- Structure: verse-chorus "the most used structure in popular music" became "the main structure of pop and rock since the 1960s"; "Sections are almost always 4, 8 or 16 bars" and "Music is felt in groups of 4 bars" softened to "usually" and "often"; "Most pop songs reach the chorus within 45 to 60 seconds" corrected to "the top-10 pop hits of 2017 reached their first chorus after 39 seconds on average" (Hit Songs Deconstructed); AABA "the form of most jazz standards" became "the main form of American popular song from the mid-1920s into the 1950s, and of many jazz standards"; the B "often starts on the IV" (unconfirmed) became "often starts away from home"; strophic "the oldest song form" became "one of the oldest song forms". Rap's 16-bar verse, 4- or 8-bar hooks, D&B at 170 to 175 BPM (agrees with Styles) and 32-bar DJ intros: confirmed, no change.
- Rhythm (older topics): "almost all pop, rock, hip-hop and dance music has four beats" became "most" (agrees with the Time signatures topic); the swing text now says the long-short ratio is "often close to" two-thirds and one third, "wider at slow tempos, nearer even at fast ones". Note names, dots, triplets, counting and the tempo ranges (agree with Styles): confirmed, no change. Its `SOURCES.md` note now covers the whole guide.
- Tests: "Chords has no sources recorded yet" became "Chords has sources" (summary contains "pages read"); the Rhythm half of the notes check is dropped, the Delay half stays.
- `SOURCES.md`: headings for Chords, Chord shapes and Structure filled (reader note, pages read, searches); Rhythm's note rewritten and four pages and one search added; Styles gains the Pachelbel page.
- Chord shapes, the hard-chord text: "there is no open shape for it, so one finger has to hold down several strings at once (a barre)" was also shown for movable shapes without a barre; now "there is no easy open shape for it, so it is played further up the neck, often with one finger holding down several strings at once (a barre)".

Group A2 (dawTheory, 2026-11-07):
- Gain staging, *Quiet, healthy, hot, over*: "0 dB is the loudest a sound file or a sound card can take" became "the loudest a sound card, or a 16- or 24-bit sound file, can take" (a 32-bit float file holds more, as the *Over 0 dB* topic and Routing say).
- Gain staging, *Louder sounds better*: "Even 1 dB is enough to fool you" softened to "Even a difference of about 1 dB can be enough to fool you" (practitioner advice; no study found pins the amount).
- Gain staging, the master hint: "it really distorts, in your speakers and in an exported file" became "on the way to your speakers and in an exported 16- or 24-bit file" (DawIt exports 32-bit float too, which doesn't clip; agrees with Routing).
- Gain staging, confirmed with no change: the bus sums (worked out in the note: 3.01, 6.02, 12.04 dB; lined-up drums +2.04, shown as +2.0, agreeing with Routing and its test), floating point's room above 0 dB, clipping of 16- and 24-bit files, peaks of -18 to -6 dB into analog-modelled plugins, master peaks of -6 to -3 dB while mixing (agrees with Routing), faders within about 10 dB of 0 dB. The zones (-24 and -6 dB) are DawIt's own and say so in the note.
- EQ, *The frequency ranges*: "a voice sings notes between about 100 Hz and 1 kHz" became "about 80 Hz" (the guide's own instruments topic, its picture and the male voice's F2 say 80 Hz).
- EQ, *Louder sounds better*: "the second time 1 dB louder, and most people say the louder one sounds better" softened to "a little louder (even 1 dB can be enough), and the louder one tends to sound better" (agrees with Gain staging).
- EQ, *How DawIt EQ works*: the biquad "makes every output sample from the last two input samples and the last two output samples" was wrong (five numbers, four samples); now "from the newest input sample, the two before it and the last two output samples" (Audio EQ Cookbook). "Which most digital EQs use" softened to "many".
- EQ, the Remove hum starting point: "60 Hz in the Americas" became "60 Hz in most of the Americas" (some South American countries use 50 Hz).
- EQ, confirmed with no change: telephone-line origin of equalisation, Massenburg's 1972 paper, 20 Hz to 20 kHz and adults to about 15-17 kHz, the ear most sensitive at 2-5 kHz, the 1 dB just-noticeable difference, the range names (edges vary by source, said in the note), the note frequencies, Q and slope numbers (worked out from DawIt EQ's filters), the instrument ranges and problem frequencies (typical values, said in the note). The explainer's texts agree with the guide.
- `SOURCES.md`: Gain staging's heading filled (note with the worked sums, 9 pages, 4 searches); EQ's planning note replaced, 6 pages and 4 searches added.

Group A3 (dawTheory, 2026-11-07):
- Compressor, *Turning down the loud parts*: "first built for radio and records in the 1930s" became "first built for radio in the 1930s ... and were then used to cut records" (the first radio limiters date from 1931; no 1930s record-cutting compressor found). LA-2A "early 1960s" (1962, UA and Wikipedia; some sources say 1965) and the 1176 of 1967: confirmed, no change.
- Compressor, *Attack*: added "Compressors measure it in different ways; in DawIt Compressor, ..." before the 63% and 95% (there is no industry standard; MathWorks uses 10% to 90%). The 63%/95% are worked out from DawIt's time constants (1 - 1/e, 1 - 1/e³).
- Compressor, the kick's click: "lasts 5-10 ms" became "lasts only a few milliseconds" in the guide, the explainer's attack advice and the Kick drum starting point (no source pins 5-10 ms).
- Compressor explainer, *Ratio*: "From about 20:1 hardly anything gets over: that's a limiter" became "From about 10:1 it's usually called limiting: at 20:1 hardly anything gets over" (10:1 and more is generally considered limiting).
- Compressor explainer, the attack readout: "the first 0.29 × attack get less than a quarter" was 25.2%; the factor is now 0.28 (24.4%).
- Compressor, confirmed with no change: the soft-knee curve (Giannoulis, Massberg and Reiss, JAES 2012, the 0.6 dB at the threshold of a 6 dB knee at 4:1), 4:1 examples, a sine's RMS 3 dB under its peak, New York compression, the sidechain filter numbers (worked out from DawIt's filter). The typical attack, release, ratio and reduction values per instrument are within the ranges guides give; they differ widely, said in the note.
- Delay, *How a delay works*: "the Echoplex (1961)" became "(first made in 1959)" (decision 5); the Binson Echorec, listed among tape machines, uses a magnetic drum and now says so; "In the late 1970s 'bucket brigade' chips ... hundreds of tiny capacitors" became "From the mid-1970s ... hundreds or thousands" (pedals from 1976, chips of 512 to 4096 stages); "Then came digital delays" implied they came after analog pedals, now "began in the 1970s as expensive studio units and reached guitar pedals in the 1980s" (Eventide 1971, Boss DD-2 1984).
- Delay, *Feedback*: "How many you hear: until they are about 60 dB down" became "How many there are before they have faded by 60 dB (the usual measure of a sound dying away) ... In a mix you hear fewer" (so it agrees with the starting points' "two or three repeats" at 30%); the feedback readout and the knob text say "before they have faded by 60 dB" and "fade out over about ten" instead of "are heard". The counts 6, 10, 20 and 66: worked out, right.
- Delay, *Short delays*: flanger and chorus times are now "about 1-10 ms" and "with longer times ... about 10-30 ms" (Wikipedia: flanging usually under 20 ms, chorus longer). Haas 1949, the echo threshold, 343 m/s, the RE-201 of 1974, slapback with Elvis at Sun (1954) and the comb filter's first notch: confirmed. Slapback 80-150 ms and doubling 20-30 ms are narrower than some sources (60-250 ms, 30-50 ms): kept as typical values, said in the note.
- Tests: the "Delay note is read" check is dropped (the note changed; made-up text covers notes).
- `SOURCES.md`: Compressor's heading filled (note, 4 pages, 5 searches); Delay's note rewritten for the whole guide, 7 pages and 1 search added, the Precedence effect line notes Haas's 1949 thesis.

Group A4 (dawTheory, 2026-11-07):
- Saturation, the harmonics readout: "below 1% you hardly hear it, around 3-10% is clear warmth" now starts "Roughly:" and adds "High harmonics show sooner than low ones, which blend with the note" (hi-fi rules of thumb, 1% just noticeable and 3% objectionable; how much you hear depends on which harmonics). "Fuzz is far over 30%": right (a square is 48%, worked out; DawIt's fuzz at 40 dB is 45-54%).
- Saturation, *Even and odd*: "Tape and most transistor circuits" became "many" (only balanced circuits are symmetric; a single-ended stage is lopsided, transistor or tube); "like a clarinet, whose sound is mostly odd harmonics" became "like the low notes of a clarinet" (in its higher registers the even harmonics are back).
- Saturation, confirmed with no change: tape's mostly odd, 3rd-harmonic saturation, single-ended tubes' 2nd harmonic, fuzz history (Maestro 1962, 'Satisfaction' 1965, Fuzz Face 1966, 'Rumble' 1958, 'You Really Got Me' 1964; agrees with Distortion), tape until the 1990s, guitar speakers up to about 5 kHz, the ear's 2-5 kHz (agrees with EQ), the harmonics' intervals, the intermodulation spacings (55, 27.5 and 7.3 Hz) and the aliasing example (45.5 kHz folds to 2.5 kHz), all worked out. Its planning note replaced.
- Distortion, *Overdrive, distortion, fuzz*: "the MXR Distortion+ and ProCo RAT around 1978" became "the MXR Distortion+ of 1974, the ProCo RAT of 1979" (the plan's correction). The Tube Screamer's 1979: confirmed (Wikipedia, Guitar World, Chicago Music Exchange), no change.
- Distortion, the tight knob text and *Tight*: "the Tube Screamer cuts below about 720 Hz before its diodes" was not quite right (its clipping stage passes the lows at unity and gives the full gain only above 720 Hz); now "turns the lows below about 720 Hz up far less on their way into its diodes". 720 Hz worked out: 1 / (2π × 4.7 kΩ × 47 nF).
- Distortion, *Octaves and folds*: "Roger Mayer built the Octavia for Jimi Hendrix" became "built the Octavia, and Jimi Hendrix played it on the solo of 'Purple Haze' (1967)" (Mayer showed it to Jimmy Page first); Buchla's wavefolders "of the early 1970s" became "of the 1970s" (no year for the 259 found).
- Distortion, *Sustain and noise*: "its level falls by about 20 dB a second" (no measured rate for electric guitars found) became "the one in the picture falls by about 20 dB a second" (its pluck fades with a 0.45 s time constant: 19.3 dB a second; agrees with Sound's 17.4 and Physical's 20).
- Distortion, the speaker knob text and *Tone and the speaker*: a 12-inch speaker "can't move enough air below about 90 Hz" became "about 80 Hz" (a Celestion Vintage 30 is rated 70 Hz to 5 kHz, resonance 75 Hz; DawIt's model cuts at 85 Hz). Above 5 kHz: confirmed.
- Distortion, confirmed with no change: Fuzz Face 1966, Big Muff 1969 and its mid-dipping tone control, Maestro 1962, diodes in the feedback (overdrive) and to ground (distortion), the THD figures (worked out from DawIt's curves: crunch at 26 dB about 42%, fuzz 45-54%), the power chord's difference tone an octave below the E (worked out). The cone's ring at 2-3 kHz is kept as typical, said in the note.
- `SOURCES.md`: Saturation's and Distortion's headings filled (reader notes with the worked numbers; Saturation 3 pages and 6 searches, Distortion 5 pages and 7 searches).

Group A5 (dawTheory, 2026-11-07):
- Subtractive, *Start rich, take away*: "It is the oldest and most common kind of synthesis" was wrong (additive is older: the Telharmonium of 1897); now "one of the oldest kinds of synthesis, and for decades it was nearly the only one" (Wikipedia: nearly universal until digital synths). "Most software synths today" softened to "Many".
- Subtractive, the same topic: "the Prophet-5 (1978) and the Roland Juno series made it polyphonic" read as if they were the first polysynths; now "the Prophet-5 (1978), the first polyphonic synth that could store its sounds, and the Roland Juno series (from 1982) brought it to chords".
- Subtractive, *The oscillators* and the wave knob text: the square "like a clarinet, whose closed tube makes mostly odd harmonics" became "like the low notes of a clarinet, whose tube, closed at one end, makes mostly odd harmonics there" (agrees with Saturation and Physical); the sine "the filter can't change it" became "the filter can change its level but not its colour".
- Subtractive, *The LFO*: vibrato "5-6 times a second" became "5-7" (agrees with the knob text, Singing, Tune, Additive and FM); PWM "the lush sound of the Roland Juno" became "part of the lush sound of the Roland Juno, along with its chorus" (its chorus is the bigger part).
- Subtractive, the PWM strings starting point: the 40% depth "swings from very thin to past a square" was wrong (30% ± 18 points reaches 12% to 48%); now "from 12% (very thin) to 48%, nearly a square".
- Subtractive, confirmed with no change: Moog modular 1964, Switched-On Bach 1968, Minimoog 1970 (mono, 24 dB), Prophet-5 1978, TB-303 1981 (24 dB, acid house), Oberheim SEM 2-pole (12 dB), the wave harmonics and readouts (-6, -12, -20; -9.5, -14; -19, -28 dB), the 24 dB octave (24.6 dB worked out from DawIt's two stages), the beat rate, key tracking (A3 is -0.25 octaves from middle C), the LFO depths, 16 voices, every preset's numbers against its settings. Typical settings kept as typical, said in the note.
- Additive, *Build it from sines*: the plan's correction, "Telharmonium (patented in 1897)"; "mixed by drawbars" was wrong for the Telharmonium, now "mixed by stops on the Telharmonium and by drawbars on the Hammond"; "the Synclavier and the Kawai K5 (1987) gave each sound dozens of harmonics" became "the Synclavier (from 1977) built its sounds from digital sines, the Kawai K5 (1987) gave each sound up to 126 harmonics" (no count found for the Synclavier); "Every steady sound" became "Every steady note" (noise isn't harmonics).
- Additive, *The harmonic series*: "from the 16th up they're less than a semitone apart" was wrong (17/16 is 105 cents); now "from the 17th up" (18/17 is 99 cents).
- Additive, *Phase*: "Ohm said so in 1843 and Helmholtz showed it in the 1860s" credited Ohm with the phase claim; now Ohm's law is that the ear hears a sound as its sines, Helmholtz "argued" that the lining up hardly matters, and "later research found some exceptions".
- Additive, *Harmonics that move*: the trumpet's high harmonics building up "over the first tenth of a second or so" (no time found) became "a moment later".
- Additive, *Drawbars* and the Drawbar organ starting point: the 16' and 5 1/3' "sound an octave below and a fifth above that, under the note" was wrong (5 1/3' is a fifth above the 8'); now "an octave below the note and a fifth above it: neither is a whole-number multiple of the note". "About 3 dB a step" became "roughly 3 dB a step (more near the bottom)" in the guide and "near the top each step is about 3 dB" in the starting point (measured steps near the bottom are larger). The comment in `BuiltInAdditive.h` ("the four lowest drawbars") now says "the four middle drawbars".
- Additive, the Even knob text: "like a clarinet" became "like a clarinet's low notes" (agrees with the guide's own *Brightness* topic and Saturation).
- Additive, confirmed with no change: Fourier 1822, Hammond 1935 and 91 tonewheels, the footages and colours, the 6.9 Hz scanner vibrato, Schroeder 1970, the harmonic series of A3 (7th -31, 11th -49, 13th +41 cents), the Gibbs overshoot (about 9%), the stiff-string formula and the bell's partials, the "ah" formants, every starting point's numbers (swell 8 and 120 ms, fade ratios, the 8th 194 and 12th 390 cents sharp at full stretch). Agrees with *What is sound?* on the harmonics of the waves (Sound's "a square ... hollow like a clarinet" is a loose simile; left for Sound's own pass).
- `SOURCES.md`: Subtractive's heading filled (note with the worked slope and beat numbers, 7 pages, 1 search); Additive's partial note replaced (note with the worked harmonic-series numbers, 8 pages, 13 searches including the clarinet line reused from Saturation).

Group A6 (dawTheory, 2026-11-07):
- FM, the level knob text: "the first sidebands appear, about 44% as loud as the carrier" was off (44% is of the unmodulated carrier; the carrier itself is then at 77%, so the sidebands are 58% of it); now "each at 44% of the carrier's unmodulated level, while the carrier drops to 77%" (J1(1) = 0.44, J0(1) = 0.77; agrees with the level readout).
- FM, *Brightness that moves*: "Every electric piano, bass and mallet patch on a DX7 works like this" softened to "Most ... work like this".
- FM, *Ratios*: 1 : 2 "the hollow sound of a square wave or a clarinet" became "a clarinet's low notes" (agrees with Saturation, Subtractive and Additive).
- FM, the "Two operators, 1 : 1" starting point: "the 5th already 20 dB down" became "the 5th already 27 dB under the 2nd" (worked out: at an index of 2 the 2nd harmonic is 0.71, the 5th 0.033, folded sidebands included).
- FM, confirmed with no change: Chowning at Stanford in 1967 by speeding up a vibrato (he dates his first experiments 1966-67, said in the note), the 1973 JAES paper, the licence to Yamaha, the DX7 of 1983 and its sales, phase modulation in Yamaha's synths, the AdLib, Sound Blaster and Mega Drive chips, four-operator synths with eight algorithms and feedback on operator 4, the DX7's six operators and 32 algorithms, Chowning's bell (1 : 1.4, index 10) and brass (index following the loudness, after Risset's trumpet analysis at Bell Labs), the electric piano's tine at 14, the marimba's second mode at 4 times, the Bessel numbers, the index 2.4 and Carson's rule (worked out), the fine knob as on a DX7, every starting point's numbers.
- Drums, *The kick* and the Bend knob text: "an 808 kick at +24 semitones begins two octaves up" was wrong for the real TR-808, whose kick falls only about a semitone; now "the 808-style kick here" / "DawIt's 808-style kick", with "(a real TR-808's kick bends far less, about a semitone)". "A synthesised kick does exactly that, and nothing else" became "does the same, often far more than a real drum". The recipes now say "808-style kick" and "909-style kick".
- Drums, *Drums from oscillators and noise* and the 909-style starting point: the TR-909 "played recorded cymbals" became "recorded hi-hats and cymbals" (its hi-hats, crash and ride are 6-bit samples).
- Drums, *Toms and the cowbell*: the cowbell's 540 and 800 Hz "are about a sixth apart" was wrong (1200 × log2 (800/540) = 680 cents); now "almost a fifth apart, but about 20 cents short of one".
- Drums, confirmed with no change: the Sideman of 1959, the CR-78 of 1978, the TR-808 of 1980 and its poor sales, the TR-909 of 1983 (agrees with Styles), the LinnDrum of 1982, the 808 cymbal's six squares (205.3 to 800 Hz, Werner, Abel and Smith), the cowbell's 540 and 800 Hz, the clap's bursts and tail, one accent for the whole 808, the Syndrum and Simmons dates, the membrane's 1.59, the General MIDI notes and the low notes' frequencies (worked out). The clap's band around 1 kHz has no source beyond clone designs: kept as "around", said in the note.
- `SOURCES.md`: FM's heading filled (note with the worked Bessel and Carson numbers, 6 pages, 6 searches); Drums' heading filled (note with the worked membrane and cowbell numbers, 4 pages, 7 searches).
- Follow-up for #157 (decision 2): the Drums guide could explain how a real 808 kick differs from the big-bend kicks of later machines and software (only the one-line caveat was added).

Group A7 (dawTheory, 2026-11-07):
- Physical, *Where it's played*: "Piano makers place the hammers at about 1/7 to 1/9 of the string, where the harsh 7th to 9th harmonics are weak" was contradicted by Conklin (that's what older books say; modern pianos hit the bass a little under 1/8, the treble at 1/12 to 1/17, and he doesn't aim to remove any partial); now "a little under 1/8 ... in the bass, nearer the end in the top notes; old books said ... piano makers today set it for the best tone". The Hammered dulcimer starting point's why text follows ("hit its bass strings at a little under 1/8 of their length, much as here").
- Physical, *Losses* and the damping knob text: losses "roughly/about with its square" stated as a law of real strings and tubes (no source; Smith only says losses grow with frequency); now "Models often make them grow with its square, as this one does".
- Physical, the decay knob text: "A guitar rings 3 to 8 seconds, a piano's low notes over 20, a violin's pluck under one" (no measured figures found) softened to "for seconds ... for tens of seconds ... far less".
- Physical, the resonator knob text: a tube "an octave lower than a string as long would be" was wrong (a string's pitch depends on its tension); now "than a tube as long that is open at both ends, such as a flute" (agrees with *Tubes and reeds*).
- Physical, *Tubes and reeds*: "Blow harder and it jumps up a twelfth" became "Overblow it (a clarinettist opens the register key to help) ... not an octave as a flute does".
- Physical, the vibrato depth text: "A violinist's is about 20 to 30 cents (20-30%), a singer's often a little more" disagreed with Singing and Tune (a classical singer's 50 to 75 cents above and below, twice or more); now "a few tens of cents (one study measured about 30, ranging from 10 to 60); a classical singer's is wider, about 50 to 75 cents above and below". *Shaping the note*: "a few cents, five or six times a second" became "by up to a few tens of cents, five to seven times a second" (agrees with the rate knob, Singing, Tune).
- Physical, *Modelling the instrument*: "Every other synth builds a sound from waves" became "Most synths".
- Physical, starting points (numbers worked out from their settings): Init's 2 kHz partial "about 0.6 s" is 0.53 s, now "about half a second"; Harp "the 2nd, 4th and 6th harmonics start at a third of their strength or less" was wrong at 45% (31%, 59%, 81%), now "the 2nd at about a third ... the 4th at under two thirds", and its why text "Harpists pluck near the middle ..., which weakens the even harmonics" (no source for where harpists pluck; only the low even ones are weakened) now "Plucked near the middle ..., which weakens the low even harmonics, the 2nd most"; Hammered dulcimer's 75% hardness touches 0.36 ms, "about half a millisecond" now "about a third", and at 12% the 8th harmonic is at 13%, "missing" now "nearly missing" (the guide's own missing mark is under 8%); Pizzicato's 25% "over the fingerboard's end" now "near" (on a violin's open string the fingerboard ends at about 18% from the bridge).
- Physical, confirmed with no change: Karplus and Strong 1983 and the averaging of two samples, Smith's waveguides in the 1980s and early 1990s, the VL1 of 1994 (some say 1993, said in the note), Korg's Prophecy (1995) and Z1 (1997), Sculpture 2004, Pianoteq 2006, Helmholtz's vibration microscope and corner (1863), Schelleng 1973, McIntyre, Schumacher and Woodhouse 1983, closed-tube odd harmonics and the octave below a flute, the reed as a valve, harder felt at higher speed, the 1/T rule and its examples, A3's 4.55 ms and 218.2 samples, the missing harmonics at 1/5 and 1/2, every recipe's numbers against its settings. The typical settings are DawIt's own, said in the note.
- `SOURCES.md`: Physical's heading filled (reader note with the worked numbers, 4 pages, 11 searches, the clarinet line reused from Saturation and Additive).
- Follow-up for #157 (decision 2): the Physical guide could say where harpists and violinists usually pluck once a source is found, and that a real clarinet's higher resonances drift from exact odd harmonics.

Group C (dawImplement, 2026-11-07):
- Tests: four new checks in `runGuideSourcesTests` (every key heading in `SOURCES.md`, at least 35, has a page read; no summary of "No sources recorded yet"; no note with that text or "before DawIt began keeping a list"). Chords' test was done in A1. `./build.sh --test`: All checks passed, no warnings.
- EQ (decision 3): "many adults only up to 15 or 17 kHz" (guide text) and "many adults up to 15-17 kHz" (the range picture's label) became "many adults hear less above about 15 kHz", which is what the Hearing range page in `SOURCES.md` supports (threshold rises sharply around 15 kHz).
- Docs: `SOURCES.md` introduction no longer says some guides have no sources; the theory-guides line in `docs/tests.md` names the new checks. No README or `docs/code-layout.md` change.
- Real-app check: `{"guide": key, "topic": "Sources"}` for chords, gainStaging, distortion, physical (scale 1, window): each shows the reader note, pages and searches with links, nothing cut off (chords and physical opened and read). `--export-guides` exited 0 with 35 PDFs.
