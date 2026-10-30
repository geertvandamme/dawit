# Code layout

Where each file lives and what it holds. Find a file with Grep rather than reading this whole page.

```
src/
  Main.cpp                 application + main window
  engine/
    Engine.{h,cpp}         audio device, plugin formats, tracks, transport, audio callback (and left minus right on the speakers)
    Track.{h,cpp}          per-track rendering: clips → plugins → volume/pan
    Clip.h                 AudioClip (in-memory audio + waveform peaks), MidiClip (notes)
    BuiltInSynth.h/.cpp    DawIt Synth: the built-in instrument and its sounds
    BuiltInEq.{h,cpp}      DawIt EQ: band filters (Audio EQ Cookbook), solo, match level, starting points, instrument ranges
    BuiltInCompressor.{h,cpp}  DawIt Compressor: the curve, attack/release, sidechain filter, match level, the
                           history for the graphs, starting points per instrument
    BuiltInGate.{h,cpp}    DawIt Gate: detector, open/closed with hysteresis and hold, expander curve, key filters, match level,
                           the last 4 seconds as moments, the simulation, examples, starting points, the advice on your track
    BuiltInStereo.{h,cpp}  DawIt Stereo: crossover, width and listen modes, match level, starting points and track advice
    StereoMeasure.{h,cpp}  shared stereo correlation, mid/side measurements and any-reader history and sample rings
    PitchTracker.{h,cpp}   McLeod pitch tracking through the FFT, allocation-free: the readings DawIt Tune and the wave editor's pitch use
    Tune.{h,cpp}           DawIt Tune's rules with no audio: cents, targets, tolerance, amount, glide, vibrato centre, starting points, made-up takes
    Tuner.{h,cpp}          the Tuner window's engine side: the input tap (`tuner::InputTap`, a FIFO from the audio callback) and `tuner::Listener` (the tracker with a long window, median of five, 2 s hold, level and silence, beats of two close notes)
    Psola.{h,cpp}          the TD-PSOLA shifter: moves a voice's pitch and keeps its formants, held back by a fixed latency
    SungVoice.h            a made-up sung "ah" for tests, the test sound and the example song's take
    SongContext.h          what a plugin implements to get the song's key and chord, or to keep its latency while bypassed
    BuiltInTune.{h,cpp}    DawIt Tune: the tracker, correction and shifter on the audio thread, latency, the frames and the take log
    BuiltInReverb.{h,cpp}  DawIt Reverb: six types, early reflections, feedback delay networks and spring loops,
                           response and decay measurement, separate kind auditions, starting points
    ReverbKinds.{h,cpp}    reverb kinds: text, showcases, track-name matching, advice and derived acoustic numbers
    TrackNameWords.h      shared track-name word matching for routing and reverb advice
    BuiltInDelay.{h,cpp}   DawIt Delay: tempo-synced or free echoes with filtered feedback, ping-pong, starting points
    BuiltInSaturation.{h,cpp}  DawIt Saturation: the tape, tube and clip curves at 4x oversampling, the harmonics of a
                           sine, what it adds, match level, starting points
    BuiltInDistortion.{h,cpp}  DawIt Distortion: tight, the overdrive, distortion, fuzz, rectify and fold curves at 4x
                           oversampling, tone and speaker, the harmonics of a note, what it adds, match level, starting points
    BuiltInSubtractive.{h,cpp}  DawIt Subtractive: oscillators, filter, envelopes, LFO, voices, explained starting sounds, the numbers
                           its pictures show (harmonics, filter response, envelope levels)
    BuiltInAdditive.{h,cpp}  DawIt Additive: 32 harmonics with brightness, even, stretch and phases, swell and fade, the amp
                           envelope, LFO, voices, starting points, and the numbers its pictures show
    BuiltInFm.{h,cpp}      DawIt FM: four operators with ratios, levels and envelopes, eight algorithms, feedback, the LFO,
                           voices at twice the rate, starting points, Bessel sidebands and the measured spectrum
    BuiltInDrums.{h,cpp}   DawIt Drums: nine drums from sines, the 808's squares and noise, the General MIDI drum map, the
                           choke, kits, and the voice that plays a hit for the instrument, the pictures and the tests
    BuiltInPhysical.{h,cpp} DawIt Physical: a string or tube as a delay line with its damping and decay, the pluck, strike,
                           bow and reed, the voice for the instrument, the pictures and the tests, starting points
    EffectTools.h          what the reverb, delay and saturation share: the level history, the test sound (a clap or a note), the dry/wet mix
    BuiltInFormat.{h,cpp}  the "DawIt" plugin format that puts the built-in effects in the chain
    ExampleSong.{h,cpp}    the example song for screenshots: A minor pop song with sections, chords, lyrics, buses and vocals
    GainStaging.{h,cpp}    gain staging: the level zones, the hints for a chain (also for a bus), the guide's texts
    Routing.{h,cpp}        the tree of tracks and buses, and the sends to returns: render order, visible rows, solo, where a dragged track lands
    RoutingGuide.{h,cpp}   the Buses, sends and routing guide's topics, the sums, send levels, families and the advice on your song (including the order of effects)
    SignalFlow.{h,cpp}     the signal flow of the whole song as plain data: boxes, lines, levels, the selected path and the layout (read-only)
    SoundGuide.{h,cpp}     what is sound: the guide's texts, the tone shapes and harmonics, pitch, what a moment of a clip is,
                           the envelope of a note measured on a clip, the mono check (overall and in five bands), left minus right
                           measured on a clip, the made-up example clip and a typical mix's parts; the engine's speaker mode (stereo, mono,
                           left minus right) is in `Engine`
    ComputerKeyboard.{h,cpp}  playing notes with the letter keys: the letter-to-note map, octave, velocity steps, held keys and the keys the scale filter silences (no UI)
    HitsToMidi.{h,cpp}     hits to notes through the tempo map (feel or grid snap), the new DawIt Drums track, the Listen render, the example drum loop
    DrumTriggerGuide.{h,cpp}  How drum triggers work: the topics, texts, tooltips, the numbers and the *Your clip* rules
    MidiGuide.{h,cpp}      what is MIDI: the guide's texts, messages in bytes and words, a clip as messages, its MIDI file part by part
    MusicalKey.h           song key: tonic + scale, intervals, in-key check
    MidiExport.{h,cpp}     clips and tracks → Standard MIDI Files
    Bounce.{h,cpp}         bounce to WAV: the range, offline rendering to a file or memory through the engine (the bounce focus: the mix, tracks heard as soloed, one chain), the tail, bounce in place and a clip written to WAV
    PdfWriter.{h,cpp}      PDF files of pages drawn with juce::Graphics, one page at a time, each a lossless picture (PNG predictors), with bookmarks and links
    GuideBooklet.{h,cpp}   a guide's or a set of guides' booklet: covers with contents, a page per topic, paper sizes and paper colours
    LatencyMeasure.{h,cpp} measuring latency: the click signal, the pure round-trip finder (`daw::latency`)
    RecordingGuide.{h,cpp} How recording works: topics and texts, the buffer, round-trip, distance and file-size formulas, picture data and the *Your session* analysis (`daw::recordingguide`)
    TuningGuide.{h,cpp}    How to tune a guitar (`gen::tuning`): the tunings, readings (note, cents, string, words), zones, cents/beat/harmonic formulas, topic texts, picture data and the *Your tuning* advice
    ProjectFile.{h,cpp}    .dawit project files: JSON save and load
    Chord.h                chord (root + intervals): names, spelling, parsing; the chord track's ChordMap
    ChordEditing.h         copying, pasting, moving and repeating bars of the chord track; chords as text
    Section.h              song sections (intro, verse, chorus...): types, energy, tidying
    Tempo.h                a tempo written and typed with up to two decimals (96.84, 97,5)
    TempoMap.{h,cpp}       the song's tempo over time: one tempo or a tempo per bar, beats to seconds and back
    Lyrics.{h,cpp}         the lyrics track: typed text to timed syllables, spreading over sections, dragging and tapping,
                           splitting English words into syllables
    Stems.{h,cpp}          stem separation: model download, Demucs in parallel pieces, stem tracks (all, or one),
                           the karaoke version (stems mixed back without the vocals, the guide vocals)
    AudioToMidi.{h,cpp}    audio to MIDI: Basic Pitch on ONNX Runtime, note finding, the new MIDI track
    OnnxEnvironment        the one ONNX Runtime environment the models share
    YamlLite               a small reader for the part of YAML that DiffSinger voices use
    Voice, VoiceInstall    DiffSinger voices: reading and checking a voice folder (named missing files, English, vocoder), the Voices folder, adding a .zip, .oudep or folder
    Singer                 the interface that turns a phrase into audio, rendering phrases into one buffer, adding the vocal track
    DiffSinger             the real Singer: a voice's duration, pitch, acoustic and vocoder ONNX models, in OpenUtau's order
    ModelDownload.{h,cpp}  the models downloaded on first use (Demucs, Whisper): pinned URLs, sizes, SHA-256 checks
  analysis/
    MonoAudio.{h,cpp}      audio to mono at the rate an analyser listens at
    TempoFinder.{h,cpp}    detecting the tempo: onsets, how often they repeat, beats (Ellis' dynamic programming),
                           drift, the first downbeat, following a drifting recording with a tempo per bar
    HitFinder.{h,cpp}      drum hits in audio (`analysis::hits`): candidates from the rise in a 256-sample FFT, sharp attacks, picking by sensitivity and gap, drum by band, velocity curves, hand edits
    ChordFinder.{h,cpp}    detecting chords: the notes per bar without the drums, chord templates, the bass, the key,
                           a Viterbi pass over the bars
    StructureFinder.{h,cpp} detecting the song structure: each bar's notes, sound, loudness and drums, where parts
                           start (novelty peaks, 4-bar phrases), which parts are the same music, naming them
    OnsetFinder.{h,cpp}    where notes and syllables begin in mono audio (spectral flux), used to snap lyrics to the singing
    LyricsFinder.{h,cpp}   detecting lyrics (English only): Whisper through whisper.cpp, finding the singing,
                           cleaning up the words, lines, verses and syllables on the song's beats
  generate/
    GenerationRequest.h    part, style, bars, key, tempo, chord track, harmony context
    Harmony.h              roman numerals, functions and usual next chords for the chord menu
    ChordGuide             beginner explanations of chords, moves and well-known progressions
    ChordDiagrams          how to play chords: piano inversions and voice leading, guitar shapes with fingers, barres and difficulty
    PlayAlong              playing along: what to show at a beat (chord now, next change, section and bar, chords coming, round the loop), capo shapes, tips
    SingAlong              singing along: the line at a beat, how much of each syllable is sung, the count-in, breaths, tips
    Rhyme.{h,cpp}          rhymes by sound: rhyme part and onset, the six kinds, syllable counts and stress, dictionary rhymes, hyphenating
    LyricsAi.{h,cpp}       the lyrics window's AI: the context of a range, prompts and schemas, parsing and checking replies, labels, applying, canned replies for screenshots
    LyricsGuide            how lyrics are written: topics, pictures, rhyme schemes and a look at the song's own lyrics, how a computer sings, getting a voice
    TuneAnalysis           DawIt Tune's take log turned into notes, errors per section and advice ("the verse sits 18 cents flat")
    TuneGuideText          the ten pages of What is pitch correction?: names, summaries and texts
    SingingGuide           singing techniques: topics, voice types, the range check on the song's melody, example data for the pictures
    Pronunciation          how an English word sounds: the voice's dictionary, the built-in CMU Pronouncing Dictionary, a guess from the spelling
    SingPairing            Sing this: the top-note melody, which syllable goes on which note (As timed, One syllable per note) and the advice
    SingScore              Sing this: phrases of sounds with frame lengths, breaths and the pitch of every frame
    SingMatch              Sing this: matching the notes to the words (a note on each syllable's beat, or merging the shortest notes and splitting the longest)
    ChordSheet             what a key's chord sheet holds, spelled for the key, and the capo that makes it easier on guitar
    StructureGuide         beginner explanations of song structure: sections, typical structures per style of music
    RhythmGuide            beginner explanations of rhythm: note values, counting, tempo; naming note lengths
    Notation               MIDI as sheet music: spelling, staff steps and ledger lines, auto clef, key-signature positions, one voice, the sixteenth/triplet grid, splitting and ties, rests, beams, stems, chord and section marks, line breaks (`gen::notation::build`)
    ReadingGuide           the guide How to read music (`gen::readmusic`): topics, texts, example scores and the *Your part* report
    TheoryGuides           every guide in one list for the Theory Guides window: areas, summaries, topics, the path for
                           beginners and the search
    PromptBuilder          prompts, random ideas per take, and the JSON schema for each part
    Takes                  Variation → temperature and ideas; scoring a take
    OllamaClient           HTTP client for Ollama (streaming chat and raw constrained requests)
    ChatMusician           constrained melody and chord requests, validation, chunks and voicings
    NoteRealizer           model reply → notes (chords, scale degrees, drums, quantise, loop)
    ChordStyles            playing chords in an instrument's style: patterns of chord roles, voicings, guitar shapes
    ChordStyles.json       the built-in styles, with the parts and styles of music they suit
    SongTemplates          fourteen rule-based whole-song plans and the engine build that makes their tracks, clips, buses, effects and sends
    StylesGuide            Styles of music: eighteen topics, examples and advice about the open song
  ui/
    MainComponent          layout, keyboard shortcuts, plugin list and audio settings windows
    TransportBar           transport, tempo, position, master
    ArrangementView        ruler, chord track, scrolling/zoom, track headers + lanes
    SectionLane            Sections row: draw, resize, type, rename, loop and delete sections
    ChordLane              chord track row: draws chords, chord picker menu, Play chords as...
    LyricsLane             Lyrics row: syllables and lines, the open view, dragging, tapping along
    LyricsEditWindow       the non-modal Lyrics window: the text box and the Write with AI panel (brief, actions, results, rhymes)
    LyricsExplainer        the guide to writing lyrics: topics, pictures (words, beat grids, melodies)
    RecordingExplainer     the How recording works guide: nine pictures and the live *Your session* topic
    TuningExplainer        the How to tune a guitar guide: ten pictures (clickable strings in the tunings topic) and the live *Your tuning* topic
    RecordingActions.{h,cpp} recording UI shared by header, mixer and menus: the input chip and menu, the Monitor warning, the input meter, the after-stop messages, `makeSnapshot`
    AudioSettingsPanel     Audio / MIDI Settings: the device selector with an input meter, the latency line and Measure latency
    SingingExplainer       the singing techniques guide: voice row (range), 13 pictures (keyboards, pitch curves, piano-roll notes), previews, your melody against your range
    ChordExplainer         the chord progression guide window, opened from the chord menu or the Chords label
    ChordDiagramViews      keyboard and guitar chord box diagrams, for one chord or a progression, on screen or printed
    TempoLane              the Tempo row: the tempo bar by bar while it follows a recording, making it steady
    ChordPlayAlong         the play-along window: the chord now and the next one large on piano and guitar, the countdown, the chords coming
    SingWindow             the Sing window: voice, melody, words, pitch, quality, the picture of notes with syllables and sounds, advice, how to get a voice
    SingActions            Sing this: opens the window, renders in the background behind a progress window, adds the vocal track
    LyricsSingAlong        the sing-along window: the line to sing large, coloured as it's sung, the next line, the count-in
    GuidePrint             Export PDF... in the guide windows and the Theory Guides window: the dialog and the export job
    ChordSheetExport       the chord sheets' A4 pages: a key's chords (File > Export Chord Sheet) and the song's (the chord track's PDF...)
    StructureExplainer     the song structure guide window, with the structure strip and energy curve
    StylesExplainer        the Styles of music guide window, groove playback and PDF source
    StylePictures          the shared tempo, arrangement, drum-grid and routing pictures for templates and their guide
    NewFromStyleWindow     the File > New Project from Style window: style, key, tempo, notes, Create and Shuffle
    RhythmExplainer        the note lengths guide window: example rhythms with symbols, counts and playback
    TheoryGuidesWindow     the Theory Guides window (areas, cards with topics, search), and opening a built-in effect's or
                           synth's guide on the song's one or on a stand-in on no track
    TrackActions.{h,cpp}   the track menu, Create bus, Output, Ungroup
    RoutingExplainer       the Buses, sends and routing guide window: the routing tree drawn live, with sends, the send diagrams and the signal flow example
    SignalFlowView         the Signal Flow window and the painter of the flow diagram (also used by the guide and its PDF)
    ChainPictures          the small EQ, Compressor and Gate curves shared by the device bar and the Signal Flow window
    TrackHeader            name, M/S, volume, pan, meter, plugin chain menu, bus fold arrow, drag to move
    TrackLanes             clip drawing and editing, file drag-and-drop
    PianoRoll              MIDI note editor, with the header switch to the score and the Velocity mode (V) that drags selected notes' velocity
    ScoreView              the score view: a scrolling score with the clef pinned, playhead, lit notes, tooltips, clicks and the Clef menu
    ScorePainter           draws a `notation::Score` with Bravura: shared by the score view, the guide's pictures and the PDF
    ReadingExplainer       the guide How to read music: pictures from `ScorePainter`, Play, PDF
    SheetMusicExport       the sheet music PDF: part, dialog, pages (A4 or Letter, 300 dpi) and File > Export Sheet Music
    PitchRows.h            piano roll rows: every pitch, or only the song key's
    GeneratePanel          AI generate panel inside the piano roll, including staged ChatMusician takes and preview
    InspectorPanel         the inspector: the selected track's name, M/S/Output, effects, sends (SendKnob, wide look), pan and volume
                           with a meter, on the left at full height; InspectorEdge is its drag edge (200 to 360 px)
    SendKnob               one send row (dial, return name, PRE, level) shared by the mixer strip (compact look) and the inspector (wide)
    MixerView              mixer: a channel strip per track (mute, solo, pan, fader, meter) and the master
    WaveEditor             audio clip editor: waveform at the clip's gain, zoom down to samples, what is under the mouse,
                           Reverse and Gain
    ComputerKeyboardTyping the one place that decides what a plain letter does while typing is on (before TransportShortcuts); key-up checks, 30 Hz watchdog, `typing.*` settings
    GuideButton            the guide icon (an open book with five rising bars, `guideicon::draw`, `makeDrawable`, `addItem` for menus) and `GuideButton`, the button that opens a guide: icon alone for "?", else icon then label
    ComputerKeyboardStrip  the 64 px strip at the bottom of the window and `keyboardpicture::paint`, the keyboard painter shared with the guide
    MidiExplainer          the "What is MIDI?" guide: a picture per topic with the piano roll's clip as example, a keyboard to click
    SoundExplainer         the "What is sound?" guide: a live picture per topic with the clip as example, the tone,
                           and left minus right against Remove vocals
    DeviceBar              selected track's instrument + effects, fader and master, with meters, the built-in effects' curves,
                           and the gain staging: coloured arrows, loudness badges, hints
    DevScreenshots         --screenshots: sets up the views a shot list asks for and saves each as a PNG
    GainStagingExplainer   the gain staging guide: the chain drawn live, the topics
    BuiltInSynthEditor     DawIt Synth window: pick the track's built-in sound
    EqEditor               DawIt EQ window: the graph over the spectrum, draggable bands, a column per band
    EqExplainer            the EQ's explanations, live numbers for the settings, the Q picture, the instruments, Find a bad frequency
    EqGuide                What is EQ?: its pages and pictures, drawn from the EQ's filters
    CompressorEditor       DawIt Compressor window: the curve, the last 4 seconds, the meters, the knobs
    CompressorExplainer    the compressor's explanations, typical settings, timing and curve pictures, the step-by-step setup
    CompressorGuide        What is compression?: its pages, and made-up vocal and drum levels run through the compressor
    EffectWindow           what the built-in effect and synth windows share: the frame, knob groups (in rows), explainer, history and guide window
    ReverbEditor           DawIt Reverb window: Type menu, response with draggable dots, kind drawing, last 4 seconds
    ReverbGuide            reverb explanations and live numbers, response and kind drawings, sixteen What is reverb? topics
    DelayEditor            DawIt Delay window: the echoes with draggable dots, the wall, the last 4 seconds
    DelayGuide             the delay's explanations and live numbers, the echo and wall drawings, What is delay?
    SaturationEditor       DawIt Saturation window: the curve to drag, the harmonics, the last 4 seconds
    SaturationGuide        the saturation's explanations and live numbers, the curve and harmonics drawings, What is saturation?
    GateEditor             DawIt Gate window: the curve with draggable dots, the last 4 seconds, the key filters
    GateGuide              the gate's explanations and live numbers, the curve, history and key drawings, What is a gate?
    StereoEditor           DawIt Stereo window: mid/side spectrum, goniometers, correlations and controls
    StereoGuide            stereo-width topics, example pictures and the playing track's advice
    StereoScope            shared goniometer and correlation-bar painters
    TunePictures           DawIt Tune's tuner, trace and zone colours, shared by its window, guide and chip
    TuneEditor             DawIt Tune window: mode, target, speed, amount, tolerance, keep vibrato, the tuner and the trace
    TunerWindow            the Tuner (View > Tuner..., Cmd+7): tuning, input, reference and mute controls, the note and needle, string buttons that play their note, keys 1 to 6, the level bar and the session's last readings
    TuneGuide              What is pitch correction?: the pictures and the live Your track page
    DistortionEditor       DawIt Distortion window: the curve to drag, the harmonics, the filters, the last 4 seconds
    DistortionGuide        the distortion's explanations and live numbers, the curve, harmonics and filter drawings, What is distortion?
    SubtractiveEditor      DawIt Subtractive window: the harmonics and filter to drag, the wave, the envelopes, the last 4 seconds
    SubtractiveGuide       the synth's explanations and live numbers, its drawings, What is subtractive synthesis?
    AdditiveEditor         DawIt Additive window: the harmonics to draw, the wave, a note over time, the last 4 seconds
    AdditiveGuide          the additive synth's explanations and live numbers, its drawings, What is additive synthesis?
    FmEditor               DawIt FM window: the operators to click and drag, the spectrum, the wave, the last 4 seconds
    FmGuide                the FM synth's explanations and live numbers, its drawings, What is FM synthesis?
    DrumsEditor            DawIt Drums window: the pads, a hit over time, its frequencies, the last 4 seconds
    DrumsGuide             the drum synth's explanations and live numbers, its drawings, What is drum synthesis?
    PhysicalEditor         DawIt Physical window: the model to drag along, the harmonics, how long each rings, the last 4 seconds
    PhysicalGuide          the physical-modelling synth's explanations and live numbers, its drawings, What is physical modelling?
    PluginActions          shared add/open/remove/move plugin logic
    MidiExportActions      export dialogs and dragging clips out as .mid files
    BounceDialog           File > Bounce to WAV: what (the mix or the selected tracks), range, format and tail, the save dialog and the progress window
    BounceActions          Bounce in place with its progress window, Export clip as WAV file... and dragging an audio clip out as a WAV
    StemActions            Split into stems and Remove vocals: the karaoke dialog, download question, progress window,
                           adding the stems or the karaoke version
    AudioToMidiActions     Convert to MIDI: options dialog, progress window, adding the MIDI track
    HitsToMidiActions      Hits to MIDI: the progress window finding candidates, then opening the window
    HitsToMidiWindow       the Hits to MIDI window: waveform, markers by drum, controls, Listen, Make MIDI track
    DrumTriggerExplainer   the How drum triggers work guide: pictures, clicks that play, *Your clip* from the window or a drum clip
    TempoActions           Detect tempo: the tempo found, half or double, setting it, lining the clip up with the bars
    ChordActions           Detect chords: the chords and key found, the tempo check and its advice, the chord track
    StructureActions       Detect song structure: the parts found and why, the tempo check, the Sections row
    LyricsActions          Detect lyrics (English only): options dialog, downloads, vocals, Whisper, the lyrics track,
                           the muted vocal stem
    ChordStyleActions      Play chords as...: instrument, style and bars dialog
    ProjectManager         New/Open/Save, recent projects, unsaved-changes prompt
    PluginWindows.h        plugin editor windows (and the DawIt Synth's)
cmake/
  DawItBuiltIns.cmake      dawit_add_builtin: one call per built-in adds its sources to DawIt and DawItTests, writes the
                           generated DawItBuiltInList.h, and (with DAWIT_BUILD_PLUGINS) makes its VST3 and AU target
src/plugin/
  PluginEntry.cpp.in       template of each plugin's createPluginFilter, filled in per built-in into build/<cfg>/plugin/
src/engine/OwnPluginCopies.h  plugins::isDawItPluginCopy: DawIt's menus hide its own VST3 and AU copies
src/ui/GuideSource.h       guideprint::Source and the Export PDF hook (empty in a plugin), apart from Engine.h
build.sh                   --test, --plugins (also build the plugins), --install-plugins (also copy them to the DAWs' folders), --clean
tests/EngineTests.cpp      headless engine tests
tools/ideas.py             the ideas list for agents: index, show, search, add, link-plan, set-tag and check, so they
                           needn't grep FEATURE_IDEAS.html
tools/agent-usage.py       what the project's subagents spent their tokens on, from Claude Code's transcripts
                           (--since, --type, --runs)
```
