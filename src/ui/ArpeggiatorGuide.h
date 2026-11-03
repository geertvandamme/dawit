#pragma once

#include "engine/BuiltInArpeggiator.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Arpeggiator: what each control and part of its window does, with live numbers,
    and the pictures both the window and the "What is an arpeggiator?" guide use: the keyboard with the keys held and
    the note playing, the row of steps over the song's beats, and the last 2 bars of notes it played.
*/
namespace daw::arptext
{
/** The parts of the window with a topic of their own; the knobs are 0 up (arp::Knob), the shared topics fxui::CommonTopic.
    The five brushes are `kindButton` plus the step kind. */
enum Topic { keyboard = 50, stepRow, notesView, kindButton };

/** The guide page *Writing the arpeggio out as notes* (0-based). */
constexpr int writeOutPage = 10;

inline const juce::Colour colour { 0xff3dc9b0 };

/** What a kind of step is drawn in: teal for a note, violet for the chord, blue for the bass, a dark gap for a rest, grey for a tie. */
juce::Colour getKindColour (arp::StepKind);

juce::String getTitle (int topic);

/** What a control or part of the window does: the tooltip texts of the plan. */
juce::String describe (int topic);

/** Typical settings and when they suit, for a knob. */
juce::String getTypical (int topic);

/** What these settings mean in numbers, at the tempo the arpeggiator uses. */
juce::String describeSettings (int topic, const arp::Settings&, double bpm);

//==============================================================================
/** Where the 16 cells of the step row sit, with room above for the step numbers and below for the beats. */
struct StepGrid
{
    juce::Rectangle<float> cells;
    float cellWidth = 1.0f;

    juce::Rectangle<float> getCell (int step) const { return { cells.getX() + cellWidth * (float) step, cells.getY(), cellWidth, cells.getHeight() }; }

    /** The step under a point, or -1 outside the row. */
    int getStepAt (juce::Point<float>) const;
};

StepGrid layoutSteps (juce::Rectangle<float> bounds);

/** Draws the steps over the beat and bar lines: the kind of each with a letter, accents marked, the playing step (-1 for none) lit and the hovered one (-1) outlined. */
void drawSteps (juce::Graphics&, const StepGrid&, const arp::Settings&, int playingStep, int hoveredStep, bool labels);

/** "Step 3 of 16 · Chord · accented. ...": what a cell is and how to change it, for its tooltip. */
juce::String getStepDescription (const arp::Settings&, int step, arp::StepKind brush);

//==============================================================================
/** The keys it follows and the notes it sounds, as bit masks (bit p % 64 of word p / 64). */
struct KeyboardState
{
    std::array<std::uint64_t, 2> held {}, sounding {};
    int chordPitchClasses = 0;   // the chord track's chord of the bar, 0 for none
};

/** Reads the processor's atomics. */
KeyboardState getKeyboardState (const BuiltInArpeggiator&);

/** The chord's name over a keyboard picture: "Am7", the note names, or a hint when nothing is held. */
juce::String getHeldChordName (const KeyboardState&);

/** A keyboard strip: the held keys outlined and named by their place in the chord (R, 3, 5, 7), the notes sounding lit. */
void drawKeyboard (juce::Graphics&, juce::Rectangle<float>, const KeyboardState&);

/** The steps of the last 2 bars, oldest first (at most 64 beats' worth). */
std::vector<arp::StepRecord> getLastBars (BuiltInArpeggiator&);

/** The last 2 bars of notes as a small piano roll against the beats. */
void drawLastBars (juce::Graphics&, juce::Rectangle<float>, const std::vector<arp::StepRecord>&, const arp::Settings&, double nowBeat);

/** The pages of "What is an arpeggiator?", with pictures that follow the arpeggiator's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInArpeggiator&);
} // namespace daw::arptext
