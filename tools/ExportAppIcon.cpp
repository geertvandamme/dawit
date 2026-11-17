#include "ui/AppIcon.h"

/** DawItExportIcon <folder>: writes the app icon's PNG set and app-icon.ico into the folder (see src/ui/AppIcon.h).
    A console tool with no NSApplication or window, so it runs over SSH, in CI and in sandboxes. It draws with the same
    code as the app, so the files can't differ from the icon the app shows. Exit code 0 on success, 1 on a failure. */
int main (int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: DawItExportIcon <folder>" << std::endl;
        return 1;
    }

    juce::ScopedJuceInitialiser_GUI juceInit;
    const juce::File folder (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[1])));

    if (! daw::appicon::exportAll (folder))
    {
        std::cerr << "Could not write the icon files into " << folder.getFullPathName() << std::endl;
        return 1;
    }

    std::cout << "Wrote the app icon files into " << folder.getFullPathName() << std::endl;
    return 0;
}
