#include "engine/Engine.h"
#include "ui/DevScreenshots.h"
#include "ui/MainComponent.h"

class DawItApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    // Screenshot runs (a developer flag) may run next to a normal instance.
    bool moreThanOneInstanceAllowed() override { return getCommandLineParameters().contains ("--screenshot") || getCommandLineParameters().contains ("--export-guides"); }

    void initialise (const juce::String& commandLine) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
        // --screenshots runs on settings of its own, so it starts the same every time and leaves the user's alone.
        engine = std::make_unique<daw::Engine> (getCommandLineParameterArray().contains ("--screenshots"));
        mainWindow = std::make_unique<MainWindow> (getApplicationName(), *engine);

        // Developer flags: --demo fills the edit with example tracks,
        // --screenshot <file.png> renders the window to an image and quits, --export-guides <folder> saves the guides as PDFs,
        // --screenshots <shots.json> takes the screenshots in a list (see ScreenshotRunner).
        const auto args = getCommandLineParameterArray();
        auto* content = dynamic_cast<daw::MainComponent*> (mainWindow->getContentComponent());

        if (args.contains ("--demo") && content != nullptr)
            content->createDemoContent();

        openProjectsIn (commandLine);

        // --export-guides <folder>: every guide as a PDF in the folder, then Start here; exit code 1 if anything failed.
        if (const int i = args.indexOf ("--export-guides"); i >= 0 && i + 1 < args.size() && content != nullptr)
        {
            const juce::File folder (juce::File::getCurrentWorkingDirectory().getChildFile (args[i + 1]));

            juce::Timer::callAfterDelay (1500, [content, folder]
            {
                content->exportAllGuides (folder, [] (bool ok)
                {
                    juce::JUCEApplication::getInstance()->setApplicationReturnValue (ok ? 0 : 1);
                    quit();
                });
            });
        }

        // --screenshots <shots.json>: every shot in the list; exit code 1 if any failed.
        if (const int i = args.indexOf ("--screenshots"); i >= 0 && i + 1 < args.size() && content != nullptr)
        {
            const juce::File list (juce::File::getCurrentWorkingDirectory().getChildFile (args[i + 1]));

            daw::ScreenshotRunner::start (*content, list, [] (bool ok)
            {
                juce::JUCEApplication::getInstance()->setApplicationReturnValue (ok ? 0 : 1);
                quit(); // no unsaved-changes prompt
            });
        }

        if (const int i = args.indexOf ("--screenshot"); i >= 0 && i + 1 < args.size() && content != nullptr)
        {
            const juce::File target (juce::File::getCurrentWorkingDirectory().getChildFile (args[i + 1]));

            juce::Timer::callAfterDelay (1500, [content, target]
            {
                const auto image = content->createComponentSnapshot (content->getLocalBounds(), true, 1.0f);
                target.deleteFile();

                if (auto stream = target.createOutputStream())
                    juce::PNGImageFormat().writeImageToStream (image, *stream);

                quit(); // no unsaved-changes prompt
            });
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        engine = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override
    {
        // Offer to save unsaved changes first; quitting waits for the answer.
        if (auto* content = mainWindow != nullptr ? dynamic_cast<daw::MainComponent*> (mainWindow->getContentComponent()) : nullptr)
            content->askToClose ([] { quit(); });
        else
            quit();
    }

    // macOS also uses this to open a project double-clicked in the Finder.
    void anotherInstanceStarted (const juce::String& commandLine) override { openProjectsIn (commandLine); }

private:
    /** Opens the first project file named on the command line, if any. */
    void openProjectsIn (const juce::String& commandLine)
    {
        auto* content = mainWindow != nullptr ? dynamic_cast<daw::MainComponent*> (mainWindow->getContentComponent()) : nullptr;

        for (auto& arg : juce::StringArray::fromTokens (commandLine, true))
        {
            const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (arg.unquoted());

            if (content != nullptr && daw::project::isProjectFile (file) && file.existsAsFile())
            {
                content->openProject (file);
                return;
            }
        }
    }

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, daw::Engine& dawEngine)
            : juce::DocumentWindow (name, daw::theme::background, juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new daw::MainComponent (dawEngine), true);
            setResizable (true, true);
            setResizeLimits (960, 600, 10000, 10000);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);

            if (auto* content = getContentComponent())
                content->grabKeyboardFocus();
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    daw::DawLookAndFeel lookAndFeel;
    std::unique_ptr<daw::Engine> engine;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (DawItApplication)
