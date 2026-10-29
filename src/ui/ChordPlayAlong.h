#pragma once

#include "ChordDiagramViews.h"
#include "engine/Engine.h"
#include "generate/PlayAlong.h"

namespace daw
{
/**
    Karaoke for the chords: a window that shows the chord playing right now, large, as a piano
    keyboard, a guitar chord box or both, with the next chord beside it, so you can play along on a
    real instrument from across the room. It follows the song as it plays (what's heard, with the
    audio output's latency taken off) and the loop range.

    The next chord lights up on the last beat before the change, with the keys and fingers that stay
    ringed green; a bar under the chord runs out over the beats until the change, with the bar's
    beats as dots. Above: the section and the bar in it ("Chorus 2 · bar 3 of 8"). Along the bottom:
    the chords coming, in their colours. Bars without a chord say "No chord" and when the next one
    comes. A capo shows the guitar shapes to play with it. The instrument, capo and note names are
    remembered between sessions. The window can be resized and made full screen; the diagrams grow
    with it.
*/
class ChordPlayAlong final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit ChordPlayAlong (Engine&);
    ~ChordPlayAlong() override;

    enum class Instrument { piano, guitar, both };

    /** Called when "Tips..." is clicked: the chord guide's topic on playing along. */
    std::function<void()> onOpenGuide;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void setInstrument (Instrument);
    void setCapo (int);

    /** The chord track's changes, voicings and shapes, again when the chords or the capo change. */
    void rebuild();

    /** Follows the playhead: the diagrams and text when what's shown changes, the countdown always. */
    void update (bool force);
    void showDiagrams();

    /** Fits the diagrams in an area: side by side, or `stacked` (the keyboard above the box). */
    void layoutDiagrams (juce::Rectangle<int> area, KeyboardDiagram&, GuitarDiagram&, bool stacked);

    void paintCurrent (juce::Graphics&);
    void paintNext (juce::Graphics&);
    void paintCountdown (juce::Graphics&);
    void paintComing (juce::Graphics&);

    Engine& engine;
    Instrument instrument = Instrument::both;
    int capo = 0;
    bool noteNames = true;

    std::vector<gen::playalong::Change> changes;
    ChordMap changesFrom;
    std::vector<std::vector<int>> voicings;
    std::vector<gen::diagrams::GuitarShape> shapes;
    int pianoLow = 48, pianoHigh = 76;
    int easiestCapo = 0;

    gen::playalong::Moment moment;
    bool aboutToChange = false, wasPlaying = false;

    KeyboardDiagram currentKeys, nextKeys;
    GuitarDiagram currentBox, nextBox;
    juce::TextButton playButton, pianoButton { "Piano" }, guitarButton { "Guitar" }, bothButton { "Both" }, tipsButton { "Tips..." };
    juce::ComboBox capoBox;
    juce::ToggleButton namesToggle { "Note names" };

    // Where each part is drawn, set by resized().
    juce::Rectangle<int> positionArea, currentArea, currentNameArea, currentDiagramArea, countdownArea,
                         nextArea, nextNameArea, nextDiagramArea, comingArea;

    static constexpr int width = 1000, height = 700;
};
} // namespace daw
