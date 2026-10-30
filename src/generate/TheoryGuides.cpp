#include "TheoryGuides.h"
#include "LyricsGuide.h"
#include "ReadingGuide.h"
#include "RhythmGuide.h"
#include "SingingGuide.h"
#include "StylesGuide.h"
#include "TuningGuide.h"
#include "engine/GainStaging.h"
#include "engine/MidiGuide.h"
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
        case Area::effects:     return "Space, echoes and colour: reverb, delay, saturation, distortion";
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

Entry getEntry (Guide g)
{
    switch (g)
    {
        case Guide::sound:
            return { g, Area::sound, "What is sound?",
                     "Air moving, waves, loudness in decibels, pitch in hertz, timbre and envelopes, how a sound becomes numbers, phase, and mono.",
                     "The wave editor (What is sound?)", {},
                     words ("wave hertz hz pitch decibel db loudness harmonics overtone sample rate bit depth phase karaoke vocals envelope adsr attack decay sustain release mono stereo correlation"),
                     getNames (soundguide::allTopics, [] (auto t) { return soundguide::getTopicName (t); }) };

        case Guide::midi:
            return { g, Area::midi, "What is MIDI?",
                     "What MIDI is and isn't, its history, the messages, note numbers, velocity, controllers and files.",
                     "The piano roll (What is MIDI?), the computer keyboard strip", {},
                     words ("note number velocity controller cc channel keyboard message mid file sustain pitch bend "
                            "computer keyboard musical typing typing qwerty play without a keyboard"),
                     getNames (midiguide::allTopics, [] (auto t) { return midiguide::getTopicName (t); }) };

        case Guide::rhythm:
            return { g, Area::rhythm, "Note lengths, beats and bars",
                     "Bars and beats, whole to sixteenth notes, rests, dotted notes and triplets, counting and tempo.",
                     "The ruler's and the Tempo row's menus, and the ? in the piano roll", {},
                     words ("beat bar measure tempo bpm quarter eighth sixteenth crotchet quaver minim semibreve rest dotted triplet "
                            "counting time signature snap"),
                     getNames (rhythm::allTopics, [] (auto t) { return rhythm::getTopicName (t); }) };

        case Guide::chords:
            return { g, Area::harmony, "Chord progressions",
                     "How chords work in a key, the circle of fifths, well-known progressions, and your own chord track.",
                     "The Chords label and the chord menu", {},
                     words ("chord progression key major minor scale circle fifths numeral roman diatonic relative tonic "
                            "dominant subdominant"),
                     { "How chords work", "The circle of fifths", "Your chord track", "Playing along", "Well-known progressions" } };

        case Guide::chordShapes:
            return { g, Area::harmony, "Piano and guitar chords",
                     "How to play a chord on piano and on guitar, with its notes, from the chord at the playhead.",
                     "A chord's menu (How to play...)", {},
                     words ("chord piano guitar shape fingering fret finger diagram voicing keyboard play notes"),
                     {} };

        case Guide::structure:
            return { g, Area::songwriting, "Song structure",
                     "What each section is for, the typical structures of different styles, and your song's own sections.",
                     "The Sections label and the section menu", {},
                     words ("section intro verse chorus pre-chorus bridge drop breakdown outro form arrangement energy"),
                     { "How songs are built", "The sections", "Your song's structure", "Typical structures" } };

        case Guide::styles:
            return { g, Area::songwriting, "Styles of music",
                     "Fourteen styles from ballad to drum & bass: their tempo, groove, chords, sounds and structure, a song to start from in each, and how your song compares.",
                     "File > New Project from Style... and the Song structure guide", {},
                     words ("style genre template pop ballad dance rock hip-hop hiphop boom bap trap house techno trance drum bass dnb jungle lofi lo-fi chillhop blues shuffle reggaeton dembow afrobeats afrobeat film score cinematic groove four-on-the-floor backbeat half-time 808 909 new project"),
                     getNames (stylesguide::allTopics, [] (auto t) { return stylesguide::getTopicName (t); }) };

        case Guide::lyrics:
            return { g, Area::songwriting, "How lyrics are written",
                     "What each part says, lines, rhyme, finding rhymes, writing with AI, stress on the beat, words on a melody, the hook, how a computer sings, getting a voice, and your lyrics.",
                     "The ? in the Lyrics row and in the Sing window, and the Lyrics window's Write with AI panel", {},
                     words ("words rhyme syllable stress hook title verse chorus prosody singing sing songwriting voice vocal synthesis synthesiser diffsinger phoneme voicebank vocoder "
                            "ai ollama rewrite rhymes rhyming dictionary near rhyme slant family assonance consonance"),
                     getNames (lyricsguide::allTopics, [] (auto t) { return lyricsguide::getTopicName (t); }) };

        case Guide::singing:
            return { g, Area::singing, "Singing techniques",
                     "How the voice works, breathing, warming up, voice types and your range, registers, singing in tune, vowels, vibrato, belting, harmonies, "
                     "looking after your voice, the microphone, and your melody against your range.",
                     "Sing along's Tips..., the lyrics guide's Singing along, and a vocal track's menu", {},
                     words ("sing singer singing voice vocal vocals vox range tenor soprano alto contralto mezzo baritone bass breath breathing diaphragm warm-up warmup trill "
                            "pitch tune cents flat sharp vibrato belt belting falsetto register chest head mix break passaggio hoarse hoarseness microphone mic pop filter "
                            "plosive proximity harmony double key suits"),
                     getNames (singing::allTopics, [] (auto t) { return singing::getTopicName (t); }) };

        case Guide::tune:
            return { g, Area::singing, "What is pitch correction?",
                     "Cents and what ears notice, how a tuner finds the pitch, target notes, speed and amount, vibrato, formants, hard tune, using it well, and how in tune your track is.",
                     "DawIt Tune's window and the singing guide's Singing in tune", "DawIt Tune",
                     words ("tune tuner tuning autotune auto-tune pitch correction intonation flat sharp cents retune speed humanize vibrato formant chipmunk psola melodyne t-pain cher believe robot hard scale snap"),
                     {} };

        case Guide::gainStaging:
            return { g, Area::mixing, "Gain staging",
                     "How loud the sound is at each step of a track's chain, why it matters, and how to fix a level.",
                     "The device bar (Gain staging?)", {},
                     words ("gain level headroom clipping clip dbfs fader peak meter loud quiet volume"),
                     getNames (gainstaging::allTopics, [] (auto t) { return gainstaging::getTopicName (t); }) };

        case Guide::eq:
            return { g, Area::mixing, "What is EQ?",
                     "Turning frequencies up and down: the ranges, band types, Q and slopes, where instruments sit, masking and common problems.",
                     "DawIt EQ's window (What is EQ?)", "DawIt EQ",
                     words ("eq equaliser equalizer equalisation frequency cut boost bell shelf filter high-pass low-pass mud "
                            "muddy harsh boomy boxy thin dull bright sibilant vocal voice"),
                     {} };

        case Guide::compressor:
            return { g, Area::mixing, "What is compression?",
                     "Turning down the loud parts: threshold, ratio, knee, attack, release, makeup, parallel compression and how much to use.",
                     "DawIt Compressor's window (What is compression?)", "DawIt Compressor",
                     words ("compressor compression dynamics punch glue squash even level limiting limiter pumping vocal voice "
                            "drums bass"),
                     {} };

        case Guide::gate:
            return { g, Area::mixing, "What is a gate?",
                     "Closing the gaps between notes: threshold and hysteresis, range and ratio, attack, hold and release, the key filter, and gated reverb.",
                     "DawIt Gate's window (What is a gate?)", "DawIt Gate",
                     words ("gate gating noise-gate noise hiss hum bleed spill expander expansion downward chatter hysteresis hold key "
                            "sidechain kick snare toms drums gated reverb"),
                     {} };

        case Guide::stereo:
            return { g, Area::mixing, "What is stereo width?",
                     "Mid and side, the goniometer and correlation meter, mono and phase, why the low end stays in the middle, mono below a frequency, width, and your track.",
                     "DawIt Stereo's window (What is stereo width?)", "DawIt Stereo",
                     words ("stereo width wide narrow mono mono-compatible mono-compatibility mono-maker bass-mono mid side mid-side M/S correlation phase "
                            "goniometer vectorscope Lissajous imager imaging widener spread low-end sub bass kick vinyl club crossover Linkwitz-Riley"),
                     {} };

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
                     getNames (routingguide::allTopics, [] (auto t) { return routingguide::getTopicName (t); }) };

        case Guide::reverb:
            return { g, Area::effects, "What is reverb?",
                     "How a room answers a sound: early reflections and the tail, decay, damping, pre-delay, the kinds of reverb (room, hall, chamber, plate, spring, digital) side by side, reverb in a mix, and your track.",
                     "DawIt Reverb's window (What is reverb?)", "DawIt Reverb",
                     words ("reverb room hall plate space decay pre-delay reflections ambience wet dry echo chamber spring EMT tank convolution gated shimmer reverse kinds types"),
                     {} };

        case Guide::delay:
            return { g, Area::effects, "What is delay?",
                     "Echoes in time with the song: feedback, slapback and doubling, darker repeats, ping-pong, and delay in a mix.",
                     "DawIt Delay's window (What is delay?)", "DawIt Delay",
                     words ("delay echo feedback tempo sync ping-pong slapback repeats"),
                     {} };

        case Guide::saturation:
            return { g, Area::effects, "What is saturation?",
                     "Gently bending the wave adds harmonics: even and odd, soft and hard, tape, tubes and transistors, and warmth in a mix.",
                     "DawIt Saturation's window (What is saturation?)", "DawIt Saturation",
                     words ("saturation warmth warm tape tube valve harmonics drive analogue analog"),
                     {} };

        case Guide::distortion:
            return { g, Area::effects, "What is distortion?",
                     "Bending the wave all the way: overdrive, distortion and fuzz, rectifying and folding, tone, and distortion in a mix.",
                     "DawIt Distortion's window (What is distortion?)", "DawIt Distortion",
                     words ("distortion overdrive fuzz clipping drive harmonics guitar amp"),
                     {} };

        case Guide::subtractive:
            return { g, Area::synthesis, "What is subtractive synthesis?",
                     "Bright waves shaped by a filter: the oscillators, cutoff and resonance, envelopes, the LFO, and recipes.",
                     "DawIt Subtractive's window (What is subtractive synthesis?)", "DawIt Subtractive",
                     words ("synth synthesiser synthesizer oscillator filter cutoff resonance envelope adsr lfo saw square pad "
                            "bass lead"),
                     {} };

        case Guide::additive:
            return { g, Area::synthesis, "What is additive synthesis?",
                     "A sound built from sine waves, one per harmonic: the harmonic series, the classic waves, bells, and Hammond drawbars.",
                     "DawIt Additive's window (What is additive synthesis?)", "DawIt Additive",
                     words ("synth synthesiser synthesizer additive harmonics partials organ drawbar sine overtone spectrum"),
                     {} };

        case Guide::fm:
            return { g, Area::synthesis, "What is FM synthesis?",
                     "One sine wobbling another's pitch: carriers and modulators, ratios, the index, algorithms, and bells and keys.",
                     "DawIt FM's window (What is FM synthesis?)", "DawIt FM",
                     words ("synth synthesiser synthesizer fm frequency modulation operator carrier modulator ratio index bell "
                            "electric piano dx7"),
                     {} };

        case Guide::drums:
            return { g, Area::synthesis, "What is drum synthesis?",
                     "Every drum made from oscillators and noise, like a TR-808: kick, snare, hi-hats, clap, toms, and building a kit.",
                     "DawIt Drums' window (What is drum synthesis?)", "DawIt Drums",
                     words ("drums drum machine kick snare hi-hat hat cymbal clap tom cowbell 808 beat"),
                     {} };

        case Guide::physical:
            return { g, Area::synthesis, "What is physical modelling?",
                     "A string or a tube modelled from physics, plucked, struck, bowed or blown.",
                     "DawIt Physical's window (What is physical modelling?)", "DawIt Physical",
                     words ("physical modelling modeling string pluck bow strike blow tube waveguide karplus-strong guitar "
                            "cello clarinet harp"),
                     {} };

        case Guide::recording:
            return { g, Area::sound, "How recording works",
                     "Recording a voice or instrument: the audio interface, input levels, latency, monitoring and takes, with your own device as the example.",
                     "An audio track's right-click menu (About recording audio...), a take's menu, the ? in Audio / MIDI Settings, and the Monitor warning", {},
                     words ("record recording audio interface microphone mic input preamp gain phantom power 48v condenser dynamic hi-z instrument line level latency buffer "
                            "delay monitoring monitor headphones feedback howl take takes comp comping loop clipping clip level xlr usb"),
                     getNames (recordingguide::allTopics, [] (auto t) { return recordingguide::getTopicName (t); }) };

        case Guide::tuning:
            return { g, Area::sound, "How to tune a guitar",
                     "Tuning a guitar, bass or ukulele: cents and beats, the tunings, the reference pitch, tuning by ear and with harmonics, and whether your tuning suits the song.",
                     "The Tuner window (View > Tuner..., Cmd+7)", {},
                     words ("tuner tuning tune guitar bass ukulele uke string strings standard drop dadgad half step down eb flat sharp cents hz 440 442 432 415 "
                            "reference concert pitch harmonic harmonics fret fifth beats peg capo intonation stretch"),
                     getNames (tuning::allTopics, [] (auto t) { return tuning::getTopicName (t); }) };

        case Guide::triggers:
            return { g, Area::midi, "How drum triggers work",
                     "Turning drum hits in a recording into MIDI notes: transients, sensitivity and gap, velocity, telling kick, snare and hi-hat apart, false hits, the grid, and replacing or layering with the phase lined up.",
                     "An audio clip's right-click menu (Hits to MIDI...), the ? in the Hits to MIDI window", {},
                     words ("trigger triggers drum replacement replace replacer layer layering sample samples augment hits hit onset onsets transient transients threshold "
                            "sensitivity retrigger mask gap bleed spill ghost flam roll kick snare hihat hi-hat velocity quantise quantize snap groove feel phase polarity "
                            "comb audio to midi drums to midi stem loop"),
                     getNames (triggerguide::allTopics, [] (auto t) { return triggerguide::getTopicName (t); }) };

        case Guide::readMusic:
            return { g, Area::readingMusic, "How to read music",
                     "Read sheet music: the staff, the treble and bass clefs, note names, ledger lines, sharps and flats, key signatures, and how DawIt writes your MIDI as a score.",
                     "The Score view and Generate panel's guide buttons, its right-click menu, and File > Export Sheet Music (PDF)...", {},
                     words ("sheet music notation score staff stave clef treble bass g f note names ledger line middle c grand sharp flat natural accidental key signature "
                            "read music sight-reading do i need to read music time bar tempo tie beam rest triplet lead chord symbols abc generated musician"),
                     getNames (readmusic::allTopics, [] (auto t) { return readmusic::getTopicName (t); }) };
    }

    return {};
}

std::vector<Entry> getEntries()
{
    std::vector<Entry> entries;

    for (auto g : allGuides)
        entries.push_back (getEntry (g));

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
            int best = 0;

            if (contains (e.name, w))
                best = 3;
            else if (contains (e.keywords.joinIntoString (" "), w) || contains (e.plugin, w))
                best = 2;
            else if (contains (e.summary, w))
                best = 1;

            for (int t = 0; t < e.topics.size(); ++t)
            {
                if (contains (e.topics[t], w))
                {
                    ++topicHits[(size_t) t];
                    best = juce::jmax (best, 2);
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
