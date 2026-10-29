#include "ModelDownload.h"

#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>

namespace daw::models
{
juce::File getFolder()
{
    // Next to the settings file, like the Engine's ApplicationProperties.
    juce::PropertiesFile::Options options;
    options.applicationName = "DawIt";
    options.folderName = "DawIt";
    options.osxLibrarySubFolder = "Application Support";
    options.filenameSuffix = "settings";

    return options.getDefaultFile().getSiblingFile ("Models");
}

juce::File getFile (const Model& model)
{
    return getFolder().getChildFile (model.fileName);
}

bool isInstalled (const Model& model)
{
    const auto file = getFile (model);
    return file.existsAsFile() && file.getSize() == model.size;
}

juce::String formatSize (juce::int64 bytes)
{
    if (bytes >= 1000000000)
        return juce::String ((double) bytes / 1.0e9, 1) + " GB";

    return juce::String (juce::roundToInt ((double) bytes / 1.0e6)) + " MB";
}

juce::String download (const Model& model, const ProgressFn& progress)
{
    const auto target = getFile (model);
    const auto partial = target.getSiblingFile (target.getFileName() + ".part");

    if (! target.getParentDirectory().createDirectory())
        return "Couldn't create the folder " + target.getParentDirectory().getFullPathName() + ".";

    int status = 0;
    auto stream = juce::URL (model.url).createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                                               .withConnectionTimeoutMs (20000)
                                                               .withNumRedirectsToFollow (10)
                                                               .withStatusCode (&status));

    if (stream == nullptr || status >= 400)
        return "Couldn't download the model from Hugging Face" + (status >= 400 ? " (HTTP " + juce::String (status) + ")" : juce::String())
             + ". Check the internet connection and try again.";

    {
        partial.deleteFile();
        juce::FileOutputStream out (partial);

        if (out.failedToOpen())
            return "Couldn't write " + partial.getFullPathName() + ".";

        juce::HeapBlock<char> chunk (1 << 16);
        juce::int64 received = 0;

        while (! stream->isExhausted())
        {
            const int numRead = stream->read (chunk.get(), 1 << 16);

            if (numRead <= 0)
                break;

            if (! out.write (chunk.get(), (size_t) numRead))
                return "Couldn't write " + partial.getFullPathName() + ". Is the disk full?";

            received += numRead;

            if (! progress ((double) received / (double) model.size))
            {
                out.flush();
                partial.deleteFile();
                return {};
            }
        }
    }

    if (partial.getSize() != model.size || juce::SHA256 (partial).toHexString() != model.sha256)
    {
        partial.deleteFile();
        return "The downloaded model is incomplete or damaged. Try again.";
    }

    if (! partial.moveFileTo (target))
        return "Couldn't save the model to " + target.getFullPathName() + ".";

    return {};
}
} // namespace daw::models
