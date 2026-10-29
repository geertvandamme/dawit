#include "MidiExportActions.h"
#include "engine/MidiExport.h"

namespace daw::midiactions
{
namespace
{
    std::unique_ptr<juce::FileChooser> chooser;

    midiexport::SongInfo getSongInfo (const Engine& engine)
    {
        return { engine.getTempoMap(), engine.getKey() };
    }

    juce::String clipFileName (const Track& track, const MidiClip& clip)
    {
        return juce::File::createLegalFileName (track.name + (clip.name.isNotEmpty() ? " - " + clip.name : juce::String())) + ".mid";
    }

    /** The file is built now, so later edits (or deleting the clip) while the dialog is open don't matter. */
    void saveWithChooser (Engine& engine, juce::MidiFile midi, const juce::String& fileName)
    {
        auto& settings = *engine.properties.getUserSettings();
        const juce::File folder (settings.getValue ("export.midiFolder",
                                                    juce::File::getSpecialLocation (juce::File::userMusicDirectory).getFullPathName()));

        chooser = std::make_unique<juce::FileChooser> ("Export as MIDI file", folder.getChildFile (fileName), "*.mid");
        const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

        chooser->launchAsync (flags, [&settings, midi] (const juce::FileChooser& fc)
        {
            auto target = fc.getResult();

            if (target == juce::File())
                return;

            target = target.withFileExtension (".mid");
            settings.setValue ("export.midiFolder", target.getParentDirectory().getFullPathName());
            settings.saveIfNeeded();

            if (juce::String error; ! midiexport::write (midi, target, error))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", error);
        });
    }
}

void exportClip (Engine& engine, const Track& track, const MidiClip& clip)
{
    saveWithChooser (engine, midiexport::createClipFile (clip, track.name, getSongInfo (engine)), clipFileName (track, clip));
}

void exportTrack (Engine& engine, const Track& track)
{
    saveWithChooser (engine, midiexport::createTrackFile (track, getSongInfo (engine)),
                     juce::File::createLegalFileName (track.name) + ".mid");
}

void dragClipOut (Engine& engine, const Track& track, const MidiClip& clip, juce::Component& source)
{
    // A fresh folder per drag, as the receiving app may still be reading the previous file.
    // Folders from earlier drags are cleaned up once they are an hour old.
    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DawIt MIDI");

    for (auto& old : root.findChildFiles (juce::File::findDirectories, false))
        if (old.getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::hours (1))
            old.deleteRecursively();

    const auto folder = root.getNonexistentChildFile ("drag", {}, false);
    const auto file = folder.getChildFile (clipFileName (track, clip));
    juce::String error;

    if (! midiexport::write (midiexport::createClipFile (clip, track.name, getSongInfo (engine)), file, error))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", error);
        return;
    }

    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, &source);
}
} // namespace daw::midiactions
