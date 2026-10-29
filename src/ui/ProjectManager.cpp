#include "ProjectManager.h"

namespace daw
{
namespace
{
    constexpr const char* recentFilesKey = "recentProjects";
    constexpr const char* projectFolderKey = "project.folder";
}

ProjectManager::ProjectManager (Engine& e) : engine (e)
{
    recentFiles.setMaxNumberOfItems (10);
    recentFiles.restoreFromString (engine.properties.getUserSettings()->getValue (recentFilesKey));
    markSaved ({}, project::toText (engine, {}));

    // Checking for edits without plugin state is quick; the full check runs before anything is lost.
    startTimer (1000);
}

ProjectManager::~ProjectManager() = default;

juce::String ProjectManager::getProjectName() const
{
    return file == juce::File() ? juce::String ("Untitled") : file.getFileNameWithoutExtension();
}

juce::String ProjectManager::getTitle() const
{
    return getProjectName() + (edited ? " (edited)" : "") + " - DawIt";
}

void ProjectManager::timerCallback()
{
    edited = project::toText (engine, file, false) != savedTextWithoutPlugins;

    if (const auto title = getTitle(); title != lastTitle)
    {
        lastTitle = title;

        if (onTitleChanged)
            onTitleChanged (title);
    }
}

bool ProjectManager::hasUnsavedChanges() const
{
    return project::toText (engine, file) != savedText;
}

void ProjectManager::markSaved (const juce::File& newFile, const juce::String& text)
{
    file = newFile;
    engine.setRecordingFolder (file.existsAsFile() ? file.getSiblingFile (project::audioFolderName) : juce::File());   // takes go next to the project
    savedText = text;
    savedTextWithoutPlugins = project::toText (engine, file, false);
    timerCallback();
}

//==============================================================================
void ProjectManager::askToSaveChanges (std::function<void()> proceed)
{
    if (! hasUnsavedChanges())
    {
        proceed();
        return;
    }

    const auto name = file == juce::File() ? juce::String ("Untitled") : file.getFileNameWithoutExtension();
    const auto options = juce::MessageBoxOptions()
                             .withIconType (juce::MessageBoxIconType::QuestionIcon)
                             .withTitle ("Save changes to \"" + name + "\"?")
                             .withMessage ("Your changes will be lost if you don't save them.")
                             .withButton ("Save")
                             .withButton ("Don't Save")
                             .withButton ("Cancel");

    // Results: 1 = Save, 2 = Don't Save, 0 = Cancel.
    juce::AlertWindow::showAsync (options, [this, proceed] (int result)
    {
        if (result == 1)
            save ([proceed] (bool saved) { if (saved) proceed(); });
        else if (result == 2)
            proceed();
    });
}

void ProjectManager::replaceEdit (const std::function<void()>& change)
{
    if (beforeReplace)
        beforeReplace();

    change();

    if (afterReplace)
        afterReplace();
}

void ProjectManager::newProject()
{
    askToSaveChanges ([this]
    {
        replaceEdit ([this] { engine.resetEdit(); });
        markSaved ({}, project::toText (engine, {}));
    });
}

void ProjectManager::newProjectFrom (std::function<bool()> maker, bool replaceWithoutAsking, std::function<void (bool)> completion)
{
    auto create = [this, makeProject = std::move (maker), finish = std::move (completion)]
    {
        bool made = false;
        replaceEdit ([&]
        {
            engine.resetEdit();
            markSaved ({}, project::toText (engine, {}));
            made = makeProject();
        });
        timerCallback();

        if (finish)
            finish (made);
    };

    if (replaceWithoutAsking)
        create();
    else
        askToSaveChanges (std::move (create));
}

void ProjectManager::open()
{
    askToSaveChanges ([this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Open a project", getDefaultFolder(), project::fileWildcards);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
        {
            if (fc.getResult() != juce::File())
                loadFile (fc.getResult());
        });
    });
}

void ProjectManager::openFile (const juce::File& projectFile)
{
    askToSaveChanges ([this, projectFile] { loadFile (projectFile); });
}

void ProjectManager::loadFile (const juce::File& projectFile)
{
    if (! projectFile.existsAsFile())
    {
        recentFiles.removeFile (projectFile);
        rememberRecentFiles();
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't open the project",
                                                projectFile.getFullPathName() + " doesn't exist.");
        return;
    }

    juce::String error;
    juce::StringArray warnings;
    bool loaded = false;

    replaceEdit ([&] { loaded = project::load (engine, projectFile, error, warnings); });

    if (! loaded)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't open the project", error);
        return;
    }

    // What actually loaded counts as saved, so a missing plugin doesn't mark the project as edited.
    markSaved (projectFile, project::toText (engine, projectFile));
    recentFiles.addFile (projectFile);
    rememberRecentFiles();

    if (! warnings.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Some parts couldn't be loaded",
                                                "- " + warnings.joinIntoString ("\n- "));
}

//==============================================================================
void ProjectManager::save (std::function<void (bool)> done)
{
    if (file == juce::File())
        saveAs (std::move (done));
    else
        writeTo (file, done);
}

void ProjectManager::saveAs (std::function<void (bool)> done)
{
    const auto suggested = file != juce::File() ? file : getDefaultFolder().getChildFile (juce::String ("Untitled") + project::fileExtension);
    chooser = std::make_unique<juce::FileChooser> ("Save the project", suggested, juce::String ("*") + project::fileExtension);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, done] (const juce::FileChooser& fc)
    {
        if (fc.getResult() == juce::File())
        {
            if (done)
                done (false);

            return;
        }

        writeTo (fc.getResult().withFileExtension (project::fileExtension), done);
    });
}

void ProjectManager::writeTo (const juce::File& target, const std::function<void (bool)>& done)
{
    juce::String text, error;
    const bool saved = project::save (engine, target, text, error);

    if (saved)
    {
        markSaved (target, text);
        recentFiles.addFile (target);
        rememberRecentFiles();
        engine.properties.getUserSettings()->setValue (projectFolderKey, target.getParentDirectory().getFullPathName());
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't save the project", error);
    }

    if (done)
        done (saved);
}

void ProjectManager::rememberRecentFiles()
{
    auto& settings = *engine.properties.getUserSettings();
    settings.setValue (recentFilesKey, recentFiles.toString());
    settings.saveIfNeeded();
}

juce::File ProjectManager::getDefaultFolder() const
{
    return engine.properties.getUserSettings()->getValue (projectFolderKey,
                                                          juce::File::getSpecialLocation (juce::File::userMusicDirectory).getFullPathName());
}
} // namespace daw
