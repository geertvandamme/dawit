#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "engine/SoundGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    A guide to what sound is, for beginners, opened from the waveform editor: moving air, waveforms,
    amplitude, frequency, timbre, sampling, phase, phase cancellation and taking the vocals out. Each
    topic has a live picture: the air moving, the tone the guide plays (its shape, frequency, level,
    and its copy's phase or delay and polarity, set below the picture), and the audio clip in the
    waveform editor as the example, at the playhead while the song plays. The shape is the timbre also
    measures the envelope (attack, decay, sustain, release) of the note under the playhead. Checking
    in mono measures what the clip loses when left and right are added, and plays the song in mono to
    hear it. Taking the vocals out compares left minus right with Remove vocals on a typical mix and
    measures it on the clip, plays the song as left minus right to hear it, and offers Remove vocals
    for the clip. With no clip in the editor, the timbre and mono pictures use a made-up example clip.
*/
class SoundExplainer final : public juce::Component,
                             public guideprint::Source,
                             private juce::ListBoxModel,
                             private juce::Timer
{
public:
    /** The clip in the waveform editor, and the sample to look at in it. */
    struct ClipView
    {
        const AudioClip* clip = nullptr;
        int focusSample = 0;
    };

    SoundExplainer (Engine&, std::function<ClipView()> getClipView);
    ~SoundExplainer() override;

    /** Called with the clip in the waveform editor to make its karaoke version (Remove vocals). */
    std::function<void (const AudioClip&)> onRemoveVocals;

    /** Called by the Stereo guide button of Checking in mono: opens What is DawIt Stereo? at its mono page. */
    std::function<void()> onOpenStereoGuide;

    void showTopic (soundguide::Topic);

    juce::String getPrintTitle() const override { return "What is sound?"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** What the picture shows of the clip, copied from it. */
    struct ClipData
    {
        bool present = false;
        juce::String name;
        double rate = 44100.0;
        int channels = 0, numSamples = 0;
        float peakDb = -100.0f, rmsDb = -100.0f;          // the whole clip, at its gain
        std::vector<float> overviewMin, overviewMax;     // the whole clip, in columns
        int focusSample = 0;
        std::vector<float> window;                       // mono, from the focus on
        float pitchHz = 0.0f;
        std::vector<float> spectrumDb;                   // from the focus on, per FFT bin
        bool example = false;                            // the made-up example clip, not one from the editor
        bool envelopeMeasured = false;
        soundguide::Envelope envelope;                   // the note at the focus (Timbre)
        bool monoMeasured = false;
        soundguide::MonoCheck mono;                      // the whole clip (Checking in mono)
        bool leftMinusRightMeasured = false;
        soundguide::LeftMinusRight leftMinusRight;      // the whole clip, for taking the vocals out
    };

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void timerCallback() override;

    void updateTone();
    void updateClip();
    void updateControls();
    const AudioClip& getExampleClip();

    Engine& engine;
    std::function<ClipView()> getClipView;
    soundguide::Topic topic = soundguide::Topic::air;

    juce::Label title, hintLabel;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextButton playButton { "Play tone" };
    juce::ComboBox shapeBox;
    juce::Label frequencyLabel { {}, "Frequency" }, levelLabel { {}, "Level" }, phaseLabel { {}, "Phase" }, delayLabel { {}, "Delay" };
    juce::Slider frequencySlider, levelSlider, phaseSlider, delaySlider;
    juce::ToggleButton flipButton { "Flip the copy" };
    juce::ToggleButton leftMinusRightButton { "Hear left minus right" };
    juce::ToggleButton monoButton { "Hear in mono" };
    GuideButton stereoGuideButton { "Mono in the Stereo guide..." };
    juce::TextButton removeVocalsButton { "Remove vocals from this clip..." };
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::sound };
    std::optional<soundguide::Topic> topicBeforePrinting;

    // What updateClip() last measured the whole clip for, so it is measured again only when it changes.
    const AudioClip* measuredClip = nullptr;
    int measuredSamples = -1;
    float measuredGain = 0.0f;
    const AudioClip* measuredStereoClip = nullptr;   // what left minus right was last measured on
    int measuredStereoSamples = -1;
    const AudioClip* measuredMonoClip = nullptr;     // what Checking in mono last measured
    int measuredMonoSamples = -1;
    const AudioClip* envelopeClip = nullptr;         // what the envelope was last measured on, and where
    int envelopeSamples = -1, envelopeFocus = -1;
    float envelopeGain = 0.0f;
    std::unique_ptr<AudioClip> exampleClip;          // made up, for when no clip is in the editor
    ClipData clipData;
    int ticks = 0;

    static constexpr int width = 1000, height = 790, pictureHeight = 340;
};
} // namespace daw
