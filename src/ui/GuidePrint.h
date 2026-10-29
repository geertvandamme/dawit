#pragma once

#include "GuideSource.h"
#include "engine/Engine.h"
#include "generate/TheoryGuides.h"

#include <optional>

/**
    "Export PDF..." for the guides: every guide window can be saved as a booklet (a cover with the contents, then
    a page per topic with its picture and text as the window shows them), and the Theory Guides window and the File
    menu save a set of guides as one. This is what each guide window gives for it (`Source`), the shared button,
    the dialog and the export job, which photographs each guide's topics one by one while the window turns
    through them.
*/
namespace daw::guideprint
{
/** A guide to export: its window's content, or one made for the export and deleted after it. */
struct Item
{
    gen::guides::Guide guide = gen::guides::Guide::sound;
    std::unique_ptr<juce::AudioPluginInstance> plugin;         // a fresh built-in for an effect's guide; outlives `owned`
    std::unique_ptr<juce::Component> owned;                    // the guide made for the export
    juce::Component::SafePointer<juce::Component> component;   // the open window's content or `owned`: also the Source
    Source* source = nullptr;
    juce::String sourceText;                                   // overrides the Source's own line when set

    /** Uses an open guide window's content. */
    template <typename T>
    void useOpen (T* window)
    {
        component = window;
        source = window;
    }

    /** Takes a guide made for the export, deleted with the item. */
    template <typename T>
    void adopt (std::unique_ptr<T> made)
    {
        useOpen (made.get());
        owned = std::move (made);
    }
};

/** What the export needs from the app, set by MainComponent while it exists. */
struct App
{
    Engine* engine = nullptr;
    std::function<juce::String()> getProjectName;
    std::function<Item (gen::guides::Guide)> getGuide;          // the open guide, or one made for the export
};

void setApp (App);

/** The guide that explains a built-in effect or synth ("DawIt Reverb"), if there is one. */
std::optional<gen::guides::Guide> findGuideForPlugin (const juce::String& identifier);

/** A component drawn into an image `scale` times its size, whether or not it is on screen. */
juce::Image snapshot (juce::Component&, float scale);

/** Cmd+P. */
bool isExportKey (const juce::KeyPress&);

/** "Export PDF...": opens the dialog for its Source. In its window it also handles Cmd+P. */
class ExportButton final : public juce::TextButton
{
public:
    ExportButton (Source&, gen::guides::Guide, const juce::String& tooltip = {});
    ~ExportButton() override;

    void parentHierarchyChanged() override;

private:
    struct Keys final : juce::KeyListener
    {
        explicit Keys (ExportButton& b) : button (b) {}
        bool keyPressed (const juce::KeyPress&, juce::Component*) override;
        ExportButton& button;
    };

    Source& source;
    gen::guides::Guide guide;
    Keys keys { *this };
    juce::Component::SafePointer<juce::Component> listeningTo;
};

/** The dialog for one guide, then the save dialog and the export. */
void exportGuide (Source&, gen::guides::Guide);

/** The dialog for a set of guides: starts on `shown` (named `shownName`, such as "Start here" or "Sound", or a
    search's results), with the path, every area and all guides to choose instead. */
void exportGuides (std::vector<gen::guides::Guide> shown, const juce::String& shownName);

/** What an export made, for the developer flag's checks. */
struct Report
{
    int pages = 0;
    std::vector<juce::StringArray> topicNames;   // per guide: the topics the booklet has, in order
    bool topicsOnOwnPages = true;                // every topic starts a page of its own, in order
};

/** Exports the items to a file with no dialogs and calls back with the result (the developer flag uses it). */
void exportToFile (std::vector<Item>, const juce::String& title, const juce::String& summary,
                   const booklet::Options&, const juce::File&, std::function<void (juce::Result, const Report&)> done);

/** The text the covers say about a set: "Seven guides in an order that builds up from sound to the mixer." */
juce::String describeSet (const juce::String& name, const std::vector<gen::guides::Guide>&);
} // namespace daw::guideprint
