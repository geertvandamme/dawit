#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/Tune.h"

/**
    The pictures of DawIt Tune, drawn in one place so its window, its guide, its chip in the device bar
    and the Signal Flow window all look the same: the tuner (the note, how many cents it is out, and
    whether it is in the key), the trace (the pitch over the last seconds on the piano roll's rows) and
    the colours of the three zones.
*/
namespace daw::tunepics
{
/** DawIt Tune's own colour. */
inline const juce::Colour colour { 0xffa78bfa };

/** Green within 10 cents of a note, yellow within 25, red beyond. */
juce::Colour getZoneColour (tune::Zone);

/** The semitone nearest to a pitch and how many cents the pitch is above it (below 0: flat). */
struct Nearest { int note = 0; double cents = 0.0; };
Nearest getNearest (double midi);

/** "A3", with middle C as C4. */
juce::String getNoteName (int midi);

/** "18 cents flat", "3 cents sharp" or "in tune" for under half a cent. */
juce::String describeCents (double cents);

/** The key's name for the tuner's last line: "A minor". Empty to leave the line out. */
struct KeyInfo
{
    juce::String name;
    int mask = tune::allNotes;
};

/** The big tuner: the nearest note, the needle from -50 to +50 cents in the zone colour, the cents in words and whether the note is in the key. A dash for no clear pitch. */
void paintTuner (juce::Graphics&, juce::Rectangle<float> area, double sungMidi, const KeyInfo&);

/** What the big tuner draws, worked out by the caller: DawIt Tune builds it from a sung pitch, the Tuner window from a string reading. */
struct TunerPicture
{
    bool has = false;
    juce::String note;           // "A2": the big text; a dash when there is no note
    double cents = 0.0;          // where the needle points (clamped to -50 to +50)
    double greenCents = 10.0;    // green within this many cents, yellow within the next, red beyond
    double yellowCents = 25.0;
    juce::String line;           // the cents in words (empty: "no clear pitch" when there is no note)
    juce::String extraLine;      // under it: the key line, or whatever the caller wants; empty to leave it out
    float fade = 1.0f;           // 1 for a note sounding, falling to 0 as a held note fades
    juce::String noteInfo;       // a third line, under the extra one, in the dim colour; empty to leave it out
    float maxNoteSize = 54.0f;   // the biggest the note's name may be drawn, in points
};

/** The big tuner from a ready picture: the note, the needle with its zone bands, then up to three lines of text. */
void paintTuner (juce::Graphics&, juce::Rectangle<float> area, const TunerPicture&);

/** A tiny tuner for a chip: the note's name and a needle. */
void paintMiniTuner (juce::Graphics&, juce::Rectangle<float> area, double sungMidi, bool dim);

/** What the trace shows. */
struct Trace
{
    const std::vector<tune::Frame>* frames = nullptr;
    double framesPerSecond = 172.0;
    double seconds = 8.0;          // how much of the newest frames to show
    int keyMask = tune::allNotes;  // the key's rows are shaded
    bool showCorrected = true;
    bool showBars = true;          // bar numbers along the bottom, where the frames know the song position
    int fixedLow = -1;             // the lowest row, or -1 to centre on the recent notes
};

/** Draws the rows, the sung line (green, yellow or red by how far from a note), the corrected line in the accent colour, with gaps for no clear pitch. */
void paintTrace (juce::Graphics&, juce::Rectangle<float> area, const Trace&);

/** What is under the mouse on a trace. */
struct TraceHit
{
    bool valid = false;
    int frame = 0;
    double beat = -1.0;     // -1 while the song wasn't playing
    double sung = -1.0;
    Nearest nearest;
};

TraceHit hitTrace (juce::Rectangle<float> area, const Trace&, juce::Point<float>);

/** "Bar 9 · A3 · 18 cents flat · in G major" (no bar while the song wasn't playing). */
juce::String describeHit (const TraceHit&, const KeyInfo&);

/** The lowest row of the trace's area: the width of the left margin that holds the note names. */
constexpr float traceMargin = 30.0f;

/** Frames of made-up pitch, for the guide's pictures: `sung` and `corrected` (below 0 for gaps), one per frame. */
std::vector<tune::Frame> makeFrames (const std::vector<float>& sung, const std::vector<float>& corrected);
} // namespace daw::tunepics
