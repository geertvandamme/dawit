#pragma once

#include "SingPairing.h"
#include "engine/Singer.h"

namespace daw::gen::singthis
{
/** The sung score: the paired melody turned into phrases of sounds with their lengths in frames and a pitch for
    every frame, ready for a Singer. UI-free and tested. */

constexpr double phraseRestSeconds = 0.5;      // a rest this long or longer starts a new phrase
constexpr double maxBreathSeconds = 0.3;       // the breath before a phrase, at most this long and at most half the rest before it
constexpr double fallbackConsonantSeconds = 0.08;   // without a duration model, each consonant lasts this long
constexpr double slideSeconds = 0.05;          // Exact notes slide between notes over this long

/** 440 · 2^((midi − 69) / 12): C4 is 261.63 Hz. */
double midiToHz (double midi);

struct ScoreSettings
{
    bool breaths = true;        // a breath in the rest before a phrase that follows a rest of half a second or more
    bool exactPitch = false;    // true: the pitch stays on each note; false: the voice's pitch model draws it when it has one
};

/** Where one syllable's vowel ended up, to check that it starts on its note. */
struct SyllableTiming
{
    int phrase = 0;                  // index in Score::phrases
    double noteStartSeconds = 0.0;   // when its note starts
    double noteSeconds = 0.0;        // how long its note (with the notes that hold it) lasts
    double vowelStartSeconds = 0.0;  // when its vowel starts in the sung phrase, after rounding to frames
    double vowelSeconds = 0.0;       // how long the vowel lasts
    int word = 0;
};

struct Score
{
    std::vector<sing::Phrase> phrases;
    std::vector<SyllableTiming> syllables;                  // one per sung syllable, in order
    double spanStartSeconds = 0.0, spanEndSeconds = 0.0;   // the melody's first note starts and its last ends
    juce::StringArray warnings;                             // what the singer left out ("The pitch model failed ...")
};

/** Builds the score of a pairing. Vowels start on their note, leading consonants end at the note's start (and may
    reach back into the note before), word-final consonants close the note, and the notes a syllable is held over share
    its vowel. On short notes the consonants around a vowel are shortened together so that the vowel keeps 40% of its note
    (and at least three frames, 35 ms); the frames are rounded boundary by boundary, and a sound that would get less than
    a frame takes it from the sounds before it, so vowels stay on their notes and the phrase keeps its length. Each phrase gets 0.5 s of padding first. With a `singer` the
    consonants' lengths come from its duration model and, unless `exactPitch`, the pitch from its pitch model. */
Score buildScore (const Melody&, const Pairing&, const sing::Voice*, const ScoreSettings&,
                  sing::Singer* singer = nullptr, const sing::CancelFn& cancel = {});

/** How well the vowels sit on their notes: the error of each vowel's start against its note's start, in ms
    (positive: late), and the shortest vowel as a share of its note. */
struct TimingReport
{
    int count = 0;
    double meanAbsErrorMs = 0.0, maxAbsErrorMs = 0.0, meanErrorMs = 0.0;
    int overOneFrame = 0;            // vowels more than one frame (11.6 ms) off
    double shortestVowelMs = 0.0;
    double smallestVowelShare = 1.0; // vowel length / note length, the smallest
};

TimingReport measureTiming (const Score&);

/** When a phoneme of a phrase starts in the song, in seconds (for the tests and the picture). */
double getPhonemeStartSeconds (const sing::Phrase&, size_t phonemeIndex);
double getPhonemeEndSeconds (const sing::Phrase&, size_t phonemeIndex);
} // namespace daw::gen::singthis
