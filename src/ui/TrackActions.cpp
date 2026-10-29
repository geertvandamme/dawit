#include "GuideButton.h"
#include "TrackActions.h"
#include "BounceActions.h"
#include "MidiExportActions.h"
#include "PluginActions.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInDelay.h"
#include "engine/BuiltInReverb.h"
#include "engine/Routing.h"
#include "generate/SingingGuide.h"

namespace daw
{
namespace
{
    constexpr int createBusItem = 1, removeFromBusItem = 2, foldItem = 3, exportItem = 4, scaleItem = 5, deleteItem = 6,
                  ungroupItem = 7, deleteBusItem = 8, guideItem = 9, deleteReturnItem = 10, signalFlowItem = 11, singingItem = 12, bounceItem = 13, bounceInPlaceItem = 14, monitorItem = 15, recordingGuideItem = 16,
                  outputBase = 100, sendsBase = 1000, inputBase = 20000;

    constexpr float defaultSendDb = -10.0f;

    /** The effect's mix in %, or 100 for anything else. */
    float getMixPercent (const PluginSlot& slot)
    {
        if (auto* reverb = dynamic_cast<BuiltInReverb*> (slot.instance.get()))
            return reverb->getSettings().mixPercent;

        if (auto* delay = dynamic_cast<BuiltInDelay*> (slot.instance.get()))
            return delay->getSettings().mixPercent;

        return 100.0f;
    }

    void setMixPercent (PluginSlot& slot, float mix)
    {
        if (auto* reverb = dynamic_cast<BuiltInReverb*> (slot.instance.get()))
        {
            auto settings = reverb->getSettings();
            settings.mixPercent = mix;
            reverb->setSettings (settings);
        }
        else if (auto* delay = dynamic_cast<BuiltInDelay*> (slot.instance.get()))
        {
            auto settings = delay->getSettings();
            settings.mixPercent = mix;
            delay->setSettings (settings);
        }
    }

    PluginSlot* findEffect (Track& track, const juce::String& identifier)
    {
        for (auto& slot : track.plugins)
            if (slot->instance != nullptr && slot->instance->getName() == identifier)
                return slot.get();

        return nullptr;
    }

    juce::PluginDescription getEffectDescription (const juce::String& identifier)
    {
        return identifier == BuiltInDelay::identifier ? BuiltInDelay::getDescription() : BuiltInReverb::getDescription();
    }

    juce::String describeTargets (const std::vector<Track*>& targets, const Tracks& all)
    {
        if (targets.size() > 1)
            return juce::String ((int) targets.size()) + " tracks";

        auto& t = *targets.front();

        if (t.isReturn())
        {
            const int count = (int) routing::getSenders (t, all).size();
            return t.name + " (return, " + juce::String (count) + (count == 1 ? " send)" : " sends)");
        }

        if (! t.isBus())
            return t.name;

        const int count = (int) std::count_if (all.begin(), all.end(), [&t] (auto& other) { return routing::playsInto (*other, t); });
        return t.name + " (bus, " + juce::String (count) + (count == 1 ? " track)" : " tracks)");
    }

    int countInside (const Track& bus, const Tracks& all)
    {
        return (int) std::count_if (all.begin(), all.end(), [&bus] (auto& other) { return routing::playsInto (*other, bus); });
    }
}

TrackActions::TrackActions (Engine& e, Selection& s, PluginWindowManager& w, juce::Component& parent)
    : engine (e), selection (s), pluginWindows (w), dialogParent (parent)
{
}

bool TrackActions::isAlive (const Track& track) const
{
    return engine.indexOf (&track) >= 0;
}

juce::String TrackActions::getOutputName (const Track& track)
{
    return track.bus != nullptr ? track.bus->name : juce::String ("Master");
}

std::vector<Track*> TrackActions::getTargets (Track& clicked) const
{
    if (selection.isSelected (&clicked))
    {
        // In the order of the rows, not of the clicks.
        std::vector<Track*> targets;

        for (auto& t : engine.getTracks())
            if (selection.isSelected (t.get()))
                targets.push_back (t.get());

        return targets;
    }

    return { &clicked };
}

//==============================================================================
std::vector<Track*> TrackActions::getOutputChoices (const std::vector<Track*>& targets) const
{
    std::vector<Track*> choices;

    for (auto& t : engine.getTracks())
        if (t->isBus() && std::all_of (targets.begin(), targets.end(), [&t] (Track* target) { return routing::canMoveInto (*target, t.get()); }))
            choices.push_back (t.get());

    return choices;
}

juce::PopupMenu TrackActions::buildOutputMenu (const std::vector<Track*>& targets, int firstId) const
{
    const auto choices = getOutputChoices (targets);
    const auto allIn = [&targets] (const Track* bus)
    {
        return std::all_of (targets.begin(), targets.end(), [bus] (Track* t) { return t->bus == bus; });
    };

    juce::PopupMenu menu;
    menu.addItem (firstId, "Master", true, allIn (nullptr));

    if (! choices.empty())
        menu.addSeparator();

    for (size_t i = 0; i < choices.size(); ++i)
        menu.addItem (firstId + 1 + (int) i, juce::String::repeatedString ("    ", routing::getDepth (*choices[i])) + choices[i]->name, true, allIn (choices[i]));

    menu.addSeparator();
    menu.addItem (firstId + 1 + (int) choices.size(), "New bus...");
    return menu;
}

juce::PopupMenu TrackActions::makeOutputMenu (const Track& track, int firstId) const
{
    return buildOutputMenu ({ const_cast<Track*> (&track) }, firstId);
}

void TrackActions::moveOut (Track& track)
{
    if (track.bus == nullptr)
        return;

    // To the top level, right below the bus it was in.
    auto* top = track.bus;

    while (top->bus != nullptr)
        top = top->bus;

    const auto& tracks = engine.getTracks();
    const int next = routing::getBlockEnd (tracks, engine.indexOf (top));
    engine.moveTrack (track, nullptr, next < (int) tracks.size() ? tracks[(size_t) next].get() : nullptr);
}

void TrackActions::applyOutput (const std::vector<Track*>& targets, int firstId, int chosenId)
{
    const auto choices = getOutputChoices (targets);
    const int index = chosenId - firstId;

    if (index < 0 || index > (int) choices.size() + 1)
        return;

    if (index == (int) choices.size() + 1)
    {
        createBus (targets, routingguide::suggestBusName (routing::getOutermost (targets, engine.getTracks()), engine.getTracks()));
        return;
    }

    auto* bus = index == 0 ? nullptr : choices[(size_t) index - 1];

    for (auto* t : targets)
    {
        if (! isAlive (*t) || t->bus == bus)
            continue;

        if (bus == nullptr)
            moveOut (*t);
        else
            engine.moveTrack (*t, bus);
    }
}

void TrackActions::applyOutput (Track& track, int firstId, int chosenId)
{
    applyOutput (std::vector<Track*> { &track }, firstId, chosenId);
}

void TrackActions::showOutputMenu (Track& track, juce::Component* target)
{
    auto menu = makeOutputMenu (track, outputBase);
    menu.addSeparator();
    guideicon::addItem (menu, guideItem, "About buses, sends and routing...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [this, &track, alive = juce::Component::SafePointer<juce::Component> (&dialogParent)] (int result)
                        {
                            if (alive == nullptr || result == 0 || ! isAlive (track))
                                return;

                            if (result == guideItem)
                            {
                                if (onShowGuide != nullptr)
                                    onShowGuide (routingguide::Topic::what);
                            }
                            else
                            {
                                applyOutput (track, outputBase, result);
                            }
                        });
}

//==============================================================================
void TrackActions::showTrackMenu (Track& clicked, juce::Component* target)
{
    const auto targets = getTargets (clicked);
    const auto& all = engine.getTracks();
    const bool single = targets.size() == 1;
    auto& first = *targets.front();
    const bool isBus = single && first.isBus();
    const bool isReturn = single && first.isReturn();
    const bool anyReturn = std::any_of (targets.begin(), targets.end(), [] (Track* t) { return t->isReturn(); });

    juce::PopupMenu menu;
    menu.addSectionHeader (describeTargets (targets, all));

    if (isReturn)
    {
        // A return plays into the master only.
        juce::PopupMenu output;
        output.addItem (outputBase, "Master", false, true);
        menu.addSubMenu ("Output", output);
        menu.addSeparator();
        menu.addItem (deleteReturnItem, "Delete return track...");
        menu.addSeparator();
        menu.addItem (signalFlowItem, "Show in Signal Flow...");
        guideicon::addItem (menu, guideItem, "About buses, sends and routing...");

        juce::PopupMenu::Options returnOptions;

        if (target != nullptr)
            returnOptions = returnOptions.withTargetComponent (target);

        menu.showMenuAsync (returnOptions, [this, targets, alive = juce::Component::SafePointer<juce::Component> (&dialogParent)] (int result)
        {
            if (alive == nullptr || result == 0 || ! isAlive (*targets.front()))
                return;

            if (result == deleteReturnItem && onDelete != nullptr)
                onDelete (targets, true);
            else if (result == signalFlowItem && onShowSignalFlow != nullptr)
                onShowSignalFlow (*targets.front());
            else if (result == guideItem && onShowGuide != nullptr)
                onShowGuide (routingguide::Topic::sends);
        });

        return;
    }

    {
        juce::PopupMenu::Item create (targets.size() > 1 ? "Create bus from " + juce::String ((int) targets.size()) + " tracks" : juce::String ("Create bus"));
        create.setID (createBusItem);
        create.shortcutKeyDescription = juce::String::fromUTF8 ("\xe2\x87\xa7\xe2\x8c\x98G");
        menu.addItem (create);
    }

    menu.addSubMenu ("Output", buildOutputMenu (targets, outputBase));

    if (single && first.bus != nullptr)
        menu.addItem (removeFromBusItem, "Remove from bus \"" + first.bus->name + "\"");

    if (! anyReturn)
        menu.addSubMenu ("Sends", makeSendsMenu (targets, sendsBase));

    menu.addSeparator();

    if (isBus)
        menu.addItem (foldItem, first.folded ? "Show tracks" : "Hide tracks");

    if (single && first.isAudio())
    {
        // Recording: the input an armed track records, and hearing it while you play.
        menu.addSubMenu ("Input", recordingui::makeInputMenu (engine, first, inputBase), ! engine.isRecording());
        menu.addItem (monitorItem, "Monitor input", first.armed.load(), first.monitor.load());
    }

    if (single && first.isMidi())
    {
        const bool hasMidi = std::any_of (first.clips.begin(), first.clips.end(), [] (auto& c) { return c->kind == Clip::Kind::midi; });
        menu.addItem (exportItem, "Export track as MIDI file...", hasMidi);
        menu.addItem (scaleItem, "Show only notes in " + engine.getKey().getName(), true, first.scaleFilter);
    }

    if (! anyReturn)
    {
        menu.addSeparator();
        menu.addItem (bounceItem, "Bounce to WAV...", onBounceToWav != nullptr);

        if (single && (first.isAudio() || first.isMidi()))
        {
            const bool hasClip = std::any_of (first.clips.begin(), first.clips.end(), [] (auto& c) { return ! c->muted; });
            juce::PopupMenu::Item item ("Bounce in place");
            item.setID (bounceInPlaceItem);
            item.setEnabled (hasClip && ! engine.isRecording());
            item.shortcutKeyDescription = juce::String::fromUTF8 ("\xe2\x87\xa7\xe2\x8c\x98" "B");
            menu.addItem (item);
        }
    }

    menu.addSeparator();

    if (isBus)
    {
        menu.addItem (ungroupItem, "Ungroup (keep its tracks)");
        menu.addItem (deleteBusItem, "Delete bus and its " + juce::String (countInside (first, all)) + " tracks...");
    }
    else
    {
        menu.addItem (deleteItem, targets.size() > 1 ? "Delete " + juce::String ((int) targets.size()) + " tracks" : juce::String ("Delete track"));
    }

    menu.addSeparator();
    menu.addItem (signalFlowItem, "Show in Signal Flow...");

    // A vocal track (by its name) offers the singing guide, just above the routing guide.
    if (single && onShowSingingGuide != nullptr && gen::singing::hasVocalWord (first.name))
        guideicon::addItem (menu, singingItem, "Singing techniques...");

    guideicon::addItem (menu, guideItem, "About buses, sends and routing...");

    if (single && first.isAudio())
        guideicon::addItem (menu, recordingGuideItem, "About recording audio...", onShowRecordingGuide != nullptr);

    juce::PopupMenu::Options options;

    if (target != nullptr)
        options = options.withTargetComponent (target);

    menu.showMenuAsync (options, [this, targets, alive = juce::Component::SafePointer<juce::Component> (&dialogParent)] (int result)
    {
        if (alive == nullptr || result == 0)
            return;

        for (auto* t : targets)
            if (! isAlive (*t))
                return;

        auto& primary = *targets.front();

        if (result >= inputBase)
            recordingui::applyInputChoice (engine, primary, inputBase, result);
        else if (result >= sendsBase)
            applySendsChoice (targets, sendsBase, result);
        else if (result >= outputBase)
            applyOutput (targets, outputBase, result);
        else if (result == createBusItem)
            createBus (targets, routingguide::suggestBusName (routing::getOutermost (targets, engine.getTracks()), engine.getTracks()));
        else if (result == removeFromBusItem)
            removeFromBus (primary);
        else if (result == foldItem)
            engine.setFolded (primary, ! primary.folded);
        else if (result == exportItem)
            midiactions::exportTrack (engine, primary);
        else if (result == scaleItem)
            engine.setScaleFilter (primary, ! primary.scaleFilter);
        else if (result == bounceItem && onBounceToWav != nullptr)
            onBounceToWav (targets);
        else if (result == bounceInPlaceItem)
            bounceui::bounceInPlace (engine, selection, primary);
        else if (result == deleteItem && onDelete != nullptr)
            onDelete (targets, true);
        else if (result == ungroupItem)
            ungroup (primary);
        else if (result == deleteBusItem && onDelete != nullptr)
            onDelete (targets, true);
        else if (result == signalFlowItem && onShowSignalFlow != nullptr)
            onShowSignalFlow (primary);
        else if (result == singingItem && onShowSingingGuide != nullptr)
            onShowSingingGuide();
        else if (result == guideItem && onShowGuide != nullptr)
            onShowGuide (routingguide::Topic::what);
        else if (result == monitorItem)
            recordingui::setMonitor (engine, primary, ! primary.monitor.load(), onShowRecordingGuide);
        else if (result == recordingGuideItem && onShowRecordingGuide != nullptr)
            onShowRecordingGuide (recordingguide::Topic::air);
    });
}

//==============================================================================
Track* TrackActions::addReturn (ReturnKind kind, const std::vector<Track*>& sendFrom)
{
    auto* ret = engine.addReturn (kind == ReturnKind::reverb ? "Reverb" : kind == ReturnKind::delay ? "Delay" : "");

    if (kind != ReturnKind::empty)
    {
        juce::String error;
        auto* slot = engine.addPlugin (*ret, kind == ReturnKind::reverb ? BuiltInReverb::getDescription() : BuiltInDelay::getDescription(), error);

        if (slot != nullptr)
            plugins::applyReturnStart (*slot);
    }

    for (auto* t : sendFrom)
        if (isAlive (*t) && ! t->isReturn())
            engine.setSend (*t, *ret, defaultSendDb);

    selection.select (ret);
    return ret;
}

void TrackActions::showNewReturnMenu (juce::Component* target)
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("New return track");
    menu.addItem (1, "Reverb: DawIt Reverb, a plate");
    menu.addItem (2, "Delay: DawIt Delay, quarter-note echoes");
    menu.addItem (3, "Empty");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [this, alive = juce::Component::SafePointer<juce::Component> (&dialogParent)] (int result)
                        {
                            if (alive == nullptr || result == 0)
                                return;

                            addReturn (result == 1 ? ReturnKind::reverb : result == 2 ? ReturnKind::delay : ReturnKind::empty);
                        });
}

juce::PopupMenu TrackActions::makeSendsMenu (const std::vector<Track*>& targets, int firstId) const
{
    juce::PopupMenu menu;
    const auto returns = routing::getReturns (engine.getTracks());

    for (size_t i = 0; i < returns.size(); ++i)
    {
        const bool allOn = ! targets.empty() && std::all_of (targets.begin(), targets.end(), [&] (Track* t) { return routing::isSendOn (*t, *returns[i]); });
        juce::String text = returns[i]->name;

        if (targets.size() == 1 && allOn)
            text << " (" << juce::String (routing::findSend (*targets.front(), *returns[i])->levelDb.load(), 1) << " dB)";

        menu.addItem (firstId + (int) i, text, true, allOn);
    }

    if (! returns.empty())
        menu.addSeparator();

    juce::PopupMenu fresh;
    fresh.addItem (firstId + 100, "Reverb");
    fresh.addItem (firstId + 101, "Delay");
    fresh.addItem (firstId + 102, "Empty");
    menu.addSubMenu ("New return track", fresh);
    return menu;
}

Track* TrackActions::applySendsChoice (const std::vector<Track*>& targets, int firstId, int chosenId)
{
    const int index = chosenId - firstId;

    if (index >= 100 && index <= 102)
    {
        return addReturn (index == 100 ? ReturnKind::reverb : index == 101 ? ReturnKind::delay : ReturnKind::empty, targets);
    }

    const auto returns = routing::getReturns (engine.getTracks());

    if (index < 0 || index >= (int) returns.size())
        return nullptr;

    auto& ret = *returns[(size_t) index];
    const bool allOn = std::all_of (targets.begin(), targets.end(), [&ret] (Track* t) { return routing::isSendOn (*t, ret); });

    for (auto* t : targets)
    {
        if (! isAlive (*t))
            continue;

        if (allOn)
            engine.removeSend (*t, ret);
        else if (! routing::isSendOn (*t, ret))
            engine.setSend (*t, ret, defaultSendDb);
    }

    return allOn ? targets.front() : &ret;
}

void TrackActions::showSendsButtonMenu (Track& from, juce::Component* target)
{
    auto menu = makeSendsMenu ({ &from }, 1);
    menu.addSeparator();
    guideicon::addItem (menu, 1000, "About sends and returns...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [this, fromPtr = &from, alive = juce::Component::SafePointer<juce::Component> (&dialogParent)] (int result)
                        {
                            if (alive != nullptr && result != 0 && isAlive (*fromPtr))
                                sendsButtonChoice (*fromPtr, result);
                        });
}

void TrackActions::sendsButtonChoice (Track& from, int id)
{
    if (id == 1000)
    {
        if (onShowGuide != nullptr)
            onShowGuide (routingguide::Topic::sends);

        return;
    }

    if (auto* show = applySendsChoice ({ &from }, 1, id); show != nullptr && onShowSends != nullptr)
        onShowSends (*show);
}

void TrackActions::showSendMenu (Track& from, Track& returnTrack, juce::Component* target)
{
    auto* send = routing::findSend (from, returnTrack);
    const bool pre = send != nullptr && send->preFader.load();

    juce::PopupMenu menu;
    menu.addSectionHeader (from.name + " " + juce::String::fromUTF8 ("\xe2\x86\x92") + " " + returnTrack.name);
    menu.addItem (1, "Post-fader", true, ! pre);
    menu.addItem (2, "Pre-fader", true, pre);
    menu.addSeparator();
    menu.addItem (3, "Off", send != nullptr && send->isOn());
    menu.addItem (4, "Show " + returnTrack.name + "'s chain");
    menu.addSeparator();
    guideicon::addItem (menu, 5, "About sends and returns...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                        [this, fromPtr = &from, retPtr = &returnTrack, alive = juce::Component::SafePointer<juce::Component> (&dialogParent)] (int result)
                        {
                            if (alive == nullptr || result == 0 || ! isAlive (*fromPtr) || ! isAlive (*retPtr))
                                return;

                            if (result == 1 || result == 2)
                            {
                                if (routing::findSend (*fromPtr, *retPtr) == nullptr)
                                    engine.setSend (*fromPtr, *retPtr, defaultSendDb);

                                if (auto* s = routing::findSend (*fromPtr, *retPtr))
                                    s->preFader = result == 2;

                                engine.sendChangeMessage();
                            }
                            else if (result == 3)
                            {
                                engine.setSend (*fromPtr, *retPtr, sendOffDb);   // Off, keeping its pre- or post-fader choice
                            }
                            else if (result == 4)
                            {
                                selection.select (retPtr);
                            }
                            else if (result == 5 && onShowGuide != nullptr)
                            {
                                onShowGuide (routingguide::Topic::sends);
                            }
                        });
}

void TrackActions::shareOnReturn (const std::vector<Track*>& tracks, const juce::String& identifier, Track* existing)
{
    if (tracks.empty() || (existing != nullptr && ! isAlive (*existing)))
        return;

    for (auto* t : tracks)
        if (! isAlive (*t))
            return;

    auto* first = findEffect (*tracks.front(), identifier);

    if (first == nullptr)
        return;

    Track* ret = existing;

    if (ret == nullptr)
    {
        ret = engine.addReturn (identifier == BuiltInDelay::identifier ? "Delay" : "Reverb");
        juce::String error;
        auto* slot = engine.addPlugin (*ret, getEffectDescription (identifier), error);

        if (slot == nullptr)
        {
            engine.removeTrack (ret);
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't add the effect", error);
            return;
        }

        // The first track's settings, at 100% wet.
        juce::MemoryBlock state;
        first->instance->getStateInformation (state);
        slot->instance->setStateInformation (state.getData(), (int) state.getSize());
        setMixPercent (*slot, 100.0f);
    }

    for (auto* t : tracks)
    {
        auto* own = findEffect (*t, identifier);

        if (own == nullptr)
            continue;

        engine.setSend (*t, *ret, routingguide::getSendDbForMix (getMixPercent (*own)));
        engine.setPluginBypassed (*own, true);
    }

    selection.select (ret);
}

void TrackActions::addReturnEffect (Track& returnTrack, const juce::String& identifier)
{
    if (! returnTrack.isReturn() || ! isAlive (returnTrack))
        return;

    plugins::add (engine, pluginWindows, returnTrack, getEffectDescription (identifier));
}

void TrackActions::makeReturnWet (Track& returnTrack, const juce::String& identifier)
{
    if (! isAlive (returnTrack))
        return;

    if (auto* slot = findEffect (returnTrack, identifier))
    {
        setMixPercent (*slot, 100.0f);
        engine.sendChangeMessage();
    }
}

void TrackActions::deleteReturn (Track& returnTrack)
{
    if (onDelete != nullptr && isAlive (returnTrack))
        onDelete ({ &returnTrack }, true);
}

//==============================================================================
bool TrackActions::canCreateBus() const
{
    return ! selection.getTracks().empty();
}

Track* TrackActions::createBus()
{
    const auto selected = selection.getTracks();

    if (selected.empty())
        return nullptr;

    return createBus (selected, routingguide::suggestBusName (routing::getOutermost (selected, engine.getTracks()), engine.getTracks()));
}

void TrackActions::moveIntoBus (const std::vector<Track*>& tracks, Track& bus)
{
    if (engine.indexOf (&bus) < 0)
        return;

    for (auto* t : tracks)
        if (engine.indexOf (t) >= 0 && t->bus != &bus)
            engine.moveTrack (*t, &bus);

    selection.select (&bus);
}

Track* TrackActions::createBus (const std::vector<Track*>& tracks, const juce::String& name, Track* beside)
{
    auto* bus = beside != nullptr && engine.indexOf (beside) >= 0 ? engine.createBusBeside (tracks, *beside, name) : engine.createBus (tracks, name);

    if (bus != nullptr)
    {
        selection.select (bus);

        if (onEditName != nullptr)
            onEditName (*bus);
    }

    return bus;
}

Track* TrackActions::addEmptyBus()
{
    const int index = selection.getTrack() != nullptr ? engine.indexOf (selection.getTrack()) + 1 : -1;
    auto* bus = engine.addBus (index > 0 ? index : -1);
    selection.select (bus);

    if (onEditName != nullptr)
        onEditName (*bus);

    return bus;
}

void TrackActions::removeFromBus (Track& track)
{
    engine.removeFromBus (track);
}

void TrackActions::ungroup (Track& bus)
{
    if (! bus.isBus() || ! isAlive (bus))
        return;

    const auto& all = engine.getTracks();
    const int numTracks = (int) routing::getMembers (bus, all).size();
    const auto where = bus.bus != nullptr ? bus.bus->name : juce::String ("master");

    auto remove = [this, alive = juce::Component::SafePointer<juce::Component> (&dialogParent), &bus]
    {
        if (alive == nullptr || ! isAlive (bus))
            return;

        for (auto& slot : bus.plugins)
            pluginWindows.closeFor (*slot->instance);

        const auto members = routing::getMembers (bus, engine.getTracks());
        selection.forget (&bus);
        engine.ungroup (bus);

        if (! members.empty())
            selection.select (members.front());
    };

    if (bus.plugins.empty())
    {
        juce::MessageManager::callAsync (remove);
        return;
    }

    const int effects = (int) bus.plugins.size();
    auto options = juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon, "Remove bus",
                                                                 "Remove the bus " + bus.name + " and its " + juce::String (effects)
                                                                     + (effects == 1 ? " effect? " : " effects? ") + "Its "
                                                                     + juce::String (numTracks) + (numTracks == 1 ? " track stays and plays into the " : " tracks stay and play into the ")
                                                                     + where + ".",
                                                                 "Remove", "Cancel", &dialogParent);
    juce::AlertWindow::showAsync (options, [remove] (int result)
    {
        if (result == 1)
            remove();
    });
}

void TrackActions::addGlueCompressor (Track& bus)
{
    if (! bus.isBus() || ! isAlive (bus))
        return;

    juce::String error;
    auto* slot = engine.addPlugin (bus, BuiltInCompressor::getDescription(), error);
    auto* compressor = slot != nullptr ? dynamic_cast<BuiltInCompressor*> (slot->instance.get()) : nullptr;

    if (compressor == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't add the compressor", error);
        return;
    }

    for (auto& preset : comp::getPresets())
        if (preset.name == "Drum bus")
            compressor->setSettings (preset.settings);

    plugins::open (pluginWindows, bus, *slot);
}
} // namespace daw
