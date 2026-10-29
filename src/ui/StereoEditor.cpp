#include "GuideButton.h"
#include "StereoEditor.h"
#include <juce_dsp/juce_dsp.h>
namespace daw
{
using namespace fxui;
StereoPictures::StereoPictures (BuiltInStereo& p, std::function<void (int)> open, std::function<void (int)> hoverTopic) : processor (p), guide (std::move (open)), onHover (std::move (hoverTopic)) { setTopic (*this, 50); }
juce::Rectangle<float> StereoPictures::spectrumBounds() const
{
    auto r = getLocalBounds().toFloat().reduced (10.0f);
    r.removeFromRight (juce::jmin (320.0f, r.getWidth() * 0.44f));
    return r.withTrimmedBottom (65.0f).reduced (5.0f, 20.0f);
}
void StereoPictures::paint (juce::Graphics& g)
{
    auto all = getLocalBounds().toFloat().reduced (5.0f);
    auto scopeArea = all.removeFromRight (juce::jmin (320.0f, all.getWidth() * 0.44f));
    drawPanel (g, all.reduced (2.0f));
    auto plot = spectrumBounds();
    const auto settings = processor.getSettings();
    auto xFor = [&] (float hz) { return plot.getX() + plot.getWidth() * std::log (juce::jlimit (20.0f, 20000.0f, hz) / 20.0f) / std::log (1000.0f); };
    if (settings.monoBelowHz > 0.0f)
    {
        g.setColour (stereoScope::lowColour.withAlpha (0.12f));
        g.fillRect (plot.withRight (xFor (settings.monoBelowHz)));
        g.setColour (stereoScope::lowColour);
        g.drawVerticalLine ((int) xFor (settings.monoBelowHz), plot.getY(), plot.getBottom());
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("mono below " + juce::String ((int) settings.monoBelowHz) + " Hz", plot.withHeight (16.0f), juce::Justification::topLeft);
    }
    if (settings.monoBelowHz > 0.0f)
    {
        juce::Path response;
        for (int x = 0; x < (int) plot.getWidth(); ++x)
        {
            const double hz = 20.0 * std::pow (1000.0, (double) x / (double) plot.getWidth());
            const auto gain = stereo::highMagnitude (hz / settings.monoBelowHz, settings.slope);
            const float db = (float) juce::Decibels::gainToDecibels (gain, -90.0);
            const float y = plot.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f) * plot.getHeight();
            if (x == 0) response.startNewSubPath (plot.getX(), y); else response.lineTo (plot.getX() + (float) x, y);
        }
        g.setColour (stereoScope::lowColour.withAlpha (0.65f));
        g.strokePath (response, juce::PathStrokeType (1.0f));
    }
    g.setColour (theme::textDim.withAlpha (0.4f));
    for (int db : { -90, -60, -30, 0 })
    {
        const float y = plot.getBottom() - (float) (db + 90) * plot.getHeight() / 90.0f;
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
    }
    for (float f : { 20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f })
    {
        const float x = xFor (f);
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (x - 15.0f, plot.getBottom(), 30.0f, 15.0f), juce::Justification::centred);
    }
    // Read the ring without consuming it: each view can inspect the same recent sound.
    auto spectrum = [&] (const stereo::Measurement& measurement, bool before)
    {
        constexpr int size = 8192;
        auto points = measurement.getPoints();
        if (points.size() < size) return;
        juce::dsp::FFT fft (13);
        std::array<float, size * 2> mid {}, side {};
        const auto offset = points.size() - size;
        for (int i = 0; i < size; ++i)
        {
            const auto& point = points[offset + (size_t) i];
            const float l = point.left, r = point.right;
            const float window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (size - 1));
            mid[(size_t) i] = (l + r) * 0.5f * window;
            side[(size_t) i] = (l - r) * 0.5f * window;
        }
        fft.performFrequencyOnlyForwardTransform (mid.data());
        fft.performFrequencyOnlyForwardTransform (side.data());
        auto trace = [&] (const auto& bins, juce::Colour colour)
        {
            juce::Path path;
            const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
            for (int x = 0; x < (int) plot.getWidth(); ++x)
            {
                const double hz = 20.0 * std::pow (1000.0, (double) x / (double) plot.getWidth());
                const int bin = juce::jlimit (1, size / 2 - 1, juce::roundToInt (hz * size / sampleRate));
                const float db = juce::Decibels::gainToDecibels (bins[(size_t) bin] * 4.0f / (float) size, -90.0f);
                const float y = plot.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f) * plot.getHeight();
                if (x == 0) path.startNewSubPath (plot.getX(), y); else path.lineTo (plot.getX() + (float) x, y);
            }
            if (! before)
            {
                auto fill = path; fill.lineTo (plot.getRight(), plot.getBottom()); fill.lineTo (plot.getX(), plot.getBottom()); fill.closeSubPath();
                g.setColour (colour.withAlpha (0.18f)); g.fillPath (fill);
            }
            g.setColour (colour.withAlpha (before ? 0.45f : 0.95f)); g.strokePath (path, juce::PathStrokeType (before ? 1.0f : 1.5f));
        };
        trace (mid, theme::textDim); trace (side, stereoScope::colour);
    };
    spectrum (processor.inputMeasurement, true);
    spectrum (processor.outputMeasurement, false);
    g.setColour (theme::textDim);
    for (int db : { -90, -60, -30, 0 })
    {
        const float y = plot.getBottom() - (float) (db + 90) * plot.getHeight() / 90.0f;
        g.drawText (juce::String (db) + " dB", juce::Rectangle<float> (plot.getRight() - 48.0f, juce::jlimit (plot.getY(), plot.getBottom() - 13.0f, y - 13.0f), 46.0f, 13.0f), juce::Justification::centredRight);
    }
    // Four seconds of input and output peak levels.
    auto history = all.withTop (plot.getBottom() + 20.0f).reduced (8.0f, 2.0f);
    const auto inBlocks = processor.inputMeasurement.getBlocks(), outBlocks = processor.outputMeasurement.getBlocks();
    auto historyLine = [&] (const std::vector<stereo::Block>& blocks, juce::Colour colour)
    {
        juce::Path line; const size_t start = blocks.size() > 80 ? blocks.size() - 80 : 0;
        for (size_t i = start; i < blocks.size(); ++i)
        {
            const float x = history.getRight() - (float) (blocks.size() - i) * history.getWidth() / 80.0f;
            const float db = juce::Decibels::gainToDecibels (blocks[i].peak, -90.0f);
            const float y = history.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f) * history.getHeight();
            if (i == start) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }
        g.setColour (colour); g.strokePath (line, juce::PathStrokeType (1.0f));
    };
    historyLine (inBlocks, theme::textDim); historyLine (outBlocks, stereoScope::colour);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("MID AND SIDE BY FREQUENCY", all.removeFromTop (20.0f).reduced (8.0f, 0), juce::Justification::centredLeft);
    auto in = scopeArea.removeFromLeft (scopeArea.getWidth() * 0.5f).reduced (3.0f);
    auto out = scopeArea.reduced (3.0f);
    auto draw = [&] (juce::Rectangle<float> area, const juce::String& name, const stereo::Measurement& m)
    {
        drawPanel (g, area);
        area.reduce (5.0f, 5.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (name, area.removeFromTop (18.0f), juce::Justification::centred);
        auto bars = area.removeFromBottom (42.0f);
        stereoScope::drawGoniometer (g, area, m.getPoints(), true, processor.getSampleRate());
        stereoScope::drawCorrelation (g, bars.removeFromTop (20.0f), "Low", m.getCorrelation (true));
        stereoScope::drawCorrelation (g, bars, "All", m.getCorrelation (false));
    };
    draw (in, "IN", processor.inputMeasurement);
    draw (out, "OUT", processor.outputMeasurement);
}
void StereoPictures::setFrequency (float x)
{
    auto r = spectrumBounds();
    const float v = juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth());
    processor.setKnob (stereo::Knob::monoBelow, v < 0.01f ? 0.0f : juce::jlimit (20.0f, 500.0f, 20.0f * std::pow (1000.0f, v)));
    repaint();
}
void StereoPictures::mouseMove (const juce::MouseEvent& e)
{
    if (onHover == nullptr) return;
    const auto plot = spectrumBounds();
    if (e.position.x <= plot.getRight()) onHover (50);
    else if (e.position.y > (float) getHeight() - 52.0f) onHover (53);
    else onHover (52);
}
void StereoPictures::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        const int frequencies[] = { 0, 80, 100, 120, 150, 200, 250 };
        for (int i = 0; i < 7; ++i) menu.addItem (i + 1, frequencies[i] == 0 ? "Off" : juce::String (frequencies[i]) + " Hz");
        menu.addItem (8, "Reset all"); menu.addSeparator(); guideicon::addItem (menu, 9, "What is stereo width?");
        const bool scope = ! spectrumBounds().contains (e.position);
        if (scope) { menu.clear(); guideicon::addItem (menu, 9, "What is stereo width?"); }
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<StereoPictures> (this), scope] (int n)
        {
            if (safe == nullptr) return;
            const int values[] = { 0, 80, 100, 120, 150, 200, 250 };
            if (n >= 1 && n <= 7) safe->processor.setKnob (stereo::Knob::monoBelow, (float) values[n - 1]);
            else if (n == 8) safe->processor.setSettings (stereo::Settings {});
            else if (n == 9 && safe->guide) safe->guide (scope ? 2 : 5);
        });
        return;
    }
    if (spectrumBounds().contains (e.position)) { dragFrequency = processor.getSettings().monoBelowHz; dragging = true; processor.getKnobParameter (stereo::Knob::monoBelow)->beginChangeGesture(); setFrequency (e.position.x); }
}
void StereoPictures::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    if (e.mods.isAltDown())
        processor.setKnob (stereo::Knob::monoBelow, juce::jlimit (20.0f, 500.0f, juce::jmax (20.0f, dragFrequency) * std::exp ((float) e.getDistanceFromDragStartX() * 0.002f)));
    else setFrequency (e.position.x);
}
void StereoPictures::mouseUp (const juce::MouseEvent&) { if (dragging) processor.getKnobParameter (stereo::Knob::monoBelow)->endChangeGesture(); dragging = false; }
void StereoPictures::mouseDoubleClick (const juce::MouseEvent& e) { if (spectrumBounds().contains (e.position)) processor.setKnob (stereo::Knob::monoBelow, 120.0f); }
void StereoPictures::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) { processor.setKnob (stereo::Knob::width, juce::jlimit (0.0f, 200.0f, processor.getSettings().width + wheel.deltaY * 20.0f)); }
juce::String StereoPictures::getTooltip()
{
    const auto plot = spectrumBounds();
    const auto mouse = getMouseXYRelative().toFloat();
    if (! plot.contains (mouse)) return "In: the sound going in. Out: coming out. Vertical is mono, a cloud is wide.";
    const double hz = 20.0 * std::pow (1000.0, (mouse.x - plot.getX()) / plot.getWidth());
    const auto points = processor.outputMeasurement.getPoints();
    if (points.size() < 64) return "Drag to set Mono below; Alt for fine steps; double-click for 120 Hz; scroll for Width.";
    const double rate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    double mr = 0.0, mi = 0.0, sr = 0.0, si = 0.0;
    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& point = points[i];
        const double l = point.left, r = point.right;
        const double window = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * (double) i / (double) (points.size() - 1));
        const double phase = juce::MathConstants<double>::twoPi * hz * (double) i / rate;
        const double c = std::cos (phase) * window, s = std::sin (phase) * window;
        mr += (l + r) * 0.5 * c; mi += (l + r) * 0.5 * s;
        sr += (l - r) * 0.5 * c; si += (l - r) * 0.5 * s;
    }
    const auto db = [&] (double real, double imaginary) { return juce::roundToInt (juce::Decibels::gainToDecibels (4.0 * std::hypot (real, imaginary) / (double) points.size(), -90.0)); };
    return juce::String (juce::roundToInt (hz)) + juce::String::fromUTF8 (" Hz · mid ") + juce::String (db (mr, mi)) + juce::String::fromUTF8 (" dB · side ") + juce::String (db (sr, si)) + " dB (after)";
}
BuiltInStereoEditor::BuiltInStereoEditor (BuiltInStereo& p)
    : EffectEditor (p, "DawIt Stereo", "What is stereo width?", "Hear what it removes", p.listenToRemoved, p.clap, stereoScope::colour, fx::Clap::Sound::wideBass),
      stereoProcessor (p), pictures (p, [this] (int page) { openGuide (page); }, [this] (int topic) { hover (topic); })
{
    addAndMakeVisible (pictures);
    const std::pair<stereo::Knob, const char*> names[] = { { stereo::Knob::monoBelow, "Mono below" }, { stereo::Knob::slope, "Slope" }, { stereo::Knob::width, "Width" }, { stereo::Knob::listen, "Listen" }, { stereo::Knob::output, "Output" } };
    for (const auto& [knob, name] : names)
        knobs.add (new EffectKnob (p.getParameters(), BuiltInStereo::getParameterId (knob), name, (int) knob, stereoScope::colour));
    knobs[0]->slider.textFromValueFunction = [] (double value) { return value < 20.0 ? juce::String ("Off") : juce::String (juce::roundToInt (value)) + " Hz"; };
    knobs[2]->slider.textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt (value)) + "%"; };
    knobs[4]->slider.textFromValueFunction = [] (double value) { return juce::String (value, 1) + " dB"; };
    for (auto index : { 0, 2, 4 })
        knobs[index]->slider.updateText();
    knobs[0]->slider.setTooltip ("Mono below: everything under this frequency is made the same in both speakers. 100 to 150 Hz keeps the kick and bass solid on small and club systems.");
    knobs[1]->slider.setTooltip ("Slope: how sharply mono turns into stereo at the frequency.");
    knobs[2]->slider.setTooltip ("Width above the Mono below frequency: 0% mono, 100% as it was, 200% wider.");
    knobs[3]->slider.setTooltip ("Listen: Stereo, Mono to check as a phone hears it, or Side to hear the width alone.");
    knobs[4]->slider.setTooltip ("Output level.");
    slope.button.setTooltip ("Slope: how sharply mono turns into stereo at the frequency.");
    listen.button.setTooltip ("Listen: Stereo, Mono to check as a phone hears it, or Side to hear the width alone.");
    slope.button.onClick = [this] { const int current = stereoProcessor.getSettings().slope; stereoProcessor.setKnob (stereo::Knob::slope, current == 12 ? 24.0f : current == 24 ? 48.0f : 12.0f); };
    listen.button.onClick = [this] { stereoProcessor.setKnob (stereo::Knob::listen, (float) ((stereoProcessor.getSettings().listen + 1) % 3)); };
    match.button.setTooltip ("Match level: same loudness in and out, for a fair comparison.");
    match.button.setColour (juce::TextButton::buttonOnColourId, stereoScope::colour.darker (0.4f));
    match.button.onClick = [this] { stereoProcessor.setMatchingLevel (! stereoProcessor.isMatchingLevel()); };
    setGroups ({ { "MONO BELOW", { knobs[0], &slope }, 0 }, { "WIDTH", { knobs[2] }, 0 }, { "CHECK", { &listen }, 0 }, { "LEVEL", { knobs[4], &match }, 0 } });
    juce::StringArray namesList;
    for (const auto& preset : stereo::getPresets()) namesList.add (preset.name);
    setPresetNames (namesList);
    clapButton.setButtonText ("Wide bass");
    clapButton.setTooltip ("Plays a test sound: a low A at 55 Hz and a shaker, both wide. Turn Mono below up and hear the bass move to the middle while the shaker stays wide.");
    tick(); showTopic (CommonTopic::overview); setSize (1240, 640);
}
void BuiltInStereoEditor::applyPreset (int index)
{
    const auto& list = stereo::getPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
    stereoProcessor.applyPreset (list[(size_t) index]); presetIndex = index;
    showPresetName (list[(size_t) index].name); showTopic (CommonTopic::presets);
}
std::vector<Guide::Page> BuiltInStereoEditor::createGuidePages() { return stereotext::makeGuide (stereoProcessor); }
void BuiltInStereoEditor::layoutPictures (juce::Rectangle<int> r) { pictures.setBounds (r); }
void BuiltInStereoEditor::tick()
{
    const auto settings = stereoProcessor.getSettings();
    slope.button.setButtonText (juce::String (settings.slope) + " dB/oct");
    listen.button.setButtonText (settings.listen == 0 ? "Stereo" : settings.listen == 1 ? "Mono" : "Side");
    match.button.setButtonText (stereoProcessor.isMatchingLevel() ? "On" : "Off");
    match.button.setToggleState (stereoProcessor.isMatchingLevel(), juce::dontSendNotification);
    pictures.repaint();
}
juce::AttributedString BuiltInStereoEditor::buildText (int topic)
{
    juce::AttributedString text; text.setWordWrap (juce::AttributedString::byWord);
    const char* titles[] = { "Mono below", "Slope", "Width", "Listen", "Output" };
    const char* descriptions[] = {
        "Everything under this frequency is made the same in both speakers: mono. Above it, the sound keeps its width. Typical: 100 to 150 Hz on a mix or a bus, 150 to 250 Hz on a bass. Off, all the way down, leaves the whole sound as it is.",
        "How quickly the sound changes from mono to stereo around the Mono below frequency. 12 dB per octave is gentle, 24 the usual choice, 48 sharp. An octave below the frequency the side is 14, 25 or 48 dB down.",
        "How wide the sound is above the Mono below frequency: 0% is mono, 100% as it was, 200% the side twice as big. Turning it up makes the sound louder too; Match level evens that out. Typical: 100%, or 110 to 140% for pads and reverbs.",
        "Stereo: the result. Mono: left and right added, in both speakers, as a phone or a club's subwoofers play it. Side: only what differs between left and right, to hear what the width is made of. Mono and Side are checks and are saved with the project: switch back to Stereo before you export.",
        "The level after everything else, from -12 to +12 dB." };
    if (topic >= 0 && topic < stereo::numKnobs) { addTitle (text, titles[topic], stereoScope::colour); addLead (text, descriptions[topic]); }
    else if (topic == CommonTopic::presets && juce::isPositiveAndBelow (presetIndex, (int) stereo::getPresets().size()))
    { const auto& p = stereo::getPresets()[(size_t) presetIndex]; addTitle (text, p.name, stereoScope::colour); addLead (text, p.why); }
    else if (topic == 50) { addTitle (text, "Mid and side by frequency", stereoScope::colour); addLead (text, "Mid and side by frequency. Grey: the mid, what both speakers share. Pink: the side, what differs; the thin line is before, the filled shape after. The amber part is made mono. Drag the line to set Mono below; double-click it for 120 Hz."); }
    else if (topic == 52) { addTitle (text, "Goniometers", stereoScope::colour); addLead (text, "Each dot is a moment of the sound: up is the mid, sideways the side. A vertical line is mono; leaning left or right, panned; a cloud, wide; lying flat, left and right cancel in mono. Amber dots are below the Mono below frequency."); }
    else if (topic == 53) { addTitle (text, "Correlation bars", stereoScope::colour); addLead (text, "From -1 to +1, over about a third of a second. Low: below the Mono below frequency. All: the whole sound. +1 is mono, 0 unrelated, below 0 parts cancel in mono."); }
    else if (topic == 51) { addTitle (text, "Match level", stereoScope::colour); addLead (text, "Keeps the sound as loud coming out as going in, so you hear the width and not a change of loudness. Not applied in Mono and Side."); }
    else { addTitle (text, "DawIt Stereo", stereoScope::colour); addLead (text, "Make the low end mono and set the width above it. The goniometers and correlation bars show the sound going in and coming out."); }
    return text;
}
}
