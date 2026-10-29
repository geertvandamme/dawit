#include "CompressorEditor.h"

namespace daw
{
using comptext::Topic;
using comp::Knob;

namespace
{
    constexpr float lowestDb = -60.0f;
    using Point = BuiltInCompressor::HistoryPoint;

    /** Marks a control with what the explainer should tell about it when the mouse rests on it. */
    void setTopic (juce::Component& c, Topic topic)
    {
        c.getProperties().set ("compTopic", (int) topic);
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (area, 4.0f);
    }
}

//==============================================================================
CompressorHistory::CompressorHistory (BuiltInCompressor& p) : processor (p)
{
    ring.resize (32768);
    incoming.resize (32768);
    setTopic (*this, Topic::history);
}

double CompressorHistory::getPointsPerSecond() const
{
    const double rate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    return rate / BuiltInCompressor::historyChunk;
}

void CompressorHistory::update()
{
    const int n = processor.readHistory (incoming.data(), (int) incoming.size());

    for (int i = 0; i < n; ++i)
    {
        ring[(size_t) ringPosition] = incoming[(size_t) i];
        ringPosition = (ringPosition + 1) % (int) ring.size();
    }

    filled = juce::jmin ((int) ring.size(), filled + n);
}

std::vector<Point> CompressorHistory::getNewest (int count) const
{
    count = juce::jlimit (0, filled, count);
    std::vector<Point> points ((size_t) count);

    for (int i = 0; i < count; ++i)
        points[(size_t) i] = ring[(size_t) ((ringPosition - count + i + (int) ring.size()) % (int) ring.size())];

    return points;
}

comptext::Recent CompressorHistory::getRecent() const
{
    comptext::Recent r;

    for (auto& p : getNewest ((int) (seconds * getPointsPerSecond())))
    {
        r.loudestDb = juce::jmax (r.loudestDb, p.detectorDb);
        r.loudestInputDb = juce::jmax (r.loudestInputDb, p.inputDb);
        r.mostReductionDb = juce::jmax (r.mostReductionDb, p.reductionDb);
    }

    return r;
}

void CompressorHistory::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawPanel (g, bounds);
    auto legend = bounds.removeFromTop (18.0f).reduced (8.0f, 0.0f);
    auto times = bounds.removeFromBottom (14.0f);
    auto labels = bounds.removeFromLeft (30.0f);
    auto area = bounds.reduced (0.0f, 4.0f).withTrimmedRight (8.0f);
    auto toY = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, area.getBottom(), area.getY()); };

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= lowestDb; db -= 12.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.drawText (juce::String (juce::roundToInt (db)), labels.withHeight (12.0f).withCentre ({ labels.getCentreX(), toY (db) }),
                    juce::Justification::centredRight);
    }

    g.drawText ("4 s ago", times.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
    g.drawText ("now", times.withTrimmedRight (8.0f), juce::Justification::centredRight);

    const auto s = processor.getSettings();
    const int shown = juce::jmin ((int) ring.size(), (int) (seconds * getPointsPerSecond()));
    const auto points = getNewest (shown);
    const int missing = shown - (int) points.size();
    const int columns = juce::jmax (1, (int) area.getWidth());
    const bool showDetector = s.detection == comp::Detection::rms || comp::isSidechainFilterOn (s.sidechainHz);
    juce::Path out, detector;
    bool started = false;   // a path with only its start point still counts as empty

    for (int c = 0; c < columns; ++c)
    {
        const int from = c * shown / columns - missing, to = (c + 1) * shown / columns - missing;

        if (to <= 0)
            continue;

        Point most;

        for (int i = juce::jmax (0, from); i < to; ++i)
        {
            const auto& p = points[(size_t) i];
            most.inputDb = juce::jmax (most.inputDb, p.inputDb);
            most.outputDb = juce::jmax (most.outputDb, p.outputDb);
            most.detectorDb = juce::jmax (most.detectorDb, p.detectorDb);
            most.reductionDb = juce::jmax (most.reductionDb, p.reductionDb);
        }

        const float x = area.getX() + (float) c;
        const float inY = toY (most.inputDb);
        g.setColour (theme::textDim.withAlpha (0.35f));
        g.fillRect (x, inY, 1.0f, area.getBottom() - inY);

        if (most.reductionDb > 0.05f)
        {
            g.setColour (theme::meterRed.withAlpha (0.75f));
            g.fillRect (x, inY, 1.0f, juce::jmax (1.0f, toY (most.inputDb - most.reductionDb) - inY));
        }

        if (! started)
        {
            out.startNewSubPath (x, toY (most.outputDb));
            detector.startNewSubPath (x, toY (most.detectorDb));
            started = true;
        }
        else
        {
            out.lineTo (x, toY (most.outputDb));
            detector.lineTo (x, toY (most.detectorDb));
        }
    }

    g.setColour (theme::accent);
    g.strokePath (out, juce::PathStrokeType (1.5f));

    if (showDetector)
    {
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.strokePath (detector, juce::PathStrokeType (0.8f));
    }

    const float dashes[] = { 5.0f, 4.0f };
    g.setColour (comptext::curveColour);
    g.drawDashedLine ({ area.getX(), toY (s.thresholdDb), area.getRight(), toY (s.thresholdDb) }, dashes, 2, 1.2f);

    // The legend.
    g.setFont (juce::FontOptions (10.5f));
    auto key = [&] (juce::Colour colour, const juce::String& text)
    {
        g.setColour (colour);
        g.fillRect (legend.removeFromLeft (9.0f).withSizeKeepingCentre (9.0f, 9.0f));
        legend.removeFromLeft (4.0f);
        g.setColour (theme::textDim);
        const float width = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 12.0f;
        g.drawText (text, legend.removeFromLeft (width), juce::Justification::centredLeft);
    };

    key (theme::textDim.withAlpha (0.6f), "in");
    key (theme::meterRed, "turned down");
    key (theme::accent, "out");
    key (comptext::curveColour, "threshold");

    if (showDetector)
        key (juce::Colours::white.withAlpha (0.6f), "what it hears");
}

//==============================================================================
CompressorCurve::CompressorCurve (BuiltInCompressor& p, const CompressorHistory& h) : processor (p), history (h)
{
    setTopic (*this, Topic::curve);
}

juce::Rectangle<float> CompressorCurve::getPlotArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (30.0f).withTrimmedBottom (18.0f).withTrimmedTop (8.0f).withTrimmedRight (10.0f);
}

float CompressorCurve::toX (double db) const
{
    const auto a = getPlotArea();
    return juce::jmap ((float) juce::jlimit ((double) lowestDb, 0.0, db), lowestDb, 0.0f, a.getX(), a.getRight());
}

float CompressorCurve::toY (double db) const
{
    const auto a = getPlotArea();
    return juce::jmap ((float) juce::jlimit ((double) lowestDb, 0.0, db), lowestDb, 0.0f, a.getBottom(), a.getY());
}

double CompressorCurve::fromX (float x) const
{
    const auto a = getPlotArea();
    return juce::jmap (x, a.getX(), a.getRight(), lowestDb, 0.0f);
}

double CompressorCurve::fromY (float y) const
{
    const auto a = getPlotArea();
    return juce::jmap (y, a.getBottom(), a.getY(), lowestDb, 0.0f);
}

juce::Point<float> CompressorCurve::getHandlePosition (Handle h) const
{
    const auto s = processor.getSettings();
    const double in = h == Handle::threshold ? s.thresholdDb : 0.0;
    return { toX (in), toY (in - comp::getReductionDb (s, in)) };
}

CompressorCurve::Handle CompressorCurve::findHandle (juce::Point<float> position) const
{
    for (auto h : { Handle::threshold, Handle::ratio })
        if (getHandlePosition (h).getDistanceFrom (position) < 11.0f)
            return h;

    return Handle::none;
}

void CompressorCurve::paint (juce::Graphics& g)
{
    const auto s = processor.getSettings();
    const auto area = getPlotArea();
    drawPanel (g, getLocalBounds().toFloat());

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= lowestDb; db -= 12.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);
        g.fillRect (toX (db), area.getY(), 1.0f, area.getHeight());
        g.setColour (theme::textDim);
        g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (26.0f, 12.0f).withCentre ({ 15.0f, toY (db) }), juce::Justification::centredRight);
        g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ toX (db), area.getBottom() + 9.0f }),
                    juce::Justification::centred);
    }

    g.drawText ("out", area.withHeight (14.0f).withTrimmedLeft (4.0f), juce::Justification::centredLeft);
    g.drawText ("in (dB)", area.withTop (area.getBottom() - 14.0f).withTrimmedRight (4.0f), juce::Justification::centredRight);

    // The knee: where the ratio fades in.
    if (s.kneeDb > 0.5f)
    {
        g.setColour (comptext::curveColour.withAlpha (0.08f));
        const float left = toX (s.thresholdDb - s.kneeDb / 2.0f), right = toX (s.thresholdDb + s.kneeDb / 2.0f);
        g.fillRect (left, area.getY(), right - left, area.getHeight());
    }

    const float dashes[] = { 4.0f, 3.0f };
    g.setColour (theme::gridBar);
    g.drawDashedLine ({ area.getX(), area.getBottom(), area.getRight(), area.getY() }, dashes, 2, 1.0f);

    // The curve, and in red what it turns down.
    juce::Path curvePath, removed;
    removed.startNewSubPath (area.getX(), area.getBottom());

    for (int x = 0; x <= (int) area.getWidth(); ++x)
    {
        const double in = fromX (area.getX() + (float) x);
        const float y = toY (in - comp::getReductionDb (s, in));
        x == 0 ? curvePath.startNewSubPath (area.getX(), y) : curvePath.lineTo (area.getX() + (float) x, y);
        removed.lineTo (area.getX() + (float) x, y);
    }

    removed.lineTo (area.getRight(), area.getY());
    removed.closeSubPath();
    g.setColour (theme::meterRed.withAlpha (0.18f));
    g.fillPath (removed);

    g.setColour (comptext::curveColour.withAlpha (0.4f));
    g.fillRect (toX (s.thresholdDb), area.getY(), 1.0f, area.getHeight());
    g.setColour (comptext::curveColour);
    g.strokePath (curvePath, juce::PathStrokeType (2.2f));

    // The track as it plays: where it is now, and a short trail behind.
    const auto trail = history.getNewest ((int) (0.3 * history.getPointsPerSecond()));

    for (size_t i = 0; i < trail.size(); i += 4)
    {
        const auto& p = trail[i];

        if (p.detectorDb < lowestDb)
            continue;

        const float age = (float) i / (float) juce::jmax ((size_t) 1, trail.size());
        g.setColour (juce::Colours::white.withAlpha (0.1f + 0.35f * age));
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre ({ toX (p.detectorDb), toY (p.detectorDb - p.reductionDb) }));
    }

    float nowReduction = 0.0f;

    if (! trail.empty() && trail.back().detectorDb >= lowestDb)
    {
        const auto& p = trail.back();
        nowReduction = p.reductionDb;
        const auto dot = juce::Point<float> (toX (p.detectorDb), toY (p.detectorDb - p.reductionDb));
        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (dot));
    }

    for (auto h : { Handle::threshold, Handle::ratio })
    {
        const float radius = h == hovered || h == dragging ? 8.0f : 6.0f;
        const auto c = getHandlePosition (h);
        g.setColour (comptext::curveColour);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c), 1.0f);
    }

    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.setColour (comptext::curveColour);
    auto info = area.reduced (8.0f, 6.0f).withHeight (16.0f).withTrimmedLeft (20.0f);
    g.drawText ("Threshold " + comp::formatKnob (Knob::threshold, s) + "   " + comp::formatKnob (Knob::ratio, s), info, juce::Justification::centredLeft);
    g.setColour (nowReduction > 0.05f ? theme::meterRed.brighter (0.3f) : theme::textDim);
    g.drawText ("turned down now: " + juce::String (nowReduction, 1) + " dB", info.translated (0.0f, 16.0f), juce::Justification::centredLeft);
}

void CompressorCurve::mouseMove (const juce::MouseEvent& e)
{
    const auto h = findHandle (e.position);

    if (h != hovered)
    {
        hovered = h;
        repaint();
    }

    if (onHover)
        onHover (h == Handle::threshold ? Topic::threshold : h == Handle::ratio ? Topic::ratio : Topic::curve);
}

void CompressorCurve::mouseDown (const juce::MouseEvent& e)
{
    dragging = findHandle (e.position);

    if (dragging == Handle::none)
        return;

    if (auto* p = processor.getKnobParameter (dragging == Handle::threshold ? Knob::threshold : Knob::ratio))
        p->beginChangeGesture();
}

void CompressorCurve::mouseDrag (const juce::MouseEvent& e)
{
    const auto s = processor.getSettings();

    if (dragging == Handle::threshold)
    {
        processor.setKnob (Knob::threshold, juce::jlimit (comp::minThreshold, comp::maxThreshold, (float) (std::round (fromX (e.position.x) * 10.0) / 10.0)));
    }
    else if (dragging == Handle::ratio)
    {
        // Where the curve ends at 0 dB in: 0 dB out is 1:1, the threshold itself is as flat as it goes.
        const double out = fromY (e.position.y), over = -s.thresholdDb;
        const double ratio = out >= -0.01 ? 1.0 : out <= s.thresholdDb + 0.01 ? comp::maxRatio : over / (out - s.thresholdDb);
        processor.setKnob (Knob::ratio, juce::jlimit (comp::minRatio, comp::maxRatio, (float) ratio));
    }

    repaint();
}

void CompressorCurve::mouseUp (const juce::MouseEvent&)
{
    if (dragging != Handle::none)
        if (auto* p = processor.getKnobParameter (dragging == Handle::threshold ? Knob::threshold : Knob::ratio))
            p->endChangeGesture();

    dragging = Handle::none;
    repaint();
}

void CompressorCurve::mouseDoubleClick (const juce::MouseEvent&)
{
    const comp::Settings d;
    processor.setKnob (Knob::threshold, d.thresholdDb);
    processor.setKnob (Knob::ratio, d.ratio);
    processor.setKnob (Knob::knee, d.kneeDb);
}

//==============================================================================
CompressorMeter::CompressorMeter (const CompressorHistory& h) : history (h)
{
    setTopic (*this, Topic::meter);
}

void CompressorMeter::update()
{
    // What came in since the last tick, at 30 per second.
    const auto points = history.getNewest (juce::jmax (1, (int) (history.getPointsPerSecond() / 30.0)));
    float newIn = -100.0f, newOut = -100.0f, newReduction = 0.0f;

    for (auto& p : points)
    {
        newIn = juce::jmax (newIn, p.inputDb);
        newOut = juce::jmax (newOut, p.outputDb);
        newReduction = juce::jmax (newReduction, p.reductionDb);
    }

    in = juce::jmax (newIn, in - 1.0f);
    out = juce::jmax (newOut, out - 1.0f);
    reduction = newReduction;

    // The number holds the most for a second, so it can be read.
    if (newReduction >= reductionHeld || --holdTicks <= 0)
    {
        reductionHeld = newReduction;
        holdTicks = 30;
    }
}

void CompressorMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawPanel (g, bounds);
    bounds = bounds.reduced (6.0f, 6.0f);
    auto number = bounds.removeFromBottom (30.0f);
    auto heads = bounds.removeFromTop (14.0f);
    const float width = bounds.getWidth() / 3.0f;
    constexpr float maxReduction = 24.0f;

    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    const char* names[] = { "IN", "GR", "OUT" };

    for (int i = 0; i < 3; ++i)
    {
        g.setColour (theme::textDim);
        g.drawText (names[i], heads.withX (bounds.getX() + width * (float) i).withWidth (width), juce::Justification::centred);
        const auto bar = bounds.withX (bounds.getX() + width * (float) i).withWidth (width).reduced (4.0f, 0.0f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRect (bar);

        if (i == 1)
        {
            // Gain reduction hangs from the top: 0 to 24 dB.
            g.setColour (theme::meterRed);
            g.fillRect (bar.withHeight (bar.getHeight() * juce::jlimit (0.0f, 1.0f, reduction / maxReduction)));
            g.setFont (juce::FontOptions (9.0f));

            for (float db = 6.0f; db < maxReduction; db += 6.0f)
            {
                const float y = bar.getY() + bar.getHeight() * db / maxReduction;
                g.setColour (theme::textDim.withAlpha (0.6f));
                g.fillRect (bar.getX(), y, bar.getWidth(), 1.0f);
            }
        }
        else
        {
            const float level = juce::jlimit (0.0f, 1.0f, ((i == 0 ? in : out) - lowestDb) / -lowestDb);
            g.setColour (i == 0 ? theme::textDim : theme::accent);
            g.fillRect (bar.withTop (bar.getBottom() - bar.getHeight() * level));
        }
    }

    g.setColour (reductionHeld > 0.05f ? theme::meterRed.brighter (0.3f) : theme::textDim);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (reductionHeld > 0.05f ? "-" + juce::String (reductionHeld, 1) : juce::String ("0.0"), number.removeFromTop (17.0f), juce::Justification::centred);
    g.setFont (juce::FontOptions (9.5f));
    g.setColour (theme::textDim);
    g.drawText ("dB turned down", number, juce::Justification::centred);
}

//==============================================================================
/** One knob: its name, the dial and its value. */
class BuiltInCompressorEditor::KnobControl final : public juce::Component
{
public:
    KnobControl (BuiltInCompressor& p, Knob k, const juce::String& name) : knob (k), label ({}, name)
    {
        setTopic (*this, comptext::getTopic (k));
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, theme::text);

        slider.setScrollWheelEnabled (false);   // scrolling never changes a value
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        slider.setColour (juce::Slider::rotarySliderFillColourId, comptext::curveColour);
        slider.setColour (juce::Slider::textBoxTextColourId, theme::text);
        slider.setWantsKeyboardFocus (false);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.getParameters(), BuiltInCompressor::getParameterId (k), slider);

        if (auto* parameter = p.getKnobParameter (k))
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        addAndMakeVisible (label);
        addAndMakeVisible (slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (16));
        slider.setBounds (r);
    }

    const Knob knob;
    juce::Label label;
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
BuiltInCompressorEditor::BuiltInCompressorEditor (BuiltInCompressor& p) : juce::AudioProcessorEditor (&p), compressor (p)
{
    title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (title);

    presets.setTextWhenNothingSelected ("Starting point...");
    const auto& list = comp::getPresets();

    for (size_t i = 0; i < list.size(); ++i)
        presets.addItem (list[i].name, (int) i + 1);

    presets.onChange = [this] { applyPreset (presets.getSelectedId() - 1); };
    setTopic (presets, Topic::presets);

    lessonButton.onClick = [this] { explainer.startLesson(); };
    explainer.onLessonChanged = [this] { lessonButton.setEnabled (! explainer.isInLesson()); };
    explainer.getRecent = [this] { return history.getRecent(); };
    explainer.findThreshold = [this] (const comp::Settings& s, float reductionDb) { return findThreshold (s, s, reductionDb); };

    listenButton.setClickingTogglesState (false);
    listenButton.setColour (juce::TextButton::buttonOnColourId, theme::meterRed.darker (0.3f));
    listenButton.onClick = [this] { compressor.listenToRemoved = ! compressor.listenToRemoved.load(); };
    setTopic (listenButton, Topic::listen);

    guideButton.setTooltip ("What is compression?: a guide for beginners, with pictures that follow this window: threshold and "
                            "ratio, the knee, attack and release, makeup, peak and RMS, the sidechain filter, parallel compression and "
                            "how much to use");
    if (fxui::isPlugin (*getAudioProcessor()))
        guideButton.setTooltip ("Opens the guide to " + getAudioProcessor()->getName() + " inside this window, with pictures that follow your settings.");
    guideButton.onClick = [this] { openGuide(); };

    helpButton.setTooltip ("How the compressor works and how to read the window");
    helpButton.onClick = [this] { explainer.showOverview(); };

    matchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (compressor.getParameters(), "match", matchButton);
    setTopic (matchButton, Topic::match);
    matchValue.setColour (juce::Label::textColourId, theme::textDim);
    matchValue.setFont (juce::FontOptions (12.0f));
    setTopic (matchValue, Topic::match);

    for (juce::Component* c : std::initializer_list<juce::Component*> { &presets, &lessonButton, &listenButton, &guideButton, &helpButton, &matchButton, &matchValue })
    {
        c->setWantsKeyboardFocus (false);
        addAndMakeVisible (c);
    }

    const std::pair<Knob, const char*> names[] = { { Knob::threshold, "Threshold" }, { Knob::ratio, "Ratio" }, { Knob::knee, "Knee" },
                                                   { Knob::attack, "Attack" }, { Knob::release, "Release" }, { Knob::makeup, "Makeup" },
                                                   { Knob::mix, "Mix" }, { Knob::sidechain, "Sidechain" } };

    for (auto& [knob, name] : names)
        addAndMakeVisible (knobs.add (new KnobControl (compressor, knob, name)));

    // Peak or RMS: one click switches.
    detection.onClick = [this] { compressor.setKnob (Knob::detection, compressor.getSettings().detection == comp::Detection::rms ? 0.0f : 1.0f); };
    detection.setWantsKeyboardFocus (false);
    detectionLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    detectionLabel.setJustificationType (juce::Justification::centred);
    setTopic (detection, Topic::detection);
    setTopic (detectionLabel, Topic::detection);
    addAndMakeVisible (detection);
    addAndMakeVisible (detectionLabel);

    curve.onHover = [this] (Topic t) { hover (t); };

    addAndMakeVisible (curve);
    addAndMakeVisible (history);
    addAndMakeVisible (meter);
    addAndMakeVisible (explainer);

    addMouseListener (this, true);
    setSize (1240, 600);
    startTimerHz (30);
}

void BuiltInCompressorEditor::openGuide (int page)
{
    guideHost.show ("What is compression?", [this]
    {
        return new fxui::Guide ("What is compression?", comptext::makeGuide (compressor), nullptr,
                                "The pictures follow the settings in DawIt Compressor's window.");
    }, "the settings in " + getAudioProcessor()->getName() + "'s window", page);
}

BuiltInCompressorEditor::~BuiltInCompressorEditor()
{
    removeMouseListener (this);

    // Listening to what's removed is for while the window is open.
    compressor.listenToRemoved = false;
}

void BuiltInCompressorEditor::applyPreset (int index)
{
    const auto& list = comp::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    const auto& preset = list[(size_t) index];
    const auto before = compressor.getSettings();
    compressor.applyPreset (preset);

    // While the track plays, put the threshold where the loudest part gets the reduction this instrument wants.
    const auto threshold = findThreshold (preset.settings, before, preset.targetReductionDb);

    if (threshold.has_value())
        compressor.setKnob (Knob::threshold, *threshold);

    explainer.showPreset (index, threshold.has_value());

    // Show the name but select nothing, so picking the same one again (while the track plays) works.
    presets.setTextWhenNothingSelected (preset.name);
    presets.setSelectedId (0, juce::dontSendNotification);
}

std::optional<float> BuiltInCompressorEditor::findThreshold (const comp::Settings& settings, const comp::Settings& heardWith, float reductionDb) const
{
    if (! history.getRecent().hasSignal())
        return std::nullopt;

    // What the compressor heard only counts when it listened the same way; otherwise start from the peaks
    // (RMS hears a tone 3 dB lower).
    const bool sameEars = heardWith.detection == settings.detection
                          && comp::isSidechainFilterOn (heardWith.sidechainHz) == comp::isSidechainFilterOn (settings.sidechainHz);
    const float rmsOffset = settings.detection == comp::Detection::rms ? 3.0f : 0.0f;
    std::vector<float> levels;

    for (auto& p : history.getNewest ((int) (4.0 * history.getPointsPerSecond())))
        levels.push_back (sameEars ? p.detectorDb : p.inputDb - rmsOffset);

    const float threshold = comp::findThreshold (settings, levels, history.getPointsPerSecond(), reductionDb);

    if (threshold <= comp::minThreshold || threshold >= comp::maxThreshold)
        return std::nullopt;

    return std::round (threshold * 10.0f) / 10.0f;
}

void BuiltInCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::panel);
    g.fillRect (getLocalBounds().removeFromTop (headerHeight));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, headerHeight - 1, getWidth(), 1);

    const char* groups[] = { "HOW MUCH", "HOW FAST", "LEVEL", "WHAT IT LISTENS TO" };
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

    for (size_t i = 0; i < groupAreas.size(); ++i)
    {
        const auto area = groupAreas[i].toFloat();
        g.setColour (theme::panel);
        g.fillRoundedRectangle (area, 5.0f);
        g.setColour (theme::textDim);
        g.drawText (groups[i], area.withHeight (18.0f).reduced (8.0f, 0.0f), juce::Justification::centredLeft);
    }
}

void BuiltInCompressorEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (12, 9);

    title.setBounds (header.removeFromLeft (150));
    presets.setBounds (header.removeFromLeft (180));
    header.removeFromLeft (10);
    lessonButton.setBounds (header.removeFromLeft (170));
    header.removeFromLeft (10);
    guideButton.setBounds (header.removeFromLeft (184));

    helpButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (14);
    matchValue.setBounds (header.removeFromRight (60));
    matchButton.setBounds (header.removeFromRight (106));
    header.removeFromRight (12);
    listenButton.setBounds (header.removeFromRight (160));

    explainer.setBounds (r.removeFromRight (explainerWidth));

    // The knobs, in four groups: how much, how fast, level, what it listens to.
    auto knobArea = r.removeFromBottom (knobsHeight).reduced (12, 8);
    const int sizes[] = { 3, 2, 2, 2 };
    constexpr int gap = 8;
    const int column = (knobArea.getWidth() - gap * 3) / comp::numKnobs;
    int next = 0;

    for (size_t g = 0; g < groupAreas.size(); ++g)
    {
        auto area = knobArea.removeFromLeft (column * sizes[g]);
        knobArea.removeFromLeft (gap);
        groupAreas[g] = area;
        area.removeFromTop (18);
        area = area.reduced (2, 4);

        for (int i = 0; i < sizes[g]; ++i, ++next)
        {
            auto cell = area.removeFromLeft (column).reduced (3, 0);

            if (next < knobs.size())
            {
                knobs[next]->setBounds (cell);
            }
            else
            {
                detectionLabel.setBounds (cell.removeFromTop (16));
                detection.setBounds (cell.withSizeKeepingCentre (64, 26));
            }
        }
    }

    auto top = r.reduced (12, 10);
    curve.setBounds (top.removeFromLeft (juce::jmin (top.getHeight(), 340)));
    top.removeFromLeft (10);
    meter.setBounds (top.removeFromRight (96));
    top.removeFromRight (10);
    history.setBounds (top);
}

void BuiltInCompressorEditor::mouseEnter (const juce::MouseEvent& e)
{
    // The control the mouse is on (the curve tells about its dots itself), or nothing to explain.
    for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
    {
        if (c == &curve)
            return;

        if (c->getProperties().contains ("compTopic"))
        {
            hover ((Topic) (int) c->getProperties()["compTopic"]);
            return;
        }
    }

    hover (std::nullopt);
}

void BuiltInCompressorEditor::hover (std::optional<Topic> topic)
{
    if (topic == pendingTopic)
        return;

    pendingTopic = topic;
    pendingTicks = 0;
}

void BuiltInCompressorEditor::timerCallback()
{
    history.update();
    meter.update();

    for (auto* c : { (juce::Component*) &history, (juce::Component*) &curve, (juce::Component*) &meter })
        c->repaint();

    const auto s = compressor.getSettings();
    detection.setButtonText (comp::formatKnob (Knob::detection, s));
    listenButton.setToggleState (compressor.listenToRemoved.load(), juce::dontSendNotification);

    const bool matching = compressor.isMatchingLevel();
    matchValue.setText (matching ? (compressor.matchGainDb.load() >= 0.05f ? "+" : "") + juce::String (compressor.matchGainDb.load(), 1) + " dB"
                                 : juce::String(), juce::dontSendNotification);

    for (auto* k : knobs)
        if (k->knob == Knob::makeup)
            k->setAlpha (matching ? 0.45f : 1.0f);

    // The explainer follows once the mouse has rested a quarter of a second.
    if (pendingTopic.has_value() && ++pendingTicks >= 8)
    {
        explainer.showTopic (*pendingTopic);
        pendingTopic.reset();
    }

    if (++ticks % 3 == 0)
        explainer.refresh();
}
} // namespace daw
