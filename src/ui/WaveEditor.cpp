#include "WaveEditor.h"
#include "engine/SoundGuide.h"

namespace daw
{
namespace
{
    // 0 dB sits a little inside the edge, so a clip turned up past it still shows how far.
    constexpr float zeroDbHeight = 0.8f;

    juce::String formatDb (float db)
    {
        return (db > 0.0f ? "+" : "") + juce::String (db, 1) + " dB";
    }

    /** The loudest sample of the clip, before its gain. */
    float getRawPeak (const AudioClip& clip)
    {
        float peak = 0.0f;

        for (size_t b = 0; b < clip.peakMax.size(); ++b)
            peak = juce::jmax (peak, clip.peakMax[b], -clip.peakMin[b]);

        return peak;
    }
}

//==============================================================================
void WaveformView::setClip (AudioClip* c)
{
    if (c != clip)
    {
        viewStart = viewLength = 0.0;
        tipSample = -1;
    }

    clip = c;
    repaint();

    if (onZoomChanged)
        onZoomChanged();
}

double WaveformView::getVisibleLength() const
{
    const double total = clip != nullptr ? (double) clip->audio.getNumSamples() : 0.0;
    return viewLength > 0.0 ? juce::jmin (viewLength, total) : total;
}

float WaveformView::sampleToX (double sample) const
{
    const double length = getVisibleLength();
    return length > 0.0 ? (float) ((sample - viewStart) / length * getWidth()) : 0.0f;
}

double WaveformView::xToSample (float x) const
{
    return viewStart + x / juce::jmax (1, getWidth()) * getVisibleLength();
}

float WaveformView::beatToX (double beat) const
{
    return sampleToX (engine.getTempoMap().getSecondsBetween (clip->startBeat, beat) * clip->sourceSampleRate);
}

void WaveformView::showSeconds (double startSeconds, double lengthSeconds)
{
    if (clip != nullptr)
        setView (startSeconds * clip->sourceSampleRate, lengthSeconds * clip->sourceSampleRate);
}

void WaveformView::setView (double start, double length)
{
    if (clip == nullptr)
        return;

    const double total = clip->audio.getNumSamples();
    length = juce::jlimit (juce::jmin (total, getWidth() / (double) maxPixelsPerSample), total, length);
    start = juce::jlimit (0.0, total - length, start);

    if (juce::approximatelyEqual (start, viewStart) && juce::approximatelyEqual (length, getVisibleLength()))
        return;

    viewStart = start;
    viewLength = length >= total ? 0.0 : length;
    tipSample = -1;
    repaint();

    if (onZoomChanged)
        onZoomChanged();
}

void WaveformView::zoomAround (float x, double factor)
{
    if (clip == nullptr || factor <= 0.0)
        return;

    const double anchor = xToSample (x), length = getVisibleLength() / factor;
    setView (anchor - x / juce::jmax (1, getWidth()) * length, length);
}

bool WaveformView::isZoomedIn() const
{
    return clip != nullptr && viewLength > 0.0;
}

bool WaveformView::canZoomIn() const
{
    return clip != nullptr && getVisibleLength() * maxPixelsPerSample > getWidth() + 1.0;
}

int WaveformView::getFocusSample() const
{
    if (clip == nullptr)
        return 0;

    const double beat = engine.getPositionBeats();

    if (engine.isPlaying() && beat >= clip->startBeat && beat < clip->getEndBeat (engine.getTempoMap()))
        return (int) (engine.getTempoMap().getSecondsBetween (clip->startBeat, beat) * clip->sourceSampleRate);

    return (int) (viewStart + getVisibleLength() * 0.5);
}

std::vector<float> WaveformView::readMono (const AudioClip& clip, int start, int numSamples)
{
    std::vector<float> mono ((size_t) juce::jmax (0, numSamples), 0.0f);
    const auto& audio = clip.audio;
    const int from = juce::jmax (0, start), to = juce::jmin (audio.getNumSamples(), start + numSamples);
    const float gain = clip.getGain() / (float) juce::jmax (1, audio.getNumChannels());

    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
    {
        const float* data = audio.getReadPointer (ch);

        for (int i = from; i < to; ++i)
            mono[(size_t) (i - start)] += data[i] * gain;
    }

    return mono;
}

void WaveformView::paint (juce::Graphics& g)
{
    g.fillAll (theme::lane);

    if (clip == nullptr || clip->audio.getNumSamples() == 0)
        return;

    const auto bounds = getLocalBounds().toFloat();
    const float midY = bounds.getCentreY(), zeroDb = bounds.getHeight() * 0.5f * zeroDbHeight;

    // Bar lines and numbers of the song, and beat lines when there is room.
    const auto& tempo = engine.getTempoMap();
    const double start = tempo.getBeatAfter (clip->startBeat, viewStart / clip->sourceSampleRate);
    const double end = tempo.getBeatAfter (clip->startBeat, (viewStart + getVisibleLength()) / clip->sourceSampleRate);
    const bool showBeats = beatToX (start + 1.0) - beatToX (start) > 12.0f;
    g.setFont (juce::FontOptions (11.0f));

    for (double beat = std::ceil (start - 1e-6); beat < end; beat += 1.0)
    {
        const bool isBar = std::fmod (beat, 4.0) < 1e-6;

        if (! isBar && ! showBeats)
            continue;

        const float x = beatToX (beat);
        g.setColour (isBar ? theme::gridBar : theme::grid);
        g.fillRect (x, 0.0f, 1.0f, bounds.getHeight());

        if (isBar)
        {
            g.setColour (theme::textDim);
            g.drawText (juce::String ((int) (beat / 4.0) + 1), juce::Rectangle<float> (x + 4.0f, 2.0f, 40.0f, 14.0f), juce::Justification::centredLeft);
        }
    }

    const auto& audio = clip->audio;
    const int numSamples = audio.getNumSamples(), numBlocks = (int) clip->peakMax.size();
    const double samplesPerPixel = getVisibleLength() / bounds.getWidth();
    const float gain = clip->getGain();
    const auto colour = theme::accent.brighter (0.3f);
    auto yOf = [&] (float value) { return juce::jlimit (0.0f, bounds.getBottom(), midY - value * zeroDb); };

    if (samplesPerPixel >= 1.0)
    {
        // The waveform: min and max of every column, both channels together, at the clip's gain.
        for (int x = 0; x < getWidth(); ++x)
        {
            const int s0 = (int) (viewStart + x * samplesPerPixel);
            const int s1 = juce::jlimit (s0 + 1, numSamples, (int) (viewStart + (x + 1) * samplesPerPixel));

            if (s0 >= numSamples)
                break;

            float lo = 0.0f, hi = 0.0f;

            if (samplesPerPixel >= AudioClip::peakBlockSize)
            {
                // Zoomed out: the overview is fine enough.
                for (int b = s0 / AudioClip::peakBlockSize; b < juce::jmin (numBlocks, s1 / AudioClip::peakBlockSize + 1); ++b)
                {
                    lo = juce::jmin (lo, clip->peakMin[(size_t) b]);
                    hi = juce::jmax (hi, clip->peakMax[(size_t) b]);
                }
            }
            else
            {
                for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                {
                    const auto range = juce::FloatVectorOperations::findMinAndMax (audio.getReadPointer (ch, s0), s1 - s0);
                    lo = juce::jmin (lo, range.getStart());
                    hi = juce::jmax (hi, range.getEnd());
                }
            }

            lo *= gain;
            hi *= gain;
            const float top = midY - juce::jmin (hi, 1.0f) * zeroDb, bottom = midY - juce::jmax (lo, -1.0f) * zeroDb;
            g.setColour (colour);
            g.drawVerticalLine (x, top, bottom + 1.0f);

            // Past 0 dB: red, up to the edge.
            g.setColour (theme::meterRed);

            if (hi > 1.0f)
                g.drawVerticalLine (x, yOf (hi), top);

            if (lo < -1.0f)
                g.drawVerticalLine (x, bottom, yOf (lo));
        }
    }
    else
    {
        // Zoomed in past one sample per pixel: the line through the samples (the channels averaged),
        // and the samples themselves once there is room for them.
        const int first = juce::jmax (0, (int) viewStart - 1);
        const int last = juce::jmin (numSamples - 1, (int) std::ceil (viewStart + getVisibleLength()) + 1);
        const auto samples = readMono (*clip, first, last - first + 1);
        juce::Path line;

        for (int i = first; i <= last; ++i)
        {
            const juce::Point<float> p (sampleToX (i), yOf (samples[(size_t) (i - first)]));

            if (i == first)
                line.startNewSubPath (p);
            else
                line.lineTo (p);
        }

        g.setColour (colour);
        g.strokePath (line, juce::PathStrokeType (1.5f));

        if (1.0 / samplesPerPixel >= 6.0)
        {
            for (int i = first; i <= last; ++i)
            {
                const float value = samples[(size_t) (i - first)];
                g.setColour (std::abs (value) > 1.0f ? theme::meterRed : colour);
                g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ sampleToX (i), yOf (value) }));
            }
        }
    }

    // The centre line and 0 dB.
    g.setColour (theme::gridBar);
    g.fillRect (0.0f, midY, bounds.getWidth(), 1.0f);
    const float dashes[] = { 4.0f, 4.0f };

    for (const float y : { midY - zeroDb, midY + zeroDb })
        g.drawDashedLine (juce::Line<float> (0.0f, y, bounds.getWidth(), y), dashes, 2, 1.0f);

    g.setColour (theme::textDim);
    g.drawText ("0 dB", juce::Rectangle<float> (bounds.getRight() - 44.0f, midY - zeroDb - 15.0f, 40.0f, 14.0f), juce::Justification::centredRight);

    // How much time the width shows, once zoomed in.
    if (isZoomedIn())
        g.drawText (soundguide::formatTime (getVisibleLength() / clip->sourceSampleRate) + " across"
                        + (samplesPerPixel < 1.0 ? ", " + juce::String (juce::roundToInt (getVisibleLength())) + " samples" : juce::String()),
                    juce::Rectangle<float> (bounds.getRight() - 304.0f, bounds.getBottom() - 16.0f, 300.0f, 14.0f), juce::Justification::centredRight);

    // Playhead, where updatePlayhead() last put it.
    g.setColour (theme::playhead);
    g.fillRect (playheadX, 0.0f, 1.5f, bounds.getHeight());
}

void WaveformView::updatePlayhead()
{
    if (clip == nullptr)
        return;

    const double beat = engine.getPositionBeats();
    const bool inside = engine.isPlaying() && beat >= clip->startBeat && beat <= clip->getEndBeat (engine.getTempoMap());
    const float x = inside ? beatToX (beat) : -100.0f;

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 6, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 6, getHeight());
}

void WaveformView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (clip == nullptr)
        return;

    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        if (wheel.deltaY != 0.0f)
            zoomAround (e.position.x, wheel.deltaY > 0 ? 1.12 : 1.0 / 1.12);

        return;
    }

    // Any other scrolling moves along the clip.
    const float delta = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
    setView (viewStart - delta * getVisibleLength() * 0.5, getVisibleLength());
}

void WaveformView::mouseMagnify (const juce::MouseEvent& e, float scale)
{
    zoomAround (e.position.x, scale);
}

void WaveformView::mouseDoubleClick (const juce::MouseEvent& e)
{
    // In and out: four times closer, or back to the whole clip.
    if (isZoomedIn())
        setView (0.0, clip->audio.getNumSamples());
    else
        zoomAround (e.position.x, 4.0);
}

juce::String WaveformView::getTooltip()
{
    if (clip == nullptr || clip->audio.getNumSamples() == 0)
        return {};

    // What is just before the mouse and right after it, over at least 30 ms or a few pixels.
    const double rate = clip->sourceSampleRate;
    const int sample = (int) xToSample (getMouseXYRelative().toFloat().x);
    const double length = juce::jmax (0.03 * rate, 4.0 * getVisibleLength() / juce::jmax (1, getWidth()));

    if (tipSample >= 0 && std::abs (sample - tipSample) < length * 0.25 && juce::approximatelyEqual (tipLength, length)
        && juce::approximatelyEqual (tipGain, clip->gainDb))
        return tip;

    auto rmsDb = [] (const std::vector<float>& audio)
    {
        double sum = 0.0;
        for (const float v : audio)
            sum += (double) v * v;

        return juce::Decibels::gainToDecibels ((float) std::sqrt (sum / (double) juce::jmax ((size_t) 1, audio.size())), -100.0f);
    };

    const int n = (int) length;
    const auto before = readMono (*clip, sample - n, n), here = readMono (*clip, sample, n);
    float peak = 0.0f;

    for (const float v : here)
        peak = juce::jmax (peak, std::abs (v));

    const auto pitchWindow = readMono (*clip, sample, (int) (0.05 * rate));
    const float pitch = soundguide::estimatePitch (pitchWindow.data(), (int) pitchWindow.size(), rate);

    tip = soundguide::describeMoment (rmsDb (before), rmsDb (here), juce::Decibels::gainToDecibels (peak, -100.0f), pitch)
          + "\n\nCmd + scroll (or + and -, or double-click) zooms in and out. \"What is sound?\" explains what you see.";
    tipSample = sample;
    tipLength = length;
    tipGain = clip->gainDb;
    return tip;
}

//==============================================================================
WaveEditor::WaveEditor (Engine& e, Selection& s) : engine (e), selection (s)
{
    title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (title);

    zoomInButton.onClick = [this] { waveform.zoomAroundCentre (2.0); };
    zoomOutButton.onClick = [this] { waveform.zoomAroundCentre (0.5); };
    zoomInButton.setTooltip ("Zoom in, down to the single samples (or Cmd/Ctrl + scroll wheel, or double-click the waveform)");
    zoomOutButton.setTooltip ("Zoom out, up to the whole clip");

    for (auto* b : { &zoomInButton, &zoomOutButton })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    waveform.onZoomChanged = [this] { updateZoomButtons(); };

    soundGuideButton.setWantsKeyboardFocus (false);
    soundGuideButton.setTooltip ("A guide to what sound is, from moving air to the numbers in a WAV file, with this clip as the example");
    soundGuideButton.onClick = [this] { if (onShowSoundGuide) onShowSoundGuide(); };
    addAndMakeVisible (soundGuideButton);

    reverseButton.setWantsKeyboardFocus (false);
    reverseButton.setTooltip ("Plays the clip backwards, as in its right-click menu. Click again to turn it back.");
    reverseButton.onClick = [this]
    {
        if (clip != nullptr && ! engine.isRecording())
            engine.reverseClip (*clip);
    };
    addAndMakeVisible (reverseButton);

    gainLabel.setColour (juce::Label::textColourId, theme::textDim);
    gainLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (gainLabel);

    gainSlider.textFromValueFunction = [] (double value) { return formatDb ((float) value); };
    gainSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters ("-+.0123456789").getDoubleValue(); };
    gainSlider.setScrollWheelEnabled (false);   // scrolling never changes a value
    gainSlider.setRange (AudioClip::minGainDb, AudioClip::maxGainDb, 0.1);
    gainSlider.setDoubleClickReturnValue (true, 0.0);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    gainSlider.setTooltip ("Makes the whole clip louder or quieter, before the track's effects and fader. "
                           "+6 dB is about twice the level and -6 dB about half. Double-click to go back to 0 dB.");
    gainSlider.onValueChange = [this]
    {
        if (clip != nullptr)
            engine.setClipGain (*clip, (float) gainSlider.getValue());
    };
    addAndMakeVisible (gainSlider);

    closeButton.setWantsKeyboardFocus (false);
    closeButton.setTooltip ("Close the clip editor, to give the tracks more room. Double-click a clip to open it again.");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    info.setFont (juce::FontOptions (12.5f));
    info.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (info);
    addAndMakeVisible (waveform);

    selection.addChangeListener (this);
    engine.addChangeListener (this);
    showSelectedClip();
    startTimerHz (30);
}

WaveEditor::~WaveEditor()
{
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

void WaveEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, headerHeight - 1, getWidth(), 1);
}

void WaveEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (8, 4);

    closeButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (12);
    gainSlider.setBounds (header.removeFromRight (240));
    gainLabel.setBounds (header.removeFromRight (44));
    header.removeFromRight (16);
    reverseButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (16);
    zoomInButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (2);
    zoomOutButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (8);
    soundGuideButton.setBounds (header.removeFromRight (144));
    header.removeFromRight (8);
    title.setBounds (header);

    info.setBounds (r.removeFromBottom (infoHeight).reduced (8, 0));
    waveform.setBounds (r);
    updateZoomButtons();
}

void WaveEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    showSelectedClip();
}

void WaveEditor::showSelectedClip()
{
    auto* selected = selection.getClip();
    clip = selected != nullptr && selected->kind == Clip::Kind::audio ? static_cast<AudioClip*> (selected) : nullptr;

    if (auto* track = selection.getTrack(); track != nullptr && clip != nullptr)
        title.setText (track->name + "  -  " + clip->name, juce::dontSendNotification);
    else
        title.setText ({}, juce::dontSendNotification);

    gainSlider.setValue (clip != nullptr ? clip->gainDb : 0.0, juce::dontSendNotification);
    gainSlider.setEnabled (clip != nullptr);
    reverseButton.setEnabled (clip != nullptr && ! engine.isRecording());
    info.setText (describeClip(), juce::dontSendNotification);
    waveform.setClip (clip);
}

void WaveEditor::updateZoomButtons()
{
    zoomInButton.setEnabled (waveform.canZoomIn());
    zoomOutButton.setEnabled (waveform.isZoomedIn());
}

juce::String WaveEditor::describeClip() const
{
    if (clip == nullptr)
        return {};

    const double seconds = clip->getLengthSeconds();
    const int channels = clip->audio.getNumChannels();
    juce::String text;
    text << juce::String (seconds, 1) << " s (" << juce::String (clip->getLengthBeats (engine.getTempoMap()) / 4.0, 1) << " bars)   "
         << juce::String (clip->sourceSampleRate / 1000.0, 1) << " kHz " << (channels > 1 ? "stereo" : "mono");

    const float peak = getRawPeak (*clip) * clip->getGain();

    if (peak <= 0.0f)
        return text << "   Silent";

    const float peakDb = juce::Decibels::gainToDecibels (peak);
    text << "   Peak " << formatDb (peakDb);

    if (peak > 1.0f)
        text << ", above 0 dB (the red parts), so it may clip (distort) on its way out. Turning the gain down by "
             << juce::String (std::ceil (peakDb * 10.0f) / 10.0f, 1) << " dB brings it under.";

    return text;
}
} // namespace daw
