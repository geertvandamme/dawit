#include "GuideButton.h"
#include "MeterEditor.h"
#include "MeterGuide.h"

namespace daw
{
using namespace fxui;

namespace meterui
{
juce::Colour getLoudnessColour (std::optional<double> integratedLufs, double targetLufs, bool showTarget)
{
    if (! integratedLufs || ! showTarget)
        return theme::textDim;

    return std::abs (*integratedLufs - targetLufs) <= 1.0 ? theme::meterGreen : theme::meterYellow;
}

juce::String formatValue (std::optional<double> value, int decimals)
{
    return value ? juce::String (*value, decimals) : juce::String::fromUTF8 ("\xe2\x80\x93");
}
}

namespace
{
constexpr double historyFloor = -48.0, barFloor = -60.0;
constexpr int topicReadings = 50, topicIntegrated = 51, topicTruePeak = 52, topicRange = 53, topicPeaks = 54, topicHistory = 55, topicLevels = 56, topicCorrelation = 57;

juce::String withSign (double v, int decimals = 1)
{
    const double rounded = std::round (std::abs (v) * std::pow (10.0, decimals)) / std::pow (10.0, decimals);
    return juce::String (v < 0.0 && rounded > 0.0 ? "-" : "+") + juce::String (rounded, decimals);
}

float dbToX (double db, double floorDb, juce::Rectangle<float> r)
{
    return r.getX() + (float) juce::jlimit (0.0, 1.0, (db - floorDb) / -floorDb) * r.getWidth();
}

void fitText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, juce::Justification j, bool bold = false)
{
    g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText (text, r.toNearestInt(), j, 1, 0.7f);
}
}

MeterView::MeterView (BuiltInMeter& p, std::function<void (int)> open, std::function<void (int)> hoverTopic)
    : processor (p), guide (std::move (open)), onHover (std::move (hoverTopic))
{
    lastResetCount = p.getResetCount();
}

MeterView::Areas MeterView::getAreas() const
{
    Areas a;
    auto all = getLocalBounds().toFloat();
    a.readings = all.removeFromLeft (286.0f);
    all.removeFromLeft (10.0f);
    auto left = a.readings.reduced (14.0f, 12.0f);
    a.momentary = left.removeFromTop (26.0f);
    a.shortTerm = left.removeFromTop (26.0f);
    left.removeFromTop (12.0f);
    a.integrated = left.removeFromTop (14.0f + 46.0f);
    a.toTarget = left.removeFromTop (20.0f);
    left.removeFromTop (12.0f);
    a.range = left.removeFromTop (20.0f);
    a.maxShort = left.removeFromTop (20.0f);
    left.removeFromTop (12.0f);
    a.truePeak = left.removeFromTop (14.0f + 30.0f);
    left.removeFromTop (12.0f);
    a.peaks = left.removeFromTop (60.0f);
    a.history = all.removeFromTop (all.getHeight() - 130.0f);
    all.removeFromTop (8.0f);
    a.levels = all.removeFromTop (all.getHeight() - 30.0f);
    all.removeFromTop (4.0f);
    a.correlation = all;
    return a;
}

int MeterView::getTopicAt (juce::Point<float> p) const
{
    const auto a = getAreas();
    if (a.momentary.contains (p) || a.shortTerm.contains (p)) return topicReadings;
    if (a.integrated.contains (p) || a.toTarget.contains (p)) return topicIntegrated;
    if (a.truePeak.contains (p)) return topicTruePeak;
    if (a.range.contains (p) || a.maxShort.contains (p)) return topicRange;
    if (a.peaks.contains (p)) return topicPeaks;
    if (a.history.contains (p)) return topicHistory;
    if (a.levels.contains (p)) return topicLevels;
    if (a.correlation.contains (p)) return topicCorrelation;
    return -1;
}

void MeterView::update()
{
    constexpr float dt = 1.0f / 30.0f;

    if (processor.getResetCount() != lastResetCount)
    {
        lastResetCount = processor.getResetCount();
        shownDb = { { -90.0f, -90.0f } }; holdDb = shownDb; holdAge = { { 0.0f, 0.0f } };
    }

    for (size_t c = 0; c < 2; ++c)
    {
        const float db = juce::Decibels::gainToDecibels (processor.getBlockPeak ((int) c), -90.0f);
        shownDb[c] = juce::jmax (db, shownDb[c] - 40.0f * dt);

        if (db >= holdDb[c]) { holdDb[c] = db; holdAge[c] = 0.0f; }
        else
        {
            holdAge[c] += dt;
            if (holdAge[c] > 1.5f) holdDb[c] = juce::jmax (db, holdDb[c] - 20.0f * dt);
        }
    }

    repaint();
}

void MeterView::drawBar (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& label, std::optional<double> lufs,
                         const loudness::Readings&, const meterfx::Settings& settings)
{
    g.setColour (theme::textDim);
    fitText (g, label, area.removeFromLeft (16.0f), 12.0f, juce::Justification::centredLeft, true);
    g.setColour (lufs ? theme::text : theme::textDim);
    fitText (g, lufs ? juce::String (*lufs, 1) + " LUFS" : juce::String::fromUTF8 ("\xe2\x80\x93"), area.removeFromRight (86.0f), 13.0f, juce::Justification::centredRight, true);
    auto bar = area.reduced (4.0f, 8.0f);
    g.setColour (theme::textDim.withAlpha (0.2f));
    g.fillRoundedRectangle (bar, 2.0f);

    if (lufs)
    {
        g.setColour (meterui::colour);
        g.fillRoundedRectangle (bar.withRight (juce::jmax (bar.getX() + 2.0f, dbToX (*lufs, barFloor, bar))), 2.0f);
    }

    if (settings.showTarget)
    {
        const float x = dbToX (settings.targetLufs, barFloor, bar);
        g.setColour (theme::text);
        g.drawVerticalLine ((int) x, bar.getY() - 3.0f, bar.getBottom() + 3.0f);
    }
}

void MeterView::drawHistory (juce::Graphics& g, juce::Rectangle<float> area, const meterfx::Settings& settings, const loudness::Readings& readings)
{
    drawPanel (g, area);
    g.setColour (theme::textDim);
    fitText (g, "THE LAST 2 MINUTES", area.reduced (10.0f, 4.0f).removeFromTop (16.0f), 11.0f, juce::Justification::centredLeft, true);
    auto plot = area.reduced (46.0f, 24.0f).withTrimmedTop (0.0f);
    plot.removeFromBottom (2.0f);
    auto yFor = [&] (double db) { return plot.getBottom() - (float) juce::jlimit (0.0, 1.0, (db - historyFloor) / -historyFloor) * plot.getHeight(); };

    g.setFont (juce::FontOptions (10.0f));
    for (int db : { -48, -36, -24, -12, 0 })
    {
        const float y = yFor (db);
        g.setColour (theme::textDim.withAlpha (0.3f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (theme::textDim);
        g.drawText (juce::String (db), juce::Rectangle<float> (area.getX() + 4.0f, y - 7.0f, 38.0f, 14.0f), juce::Justification::centredRight);
    }
    for (int s : { 0, 30, 60, 90, 120 })
    {
        const float x = plot.getRight() - (float) s / 120.0f * plot.getWidth();
        g.setColour (theme::textDim.withAlpha (0.2f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (theme::textDim);
        g.drawText (s == 0 ? juce::String ("now") : juce::String (s / 60) + ":" + juce::String (s % 60).paddedLeft ('0', 2), juce::Rectangle<float> (x - 22.0f, plot.getBottom() + 3.0f, 44.0f, 14.0f), juce::Justification::centred);
    }

    const auto points = processor.getHistory();
    const auto count = (int) points.size();
    auto line = [&] (bool shortTerm, float thickness, juce::Colour c)
    {
        juce::Path path; bool open = false;
        for (int i = 0; i < count; ++i)
        {
            const float v = shortTerm ? points[(size_t) i].shortTerm : points[(size_t) i].momentary;
            if (std::isnan (v)) { open = false; continue; }
            const float x = plot.getRight() - (float) (count - 1 - i) / (float) meterfx::historyLength * plot.getWidth();
            const float y = yFor (v);
            if (open) path.lineTo (x, y); else { path.startNewSubPath (x, y); open = true; }
        }
        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved));
    };
    line (false, 1.0f, theme::textDim.brighter (0.2f));
    line (true, 2.2f, meterui::colour);

    auto dashedLine = [&] (double db, juce::Colour c, float on, float off)
    {
        juce::Path p, dashed; p.startNewSubPath (plot.getX(), yFor (db)); p.lineTo (plot.getRight(), yFor (db));
        const float dashes[] = { on, off };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, p, dashes, 2);
        g.setColour (c); g.fillPath (dashed);
    };
    if (settings.showTarget) dashedLine (settings.targetLufs, theme::text.withAlpha (0.8f), 6.0f, 4.0f);
    if (readings.integrated) dashedLine (*readings.integrated, meterui::getLoudnessColour (readings.integrated, settings.targetLufs, settings.showTarget), 1.5f, 3.0f);

    const auto mouse = getMouseXYRelative().toFloat();
    if (plot.contains (mouse) && isMouseOver())
    {
        g.setColour (theme::text.withAlpha (0.5f));
        g.drawVerticalLine ((int) mouse.x, plot.getY(), plot.getBottom());
    }
}

void MeterView::drawLevels (juce::Graphics& g, juce::Rectangle<float> area, const meterfx::Settings& settings, const loudness::Readings& readings)
{
    drawPanel (g, area);
    area = area.reduced (10.0f, 6.0f);
    g.setColour (theme::textDim);
    fitText (g, "LEVELS", area.removeFromTop (14.0f), 11.0f, juce::Justification::centredLeft, true);
    auto scale = area.removeFromBottom (14.0f);
    const char* names[] = { "L", "R" };
    juce::Rectangle<float> barsArea;
    for (size_t c = 0; c < 2; ++c)
    {
        auto row = area.removeFromTop (area.getHeight() / (float) (2 - c));
        g.setColour (theme::textDim);
        fitText (g, names[c], row.removeFromLeft (16.0f), 12.0f, juce::Justification::centredLeft, true);
        auto readout = row.removeFromRight (74.0f);
        auto bar = row.reduced (4.0f, 4.0f);
        if (c == 0) barsArea = bar; else barsArea = barsArea.getUnion (bar);
        g.setColour (theme::textDim.withAlpha (0.2f));
        g.fillRoundedRectangle (bar, 2.0f);
        const bool over = shownDb[c] > (float) settings.ceilingDbtp;
        g.setColour (over ? theme::meterRed : theme::meterGreen);
        g.fillRoundedRectangle (bar.withRight (juce::jmax (bar.getX() + 1.0f, dbToX (shownDb[c], barFloor, bar))), 2.0f);
        const float hx = dbToX (holdDb[c], barFloor, bar);
        g.setColour (holdDb[c] > (float) settings.ceilingDbtp ? theme::meterRed : theme::text);
        g.fillRect (hx - 1.0f, bar.getY(), 2.0f, bar.getHeight());
        g.setColour (holdDb[c] > (float) settings.ceilingDbtp ? theme::meterRed : theme::text);
        fitText (g, holdDb[c] <= -89.0f ? juce::String::fromUTF8 ("\xe2\x80\x93") : juce::String (holdDb[c], 1) + " dB", readout, 11.0f, juce::Justification::centredRight);
    }
    // The ceiling and the true peak, across both bars.
    g.setColour (theme::meterRed.withAlpha (0.8f));
    g.drawVerticalLine ((int) dbToX (settings.ceilingDbtp, barFloor, barsArea), barsArea.getY(), barsArea.getBottom());
    if (readings.truePeak)
    {
        const float x = dbToX (*readings.truePeak, barFloor, barsArea);
        juce::Path tri; tri.addTriangle (x - 4.0f, barsArea.getBottom() + 5.0f, x + 4.0f, barsArea.getBottom() + 5.0f, x, barsArea.getBottom() - 1.0f);
        g.setColour (*readings.truePeak > settings.ceilingDbtp ? theme::meterRed : theme::accent);
        g.fillPath (tri);
    }
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (9.0f));
    scale.removeFromLeft (16.0f); scale.removeFromRight (74.0f); scale = scale.reduced (4.0f, 0.0f);
    for (int db : { -60, -48, -36, -24, -12, 0 })
        g.drawText (juce::String (db), juce::Rectangle<float> (dbToX (db, barFloor, scale) - 14.0f, scale.getY() + 2.0f, 28.0f, 11.0f), juce::Justification::centred);
}

void MeterView::paint (juce::Graphics& g)
{
    const auto a = getAreas();
    const auto settings = processor.getSettings();
    const auto readings = processor.getReadings();
    const bool measuring = processor.isMeasuring();

    drawPanel (g, a.readings);
    drawBar (g, a.momentary, "M", readings.momentary, readings, settings);
    drawBar (g, a.shortTerm, "S", readings.shortTerm, readings, settings);

    const auto colour = meterui::getLoudnessColour (readings.integrated, settings.targetLufs, settings.showTarget);
    auto integrated = a.integrated;
    g.setColour (theme::textDim);
    fitText (g, measuring ? "INTEGRATED" : "INTEGRATED (NOT MEASURING: STOPPED)", integrated.removeFromTop (14.0f), 11.0f, juce::Justification::centredLeft, true);
    g.setColour (colour);
    fitText (g, meterui::formatValue (readings.integrated) + (readings.integrated ? " LUFS" : ""), integrated, 34.0f, juce::Justification::centredLeft, true);
    g.setColour (settings.showTarget && readings.integrated ? colour : theme::textDim);
    fitText (g, ! settings.showTarget ? "no target" : readings.integrated ? "to target " + withSign (*readings.integrated - settings.targetLufs) + " LU" : "to target " + meterui::formatValue ({}),
             a.toTarget, 13.0f, juce::Justification::centredLeft);

    auto pair = [&] (juce::Rectangle<float> r, const juce::String& name, const juce::String& value, juce::Colour c = theme::text)
    {
        g.setColour (theme::textDim); fitText (g, name, r.removeFromLeft (r.getWidth() * 0.5f), 12.0f, juce::Justification::centredLeft);
        g.setColour (c); fitText (g, value, r, 12.5f, juce::Justification::centredRight, true);
    };
    pair (a.range, "Loudness range", readings.range ? juce::String (*readings.range, 1) + " LU" : meterui::formatValue ({}));
    pair (a.maxShort, "Max short-term", readings.maxShortTerm ? juce::String (*readings.maxShortTerm, 1) + " LUFS" : meterui::formatValue ({}));

    const bool tpOver = readings.truePeak && *readings.truePeak > settings.ceilingDbtp;
    auto tp = a.truePeak;
    g.setColour (theme::textDim);
    fitText (g, "TRUE PEAK", tp.removeFromTop (14.0f), 11.0f, juce::Justification::centredLeft, true);
    g.setColour (tpOver ? theme::meterRed : theme::text);
    fitText (g, meterui::formatValue (readings.truePeak) + (readings.truePeak ? " dBTP" : ""), tp, 22.0f, juce::Justification::centredLeft, true);

    auto peaks = a.peaks;
    pair (peaks.removeFromTop (20.0f), "Sample peak", readings.samplePeak ? juce::String (*readings.samplePeak, 1) + " dBFS" : meterui::formatValue ({}),
          readings.samplePeak && *readings.samplePeak > settings.ceilingDbtp ? theme::meterRed : theme::text);
    pair (peaks.removeFromTop (20.0f), "RMS (3 s)", readings.rms ? juce::String (*readings.rms, 1) + " dBFS" : meterui::formatValue ({}));
    pair (peaks, "Crest factor", readings.crest ? juce::String (*readings.crest, 1) + " dB" : meterui::formatValue ({}));

    drawHistory (g, a.history, settings, readings);
    drawLevels (g, a.levels, settings, readings);
    stereoScope::drawCorrelation (g, a.correlation.reduced (6.0f, 2.0f), "Corr.", processor.measurement.getCorrelation (false));
}

void MeterView::mouseMove (const juce::MouseEvent& e)
{
    if (onHover == nullptr) return;
    const int topic = getTopicAt (e.position);
    if (topic >= 0) onHover (topic);
    repaint (getAreas().history.toNearestInt());
}

void MeterView::mouseDown (const juce::MouseEvent& e)
{
    const auto a = getAreas();
    const bool onHistory = a.history.contains (e.position), onReadings = a.readings.contains (e.position);

    if (e.mods.isPopupMenu() && (onHistory || onReadings))
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Reset");
        menu.addItem (2, "Reset on play", true, processor.getSettings().resetOnPlay);
        menu.addSeparator();
        guideicon::addItem (menu, 3, "What is loudness?...");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<MeterView> (this)] (int n)
        {
            if (safe == nullptr) return;
            if (n == 1) safe->processor.requestReset();
            else if (n == 2) safe->processor.setResetOnPlay (! safe->processor.getSettings().resetOnPlay);
            else if (n == 3 && safe->guide) safe->guide (-1);
        });
        return;
    }

    if (a.integrated.contains (e.position) || a.toTarget.contains (e.position) || a.truePeak.contains (e.position))
        processor.requestReset();
}

juce::String MeterView::getTooltip()
{
    const auto a = getAreas();
    const auto mouse = getMouseXYRelative().toFloat();
    const auto readings = processor.getReadings();

    if (a.history.contains (mouse))
    {
        const auto plot = a.history.reduced (46.0f, 24.0f);
        if (plot.contains (mouse))
        {
            const auto points = processor.getHistory();
            const float ago = (plot.getRight() - mouse.x) / plot.getWidth() * 120.0f;
            const int index = (int) points.size() - 1 - juce::roundToInt (ago * 10.0f);
            const int seconds = juce::roundToInt (ago);
            const auto clock = juce::String (seconds / 60) + ":" + juce::String (seconds % 60).paddedLeft ('0', 2);
            if (juce::isPositiveAndBelow (index, (int) points.size()) && ! std::isnan (points[(size_t) index].shortTerm))
                return clock + " ago" + juce::String::fromUTF8 (" · M ") + juce::String (points[(size_t) index].momentary, 1)
                       + juce::String::fromUTF8 (" · S ") + juce::String (points[(size_t) index].shortTerm, 1) + " LUFS";
            return clock + " ago: no reading";
        }
        return "The last 2 minutes of loudness: momentary (thin line) and short-term (thick line), the target (dashed) and integrated (dotted).";
    }
    if (a.momentary.contains (mouse)) return "Momentary loudness: the last 0.4 seconds, in LUFS. The line marks the target.";
    if (a.shortTerm.contains (mouse)) return "Short-term loudness: the last 3 seconds, in LUFS. The one to watch while you mix.";
    if (a.integrated.contains (mouse) || a.toTarget.contains (mouse))
        return "Integrated loudness: everything since the last reset, with quiet parts left out. Click to reset the measurement.";
    if (a.range.contains (mouse)) return "Loudness range (LRA): how far the loudness moves between quiet and loud parts, in LU.";
    if (a.maxShort.contains (mouse)) return "The loudest short-term reading since the last reset.";
    if (a.truePeak.contains (mouse))
        return "True peak: the highest the wave goes, also between the samples, in dBTP (4 times oversampled). Click to reset. Red above the ceiling.";
    if (a.peaks.contains (mouse))
        return "Sample peak is the highest sample since the last reset, RMS the average level of the last 3 seconds, and crest factor the gap between the peak and the RMS of those 3 seconds.";
    if (a.levels.contains (mouse))
        return "Left and right sample peaks with a hold, the true peak (triangle) and the ceiling (red line).";
    if (a.correlation.contains (mouse)) return "Correlation: +1 is mono, 0 unrelated, below 0 parts cancel in mono.";
    juce::ignoreUnused (readings);
    return {};
}

BuiltInMeterEditor::BuiltInMeterEditor (BuiltInMeter& p)
    : EffectEditor (p, "DawIt Meter", "What is loudness?", "", p.listenToRemoved, p.clap, meterui::colour, fx::Clap::Sound::calibrationTone),
      meterProcessor (p), view (p, [this] (int page) { openGuide (page); }, [this] (int topic) { hover (topic); })
{
    addAndMakeVisible (view);
    listenButton.setVisible (false);   // it never changes the sound: nothing to hear
    knobs.add (new EffectKnob (p.getParameters(), BuiltInMeter::getParameterId (meterfx::Knob::target), "Target", 0, meterui::colour));
    knobs.add (new EffectKnob (p.getParameters(), BuiltInMeter::getParameterId (meterfx::Knob::ceiling), "Ceiling", 1, meterui::colour));
    knobs[0]->slider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " LUFS"; };
    knobs[1]->slider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dBTP"; };
    for (auto* k : knobs) k->slider.updateText();
    knobs[0]->slider.setTooltip ("Target: the integrated loudness you are aiming for, in LUFS. Streaming services play at about -14.");
    knobs[1]->slider.setTooltip ("Ceiling: the true peak you don't want to pass, in dBTP. Streaming asks for -1 or lower.");
    showTargetSwitch.button.setTooltip ("Show target: draws the target on the bars and the history, and colours the integrated number by how close it is.");
    resetNow.button.setTooltip ("Reset: starts a new measurement. Integrated loudness, loudness range, maxima and true peak start again.");
    resetOnPlay.button.setTooltip ("Reset on play: starts a new measurement each time the song starts playing, so integrated loudness is that of one play-through.");
    showTargetSwitch.button.onClick = [this] { meterProcessor.setShowTarget (! meterProcessor.getSettings().showTarget); };
    resetNow.button.setButtonText ("Reset");
    resetNow.button.onClick = [this] { meterProcessor.requestReset(); showTopic (3); };
    resetOnPlay.button.onClick = [this] { meterProcessor.setResetOnPlay (! meterProcessor.getSettings().resetOnPlay); };
    setGroups ({ { "TARGET", { knobs[0], knobs[1], &showTargetSwitch }, 0 }, { "MEASURE", { &resetNow, &resetOnPlay }, 0 } });

    juce::StringArray names;
    for (const auto& preset : meterfx::getPresets()) names.add (preset.name);
    setPresetNames (names);
    clapButton.setTooltip ("Plays the EBU check tone: a 1 kHz sine at -18 dBFS in both channels, for 3 seconds. It should read -18.0 LUFS.");
    tick(); showTopic (CommonTopic::overview); setSize (1240, 640);
}

void BuiltInMeterEditor::applyPreset (int index)
{
    const auto& list = meterfx::getPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
    meterProcessor.applyPreset (list[(size_t) index]); presetIndex = index;
    showPresetName (list[(size_t) index].name); showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInMeterEditor::createGuidePages() { return metertext::makeGuide (meterProcessor); }

void BuiltInMeterEditor::layoutPictures (juce::Rectangle<int> r) { view.setBounds (r); }

void BuiltInMeterEditor::tick()
{
    const auto s = meterProcessor.getSettings();
    showTargetSwitch.button.setButtonText (s.showTarget ? "On" : "Off");
    showTargetSwitch.button.setToggleState (s.showTarget, juce::dontSendNotification);
    resetOnPlay.button.setButtonText (s.resetOnPlay ? "On" : "Off");
    resetOnPlay.button.setToggleState (s.resetOnPlay, juce::dontSendNotification);
    view.update();
}

juce::AttributedString BuiltInMeterEditor::buildText (int topic)
{
    juce::AttributedString text; text.setWordWrap (juce::AttributedString::byWord);
    const auto c = meterui::colour;
    switch (topic)
    {
        case 0: addTitle (text, "Target", c); addLead (text, "The integrated loudness you are aiming for, in LUFS. The line on the bars and the history marks it, and the integrated number turns green within 1 LU of it. Streaming services play songs at about -14 LUFS, European television at -23. Pick a starting point to set it for you."); break;
        case 1: addTitle (text, "Ceiling", c); addLead (text, "The true peak you don't want to pass, in dBTP. Streaming asks for -1 dBTP or lower, because encoding to MP3 or AAC can raise the peaks. The true peak and the level bars turn red above it."); break;
        case 2: addTitle (text, "Show target", c); addLead (text, "Off while you mix with no target: the target line goes, and the integrated number stays grey instead of turning green or yellow."); break;
        case 3: addTitle (text, "Reset", c); addLead (text, "Starts a new measurement: integrated loudness, loudness range, the maxima and the true peak begin again. Clicking the integrated number or the true peak does the same."); break;
        case 4: addTitle (text, "Reset on play", c); addLead (text, "Starts a new measurement each time the song starts playing, so integrated loudness is the loudness of one play-through. Switch it off to keep measuring across stops, or to measure a loop."); break;
        case topicReadings: addTitle (text, "Momentary and short-term", c); addLead (text, "M is the loudness of the last 0.4 seconds and jumps with every beat. S is the last 3 seconds and moves smoothly: the one to watch while you mix. Both are in LUFS (1 LU is 1 dB). The white tick marks the target."); break;
        case topicIntegrated: addTitle (text, "Integrated loudness", c); addLead (text, "The loudness of everything since the last reset, in LUFS, with silence and very quiet parts left out. It's the number streaming services and broadcasters use. For the real figure, play the song from start to end. 'To target' is how many LU you are above (+) or below (-) the target."); break;
        case topicTruePeak: addTitle (text, "True peak", c); addLead (text, "The highest the wave goes between the samples, in dBTP, worked out with 4 times oversampling, close to the wave a converter makes. It can be higher than the highest sample. Red means it is above the ceiling. Click to reset."); break;
        case topicRange: addTitle (text, "Loudness range", c); addLead (text, "LRA is how far the loudness moves between the quieter and the louder parts, in LU, ignoring the quietest 10% and the loudest 5%. Dense pop masters are often 3 to 6 LU. Max short-term is the loudest 3-second reading so far."); break;
        case topicPeaks: addTitle (text, "Sample peak, RMS and crest", c); addLead (text, "Sample peak is the highest single sample so far, in dBFS. RMS is the average level of the last 3 seconds. Crest factor is the gap between the peak and the RMS in the last 3 seconds: a steady sine has 3 dB, punchy drums much more."); break;
        case topicHistory: addTitle (text, "The last 2 minutes", c); addLead (text, "The thin line is momentary loudness, the thick one short-term. The dashed line is the target and the dotted one the integrated loudness so far. The scale runs from -48 to 0 LUFS. Rest the mouse on the picture to read a moment."); break;
        case topicLevels: addTitle (text, "Levels", c); addLead (text, "The left and right sample peaks with a hold, from -60 to 0 dB. The triangle is the true peak so far and the red line the ceiling. A bar turns red above the ceiling."); break;
        case topicCorrelation: addTitle (text, "Correlation", c); addLead (text, "From -1 to +1, over about a third of a second. +1 is mono, 0 unrelated, and below 0 parts of the sound cancel when the song is played in mono."); break;
        case CommonTopic::presets:
            if (juce::isPositiveAndBelow (presetIndex, (int) meterfx::getPresets().size()))
            { const auto& p = meterfx::getPresets()[(size_t) presetIndex]; addTitle (text, p.name, c); addLead (text, p.why); break; }
            [[fallthrough]];
        default:
            addTitle (text, "DawIt Meter", c);
            addLead (text, "Measures how loud the sound is the way streaming services and broadcasters do, and passes it on unchanged. Put it last on a track, on a bus or on a bus that holds the whole mix, and play the song from the start for the integrated loudness. A mono channel is measured as one channel, as BS.1770 says, so it reads 3 LU lower than the same sound on both sides of a stereo track.");
            break;
    }
    return text;
}
}
