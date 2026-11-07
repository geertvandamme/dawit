#pragma once

#include "engine/Engine.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The Notebook window: free text at three levels, as tabs. *App* is your own notebook, the same in every song (kept in
    `Notebook.txt` on this computer, saved a second after you stop typing, when the window closes and when the app
    quits); *Project* holds notes about this song (saved in the project); *Track* holds notes about the selected track
    (saved with the track). Project and track notes are written to the engine on every keystroke, so Save and the
    unsaved-changes check always see them.

    The Track tab follows the selected track. The text box is reloaded only when what it shows changes (tab, track,
    a new edit) or the engine's text differs from it, never in reply to its own write, so the caret stays put. The last tab
    is remembered in `view.notebookTab`.
*/
class NotebookWindow final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    enum class Tab { app, project, track };

    NotebookWindow (Engine&, Selection&);
    ~NotebookWindow() override;

    /** Switches to a tab (the Track tab shows the selected track). */
    void showTab (Tab);

    Tab getTab() const noexcept { return tab; }

    /** Writes the App notebook now if it has unsaved typing. */
    void flush();

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    static constexpr int defaultWidth = 560, defaultHeight = 480, minWidth = 420, minHeight = 320;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    /** Follows the engine and the selection; reloads the text box only when what it shows changed. */
    void refresh (bool force);
    juce::String getEngineText() const;
    juce::String getDescription() const;
    juce::String getPlaceholder() const;
    void typed();

    Engine& engine;
    Selection& selection;

    Tab tab = Tab::project;
    Track* shownTrack = nullptr;   // the Track tab's track; never kept once the edit no longer holds it
    int shownGeneration = -1;
    bool appDirty = false;
    juce::String appText, saveError;

    juce::TextButton appButton { "App" }, projectButton { "Project" }, trackButton { "Track" };
    juce::TextEditor editor;
    juce::Rectangle<int> descriptionArea;
};
}
