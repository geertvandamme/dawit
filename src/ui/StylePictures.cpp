#include "StylePictures.h"

#include "SectionLane.h"
#include "Theme.h"

namespace daw
{
namespace
{
    float tempoX (juce::Rectangle<float> area, double bpm)
    {
        return area.getX() + area.getWidth() * (float) ((bpm - 50.0) / 140.0);
    }
}

void StylePicture::show (Mode newMode, const gen::templates::SongPlan& newPlan, const gen::templates::Style* newStyle, double newSongTempo)
{
    mode = newMode;
    plan = newPlan;
    style = newStyle;
    songTempo = newSongTempo;
    repaint();
}

juce::String StylePicture::getTooltip()
{
    switch (mode)
    {
        case Mode::tempoMap: return "Each bar is a style's usual tempo range. The red line is your song.";
        case Mode::arrangement: return "Coloured sections across the top; each row shows where a part plays.";
        case Mode::groove: return style != nullptr ? style->name + " drum steps; brighter squares are louder hits." : juce::String();
        case Mode::routing: return "The drum tracks share a bus; dashed lines are sends to the Reverb return.";
    }
    return {};
}

void StylePicture::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelLight);
    auto area = getLocalBounds().toFloat().reduced (10.0f);
    switch (mode)
    {
        case Mode::tempoMap: paintTempoMap (g, area); break;
        case Mode::arrangement: paintArrangement (g, area); break;
        case Mode::groove: paintGroove (g, area); break;
        case Mode::routing: paintRouting (g, area); break;
    }
}

void StylePicture::paintTempoMap (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto& styles = gen::templates::getStyles();
    if (style != nullptr)
    {
        auto label = area.removeFromLeft (90.0f);
        auto rangeArea = area.reduced (6.0f, 0.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawFittedText (style->name, label.toNearestInt(), juce::Justification::centredLeft, 1);
        const float y = rangeArea.getCentreY();
        g.setColour (theme::gridBar);
        g.fillRect (rangeArea.getX(), y, rangeArea.getWidth(), 1.0f);
        const auto xOf = [&] (double bpm) { return tempoX (rangeArea, bpm); };
        g.setColour (theme::accent);
        g.fillRoundedRectangle ({ xOf (style->minimumBpm), y - 7.0f, xOf (style->maximumBpm) - xOf (style->minimumBpm), 14.0f }, 3.0f);
        g.setColour (theme::text);
        g.fillRect (xOf (style->defaultBpm), y - 12.0f, 2.0f, 24.0f);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (juce::String (style->minimumBpm), rangeArea.withHeight (18.0f), juce::Justification::topLeft);
        g.drawText (juce::String (style->defaultBpm) + " BPM", rangeArea.withHeight (18.0f), juce::Justification::centredTop);
        g.drawText (juce::String (style->maximumBpm), rangeArea.withHeight (18.0f), juce::Justification::topRight);
        return;
    }
    const float row = area.getHeight() / (float) styles.size();
    g.setFont (juce::FontOptions (10.5f));

    for (size_t i = 0; i < styles.size(); ++i)
    {
        const auto& item = styles[i];
        const float y = area.getY() + row * (float) i;
        g.setColour (theme::textDim);
        g.drawFittedText (item.name, juce::Rectangle<float> (area.getX(), y, 88.0f, row).toNearestInt(), juce::Justification::centredLeft, 1);
        auto range = juce::Rectangle<float> (tempoX (area.withTrimmedLeft (92.0f), item.minimumBpm), y + row * 0.28f,
                                             tempoX (area.withTrimmedLeft (92.0f), item.maximumBpm) - tempoX (area.withTrimmedLeft (92.0f), item.minimumBpm), row * 0.44f);
        g.setColour (style == &item ? theme::accent : theme::accent.withAlpha (0.38f));
        g.fillRoundedRectangle (range, 2.0f);
        g.setColour (theme::text);
        g.fillRect (tempoX (area.withTrimmedLeft (92.0f), item.defaultBpm), y + row * 0.18f, 1.5f, row * 0.64f);
    }

    g.setColour (theme::playhead);
    g.fillRect (tempoX (area.withTrimmedLeft (92.0f), songTempo), area.getY(), 2.0f, area.getHeight());
}

void StylePicture::paintArrangement (juce::Graphics& g, juce::Rectangle<float> area) const
{
    if (plan.sections.empty()) return;
    const int bars = juce::jmax (1, plan.getTotalBars());
    const float labelWidth = 64.0f;
    auto timeline = area.withTrimmedLeft (labelWidth);
    const auto xOf = [&] (double beat) { return timeline.getX() + timeline.getWidth() * (float) beat / (float) (bars * 4); };
    const float sectionHeight = 24.0f;

    for (const auto& section : plan.sections)
    {
        auto block = juce::Rectangle<float> (xOf (section.startBar * 4.0), area.getY(), xOf (section.getEndBar() * 4.0) - xOf (section.startBar * 4.0), sectionHeight).reduced (1.0f);
        g.setColour (SectionLane::getColour (section.type));
        g.fillRoundedRectangle (block, 2.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawFittedText (section.getDisplayName(), block.toNearestInt().reduced (2, 0), juce::Justification::centred, 1);
    }

    const float rowHeight = juce::jmax (13.0f, (area.getHeight() - sectionHeight - 3.0f) / (float) juce::jmax (1, (int) plan.parts.size()));
    for (size_t i = 0; i < plan.parts.size(); ++i)
    {
        const auto& part = plan.parts[i];
        const float y = area.getY() + sectionHeight + 3.0f + rowHeight * (float) i;
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawFittedText (part.name, juce::Rectangle<float> (area.getX(), y, labelWidth - 4.0f, rowHeight).toNearestInt(), juce::Justification::centredLeft, 1);
        for (const auto& clip : part.clips)
        {
            auto block = juce::Rectangle<float> (xOf (clip.startBeat), y + 2.0f, juce::jmax (2.0f, xOf (clip.startBeat + clip.lengthBeats) - xOf (clip.startBeat) - 1.0f), rowHeight - 4.0f);
            g.setColour ((part.drum ? theme::meterYellow : theme::accent).withAlpha (clip.notes.empty() ? 0.28f : 0.72f));
            g.fillRoundedRectangle (block, 2.0f);
        }
    }
}

void StylePicture::paintGroove (juce::Graphics& g, juce::Rectangle<float> area) const
{
    if (style == nullptr) return;
    double beats = style->id == "trap" ? 8.0 : 4.0;
    int steps = style->id == "blues" ? 12 : (int) beats * 4;
    const float labelWidth = 64.0f;
    auto grid = area.withTrimmedLeft (labelWidth);
    const float row = area.getHeight() / (float) juce::jmax (1, (int) style->drums.size());

    for (int step = 0; step <= steps; ++step)
    {
        const float x = grid.getX() + grid.getWidth() * (float) step / (float) steps;
        g.setColour (step % (style->id == "blues" ? 3 : 4) == 0 ? theme::gridBar : theme::grid);
        g.fillRect (x, grid.getY(), 1.0f, grid.getHeight());
    }
    for (size_t i = 0; i < style->drums.size(); ++i)
    {
        const auto& drum = style->drums[i];
        const float y = area.getY() + row * (float) i;
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText (drum.name, juce::Rectangle<float> (area.getX(), y, labelWidth - 4.0f, row).toNearestInt(), juce::Justification::centredLeft, 1);
        for (const auto& hit : drum.hits)
        {
            for (double beat = hit.beat; beat < beats; beat += drum.patternBeats)
            {
                const float x = grid.getX() + grid.getWidth() * (float) (beat / beats);
                g.setColour (theme::meterYellow.withAlpha (juce::jlimit (0.35f, 1.0f, hit.velocity)));
                g.fillRoundedRectangle ({ x + 2.0f, y + 3.0f, juce::jmax (3.0f, grid.getWidth() / (float) steps - 4.0f), row - 6.0f }, 2.0f);
            }
        }
    }
}

void StylePicture::paintRouting (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const juce::StringArray names { "Kick", "Snare", "Hats", "Drums bus", "Bass", "Chords", "Pad", "Melody", "Reverb", "Master" };
    const int columns[] { 0, 0, 0, 1, 1, 1, 1, 1, 2, 3 };
    const float columnWidth = area.getWidth() / 4.0f;
    const float rowHeight = area.getHeight() / 6.0f;
    std::vector<juce::Rectangle<float>> boxes;
    int rowAtColumn[] { 0, 0, 1, 2 };

    for (int i = 0; i < names.size(); ++i)
    {
        const int column = columns[i];
        const int row = rowAtColumn[column]++;
        auto box = juce::Rectangle<float> (area.getX() + columnWidth * (float) column + 5.0f,
                                           area.getY() + rowHeight * (float) row + 4.0f, columnWidth - 18.0f, rowHeight - 8.0f);
        boxes.push_back (box);
        g.setColour (i == 8 ? theme::accent.withAlpha (0.5f) : theme::panel);
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (10.5f, i == 3 || i == 8 || i == 9 ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (names[i], box.toNearestInt().reduced (3), juce::Justification::centred, 1);
    }

    auto line = [&] (int from, int to, bool send)
    {
        g.setColour (send ? theme::accent.withAlpha (0.65f) : theme::textDim);
        const auto& fromBox = boxes[(size_t) from];
        const auto& toBox = boxes[(size_t) to];
        const juce::Line<float> path ({ fromBox.getRight(), fromBox.getCentreY() }, { toBox.getX(), toBox.getCentreY() });
        if (send)
        {
            const float dashes[] { 5.0f, 4.0f };
            g.drawDashedLine (path, dashes, 2, 1.0f);
        }
        else
        {
            g.drawLine (path, 1.5f);
        }
    };
    line (0, 3, false); line (1, 3, false); line (2, 3, false); line (3, 9, false);
    for (int i = 4; i <= 7; ++i) line (i, 9, false);
    line (1, 8, true); line (5, 8, true); line (6, 8, true); line (7, 8, true); line (8, 9, false);
}
} // namespace daw
