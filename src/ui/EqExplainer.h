#pragma once

#include "engine/BuiltInEq.h"
#include "Theme.h"

namespace daw
{
namespace eqtext
{
    /** What the mouse is on, so the explainer can tell about it. */
    enum class Topic { overview, band, type, frequency, gain, q, slope, power, solo, output, match, presets, instruments };

    juce::Colour getBandColour (int band);

    /** "320 Hz", "2.50 kHz", "12.0 kHz". */
    juce::String formatFrequency (double hz);

    /** "+3.0 dB", "-4.5 dB", "0.0 dB". */
    juce::String formatGain (double db);

    /** The name and character of the frequency range around `hz`: sub-bass, bass, low mids, ... */
    juce::String describeRegion (double hz);

    /** "close to the note A2", for the range of a piano; empty above it. */
    juce::String describeNote (double hz);

    juce::String describeType (eq::BandType);
    juce::String howToUse (eq::BandType);
    juce::String whatQDoes (eq::BandType);

    /** What these settings do, in numbers: the range a bell works on, how far a cut has gone an
        octave further, the bump of a resonant cut, ... */
    juce::String describeSettings (const eq::Band&, double sampleRate);
} // namespace eqtext

//==============================================================================
/**
    The right-hand side of the EQ's window: explains whatever the mouse is on (a band, one of its
    controls, a starting point, an instrument of the overview), with live numbers for the current
    settings and a picture of what Q does to that type. It also runs "Find a bad frequency", a short
    guided exercise. Scroll it when the text runs long.
*/
class EqExplainer final : public juce::Component
{
public:
    explicit EqExplainer (BuiltInEq&);
    ~EqExplainer() override;

    void showOverview();
    void showBand (int band, eqtext::Topic);
    void showTopic (eqtext::Topic);
    void showPreset (int presetIndex);

    /** An instrument of eq::getInstruments(), and one of its ranges or -1. */
    void showInstrument (int instrument, int zone);

    /** Sweep a narrow boost to find the frequency that sounds worst, then turn it into a cut. */
    void startLesson (int band);
    bool isInLesson() const noexcept { return lessonStep > 0; }
    int getFocusedBand() const noexcept { return lessonStep > 0 ? lessonBand : focusBand; }

    /** Called when a lesson starts or ends, so the editor can update its button. */
    std::function<void()> onLessonChanged;

    /** Rewrites the text when the settings it talks about changed. Called by the editor's timer. */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::AttributedString buildText() const;
    void drawQPicture (juce::Graphics&, juce::Rectangle<float>) const;
    void endLesson (bool restore);

    BuiltInEq& processor;
    eqtext::Topic topic = eqtext::Topic::overview;
    int focusBand = -1, presetIndex = -1, instrumentIndex = -1, instrumentZone = -1;

    int lessonStep = 0, lessonBand = 2;   // 0: no lesson, 1: sweeping a boost, 2: listening to the cut
    eq::Band lessonSaved;
    juce::TextButton lessonNext, lessonCancel;

    juce::String shownText, shownPage;
    juce::AttributedString shownAttributed;
    bool showPicture = false;
    juce::TextLayout layout;
    juce::Rectangle<int> textArea, pictureArea;
    float scroll = 0.0f;
};
} // namespace daw
