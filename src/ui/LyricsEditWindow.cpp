#include "GuideButton.h"
#include "LyricsEditWindow.h"
#include "LyricsExplainer.h"
#include "generate/OllamaClient.h"

#include <map>

namespace daw
{
namespace
{
    namespace ai = gen::lyricsai;

    constexpr const char* defaultModel = "qwen2.5:7b";
    constexpr int panelGap = 12;

    juce::PropertiesFile& settings (Engine& engine) { return *engine.properties.getUserSettings(); }

    struct AskInfo
    {
        const char* id;
        const char* label;
        const char* tooltip;
        bool defaultOn;
    };

    const AskInfo askInfos[] {
        { "concrete", "More concrete", "Things you can see, hear or touch instead of named feelings: \"your coat by the door\" instead of \"I miss you\".", false },
        { "simpler", "Simpler words", "Everyday words, said the way people speak: lyrics are heard once, at speed.", false },
        { "syllables", "Keep the syllables", "As many syllables as the original, so it fits the same melody.", true },
        { "rhyme", "Keep the rhyme", "End on a word that rhymes with the line it pairs with.", true },
        { "new", "New words", "The same idea without the original's words.", false },
        { "angle", "Another angle", "A different picture or point of view on the same idea.", false },
        { "shorter", "Shorter", "Fewer syllables than the original.", false },
        { "longer", "Longer", "More syllables than the original.", false }
    };

    juce::String capitalise (const juce::String& s) { return s.substring (0, 1).toUpperCase() + s.substring (1); }

    /** The heading above the results: what the request was for. */
    juce::String describe (const ai::Request& r, const ai::Context& c)
    {
        int syllables = 0;

        for (int s : c.lineSyllables)
            syllables += s;

        const auto syllableText = syllables > 0 ? " " + juce::String::fromUTF8 ("\xc2\xb7") + " " + juce::String (syllables) + (syllables == 1 ? " syllable" : " syllables")
                                                : juce::String();
        const auto dot = " " + juce::String::fromUTF8 ("\xc2\xb7") + " ";

        switch (r.kind)
        {
            case ai::Kind::song:           return "The whole song" + dot + juce::String ((int) ai::buildSongPlan (r.sections).items.size()) + " sections";
            case ai::Kind::sectionRewrite: return "Rewrite " + c.heading + dot + juce::String (c.linesWanted) + (c.linesWanted == 1 ? " line" : " lines");
            case ai::Kind::sectionAdd:     return "New " + c.heading + dot + juce::String (c.linesWanted) + (c.linesWanted == 1 ? " line" : " lines");
            case ai::Kind::nextLine:       return "Next line after line " + juce::String (c.lastLine) + " of " + c.heading
                                                  + (c.previousSyllables > 0 ? dot + juce::String (c.previousSyllables) + " syllables before" : juce::String());
            case ai::Kind::rhymes:         return "Rhymes for \"" + c.word + "\"";
            case ai::Kind::selection:
            {
                if (! c.wholeLines)
                    return "Words in line " + juce::String (c.firstLine) + " of " + c.heading + syllableText;

                return (c.firstLine == c.lastLine ? "Line " + juce::String (c.firstLine) : "Lines " + juce::String (c.firstLine) + "-" + juce::String (c.lastLine))
                       + " of " + c.heading + syllableText;
            }
        }

        return {};
    }

    /** The first sung section of the Sections row that has no words in the text yet, as a heading and bars. */
    std::optional<std::pair<juce::String, int>> findNextSectionWithoutWords (const juce::String& text, const SectionList& sections)
    {
        const auto plan = ai::buildSongPlan (sections);

        if (plan.isDefaultForm)
            return std::nullopt;

        std::map<int, int> existing, needed;
        bool atParagraphStart = true;

        for (auto& line : juce::StringArray::fromLines (text))
        {
            const auto t = line.trim();

            if (t.isEmpty())
            {
                atParagraphStart = true;
                continue;
            }

            if (t.startsWithChar ('[') && t.endsWithChar (']'))
            {
                const auto name = t.substring (1, t.length() - 1).trim();

                for (auto& item : plan.items)
                    if (name.startsWithIgnoreCase (sectionTypes::getName (item.type)))
                        ++existing[(int) item.type];

                atParagraphStart = false;
            }
            else if (atParagraphStart)
            {
                ++existing[(int) SectionType::verse];
                atParagraphStart = false;
            }
        }

        for (auto& item : plan.items)
            if (++needed[(int) item.type] > existing[(int) item.type])
                return std::make_pair (item.heading, item.lines * 2);

        return std::nullopt;
    }

    bool isWordCharacter (juce::juce_wchar ch) { return juce::CharacterFunctions::isLetter (ch) || ch == '\'' || ch == '-'; }
}

//==============================================================================
/** The lyrics box, with the AI's entries in its right-click menu. */
class LyricsEditWindow::TextBox final : public juce::TextEditor
{
public:
    std::function<void (juce::PopupMenu&)> onAddItems;
    std::function<bool (int)> onAction;

    void addPopupMenuItems (juce::PopupMenu& menu, const juce::MouseEvent* e) override
    {
        juce::TextEditor::addPopupMenuItems (menu, e);

        if (onAddItems != nullptr)
        {
            menu.addSeparator();
            onAddItems (menu);
        }
    }

    void performPopupMenuAction (int id) override
    {
        if (onAction != nullptr && onAction (id))
            return;

        juce::TextEditor::performPopupMenuAction (id);
    }
};

//==============================================================================
struct LyricsEditWindow::Chip
{
    juce::String word, kindLabel;
    int colour = -1;   // a rhyme colour (the kind), or -1
};

/** The suggestions: alternatives (their lines and DawIt's labels) or rhymes (words in chips). Click selects, double-click uses. */
class LyricsEditWindow::ResultsView final : public juce::Component
{
public:
    struct Row
    {
        enum class Type { heading, alternative, chips };

        Type type = Type::heading;
        juce::String text;
        int alt = -1;
        std::vector<std::pair<juce::String, bool>> labels;   // text, warn
        std::vector<Chip> chips;

        // layout
        int y = 0, height = 0;
        juce::TextLayout body;
        std::vector<juce::TextLayout> labelLayouts;
        std::vector<juce::Rectangle<int>> chipRects;
    };

    std::function<void (int, int)> onSelect, onUse;

    void setRows (std::vector<Row> newRows, int width)
    {
        rows = std::move (newRows);
        selectedRow = selectedChip = -1;
        layout (width);
    }

    const Row& getRow (int i) const { return rows[(size_t) i]; }
    int getNumRows() const noexcept { return (int) rows.size(); }

    void setSelected (int row, int chip)
    {
        selectedRow = row;
        selectedChip = chip;
        repaint();
    }

    void layout (int width)
    {
        width = juce::jmax (120, width);
        int y = 2;

        for (auto& row : rows)
        {
            row.y = y;

            switch (row.type)
            {
                case Row::Type::heading:
                    row.height = 24;
                    break;

                case Row::Type::alternative:
                {
                    row.body = makeLayout (row.text, juce::FontOptions (14.5f), theme::text, (float) width - 40.0f);
                    row.labelLayouts.clear();
                    float h = 8.0f + row.body.getHeight() + 2.0f;

                    for (auto& label : row.labels)
                    {
                        row.labelLayouts.push_back (makeLayout (label.first, juce::FontOptions (12.5f), label.second ? theme::meterYellow : theme::textDim, (float) width - 40.0f));
                        h += row.labelLayouts.back().getHeight() + 1.0f;
                    }

                    row.height = juce::roundToInt (h) + 8;
                    break;
                }

                case Row::Type::chips:
                {
                    row.chipRects.clear();
                    int x = 4, cy = 2, lineHeight = 26;

                    for (auto& chip : row.chips)
                    {
                        const int w = chipWidth (chip);

                        if (x + w > width - 4 && x > 4)
                        {
                            x = 4;
                            cy += lineHeight + 4;
                        }

                        row.chipRects.push_back ({ x, cy, w, lineHeight });
                        x += w + 4;
                    }

                    row.height = cy + lineHeight + 6;
                    break;
                }
            }

            y += row.height;
        }

        setSize (width, y + 4);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < (int) rows.size(); ++i)
        {
            auto& row = rows[(size_t) i];

            switch (row.type)
            {
                case Row::Type::heading:
                    g.setColour (theme::textDim);
                    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
                    g.drawFittedText (row.text, 4, row.y, getWidth() - 8, row.height, juce::Justification::bottomLeft, 1, 0.85f);
                    break;

                case Row::Type::alternative:
                {
                    const auto bounds = juce::Rectangle<int> (0, row.y + 2, getWidth(), row.height - 4).toFloat();
                    g.setColour (i == selectedRow ? theme::accent.withAlpha (0.28f) : theme::panelLight);
                    g.fillRoundedRectangle (bounds, 4.0f);

                    if (i == selectedRow)
                    {
                        g.setColour (theme::accent);
                        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
                    }

                    g.setColour (theme::textDim);
                    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
                    g.drawText (juce::String (row.alt + 1), 6, row.y + 8, 22, 18, juce::Justification::centredLeft, false);

                    float y = (float) row.y + 8.0f;
                    row.body.draw (g, juce::Rectangle<float> (30.0f, y, (float) getWidth() - 40.0f, row.body.getHeight()));
                    y += row.body.getHeight() + 2.0f;

                    for (auto& label : row.labelLayouts)
                    {
                        label.draw (g, juce::Rectangle<float> (30.0f, y, (float) getWidth() - 40.0f, label.getHeight()));
                        y += label.getHeight() + 1.0f;
                    }

                    break;
                }

                case Row::Type::chips:
                    for (size_t c = 0; c < row.chips.size(); ++c)
                    {
                        auto& chip = row.chips[c];
                        const auto rect = row.chipRects[c].translated (0, row.y).toFloat();
                        const auto colour = LyricsExplainer::getRhymeColour (chip.colour);
                        const bool selected = i == selectedRow && (int) c == selectedChip;

                        g.setColour (selected ? colour.withAlpha (0.35f) : theme::panelLight);
                        g.fillRoundedRectangle (rect, 4.0f);
                        g.setColour (selected ? colour : colour.withAlpha (0.5f));
                        g.drawRoundedRectangle (rect.reduced (0.5f), 4.0f, 1.0f);

                        const juce::Font wordFont (juce::FontOptions (14.0f, juce::Font::bold));
                        g.setFont (wordFont);
                        g.setColour (colour);
                        const float wordWidth = juce::GlyphArrangement::getStringWidth (wordFont, chip.word);
                        g.drawText (chip.word, rect.withTrimmedLeft (7.0f).withWidth (wordWidth + 2.0f), juce::Justification::centredLeft, false);

                        if (chip.kindLabel.isNotEmpty())
                        {
                            g.setColour (theme::textDim);
                            g.setFont (juce::FontOptions (11.5f));
                            g.drawText (chip.kindLabel, rect.withTrimmedLeft (7.0f + wordWidth + 6.0f), juce::Justification::centredLeft, false);
                        }
                    }

                    break;
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto hit = findHit (e.getPosition());

        if (hit.first >= 0 && onSelect != nullptr)
            onSelect (hit.first, hit.second);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const auto hit = findHit (e.getPosition());

        if (hit.first >= 0 && onUse != nullptr)
            onUse (hit.first, hit.second);
    }

private:
    static juce::TextLayout makeLayout (const juce::String& s, const juce::FontOptions& font, juce::Colour colour, float width)
    {
        juce::AttributedString a;
        a.setWordWrap (juce::AttributedString::byWord);
        a.append (s, juce::Font (font), colour);
        juce::TextLayout l;
        l.createLayout (a, width);
        return l;
    }

    static int chipWidth (const Chip& chip)
    {
        const float word = juce::GlyphArrangement::getStringWidth (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)), chip.word);
        const float kind = chip.kindLabel.isEmpty() ? 0.0f
                                                    : 6.0f + juce::GlyphArrangement::getStringWidth (juce::Font (juce::FontOptions (11.5f)), chip.kindLabel);
        return juce::roundToInt (word + kind) + 18;
    }

    std::pair<int, int> findHit (juce::Point<int> p) const
    {
        for (int i = 0; i < (int) rows.size(); ++i)
        {
            auto& row = rows[(size_t) i];

            if (row.type == Row::Type::alternative && p.y >= row.y && p.y < row.y + row.height)
                return { i, -1 };

            if (row.type == Row::Type::chips)
                for (size_t c = 0; c < row.chipRects.size(); ++c)
                    if (row.chipRects[c].translated (0, row.y).contains (p))
                        return { i, (int) c };
        }

        return { -1, -1 };
    }

    std::vector<Row> rows;
    int selectedRow = -1, selectedChip = -1;
};

//==============================================================================
LyricsEditWindow::LyricsEditWindow (Engine& e, int firstBar)
    : engine (e), startBar (firstBar), openedText (lyrics::toText (e.getLyrics())),
      stopFlag (std::make_shared<std::atomic<bool>> (false)), receivedChars (std::make_shared<std::atomic<int>> (0))
{
    text = std::make_unique<TextBox>();
    text->setMultiLine (true, true);
    text->setReturnKeyStartsNewLine (true);
    text->setScrollbarsShown (true);
    text->setFont (juce::FontOptions (15.0f));
    text->setColour (juce::TextEditor::backgroundColourId, theme::background);
    text->setColour (juce::TextEditor::textColourId, theme::text);
    text->setColour (juce::TextEditor::outlineColourId, theme::gridBar);
    text->setTextToShowWhenEmpty ("[Verse]\nI walked a-long the ri-ver\n...\n\n[Chorus]\n...", theme::textDim);
    text->setText (openedText, false);
    text->onTextChange = [this] { updateUseState(); updateActionLabels(); };
    text->onAddItems = [this] (juce::PopupMenu& menu) { addPopupItems (menu); };
    text->onAction = [this] (int id) { if (id < 1001) return false; performPopupAction (id); return true; };
    addAndMakeVisible (*text);

    hint.setText ("One line per sung line, an empty line between verses, and hyphens (to-ge-ther) to split a word into syllables "
                  "that each get their own place in time. A heading such as [Verse], [Chorus] or [Bridge] above a verse puts it on "
                  "that section, and a chorus typed once goes on every chorus. Without headings the verses go on the sung sections "
                  "in order, or follow two bars a line from the playhead when there are no sections.",
                  juce::dontSendNotification);
    hint.setFont (juce::FontOptions (12.5f));
    hint.setColour (juce::Label::textColourId, theme::textDim);
    hint.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (hint);

    outsideNote.setText ("The lyrics changed outside this window. Apply replaces them with this text.", juce::dontSendNotification);
    outsideNote.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    outsideNote.setColour (juce::Label::textColourId, theme::meterYellow);
    addChildComponent (outsideNote);

    keepTiming.setButtonText ("Keep the timing of the words that didn't change");
    keepTiming.setToggleState (true, juce::dontSendNotification);
    keepTiming.setTooltip ("Off: spread all the lines over the sections again, forgetting where words were dragged or tapped");
    keepTiming.setVisible (! engine.getLyrics().isEmpty());
    addChildComponent (keepTiming);

    applyButton.setTooltip ("Put this text in the song. Words that didn't change keep their timing.");
    cancelButton.setTooltip ("Close the window without changing the song");
    applyButton.onClick = [this] { apply(); };
    cancelButton.onClick = [this] { close(); };
    addAndMakeVisible (applyButton);
    addAndMakeVisible (cancelButton);

    // The panel.
    foldButton.setTooltip ("Show or hide the Write with AI panel.");
    foldButton.onClick = [this] { setPanelOpen (! panelOpen); };
    addAndMakeVisible (foldButton);

    helpButton.setTooltip ("How lyrics are written with AI: what it does well, what DawIt checks, and who owns the words.");
    helpButton.onClick = [this] { if (onShowGuide) onShowGuide (gen::lyricsguide::Topic::writeWithAi); };
    addAndMakeVisible (helpButton);

    for (auto* l : { &modelLabel, &aboutLabel, &moodLabel, &viewLabel, &asksLabel, &wordsLabel })
    {
        l->setColour (juce::Label::textColourId, theme::textDim);
        l->setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (l);
    }

    modelLabel.setText ("Model", juce::dontSendNotification);
    aboutLabel.setText ("About", juce::dontSendNotification);
    moodLabel.setText ("Mood", juce::dontSendNotification);
    viewLabel.setText ("Point of view", juce::dontSendNotification);
    asksLabel.setText ("Ask", juce::dontSendNotification);
    wordsLabel.setText ("Your words", juce::dontSendNotification);

    modelBox.setTooltip ("The Ollama model that writes, the same as the piano roll's generator.");
    modelBox.onChange = [this]
    {
        if (modelsLoaded && modelBox.getText().isNotEmpty())
        {
            settings (engine).setValue ("generator.model", modelBox.getText());
            settings (engine).saveIfNeeded();
        }
    };
    addAndMakeVisible (modelBox);
    reloadButton.setTooltip ("Reload the list of models from Ollama.");
    reloadButton.onClick = [this] { refreshModels(); };
    addAndMakeVisible (reloadButton);

    auto styleEditor = [] (juce::TextEditor& ed)
    {
        ed.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
        ed.setColour (juce::TextEditor::textColourId, theme::text);
        ed.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        ed.setFont (juce::FontOptions (13.5f));
    };

    aboutEditor.setMultiLine (true, true);
    aboutEditor.setReturnKeyStartsNewLine (false);
    aboutEditor.setScrollbarsShown (true);
    styleEditor (aboutEditor);
    aboutEditor.setTextToShowWhenEmpty ("a night walk after a break-up", theme::textDim);
    aboutEditor.setTooltip ("What the song is about, in your own words: the AI reads this for every request. Saved with the song.");
    aboutEditor.onTextChange = [this] { briefEdited(); };
    addAndMakeVisible (aboutEditor);

    for (auto& option : lyrics::brief::getMoods())
        moodBox.addItem (option.second, moodBox.getNumItems() + 1);

    for (auto& option : lyrics::brief::getPointsOfView())
        viewBox.addItem (option.second, viewBox.getNumItems() + 1);

    moodBox.setTooltip ("The feeling the words should carry.");
    viewBox.setTooltip ("Who is singing to whom: I (a confession), you (to someone), we, or a story about others.");
    moodBox.onChange = [this] { briefEdited(); };
    viewBox.onChange = [this] { briefEdited(); };
    addAndMakeVisible (moodBox);
    addAndMakeVisible (viewBox);

    songButton.setTooltip ("Writes words for every sung section on the Sections row: a verse per verse, the chorus once (it goes on every chorus), "
                           "the pre-chorus and the bridge, a line per 2 bars. You see it before it goes in.");
    sectionButton.setTooltip ("Rewrite the section the cursor is in, or add a verse, chorus, bridge or other section at the cursor, written to follow what's there.");
    selectionButton.setTooltip ("Three other ways to write the selected words or lines, with the same syllables and rhyme where you ask for them (Cmd+R). "
                                "With nothing selected: the next line after the cursor.");
    rhymesButton.setTooltip ("Rhymes for the selected word or the word at the cursor: from the pronouncing dictionary, and picked for your line by the AI (Shift+Cmd+R).");
    songButton.onClick = [this] { startRequest (ai::Kind::song); };
    sectionButton.onClick = [this] { showSectionMenu(); };
    selectionButton.onClick = [this]
    {
        startRequest (text->getHighlightedRegion().isEmpty() ? ai::Kind::nextLine : ai::Kind::selection);
    };
    rhymesButton.onClick = [this] { startRequest (ai::Kind::rhymes); };

    for (auto* b : { &songButton, &sectionButton, &selectionButton, &rhymesButton })
    {
        // The buttons keep the focus (and the selection) in the text box.
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    const auto saved = settings (engine).containsKey ("lyrics.aiAsk") ? settings (engine).getValue ("lyrics.aiAsk") : juce::String();
    const auto enabled = juce::StringArray::fromTokens (saved, ",", "");

    for (auto& info : askInfos)
    {
        auto button = std::make_unique<juce::TextButton> (info.label);
        button->setClickingTogglesState (true);
        button->setToggleState (settings (engine).containsKey ("lyrics.aiAsk") ? enabled.contains (info.id) : info.defaultOn, juce::dontSendNotification);
        button->setTooltip (info.tooltip);
        button->setMouseClickGrabsKeyboardFocus (false);
        button->onClick = [this] { saveAsks(); };
        addAndMakeVisible (*button);
        askButtons.push_back (std::move (button));
    }

    wordsEditor.setMultiLine (false);
    styleEditor (wordsEditor);
    wordsEditor.setTextToShowWhenEmpty ("make it about the sea", theme::textDim);
    wordsEditor.setTooltip ("Anything else to ask for, such as \"make it about the sea\" or \"use the word harbour\".");
    addAndMakeVisible (wordsEditor);

    resultHeading.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    resultHeading.setColour (juce::Label::textColourId, theme::text);
    addAndMakeVisible (resultHeading);

    resultHelp.setTooltip ("Finding a rhyme: from perfect to loose, in the lyrics guide.");
    resultHelp.onClick = [this] { if (onShowGuide) onShowGuide (gen::lyricsguide::Topic::findRhyme); };
    addChildComponent (resultHelp);

    statusLabel.setFont (juce::FontOptions (12.5f));
    statusLabel.setColour (juce::Label::textColourId, theme::textDim);
    statusLabel.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (statusLabel);

    useButton.setTooltip ("Put the selected alternative in place of the highlighted text. Cmd+Z in the text takes it back.");
    againButton.setTooltip ("Asks for new alternatives with the same request.");
    stopButton.setTooltip ("Stop the request that is running.");
    useButton.onClick = [this] { useSelected(); };
    againButton.onClick = [this] { runAgain(); };
    stopButton.onClick = [this] { cancelRequest(); };

    for (auto* b : { &useButton, &againButton, &stopButton })
    {
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    useButton.setEnabled (false);
    againButton.setEnabled (false);
    stopButton.setVisible (false);

    results = std::make_unique<ResultsView>();
    results->onSelect = [this] (int row, int chip) { selectResult (row, chip); };
    results->onUse = [this] (int row, int chip) { selectResult (row, chip); useSelected(); };
    resultsViewport.setViewedComponent (results.get(), false);
    resultsViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (resultsViewport);

    loadBriefFromEngine();
    engine.addChangeListener (this);

    panelOpen = settings (engine).getBoolValue ("view.lyricsAiOpen", true);
    setSize (panelOpen ? 980 : 600, 780);
    setPanelOpen (panelOpen);   // sets the visibility and the button's text; the window already has its size
    refreshModels();
    updateActionLabels();
    startTimerHz (8);
}

LyricsEditWindow::~LyricsEditWindow()
{
    stopFlag->store (true);
    ++requestId;
    engine.removeChangeListener (this);
}

void LyricsEditWindow::paint (juce::Graphics& g) { g.fillAll (theme::panel); }

void LyricsEditWindow::visibilityChanged()
{
    if (isShowing())
        text->grabKeyboardFocus();
}

void LyricsEditWindow::setPanelOpen (bool open)
{
    const bool changed = open != panelOpen;
    panelOpen = open;
    settings (engine).setValue ("view.lyricsAiOpen", open);

    if (changed)
        if (auto* top = getTopLevelComponent(); top != nullptr && top != this)
            top->setSize (juce::jmax (open ? 940 : 560, top->getWidth() + (open ? 1 : -1) * (panelWidth + panelGap)), top->getHeight());

    foldButton.setButtonText ("Write with AI " + juce::String::fromUTF8 (open ? "\xe2\x96\xbe" : "\xe2\x96\xb8"));

    for (auto* c : std::initializer_list<juce::Component*> { &helpButton, &modelLabel, &aboutLabel, &moodLabel, &viewLabel, &asksLabel, &wordsLabel,
                                                              &modelBox, &moodBox, &viewBox, &reloadButton, &aboutEditor, &wordsEditor, &songButton,
                                                              &sectionButton, &selectionButton, &rhymesButton, &resultHeading, &statusLabel, &useButton,
                                                              &againButton, &resultsViewport })
        c->setVisible (open);

    for (auto& b : askButtons)
        b->setVisible (open);

    stopButton.setVisible (open && running);
    resultHelp.setVisible (open && haveResult && lastRequest.kind == ai::Kind::rhymes);
    resized();
}

void LyricsEditWindow::resized()
{
    auto r = getLocalBounds().reduced (16);

    auto top = r.removeFromTop (66);
    auto foldArea = top.removeFromRight (196);
    hint.setBounds (top.withTrimmedRight (8));
    auto foldRow = foldArea.removeFromTop (24);
    helpButton.setBounds (foldRow.removeFromRight (24));
    foldRow.removeFromRight (4);
    foldButton.setBounds (foldRow.removeFromRight (160));

    if (outsideNote.isVisible())
        outsideNote.setBounds (r.removeFromTop (20));
    else
        outsideNote.setBounds ({});

    r.removeFromTop (4);

    auto buttons = r.removeFromBottom (28);
    cancelButton.setBounds (buttons.removeFromRight (90));
    buttons.removeFromRight (8);
    applyButton.setBounds (buttons.removeFromRight (90));
    keepTiming.setBounds (buttons);
    r.removeFromBottom (10);

    if (panelOpen)
    {
        auto panel = r.removeFromRight (panelWidth);
        r.removeFromRight (panelGap);
        layoutPanel (panel);
    }

    text->setBounds (r);
}

void LyricsEditWindow::layoutPanel (juce::Rectangle<int> a)
{
    auto row = [&] (int h) { auto x = a.removeFromTop (h); a.removeFromTop (6); return x; };

    auto modelRow = row (26);
    modelLabel.setBounds (modelRow.removeFromLeft (46));
    reloadButton.setBounds (modelRow.removeFromRight (28));
    modelRow.removeFromRight (4);
    modelBox.setBounds (modelRow);

    auto aboutRow = row (46);
    aboutLabel.setBounds (aboutRow.removeFromLeft (46));
    aboutEditor.setBounds (aboutRow);

    auto moodRow = row (26);
    moodLabel.setBounds (moodRow.removeFromLeft (46));
    moodBox.setBounds (moodRow.removeFromLeft (100));
    moodRow.removeFromLeft (8);
    viewLabel.setBounds (moodRow.removeFromLeft (84));
    viewBox.setBounds (moodRow);

    auto first = row (28);
    songButton.setBounds (first.removeFromLeft (first.getWidth() / 2 - 3));
    first.removeFromLeft (6);
    sectionButton.setBounds (first);

    auto second = row (28);
    selectionButton.setBounds (second.removeFromLeft (second.getWidth() / 2 - 3));
    second.removeFromLeft (6);
    rhymesButton.setBounds (second);

    asksLabel.setBounds (a.removeFromTop (18));
    a.removeFromTop (2);

    int x = a.getX(), y = a.getY();
    const juce::Font font (juce::FontOptions (13.0f));

    for (auto& b : askButtons)
    {
        const int w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, b->getButtonText())) + 18;

        if (x + w > a.getRight() && x > a.getX())
        {
            x = a.getX();
            y += 26;
        }

        b->setBounds (x, y, w, 22);
        x += w + 4;
    }

    a.removeFromTop (y + 26 - a.getY() + 4);

    auto wordsRow = row (26);
    wordsLabel.setBounds (wordsRow.removeFromLeft (70));
    wordsEditor.setBounds (wordsRow);

    auto headingRow = a.removeFromTop (22);
    resultHelp.setBounds (headingRow.removeFromRight (24));
    resultHeading.setBounds (headingRow);
    a.removeFromTop (4);

    auto bottom = a.removeFromBottom (28);
    againButton.setBounds (bottom.removeFromRight (90));
    bottom.removeFromRight (6);
    useButton.setBounds (bottom.removeFromRight (70));
    bottom.removeFromRight (6);
    stopButton.setBounds (bottom.removeFromRight (80));
    a.removeFromBottom (4);
    statusLabel.setBounds (a.removeFromBottom (34));
    a.removeFromBottom (4);

    resultsViewport.setBounds (a);
    results->layout (resultsViewport.getMaximumVisibleWidth());
}

//==============================================================================
bool LyricsEditWindow::keyPressed (const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();

    if (mods.isCommandDown() && (key.getKeyCode() == 'R' || key.getKeyCode() == 'r'))
    {
        if (! panelOpen)
            setPanelOpen (true);

        if (mods.isShiftDown())
            startRequest (ai::Kind::rhymes);
        else
            startRequest (text->getHighlightedRegion().isEmpty() ? ai::Kind::nextLine : ai::Kind::selection);

        return true;
    }

    return false;
}

void LyricsEditWindow::apply()
{
    const auto typed = text->getText();
    const auto& sections = engine.getSections();

    if (keepTiming.isVisible() && keepTiming.getToggleState())
        engine.setLyrics (lyrics::setText (engine.getLyrics(), typed, sections));
    else
        engine.setLyrics (lyrics::place (lyrics::parse (typed), sections, startBar));

    close();
}

void LyricsEditWindow::close()
{
    if (onClose != nullptr)
        onClose();
}

void LyricsEditWindow::changeListenerCallback (juce::ChangeBroadcaster*)
{
    const bool outside = lyrics::toText (engine.getLyrics()) != openedText;

    if (outside != outsideNote.isVisible())
    {
        outsideNote.setVisible (outside);
        resized();
    }

    loadBriefFromEngine();
}

//==============================================================================
void LyricsEditWindow::briefEdited()
{
    if (refreshingBrief)
        return;

    LyricsBrief b;
    b.about = aboutEditor.getText();
    const auto& moods = lyrics::brief::getMoods();
    const auto& views = lyrics::brief::getPointsOfView();

    if (juce::isPositiveAndBelow (moodBox.getSelectedId() - 1, (int) moods.size()))
        b.mood = moods[(size_t) moodBox.getSelectedId() - 1].first;

    if (juce::isPositiveAndBelow (viewBox.getSelectedId() - 1, (int) views.size()))
        b.pointOfView = views[(size_t) viewBox.getSelectedId() - 1].first;

    engine.setLyricsBrief (b);
}

void LyricsEditWindow::loadBriefFromEngine()
{
    const auto& b = engine.getLyricsBrief();
    const auto& moods = lyrics::brief::getMoods();
    const auto& views = lyrics::brief::getPointsOfView();
    auto indexOf = [] (const std::vector<lyrics::brief::Option>& options, const juce::String& id)
    {
        for (size_t i = 0; i < options.size(); ++i)
            if (options[i].first == id)
                return (int) i;

        return 0;
    };

    const juce::ScopedValueSetter<bool> guard (refreshingBrief, true);

    if (! aboutEditor.hasKeyboardFocus (true) && aboutEditor.getText().trim() != b.about)
        aboutEditor.setText (b.about, false);

    moodBox.setSelectedId (indexOf (moods, b.mood) + 1, juce::dontSendNotification);
    viewBox.setSelectedId (indexOf (views, b.pointOfView) + 1, juce::dontSendNotification);
}

ai::Asks LyricsEditWindow::getAsks() const
{
    ai::Asks a;
    auto on = [this] (const char* id)
    {
        for (size_t i = 0; i < askButtons.size(); ++i)
            if (juce::String (askInfos[i].id) == id)
                return askButtons[i]->getToggleState();

        return false;
    };

    a.moreConcrete = on ("concrete");
    a.simplerWords = on ("simpler");
    a.keepSyllables = on ("syllables");
    a.keepRhyme = on ("rhyme");
    a.newWords = on ("new");
    a.anotherAngle = on ("angle");
    a.shorter = on ("shorter");
    a.longer = on ("longer");
    a.yourWords = wordsEditor.getText().trim();
    return a;
}

void LyricsEditWindow::saveAsks()
{
    juce::StringArray enabled;

    for (size_t i = 0; i < askButtons.size(); ++i)
        if (askButtons[i]->getToggleState())
            enabled.add (askInfos[i].id);

    settings (engine).setValue ("lyrics.aiAsk", enabled.joinIntoString (","));
    settings (engine).saveIfNeeded();
}

//==============================================================================
juce::String LyricsEditWindow::getServerUrl() const
{
    return settings (engine).getValue ("generator.ollamaUrl", "http://localhost:11434");
}

void LyricsEditWindow::setStatus (const juce::String& message, bool isError)
{
    statusLabel.setColour (juce::Label::textColourId, isError ? juce::Colour (0xffff7a7a) : theme::textDim);
    statusLabel.setText (message, juce::dontSendNotification);
}

void LyricsEditWindow::refreshModels()
{
    const int refreshId = ++modelRefreshId;
    modelsLoaded = false;
    modelBox.clear (juce::dontSendNotification);
    setStatus ("Looking for Ollama models...");

    const auto url = getServerUrl();
    juce::Thread::launch ([safeThis = juce::Component::SafePointer<LyricsEditWindow> (this), url, refreshId]
    {
        juce::String error;
        const auto models = gen::OllamaClient (url).listModels (error);

        juce::MessageManager::callAsync ([safeThis, models, error, refreshId]
        {
            auto* self = safeThis.getComponent();

            if (self == nullptr || self->modelRefreshId != refreshId)
                return;

            self->modelsLoaded = true;
            const auto wanted = settings (self->engine).getValue ("generator.model", defaultModel);
            self->modelBox.clear (juce::dontSendNotification);
            self->modelBox.addItemList (models, 1);

            if (! models.isEmpty())
                self->modelBox.setSelectedItemIndex (models.contains (wanted) ? models.indexOf (wanted) : juce::jmax (0, models.indexOf (defaultModel)),
                                                     juce::dontSendNotification);

            if (error.isNotEmpty())
                self->modelError = error;
            else if (models.isEmpty())
                self->modelError = "No chat models installed. In a terminal, run: ollama pull " + juce::String (defaultModel);
            else
                self->modelError = {};

            if (self->modelError.isNotEmpty())
                self->setStatus (self->modelError, true);
            else
                self->setStatus (juce::String (models.size()) + " model(s) available.");
        });
    });
}

//==============================================================================
void LyricsEditWindow::startRequest (ai::Kind kind, const juce::String& addHeading, int addBars)
{
    ai::Request r;
    r.kind = kind;
    r.text = text->getText();
    const auto region = text->getHighlightedRegion();
    r.rangeStart = region.isEmpty() ? text->getCaretPosition() : region.getStart();
    r.rangeEnd = region.isEmpty() ? r.rangeStart : region.getEnd();

    if (addHeading.isNotEmpty())
        r.addHeading = addHeading;

    r.addBars = addBars;
    launch (r);
}

void LyricsEditWindow::launch (ai::Request r)
{
    if (running)
        return;

    r.sections = engine.getSections();
    r.key = engine.getKey();
    r.bpm = engine.getTempo();
    r.brief = lyrics::brief::tidy (engine.getLyricsBrief());
    r.asks = getAsks();
    r.seed = 1 + juce::Random::getSystemRandom().nextInt (std::numeric_limits<int>::max() - 1);

    auto c = ai::buildContext (r);

    if (c.error.isNotEmpty())
    {
        setStatus (c.error, true);
        return;
    }

    const auto model = modelBox.getText();

    if (model.isEmpty() && r.kind != ai::Kind::rhymes)
    {
        setStatus (modelError.isNotEmpty() ? modelError : "No chat models installed. In a terminal, run: ollama pull " + juce::String (defaultModel), true);
        return;
    }

    lastRequest = r;
    lastContext = c;
    haveLastRequest = true;
    haveResult = false;
    running = true;
    staleShown = false;
    resultHeading.setText (describe (r, c), juce::dontSendNotification);
    resultHelp.setVisible (false);
    stopFlag = std::make_shared<std::atomic<bool>> (false);
    receivedChars = std::make_shared<std::atomic<int>> (0);
    spinnerStep = 0;
    stopButton.setVisible (panelOpen);
    useButton.setEnabled (false);
    againButton.setEnabled (false);

    for (auto* b : { &songButton, &sectionButton, &selectionButton, &rhymesButton })
        b->setEnabled (false);

    const int id = ++requestId;
    const auto url = getServerUrl();
    auto flag = stopFlag;
    auto chars = receivedChars;

    // The worker gets plain copies, so it needs no lock and never touches the edit.
    juce::Thread::launch ([safeThis = juce::Component::SafePointer<LyricsEditWindow> (this), r, c, model, url, flag, chars, id]
    {
        ai::Result result;
        juce::String chatError;
        gen::rhyme::RhymeList dictionary;

        if (r.kind == ai::Kind::rhymes)
            dictionary = gen::rhyme::findRhymes (c.word);

        if (r.kind != ai::Kind::rhymes || model.isNotEmpty())
        {
            const auto system = ai::buildSystemPrompt();

            for (int attempt = 0; attempt < 2 && ! flag->load(); ++attempt)
            {
                auto attemptRequest = r;
                attemptRequest.seed = r.seed + attempt;

                gen::OllamaClient::ChatOptions options;
                options.model = model;
                options.systemPrompt = system;
                options.userPrompt = ai::buildUserPrompt (attemptRequest, c);
                options.schema = ai::buildSchema (attemptRequest, c);
                options.temperature = 0.9;
                options.seed = attemptRequest.seed;
                options.maxTokens = r.kind == ai::Kind::song ? 8192 : 4096;
                options.shouldStop = [flag] { return flag->load(); };
                options.onProgress = [chars] (int n) { chars->store (n); };

                juce::String error;
                const auto reply = gen::OllamaClient (url).chat (options, error);

                if (error.isNotEmpty())
                {
                    chatError = error;
                    break;
                }

                result = ai::parseReply (attemptRequest, c, reply);

                if (! result.shouldRetry)
                    break;
            }
        }

        if (flag->load())
            return;

        juce::MessageManager::callAsync ([safeThis, id, result, chatError, dictionary]
        {
            if (auto* self = safeThis.getComponent(); self != nullptr && self->requestId == id)
                self->deliver (id, result, chatError, dictionary);
        });
    });
}

void LyricsEditWindow::showCanned (const juce::String& state)
{
    modelRefreshId += 1;   // a model list that is still on its way must not overwrite the canned status
    modelsLoaded = true;
    modelBox.clear (juce::dontSendNotification);

    if (state != "noOllama")
    {
        modelBox.addItem (defaultModel, 1);
        modelBox.setSelectedItemIndex (0, juce::dontSendNotification);
    }

    // Line 3 of the first verse (rhymes: the last word of line 2 of the first chorus, "light").
    const auto all = text->getText();
    const bool forRhymes = state == "rhymes";
    int start = all.indexOf (forRhymes ? "[Chorus" : "[Verse");
    start = start < 0 ? 0 : all.indexOf (start, "\n") + 1;

    for (int i = 0; i < (forRhymes ? 1 : 2) && start > 0; ++i)
        start = all.indexOf (start, "\n") + 1;

    int end = all.indexOf (start, "\n");
    end = end < 0 ? all.length() : end;

    ai::Request r;
    r.kind = state == "song" ? ai::Kind::song : state == "rhymes" ? ai::Kind::rhymes : ai::Kind::selection;
    r.text = all;
    r.rangeStart = start;
    r.rangeEnd = state == "rhymes" ? start : end;

    if (state == "rhymes")
    {
        // The cursor on the last word of the line.
        r.rangeStart = r.rangeEnd = end - 1;
    }

    r.sections = engine.getSections();
    r.key = engine.getKey();
    r.bpm = engine.getTempo();
    r.brief = lyrics::brief::tidy (engine.getLyricsBrief());
    r.asks = getAsks();

    auto c = ai::buildContext (r);
    lastRequest = r;
    lastContext = c;
    haveLastRequest = true;
    resultHeading.setText (describe (r, c), juce::dontSendNotification);

    if (state == "noOllama")
    {
        haveResult = false;
        setStatus ("Can't reach Ollama. Start it, then press the reload button next to the model box.", true);
        updateUseState();
        return;
    }

    lastResult = ai::parseReply (r, c, ai::getCannedReply (r, c, true));
    lastDictionary.clear();
    lastGuessed = false;

    if (r.kind == ai::Kind::rhymes)
    {
        auto list = gen::rhyme::findRhymes (c.word);
        lastDictionary = std::move (list.items);
        lastGuessed = list.guessed;
    }

    haveResult = true;
    showResult ({});
}

void LyricsEditWindow::runAgain()
{
    if (running || ! haveLastRequest)
        return;

    launch (lastRequest);
}

void LyricsEditWindow::cancelRequest()
{
    if (! running)
        return;

    stopFlag->store (true);
    ++requestId;
    running = false;
    stopButton.setVisible (false);

    for (auto* b : { &songButton, &sectionButton, &selectionButton, &rhymesButton })
        b->setEnabled (true);

    setStatus ("Stopped.");
    updateUseState();
}

void LyricsEditWindow::deliver (int, ai::Result result, juce::String chatError, gen::rhyme::RhymeList dictionary)
{
    running = false;
    stopButton.setVisible (false);

    for (auto* b : { &songButton, &sectionButton, &selectionButton, &rhymesButton })
        b->setEnabled (true);

    lastResult = std::move (result);
    lastDictionary = std::move (dictionary.items);
    lastGuessed = dictionary.guessed;
    haveResult = true;
    showResult (chatError);
}

void LyricsEditWindow::showResult (const juce::String& chatError)
{
    using Row = ResultsView::Row;
    std::vector<Row> rows;
    const bool isRhymes = lastRequest.kind == ai::Kind::rhymes;
    int firstSelectable = -1, firstChip = -1;

    auto addHeading = [&] (const juce::String& t) { Row row; row.type = Row::Type::heading; row.text = t; rows.push_back (std::move (row)); };

    if (isRhymes)
    {
        if (! lastResult.rhymes.empty())
        {
            addHeading ("Picked for your line by the AI");
            Row row;
            row.type = Row::Type::chips;

            for (auto& pick : lastResult.rhymes)
            {
                const bool known = pick.kind != gen::rhyme::Kind::unknown && pick.kind != gen::rhyme::Kind::none;
                row.chips.push_back ({ pick.word, (known ? gen::rhyme::getKindName (pick.kind) : juce::String()) + (pick.guessed ? (known ? ", " : "") + juce::String ("guessed") : juce::String()),
                                       known ? (int) pick.kind : -1 });
            }

            firstSelectable = (int) rows.size();
            firstChip = 0;
            rows.push_back (std::move (row));
        }

        addHeading (lastGuessed ? "From the dictionary (guessed from the spelling: the word isn't in it)" : "From the dictionary");

        if (lastDictionary.empty())
            addHeading ("No rhymes found.");

        for (size_t i = 0; i < lastDictionary.size();)
        {
            size_t end = i;

            while (end < lastDictionary.size() && lastDictionary[end].kind == lastDictionary[i].kind && lastDictionary[end].syllables == lastDictionary[i].syllables)
                ++end;

            addHeading (capitalise (gen::rhyme::getKindName (lastDictionary[i].kind)) + " rhymes, " + juce::String (lastDictionary[i].syllables)
                        + (lastDictionary[i].syllables == 1 ? " syllable" : " syllables"));
            Row row;
            row.type = Row::Type::chips;

            for (size_t k = i; k < end; ++k)
                row.chips.push_back ({ lastDictionary[k].word, {}, (int) lastDictionary[k].kind });

            if (firstSelectable < 0)
            {
                firstSelectable = (int) rows.size();
                firstChip = 0;
            }

            rows.push_back (std::move (row));
            i = end;
        }
    }
    else
    {
        for (size_t i = 0; i < lastResult.alternatives.size(); ++i)
        {
            auto& alt = lastResult.alternatives[i];
            Row row;
            row.type = Row::Type::alternative;
            row.alt = (int) i;
            row.text = lastRequest.kind == ai::Kind::song ? alt.text : alt.lines.joinIntoString ("\n");

            if (alt.syllableLabel.isNotEmpty())
                row.labels.push_back ({ alt.syllableLabel, alt.syllableWarn });

            if (alt.rhymeLabel.isNotEmpty())
                row.labels.push_back ({ alt.rhymeLabel, alt.rhymeWarn });

            if (alt.guessedLabel.isNotEmpty())
                row.labels.push_back ({ alt.guessedLabel, false });

            if (lastRequest.kind != ai::Kind::song && alt.summary.isNotEmpty())
                row.labels.push_back ({ alt.summary, false });

            for (auto& s : alt.sectionSummaries)
                row.labels.push_back ({ s, false });

            if (alt.note.isNotEmpty())
                row.labels.push_back ({ alt.note, false });

            if (firstSelectable < 0)
                firstSelectable = (int) rows.size();

            rows.push_back (std::move (row));
        }
    }

    results->setRows (std::move (rows), resultsViewport.getMaximumVisibleWidth());
    resultsViewport.setViewPosition (0, 0);
    resultHelp.setVisible (panelOpen && isRhymes);

    selectedRow = selectedChip = -1;

    if (firstSelectable >= 0)
        selectResult (firstSelectable, firstChip);

    // The text the answer is for stays highlighted while its results are shown.
    if (lastContext.replaceStart < lastContext.replaceEnd)
        text->setHighlightedRegion ({ lastContext.replaceStart, lastContext.replaceEnd });

    if (lastResult.error.isNotEmpty() && ! isRhymes)
        setStatus (lastResult.error, true);
    else if (chatError.isNotEmpty() && ! isRhymes)
        setStatus (chatError, true);
    else if (isRhymes && chatError.isNotEmpty())
        setStatus ("Dictionary only. " + chatError, true);
    else if (isRhymes && modelBox.getText().isEmpty())
        setStatus ("Dictionary only: no Ollama model is available" + juce::String (modelError.isNotEmpty() ? " (" + modelError + ")" : ""));
    else if (lastResult.note.isNotEmpty())
        setStatus (lastResult.note);
    else if (isRhymes)
        setStatus ("Double-click a rhyme (or Use) to replace the word.");
    else
        setStatus ("Click an alternative to select it; double-click it (or Use) to put it in place.");

    updateUseState();
}

void LyricsEditWindow::selectResult (int row, int chip)
{
    selectedRow = row;
    selectedChip = chip;
    results->setSelected (row, chip);
    updateUseState();
}

void LyricsEditWindow::updateUseState()
{
    const bool unchanged = haveResult && ai::applyWord (text->getText(), lastContext, {}).has_value();
    const bool hasSelection = selectedRow >= 0 && selectedRow < results->getNumRows();
    const bool producedSomething = haveResult && (lastRequest.kind == ai::Kind::rhymes ? hasSelection : (hasSelection && ! lastResult.alternatives.empty()));

    useButton.setEnabled (! running && producedSomething && unchanged);
    againButton.setEnabled (! running && haveLastRequest && (! haveResult || unchanged));

    if (haveResult && ! unchanged && ! staleShown)
    {
        staleShown = true;
        setStatus ("The text changed here: ask again", true);
    }
}

void LyricsEditWindow::useSelected()
{
    if (running || ! haveResult || selectedRow < 0 || selectedRow >= results->getNumRows())
        return;

    const auto& row = results->getRow (selectedRow);
    std::optional<ai::Applied> applied;
    juce::String replacement;

    if (lastRequest.kind == ai::Kind::rhymes)
    {
        if (! juce::isPositiveAndBelow (selectedChip, (int) row.chips.size()))
            return;

        applied = ai::applyWord (text->getText(), lastContext, row.chips[(size_t) selectedChip].word);
    }
    else
    {
        if (! juce::isPositiveAndBelow (row.alt, (int) lastResult.alternatives.size()))
            return;

        applied = ai::applyAlternative (text->getText(), lastContext, lastResult.alternatives[(size_t) row.alt]);
    }

    if (! applied.has_value())
    {
        staleShown = true;
        setStatus ("The text changed here: ask again", true);
        useButton.setEnabled (false);
        return;
    }

    replacement = applied->text.substring (lastContext.replaceStart, applied->end);

    auto put = [safeThis = juce::Component::SafePointer<LyricsEditWindow> (this), applied, replacement]
    {
        if (auto* self = safeThis.getComponent())
            self->applyText (*applied, replacement);
    };

    if (lastRequest.kind == ai::Kind::song && lastContext.target.trim().isNotEmpty())
    {
        juce::AlertWindow::showAsync (juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon, "Replace all the lyrics?",
                                                                                     "The song's text in the box is replaced by this one. Cmd+Z in the text takes it back.",
                                                                                     "Replace", "Cancel", this),
                                      [put] (int result) { if (result == 1) put(); });
        return;
    }

    put();
}

void LyricsEditWindow::applyText (const ai::Applied& applied, const juce::String& replacement)
{
    // Through the box, so that Cmd+Z undoes it.
    if (lastContext.replaceStart == lastContext.replaceEnd)
        text->setCaretPosition (lastContext.replaceStart);
    else
        text->setHighlightedRegion ({ lastContext.replaceStart, lastContext.replaceEnd });

    text->insertTextAtCaret (replacement);

    if (text->getText() != applied.text)
        text->setText (applied.text, true);

    text->setHighlightedRegion ({ applied.start, applied.end });
    text->grabKeyboardFocus();
    updateUseState();
}

//==============================================================================
void LyricsEditWindow::updateActionLabels()
{
    selectionButton.setButtonText (text->getHighlightedRegion().isEmpty() ? "Write the next line" : "Rewrite selection");
}

void LyricsEditWindow::timerCallback()
{
    updateActionLabels();

    if (running)
    {
        spinnerStep = (spinnerStep + 1) % 4;
        const int n = receivedChars->load();
        setStatus ("Writing" + juce::String ("...").substring (0, spinnerStep) + (n > 0 ? " (" + juce::String (n) + " characters received)" : " waiting for the model"));
    }
}

juce::String LyricsEditWindow::getWordAtCaret() const
{
    const auto all = text->getText();
    const auto region = text->getHighlightedRegion();
    int a = region.isEmpty() ? text->getCaretPosition() : region.getStart();
    int b = region.isEmpty() ? a : region.getEnd();

    if (region.isEmpty())
    {
        while (a > 0 && isWordCharacter (all[a - 1]))
            --a;

        while (b < all.length() && isWordCharacter (all[b]))
            ++b;
    }

    return all.substring (a, b).trim();
}

void LyricsEditWindow::showSectionMenu()
{
    ai::Request probe;
    probe.kind = ai::Kind::sectionRewrite;
    probe.text = text->getText();
    probe.rangeStart = probe.rangeEnd = text->getCaretPosition();
    const auto context = ai::buildContext (probe);
    const auto next = findNextSectionWithoutWords (probe.text, engine.getSections());

    juce::PopupMenu menu;
    menu.addItem (1, context.error.isEmpty() ? "Rewrite [" + context.heading + "]" : juce::String ("Rewrite the section the cursor is in"), context.error.isEmpty());
    menu.addItem (2, next.has_value() ? "Write the next section without words: " + next->first : juce::String ("Write the next section without words"), next.has_value());
    menu.addSeparator();

    static const SectionType addTypes[] { SectionType::verse, SectionType::preChorus, SectionType::chorus, SectionType::bridge, SectionType::outro };

    for (int i = 0; i < (int) std::size (addTypes); ++i)
        menu.addItem (10 + i, "Add a " + sectionTypes::getName (addTypes[i]).toLowerCase() + " here");

    menu.addSeparator();
    guideicon::addItem (menu, 20, "How lyrics are written...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sectionButton),
                        [safeThis = juce::Component::SafePointer<LyricsEditWindow> (this), next] (int id)
    {
        auto* self = safeThis.getComponent();

        if (self == nullptr || id == 0)
            return;

        if (id == 1)
            self->startRequest (ai::Kind::sectionRewrite);
        else if (id == 2 && next.has_value())
            self->startRequest (ai::Kind::sectionAdd, next->first, next->second);
        else if (id >= 10 && id < 10 + (int) std::size (addTypes))
            self->startRequest (ai::Kind::sectionAdd, sectionTypes::getName (addTypes[id - 10]), 8);
        else if (id == 20 && self->onShowGuide != nullptr)
            self->onShowGuide (gen::lyricsguide::Topic::sections);
    });
}

void LyricsEditWindow::addPopupItems (juce::PopupMenu& menu)
{
    const auto word = getWordAtCaret();
    menu.addItem (1001, "Rewrite with AI...", ! text->getHighlightedRegion().isEmpty() && ! running);
    menu.addItem (1002, "Write the next line...", ! running);
    menu.addItem (1003, word.isNotEmpty() ? "Rhymes for \"" + word.substring (0, 24) + "\"..." : juce::String ("Rhymes..."), word.isNotEmpty() && ! running);
    menu.addSeparator();
    guideicon::addItem (menu, 1004, "How lyrics are written...");
}

void LyricsEditWindow::performPopupAction (int id)
{
    if (! panelOpen)
        setPanelOpen (true);

    if (id == 1001)
        startRequest (ai::Kind::selection);
    else if (id == 1002)
        startRequest (ai::Kind::nextLine);
    else if (id == 1003)
        startRequest (ai::Kind::rhymes);
    else if (id == 1004 && onShowGuide != nullptr)
        onShowGuide (gen::lyricsguide::Topic::sections);
}
} // namespace daw
