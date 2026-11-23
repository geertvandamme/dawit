#include "DescribeController.h"

namespace daw
{
using namespace fxui;

DescribeController::DescribeController (juce::Component& o, DescribeConfig c, bool allowBar)
    : owner (o), config (std::move (c))
{
    if (! allowBar || barFactory == nullptr)
        return;

    bar = barFactory (*this, config);
    owner.addChildComponent (*bar);

    button.setWantsKeyboardFocus (false);
    button.setColour (juce::TextButton::buttonColourId, config.colour.withAlpha (0.35f));
    button.setTooltip (config.describeTip);
    button.onClick = [this] { setDescribeBarOpen (! open); };
    setTopic (button, config.topic);
    owner.addAndMakeVisible (button);

    guideLink.setTooltip ("Opens the guide at \"Describe a sound to the AI\": what the AI does, what DawIt checks and the walkthrough.");
    guideLink.setColour (juce::TextButton::buttonColourId, theme::panel);
    guideLink.setColour (juce::TextButton::textColourOffId, config.colour);
    guideLink.onClick = [this] { openDescribeGuide(); };
    setTopic (guideLink, config.topic);
    owner.addChildComponent (guideLink);
}

void DescribeController::setDescribeBarOpen (bool shouldOpen)
{
    if (bar == nullptr || shouldOpen == open)
        return;

    open = shouldOpen;
    bar->setVisible (open);
    button.setToggleState (open, juce::dontSendNotification);
    owner.resized();

    if (open)
    {
        bar->opened();
    }
    else
    {
        bar->closed();
        setDescribeHighlight ({});
        setupSteps.clear();
    }
}

void DescribeController::setDescribeHighlight (const std::vector<int>& knobs)
{
    highlight = knobs;

    if (config.onHighlight != nullptr)
        config.onHighlight (highlight);

    owner.repaint();
}

void DescribeController::showDescription (const juce::StringArray& steps)
{
    setupSteps = steps;
    config.showTopic (config.topic);
}

void DescribeController::openDescribeGuide()
{
    config.openGuide();
}

void DescribeController::layout (juce::Button& guideButton, juce::Rectangle<int> explainerBounds)
{
    if (bar == nullptr)
        return;

    // Describe a sound sits right of the starting points and Save..., and pushes the guide button along.
    const auto guide = guideButton.getBounds();
    button.setBounds (guide.withWidth (140));
    guideButton.setBounds (guide.translated (150, 0));
    guideLink.setBounds (explainerBounds.removeFromBottom (30).reduced (14, 4));
}

void DescribeController::layoutBar (juce::Rectangle<int>& pictures)
{
    if (bar != nullptr && open)
    {
        bar->setBounds (pictures.removeFromTop (DescribeBarBase::barHeight));
        pictures.removeFromTop (8);
    }
}

void DescribeController::tick (int currentTopic)
{
    if (bar == nullptr)
        return;

    if (open)
        bar->tick();

    guideLink.setVisible (currentTopic == config.topic && setupSteps.isEmpty());
}

void DescribeController::paintRing (juce::Graphics& g) const
{
    if (highlight.empty() || config.getComponent == nullptr)
        return;

    std::vector<juce::Component*> done;

    for (int knob : highlight)
    {
        auto* cell = config.getComponent (knob);

        if (cell == nullptr || ! cell->isVisible() || std::find (done.begin(), done.end(), cell) != done.end())
            continue;

        done.push_back (cell);
        const auto area = owner.getLocalArea (cell, cell->getLocalBounds()).toFloat().expanded (1.0f);
        g.setColour (theme::accent.withAlpha (0.22f));
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (theme::accent);
        g.drawRoundedRectangle (area.reduced (1.0f), 6.0f, 2.5f);
    }
}

void DescribeController::buildText (juce::AttributedString& text) const
{
    namespace sd = gen::sounddescribe;
    const auto& d = *config.last;
    const auto& table = *config.table;
    const auto arrow = juce::String::fromUTF8 (" \xe2\x86\x92 ");

    if (! setupSteps.isEmpty())
    {
        addTitle (text, "Set up Ollama", config.colour);
        addLead (text, "Describe a sound asks a language model that runs on your own computer. Nothing you type leaves it.");

        for (auto& line : setupSteps)
            addSection (text, {}, line);

        return;
    }

    if (d.isEmpty())
    {
        addTitle (text, "Describe a sound", config.colour);
        addLead (text, "Type what you want to hear, such as \"" + config.exampleWords + "\". An AI model on your computer picks the nearest "
                       "starting point and suggests only the " + config.whatItSets + " to change, each with a reason.");
        addSection (text, "What DawIt checks", "A language model writes likely text and doesn't hear sound, so DawIt reads every value itself, "
                                               "keeps it inside each knob's range and tells you when the result doesn't match your words.");
        addSection (text, "Applying it", "Nothing changes until you choose: Walk me through it turns the changes into a lesson, " + config.stepUnit
                                         + "; Set it all now changes " + config.allUnit + ". Undo in the bar takes it back.");
        return;
    }

    addTitle (text, "\"" + d.words + "\"", config.colour);

    if (d.summary.isNotEmpty())
        addLead (text, d.summary);

    addSection (text, {}, "Starting from: " + (d.followUp || d.base == sd::currentSoundName ? juce::String ("the sound in the window") : d.base));
    addSection (text, {}, "Reasons written by the AI model; " + config.hoverHint);

    std::vector<int> groupsDone;

    for (auto& c : d.changes)
    {
        const int group = table.knobs[(size_t) c.knob].group;

        if (group < 0)
        {
            addSection (text, (config.getTitle != nullptr ? config.getTitle (c.knob) : table.knobs[(size_t) c.knob].title) + ": " + sd::formatKnobValue (table, c.knob, c.before) + arrow
                                  + sd::formatKnobValue (table, c.knob, c.after), c.reason);

            if (c.note.isNotEmpty())
                addSection (text, "DawIt's note", c.note);

            continue;
        }

        if (std::find (groupsDone.begin(), groupsDone.end(), group) != groupsDone.end())
            continue;

        groupsDone.push_back (group);
        juce::StringArray moves, reasons;

        for (auto& m : d.changes)
        {
            if (table.knobs[(size_t) m.knob].group != group)
                continue;

            const auto label = table.memberLabel != nullptr ? table.memberLabel (m.knob) : table.knobs[(size_t) m.knob].title;
            moves.add (label + " " + sd::formatKnobValue (table, m.knob, m.before) + arrow + sd::formatKnobValue (table, m.knob, m.after));
            reasons.add (label + ": " + m.reason);
        }

        const auto title = juce::isPositiveAndBelow (group, table.groupTitles.size()) ? table.groupTitles[group] : juce::String ("Group");
        addSection (text, title + ": " + moves.joinIntoString (", "), reasons.joinIntoString ("\n"));

        for (auto& m : d.changes)
            if (table.knobs[(size_t) m.knob].group == group && m.note.isNotEmpty())
                addSection (text, "DawIt's note", m.note);
    }

    for (auto& note : d.notes)   // a clamp note is already shown under its change
        if (std::none_of (d.changes.begin(), d.changes.end(), [&note] (const auto& c) { return c.note == note; }))
            addSection (text, "DawIt's note", note);
}
} // namespace daw
