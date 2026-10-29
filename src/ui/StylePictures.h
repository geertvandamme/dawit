#pragma once

#include "generate/SongTemplates.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
/** Draws the tempo map, mini arrangement, drum grid and routing used by both song-template windows and PDFs. */
class StylePicture final : public juce::Component, public juce::TooltipClient
{
public:
    enum class Mode { tempoMap, arrangement, groove, routing };

    /** Shows one kind of picture for a song plan; the style may be null only for a tempo map. */
    void show (Mode, const gen::templates::SongPlan&, const gen::templates::Style*, double songTempo = 120.0);

    /** The item under the mouse, phrased as a beginner-facing tooltip. */
    juce::String getTooltip() override;
    void paint (juce::Graphics&) override;

private:
    void paintTempoMap (juce::Graphics&, juce::Rectangle<float>) const;
    void paintArrangement (juce::Graphics&, juce::Rectangle<float>) const;
    void paintGroove (juce::Graphics&, juce::Rectangle<float>) const;
    void paintRouting (juce::Graphics&, juce::Rectangle<float>) const;

    Mode mode = Mode::arrangement;
    gen::templates::SongPlan plan;
    const gen::templates::Style* style = nullptr;
    double songTempo = 120.0;
};
} // namespace daw
