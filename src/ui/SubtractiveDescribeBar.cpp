#include "SubtractiveDescribeBar.h"

#include "GuideButton.h"
#include "generate/OllamaClient.h"

namespace daw
{
namespace sd = gen::sounddescribe;
using sub::Knob;

namespace
{
    constexpr const char* defaultModel = "qwen2.5:7b";
    constexpr const char* recentKey = "describe.subtractive.recent";
    constexpr int maxRecent = 8;
    constexpr juce::int64 doneMillis = 800;
    const juce::Colour errorColour { 0xffff7a7a };

    juce::String fromUtf8 (const char* s) { return juce::String::fromUTF8 (s); }
}

//==============================================================================
/** The status line: DawIt's notes first in yellow, then the status, shrinking to fit two lines. */
class SubtractiveDescribeBar::StatusView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    void set (const juce::StringArray& newNotes, const juce::String& newText, bool isError, const juce::String& tip)
    {
        notes = newNotes;
        text = newText;
        error = isError;
        setTooltip (tip);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();

        for (float size : { 12.5f, 11.5f, 10.5f, 9.5f })
        {
            juce::AttributedString s;
            s.setWordWrap (juce::AttributedString::byWord);

            for (auto& n : notes)
                s.append (n + "  ", juce::FontOptions (size), theme::meterYellow);

            s.append (text, juce::FontOptions (size), error ? errorColour : theme::textDim);

            juce::TextLayout layout;
            layout.createLayout (s, area.getWidth());

            if (layout.getHeight() <= area.getHeight() + 0.5f || size < 10.0f)
            {
                layout.draw (g, area);
                return;
            }
        }
    }

private:
    juce::StringArray notes;
    juce::String text;
    bool error = false;
};

//==============================================================================
SubtractiveDescribeBar::SubtractiveDescribeBar (BuiltInSubtractiveEditor& e, Engine& eng)
    : editor (e), synth (e.getSynth()), engine (eng)
{
    status = std::make_unique<StatusView>();
    addAndMakeVisible (*status);

    text.setMultiLine (false);
    text.setReturnKeyStartsNewLine (false);
    text.setFont (juce::FontOptions (14.0f));
    text.setTextToShowWhenEmpty ("Describe a sound: a warm pad that swells in slowly", theme::textDim);
    text.setTooltip ("What you want to hear, in your own words. After an answer, type a change such as \"a bit brighter\" or \"softer start\". Press Return to ask.");
    text.onReturnKey = [this] { ask (false); };
    text.onEscapeKey = [this]
    {
        if (phase == Phase::waiting)
            cancel();
        else
            editor.setDescribeBarOpen (false);
    };
    addAndMakeVisible (text);

    recentButton.setButtonText (fromUtf8 ("\xe2\x96\xbe"));
    recentButton.setTooltip ("The last descriptions you asked for.");
    recentButton.onClick = [this] { showRecentMenu(); };

    startButton.setTooltip ("Best starting point: the AI picks the closest starting point and changes as little as possible. "
                            "Current sound: it changes the sound in the window now, for follow-ups.");
    startButton.onClick = [this]
    {
        startFromCurrent = ! startFromCurrent;
        updateControls();
    };

    modelBox.setTooltip ("The Ollama model that answers. The same choice as in the Generate panel and the Lyrics window.");
    modelBox.onChange = [this]
    {
        cannedModel = {};
        getSettings().setValue ("generator.model", modelBox.getText());
        getSettings().saveIfNeeded();
        updateSetupInfo();
        updateControls();
    };

    askButton.setTooltip ("Ask the model. It takes a few seconds; nothing changes until you choose how to apply the answer.");
    askButton.onClick = [this]
    {
        if (phase == Phase::waiting)
            cancel();
        else
            ask (false);
    };

    closeButton.setButtonText (fromUtf8 ("\xc3\x97"));
    closeButton.setTooltip ("Close the bar. The sound stays as it is.");
    closeButton.onClick = [this] { editor.setDescribeBarOpen (false); };

    walkButton.setTooltip ("Go through the changes one knob at a time: DawIt lights each knob and waits for you to turn it.");
    walkButton.onClick = [this] { startWalk(); };
    allButton.setTooltip ("Change every knob at once. The panel on the right lists each change and why.");
    allButton.onClick = [this] { setAllNow(); };
    againButton.setTooltip ("Ask the same words again for a different answer.");
    againButton.onClick = [this] { ask (true); };
    undoButton.setTooltip ("Take back the last change made here: a whole answer or one step. The app's Undo doesn't cover synth knobs.");
    undoButton.onClick = [this] { undo(); };
    retryButton.setTooltip ("Look for Ollama and its models again.");
    retryButton.onClick = [this] { refreshModels(); };
    setupButton.setTooltip ("How to install Ollama and get a model; shown in the panel on the right.");
    setupButton.onClick = [this] { showSetup (! setupShown); };
    backButton.setTooltip ("Go back a step and put that knob back as it was.");
    backButton.onClick = [this] { stepBack(); };
    skipButton.setTooltip ("Leave this knob as it is and go to the next one.");
    skipButton.onClick = [this] { stepSkip(); };
    setForMeButton.setTooltip ("Turn this knob to the suggested value for you, then go to the next one.");
    setForMeButton.onClick = [this] { stepSet(); };
    finishButton.setTooltip ("Set every knob that's left now and end the walkthrough.");
    finishButton.onClick = [this] { stepFinishAll(); };

    for (auto* b : { &recentButton, &startButton, &askButton, &closeButton, &walkButton, &allButton, &againButton, &undoButton,
                     &retryButton, &setupButton, &backButton, &skipButton, &setForMeButton, &finishButton })
    {
        b->setWantsKeyboardFocus (false);
        addChildComponent (*b);
    }

    modelBox.setWantsKeyboardFocus (false);

    for (auto* c : std::initializer_list<juce::Component*> { &recentButton, &startButton, &askButton, &closeButton })
        c->setVisible (true);

    addAndMakeVisible (modelBox);
    status->setTooltip ({});
    addMouseListener (this, true);
    updateControls();
}

SubtractiveDescribeBar::~SubtractiveDescribeBar()
{
    removeMouseListener (this);
    ++requestId;
    ++modelRefreshId;

    if (stopFlag != nullptr)
        stopFlag->store (true);
}

//==============================================================================
juce::PropertiesFile& SubtractiveDescribeBar::getSettings() const
{
    return *engine.properties.getUserSettings();
}

juce::String SubtractiveDescribeBar::getServerUrl() const
{
    return getSettings().getValue ("generator.ollamaUrl", "http://localhost:11434");
}

juce::String SubtractiveDescribeBar::getModel() const
{
    return cannedModel.isNotEmpty() ? cannedModel : modelBox.getText().trim();
}

juce::StringArray SubtractiveDescribeBar::getRecent() const
{
    juce::StringArray list;
    list.addLines (getSettings().getValue (recentKey));
    list.removeEmptyStrings();
    return list;
}

void SubtractiveDescribeBar::addRecent (const juce::String& words)
{
    auto list = getRecent();
    list.removeString (words, true);
    list.insert (0, words);

    while (list.size() > maxRecent)
        list.remove (list.size() - 1);

    getSettings().setValue (recentKey, list.joinIntoString ("\n"));
    getSettings().saveIfNeeded();
}

void SubtractiveDescribeBar::showRecentMenu()
{
    const auto list = getRecent();
    juce::PopupMenu menu;

    for (int i = 0; i < list.size(); ++i)
        menu.addItem (i + 1, list[i]);

    if (list.isEmpty())
        menu.addItem (1, "No descriptions yet", false);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&recentButton),
                        [safe = juce::Component::SafePointer<SubtractiveDescribeBar> (this), list] (int result)
                        {
                            if (auto* self = safe.getComponent(); self != nullptr && result >= 1 && result <= list.size())
                            {
                                self->text.setText (list[result - 1], juce::dontSendNotification);
                                self->text.grabKeyboardFocus();
                            }
                        });
}

//==============================================================================
void SubtractiveDescribeBar::paint (juce::Graphics& g)
{
    fxui::drawPanel (g, getLocalBounds().toFloat());
}

void SubtractiveDescribeBar::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    auto row1 = r.removeFromTop (26);
    closeButton.setBounds (row1.removeFromRight (26));
    row1.removeFromRight (8);
    askButton.setBounds (row1.removeFromRight (74));
    row1.removeFromRight (8);
    modelBox.setBounds (row1.removeFromRight (150));
    row1.removeFromRight (8);
    startButton.setBounds (row1.removeFromRight (160));
    row1.removeFromRight (8);
    recentButton.setBounds (row1.removeFromRight (26));
    row1.removeFromRight (4);
    text.setBounds (row1);

    r.removeFromTop (6);
    auto row2 = r;
    juce::TextButton* order[] = { &walkButton, &allButton, &againButton, &backButton, &skipButton, &setForMeButton, &finishButton,
                                  &retryButton, &setupButton, &undoButton };
    const juce::Font font { juce::FontOptions (14.0f) };

    for (auto it = std::rbegin (order); it != std::rend (order); ++it)
    {
        auto* b = *it;

        if (! b->isVisible())
            continue;

        const int w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, b->getButtonText())) + 26;
        b->setBounds (row2.removeFromRight (w).withSizeKeepingCentre (w, 26));
        row2.removeFromRight (6);
    }

    status->setBounds (row2);
}

void SubtractiveDescribeBar::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && e.eventComponent != &text)
        showMenu();
}

void SubtractiveDescribeBar::showMenu()
{
    juce::PopupMenu menu;
    menu.addItem ("Undo the last change", undoList.canUndo(), false, [safe = juce::Component::SafePointer<SubtractiveDescribeBar> (this)]
                  {
                      if (auto* self = safe.getComponent())
                          self->undo();
                  });
    menu.addItem ("Close the bar", [safe = juce::Component::SafePointer<SubtractiveDescribeBar> (this)]
                  {
                      if (auto* self = safe.getComponent())
                          self->editor.setDescribeBarOpen (false);
                  });
    menu.addSeparator();
    guideicon::addItem (menu, "Describing sounds guide...", [safe = juce::Component::SafePointer<SubtractiveDescribeBar> (this)]
                        {
                            if (auto* self = safe.getComponent())
                                self->editor.openDescribeGuide();
                        });
    menu.showMenuAsync ({});
}

//==============================================================================
void SubtractiveDescribeBar::opened()
{
    text.grabKeyboardFocus();
    refreshModels();
    updateControls();
}

void SubtractiveDescribeBar::closed()
{
    if (phase == Phase::waiting)
        cancel();

    walk.reset();
    stepMade.clear();
    phase = Phase::idle;
    statusText = {};
    setupShown = false;
    updateControls();
}

void SubtractiveDescribeBar::setWords (const juce::String& words)
{
    text.setText (words, juce::dontSendNotification);
}

//==============================================================================
void SubtractiveDescribeBar::refreshModels()
{
    const int id = ++modelRefreshId;
    refreshing = true;
    updateControls();

    const auto url = getServerUrl();
    juce::Thread::launch ([safe = juce::Component::SafePointer<SubtractiveDescribeBar> (this), url, id]
    {
        const auto list = gen::OllamaClient (url).listModelInfo();

        juce::MessageManager::callAsync ([safe, list, id]
        {
            if (auto* self = safe.getComponent(); self != nullptr && self->modelRefreshId == id)
                self->applyModelList (list);
        });
    });
}

void SubtractiveDescribeBar::applyModelList (const gen::OllamaClient::ModelList& list)
{
    modelList = list;
    haveModelList = true;
    refreshing = false;
    cannedModel = {};

    const auto names = list.getNames();
    modelBox.clear (juce::dontSendNotification);
    modelBox.addItemList (names, 1);

    if (! names.isEmpty())
    {
        const auto wanted = getSettings().getValue ("generator.model", defaultModel);
        int index = names.indexOf (wanted);

        if (index < 0)
            index = names.indexOf (defaultModel);

        modelBox.setSelectedItemIndex (juce::jmax (0, index), juce::dontSendNotification);
    }

    updateSetupInfo();
    updateControls();
}

void SubtractiveDescribeBar::updateSetupInfo()
{
    setupInfo = gen::aisetup::describe (gen::aisetup::Mode::ai, modelList, getServerUrl(), getModel());
}

bool SubtractiveDescribeBar::setCannedSetup (const juce::String& name)
{
    gen::OllamaClient::ModelList list;
    list.reachable = name != "unreachable";

    if (! list.reachable)
        list.error = "Can't reach Ollama at " + getServerUrl();

    if (name == "notInstalled")
        list.models = { { "qwen2.5:7b", "d-qwen", "qwen2" } };
    else if (name != "unreachable" && name != "noModels")
        return false;

    ++modelRefreshId;   // a real answer that is still on its way is ignored
    applyModelList (list);

    if (name == "notInstalled")
        cannedModel = "llama3:8b";

    updateSetupInfo();
    updateControls();
    return true;
}

void SubtractiveDescribeBar::showSetup (bool show)
{
    setupShown = show;
    editor.showDescription (show ? gen::aisetup::getSetupSteps() : juce::StringArray());
    updateControls();
}

//==============================================================================
void SubtractiveDescribeBar::ask (bool again)
{
    const auto words = again ? lastRequest.words : text.getText().trim();

    if (words.isEmpty() || phase == Phase::waiting)
        return;

    if (! isReady())
    {
        phase = Phase::idle;
        statusText = {};
        updateControls();
        return;
    }

    sd::Request r;

    if (again)
    {
        r = lastRequest;
        r.seed = juce::Random::getSystemRandom().nextInt (99998) + 1;
    }
    else
    {
        r.words = words;
        r.followUp = startFromCurrent;
        addRecent (words);
    }

    r.current = synth.getSettings();
    r.model = getModel();
    lastRequest = r;

    walk.reset();
    stepMade.clear();
    editor.setHighlight (std::nullopt);
    setupShown = false;
    phase = Phase::waiting;
    statusText = "Asking " + r.model + "... The first answer can take longer while the model loads.";
    statusIsError = false;

    stopFlag = std::make_shared<std::atomic<bool>> (false);
    auto flag = stopFlag;
    const int id = ++requestId;
    const auto url = getServerUrl();
    updateControls();

    juce::Thread::launch ([safe = juce::Component::SafePointer<SubtractiveDescribeBar> (this), r, url, flag, id]
    {
        gen::OllamaClient::ChatOptions options;
        options.model = r.model;
        options.systemPrompt = sd::buildSystemPrompt();
        options.userPrompt = sd::buildUserPrompt (r);
        options.schema = sd::buildSchema();
        options.temperature = sd::temperature;
        options.seed = r.seed;
        options.think = false;
        options.shouldStop = [flag] { return flag->load(); };

        juce::String error;
        const auto reply = gen::OllamaClient (url).chat (options, error);

        if (flag->load())
            return;

        sd::Result result;

        if (error.isEmpty())
            result = sd::readReply (r, reply);

        juce::MessageManager::callAsync ([safe, id, result, error, r]
        {
            auto* self = safe.getComponent();

            if (self == nullptr || self->requestId != id)
                return;

            if (error.isNotEmpty())
            {
                self->phase = Phase::idle;
                self->statusText = error;
                self->statusIsError = true;
                self->updateControls();
                return;
            }

            self->deliver (result, r);
        });
    });
}

void SubtractiveDescribeBar::cancel()
{
    if (stopFlag != nullptr)
        stopFlag->store (true);

    ++requestId;
    phase = Phase::idle;
    statusText = "Cancelled. Nothing changes.";
    statusIsError = false;
    updateControls();
}

void SubtractiveDescribeBar::deliver (const sd::Result& result, const sd::Request& request)
{
    statusIsError = false;

    if (! result.ok)
    {
        phase = Phase::idle;
        statusText = result.error.isNotEmpty() ? result.error : "The model's answer couldn't be read. Ask again, or pick another model.";
        statusIsError = true;
        updateControls();
        return;
    }

    synth.lastDescription = result.sound;
    synth.lastDescription.model = request.model;
    startFromCurrent = true;   // the next words are a follow-up
    phase = Phase::answered;
    showAnswer();
}

void SubtractiveDescribeBar::showAnswer()
{
    const auto& d = synth.lastDescription;
    const auto from = d.followUp || d.base == sd::currentSoundName ? juce::String ("the current sound") : d.base;
    const int n = (int) d.changes.size();

    if (n == 0)
        statusText = "The model suggests no changes from " + from + ". Try other words.";
    else
        statusText = "From " + from + ": " + juce::String (n) + (n == 1 ? " change" : " changes") + ". Walk me through it, or set it all now.";

    editor.setHighlight (std::nullopt);
    editor.showDescription();
    updateControls();
}

bool SubtractiveDescribeBar::showCannedReply (const juce::String& reply, bool followUp)
{
    sd::Request r;
    r.words = text.getText().trim();
    r.followUp = followUp;
    r.current = synth.getSettings();
    r.model = getModel().isNotEmpty() ? getModel() : juce::String (defaultModel);
    lastRequest = r;

    ++requestId;
    const auto result = sd::readReply (r, reply);
    deliver (result, r);
    return result.ok;
}

//==============================================================================
void SubtractiveDescribeBar::startWalk()
{
    if (synth.lastDescription.changes.empty())
        return;

    walk = std::make_unique<sd::Walkthrough> (synth.lastDescription, synth.getSettings());
    stepMade.assign ((size_t) walk->getNumSteps(), 0);
    synth.lastDescription.made = 0;
    reachedIndex = -1;
    phase = Phase::walking;
    editor.showDescription();
    updateStep();
}

bool SubtractiveDescribeBar::startWalkthrough (int step)
{
    startWalk();

    for (int i = 0; i < step && phase == Phase::walking; ++i)
        stepSet();

    return phase == Phase::walking;
}

int SubtractiveDescribeBar::countMade() const
{
    return (int) std::count (stepMade.begin(), stepMade.end(), (char) 1);
}

void SubtractiveDescribeBar::updateStep()
{
    if (walk == nullptr)
        return;

    synth.lastDescription.made = countMade();

    if (walk->isFinished())
    {
        endWalk();
        return;
    }

    const auto& step = walk->getStep (walk->getIndex());
    const auto prefix = "Step " + juce::String (walk->getIndex() + 1) + " of " + juce::String (walk->getNumSteps()) + fromUtf8 (" \xc2\xb7 ");

    if (step.loadsBase)
    {
        editor.setHighlight (std::nullopt);
        statusText = prefix + step.reason + ": click Set it for me.";
    }
    else
    {
        const auto info = sub::getKnobInfo (step.knob);
        const auto now = sd::formatKnobValue (step.knob, sub::getKnobValue (step.knob, synth.getSettings()));
        editor.setHighlight (step.knob);
        statusText = prefix + subtext::getTitle ((int) step.knob) + ": " + (info.isChoice ? "click it until it shows " : "turn it to ")
                     + sd::formatKnobValue (step.knob, step.target) + " (now " + now + "). " + step.reason;

        if (step.note.isNotEmpty())
            statusText << " " << step.note;
    }

    statusIsError = false;
    updateControls();
}

void SubtractiveDescribeBar::endWalk()
{
    const int made = countMade();
    synth.lastDescription.made = made;
    walk.reset();
    phase = Phase::finished;
    editor.setHighlight (std::nullopt);
    statusText = "All " + juce::String (made) + (made == 1 ? " change" : " changes") + " made. Play a note and listen; type a change to refine it.";
    updateControls();
}

void SubtractiveDescribeBar::stepSet()
{
    if (walk == nullptr || walk->isFinished())
        return;

    auto live = synth.getSettings();
    undoList.push (live);

    if (! walk->getStep (walk->getIndex()).loadsBase)
        stepMade[(size_t) walk->getIndex()] = 1;

    walk->setForUser (live);
    synth.setSettings (live);
    reachedIndex = -1;
    updateStep();
}

void SubtractiveDescribeBar::stepSkip()
{
    if (walk == nullptr || walk->isFinished())
        return;

    walk->skip (synth.getSettings());
    reachedIndex = -1;
    updateStep();
}

void SubtractiveDescribeBar::stepBack()
{
    if (walk == nullptr || walk->getIndex() <= 0)
        return;

    auto live = synth.getSettings();
    walk->back (live);
    synth.setSettings (live);
    stepMade[(size_t) walk->getIndex()] = 0;
    reachedIndex = -1;
    updateStep();
}

void SubtractiveDescribeBar::stepFinishAll()
{
    if (walk == nullptr)
        return;

    auto live = synth.getSettings();
    undoList.push (live);

    for (int i = walk->getIndex(); i < walk->getNumSteps(); ++i)
        if (! walk->getStep (i).loadsBase)
            stepMade[(size_t) i] = 1;

    walk->finishAll (live);
    synth.setSettings (live);
    updateStep();
}

void SubtractiveDescribeBar::setAllNow()
{
    auto& d = synth.lastDescription;

    if (d.changes.empty())
        return;

    undoList.push (synth.getSettings());
    synth.setSettings (sd::getResultSettings (d));
    d.made = (int) d.changes.size();
    walk.reset();
    stepMade.clear();
    phase = Phase::finished;
    editor.setHighlight (std::nullopt);
    statusText = "All " + juce::String (d.made) + (d.made == 1 ? " change" : " changes") + " made. Play a note and listen; type a change to refine it.";
    editor.showDescription();
    updateControls();
}

void SubtractiveDescribeBar::undo()
{
    sub::Settings previous;

    if (! undoList.pop (previous))
        return;

    synth.setSettings (previous);
    walk.reset();
    stepMade.clear();
    synth.lastDescription.made = 0;
    editor.setHighlight (std::nullopt);

    if (synth.lastDescription.changes.empty())
    {
        phase = Phase::idle;
        statusText = "Undone.";
    }
    else
    {
        phase = Phase::answered;
        statusText = "Undone. Walk me through it, or set it all now.";
    }

    editor.showDescription();
    updateControls();
}

void SubtractiveDescribeBar::tick()
{
    if (phase != Phase::walking || walk == nullptr || walk->isFinished())
        return;

    const auto live = synth.getSettings();
    const int index = walk->getIndex();

    if (walk->isCurrentReached (live))
    {
        const auto knob = walk->getStep (index).knob;

        if (reachedIndex != index)
        {
            reachedIndex = index;
            reachedSince = juce::Time::getMillisecondCounter();
            statusText = "Done: you set " + sd::formatKnobValue (knob, sub::getKnobValue (knob, live));
            updateControls();
        }
        else if ((juce::int64) juce::Time::getMillisecondCounter() - reachedSince >= doneMillis)
        {
            stepMade[(size_t) index] = 1;
            walk->markDone (live);   // the user's own value stays
            reachedIndex = -1;
            updateStep();
        }
    }
    else if (reachedIndex == index)
    {
        reachedIndex = -1;
        updateStep();
    }
}

//==============================================================================
void SubtractiveDescribeBar::updateControls()
{
    const bool answered = phase == Phase::answered, walking = phase == Phase::walking, finished = phase == Phase::finished;
    const bool hasChanges = ! synth.lastDescription.changes.empty();
    const bool idleProblem = phase == Phase::idle && haveModelList && ! refreshing && setupInfo.offersRetry;

    startButton.setButtonText (startFromCurrent ? "Current sound" : "Best starting point");
    askButton.setButtonText (phase == Phase::waiting ? "Cancel" : "Ask");
    askButton.setTooltip (phase == Phase::waiting ? "Stop waiting for the answer. Nothing changes."
                                                  : "Ask the model. It takes a few seconds; nothing changes until you choose how to apply the answer.");

    walkButton.setVisible (answered && hasChanges);
    allButton.setVisible (answered && hasChanges);
    againButton.setVisible ((answered || finished) && ! lastRequest.words.isEmpty());
    backButton.setVisible (walking);
    skipButton.setVisible (walking);
    setForMeButton.setVisible (walking);
    finishButton.setVisible (walking);
    retryButton.setVisible (idleProblem);
    setupButton.setVisible (idleProblem);
    undoButton.setVisible (phase != Phase::waiting && undoList.canUndo());
    backButton.setEnabled (walking && walk != nullptr && walk->getIndex() > 0);
    setupButton.setButtonText (setupShown ? "Hide steps" : "Set up...");

    juce::String line = statusText;
    bool isError = statusIsError;

    if (phase == Phase::idle && statusText.isEmpty())
    {
        if (refreshing || ! haveModelList)
            line = "Looking for Ollama models...";
        else if (! setupInfo.ready)
            line = setupInfo.status;
        else
            line = "Type what you want to hear, then press Return or Ask. " + setupInfo.status + ".";
    }

    const bool showNotes = answered || finished;
    const auto tip = walking ? juce::String ("The model's reason. Hover the knob for DawIt's own explanation.")
                             : (showNotes ? synth.lastDescription.notes.joinIntoString ("  ") + "  " + line : line);
    status->set (showNotes ? synth.lastDescription.notes : juce::StringArray(), line, isError, tip);
    resized();
    repaint();
}
} // namespace daw
