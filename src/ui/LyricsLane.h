#pragma once

#include "engine/Engine.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The lyrics track, a row under the chord track that is always there. Each sung syllable is a
    block at its beat, and above them a bar for each line, from where it starts to where the singing
    stops. It makes no sound.

    Type the whole lyrics first (double-click, or editText()): one line per sung line, an empty
    line between verses, "[Chorus]" above a verse, hyphens inside words for their syllables. They
    are spread over the song's sections. Then fix the timing:
    - drag a syllable: it is placed there (pinned), and the unplaced ones around it stretch or
      squeeze up to the next placed one; Alt drags without snapping;
    - drag a line's bar to move the whole line, or its right end to move where the line stops;
    - tap along (startTapping()): while the song plays, each key press places the next syllable
      at the playhead, the first of a line moving the whole line. The row opens while tapping and
      shows the line being tapped with the next syllable lit, the line after it, and which keys
      do what. Esc stops.
    Zoomed out, each line shows as one block with its words. Opened (setExpanded()), the row also
    writes every line out in full under the syllables, readable at any zoom: lines too close
    together for their words take turns on three rows, and the syllable being sung lights up.
*/
class LyricsLane final : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener
{
public:
    LyricsLane (Engine&, TimelineView&);
    ~LyricsLane() override;

    static constexpr int height = 40;   // closed, and the syllables' strip when open

    /** Opening writes the lines out in full under the syllables. Kept in the app's settings. */
    void setExpanded (bool shouldBeExpanded);
    bool isExpanded() const noexcept { return expanded; }

    /** The row's height: `height`, or taller when open. */
    int getPreferredHeight() const noexcept;

    /** Called when the row opens or closes, so the layout and header can follow. */
    std::function<void()> onLayoutChanged;

    /** The colour of the lyrics, which other views use too. */
    static inline const juce::Colour colour { 0xffc9a2ff };

    /** A line's words as they're sung, without the hyphens: "to-ge-ther now" gives "together now". */
    static juce::String getLineText (const LyricLine&);

    /** Opens the window to type or edit the whole text: asks the owner, through `onEditText`. */
    void editText();

    /** Set by the owner (ArrangementView): opens the Lyrics window. */
    std::function<void()> onEditText;

    /** Tap along from the first syllable at or after `fromBeat` (by default the playhead), starting
        playback a bar before it when the song isn't playing. */
    void startTapping (std::optional<double> fromBeat = std::nullopt);

    /** Ends tapping; with `stopPlayback`, also stops the song when tapping started it. */
    void stopTapping (bool stopPlayback = false);
    bool isTapping() const noexcept { return tapTarget.has_value(); }

    /** Called when "How lyrics are written..." is picked. */
    std::function<void()> onShowGuide;

    /** Called when "Sing along with the lyrics..." is picked: the words large, to read while recording vocals. */
    std::function<void()> onSingAlong;
    std::function<void()> onSingLyrics;   // Sing the lyrics with a computer voice

    /** Called when tapping starts or stops, so the header can follow. */
    std::function<void()> onTappingChanged;

    void updatePlayhead();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusLost (FocusChangeType) override;

private:
    enum class Target { none, syllable, line, lineEnd };

    struct Hit
    {
        Target target = Target::none;
        lyrics::Position position;   // the syllable, or the line (syllable 0)
    };

    /** Where a line's words go when the row is open: its text row, from its start to where its
        words end, cut off where later words on the same row begin. */
    struct TextPlacement
    {
        size_t line = 0;
        int row = 0;
        float x = 0.0f, textEnd = 0.0f, clipEnd = 0.0f;
    };

    std::vector<TextPlacement> getTextPlacements (const Lyrics&) const;

    /** While tapping, in the open part of the row: the line being tapped, the next one, and the keys. */
    void paintTapGuide (juce::Graphics&, const Lyrics&);
    float getTextRowY (int row) const noexcept { return (float) height + 3.0f + (float) row * textRowHeight; }

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    const Lyrics& getShown() const { return preview.has_value() ? *preview : engine.getLyrics(); }
    bool isOverview (const LyricLine&) const;
    Hit findHit (juce::Point<float>) const;
    double snap (double beat, const juce::ModifierKeys&) const;
    double getHeardBeat() const;   // the playhead minus the audio output's latency
    void showMenu (const Hit&, double beat);
    void updateHover (const Hit&);

    Engine& engine;
    TimelineView& timeline;
    float playheadX = -100.0f;
    Hit hover;

    Hit drag;
    Lyrics dragOriginal;
    double dragOffset = 0.0;           // from the mouse to what's dragged, in beats
    bool dragged = false;
    std::optional<Lyrics> preview;     // the lyrics as they'll be when the drag ends

    std::optional<lyrics::Position> tapTarget;   // the syllable the next tap places
    bool openedForTapping = false, playingForTapping = false;
    std::optional<lyrics::Position> sung;        // being sung at the playhead, highlighted
    bool expanded = false;

    static constexpr float lineTop = 3.0f, lineHeight = 9.0f, syllableTop = 15.0f, syllableHeight = 22.0f;
    static constexpr int textRows = 3;
    static constexpr float textRowHeight = 19.0f, textFontHeight = 13.5f;
};
} // namespace daw
