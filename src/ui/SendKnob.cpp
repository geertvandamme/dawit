#include "SendKnob.h"
#include "engine/Routing.h"

namespace daw
{
SendKnob::SendKnob (Engine& e, TrackActions& a, Track& s, Track& r, Look l)
    : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox), engine (e), actions (a), sender (&s), returnTrack (&r), look (l)
{
    setScrollWheelEnabled (false);
    setRange (sendOffDb, 6.0, 0.1);
    setSkewFactorFromMidPoint (-12.0);
    setDoubleClickReturnValue (true, sendOffDb);
    setPopupDisplayEnabled (true, true, nullptr);
    textFromValueFunction = [] (double v) { return v <= sendOffDb ? juce::String ("Off") : juce::String (v, 1) + " dB"; };
    setColour (juce::Slider::rotarySliderFillColourId, r.colour);
    setColour (juce::Slider::rotarySliderOutlineColourId, theme::gridBar);
    setWantsKeyboardFocus (false);
    refresh();

    onValueChange = [this]
    {
        if (isAlive())
            engine.setSend (*sender, *returnTrack, (float) getValue());
    };
}

bool SendKnob::isAlive() const
{
    // A track or a return can go before the views catch up; never touch one that is gone.
    return engine.indexOf (sender) >= 0 && engine.indexOf (returnTrack) >= 0;
}

Send* SendKnob::getSend() const
{
    return isAlive() ? routing::findSend (*sender, *returnTrack) : nullptr;
}

void SendKnob::refresh()
{
    if (! isAlive())
        return;

    const auto* send = getSend();
    const double level = send != nullptr && send->isOn() ? (double) send->levelDb.load() : (double) sendOffDb;

    if (! isMouseButtonDown() && ! juce::approximatelyEqual ((float) getValue(), (float) level))
        setValue (level, juce::dontSendNotification);

    setColour (juce::Slider::rotarySliderFillColourId, returnTrack->colour);

    const auto text = returnTrack->name + "|" + juce::String (level, 1) + (send != nullptr && send->preFader.load() ? "|pre" : "")
                      + returnTrack->colour.toString();

    if (text != shownText)
    {
        shownText = text;
        repaint();
    }
}

void SendKnob::paint (juce::Graphics& g)
{
    if (! isAlive())
        return;

    const auto dialBounds = juce::Rectangle<int> (0, (getHeight() - dialSize) / 2, dialSize, dialSize);
    const auto area = dialBounds.toFloat().reduced (3.0f);
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto centre = area.getCentre();
    const float from = juce::MathConstants<float>::pi * 1.2f, to = juce::MathConstants<float>::pi * 2.8f;
    const float angle = from + (to - from) * (float) valueToProportionOfLength (getValue());
    const bool on = getValue() > (double) sendOffDb;

    juce::Path trackArc, value;
    trackArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, from, to, true);
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, from, angle, true);
    g.setColour (theme::gridBar.brighter (0.4f));
    g.strokePath (trackArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (on)
    {
        g.setColour (returnTrack->colour);
        g.strokePath (value, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (on ? returnTrack->colour.brighter (0.4f) : theme::textDim);
    g.fillEllipse (centre.x + radius * std::sin (angle) - 2.0f, centre.y - radius * std::cos (angle) - 2.0f, 4.0f, 4.0f);

    // The name to the right of the dial, in the return's colour; PRE for a send before the fader, and in the wide look the level.
    auto cell = getLocalBounds().withTrimmedLeft (dialSize + 3);

    if (look == Look::wide)
    {
        g.setColour (on ? theme::text : theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (on ? juce::String (getValue(), 1) + " dB" : juce::String ("Off"), cell.removeFromRight (52), juce::Justification::centredRight);
    }

    if (const auto* send = getSend(); send != nullptr && send->isOn() && send->preFader.load())
    {
        g.setColour (theme::meterYellow);
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText ("PRE", cell.removeFromRight (look == Look::wide ? 24 : 18), juce::Justification::centredRight);
    }

    g.setColour (returnTrack->colour);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (returnTrack->name, cell, juce::Justification::centredLeft, true);
}

bool SendKnob::hitTest (int x, int y)
{
    if (look == Look::wide)
        return juce::Slider::hitTest (x, y);

    return juce::Rectangle<int> (0, (getHeight() - dialSize) / 2, dialSize, dialSize).contains (x, y);
}

void SendKnob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (isAlive())
            actions.showSendMenu (*sender, *returnTrack, this);

        return;
    }

    juce::Slider::mouseDown (e);
}

juce::String SendKnob::getTooltip()
{
    if (! isAlive())
        return {};

    const auto* send = getSend();

    if (send == nullptr || ! send->isOn())
        return "Send to " + returnTrack->name + ": off. Drag up to send some of this track to the " + returnTrack->name + " return.";

    return "Send to " + returnTrack->name + ": how much of this track goes to the " + returnTrack->name + " return, now "
           + juce::String (send->levelDb.load(), 1) + " dB, taken " + (send->preFader.load() ? "before" : "after")
           + " the fader. Drag up for more; double-click to turn it off. Right-click for pre- or post-fader.";
}
} // namespace daw
