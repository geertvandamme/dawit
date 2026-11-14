#include "StartPanel.h"
#include "generate/SongTemplates.h"

namespace daw
{
/** One tile: a title, a subtitle and a tooltip that says the whole of what it does. */
class StartPanel::Tile final : public juce::Button
{
public:
    Tile (const juce::String& titleText, const juce::String& subtitleText, const juce::String& tooltipText)
        : juce::Button (titleText), title (titleText), subtitle (subtitleText)
    {
        setTooltip (tooltipText);
        setTitle (titleText);
        setDescription (subtitleText + ". " + tooltipText);
        setWantsKeyboardFocus (true);
        setMouseClickGrabsKeyboardFocus (false);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    /** Whether the subtitle is drawn (the tooltip always keeps it). */
    void setShowSubtitle (bool show)
    {
        if (show != showSubtitle)
        {
            showSubtitle = show;
            repaint();
        }
    }

    bool isShowingSubtitle() const noexcept { return showSubtitle; }

    void paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        const bool focused = hasKeyboardFocus (false);

        g.setColour (isButtonDown ? theme::panelLight.brighter (0.15f) : isMouseOver ? theme::panelLight.brighter (0.08f) : theme::panelLight);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (focused ? theme::accent : isMouseOver ? theme::accent.withAlpha (0.7f) : theme::gridBar);
        g.drawRoundedRectangle (r, 8.0f, focused ? 2.0f : 1.0f);

        auto inner = getLocalBounds().reduced (12, 10);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        const int titleHeight = showSubtitle ? juce::jmin (inner.getHeight(), 36) : inner.getHeight();
        g.drawFittedText (title, inner.removeFromTop (titleHeight), showSubtitle ? juce::Justification::topLeft : juce::Justification::centredLeft, 2, 0.7f);

        if (showSubtitle && inner.getHeight() > 12)
        {
            inner.removeFromTop (4);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.5f));
            g.drawFittedText (subtitle, inner, juce::Justification::topLeft, 4, 0.8f);
        }
    }

private:
    juce::String title, subtitle;
    bool showSubtitle = true;
};

StartPanel::StartPanel()
{
    setInterceptsMouseClicks (false, true);   // only the tiles take the mouse; a file dropped here reaches the lanes
    setAccessible (true);
    setTitle ("Start your song");

    const auto styles = juce::String ((int) gen::templates::getStyles().size());

    struct Def { Choice choice; const char* title; juce::String subtitle, tooltip; };
    const Def defs[] = {
        { Choice::style, "Start from a style", "A whole song sketch in one of " + styles + " styles",
          "Opens New Project from Style (Cmd+Shift+N). Choose a style, key and tempo, and DawIt builds the sections, chords and tracks of a song, "
          "with notes to start from if you want them. It replaces this empty song and asks to save first if you changed anything." },
        { Choice::instrument, "Add an instrument or drums", "MIDI: notes you can change, played by a sound you choose",
          "Adds a MIDI track. MIDI isn't sound: it's the notes to play, and the track's instrument turns them into sound. "
          "So you can change any note, or the whole instrument, later. Choose a sound from the list." },
        { Choice::record, "Record voice or instrument", "Audio: the sound itself, from a microphone or an instrument",
          "Adds an audio track for a recording. Audio is the sound itself: you can cut it, move it and add effects, but not change its notes "
          "the way you can with MIDI. Nothing records until you press Record." },
        { Choice::importAudio, "Import audio", "Bring in a recording or a loop, such as a WAV file",
          "Choose audio files to put on a new audio track at the playhead (Cmd+I). You can also drop files here." },
        { Choice::learn, "Learn", "The guides, and a five-minute first exercise",
          "Opens Theory Guides at Start here (Cmd+G). The first thing there is a short exercise in your own song: hear a note, "
          "change a chord, add a rhythm and save." },
        { Choice::learnSong, "Learn from a song you like", "Find its chords and play along",
          "Opens Learn the Chords of a Song: a short exercise with a song file you choose. You bring it in, DawIt finds its chords, "
          "you play along with them and swap one to hear the difference. The file stays on your computer." } };

    for (const auto& d : defs)
    {
        auto* tile = tiles.add (new Tile (d.title, d.subtitle, d.tooltip));
        tile->onClick = [this, choice = d.choice, tile] { if (onChoice != nullptr) onChoice (choice, *tile); };
        addAndMakeVisible (tile);
    }
}

StartPanel::~StartPanel() = default;

juce::Component& StartPanel::getTile (Choice choice)
{
    return *tiles[(int) choice];
}

void StartPanel::resized()
{
    constexpr int margin = 16, gap = 8, headingHeight = 30, lineHeight = 22, footerHeight = 24, preferredTileHeight = 118, minTileHeight = 56;
    const int width = getWidth(), height = getHeight();
    const int count = tiles.size();

    // One row, unless the tiles would be narrower than 120 px: then rows of 3.
    const bool oneRow = (width - 2 * margin - (count - 1) * gap) / count >= minTileWidth;
    const int perRow = oneRow ? count : 3;
    const int rowCount = (count + perRow - 1) / perRow;

    // Give up the dim line, then the footer, before the tiles get smaller than their text.
    bool showLine = true, showFooter = true;
    auto needed = [&] (int tileHeight) { return 12 + headingHeight + (showLine ? lineHeight + 4 : 0) + rowCount * tileHeight + (rowCount - 1) * gap + (showFooter ? footerHeight + 4 : 0) + 8; };
    int tileHeight = preferredTileHeight;

    while (tileHeight > minTileHeight && needed (tileHeight) > height)
        tileHeight -= 4;

    if (needed (tileHeight) > height)
        showLine = false;

    if (needed (tileHeight) > height)
        showFooter = false;

    int y = 12;
    const int left = margin, areaWidth = juce::jmax (0, width - 2 * margin);

    headingArea = { left, y, areaWidth, headingHeight };
    y += headingHeight;
    lineArea = {};

    if (showLine)
    {
        lineArea = { left, y, areaWidth, lineHeight };
        y += lineHeight + 4;
    }
    else
        y += 4;

    const int tileWidth = juce::jmax (1, (areaWidth - (perRow - 1) * gap) / perRow);

    for (int i = 0; i < count; ++i)
    {
        const int row = i / perRow, column = i % perRow;
        auto* tile = tiles[i];
        tile->setBounds (left + column * (tileWidth + gap), y + row * (tileHeight + gap), tileWidth, tileHeight);
        tile->setShowSubtitle (tileWidth >= minTileWidth && tileHeight >= 84);
    }

    y += rowCount * tileHeight + (rowCount - 1) * gap + 4;
    footerArea = showFooter ? juce::Rectangle<int> (left, y, areaWidth, footerHeight) : juce::Rectangle<int>();
}

void StartPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::lane);

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawFittedText ("Start your song", headingArea, juce::Justification::centredLeft, 1, 0.7f);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (13.0f));

    if (! lineArea.isEmpty())
        g.drawFittedText ("Pick one way to begin. You can add the others at any time.", lineArea, juce::Justification::centredLeft, 1, 0.7f);

    if (! footerArea.isEmpty())
        g.drawFittedText ("You can also drop audio files here to import them.", footerArea, juce::Justification::centredLeft, 1, 0.7f);
}
} // namespace daw
