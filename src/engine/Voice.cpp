#include "Voice.h"

#include <juce_data_structures/juce_data_structures.h>

namespace daw::sing
{
const std::vector<juce::String>& getCmuVowels()
{
    static const std::vector<juce::String> v { "aa", "ae", "ah", "ao", "aw", "ay", "eh", "er", "ey", "ih", "iy", "ow", "oy", "uh", "uw" };
    return v;
}

const std::vector<juce::String>& getCmuConsonants()
{
    static const std::vector<juce::String> c { "b", "ch", "d", "dh", "f", "g", "hh", "jh", "k", "l", "m", "n",
                                               "ng", "p", "r", "s", "sh", "t", "th", "v", "w", "y", "z", "zh" };
    return c;
}

namespace
{
    juce::File voicesFolderOverride;
}

void overrideVoicesFolder (const juce::File& f) { voicesFolderOverride = f; }

juce::File getVoicesFolder()
{
    if (voicesFolderOverride != juce::File())
        return voicesFolderOverride;

    juce::PropertiesFile::Options options;
    options.applicationName = "DawIt";
    options.folderName = "DawIt";
    options.osxLibrarySubFolder = "Application Support";
    options.filenameSuffix = "settings";

    return options.getDefaultFile().getSiblingFile ("Voices");
}

//==============================================================================
juce::String Voice::stripPrefix (const juce::String& symbol) const
{
    const int slash = symbol.indexOfChar ('/');
    return (slash >= 0 ? symbol.substring (slash + 1) : symbol).toLowerCase();
}

juce::String Voice::mapPhoneme (const juce::String& cmuPhoneme) const
{
    for (const auto& candidate : { languagePrefix + cmuPhoneme, cmuPhoneme })
        if (phonemeIds.count (candidate) > 0)
            return candidate;

    return {};
}

bool Voice::isVowel (const juce::String& symbol) const
{
    if (dictionaryVowels.count (symbol) > 0)
        return true;

    const auto bare = stripPrefix (symbol);

    for (auto& v : getCmuVowels())
        if (v == bare)
            return true;

    return false;
}

//==============================================================================
namespace
{
    juce::String readText (const juce::File& f)
    {
        return f.loadFileAsString();
    }

    /** A child file by name, ignoring case; an empty File when it isn't there. */
    juce::File findChild (const juce::File& folder, const juce::String& name)
    {
        for (auto& f : folder.findChildFiles (juce::File::findFilesAndDirectories, false))
            if (f.getFileName().equalsIgnoreCase (name))
                return f;

        return {};
    }

    /** The symbols of a phonemes.json ({"SP": 0, ...}) or phonemes.txt (one per line, or "symbol id"). */
    bool readPhonemes (const juce::File& file, std::map<juce::String, int>& out, juce::String& error)
    {
        const auto text = readText (file);

        if (file.hasFileExtension ("json"))
        {
            const auto json = juce::JSON::parse (text);

            if (auto* object = json.getDynamicObject())
            {
                for (auto& p : object->getProperties())
                    out[p.name.toString()] = (int) p.value;

                return ! out.empty() || (error = file.getFileName() + " has no phonemes", false);
            }

            error = file.getFileName() + " isn't a list of phonemes with their numbers";
            return false;
        }

        int id = 0;

        for (auto line : juce::StringArray::fromLines (text))
        {
            line = line.trim();

            if (line.isEmpty())
                continue;

            auto parts = juce::StringArray::fromTokens (line, " \t", "");
            parts.removeEmptyStrings();

            if (parts.size() >= 2 && parts[1].containsOnly ("0123456789"))
                out[parts[0]] = parts[1].getIntValue();
            else
                out[parts[0]] = id;

            ++id;
        }

        return ! out.empty() || (error = file.getFileName() + " has no phonemes", false);
    }

    bool readLanguages (const juce::File& file, std::map<juce::String, int>& out)
    {
        const auto json = juce::JSON::parse (readText (file));

        if (auto* object = json.getDynamicObject())
            for (auto& p : object->getProperties())
                out[p.name.toString()] = (int) p.value;

        return ! out.empty();
    }

    /** `dsdict-en.yaml`: `symbols` (a name and a type each) and `entries` (a grapheme and its phonemes each). */
    void readDictionary (const yaml::Node& root, Voice& voice)
    {
        if (const auto* symbols = root.find ("symbols"); symbols != nullptr && symbols->isList())
            for (auto& s : symbols->items)
                if (s.getString ("type") == "vowel")
                    voice.dictionaryVowels.insert (s.getString ("symbol"));

        if (const auto* entries = root.find ("entries"); entries != nullptr && entries->isList())
        {
            for (auto& e : entries->items)
            {
                const auto word = e.getString ("grapheme").toLowerCase();
                const auto* sounds = e.find ("phonemes");

                if (word.isEmpty() || sounds == nullptr || ! sounds->isList())
                    continue;

                std::vector<juce::String> list;

                for (auto& p : sounds->items)
                    list.push_back (p.scalar);

                voice.dictionary.emplace (word, std::move (list));
            }
        }
    }

    /** The terms to show: "Terms of Use" first, then a readme, then a licence. */
    juce::File findTerms (const juce::File& root)
    {
        juce::File readme, licence;

        for (auto& f : root.findChildFiles (juce::File::findFiles, false))
        {
            const auto name = f.getFileName().toLowerCase();

            if (name.startsWith ("terms"))
                return f;

            if (name.startsWith ("readme"))
                readme = f;
            else if (name.startsWith ("licen"))
                licence = f;
        }

        return readme != juce::File() ? readme : licence;
    }

    /** The vocoder a voice names: in its own dsvocoder folder, or in the shared Vocoders folder. */
    bool findVocoder (const juce::String& name, const juce::File& root, const juce::File& vocodersFolder, VoiceFiles& out)
    {
        for (const auto& base : { root.getChildFile ("dsvocoder"), vocodersFolder })
        {
            if (! base.isDirectory())
                continue;

            // A folder with the vocoder's name, or any folder whose vocoder.yaml gives that name.
            for (auto& config : base.findChildFiles (juce::File::findFiles, true, "vocoder.yaml"))
            {
                const auto parsed = yaml::parse (config.loadFileAsString());
                const auto declared = parsed.ok() ? parsed.root.getString ("name") : juce::String();

                if (config.getParentDirectory().getFileName().equalsIgnoreCase (name) || declared.equalsIgnoreCase (name))
                {
                    const auto modelName = parsed.ok() ? parsed.root.getString ("model", "model.onnx") : juce::String ("model.onnx");
                    out.vocoderConfig = config;
                    out.vocoderModel = config.getSiblingFile (modelName);
                    return true;
                }
            }
        }

        return false;
    }
}

juce::File findVoiceRoot (const juce::File& folder)
{
    if (findChild (folder, "dsconfig.yaml").existsAsFile())
        return folder;

    for (auto& sub : folder.findChildFiles (juce::File::findDirectories, false))
    {
        if (sub.getFileName().startsWith ("__MACOSX"))
            continue;

        if (findChild (sub, "dsconfig.yaml").existsAsFile())
            return sub;

        for (auto& sub2 : sub.findChildFiles (juce::File::findDirectories, false))
            if (findChild (sub2, "dsconfig.yaml").existsAsFile())
                return sub2;
    }

    return {};
}

VoiceCheck loadVoice (const juce::File& folder, const juce::File& vocodersFolder)
{
    VoiceCheck result;
    const auto root = findVoiceRoot (folder);

    if (root == juce::File())
    {
        result.error = "dsconfig.yaml is missing from " + folder.getFileName() + ": it isn't a DiffSinger voice in OpenUtau's layout.";
        return result;
    }

    Voice voice;
    voice.files.root = root;
    voice.name = root.getFileName();
    auto parsed = yaml::parse (findChild (root, "dsconfig.yaml").loadFileAsString());

    if (! parsed.ok())
    {
        result.error = "dsconfig.yaml can't be read (" + parsed.error + ").";
        return result;
    }

    voice.config = std::move (parsed.root);

    if (! voice.config.isMap())
    {
        result.error = "dsconfig.yaml doesn't hold the voice's settings.";
        return result;
    }

    // The name and author, from OpenUtau's character files.
    if (const auto txt = findChild (root, "character.txt"); txt.existsAsFile())
        for (auto line : juce::StringArray::fromLines (txt.loadFileAsString()))
        {
            if (line.startsWithIgnoreCase ("name="))
                voice.name = line.fromFirstOccurrenceOf ("=", false, false).trim();
            else if (line.startsWithIgnoreCase ("author="))
                voice.author = line.fromFirstOccurrenceOf ("=", false, false).trim();
        }

    if (const auto cy = findChild (root, "character.yaml"); cy.existsAsFile())
    {
        const auto c = yaml::parse (cy.loadFileAsString());

        if (c.ok())
        {
            if (c.root.getString ("name").isNotEmpty())
                voice.name = c.root.getString ("name");

            if (c.root.getString ("author").isNotEmpty())
                voice.author = c.root.getString ("author");
        }
    }

    auto need = [&] (const juce::String& key, const juce::String& fallback, juce::File& out) -> bool
    {
        const auto name = voice.config.getString (key, fallback);
        out = name.isEmpty() ? juce::File() : root.getChildFile (name);

        if (name.isEmpty() || ! out.existsAsFile())
        {
            result.error = (name.isEmpty() ? juce::String (key) + " is not named in dsconfig.yaml" : name + " is missing from " + root.getFileName())
                           + ".";
            return false;
        }

        return true;
    };

    if (! need ("acoustic", "acoustic.onnx", voice.files.acoustic))
        return result;

    // The phonemes: the file dsconfig names, or phonemes.json / phonemes.txt next to it.
    {
        auto name = voice.config.getString ("phonemes");
        voice.files.phonemes = name.isNotEmpty() ? root.getChildFile (name) : juce::File();

        if (! voice.files.phonemes.existsAsFile())
            for (const auto& alt : { "phonemes.json", "phonemes.txt" })
                if (root.getChildFile (alt).existsAsFile())
                    voice.files.phonemes = root.getChildFile (alt);

        if (! voice.files.phonemes.existsAsFile())
        {
            result.error = (name.isNotEmpty() ? name : juce::String ("phonemes.json")) + " is missing from " + root.getFileName() + ".";
            return result;
        }

        juce::String error;

        if (! readPhonemes (voice.files.phonemes, voice.phonemeIds, error))
        {
            result.error = error + ".";
            return result;
        }
    }

    if (voice.phonemeIds.count ("SP") == 0)
    {
        result.error = voice.files.phonemes.getFileName() + " has no SP (silence) in it.";
        return result;
    }

    if (const auto langName = voice.config.getString ("languages"); langName.isNotEmpty() && root.getChildFile (langName).existsAsFile())
    {
        voice.files.languages = root.getChildFile (langName);
        readLanguages (voice.files.languages, voice.languageIds);

        if (const auto it = voice.languageIds.find ("en"); it != voice.languageIds.end())
            voice.englishLanguageId = it->second;
    }

    // The dictionary: dsdict-en.yaml (OpenUtau keeps it beside the duration model), then dsdict.yaml.
    for (const auto& name : { "dsdict-en.yaml", "dsdict.yaml" })
    {
        juce::File found;

        for (auto& base : { root, root.getChildFile ("dsdur"), root.getChildFile ("dspitch"), root.getChildFile ("dsmain") })
            if (found == juce::File() && findChild (base, name).existsAsFile())
                found = findChild (base, name);

        if (found != juce::File())
        {
            const auto d = yaml::parse (found.loadFileAsString());

            if (! d.ok())
            {
                result.error = found.getFileName() + " can't be read (" + d.error + ").";
                return result;
            }

            if (voice.files.dictionary == juce::File())
                voice.files.dictionary = found;

            readDictionary (d.root, voice);

            if (juce::String (name) == "dsdict-en.yaml")
                break;
        }
    }

    // English: phonemes with an "en/" prefix, or the sounds of ay and iy, or a dsdict-en file.
    {
        bool prefixed = false;

        for (auto& p : voice.phonemeIds)
            if (p.first.startsWith ("en/"))
                prefixed = true;

        if (prefixed)
            voice.languagePrefix = "en/";

        const bool bare = voice.phonemeIds.count ("ay") > 0 && voice.phonemeIds.count ("iy") > 0;
        voice.hasEnglish = prefixed || bare || (voice.files.dictionary != juce::File() && voice.files.dictionary.getFileName() == "dsdict-en.yaml");
    }

    if (! voice.hasEnglish)
    {
        result.error = voice.name + " has no English: its phonemes have no \"en/\" sounds, and there is no dsdict-en.yaml. "
                       "This version of Sing this sings English only.";
        return result;
    }

    voice.sampleRate = voice.config.getInt ("sample_rate", 44100);
    voice.hopSize = voice.config.getInt ("hop_size", 512);
    voice.numMelBins = voice.config.getInt ("num_mel_bins", 128);

    if (const auto* speakers = voice.config.find ("speakers"); speakers != nullptr && speakers->isList())
        voice.numSpeakers = (int) speakers->items.size();

    // The vocoder.
    {
        const auto name = voice.config.getString ("vocoder");

        if (name.isEmpty())
        {
            result.error = "vocoder is not named in dsconfig.yaml.";
            return result;
        }

        if (! findVocoder (name, root, vocodersFolder, voice.files))
        {
            result.vocoderMissing = true;
            result.error = "The vocoder \"" + name + "\" is missing. Add it with Add a voice... (a .oudep file), or put it in the Vocoders folder.";
            return result;
        }

        if (! voice.files.vocoderModel.existsAsFile())
        {
            result.error = voice.files.vocoderModel.getFileName() + " is missing from the vocoder \"" + name + "\".";
            return result;
        }

        // Its licence, when the voice ships one for it (Hanami's is in __License).
        for (auto& dir : { voice.files.vocoderConfig.getParentDirectory(), root.getChildFile ("__License") })
            for (auto& f : dir.findChildFiles (juce::File::findFiles, false, "*ocoder*"))
                if (f.hasFileExtension ("txt;md") && f.loadFileAsString().containsIgnoreCase ("NonCommercial"))
                    voice.vocoderLicence = "CC BY-NC-SA 4.0";
    }

    // The optional models. Each sub-folder has a dsconfig.yaml that names its files.
    for (auto sub : { "dsdur", "dspitch" })
    {
        const auto dir = findChild (root, sub);
        const auto cfgFile = findChild (dir, "dsconfig.yaml");

        if (! cfgFile.existsAsFile())
            continue;

        const auto cfg = yaml::parse (cfgFile.loadFileAsString());

        if (! cfg.ok())
        {
            result.error = juce::String (sub) + "/dsconfig.yaml can't be read (" + cfg.error + ").";
            return result;
        }

        const bool isDur = juce::String (sub) == "dsdur";
        auto& linguistic = isDur ? voice.files.durLinguistic : voice.files.pitchLinguistic;
        auto& model = isDur ? voice.files.dur : voice.files.pitch;
        const auto linguisticName = cfg.root.getString ("linguistic");
        const auto modelName = cfg.root.getString (isDur ? "dur" : "pitch");

        if (modelName.isEmpty())
            continue;

        linguistic = linguisticName.isEmpty() ? juce::File() : dir.getChildFile (linguisticName);
        model = dir.getChildFile (modelName);

        if (const auto* sp = cfg.root.find ("speakers"); sp != nullptr && sp->isList() && ! sp->items.empty())
            (isDur ? voice.files.durSpeaker : voice.files.pitchSpeaker) = dir.getChildFile (sp->items.front().scalar + ".emb");

        for (auto* f : { &linguistic, &model })
        {
            if (*f != juce::File() && ! f->existsAsFile())
            {
                result.error = f->getFileName() + " is missing from " + juce::String (sub) + ".";
                return result;
            }
        }
    }

    voice.files.terms = findTerms (root);
    result.voice = std::move (voice);
    return result;
}

std::vector<juce::File> listVoices (const juce::File& voicesFolder)
{
    std::vector<juce::File> voices;

    for (auto& f : voicesFolder.findChildFiles (juce::File::findDirectories, false))
        if (f.getFileName() != "Vocoders" && findVoiceRoot (f) != juce::File())
            voices.push_back (f);

    std::sort (voices.begin(), voices.end(), [] (const juce::File& a, const juce::File& b) { return a.getFileName() < b.getFileName(); });
    return voices;
}
} // namespace daw::sing
