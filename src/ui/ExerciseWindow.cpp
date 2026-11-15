#include "ExerciseWindow.h"

namespace daw
{
namespace firstsong = gen::firstsong;
namespace exercises = gen::exercises;

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
class ExerciseWindow::StepRow final : public juce::Button
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
class ExerciseWindow::Body final : public juce::Component
{
public:
    using Content = BodyContent;

    Body()
    {
        showMe.setButtonText ("Show me");
        showMe.setTooltip ("Points at the control this step is about");
        addAndMakeVisible (action);
        addAndMakeVisible (showMe);
        addChildComponent (link[0]);
        addChildComponent (link[1]);
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
            statusLayout = makeLayout (c.status, 12.5f, c.statusDim ? theme::textDim : theme::meterYellow, tw);
            statusArea = { 2.0f, y, tw, statusLayout.getHeight() };
            y += statusLayout.getHeight() + 8.0f;
        }

        doneNoteArea = {};

        if (c.doneNote.isNotEmpty())
        {
            doneNoteLayout = makeLayout (c.doneNote, 12.5f, theme::meterGreen, tw);
            doneNoteArea = { 2.0f, y, tw, doneNoteLayout.getHeight() };
            y += doneNoteLayout.getHeight() + 8.0f;
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

        // One chip per linked page, side by side while they fit, else one under the other.
        int chipX = 2;
        bool placedAny = false;

        for (int i = 0; i < 2; ++i)
        {
            const bool used = i < c.linkTexts.size();
            link[i].setVisible (used);

            if (! used)
                continue;

            link[i].setButtonText (c.linkTexts[i]);
            link[i].setTooltip ("Opens this page of the guide");
            const int chipWidth = juce::jmin (w - 4, link[i].getBestWidthForHeight (24) + 8
                                                      + (int) juce::GlyphArrangement::getStringWidth (juce::FontOptions (14.0f), c.linkTexts[i]));

            if (placedAny && chipX + chipWidth > w - 2)
            {
                chipX = 2;
                y += 28.0f;
            }

            link[i].setBounds (chipX, (int) y, chipWidth, 24);
            chipX += chipWidth + 6;
            placedAny = true;
        }

        if (placedAny)
            y += 28.0f;

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

        if (! doneNoteArea.isEmpty())
            doneNoteLayout.draw (g, doneNoteArea);
    }

    juce::TextButton action;
    juce::TextButton showMe;
    GuideButton link[2];

private:
    Content content;
    juce::TextLayout subtitleLayout, doLayout, whyLayout, statusLayout, doneNoteLayout;
    juce::Rectangle<float> subtitleArea, doArea, whyArea, statusArea, doneNoteArea;
};

//==============================================================================
ExerciseWindow::ExerciseWindow (Engine& e, Selection& s, exercises::Exercise which, std::function<exercises::Context()> getContextFn, int startStep, bool startFinished)
    : engine (e), selection (s), exercise (which), contextProvider (std::move (getContextFn)), info (exercises::getInfo (which))
{
    setWantsKeyboardFocus (true);
    numSteps = (int) info.steps.size();
    done.assign ((size_t) numSteps, false);

    // The place comes from the owner's memory (this session and project), never from settings.
    step = exercises::clampStep (exercise, startStep);
    finished = startFinished;

    for (int i = 0; i < numSteps; ++i)
    {
        rows.push_back (std::make_unique<StepRow> (i, info.steps[(size_t) i].title, info.steps[(size_t) i].subtitle));
        rows.back()->onClick = [this, i] { setStep (i); };
        addAndMakeVisible (*rows.back());
    }

    body = std::make_unique<Body>();
    body->action.onClick = [this] { if (onAction != nullptr) onAction (step, currentFix); };
    body->showMe.onClick = [this] { if (onShowMe != nullptr) onShowMe (step); };

    for (int i = 0; i < 2; ++i)
        body->link[i].onClick = [this, i]
        {
            const auto& links = info.steps[(size_t) step].links;

            if (onOpenLink != nullptr && i < (int) links.size())
                onOpenLink (links[(size_t) i]);
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
    carryOnButton.setButtonText (info.carryOnLabel);
    carryOnButton.setTooltip (info.carryOnTooltip);
    carryOnButton.onClick = [this] { if (onCarryOn != nullptr) onCarryOn(); };
    carryOnButton.setColour (juce::TextButton::buttonColourId, theme::accent.darker (0.3f));
    startOverButton.setTooltip ("Back to step 1. Your song stays as it is.");
    startOverButton.onClick = [this] { startOver(); };

    for (auto* b : { &backButton, &skipButton, &nextButton, &carryOnButton, &startOverButton })
        addChildComponent (*b);

    engine.addChangeListener (this);
    selection.addChangeListener (this);
    startTimerHz (2);
    // Each exercise has its own height (`Info::windowHeight`): longer texts and a status line need a taller window.
    setSize (windowWidth, info.windowHeight);   // last: resized() needs the rows and the body
    refresh();
}

ExerciseWindow::~ExerciseWindow()
{
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
}

exercises::Context ExerciseWindow::getContext() const
{
    return contextProvider != nullptr ? contextProvider() : exercises::Context();
}

bool ExerciseWindow::isDone (int index) const
{
    return exercises::isStepDone (exercise, index, engine, getContext());
}

int ExerciseWindow::getDoneCount() const noexcept
{
    int n = 0;

    for (bool d : done)
        n += d ? 1 : 0;

    return n;
}

juce::TextButton& ExerciseWindow::getActionButton() noexcept
{
    return body->action;
}

void ExerciseWindow::saveSettings()
{
    if (onProgressChanged != nullptr)
        onProgressChanged (step, finished);

    // "Finished once" outlives the session; it only ever turns on when all steps were ticked, or off on Start over.
    auto& settings = *engine.properties.getUserSettings();
    settings.setValue (exercises::getDoneSettingKey (exercise), finished);
    settings.saveIfNeeded();
}

void ExerciseWindow::setStep (int newStep)
{
    newStep = juce::jlimit (0, numSteps - 1, newStep);

    if (newStep == step)
        return;

    step = newStep;
    saveSettings();
    bodySignature = {};
    refresh();
}

void ExerciseWindow::returnToFirstStep()
{
    finished = false;
    step = 0;

    if (onProgressChanged != nullptr)
        onProgressChanged (step, finished);

    bodySignature = {};
    refresh();
}

void ExerciseWindow::startOver()
{
    finished = false;
    step = 0;

    if (onStartOver != nullptr)
        onStartOver();

    saveSettings();
    bodySignature = {};
    refresh();
}

void ExerciseWindow::refresh()
{
    const auto context = getContext();

    for (int i = 0; i < numSteps; ++i)
        done[(size_t) i] = exercises::isStepDone (exercise, i, engine, context);

    // Ticked together, once: remembered, and the list gives way to the completion text.
    if (! finished && getDoneCount() == numSteps)
    {
        finished = true;
        saveSettings();
    }

    const bool firstSongExercise = exercise == exercises::Exercise::firstSong;

    BodyContent c;
    const auto& s = info.steps[(size_t) step];
    const auto action = exercises::getAction (exercise, step, engine, context);
    const auto status = exercises::getStatus (exercise, step, engine, context);
    currentFix = action.fix;
    c.subtitle = s.subtitle;
    c.doText = exercises::getDoText (exercise, step, engine, context);
    c.why = exercises::getWhy (exercise, step, engine, context);
    c.actionLabel = action.label;
    c.actionTip = action.tooltip;
    c.status = status.text;
    c.statusDim = ! firstSongExercise;
    c.hasShowMe = ! (firstSongExercise && step == 3);

    for (const auto& link : s.links)
        c.linkTexts.add (link.topic);

    if (done[(size_t) step])
        c.doneNote = exercises::getDoneNote (exercise, step, engine, context);

    if (firstSongExercise)
    {
        bool drums = false;

        for (auto& t : engine.getTracks())
            drums = drums || firstsong::isDrumTrack (*t);

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

        if (step == 3 && context.saved.hasFile && context.saved.unsaved)
            c.status = "Changed since you saved: press Save again.";
    }

    const auto signature = juce::String (step) + "|" + c.doText + "|" + c.why + "|" + c.actionLabel + "|" + c.actionTip + "|" + c.status
                           + "|" + c.doneNote + "|" + juce::String ((int) c.actionEnabled) + "|" + juce::String ((int) finished);

    for (int i = 0; i < numSteps; ++i)
        rows[(size_t) i]->setState (done[(size_t) i], i == step);

    if (signature != bodySignature)
    {
        bodySignature = signature;
        bodyContent = c;
        resized();
    }

    backButton.setEnabled (step > 0);
    skipButton.setEnabled (step < numSteps - 1);
    nextButton.setEnabled (step < numSteps - 1);
    nextButton.setColour (juce::TextButton::buttonColourId, done[(size_t) step] ? theme::accent.darker (0.3f) : theme::panelLight);
    repaint();
}

void ExerciseWindow::resized()
{
    auto r = getLocalBounds().reduced (margin);
    introLayout = makeLayout (info.intro, 12.5f, theme::textDim, (float) r.getWidth());
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
        finishedLayout = makeLayout (info.completion, 14.0f, theme::text, (float) r.getWidth());
        finishedArea = r.removeFromTop ((int) finishedLayout.getHeight() + 12);
        auto buttons = r.removeFromTop (30);
        const int carryOnWidth = juce::jmin (r.getWidth() - 108,
                                             (int) juce::GlyphArrangement::getStringWidth (juce::FontOptions (14.0f), info.carryOnLabel) + 30);
        carryOnButton.setBounds (buttons.removeFromLeft (juce::jmax (150, carryOnWidth)));
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

    for (int i = 0; i < numSteps; ++i)
    {
        rows[(size_t) i]->setBounds (r.removeFromTop (rowHeight));

        if (i == step)
        {
            const int below = (numSteps - 1 - i) * rowHeight;
            const int available = juce::jmax (40, r.getHeight() - below);
            const int bodyWidth = r.getWidth() - 12;
            const int bodyHeight = body->layout (bodyWidth, bodyContent);
            viewport.setBounds (r.removeFromTop (juce::jmin (bodyHeight, available)));
        }
    }
}

void ExerciseWindow::paint (juce::Graphics& g)
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
    g.drawFittedText (juce::String (getDoneCount()) + " of " + juce::String (numSteps) + " done", countArea, juce::Justification::centred, 1, 0.8f);
}

bool ExerciseWindow::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && onClose != nullptr)
    {
        onClose();
        return true;
    }

    return false;
}
} // namespace daw
