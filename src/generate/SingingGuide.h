#pragma once

#include "SingPairing.h"
#include "engine/Lyrics.h"
#include "engine/MusicalKey.h"
#include "engine/Section.h"

#include <vector>

namespace daw::gen
{
/** A guide to singing techniques for people who sing on their own songs and have never had a lesson: how the voice
    works, breathing, warming up, voice types and range, registers, singing in tune, vowels, vibrato, loud singing,
    harmonies, looking after the voice and the microphone. Its last topic checks the song's melody against the
    singer's range. UI-free and tested. */
namespace singing
{
    enum class Topic { voice, breathing, warmup, range, registers, tune, vowels, vibrato, loud, harmony, care, microphone, yours };

    constexpr Topic allTopics[] { Topic::voice, Topic::breathing, Topic::warmup, Topic::range, Topic::registers, Topic::tune,
                                  Topic::vowels, Topic::vibrato, Topic::loud, Topic::harmony, Topic::care, Topic::microphone, Topic::yours };

    juce::String getTopicName (Topic);      // "Posture and breathing"
    juce::String getTopicSummary (Topic);   // one line for the list

    //==============================================================================
    /** The name of a MIDI note with its octave, sharps for black keys: 60 is "C4" (middle C), 69 is "A4". */
    juce::String getNoteName (int midiNote);

    /** The pitch of a MIDI note in Hz, with A4 (69) at 440. */
    double getFrequency (int midiNote);

    /** The notes the voice row can offer: C2 to C7. */
    constexpr int lowestChoice = 36, highestChoice = 96;

    /** The highest note stays at least this many semitones above the lowest. */
    constexpr int minSpan = 5;

    /** A singer's range as two MIDI notes; -1 means not set. */
    struct Range
    {
        int lowest = -1, highest = -1;

        /** True when both notes are set and the highest is at least minSpan above the lowest. */
        bool isSet() const noexcept { return lowest >= 0 && highest >= lowest + minSpan; }
        bool operator== (const Range&) const = default;
    };

    /** A voice type with its usual range, as voice-teaching books give it. */
    struct VoiceType
    {
        juce::String name;        // "Tenor"
        int lowest = 0, highest = 0;
    };

    /** Bass, baritone, tenor, alto, mezzo-soprano and soprano, in that order, each spanning 24 semitones. */
    const std::vector<VoiceType>& getVoiceTypes();

    /** The index into getVoiceTypes() of the type whose range's middle is nearest the middle of the range; a tie
        goes to the lower type. -1 when the range isn't set. */
    int getClosestType (Range);

    /** The indexes of the types whose usual range holds every note from `lowest` to `highest`. */
    std::vector<int> getTypesThatFit (int lowest, int highest);

    /** The index of the type whose range is exactly this one, or -1. */
    int getExactType (Range);

    /** "Not set", "Tenor", or "Custom (closest: tenor)": what the voice row's type menu shows. */
    juce::String getRangeLabel (Range);

    //==============================================================================
    /** True when a track's name has a vocal word (vocal, vox, voice, sing, singer, lead, melody, bv, backing,
        harmony, choir) at the start of one of its words. */
    bool hasVocalWord (const juce::String& trackName);

    /** What the melody choice needs to know about a MIDI track. */
    struct TrackInfo
    {
        juce::String name;
        bool hasNotes = false;
        bool playsDrums = false;
    };

    /** The melody track to start on: the selected one (an index into `tracks`, or -1) when it has notes, else the
        first with notes whose name has a vocal word, else the first with notes that doesn't play drums. -1 when
        none. */
    int chooseMelodyTrack (const std::vector<TrackInfo>& tracks, int selected);

    //==============================================================================
    /** Where the melody's notes are, against the singer's range. */
    struct SectionRange
    {
        juce::String name;                          // "Chorus"
        SectionType type = SectionType::verse;
        int lowest = 0, highest = 0;                // the melody's lowest and highest note in the section
        double beats = 0.0;                         // sung time in the section
    };

    /** What the range check found, and its advice. */
    struct Analysis
    {
        /** What the first applicable rule found. */
        enum class Verdict { noMelody, noRange, noKey, tooHigh, tooLow, nearTop, nearBottom, fits };

        Verdict verdict = Verdict::noMelody;
        juce::String trackName;
        bool hasMelody = false;
        int lowest = 0, highest = 0;                // the melody's lowest and highest notes
        int lowestBar = 1, highestBar = 1;          // where they first are (the first bar is 1)
        std::vector<SectionRange> sections;         // for the picture, in song order
        Range range;                                // the singer's range, when set

        int outside = 0;                            // semitones above or below the range (too high / too low)
        int shift = 0;                              // the suggested key change in semitones, 0 when none
        bool octave = false;                        // the suggestion is an octave
        int newLowest = 0, newHighest = 0;          // the melody after the shift
        juce::String newKeyName;                    // the key after the shift: "G minor"
        double share = 0.0;                         // near the top / bottom: the share of the sung time, 0 to 1
        int sparesTop = 0, sparesBottom = 0;        // when it fits: semitones to spare
        juce::String sectionName;                   // the section the finding is mostly in ("chorus"), or empty

        juce::StringArray paragraphs;               // the advice, one paragraph each, rule 9 last
    };

    /** Checks the melody (the top line, from `singthis::buildMelody`) against the range with DawIt's rules: whether
        it fits, whether it sits near the top or bottom of the range, and the key (or octave) that would suit the
        singer. A range that isn't set gives the melody's own notes and the voice types that fit. */
    Analysis analyse (const singthis::Melody&, const juce::String& trackName, const SectionList&, const MusicalKey&,
                      const Lyrics&, double bpm, Range);

    //==============================================================================
    /** What `explain` knows about the song and the singer. */
    struct Context
    {
        Range range;
        Analysis analysis;
        MusicalKey key;
        double bpm = 120.0;
        Lyrics lyrics;
    };

    /** The text of a topic. "Warming up" names the scale's start note from the range; "Your voice and your song" is
        the analysis. */
    juce::String explain (Topic, const Context&);

    //==============================================================================
    // Example data for the pictures.

    /** One harmonic line of the voice picture: a spectrum of a 220 Hz buzz shaped by two formants. */
    struct VoiceExample
    {
        double fundamental = 220.0;
        int harmonics = 18;                           // up to 3,960 Hz
        double ahFormants[2] { 730.0, 1090.0 };
        double eeFormants[2] { 270.0, 2290.0 };
        int previewNote = 57;                         // A3, played by a click

        /** The height (0 to 1) of the smooth envelope with those two formants at a frequency. */
        static double getEnvelope (const double (&formants)[2], double hz);
    };

    VoiceExample getVoiceExample();

    /** A line of the breathing picture, in beats from the start. */
    struct BreathLine
    {
        juce::String text;
        double startBeat = 0.0, endBeat = 0.0;        // the words
        double restEndBeat = 0.0;                     // the rest after it ends here (equal to endBeat: no rest)
        juce::String caption;
        double breathMark = -1.0;                     // a dotted mark at this beat, or -1
    };

    struct BreathingExample
    {
        double bpm = 96.0;
        std::vector<BreathLine> lines;

        double getRestSeconds (size_t line) const { return (lines[line].restEndBeat - lines[line].endBeat) * 60.0 / bpm; }
        double getLineSeconds (size_t line) const { return (lines[line].endBeat - lines[line].startBeat) * 60.0 / bpm; }
    };

    BreathingExample getBreathingExample();

    struct NoteBlock
    {
        int pitch = 60;
        double start = 0.0, length = 1.0;   // beats
        float db = 0.0f;                    // loud and soft: the level
        juce::String text;
    };

    /** The warm-up picture: a five-note scale up and down, the same a semitone higher (faint), and a siren. */
    struct WarmupExample
    {
        std::vector<NoteBlock> scale, repeat;
        int sirenLow = 55, sirenHigh = 72;
        double sirenBeats = 4.0;
        int startNote = 60;
    };

    /** With a range set the scale starts at lowest + 5; otherwise at middle C. */
    WarmupExample getWarmupExample (Range);

    /** The first note of the warm-up scale: lowest + 5, or middle C without a range. */
    int getWarmupStart (Range);

    /** A chest/middle/head split with its breaks, for the registers picture. */
    struct RegisterExample
    {
        juce::String name;                // "Tenor"
        int lowest = 0, highest = 0;
        int chestTop = 0, headBottom = 0; // chest voice up to here, head voice from here
        int firstBreak = 0, secondBreak = 0;
    };

    std::vector<RegisterExample> getRegisterExamples();

    /** A sung note as its pitch over time against a target, in cents. */
    struct TuneCurve
    {
        enum class Kind { slide, vibrato, drift };
        Kind kind = Kind::slide;
        juce::String name, note;
        double seconds = 1.5;

        double getCents (double t) const;   // the pitch error at time t
    };

    std::vector<TuneCurve> getTuneExamples();
    constexpr double tuneToleranceCents = 25.0;   // "about where listeners hear it"

    /** The sounds of a word in a vowels picture, in beats. */
    struct VowelPart
    {
        juce::String symbol;
        double start = 0.0, end = 0.0;
        bool vowel = false;
    };

    /** A word and the beats its note covers. */
    struct VowelWord
    {
        juce::String text;
        double start = 0.0, end = 0.0;
    };

    struct VowelLine
    {
        juce::String label, note;
        bool wrong = false;
        std::vector<VowelWord> words;
        std::vector<VowelPart> parts;
    };

    std::vector<VowelLine> getVowelExamples();

    /** A held note's pitch swing for the vibrato picture. */
    struct VibratoCurve
    {
        juce::String name;
        double extentCents = 0.0, rateHz = 0.0, startSeconds = 0.0;

        double getCents (double t) const;
        double getPeriodSeconds() const { return rateHz > 0.0 ? 1.0 / rateHz : 0.0; }
    };

    struct VibratoExample
    {
        double seconds = 2.0;
        std::vector<VibratoCurve> curves;   // straight, late, through
    };

    VibratoExample getVibratoExample();

    /** The loud and soft picture: verse and chorus notes with their levels. */
    std::vector<NoteBlock> getLoudExample();

    /** Lead, harmony and a double, for the harmonies picture. */
    struct HarmonyExample
    {
        std::vector<NoteBlock> lead, harmony;
        double doubleOffsetBeats = 0.03;   // about 20 ms at 96 BPM
        double bpm = 96.0;
    };

    HarmonyExample getHarmonyExample();

    /** The level of the voice at a distance from the microphone, against 20 cm: 20 log10 (20 / d). */
    double getMicLevelDb (double distanceCm);

    /** The distances the microphone picture marks, in cm. */
    std::vector<double> getMicDistances();

    constexpr double micZoneNear = 15.0, micZoneFar = 23.0;       // the studio vocal zone, cm
    constexpr double popFilterNear = 5.0, popFilterFar = 8.0;     // the pop filter in front of the mic, cm

    /** The advice of the care picture: what to do every day, and when to stop and see a doctor. */
    struct CareExample
    {
        juce::StringArray everyDay, doctor;
    };

    CareExample getCareExample();

    /** How many things a topic's picture shows (0 would mean an empty picture). */
    size_t getExampleSize (Topic, Range);
} // namespace singing
} // namespace daw::gen
