#include "PluginActions.h"
#include "BuiltInSynthEditor.h"
#include "engine/BuiltInDelay.h"
#include "engine/BuiltInFormat.h"
#include "engine/BuiltInReverb.h"

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
    menu.addSectionHeader ("Built-in");

    if (instruments)
        menu.addItem (builtInSynthItem, "DawIt Synth  -  " + juce::String ((int) builtInSounds::all.size()) + " ready-made sounds, no knobs",
                      true, track.usesBuiltInSynth());

    for (auto& d : instruments ? BuiltInFormat::getInstruments() : BuiltInFormat::getEffects())
    {
        choices.add (d);
        menu.addItem (choices.size(), d.name + "  -  explains itself as you use it");
    }

    menu.addSectionHeader ("Plugins");

    const int numBuiltIn = choices.size();
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
    menu.addSeparator();
    menu.addItem (3, "Move left", index > 0);
    menu.addItem (4, "Move right", index < (int) track.plugins.size() - 1);
    menu.addSeparator();
    menu.addItem (5, "Remove");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [&engine, &windows, trackPtr = &track, slotPtr = &slot, index] (int result)
                        {
                            if (result <= 0 || engine.indexOf (trackPtr) < 0 || ! contains (*trackPtr, slotPtr))
                                return;

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
