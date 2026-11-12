#include "GeneratePanel.h"
#include "engine/Tempo.h"
#include "generate/ChordStyles.h"
#include "generate/ChatMusician.h"
#include "TimeSignatureUi.h"
#include "generate/OllamaClient.h"
#include "generate/PromptBuilder.h"
#include "generate/Takes.h"

namespace daw
{
namespace
{
    namespace ai = gen::aisetup;

    constexpr const char* defaultModel = "qwen2.5:7b";
    constexpr int defaultTakes = 3, maxTakes = 4;
    constexpr int labelWidth = 62;

    // Reply limits in tokens. Thinking comes out of the same budget, so it gets far more room.
    constexpr int replyLimit = 4096, replyLimitWhenThinking = 16384;

   #if JUCE_MAC
    constexpr const char* undoKeys = "Cmd+Z";
   #else
    constexpr const char* undoKeys = "Ctrl+Z";
   #endif

    /** A label's height for its text at a width: at least `minHeight`, taller when the text wraps onto more lines, so no line is cut off. */
    int getLabelHeight (const juce::Label& label, int width, int minHeight)
    {
        juce::AttributedString text;
        text.setText (label.getText());
        text.setFont (label.getFont());
        const auto border = label.getBorderSize();
        juce::TextLayout layout;
        layout.createLayout (text, (float) juce::jmax (1, width - border.getLeftAndRight()));
        return juce::jmax (minHeight, (int) std::ceil (layout.getHeight()) + border.getTopAndBottom() + 2);
    }

    juce::PropertiesFile& settings (Engine& engine) { return *engine.properties.getUserSettings(); }

    /** The song bar (of the chord track) where a bar of the clip starts. */
    int songBarOf (double barLength, const MidiClip& clip, int clipBar)
    {
        return (int) std::floor (clip.startBeat / barLength + clipBar + 1.0e-6);
    }

    /** e.g. "Am - F - C - G", naming a chord held over several bars once. */
    juce::String joinChordNames (const std::vector<const Chord*>& chords)
    {
        juce::StringArray names;

        for (auto* chord : chords)
        {
            const auto name = chord != nullptr ? chord->getName() : juce::String ("-");

            if (names.isEmpty() || names[names.size() - 1] != name)
                names.add (name);
        }

        return names.joinIntoString (" - ");
    }

    /** Song bars (from 0) as shown to the user: "1-4" or "1, 3-4", each bar one more than its number. */
    juce::String formatBars (std::vector<int> bars)
    {
        std::sort (bars.begin(), bars.end());
        juce::StringArray parts;
        const auto dash = juce::String::fromUTF8 ("\xe2\x80\x93");

        for (size_t i = 0; i < bars.size();)
        {
            size_t j = i;

            while (j + 1 < bars.size() && bars[j + 1] == bars[j] + 1)
                ++j;

            parts.add (i == j ? juce::String (bars[i] + 1) : juce::String (bars[i] + 1) + dash + juce::String (bars[j] + 1));
            i = j + 1;
        }

        return parts.joinIntoString (", ");
    }

}

bool GeneratePanel::ModeButton::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::upKey)  { if (onArrow) onArrow (-1); return true; }
    if (key == juce::KeyPress::rightKey || key == juce::KeyPress::downKey) { if (onArrow) onArrow (1); return true; }
    return juce::TextButton::keyPressed (key);
}

void GeneratePanel::SetupCallout::paint (juce::Graphics& g)
{
    g.setColour (theme::panelLight);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    g.setColour (theme::accent.withAlpha (0.5f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);
}

//==============================================================================
GeneratePanel::GeneratePanel (Engine& e) : engine (e)
{
    setFocusContainerType (juce::Component::FocusContainerType::focusContainer);
    form.setFocusContainerType (juce::Component::FocusContainerType::focusContainer);
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&form, false);
    viewport.setScrollBarsShown (true, false);
    addMouseListener (this, false);
    form.addMouseListener (this, false);

    title.setText ("Generate", juce::dontSendNotification);
    title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    form.addAndMakeVisible (title);

    for (auto* l : { &infoLabel, &modeLine, &partNote, &aiStatus, &checkLine, &setupText })
    {
        l->setColour (juce::Label::textColourId, theme::textDim);
        l->setFont (juce::FontOptions (12.0f));
        l->setJustificationType (juce::Justification::topLeft);
        l->setMinimumHorizontalScale (1.0f);
    }

    aiStatus.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    setupText.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    modeLine.setInterceptsMouseClicks (false, false);
    form.addAndMakeVisible (infoLabel);
    form.addAndMakeVisible (modeLine);
    form.addChildComponent (partNote);
    form.addChildComponent (aiStatus);
    form.addChildComponent (checkLine);

    partNote.setText ("Melody, drums and percussion need AI part.", juce::dontSendNotification);

    closeButton.setTooltip ("Close");
    closeButton.setTitle ("Close the Generate panel");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    form.addAndMakeVisible (closeButton);

    // Modes: a radio group of three buttons; the arrow keys move between them.
    static const char* modeTips[] = { "DawIt plays a pattern from its own library over the chord track. No AI needed; works offline.",
                                      "A local AI model on your computer writes the part. Needs Ollama and a model.",
                                      "A separately installed ChatMusician model writes a simple melody or chord progression. 4/4 only." };

    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[i];
        b.setRadioGroupId (4711);
        b.setClickingTogglesState (true);
        b.setColour (juce::TextButton::buttonOnColourId, theme::accent.withAlpha (0.7f));
        b.setTooltip (modeTips[i]);
        b.setTitle (b.getButtonText() + " mode");
        b.setConnectedEdges (i == 0 ? juce::Button::ConnectedOnRight : i == 2 ? juce::Button::ConnectedOnLeft
                                                                              : juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
        b.onClick = [this, i] { if (modeButtons[i].getToggleState()) setMode ((ai::Mode) i); };
        b.onArrow = [this] (int step)
        {
            const int next = juce::jlimit (0, 2, (int) mode + step);
            setMode ((ai::Mode) next);
            modeButtons[next].grabKeyboardFocus();
        };
        form.addAndMakeVisible (b);
    }

    // AI status, Retry, Set up...
    retryButton.setTooltip ("Ask Ollama again which models it has.");
    retryButton.onClick = [this] { refreshModels(); };
    setupButton.setTooltip ("Show the steps to get Ollama and a model on this computer.");
    setupButton.onClick = [this] { showSetup (! setupShown); };
    usePatternButton.setTooltip ("Switch to Built-in pattern: DawIt's own patterns, no AI and no Ollama.");
    usePatternButton.setColour (juce::TextButton::textColourOffId, theme::accent);
    usePatternButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    usePatternButton.onClick = [this] { setMode (ai::Mode::pattern); };
    testAgainButton.setTooltip ("Ask the model for one test bar again.");
    testAgainButton.onClick = [this] { generateChatMusician (true); };

    for (juce::Component* c : { (juce::Component*) &retryButton, (juce::Component*) &setupButton, (juce::Component*) &usePatternButton,
                                (juce::Component*) &testAgainButton, (juce::Component*) &setupCallout })
        form.addChildComponent (c);

    setupCallout.addAndMakeVisible (setupText);
    openOllamaButton.setTooltip ("Open the Ollama website in your browser.");
    openOllamaButton.onClick = [] { juce::URL ("https://ollama.com").launchInDefaultBrowser(); };
    copyServeButton.setTooltip ("Copy the command that starts Ollama.");
    copyServeButton.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard ("ollama serve");
        setStatus ("Copied: ollama serve");
    };
    copyPullButton.setTooltip ("Copy the command that downloads a model (4.7 GB).");
    copyPullButton.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard (juce::String ("ollama pull ") + defaultModel);
        setStatus (juce::String ("Copied: ollama pull ") + defaultModel);
    };
    for (auto* b : { &openOllamaButton, &copyServeButton, &copyPullButton })
        setupCallout.addAndMakeVisible (b);

    // ChatMusician
    chatTask.addItem ("Melody over song chords", 1);
    chatTask.addItem ("Chord progression", 2);
    chatTask.setSelectedId (1);
    chatTask.setTooltip ("Choose whether to write a melody or a chord progression.");
    chatTask.setTitle ("ChatMusician task");
    chatModelBox.setTooltip ("The Ollama model that writes the ChatMusician music. Models with \"chatmusician\" in the name come first.");
    chatModelBox.setTitle ("ChatMusician model");
    chatModelBox.setTextWhenNothingSelected ("Choose a model");
    chatSevenths.setTooltip ("Allow seventh chords as well as triads.");
    chatBorrowed.setTooltip ("Allow triads borrowed from the parallel major or minor key.");
    chatDominant.setTooltip ("In a minor key, allow a major dominant chord.");
    chatFirst.setTextToShowWhenEmpty ("First chord (optional)", theme::textDim);
    chatLast.setTextToShowWhenEmpty ("Last chord (optional)", theme::textDim);
    chatFirst.setTooltip ("Optional required first chord, such as Am.");
    chatLast.setTooltip ("Optional required last chord, such as E7.");
    chatFirst.setTitle ("First chord");
    chatLast.setTitle ("Last chord");
    chatGuide.setTooltip ("Open the reading music guide, including ABC and generated music.");
    chatTask.onChange = [this] { saveSettings(); updateControls(); };
    chatGuide.onClick = [this] { if (onShowReadingGuide) onShowReadingGuide(); };
    chatModelBox.onChange = [this] { saveSettings(); updateSetup(); };

    for (juce::Component* c : { (juce::Component*) &chatModelLabel, (juce::Component*) &chatModelBox, (juce::Component*) &chatTaskLabel,
                                (juce::Component*) &chatTask, (juce::Component*) &chatSevenths, (juce::Component*) &chatBorrowed,
                                (juce::Component*) &chatDominant, (juce::Component*) &chatFirst, (juce::Component*) &chatLast,
                                (juce::Component*) &chatGuide })
        form.addChildComponent (c);

    for (auto* l : { &partLabel, &styleLabel, &promptLabel, &modelLabel, &variationLabel, &playingLabel, &chatModelLabel, &chatTaskLabel })
    {
        l->setColour (juce::Label::textColourId, theme::textDim);
        l->setInterceptsMouseClicks (false, false);
    }

    for (int i = 0; i < gen::numParts; ++i)
        partBox.addItem (gen::getPartName ((gen::Part) i), i + 1);

    partBox.setTooltip ("What kind of part to make.");
    partBox.setTitle ("Part");
    styleBox.addItemList (gen::getStylePresets(), 1);
    styleBox.setEditableText (true);
    styleBox.setTooltip ("Pick a style or type your own");
    styleBox.setTitle ("Style");
    modelBox.setTitle ("AI model");
    playingBox.setTitle ("Playing style");
    takesBox.setTitle ("Takes");
    variationSlider.setTitle ("Variation");

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (true);
    promptEditor.setTextToShowWhenEmpty ("Optional: mood, rhythm, references...", theme::textDim);
    promptEditor.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    promptEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    promptEditor.setTitle ("Prompt");
    promptEditor.setTooltip ("Optional words for the AI: mood, rhythm, references.");

    modelBox.setTooltip ("Installed Ollama models");
    modelBox.setEditableText (true);

    variationSlider.setScrollWheelEnabled (false);   // scrolling never changes a value
    variationSlider.setRange (0.0, 1.0);
    variationSlider.setDoubleClickReturnValue (true, gen::defaultVariation);
    variationSlider.setPopupDisplayEnabled (true, true, nullptr);
    variationSlider.textFromValueFunction = [] (double v)
    {
        return juce::String (v < 0.1 ? "Safe" : v < 0.34 ? "Low" : v < 0.67 ? "Medium" : "High") + " variation";
    };
    variationSlider.setTooltip ("Low: close to what is typical for the style. High: more surprising. Raises the sampling "
                                "temperature, and gives each take a few random ideas to follow (such as \"sparse\" "
                                "or \"a rising shape\"). Double-click for the default.");

    for (int n = 1; n <= maxTakes; ++n)
        takesBox.addItem (juce::String (n) + (n == 1 ? " take" : " takes"), n);
    takesBox.setTooltip ("How many versions to make per click. They are scored and the best is staged first; "
                         "the take buttons below stage the others. More takes take longer.");

    playingBox.setTooltip (juce::String::fromUTF8 ("How the part is played: a piano, guitar, synth, strings or bass style from DawIt's library, "
                                                   "following the chord track. \xe2\x98\x85 marks styles typical for the chosen style of music. "
                                                   "In AI part, Free lets the model write the part itself and a style is varied by the model."));
    thinkToggle.setTooltip ("Let the model reason before it answers: often more musical, but several times slower. "
                            "Only for models that can think, such as qwen3 and gemma4; others answer straight away.");

    for (juce::Component* c : { (juce::Component*) &partLabel, (juce::Component*) &partBox, (juce::Component*) &styleLabel,
                                (juce::Component*) &styleBox, (juce::Component*) &playingLabel, (juce::Component*) &playingBox,
                                (juce::Component*) &modelLabel, (juce::Component*) &modelBox, (juce::Component*) &promptLabel,
                                (juce::Component*) &promptEditor, (juce::Component*) &variationLabel,
                                (juce::Component*) &variationSlider, (juce::Component*) &takesBox, (juce::Component*) &thinkToggle })
        form.addChildComponent (c);

    // The footer does not scroll with the settings.
    intoLabel.setFont (juce::FontOptions (12.0f));
    intoLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    intoLabel.setMinimumHorizontalScale (0.8f);
    chordLine.setFont (juce::FontOptions (12.0f));
    chordLine.setColour (juce::Label::textColourId, theme::textDim);
    chordLine.setJustificationType (juce::Justification::topLeft);
    chordLine.setMinimumHorizontalScale (1.0f);
    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setJustificationType (juce::Justification::topLeft);
    statusLabel.setMinimumHorizontalScale (1.0f);

    replaceButton.setRadioGroupId (4712);
    addButton.setRadioGroupId (4712);
    for (auto* b : { &replaceButton, &addButton })
    {
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, theme::accent.withAlpha (0.7f));
    }
    replaceButton.setConnectedEdges (juce::Button::ConnectedOnRight);
    addButton.setConnectedEdges (juce::Button::ConnectedOnLeft);
    replaceButton.setTooltip ("The generated notes take the place of the clip's own notes when you apply.");
    addButton.setTooltip ("The generated notes are added to the clip's own notes when you apply.");
    replaceButton.onClick = [this] { saveSettings(); staging.setReplace (true); showOverlay(); updateControls(); };
    addButton.onClick = [this] { saveSettings(); staging.setReplace (false); showOverlay(); updateControls(); };

    generateButton.setColour (juce::TextButton::buttonColourId, theme::accent.withAlpha (0.7f));
    generateButton.onClick = [this] { cancelFlag != nullptr ? cancel() : generate(); };
    previewButton.setTooltip ("Play the song from the start of the clip to its end with the staged take on the track's own "
                              "instrument. Press again to stop. The song is unchanged.");
    applyButton.setTooltip ("Write the staged take into the clip as one undo step.");
    discardButton.setTooltip ("Throw the staged take away. The song never changed.");
    undoApplyButton.setTooltip ("Take back the last Apply (the same as Undo), while it is still the latest change.");
    scopeBox.addItem ("Notes and chords", 1);
    scopeBox.addItem ("Notes only", 2);
    scopeBox.addItem ("Chords only", 3);
    scopeBox.setSelectedId (1, juce::dontSendNotification);
    scopeBox.setTooltip ("What Apply writes: the notes, the proposed chords (only into empty chord-track bars), or both.");
    scopeBox.setTitle ("What to apply");
    previewButton.onClick = [this] { previewStaged(); };
    applyButton.onClick = [this]
    {
        applyStaged (scopeBox.isVisible() ? (Engine::GenerateScope) (scopeBox.getSelectedId() - 1) : Engine::GenerateScope::notesAndChords);
    };
    discardButton.onClick = [this] { discardStaged ("Discarded. The song is unchanged."); };
    undoApplyButton.onClick = [this] { undoApply(); };

    for (juce::Component* c : { (juce::Component*) &intoLabel, (juce::Component*) &replaceButton, (juce::Component*) &addButton,
                                (juce::Component*) &chordLine, (juce::Component*) &generateButton, (juce::Component*) &undoApplyButton,
                                (juce::Component*) &previewButton, (juce::Component*) &applyButton, (juce::Component*) &discardButton,
                                (juce::Component*) &scopeBox, (juce::Component*) &statusLabel })
        addAndMakeVisible (c);

    // Restore the last-used settings.
    auto& s = settings (engine);
    modelChecks = ai::ModelChecks::fromString (s.getValue ("generator.modelChecks"));
    mode = s.containsKey ("generator.mode") ? ai::getModeFromId (s.getValue ("generator.mode"))
                                           : ai::migrateMode (s.getBoolValue ("generator.chatMusician", false), s.getValue ("generator.model"));
    chatTask.setSelectedId (juce::jlimit (1, 2, s.getIntValue ("generator.chatTask", 1)), juce::dontSendNotification);
    partBox.setSelectedId (juce::jlimit (1, gen::numParts, s.getIntValue ("generator.part", 1)), juce::dontSendNotification);
    styleBox.setText (s.getValue ("generator.style", "Pop"), juce::dontSendNotification);
    const bool replace = s.getBoolValue ("generator.replace", true);
    replaceButton.setToggleState (replace, juce::dontSendNotification);
    addButton.setToggleState (! replace, juce::dontSendNotification);
    thinkToggle.setToggleState (s.getBoolValue ("generator.think", false), juce::dontSendNotification);
    variationSlider.setValue (juce::jlimit (0.0, 1.0, s.getDoubleValue ("generator.variation", gen::defaultVariation)), juce::dontSendNotification);
    takesBox.setSelectedId (juce::jlimit (1, maxTakes, s.getIntValue ("generator.takes", defaultTakes)), juce::dontSendNotification);
    modelBox.setText (s.getValue ("generator.model", defaultModel), juce::dontSendNotification);
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setToggleState (i == (int) mode, juce::dontSendNotification);
    updatePartBox();
    updatePlayingStyles();

    // The part and the style of music change which playing styles are offered (and marked); in Built-in pattern the new
    // choice is staged straight away when a take is showing, which makes trying styles quick.
    partBox.onChange = [this] { updatePlayingStyles(); saveSettings(); restageIfShowing(); updateControls(); };
    styleBox.onChange = [this] { updatePlayingStyles(); saveSettings(); restageIfShowing(); updateControls(); };
    playingBox.onChange = [this] { saveSettings(); restageIfShowing(); updateControls(); };
    modelBox.onChange = [this] { saveSettings(); updateSetup(); };
    thinkToggle.onClick = [this] { saveSettings(); };
    variationSlider.onValueChange = [this] { settings (engine).setValue ("generator.variation", variationSlider.getValue()); };
    takesBox.onChange = [this] { saveSettings(); };

    // Tab order: modes first, then the settings, then the footer.
    int order = 1;
    for (juce::Component* c : { (juce::Component*) &modeButtons[0], (juce::Component*) &modeButtons[1], (juce::Component*) &modeButtons[2],
                                (juce::Component*) &retryButton, (juce::Component*) &setupButton, (juce::Component*) &usePatternButton,
                                (juce::Component*) &testAgainButton, (juce::Component*) &chatModelBox, (juce::Component*) &modelBox,
                                (juce::Component*) &partBox, (juce::Component*) &styleBox, (juce::Component*) &playingBox,
                                (juce::Component*) &promptEditor, (juce::Component*) &variationSlider, (juce::Component*) &takesBox,
                                (juce::Component*) &thinkToggle, (juce::Component*) &chatTask, (juce::Component*) &chatSevenths,
                                (juce::Component*) &chatBorrowed, (juce::Component*) &chatDominant, (juce::Component*) &chatFirst,
                                (juce::Component*) &chatLast, (juce::Component*) &chatGuide, (juce::Component*) &replaceButton,
                                (juce::Component*) &addButton, (juce::Component*) &generateButton, (juce::Component*) &previewButton,
                                (juce::Component*) &applyButton, (juce::Component*) &scopeBox, (juce::Component*) &discardButton,
                                (juce::Component*) &undoApplyButton, (juce::Component*) &closeButton })
        c->setExplicitFocusOrder (order++);

    engine.addChangeListener (this);
    updateSetup();
}

GeneratePanel::~GeneratePanel()
{
    engine.removeChangeListener (this);
    stopTimer();

    if (previewing)
        engine.stop();

    clearOverlay();

    if (cancelFlag != nullptr)
        *cancelFlag = true;
}

juce::String GeneratePanel::getServerUrl() const
{
    return settings (engine).getValue ("generator.ollamaUrl", "http://localhost:11434");
}

void GeneratePanel::saveSettings()
{
    auto& s = settings (engine);
    s.setValue ("generator.mode", ai::getModeId (mode));
    s.setValue ("generator.part", partBox.getSelectedId());
    s.setValue ("generator.style", styleBox.getText());
    s.setValue ("generator.replace", replaceButton.getToggleState());
    s.setValue ("generator.think", thinkToggle.getToggleState());
    s.setValue ("generator.variation", variationSlider.getValue());
    s.setValue ("generator.takes", takesBox.getSelectedId());
    s.setValue ("generator.chatTask", chatTask.getSelectedId());
    s.setValue ("generator.modelChecks", modelChecks.toString());

    if (getPartId().isNotEmpty() && playingBox.getSelectedId() != 0)
        s.setValue ("generator.playing." + getPartId() + (isMode (ai::Mode::pattern) ? ".pattern" : ""), getPlayingStyleId());

    if (modelBox.getText().isNotEmpty() && modelsLoaded)
        s.setValue ("generator.model", modelBox.getText());

    if (getAiModel().isNotEmpty() && isMode (ai::Mode::chatMusician))
        s.setValue ("generator.chatMusicianModel", getAiModel());

    s.saveIfNeeded();
}

Track* GeneratePanel::findTrackFor (const MidiClip* c) const
{
    // After a project was opened or started, an old clip pointer may now name a new clip.
    return engine.getEditGeneration() == clipsEditGeneration ? engine.findTrackFor (c) : nullptr;
}

void GeneratePanel::setTarget (Track* t, MidiClip* c)
{
    if (clip != c)
    {
        if (cancelFlag != nullptr)
            cancel();

        if (staging.isStaged() || overlayClip != nullptr || previewing)
            discardStaged();

        appliedSteps = -1;
    }

    track = t;
    clip = c;
    clipsEditGeneration = engine.getEditGeneration();
    updateControls();
}

void GeneratePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // A change to the clip, its chords, the key or the time signature, or another project, sets a staged take aside.
    if (staging.isStale (engine))
        discardStaged ("The clip or its chords changed while this take was waiting, so it was set aside. Generate again.", true);

    updateControls();
}

void GeneratePanel::visibilityChanged()
{
    if (isVisible())
    {
        if (! isMode (ai::Mode::pattern) && ! modelsLoaded)
            refreshModels();
    }
    else
    {
        // Closing the panel discards a staged take and stops a running request.
        if (cancelFlag != nullptr)
            cancel();

        discardStaged();
    }
}

void GeneratePanel::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        showMenu();
}

void GeneratePanel::showMenu()
{
    const bool running = cancelFlag != nullptr;
    juce::PopupMenu menu;
    menu.addItem (running ? "Cancel" : generateButton.getButtonText(), running || generateButton.isEnabled(),
                  false, [this] { cancelFlag != nullptr ? cancel() : generate(); });
    menu.addItem (previewing ? "Stop preview" : "Preview", previewButton.isEnabled(), false, [this] { previewStaged(); });
    menu.addItem ("Apply", applyButton.isEnabled(), false, [this] { applyButton.onClick(); });
    menu.addItem ("Discard", discardButton.isEnabled(), false, [this] { discardButton.onClick(); });
    menu.addItem ("Undo apply", undoApplyButton.isEnabled(), false, [this] { undoApply(); });
    menu.addSeparator();

    if (isMode (ai::Mode::chatMusician))
        menu.addItem ("How to read music...", onShowReadingGuide != nullptr, false, [this] { if (onShowReadingGuide) onShowReadingGuide(); });
    else
        menu.addItem ("Styles of music...", onShowStylesGuide != nullptr, false, [this] { if (onShowStylesGuide) onShowStylesGuide(); });

    menu.showMenuAsync (juce::PopupMenu::Options());
}

//==============================================================================
void GeneratePanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, 0, 1, getHeight());
    g.setColour (theme::panelLight);
    g.fillRect (1, footerTop - 1, getWidth() - 1, 1);   // the footer's top edge
}

void GeneratePanel::resized()
{
    auto area = getLocalBounds().withTrimmedLeft (1);
    const int fullWidth = area.getWidth() - 20;
    const bool staged = staging.isStaged();
    const bool chordsProposed = staged && scopeBox.isVisible();
    const bool haveTakes = ! takeButtons.isEmpty();

    chordLine.setSize (fullWidth, 16);
    const int chordHeight = chordLine.getText().isEmpty() ? 0 : juce::jmin (30, getLabelHeight (chordLine, fullWidth, 16));

    // Footer: the target, the actions and the status, always on screen. In a short panel (a small window) the rows get
    // tighter and the status shrinks to one line first, so the settings above keep some room.
    const bool compact = area.getHeight() < 280;
    const int gap = compact ? 2 : 4, intoHeight = compact ? 14 : 16, smallRow = compact ? 20 : 24,
              mainRow = compact ? 22 : 28, actionRow = compact ? 22 : 26;
    auto countRows = [&] (int chordRowHeight)
    {
        return intoHeight + gap + smallRow + gap + (chordRowHeight > 0 ? chordRowHeight + gap : 0) + mainRow + (haveTakes ? gap + smallRow : 0)
             + (staged ? gap + actionRow : 0) + (chordsProposed ? gap + smallRow : 0);
    };

    int rows = countRows (chordHeight);
    int chordShown = chordHeight;

    // In a very short panel the chord line gives way (its text is still the tooltip), so the buttons are never pushed off.
    if (compact && area.getHeight() - (rows + 14 + 18) < 50 && chordHeight > 0)
    {
        chordShown = 0;
        rows = countRows (0);
    }

    int statusHeight = 42;
    if (area.getHeight() - (rows + statusHeight + 18) < 120)
        statusHeight = 14;
    const int footerHeight = 6 + rows + 6 + statusHeight + 6;
    footerTop = area.getBottom() - footerHeight;
    auto footer = area.removeFromBottom (footerHeight).reduced (10, 6);

    intoLabel.setBounds (footer.removeFromTop (intoHeight));
    footer.removeFromTop (gap);
    {
        auto line = footer.removeFromTop (smallRow);
        const int half = line.getWidth() / 2;
        replaceButton.setBounds (line.removeFromLeft (half));
        addButton.setBounds (line);
    }
    footer.removeFromTop (gap);

    if (chordShown > 0)
    {
        chordLine.setBounds (footer.removeFromTop (chordShown));
        footer.removeFromTop (gap);
    }
    else
        chordLine.setBounds ({});

    {
        auto line = footer.removeFromTop (mainRow);

        if (undoApplyButton.isVisible())
        {
            undoApplyButton.setBounds (line.removeFromRight (86));
            line.removeFromRight (6);
        }

        generateButton.setBounds (line);
    }

    if (haveTakes)
    {
        footer.removeFromTop (gap);
        auto line = footer.removeFromTop (smallRow);
        const int buttonWidth = (line.getWidth() - 4 * (takeButtons.size() - 1)) / takeButtons.size();

        for (auto* b : takeButtons)
        {
            b->setBounds (line.removeFromLeft (buttonWidth));
            line.removeFromLeft (4);
        }
    }

    if (staged)
    {
        footer.removeFromTop (gap);
        auto line = footer.removeFromTop (actionRow);
        const int w = (line.getWidth() - 8) / 3;
        previewButton.setBounds (line.removeFromLeft (w));
        line.removeFromLeft (4);
        applyButton.setBounds (line.removeFromLeft (w));
        line.removeFromLeft (4);
        discardButton.setBounds (line);
    }

    if (chordsProposed)
    {
        footer.removeFromTop (gap);
        scopeBox.setBounds (footer.removeFromTop (smallRow));
    }

    footer.removeFromTop (6);
    statusLabel.setBounds (footer.removeFromTop (statusHeight));

    //----------------------------------------------------------------------
    viewport.setBounds (area);
    const int width = viewport.getMaximumVisibleWidth();
    auto r = juce::Rectangle<int> (0, 0, width, 4000).reduced (10, 6);

    auto top = r.removeFromTop (24);
    closeButton.setBounds (top.removeFromRight (22).reduced (0, 1));
    title.setBounds (top);

    infoLabel.setSize (r.getWidth(), 16);
    infoLabel.setBounds (r.removeFromTop (getLabelHeight (infoLabel, r.getWidth(), 16)));
    r.removeFromTop (6);

    {
        auto line = r.removeFromTop (26);
        const int w = line.getWidth() / 3;
        modeButtons[0].setBounds (line.removeFromLeft (w));
        modeButtons[1].setBounds (line.removeFromLeft (w));
        modeButtons[2].setBounds (line);
    }

    r.removeFromTop (4);
    modeLine.setSize (r.getWidth(), 16);
    modeLine.setBounds (r.removeFromTop (getLabelHeight (modeLine, r.getWidth(), 16)));
    r.removeFromTop (6);

    if (aiStatus.isVisible())
    {
        aiStatus.setSize (r.getWidth(), 16);
        aiStatus.setBounds (r.removeFromTop (getLabelHeight (aiStatus, r.getWidth(), 16)));
        r.removeFromTop (4);

        auto line = r.removeFromTop (24);
        if (retryButton.isVisible())
        {
            retryButton.setBounds (line.removeFromLeft (70));
            line.removeFromLeft (4);
        }
        if (setupButton.isVisible())
            setupButton.setBounds (line.removeFromLeft (84));
        r.removeFromTop (4);

        if (usePatternButton.isVisible())
        {
            usePatternButton.setBounds (r.removeFromTop (24));
            r.removeFromTop (4);
        }

        if (setupCallout.isVisible())
        {
            setupText.setSize (r.getWidth() - 16, 16);
            const int textHeight = getLabelHeight (setupText, r.getWidth() - 16, 16);
            const bool pull = ! isMode (ai::Mode::chatMusician);
            const int calloutHeight = 8 + textHeight + 6 + 24 + 4 + 24 + (pull ? 4 + 24 : 0) + 8;
            setupCallout.setBounds (r.removeFromTop (calloutHeight));
            auto inner = setupCallout.getLocalBounds().reduced (8);
            setupText.setBounds (inner.removeFromTop (textHeight));
            inner.removeFromTop (6);
            openOllamaButton.setBounds (inner.removeFromTop (24));
            inner.removeFromTop (4);
            copyServeButton.setBounds (inner.removeFromTop (24));
            copyPullButton.setVisible (pull);
            if (pull)
            {
                inner.removeFromTop (4);
                copyPullButton.setBounds (inner.removeFromTop (24));
            }
            r.removeFromTop (6);
        }
    }

    auto row = [&r] (juce::Label& label, juce::Component& control, int height = 24)
    {
        auto line = r.removeFromTop (height);
        label.setBounds (line.removeFromLeft (labelWidth));
        control.setBounds (line);
        r.removeFromTop (5);
    };

    if (isMode (ai::Mode::chatMusician))
    {
        row (chatModelLabel, chatModelBox);

        if (checkLine.isVisible())
        {
            checkLine.setSize (r.getWidth(), 16);
            checkLine.setBounds (r.removeFromTop (getLabelHeight (checkLine, r.getWidth(), 16)));
            r.removeFromTop (4);
        }

        if (testAgainButton.isVisible())
        {
            testAgainButton.setBounds (r.removeFromTop (24).removeFromLeft (100));
            r.removeFromTop (5);
        }

        row (chatTaskLabel, chatTask);

        if (chatTask.getSelectedId() == 2)
        {
            for (auto* c : { &chatSevenths, &chatBorrowed, &chatDominant })
                c->setBounds (r.removeFromTop (22));

            chatFirst.setBounds (r.removeFromTop (24));
            r.removeFromTop (2);
            chatLast.setBounds (r.removeFromTop (24));
            r.removeFromTop (4);
        }

        chatGuide.setBounds (r.removeFromTop (24));
        form.setSize (width, r.getY() + 6);
        return;
    }

    if (isMode (ai::Mode::ai))
        row (modelLabel, modelBox);

    row (partLabel, partBox);

    if (partNote.isVisible())
    {
        partNote.setSize (r.getWidth(), 16);
        partNote.setBounds (r.removeFromTop (getLabelHeight (partNote, r.getWidth(), 16)));
        r.removeFromTop (5);
    }

    row (styleLabel, styleBox);

    if (playingBox.isVisible())
        row (playingLabel, playingBox);

    if (isMode (ai::Mode::ai))
    {
        row (promptLabel, promptEditor, 46);

        auto line = r.removeFromTop (24);
        variationLabel.setBounds (line.removeFromLeft (labelWidth));
        takesBox.setBounds (line.removeFromRight (82));
        line.removeFromRight (4);
        variationSlider.setBounds (line);
        r.removeFromTop (5);

        thinkToggle.setBounds (r.removeFromTop (22).withTrimmedLeft (labelWidth - 4));
    }

    form.setSize (width, r.getY() + 6);
}

//==============================================================================
bool GeneratePanel::hasChordOnEveryBar() const
{
    if (clip == nullptr)
        return false;

    for (int bar = 0; bar < getClipBars(); ++bar)
        if (engine.getChord (songBarOf (engine.getBarLength(), *clip, bar)) == nullptr)
            return false;

    return true;
}

void GeneratePanel::updateControls()
{
    const bool running = cancelFlag != nullptr;
    const bool staged = staging.isStaged();
    const bool pattern = isMode (ai::Mode::pattern), aiPart = isMode (ai::Mode::ai), chat = isMode (ai::Mode::chatMusician);
    const bool haveClip = clip != nullptr && engine.getEditGeneration() == clipsEditGeneration && engine.findTrackFor (clip) != nullptr;
    const auto signature = engine.getTimeSignature();

    infoLabel.setText (engine.getKey().getName() + "   " + timesig::toString (signature) + "   " + formatBpm (engine.getTempo()) + " BPM"
                           + (getInfoNote().isEmpty() ? juce::String() : "\n" + getInfoNote()),
                       juce::dontSendNotification);

    static const char* modeLines[] = { "DawIt plays a pattern from its own library over the chord track. No AI needed; works offline.",
                                       "A local AI model on your computer writes the part. Needs Ollama and a model.",
                                       "A separately installed ChatMusician model writes a simple melody or chord progression. 4/4 only." };
    modeLine.setText (modeLines[(int) mode], juce::dontSendNotification);

    // AI status
    const bool showAi = ! pattern;
    aiStatus.setVisible (showAi);
    aiStatus.setText (modelsLoaded ? setupInfo.status : juce::String ("Looking for Ollama models..."), juce::dontSendNotification);
    retryButton.setVisible (showAi && modelsLoaded);
    retryButton.setButtonText (setupInfo.offersRetry ? "Retry" : "Refresh");
    retryButton.setEnabled (! running);
    setupButton.setVisible (showAi && modelsLoaded && setupInfo.offersRetry);
    usePatternButton.setVisible (showAi && modelsLoaded && setupInfo.offersRetry);
    setupCallout.setVisible (showAi && setupShown);
    checkLine.setVisible (chat && modelsLoaded && setupInfo.checkLine.isNotEmpty());
    checkLine.setText (setupInfo.checkLine, juce::dontSendNotification);
    testAgainButton.setVisible (chat && modelsLoaded && setupInfo.offersTestAgain);
    testAgainButton.setEnabled (! running);

    if (setupCallout.isVisible())
    {
        setupText.setText (ai::getSetupSteps (mode).joinIntoString ("\n"), juce::dontSendNotification);
        copyPullButton.setVisible (! chat);
    }

    // Per-mode settings
    for (juce::Component* c : { (juce::Component*) &chatModelLabel, (juce::Component*) &chatModelBox, (juce::Component*) &chatTaskLabel,
                                (juce::Component*) &chatTask, (juce::Component*) &chatGuide })
        c->setVisible (chat);
    for (juce::Component* c : { (juce::Component*) &chatSevenths, (juce::Component*) &chatBorrowed, (juce::Component*) &chatDominant,
                                (juce::Component*) &chatFirst, (juce::Component*) &chatLast })
        c->setVisible (chat && chatTask.getSelectedId() == 2);
    for (juce::Component* c : { (juce::Component*) &partLabel, (juce::Component*) &partBox, (juce::Component*) &styleLabel, (juce::Component*) &styleBox })
        c->setVisible (! chat);
    for (juce::Component* c : { (juce::Component*) &modelLabel, (juce::Component*) &modelBox, (juce::Component*) &promptLabel,
                                (juce::Component*) &promptEditor, (juce::Component*) &variationLabel, (juce::Component*) &variationSlider,
                                (juce::Component*) &takesBox, (juce::Component*) &thinkToggle })
        c->setVisible (aiPart);
    partNote.setVisible (pattern);
    playingLabel.setVisible (! chat && getPartId().isNotEmpty());
    playingBox.setVisible (! chat && getPartId().isNotEmpty());

    for (juce::Component* c : { (juce::Component*) &partBox, (juce::Component*) &styleBox, (juce::Component*) &playingBox, (juce::Component*) &modelBox,
                                (juce::Component*) &promptEditor, (juce::Component*) &variationSlider, (juce::Component*) &takesBox,
                                (juce::Component*) &thinkToggle, (juce::Component*) &chatModelBox, (juce::Component*) &chatTask,
                                (juce::Component*) &chatSevenths, (juce::Component*) &chatBorrowed, (juce::Component*) &chatDominant,
                                (juce::Component*) &chatFirst, (juce::Component*) &chatLast })
        c->setEnabled (! running);
    for (auto& b : modeButtons)
        b.setEnabled (! running);

    // The footer
    intoLabel.setText (haveClip ? "Into: " + (track != nullptr ? track->name : juce::String()) + juce::String::fromUTF8 (" \xc2\xb7 \"") + clip->name
                                      + juce::String::fromUTF8 ("\" \xc2\xb7 ") + describeBars()
                                : juce::String ("No clip selected"),
                       juce::dontSendNotification);
    intoLabel.setTooltip (intoLabel.getText());
    replaceButton.setEnabled (! running);
    addButton.setEnabled (! running);

    juce::String chordText;

    if (pattern && haveClip && ! hasChordOnEveryBar())
        chordText = describeMissingChords();
    else if (staged && ! staging.getTake().chords.empty())
        chordText = describeProposedChords();

    chordLine.setText (chordText, juce::dontSendNotification);
    chordLine.setTooltip (chordText);

    generateButton.setButtonText (running ? "Cancel" : pattern ? "Make pattern" : "Generate");

    bool canGenerate = false;
    const bool chatAllowed = gen::chatmusician::isAvailable (signature);

    if (haveClip)
    {
        if (pattern)
            canGenerate = hasChordOnEveryBar() && getPartId().isNotEmpty() && getPlayingStyleId().isNotEmpty();
        else if (aiPart)
            canGenerate = modelsLoaded && setupInfo.canStart() && getAiModel().isNotEmpty();
        else
            canGenerate = modelsLoaded && chatAllowed && setupInfo.canStart() && getAiModel().isNotEmpty();
    }

    generateButton.setEnabled (running || canGenerate);

    previewButton.setVisible (staged);
    applyButton.setVisible (staged);
    discardButton.setVisible (staged);
    previewButton.setButtonText (previewing ? "Stop" : "Preview");
    previewButton.setEnabled (! running);
    applyButton.setEnabled (! running);
    discardButton.setEnabled (! running);

    bool chordsToAdd = false;

    if (staged && ! staging.getTake().chords.empty())
        chordsToAdd = ! gen::staging::splitChordProposal (engine.getChords(), staging.getTarget().firstBar, staging.getTake().chords).added.empty();

    scopeBox.setVisible (staged && chordsToAdd);
    undoApplyButton.setVisible (canUndoApply());

    for (auto* b : takeButtons)
        b->setEnabled (! running);

    resized();
}

juce::String GeneratePanel::getInfoNote() const
{
    const auto signature = engine.getTimeSignature();

    // ChatMusician writes bars of 4/4; the built-in patterns are 4/4 too, so in another signature each bar plays one pattern bar, cut or repeated to fit.
    if (isMode (ai::Mode::chatMusician))
        return gen::chatmusician::isAvailable (signature) ? juce::String() : gen::chatmusician::getUnavailableReason();

    if (signature != timesig::fourFour && getPlayingStyleId().isNotEmpty())
        return "This pattern is written in 4/4: each bar of " + timesig::toString (signature) + " ("
               + timesigui::formatBeats (timesig::getBarLength (signature)) + " beats) plays one bar of it, cut or repeated to fit.";

    return {};
}

void GeneratePanel::setStatus (const juce::String& text, bool isError)
{
    statusLabel.setColour (juce::Label::textColourId, isError ? juce::Colour (0xffff7a7a) : theme::textDim);
    statusLabel.setText (text, juce::dontSendNotification);
    statusLabel.setTooltip (text);   // the footer shows three lines; the tooltip has all of it
}

//==============================================================================
void GeneratePanel::setMode (ai::Mode m, bool save)
{
    if (m != mode)
    {
        if (cancelFlag != nullptr)
            cancel();

        discardStaged();
        setStatus ({});   // what the last mode said (an apply, a failure) doesn't belong to this one
        mode = m;
    }

    for (int i = 0; i < 3; ++i)
        modeButtons[i].setToggleState (i == (int) mode, juce::dontSendNotification);

    updatePartBox();
    updatePlayingStyles();

    if (! isMode (ai::Mode::pattern) && ! modelsLoaded && isVisible())
        refreshModels();

    if (save)
        saveSettings();

    updateSetup();
}

bool GeneratePanel::stageCannedTakes()
{
    if (clip == nullptr)
        return false;

    discardStaged();
    generatingRequest = makeRequest();
    generatingRequest.part = gen::Part::melody;
    staging.begin (engine, *clip);
    cancelFlag = std::make_shared<std::atomic<bool>> (false);
    takesPart = gen::Part::melody;
    pendingTakes.clear();
    failedTakes = 0;
    takeError.clear();

    const int bars = juce::jmax (1, staging.getTarget().numBars);
    const Chord progression[] = { { 9, { 0, 3, 7 } }, { 5, { 0, 4, 7 } }, { 0, { 0, 4, 7 } }, { 7, { 0, 4, 7 } } };
    const int scale[] = { 69, 72, 76, 74, 72, 71, 67, 64 };

    for (int take = 0; take < 3; ++take)
    {
        Take made;
        made.score = 0.82 - 0.07 * take;
        made.result.summary = juce::String (8 * bars - take * 3) + " notes";

        for (int i = 0; i < 8 * bars - take * 3; ++i)
            made.result.notes.push_back ({ scale[(i + take * 2) % 8], i * 0.5, 0.5, 0.8f });

        for (int bar = 0; bar < bars; ++bar)
            made.chords.push_back (progression[bar % 4]);

        pendingTakes.push_back (made);
    }

    finishAi();
    return true;
}

bool GeneratePanel::setCannedSetup (const juce::String& name)
{
    gen::OllamaClient::ModelList list;
    list.reachable = name != "unreachable";
    modelChecks = {};

    if (! list.reachable)
        list.error = "Can't reach Ollama at " + getServerUrl();

    if (name == "notInstalled")
    {
        list.models = { { "qwen2.5:7b", "d-qwen", "qwen2" } };
        modelBox.setText ("llama3:8b", juce::dontSendNotification);
    }
    else if (name == "unchecked" || name == "failed")
    {
        list.models = { { "chatmusician-q4", "d-chat", "llama" }, { "qwen2.5:7b", "d-qwen", "qwen2" } };

        if (name == "failed")
            modelChecks.set ("d-chat", false, "2026-11-11");
    }
    else if (name != "unreachable" && name != "noModels")
        return false;

    ++modelRefreshId;   // a real answer that is still on its way is ignored
    applyModelList (list);

    if (name == "unchecked" || name == "failed")
    {
        chatModelBox.setSelectedId (1, juce::dontSendNotification);
        updateSetup();
    }

    return true;
}

void GeneratePanel::showSetup (bool show)
{
    setupShown = show;
    updateControls();
}

juce::String GeneratePanel::getAiModel() const
{
    if (isMode (ai::Mode::chatMusician))
    {
        const int id = chatModelBox.getSelectedId();
        return id >= 1 && id <= chatModelNames.size() ? chatModelNames[id - 1] : juce::String();
    }

    return modelBox.getText().trim();
}

void GeneratePanel::updateSetup()
{
    setupInfo = ai::describe (mode, modelList, getServerUrl(), getAiModel(), modelChecks);
    updateControls();
}

void GeneratePanel::refreshModels()
{
    const int refreshId = ++modelRefreshId;
    modelsLoaded = false;
    updateControls();

    const auto url = getServerUrl();
    juce::Thread::launch ([safeThis = juce::Component::SafePointer<GeneratePanel> (this), url, refreshId]
    {
        const auto list = gen::OllamaClient (url).listModelInfo();

        juce::MessageManager::callAsync ([safeThis, list, refreshId]
        {
            auto* self = safeThis.getComponent();

            if (self != nullptr && self->modelRefreshId == refreshId)
                self->applyModelList (list);
        });
    });
}

void GeneratePanel::applyModelList (const gen::OllamaClient::ModelList& list)
{
    ++modelRefreshId;   // a slower, older answer is ignored
    modelList = list;
    modelsLoaded = true;

    // The AI part's models.
    const auto names = list.getNames();
    const auto typed = modelBox.getText().trim();
    const auto wanted = typed.isNotEmpty() ? typed : settings (engine).getValue ("generator.model", defaultModel);
    modelBox.clear (juce::dontSendNotification);
    modelBox.addItemList (names, 1);

    if (! names.isEmpty())
        modelBox.setSelectedItemIndex (names.contains (wanted) ? names.indexOf (wanted) : juce::jmax (0, names.indexOf (defaultModel)),
                                       juce::dontSendNotification);
    if (typed.isNotEmpty() && ! names.contains (typed))
        modelBox.setText (typed, juce::dontSendNotification);

    // The ChatMusician models: those named so first, the others under a heading.
    const auto chosen = chatModelBox.getSelectedId() >= 1 && chatModelBox.getSelectedId() <= chatModelNames.size()
                            ? chatModelNames[chatModelBox.getSelectedId() - 1]
                            : settings (engine).getValue ("generator.chatMusicianModel");
    const auto ordered = ai::orderForChatMusician (list.models);
    chatModelBox.clear (juce::dontSendNotification);
    chatModelNames.clear();

    for (auto* group : { &ordered.likely, &ordered.others })
    {
        if (group == &ordered.others && ! group->empty())
            chatModelBox.addSectionHeading ("Other models (not ChatMusician by name)");

        for (auto& model : *group)
        {
            chatModelNames.add (model.name);
            chatModelBox.addItem (model.name, chatModelNames.size());
        }
    }

    const int index = chatModelNames.indexOf (chosen);
    chatModelBox.setSelectedId (index >= 0 ? index + 1 : 0, juce::dontSendNotification);
    updateSetup();
}

//==============================================================================
juce::String GeneratePanel::getPartId() const
{
    switch ((gen::Part) juce::jlimit (0, gen::numParts - 1, partBox.getSelectedId() - 1))
    {
        case gen::Part::chords:   return "chords";
        case gen::Part::arpeggio: return "arpeggio";
        case gen::Part::bassline: return "bassline";
        case gen::Part::melody:
        case gen::Part::drums:
        case gen::Part::percussion: break;
    }

    return {};
}

juce::String GeneratePanel::getPlayingStyleId() const
{
    const int id = playingBox.getSelectedId();
    return playingBox.isVisible() && id >= 2 ? playingIds[id - 2] : juce::String();
}

void GeneratePanel::updatePartBox()
{
    // Built-in patterns exist for chords, arpeggio and bassline only.
    const bool pattern = isMode (ai::Mode::pattern);

    for (int i = 0; i < gen::numParts; ++i)
    {
        const auto part = (gen::Part) i;
        const bool hasPattern = part == gen::Part::chords || part == gen::Part::arpeggio || part == gen::Part::bassline;
        partBox.setItemEnabled (i + 1, ! pattern || hasPattern);
    }

    const auto part = (gen::Part) juce::jlimit (0, gen::numParts - 1, partBox.getSelectedId() - 1);

    if (pattern && part != gen::Part::chords && part != gen::Part::arpeggio && part != gen::Part::bassline)
        partBox.setSelectedId (1, juce::dontSendNotification);
}

void GeneratePanel::updatePlayingStyles()
{
    namespace styles = gen::styles;
    const bool pattern = isMode (ai::Mode::pattern);
    const auto partId = getPartId();
    const auto wanted = settings (engine).getValue ("generator.playing." + partId + (pattern ? ".pattern" : ""),
                                                    pattern ? settings (engine).getValue ("generator.playing." + partId) : juce::String());
    const auto genre = styleBox.getText();

    playingBox.clear (juce::dontSendNotification);
    playingIds.clear();

    if (! pattern)
        playingBox.addItem ("Free (the model decides)", 1);

    for (auto instrument : styles::allInstruments)
    {
        bool heading = false;

        for (auto* style : styles::getStylesFor (instrument))
        {
            if (! style->parts.contains (partId))
                continue;

            if (! heading)
                playingBox.addSectionHeading (styles::getInstrumentName (instrument));

            heading = true;
            playingIds.add (style->id);
            playingBox.addItem (style->name + (style->suitsGenre (genre) ? juce::String::fromUTF8 ("  \xe2\x98\x85") : juce::String()),
                                playingIds.size() + 1);
        }
    }

    const int index = playingIds.indexOf (wanted);
    playingBox.setSelectedId (index >= 0 ? index + 2 : pattern && ! playingIds.isEmpty() ? 2 : 1, juce::dontSendNotification);

    const bool show = partId.isNotEmpty() && ! isMode (ai::Mode::chatMusician);
    playingBox.setVisible (show);
    playingLabel.setVisible (show);
}

juce::StringArray GeneratePanel::collectHarmony (int bars) const
{
    juce::StringArray perBar;

    for (int bar = 0; bar < bars; ++bar)
    {
        const double barStart = clip->startBeat + bar * engine.getBarLength(), barEnd = barStart + engine.getBarLength();
        std::array<bool, 12> sounding {};

        for (auto& t : engine.getTracks())
        {
            // Other MIDI tracks only, and not drum or percussion parts (their notes aren't pitches).
            if (t.get() == track || ! t->isMidi() || t->name.containsIgnoreCase ("drum") || t->name.containsIgnoreCase ("perc")
                || (t->usesBuiltInSynth() && builtInSounds::isPercussive (t->builtInSound)))
                continue;

            for (auto& c : t->clips)
            {
                auto& other = static_cast<const MidiClip&> (*c);

                if (other.muted || other.name.containsIgnoreCase ("drum") || other.name.containsIgnoreCase ("perc"))
                    continue;

                const double clipEnd = other.startBeat + other.lengthBeats;

                for (auto& n : other.notes)
                {
                    const double on = other.startBeat + n.start;
                    const double off = juce::jmin (on + n.length, clipEnd);

                    if (n.start < other.lengthBeats && on < barEnd && off > barStart)
                        sounding[(size_t) (n.pitch % 12)] = true;
                }
            }
        }

        juce::StringArray names;
        const int tonic = engine.getKey().tonic;

        for (int i = 0; i < 12; ++i)
            if (sounding[(size_t) ((tonic + i) % 12)])
                names.add (MusicalKey::getTonicName (tonic + i));

        perBar.add (names.joinIntoString (" "));
    }

    return perBar;
}

int GeneratePanel::getClipBars() const
{
    // Whole bars covering the clip; the result is trimmed to the clip's exact end.
    return clip != nullptr ? juce::jmax (1, (int) std::ceil (clip->lengthBeats / engine.getBarLength() - 1.0e-6)) : 4;
}

int GeneratePanel::getFirstBar() const
{
    return clip != nullptr ? songBarOf (engine.getBarLength(), *clip, 0) : 0;
}

juce::String GeneratePanel::describeBars() const
{
    std::vector<int> bars;

    for (int bar = 0; bar < getClipBars(); ++bar)
        bars.push_back (getFirstBar() + bar);

    return (bars.size() == 1 ? "bar " : "bars ") + formatBars (bars);
}

juce::String GeneratePanel::describeMissingChords() const
{
    std::vector<int> missing;

    for (int bar = 0; bar < getClipBars(); ++bar)
        if (engine.getChord (songBarOf (engine.getBarLength(), *clip, bar)) == nullptr)
            missing.push_back (getFirstBar() + bar);

    return juce::String (missing.size() == 1 ? "Bar " : "Bars ") + formatBars (missing) + (missing.size() == 1 ? " has" : " have")
           + " no chord. Add chords on the Chords row (right-click selected bars for a random progression), or use AI part.";
}

juce::String GeneratePanel::describeProposedChords() const
{
    const auto split = gen::staging::splitChordProposal (engine.getChords(), staging.getTarget().firstBar, staging.getTake().chords);
    std::vector<const Chord*> added;
    std::vector<int> addedBars;

    for (auto& [bar, chord] : split.added)
    {
        added.push_back (&chord);
        addedBars.push_back (bar);
    }

    juce::String text;

    if (added.empty())
        text = "Chords: every bar already has one, so none are added.";
    else
        text = "Chords: adds " + joinChordNames (added) + (addedBars.size() == 1 ? " to empty bar " : " to empty bars ") + formatBars (addedBars);

    if (! split.kept.empty())
    {
        juce::StringArray keeps;

        for (int bar : split.kept)
            if (auto* chord = engine.getChord (bar))
                keeps.add ("bar " + juce::String (bar + 1) + " keeps " + chord->getName());

        if (! added.empty())
            text << " (" << keeps.joinIntoString (", ") << ")";
    }

    return text;
}

gen::Request GeneratePanel::makeRequest() const
{
    gen::Request r;
    r.part = (gen::Part) juce::jlimit (0, gen::numParts - 1, partBox.getSelectedId() - 1);
    r.style = styleBox.getText().trim().isNotEmpty() ? styleBox.getText().trim() : juce::String ("Pop");
    r.bars = getClipBars();
    r.key = engine.getKey();
    r.bpm = engine.getTempo();
    r.timeSignature = engine.getTimeSignature();
    r.extraPrompt = isMode (ai::Mode::ai) ? promptEditor.getText() : juce::String();
    r.playingStyle = getPlayingStyleId();
    r.varyStyle = ! isMode (ai::Mode::pattern);   // a pattern is played as it is; the AI varies it

    for (int bar = 0; bar < r.bars; ++bar)
    {
        const auto* chord = engine.getChord (songBarOf (engine.getBarLength(), *clip, bar));
        r.chordTrack.push_back (chord != nullptr ? std::optional<Chord> (*chord) : std::nullopt);
    }

    if (! gen::isPercussive (r.part))
        r.harmonyPerBar = collectHarmony (r.getPatternBars());

    return r;
}

//==============================================================================
void GeneratePanel::generate()
{
    if (clip == nullptr || engine.getEditGeneration() != clipsEditGeneration || engine.findTrackFor (clip) == nullptr)
        return;

    saveSettings();
    appliedSteps = -1;

    if (isMode (ai::Mode::pattern))
        generatePattern();
    else if (isMode (ai::Mode::ai))
        generateAi();
    else
        generateChatMusician (false);
}

void GeneratePanel::generatePattern()
{
    // Never contacts Ollama: the notes come from DawIt's own library over the chord track.
    if (! hasChordOnEveryBar() || getPlayingStyleId().isEmpty())
        return;

    const auto request = makeRequest();
    discardStaged();
    const int id = staging.begin (engine, *clip);

    gen::Result result;
    juce::String error;

    if (! gen::realize (request, "{}", result, error))
    {
        setStatus (error, true);
        updateControls();
        return;
    }

    generatingRequest = request;
    takesPart = request.part;
    takes = { { result, {}, 1.0, {} } };
    shownTake = 0;
    bestTake = 0;

    juce::String clipName = request.style + " " + gen::getPartShortName (request.part);

    if (const auto* style = gen::styles::findStyle (request.playingStyle))
        clipName = style->name;

    stageResult (id, result.notes, {}, result.summary, clipName);
}

void GeneratePanel::restageIfShowing()
{
    if (isMode (ai::Mode::pattern) && staging.isStaged() && generateButton.isEnabled() && cancelFlag == nullptr)
    {
        // Updating the controls first lets the button's state follow the new choice.
        updateControls();

        if (generateButton.isEnabled())
            generatePattern();
    }
}

void GeneratePanel::generateAi()
{
    if (getAiModel().isEmpty())
        return;

    const auto request = makeRequest();
    discardStaged();
    const int id = staging.begin (engine, *clip);
    cancelFlag = std::make_shared<std::atomic<bool>> (false);
    generatingRequest = request;
    takesPart = request.part;
    pendingTakes.clear();
    failedTakes = 0;
    takeError.clear();
    progressText = "waiting for the model";
    spinnerPhase = 0;
    updateTimer();
    timerCallback();
    updateControls();

    const auto variation = variationSlider.getValue();
    const auto flag = cancelFlag;
    const auto safeThis = juce::Component::SafePointer<GeneratePanel> (this);
    const int takeCount = juce::jlimit (1, maxTakes, takesBox.getSelectedId());

    auto post = [safeThis, id] (const gen::Result& result, const juce::String& error, const juce::StringArray& directions)
    {
        juce::MessageManager::callAsync ([safeThis, id, result, error, directions]
        {
            if (auto* self = safeThis.getComponent())
                self->takeArrived (id, result, error, directions);
        });
    };

    auto done = [safeThis, id]
    {
        juce::MessageManager::callAsync ([safeThis, id]
        {
            if (auto* self = safeThis.getComponent())
                self->allTakesArrived (id);
        });
    };

    gen::OllamaClient::ChatOptions options;
    options.model = getAiModel();
    options.systemPrompt = gen::buildSystemPrompt (generatingRequest.timeSignature);
    options.schema = gen::buildSchema (generatingRequest);
    options.think = thinkToggle.getToggleState();
    options.maxTokens = options.think ? replyLimitWhenThinking : replyLimit;
    options.temperature = gen::getOllamaTemperature (variation);
    options.shouldStop = [flag] { return flag->load(); };

    const auto url = getServerUrl();

    // The requests run on their own thread so the UI (and audio) never wait for the model.
    // Ollama answers one request at a time anyway, so the takes are made one after the other.
    juce::Thread::launch ([options, request, url, flag, safeThis, id, takeCount, variation, post, done]() mutable
    {
        juce::Random random;
        bool couldNotThink = false;

        for (int take = 0; take < takeCount && ! flag->load(); ++take)
        {
            // A fresh seed and a few random ideas per take, so the takes differ from each other.
            auto takeRequest = request;
            takeRequest.directions = gen::pickDirections (request, gen::getDirectionCount (variation), random);

            auto takeOptions = options;
            takeOptions.userPrompt = gen::buildUserPrompt (takeRequest);
            takeOptions.seed = 1 + random.nextInt (std::numeric_limits<int>::max() - 1);
            takeOptions.think = options.think && ! couldNotThink;

            const auto prefix = takeCount > 1 ? "take " + juce::String (take + 1) + " of " + juce::String (takeCount) + ": "
                                              : juce::String();
            int lastReported = 0;

            auto report = [safeThis, id, &lastReported] (int chars, const juce::String& text)
            {
                if (chars - lastReported < 40)
                    return;

                lastReported = chars;
                juce::MessageManager::callAsync ([safeThis, id, text]
                {
                    if (auto* self = safeThis.getComponent(); self != nullptr && self->staging.getRequestId() == id)
                        self->progressText = text;
                });
            };

            takeOptions.onThinking = [report, prefix] (int chars) { report (chars, prefix + "thinking (" + juce::String (chars) + " characters so far)"); };
            takeOptions.onProgress = [report, prefix, &lastReported] (int chars)
            {
                if (chars < lastReported) // the answer starts after the thinking: count again from zero
                    lastReported = 0;

                report (chars, prefix + "received " + juce::String (chars) + " characters");
            };

            bool thinkingUnsupported = false;
            takeOptions.onThinkingUnsupported = [&thinkingUnsupported] { thinkingUnsupported = true; };

            juce::String error;
            const auto reply = gen::OllamaClient (url).chat (takeOptions, error);
            const bool chatFailed = error.isNotEmpty();
            gen::Result result;

            if (! chatFailed)
                gen::realize (takeRequest, reply, result, error);

            couldNotThink = couldNotThink || thinkingUnsupported;

            if (error.isEmpty() && couldNotThink)
                result.summary << " (" << options.model << " can't think first, so it answered straight away)";

            if (flag->load())
                return;

            post (result, error, takeRequest.directions);

            // Ollama itself failed (not running, model missing...): the other takes would fail the same way.
            if (chatFailed)
                break;
        }

        if (! flag->load())
            done();
    });
}

void GeneratePanel::generateChatMusician (bool checkOnly)
{
    if (clip == nullptr || engine.findTrackFor (clip) == nullptr)
        return;

    const auto model = getAiModel();
    const auto* info = modelList.find (model);

    if (model.isEmpty() || info == nullptr)
    {
        setStatus ("Choose an Ollama model.", true);
        return;
    }

    const bool chordTask = chatTask.getSelectedId() == 2;
    std::vector<Chord> source;
    std::vector<juce::StringArray> choices;
    const int bars = getClipBars();

    if (! checkOnly)
    {
        const auto chunks = gen::chatmusician::splitRange (clip->lengthBeats);

        if (chunks.empty() || std::abs (clip->startBeat / 4.0 - std::round (clip->startBeat / 4.0)) > 1.0e-9)
        {
            setStatus ("Choose a clip that starts on a bar line and contains whole 4/4 bars.", true);
            return;
        }

        for (int bar = 0; bar < bars && ! chordTask; ++bar)
        {
            const auto* chord = engine.getChord (songBarOf (engine.getBarLength(), *clip, bar));

            if (chord == nullptr)
            {
                setStatus ("Set a chord on every bar before generating a melody.", true);
                return;
            }

            source.push_back (*chord);
        }

        if (chordTask)
        {
            const auto vocabulary = gen::chatmusician::chordVocabulary (engine.getKey(), chatSevenths.getToggleState(),
                                                                        chatBorrowed.getToggleState(), chatDominant.getToggleState());
            choices.assign ((size_t) bars, vocabulary);
            const auto first = chatFirst.getText().trim(), last = chatLast.getText().trim();

            if ((first.isNotEmpty() && ! vocabulary.contains (first)) || (last.isNotEmpty() && ! vocabulary.contains (last))
                || (bars == 1 && first.isNotEmpty() && last.isNotEmpty() && first != last))
            {
                setStatus ("First and last chords must be in the selected vocabulary and agree for a one-bar clip.", true);
                return;
            }

            if (first.isNotEmpty()) choices.front() = { first };
            if (last.isNotEmpty()) choices.back() = { last };
        }
    }

    discardStaged();
    const int id = staging.begin (engine, *clip);
    cancelFlag = std::make_shared<std::atomic<bool>> (false);
    const auto flag = cancelFlag;
    const auto key = engine.getKey();
    const auto url = getServerUrl();
    const auto digest = info->digest;
    const bool needsCheck = checkOnly || setupInfo.state == ai::State::needsCheck || setupInfo.state == ai::State::checkFailed;
    const auto safe = juce::Component::SafePointer<GeneratePanel> (this);
    generatingRequest = {};
    generatingRequest.part = chordTask ? gen::Part::chords : gen::Part::melody;
    takesPart = generatingRequest.part;
    progressText = needsCheck ? "Checking the model with one test bar..." : "Waiting for the local model.";
    spinnerPhase = 0;
    updateTimer();
    timerCallback();
    updateControls();

    juce::Thread::launch ([safe, flag, key, url, model, digest, id, source, choices, chordTask, needsCheck, checkOnly]
    {
        juce::Random random;

        auto request = [flag, url, model, chordTask, &random] (const juce::String& prompt, const juce::var& schema, juce::String& requestError)
        {
            gen::OllamaClient::ChatOptions options;
            options.model = model; options.userPrompt = prompt; options.schema = schema;
            options.maxTokens = chordTask ? 256 : 384; options.temperature = chordTask ? 0.8 : 0.7;
            options.seed = 1 + random.nextInt (std::numeric_limits<int>::max() - 1);
            options.shouldStop = [flag] { return flag->load(); };
            return gen::OllamaClient (url).generateRaw (options, requestError);
        };

        const auto cancelled = [flag] { return flag->load(); };

        // The one-bar format check comes first for a model that has not passed it. Only a reply that arrives and
        // doesn't follow the format counts as a failure; a request that couldn't be made is just an error.
        if (needsCheck)
        {
            bool requestFailed = false;
            juce::String checkError;
            auto checkRequest = [&request, &requestFailed] (const juce::String& prompt, const juce::var& schema, juce::String& requestError)
            {
                auto reply = request (prompt, schema, requestError);
                requestFailed = requestError.isNotEmpty();
                return reply;
            };
            const bool passed = ai::runOneBarCheck (checkRequest, checkError);

            if (flag->load())
                return;

            juce::MessageManager::callAsync ([safe, flag, digest, passed, requestFailed, checkError, checkOnly]
            {
                auto* self = safe.getComponent();

                if (self == nullptr || self->cancelFlag != flag)
                    return;

                if (! requestFailed)
                    self->modelChecks.set (digest, passed);

                if (! passed || checkOnly)
                {
                    self->cancelFlag = nullptr;
                    self->updateTimer();
                    self->saveSettings();
                    self->updateSetup();
                    self->setStatus (passed ? juce::String ("The model passed the one-bar test.")
                                            : requestFailed ? checkError : "The model failed the one-bar test: " + checkError, ! passed);
                    return;
                }

                self->saveSettings();
                self->updateSetup();
                self->progressText = "Waiting for the local model.";
            });

            if (! passed || checkOnly)
                return;
        }

        juce::String error;
        std::vector<MidiNote> notes;
        std::vector<Chord> chords;
        bool ok = false;

        if (chordTask)
        {
            ok = gen::chatmusician::generateChords (key, choices, [&request] (const juce::String& p, const juce::var& s, int, juce::String& e) { return request (p, s, e); },
                                                    cancelled, chords, error);

            if (ok && ! gen::chatmusician::voiceChords (chords, 0.0, notes))
            {
                ok = false;
                error = "These chords could not be voiced.";
            }
        }
        else
        {
            ok = gen::chatmusician::generateMelody (key, source, request, cancelled, notes, error);
        }

        if (flag->load())
            return;

        juce::MessageManager::callAsync ([safe, flag, id, ok, notes, chords, error, chordTask]
        {
            auto* self = safe.getComponent();

            if (self == nullptr || self->cancelFlag != flag)
                return;

            self->cancelFlag = nullptr;
            self->updateTimer();

            if (! ok)
            {
                self->setStatus (error, true);
                self->updateControls();
                return;
            }

            self->takes = { { {}, chords, 1.0, {} } };
            self->shownTake = 0;
            self->stageResult (id, notes, chords, chordTask ? "A chord progression" : "A melody", {});
        });
    });
}

void GeneratePanel::cancel()
{
    if (cancelFlag != nullptr)
        *cancelFlag = true;

    // Nothing is staged, not even takes that already arrived.
    staging.cancel();
    pendingTakes.clear();
    cancelFlag = nullptr;
    updateTimer();
    setStatus ("Cancelled. The song is unchanged.");
    updateControls();
}

void GeneratePanel::updateTimer()
{
    if (cancelFlag != nullptr || previewing)
        startTimerHz (8);
    else
        stopTimer();
}

void GeneratePanel::timerCallback()
{
    if (previewing && (! engine.isPlaying() || engine.getPositionBeats() >= previewEnd))
    {
        stopPreview();
        updateControls();
    }

    if (cancelFlag != nullptr)
    {
        static const char* frames[] = { "|", "/", "-", "\\" };
        setStatus (juce::String ("Generating ") + gen::getPartName (generatingRequest.part).toLowerCase() + " with "
                   + getAiModel() + "  " + frames[spinnerPhase++ % 4] + "\n" + progressText);
    }
}

void GeneratePanel::takeArrived (int id, const gen::Result& result, const juce::String& error, const juce::StringArray& directions)
{
    if (id != staging.getRequestId() || cancelFlag == nullptr)
        return;

    if (error.isNotEmpty())
    {
        ++failedTakes;

        if (takeError.isEmpty())
            takeError = error;

        return;
    }

    // Proposed chords, one per bar: a bar the model left to the chord track keeps the chord that is there.
    std::vector<Chord> chords;
    const int firstBar = staging.getTarget().firstBar;

    for (size_t i = 0; i < result.newChords.size(); ++i)
    {
        if (result.newChords[i].has_value())
            chords.push_back (*result.newChords[i]);
        else if (const auto* existing = engine.getChord (firstBar + (int) i))
            chords.push_back (*existing);
        else
        {
            chords.clear();
            break;
        }
    }

    pendingTakes.push_back ({ result, chords, gen::scoreTake (generatingRequest, result), directions });
}

void GeneratePanel::allTakesArrived (int id)
{
    if (id == staging.getRequestId() && cancelFlag != nullptr)
        finishAi();
}

void GeneratePanel::finishAi()
{
    cancelFlag = nullptr;
    updateTimer();

    if (pendingTakes.empty())
    {
        setStatus (takeError.isNotEmpty() ? takeError : juce::String ("No usable take came back. Try again."), true);
        updateControls();
        return;
    }

    takes = std::move (pendingTakes);
    pendingTakes.clear();
    takesClipName = generatingRequest.style + " " + gen::getPartShortName (generatingRequest.part);

    if (const auto* style = generatingRequest.usesPlayingStyle() ? gen::styles::findStyle (generatingRequest.playingStyle) : nullptr)
        takesClipName = style->name;

    bestTake = (int) std::distance (takes.begin(), std::max_element (takes.begin(), takes.end(),
                                                                       [] (const Take& a, const Take& b) { return a.score < b.score; }));
    shownTake = -1;
    updateTakeButtons();
    stageTake (bestTake);

    if (failedTakes > 0 && staging.isStaged())
        setStatus (statusLabel.getText() + "\n" + juce::String (failedTakes) + (failedTakes == 1 ? " take" : " takes") + " failed: " + takeError);
}

void GeneratePanel::stageTake (int index)
{
    if (index < 0 || index >= (int) takes.size())
        return;

    const auto& take = takes[(size_t) index];
    shownTake = index;
    const auto summary = takes.size() > 1
        ? "Take " + juce::String (index + 1) + " of " + juce::String ((int) takes.size()) + " (score "
              + juce::String (juce::roundToInt (take.score * 100.0)) + (index == bestTake ? ", the best" : "") + "): " + take.result.summary + "."
        : take.result.summary + ".";

    if (! stageResult (staging.getRequestId(), take.result.notes, take.chords, summary, takesClipName))
    {
        takes.clear();
        updateTakeButtons();
        return;
    }

    for (int i = 0; i < takeButtons.size(); ++i)
        takeButtons[i]->setToggleState (i == index, juce::dontSendNotification);
}

bool GeneratePanel::stageResult (int id, const std::vector<MidiNote>& notes, const std::vector<Chord>& chords,
                                 const juce::String& summary, const juce::String& clipName)
{
    const auto outcome = staging.deliver (engine, id, notes, chords, replaceButton.getToggleState());

    switch (outcome)
    {
        case gen::staging::Staging::Outcome::oldRequest:
        case gen::staging::Staging::Outcome::cancelled:
            return false;

        case gen::staging::Staging::Outcome::stale:
            setStatus ("The clip or its chords changed while this take was waiting, so it was set aside. Generate again.", true);
            updateControls();
            return false;

        case gen::staging::Staging::Outcome::staged:
            break;
    }

    takesClipName = clipName;
    showOverlay();

    juce::String status = "Take ready, not applied.\nPreview, then Apply or Discard.\n" + summary;
    auto* targetTrack = findTrackFor (clip);

    if (targetTrack != nullptr && gen::isPercussive (takesPart) && targetTrack->usesBuiltInSynth()
        && ! builtInSounds::isPercussive (targetTrack->builtInSound))
        status << "\nTip: pick " << gen::getPartName (takesPart) << " in the DawIt Synth (click it in the device bar) so the hits play as "
               << gen::getPartShortName (takesPart) << ".";

    setStatus (status);
    updateControls();
    return true;
}

//==============================================================================
void GeneratePanel::showOverlay()
{
    if (! staging.isStaged() || clip == nullptr || engine.getEditGeneration() != clipsEditGeneration || engine.findTrackFor (clip) == nullptr)
        return;

    overlayClip = clip;
    overlayGeneration = engine.getEditGeneration();
    engine.setClipPreview (*clip, staging.getTake().notes, staging.isReplacing());

    if (onStagedChanged)
        onStagedChanged();
}

void GeneratePanel::clearOverlay()
{
    if (overlayClip == nullptr)
        return;

    if (engine.getEditGeneration() == overlayGeneration && engine.findTrackFor (overlayClip) != nullptr)
        engine.clearClipPreview (*overlayClip);

    overlayClip = nullptr;

    if (onStagedChanged)
        onStagedChanged();
}

void GeneratePanel::discardStaged (const juce::String& message, bool isError)
{
    const bool hadTake = staging.isStaged();
    stopPreview();
    clearOverlay();
    staging.discard();
    takes.clear();
    updateTakeButtons();

    if (message.isNotEmpty())
        setStatus (message, isError);
    else if (hadTake)
        setStatus ({});   // "Take ready" no longer holds

    updateControls();
}

void GeneratePanel::previewStaged()
{
    if (! staging.isStaged() || clip == nullptr)
        return;

    if (previewing)
    {
        stopPreview();
        updateControls();
        return;
    }

    if (staging.isStale (engine))
    {
        discardStaged ("The clip or its chords changed while this take was waiting, so it was set aside. Generate again.", true);
        return;
    }

    previewEnd = clip->startBeat + clip->lengthBeats;
    engine.setPositionBeats (clip->startBeat);
    engine.play();
    previewing = true;
    updateTimer();
    setStatus ("Previewing on the track's own instrument. The song is unchanged until you Apply.");
    updateControls();
}

void GeneratePanel::stopPreview()
{
    if (! previewing)
        return;

    previewing = false;
    engine.stop();
    updateTimer();
}

bool GeneratePanel::canUndoApply() const
{
    return cancelFlag == nullptr && appliedSteps >= 0 && engine.getNumUndoSteps() == appliedSteps && engine.getUndoName() == appliedName;
}

void GeneratePanel::applyStaged (Engine::GenerateScope scope)
{
    if (! staging.isStaged() || clip == nullptr || cancelFlag != nullptr)
        return;

    if (staging.isStale (engine))
    {
        discardStaged ("The clip or its chords changed while this take was waiting, so it was set aside. Generate again.", true);
        return;
    }

    stopPreview();

    auto* targetTrack = findTrackFor (clip);
    const auto proposal = gen::staging::splitChordProposal (engine.getChords(), staging.getTarget().firstBar, staging.getTake().chords);
    const auto trackName = targetTrack != nullptr ? targetTrack->name : juce::String();
    const auto stepName = "Generate " + gen::getPartName (takesPart).toLowerCase() + " in '" + trackName + "'";
    const auto notes = staging.getNotesAfterApply();
    const auto where = describeBars();
    const auto result = engine.applyGenerated (*clip, notes, proposal.added, scope, takesClipName, stepName);

    overlayClip = nullptr;   // the engine ended the preview with the step
    staging.discard();
    takes.clear();
    updateTakeButtons();

    if (onStagedChanged)
        onStagedChanged();

    if (! result.applied)
    {
        setStatus ("Nothing was applied: the clip is no longer in the song.", true);
        updateControls();
        return;
    }

    appliedSteps = result.undoSteps;
    appliedName = result.stepName;

    juce::String status = "Applied to " + trackName + ", " + where + ".\n" + undoKeys + " or Undo apply takes it back.";

    if (result.chordsAdded > 0)
        status << "\nAdded " << result.chordsAdded << (result.chordsAdded == 1 ? " chord" : " chords") << " to the chord track.";

    if (result.chordsKept > 0)
        status << " " << result.chordsKept << (result.chordsKept == 1 ? " bar kept its chord." : " bars kept their chords.");

    setStatus (status);
    updateControls();
}

void GeneratePanel::undoApply()
{
    if (! canUndoApply())
        return;

    appliedSteps = -1;

    if (onUndoApply)
        onUndoApply();

    setStatus ("Undone. The song is as it was before Apply.");
    updateControls();
}

void GeneratePanel::updateTakeButtons()
{
    takeButtons.clear();

    if (takes.size() > 1)
    {
        for (int i = 0; i < (int) takes.size(); ++i)
        {
            const auto& take = takes[(size_t) i];
            auto* b = takeButtons.add (new juce::TextButton ("Take " + juce::String (i + 1) + " ("
                                                             + juce::String (juce::roundToInt (take.score * 100.0)) + ")"));
            b->setColour (juce::TextButton::buttonOnColourId, theme::accent.withAlpha (0.7f));
            b->setToggleState (i == shownTake, juce::dontSendNotification);

            auto tooltip = take.result.summary + "\nScore " + juce::String (juce::roundToInt (take.score * 100.0)) + " of 100"
                           + (i == bestTake ? ", the best of these takes." : ".");
            if (! take.directions.isEmpty())
                tooltip << "\nIdeas it was given: " << take.directions.joinIntoString ("; ");

            b->setTooltip (tooltip);
            b->onClick = [this, i] { stageTake (i); };
            addAndMakeVisible (b);
        }
    }

    resized();
}
} // namespace daw
