#include "TimeSignatureUi.h"

#include "GuideButton.h"
#include "engine/Tempo.h"
#include "generate/RhythmGuide.h"

#include <algorithm>

namespace daw::timesigui
{
juce::String formatBeats (double beats)
{
    auto text = juce::String (beats, 2);

    while (text.endsWithChar ('0'))
        text = text.dropLastCharacters (1);

    return text.endsWithChar ('.') ? text.dropLastCharacters (1) : text;
}

namespace
{
    /** The dotted note a compound metre feels: 6/8 a dotted quarter, 6/4 a dotted half, 6/16 a dotted eighth. */
    juce::String dottedNoteName (daw::TimeSignature sig)
    {
        switch (sig.denominator)
        {
            case 2:  return "dotted whole note";
            case 4:  return "dotted half note";
            case 16: return "dotted eighth note";
            default: return "dotted quarter note";
        }
    }

    juce::String getHint (daw::TimeSignature sig)
    {
        if (sig == daw::TimeSignature { 2, 4 })  return "two beats: marches and polkas";
        if (sig == daw::TimeSignature { 3, 4 })  return "three beats: waltzes";
        if (sig == daw::TimeSignature { 4, 4 })  return "four beats: most pop and rock";
        if (sig == daw::TimeSignature { 5, 4 })  return "five beats: 3 + 2 or 2 + 3";
        if (sig == daw::TimeSignature { 6, 8 })  return "two beats of three";
        if (sig == daw::TimeSignature { 7, 8 })  return "2+2+3";
        if (sig == daw::TimeSignature { 9, 8 })  return "three beats of three";
        if (sig == daw::TimeSignature { 12, 8 }) return "four beats of three: slow blues";
        if (sig == daw::TimeSignature { 2, 2 })  return "cut time: two half notes";
        return {};
    }

    /** The first sentence of a tooltip: "Time signature 6/8: two beats in each bar, ...". */
    juce::String getName (daw::TimeSignature sig)
    {
        return "Time signature " + timesig::toString (sig) + ": " + timesig::describe (sig) + ". ";
    }
} // namespace

Line getLineKind (daw::TimeSignature sig, double beat)
{
    constexpr double eps = 1.0e-6;
    const double offset = beat - timesig::getBarStart (sig, timesig::getBarAt (sig, beat + eps));

    if (offset < eps)
        return Line::bar;

    for (auto start : timesig::getGroupStarts (sig))
        if (std::abs (offset - start) < eps)
            return Line::beat;

    const double step = getLineStep (sig);
    const double steps = offset / step;

    if (step < timesig::getPulseLength (sig) + eps && (sig.denominator == 2 || timesig::hasLongGroups (sig)) && std::abs (steps - std::round (steps)) < eps)
        return Line::pulse;

    return Line::other;
}

double getLineStep (daw::TimeSignature sig)
{
    if (timesig::hasLongGroups (sig))
        return juce::jmin (1.0, timesig::getPulseLength (sig));

    return sig.denominator == 16 ? timesig::getPulseLength (sig) : 1.0;
}

juce::String getButtonTooltip (const Engine& engine)
{
    const auto sig = engine.getTimeSignature();
    auto text = getName (sig) + "Click to choose another. The tempo always counts quarter notes";

    if (timesig::getKind (sig) == timesig::Kind::compound)
        text << "; you feel the " << dottedNoteName (sig) << " at " << formatBpm (timesig::getFeltBpm (sig, engine.getTempo())) << " a minute";

    return text + ".";
}

juce::String getMarkTooltip (daw::TimeSignature sig)
{
    return getName (sig) + "Click to change it.";
}

juce::String getMetronomeSentence (daw::TimeSignature sig)
{
    return "In " + timesig::toString (sig) + " it clicks " + juce::String (sig.numerator) + (sig.numerator == 1 ? " time" : " times") + " a bar: "
           + gen::rhythm::describeClicks (sig) + ".";
}

bool hasPartsToMove (const Engine& engine)
{
    for (const auto& track : engine.getTracks())
        if (track != nullptr && ! track->clips.empty())
            return true;

    return ! engine.getChords().empty() || ! engine.getSections().empty() || ! engine.getLyrics().isEmpty()
           || engine.getTempoMap().getChanges().size() > 1;
}

void addItems (juce::PopupMenu& menu, Engine& engine, juce::Component* owner)
{
    const bool allowed = ! engine.isRecording();
    const auto current = engine.getTimeSignature();

    for (auto sig : timesig::getCommon())
    {
        const auto hint = getHint (sig);
        menu.addItem (timesig::toString (sig) + (hint.isEmpty() ? juce::String() : "   " + hint), allowed, sig == current,
                      [&engine, sig, safe = juce::Component::SafePointer<juce::Component> (owner)]
                      {
                          choose (engine, sig, safe.getComponent());
                      });
    }

    menu.addItem ("Other...", allowed, std::find (timesig::getCommon().begin(), timesig::getCommon().end(), current) == timesig::getCommon().end(),
                  [&engine, safe = juce::Component::SafePointer<juce::Component> (owner)] { showOtherDialog (engine, safe.getComponent()); });
}

void showMenu (Engine& engine, juce::Component& target, std::function<void()> onShowGuide, bool atMouse)
{
    juce::PopupMenu menu;
    addItems (menu, engine, &target);

    if (onShowGuide != nullptr)
    {
        menu.addSeparator();
        guideicon::addItem (menu, "Time signatures guide...", std::move (onShowGuide));
    }

    auto options = juce::PopupMenu::Options().withTargetComponent (target);

    if (atMouse)
        options = options.withMousePosition();

    menu.showMenuAsync (options);
}

void choose (Engine& engine, daw::TimeSignature sig, juce::Component* owner)
{
    sig = timesig::tidy (sig);

    if (sig == engine.getTimeSignature() || engine.isRecording())
        return;

    if (! hasPartsToMove (engine))
    {
        engine.changeTimeSignature (sig, false);
        return;
    }

    const auto old = engine.getTimeSignature();
    const auto text = "A bar of " + timesig::toString (sig) + " lasts " + formatBeats (timesig::getBarLength (sig)) + " beats instead of "
                      + formatBeats (timesig::getBarLength (old)) + ".\n\n"
                      "Keep each part in its bar: clips, lyrics and tempo changes move with their bars, and chords and sections stay on theirs. "
                      "The notes inside a clip keep their rhythm, so a " + timesig::toString (old) + " pattern now runs across the bar lines, "
                      "and notes past a clip's new end are hidden, not deleted.\n\n"
                      "Keep the timing: everything stays where it sounds and the bar lines move under it, so chords and sections move with the bars.\n\n"
                      "Undo (Cmd+Z) puts it back.";
    auto options = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon)
                       .withTitle ("Change to " + timesig::toString (sig) + "?")
                       .withMessage (text)
                       .withButton ("Keep each part in its bar")
                       .withButton ("Keep the timing")
                       .withButton ("Cancel")
                       .withAssociatedComponent (owner);
    // Three buttons answer 1, 2 and 0 (the last).
    juce::AlertWindow::showAsync (options, [&engine, sig] (int result)
    {
        if (result == 1 || result == 2)
            engine.changeTimeSignature (sig, result == 1);
    });
}

void showOtherDialog (Engine& engine, juce::Component* owner)
{
    auto* window = new juce::AlertWindow ("Other time signature",
                                          "Top: how many notes fill a bar, 1 to 15.\nBottom: which note is counted: 2 half, 4 quarter, 8 eighth, 16 sixteenth.",
                                          juce::MessageBoxIconType::QuestionIcon, owner);
    juce::StringArray tops;

    for (int n = timesig::minNumerator; n <= timesig::maxNumerator; ++n)
        tops.add (juce::String (n));

    window->addComboBox ("top", tops, "Top");
    window->addComboBox ("bottom", { "2", "4", "8", "16" }, "Bottom");
    const auto current = engine.getTimeSignature();

    if (auto* top = window->getComboBoxComponent ("top"))
    {
        top->setSelectedItemIndex (juce::jlimit (0, tops.size() - 1, current.numerator - timesig::minNumerator), juce::dontSendNotification);
        top->setTooltip ("Top: how many notes fill a bar, 1 to 15.");
    }

    if (auto* bottom = window->getComboBoxComponent ("bottom"))
    {
        bottom->setSelectedItemIndex (1, juce::dontSendNotification);

        for (int i = 0; i < bottom->getNumItems(); ++i)
            if (bottom->getItemText (i).getIntValue() == current.denominator)
                bottom->setSelectedItemIndex (i, juce::dontSendNotification);

        bottom->setTooltip ("Bottom: which note is counted: 2 half, 4 quarter, 8 eighth, 16 sixteenth.");
    }

    window->addButton ("Change", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    window->enterModalState (true, juce::ModalCallbackFunction::create ([&engine, window, safe = juce::Component::SafePointer<juce::Component> (owner)] (int result)
    {
        if (result != 1)
            return;

        const int top = window->getComboBoxComponent ("top")->getText().getIntValue();
        const int bottom = window->getComboBoxComponent ("bottom")->getText().getIntValue();
        choose (engine, { top, bottom }, safe.getComponent());
    }), true);
}
} // namespace daw::timesigui
