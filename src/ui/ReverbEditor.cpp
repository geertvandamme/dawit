#include "ReverbEditor.h"
#include "engine/ReverbKinds.h"

namespace daw
{
using verb::Knob;
using namespace fxui;

//==============================================================================
ReverbResponseView::ReverbResponseView (BuiltInReverb& p) : processor (p)
{
    setTopic (*this, verbtext::response);
}

juce::Point<float> ReverbResponseView::getHandlePosition (Handle h) const
{
    const auto s = processor.getSettings();
    const auto& r = cache.get (s);
    const double start = s.predelayMs / 1000.0;

    if (h == Handle::predelay)
        return { plot.toX (start), plot.toY (r.tailStartDb) };

    return { plot.toX (start + s.decaySeconds), plot.toY (r.tailStartDb - 60.0) };
}

ReverbResponseView::Handle ReverbResponseView::findHandle (juce::Point<float> position) const
{
    for (auto h : { Handle::decay, Handle::predelay })
        if (getHandlePosition (h).getDistanceFrom (position) < 11.0f)
            return h;

    return Handle::none;
}

void ReverbResponseView::paint (juce::Graphics& g)
{
    const auto s = processor.getSettings();
    drawPanel (g, getLocalBounds().toFloat());
    const double seconds = dragging != Handle::none ? dragSeconds : verbtext::getShownSeconds (s);
    plot = verbtext::drawResponse (g, getLocalBounds().toFloat().reduced (8.0f, 6.0f), cache.get (s), processor.tempoBpm.load(), seconds, true);

    for (auto h : { Handle::predelay, Handle::decay })
    {
        const float radius = h == hovered || h == dragging ? 8.0f : 6.0f;
        const auto c = getHandlePosition (h);
        g.setColour (h == Handle::predelay ? theme::accent : verbtext::colour);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c), 1.0f);
    }
}

void ReverbResponseView::mouseMove (const juce::MouseEvent& e)
{
    const auto h = findHandle (e.position);

    if (h != hovered)
    {
        hovered = h;
        repaint();
    }

    if (onHover)
        onHover (h == Handle::none ? (int) verbtext::response : (int) getKnob (h));
}

void ReverbResponseView::mouseDown (const juce::MouseEvent& e)
{
    dragging = findHandle (e.position);
    dragSeconds = plot.seconds;

    if (dragging != Handle::none)
        if (auto* p = processor.getKnobParameter (getKnob (dragging)))
            p->beginChangeGesture();
}

void ReverbResponseView::mouseDrag (const juce::MouseEvent& e)
{
    const auto s = processor.getSettings();
    const double t = plot.fromX (e.position.x);

    if (dragging == Handle::predelay)
        processor.setKnob (Knob::predelay, juce::jlimit (0.0f, verb::maxPredelay, (float) std::round (t * 1000.0)));
    else if (dragging == Handle::decay)
        processor.setKnob (Knob::decay, juce::jlimit (verb::minDecay, verb::maxDecay, (float) (std::round ((t - s.predelayMs / 1000.0) * 100.0) / 100.0)));

    repaint();
}

void ReverbResponseView::mouseUp (const juce::MouseEvent&)
{
    if (dragging != Handle::none)
        if (auto* p = processor.getKnobParameter (getKnob (dragging)))
            p->endChangeGesture();

    dragging = Handle::none;
    repaint();
}

void ReverbResponseView::mouseDoubleClick (const juce::MouseEvent&)
{
    const verb::Settings d;
    processor.setKnob (Knob::predelay, d.predelayMs);
    processor.setKnob (Knob::decay, d.decaySeconds);
}

//==============================================================================
/** The room, seen from above, moving after a clap. */
class BuiltInReverbEditor::Room final : public juce::Component
{
public:
    Room (BuiltInReverb& p, const double& clap) : processor (p), clapTime (clap)
    {
        setTopic (*this, verbtext::room);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (verb::kinds::getName (processor.getSettings().type).toUpperCase() + ": WHAT MAKES THE REVERB", r.removeFromTop (16.0f), juce::Justification::centredLeft);
        verbtext::drawKind (g, r, processor.getSettings(), juce::Time::getMillisecondCounterHiRes() / 1000.0 - clapTime, true);
    }

private:
    BuiltInReverb& processor;
    const double& clapTime;
};

//==============================================================================
BuiltInReverbEditor::BuiltInReverbEditor (BuiltInReverb& p)
    : EffectEditor (p, "DawIt Reverb", "What is reverb?", "Hear only the reverb", p.listenToEffect, p.clap, verbtext::colour),
      reverb (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [&p] { return p.tempoBpm.load(); }, "the reverb", verbtext::colour)
{
    room = std::make_unique<Room> (reverb, clapTime);
    addAndMakeVisible (response);
    addAndMakeVisible (*room);
    addAndMakeVisible (history);
    response.onHover = [this] (int topic) { hover (topic); };

    const std::pair<Knob, const char*> names[] = { { Knob::predelay, "Pre-delay" }, { Knob::size, "Size" }, { Knob::decay, "Decay" },
                                                   { Knob::damping, "Damping" }, { Knob::lowCut, "Low cut" }, { Knob::early, "Early" },
                                                   { Knob::width, "Width" }, { Knob::mix, "Mix" } };

    for (auto& [knob, name] : names)
        knobs.add (new EffectKnob (reverb.getParameters(), BuiltInReverb::getParameterId (knob), name, (int) knob, verbtext::colour));

    typeSwitch.button.setTooltip ("Type: what makes the reverb: Digital, Room, Hall, Chamber, Plate or Spring. Click to choose; the knobs keep their settings.");
    typeSwitch.button.onClick = [this]
    {
        juce::PopupMenu menu;
        const char* labels[] = { "Digital: DawIt's own, any space", "Room: close walls, quick early echoes", "Hall: far walls, a gap, a slow swell", "Chamber: a hard room, a speaker and a microphone", "Plate: a steel sheet, dense and bright", "Spring: repeats and a boing, as in guitar amps" };
        for (int index = 0; index < 6; ++index)
            menu.addItem (index + 1, labels[index], true, (int) reverb.getSettings().type == index);
        menu.addSeparator();
        menu.addItem (7, "What are the kinds of reverb?...");
        const juce::Component::SafePointer<BuiltInReverbEditor> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&typeSwitch.button), [safe] (int result)
        {
            if (safe == nullptr || result == 0) return;
            if (result == 7) safe->openGuide (5);
            else
            {
                safe->reverb.setType ((verb::Type) (result - 1));
                safe->showTopic (verbtext::type);
            }
        });
    };

    setGroups ({ { "KIND", { &typeSwitch } }, { "WHEN", { knobs[0] } }, { "THE ROOM", { knobs[1], knobs[2], knobs[3] } },
                 { "ITS SOUND", { knobs[4], knobs[5], knobs[6] } }, { "HOW MUCH", { knobs[7] } } });

    juce::StringArray presetNames;

    for (auto& preset : verb::getPresets())
        presetNames.add (preset.name + " (" + verb::kinds::getName (preset.settings.type) + ")");

    setPresetNames (presetNames);
    showTopic (CommonTopic::overview);
    setSize (1240, 620);
}

void BuiltInReverbEditor::applyPreset (int index)
{
    const auto& list = verb::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    reverb.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInReverbEditor::createGuidePages()
{
    return verbtext::makeGuide (reverb);
}

void BuiltInReverbEditor::layoutPictures (juce::Rectangle<int> r)
{
    room->setBounds (r.removeFromRight (290));
    r.removeFromRight (10);
    response.setBounds (r.removeFromTop (r.getHeight() * 3 / 5));
    r.removeFromTop (10);
    history.setBounds (r);
}

void BuiltInReverbEditor::tick()
{
    const auto settings = reverb.getSettings();
    typeSwitch.button.setButtonText (verb::kinds::getName (settings.type));
    const bool earlyEnabled = settings.type != verb::Type::plate && settings.type != verb::Type::spring;
    knobs[5]->setEnabled (earlyEnabled);
    knobs[5]->setAlpha (earlyEnabled ? 1.0f : 0.4f);
    knobs[5]->slider.setTooltip (earlyEnabled ? "Early reflections: the first echoes off the walls." : "Early reflections: a " + verb::kinds::getName (settings.type).toLowerCase() + " has no walls, so it has no early reflections. This knob works on the other types.");
    history.update();
    response.repaint();
    room->repaint();
}

void BuiltInReverbEditor::clapped()
{
    clapTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
}

juce::AttributedString BuiltInReverbEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = reverb.getSettings();
    const double bpm = reverb.tempoBpm.load();
    const auto& list = verb::getPresets();
    const verb::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, verbtext::colour);
        addLead (text, preset->why);
        addSection (text, "Type: " + verb::kinds::getName (preset->settings.type), preset->typeNote);
        addSection (text, {}, "A start, not an answer: play the track and adjust by ear, the mix most of all.");

        for (int k = 0; k < verb::numKnobs; ++k)
            addSection (text, verbtext::getTitle (k) + ": " + verb::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    if (topic == verbtext::type)
    {
        addTitle (text, "Type", verbtext::colour);
        addLead (text, verb::kinds::describeSettings (s));
        addSection (text, "What it does", verb::kinds::getTypeDescription());
        addSection (text, "When and how it's set", verb::kinds::getTypeTypical());
        return text;
    }

    if (topic >= 0 && topic < verb::numKnobs)
    {
        addTitle (text, verbtext::getTitle (topic), verbtext::colour);
        addLead (text, verbtext::describeSettings (topic, s, bpm));
        addSection (text, "What it does", verbtext::describe (topic));
        addSection (text, "When and how it's set", verbtext::getTypical (topic));

        if (preset != nullptr)
            addSection (text, "In the " + preset->name + " starting point: " + verb::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);
        else
            addSection (text, {}, "Pick a starting point to read why it's set the way it is for that sound.");

        return text;
    }

    addTitle (text, verbtext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "Reverb puts a sound in a space. In every room, a sound reaches you first straight away and then again "
                           "and again off the walls, thousands of times, quieter each time, until it dies away. That is reverb; "
                           "without it, a recording sounds dry and close, as if it were played inside your head.");
            addSection (text, "When to use it", "In almost every mix: to put dry recordings (vocals, guitars, synths made or recorded "
                                                "without a room) in a space so they sound real, to place parts at a distance (more reverb "
                                                "sounds further back), and to make the parts sound like one band in one room. Big and long "
                                                "as an effect on pads, ballads and build-ups. Usually little or none on bass and kick, and "
                                                "less on fast, busy parts, which it blurs.");
            addSection (text, "How it's usually set", "Short rooms (0.3-0.8 s) on drums and guitars, 1-2 s on vocals and most "
                                                      "instruments, long halls for ballads and pads. A mix of 10-30%, the low cut at "
                                                      "150-300 Hz so the lows stay clear, and 20-100 ms of pre-delay on vocals so the words "
                                                      "stay in front. Similar settings on parts that belong in one room, and last in the "
                                                      "chain, after EQ, compression, distortion and delay.");
            addSection (text, "How it works", "After the Pre-delay come the early reflections, the first echoes off the nearest "
                                              "walls (Early, spread out by the Size), then the tail, which lasts the Decay time. "
                                              "Damping lets its highs die away sooner, Low cut keeps it out of the lows, Width sets "
                                              "how stereo it is, and Mix how much of it you hear.");
            addSection (text, "Reading the window", "Left at the top, what one clap turns into: white the clap, orange the early "
                                                    "reflections, green the tail. Under it, the last 4 seconds of the track and its "
                                                    "reverb. Right, the room the size stands for. Hover anything to read about it here; "
                                                    "scroll here when the text runs long.");
            addSection (text, "Getting started", "Pick a starting point for your sound; each tells why every knob is set the way it "
                                                 "is. Click Clap to hear the room, even while the song is stopped. Read What is reverb? "
                                                 "for how reverb works, from bouncing sound to mixing.");
            break;

        case CommonTopic::presets:
            addSection (text, {}, verbtext::describe (topic));
            break;

        case CommonTopic::clap:
            addLead (text, "Claps into the reverb with the settings as they are now.");
            addSection (text, {}, verbtext::describe (topic));
            break;

        case CommonTopic::listen:
            addLead (text, reverb.listenToEffect.load() ? "On: you hear only the reverb." : "Off: you hear the track with its reverb.");
            addSection (text, {}, verbtext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [in, effect] = history.getLoudest();

            if (in > -70.0f && effect > -90.0f)
                addLead (text, "The last 4 seconds: the track up to " + juce::String (in, 1) + " dB, its reverb up to " + juce::String (effect, 1)
                                   + " dB.");
            else
                addLead (text, "Play the track, or click Clap, to see it here.");

            addSection (text, {}, verbtext::describe (topic));
            break;
        }

        default:
            addLead (text, verbtext::describeSettings (topic, s, bpm));
            addSection (text, {}, verbtext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
