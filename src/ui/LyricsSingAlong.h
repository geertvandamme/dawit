#pragma once

#include "engine/Engine.h"
#include "generate/SingAlong.h"
#include "Theme.h"

namespace daw
{
/**
    The lyrics to read while recording vocals: a window that shows the line to sing now in large
    letters, readable from the microphone, the next line
    under it and the line before fading out above, following the song as it plays (what's heard,
    with the audio output's latency taken off) and the loop range. The colour runs over each
    syllable while it's sung; before a line that comes after a rest, four dots count down the last
    beats. Above: the section and the bar ("Chorus 2 · bar 3 of 8"). Bars without singing say when
    the next line comes. The text size and whether the line before shows are remembered between
    sessions. The window can be resized and made full screen; the words grow with it.
*/
class LyricsSingAlong final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit LyricsSingAlong (Engine&);
    ~LyricsSingAlong() override;

    /** "Tips..." is a menu: its first item calls this, to open the lyrics guide's topic on singing along. */
    std::function<void()> onOpenGuide;

    /** Its second item calls this, to open the singing guide at Posture and breathing. */
    std::function<void()> onOpenSingingGuide;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** A syllable placed in a laid-out line: its text (with the space after a word), where it is,
        and on which row when the line wraps. */
    struct Piece
    {
        juce::String text;
        size_t syllable = 0;
        int row = 0;
        float x = 0.0f, width = 0.0f;
    };

    /** A line laid out to fit a width: one row, or two when one would make the letters too small. */
    struct Layout
    {
        std::vector<Piece> pieces;
        int rows = 1;
        float fontSize = 0.0f;
        std::array<float, 2> rowWidths {};
    };

    /** Lays out a line at up to `maxSize`, centred in `width`. */
    static Layout layOut (const LyricLine&, float maxSize, float width, float maxHeight);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    /** Follows the playhead: everything when the line changes, the line being sung and the count-in otherwise. */
    void update (bool force);

    float getTextScale() const;

    void paintLine (juce::Graphics&, const LyricLine&, juce::Rectangle<int> area, float maxSize, bool current, juce::Colour);
    void paintWait (juce::Graphics&);

    Engine& engine;
    int textSize = 1;            // 0 large, 1 larger, 2 largest
    bool showPrevious = true;

    double heardBeat = 0.0;
    gen::singalong::Moment moment;
    gen::playalong::Moment position;   // the section and bar
    int countIn = 0;
    bool wasPlaying = false;

    juce::TextButton playButton, tipsButton { "Tips..." };
    juce::ComboBox sizeBox;
    juce::ToggleButton previousToggle { "Line before" };

    // Where each part is drawn, set by resized().
    juce::Rectangle<int> positionArea, previousArea, waitArea, currentArea, nextArea;

    static constexpr int width = 1000, height = 700;
};
} // namespace daw
