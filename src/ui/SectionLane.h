#pragma once

#include "engine/Engine.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The song's sections (intro, verse, chorus, drop...), a row under the ruler that is always there.
    Drag across bars to mark a section and pick its type; drag a section's edge to make it longer or
    shorter; click it to change its type, rename it, loop it or delete it.
    Like the chord track it makes no sound: it shows the song's structure.
*/
class SectionLane final : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener
{
public:
    SectionLane (Engine&, TimelineView&);
    ~SectionLane() override;

    static constexpr int height = 24;

    /** The colour of a section type, which other views use too. */
    static juce::Colour getColour (SectionType);

    /** Called with the clicked bar when "Song structure guide..." is picked. */
    std::function<void (int bar)> onShowGuide;

    void updatePlayhead();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    enum class Drag { none, create, resizeStart, resizeEnd };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    int barAt (float x) const;
    int barEdgeAt (float x) const;   // the nearest bar line
    int sectionIndexAt (int bar) const;
    Drag edgeAt (float x, int& sectionIndex) const;

    /** The bars around `bar` that no section uses: [first, end). */
    std::pair<int, int> getFreeRange (int bar) const;

    void showAddMenu (int firstBar, int endBar);
    void showSectionMenu (int index);
    void rename (int index);
    void addSection (Section);

    Engine& engine;
    TimelineView& timeline;
    int hoverBar = -1;
    float playheadX = -100.0f;

    Drag drag = Drag::none;
    int dragIndex = -1, dragAnchorBar = 0;
    std::optional<Section> preview;   // the section being drawn or resized

    static constexpr int defaultBars = 8;
};
} // namespace daw
