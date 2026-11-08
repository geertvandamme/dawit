#include "PluginActions.h"
#include "BuiltInSynthEditor.h"
#include "SamplerActions.h"
#include "engine/BuiltInDelay.h"
#include "engine/BuiltInFormat.h"
#include "engine/BuiltInReverb.h"
#include "engine/UserPresets.h"

#include <map>

namespace daw::plugins
{
bool contains (const Track& track, const PluginSlot* slot)
{
    return std::any_of (track.plugins.begin(), track.plugins.end(), [slot] (auto& p) { return p.get() == slot; });
}

juce::String getFormatLabel (const juce::PluginDescription& d)
{
    if (d.pluginFormatName == BuiltInFormat::formatName)
        return "built-in";

    return d.pluginFormatName == "AudioUnit" ? juce::String ("AU") : d.pluginFormatName;
}

juce::PopupMenu createAddMenu (const juce::KnownPluginList& known, bool instruments, int firstItemId,
                               juce::Array<juce::PluginDescription>& choices)
{
    std::map<juce::String, juce::PopupMenu> byVendor;
    auto types = known.getTypes();
    std::sort (types.begin(), types.end(), [] (auto& a, auto& b)
               {
                   const int byName = a.name.compareNatural (b.name);
                   return byName != 0 ? byName < 0 : a.pluginFormatName < b.pluginFormatName;
               });

    // Many plugins come as both VST3 and AU: once the list holds more than one format, say which is which.
    const bool showFormat = std::any_of (types.begin(), types.end(), [&types] (auto& d)
                                         { return d.pluginFormatName != types.getReference (0).pluginFormatName; });

    for (auto& d : types)
    {
        if (d.isInstrument != instruments || isDawItPluginCopy (d))
            continue;

        choices.add (d);
        byVendor[d.manufacturerName.isEmpty() ? juce::String ("Unknown") : d.manufacturerName]
            .addItem (firstItemId + choices.size() - 1, showFormat ? d.name + "  (" + getFormatLabel (d) + ")" : d.name);
    }

    juce::PopupMenu menu;
    for (auto& [vendor, items] : byVendor)
        menu.addSubMenu (vendor, items);

    return menu;
}

void open (PluginWindowManager& windows, Track& track, PluginSlot& slot)
{
    windows.show (*slot.instance, track.name + " - " + slot.instance->getName());
}

void openBuiltInSynth (Engine& engine, PluginWindowManager& windows, Track& track)
{
    windows.show (&track, track.name + " - DawIt Synth", [&] { return new BuiltInSynthEditor (engine, windows, track); });
}

void useBuiltInSynth (Engine& engine, PluginWindowManager& windows, Track& track)
{
    std::vector<PluginSlot*> instruments;

    for (auto& p : track.plugins)
        if (p->isInstrument)
            instruments.push_back (p.get());

    for (auto* slot : instruments)
        remove (engine, windows, track, *slot);

    openBuiltInSynth (engine, windows, track);
}

void applyReturnStart (PluginSlot& slot)
{
    if (auto* reverb = dynamic_cast<BuiltInReverb*> (slot.instance.get()))
        reverb->setSettings (verb::getReturnPreset().settings);
    else if (auto* delay = dynamic_cast<BuiltInDelay*> (slot.instance.get()))
        delay->setSettings (dly::getReturnPreset().settings);
}

void add (Engine& engine, PluginWindowManager& windows, Track& track, const juce::PluginDescription& description)
{
    std::vector<PluginSlot*> previousInstruments;

    if (description.isInstrument)
        for (auto& p : track.plugins)
            if (p->isInstrument)
                previousInstruments.push_back (p.get());

    juce::String error;
    auto* slot = engine.addPlugin (track, description, error, description.isInstrument ? 0 : -1);

    if (slot == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't load plugin", error);
        return;
    }

    for (auto* old : previousInstruments)
        remove (engine, windows, track, *old);

    // DawIt's reverb and delay start at their Return: starting point on a return track (100% mix).
    if (track.isReturn())
        applyReturnStart (*slot);

    if (description.isInstrument)
        windows.closeFor (&track); // the built-in synth's window

    open (windows, track, *slot);
}

void remove (Engine& engine, PluginWindowManager& windows, Track& track, PluginSlot& slot)
{
    windows.closeFor (*slot.instance);
    engine.removePlugin (track, &slot);
}

void showAddMenu (Engine& engine, PluginWindowManager& windows, Track& track, bool instruments, juce::Component* target)
{
    if (instruments && track.hasInput())
    {
        // A bus plays what its tracks send into it, and a return what is sent to it: neither has an instrument.
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, track.isReturn() ? "No instrument on a return" : "No instrument on a bus",
                                                track.isReturn() ? "A return has no instrument: it plays what tracks send to it."
                                                                 : "A bus has no instrument: it plays what its tracks send into it.");
        return;
    }

    juce::Array<juce::PluginDescription> choices;
    juce::PopupMenu menu;

    // DawIt's own effects and instruments first: they're always there, without scanning. The DawIt Synth
    // isn't a plugin (a track plays it when it has no instrument), so it gets an item of its own.
    constexpr int builtInSynthItem = 100000;

    // MIDI effects change the notes before they reach the instrument: only a MIDI track has notes to change.
    if (! instruments && track.isMidi())
    {
        menu.addSectionHeader ("Before the instrument");

        for (auto& d : BuiltInFormat::getMidiEffects())
        {
            choices.add (d);
            menu.addItem (choices.size(), d.name + "  -  turns held chords into patterns");
        }
    }

    menu.addSectionHeader ("Built-in");

    if (instruments)
        menu.addItem (builtInSynthItem, "DawIt Synth  -  " + juce::String ((int) builtInSounds::all.size()) + " ready-made sounds, no knobs",
                      true, track.usesBuiltInSynth());

    for (auto& d : instruments ? BuiltInFormat::getInstruments() : BuiltInFormat::getEffects())
    {
        choices.add (d);
        menu.addItem (choices.size(), d.name + "  -  explains itself as you use it");
    }

    if (instruments)
        menu.addSubMenu ("Sampled sounds  -  real instruments for DawIt Sampler", sampleractions::createTrackMenu (engine, windows, track));

    menu.addSectionHeader ("Plugins");

    const int numBuiltIn = choices.size();   // also counts the MIDI effects above
    auto pluginMenu = createAddMenu (engine.knownPlugins, instruments, 1, choices);

    for (juce::PopupMenu::MenuItemIterator it (pluginMenu); it.next();)
        menu.addItem (it.getItem());

    if (choices.size() == numBuiltIn)
        menu.addItem (-1, juce::String ("No ") + (instruments ? "instrument" : "effect")
                              + " plugins found yet - use File > Plugins... to scan", false);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [&engine, &windows, trackPtr = &track, choices] (int result)
                        {
                            if (engine.indexOf (trackPtr) < 0)
                                return;

                            if (result == builtInSynthItem)
                                useBuiltInSynth (engine, windows, *trackPtr);
                            else if (result >= 1 && result <= choices.size())
                                add (engine, windows, *trackPtr, choices.getReference (result - 1));
                        });
}

namespace
{
    /** Shows why a preset call failed. */
    void showPresetProblem (const juce::String& message)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Preset", message);
    }

    /** Loads a preset into a slot's plugin; a hosted plugin changes state under the engine lock. */
    void loadPresetInto (Engine& engine, PluginSlot& slot, const presets::Preset& preset)
    {
        presets::ApplyResult result;

        if (preset.plugin.isBuiltIn())
            result = presets::apply (preset, *slot.instance);
        else
        {
            const juce::ScopedLock lock (engine.getLock());
            result = presets::apply (preset, *slot.instance);
        }

        if (! result.ok)
            showPresetProblem (result.message);
        else if (result.versionDiffers)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Preset loaded",
                                                    "\"" + preset.name + "\" was saved with version " + result.savedVersion
                                                        + " of this plugin; it may sound a little different now.");
    }

    /** Saves the plugin's settings under a name; asks before replacing. */
    void saveSettingsAs (Engine& engine, PluginSlot& slot, const juce::String& rawName, bool replace)
    {
        const auto name = presets::tidyName (rawName);

        if (const auto why = presets::checkName (name); why.isNotEmpty())
        {
            showPresetProblem (why);
            return;
        }

        juce::MemoryBlock state;
        {
            const juce::ScopedLock lock (engine.getLock());
            slot.instance->getStateInformation (state);
        }

        const auto id = presets::identityOf (*slot.instance);
        const presets::Library library;
        const auto result = library.save (id, name, state, replace);

        if (result.nameTaken)
        {
            // The slot may be gone by the time the answer comes: save from the state taken now.
            juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                              .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                              .withTitle ("Replace preset?")
                                              .withMessage ("A preset called \"" + name + "\" is there already. Replace it?")
                                              .withButton ("Replace")
                                              .withButton ("Cancel"),
                                          [id, name, state] (int choice)
                                          {
                                              if (choice != 1)
                                                  return;

                                              const auto again = presets::Library().save (id, name, state, true);

                                              if (! again.ok)
                                                  showPresetProblem (again.message);
                                          });
            return;
        }

        if (! result.ok)
        {
            showPresetProblem (result.message);
            return;
        }

        // Said once: what a plugin saves is what it reports.
        static bool toldAboutHosted = false;

        if (! id.isBuiltIn() && ! toldAboutHosted)
        {
            toldAboutHosted = true;
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Saved \"" + result.name + "\"",
                                                    "DawIt saves what the plugin reports as its settings. Most plugins restore all of it. Some keep part "
                                                    "of their sound elsewhere (samples, a library, a licence) and may not sound the same. Listen after loading.");
        }
    }

    /** The name prompt: a text box and Save / Cancel. */
    void askPresetName (const juce::String& suggestion, std::function<void (const juce::String&)> onName)
    {
        auto* window = new juce::AlertWindow ("Save settings as preset", "Name your preset (1 to 60 characters).", juce::MessageBoxIconType::NoIcon);
        window->addTextEditor ("name", suggestion);
        window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        window->enterModalState (true, juce::ModalCallbackFunction::create ([window, onName] (int result)
                                                                           {
                                                                               if (result == 1)
                                                                                   onName (window->getTextEditorContents ("name"));
                                                                           }),
                                 true);
    }

    void importInto (Engine& engine, PluginSlot& slot, std::shared_ptr<juce::FileChooser> chooser)
    {
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [&engine, slotPtr = &slot, chooser] (const juce::FileChooser& fc)
                              {
                                  juce::ignoreUnused (engine);

                                  if (fc.getResult() == juce::File())
                                      return;

                                  const auto id = presets::identityOf (*slotPtr->instance);
                                  const presets::Library library;
                                  const auto result = library.importFile (id, fc.getResult());

                                  if (! result.ok)
                                  {
                                      showPresetProblem (result.message);
                                      return;
                                  }

                                  const auto parsed = library.load (id, result.name);

                                  if (parsed.ok)
                                      loadPresetInto (engine, *slotPtr, parsed.preset);
                              });
    }
}

void showSlotMenu (Engine& engine, PluginWindowManager& windows, Track& track, PluginSlot& slot, juce::Component* target)
{
    const auto it = std::find_if (track.plugins.begin(), track.plugins.end(), [&slot] (auto& p) { return p.get() == &slot; });

    if (it == track.plugins.end())
        return;

    const int index = (int) std::distance (track.plugins.begin(), it);

    juce::PopupMenu menu;
    menu.addSectionHeader (slot.instance->getName());
    menu.addItem (1, "Open editor");
    menu.addItem (2, "Bypass", true, slot.bypassed.load());

    // Your own presets for this plugin: built-ins and hosted plugins alike.
    const auto identity = presets::identityOf (*slot.instance);
    const auto entries = presets::Library().list (identity);
    juce::MemoryBlock currentState;
    {
        const juce::ScopedLock lock (engine.getLock());
        slot.instance->getStateInformation (currentState);
    }

    juce::PopupMenu presetMenu;

    if (! identity.isBuiltIn())
        presetMenu.addSectionHeader ("Saves what the plugin reports as its settings");

    for (size_t i = 0; i < entries.size(); ++i)
        presetMenu.addItem (100 + (int) i, entries[i].readable ? entries[i].name : entries[i].name + " (can't be read)", entries[i].readable);

    if (entries.empty())
        presetMenu.addItem (-1, "No presets of your own yet", false);

    presetMenu.addSeparator();
    presetMenu.addItem (90, currentState.getSize() > 0 ? juce::String ("Save settings as preset...") : juce::String ("This plugin reports no settings to save"),
                        currentState.getSize() > 0);
    presetMenu.addItem (91, "Import preset file...");
    presetMenu.addItem (92, "Export settings...", currentState.getSize() > 0);
    menu.addSubMenu ("Presets", presetMenu);

    menu.addSeparator();
    menu.addItem (3, "Move left", index > 0);
    menu.addItem (4, "Move right", index < (int) track.plugins.size() - 1);
    menu.addSeparator();
    menu.addItem (5, "Remove");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [&engine, &windows, trackPtr = &track, slotPtr = &slot, index, entries, identity, currentState] (int result)
                        {
                            if (result <= 0 || engine.indexOf (trackPtr) < 0 || ! contains (*trackPtr, slotPtr))
                                return;

                            if (result >= 90)
                            {
                                if (result >= 100)
                                {
                                    const auto parsed = presets::Library().load (identity, entries[(size_t) (result - 100)].name);

                                    if (parsed.ok)
                                        loadPresetInto (engine, *slotPtr, parsed.preset);
                                    else
                                        showPresetProblem ("Couldn't load \"" + entries[(size_t) (result - 100)].name + "\": " + parsed.error);
                                }
                                else if (result == 90)
                                {
                                    const presets::Library library;
                                    juce::String suggestion;

                                    for (int n = 1; suggestion.isEmpty(); ++n)
                                    {
                                        const auto candidate = "My " + (slotPtr->instance->getName().startsWith ("DawIt ") ? slotPtr->instance->getName().fromFirstOccurrenceOf ("DawIt ", false, false)
                                                                                                                         : slotPtr->instance->getName()) + " " + juce::String (n);
                                        presets::Entry unused;

                                        if (! library.find (identity, candidate, unused))
                                            suggestion = candidate;
                                    }

                                    askPresetName (suggestion, [&engine, trackPtr, slotPtr] (const juce::String& name)
                                                   {
                                                       if (engine.indexOf (trackPtr) >= 0 && contains (*trackPtr, slotPtr))
                                                           saveSettingsAs (engine, *slotPtr, name, false);
                                                   });
                                }
                                else if (result == 91)
                                {
                                    importInto (engine, *slotPtr, std::make_shared<juce::FileChooser> ("Import a preset file", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.dawitpreset"));
                                }
                                else if (result == 92)
                                {
                                    auto chooser = std::make_shared<juce::FileChooser> ("Export these settings", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                                                                   .getChildFile (presets::tidyName (slotPtr->instance->getName()) + ".dawitpreset"),
                                                                                         "*.dawitpreset");
                                    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                                                          [identity, currentState, chooser, name = slotPtr->instance->getName()] (const juce::FileChooser& fc)
                                                          {
                                                              if (fc.getResult() == juce::File())
                                                                  return;

                                                              const auto written = presets::Library::exportState (identity, name, currentState, fc.getResult().withFileExtension ("dawitpreset"));

                                                              if (! written.ok)
                                                                  showPresetProblem (written.message);
                                                          });
                                }

                                return;
                            }

                            switch (result)
                            {
                                case 1: open (windows, *trackPtr, *slotPtr); break;
                                case 2: engine.setPluginBypassed (*slotPtr, ! slotPtr->bypassed.load()); break;
                                case 3: engine.movePlugin (*trackPtr, slotPtr, index - 1); break;
                                case 4: engine.movePlugin (*trackPtr, slotPtr, index + 1); break;
                                case 5: remove (engine, windows, *trackPtr, *slotPtr); break;
                                default: break;
                            }
                        });
}
} // namespace daw::plugins
