#include "DelayEditor.h"

namespace daw
{
using dly::Knob;
using namespace fxui;

//==============================================================================
DelayEchoView::DelayEchoView (BuiltInDelay& p) : processor (p)
{
    setTopic (*this, dlytext::echoes);
}

std::optional<juce::Point<float>> DelayEchoView::getHandlePosition (Handle h) const
{
    const auto s = processor.getSettings();
    const auto list = dlytext::getEchoes (s, processor.tempoBpm.load(), 2);

    if (list.empty())
        return std::nullopt;

    const auto& first = list.front();

    if (h == Handle::time)
        return juce::Point<float> { plot.toX (first.seconds), plot.toY (first.onLeft ? plot.left : plot.right, first.db) };

    // Without feedback there is no second echo: the dot waits at the bottom, to be pulled up.
    const auto& lane = s.mode == dly::Mode::pingPong ? plot.right : plot.left;
    const double db = list.size() > 1 ? list[1].db : -60.0;
    return juce::Point<float> { plot.toX (2.0 * first.seconds), plot.toY (lane, db) };
}

DelayEchoView::Handle DelayEchoView::findHandle (juce::Point<float> position) const
{
    for (auto h : { Handle::feedback, Handle::time })
        if (const auto c = getHandlePosition (h); c.has_value() && c->getDistanceFrom (position) < 11.0f)
            return h;

    return Handle::none;
}

juce::RangedAudioParameter* DelayEchoView::getParameter (Handle h) const
{
    if (h == Handle::feedback)
        return processor.getKnobParameter (Knob::feedback);

    if (processor.getSettings().sync)
        return dynamic_cast<juce::RangedAudioParameter*> (processor.getParameters().getParameter (BuiltInDelay::noteId));

    return processor.getKnobParameter (Knob::time);
}

void DelayEchoView::paint (juce::Graphics& g)
{
    const auto s = processor.getSettings();
    const double bpm = processor.tempoBpm.load();
    drawPanel (g, getLocalBounds().toFloat());
    const double seconds = dragging != Handle::none ? dragSeconds : dlytext::getShownSeconds (s, bpm);
    plot = dlytext::drawEchoes (g, getLocalBounds().toFloat().reduced (8.0f, 6.0f), s, bpm, seconds, true);

    for (auto h : { Handle::time, Handle::feedback })
    {
        if (const auto c = getHandlePosition (h))
        {
            const float radius = h == hovered || h == dragging ? 8.0f : 6.0f;
            g.setColour (h == Handle::time ? theme::accent : dlytext::colour);
            g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (*c));
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (*c), 1.0f);
        }
    }
}

void DelayEchoView::mouseMove (const juce::MouseEvent& e)
{
    const auto h = findHandle (e.position);

    if (h != hovered)
    {
        hovered = h;
        repaint();
    }

    if (onHover)
        onHover (h == Handle::time ? (int) Knob::time : h == Handle::feedback ? (int) Knob::feedback : (int) dlytext::echoes);
}

void DelayEchoView::mouseDown (const juce::MouseEvent& e)
{
    dragging = findHandle (e.position);
    dragSeconds = plot.seconds;

    if (auto* p = dragging != Handle::none ? getParameter (dragging) : nullptr)
        p->beginChangeGesture();
}

void DelayEchoView::mouseDrag (const juce::MouseEvent& e)
{
    const auto s = processor.getSettings();
    const double bpm = processor.tempoBpm.load();

    if (dragging == Handle::time)
    {
        const double ms = juce::jmax (1.0, plot.fromX (e.position.x) * 1000.0);

        if (s.sync)
            processor.setNote (dly::findNearestNote (ms, bpm));
        else
            processor.setKnob (Knob::time, juce::jlimit (dly::minTime, dly::maxTime, (float) std::round (ms)));
    }
    else if (dragging == Handle::feedback)
    {
        // The height of the second echo, next to the first, is the feedback.
        const auto list = dlytext::getEchoes (s, bpm, 1);
        const auto& lane = s.mode == dly::Mode::pingPong ? plot.right : plot.left;
        const double db = juce::jmap ((double) e.position.y, (double) lane.getBottom(), (double) lane.getY() + 2.0, -60.0, 0.0);
        const double relative = juce::jmin (0.0, db - (list.empty() ? 0.0 : list.front().db));
        const float feedback = db <= -59.0 ? 0.0f : (float) std::round (100.0 * juce::Decibels::decibelsToGain (relative));
        processor.setKnob (Knob::feedback, juce::jlimit (0.0f, dly::maxFeedback, feedback));
    }

    repaint();
}

void DelayEchoView::mouseUp (const juce::MouseEvent&)
{
    if (auto* p = dragging != Handle::none ? getParameter (dragging) : nullptr)
        p->endChangeGesture();

    dragging = Handle::none;
    repaint();
}

void DelayEchoView::mouseDoubleClick (const juce::MouseEvent&)
{
    const dly::Settings d;
    processor.setKnob (Knob::time, d.timeMs);
    processor.setNote (d.note);
    processor.setKnob (Knob::feedback, d.feedbackPercent);
}

//==============================================================================
/** You and the wall that would give the echo. */
class BuiltInDelayEditor::Wall final : public juce::Component
{
public:
    Wall (BuiltInDelay& p, const double& clap) : processor (p), clapTime (clap)
    {
        setTopic (*this, dlytext::wall);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("WHERE THIS ECHO COMES FROM", r.removeFromTop (16.0f), juce::Justification::centredLeft);
        dlytext::drawWall (g, r, processor.getSettings(), processor.tempoBpm.load(), juce::Time::getMillisecondCounterHiRes() / 1000.0 - clapTime, true);
    }

private:
    BuiltInDelay& processor;
    const double& clapTime;
};

//==============================================================================
BuiltInDelayEditor::BuiltInDelayEditor (BuiltInDelay& p)
    : EffectEditor (p, "DawIt Delay", "What is delay?", "Hear only the echoes", p.listenToEffect, p.clap, dlytext::colour),
      delay (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [&p] { return p.tempoBpm.load(); }, "the echoes", dlytext::colour)
{
    wall = std::make_unique<Wall> (delay, clapTime);
    addAndMakeVisible (echoes);
    addAndMakeVisible (*wall);
    addAndMakeVisible (history);
    echoes.onHover = [this] (int topic) { hover (topic); };

    const std::pair<Knob, const char*> names[] = { { Knob::time, "Time" }, { Knob::feedback, "Feedback" }, { Knob::highCut, "High cut" },
                                                   { Knob::lowCut, "Low cut" }, { Knob::mix, "Mix" } };

    for (auto& [knob, name] : names)
        knobs.add (new EffectKnob (delay.getParameters(), BuiltInDelay::getParameterId (knob), name, (int) knob, dlytext::colour));

    syncSwitch.button.onClick = [this] { delay.setSync (! delay.getSettings().sync); };
    modeSwitch.button.onClick = [this]
    {
        delay.setKnob (Knob::mode, delay.getSettings().mode == dly::Mode::pingPong ? 0.0f : 1.0f);
    };

    setGroups ({ { "WHEN", { knobs[0], &syncSwitch } }, { "HOW MANY", { knobs[1] } }, { "THEIR SOUND", { knobs[2], knobs[3] } },
                 { "WHERE", { &modeSwitch } }, { "HOW MUCH", { knobs[4] } } });

    juce::StringArray presetNames;

    for (auto& preset : dly::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInDelayEditor::applyPreset (int index)
{
    const auto& list = dly::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    delay.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInDelayEditor::createGuidePages()
{
    return dlytext::makeGuide (delay);
}

void BuiltInDelayEditor::layoutPictures (juce::Rectangle<int> r)
{
    wall->setBounds (r.removeFromRight (290).removeFromTop (r.getHeight() * 3 / 5));
    r.removeFromRight (10);
    echoes.setBounds (r.removeFromTop (r.getHeight() * 3 / 5));
    r.removeFromTop (10);
    history.setBounds (r.withWidth (r.getWidth() + 300));
}

void BuiltInDelayEditor::tick()
{
    const auto s = delay.getSettings();

    // The time knob is a note value while it follows the tempo.
    knobs[0]->attachTo (s.sync ? BuiltInDelay::noteId : BuiltInDelay::getParameterId (Knob::time));
    knobs[0]->label.setText (s.sync ? "Note" : "Time", juce::dontSendNotification);
    syncSwitch.button.setButtonText (s.sync ? "On" : "Off");
    syncSwitch.button.setToggleState (s.sync, juce::dontSendNotification);
    syncSwitch.button.setColour (juce::TextButton::buttonOnColourId, dlytext::colour.darker (0.4f));
    modeSwitch.button.setButtonText (dly::formatKnob (Knob::mode, s));

    history.update();
    echoes.repaint();
    wall->repaint();
}

void BuiltInDelayEditor::clapped()
{
    clapTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
}

juce::AttributedString BuiltInDelayEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = delay.getSettings();
    const double bpm = delay.tempoBpm.load();
    const auto& list = dly::getPresets();
    const dly::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, dlytext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear, the mix and feedback most of all.");

        for (int k = 0; k < dly::numKnobs; ++k)
            addSection (text, dlytext::getTitle (k) + ": " + dly::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    const bool isControl = (topic >= 0 && topic < dly::numKnobs) || topic == dlytext::sync;

    if (isControl)
    {
        const int knob = topic == dlytext::sync ? (int) Knob::time : topic;
        addTitle (text, dlytext::getTitle (topic), dlytext::colour);
        addLead (text, dlytext::describeSettings (topic, s, bpm));
        addSection (text, "What it does", dlytext::describe (topic));
        addSection (text, "When and how it's set", dlytext::getTypical (knob));

        if (preset != nullptr)
            addSection (text, "In the " + preset->name + " starting point: " + dly::formatKnob ((Knob) knob, preset->settings), preset->notes[(size_t) knob]);
        else
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that use.");

        return text;
    }

    addTitle (text, dlytext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "A delay plays a sound again a moment later: an echo. With feedback, the echo is played again, and again, "
                           "each time a little quieter, like a shout bouncing between two cliffs.");
            addSection (text, "When to use it", "When a part should get space without the wash of a reverb: echoes behind a vocal or "
                                                "lead in the gaps between phrases, a slapback on a rock'n'roll or country vocal or guitar, "
                                                "repeats that are part of a guitar or synth pattern, a short delay that makes a mono part "
                                                "wide, and long trails as an effect in dub, ambient and build-ups. Less on busy parts, "
                                                "where the echoes clutter.");
            addSection (text, "How it's usually set", "In time (1/4, 1/8 or a dotted 1/8) for echoes in the groove, free at 80-150 ms for "
                                                      "slapback or 15-35 ms for doubling. Feedback 20-40% for a few repeats, a mix of "
                                                      "10-30% so the echoes stay behind the sound, the high cut at 3-6 kHz and the low cut "
                                                      "at 150-300 Hz so they stay dark, thin and out of the way. Before the reverb in the chain.");
            addSection (text, "How it works", "The Time is how long after the sound its echo comes: a note value of the song's tempo "
                                              "(Sync on) or milliseconds. Feedback makes more echoes, the High cut and Low cut make each "
                                              "repeat darker and thinner, the Mode keeps the echoes in place or bounces them left and "
                                              "right, and Mix sets how loud they are.");
            addSection (text, "Reading the window", "Left at the top, a clap and its echoes in the left and right speaker, against the "
                                                    "beats. Under it, the last 4 seconds of the track and its echoes. Right, how far away a "
                                                    "wall would be to give this echo. Hover anything to read about it here; scroll here when "
                                                    "the text runs long.");
            addSection (text, "Getting started", "Pick a starting point for your sound; each tells why every control is set the way it "
                                                 "is. Click Clap to hear the echoes, even while the song is stopped. Read What is delay? "
                                                 "for how echoes and delays work.");
            break;

        case CommonTopic::clap:
            addLead (text, "Claps into the delay with the settings as they are now.");
            addSection (text, {}, dlytext::describe (topic));
            break;

        case CommonTopic::listen:
            addLead (text, delay.listenToEffect.load() ? "On: you hear only the echoes." : "Off: you hear the track with its echoes.");
            addSection (text, {}, dlytext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [in, effect] = history.getLoudest();

            if (in > -70.0f && effect > -90.0f)
                addLead (text, "The last 4 seconds: the track up to " + juce::String (in, 1) + " dB, its echoes up to " + juce::String (effect, 1) + " dB.");
            else
                addLead (text, "Play the track, or click Clap, to see it here.");

            addSection (text, {}, dlytext::describe (topic));
            break;
        }

        default:
        {
            const auto lead = dlytext::describeSettings (topic, s, bpm);

            if (lead.isNotEmpty())
                addLead (text, lead);

            addSection (text, {}, dlytext::describe (topic));
            break;
        }
    }

    return text;
}
} // namespace daw
