#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace daw
{
/** The song's time signature: `numerator` notes of the `denominator` kind fill a bar (3/4: three quarter
    notes; 6/8: six eighth notes). One signature for the whole song. The timeline stays in quarter-note beats
    whatever the signature: a quarter note is one beat in 3/4 and in 6/8 too, and only the length of a bar
    changes (see timesig::getBarLength()). */
struct TimeSignature
{
    int numerator = 4;     // 1 to 15
    int denominator = 4;   // 2, 4, 8 or 16

    bool operator== (const TimeSignature& other) const noexcept { return numerator == other.numerator && denominator == other.denominator; }
    bool operator!= (const TimeSignature& other) const noexcept { return ! operator== (other); }
};

/** Helpers on a time signature. All lengths are in quarter notes (the timeline's beats), the first bar is
    bar 0 (shown as 1). Song code never writes a bar length: it asks the song's TimeSignature. */
namespace timesig
{
    constexpr int minNumerator = 1, maxNumerator = 15;

    /** The signature used by examples written in 4/4 (guide pictures, built-in patterns), named so that they
        say so. */
    inline constexpr TimeSignature fourFour { 4, 4 };

    /** The nine signatures the menu offers, in order: 2/4, 3/4, 4/4, 5/4, 6/8, 7/8, 9/8, 12/8, 2/2. */
    const std::vector<TimeSignature>& getCommon();

    /** True for 2, 4, 8 and 16. */
    bool isValidDenominator (int denominator) noexcept;

    /** The signature with the top number kept within 1 to 15, and a bottom number that isn't 2, 4, 8 or 16 made
        4: 0/4 becomes 1/4, 16/4 becomes 15/4, 4/3 becomes 4/4. */
    TimeSignature tidy (TimeSignature) noexcept;

    /** How long a bar is, in quarter notes: top x 4 / bottom (3/4: 3, 6/8: 3, 7/8: 3.5, 2/2: 4). */
    double getBarLength (TimeSignature) noexcept;

    /** How long the bottom note is, in quarter notes: 4 / bottom (x/2: 2, x/4: 1, x/8: 0.5, x/16: 0.25). */
    double getPulseLength (TimeSignature) noexcept;

    /** Simple metres count the bottom note and split it in two; compound ones (top 6, 9, 12, 15) count dotted
        notes of three bottom notes; the rest are odd, a mix of twos and threes. */
    enum class Kind { simple, compound, odd };
    Kind getKind (TimeSignature) noexcept;

    /** The beat groups of a bar, as the number of bottom notes in each: 6/8 is {3, 3}, 7/8 {2, 2, 3}, 5/4 five
        ones, 2/2 {1, 1}, 6/4 {3, 3}. They add up to the top number. DawIt's fixed choice, in this order: top 1
        one group; top 6, 9, 12 or 15 groups of three; top 2, 3 or 4, or any other top over 2 or 4, one group per
        bottom note; other tops over 8 or 16 in twos with a final three when odd. */
    std::vector<int> getGroups (TimeSignature);

    /** Where each beat group starts, in quarter notes from the start of the bar: 6/8 {0, 1.5}, 7/8 {0, 1, 2}. */
    std::vector<double> getGroupStarts (TimeSignature);

    /** True when the groups are longer than one bottom note, so the groups are not all the same as the pulses
        (6/8, 7/8, 6/4; not 4/4, 3/4, 5/4 or 2/2). */
    bool hasLongGroups (TimeSignature);

    /** How the metronome clicks one pulse (a bottom note): the first of the bar loudly, the first of each
        other group medium where groups are longer than a pulse, the others softly. */
    enum class Click { bar, group, beat };

    /** The click on pulse `pulseInBar` (0 to top - 1) of a bar. In x/4 and x/2 every pulse after the first is
        `beat`; where groups are longer than a pulse, the other group starts are `group`. */
    Click getClick (TimeSignature, int pulseInBar);

    /** The bar a beat falls in (0 for the first bar), also for negative beats (before the song start). */
    int getBarAt (TimeSignature, double beat) noexcept;

    /** The beat a bar starts at. */
    double getBarStart (TimeSignature, int bar) noexcept;

    /** The beat of the bar line at or before `beat`, as the plugin playhead reports. */
    double getBarStartAtOrBefore (TimeSignature, double beat) noexcept;

    /** A position as the display shows it: bar.beat.sixteenth, all counted from 1; the beat counts the bottom
        note and the third number the sixteenths inside it (x/4: 1 to 4, x/8: 1 to 2, x/2: 1 to 8, x/16: 1).
        4/4 at beat 18.25: "5.3.2"; 3/4: "7.1.2"; 6/8 at 4.75: "2.4.2". */
    juce::String formatPosition (TimeSignature, double beat);

    /** "3/4". */
    juce::String toString (TimeSignature);

    /** A sentence fragment saying what a bar is: "three quarter-note beats in each bar", "two beats in each bar,
        each three eighth notes long (six eighths in all)", "seven eighth notes in each bar, grouped 2+2+3". */
    juce::String describe (TimeSignature);

    /** The beat you feel at a tempo that counts quarter notes: BPM, or BPM divided by the length of the dotted
        beat in compound metres (6/8 at 120 BPM: 80; 6/4: BPM / 3). */
    double getFeltBpm (TimeSignature, double bpm) noexcept;

    /** How long a bar lasts at a steady tempo, in seconds: bar length x 60 / BPM. */
    double getBarSeconds (TimeSignature, double bpm) noexcept;

    /** MIDI clocks per metronome click for the MIDI file's time signature event (24 clocks a quarter note): a
        click per group in the compound metres, else per bottom note: x/4 24, x/2 48, 6/8 36, 7/8 12, 6/16 18. */
    int getMidiClocksPerClick (TimeSignature) noexcept;

    /** Where a beat goes when the signature changes from `from` to `to` and each bar keeps its place: the beat
        scaled by the ratio of the bar lengths, so bar 4 stays bar 4 and a position half a bar in stays half a bar
        in (4/4 to 3/4: 16 becomes 12 and 18 becomes 13.5). */
    double mapBeatKeepingBars (double beat, TimeSignature from, TimeSignature to) noexcept;
} // namespace timesig
} // namespace daw
