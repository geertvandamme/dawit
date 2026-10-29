#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "GainStagingExplainer.h"
#include "PluginWindows.h"
#include "Selection.h"
#include "Theme.h"
#include "TrackActions.h"

namespace daw
{
/**
    Shows the selected track's instrument and effects as a row of chips in signal order, then its fader
    and the master, each with meters and peak dB readouts for the level going in and coming out
    (instruments: out only).
    DawIt EQ, DawIt Compressor and DawIt Gate also show a tiny picture of their settings and the signal: the EQ
    curve over the spectrum of what comes out, or the compressor's or the gate's curve of level in against level
    out with a dot that follows the track.
    Click a chip to open its editor (for the built-in synth: its sound picker), click its power dot to bypass it, click its x to remove it,
    drag an effect to reorder the chain, and right-click it for more options. Chips keep their size:
    a chain longer than the bar scrolls sideways.

    It also shows the gain staging (see GainStaging.h): the arrows between the chips take the colour of
    the level passing through them, with its peak above them; each effect has a badge with how much
    louder or quieter it makes the sound (on DawIt EQ, Compressor and Gate a click switches match
    level); and a ! under an arrow points at something worth a look. "Gain staging?" opens the guide.

    A track in a bus shows the buses it plays into after its fader (click one to see that bus's own chain), and
    a bus shows what feeds it first: its tracks added up.

    A track's sends that are on show as half-height chips in a second row below the line, taken before the fader
    chip (pre-fader) or after it (post-fader): a short branch drops from the line into each, with the return's
    colour, its name, the level and a meter. Drag one up or down to change the level, click it to see the return's
    chain, right-click it for the send's menu. A return's chain starts with the tracks that send to it.
*/
class DeviceBar final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    DeviceBar (Engine&, Selection&, PluginWindowManager&, TrackActions&);
    ~DeviceBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int preferredHeight = 46, sendRowHeight = 22, sendRowGap = 4;

    /** The height the bar wants: a second row more while the shown track has sends that are on. */
    int getPreferredHeight() const noexcept { return preferredHeight + (shownSends.empty() ? 0 : sendRowHeight + sendRowGap); }

    /** Called when getPreferredHeight() changed, so the window can lay out again. */
    std::function<void()> onPreferredHeightChanged;

    /** Called when the fader or master chip is clicked. */
    std::function<void()> onShowMixer;

    /** Opens the gain staging guide at a topic. */
    void showGuide (gainstaging::Topic);

    /** The gain staging guide for the PDF export: the open window's content, or one made for the export. */
    guideprint::Item getPrintGuide();

    /** The chain's levels and hints right now, for the guide. */
    gainstaging::Snapshot getSnapshot() const;

private:
    class Chip;

    /** The chips side by side, inside the viewport; draws the arrows between them and the drop marker. */
    struct ChipRow final : public juce::Component, public juce::TooltipClient
    {
        std::function<void (juce::Graphics&)> onPaint, onPaintOverChildren;
        std::function<juce::String (juce::Point<int>)> onTooltip;
        std::function<void (juce::Point<int>)> onClick;
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
        void paintOverChildren (juce::Graphics& g) override { if (onPaintOverChildren) onPaintOverChildren (g); }
        juce::String getTooltip() override { return onTooltip ? onTooltip (getMouseXYRelative()) : juce::String(); }
        void mouseUp (const juce::MouseEvent& e) override { if (onClick && ! e.mouseWasDraggedSinceMouseDown()) onClick (e.getPosition()); }
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refresh();

    void paintArrows (juce::Graphics&);
    void paintBranches (juce::Graphics&);
    std::vector<std::pair<const Track*, bool>> getSends (const Track*) const;
    int getFaderIndex() const;
    void paintDropMarker (juce::Graphics&);

    // The gap between chip `arrow` and the next one, the ! in it, and the hint it shows (or null):
    // the hint about the chip the arrow goes into, or for the first arrow also about the first chip.
    juce::Rectangle<int> getArrowArea (int arrow) const;
    juce::Rectangle<int> getMarkerArea (int arrow) const;
    const gainstaging::Hint* getArrowHint (int arrow) const;
    void updateGainStaging();

    // Reordering effects by dragging their chips. Positions are relative to the chip row.
    void dragChip (Chip&, const juce::MouseEvent&);
    void endChipDrag();
    std::vector<Chip*> getEffectChipsExcept (const Chip*) const;

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    TrackActions& actions;

    Track* shownTrack = nullptr;
    std::vector<const PluginSlot*> shownSlots;
    std::vector<const Track*> shownBuses;    // the buses the track plays into, innermost first
    std::vector<std::pair<const Track*, bool>> shownSends;   // the returns its sends that are on go to, and whether before the fader
    juce::OwnedArray<Chip> sendChips;
    std::vector<gainstaging::Stop> sendStops;
    juce::String shownFeed;                  // a bus: the names of the tracks that feed it
    bool shownBuiltInSynth = false;
    BuiltInSound shownSound = BuiltInSound::piano;

    juce::Viewport viewport;
    ChipRow chipRow;
    juce::OwnedArray<Chip> chips;
    Chip* draggedChip = nullptr;
    int dragGrabX = 0, dropIndex = -1, dropMarkerX = -1;
    juce::TextButton addInstrumentButton { "+ Instrument" }, addEffectButton { "+ Effect" };
    GuideButton guideButton { "Gain staging?" };

    std::vector<gainstaging::Stop> stops;     // one per chip
    std::vector<gainstaging::Hint> hints;
    std::vector<juce::String> shownArrows;    // what each arrow showed, to repaint only changes
    int guideKey = 0;                         // the guide's window, by address
    juce::Component::SafePointer<GainStagingExplainer> guide;

    static constexpr int titleWidth = 150, arrowGap = 30, scrollBarThickness = 5;
};
} // namespace daw
