#pragma once

#include "engine/ProjectFile.h"

namespace daw
{
/**
    The open project file: New, Open, Save and Save As, the recent projects list, and asking to save
    unsaved changes before the project is replaced or the app quits. Message thread only.
*/
class ProjectManager final : private juce::Timer
{
public:
    explicit ProjectManager (Engine&);
    ~ProjectManager() override;

    /** Called before the edit is replaced, to forget pointers to tracks and clips. */
    std::function<void()> beforeReplace;

    /** Called after a project was opened or a new one started. */
    std::function<void()> afterReplace;

    /** Called with the window title whenever it changes: the project name, and whether it was edited. */
    std::function<void (const juce::String&)> onTitleChanged;

    void newProject();

    /** Starts an untitled project made by `make`. It offers to save first unless the caller knows the current edit is
        the unchanged result it last made. The empty project is the saved baseline, so a successful result is edited. */
    void newProjectFrom (std::function<bool()> make, bool replaceWithoutAsking, std::function<void (bool made)> done = {});
    void open();
    void openFile (const juce::File&);
    void save (std::function<void (bool saved)> done = {});
    void saveAs (std::function<void (bool saved)> done = {});

    /** Offers to save unsaved changes, then calls `proceed` unless the user cancelled. */
    void askToSaveChanges (std::function<void()> proceed);

    juce::RecentlyOpenedFilesList& getRecentFiles() noexcept { return recentFiles; }
    juce::String getTitle() const;

    /** The project's file name without its extension, or "Untitled". */
    juce::String getProjectName() const;

private:
    void timerCallback() override;
    bool hasUnsavedChanges() const;
    void markSaved (const juce::File&, const juce::String& text);
    void replaceEdit (const std::function<void()>& change);
    void loadFile (const juce::File&);
    void writeTo (const juce::File&, const std::function<void (bool)>& done);
    void rememberRecentFiles();
    juce::File getDefaultFolder() const;

    Engine& engine;
    juce::File file;
    juce::String savedText, savedTextWithoutPlugins;
    bool edited = false;
    juce::String lastTitle;
    juce::RecentlyOpenedFilesList recentFiles;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace daw
