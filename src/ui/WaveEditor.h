#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/** The waveform of an audio clip, drawn at the clip's gain: the whole clip, or zoomed in down to
    single samples. The dashed lines are 0 dB, the loudest a file can hold; what goes past them is red.
    Bar lines and numbers follow the song. Hovering says what is under the mouse: silence, an attack,
    a fade, or a held sound and its pitch. */
class WaveformView final : public juce::Component, public juce::TooltipClient
{
public:
    explicit WaveformView (Engine& e) : engine (e) {}

    void setClip (AudioClip*);

    /** Zooms in (factor > 1) or out, keeping the sample at x where it is. */
    void zoomAround (float x, double factor);
    void zoomAroundCentre (double factor) { zoomAround (getWidth() * 0.5f, factor); }
    bool isZoomedIn() const;
    bool canZoomIn() const;

    /** The sample the sound guide looks at: the playhead while it plays in the clip, else the middle of what is shown. */
    int getFocusSample() const;

    /** Zooms to `lengthSeconds` of the clip starting at `startSeconds` (kept inside the clip). */
    void showSeconds (double startSeconds, double lengthSeconds);

    /** Called when the zoom changes, to update the + and - buttons. */
    std::function<void()> onZoomChanged;

    /** All channels averaged, at the clip's gain; zeros outside the clip. */
    static std::vector<float> readMono (const AudioClip&, int start, int numSamples);

    void updatePlayhead();
    void paint (juce::Graphics&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scale) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    float beatToX (double beat) const;
    float sampleToX (double sample) const;
    double xToSample (float x) const;
    double getVisibleLength() const;
    void setView (double start, double length);

    Engine& engine;
    AudioClip* clip = nullptr;
    float playheadX = -100.0f;
    double viewStart = 0.0, viewLength = 0.0;   // in samples; a length of 0 shows the whole clip

    // The last hover text, so it is worked out again only when the mouse moves on.
    juce::String tip;
    int tipSample = -1;
    double tipLength = 0.0;
    float tipGain = 0.0f;

    static constexpr float maxPixelsPerSample = 24.0f;
};

/** Bottom panel: shows the selected audio clip, with zoom, Reverse and its gain, and opens the
    "What is sound?" guide. Double-clicking an audio clip opens it; the x closes it. */
class WaveEditor final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    WaveEditor (Engine&, Selection&);
    ~WaveEditor() override;

    /** Called when the close button is clicked: hide the editor. */
    std::function<void()> onClose;

    /** Called when "What is sound?" is clicked: open the sound guide. */
    std::function<void()> onShowSoundGuide;

    /** The clip shown, or nullptr. */
    AudioClip* getClip() const noexcept { return clip; }

    /** See WaveformView::getFocusSample(). */
    int getFocusSample() const { return waveform.getFocusSample(); }

    /** Zooms to a stretch of the clip, for the screenshots' scenes. */
    void showSeconds (double startSeconds, double lengthSeconds) { waveform.showSeconds (startSeconds, lengthSeconds); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override { waveform.updatePlayhead(); }
    void showSelectedClip();
    void updateZoomButtons();
    juce::String describeClip() const;

    Engine& engine;
    Selection& selection;
    AudioClip* clip = nullptr;

    juce::Label title, gainLabel { {}, "Gain" }, info;
    juce::Slider gainSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::TextButton reverseButton { "Reverse" }, closeButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) },
                     zoomInButton { "+" }, zoomOutButton { "-" };
    GuideButton soundGuideButton { "What is sound?" };
    WaveformView waveform { engine };

    static constexpr int headerHeight = 30, infoHeight = 24;
};
} // namespace daw
