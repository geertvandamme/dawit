#pragma once

#include "engine/Engine.h"
#include "EffectWindow.h"
#include "GuidePrint.h"
#include "generate/TheoryGuides.h"
#include "PluginWindows.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    Opens the guide of a built-in effect or synth from the Theory Guides window. It opens on the selected track's
    one when that track has it, otherwise on the first one in the song, and otherwise on a stand-in on no track,
    which plays straight to the output, so its clap or test note is still heard and its knobs can be tried.
*/
class BuiltInGuides final
{
public:
    BuiltInGuides (Engine&, Selection&, PluginWindowManager&);
    ~BuiltInGuides();

    /** Opens the plugin's window and its "What is ...?" guide at a page; -1 opens the guide where it is. */
    void open (const juce::String& identifier, int topic);

    /** Which one it would open: "Opens the one on Bass" or "Opens one on no track, to try out". */
    juce::String describeTarget (const juce::String& identifier) const;

    /** The pages of a built-in effect's or synth's "What is ...?" guide, read from a guide made for a new one. */
    static juce::StringArray getPageNames (const juce::String& identifier);

    /** The pages of the guide of a built-in effect or synth, made for this one (empty for any other processor). */
    static std::vector<fxui::Guide::Page> makePages (juce::AudioProcessor&);

    /** The effect's or synth's guide for an export: the one on the song's track (preferring the selected track) with
        its settings, or on a fresh one with its starting settings when the song has none. Made for the export, and
        deleted with the item. */
    guideprint::Item getPrintGuide (const juce::String& identifier);

    /** gen::guides::getEntries(), with the pages of the guides built on a plugin filled in. */
    static std::vector<gen::guides::Entry> getEntries();

private:
    class StandIn;

    /** The one in the song, preferring the selected track; nullptr when the song has none. */
    juce::AudioPluginInstance* find (const juce::String& identifier, const Track** onTrack = nullptr) const;

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;
    std::vector<std::unique_ptr<StandIn>> standIns;
};

//==============================================================================
/**
    The Theory Guides window (View > Theory Guides..., Cmd+G): every guide in DawIt in one place, grouped by area, with
    a short path for beginners and a search. Each guide is a card with what it covers, where else it opens from and
    its topics; clicking a topic opens the guide there, and clicking the card opens it at its start. The guides
    still open from where their theory applies, as each card says.
*/
class TheoryGuidesView final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit TheoryGuidesView (std::vector<gen::guides::Entry>);
    ~TheoryGuidesView() override;

    /** Opens a guide at a topic (an index into its entry's topics), or -1 at its start. */
    std::function<void (gen::guides::Guide, int topic)> onOpen;

    /** A line on what a guide built on a plugin opens on ("Opens the one on Bass"); empty for the others. */
    std::function<juce::String (const gen::guides::Entry&)> describeTarget;

    /** The colour of an area, on its cards and in the list. */
    static juce::Colour getColour (gen::guides::Area);

    /** Shows a row of the list: the path for beginners, an area, or every guide. */
    void showRow (int row);

    /** Searches the guides, as typing in the search field does. */
    void search (const juce::String& query);

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    class Card;

    // The list: the path for beginners, the areas, then every guide.
    static constexpr int startRow = 0, firstAreaRow = 1;
    static int getAllRow() noexcept { return firstAreaRow + (int) std::size (gen::guides::allAreas); }

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;

    void refresh();
    void layoutCards();
    void exportShown();
    int countGuides (int row) const;

    std::vector<gen::guides::Entry> entries;
    int shownRow = startRow;
    juce::String heading, subheading;

    std::vector<gen::guides::Guide> shownGuides;   // the guides on show, in order
    juce::String shownName;                        // "Start here", an area's name, "All guides" or the search
    juce::Label title, intro;
    juce::TextButton exportButton { "Export PDF..." };
    juce::TextEditor searchBox;
    juce::ListBox list;
    juce::Viewport viewport;
    juce::Component cardHolder;
    juce::OwnedArray<Card> cards;
    juce::Rectangle<int> headingArea;

    static constexpr int width = 1040, height = 760;
};
} // namespace daw
