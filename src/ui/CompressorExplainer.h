#pragma once

#include "engine/BuiltInCompressor.h"
#include "Theme.h"

namespace daw
{
namespace comptext
{
    /** What the mouse is on, so the explainer can tell about it. The knobs come first, in the order of comp::Knob. */
    enum class Topic { threshold, ratio, knee, attack, release, makeup, mix, sidechain, detection,
                       overview, curve, history, meter, match, listen, presets };

    inline Topic getTopic (comp::Knob k) { return (Topic) (int) k; }
    inline bool isKnob (Topic t) { return (int) t < comp::numKnobs; }

    /** The curve and threshold; the gain reduction is theme::meterRed, the output theme::accent. */
    inline const juce::Colour curveColour { 0xffffa94d };

    /** What played lately, from the editor's history: over the last 4 seconds, the loudest the
        compressor heard (after its sidechain filter and detection), the loudest peak that came in,
        and the most it turned down. */
    struct Recent
    {
        float loudestDb = -100.0f, loudestInputDb = -100.0f, mostReductionDb = 0.0f;
        bool hasSignal() const { return loudestInputDb > -70.0f; }
    };

    juce::String getTitle (Topic);

    /** What a knob or part of the window does. */
    juce::String describe (Topic);

    /** Typical settings per instrument, for a knob. */
    juce::String getTypical (Topic);

    /** What these settings do, in numbers, for what played lately and at the song's tempo (0: unknown). */
    juce::String describeSettings (Topic, const comp::Settings&, const Recent&, double bpm);
} // namespace comptext

//==============================================================================
/**
    The right-hand side of the compressor's window: explains whatever the mouse is on, with live
    numbers for the current settings and what just played, typical settings per instrument, and
    pictures of what attack, release, ratio and knee do. It also runs "Set it up step by step", a
    guided walk through threshold, attack, release and level. Scroll it when the text runs long.
*/
class CompressorExplainer final : public juce::Component
{
public:
    explicit CompressorExplainer (BuiltInCompressor&);

    /** What played lately; set by the editor. */
    std::function<comptext::Recent()> getRecent;

    /** The threshold that turns the loudest of what played lately down by this much; set by the editor. */
    std::function<std::optional<float> (const comp::Settings&, float reductionDb)> findThreshold;

    void showOverview();
    void showTopic (comptext::Topic);

    /** After a starting point was picked; `thresholdFromTrack` says whether the threshold was set from what just played. */
    void showPreset (int presetIndex, bool thresholdFromTrack);

    void startLesson();
    bool isInLesson() const noexcept { return lessonStep > 0; }

    /** Called when a lesson starts or ends, so the editor can update its button. */
    std::function<void()> onLessonChanged;

    /** Rewrites the text when what it talks about changed. Called by the editor's timer. */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum class Picture { none, timing, curve };

    comptext::Recent recent() const { return getRecent != nullptr ? getRecent() : comptext::Recent(); }
    juce::AttributedString buildText() const;
    Picture getPicture() const;
    void drawTimingPicture (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCurvePicture (juce::Graphics&, juce::Rectangle<float>) const;
    void setLessonStep (int step);
    void endLesson (bool restore);
    void updateHelperButton();

    BuiltInCompressor& processor;
    comptext::Topic topic = comptext::Topic::overview;
    int presetIndex = -1;           // the starting point picked last, until the knobs are reset
    bool presetThresholdFromTrack = false;
    bool showingPreset = false;

    int lessonStep = 0;             // 0: no lesson, 1-4: threshold, attack, release, level
    comp::Settings lessonSaved;
    bool lessonSavedMatch = false;
    juce::TextButton lessonHelper, lessonNext, lessonCancel;

    juce::String shownText, shownPage;
    juce::AttributedString shownAttributed;
    Picture shownPicture = Picture::none;
    juce::TextLayout layout;
    juce::Rectangle<int> textArea, pictureArea;
    float scroll = 0.0f;
};
} // namespace daw
