#include "TheoryGuides.h"
#include "InstrumentsGuide.h"
#include "KeyFindingGuide.h"
#include "LyricsGuide.h"
#include "ReadingGuide.h"
#include "RhythmGuide.h"
#include "SingingGuide.h"
#include "StylesGuide.h"
#include "ChordGuide.h"
#include "PlayAlong.h"
#include "StructureGuide.h"
#include "TuningGuide.h"
#include "engine/GainStaging.h"
#include "engine/MidiGuide.h"
#include "engine/HandoverGuide.h"
#include "engine/RecordingGuide.h"
#include "engine/DrumTriggerGuide.h"
#include "engine/RoutingGuide.h"
#include "engine/SoundGuide.h"

namespace daw::gen::guides
{
juce::String getAreaName (Area a)
{
    switch (a)
    {
        case Area::sound:       return "Sound";
        case Area::midi:        return "MIDI";
        case Area::rhythm:      return "Rhythm";
        case Area::readingMusic: return "Reading music";
        case Area::harmony:     return "Chords and keys";
        case Area::songwriting: return "Writing songs";
        case Area::singing:     return "Singing";
        case Area::mixing:      return "Mixing";
        case Area::effects:     return "Effects";
        case Area::synthesis:   return "Synthesis";
    }

    return {};
}

juce::String getAreaSummary (Area a)
{
    switch (a)
    {
        case Area::sound:       return "What sound is, and how a computer stores it";
        case Area::midi:        return "Notes as numbers, and how they reach an instrument";
        case Area::rhythm:      return "Beats, bars, note lengths and tempo";
        case Area::readingMusic: return "Notes on the staff, clefs, keys and the score";
        case Area::harmony:     return "Keys, chords, progressions, and how to play them";
        case Area::songwriting: return "How songs are built, and how lyrics are written";
        case Area::singing:     return "Your voice: how it works, singing in tune, and recording it";
        case Area::mixing:      return "Levels, EQ, compression, gates, stereo width and buses: making the parts fit";
        case Area::effects:     return "Space, echoes, colour and rhythm: reverb, delay, slicing, step filters, saturation, distortion";
        case Area::synthesis:   return "Making sounds from scratch, five ways";
    }

    return {};
}

namespace
{
    template <typename Topics, typename Name>
    juce::StringArray getNames (const Topics& topics, Name name)
    {
        juce::StringArray names;

        for (auto t : topics)
            names.add (name (t));

        return names;
    }

    juce::StringArray words (const char* list)
    {
        return juce::StringArray::fromTokens (list, " ", {});
    }
} // namespace

const juce::String sourcesTopicName { "Sources" };

juce::String getKey (Guide guide)
{
    using G = Guide;

    switch (guide)
    {
        case G::readMusic:    return "readMusic";
        case G::keyFinding:   return "keyFinding";
        case G::sound:        return "sound";
        case G::midi:         return "midi";
        case G::rhythm:       return "rhythm";
        case G::chords:       return "chords";
        case G::chordShapes:  return "chordShapes";
        case G::structure:    return "structure";
        case G::styles:       return "styles";
        case G::lyrics:       return "lyrics";
        case G::singing:      return "singing";
        case G::tune:         return "tune";
        case G::harmoniser:   return "harmoniser";
        case G::gainStaging:  return "gainStaging";
        case G::eq:           return "eq";
        case G::compressor:   return "compressor";
        case G::gate:         return "gate";
        case G::stereo:       return "stereo";
        case G::routing:      return "routing";
        case G::loudness:     return "loudness";
        case G::reverb:       return "reverb";
        case G::delay:        return "delay";
        case G::slicer:       return "slicer";
        case G::arpeggiator:  return "arpeggiator";
        case G::sampler:      return "sampler";
        case G::handover:     return "handover";
        case G::instruments:  return "instruments";
        case G::stepFilter:   return "stepFilter";
        case G::spectralFreeze: return "spectralFreeze";
        case G::saturation:   return "saturation";
        case G::distortion:   return "distortion";
        case G::subtractive:  return "subtractive";
        case G::additive:     return "additive";
        case G::fm:           return "fm";
        case G::drums:        return "drums";
        case G::physical:     return "physical";
        case G::recording:    return "recording";
        case G::tuning:       return "tuning";
        case G::triggers:     return "triggers";
    }

    return {};
}

std::optional<Guide> findGuide (const juce::String& key)
{
    for (auto g : allGuides)
        if (getKey (g).equalsIgnoreCase (key))
            return g;

    return std::nullopt;
}

Entry getEntry (Guide g)
{
    switch (g)
    {
        case Guide::sound:
            return { g, Area::sound, "What is sound?",
                     "Air moving, waves, loudness in decibels, pitch in hertz, timbre and envelopes, how a sound becomes numbers, phase, and mono.",
                     "The wave editor (What is sound?)", {},
                     words ("wave hertz hz pitch decibel db loudness harmonics overtone sample rate bit depth phase karaoke vocals envelope adsr attack decay sustain release mono stereo correlation"),
                     getNames (soundguide::allTopics, [] (auto t) { return soundguide::getTopicName (t); }), {} };

        case Guide::midi:
            return { g, Area::midi, "What is MIDI?",
                     "What MIDI is and isn't, its history, the messages, note numbers, velocity, pedals and pitch bend, controllers, files, and turning a melody backwards, upside down or into new notes.",
                     "The piano roll (What is MIDI?), the computer keyboard strip, the Transform menu", {},
                     words ("note number velocity controller cc channel keyboard message mid file sustain pitch bend "
                            "sustain pedal damper mod wheel modulation expression volume controller lane aftertouch pressure "
                            "computer keyboard musical typing typing qwerty play without a keyboard "
                            "retrograde inversion invert reverse backwards upside down mirror random randomise transform variation"),
                     getNames (midiguide::allTopics, [] (auto t) { return midiguide::getTopicName (t); }), {} };

        case Guide::rhythm:
            return { g, Area::rhythm, "Note lengths, beats and bars",
                     "Bars and beats, time signatures from 3/4 to 7/8, whole to sixteenth notes, rests, dotted notes and triplets, counting, tempo, quantising and feel, and your song's metre.",
                     "The ruler's and the Tempo row's menus, the time signature next to the tempo, the ? in the piano roll, and the Quantise notes window", {},
                     words ("beat bar measure tempo bpm quarter eighth sixteenth crotchet quaver minim semibreve rest dotted triplet "
                            "counting time signature snap waltz metre meter compound simple odd shuffle common cut alla breve 3/4 6/8 12/8 5/4 7/8 "
                            "quantise quantize quantising quantizing quantisation quantization strength groove feel tight loose timing humanise humanize laid-back"),
                     getNames (rhythm::allTopics, [] (auto t) { return rhythm::getTopicName (t); }), {} };

        case Guide::chords:
            return { g, Area::harmony, "Chord progressions",
                     "How chords work in a key, the circle of fifths, well-known progressions, and your own chord track.",
                     "The Chords label and the chord menu", {},
                     words ("chord progression key major minor scale circle fifths numeral roman diatonic relative tonic "
                            "dominant subdominant"),
                     { "How chords work", "The circle of fifths", "Your chord track", "Playing along", "Well-known progressions" }, {} };

        case Guide::chordShapes:
            return { g, Area::harmony, "Piano and guitar chords",
                     "How to play a chord on piano and on guitar, with its notes, from the chord at the playhead.",
                     "A chord's menu (How to play...)", {},
                     words ("chord piano guitar shape fingering fret finger diagram voicing keyboard play notes"),
                     {}, {} };

        case Guide::structure:
            return { g, Area::songwriting, "Song structure",
                     "What each section is for, the typical structures of different styles, and your song's own sections.",
                     "The Sections label and the section menu", {},
                     words ("section intro verse chorus pre-chorus bridge drop breakdown outro form arrangement energy"),
                     { "How songs are built", "The sections", "Your song's structure", "Typical structures" }, {} };

        case Guide::styles:
            return { g, Area::songwriting, "Styles of music",
                     "Fourteen styles from ballad to drum & bass: their tempo, groove, chords, sounds and structure, a song to start from in each, and how your song compares.",
                     "File > New Project from Style... and the Song structure guide", {},
                     words ("style genre template pop ballad dance rock hip-hop hiphop boom bap trap house techno trance drum bass dnb jungle lofi lo-fi chillhop blues shuffle reggaeton dembow afrobeats afrobeat film score cinematic groove four-on-the-floor backbeat half-time 808 909 new project"),
                     getNames (stylesguide::allTopics, [] (auto t) { return stylesguide::getTopicName (t); }), {} };

        case Guide::lyrics:
            return { g, Area::songwriting, "How lyrics are written",
                     "What each part says, lines, rhyme, finding rhymes, writing with AI, stress on the beat, words on a melody, the hook, how a computer sings, getting a voice, and your lyrics.",
                     "The ? in the Lyrics row and in the Sing window, and the Lyrics window's Write with AI panel", {},
                     words ("words rhyme syllable stress hook title verse chorus prosody singing sing songwriting voice vocal synthesis synthesiser diffsinger phoneme voicebank vocoder "
                            "ai ollama rewrite rhymes rhyming dictionary near rhyme slant family assonance consonance"),
                     getNames (lyricsguide::allTopics, [] (auto t) { return lyricsguide::getTopicName (t); }), {} };

        case Guide::singing:
            return { g, Area::singing, "Singing techniques",
                     "How the voice works, breathing, warming up, voice types and your range, measuring your range from a recording, registers, singing in tune, vowels, vibrato, belting, harmonies, "
                     "looking after your voice, the microphone, and your melody against your range.",
                     "Sing along's Tips..., the lyrics guide's Singing along, a vocal track's menu, and an audio clip's Measure singing range...", {},
                     words ("sing singer singing voice vocal vocals vox range tenor soprano alto contralto mezzo baritone bass breath breathing diaphragm warm-up warmup trill "
                            "pitch tune cents flat sharp vibrato belt belting falsetto register chest head mix break passaggio hoarse hoarseness microphone mic pop filter "
                            "plosive proximity harmony double key suits measure measuring measured recording held fry"),
                     getNames (singing::allTopics, [] (auto t) { return singing::getTopicName (t); }), {} };

        case Guide::tune:
            return { g, Area::singing, "What is pitch correction?",
                     "Cents and what ears notice, how a tuner finds the pitch, target notes, speed and amount, vibrato, formants, hard tune, using it well, and how in tune your track is.",
                     "DawIt Tune's window and the singing guide's Singing in tune", "DawIt Tune",
                     words ("tune tuner tuning autotune auto-tune pitch correction intonation flat sharp cents retune speed humanize vibrato formant chipmunk psola melodyne t-pain cher believe robot hard scale snap"),
                     {}, {} };

        case Guide::harmoniser:
            return { g, Area::singing, "What is a harmoniser?",
                     "Intervals, fixed and in-the-key harmony, which intervals sound how, doubling, formants, what a harmoniser can't hear, and what the voices made of your track.",
                     "DawIt Harmoniser's window and the singing guide's Harmonies and doubles", "DawIt Harmoniser",
                     words ("harmoniser harmonizer harmony harmonies backing vocals third fifth sixth octave interval diatonic scale steps fixed doubling double tracking adt detune micro pitch shift formant pitch shifter voices"),
                     {}, {} };

        case Guide::gainStaging:
            return { g, Area::mixing, "Gain staging",
                     "How loud the sound is at each step of a track's chain, why it matters, and how to fix a level.",
                     "The device bar (Gain staging?)", {},
                     words ("gain level headroom clipping clip dbfs fader peak meter loud quiet volume"),
                     getNames (gainstaging::allTopics, [] (auto t) { return gainstaging::getTopicName (t); }), {} };

        case Guide::eq:
            return { g, Area::mixing, "What is EQ?",
                     "Turning frequencies up and down: the ranges, band types, Q and slopes, where instruments sit, masking and common problems.",
                     "DawIt EQ's window (What is EQ?)", "DawIt EQ",
                     words ("eq equaliser equalizer equalisation frequency cut boost bell shelf filter high-pass low-pass mud "
                            "muddy harsh boomy boxy thin dull bright sibilant vocal voice"),
                     {}, {} };

        case Guide::compressor:
            return { g, Area::mixing, "What is compression?",
                     "Turning down the loud parts: threshold, ratio, knee, attack, release, makeup, parallel compression and how much to use.",
                     "DawIt Compressor's window (What is compression?)", "DawIt Compressor",
                     words ("compressor compression dynamics punch glue squash even level limiting limiter pumping vocal voice "
                            "drums bass"),
                     {}, {} };

        case Guide::gate:
            return { g, Area::mixing, "What is a gate?",
                     "Closing the gaps between notes: threshold and hysteresis, range and ratio, attack, hold and release, the key filter, and gated reverb.",
                     "DawIt Gate's window (What is a gate?)", "DawIt Gate",
                     words ("gate gating noise-gate noise hiss hum bleed spill expander expansion downward chatter hysteresis hold key "
                            "sidechain kick snare toms drums gated reverb"),
                     {}, {} };

        case Guide::stereo:
            return { g, Area::mixing, "What is stereo width?",
                     "Mid and side, the goniometer and correlation meter, mono and phase, why the low end stays in the middle, mono below a frequency, width, and your track.",
                     "DawIt Stereo's window (What is stereo width?)", "DawIt Stereo",
                     words ("stereo width wide narrow mono mono-compatible mono-compatibility mono-maker bass-mono mid side mid-side M/S correlation phase "
                            "goniometer vectorscope Lissajous imager imaging widener spread low-end sub bass kick vinyl club crossover Linkwitz-Riley"),
                     {}, {} };

        case Guide::routing:
            return { g, Area::mixing, "Buses, sends and routing",
                     "Where each track's sound goes: buses that group tracks, sends to a shared reverb or delay, levels that add up, glue compression, solo and mute, the order of effects, and your song's routing.",
                     "A track's right-click menu (About buses, sends and routing...), the ? in a bus's or a return's header, the mixer's Output menu, a send knob's right-click menu, the ? on the master strip (the mix bus), the inspector's ? and right-click menu, and How sound flows? in the Signal Flow window", {},
                     words ("bus buses group grouping groups folder submix sub-mix stem routing route output master signal flow glue "
                            "mix-bus drum-bus mix bus master bus stereo bus bus compressor glue the mix headroom "
                            "send sends aux aux-send return returns return-track fx-channel effect-return pre-fader post-fader prefader postfader "
                            "insert inserts wet dry shared-reverb reverb-bus depth distance "
                            "signal-flow signal-chain chain effect-order plugin-order order-of-effects flow map "
                            "bounce export render print freeze stems multitracks wav inspector channel strip channel-strip selected track selected-track"),
                     getNames (routingguide::allTopics, [] (auto t) { return routingguide::getTopicName (t); }), {} };

        case Guide::loudness:
            return { g, Area::mixing, "What is loudness? LUFS explained",
                     "Why peaks aren't loudness, LUFS and K-weighting, momentary, short-term and integrated loudness, loudness range, true peak, streaming targets and normalisation, the loudness war, and your song's numbers.",
                     "DawIt Meter's window (What is loudness?), File > Measure Loudness..., and the gain staging guide", "DawIt Meter",
                     words ("loudness LUFS LKFS LU loud quiet meter metering loudness-meter integrated short-term momentary gating LRA loudness-range true-peak dBTP "
                            "inter-sample intersample peak K-weighting BS.1770 R128 EBU ATSC A/85 normalisation normalization Spotify Apple YouTube Tidal Amazon "
                            "podcast broadcast streaming target mastering limiter loudness-war war crest PLR headroom"),
                     {}, {} };

        case Guide::reverb:
            return { g, Area::effects, "What is reverb?",
                     "How a room answers a sound: early reflections and the tail, decay, damping, pre-delay, the kinds of reverb (room, hall, chamber, plate, spring, digital) side by side, reverb in a mix, and your track.",
                     "DawIt Reverb's window (What is reverb?)", "DawIt Reverb",
                     words ("reverb room hall plate space decay pre-delay reflections ambience wet dry echo chamber spring EMT tank convolution gated shimmer reverse kinds types"),
                     {}, {} };

        case Guide::delay:
            return { g, Area::effects, "What is delay?",
                     "Echoes in time with the song: feedback, slapback and doubling, darker repeats, ping-pong, and delay in a mix.",
                     "DawIt Delay's window (What is delay?)", "DawIt Delay",
                     words ("delay echo feedback tempo sync ping-pong slapback repeats"),
                     {}, {} };

        case Guide::arpeggiator:
            return { g, Area::midi, "What is an arpeggiator?",
                     "Turning held chords into patterns: chord tones, note order, octaves, note length, accents, swing and syncopation, and writing it out as notes, and your track.",
                     "DawIt Arpeggiator's window (What is an arpeggiator?)", "DawIt Arpeggiator",
                     words ("arpeggiator arpeggio arpeggios arp arps broken-chord broken chord chord-trigger alberti pattern order up down latch hold gate swing shuffle "
                            "accent syncopation off-beat tresillo 3-3-2 octaves rate midi-effect write-out written-out convert"),
                     {}, {} };

        case Guide::slicer:
            return { g, Area::effects, "What is a slicer?",
                     "Cutting a track into steps on the beat: repeats and stutters, reverse, trance gates, clicks and smoothing, and your track.",
                     "DawIt Slicer's window (What is a slicer?)", "DawIt Slicer",
                     words ("slicer slice slices slicing stutter stutters repeat beat-repeat reverse backwards trance-gate gating chop chops chopping glitch roll rolls step steps pattern"),
                     {}, {} };

        case Guide::stepFilter:
            return { g, Area::effects, "What is a step filter?",
                     "Moving a filter in a rhythm: cutoff and resonance, low-, band- and high-pass, steps, glide, mix, and your track.",
                     "DawIt Step Filter's window (What is a step filter?)", "DawIt Step Filter",
                     words ("step-filter stepfilter filter filters filtering cutoff resonance low-pass lowpass high-pass highpass band-pass bandpass "
                            "sequencer filter-sequencer pattern steps glide acid wah sweep rhythmic tone"),
                     {}, {} };

        case Guide::spectralFreeze:
            return { g, Area::effects, "What is a spectral freeze?",
                     "Holding a moment of sound as a texture: spectra, frequency bins, time against frequency, why a freeze isn't a loop, and your track.",
                     "DawIt Spectral Freeze's window (What is a spectral freeze?)", "DawIt Spectral Freeze",
                     words ("spectral freeze spectral-freeze hold sustain infinite texture pad drone spectrum fft fourier bins frequency-bins resolution window hann phase-vocoder snapshot ambient"),
                     {}, {} };

        case Guide::saturation:
            return { g, Area::effects, "What is saturation?",
                     "Gently bending the wave adds harmonics: even and odd, soft and hard, tape, tubes and transistors, and warmth in a mix.",
                     "DawIt Saturation's window (What is saturation?)", "DawIt Saturation",
                     words ("saturation warmth warm tape tube valve harmonics drive analogue analog"),
                     {}, {} };

        case Guide::distortion:
            return { g, Area::effects, "What is distortion?",
                     "Bending the wave all the way: overdrive, distortion and fuzz, rectifying and folding, tone, and distortion in a mix.",
                     "DawIt Distortion's window (What is distortion?)", "DawIt Distortion",
                     words ("distortion overdrive fuzz clipping drive harmonics guitar amp"),
                     {}, {} };

        case Guide::subtractive:
            return { g, Area::synthesis, "What is subtractive synthesis?",
                     "Bright waves shaped by a filter: the oscillators, cutoff and resonance, envelopes, the LFO, and recipes.",
                     "DawIt Subtractive's window (What is subtractive synthesis?)", "DawIt Subtractive",
                     words ("synth synthesiser synthesizer oscillator filter cutoff resonance envelope adsr lfo saw square pad "
                            "bass lead"),
                     {}, {} };

        case Guide::sampler:
            return { g, Area::synthesis, "What is a sampler?",
                     "Real instruments from recordings: root notes, key zones, velocity layers, loops, SoundFonts and General MIDI.",
                     "DawIt Sampler's window (What is a sampler?), the Sampled sounds menu", "DawIt Sampler",
                     words ("sampler sample sampled soundfont sf2 sf3 sfz multisample zone velocity layer loop round robin general midi gm program bank drum kit musescore "
                            "realistic piano strings"),
                     {}, {} };

        case Guide::additive:
            return { g, Area::synthesis, "What is additive synthesis?",
                     "A sound built from sine waves, one per harmonic: the harmonic series, the classic waves, bells, and Hammond drawbars.",
                     "DawIt Additive's window (What is additive synthesis?)", "DawIt Additive",
                     words ("synth synthesiser synthesizer additive harmonics partials organ drawbar sine overtone spectrum"),
                     {}, {} };

        case Guide::fm:
            return { g, Area::synthesis, "What is FM synthesis?",
                     "One sine wobbling another's pitch: carriers and modulators, ratios, the index, algorithms, and bells and keys.",
                     "DawIt FM's window (What is FM synthesis?)", "DawIt FM",
                     words ("synth synthesiser synthesizer fm frequency modulation operator carrier modulator ratio index bell "
                            "electric piano dx7"),
                     {}, {} };

        case Guide::drums:
            return { g, Area::synthesis, "What is drum synthesis?",
                     "Every drum made from oscillators and noise, like a TR-808: kick, snare, hi-hats, clap, toms, and building a kit.",
                     "DawIt Drums' window (What is drum synthesis?)", "DawIt Drums",
                     words ("drums drum machine kick snare hi-hat hat cymbal clap tom cowbell 808 beat"),
                     {}, {} };

        case Guide::physical:
            return { g, Area::synthesis, "What is physical modelling?",
                     "A string or a tube modelled from physics, plucked, struck, bowed or blown.",
                     "DawIt Physical's window (What is physical modelling?)", "DawIt Physical",
                     words ("physical modelling modeling string pluck bow strike blow tube waveguide karplus-strong guitar "
                            "cello clarinet harp"),
                     {}, {} };

        case Guide::handover:
            return { g, Area::mixing, "Sending stems to an engineer",
                     "What stems and multitracks are, what a mixing or a mastering engineer wants, preparing and checking the files, headroom, what to send with them, revisions, and your song as files.",
                     "The ? in the Bounce to WAV window, a track's right-click menu (Sending stems to an engineer...), and the routing guide's Stems and multitracks", {},
                     words ("stems stem multitracks multitrack handover hand-over deliver delivery send sending export bounce files folder mixing-engineer mastering-engineer engineer mixer premaster pre-master "
                            "stem-mastering stereo-mix rough-mix reference null null-test test polarity dither normalise normalize headroom limiter 24-bit 32-bit float sample-rate wav revisions revision notes tempo key naming"),
                     getNames (handoverguide::allTopics, [] (auto t) { return handoverguide::getTopicName (t); }), {} };

        case Guide::recording:
            return { g, Area::sound, "How recording works",
                     "Recording a voice or instrument: the audio interface, input levels, latency, monitoring and takes, with your own device as the example.",
                     "An audio track's right-click menu (About recording audio...), a take's menu, the ? in Audio / MIDI Settings, and the Monitor warning", {},
                     words ("record recording audio interface microphone mic input preamp gain phantom power 48v condenser dynamic hi-z instrument line level latency buffer "
                            "delay monitoring monitor headphones feedback howl take takes comp comping loop clipping clip level xlr usb"),
                     getNames (recordingguide::allTopics, [] (auto t) { return recordingguide::getTopicName (t); }), {} };

        case Guide::tuning:
            return { g, Area::sound, "How to tune a guitar",
                     "Tuning a guitar, bass or ukulele: cents and beats, the tunings, the reference pitch, tuning by ear and with harmonics, and whether your tuning suits the song.",
                     "The Tuner window (View > Tuner..., Cmd+7)", {},
                     words ("tuner tuning tune guitar bass ukulele uke string strings standard drop dadgad half step down eb flat sharp cents hz 440 442 432 415 "
                            "reference concert pitch harmonic harmonics fret fifth beats peg capo intonation stretch"),
                     getNames (tuning::allTopics, [] (auto t) { return tuning::getTopicName (t); }), {} };

        case Guide::triggers:
            return { g, Area::midi, "How drum triggers work",
                     "Turning drum hits in a recording into MIDI notes: transients, sensitivity and gap, velocity, telling kick, snare and hi-hat apart, false hits, the grid, and replacing or layering with the phase lined up.",
                     "An audio clip's right-click menu (Hits to MIDI...), the ? in the Hits to MIDI window", {},
                     words ("trigger triggers drum replacement replace replacer layer layering sample samples augment hits hit onset onsets transient transients threshold "
                            "sensitivity retrigger mask gap bleed spill ghost flam roll kick snare hihat hi-hat velocity quantise quantize snap groove feel phase polarity "
                            "comb audio to midi drums to midi stem loop"),
                     getNames (triggerguide::allTopics, [] (auto t) { return triggerguide::getTopicName (t); }), {} };

        case Guide::readMusic:
            return { g, Area::readingMusic, "How to read music",
                     "Read sheet music: the staff, the treble and bass clefs, note names, ledger lines, sharps and flats, key signatures, and how DawIt writes your MIDI as a score.",
                     "The Score view and Generate panel's guide buttons, its right-click menu, and File > Export Sheet Music (PDF)...", {},
                     words ("sheet music notation score staff stave clef treble bass g f note names ledger line middle c grand sharp flat natural accidental key signature "
                            "read music sight-reading do i need to read music time bar tempo tie beam rest triplet lead chord symbols abc generated musician"),
                     getNames (readmusic::allTopics, [] (auto t) { return readmusic::getTopicName (t); }), {} };

        case Guide::instruments:
            return { g, Area::midi, "Instruments in MIDI",
                     "How each family of instruments makes its sound, its range and how it's played, and how to write it in MIDI so it sounds real: keys, guitar, bass, strings, brass, woodwinds, voice, synths and drums.",
                     "A MIDI clip's right-click menu and a MIDI track's FX menu (About writing for this instrument...)", {},
                     words ("instrument instruments orchestra orchestration family range ranges violin viola cello double bass contrabass strings bow bowing legato spiccato pizzicato tremolo divisi vibrato keyswitch articulation "
                            "brass trumpet trombone horn tuba mute valve slide woodwind flute piccolo clarinet oboe bassoon saxophone sax reed breath guitar strum strumming fingerpicking palm mute bassist slap ghost note "
                            "approach note root fifth pianist soundboard hammer organ choir satb backing vocals chops synth portamento glide drummer groove percussion shaker conga tabla steel pan kalimba timpani marimba "
                            "xylophone membrane harmonics odd standing wave realistic humanise humanize transposing"),
                     getNames (instruments::allTopics, [] (auto t) { return instruments::getTopicName (t); }), {} };

        case Guide::keyFinding:
            return { g, Area::harmony, "Finding the key and chords",
                     "Keys, scales and modes, how a chord is named from its notes, and the key and chords of your own MIDI clip.",
                     "A MIDI clip's menu (Detect key and chords...) and the piano roll's Key and chords button", {},
                     words ("key scale mode modes ionian dorian phrygian lydian mixolydian aeolian locrian pentatonic blues harmonic melodic minor "
                            "inversion slash bass sus suspended diminished augmented add9 passing neighbour detect analyse find"),
                     getNames (keyfinding::allTopics, [] (auto t) { return keyfinding::getTopicName (t); }), {} };
    }

    return {};
}

namespace
{
    template <typename Topics, typename Text>
    juce::StringArray getTexts (const Topics& topics, Text text)
    {
        juce::StringArray texts;

        for (auto t : topics)
            texts.add (text (t));

        return texts;
    }

    /** The text of each page of a guide without a song (an empty song, C major, 120 BPM), for the search. Empty for a
        guide built on a plugin: its pages are made with the plugin. */
    juce::StringArray getGuideTexts (Guide g)
    {
        const MusicalKey key;   // C major

        switch (g)
        {
            case Guide::sound:      return getTexts (soundguide::allTopics, [] (auto t) { return soundguide::explain (t); });
            case Guide::midi:       return getTexts (midiguide::allTopics, [] (auto t) { return midiguide::explain (t, nullptr, 120.0); });
            case Guide::rhythm:     return getTexts (rhythm::allTopics, [] (auto t) { return rhythm::explain (t, 120.0, daw::TimeSignature {}); });
            case Guide::styles:     return getTexts (stylesguide::allTopics, [] (auto t) { return stylesguide::explainGeneral (t); });
            case Guide::lyrics:     return getTexts (lyricsguide::allTopics, [] (auto t) { return lyricsguide::explain (t, Lyrics {}, SectionList {}, 120.0); });
            case Guide::singing:    return getTexts (singing::allTopics, [] (auto t) { return singing::explain (t, singing::Context {}); });
            case Guide::gainStaging: return getTexts (gainstaging::allTopics, [] (auto t) { return gainstaging::explain (t); });
            case Guide::routing:    return getTexts (routingguide::allTopics, [] (auto t) { return routingguide::explain (t); });
            case Guide::recording:  return getTexts (recordingguide::allTopics, [] (auto t) { return recordingguide::explain (t); });
            case Guide::tuning:     return getTexts (tuning::allTopics, [] (auto t) { return tuning::explain (t); });
            case Guide::triggers:   return getTexts (triggerguide::allTopics, [] (auto t) { return triggerguide::explain (t); });
            case Guide::readMusic:  return getTexts (readmusic::allTopics, [] (auto t) { return readmusic::explain (t, readmusic::Part()); });
            case Guide::handover:   return getTexts (handoverguide::allTopics, [] (auto t) { return handoverguide::explain (t); });
            case Guide::instruments: return getTexts (instruments::allTopics, [] (auto t) { return instruments::explain (t); });

            case Guide::keyFinding:
                return getTexts (keyfinding::allTopics, [] (auto t)
                                 { return t == keyfinding::Topic::yours ? keyfinding::explainYourClip ({}) : keyfinding::getText (t); });

            case Guide::chords:
            {
                juce::String progressions;

                for (auto& p : guide::getCommonProgressions (key))
                    progressions += p.name + ": " + p.summary + "\n\n" + guide::explainProgression (key, p) + "\n\n";

                return { guide::getIntroduction (key), guide::explainCircleOfFifths (key), guide::explainChordTrack (key, {}),
                         playalong::explainPlayingAlong ({}, 120.0), progressions.trim() };
            }

            case Guide::structure:
            {
                juce::String typical;

                for (auto& st : structure::getStructures())
                    typical += structure::explainStructure (st, 120.0) + "\n\n";

                return { structure::getIntroduction(), structure::explainSections(), structure::explainSongStructure ({}, 120.0), typical.trim() };
            }

            case Guide::chordShapes: case Guide::tune: case Guide::harmoniser: case Guide::eq: case Guide::compressor:
            case Guide::gate: case Guide::stereo: case Guide::loudness: case Guide::reverb: case Guide::delay: case Guide::slicer:
            case Guide::stepFilter: case Guide::spectralFreeze: case Guide::saturation: case Guide::distortion:
            case Guide::subtractive: case Guide::additive: case Guide::fm: case Guide::drums: case Guide::physical:
            case Guide::arpeggiator: case Guide::sampler:
                break;
        }

        return {};
    }
} // namespace

std::vector<Entry> getEntries()
{
    std::vector<Entry> entries;

    for (auto g : allGuides)
    {
        auto entry = getEntry (g);

        // Every guide ends with its sources (read from SOURCES.md). A guide built on a plugin has no topics here: its
        // window adds the page itself.
        if (! entry.topics.isEmpty())
        {
            entry.texts = getGuideTexts (g);
            jassert (entry.texts.size() == entry.topics.size());
            entry.topics.add (sourcesTopicName);
            entry.texts.add ({});   // the Sources page has no text of its own
        }

        entry.keywords.addArray (words ("sources references reference citations where this come from"));
        entries.push_back (std::move (entry));
    }

    return entries;
}

std::vector<Guide> getStartPath()
{
    return { Guide::sound, Guide::rhythm, Guide::chords, Guide::structure, Guide::styles, Guide::gainStaging, Guide::eq, Guide::compressor };
}

juce::StringArray getSearchWords (const juce::String& query)
{
    static const juce::StringArray small { "a", "an", "the", "is", "are", "what", "whats", "how", "do", "does", "i", "my", "to",
                                           "of", "in", "on", "for", "and", "or", "why", "it", "its", "can", "with", "me" };
    juce::StringArray result;

    // Letters, digits, hyphens and apostrophes make words; "what's" becomes "whats".
    juce::String word;
    const auto flush = [&]
    {
        word = word.removeCharacters ("'").trimCharactersAtStart ("-").trimCharactersAtEnd ("-");

        if (word.isNotEmpty() && ! small.contains (word))
            result.addIfNotAlreadyThere (word);

        word.clear();
    };

    for (auto c : query.toLowerCase())
    {
        if (juce::CharacterFunctions::isLetterOrDigit (c) || c == '-' || c == '\'')
            word += juce::String::charToString (c);
        else
            flush();
    }

    flush();
    return result;
}

namespace
{
    /** True when a word of `text` starts with `word`, or (for plurals and the like) `word` starts with a word of at least
        4 letters of it. */
    bool contains (const juce::String& text, const juce::String& word)
    {
        for (auto& t : getSearchWords (text))
            if (t.startsWith (word) || (t.length() >= 4 && word.startsWith (t)))
                return true;

        return false;
    }
} // namespace

namespace
{
    /** True when `word` starts a word somewhere in `text` (case does not matter). */
    bool textHasWord (const juce::String& text, const juce::String& word)
    {
        for (int from = 0;;)
        {
            const int at = text.indexOfIgnoreCase (from, word);

            if (at < 0)
                return false;

            if (at == 0 || ! juce::CharacterFunctions::isLetterOrDigit (text[at - 1]))
                return true;

            from = at + 1;
        }
    }
} // namespace

juce::String getExcerpt (const juce::String& text, const juce::StringArray& words)
{
    int best = -1, bestLength = 0;

    for (auto& w : words)
    {
        for (int from = 0;;)
        {
            const int at = text.indexOfIgnoreCase (from, w);

            if (at < 0)
                break;

            if (at == 0 || ! juce::CharacterFunctions::isLetterOrDigit (text[at - 1]))
            {
                if (best < 0 || at < best)
                {
                    best = at;
                    bestLength = w.length();
                }

                break;
            }

            from = at + 1;
        }
    }

    if (best < 0)
        return {};

    // About 60 characters before the word and the rest after it, cut at spaces, in one line.
    int start = juce::jmax (0, best - 60);
    int end = juce::jmin (text.length(), start + 150);

    if (start > 0)
    {
        const int space = text.indexOfChar (start, ' ');

        if (space >= 0 && space < best)
            start = space + 1;
    }

    if (end < text.length())
    {
        const int space = text.substring (0, end).lastIndexOfChar (' ');

        if (space > best + bestLength)
            end = space;
    }

    auto line = text.substring (start, end).replaceCharacters ("\n\r\t", "   ").trim();

    while (line.contains ("  "))
        line = line.replace ("  ", " ");

    return (start > 0 ? "... " : "") + line + (end < text.length() ? " ..." : "");
}

std::vector<Match> search (const std::vector<Entry>& entries, const juce::String& query)
{
    const auto queryWords = getSearchWords (query);
    std::vector<Match> matches;

    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        Match match { i, {}, 0 };
        bool all = true;
        std::vector<int> topicHits ((size_t) e.topics.size(), 0);

        for (auto& w : queryWords)
        {
            // Scores: name 6, topic or keywords 4, summary 2, page text 1.
            int best = 0;

            if (contains (e.name, w))
                best = 6;
            else if (contains (e.keywords.joinIntoString (" "), w) || contains (e.plugin, w))
                best = 4;
            else if (contains (e.summary, w))
                best = 2;

            for (int t = 0; t < e.topics.size(); ++t)
            {
                if (contains (e.topics[t], w))
                {
                    ++topicHits[(size_t) t];
                    best = juce::jmax (best, 4);
                }
            }

            // Only a word nothing else has is looked for in the text, which is the slowest part.
            if (best == 0)
            {
                for (int t = 0; t < e.texts.size() && t < e.topics.size(); ++t)
                {
                    if (textHasWord (e.texts[t], w))
                    {
                        ++topicHits[(size_t) t];
                        best = 1;
                    }
                }
            }

            if (best == 0)
            {
                all = false;
                break;
            }

            match.score += best;
        }

        if (! all)
            continue;

        // The topics that match the most words; none named means the guide as a whole matches.
        const int most = topicHits.empty() ? 0 : *std::max_element (topicHits.begin(), topicHits.end());

        for (int t = 0; t < (int) topicHits.size(); ++t)
            if (most > 0 && topicHits[(size_t) t] == most)
                match.topics.push_back (t);

        matches.push_back (match);
    }

    std::stable_sort (matches.begin(), matches.end(), [] (const Match& a, const Match& b) { return a.score > b.score; });
    return matches;
}
} // namespace daw::gen::guides
