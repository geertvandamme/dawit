#include "VoiceInstall.h"

namespace daw::sing
{
juce::File getVocodersFolder (const juce::File& voicesFolder) { return voicesFolder.getChildFile ("Vocoders"); }

bool removeVoice (const juce::File& voiceFolder)
{
    return voiceFolder.isDirectory() && voiceFolder.moveToTrash();
}

namespace
{
    /** A name that isn't taken in the folder: "Name", "Name 2"... */
    juce::File uniqueChild (const juce::File& folder, const juce::String& name)
    {
        auto f = folder.getChildFile (name);

        for (int i = 2; f.exists(); ++i)
            f = folder.getChildFile (name + " " + juce::String (i));

        return f;
    }

    /** Vocoder folders inside a folder: those with a vocoder.yaml. */
    juce::Array<juce::File> findVocoderFolders (const juce::File& folder)
    {
        juce::Array<juce::File> found;

        for (auto& f : folder.findChildFiles (juce::File::findFiles, true, "vocoder.yaml"))
            found.add (f.getParentDirectory());

        return found;
    }

    InstallResult failure (const juce::String& message)
    {
        InstallResult r;
        r.error = message;
        return r;
    }

    /** Unpacks a zip into `target`, refusing any entry that would land outside it. */
    juce::String unzip (const juce::File& zip, const juce::File& target, const InstallProgressFn& progress, bool& cancelled)
    {
        juce::ZipFile file (zip);

        if (file.getNumEntries() == 0)
            return zip.getFileName() + " is empty or isn't a zip file.";

        target.createDirectory();

        for (int i = 0; i < file.getNumEntries(); ++i)
        {
            const auto* entry = file.getEntry (i);
            const auto name = entry->filename;

            if (name.contains ("..") || name.startsWithChar ('/') || name.contains (":") || name.startsWith ("__MACOSX"))
            {
                if (name.startsWith ("__MACOSX"))
                    continue;

                return zip.getFileName() + " has a file with an unsafe path (" + name + "), so it was not unpacked.";
            }

            if (progress && ! progress ((double) i / (double) file.getNumEntries()))
            {
                cancelled = true;
                return {};
            }

            const auto result = file.uncompressEntry (i, target);

            if (result.failed())
                return "Unpacking " + name + " failed: " + result.getErrorMessage();
        }

        return {};
    }
}

InstallResult installVoice (const juce::File& source, const juce::File& voicesFolder, const InstallProgressFn& progress)
{
    if (! source.exists())
        return failure(source.getFileName() + " isn't there.");

    voicesFolder.createDirectory();
    const auto vocoders = getVocodersFolder (voicesFolder);

    // Where the contents are looked at: the folder itself, or the zip unpacked to a work folder.
    juce::File workFolder, content;
    bool unpacked = false;

    if (source.isDirectory())
    {
        content = source;
    }
    else if (source.hasFileExtension ("zip;oudep"))
    {
        workFolder = uniqueChild (voicesFolder, ".unpacking");
        bool cancelled = false;
        const auto error = unzip (source, workFolder, progress, cancelled);

        if (cancelled || error.isNotEmpty())
        {
            workFolder.deleteRecursively();
            InstallResult r;
            r.error = error;
            r.cancelled = cancelled;
            return r;
        }

        content = workFolder;
        unpacked = true;
    }
    else
        return failure (source.getFileName() + " isn't a .zip, a .oudep or a folder.");

    auto cleanUp = [&] { if (unpacked) workFolder.deleteRecursively(); };

    // A voice has a dsconfig.yaml; a vocoder pack only a vocoder.yaml.
    const auto voiceRoot = findVoiceRoot (content);

    if (voiceRoot == juce::File())
    {
        const auto vocoderFolders = findVocoderFolders (content);

        if (vocoderFolders.isEmpty())
        {
            cleanUp();
            return failure (source.getFileName() + " isn't a DiffSinger voice or vocoder: there is no dsconfig.yaml or vocoder.yaml in it. "
                            "A voice folder holds dsconfig.yaml, the acoustic model and a phonemes file.");
        }

        InstallResult r;
        r.wasVocoder = true;

        for (auto& v : vocoderFolders)
        {
            const auto dest = uniqueChild (vocoders, v.getFileName());
            vocoders.createDirectory();
            if (! v.copyDirectoryTo (dest))
            {
                cleanUp();
                return failure ("Copying the vocoder " + v.getFileName() + " into the Vocoders folder failed.");
            }

            r.installed = dest;
        }

        cleanUp();
        return r;
    }

    // The voice goes into its own folder named after the voice's folder (or the zip).
    const auto stem = source.isDirectory() ? source.getFileName() : source.getFileNameWithoutExtension();
    const auto dest = uniqueChild (voicesFolder, stem);
    bool copied = false;

    if (unpacked)
    {
        // Move the whole unpacked tree, so a voice packed in folders keeps its layout.
        const auto top = content.findChildFiles (juce::File::findDirectories, false);
        const bool single = top.size() == 1 && content.findChildFiles (juce::File::findFiles, false).isEmpty();
        copied = (single ? top.getFirst() : content).moveFileTo (dest);
    }
    else
        copied = content.copyDirectoryTo (dest);

    cleanUp();

    if (! copied)
    {
        dest.deleteRecursively();
        return failure ("Copying " + source.getFileName() + " into the Voices folder failed. Is there room on the disk?");
    }

    auto check = loadVoice (dest, vocoders);
    InstallResult r;
    r.installed = dest;

    if (check.voice.has_value())
        return r;

    if (check.vocoderMissing)
    {
        r.notice = check.error;   // keep the voice: the vocoder can be added after
        return r;
    }

    dest.deleteRecursively();
    return failure (check.error);
}
} // namespace daw::sing
