#pragma once

#include "generate/Notation.h"
#include "Theme.h"

namespace daw
{
/** Draws a `gen::notation::Score` with Bravura, the SMuFL music font built into the app: clefs, note heads,
    stems, flags, beams, rests, accidentals, ties, bar lines, chord symbols and section names. It only scales
    and draws: where things go comes from `gen::notation` (staff steps, x positions, lines). The score view, the
    theory guide's pictures and the sheet-music PDF all draw through it, with their own ink colours.

    Sizes are in pixels; `space` is the distance between two staff lines. Everything here is relative to
    `space`, so a score can be drawn at any size. */
namespace scorepaint
{
    /** The colours of one drawing: light ink on the dark panel in the app, black on white on paper. */
    struct Ink
    {
        juce::Colour ink = theme::text;                 // staff lines, notes, clefs
        juce::Colour dim = theme::textDim;              // bar numbers
        juce::Colour accent = theme::accent;            // notes that are sounding or picked out
        juce::Colour background = theme::panel;         // under the pinned header
    };

    /** What a place on the drawing is, for tooltips and clicks. */
    enum class HitKind { note, rest, clef, keySignature, timeSignature, chordSymbol, tripletBracket };

    struct Hit
    {
        HitKind kind = HitKind::note;
        int staff = 0;
        int bar = 0;                    // in the score
        int event = -1;                 // in the bar, for notes and rests
        juce::Rectangle<float> bounds;
        juce::String text;              // the chord symbol
    };

    /** What a system draws besides the staves. */
    struct Options
    {
        bool barNumbers = true;
        bool chordSymbols = true;
        bool sectionsAndTempo = true;
        bool header = true;             // clef, key signature (and time signature on the first line)
        bool finalBarLine = true;       // the thin and thick bar line after the score's last bar
        int firstBar = -1, lastBar = -1;    // the bars of the line to draw, or -1 for all (to draw only what's visible)
    };

    /** Called for each event to say whether to light it up: (staff, bar in the score, event in the bar). */
    using LitFunction = std::function<bool (int, int, int)>;

    /** Pixels from the top line of the first staff up to the top of what's drawn above it (names, tempo, chords, bar numbers). */
    float getTopMargin (float space);

    /** Pixels from the bottom line of the last staff down to the end of what's drawn under it (ledger lines). */
    float getBottomMargin (float space);

    /** Pixels from the top line of the first staff up to the top of what a line of music draws above it: more when the
        line has section names or tempo marks, then chord symbols, then only bar numbers. For pages and pictures that
        stack lines tightly; the scrolling score uses `getTopMargin`. */
    float getLineTopMargin (const gen::notation::Score&, const gen::notation::Line&, float space);

    /** Pixels from the top line of the first staff to the bottom line of the last one. */
    float getStaffBlockHeight (const gen::notation::Score&, float space);

    /** Pixels from the top line of the first staff to the top line of staff `index`. */
    float getStaffOffset (int index, float space);

    /** The y of a staff step (0 = bottom line) on a staff whose top line is at `staffTop`. */
    float getStepY (float staffTop, int step, float space);

    /** Draws one line of music (a `Line` from `gen::notation::layoutLines`) with its first staff's top line at `yTop`
        and its left edge at `x`. Fills `hits` with what is drawn, if given. */
    void drawSystem (juce::Graphics&, const gen::notation::Score&, const gen::notation::Line&, float x, float yTop, float space,
                     const Ink&, const Options&, std::vector<Hit>* hits = nullptr, const LitFunction& lit = {});

    /** Draws the clef and key signature (and the time signature) of every staff, with the brace and the bar line that
        joins a grand staff, at `x`. Used by `drawSystem`, and on its own for the score view's pinned header. */
    void drawHeader (juce::Graphics&, const gen::notation::Score&, bool timeSignature, float x, float yTop, float space,
                     const Ink&, std::vector<Hit>* hits = nullptr);

    /** Draws a clef alone (to check the font, or for a picture). */
    void drawClef (juce::Graphics&, gen::notation::Clef, int writtenShift, float x, float staffTop, float space, juce::Colour);

    /** The name the score gives a note value: "quarter note", "dotted eighth note", "eighth-note triplet". */
    juce::String describeEvent (const gen::notation::Event&);
} // namespace scorepaint
} // namespace daw
