#include "AboutInfo.h"
#include "DawItNotices.h"

#include <algorithm>

namespace daw::about
{
    namespace
    {
        struct Entry
        {
            const char* fileName;
            const char* name;
            Group group;
        };

        const char* const repoUrl = "https://github.com/geertvandamme/dawit";

        const char* const ollamaText =
            "Ollama runs on your own computer as a separate program. DawIt sends it requests over a local connection. "
            "Ollama and each language model you install have their own licences; DawIt does not include or distribute them.";

        const Entry entries[] =
        {
            { "LICENSE", "DawIt's licence and component list", Group::dawit },
            { "README.md", "Where these texts come from", Group::dawit },

            { "Arm-optimized-routines-LICENSE.txt", "Arm optimized routines", Group::builtIn },
            { "AudioUnitSDK-LICENSE.txt", "Apple Audio Unit SDK", Group::builtIn },
            { "Basic-Pitch-LICENSE.txt", "Basic Pitch: licence", Group::builtIn },
            { "Basic-Pitch-NOTICE.txt", "Basic Pitch: notice", Group::builtIn },
            { "Bravura-LICENSE.txt", "Bravura music font", Group::builtIn },
            { "CMUdict-LICENSE.txt", "CMU Pronouncing Dictionary", Group::builtIn },
            { "COPYING.APACHE", "Eigen: COPYING.APACHE", Group::builtIn },
            { "COPYING.BSD", "Eigen: COPYING.BSD", Group::builtIn },
            { "COPYING.MINPACK", "Eigen: COPYING.MINPACK", Group::builtIn },
            { "COPYING.MPL2", "Eigen: COPYING.MPL2", Group::builtIn },
            { "COPYING.README", "Eigen: COPYING.README", Group::builtIn },
            { "GPL-3.0.txt", "GNU GPL v3 (the VST3 SDK's licence as DawIt uses it)", Group::builtIn },
            { "JUCE-FLAC-LICENSE.txt", "JUCE: FLAC", Group::builtIn },
            { "JUCE-HarfBuzz-COPYING.txt", "JUCE: HarfBuzz", Group::builtIn },
            { "JUCE-HarfBuzz-base-table-LICENSE.txt", "JUCE: HarfBuzz (OpenType layout tables)", Group::builtIn },
            { "JUCE-HarfBuzz-hb-ucd-LICENSE.txt", "JUCE: HarfBuzz (Unicode data)", Group::builtIn },
            { "JUCE-JPEG-README.txt", "JUCE: JPEG", Group::builtIn },
            { "JUCE-LICENSE.md", "JUCE", Group::builtIn },
            { "JUCE-Ogg-Vorbis-LICENSE.txt", "JUCE: Ogg Vorbis", Group::builtIn },
            { "JUCE-SheenBidi-LICENSE.txt", "JUCE: SheenBidi", Group::builtIn },
            { "JUCE-libpng-LICENSE.txt", "JUCE: libpng", Group::builtIn },
            { "JUCE-libvorbis-COPYING.txt", "JUCE: libvorbis", Group::builtIn },
            { "JUCE-zlib-LICENSE.txt", "JUCE: zlib", Group::builtIn },
            { "ONNX-Runtime-LICENSE.txt", "ONNX Runtime", Group::builtIn },
            { "ONNX-Runtime-ThirdPartyNotices.txt", "ONNX Runtime: third-party notices", Group::builtIn },
            { "ONNX-nlohmann-json-3.12.0-LICENSE.txt", "ONNX Runtime: nlohmann/json", Group::builtIn },
            { "ONNX-zlib-1.3.2-LICENSE.txt", "ONNX Runtime: zlib", Group::builtIn },
            { "TinySoundFont-LICENSE.txt", "TinySoundFont", Group::builtIn },
            { "Unicode-16-emoji-data-NOTICE.txt", "Unicode emoji data: notice", Group::builtIn },
            { "Unicode-3.0-LICENSE.txt", "Unicode licence, version 3.0", Group::builtIn },
            { "VST3-SDK-LICENSE.txt", "VST3 SDK", Group::builtIn },
            { "VST3-SDK-base-public-LICENSE.txt", "VST3 SDK: base and public.sdk", Group::builtIn },
            { "YaRN-LICENSE.txt", "YaRN", Group::builtIn },
            { "demucs.cpp-LICENSE.txt", "demucs.cpp", Group::builtIn },
            { "stb_vorbis-LICENSE.txt", "stb_vorbis", Group::builtIn },
            { "whisper.cpp-ggml-LICENSE.txt", "whisper.cpp and ggml", Group::builtIn },

            { "Demucs-weights-LICENSE.txt", "Demucs model weights", Group::downloaded },
            { "MuseScore-General-LICENSE.md", "MuseScore General sound font", Group::downloaded },
            { "Whisper-weights-LICENSE.txt", "Whisper model weights", Group::downloaded },

            { "Inno-Setup-LICENSE.txt", "Inno Setup", Group::installer }
        };

        const Entry* findEntry (const juce::String& fileName)
        {
            for (const auto& e : entries)
                if (fileName == e.fileName)
                    return &e;

            return nullptr;
        }

        int getGroupRank (Group group)
        {
            switch (group)
            {
                case Group::dawit:      return 0;
                case Group::builtIn:    return 1;
                case Group::downloaded: return 2;
                case Group::installer:  return 3;
                case Group::separate:   return 4;
                case Group::other:      return 5;
            }

            return 5;
        }

        juce::String getShort (const juce::String& commit)
        {
            return commit.substring (0, 7);
        }

        juce::String getCommitText (const juce::String& commit)
        {
            return commit.isEmpty() ? juce::String ("commit unknown") : "commit " + getShort (commit);
        }
    }

    juce::String getGroupName (Group group)
    {
        switch (group)
        {
            case Group::dawit:      return "DawIt";
            case Group::builtIn:    return "Built into DawIt";
            case Group::downloaded: return "Downloaded when you ask";
            case Group::installer:  return "Used to make the Windows installer";
            case Group::separate:   return "Separate programs";
            case Group::other:      return "Other notices";
        }

        return {};
    }

    bool findNoticeEntry (const juce::String& fileName, juce::String& name, Group& group)
    {
        if (auto* e = findEntry (fileName))
        {
            name = e->name;
            group = e->group;
            return true;
        }

        return false;
    }

    std::vector<Notice> getNotices()
    {
        std::vector<Notice> notices;

        for (int i = 0; i < dawnotices::namedResourceListSize; ++i)
        {
            const auto* resourceName = dawnotices::namedResourceList[i];
            int size = 0;
            const auto* data = dawnotices::getNamedResource (resourceName, size);

            if (data == nullptr)
                continue;

            Notice n;
            n.fileName = juce::String::fromUTF8 (dawnotices::getNamedResourceOriginalFilename (resourceName));
            n.text = juce::String::fromUTF8 (data, size);

            if (auto* e = findEntry (n.fileName))
            {
                n.name = e->name;
                n.group = e->group;
            }
            else
            {
                n.name = n.fileName;
                n.group = Group::other;
            }

            notices.push_back (std::move (n));
        }

        Notice ollama;
        ollama.name = "Ollama";
        ollama.group = Group::separate;
        ollama.text = ollamaText;
        notices.push_back (std::move (ollama));

        // DawIt's own two files keep their fixed order (licence first); every other group is alphabetical.
        std::stable_sort (notices.begin(), notices.end(), [] (const Notice& a, const Notice& b)
        {
            const auto ra = getGroupRank (a.group);
            const auto rb = getGroupRank (b.group);

            if (ra != rb)
                return ra < rb;

            if (a.group == Group::dawit)
                return a.fileName == "LICENSE" && b.fileName != "LICENSE";

            return a.name.compareNatural (b.name) < 0;
        });

        return notices;
    }

    juce::String getNoticeHeader (const Notice& notice)
    {
        auto header = notice.name + " (" + getGroupName (notice.group) + ")";

        if (notice.group == Group::downloaded)
            header += ". Not in the app: DawIt downloads this after asking you, and keeps this notice beside it.";

        return header;
    }

    juce::String getVersionLine (const juce::String& version, const juce::String& commit)
    {
        return "Version " + version + juce::String::fromUTF8 (" \xc2\xb7 ") + getCommitText (commit);
    }

    juce::String getCopyLine (const juce::String& version, const juce::String& commit)
    {
        return "DawIt " + version + " (" + getCommitText (commit) + ")";
    }

    SourceInfo getSourceInfo (const juce::String& version, const juce::String& commit, bool hasLocalChanges,
                              bool isExactTag, const juce::String& urlOverride)
    {
        const juce::String repo (repoUrl);
        const auto tagUrl = repo + "/tree/v" + version;
        const auto prefix = juce::String ("Source code of this version: ");
        const auto shortRepo = repo.fromFirstOccurrenceOf ("https://", false, false);
        const auto tagLine = prefix + shortRepo + ", tag v" + version;

        SourceInfo info;

        if (urlOverride.isNotEmpty())
        {
            info.url = urlOverride;
            info.line = prefix + urlOverride.fromFirstOccurrenceOf ("://", false, false);
            return info;
        }

        if (commit.isEmpty() || hasLocalChanges)
        {
            info.url = tagUrl;
            info.line = tagLine;
            info.note = "This build has local changes or no record of its commit. The link opens the source of DawIt "
                        + version + " as released.";
        }
        else if (isExactTag)
        {
            info.url = tagUrl;
            info.line = tagLine;
        }
        else
        {
            info.url = repo + "/tree/" + commit;
            info.line = prefix + shortRepo + " at commit " + getShort (commit);
        }

        return info;
    }

    juce::String getLicenceText()
    {
        return "DawIt is free software, licensed under the GNU Affero General Public License, version 3 only "
               "(AGPL-3.0-only). It comes with no warranty: not even the implied warranty that it is fit for sale "
               "or for a particular purpose.";
    }

    juce::String getRightsText()
    {
        return "You may run DawIt for any purpose, study how it works, change it, and share it, changed or not. "
               "When you pass it on, you must pass on these same rights and make its source code available. "
               "The full terms are under Licences...";
    }

    juce::String getThirdPartyText()
    {
        return "DawIt is built with JUCE, demucs.cpp and Eigen, ONNX Runtime with Basic Pitch, whisper.cpp, "
               "TinySoundFont and others. Each keeps its own licence: see Licences... Ollama, which DawIt can talk to "
               "for AI, is a separate program with its own licence and is not part of DawIt.";
    }
}
