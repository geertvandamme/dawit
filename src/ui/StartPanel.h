#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Theme.h"

namespace daw
{
/**
    "Start your song": what an empty song shows in the track area instead of a blank screen. A heading, a dim line and
    five large tiles, each the start of a real task (Start from a style, Add an instrument or drums, Record voice or
    instrument, Import audio, Learn), then the line about dropping audio files.

    The tiles are real buttons: keyboard focus in Tab order, Return or Space, a tooltip and an accessible title and
    description. They are at least 120 px wide: in a narrower area they wrap into rows of 3 and 2, and on a short area
    the tiles shrink and a tile gives up its subtitle (the tooltip keeps it). The panel itself is click-through apart from
    its tiles, so audio files still drop onto the lanes below it. ArrangementView shows it only while the song has no
    tracks. Message thread only.
*/
class StartPanel final : public juce::Component
{
public:
    enum class Choice { style, instrument, record, importAudio, learn };

    StartPanel();
    ~StartPanel() override;

    /** Called when a tile is clicked, with the tile (the instrument menu opens beside it). */
    std::function<void (Choice, juce::Component& tile)> onChoice;

    /** The tile of a choice, to point at or press. */
    juce::Component& getTile (Choice);

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int minTileWidth = 120;

private:
    class Tile;

    juce::OwnedArray<Tile> tiles;
    juce::Rectangle<int> headingArea, lineArea, footerArea;
};
} // namespace daw
