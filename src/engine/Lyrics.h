#pragma once

#include "Section.h"

#include <optional>

namespace daw
{
/** One sung syllable of the lyrics track: a whole short word, or a part of a longer one
    ("to-ge-ther" is three). */
struct LyricSyllable
{
    juce::String text;        // without the hyphen: "to", "ge", "ther,"
    bool endsWord = true;     // false when the word goes on in the next syllable
    bool pinned = false;      // placed by the user (dragged or tapped); the others are spread out between
    double beat = 0.0;        // where it's sung, in beats; for an unpinned one worked out by lyrics::spread()

    bool operator== (const LyricSyllable& other) const
    {
        return text == other.text && endsWord == other.endsWord && pinned == other.pinned && juce::exactlyEqual (beat, other.beat);
    }
};

/** A sung line: from its first syllable (at `startBeat`) to where the singing stops (`endBeat`). */
struct LyricLine
{
    double startBeat = 0.0, endBeat = 6.0;
    bool startsVerse = false;   // an empty line before it in the text (or the first line)
    juce::String heading;       // "Chorus", from a "[Chorus]" line above the verse
    std::vector<LyricSyllable> syllables;

    /** Where syllable `index` stops: at the next one, or at the end of the line. */
    double getSyllableEnd (size_t index) const { return index + 1 < syllables.size() ? syllables[index + 1].beat : endBeat; }

    bool operator== (const LyricLine& other) const
    {
        return juce::exactlyEqual (startBeat, other.startBeat) && juce::exactlyEqual (endBeat, other.endBeat)
               && startsVerse == other.startsVerse && heading == other.heading && syllables == other.syllables;
    }
};

/** The song's lyrics: the lines in the order they're sung. Like the chord track they make no sound. */
struct Lyrics
{
    std::vector<LyricLine> lines;

    bool isEmpty() const noexcept { return lines.empty(); }
    bool operator== (const Lyrics& other) const { return lines == other.lines; }
    bool operator!= (const Lyrics& other) const { return ! (*this == other); }
};

/** What the song is about, for the Lyrics window's Write with AI panel: free text, a mood and a point of view. It is
    saved with the song and read by every request. `mood` and `pointOfView` are ids from `lyrics::brief`, empty for "Any". */
struct LyricsBrief
{
    juce::String about;
    juce::String mood;
    juce::String pointOfView;

    bool isEmpty() const noexcept { return about.isEmpty() && mood.isEmpty() && pointOfView.isEmpty(); }
    bool operator== (const LyricsBrief& other) const { return about == other.about && mood == other.mood && pointOfView == other.pointOfView; }
    bool operator!= (const LyricsBrief& other) const { return ! (*this == other); }
};

/**
    Typing lyrics and placing them in time.

    The text is written the way lyrics usually are: one sung line per line, an empty line between
    verses, an optional "[Chorus]" heading above a verse, and hyphens splitting a word into the
    syllables that get their own place in time ("to-ge-ther"). parse() reads it without timing,
    place() spreads it over the song's sections, and setText() edits lyrics that are already timed,
    keeping the timing of every syllable that didn't change.

    Timing: a line starts at its first syllable and ends at `endBeat`. Syllables the user placed
    (pinned) stay where they are; the others are spread evenly between the pinned ones around
    them, so dragging one word stretches or squeezes the words up to the next placed one.
*/
namespace lyrics
{
    constexpr double minGap = 0.125;          // the closest two syllables get, in beats (a 32nd note)
    constexpr double beatsPerSyllable = 1.0;  // a new line fills its phrase, but no slower than this
    constexpr double beatsPerLine = 8.0;      // two bars a line where there are no sections to follow

    /** The text as lines and syllables, without timing. */
    Lyrics parse (const juce::String& text);

    /** Back to text: the lines with hyphens inside words, empty lines between verses, and headings. */
    juce::String toText (const Lyrics&);

    /** Places parsed lyrics: each verse on a section (by its heading when it has one: a chorus typed
        once goes on every chorus; otherwise in order on the sections that are usually sung), the
        lines spread over the section's bars. Verses left over, or all of them when there are no
        sections, follow two bars a line from `firstBar` or after the last one placed. */
    Lyrics place (const Lyrics& parsed, const SectionList&, int firstBar = 0);

    /** New text for lyrics that may be timed already: syllables that stay the same keep their timing
        (and lines theirs), and new lines go in the gaps, on a free section where one fits. Without
        timed lyrics to keep it places the text over the sections. */
    Lyrics setText (const Lyrics& current, const juce::String& text, const SectionList&);

    /** Keeps every line in order (pinned syllables after the one before, the end after the last)
        and spreads the unpinned syllables. Lines without syllables go. */
    void tidy (Lyrics&);

    /** Spreads a line's unpinned syllables evenly between its start, its pinned ones and its end. */
    void spread (LyricLine&);

    /** Where a syllable is: its line and its place in the line. */
    struct Position
    {
        int line = 0, syllable = 0;
        bool operator== (const Position& other) const { return line == other.line && syllable == other.syllable; }
    };

    bool isValid (const Lyrics&, Position);

    /** The syllable after `p`, in the next line after a line's last; nullopt after the last of all. */
    std::optional<Position> getNext (const Lyrics&, Position);

    /** The syllable being sung at `beat` (from its start to the next one's), or nullopt between lines. */
    std::optional<Position> findAt (const Lyrics&, double beat);

    /** The first syllable sung at or after `beat`. */
    std::optional<Position> findFrom (const Lyrics&, double beat);

    /** Drags a syllable to `beat` and pins it, within the syllables placed around it. The first
        one is the line's start. */
    void moveSyllable (Lyrics&, Position, double beat);

    /** Moves a whole line, keeping its timing, but not past the lines around it. */
    void moveLine (Lyrics&, int line, double startBeat);

    /** Moves where a line stops, squeezing or stretching the unpinned syllables before it. */
    void setLineEnd (Lyrics&, int line, double endBeat);

    /** Lets a syllable (or with syllable -1, a whole line) be spread with the others again. */
    void unpin (Lyrics&, int line, int syllable = -1);

    /** Splits an English word into its sung syllables by spelling rules: "together" gives "to",
        "ge", "ther" and "dancing," gives "danc", "ing,". Vowel groups make the syllables (a silent
        e at the end doesn't, "-le" after a consonant does), and the consonants between them are
        shared out the way dictionaries hyphenate. Punctuation stays on the first or last part.
        Short words, and words it can't be sure of (digits, capitals inside, letters other than
        a to z), come back whole. English only: other languages are spelt by other rules. */
    juce::StringArray splitEnglishWord (const juce::String& word);

    /** The moods and points of view a song brief can name: (id, name shown). The first of each is "Any", with an
        empty id. */
    namespace brief
    {
        using Option = std::pair<juce::String, juce::String>;
        const std::vector<Option>& getMoods();
        const std::vector<Option>& getPointsOfView();

        /** The brief tidied: the text trimmed, and a mood or point of view that isn't in the lists made "Any". */
        LyricsBrief tidy (LyricsBrief);
    }

    /** Tap along: the syllable at `p` is sung at `beat`. Tapping a line's first syllable moves the
        whole line there; tapping its last ends the line just after it. */
    void tap (Lyrics&, Position, double beat);
} // namespace lyrics
} // namespace daw
