#include "NotebookWindow.h"

#include "engine/Notebook.h"

namespace daw
{
namespace
{
    constexpr int tabHeight = 28, pad = 12;

    juce::String tabToKey (NotebookWindow::Tab t)
    {
        return t == NotebookWindow::Tab::app ? "app" : t == NotebookWindow::Tab::track ? "track" : "project";
    }
}

NotebookWindow::NotebookWindow (Engine& e, Selection& s) : engine (e), selection (s)
{
    appText = notebook::loadAppNotebook();

    const struct { juce::TextButton* button; Tab tab; const char* tip; } tabs[]
    {
        { &appButton, Tab::app, "Your own notebook, the same in every song (kept on this computer)" },
        { &projectButton, Tab::project, "Notes about this song (saved in the project)" },
        { &trackButton, Tab::track, "Notes about the selected track (saved with the track in the project)" }
    };

    for (auto& t : tabs)
    {
        t.button->setClickingTogglesState (true);
        t.button->setRadioGroupId (95);
        t.button->setTooltip (t.tip);
        t.button->setWantsKeyboardFocus (false);
        t.button->setColour (juce::TextButton::buttonColourId, theme::panelLight);
        t.button->setColour (juce::TextButton::buttonOnColourId, theme::accent);
        t.button->setColour (juce::TextButton::textColourOffId, theme::text);
        t.button->setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        t.button->onClick = [this, which = t.tab] { showTab (which); };
        addAndMakeVisible (*t.button);
    }

    closeButton.setTooltip ("Close the Notebook");
    closeButton.setWantsKeyboardFocus (false);
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    editor.setMultiLine (true, true);
    editor.setReturnKeyStartsNewLine (true);
    editor.setTabKeyUsedAsCharacter (true);
    editor.setScrollbarsShown (true);
    editor.setFont (juce::FontOptions (15.0f));
    editor.setIndents (8, 6);
    editor.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    editor.setColour (juce::TextEditor::textColourId, theme::text);
    editor.setColour (juce::TextEditor::outlineColourId, theme::grid);
    editor.setColour (juce::TextEditor::focusedOutlineColourId, theme::accent);
    editor.setColour (juce::TextEditor::highlightColourId, theme::accent.withAlpha (0.4f));
    editor.setTooltip ("Type your notes. Cmd+Z undoes typing. The App notebook is kept as you type; project and track notes are saved when you save the project.");
    editor.onTextChange = [this] { typed(); };
    addAndMakeVisible (editor);

    const auto saved = engine.properties.getUserSettings()->getValue ("view.notebookTab", "project");
    tab = saved == "app" ? Tab::app : saved == "track" ? Tab::track : Tab::project;

    engine.addChangeListener (this);
    selection.addChangeListener (this);
    setSize (defaultWidth, defaultHeight);
    refresh (true);
}

NotebookWindow::~NotebookWindow()
{
    stopTimer();
    flush();
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
}

void NotebookWindow::showTab (Tab t)
{
    tab = t;
    engine.properties.getUserSettings()->setValue ("view.notebookTab", tabToKey (tab));
    refresh (true);
}

void NotebookWindow::flush()
{
    if (! appDirty)
        return;

    stopTimer();
    appDirty = false;
    saveError = notebook::saveAppNotebook (appText) ? juce::String() : "Couldn't save the notebook to " + notebook::getNotebookFile().getFullPathName();
    repaint();
}

void NotebookWindow::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh (false);
}

void NotebookWindow::timerCallback()
{
    flush();
}

juce::String NotebookWindow::getEngineText() const
{
    if (tab == Tab::app)
        return appText;

    if (tab == Tab::project)
        return engine.getProjectNotes();

    return shownTrack != nullptr ? shownTrack->trackNotes : juce::String();
}

juce::String NotebookWindow::getDescription() const
{
    if (! saveError.isEmpty() && tab == Tab::app)
        return saveError;

    if (tab == Tab::app)
        return "Your notes across all songs. Saved on this computer.";

    if (tab == Tab::project)
        return "Notes about this song. Saved with the project.";

    if (shownTrack == nullptr)
        return "Select a track to write notes about it.";

    return "Notes about " + shownTrack->name + ". Saved with the track in the project.";
}

juce::String NotebookWindow::getPlaceholder() const
{
    if (tab == Tab::app)
        return "For example: \"Ratio 3:1 on the vocal compressor sounded natural.\" \"Try Am F C G in the next song.\" A to-do list.";

    if (tab == Tab::project)
        return "For example: what the song is about, the plan (verse, chorus, verse, chorus, bridge, chorus), what to fix next, \"v3: new bridge, louder vocal\".";

    return "For example: \"Recorded with the microphone 20 cm from the guitar.\" \"Take 3 has the best chorus.\" \"Fix: the snare is late in bar 12.\"";
}

void NotebookWindow::refresh (bool force)
{
    // The shown track must still be in the edit (a deleted track is gone, an undo may have brought it back as another object).
    Track* wanted = selection.getTrack();

    if (wanted != nullptr && engine.indexOf (wanted) < 0)
        wanted = nullptr;

    const bool changed = force || wanted != shownTrack || engine.getEditGeneration() != shownGeneration;
    shownTrack = wanted;
    shownGeneration = engine.getEditGeneration();

    appButton.setToggleState (tab == Tab::app, juce::dontSendNotification);
    projectButton.setToggleState (tab == Tab::project, juce::dontSendNotification);
    trackButton.setToggleState (tab == Tab::track, juce::dontSendNotification);
    trackButton.setButtonText ("Track");

    const bool usable = tab != Tab::track || shownTrack != nullptr;
    editor.setEnabled (usable);

    // Never in reply to our own write: the box is reloaded only when what it shows changed, or the engine's text differs from it.
    const auto text = getEngineText();

    if (changed || notebook::normaliseLineEndings (editor.getText()) != text)
        editor.setText (text, false);

    resized();
    repaint();
}

void NotebookWindow::typed()
{
    const auto text = notebook::normaliseLineEndings (editor.getText());

    if (tab == Tab::app)
    {
        appText = text;
        appDirty = true;
        saveError = {};
        startTimer (1000);
    }
    else if (tab == Tab::project)
    {
        engine.setProjectNotes (text);
    }
    else if (shownTrack != nullptr)
    {
        engine.setTrackNotes (*shownTrack, text);
    }

    repaint();
}

void NotebookWindow::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    g.setColour (saveError.isNotEmpty() && tab == Tab::app ? theme::meterRed : theme::textDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (getDescription(), descriptionArea, juce::Justification::topLeft, 2, 0.9f);
}

void NotebookWindow::paintOverChildren (juce::Graphics& g)
{
    // The placeholder is drawn here, wrapped, because the editor's own is a single line and cut off.
    if (editor.isEnabled() && editor.getTotalNumChars() == 0)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText (getPlaceholder(), editor.getBounds().reduced (10, 8), juce::Justification::topLeft, 6, 1.0f);
    }
}

void NotebookWindow::resized()
{
    auto r = getLocalBounds().reduced (pad);
    auto tabs = r.removeFromTop (tabHeight);
    closeButton.setBounds (tabs.removeFromRight (28));
    tabs.removeFromRight (4);
    const int tabWidth = juce::jmax (0, (tabs.getWidth() - 8) / 3);
    appButton.setBounds (tabs.removeFromLeft (tabWidth));
    tabs.removeFromLeft (4);
    projectButton.setBounds (tabs.removeFromLeft (tabWidth));
    tabs.removeFromLeft (4);
    trackButton.setBounds (tabs);
    r.removeFromTop (8);
    descriptionArea = r.removeFromTop (34);
    r.removeFromTop (6);
    editor.setBounds (r);
}
}
