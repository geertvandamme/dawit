#include "StylesExplainer.h"

#include "Theme.h"

namespace daw
{
namespace sg = gen::stylesguide;

namespace
{
gen::templates::SongPlan makeSongPicture (const Engine& engine)
{
    gen::templates::SongPlan plan;
    plan.styleName = "Your song";
    plan.tempo = engine.getTempo();
    plan.key = engine.getKey();
    plan.sections = engine.getSections();
    plan.chords = engine.getChords();

    for (const auto& track : engine.getTracks())
    {
        if (track->hasInput()) continue;

        gen::templates::PartPlan part;
        part.name = track->name;
        part.sound = track->builtInSound.load();
        part.volumeDb = track->volumeDb.load();
        part.pan = track->pan.load();

        for (const auto& clip : track->clips)
        {
            gen::templates::ClipPlan clipPlan;
            clipPlan.name = clip->name;
            clipPlan.startBeat = clip->startBeat;
            clipPlan.lengthBeats = clip->getLengthBeats (engine.getTempoMap());
            if (clip->kind == Clip::Kind::midi)
                clipPlan.notes = static_cast<const MidiClip&> (*clip).notes;
            part.clips.push_back (std::move (clipPlan));
        }
        plan.parts.push_back (std::move (part));
    }
    return plan;
}
}

StylesExplainer::StylesExplainer (Engine& e) : engine (e)
{
    title.setText ("Styles of music", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (45);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    addAndMakeVisible (pictureHolder);
    for (auto* picture : { &tempoPicture, &arrangementPicture, &groovePicture })
        pictureHolder.addAndMakeVisible (picture);

    playButton.setTooltip ("Play two bars of this style's drum pattern");
    playButton.onClick = [this] { playGroove(); };
    addAndMakeVisible (playButton);
    newSongButton.setTooltip ("Open New Project from Style with this style selected");
    newSongButton.onClick = [this]
    {
        const auto topic = sg::allTopics[(size_t) juce::jlimit (0, getNumRows() - 1, list.getSelectedRow())];
        if (onNewSong) onNewSong (sg::getStyleId (topic));
    };
    addAndMakeVisible (newSongButton);

    explanation.setMultiLine (true, true);
    explanation.setReadOnly (true);
    explanation.setCaretVisible (false);
    explanation.setScrollbarsShown (true);
    explanation.setFont (juce::FontOptions (15.0f));
    explanation.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    explanation.setColour (juce::TextEditor::textColourId, theme::text);
    explanation.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    explanation.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    explanation.setBorder (juce::BorderSize<int> (10, 12, 10, 12));
    addAndMakeVisible (explanation);
    addAndMakeVisible (exportButton);

    engine.addChangeListener (this);
    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
}

StylesExplainer::~StylesExplainer()
{
    stopTimer();
    engine.removeChangeListener (this);
}

juce::String StylesExplainer::getPrintSource() const
{
    return "the song at " + juce::String (engine.getTempo(), 1) + " BPM and DawIt's fourteen templates";
}

booklet::Topic StylesExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0) rowBeforePrinting = juce::jmax (0, list.getSelectedRow());
    index = juce::jlimit (0, getNumRows() - 1, index);
    showRow (index);
    const auto topic = sg::allTopics[(size_t) index];
    return { sg::getTopicName (topic), sg::getTopicSummary (topic), explanation.getText(),
             { pictureHolder.getWidth(), pictureHolder.getHeight() }, {} };
}

juce::Image StylesExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (pictureHolder, scale);
}

void StylesExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0) showRow (rowBeforePrinting);
    rowBeforePrinting = -1;
}

void StylesExplainer::showTopic (sg::Topic topic)
{
    const auto found = std::find (std::begin (sg::allTopics), std::end (sg::allTopics), topic);
    list.selectRow (found == std::end (sg::allTopics) ? 0 : (int) std::distance (std::begin (sg::allTopics), found));
}

void StylesExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, getNumRows())) return;
    if (selected) { g.setColour (theme::accent.withAlpha (0.35f)); g.fillRect (0, 0, w, h); }
    const auto topic = sg::allTopics[(size_t) row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawFittedText (sg::getTopicName (topic), 10, 4, w - 16, 18, juce::Justification::centredLeft, 1);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.5f));
    g.drawFittedText (sg::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, 1);
}

void StylesExplainer::selectedRowsChanged (int row)
{
    if (row >= 0) showRow (row);
}

void StylesExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (list.getSelectedRow() == getNumRows() - 1) showRow (getNumRows() - 1);
}

void StylesExplainer::showRow (int row)
{
    stopTimer();
    grooveStep = 0;
    row = juce::jlimit (0, getNumRows() - 1, row);
    const auto topic = sg::allTopics[(size_t) row];
    const auto styleId = sg::getStyleId (topic);
    const auto* style = gen::templates::findStyle (styleId);
    auto example = topic == sg::Topic::yourSong ? makeSongPicture (engine) : sg::getExample (topic);

    tempoPicture.setVisible (topic == sg::Topic::styles || style != nullptr || topic == sg::Topic::yourSong);
    arrangementPicture.setVisible (topic == sg::Topic::parts || style != nullptr || topic == sg::Topic::yourSong);
    groovePicture.setVisible (topic == sg::Topic::routing || style != nullptr);
    tempoPicture.show (StylePicture::Mode::tempoMap, example, style, engine.getTempo());
    arrangementPicture.show (StylePicture::Mode::arrangement, example, style, engine.getTempo());
    groovePicture.show (topic == sg::Topic::routing ? StylePicture::Mode::routing : StylePicture::Mode::groove, example, style, engine.getTempo());
    playButton.setVisible (style != nullptr);
    newSongButton.setVisible (style != nullptr);
    explanation.setText (sg::explain (topic, engine), false);
    explanation.moveCaretToTop (false);
    resized();
}

void StylesExplainer::playGroove()
{
    const auto topic = sg::allTopics[(size_t) juce::jlimit (0, getNumRows() - 1, list.getSelectedRow())];
    const auto* style = gen::templates::findStyle (sg::getStyleId (topic));
    if (style == nullptr) return;
    grooveStep = 0;
    startTimer (juce::jmax (35, juce::roundToInt (60000.0 / style->defaultBpm / 4.0)));
}

void StylesExplainer::timerCallback()
{
    const auto topic = sg::allTopics[(size_t) juce::jlimit (0, getNumRows() - 1, list.getSelectedRow())];
    const auto* style = gen::templates::findStyle (sg::getStyleId (topic));
    if (style == nullptr || grooveStep >= 32) { stopTimer(); return; }
    const double beat = grooveStep * 0.25;
    for (const auto& drum : style->drums)
        for (const auto& hit : drum.hits)
            if (std::abs (std::fmod (beat - hit.beat + drum.patternBeats, drum.patternBeats)) < 0.01)
                engine.previewNote (drum.pitch, hit.velocity, 90);
    ++grooveStep;
}

void StylesExplainer::paint (juce::Graphics& g) { g.fillAll (theme::panel); }

void StylesExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);
    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);
    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);
    pictureHolder.setBounds (r.removeFromTop (235));
    auto p = pictureHolder.getLocalBounds();
    const int visible = (tempoPicture.isVisible() ? 1 : 0) + (arrangementPicture.isVisible() ? 1 : 0) + (groovePicture.isVisible() ? 1 : 0);
    const int gap = 5;
    const int each = visible > 0 ? (p.getHeight() - gap * (visible - 1)) / visible : p.getHeight();
    for (auto* picture : { &tempoPicture, &arrangementPicture, &groovePicture })
        if (picture->isVisible()) { picture->setBounds (p.removeFromTop (each)); p.removeFromTop (gap); }
    r.removeFromTop (8);
    auto buttons = r.removeFromTop ((playButton.isVisible() || newSongButton.isVisible()) ? 30 : 0);
    if (playButton.isVisible()) playButton.setBounds (buttons.removeFromLeft (145));
    if (newSongButton.isVisible()) newSongButton.setBounds (buttons.removeFromRight (205));
    if (buttons.getHeight() > 0) r.removeFromTop (8);
    explanation.setBounds (r);
}
} // namespace daw
