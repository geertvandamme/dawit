#pragma once

#include "engine/Chord.h"

#include <vector>

namespace daw::gen
{
/** How to play a chord on a piano and a guitar, for the chord guide's diagrams: the piano's
    inversions and smooth voice leading through a progression, and guitar shapes with their
    fingers, barre and how hard they are. */
namespace diagrams
{
    //==============================================================================
    // Piano

    /** The chord's notes for the right hand in close position, from the inversion's lowest
        note (0 root position, 1 first inversion...), all within an octave and a bit, around
        middle C. Ninths are played as seconds. */
    std::vector<int> pianoInversion (const Chord&, int inversion);

    /** How many inversions the chord has: one per note (3 for a triad, 4 for a seventh). */
    int getNumInversions (const Chord&);

    /** "Root position", "1st inversion", "2nd inversion", "3rd inversion". */
    juce::String getInversionName (int inversion);

    /** The inversion that moves the hand least from `previous` (the notes played before): the
        smallest total distance from each new note to the nearest previous one. Root position
        when there is no previous chord. */
    int findSmoothInversion (const Chord&, const std::vector<int>& previous);

    /** The left hand's note: the root, in the octave below middle C. */
    int pianoBass (const Chord&);

    /** The right-hand voicings for a progression, each in the inversion nearest the one before,
        starting from root position. */
    std::vector<std::vector<int>> voiceProgression (const std::vector<Chord>&);

    //==============================================================================
    // Guitar

    enum class Difficulty { easy, medium, hard };

    /** A guitar chord box: the fret per string from low E to high E (-1 not played, 0 open),
        which finger frets it (1 index ... 4 little; 0 for open and unplayed strings) and the
        barre, if the index finger lies across several strings. */
    struct GuitarShape
    {
        std::vector<int> frets, fingers;
        int barreFret = 0, barreFrom = -1, barreTo = -1;   // strings, 0 = low E
        Difficulty difficulty = Difficulty::easy;
        bool playable = true;   // false when it needs more than four fingers

        bool isEmpty() const noexcept { return frets.empty(); }
        bool isPlayable() const noexcept { return playable; }
        bool hasBarre() const noexcept { return barreFret > 0; }
        bool isOpen() const;

        /** The fret the box starts at: 1 when the shape fits in the first four frets (the nut is
            drawn), otherwise its lowest fret. */
        int getBaseFret() const;

        /** As guitarists write it, low E first: "x32010", or "8 10 10 9 8 8" with frets past 9. */
        juce::String getTab() const;
    };

    /** Works out the fingers, barre and difficulty of a shape (frets per string, as findGuitarShape()). */
    GuitarShape makeGuitarShape (const std::vector<int>& frets);

    /** Ways to play the chord, the easiest first; empty when no playable shape is found. */
    std::vector<GuitarShape> getGuitarShapes (const Chord&, int maxShapes = 3);

    /** One shape per chord for a progression, each near the one before. */
    std::vector<GuitarShape> shapeProgression (const std::vector<Chord>&);

    juce::String getDifficultyName (Difficulty);   // "Easy", "Medium", "Hard"

    /** One line on what a shape asks of the player: "An open chord with three fingers",
        "A barre chord: the index finger holds all six strings at fret 2". */
    juce::String describeGuitarShape (const GuitarShape&);

    //==============================================================================
    /** For beginners: how to play the chord on piano (notes, inversions, why the root goes at the
        bottom) and on guitar (the shapes, fingers, and what players do when a chord is hard, such
        as F or Bm). */
    juce::String explainPlaying (const Chord&);
} // namespace diagrams
} // namespace daw::gen
