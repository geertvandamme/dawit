#include "NewFromStyleWindow.h"

#include "generate/StylesGuide.h"
#include "Theme.h"

namespace daw
{
namespace
{
    constexpr const char* styleKey = "newFromStyle.style";
    constexpr const char* notesKey = "newFromStyle.writeNotes";
}

NewFromStyleWindow::NewFromStyleWindow (Engine& e, ProjectManager& p) : engine (e), project (p)
{
    title.setText ("New Project from Style", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);
    summary.setFont (juce::FontOptions (14.0f));
    summary.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (summary);

    list.setModel (this);
    list.setRowHeight (34);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);
    addAndMakeVisible (picture);

    keyLabel.setText ("Key", juce::dontSendNotification);
    tempoLabel.setText ("Tempo", juce::dontSendNotification);
    addAndMakeVisible (keyLabel);
    addAndMakeVisible (tempoLabel);
    keyBox.addItem ("From the style", 1);
    for (int tonic = 0; tonic < 12; ++tonic) keyBox.addItem (MusicalKey::getTonicName (tonic), tonic + 2);
    keyBox.setSelectedId (1);
    keyBox.getRootMenu()->addSeparator();
    guideicon::addItem (*keyBox.getRootMenu(), keyGuideItemId, "Chords and keys guide...");
    keyBox.onChange = [this]
    {
        if (keyBox.getSelectedId() == keyGuideItemId)
        {
            keyBox.setSelectedId (lastKeyId, juce::dontSendNotification); // not a key: keep the choice
            if (onShowKeyGuide) onShowKeyGuide();
        }
        else
            lastKeyId = keyBox.getSelectedId();
    };
    keyBox.setTooltip ("The song's key. From the style uses one the style often uses; any key works: pick one that suits your voice.");
    addAndMakeVisible (keyBox);
    tempo.setSliderStyle (juce::Slider::LinearHorizontal);
    tempo.setTextBoxStyle (juce::Slider::TextBoxRight, false, 72, 24);
    tempo.setTextValueSuffix (" BPM");
    addAndMakeVisible (tempo);
    writeNotes.setTooltip ("Off: empty clips per section, for you to fill.");
    writeNotes.setToggleState (engine.properties.getUserSettings()->getBoolValue (notesKey, true), juce::dontSendNotification);
    writeNotes.onClick = [this] { engine.properties.getUserSettings()->setValue (notesKey, writeNotes.getToggleState()); };
    addAndMakeVisible (writeNotes);

    whatButton.onClick = [this]
    {
        if (const auto* style = getStyle(); style != nullptr && onShowGuide)
            for (auto topic : gen::stylesguide::allTopics)
                if (gen::stylesguide::getStyleId (topic) == style->id) { onShowGuide (topic); break; }
    };
    explainButton.setTooltip ("Open Styles of music at the advice for this song");
    explainButton.onClick = [this] { if (onShowGuide) onShowGuide (gen::stylesguide::Topic::yourSong); };
    shuffleButton.setTooltip ("Another song in the same style");
    shuffleButton.onClick = [this] { create (true); };
    createButton.setTooltip ("Make an untitled project from these choices");
    createButton.onClick = [this] { create (false); };
    for (auto* button : std::initializer_list<juce::TextButton*> { &whatButton, &explainButton, &shuffleButton, &createButton }) addAndMakeVisible (button);
    status.setColour (juce::Label::textColourId, theme::textDim);
    status.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (status);

    list.updateContent();
    showStyle (engine.properties.getUserSettings()->getValue (styleKey, "dance-pop"));
    setSize (width, height);
}

const gen::templates::Style* NewFromStyleWindow::getStyle() const
{
    const int row = list.getSelectedRow();
    return juce::isPositiveAndBelow (row, (int) gen::templates::getStyles().size()) ? &gen::templates::getStyles()[(size_t) row] : nullptr;
}

void NewFromStyleWindow::showStyle (const juce::String& styleId)
{
    int row = 0;
    for (size_t i = 0; i < gen::templates::getStyles().size(); ++i)
        if (gen::templates::getStyles()[i].id == styleId) row = (int) i;
    list.selectRow (row);
}

void NewFromStyleWindow::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, getNumRows())) return;
    if (selected) { g.setColour (theme::accent.withAlpha (0.35f)); g.fillRect (0, 0, w, h); }
    const auto& style = gen::templates::getStyles()[(size_t) row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (style.name, 9, 0, w - 78, h, juce::Justification::centredLeft, 1);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String (style.minimumBpm) + "-" + juce::String (style.maximumBpm), w - 68, 0, 60, h, juce::Justification::centredRight);
}

juce::String NewFromStyleWindow::getTooltipForRow (int row)
{
    if (! juce::isPositiveAndBelow (row, getNumRows())) return {};
    const auto& style = gen::templates::getStyles()[(size_t) row];
    return style.name + " " + juce::String::fromUTF8 ("\xc2\xb7") + " " + juce::String (style.minimumBpm) + " to "
           + juce::String (style.maximumBpm) + " BPM " + juce::String::fromUTF8 ("\xc2\xb7") + " " + style.structureName
           + " structure. Click to see it, double-click to make a song in it.";
}

void NewFromStyleWindow::selectedRowsChanged (int row)
{
    if (row >= 0) updateStyle();
}

void NewFromStyleWindow::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    list.selectRow (row);
    create (false);
}

void NewFromStyleWindow::updateStyle()
{
    const auto* style = getStyle();
    if (style == nullptr) return;
    seed = 0;
    tempo.setRange (style->minimumBpm, style->maximumBpm, 1.0);
    tempo.setValue (style->defaultBpm, juce::dontSendNotification);
    tempo.setTooltip (style->name + " usually runs from " + juce::String (style->minimumBpm) + " to " + juce::String (style->maximumBpm) + " BPM.");
    summary.setText (style->summary, juce::dontSendNotification);
    whatButton.setButtonText ("What is " + style->name.toLowerCase() + "?");
    const auto plan = gen::templates::makePlan (*style, std::nullopt, style->defaultBpm, 0, writeNotes.getToggleState());
    picture.show (StylePicture::Mode::arrangement, plan, style, style->defaultBpm);
    engine.properties.getUserSettings()->setValue (styleKey, style->id);
}

void NewFromStyleWindow::create (bool shuffle)
{
    const auto* style = getStyle();
    if (style == nullptr) return;
    if (shuffle) ++seed;
    const auto tonic = keyBox.getSelectedId() == 1 ? std::optional<int>() : std::optional<int> (keyBox.getSelectedId() - 2);
    const auto plan = gen::templates::makePlan (*style, tonic, tempo.getValue(), seed, writeNotes.getToggleState());
    const bool replaceWithoutAsking = lastMadeText.isNotEmpty() && daw::project::toText (engine, {}) == lastMadeText;
    auto error = std::make_shared<juce::String>();
    const auto safe = juce::Component::SafePointer<NewFromStyleWindow> (this);
    auto* const edit = &engine;
    project.newProjectFrom ([edit, plan, error] { return gen::templates::build (*edit, plan, *error); }, replaceWithoutAsking,
                            [safe, plan, error] (bool made)
    {
        if (auto* self = safe.getComponent())
        {
            if (made)
            {
                self->lastMadeText = daw::project::toText (self->engine, {});
                self->status.setText ("Made " + plan.styleName + " in " + plan.key.getName() + " at " + juce::String (plan.tempo, 0)
                                      + " BPM: " + juce::String ((int) plan.sections.size()) + " sections, "
                                      + juce::String ((int) self->engine.getTracks().size()) + " tracks, "
                                      + gen::structure::formatDuration (plan.getTotalBars(), plan.tempo) + ". Press Space to play.",
                                      juce::dontSendNotification);
                self->picture.show (StylePicture::Mode::arrangement, plan, gen::templates::findStyle (plan.styleId), plan.tempo);
            }
            else
            {
                self->status.setText ("Couldn't make the song: " + *error, juce::dontSendNotification);
            }
        }
    });
}

void NewFromStyleWindow::paint (juce::Graphics& g) { g.fillAll (theme::panel); }

void NewFromStyleWindow::resized()
{
    auto r = getLocalBounds().reduced (16, 12);
    title.setBounds (r.removeFromTop (30));
    r.removeFromTop (10);
    list.setBounds (r.removeFromLeft (220));
    r.removeFromLeft (14);
    summary.setBounds (r.removeFromTop (42));
    auto what = r.removeFromTop (28);
    whatButton.setBounds (what.removeFromRight (200));
    r.removeFromTop (6);
    picture.setBounds (r.removeFromTop (220));
    r.removeFromTop (10);
    auto controls = r.removeFromTop (28);
    keyLabel.setBounds (controls.removeFromLeft (32));
    keyBox.setBounds (controls.removeFromLeft (150));
    controls.removeFromLeft (12);
    tempoLabel.setBounds (controls.removeFromLeft (48));
    tempo.setBounds (controls.removeFromLeft (220));
    controls.removeFromLeft (10);
    writeNotes.setBounds (controls);
    r.removeFromTop (8);
    status.setBounds (r.removeFromTop (38));
    r.removeFromTop (8);
    auto buttons = r.removeFromBottom (30);
    explainButton.setBounds (buttons.removeFromLeft (170));
    createButton.setBounds (buttons.removeFromRight (92));
    buttons.removeFromRight (8);
    shuffleButton.setBounds (buttons.removeFromRight (92));
}
} // namespace daw
