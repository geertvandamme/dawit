# How DawIt's UI looks and behaves

The visual language and the interaction rules every view follows, so a new screen looks and feels like the rest without reading old UI code. The numbers come from the code as it is; when a view needs something new, match the closest existing one and add it here.

## Colours

- **The theme** is in `src/ui/Theme.h` (`daw::theme`), a dark UI:

  | Token | Hex | Used for |
  |---|---|---|
  | `background` | `#1b1c20` | window backgrounds |
  | `panel` | `#25272d` | guide windows, bars |
  | `panelLight` | `#30333a` | buttons, the topic list, the explanation box |
  | `lane`, `laneAlt` | `#222429`, `#1f2125` | track lanes, alternating |
  | `grid`, `gridBar` | `#2e3138`, `#464a54` | beat and bar lines |
  | `text`, `textDim` | `#d9dce3`, `#8a8f9a` | text, secondary text and summaries |
  | `accent`, `loop` | `#4fa3ff` | selection, toggled buttons, the loop |
  | `playhead` | `#ff5c5c` | the playhead |
  | `meterGreen`, `meterYellow`, `meterRed` | `#5fd38d`, `#f2c94c`, `#ff5c5c` | meters and badges |

- **Colours that mean something** have one static helper that every view, guide picture and the website use:
  - `SectionLane::getColour (SectionType)`: calm sections cool (verse blue), high points warm (chorus red-orange, drop red);
  - `ChordLane::getColour (Chord)`: one hue per root (`root / 12` round the colour wheel, saturation 0.45, value 0.55), so repeats in a progression stand out;
  - `LyricsLane::colour` (lilac `#c9a2ff`), `TempoLane`'s colour, `LyricsExplainer::getRhymeColour (int)` for rhymes A, B, C;
  - `TheoryGuidesView`'s `getColour (gen::guides::Area)` for the guide areas;
  - track colours come from `Engine::nextTrackColour()`, a palette of 8.
  - `stereoScope::colour` (pink `#ff7fc8`) marks stereo side and the upper band; `stereoScope::lowColour` (amber `#ffc857`) marks the low band. The editor uses approximately 150 px goniometers and the device chip uses a compact scope.
  - `tunepics::getZoneColour` gives the pitch zones of DawIt Tune: green (`meterGreen`) within 10 cents of a note, yellow within 25, red beyond; `tunepics::colour` (violet `#a78bfa`) is the effect's own, and the corrected line is drawn in `accent`. One tuner painter (`paintTuner`, `paintMiniTuner`) and one trace painter (`paintTrace`: piano-roll rows with the key's shaded, the sung line by zone, gaps for no clear pitch) draw the window, the guide and the chip.
  A new colour that means something gets its own helper; don't repeat a hex value in two places.
- **Pictures** draw on `panelLight` or a darker panel (`fxui::drawPanel`: black at 28% with 4 px rounded corners), with `textDim` for labels and grids and colour only where it carries meaning.

## Text

- **Fonts** are JUCE's default sans, sized with `juce::FontOptions`. The usual sizes:

  | Size | Where |
  |---|---|
  | 19 bold | a guide window's title |
  | 16 bold | an effect explainer's title (`fxui::addTitle`) |
  | 15 | a guide's explanation text |
  | 14 bold, then 12 | a topic list row: the name, then the summary in `textDim` |
  | 13.5 | an effect explainer's lead line, in a lighter accent (`fxui::addLead`) |
  | 12.5 bold | an effect explainer's section heading (`fxui::addSection`) |
  | 12 to 13 | labels and values in the views |
  | 10.5 to 11.5 | small labels in pictures, axes, legends |

- **Fit:** `drawFittedText` shrinks text to fit before clipping it. When there's too little room (zoomed out), show an overview or an open mode rather than unreadable text (the Lyrics row shows whole lines when the syllables won't fit).
- **Non-ASCII** (the middle dot, arrows, curly quotes) through `juce::String::fromUTF8`.
- **Words:** British spelling, short sentences, real numbers, what it's for before how it works (CLAUDE.md's text rules).

## Windows

- **Opening:** through `PluginWindowManager::show (key, title, create)`, one window per key, brought to the front when opened again; the view keeps a `SafePointer` ([recipes.md](recipes.md#a-window)).
- **Sizes:** guide windows are fixed, about 940 to 1000 wide and 700 to 800 high (`LyricsExplainer` 1000 × 740, `StructureExplainer` 940 × 700, `ChordExplainer` 940 × 800). Effect windows are 1240 × 600 to 640; instrument windows 1400 × 780. A built-in's "What is ...?" guide window is 1080 × 800 (`fxui::Guide`).
- **Menu items** that open a window end with "...".

### The guide icon

Everything that opens a theory guide uses `GuideButton` (`src/ui/GuideButton.h`): its text "?" shows only the icon, any other text shows the icon first and then the text. The icon is drawn from a path (`guideicon`), dim (`textDim`) at rest and `accent` under the mouse, and the button never takes keyboard focus. A right-click menu item that opens a guide is added with `guideicon::addItem`, which puts the icon in front. In the song rows (Sections, Chords, Lyrics) the icon is at the front of the header, in the same column just right of the fold arrow, with the name after it. The key dropdowns (transport bar, New Project from Style) end, after a separator, with a "Chords and keys guide..." item (`guideicon::addItem` on the ComboBox root menu, id 1000); choosing it restores the current key and opens the chords guide at "How chords work". A "?" that explains the window itself (an effect's overview) stays a plain "?" button.

### Guide windows

The same layout everywhere (`LyricsExplainer::resized` is the model):

```
┌──────────────────────────────────────────────────────────────┐
│ Title (19 bold)                              [Export PDF...] │  30 px row, 120 px button
│                                                              │  12 px gap
│ ┌──────────────┐  ┌────────────────────────────────────────┐ │
│ │ Topic        │  │ Picture (live, about half the height)  │ │
│ │ summary      │  │                                        │ │
│ │──────────────│  └────────────────────────────────────────┘ │  10 px gap
│ │ Topic        │  ┌────────────────────────────────────────┐ │
│ │ summary      │  │ Explanation (15, read-only, scrolls)   │ │
│ │ ...          │  │                                        │ │
│ │ Your song    │  │                                        │ │
│ └──────────────┘  └────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

- margins 16 × 12 (`getLocalBounds().reduced (16, 12)`), background `theme::panel`;
- the topic list 250 px wide, rows 46 px, on `panelLight`, the selected row in `accent` at 35%;
- 14 px between the list and the right side; the picture takes `min (preferred height, half the height + 60)`;
- the explanation is a read-only multi-line `juce::TextEditor` on `panelLight`, no outline, a 10 × 12 border;
- the last topic is always *Your song* (or the guide's own name for it).

### A guide inside a plugin window

In a plugin (`fxui::isPlugin`) the guide is not a separate window, because hosts handle extra windows differently. `fxui::GuideHost` puts it over the editor, filling it: a 38 px bar with *Back to the controls* (Esc does the same), then the guide. The guide button, covered by the overlay, only opens it. It is kept, so it reopens on the page it showed. The guide's picture shrinks in a short window so the text keeps room. There is no Export PDF button. In DawIt the guides stay in their own windows.

### Effect and instrument windows

The shared frame in `EffectWindow.h` (`fxui::EffectEditor`):

- a 44 px header (name, starting points, the guide button, test sound, match level where offered), knob groups 150 px high, and the explainer panel 350 to 360 px wide on the right, telling about whatever the mouse rests on;
- knobs are `EffectKnob` (name above, dial, value below), grouped under a heading; switches are `EffectEditor::Switch`. A Type switch with several choices opens a menu with the current choice ticked, a short description per choice and the relevant guide as its last item;
- the effect's own picture is in the middle, draggable where that makes sense; the last 4 seconds of level (`LevelHistoryView`) below it.

## The arrangement

- **Fixed sizes:** track headers 270 px wide (`ArrangementView::headerWidth`), the ruler 26 px, the transport 46 px, track rows 84 px (`TrackLanes::rowHeight`), folded song rows 20 px with a line every 5 px.
- **Song rows** (Tempo, Sections, Chords, Lyrics) sit between the ruler and the tracks, each with a header on the left (a `RowLabel` that opens its guide, or a header with an arrow and buttons). A row that opens shows more (the chord diagrams, the lyrics' lines) and remembers it (`view.<x>Open`).
- **The computer keyboard strip** (`ComputerKeyboardStrip`) is 64 px high, full width, at the very bottom of the window below the bottom panel, on `theme::panel`, while typing is on (*View > Play with Computer Keyboard*, Cmd+K). The keys are drawn by `keyboardpicture::paint`, which the "What is MIDI?" topic *The computer keyboard* shares.
- **Cmd+3** folds the song rows into thin coloured lines (`FoldedSongRows`), so every row also has a line there.
- **The timeline** is shared through `TimelineView` (`beatToX`, `xToBeat`); positions snap to the grid, Alt turns snapping off.

### The inspector

- `InspectorPanel` is a full-height panel (under the transport bar, above the computer keyboard strip) on the left of the track headers, on `theme::panel`, with a 1 px `gridBar` line on its right and a 4 px drag edge (`InspectorEdge`). Width 200 to 360 px, default 230, in `view.inspectorWidth`; open state in `view.inspectorOpen` (default true); *View > Inspector*, plain I.
- From the top: a 28 px title row (13 pt bold, `textDim`, with the guide icon and ×), the colour bar and name (15 pt bold, shrinking to fit), the type line (10.5 pt, `textDim`), M, S and Output, then a scrolling viewport with *EFFECTS* (24 px rows: power dot, name, x on hover; the instrument row first on MIDI tracks) and *SENDS* (24 px `SendKnob` rows in the wide look, one per return, then *+ Send to a new return*), and at the bottom the 38 px pan knob and the volume fader with its meter. A return shows *FED BY* instead of sends; the master shows only volume and meter.
- It shows `Selection::getTrack()` (the master when null) and changes only that track. No control takes keyboard focus.

### Sends in the mixer, the inspector and the device bar

- A send is one row (`daw::SendKnob`, compact look in the mixer, wide look in the inspector; 22 px high in the mixer) per return, one under the other, between the pan and the fader, for the first four returns: a 20 px dial (drawn by the knob itself: a grey track, an arc in the return's colour up to the level, a dot) on the left, then the return's name to its right (10.5 pt, in the return's colour) and PRE in `meterYellow` for a pre-fader send; range Off (-60) to +6 dB with the middle at -12, double-click for Off. Every strip (also the return's and the master's) keeps the space so the faders line up.
- The **Sends** button (44 px, in the header's bottom row between FX and Scale, on tracks and buses) opens the Sends menu (`TrackActions::showSendsButtonMenu`: the returns, New return track >, the guide); a choice then selects the return it turned on or made (the track when a send was turned off), shows the mixer and scrolls that strip into view (`TrackActions::onShowSends`).
- A send chip in the device bar is 22 px high, in a second row below the chain (the bar grows by 26 px while the shown track has sends that are on): it starts under the gap it is taken from, a short branch in the return's colour drops from the line into it, and it shows an arrow, the return's name, the level, PRE and a meter. Dragging it changes the level by 0.25 dB per pixel.

### The score view

- The clip editor's header has *Piano roll* and *Score* switches, the track's name, then *Clef*, *Sheet music (PDF)...* and *How to read music* on the right. The score has its clef, key and time signature pinned at the left while the bars scroll; tempo and section boxes sit above the chord symbols, and grey bar numbers under them match the ruler.
- **Ink:** light on `theme::panel` on screen, black on white in the PDF and the guide's pictures (`scorepaint::Ink`). The playhead is `theme::playhead`; a sounding note and a picture's accent notes are blue.
- Hover says what is under the mouse ("Bar 9, beat 1: A4 quarter note ... played as ..."); click plays a note or moves the playhead; double-click opens the piano roll.

## Signal Flow window

A read-only map of the whole song (`SignalFlowView`, graph in `src/engine/SignalFlow`): it calls no engine function that changes the song, and its only writes are the selection and its own settings (`view.signalFlowZoom`, `view.signalFlowScroll`, `view.signalFlowPulses`, `view.signalFlowSize`). A click on a box or a line only selects that track; there are no menus of the track, effect or send, no double-click action and no dragging of boxes. Changes are made in the device bar, the mixer and the track menus, and the map follows. Default size 1200 x 760, minimum 640 x 420, a 36 px toolbar and a 22 px legend. Zoom runs from 25% to 200% (steps of x1.25, Cmd+scroll or pinch around the mouse), *Fit* adds a 24 px margin and stays on until the user zooms, and below 60% it is an overview (effect boxes are 10 px squares without names, no mini pictures). Lines are 1.5 to 6 px by `meterProportion`, coloured by the gain-staging zones (grey, green, yellow, red); a muted or unheard line is `theme::gridBar`; sends are dashed in the return's colour; dots run one 24 px step per beat; a red over-0 dB mark holds for 2 s and the master's CLIP for 3 s; with a track selected the rest is drawn at 30%. One painter (`flowview::paint`) draws the window, the routing guide's example and its PDF.

## Interaction

The same everywhere:

- click to select or move the playhead, double-click to edit, drag to move;
- right-click opens a menu whose last item opens the guide;
- Alt drags without snapping, and Cmd copies while dragging;
- Delete removes the selection;
- Cmd+C, Cmd+V and Cmd+D copy, paste and duplicate where they make sense;
- Shift-click and Cmd-click select several tracks (headers and mixer strips);
- Space plays and pauses, in every window (`TransportShortcuts`);
- while computer-keyboard typing is on, plain letters in its map (A to L, W E T Y U O P, Z X C V) play notes and change octave and velocity: `ComputerKeyboardTyping` is consulted before `TransportShortcuts`, in the main window and the plugin windows, and a focused text box still gets its letters first.

**Commands** used often get a shortcut ([recipes.md](recipes.md#a-command-or-menu-item)).

**Focus:** buttons in a row that takes key presses use `setWantsKeyboardFocus (false)` and `setMouseClickGrabsKeyboardFocus (false)`, so clicking them doesn't take the keys away from the row.

## Tooltips

On every control and interactive area (the tooltip window waits 600 ms), saying what it does and how: "Chord track: click a bar to set its chord ... Shift-click or Shift-drag across bars to select them". Over a timeline, the tooltip says what's under the mouse: "Bar 9 · Chorus · Am".

## Live pictures

- Pictures follow the playhead and the song: views are `ChangeListener`s on the engine and repaint on a change.
- Timer-driven repaints (playhead, meters, the level history) repaint only the small region that moved (`updatePlayhead()`), about 30 times a second.
- Levels are linear gain inside and dB in the text; meters run from -60 to +6 dB (`meterProportion`, `drawStereoMeter`, `drawLevelBar` in `Theme.h`).
- A click on a picture plays the sound where it helps: a chord, a rhythm, a tone.
