#include "FirstSongExercise.h"

namespace daw
{
namespace firstsong = gen::firstsong;

namespace
{
constexpr int rowHeight = 28, margin = 12, footerHeight = 30;

juce::TextLayout makeLayout (const juce::String& text, float size, juce::Colour colour, float width)
{
    juce::AttributedString s;
    s.append (text, juce::FontOptions (size), colour);
    s.setLineSpacing (2.0f);
    s.setWordWrap (juce::AttributedString::byWord);
    juce::TextLayout l;
    l.createLayout (s, width);
    return l;
}
} // namespace

//==============================================================================
/** One step's row: a tick, a dot or an empty circle, its number and its title. Click or Return opens it. */
class FirstSongExercise::StepRow final : public juce::Button
{
public:
    StepRow (int stepIndex, const juce::String& titleText, const juce::String& subtitleText)
        : juce::Button (titleText), index (stepIndex), title (titleText)
    {
        setTooltip (subtitleText + ". Opens step " + juce::String (stepIndex + 1) + ".");
        setTitle ("Step " + juce::String (stepIndex + 1) + ": " + titleText);
        setDescription (subtitleText);
        setWantsKeyboardFocus (true);
    }

    void setState (bool isDone, bool isOpen)
    {
        if (isDone != done || isOpen != open)
        {
            done = isDone;
            open = isOpen;
            repaint();
        }
    }

    void paintButton (juce::Graphics& g, bool isMouseOver, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.0f, 1.0f);
        g.setColour (open ? theme::panelLight : isMouseOver ? theme::panelLight.withAlpha (0.5f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (r, 5.0f);

        if (hasKeyboardFocus (false))
        {
            g.setColour (theme::accent);
            g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.5f);
        }

        auto marker = r.removeFromLeft (28.0f);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));

        if (done)
        {
            g.setColour (theme::meterGreen);
            g.drawText (juce::String::fromUTF8 ("\xe2\x9c\x93"), marker, juce::Justification::centred);
        }
        else
        {
            g.setColour (open ? theme::accent : theme::textDim);
            g.drawText (open ? juce::String::fromUTF8 ("\xe2\x97\x8f") : juce::String::fromUTF8 ("\xe2\x97\x8b"), marker, juce::Justification::centred);
        }

        g.setColour (open ? theme::text : done ? theme::textDim : theme::text);
        g.setFont (juce::FontOptions (14.0f, open ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (juce::String (index + 1) + "  " + title, r.toNearestInt().reduced (2, 0), juce::Justification::centredLeft, 1, 0.8f);
    }

private:
    int index;
    juce::String title;
    bool done = false, open = false;
};

//==============================================================================
/** The open step's text and buttons, inside the viewport. */
class FirstSongExercise::Body final : public juce::Component
{
public:
    using Content = BodyContent;

    Body()
    {
        showMe.setButtonText ("Show me");
        showMe.setTooltip ("Points at the control this step is about");
        addAndMakeVisible (action);
        addAndMakeVisible (showMe);
        addChildComponent (link);
        action.setColour (juce::TextButton::buttonColourId, theme::accent.darker (0.3f));
    }

    /** Lays the text out for a width and returns the height the body needs. */
    int layout (int w, const Content& c)
    {
        content = c;
        const float tw = (float) w - 4.0f;
        float y = 2.0f;

        subtitleLayout = makeLayout (c.subtitle, 13.0f, theme::accent, tw);
        subtitleArea = { 2.0f, y, tw, subtitleLayout.getHeight() };
        y += subtitleLayout.getHeight() + 6.0f;

        doLayout = makeLayout (c.doText, 12.5f, theme::text, tw);
        doArea = { 2.0f, y, tw, doLayout.getHeight() };
        y += doLayout.getHeight() + 8.0f;

        whyLayout = makeLayout ("Why: " + c.why, 12.5f, theme::textDim, tw);
        whyArea = { 2.0f, y, tw, whyLayout.getHeight() };
        y += whyLayout.getHeight() + 8.0f;

        statusArea = {};

        if (c.status.isNotEmpty())
        {
            statusLayout = makeLayout (c.status, 12.5f, theme::meterYellow, tw);
            statusArea = { 2.0f, y, tw, statusLayout.getHeight() };
            y += statusLayout.getHeight() + 8.0f;
        }

        const int showMeWidth = c.hasShowMe ? 84 : 0;
        const int actionWidth = juce::jmin (w - 4 - (showMeWidth > 0 ? showMeWidth + 6 : 0),
                                            (int) juce::GlyphArrangement::getStringWidth (juce::FontOptions (14.0f), c.actionLabel) + 30);
        action.setButtonText (c.actionLabel);
        action.setTooltip (c.actionTip);
        action.setEnabled (c.actionEnabled);
        action.setBounds (2, (int) y, actionWidth, 28);
        showMe.setVisible (c.hasShowMe);
        showMe.setBounds (2 + actionWidth + 6, (int) y, showMeWidth, 28);
        y += 34.0f;

        link.setVisible (c.hasLink);

        if (c.hasLink)
        {
            link.setButtonText (c.linkText);
            link.setTooltip ("Opens this page of the guide");
            link.setBounds (2, (int) y, juce::jmin (w - 4, link.getBestWidthForHeight (24) + 8
                                                           + (int) juce::GlyphArrangement::getStringWidth (juce::FontOptions (14.0f), c.linkText)), 24);
            y += 28.0f;
        }

        setSize (w, (int) std::ceil (y) + 4);
        return getHeight();
    }

    void paint (juce::Graphics& g) override
    {
        subtitleLayout.draw (g, subtitleArea);
        doLayout.draw (g, doArea);
        whyLayout.draw (g, whyArea);

        if (! statusArea.isEmpty())
            statusLayout.draw (g, statusArea);
    }

    juce::TextButton action;
    juce::TextButton showMe;
    GuideButton link;

private:
    Content content;
    juce::TextLayout subtitleLayout, doLayout, whyLayout, statusLayout;
    juce::Rectangle<float> subtitleArea, doArea, whyArea, statusArea;
};

//==============================================================================
FirstSongExercise::FirstSongExercise (Engine& e, Selection& s, ChordMap& chordBaseline)
    : engine (e), selection (s), baseline (chordBaseline)
{
    setWantsKeyboardFocus (true);

    auto& settings = *engine.properties.getUserSettings();
    step = firstsong::clampStep (settings.getIntValue (firstsong::stepSettingKey, 0));
    finished = firstsong::readDone (juce::var (settings.getBoolValue (firstsong::doneSettingKey, false)));

    const auto steps = firstsong::getSteps();

    for (int i = 0; i < firstsong::numSteps; ++i)
    {
        rows[i] = std::make_unique<StepRow> (i, steps[(size_t) i].title, steps[(size_t) i].subtitle);
        rows[i]->onClick = [this, i] { setStep (i); };
        addAndMakeVisible (*rows[i]);
    }

    body = std::make_unique<Body>();
    body->action.onClick = [this] { if (onAction != nullptr) onAction (step); };
    body->showMe.onClick = [this] { if (onShowMe != nullptr) onShowMe (step); };
    body->link.onClick = [this]
    {
        const auto all = firstsong::getSteps();

        if (onOpenLink != nullptr && all[(size_t) step].hasLink)
            onOpenLink (all[(size_t) step].link);
    };
    viewport.setViewedComponent (body.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    backButton.setTooltip ("Go back to the step before");
    backButton.onClick = [this] { setStep (step - 1); };
    skipButton.setTooltip ("Move on to the next step without doing this one");
    skipButton.onClick = [this] { setStep (step + 1); };
    nextButton.setTooltip ("Move on to the next step. It lights up once this step's tick is there.");
    nextButton.onClick = [this] { setStep (step + 1); };
    nextButton.setColour (juce::TextButton::buttonOnColourId, theme::accent);
    carryOnButton.setTooltip ("Opens Theory Guides at Start here: the guides in an order that builds up");
    carryOnButton.onClick = [this] { if (onCarryOn != nullptr) onCarryOn(); };
    carryOnButton.setColour (juce::TextButton::buttonColourId, theme::accent.darker (0.3f));
    startOverButton.setTooltip ("Back to step 1. Your song stays as it is.");
    startOverButton.onClick = [this] { startOver(); };

    for (auto* b : { &backButton, &skipButton, &nextButton, &carryOnButton, &startOverButton })
        addChildComponent (*b);

    engine.addChangeListener (this);
    selection.addChangeListener (this);
    startTimerHz (2);
    setSize (windowWidth, windowHeight);   // last: resized() needs the rows and the body
    refresh();
}

FirstSongExercise::~FirstSongExercise()
{
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
}

firstsong::SavedState FirstSongExercise::getSaved() const
{
    return getSavedState != nullptr ? getSavedState() : firstsong::SavedState();
}

bool FirstSongExercise::isDone (int index) const
{
    return firstsong::isStepDone (index, engine, baseline, getSaved());
}

int FirstSongExercise::getDoneCount() const noexcept
{
    int n = 0;

    for (bool d : done)
        n += d ? 1 : 0;

    return n;
}

juce::TextButton& FirstSongExercise::getActionButton() noexcept
{
    return body->action;
}

juce::String FirstSongExercise::getMidiTrackName() const
{
    const auto* track = firstsong::findMidiTrack (engine);
    return track != nullptr ? track->name : juce::String();
}

bool FirstSongExercise::hasDrumTrack() const
{
    for (auto& t : engine.getTracks())
        if (firstsong::isDrumTrack (*t))
            return true;

    return false;
}

void FirstSongExercise::saveSettings()
{
    auto& settings = *engine.properties.getUserSettings();
    settings.setValue (firstsong::stepSettingKey, step);
    settings.setValue (firstsong::doneSettingKey, finished);
    settings.saveIfNeeded();
}

void FirstSongExercise::setStep (int newStep)
{
    newStep = juce::jlimit (0, firstsong::numSteps - 1, newStep);

    if (newStep == step)
        return;

    step = newStep;
    saveSettings();
    bodySignature = {};
    refresh();
}

void FirstSongExercise::startOver()
{
    finished = false;
    step = 0;

    if (onStartOver != nullptr)
        onStartOver();

    saveSettings();
    bodySignature = {};
    refresh();
}

void FirstSongExercise::refresh()
{
    for (int i = 0; i < firstsong::numSteps; ++i)
        done[i] = isDone (i);

    // Ticked together, once: remembered, and the list gives way to the completion text.
    if (! finished && getDoneCount() == firstsong::numSteps)
    {
        finished = true;
        saveSettings();
    }

    const bool chordsEmpty = engine.getChords().empty();
    const bool drums = hasDrumTrack();
    const auto midiName = getMidiTrackName();
    const auto saved = getSaved();

    BodyContent c;
    const auto steps = firstsong::getSteps();
    const auto& s = steps[(size_t) step];
    c.subtitle = s.subtitle;
    c.doText = firstsong::getDoText (step, engine.getKey(), midiName);
    c.why = s.why;
    c.actionLabel = firstsong::getActionLabel (step, midiName, chordsEmpty, drums);
    c.actionTip = firstsong::getActionTooltip (step, chordsEmpty, drums);
    c.hasShowMe = step != 3;
    c.hasLink = s.hasLink;
    c.linkText = s.hasLink ? s.link.topic : juce::String();

    if (step == 2 && drums)
    {
        const auto* clip = selection.getMidiClip();
        const auto* track = selection.getTrack();
        const bool ok = clip != nullptr && track != nullptr && firstsong::isDrumTrack (*track) && firstsong::canWriteBeat (engine, *clip);
        c.actionEnabled = ok;

        if (! ok)
            c.actionTip = clip != nullptr && track != nullptr && firstsong::isDrumTrack (*track) ? "Only for an empty drum clip in 4/4"
                                                                                                : "Select the drum clip first";
    }

    if (step == 3 && saved.hasFile && saved.unsaved)
        c.status = "Changed since you saved: press Save again.";

    const auto signature = juce::String (step) + "|" + c.doText + "|" + c.actionLabel + "|" + c.actionTip + "|" + c.status
                           + "|" + juce::String ((int) c.actionEnabled) + "|" + juce::String ((int) finished);

    for (int i = 0; i < firstsong::numSteps; ++i)
        rows[i]->setState (done[i], i == step);

    if (signature != bodySignature)
    {
        bodySignature = signature;
        bodyContent = c;
        resized();
    }

    backButton.setEnabled (step > 0);
    skipButton.setEnabled (step < firstsong::numSteps - 1);
    nextButton.setEnabled (step < firstsong::numSteps - 1);
    nextButton.setColour (juce::TextButton::buttonColourId, done[step] ? theme::accent.darker (0.3f) : theme::panelLight);
    repaint();
}

void FirstSongExercise::resized()
{
    auto r = getLocalBounds().reduced (margin);
    introLayout = makeLayout (firstsong::getIntro(), 12.5f, theme::textDim, (float) r.getWidth());
    introArea = r.removeFromTop ((int) introLayout.getHeight() + 6);
    auto footer = r.removeFromBottom (footerHeight);
    r.removeFromBottom (6);

    const bool list = ! finished;

    for (auto& row : rows)
        row->setVisible (list);

    viewport.setVisible (list);

    for (auto* b : { &backButton, &skipButton, &nextButton })
        b->setVisible (list);

    carryOnButton.setVisible (! list);
    startOverButton.setVisible (! list);

    if (! list)
    {
        finishedLayout = makeLayout (firstsong::getCompletionText(), 14.0f, theme::text, (float) r.getWidth());
        finishedArea = r.removeFromTop ((int) finishedLayout.getHeight() + 12);
        auto buttons = r.removeFromTop (30);
        carryOnButton.setBounds (buttons.removeFromLeft (190));
        buttons.removeFromLeft (8);
        startOverButton.setBounds (buttons.removeFromLeft (100));
        countArea = footer;
        return;
    }

    finishedArea = {};
    backButton.setBounds (footer.removeFromLeft (70));
    nextButton.setBounds (footer.removeFromRight (70));
    footer.removeFromRight (6);
    skipButton.setBounds (footer.removeFromRight (70));
    countArea = footer;

    for (int i = 0; i < firstsong::numSteps; ++i)
    {
        rows[i]->setBounds (r.removeFromTop (rowHeight));

        if (i == step)
        {
            const int below = (firstsong::numSteps - 1 - i) * rowHeight;
            const int available = juce::jmax (40, r.getHeight() - below);
            const int bodyWidth = r.getWidth() - 12;
            const int bodyHeight = body->layout (bodyWidth, bodyContent);
            viewport.setBounds (r.removeFromTop (juce::jmin (bodyHeight, available)));
        }
    }
}

void FirstSongExercise::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    introLayout.draw (g, introArea.toFloat());

    if (finished)
    {
        finishedLayout.draw (g, finishedArea.toFloat());
        return;
    }

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.5f));
    g.drawFittedText (juce::String (getDoneCount()) + " of 4 done", countArea, juce::Justification::centred, 1, 0.8f);
}

bool FirstSongExercise::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && onClose != nullptr)
    {
        onClose();
        return true;
    }

    return false;
}
} // namespace daw
