#pragma once

#include <juce_dsp/juce_dsp.h>

#include "EqGuide.h"
#include "GuideButton.h"

namespace daw
{
/**
    The curve of DawIt EQ over the spectrum of the track: the frequency runs from 20 Hz to 20 kHz
    left to right, the gain from -18 to +18 dB. Each band is a numbered dot to drag (sideways:
    frequency, up and down: gain), scroll (Q), right-click (type) or double-click (reset). Holding
    Alt while dragging solos the band.

    Instead of the spectrum it can show where common instruments live: a row per instrument with
    the range of its notes, its overtones, and the ranges where problems hide or a boost helps.
    The instrument under the mouse stands out and the others are greyed out; a click keeps it that
    way, whatever the mouse is on, until the next click.
*/
class EqGraph final : public juce::Component
{
public:
    explicit EqGraph (BuiltInEq&);

    /** The band under the mouse, or -1 when it leaves the dots. */
    std::function<void (int band)> onHover;

    /** The instrument row under the mouse (an index of eq::getInstruments()), and the range of it
        the mouse is on, or -1. When the mouse leaves the rows, the clicked instrument, if any. */
    std::function<void (int instrument, int zone)> onInstrumentHover;

    void setShowingInstruments (bool);
    bool isShowingInstruments() const noexcept { return showingInstruments; }

    /** Takes the newest samples from the EQ and updates the spectrum. Called by the editor's timer. */
    void updateSpectrum();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> getPlotArea() const;
    float frequencyToX (double hz) const;
    double xToFrequency (float x) const;
    float gainToY (double db) const;
    double yToGain (float y) const;
    juce::Point<float> getHandlePosition (int band) const;
    int findHandle (juce::Point<float>) const;
    double getSampleRate() const;
    void showBandMenu (int band);
    void setParameter (int band, const char* what, float value);

    void paintGrid (juce::Graphics&, juce::Rectangle<float>);
    void paintSpectrum (juce::Graphics&, juce::Rectangle<float>);
    void paintInstruments (juce::Graphics&, juce::Rectangle<float>);
    juce::Rectangle<float> getInstrumentRow (int instrument) const;
    std::pair<int, int> findInstrument (juce::Point<float>) const;
    void updateInstrumentHover (juce::Point<float>, bool onDot);
    void paintCurves (juce::Graphics&, juce::Rectangle<float>);

    BuiltInEq& processor;

    // The spectrum: the last fftSize samples before and after the EQ, and the level per bin in dB.
    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> ringBefore, ringAfter, fftData, readBefore, readAfter;
    int ringPosition = 0;
    std::vector<float> levelBefore, levelAfter;

    int hovered = -1, dragging = -1, soloBeforeDrag = -1;
    juce::Point<float> dragOffset;

    bool showingInstruments = false;
    int hoveredInstrument = -1, hoveredZone = -1;
    int selectedInstrument = -1;   // clicked: stays in focus, the others grey, until the next click
};

//==============================================================================
/** The window of DawIt EQ: the graph, a column of controls per band, and the explainer. */
class BuiltInEqEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BuiltInEqEditor (BuiltInEq&);
    ~BuiltInEqEditor() override;

    /** Opens "What is EQ?", or brings it to the front, at a page (-1 keeps the one it shows). */
    void openGuide (int page = -1);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;

private:
    class BandControls;

    void timerCallback() override;

    fxui::PluginMode pluginMode { *this };
    BuiltInEq& eqProcessor;
    EqGraph graph { eqProcessor };
    EqExplainer explainer { eqProcessor };
    juce::OwnedArray<BandControls> bands;

    juce::Label title { {}, "DawIt EQ" };
    juce::ComboBox presets;
    juce::TextButton lessonButton { "Find a bad frequency" }, instrumentsButton { "Instruments" }, helpButton { "?" };
    GuideButton guideButton { "What is EQ?" };
    fxui::GuideHost guideHost { *this };
    juce::ToggleButton matchButton { "Match level" };
    juce::Label matchValue, outputLabel { {}, "Output" };
    juce::Slider output { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> matchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    int ticks = 0;

    static constexpr int headerHeight = 44, bandsHeight = 150, explainerWidth = 350;
};
} // namespace daw
