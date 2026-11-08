#include "QuantiseDialog.h"
#include "GuideButton.h"
#include "Theme.h"

namespace daw::quantiseui
{
namespace
{
    /** True while the engine still has this clip (the dialog is modal, but a clip must never be touched after it is gone). */
    bool exists (const Engine& engine, const MidiClip& clip)
    {
        for (auto& track : engine.getTracks())
            for (auto& c : track->clips)
                if (c.get() == &clip)
                    return true;

        return false;
    }

    bool inScope (const MidiClip& clip, const juce::SortedSet<int>& selection, int index)
    {
        return (selection.isEmpty() || selection.contains (index)) && clip.notes[(size_t) index].start < clip.lengthBeats;
    }

    //==============================================================================
    /** Up to four bars from the first note in scope: the grid lines, every note as an outline where it is now and filled
        where it will be, joined by a line. */
    class Picture final : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        void setData (const MidiClip& c, const quantise::Notes& after, const juce::SortedSet<int>& sel, double bar, double grid)
        {
            clip = &c;
            afterNotes = after;
            selection = sel;
            barLength = bar;
            gridBeats = grid;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            g.setColour (theme::background);
            g.fillRoundedRectangle (area, 4.0f);

            if (clip == nullptr || clip->notes.empty() || afterNotes.size() != clip->notes.size())
                return;

            double first = 1.0e9;

            for (int i = 0; i < (int) clip->notes.size(); ++i)
                if (inScope (*clip, selection, i))
                    first = juce::jmin (first, clip->startBeat + clip->notes[(size_t) i].start);

            if (first > 1.0e8)
                first = clip->startBeat;

            const double viewStart = std::floor (first / barLength + 1.0e-9) * barLength;
            const double clipEnd = clip->startBeat + clip->lengthBeats;
            const double viewEnd = juce::jlimit (viewStart + barLength, viewStart + 4.0 * barLength, std::ceil (clipEnd / barLength - 1.0e-9) * barLength);
            const double viewLength = viewEnd - viewStart;

            auto inner = area.reduced (6.0f, 4.0f);
            const auto top = inner.removeFromTop (12.0f);
            const float width = inner.getWidth();
            auto xOf = [&] (double songBeat) { return inner.getX() + (float) ((songBeat - viewStart) / viewLength) * width; };

            // Grid lines: each bar line, and the grid's steps inside each bar when they are not too tight to see.
            const bool showSteps = (double) width * gridBeats / viewLength >= 3.0;
            g.setFont (juce::FontOptions (10.5f));

            for (double bar = viewStart; bar < viewEnd - 1.0e-9; bar += barLength)
            {
                g.setColour (theme::gridBar);
                g.fillRect (xOf (bar), area.getY() + 2.0f, 1.5f, area.getHeight() - 4.0f);
                g.setColour (theme::textDim);
                g.drawText ("Bar " + juce::String ((int) std::floor (bar / barLength + 1.0e-9) + 1), juce::Rectangle<float> (xOf (bar) + 4.0f, top.getY(), 60.0f, top.getHeight()),
                            juce::Justification::centredLeft, false);

                if (showSteps)
                    for (double at = gridBeats; at < barLength - 1.0e-9; at += gridBeats)
                    {
                        g.setColour (theme::grid.brighter (0.2f));
                        g.fillRect (xOf (bar + at), inner.getY(), 1.0f, inner.getHeight());
                    }
            }

            g.setColour (theme::gridBar);
            g.fillRect (xOf (viewEnd) - 1.0f, area.getY() + 2.0f, 1.5f, area.getHeight() - 4.0f);

            int low = 127, high = 0;

            for (auto& n : clip->notes)
                if (clip->startBeat + n.start >= viewStart && clip->startBeat + n.start < viewEnd)
                {
                    low = juce::jmin (low, n.pitch);
                    high = juce::jmax (high, n.pitch);
                }

            if (low > high)
                return;

            const int rows = high - low + 3;   // a free row above and below
            const float rowHeight = inner.getHeight() / (float) rows;
            const float noteHeight = juce::jlimit (2.0f, 10.0f, rowHeight - 1.0f);
            auto yOf = [&] (int pitch) { return inner.getBottom() - rowHeight * ((float) (pitch - low) + 1.5f); };
            auto block = [&] (const MidiNote& n, double length)
            {
                const float x0 = xOf (clip->startBeat + n.start);
                return juce::Rectangle<float> (x0, yOf (n.pitch) - noteHeight * 0.5f, juce::jmax (3.0f, xOf (clip->startBeat + n.start + length) - x0), noteHeight);
            };

            g.saveState();
            g.reduceClipRegion (inner.toNearestInt());

            for (int i = 0; i < (int) clip->notes.size(); ++i)
            {
                const auto& before = clip->notes[(size_t) i];
                const auto& after = afterNotes[(size_t) i];

                if (before.start < viewStart - clip->startBeat - 1.0e-9 || clip->startBeat + before.start >= viewEnd)
                    continue;

                if (! inScope (*clip, selection, i))
                {
                    g.setColour (theme::textDim.withAlpha (0.45f));
                    g.fillRoundedRectangle (block (before, before.length), 2.0f);
                    continue;
                }

                const auto was = block (before, before.length), now = block (after, after.length);

                if (! juce::exactlyEqual (before.start, after.start) || ! juce::exactlyEqual (before.length, after.length))
                {
                    g.setColour (theme::textDim);
                    g.drawLine (was.getX(), was.getCentreY(), now.getX(), now.getCentreY(), 1.0f);
                }

                g.setColour (theme::accent.withAlpha (0.9f));
                g.drawRoundedRectangle (was.reduced (0.5f), 2.0f, 1.0f);
                g.setColour (theme::accent.withMultipliedBrightness (0.6f + 0.4f * after.velocity));
                g.fillRoundedRectangle (now, 2.0f);
            }

            g.restoreState();
        }

    private:
        const MidiClip* clip = nullptr;
        quantise::Notes afterNotes;
        juce::SortedSet<int> selection;
        double barLength = 4.0, gridBeats = 0.25;
    };

    //==============================================================================
    class Content final : public juce::Component
    {
    public:
        Content (Engine& e, MidiClip& c, const juce::SortedSet<int>& sel, std::function<void()> guide)
            : engine (e), clip (c), selection (sel), onShowGuide (std::move (guide))
        {
            for (int i = selection.size(); --i >= 0;)
                if (! juce::isPositiveAndBelow (selection[i], (int) clip.notes.size()))
                    selection.remove (i);

            const auto last = getLastSettings (engine);

            int id = 1;
            for (auto& grid : quantise::getGrids())
                gridBox.addItem (grid.name, id++);

            gridBox.setSelectedId (juce::jmax (0, quantise::findGrid (quantise::getGridKey (last.gridBeats))) + 1, juce::dontSendNotification);
            gridBox.setTooltip ("The grid the notes move to. Choose the smallest note value the part really plays: 1/16 for sixteenth notes, a triplet grid for triplets and swing.");
            gridBox.onChange = [this] { update(); };
            addAndMakeVisible (gridBox);

            gridLabel.setText ("Grid", juce::dontSendNotification);
            strengthLabel.setText ("Strength", juce::dontSendNotification);

            for (auto* l : { &gridLabel, &strengthLabel })
            {
                l->setColour (juce::Label::textColourId, theme::textDim);
                addAndMakeVisible (*l);
            }

            strength.setSliderStyle (juce::Slider::LinearHorizontal);
            strength.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
            strength.setRange (0.0, 100.0, 1.0);
            strength.setTextValueSuffix ("%");
            strength.setValue (std::round (last.strength * 100.0), juce::dontSendNotification);
            strength.setTooltip ("How far each note moves towards its grid line: 100% onto the line, 50% halfway, 0% not at all. Below 100% the part keeps some of its feel.");
            strength.onValueChange = [this] { update(); };
            addAndMakeVisible (strength);

            endsToggle.setButtonText ("Quantise ends too (lengths change)");
            endsToggle.setToggleState (last.ends, juce::dontSendNotification);
            endsToggle.setTooltip ("Also move each note's end towards the nearest grid line, so lengths change. Off: every note keeps its length.");
            endsToggle.onClick = [this] { update(); };
            addAndMakeVisible (endsToggle);

            picture.setTooltip ("Where each note goes: outlines are where it is now, filled notes where Quantise puts it. The lines are the chosen grid.");
            addAndMakeVisible (picture);

            summary.setMinimumHorizontalScale (0.9f);
            addAndMakeVisible (summary);
            scope.setFont (juce::FontOptions (15.0f, juce::Font::bold));
            scope.setMinimumHorizontalScale (0.7f);
            addAndMakeVisible (scope);

            guideButton.setTooltip ("Quantising: what grid and strength do, and when perfectly in time isn't the aim. Opens the guide to note lengths, beats and bars.");
            guideButton.onClick = [this] { if (onShowGuide) onShowGuide(); };
            addAndMakeVisible (guideButton);

            quantiseButton.setTooltip ("Move the notes as the picture shows. Cmd+Z undoes it in one step.");
            quantiseButton.addShortcut (juce::KeyPress (juce::KeyPress::returnKey));
            quantiseButton.onClick = [this] { apply(); };
            cancelButton.setTooltip ("Close without changing anything.");
            cancelButton.onClick = [this] { close(); };
            addAndMakeVisible (quantiseButton);
            addAndMakeVisible (cancelButton);

            const auto name = clip.name.isNotEmpty() ? clip.name : juce::String ("this clip");
            const int total = (int) clip.notes.size();
            scopeText = total == 0 ? "'" + name + "' has no notes"
                      : selection.isEmpty() ? "All " + juce::String (total) + (total == 1 ? " note" : " notes") + " of '" + name + "' (none are selected)"
                      : selection.size() == 1 ? "The selected note of '" + name + "'"
                      : "The " + juce::String (selection.size()) + " selected notes of '" + name + "'";
            scope.setText (scopeText, juce::dontSendNotification);

            setSize (560, 380);
            update();
        }

    private:
        quantise::Settings getSettings() const
        {
            quantise::Settings s;
            const auto& grids = quantise::getGrids();
            s.gridBeats = grids[(size_t) juce::jlimit (0, (int) grids.size() - 1, gridBox.getSelectedId() - 1)].beats;
            s.strength = strength.getValue() / 100.0;
            s.ends = endsToggle.getToggleState();
            return s;
        }

        void update()
        {
            const auto settings = getSettings();
            const auto after = quantise::apply (clip.notes, selection, clip.startBeat, clip.lengthBeats, engine.getBarLength(), settings);
            const auto result = quantise::summarise (clip.notes, after, selection, clip.lengthBeats);
            picture.setData (clip, after, selection, engine.getBarLength(), settings.gridBeats);
            summary.setText (quantise::describe (result, settings, engine.getTempoMap().getBpmAt (clip.startBeat)), juce::dontSendNotification);
            quantiseButton.setEnabled (result.moved > 0);
        }

        void apply()
        {
            const auto settings = getSettings();

            if (exists (engine, clip))
            {
                saveLastSettings (engine, settings);
                engine.quantiseNotes (clip, selection, settings);
            }

            close();
        }

        void close()
        {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);
            auto top = r.removeFromTop (24);
            guideButton.setBounds (top.removeFromRight (28));
            scope.setBounds (top);
            r.removeFromTop (8);

            auto row = r.removeFromTop (24);
            gridLabel.setBounds (row.removeFromLeft (70));
            gridBox.setBounds (row.removeFromLeft (260));
            r.removeFromTop (6);

            row = r.removeFromTop (24);
            strengthLabel.setBounds (row.removeFromLeft (70));
            strength.setBounds (row);
            r.removeFromTop (6);

            endsToggle.setBounds (r.removeFromTop (24));
            r.removeFromTop (8);

            auto buttons = r.removeFromBottom (28);
            quantiseButton.setBounds (buttons.removeFromRight (100));
            buttons.removeFromRight (8);
            cancelButton.setBounds (buttons.removeFromRight (90));
            r.removeFromBottom (8);
            summary.setBounds (r.removeFromBottom (36));
            r.removeFromBottom (6);
            picture.setBounds (r);
        }

        Engine& engine;
        MidiClip& clip;
        juce::SortedSet<int> selection;
        std::function<void()> onShowGuide;
        juce::String scopeText;
        juce::Label scope, gridLabel, strengthLabel, summary;
        juce::ComboBox gridBox;
        juce::Slider strength;
        juce::ToggleButton endsToggle;
        Picture picture;
        GuideButton guideButton { "?" };
        juce::TextButton quantiseButton { "Quantise" }, cancelButton { "Cancel" };
    };
}

quantise::Settings getLastSettings (Engine& engine)
{
    auto* settings = engine.properties.getUserSettings();
    quantise::Settings s;
    const auto& grids = quantise::getGrids();
    s.gridBeats = grids[(size_t) quantise::findGrid (settings->getValue ("quantise.grid", "1/16"))].beats;
    s.strength = juce::jlimit (0.0, 100.0, (double) settings->getIntValue ("quantise.strength", 100)) / 100.0;
    s.ends = settings->getBoolValue ("quantise.ends", false);
    return s;
}

void saveLastSettings (Engine& engine, const quantise::Settings& s)
{
    auto& settings = *engine.properties.getUserSettings();
    settings.setValue ("quantise.grid", quantise::getGridKey (s.gridBeats));
    settings.setValue ("quantise.strength", juce::roundToInt (s.strength * 100.0));
    settings.setValue ("quantise.ends", s.ends);
}

bool applyLast (Engine& engine, MidiClip& clip, const juce::SortedSet<int>& selection)
{
    return engine.quantiseNotes (clip, selection, getLastSettings (engine));
}

void showDialog (Engine& engine, MidiClip& clip, const juce::SortedSet<int>& selection, std::function<void()> onShowGuide)
{
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new Content (engine, clip, selection, std::move (onShowGuide)));
    options.dialogTitle = "Quantise notes";
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

juce::String getButtonTooltip()
{
    return "Quantise: move the selected notes (or the whole clip when none are selected) towards the nearest lines of a grid, "
           "to tighten a part played in by hand. Q quantises again with your last settings.";
}
} // namespace daw::quantiseui
