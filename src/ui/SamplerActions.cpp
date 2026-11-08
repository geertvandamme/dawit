#include "SamplerActions.h"
#include "PluginActions.h"
#include "engine/SamplerGuide.h"

#include <map>

namespace daw::sampleractions
{
namespace
{
    std::function<void()>& getGuideOpener()
    {
        static std::function<void()> opener;
        return opener;
    }

    /** The sounds of a file's header, remembered while the file is unchanged: a menu opens often. */
    std::vector<soundfonts::PresetInfo> readPresetsCached (const juce::File& file)
    {
        struct Entry
        {
            juce::int64 size = 0;
            juce::Time modified;
            std::vector<soundfonts::PresetInfo> presets;
        };

        static std::map<juce::String, Entry> cache;
        auto& entry = cache[file.getFullPathName()];

        if (entry.presets.empty() || entry.size != file.getSize() || entry.modified != file.getLastModificationTime())
        {
            entry.size = file.getSize();
            entry.modified = file.getLastModificationTime();
            entry.presets = soundfonts::readPresets (file);
        }

        return entry.presets;
    }

    //==============================================================================
    /** Downloads MuseScore General in the background, with a progress window and a Cancel button. */
    class DownloadJob final : public juce::ThreadWithProgressWindow
    {
    public:
        explicit DownloadJob (std::function<void (const juce::String&)> whenDone)
            : juce::ThreadWithProgressWindow ("Downloading MuseScore General", true, true, 30000, "Cancel", nullptr),
              done (std::move (whenDone))
        {
            setStatusMessage ("Downloading DawIt Sampler's sound set ("
                              + models::formatSize (soundfonts::getMuseScoreGeneral().size) + ") to DawIt's SoundFonts folder...");
        }

        void run() override
        {
            error = soundfonts::downloadMuseScoreGeneral ([this] (double fraction)
                                                          {
                                                              setProgress (fraction);
                                                              return ! threadShouldExit();
                                                          });
        }

        void threadComplete (bool cancelled) override;

        std::function<void (const juce::String&)> done;
        juce::String error;
    };

    std::unique_ptr<DownloadJob> currentJob;

    void DownloadJob::threadComplete (bool cancelled)
    {
        // The progress window allows deleting the job here; the callback may start another.
        const auto self = std::move (currentJob);
        const auto message = cancelled ? juce::String() : error;
        const auto callback = done;

        if (! cancelled && message.isEmpty() && soundfonts::isMuseScoreGeneralInstalled())
            callback ({});
        else if (message.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't download MuseScore General", message);
    }

    /** Every sampler that waits for MuseScore General starts loading it. */
    void retryWaitingSamplers (Engine& engine)
    {
        const auto reference = soundfonts::getMuseScoreGeneralReference();

        for (auto& track : engine.getTracks())
            for (auto& slot : track->plugins)
                if (auto* s = dynamic_cast<BuiltInSampler*> (slot->instance.get()))
                    if (s->getFontReference().fileName == reference.fileName && s->getStatus() == BuiltInSampler::Status::missing)
                        s->setFont (reference);
    }

    //==============================================================================
    void showProblem (const juce::String& title, const juce::String& message)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, title, message);
    }

    /** The sound a menu item or the window is on, for ticks. */
    struct Current
    {
        soundfonts::Reference font;
        int bank = -1, program = -1;
    };

    juce::String getBankName (int bank)
    {
        return bank == soundfonts::drumBank ? juce::String ("Drum kits") : "Bank " + juce::String (bank);
    }

    /** The sounds of one SoundFont as menu items: bank 0 by General MIDI family when there are more than 16,
        other banks under "More banks", and bank 128 under "Drum kits". */
    juce::PopupMenu createFontMenu (const soundfonts::Reference& font, const std::vector<soundfonts::PresetInfo>& presets,
                                    const Current& current, const PickFn& pick)
    {
        const bool isCurrentFont = current.font == font;
        juce::PopupMenu menu;

        auto addPreset = [&] (juce::PopupMenu& to, const soundfonts::PresetInfo& p)
        {
            const bool ticked = isCurrentFont && current.bank == p.bank && current.program == p.program;
            to.addItem (p.getLabel(), true, ticked, [pick, font, bank = p.bank, program = p.program] { pick (font, bank, program); });
        };

        auto holdsCurrent = [&] (const std::vector<const soundfonts::PresetInfo*>& list)
        {
            return isCurrentFont && std::any_of (list.begin(), list.end(), [&] (auto* p) { return p->bank == current.bank && p->program == current.program; });
        };

        std::vector<const soundfonts::PresetInfo*> bankZero, drums;
        std::map<int, std::vector<const soundfonts::PresetInfo*>> others;

        for (auto& p : presets)
        {
            if (p.bank == 0)
                bankZero.push_back (&p);
            else if (p.bank == soundfonts::drumBank)
                drums.push_back (&p);
            else
                others[p.bank].push_back (&p);
        }

        if (bankZero.size() > 16)
        {
            for (int family = 0; family < sampler::numFamilies; ++family)
            {
                juce::PopupMenu sub;
                std::vector<const soundfonts::PresetInfo*> inFamily;

                for (auto* p : bankZero)
                    if (sampler::getFamily (p->program) == family)
                        inFamily.push_back (p);

                for (auto* p : inFamily)
                    addPreset (sub, *p);

                if (! inFamily.empty())
                    menu.addSubMenu (samplerguide::getFamilyName (family), sub, true, nullptr, holdsCurrent (inFamily));
            }
        }
        else
        {
            for (auto* p : bankZero)
                addPreset (menu, *p);
        }

        if (! others.empty())
        {
            juce::PopupMenu more;
            std::vector<const soundfonts::PresetInfo*> all;

            for (auto& [bank, list] : others)
            {
                juce::PopupMenu sub;

                for (auto* p : list)
                {
                    addPreset (sub, *p);
                    all.push_back (p);
                }

                more.addSubMenu (getBankName (bank), sub, true, nullptr, holdsCurrent (list));
            }

            menu.addSubMenu ("More banks", more, true, nullptr, holdsCurrent (all));
        }

        if (! drums.empty())
        {
            juce::PopupMenu kits;

            for (auto* p : drums)
                addPreset (kits, *p);

            menu.addSubMenu ("Drum kits", kits, true, nullptr, holdsCurrent (drums));
        }

        return menu;
    }
} // namespace

//==============================================================================
BuiltInSampler* findSampler (Track& track)
{
    for (auto& slot : track.plugins)
        if (auto* s = dynamic_cast<BuiltInSampler*> (slot->instance.get()))
            return s;

    return nullptr;
}

bool getFirstPreset (const soundfonts::Reference& reference, int& bank, int& program)
{
    const auto file = reference.find();

    if (file == juce::File())
        return false;

    const auto presets = readPresetsCached (file);

    if (presets.empty())
        return false;

    bank = presets.front().bank;
    program = presets.front().program;

    for (auto& p : presets)
        if (p.bank == 0 && p.program == 0)
        {
            bank = 0;
            program = 0;
            break;
        }

    return true;
}

bool needsDownload (const soundfonts::Reference& reference)
{
    return reference.fileName == soundfonts::museScoreGeneralFileName && ! soundfonts::isMuseScoreGeneralInstalled();
}

void whenMuseScoreGeneralIsHere (Engine& engine, std::function<void()> then)
{
    if (soundfonts::isMuseScoreGeneralInstalled())
    {
        then();
        return;
    }

    if (currentJob != nullptr)
        return;   // it is downloading already

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Download MuseScore General?",
                                        "DawIt Sampler's free sound set: all 128 General MIDI sounds, drum kits and more, about "
                                            + models::formatSize (soundfonts::getMuseScoreGeneral().size)
                                            + ", MIT licence. It is downloaded once to DawIt's SoundFonts folder. Until then the track plays the DawIt Synth.",
                                        "Download", "Not now", nullptr,
                                        juce::ModalCallbackFunction::create ([&engine, onHere = std::move (then)] (int result)
                                        {
                                            if (result != 1 || currentJob != nullptr)
                                                return;

                                            currentJob = std::make_unique<DownloadJob> ([&engine, onHere] (const juce::String&)
                                                                                        {
                                                                                            retryWaitingSamplers (engine);
                                                                                            onHere();
                                                                                        });
                                            currentJob->launchThread();
                                        }));
}

void addSoundFont (Engine& engine, PickFn whenAdded)
{
    auto chooser = std::make_shared<juce::FileChooser> ("Add a SoundFont", juce::File(), "*.sf2;*.sf3;*.sfz");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [&engine, chooser, onAdded = std::move (whenAdded)] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (file == juce::File())
                                  return;

                              // Other kinds of file get their one sentence; nothing is copied.
                              if (! soundfonts::isSoundFontFile (file))
                              {
                                  showProblem ("Can't add this file", soundfonts::addFile (file, false).problem);
                                  return;
                              }

                              if (const auto checked = soundfonts::check (file); ! checked.ok)
                              {
                                  showProblem ("Can't add this SoundFont", checked.problem);
                                  return;
                              }

                              const auto options = juce::MessageBoxOptions()
                                                       .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                                       .withTitle ("Add \"" + file.getFileName() + "\"")
                                                       .withMessage ("Copy it into DawIt's SoundFonts folder, or use it where it is? A copy keeps working if you "
                                                                     "move or delete the original; using it where it is saves disk space.")
                                                       .withButton ("Copy into DawIt's SoundFonts folder")
                                                       .withButton ("Use it where it is")
                                                       .withButton ("Cancel");

                              // Results: 1 = copy, 2 = use where it is, 0 = cancel.
                              juce::AlertWindow::showAsync (options, [&engine, file, onAdded] (int result)
                                                            {
                                                                if (result != 1 && result != 2)
                                                                    return;

                                                                const auto added = soundfonts::addFile (file, result == 1);

                                                                if (! added.ok)
                                                                {
                                                                    showProblem ("Can't add this SoundFont", added.problem);
                                                                    return;
                                                                }

                                                                if (result == 2)
                                                                    if (auto* settings = engine.properties.getUserSettings())
                                                                        soundfonts::addLinked (*settings, file);

                                                                int bank = 0, program = 0;

                                                                if (! getFirstPreset (added.reference, bank, program))
                                                                {
                                                                    showProblem ("Can't add this SoundFont", "It has no sounds to play.");
                                                                    return;
                                                                }

                                                                onAdded (added.reference, bank, program);
                                                            });
                          });
}

juce::PopupMenu createSampledSoundsMenu (Engine& engine, const BuiltInSampler* currentSampler, PickFn pick, std::function<void()> openGuide)
{
    Current current;

    if (currentSampler != nullptr)
    {
        current.font = currentSampler->getFontReference();
        current.bank = currentSampler->getBank();
        current.program = currentSampler->getProgram();
    }

    juce::PopupMenu menu;
    const auto muse = soundfonts::getMuseScoreGeneralReference();

    if (soundfonts::isMuseScoreGeneralInstalled())
    {
        const auto presets = readPresetsCached (muse.find());
        menu.addSubMenu ("MuseScore General", createFontMenu (muse, presets, current, pick), ! presets.empty(), nullptr, current.font == muse);
    }
    else
    {
        // Its sounds aren't known until it is here: offer the starting points, which ask to download first.
        juce::PopupMenu sub;
        sub.addSectionHeader ("Picking one asks to download it (about " + models::formatSize (soundfonts::getMuseScoreGeneral().size) + ")");

        for (auto& point : samplerguide::getStartingPoints())
            sub.addItem (point.name, true, current.font.fileName == muse.fileName && current.bank == point.bank && current.program == point.shownProgram - 1,
                         [pick, muse, bank = point.bank, program = point.shownProgram - 1] { pick (muse, bank, program); });

        menu.addSubMenu ("MuseScore General (not downloaded)", sub, true, nullptr, current.font.fileName == muse.fileName);
    }

    const auto* settings = engine.properties.getUserSettings();

    for (auto& font : soundfonts::getAddedFonts (settings))
    {
        const auto file = font.find();
        const auto presets = file == juce::File() ? std::vector<soundfonts::PresetInfo>() : readPresetsCached (file);
        menu.addSubMenu (font.fileName, createFontMenu (font, presets, current, pick), ! presets.empty(), nullptr, current.font == font);
    }

    menu.addSeparator();
    menu.addItem ("Add SoundFont...", [&engine, pick] { addSoundFont (engine, pick); });

    if (openGuide)
    {
        menu.addSeparator();
        menu.addItem ("What is a sampler?...", std::move (openGuide));
    }

    return menu;
}

void useSampledSound (Engine& engine, PluginWindowManager& windows, Track& track, const soundfonts::Reference& reference, int bank, int program)
{
    auto apply = [&engine, &windows, trackPtr = &track, reference, bank, program]
    {
        // The track may have been deleted while the question or the download was open.
        if (engine.indexOf (trackPtr) < 0)
            return;

        auto* s = findSampler (*trackPtr);

        if (s == nullptr)
        {
            plugins::add (engine, windows, *trackPtr, BuiltInSampler::getDescription());
            s = findSampler (*trackPtr);
        }

        if (s == nullptr)
            return;

        if (! (s->getFontReference() == reference) || s->getStatus() == BuiltInSampler::Status::missing)
            s->setFont (reference);

        s->setPreset (bank, program);
        s->clap.request();   // the test note, so a pick is heard straight away
    };

    if (needsDownload (reference))
        whenMuseScoreGeneralIsHere (engine, apply);
    else
        apply();
}

void setGuideOpener (std::function<void()> opener)
{
    getGuideOpener() = std::move (opener);
}

juce::PopupMenu createTrackMenu (Engine& engine, PluginWindowManager& windows, Track& track)
{
    return createSampledSoundsMenu (engine, findSampler (track),
                                    [&engine, &windows, trackPtr = &track] (const soundfonts::Reference& reference, int bank, int program)
                                    {
                                        if (engine.indexOf (trackPtr) >= 0)
                                            useSampledSound (engine, windows, *trackPtr, reference, bank, program);
                                    },
                                    getGuideOpener());
}
} // namespace daw::sampleractions
