#include "MidiTransformMenu.h"

#include "GuideButton.h"

namespace daw::transformmenu
{
namespace
{
    using miditransform::Kind;

    enum Item
    {
        reverseTime = 9001, reverseKey, reverseExact, randomisePitch, randomiseAgain,
        stayNearBase = 9010,   // + 0 to 3
        avoidRepeats = 9020, chordsTogether, undo, guide
    };

    miditransform::RandomOptions readOptions (Engine& engine)
    {
        miditransform::RandomOptions options;

        if (auto* settings = engine.properties.getUserSettings())
        {
            options.stayNear = juce::jlimit (0, 3, settings->getIntValue ("transform.stayNear", 0));
            options.avoidRepeats = settings->getBoolValue ("transform.avoidRepeats", false);
            options.chordsTogether = settings->getBoolValue ("transform.chordsTogether", true);
        }

        return options;
    }

    void writeSetting (Engine& engine, const char* key, int value)
    {
        if (auto* settings = engine.properties.getUserSettings())
            settings->setValue (key, value);
    }

    int getGuideTopic (Engine& engine, const MidiClip& clip)
    {
        if (! engine.canUndoTransform (clip))
            return guideTopicRetrograde;

        switch (engine.getLastTransformKind())
        {
            case Kind::reverseInTime: return guideTopicRetrograde;
            case Kind::invertInKey:
            case Kind::invertExactly: return guideTopicInversion;
            case Kind::randomise:     return guideTopicNewNotes;
        }

        return guideTopicRetrograde;
    }
}

void build (juce::PopupMenu& menu, Engine& engine, const MidiClip& clip, const juce::SortedSet<int>& selection, bool drumSound)
{
    const auto& notes = clip.notes;
    const double length = clip.lengthBeats;
    const bool any = ! miditransform::getAffected (notes, selection, length).empty();
    const bool canReverseTime = any && ! miditransform::sameNotes (notes, miditransform::reverseInTime (notes, selection, length));
    const bool canReversePitch = ! drumSound && miditransform::countDistinctPitches (notes, selection, length) >= 2;
    const auto options = readOptions (engine);

    menu.addItem (reverseTime, miditransform::getKindName (Kind::reverseInTime), canReverseTime);
    menu.addItem (reverseKey, miditransform::getKindName (Kind::invertInKey), canReversePitch);
    menu.addItem (reverseExact, miditransform::getKindName (Kind::invertExactly), canReversePitch);
    menu.addSeparator();
    menu.addItem (randomisePitch, "Randomise pitch in " + engine.getKey().getName(), any && ! drumSound);
    menu.addItem (randomiseAgain, "Randomise again", engine.canRandomiseAgain (clip));

    juce::PopupMenu optionsMenu;
    optionsMenu.addSectionHeader ("Stay near the old note");
    const char* names[] = { "Anywhere in the range", "1 scale step", "2 scale steps", "3 scale steps" };

    for (int i = 0; i < 4; ++i)
        optionsMenu.addItem (stayNearBase + i, names[i], true, options.stayNear == i);

    optionsMenu.addSeparator();
    optionsMenu.addItem (avoidRepeats, "Avoid repeating the same pitch", true, options.avoidRepeats);
    optionsMenu.addItem (chordsTogether, "Chords move together", true, options.chordsTogether);
    menu.addSubMenu ("Randomise options", optionsMenu);

    menu.addSeparator();
    const bool canUndo = engine.canUndoTransform (clip);
    menu.addItem (undo, canUndo ? "Undo " + miditransform::getKindName (engine.getLastTransformKind()) : juce::String ("Undo transform"), canUndo);
    menu.addSeparator();
    guideicon::addItem (menu, guide, "About reversing and randomising notes...");
}

bool handle (int result, Engine& engine, MidiClip& clip, const juce::SortedSet<int>& selection,
             const std::function<void (int topic)>& onShowGuide)
{
    const auto options = readOptions (engine);

    switch (result)
    {
        case reverseTime:   engine.transformNotes (clip, Kind::reverseInTime, selection); return true;
        case reverseKey:    engine.transformNotes (clip, Kind::invertInKey, selection); return true;
        case reverseExact:  engine.transformNotes (clip, Kind::invertExactly, selection); return true;
        case randomisePitch: engine.transformNotes (clip, Kind::randomise, selection, options); return true;
        case randomiseAgain: engine.randomiseAgain (clip, options); return true;
        case undo:          engine.undoTransform (clip); return true;
        case avoidRepeats:  writeSetting (engine, "transform.avoidRepeats", options.avoidRepeats ? 0 : 1); return true;
        case chordsTogether: writeSetting (engine, "transform.chordsTogether", options.chordsTogether ? 0 : 1); return true;
        case guide:
            if (onShowGuide != nullptr)
                onShowGuide (getGuideTopic (engine, clip));
            return true;
        default: break;
    }

    if (result >= stayNearBase && result < stayNearBase + 4)
    {
        writeSetting (engine, "transform.stayNear", result - stayNearBase);
        return true;
    }

    return false;
}

juce::PopupMenu makeSubMenu (Engine& engine, const MidiClip& clip, const juce::SortedSet<int>& selection, bool drumSound)
{
    juce::PopupMenu menu;
    build (menu, engine, clip, selection, drumSound);
    return menu;
}

void showFor (juce::Component& target, Engine& engine, MidiClip& clip, const juce::SortedSet<int>& selection, bool drumSound,
              std::function<void (int topic)> onShowGuide)
{
    juce::PopupMenu menu;
    build (menu, engine, clip, selection, drumSound);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                        [&engine, &clip, selection, showGuide = std::move (onShowGuide)] (int result)
    {
        // The clip may have been deleted while the menu was open.
        if (result != 0 && engine.findTrackFor (&clip) != nullptr)
            handle (result, engine, clip, selection, showGuide);
    });
}

juce::String getButtonTooltip()
{
    return "Transform the selected notes, or the whole clip when none are selected: reverse in time (the last note plays first), "
           "reverse in pitch (upside down), or new random pitches from the song's key over the same rhythm. Reverse twice to get back.";
}
} // namespace daw::transformmenu
