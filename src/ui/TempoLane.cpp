#include "TempoLane.h"
#include "engine/Tempo.h"

namespace daw
{
TempoLane::TempoLane (Engine& e, TimelineView& t) : engine (e), timeline (t)
{
    engine.addChangeListener (this);
}

TempoLane::~TempoLane()
{
    engine.removeChangeListener (this);
}

double TempoLane::getEndBeat() const
{
    const auto& changes = engine.getTempoMap().getChanges();
    return juce::jmax (engine.getSongEndBar() * 4.0, changes.back().beat + 4.0);
}

void TempoLane::updatePlayhead()
{
    const float x = timeline.beatToX (engine.getPositionBeats());

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 5, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 5, getHeight());
}

void TempoLane::paint (juce::Graphics& g)
{
    g.fillAll (theme::laneAlt);

    const auto& tempo = engine.getTempoMap();
    const auto& changes = tempo.getChanges();
    const double endBeat = getEndBeat();
    const auto range = tempo.getRange (0.0, endBeat);

    // Scaled between the slowest and fastest tempo, with a few BPM around them so a band's small
    // wobbles don't look like big jumps.
    const double middle = range.getStart() + range.getLength() * 0.5, span = juce::jmax (8.0, range.getLength() + 4.0);
    const float top = 5.0f, bottom = (float) getHeight() - 5.0f;
    const auto yOf = [&] (double bpm) { return bottom - (float) ((bpm - (middle - span * 0.5)) / span) * (bottom - top); };

    juce::Path line, fill;
    const float endX = timeline.beatToX (endBeat);

    for (size_t i = 0; i < changes.size(); ++i)
    {
        const float x1 = timeline.beatToX (changes[i].beat);
        const float x2 = i + 1 < changes.size() ? timeline.beatToX (changes[i + 1].beat) : endX;
        const float y = yOf (changes[i].bpm);

        if (i == 0)
        {
            line.startNewSubPath (x1, y);
            fill.startNewSubPath (x1, bottom + 5.0f);
            fill.lineTo (x1, y);
        }
        else
        {
            line.lineTo (x1, y);
            fill.lineTo (x1, y);
        }

        line.lineTo (x2, y);
        fill.lineTo (x2, y);
    }

    fill.lineTo (endX, bottom + 5.0f);
    fill.closeSubPath();

    g.setColour (colour.withAlpha (0.16f));
    g.fillPath (fill);
    g.setColour (colour);
    g.strokePath (line, juce::PathStrokeType (1.6f));

    // The BPM at a change where there's room for it, skipping ones too close to the last label.
    g.setFont (juce::FontOptions (10.5f));
    float labelEnd = -1000.0f;

    for (auto& change : changes)
    {
        const float x = timeline.beatToX (change.beat);

        if (x < labelEnd + 6.0f || x > (float) getWidth())
            continue;

        const auto text = formatBpm (std::round (change.bpm * 10.0) / 10.0);
        const float w = juce::GlyphArrangement::getStringWidth (juce::FontOptions (10.5f), text) + 4.0f;
        const float y = yOf (change.bpm);
        const auto area = juce::Rectangle<float> (x + 2.0f, y < (float) getHeight() * 0.5f ? y + 1.0f : y - 13.0f, w, 12.0f);

        if (area.getRight() > 0.0f)
        {
            g.setColour (theme::text.withAlpha (0.85f));
            g.drawText (text, area, juce::Justification::centredLeft);
        }

        labelEnd = area.getRight();
    }

    g.setColour (theme::playhead); // where updatePlayhead() last put it, as in TrackLanes
    g.fillRect (playheadX, 0.0f, 1.0f, (float) getHeight());

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

juce::String TempoLane::getTooltip()
{
    const double beat = timeline.xToBeat ((float) getMouseXYRelative().x);

    if (beat < 0.0)
        return {};

    const int bar = (int) std::floor (beat / 4.0);
    return "Bar " + juce::String (bar + 1) + juce::String::fromUTF8 ("  \xc2\xb7  ") + formatBpm (engine.getTempoMap().getBpmAt (bar * 4.0)) + " BPM"
           + "\nThe tempo bar by bar, following a recording that was played without a click, so the bars stay with the band. "
             "Right-click to make it steady again";
}

void TempoLane::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        const auto& tempo = engine.getTempoMap();
        const double average = std::round (tempo.getAverageBpm (0.0, getEndBeat()) * 100.0) / 100.0;

        juce::PopupMenu menu;
        menu.addItem (1, "Make the tempo steady (" + formatBpm (average) + " BPM, the average)...");
        menu.addSeparator();
        menu.addItem (2, "Note lengths, beats and tempo...");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                            [safeThis = juce::Component::SafePointer<TempoLane> (this)] (int result)
                            {
                                if (auto* self = safeThis.getComponent())
                                {
                                    if (result == 1)
                                        self->makeSteady();
                                    else if (result == 2 && self->onShowRhythmGuide != nullptr)
                                        self->onShowRhythmGuide();
                                }
                            });
        return;
    }

    if (e.mouseWasClicked())
    {
        const double beat = juce::jmax (0.0, timeline.xToBeat (e.position.x));
        engine.setPositionBeats (e.mods.isAltDown() ? beat : std::round (beat));
    }
}

void TempoLane::makeSteady()
{
    const double average = std::round (engine.getTempoMap().getAverageBpm (0.0, getEndBeat()) * 100.0) / 100.0;

    auto* window = new juce::AlertWindow ("Make the tempo steady",
                                          "The song gets one tempo, " + formatBpm (average) + " BPM, the average of its bars. "
                                          "The bars then stop following the recording: they drift away from the band where it "
                                          "plays faster or slower. Detect tempo on the recording can follow it again.",
                                          juce::MessageBoxIconType::NoIcon, this);
    window->addButton ("Make it steady", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    window->enterModalState (true, juce::ModalCallbackFunction::create ([safeThis = juce::Component::SafePointer<TempoLane> (this), average] (int result)
    {
        if (auto* self = safeThis.getComponent(); self != nullptr && result == 1)
            self->engine.setTempo (average);
    }), true);
}
} // namespace daw
