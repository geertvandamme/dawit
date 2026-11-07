#include "EffectWindow.h"
#include "engine/Tempo.h"
#include "engine/TrackNameWords.h"

namespace daw::fxui
{
namespace
{
    std::function<HostTrack (const juce::AudioProcessor&)> hostTrackLookup;
}

void setHostTrackLookup (std::function<HostTrack (const juce::AudioProcessor&)> lookup)
{
    hostTrackLookup = std::move (lookup);
}

HostTrack getHostTrack (const juce::AudioProcessor& processor)
{
    // A plugin in a host has no DawIt track to look up, even if this process also runs DawIt (a screenshot run does).
    if (isPlugin (processor))
        return HostTrack { trackname::pluginTrackName, false, false, {} };

    return hostTrackLookup ? hostTrackLookup (processor) : HostTrack {};
}

bool isPlugin (const juce::AudioProcessor& processor)
{
    return processor.wrapperType != juce::AudioProcessor::wrapperType_Undefined;
}

PluginMode::PluginMode (juce::AudioProcessorEditor& e) : editor (e)
{
    if (! isPlugin (*editor.getAudioProcessor()))
        return;

    lookAndFeel = std::make_unique<DawLookAndFeel>();
    editor.setLookAndFeel (lookAndFeel.get());
    tooltips = std::make_unique<juce::TooltipWindow> (&editor, 600);
}

PluginMode::~PluginMode()
{
    tooltips.reset();

    if (lookAndFeel != nullptr)
        editor.setLookAndFeel (nullptr);
}

void addPluginParagraph (juce::AttributedString& s, const juce::AudioProcessor& processor)
{
    if (! isPlugin (processor))
        return;

    auto name = processor.getName();
    name = name.startsWith ("DawIt ") ? name.fromFirstOccurrenceOf ("DawIt ", false, false) : name;
    addSection (s, "Part of DawIt",
                "This is " + name + " from DawIt, a free app for learning music theory while you make music. Here it works like any plugin: "
                "the knobs, the starting points and the guide are the same as in the app. In DawIt it also sees the song around it: its window "
                "names the track it's on, the track shows how loud the sound is at each step, and some guides, such as Reverb's, give "
                "advice by the track's name (Vocal, Snare and so on). There you can also save a guide as a PDF.");
}

void setTopic (juce::Component& c, int topic)
{
    c.getProperties().set ("fxTopic", topic);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillRoundedRectangle (area, 4.0f);
}

void addTitle (juce::AttributedString& s, const juce::String& text, juce::Colour colour)
{
    s.append (text + "\n", juce::Font (juce::FontOptions (16.0f, juce::Font::bold)), colour);
}

void addLead (juce::AttributedString& s, const juce::String& text)
{
    s.append (text + "\n\n", juce::Font (juce::FontOptions (13.5f)), theme::accent.brighter (0.4f));
}

void addSection (juce::AttributedString& s, const juce::String& heading, const juce::String& text)
{
    if (heading.isNotEmpty())
        s.append (heading + "\n", juce::Font (juce::FontOptions (12.5f, juce::Font::bold)), theme::text);

    s.append (text + "\n\n", juce::Font (juce::FontOptions (12.5f)), theme::textDim.brighter (0.35f));
}

juce::String formatSeconds (double seconds)
{
    if (seconds >= 1.0)
        return juce::String (seconds, 2) + " s";

    return juce::String (juce::roundToInt (seconds * 1000.0)) + " ms";
}

//==============================================================================
void ExplainerPanel::show (const juce::AttributedString& newText, const juce::String& page)
{
    if (page != shownPage)
    {
        shownPage = page;
        scroll = 0.0f;
    }
    else if (newText.getText() == shownText)
    {
        return;
    }

    shownText = newText.getText();
    text = newText;
    resized();
    repaint();
}

void ExplainerPanel::resized()
{
    const auto area = getLocalBounds().reduced (14);
    layout.createLayout (text, (float) juce::jmax (10, area.getWidth()));
    scroll = juce::jlimit (0.0f, juce::jmax (0.0f, layout.getHeight() - (float) area.getHeight()), scroll);
}

void ExplainerPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const float most = juce::jmax (0.0f, layout.getHeight() - (float) getLocalBounds().reduced (14).getHeight());

    if (most <= 0.0f)
    {
        Component::mouseWheelMove (e, wheel);
        return;
    }

    scroll = juce::jlimit (0.0f, most, scroll - wheel.deltaY * (wheel.isReversed ? -160.0f : 160.0f));
    repaint();
}

void ExplainerPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, 0, 1, getHeight());

    const auto area = getLocalBounds().reduced (14).toFloat();

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (area.toNearestInt());
        layout.draw (g, area.translated (0.0f, -scroll).withHeight (juce::jmax (area.getHeight(), layout.getHeight())));
    }

    // Fade the text out where there's more to scroll to.
    if (layout.getHeight() - scroll > area.getHeight() + 1.0f)
    {
        g.setGradientFill (juce::ColourGradient (theme::panel.withAlpha (0.0f), 0.0f, area.getBottom() - 34.0f, theme::panel, 0.0f, area.getBottom(), false));
        g.fillRect (area.withTop (area.getBottom() - 34.0f));
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("scroll for more", area.withTop (area.getBottom() - 13.0f), juce::Justification::centredRight);
    }

    if (scroll > 0.5f)
    {
        g.setGradientFill (juce::ColourGradient (theme::panel, 0.0f, area.getY(), theme::panel.withAlpha (0.0f), 0.0f, area.getY() + 24.0f, false));
        g.fillRect (area.withHeight (24.0f));
    }
}

//==============================================================================
EffectKnob::EffectKnob (juce::AudioProcessorValueTreeState& s, const juce::String& parameterId, const juce::String& name, int topic, juce::Colour colour)
    : label ({}, name), state (s)
{
    setTopic (*this, topic);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, theme::text);

    slider.setScrollWheelEnabled (false);   // scrolling never changes a value
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    slider.setColour (juce::Slider::textBoxTextColourId, theme::text);
    slider.setWantsKeyboardFocus (false);
    attachTo (parameterId);

    addAndMakeVisible (label);
    addAndMakeVisible (slider);
}

void EffectKnob::attachTo (const juce::String& parameterId)
{
    if (parameterId == attachedId)
        return;

    attachment.reset();
    attachedId = parameterId;
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, slider);

    if (auto* parameter = state.getParameter (parameterId))
        slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
}

void EffectKnob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

//==============================================================================
LevelHistoryView::LevelHistoryView (fx::LevelHistory& h, std::function<double()> getSampleRate, std::function<double()> getBpm,
                                    const juce::String& effectName, juce::Colour c, const juce::String& input, const juce::String& suffix)
    : history (h), sampleRate (std::move (getSampleRate)), bpm (std::move (getBpm)), name (effectName), inputName (input),
      effectSuffix (suffix), colour (c)
{
    ring.resize (8192);
    incoming.resize (16384);
    setTopic (*this, CommonTopic::history);
}

double LevelHistoryView::getPointsPerSecond() const
{
    const double rate = sampleRate != nullptr && sampleRate() > 0.0 ? sampleRate() : 48000.0;
    return rate / fx::LevelHistory::chunk;
}

void LevelHistoryView::update()
{
    const int n = history.read (incoming.data(), (int) incoming.size());

    for (int i = 0; i < n; ++i)
    {
        ring[(size_t) ringPosition] = incoming[(size_t) i];
        ringPosition = (ringPosition + 1) % (int) ring.size();
    }

    filled = juce::jmin ((int) ring.size(), filled + n);
    repaint();
}

std::vector<fx::LevelPoint> LevelHistoryView::getNewest (int count) const
{
    count = juce::jlimit (0, filled, count);
    std::vector<fx::LevelPoint> points ((size_t) count);

    for (int i = 0; i < count; ++i)
        points[(size_t) i] = ring[(size_t) ((ringPosition - count + i + (int) ring.size()) % (int) ring.size())];

    return points;
}

std::pair<float, float> LevelHistoryView::getLoudest() const
{
    float in = -100.0f, effect = -100.0f;

    for (auto& p : getNewest ((int) (seconds * getPointsPerSecond())))
    {
        in = juce::jmax (in, p.inputDb);
        effect = juce::jmax (effect, p.effectDb);
    }

    return { in, effect };
}

void LevelHistoryView::paint (juce::Graphics& g)
{
    constexpr float lowestDb = -60.0f;
    auto bounds = getLocalBounds().toFloat();
    drawPanel (g, bounds);
    auto legend = bounds.removeFromTop (18.0f).reduced (8.0f, 0.0f);
    auto times = bounds.removeFromBottom (14.0f);
    auto labels = bounds.removeFromLeft (30.0f);
    auto area = bounds.reduced (0.0f, 4.0f).withTrimmedRight (8.0f);
    auto toY = [&] (float db) { return juce::jmap (juce::jlimit (lowestDb, 0.0f, db), lowestDb, 0.0f, area.getBottom(), area.getY()); };

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= lowestDb; db -= 12.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (area.getX(), toY (db), area.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.drawText (juce::String (juce::roundToInt (db)), labels.withHeight (12.0f).withCentre ({ labels.getCentreX(), toY (db) }),
                    juce::Justification::centredRight);
    }

    g.drawText ("4 s ago", times.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
    g.drawText ("now", times.withTrimmedRight (8.0f), juce::Justification::centredRight);

    const int shown = (int) (seconds * getPointsPerSecond());
    const auto points = getNewest (shown);
    const int missing = shown - (int) points.size();
    const int columns = juce::jmax (1, (int) area.getWidth());
    juce::Path effect;
    bool started = false;

    for (int c = 0; c < columns; ++c)
    {
        const int from = c * shown / columns - missing, to = (c + 1) * shown / columns - missing;

        if (to <= 0)
            continue;

        fx::LevelPoint most;

        for (int i = juce::jmax (0, from); i < to; ++i)
        {
            most.inputDb = juce::jmax (most.inputDb, points[(size_t) i].inputDb);
            most.effectDb = juce::jmax (most.effectDb, points[(size_t) i].effectDb);
        }

        const float x = area.getX() + (float) c;
        g.setColour (theme::textDim.withAlpha (0.35f));
        g.fillRect (x, toY (most.inputDb), 1.0f, area.getBottom() - toY (most.inputDb));
        g.setColour (colour.withAlpha (0.35f));
        g.fillRect (x, toY (most.effectDb), 1.0f, area.getBottom() - toY (most.effectDb));

        if (! started)
        {
            effect.startNewSubPath (x, toY (most.effectDb));
            started = true;
        }
        else
        {
            effect.lineTo (x, toY (most.effectDb));
        }
    }

    g.setColour (colour);
    g.strokePath (effect, juce::PathStrokeType (1.5f));

    // The beats, so a tail or the echoes can be read against the tempo. They don't move with the song,
    // but they are as far apart as its beats.
    const double tempo = bpm != nullptr && bpm() > 0.0 ? bpm() : 0.0;

    if (tempo > 0.0)
    {
        const double beat = 60.0 / tempo;
        g.setColour (theme::gridBar.withAlpha (0.5f));

        for (double t = beat; t < seconds; t += beat)
            g.fillRect (area.getRight() - (float) (t / seconds) * area.getWidth(), area.getY(), 1.0f, area.getHeight());
    }

    g.setFont (juce::FontOptions (10.5f));
    auto key = [&] (juce::Colour c, const juce::String& text)
    {
        g.setColour (c);
        g.fillRect (legend.removeFromLeft (9.0f).withSizeKeepingCentre (9.0f, 9.0f));
        legend.removeFromLeft (4.0f);
        g.setColour (theme::textDim);
        const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 12.0f;
        g.drawText (text, legend.removeFromLeft (w), juce::Justification::centredLeft);
    };

    key (theme::textDim.withAlpha (0.6f), inputName);
    key (colour, name + effectSuffix);

    if (tempo > 0.0)
        key (theme::gridBar, "beats at " + formatBpm (tempo) + " BPM");
}

//==============================================================================
class Guide::Picture final : public juce::Component
{
public:
    explicit Picture (Guide& g) : guide (g) {}

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (r, 4.0f);

        if (! juce::isPositiveAndBelow (guide.page, (int) guide.pages.size()))
            return;

        const auto& current = guide.pages[(size_t) guide.page];

        if (current.paint != nullptr)
            current.paint (g, r.reduced (16.0f, 12.0f), juce::Time::getMillisecondCounterHiRes() / 1000.0 - guide.clapTime);
    }

private:
    Guide& guide;
};

Guide::Guide (const juce::String& titleText, std::vector<Page> p, std::function<void()> clap, const juce::String& hint, const juce::String& clapText,
              const juce::String& sourcesKey)
    : pages (std::move (p)), onClap (std::move (clap)), defaultClapText (clapText)
{
    title.setText (titleText, juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    hintLabel.setText (hint, juce::dontSendNotification);
    hintLabel.setFont (juce::FontOptions (12.5f));
    hintLabel.setColour (juce::Label::textColourId, theme::textDim);
    hintLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (hintLabel);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture> (*this);
    addAndMakeVisible (*picture);

    clapButton.setButtonText (clapText);
    clapButton.setWantsKeyboardFocus (false);
    clapButton.setTooltip ("Play a test sound through the effect, with the settings in its window");
    clapButton.onClick = [this] { playTestSound(); };
    addChildComponent (clapButton);
    clapButton.setVisible (onClap != nullptr);

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

    if (sourcesKey.isNotEmpty())
        sources = std::make_unique<GuideSourcesTopic> (*this, sourcesKey);

    list.updateContent();
    showPage (0);
    startTimerHz (30);
    setSize (width, height);
}

Guide::~Guide()
{
    list.setModel (nullptr);
}

void Guide::playTestSound()
{
    clapTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    if (onSources())
        return;

    if (! pages.empty() && pages[(size_t) page].ownClap)
        pages[(size_t) page].ownClap();
    else if (onClap != nullptr)
        onClap();
}

void Guide::showPage (int index)
{
    page = juce::jlimit (0, juce::jmax (0, getNumRows() - 1), index);

    if (list.getSelectedRow() != page)
        list.selectRow (page);   // comes back through selectedRowsChanged

    // The Sources topic has no picture, test sound or explanation: its panel takes their place (see resized).
    picture->setVisible (! onSources());
    explanation.setVisible (! onSources());

    if (onSources())
    {
        clapButton.setVisible (false);
        resized();
    }
    else if (! pages.empty())
    {
        const auto& current = pages[(size_t) page];
        explanation.setText (current.liveText ? current.liveText() : current.text, false);
        clapButton.setButtonText (current.clapText.isEmpty() ? defaultClapText : current.clapText);
        clapButton.setTooltip (current.clapTooltip.isEmpty() ? "Play a test sound through the effect, with the settings in its window" : current.clapTooltip);
        clapButton.setVisible (current.ownClap != nullptr || (onClap != nullptr && ! current.noClap));
        resized();
    }

    explanation.moveCaretToTop (false);
    picture->repaint();
}

void Guide::setPrintSource (const juce::String& text, const juce::String& plugin)
{
    printSource = text;

    if (exportButton != nullptr)
        return;

    if (const auto& make = guideprint::exportButtonFactory())
    {
        exportButton = make (*this, plugin);

        if (exportButton != nullptr)
        {
            addAndMakeVisible (*exportButton);
            resized();
        }
    }
}

juce::String Guide::getPrintTitle() const
{
    return title.getText();
}

booklet::Topic Guide::showPrintTopic (int index)
{
    if (pageBeforePrinting < 0)
        pageBeforePrinting = page;

    showPage (index);
    booklet::Topic topic;

    if (onSources())
        return sources->getPrintTopic();

    if (juce::isPositiveAndBelow (page, (int) pages.size()))
    {
        topic.name = pages[(size_t) page].name;
        topic.summary = pages[(size_t) page].summary;
        topic.text = pages[(size_t) page].liveText ? pages[(size_t) page].liveText() : pages[(size_t) page].text;
    }

    topic.pictureSize = { picture->getWidth(), picture->getHeight() };
    return topic;
}

juce::Image Guide::paintPrintPicture (float scale)
{
    const auto bounds = picture->getLocalBounds();
    juce::Image image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) bounds.getWidth() * scale)),
                       juce::jmax (1, juce::roundToInt ((float) bounds.getHeight() * scale)), true, juce::SoftwareImageType());
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (scale));

    const auto r = bounds.toFloat();
    g.setColour (theme::panelLight);
    g.fillRoundedRectangle (r, 4.0f);

    if (juce::isPositiveAndBelow (page, (int) pages.size()) && pages[(size_t) page].paint != nullptr)
        pages[(size_t) page].paint (g, r.reduced (16.0f, 12.0f), printSeconds);

    return image;
}

void Guide::endPrinting()
{
    if (pageBeforePrinting >= 0)
        showPage (pageBeforePrinting);

    pageBeforePrinting = -1;
}

int Guide::getNumRows()
{
    return (int) pages.size() + (sources != nullptr ? 1 : 0);
}

void Guide::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (sources != nullptr && GuideSourcesTopic::isRow (row, (int) pages.size()))
    {
        sources->paintRow (g, w, h, selected);
        return;
    }

    if (! juce::isPositiveAndBelow (row, (int) pages.size()))
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (pages[(size_t) row].name, 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (pages[(size_t) row].summary, 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void Guide::selectedRowsChanged (int row)
{
    if (juce::isPositiveAndBelow (row, getNumRows()) && row != page)
        showPage (row);
}

void Guide::timerCallback()
{
    if (! onSources() && ! pages.empty() && pages[(size_t) page].liveText)
    {
        const auto updated = pages[(size_t) page].liveText();
        if (updated != explanation.getText())
            explanation.setText (updated, false);
    }
    // The pictures follow the effect's settings and move after a clap.
    picture->repaint();
}

void Guide::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void Guide::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    title.setBounds (top.removeFromLeft (260));

    if (exportButton != nullptr)
    {
        exportButton->setBounds (top.removeFromRight (120).withSizeKeepingCentre (120, 26));
        top.removeFromRight (10);
    }

    hintLabel.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (260));
    r.removeFromLeft (14);

    if (sources != nullptr)
        sources->show (onSources(), r);

    if (onSources())
        return;

    // In a short window (a plugin's) the picture gives way, so the text keeps room to be read.
    picture->setBounds (r.removeFromTop (juce::jlimit (180, pictureHeight, r.getHeight() - 260)));
    r.removeFromTop (10);

    if (clapButton.isVisible())
    {
        const bool customButton = ! pages.empty() && pages[(size_t) page].clapText.isNotEmpty();
        clapButton.setBounds (r.removeFromTop (26).removeFromLeft (customButton ? 190 : 110));
        r.removeFromTop (10);
    }

    explanation.setBounds (r);
}

//==============================================================================
GuideWindow::GuideWindow (const juce::String& name, Guide* guide)
    : juce::DocumentWindow (name, theme::panel, juce::DocumentWindow::closeButton)
{
    setUsingNativeTitleBar (true);
    setContentOwned (guide, true);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

void GuideWindow::show (int page)
{
    if (auto* guide = dynamic_cast<Guide*> (getContentComponent()); guide != nullptr && page >= 0)
        guide->showPage (page);

    setVisible (true);
    toFront (true);
}

/** The guide inside a plugin's window: a thin bar with the Back button over the guide. */
class GuideHost::Overlay final : public juce::Component
{
public:
    Overlay (Guide* guideToOwn, std::function<void()> back) : guide (guideToOwn), onBack (std::move (back))
    {
        setWantsKeyboardFocus (true);
        backButton.setTooltip ("Back to the knobs and pictures (Esc)");
        backButton.onClick = [this] { onBack(); };
        addAndMakeVisible (backButton);
        addAndMakeVisible (*guide);
    }

    Guide& getGuide() { return *guide; }

    void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

    void resized() override
    {
        auto r = getLocalBounds();
        auto bar = r.removeFromTop (38).reduced (16, 6);
        backButton.setBounds (bar.removeFromLeft (190));
        guide->setBounds (r);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            onBack();
            return true;
        }

        return false;
    }

private:
    std::unique_ptr<Guide> guide;
    std::function<void()> onBack;
    juce::TextButton backButton { juce::String::fromUTF8 ("\xe2\x86\x90 Back to the controls") };
};

GuideHost::GuideHost (juce::AudioProcessorEditor& e) : editor (e)
{
    editor.addComponentListener (this);
}

GuideHost::~GuideHost()
{
    editor.removeComponentListener (this);
}

void GuideHost::componentMovedOrResized (juce::Component&, bool, bool)
{
    if (overlay != nullptr)
        overlay->setBounds (editor.getLocalBounds());
}

bool GuideHost::isOverlayShowing() const
{
    return overlay != nullptr && overlay->isVisible();
}

void GuideHost::show (const juce::String& windowTitle, const std::function<Guide*()>& makeGuide,
                      const juce::String& printText, int page)
{
    auto* processor = editor.getAudioProcessor();

    if (isPlugin (*processor))
    {
        if (overlay == nullptr)
        {
            overlay = std::make_unique<Overlay> (makeGuide(), [this]
            {
                if (overlay != nullptr)
                {
                    overlay->setVisible (false);
                    editor.grabKeyboardFocus();
                }
            });
            editor.addChildComponent (*overlay);
        }

        overlay->getGuide().setPrintSource (printText, processor->getName());

        if (page >= 0)
            overlay->getGuide().showPage (page);

        overlay->setBounds (editor.getLocalBounds());
        overlay->setVisible (true);
        overlay->toFront (false);
        overlay->grabKeyboardFocus();
        return;
    }

    if (window == nullptr)
        window = std::make_unique<GuideWindow> (windowTitle, makeGuide());

    if (auto* guide = dynamic_cast<Guide*> (window->getContentComponent()))
        guide->setPrintSource (printText, processor->getName());

    window->show (page);
}

void EffectEditor::Switch::FittedButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto& laf = getLookAndFeel();
    laf.drawButtonBackground (g, *this, findColour (getToggleState() ? buttonOnColourId : buttonColourId), isMouseOver, isButtonDown);
    g.setFont (laf.getTextButtonFont (*this, getHeight()));
    g.setColour (findColour (getToggleState() ? textColourOnId : textColourOffId).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (getButtonText(), getLocalBounds().reduced (3, 0), juce::Justification::centred, 1);
}

EffectEditor::Switch::Switch (const juce::String& name, int switchTopic) : label ({}, name)
{
    setTopic (*this, switchTopic);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setJustificationType (juce::Justification::centred);
    button.setWantsKeyboardFocus (false);
    addAndMakeVisible (label);
    addAndMakeVisible (button);
}

void EffectEditor::Switch::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    button.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4, 92), 26));
}

//==============================================================================
void addPresetsParagraph (juce::AttributedString& text)
{
    addSection (text, "Starting points and your presets",
                "Starting points come with DawIt and never change. Your presets are settings you saved yourself. Pick one in the same "
                "list to load it, on any track and in any song, and in DawIt's plugins in other DAWs. A preset keeps the settings, not "
                "the sound: the same reverb preset sounds different on a voice and on a drum. Saving the project already keeps every "
                "track's settings, so save a preset when you want to use them again somewhere else. Your DAW's own preset menu is "
                "separate: it saves its own files, which this list doesn't show.");
}

void addUserPresetText (juce::AttributedString& text, const UserPresetInfo& info)
{
    addTitle (text, info.name);

    const auto when = juce::Time::fromISO8601 (info.saved);
    addLead (text, when.toMilliseconds() > 0
                       ? "Your own preset, saved on " + juce::String (when.getDayOfMonth()) + " " + when.getMonthName (false) + " "
                             + juce::String (when.getYear()) + "."
                       : juce::String ("Your own preset."));
    addSection (text, {}, "Loading it set every control to what you saved; controls that didn't exist when it was saved are at their defaults.");

    if (info.versionDiffers)
        addSection (text, {}, "Saved with version " + info.savedVersion + " of this plugin; it may sound a little different now.");

    addPresetsParagraph (text);
}

//==============================================================================
/** The dropdown: rebuilds its menu each time it opens, so presets saved elsewhere (another DAW) show up. */
class PresetPicker::Combo final : public juce::ComboBox
{
public:
    std::function<void()> beforePopup;

    void showPopup() override
    {
        if (beforePopup != nullptr)
            beforePopup();

        juce::ComboBox::showPopup();
    }
};

namespace
{
    enum MenuId { userBase = 1000, importItem = 2001, exportItem = 2002, folderItem = 2003, aboutItem = 2004,
                  renameBase = 3000, deleteBase = 4000, exportBase = 5000 };
}

PresetPicker::PresetPicker (juce::AudioProcessor& p) : processor (p), combo (std::make_unique<Combo>())
{
    combo->setTextWhenNothingSelected ("Starting point...");
    combo->setTooltip ("Starting points made for this effect, then your own presets. Pick one to load its settings; the menu also saves, "
                       "imports, exports and renames your presets.");
    combo->beforePopup = [this] { buildMenu(); };
    combo->onChange = [this]
    {
        const int id = combo->getSelectedId();
        combo->setSelectedId (0, juce::dontSendNotification);
        handleChoice (id);
    };

    saveButton.setTooltip ("Save these settings as a preset of your own, to load on any track, in any song, and in DawIt's plugins in other DAWs. "
                           "Saving the project keeps this track's settings too, but only in this song.");
    saveButton.onClick = [this] { openBar (Bar::save, getDefaultName()); };

    message.setFont (juce::FontOptions (13.0f));
    message.setMinimumHorizontalScale (0.6f);
    nameBox.setMultiLine (false);
    nameBox.setReturnKeyStartsNewLine (false);
    nameBox.setInputRestrictions (presets::maxNameLength + 10);
    nameBox.setTooltip ("The preset's name, 1 to 60 characters. Press Enter to save, Esc to cancel.");
    nameBox.onReturnKey = [this] { confirmBar(); };
    nameBox.onEscapeKey = [this] { closeBar(); };
    okButton.onClick = [this] { confirmBar(); };
    cancelButton.onClick = [this] { closeBar(); };

    for (juce::Component* c : std::initializer_list<juce::Component*> { combo.get(), &saveButton, &nameBox, &message, &okButton, &cancelButton })
        addAndMakeVisible (c);

    for (juce::Component* c : std::initializer_list<juce::Component*> { combo.get(), &saveButton, &okButton, &cancelButton })
        c->setWantsKeyboardFocus (false);

    setWantsKeyboardFocus (false);
    nameBox.setVisible (false);
    message.setVisible (false);
    okButton.setVisible (false);
    cancelButton.setVisible (false);
    saveButton.setEnabled (getInstance() != nullptr);
}

PresetPicker::~PresetPicker() = default;

juce::AudioPluginInstance* PresetPicker::getInstance() const
{
    return dynamic_cast<juce::AudioPluginInstance*> (&processor);
}

presets::Identity PresetPicker::getIdentity() const
{
    if (auto* instance = getInstance())
        return presets::identityOf (*instance);

    return presets::builtInIdentity (processor.getName());
}

juce::String PresetPicker::getShortName() const
{
    const auto name = processor.getName();
    return name.startsWith ("DawIt ") ? name.fromFirstOccurrenceOf ("DawIt ", false, false) : name;
}

juce::String PresetPicker::getDefaultName() const
{
    if (lastName.isNotEmpty())
        return lastName;

    const auto id = getIdentity();

    for (int n = 1;; ++n)
    {
        const auto candidate = "My " + getShortName() + " " + juce::String (n);
        presets::Entry unused;

        if (! library.find (id, candidate, unused))
            return candidate;
    }
}

void PresetPicker::setFactoryPresets (const juce::StringArray& names, const std::vector<std::pair<int, juce::String>>& headings)
{
    factoryNames = names;
    factoryHeadings = headings;
}

int PresetPicker::findFactory (const juce::String& name) const
{
    for (int i = 0; i < factoryNames.size(); ++i)
        if (factoryNames[i].equalsIgnoreCase (name))
            return i;

    for (int i = 0; i < factoryNames.size(); ++i)
        if (factoryNames[i].startsWithIgnoreCase (name + " ("))
            return i;

    return -1;
}

void PresetPicker::showName (const juce::String& name)
{
    shownName = name;
    combo->setTextWhenNothingSelected (name);
    combo->setSelectedId (0, juce::dontSendNotification);
}

void PresetPicker::buildMenu()
{
    combo->clear (juce::dontSendNotification);
    auto& menu = *combo->getRootMenu();

    for (int i = 0; i < factoryNames.size(); ++i)
    {
        for (auto& [first, text] : factoryHeadings)
            if (first == i)
                menu.addSectionHeader (text);

        menu.addItem (i + 1, factoryNames[i]);
    }

    entries = library.list (getIdentity());
    menu.addSeparator();
    menu.addSectionHeader ("Your presets");

    if (entries.empty())
        menu.addItem (-1, "None yet: use Save... to keep these settings", false);

    for (size_t i = 0; i < entries.size(); ++i)
        menu.addItem (userBase + (int) i, entries[i].readable ? entries[i].name : entries[i].name + " (can't be read)", entries[i].readable);

    menu.addSeparator();
    menu.addItem (importItem, "Import preset file...");
    menu.addItem (exportItem, "Export these settings...", getInstance() != nullptr);

    if (entries.empty())
        menu.addItem (-1, "Manage your presets", false);
    else
    {
        juce::PopupMenu manage;

        for (size_t i = 0; i < entries.size(); ++i)
        {
            juce::PopupMenu one;
            one.addItem (renameBase + (int) i, "Rename...", entries[i].readable);
            one.addItem (deleteBase + (int) i, "Delete...");
            one.addItem (exportBase + (int) i, "Export...", entries[i].readable);
            manage.addSubMenu (entries[i].name, one);
        }

        menu.addSubMenu ("Manage your presets", manage);
    }

    menu.addItem (folderItem, "Show presets folder");
    menu.addSeparator();
    menu.addItem (aboutItem, "About starting points and presets");
}

void PresetPicker::handleChoice (int id)
{
    const auto entryAt = [this] (int index) -> const presets::Entry* { return juce::isPositiveAndBelow (index, (int) entries.size()) ? &entries[(size_t) index] : nullptr; };

    if (id >= 1 && id <= factoryNames.size())
    {
        lastName = factoryNames[id - 1];

        if (onFactory != nullptr)
            onFactory (id - 1);
    }
    else if (id >= userBase && id < userBase + 1000)
    {
        if (auto* e = entryAt (id - userBase))
            loadUserPreset (e->name);
    }
    else if (id == importItem)
        importPreset();
    else if (id == exportItem)
        exportSettings();
    else if (id == folderItem)
    {
        auto folder = library.getFolderFor (getIdentity());

        if (! folder.isDirectory())
            folder = library.getRoot();

        folder.createDirectory();
        folder.revealToUser();
    }
    else if (id == aboutItem)
    {
        if (onAbout != nullptr)
            onAbout();
    }
    else if (id >= renameBase && id < renameBase + 1000)
    {
        if (auto* e = entryAt (id - renameBase))
            openBar (Bar::rename, e->name);
    }
    else if (id >= deleteBase && id < deleteBase + 1000)
    {
        if (auto* e = entryAt (id - deleteBase))
            openBar (Bar::deleteIt, e->name);
    }
    else if (id >= exportBase && id < exportBase + 1000)
    {
        if (auto* e = entryAt (id - exportBase))
            exportPreset (e->name);
    }
}

bool PresetPicker::loadUserPreset (const juce::String& name)
{
    auto* instance = getInstance();

    if (instance == nullptr)
        return false;

    const auto parsed = library.load (getIdentity(), name);

    if (! parsed.ok)
    {
        showError ("Couldn't load \"" + name + "\": " + parsed.error);
        return false;
    }

    const auto result = presets::apply (parsed.preset, *instance);

    if (! result.ok)
    {
        showError (result.message);
        return false;
    }

    notifyLoaded (parsed.preset, result);
    return true;
}

void PresetPicker::notifyLoaded (const presets::Preset& preset, const presets::ApplyResult& result)
{
    lastName = preset.name;
    showName (preset.name);

    if (onUserPreset != nullptr)
        onUserPreset ({ preset.name, preset.saved, result.savedVersion, result.versionDiffers });
}

bool PresetPicker::saveAs (const juce::String& rawName, bool replace)
{
    const auto name = presets::tidyName (rawName);
    const auto why = presets::checkName (name);
    auto* instance = getInstance();

    if (why.isNotEmpty() || instance == nullptr)
    {
        showError (why.isNotEmpty() ? why : juce::String ("This plugin can't save presets."));
        return false;
    }

    juce::MemoryBlock state;
    instance->getStateInformation (state);

    if (state.getSize() == 0)
    {
        showError ("Couldn't save \"" + name + "\": this plugin reports no settings to save.");
        return false;
    }

    const auto id = getIdentity();
    const auto result = library.save (id, name, state, replace);

    if (result.nameTaken)
    {
        openBar (Bar::replace, name);
        return false;
    }

    if (! result.ok)
    {
        showError (result.message);
        return false;
    }

    closeBar();
    presets::Entry entry;
    library.find (id, result.name, entry);
    lastName = result.name;
    showName (result.name);

    if (onUserPreset != nullptr)
        onUserPreset ({ result.name, entry.saved, {}, false });

    return true;
}

void PresetPicker::rename (const juce::String& oldName, const juce::String& newName)
{
    const auto result = library.rename (getIdentity(), oldName, presets::tidyName (newName));

    if (! result.ok)
    {
        showError (result.message);
        return;
    }

    closeBar();

    if (shownName == oldName)
        showName (result.name);

    if (lastName == oldName)
        lastName = result.name;
}

void PresetPicker::importPreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Import a preset file", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.dawitpreset");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<PresetPicker> (this)] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr || fc.getResult() == juce::File())
                                  return;

                              const auto result = safe->library.importFile (safe->getIdentity(), fc.getResult());

                              if (! result.ok)
                              {
                                  safe->showError (result.message);
                                  return;
                              }

                              safe->loadUserPreset (result.name);
                          });
}

void PresetPicker::exportSettings()
{
    auto* instance = getInstance();

    if (instance == nullptr)
        return;

    const auto name = getDefaultName();
    chooser = std::make_unique<juce::FileChooser> ("Export these settings", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                                  .getChildFile (presets::tidyName (name) + ".dawitpreset"),
                                                   "*.dawitpreset");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe = juce::Component::SafePointer<PresetPicker> (this), name] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr || fc.getResult() == juce::File())
                                  return;

                              juce::MemoryBlock state;

                              if (auto* inst = safe->getInstance())
                                  inst->getStateInformation (state);

                              const auto result = presets::Library::exportState (safe->getIdentity(), name, state, fc.getResult().withFileExtension ("dawitpreset"));

                              if (! result.ok)
                                  safe->showError (result.message);
                          });
}

void PresetPicker::exportPreset (const juce::String& name)
{
    chooser = std::make_unique<juce::FileChooser> ("Export \"" + name + "\"", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                                 .getChildFile (presets::tidyName (name) + ".dawitpreset"),
                                                   "*.dawitpreset");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe = juce::Component::SafePointer<PresetPicker> (this), name] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr || fc.getResult() == juce::File())
                                  return;

                              const auto result = safe->library.exportTo (safe->getIdentity(), name, fc.getResult().withFileExtension ("dawitpreset"));

                              if (! result.ok)
                                  safe->showError (result.message);
                          });
}

void PresetPicker::openBar (Bar kind, const juce::String& name, const juce::String& text)
{
    stopTimer();
    bar = kind;
    barName = name;

    if (kind == Bar::rename)
        oldBarName = name;

    const bool editing = kind == Bar::save || kind == Bar::rename;
    nameBox.setVisible (editing);
    message.setVisible (! editing);
    okButton.setVisible (true);
    cancelButton.setVisible (kind != Bar::error);
    combo->setVisible (false);
    saveButton.setVisible (false);

    message.setColour (juce::Label::textColourId, kind == Bar::error ? theme::meterRed : theme::text);

    switch (kind)
    {
        case Bar::save:     okButton.setButtonText ("Save"); nameBox.setText (name, false); break;
        case Bar::rename:   okButton.setButtonText ("Rename"); nameBox.setText (name, false); break;
        case Bar::replace:  okButton.setButtonText ("Replace"); message.setText ("Replace \"" + name + "\"?", juce::dontSendNotification); break;
        case Bar::deleteIt: okButton.setButtonText ("Delete"); message.setText ("Delete \"" + name + "\"? This can't be undone.", juce::dontSendNotification); break;
        case Bar::error:    okButton.setButtonText ("OK"); message.setText (text, juce::dontSendNotification); break;
        case Bar::none:     break;
    }

    setNormalBounds (normalBounds);
    toFront (false);
    resized();
    repaint();

    if (editing)
    {
        nameBox.grabKeyboardFocus();
        nameBox.selectAll();
    }

    if (kind == Bar::error)
        startTimer (5000);
}

void PresetPicker::showError (const juce::String& text)
{
    openBar (Bar::error, barName, text);
}

void PresetPicker::closeBar()
{
    stopTimer();
    bar = Bar::none;
    nameBox.setVisible (false);
    message.setVisible (false);
    okButton.setVisible (false);
    cancelButton.setVisible (false);
    combo->setVisible (true);
    saveButton.setVisible (true);
    setNormalBounds (normalBounds);
    resized();
    repaint();
}

void PresetPicker::timerCallback()
{
    closeBar();
}

void PresetPicker::confirmBar()
{
    switch (bar)
    {
        case Bar::save:     saveAs (nameBox.getText(), false); break;
        case Bar::rename:   rename (oldBarName, nameBox.getText()); break;
        case Bar::replace:  saveAs (barName, true); break;
        case Bar::deleteIt:
        {
            const auto name = barName;
            const auto result = library.remove (getIdentity(), name);

            if (result.ok)
                closeBar();
            else
                showError (result.message);

            break;
        }
        case Bar::error:    closeBar(); break;
        case Bar::none:     break;
    }
}

bool PresetPicker::keyPressed (const juce::KeyPress& key)
{
    if (bar != Bar::none && key == juce::KeyPress::escapeKey)
    {
        closeBar();
        return true;
    }

    return false;
}

void PresetPicker::setNormalBounds (juce::Rectangle<int> r)
{
    normalBounds = r;
    setBounds (bar == Bar::none ? r : r.withWidth (juce::jmax (r.getWidth(), barWidth)));
}

void PresetPicker::paint (juce::Graphics& g)
{
    if (bar == Bar::none)
        return;

    g.setColour (theme::panelLight);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    g.setColour (bar == Bar::error ? theme::meterRed.withAlpha (0.6f) : theme::textDim.withAlpha (0.5f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);
}

void PresetPicker::resized()
{
    auto r = getLocalBounds();

    if (bar == Bar::none)
    {
        saveButton.setBounds (r.removeFromRight (saveWidth));
        r.removeFromRight (4);
        combo->setBounds (r);
        return;
    }

    r.reduce (3, 2);
    cancelButton.setBounds (r.removeFromRight (64));
    r.removeFromRight (6);
    okButton.setBounds (r.removeFromRight (72));
    r.removeFromRight (8);
    nameBox.setBounds (r);
    message.setBounds (r.reduced (4, 0));
}

EffectEditor::EffectEditor (juce::AudioProcessor& p, const juce::String& titleText, const juce::String& guideName, const juce::String& listenText,
                            std::atomic<bool>& l, fx::Clap& c, juce::Colour effectColour, fx::Clap::Sound sound)
    : juce::AudioProcessorEditor (&p), colour (effectColour), listen (l), clap (c), clapSound (sound), guideTitle (guideName)
{
    title.setText (titleText, juce::dontSendNotification);
    title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (title);

    setTopic (presets, CommonTopic::presets);
    presets.onFactory = [this] (int index) { applyPreset (index); };
    presets.onUserPreset = [this] (const UserPresetInfo& info)
    {
        userPreset = info;
        showTopic (CommonTopic::presets);
    };
    presets.onAbout = [this] { showTopic (CommonTopic::presetsAbout); };

    guideButton.setButtonText (guideName);
    guideButton.setColour (juce::TextButton::buttonColourId, effectColour.withAlpha (0.35f));
    guideButton.onClick = [this] { openGuide(); };
    setTopic (guideButton, CommonTopic::guide);
    if (isPlugin (*getAudioProcessor()))
        guideButton.setTooltip ("Opens the guide to " + getAudioProcessor()->getName() + " inside this window, with pictures that follow your settings.");

    clapButton.setButtonText (sound == fx::Clap::Sound::sungLine ? "Play a sung line" : sound == fx::Clap::Sound::note ? "Play a note" : sound == fx::Clap::Sound::hitsInHiss ? "Hits and hiss" : sound == fx::Clap::Sound::calibrationTone ? "Play the calibration tone" : "Clap");
    clapButton.onClick = [this]
    {
        clap.request (clapSound);
        clapped();
    };
    setTopic (clapButton, CommonTopic::clap);

    listenButton.setButtonText (listenText);
    listenButton.setColour (juce::TextButton::buttonOnColourId, effectColour.darker (0.3f));
    listenButton.onClick = [this] { listen = ! listen.load(); };
    setTopic (listenButton, CommonTopic::listen);

    helpButton.setTooltip ("How the effect works and how to read the window");
    helpButton.onClick = [this] { showTopic (CommonTopic::overview); };

    for (juce::Component* comp : std::initializer_list<juce::Component*> { &guideButton, &clapButton, &listenButton, &helpButton })
    {
        comp->setWantsKeyboardFocus (false);
        addAndMakeVisible (comp);
    }

    addAndMakeVisible (explainer);
    addAndMakeVisible (presets);
    addMouseListener (this, true);
    startTimerHz (30);
}

EffectEditor::~EffectEditor()
{
    removeMouseListener (this);

    // Hearing only the effect is for while the window is open.
    listen = false;
}

void EffectEditor::setGroups (std::vector<Group> newGroups)
{
    groups = std::move (newGroups);

    for (auto& group : groups)
        for (auto* cell : group.cells)
            addAndMakeVisible (cell);

    groupAreas.assign (groups.size(), {});
}

void EffectEditor::setPresetNames (const juce::StringArray& names, const std::vector<std::pair<int, juce::String>>& headings)
{
    presets.setFactoryPresets (names, headings);
}

void EffectEditor::showPresetName (const juce::String& name)
{
    userPreset.reset();
    presets.showName (name);
}

bool EffectEditor::applyStartingPoint (const juce::String& name)
{
    const int index = presets.findFactory (name);

    if (index < 0)
        return false;

    applyPreset (index);
    return true;
}

void EffectEditor::openGuide (int page)
{
    guideHost.show (guideTitle, [this]
    {
        return new Guide (guideTitle, createGuidePages(), [this] { clap.request (clapSound); clapped(); },
                          "The pictures follow the settings in " + title.getText() + "'s window.", clapButton.getButtonText(),
                          gen::guidesources::getKeyForPlugin (title.getText()));
    }, "the settings in " + title.getText() + "'s window", page);
}

void EffectEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::panel);
    g.fillRect (getLocalBounds().removeFromTop (headerHeight));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, headerHeight - 1, getWidth(), 1);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

    for (size_t i = 0; i < groupAreas.size(); ++i)
    {
        const auto area = groupAreas[i].toFloat();
        g.setColour (theme::panel);
        g.fillRoundedRectangle (area, 5.0f);
        g.setColour (theme::textDim);
        g.drawText (groups[i].name, area.withHeight (18.0f).reduced (8.0f, 0.0f), juce::Justification::centredLeft);
    }
}

void EffectEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (12, 9);

    title.setBounds (header.removeFromLeft (130));
    presets.setNormalBounds (header.removeFromLeft (270));
    header.removeFromLeft (10);
    guideButton.setBounds (header.removeFromLeft (176));

    helpButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (14);
    if (listenButton.isVisible())   // an effect that never changes the sound (DawIt Meter) hides it
    {
        listenButton.setBounds (header.removeFromRight (170));
        header.removeFromRight (10);
    }
    clapButton.setBounds (header.removeFromRight (clapSound == fx::Clap::Sound::calibrationTone ? 180 : clapSound == fx::Clap::Sound::offNote || clapSound == fx::Clap::Sound::sungLine ? 120 : clapSound == fx::Clap::Sound::note ? 100 : clapSound == fx::Clap::Sound::hitsInHiss ? 110 : 70));

    explainer.setBounds (r.removeFromRight (explainerWidth));

    // The knobs, in their groups, row by row.
    int rows = 1;
    for (auto& group : groups)
        rows = juce::jmax (rows, group.row + 1);

    auto knobRows = r.removeFromBottom (knobsHeight * rows);
    constexpr int gap = 8;

    for (int row = 0; row < rows; ++row)
    {
        auto knobArea = knobRows.removeFromTop (knobsHeight).reduced (12, 8);
        int numCells = 0, numGroups = 0;

        for (auto& group : groups)
        {
            if (group.row == row)
            {
                numCells += (int) group.cells.size();
                ++numGroups;
            }
        }

        const int column = numCells > 0 ? (knobArea.getWidth() - gap * (numGroups - 1)) / numCells : 0;

        for (size_t i = 0; i < groups.size(); ++i)
        {
            if (groups[i].row != row)
                continue;

            auto area = knobArea.removeFromLeft (column * (int) groups[i].cells.size());
            knobArea.removeFromLeft (gap);
            groupAreas[i] = area;
            area.removeFromTop (18);
            area = area.reduced (2, 4);

            for (auto* cell : groups[i].cells)
                cell->setBounds (area.removeFromLeft (column).reduced (3, 0));
        }
    }

    layoutPictures (r.reduced (12, 10));
}

void EffectEditor::mouseEnter (const juce::MouseEvent& e)
{
    for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
    {
        if (c->getProperties().contains ("fxTopic"))
        {
            hover ((int) c->getProperties()["fxTopic"]);
            return;
        }
    }

    hover (std::nullopt);
}

void EffectEditor::hover (std::optional<int> t)
{
    if (t == pendingTopic)
        return;

    pendingTopic = t;
    pendingTicks = 0;
}

juce::AttributedString EffectEditor::getText (int t)
{
    if (t == presetsAbout)
    {
        juce::AttributedString about;
        about.setWordWrap (juce::AttributedString::byWord);
        addTitle (about, "Starting points and presets");
        addSection (about, {}, "Starting points are settings made for this effect, each telling why every control is set the way it is. "
                               "They are a start, not an answer: listen and adjust.");
        addPresetsParagraph (about);
        return about;
    }

    if (t == CommonTopic::presets && userPreset.has_value())
    {
        juce::AttributedString text;
        text.setWordWrap (juce::AttributedString::byWord);
        addUserPresetText (text, *userPreset);
        return text;
    }

    auto text = buildText (t);

    if (t == CommonTopic::presets)
        addPresetsParagraph (text);

    if (t == overview)
        addPluginParagraph (text, *getAudioProcessor());

    return text;
}

void EffectEditor::showTopic (int t)
{
    topic = t;
    pendingTopic.reset();
    explainer.show (getText (topic), juce::String (topic));
}

void EffectEditor::timerCallback()
{
    listenButton.setToggleState (listen.load(), juce::dontSendNotification);
    tick();

    // The explainer follows once the mouse has rested a quarter of a second, so crossing the window doesn't change it.
    if (pendingTopic.has_value() && ++pendingTicks >= 8)
    {
        showTopic (*pendingTopic);
        return;
    }

    if (++ticks % 3 == 0)
        explainer.show (getText (topic), juce::String (topic));
}
} // namespace daw::fxui
