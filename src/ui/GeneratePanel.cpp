#include "GeneratePanel.h"
#include "engine/Tempo.h"
#include "generate/ChordStyles.h"
#include "generate/ChatMusician.h"
#include "generate/OllamaClient.h"
#include "generate/PromptBuilder.h"
#include "generate/Takes.h"

namespace daw
{
namespace
{
    constexpr const char* defaultModel = "qwen2.5:7b";
    constexpr int defaultTakes = 3, maxTakes = 4;
    constexpr int labelWidth = 62;

    // Reply limits in tokens. Thinking comes out of the same budget, so it gets far more room.
    constexpr int replyLimit = 4096, replyLimitWhenThinking = 16384;

    juce::PropertiesFile& settings (Engine& engine) { return *engine.properties.getUserSettings(); }

    /** The song bar (of the chord track) where a bar of the clip starts. */
    int songBarOf (const MidiClip& clip, int clipBar)
    {
        return (int) std::floor (clip.startBeat / 4.0 + clipBar + 1.0e-6);
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
}

GeneratePanel::GeneratePanel (Engine& e) : engine (e)
{
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&form, false);
    viewport.setScrollBarsShown (true, false);

    title.setText ("Generate", juce::dontSendNotification);
    title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    form.addAndMakeVisible (title);

    infoLabel.setColour (juce::Label::textColourId, theme::textDim);
    infoLabel.setFont (juce::FontOptions (12.0f));
    form.addAndMakeVisible (infoLabel);

    closeButton.setTooltip ("Close");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    form.addAndMakeVisible (closeButton);

    chatMode.setTooltip ("Use the ChatMusician model's limited melody or chord generator.");
    chatTask.addItem ("Melody over song chords", 1);
    chatTask.addItem ("Chord progression", 2);
    chatTask.setSelectedId (1);
    chatTask.setTooltip ("Choose whether to write a melody or a chord progression.");
    chatSevenths.setTooltip ("Allow seventh chords as well as triads.");
    chatBorrowed.setTooltip ("Allow triads borrowed from the parallel major or minor key.");
    chatDominant.setTooltip ("In a minor key, allow a major dominant chord.");
    chatFirst.setTextToShowWhenEmpty ("First chord (optional)", theme::textDim);
    chatLast.setTextToShowWhenEmpty ("Last chord (optional)", theme::textDim);
    chatFirst.setTooltip ("Optional required first chord, such as Am.");
    chatLast.setTooltip ("Optional required last chord, such as E7.");
    chatPreview.setTooltip ("Listen to the staged notes using a simple preview sound; the song is unchanged.");
    chatApplyNotes.setTooltip ("Put the staged MIDI notes in the selected clip.");
    chatApplyChords.setTooltip ("Put proposed chords into empty bars on the chord track.");
    chatUndo.setTooltip ("Undo the last ChatMusician application if the affected notes and chords have not changed.");
    chatGuide.setTooltip ("Open the reading music guide, including ABC and generated music.");
    chatMode.onClick = [this] { saveSettings(); updateControls(); resized(); };
    chatTask.onChange = [this] { saveSettings(); updateControls(); resized(); };
    chatPreview.onClick = [this] { previewChatMusician(); };
    chatApplyNotes.onClick = [this] { applyChatMusician (false); };
    chatApplyChords.onClick = [this] { applyChatMusician (true); };
    chatUndo.onClick = [this] { undoChatMusician(); };
    chatGuide.onClick = [this] { if (onShowReadingGuide) onShowReadingGuide(); };
    for (juce::Component* c : { (juce::Component*) &chatMode, (juce::Component*) &chatTask, (juce::Component*) &chatSevenths,
                                (juce::Component*) &chatBorrowed, (juce::Component*) &chatDominant, (juce::Component*) &chatFirst,
                                (juce::Component*) &chatLast, (juce::Component*) &chatPreview, (juce::Component*) &chatApplyNotes,
                                (juce::Component*) &chatApplyChords, (juce::Component*) &chatUndo, (juce::Component*) &chatGuide })
        form.addAndMakeVisible (c);

    for (auto* l : { &partLabel, &styleLabel, &promptLabel, &modelLabel, &variationLabel, &playingLabel })
    {
        l->setColour (juce::Label::textColourId, theme::textDim);
        form.addAndMakeVisible (l);
    }

    for (int i = 0; i < gen::numParts; ++i)
        partBox.addItem (gen::getPartName ((gen::Part) i), i + 1);

    styleBox.addItemList (gen::getStylePresets(), 1);
    styleBox.setEditableText (true);
    styleBox.setTooltip ("Pick a style or type your own");

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (true);
    promptEditor.setTextToShowWhenEmpty ("Optional: mood, rhythm, references...", theme::textDim);
    promptEditor.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    promptEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);

    modelBox.setTooltip ("Installed Ollama models");
    modelBox.setEditableText (true);
    refreshButton.setTooltip ("Reload the list of models from Ollama");
    refreshButton.onClick = [this] { refreshModels(); };

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
    takesBox.setTooltip ("How many versions to make per click. They are scored and the best goes in the clip; "
                         "the take buttons below swap in the others. More takes take longer.");

    playingBox.setTooltip (juce::String::fromUTF8 ("How the part is played: a piano, guitar, synth, strings or bass style from DawIt's library, "
                                                   "following the chord track. \xe2\x98\x85 marks styles typical for the chosen style of music. "
                                                   "Free lets the model write the part itself."));
    varyToggle.setTooltip ("On: the model writes a variation of the playing style's pattern (Variation sets how far it may go). "
                           "Off: the style is played as it is, straight away and without the model, unless bars still need chords.");

    replaceToggle.setTooltip ("Off: add the generated notes to the ones already in the clip");
    thinkToggle.setTooltip ("Let the model reason before it answers: often more musical, but several times slower. "
                            "Only for models that can think, such as qwen3 and gemma4; others answer straight away.");
    generateButton.setColour (juce::TextButton::buttonColourId, theme::accent.withAlpha (0.7f));
    generateButton.onClick = [this] { cancelFlag != nullptr ? cancel() : generate(); };
    undoButton.setTooltip ("Restore the clip (and the chord track) as it was before the last generation");
    undoButton.onClick = [this] { undo(); };

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setJustificationType (juce::Justification::topLeft);
    statusLabel.setMinimumHorizontalScale (1.0f);

    for (juce::Component* c : { (juce::Component*) &partBox, (juce::Component*) &styleBox,
                                (juce::Component*) &modelBox, (juce::Component*) &promptEditor, (juce::Component*) &refreshButton,
                                (juce::Component*) &variationSlider, (juce::Component*) &takesBox,
                                (juce::Component*) &replaceToggle, (juce::Component*) &thinkToggle, (juce::Component*) &generateButton,
                                (juce::Component*) &undoButton, (juce::Component*) &statusLabel, (juce::Component*) &playingBox,
                                (juce::Component*) &varyToggle })
        form.addAndMakeVisible (c);

    // Restore the last-used settings.
    auto& s = settings (engine);
    chatMode.setToggleState (s.getBoolValue ("generator.chatMusician", false), juce::dontSendNotification);
    chatTask.setSelectedId (juce::jlimit (1, 2, s.getIntValue ("generator.chatTask", 1)), juce::dontSendNotification);
    partBox.setSelectedId (juce::jlimit (1, gen::numParts, s.getIntValue ("generator.part", 1)), juce::dontSendNotification);
    styleBox.setText (s.getValue ("generator.style", "Pop"), juce::dontSendNotification);
    replaceToggle.setToggleState (s.getBoolValue ("generator.replace", true), juce::dontSendNotification);
    thinkToggle.setToggleState (s.getBoolValue ("generator.think", false), juce::dontSendNotification);
    variationSlider.setValue (juce::jlimit (0.0, 1.0, s.getDoubleValue ("generator.variation", gen::defaultVariation)), juce::dontSendNotification);
    takesBox.setSelectedId (juce::jlimit (1, maxTakes, s.getIntValue ("generator.takes", defaultTakes)), juce::dontSendNotification);
    varyToggle.setToggleState (s.getBoolValue ("generator.varyStyle", true), juce::dontSendNotification);
    updatePlayingStyles();

    // The part and the style of music change which playing styles are offered (and marked).
    for (auto* box : { &partBox, &styleBox })
        box->onChange = [this] { updatePlayingStyles(); saveSettings(); updateControls(); };

    for (auto* box : { &modelBox, &playingBox })
        box->onChange = [this] { saveSettings(); updateControls(); };

    varyToggle.onClick = [this] { saveSettings(); updateControls(); };
    replaceToggle.onClick = [this] { saveSettings(); };
    thinkToggle.onClick = [this] { saveSettings(); };
    variationSlider.onValueChange = [this] { settings (engine).setValue ("generator.variation", variationSlider.getValue()); };
    takesBox.onChange = [this] { saveSettings(); };

    engine.addChangeListener (this);
    updateControls();

}

GeneratePanel::~GeneratePanel()
{
    engine.removeChangeListener (this);

    if (chatAuditioning && engine.isAuditioning())
        engine.stopAudition();

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
    s.setValue ("generator.part", partBox.getSelectedId());
    s.setValue ("generator.style", styleBox.getText());
    s.setValue ("generator.replace", replaceToggle.getToggleState());
    s.setValue ("generator.think", thinkToggle.getToggleState());
    s.setValue ("generator.variation", variationSlider.getValue());
    s.setValue ("generator.takes", takesBox.getSelectedId());
    s.setValue ("generator.varyStyle", varyToggle.getToggleState());
    s.setValue ("generator.chatMusician", chatMode.getToggleState());
    s.setValue ("generator.chatTask", chatTask.getSelectedId());

    if (getPartId().isNotEmpty())
        s.setValue ("generator.playing." + getPartId(), getPlayingStyleId());

    if (modelBox.getText().isNotEmpty() && modelsLoaded)
        s.setValue ("generator.model", modelBox.getText());

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
        if (chatAuditioning) engine.stopAudition();
        chatAuditioning = false;
        chatStaged = false;
    }
    track = t;
    clip = c;
    updateControls();
}

void GeneratePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Song key or tempo may have changed; the clip may have been deleted.
    if (undoClip != nullptr && findTrackFor (undoClip) == nullptr)
    {
        undoClip = nullptr;
        takes.clear();
        updateTakeButtons();
    }

    updateControls();
}

void GeneratePanel::visibilityChanged()
{
    if (isVisible() && ! modelsLoaded)
        refreshModels();
}

//==============================================================================
void GeneratePanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, 0, 1, getHeight());
}

void GeneratePanel::resized()
{
    viewport.setBounds (getLocalBounds().withTrimmedLeft (1));

    const int width = viewport.getMaximumVisibleWidth();
    auto r = juce::Rectangle<int> (0, 0, width, 1000).reduced (10, 6);

    auto top = r.removeFromTop (24);
    closeButton.setBounds (top.removeFromRight (22).reduced (0, 1));
    title.setBounds (top);
    infoLabel.setBounds (r.removeFromTop (48));
    infoLabel.setJustificationType (juce::Justification::topLeft);
    r.removeFromTop (6);

    chatMode.setBounds (r.removeFromTop (22));
    if (chatMode.getToggleState())
    {
        chatTask.setBounds (r.removeFromTop (24));
        if (chatTask.getSelectedId() == 2)
        {
            chatSevenths.setBounds (r.removeFromTop (22));
            chatBorrowed.setBounds (r.removeFromTop (22));
            chatDominant.setBounds (r.removeFromTop (22));
            chatFirst.setBounds (r.removeFromTop (24));
            chatLast.setBounds (r.removeFromTop (24));
        }
        auto modelRow = r.removeFromTop (24);
        modelLabel.setBounds (modelRow.removeFromLeft (labelWidth));
        refreshButton.setBounds (modelRow.removeFromRight (62));
        modelRow.removeFromRight (4);
        modelBox.setBounds (modelRow);
        replaceToggle.setBounds (r.removeFromTop (22));
        generateButton.setBounds (r.removeFromTop (28));
        r.removeFromTop (4);
        auto actions = r.removeFromTop (26);
        chatPreview.setBounds (actions.removeFromLeft (70));
        chatApplyNotes.setBounds (actions.removeFromLeft (90));
        chatApplyChords.setBounds (actions);
        chatUndo.setBounds (r.removeFromTop (24));
        chatGuide.setBounds (r.removeFromTop (24));
        statusLabel.setBounds (r.removeFromTop (72));
        form.setSize (width, statusLabel.getBottom() + 6);
        return;
    }

    auto row = [&r] (juce::Label& label, juce::Component& control, int height = 24)
    {
        auto line = r.removeFromTop (height);
        label.setBounds (line.removeFromLeft (labelWidth));
        control.setBounds (line);
        r.removeFromTop (5);
    };

    row (partLabel, partBox);
    row (styleLabel, styleBox);

    if (playingBox.isVisible())
        row (playingLabel, playingBox);

    if (varyToggle.isVisible())
    {
        varyToggle.setBounds (r.removeFromTop (22).withTrimmedLeft (labelWidth - 4));
        r.removeFromTop (5);
    }

    {
        auto line = r.removeFromTop (24);
        modelLabel.setBounds (line.removeFromLeft (labelWidth));
        refreshButton.setBounds (line.removeFromRight (62));
        line.removeFromRight (4);
        modelBox.setBounds (line);
        r.removeFromTop (5);
    }

    row (promptLabel, promptEditor, 46);

    {
        auto line = r.removeFromTop (24);
        variationLabel.setBounds (line.removeFromLeft (labelWidth));
        takesBox.setBounds (line.removeFromRight (82));
        line.removeFromRight (4);
        variationSlider.setBounds (line);
        r.removeFromTop (5);
    }

    replaceToggle.setBounds (r.removeFromTop (22).withTrimmedLeft (labelWidth - 4));
    thinkToggle.setBounds (r.removeFromTop (22).withTrimmedLeft (labelWidth - 4));
    r.removeFromTop (6);

    auto buttons = r.removeFromTop (28);
    undoButton.setBounds (buttons.removeFromRight (64));
    buttons.removeFromRight (6);
    generateButton.setBounds (buttons);
    r.removeFromTop (6);

    if (! takeButtons.isEmpty())
    {
        auto line = r.removeFromTop (24);
        const int buttonWidth = (line.getWidth() - 4 * (takeButtons.size() - 1)) / takeButtons.size();

        for (auto* b : takeButtons)
        {
            b->setBounds (line.removeFromLeft (buttonWidth));
            line.removeFromLeft (4);
        }

        r.removeFromTop (6);
    }

    statusLabel.setBounds (r.removeFromTop (72));
    form.setSize (width, statusLabel.getBottom() + 6);
}

//==============================================================================
void GeneratePanel::updateControls()
{
    const bool running = cancelFlag != nullptr;
    const bool chat = chatMode.getToggleState();
    const bool chordTask = chatTask.getSelectedId() == 2;
    for (juce::Component* c : { (juce::Component*) &chatTask, (juce::Component*) &chatPreview, (juce::Component*) &chatApplyNotes,
                                (juce::Component*) &chatUndo, (juce::Component*) &chatGuide }) c->setVisible (chat);
    for (juce::Component* c : { (juce::Component*) &chatSevenths, (juce::Component*) &chatBorrowed, (juce::Component*) &chatDominant,
                                (juce::Component*) &chatFirst, (juce::Component*) &chatLast, (juce::Component*) &chatApplyChords })
        c->setVisible (chat && chordTask);
    for (juce::Component* c : { (juce::Component*) &partLabel, (juce::Component*) &partBox, (juce::Component*) &styleLabel,
                                (juce::Component*) &styleBox,
                                (juce::Component*) &promptLabel, (juce::Component*) &promptEditor, (juce::Component*) &variationLabel,
                                (juce::Component*) &variationSlider, (juce::Component*) &takesBox, (juce::Component*) &thinkToggle,
                                (juce::Component*) &undoButton })
        c->setVisible (! chat);
    playingLabel.setVisible (! chat && getPartId().isNotEmpty());
    playingBox.setVisible (! chat && getPartId().isNotEmpty());
    chatMode.setEnabled (! running);
    chatTask.setEnabled (! running);
    for (juce::Component* c : { (juce::Component*) &chatSevenths, (juce::Component*) &chatBorrowed,
                                (juce::Component*) &chatDominant, (juce::Component*) &chatFirst, (juce::Component*) &chatLast })
        c->setEnabled (! running);
    chatPreview.setEnabled (! running && chatStaged);
    chatApplyNotes.setEnabled (! running && chatStaged && ! chatAppliedMidi);
    chatApplyChords.setEnabled (! running && chatStaged && ! chatChords.empty() && ! chatAppliedLane);
    chatUndo.setEnabled (! running && (chatAppliedMidi || chatAppliedLane));
    const bool haveClip = clip != nullptr && engine.findTrackFor (clip) != nullptr;

    infoLabel.setText (engine.getKey().getName() + "   " + formatBpm (engine.getTempo()) + " BPM\n"
                           + (haveClip ? "Into: " + clip->name + " (" + describeClipLength() + ")\n" + describeChordTrack()
                                       : juce::String ("No clip selected")),
                       juce::dontSendNotification);

    // A playing style played as it is needs no model, so the model's settings don't apply.
    const bool withoutModel = ! chat && haveClip && playsWithoutModel();
    const bool showVary = ! chat && playingBox.isVisible() && getPlayingStyleId().isNotEmpty();

    if (varyToggle.isVisible() != showVary)
    {
        varyToggle.setVisible (showVary);
        resized();
    }

    generateButton.setButtonText (running ? "Cancel" : withoutModel ? "Play style" : "Generate");
    generateButton.setEnabled (running || (haveClip && (withoutModel || modelBox.getText().isNotEmpty())));
    undoButton.setEnabled (! running && undoClip != nullptr);

    for (juce::Component* c : { (juce::Component*) &partBox, (juce::Component*) &styleBox, (juce::Component*) &playingBox,
                                (juce::Component*) &varyToggle, (juce::Component*) &refreshButton })
        c->setEnabled (! running && (c == &refreshButton || ! chat));

    for (juce::Component* c : { (juce::Component*) &promptEditor, (juce::Component*) &modelBox, (juce::Component*) &thinkToggle,
                                (juce::Component*) &variationSlider, (juce::Component*) &takesBox })
        c->setEnabled (! running && ! withoutModel && (c == &modelBox || ! chat));

    for (auto* b : takeButtons)
    {
        b->setVisible (! chat);
        b->setEnabled (! running);
    }
}

void GeneratePanel::setStatus (const juce::String& text, bool isError)
{
    statusLabel.setColour (juce::Label::textColourId, isError ? juce::Colour (0xffff7a7a) : theme::textDim);
    statusLabel.setText (text, juce::dontSendNotification);
}

void GeneratePanel::refreshModels()
{
    const int refreshId = ++modelRefreshId;
    modelsLoaded = false;
    modelBox.clear (juce::dontSendNotification);
    updateControls();
    setStatus ("Looking for Ollama models...");

    const auto url = getServerUrl();
    juce::Thread::launch ([safeThis = juce::Component::SafePointer<GeneratePanel> (this), url, refreshId]
    {
        juce::String error;
        const auto models = gen::OllamaClient (url).listModels (error);

        juce::MessageManager::callAsync ([safeThis, models, error, refreshId]
        {
            auto* self = safeThis.getComponent();

            if (self == nullptr || self->modelRefreshId != refreshId)
                return;

            self->modelsLoaded = true;

            const auto typed = self->modelBox.getText().trim();
            const auto wanted = typed.isNotEmpty() ? typed : settings (self->engine).getValue ("generator.model", defaultModel);
            self->modelBox.clear (juce::dontSendNotification);
            self->modelBox.addItemList (models, 1);

            const int index = models.contains (wanted) ? models.indexOf (wanted)
                                                       : juce::jmax (0, models.indexOf (defaultModel));

            if (! models.isEmpty())
                self->modelBox.setSelectedItemIndex (index, juce::dontSendNotification);
            if (typed.isNotEmpty() && ! models.contains (typed))
                self->modelBox.setText (typed, juce::dontSendNotification);

            if (error.isNotEmpty())
                self->setStatus (error, true);
            else if (models.isEmpty())
                self->setStatus ("No chat models installed. In a terminal, run: ollama pull " + juce::String (defaultModel), true);
            else
                self->setStatus (juce::String (models.size()) + " model(s) available.");

            self->updateControls();
        });
    });
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

void GeneratePanel::updatePlayingStyles()
{
    namespace styles = gen::styles;
    const auto partId = getPartId();
    const auto wanted = settings (engine).getValue ("generator.playing." + partId);
    const auto genre = styleBox.getText();

    playingBox.clear (juce::dontSendNotification);
    playingIds.clear();
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
    playingBox.setSelectedId (index >= 0 ? index + 2 : 1, juce::dontSendNotification);

    const bool show = partId.isNotEmpty();
    playingBox.setVisible (show);
    playingLabel.setVisible (show);
    varyToggle.setVisible (show && getPlayingStyleId().isNotEmpty());
    resized();
}

bool GeneratePanel::playsWithoutModel() const
{
    if (clip == nullptr || getPlayingStyleId().isEmpty() || varyToggle.getToggleState())
        return false;

    for (int bar = 0; bar < getClipBars(); ++bar)
        if (engine.getChord (songBarOf (*clip, bar)) == nullptr)
            return false;

    return true;
}

juce::StringArray GeneratePanel::collectHarmony (int bars) const
{
    juce::StringArray perBar;

    for (int bar = 0; bar < bars; ++bar)
    {
        const double barStart = clip->startBeat + bar * 4.0, barEnd = barStart + 4.0;
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
    return clip != nullptr ? juce::jmax (1, (int) std::ceil (clip->lengthBeats / 4.0 - 1.0e-6)) : 4;
}

juce::String GeneratePanel::describeClipLength() const
{
    const double bars = clip->lengthBeats / 4.0;
    const bool whole = std::abs (bars - std::round (bars)) < 1.0e-6;
    const auto text = whole ? juce::String ((int) std::round (bars)) : juce::String (bars, 2);
    return text + (whole && (int) std::round (bars) == 1 ? " bar" : " bars");
}

juce::String GeneratePanel::describeChordTrack() const
{
    std::vector<const Chord*> chords;

    for (int bar = 0; bar < getClipBars(); ++bar)
        chords.push_back (engine.getChord (songBarOf (*clip, bar)));

    if (std::all_of (chords.begin(), chords.end(), [] (auto* c) { return c == nullptr; }))
        return "No chords here yet: generated chords go on the chord track";

    return "Chords: " + joinChordNames (chords);
}

gen::Request GeneratePanel::makeRequest() const
{
    gen::Request r;
    r.part = (gen::Part) juce::jlimit (0, gen::numParts - 1, partBox.getSelectedId() - 1);
    r.style = styleBox.getText().trim().isNotEmpty() ? styleBox.getText().trim() : juce::String ("Pop");
    r.bars = getClipBars();
    r.key = engine.getKey();
    r.bpm = engine.getTempo();
    r.extraPrompt = promptEditor.getText();
    r.playingStyle = getPlayingStyleId();
    r.varyStyle = varyToggle.getToggleState();

    for (int bar = 0; bar < r.bars; ++bar)
    {
        const auto* chord = engine.getChord (songBarOf (*clip, bar));
        r.chordTrack.push_back (chord != nullptr ? std::optional<Chord> (*chord) : std::nullopt);
    }

    if (! gen::isPercussive (r.part))
        r.harmonyPerBar = collectHarmony (r.getPatternBars());

    return r;
}

void GeneratePanel::playStyle (const gen::Request& request)
{
    gen::Result result;
    juce::String error;

    if (! gen::realize (request, "{}", result, error))
    {
        setStatus (error, true);
        return;
    }

    ++generationId;
    generatingClip = clip;
    clipsEditGeneration = engine.getEditGeneration();
    generatingRequest = request;
    pendingTakes = { { result, 1.0, {} } };
    failedTakes = 0;
    takeError.clear();
    finishGeneration (false);
}

void GeneratePanel::generate()
{
    if (chatMode.getToggleState())
    {
        generateChatMusician();
        return;
    }
    if (clip == nullptr || engine.findTrackFor (clip) == nullptr)
        return;

    saveSettings();
    const auto request = makeRequest();

    // A playing style as it is, over the chord track's chords: no model needed.
    if (! request.needsModel())
    {
        playStyle (request);
        return;
    }

    if (modelBox.getText().isEmpty())
        return;

    const int id = ++generationId;
    cancelFlag = std::make_shared<std::atomic<bool>> (false);
    generatingClip = clip;
    clipsEditGeneration = engine.getEditGeneration();
    generatingRequest = request;
    pendingTakes.clear();
    failedTakes = 0;
    takeError.clear();
    progressText = "waiting for the model";
    spinnerPhase = 0;
    startTimerHz (8);
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
    options.model = modelBox.getText();
    options.systemPrompt = gen::buildSystemPrompt();
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
                    if (auto* self = safeThis.getComponent(); self != nullptr && self->generationId == id)
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

bool GeneratePanel::chatSnapshotValid() const
{
    if (chatClip == nullptr || engine.getEditGeneration() != chatEditGeneration || engine.findTrackFor (chatClip) == nullptr
        || clip != chatClip || ! juce::exactlyEqual (chatClip->startBeat, chatStart)
        || ! juce::exactlyEqual (chatClip->lengthBeats, chatLength) || ! (engine.getKey() == chatKey))
        return false;
    const auto& expected = chatAppliedMidi ? chatAppliedNotes : chatOriginalNotes;
    if (chatClip->notes.size() != expected.size()) return false;
    for (size_t i = 0; i < expected.size(); ++i)
    {
        const auto& a = chatClip->notes[i];
        const auto& b = expected[i];
        if (a.pitch != b.pitch || ! juce::exactlyEqual (a.start, b.start) || ! juce::exactlyEqual (a.length, b.length)
            || ! juce::exactlyEqual (a.velocity, b.velocity)) return false;
    }
    return engine.getChords() == (chatAppliedLane ? chatAppliedChords : chatOriginalChords);
}

void GeneratePanel::previewChatMusician()
{
    if (! chatStaged || ! chatSnapshotValid()) { setStatus ("The clip or song changed. Generate again.", true); return; }
    if (chatAuditioning && engine.isAuditioning()) { engine.stopAudition(); chatAuditioning = false; return; }
    if (engine.isPlaying()) { setStatus ("Stop playback before previewing a take.", true); return; }
    const double rate = engine.getSampleRate();
    const double seconds = chatLength * 60.0 / engine.getTempo();
    if (rate <= 0.0 || seconds <= 0.0 || seconds > 120.0)
    {
        setStatus ("Preview needs an audio device and is limited to two minutes. Shorten the clip to listen.", true);
        return;
    }
    juce::AudioBuffer<float> buffer (2, (int) std::ceil (seconds * rate));
    buffer.clear();
    const double samplesPerBeat = rate * 60.0 / engine.getTempo();
    const float gain = chatChords.empty() ? 0.15f : 0.035f;
    for (const auto& note : chatNotes)
    {
        const int begin = juce::jlimit (0, buffer.getNumSamples(), (int) std::round (note.start * samplesPerBeat));
        const int end = juce::jlimit (begin, buffer.getNumSamples(), (int) std::round ((note.start + note.length) * samplesPerBeat));
        const double frequency = 440.0 * std::pow (2.0, (note.pitch - 69) / 12.0);
        for (int sample = begin; sample < end; ++sample)
        {
            const double elapsed = (sample - begin) / rate;
            const double remaining = (end - sample) / rate;
            const float envelope = (float) juce::jmin (1.0, juce::jmin (elapsed / 0.01, remaining / 0.03));
            const float value = gain * envelope * (float) std::sin (juce::MathConstants<double>::twoPi * frequency * elapsed);
            buffer.addSample (0, sample, value); buffer.addSample (1, sample, value);
        }
    }
    engine.startAudition (std::move (buffer));
    chatAuditioning = true;
    setStatus ("Previewing with a simple tone. The selected track's instrument is unchanged.");
}

void GeneratePanel::applyChatMusician (bool chordsOnly)
{
    if (! chatStaged || ! chatSnapshotValid()) { setStatus ("The clip or song changed. Generate again.", true); return; }
    if (chatAuditioning) engine.stopAudition();
    chatAuditioning = false;
    if (chordsOnly)
    {
        if (chatChords.empty() || chatAppliedLane) return;
        auto updated = engine.getChords();
        for (int bar = 0; bar < (int) chatChords.size(); ++bar)
            if (updated.count (songBarOf (*chatClip, bar)) != 0)
            {
                setStatus ("The chord track already has a chord in this range. Clear it before applying these chords.", true);
                return;
            }
        for (int bar = 0; bar < (int) chatChords.size(); ++bar)
            updated[songBarOf (*chatClip, bar)] = chatChords[(size_t) bar];
        chatAppliedChords = updated;
        chatAppliedLane = true;
        engine.setChords (std::move (updated));
        setStatus ("Applied chords to the chord track. MIDI notes are a separate choice.");
    }
    else
    {
        if (chatAppliedMidi) return;
        chatAppliedNotes = replaceToggle.getToggleState() ? chatNotes : chatOriginalNotes;
        if (! replaceToggle.getToggleState()) chatAppliedNotes.insert (chatAppliedNotes.end(), chatNotes.begin(), chatNotes.end());
        {
            const juce::ScopedLock sl (engine.getLock());
            chatClip->notes = chatAppliedNotes;
        }
        chatAppliedMidi = true;
        engine.notesChanged();
        setStatus ("Applied MIDI notes. The track's instrument is unchanged.");
    }
    updateControls();
}

void GeneratePanel::undoChatMusician()
{
    if (! chatSnapshotValid()) { setStatus ("The clip or song changed, so this undo cannot safely restore it.", true); return; }
    if (chatAuditioning) engine.stopAudition();
    chatAuditioning = false;
    if (chatAppliedMidi)
    {
        const juce::ScopedLock sl (engine.getLock());
        chatClip->notes = chatOriginalNotes;
    }
    if (chatAppliedLane) engine.setChords (chatOriginalChords);
    if (chatAppliedMidi) engine.notesChanged();
    chatAppliedMidi = chatAppliedLane = false;
    setStatus ("Restored the notes and chord track from before applying.");
    updateControls();
}

void GeneratePanel::generateChatMusician()
{
    if (clip == nullptr || engine.findTrackFor (clip) == nullptr) return;
    const auto chunks = gen::chatmusician::splitRange (clip->lengthBeats);
    if (chunks.empty() || std::abs (clip->startBeat / 4.0 - std::round (clip->startBeat / 4.0)) > 1.0e-9)
    {
        setStatus ("Choose a clip that starts on a bar line and contains whole 4/4 bars.", true);
        return;
    }
    if (modelBox.getText().trim().isEmpty()) { setStatus ("Choose an Ollama model.", true); return; }
    const bool chordTask = chatTask.getSelectedId() == 2;
    const int bars = getClipBars();
    std::vector<Chord> source;
    for (int bar = 0; bar < bars && ! chordTask; ++bar)
    {
        const auto* chord = engine.getChord (songBarOf (*clip, bar));
        if (chord == nullptr) { setStatus ("Set a chord on every bar before generating a melody.", true); return; }
        source.push_back (*chord);
    }
    std::vector<juce::StringArray> choices;
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

    if (chatAuditioning) engine.stopAudition();
    chatAuditioning = false;
    chatClip = clip;
    chatEditGeneration = engine.getEditGeneration();
    chatStart = clip->startBeat;
    chatLength = clip->lengthBeats;
    chatKey = engine.getKey();
    chatOriginalNotes = clip->notes;
    chatOriginalChords = engine.getChords();
    chatStaged = chatAppliedMidi = chatAppliedLane = false;
    chatNotes.clear(); chatChords.clear();
    const int id = ++generationId;
    cancelFlag = std::make_shared<std::atomic<bool>> (false);
    const auto flag = cancelFlag;
    const auto key = chatKey;
    const auto url = getServerUrl();
    const auto model = modelBox.getText().trim();
    const auto safe = juce::Component::SafePointer<GeneratePanel> (this);
    generatingRequest.part = chordTask ? gen::Part::chords : gen::Part::melody;
    progressText = "Waiting for the local model.";
    startTimerHz (8);
    updateControls();
    juce::Thread::launch ([safe, flag, key, url, model, id, source, choices, chordTask]
    {
        juce::String error;
        std::vector<MidiNote> notes;
        std::vector<Chord> chords;
        juce::Random random;
        auto request = [flag, url, model, chordTask, &random] (const juce::String& prompt, const juce::var& schema, int, juce::String& requestError)
        {
            gen::OllamaClient::ChatOptions options;
            options.model = model; options.userPrompt = prompt; options.schema = schema;
            options.maxTokens = chordTask ? 256 : 384; options.temperature = chordTask ? 0.8 : 0.7;
            options.seed = 1 + random.nextInt (std::numeric_limits<int>::max() - 1);
            options.shouldStop = [flag] { return flag->load(); };
            return gen::OllamaClient (url).generateRaw (options, requestError);
        };
        const auto cancelled = [flag] { return flag->load(); };
        bool ok = false;
        if (chordTask)
        {
            ok = gen::chatmusician::generateChords (key, choices, request, cancelled, chords, error);
            if (ok && ! gen::chatmusician::voiceChords (chords, 0.0, notes)) { ok = false; error = "These chords could not be voiced."; }
        }
        else
        {
            auto melodyRequest = [&request] (const juce::String& p, const juce::var& s, juce::String& e) { return request (p, s, 0, e); };
            ok = gen::chatmusician::generateMelody (key, source, melodyRequest, cancelled, notes, error);
        }
        juce::MessageManager::callAsync ([safe, id, ok, notes, chords, error]
        {
            auto* self = safe.getComponent();
            if (self == nullptr || self->generationId != id) return;
            self->cancelFlag = nullptr; self->stopTimer();
            if (! ok) self->setStatus (error, true);
            else if (! self->chatSnapshotValid()) self->setStatus ("The clip or song changed. Generate again.", true);
            else
            {
                self->chatNotes = notes; self->chatChords = chords; self->chatStaged = true;
                juce::StringArray names;
                for (const auto& chord : chords) names.add (chord.getName());
                self->setStatus (chords.empty() ? "Take ready. Preview before applying." : "Chords: " + names.joinIntoString (" - ") + "\nPreview before applying.");
            }
            self->updateControls();
        });
    });
}

void GeneratePanel::cancel()
{
    if (cancelFlag != nullptr)
        *cancelFlag = true;

    // Takes that are done are kept: stop here and pick from those.
    if (! pendingTakes.empty())
    {
        finishGeneration (true);
        return;
    }

    cancelFlag = nullptr;
    ++generationId;
    stopTimer();
    setStatus ("Cancelled.");
    updateControls();
}

void GeneratePanel::timerCallback()
{
    static const char* frames[] = { "|", "/", "-", "\\" };
    setStatus (juce::String ("Generating ") + gen::getPartName (generatingRequest.part).toLowerCase() + " with "
               + modelBox.getText() + "  " + frames[spinnerPhase++ % 4] + "\n" + progressText);
}

void GeneratePanel::takeArrived (int id, const gen::Result& result, const juce::String& error, const juce::StringArray& directions)
{
    if (id != generationId)
        return;

    if (error.isNotEmpty())
    {
        ++failedTakes;

        if (takeError.isEmpty())
            takeError = error;

        return;
    }

    pendingTakes.push_back ({ result, gen::scoreTake (generatingRequest, result), directions });
}

void GeneratePanel::allTakesArrived (int id)
{
    if (id == generationId)
        finishGeneration (false);
}

void GeneratePanel::finishGeneration (bool stoppedEarly)
{
    cancelFlag = nullptr;
    ++generationId; // takes still on their way from a stopped generation are ignored
    stopTimer();

    if (pendingTakes.empty())
        setStatus (takeError.isNotEmpty() ? takeError : juce::String ("No usable take came back. Try again."), true);
    else if (findTrackFor (generatingClip) == nullptr)
        setStatus ("The clip was deleted while generating.", true);
    else
    {
        undoClip = generatingClip;
        undoNotes = undoClip->notes;
        undoLength = undoClip->lengthBeats;
        undoName = undoClip->name;
        undoAddedChords.clear();

        takes = std::move (pendingTakes);
        takesRequest = generatingRequest;
        takesClipName = generatingRequest.style + " " + gen::getPartShortName (generatingRequest.part);

        if (const auto* style = generatingRequest.usesPlayingStyle() ? gen::styles::findStyle (generatingRequest.playingStyle) : nullptr)
            takesClipName = style->name;
        bestTake = (int) std::distance (takes.begin(), std::max_element (takes.begin(), takes.end(),
                                                                           [] (const Take& a, const Take& b) { return a.score < b.score; }));
        shownTake = -1;
        updateTakeButtons();
        applyTake (bestTake);

        auto status = statusLabel.getText();

        if (stoppedEarly)
            status << "\nStopped after " << (int) takes.size() << (takes.size() == 1 ? " take." : " takes.");

        if (failedTakes > 0)
            status << "\n" << failedTakes << (failedTakes == 1 ? " take" : " takes") << " failed: " << takeError;

        setStatus (status);
    }

    pendingTakes.clear();
    updateControls();
}

void GeneratePanel::applyTake (int index)
{
    if (undoClip == nullptr || findTrackFor (undoClip) == nullptr || index < 0 || index >= (int) takes.size())
        return;

    // Each take starts from the clip as it was before the generation.
    restoreClip();

    auto* target = undoClip;
    auto* targetTrack = findTrackFor (target);
    const auto& take = takes[(size_t) index];
    const auto& result = take.result;

    {
        const juce::ScopedLock sl (engine.getLock());

        // The clip keeps its length: drop or shorten anything past its end.
        std::vector<MidiNote> fitted;

        for (auto n : result.notes)
        {
            if (n.start >= target->lengthBeats - 1.0e-6)
                continue;

            n.length = juce::jmin (n.length, target->lengthBeats - n.start);
            fitted.push_back (n);
        }

        if (replaceToggle.getToggleState())
            target->notes = std::move (fitted);
        else
            target->notes.insert (target->notes.end(), fitted.begin(), fitted.end());

        target->name = takesClipName;
    }

    engine.notesChanged();
    shownTake = index;

    for (int i = 0; i < takeButtons.size(); ++i)
        takeButtons[i]->setToggleState (i == index, juce::dontSendNotification);

    juce::String status = takes.size() > 1
        ? "Take " + juce::String (index + 1) + " of " + juce::String ((int) takes.size()) + " (score "
              + juce::String (juce::roundToInt (take.score * 100.0)) + (index == bestTake ? ", the best" : "") + "): "
        : juce::String ("Done: ");
    status << result.summary << ".";

    // Chords the model wrote for bars without one go on the chord track.
    auto chords = engine.getChords();
    std::vector<const Chord*> added;

    for (int bar = 0; bar < (int) result.newChords.size(); ++bar)
    {
        const int songBar = songBarOf (*target, bar);
        const auto& chord = result.newChords[(size_t) bar];

        if (chord.has_value() && chords.count (songBar) == 0)
        {
            chords[songBar] = *chord;
            undoAddedChords[songBar] = *chord;
            added.push_back (&undoAddedChords[songBar]);
        }
    }

    if (! added.empty())
    {
        engine.setChords (std::move (chords));
        status << "\nAdded to the chord track: " << joinChordNames (added) << ".";
    }

    if (gen::isPercussive (takesRequest.part) && targetTrack->usesBuiltInSynth()
        && ! builtInSounds::isPercussive (targetTrack->builtInSound))
        status << "\nTip: pick " << gen::getPartName (takesRequest.part)
               << " in the DawIt Synth (click it in the device bar) so the hits play as " << gen::getPartShortName (takesRequest.part) << ".";

    setStatus (status);
}

void GeneratePanel::restoreClip()
{
    {
        const juce::ScopedLock sl (engine.getLock());
        undoClip->notes = undoNotes;
        undoClip->lengthBeats = undoLength;
        undoClip->name = undoName;
    }

    // Take back the chords the generation added, unless they have been changed since.
    if (! undoAddedChords.empty())
    {
        auto chords = engine.getChords();

        for (auto& [bar, chord] : undoAddedChords)
            if (auto it = chords.find (bar); it != chords.end() && it->second == chord)
                chords.erase (it);

        undoAddedChords.clear();
        engine.setChords (std::move (chords));
    }

    engine.notesChanged();
}

void GeneratePanel::undo()
{
    if (undoClip != nullptr && findTrackFor (undoClip) != nullptr)
    {
        restoreClip();
        setStatus ("Restored the clip.");
    }

    undoClip = nullptr;
    takes.clear();
    updateTakeButtons();
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
            b->onClick = [this, i] { applyTake (i); };
            form.addAndMakeVisible (b);
        }
    }

    resized();
}
} // namespace daw
