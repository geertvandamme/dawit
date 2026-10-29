#include "HitsToMidiWindow.h"

namespace daw
{
namespace hits = analysis::hits;

namespace
{
constexpr double gridChoices[] = { 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
constexpr double snippetSeconds = 0.4;

juce::String getDrumName (hits::Drum drum)
{
    switch (drum)
    {
        case hits::Drum::kick:  return "Kick";
        case hits::Drum::snare: return "Snare";
        case hits::Drum::hihat: return "Hi-hat";
    }

    return {};
}

const juce::String dot = juce::String::fromUTF8 (" \xc2\xb7 ");

/** The drum a clip or track name points at ("Kick loop", "snare", "hi-hat"), or none. */
std::optional<hits::Drum> guessDrum (const juce::String& names)
{
    const auto n = names.toLowerCase();

    if (n.contains ("kick"))
        return hits::Drum::kick;

    if (n.contains ("snare"))
        return hits::Drum::snare;

    if (n.contains ("hihat") || n.contains ("hi-hat") || n.contains ("hi hat") || n.contains ("hat"))
        return hits::Drum::hihat;

    return std::nullopt;
}
} // namespace

//==============================================================================
/** The waveform with its ruler, markers, faint ticks, the Sensitivity line and the playhead, and every mouse
    interaction on the hits. */
class HitsToMidiWindow::View final : public juce::Component, public juce::TooltipClient
{
public:
    explicit View (HitsToMidiWindow& o) : owner (o)
    {
        setWantsKeyboardFocus (true);
    }

    void resized() override
    {
        if (! userZoomed)
            fit();
        else
            clampView();
    }

    void paint (juce::Graphics& g) override
    {
        const auto w = getWidth();
        const auto h = getHeight();
        const auto wave = getWaveArea();
        g.fillAll (theme::lane);

        // Bars and beats.
        const auto& tempo = owner.engine.getTempoMap();
        const double firstBeat = std::floor (owner.startBeat / 4.0) * 4.0;
        const double lastBeat = tempo.getBeatAfter (owner.startBeat, owner.clipSeconds);
        g.setFont (juce::FontOptions (11.0f));

        for (double b = firstBeat; b <= lastBeat + 0.001; b += 1.0)
        {
            const float x = timeToX (tempo.getSecondsBetween (owner.startBeat, b));

            if (x < -1.0f || x > (float) w + 1.0f)
                continue;

            const bool isBar = std::fmod (std::abs (b), 4.0) < 0.001;

            if (! isBar && pxPerSec * 60.0 / tempo.getBpmAt (owner.startBeat) < 12.0)
                continue;

            g.setColour (isBar ? theme::gridBar : theme::grid);
            g.drawVerticalLine ((int) x, (float) wave.getY(), (float) wave.getBottom());
        }

        g.setColour (theme::panel);
        g.fillRect (0, 0, w, rulerHeight);
        g.setColour (theme::gridBar);
        g.drawHorizontalLine (rulerHeight - 1, 0.0f, (float) w);

        // The ruler's bar numbers sit on the panel colour.
        g.setColour (theme::textDim);
        for (double b = firstBeat; b <= lastBeat + 0.001; b += 4.0)
        {
            const float x = timeToX (tempo.getSecondsBetween (owner.startBeat, b));

            if (x >= -1.0f && x <= (float) w)
            {
                g.drawVerticalLine ((int) x, 4.0f, (float) rulerHeight);
                g.drawText (juce::String ((int) std::lround (b / 4.0) + 1), (int) x + 3, 0, 40, rulerHeight - 4, juce::Justification::centredLeft);
            }
        }

        // Waveform.
        const float mid = wave.getCentreY();
        const float half = (float) wave.getHeight() * 0.5f - 4.0f;
        g.setColour (theme::textDim.withAlpha (0.55f));

        for (int x = 0; x < w; ++x)
        {
            const double t0 = xToTime ((float) x);

            if (t0 < 0.0 || t0 > owner.clipSeconds)
                continue;

            const int b0 = juce::jlimit (0, (int) owner.peaks.size() - 1, (int) (t0 * owner.sampleRate / AudioClip::peakBlockSize));
            const int b1 = juce::jlimit (b0 + 1, (int) owner.peaks.size(), (int) (xToTime ((float) x + 1.0f) * owner.sampleRate / AudioClip::peakBlockSize) + 1);
            float peak = 0.0f;

            for (int b = b0; b < b1; ++b)
                peak = juce::jmax (peak, owner.peaks[(size_t) b]);

            const float len = juce::jmin (1.0f, peak) * half;
            g.drawVerticalLine (x, mid - len, mid + len + 1.0f);
        }

        g.setColour (theme::grid);
        g.drawHorizontalLine ((int) mid, 0.0f, (float) w);

        // The Sensitivity line: the quietest hit that counts, as a level in the waveform.
        if (! owner.candidates.empty())
        {
            const float amp = juce::Decibels::decibelsToGain (owner.loudestDb + (float) owner.sensitivity.getValue());

            if (amp < 1.0f)
            {
                g.setColour (theme::accent.withAlpha (0.8f));

                for (const float y : { mid - amp * half, mid + amp * half })
                {
                    const float dashes[] = { 5.0f, 4.0f };
                    g.drawDashedLine (juce::Line<float> (0.0f, y, (float) w, y), dashes, 2, 1.0f);
                }

                g.setFont (juce::FontOptions (11.0f));
                g.drawText ("Sensitivity " + juce::String ((int) std::lround (owner.sensitivity.getValue())) + " dB", 6, (int) (mid - amp * half) - 15, 160, 14,
                            juce::Justification::centredLeft);
            }
        }

        // Faint ticks for the rises that don't count.
        g.setColour (theme::textDim.withAlpha (0.45f));

        for (const double t : owner.faintTimes)
            if (const float x = timeToX (t); x >= 0.0f && x < (float) w)
                g.drawVerticalLine ((int) x, (float) wave.getY(), (float) wave.getY() + 10.0f);

        // The hits.
        for (int i = 0; i < (int) owner.hits.size(); ++i)
        {
            const auto& hit = owner.hits[(size_t) i];
            const bool moving = dragging && i == dragIndex;
            const float x = timeToX (moving ? dragTime : hit.time);

            if (x < -6.0f || x > (float) w + 6.0f)
                continue;

            const auto colour = getDrumColour (hit.drum);
            const bool isSelected = i == owner.selected;
            g.setColour (colour.withAlpha (isSelected ? 1.0f : 0.85f));
            g.drawLine (x, (float) wave.getY(), x, (float) wave.getBottom(), isSelected ? 2.5f : 1.5f);
            g.fillEllipse (x - 4.0f, (float) wave.getY() - 1.0f, 8.0f, 8.0f);

            if (isSelected)
            {
                g.setColour (juce::Colours::white);
                g.drawEllipse (x - 5.5f, (float) wave.getY() - 2.5f, 11.0f, 11.0f, 1.5f);
            }
        }

        // Where Listen will start, and the playhead.
        if (const float x = timeToX (owner.listenFrom); x >= 0.0f && x < (float) w)
        {
            g.setColour (theme::accent);
            juce::Path p;
            p.addTriangle (x - 5.0f, 3.0f, x + 5.0f, 3.0f, x, 12.0f);
            g.fillPath (p);
        }

        if (playheadSeconds >= 0.0)
        {
            const float x = timeToX (playheadSeconds);
            g.setColour (theme::playhead);
            g.drawVerticalLine ((int) x, 0.0f, (float) h);
        }
    }

    void setPlayhead (double seconds)
    {
        if (! juce::approximatelyEqual (seconds, playheadSeconds))
        {
            playheadSeconds = seconds;
            repaint();
        }
    }

    juce::String getTooltip() override
    {
        const auto p = getMouseXYRelative();

        if (p.y < rulerHeight)
            return "Click to start Listen from here.";

        const int index = hitAt ((float) p.x);

        if (index >= 0)
            return owner.getHitTooltip (index);

        const double t = xToTime ((float) p.x);

        if (t < 0.0 || t > owner.clipSeconds)
            return {};

        return juce::String (t, 2) + " s" + dot + owner.positionText (t) + dot + "Click to add a hit here";
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();
        const double t = xToTime (e.position.x);
        const int index = e.y < rulerHeight ? -1 : hitAt (e.position.x);

        if (e.mods.isPopupMenu())
        {
            showMenu (index, t);
            return;
        }

        dragging = false;

        if (e.y < rulerHeight)
        {
            owner.listenFrom = juce::jlimit (0.0, owner.clipSeconds, t);
            repaint();
            return;
        }

        if (index >= 0)
        {
            owner.selected = index;
            owner.selectedTime = owner.hits[(size_t) index].time;
            dragIndex = index;
            dragTime = owner.selectedTime;
            owner.updateStatus();
            repaint();
        }
        else if (t >= 0.0 && t <= owner.clipSeconds)
        {
            owner.addHit (t, e.mods.isAltDown());
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragIndex < 0 || e.mods.isPopupMenu())
            return;

        if (! dragging && e.getDistanceFromDragStart() < 3)
            return;

        dragging = true;
        dragTime = juce::jlimit (0.0, owner.clipSeconds, xToTime (e.position.x));
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const bool moved = dragging && dragIndex >= 0;
        const int index = dragIndex;
        dragging = false;
        dragIndex = -1;

        if (moved && ! e.mods.isPopupMenu())
            owner.moveHit (index, dragTime, e.mods.isAltDown());
        else
            repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (e.y < rulerHeight || e.mods.isPopupMenu())
            return;

        if (const int index = hitAt (e.position.x); index >= 0)
            owner.playSnippet (owner.hits[(size_t) index].time);
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (e.mods.isCommandDown() || e.mods.isCtrlDown())
        {
            zoomAround (std::exp (wheel.deltaY * 1.5), xToTime (e.position.x), e.position.x);
            return;
        }

        const float delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
        viewStart -= delta * 400.0 / pxPerSec;
        clampView();
        repaint();
    }

    void mouseMagnify (const juce::MouseEvent& e, float scale) override
    {
        zoomAround (scale, xToTime (e.position.x), e.position.x);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && owner.selected >= 0)
        {
            owner.removeHit (owner.selected);
            return true;
        }

        return false;
    }

private:
    static constexpr int rulerHeight = 24;

    juce::Rectangle<int> getWaveArea() const { return getLocalBounds().withTrimmedTop (rulerHeight); }
    float timeToX (double t) const { return (float) ((t - viewStart) * pxPerSec); }
    double xToTime (float x) const { return viewStart + x / pxPerSec; }

    void fit()
    {
        pxPerSec = owner.clipSeconds > 0.0 ? juce::jmax (1.0, getWidth() / owner.clipSeconds) : 100.0;
        viewStart = 0.0;
    }

    void clampView()
    {
        const double visible = getWidth() / pxPerSec;
        viewStart = juce::jlimit (0.0, juce::jmax (0.0, owner.clipSeconds - visible), viewStart);
    }

    void zoomAround (double factor, double anchorTime, float anchorX)
    {
        const double minPx = owner.clipSeconds > 0.0 ? juce::jmax (1.0, getWidth() / owner.clipSeconds) : 1.0;
        pxPerSec = juce::jlimit (minPx, 20000.0, pxPerSec * factor);
        viewStart = anchorTime - anchorX / pxPerSec;
        userZoomed = pxPerSec > minPx * 1.001;
        clampView();
        repaint();
    }

    int hitAt (float x) const
    {
        int best = -1;
        float bestDistance = 6.0f;

        for (int i = 0; i < (int) owner.hits.size(); ++i)
            if (const float d = std::abs (timeToX (owner.hits[(size_t) i].time) - x); d <= bestDistance)
            {
                bestDistance = d;
                best = i;
            }

        return best;
    }

    void showMenu (int index, double time)
    {
        juce::PopupMenu menu;

        if (index >= 0)
        {
            const auto drum = owner.hits[(size_t) index].drum;
            owner.selected = index;
            owner.selectedTime = owner.hits[(size_t) index].time;
            owner.updateStatus();
            repaint();
            menu.addItem (1, "Kick", true, drum == hits::Drum::kick);
            menu.addItem (2, "Snare", true, drum == hits::Drum::snare);
            menu.addItem (3, "Hi-hat", true, drum == hits::Drum::hihat);
            menu.addItem (4, "Delete hit");
        }
        else
        {
            menu.addItem (5, "Add hit here", time >= 0.0 && time <= owner.clipSeconds);
            menu.addItem (6, "Undo hand edits", ! owner.edits.empty());
        }

        menu.addSeparator();
        menu.addItem (7, "How drum triggers work...");

        menu.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<View> (this), index, time] (int result)
        {
            auto* self = safe.getComponent();

            if (self == nullptr || result == 0)
                return;

            auto& o = self->owner;

            if (result >= 1 && result <= 3)
                o.setDrum (index, result == 1 ? hits::Drum::kick : result == 2 ? hits::Drum::snare : hits::Drum::hihat);
            else if (result == 4)
                o.removeHit (index);
            else if (result == 5)
                o.addHit (time, false);
            else if (result == 6)
            {
                o.edits.clear();
                o.repick();
            }
            else if (result == 7)
                o.showGuide (triggerguide::Topic::findingHits);
        });
    }

    HitsToMidiWindow& owner;
    double pxPerSec = 100.0, viewStart = 0.0;
    bool userZoomed = false;
    int dragIndex = -1;
    bool dragging = false;
    double dragTime = 0.0;
    double playheadSeconds = -1.0;
};

//==============================================================================
juce::Colour HitsToMidiWindow::getDrumColour (hits::Drum drum)
{
    switch (drum)
    {
        case hits::Drum::kick:  return juce::Colour (0xfff5a04a);
        case hits::Drum::snare: return juce::Colour (0xff4fa3ff);
        case hits::Drum::hihat: return juce::Colour (0xff5fd38d);
    }

    return theme::text;
}

HitsToMidiWindow::HitsToMidiWindow (Engine& e, Track& t, AudioClip& c, std::vector<hits::Candidate> found)
    : engine (e), track (t), clip (c), candidates (std::move (found)),
      clipSeconds (c.getLengthSeconds()), sampleRate (c.sourceSampleRate), startBeat (c.startBeat)
{
    for (const auto& cand : candidates)
        loudestDb = juce::jmax (loudestDb, cand.levelDb);

    // The waveform overview: the larger of the two channels' peaks in each block.
    const int numSamples = clip.audio.getNumSamples();
    peaks.assign ((size_t) ((numSamples + AudioClip::peakBlockSize - 1) / AudioClip::peakBlockSize), 0.0f);

    for (size_t b = 0; b < peaks.size(); ++b)
    {
        const int start = (int) b * AudioClip::peakBlockSize;
        const int num = juce::jmin (AudioClip::peakBlockSize, numSamples - start);

        for (int ch = 0; ch < clip.audio.getNumChannels(); ++ch)
        {
            const auto range = juce::FloatVectorOperations::findMinAndMax (clip.audio.getReadPointer (ch, start), num);
            peaks[b] = juce::jmax (peaks[b], std::abs (range.getStart()), std::abs (range.getEnd()));
        }
    }

    auto setupLabel = [this] (juce::Label& l, const juce::String& text, bool dim)
    {
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, dim ? theme::textDim : theme::text);
        l.setInterceptsMouseClicks (false, false);
        l.setMinimumHorizontalScale (0.8f);
        addAndMakeVisible (l);
    };

    setupLabel (findLabel, "Find:", true);
    setupLabel (notesLabel, "Notes:", true);
    setupLabel (timingLabel, "Timing:", true);
    setupLabel (originalLabel, "Original:", true);
    setupLabel (sensLabel, "Sensitivity", false);
    setupLabel (gapLabel, "Minimum gap", false);
    setupLabel (writeLabel, "Write as", false);
    setupLabel (velLabel, "Velocity", false);
    setupLabel (strengthLabel, "Strength", false);
    setupLabel (hearLabel, "Hear", false);
    setupLabel (status, {}, false);
    status.setMinimumHorizontalScale (0.7f);

    auto setupSlider = [this] (juce::Slider& s, double lo, double hi, double step, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 72, 22);
        s.setRange (lo, hi, step);
        s.setTextValueSuffix (suffix);
        s.setScrollWheelEnabled (false);
        addAndMakeVisible (s);
    };

    setupSlider (sensitivity, -60.0, -6.0, 1.0, " dB");
    setupSlider (gap, 5.0, 250.0, 1.0, " ms");
    setupSlider (strength, 0.0, 100.0, 5.0, " %");

    writeAs.addItemList ({ "Sort by drum", "Kick (36)", "Snare (38)", "Hi-hat (42)" }, 1);
    velocity.addItemList ({ "Natural", "Gentle", "Strong", "Same for all (100)" }, 1);
    grid.addItemList ({ "1/4", "1/8", "1/16", "1/32", "1/8 triplet", "1/16 triplet" }, 1);
    hear.addItemList ({ "Original", "Both", "Hits" }, 1);

    for (auto* b : { &keepFeel, &snapTo })
        b->setRadioGroupId (1);

    for (auto* b : { &keepPlaying, &muteClip })
        b->setRadioGroupId (2);

    for (auto* comp : std::initializer_list<juce::Component*> { &writeAs, &velocity, &grid, &hear, &keepFeel, &snapTo, &keepPlaying, &muteClip,
                                                             &listenButton, &cancelButton, &makeButton, &helpButton })
        addAndMakeVisible (comp);

    for (auto* b : { &keepFeel, &snapTo, &keepPlaying, &muteClip })
        b->setClickingTogglesState (true);

    listenButton.setButtonText (juce::String::fromUTF8 ("\xe2\x96\xb6 Listen"));

    // Tooltips, from the guide's own text.
    using Tip = triggerguide::Tip;
    sensitivity.setTooltip (triggerguide::getTooltip (Tip::sensitivity));
    sensLabel.setTooltip (triggerguide::getTooltip (Tip::sensitivity));
    gap.setTooltip (triggerguide::getTooltip (Tip::minimumGap));
    writeAs.setTooltip (triggerguide::getTooltip (Tip::writeAs));
    velocity.setTooltip (triggerguide::getTooltip (Tip::velocity));
    keepFeel.setTooltip (triggerguide::getTooltip (Tip::keepFeel));
    snapTo.setTooltip (triggerguide::getTooltip (Tip::snapTo));
    grid.setTooltip (triggerguide::getTooltip (Tip::snapTo));
    strength.setTooltip (triggerguide::getTooltip (Tip::strength));
    keepPlaying.setTooltip (triggerguide::getTooltip (Tip::keepPlaying));
    muteClip.setTooltip (triggerguide::getTooltip (Tip::muteClip));
    listenButton.setTooltip (triggerguide::getTooltip (Tip::listen));
    hear.setTooltip (triggerguide::getTooltip (Tip::hear));
    makeButton.setTooltip (triggerguide::getTooltip (Tip::makeTrack));
    helpButton.setTooltip (triggerguide::getTooltip (Tip::help));
    cancelButton.setTooltip ("Closes the window without making anything.");

    // Remembered choices (Write as is guessed per clip).
    auto& settings = *engine.properties.getUserSettings();
    sensitivity.setValue (settings.getDoubleValue ("hits.sensitivityDb", -30.0), juce::dontSendNotification);
    gap.setValue (settings.getDoubleValue ("hits.gapMs", 30.0), juce::dontSendNotification);
    velocity.setSelectedItemIndex (juce::jlimit (0, 3, settings.getIntValue ("hits.velocity", 0)), juce::dontSendNotification);
    const bool snap = settings.getBoolValue ("hits.snap", false);
    keepFeel.setToggleState (! snap, juce::dontSendNotification);
    snapTo.setToggleState (snap, juce::dontSendNotification);
    grid.setSelectedItemIndex (juce::jlimit (0, 5, settings.getIntValue ("hits.grid", 2)), juce::dontSendNotification);
    strength.setValue (settings.getDoubleValue ("hits.strength", 100.0), juce::dontSendNotification);
    const bool replace = settings.getIntValue ("hits.original", 0) == 1;
    keepPlaying.setToggleState (! replace, juce::dontSendNotification);
    muteClip.setToggleState (replace, juce::dontSendNotification);
    hear.setSelectedItemIndex (juce::jlimit (0, 2, settings.getIntValue ("hits.hear", 1)), juce::dontSendNotification);

    const auto guess = guessDrum (track.name + " " + clip.name);
    writeAs.setSelectedItemIndex (guess.has_value() ? 1 + (int) *guess : 0, juce::dontSendNotification);

    view = std::make_unique<View> (*this);
    addAndMakeVisible (*view);
    addAndMakeVisible (helpButton);

    topicOf = { { view.get(), triggerguide::Topic::findingHits },
                { &sensitivity, triggerguide::Topic::findingHits },
                { &gap, triggerguide::Topic::findingHits },
                { &writeAs, triggerguide::Topic::whichDrum },
                { &velocity, triggerguide::Topic::velocity },
                { &keepFeel, triggerguide::Topic::grid },
                { &snapTo, triggerguide::Topic::grid },
                { &grid, triggerguide::Topic::grid },
                { &strength, triggerguide::Topic::grid },
                { &keepPlaying, triggerguide::Topic::replaceOrLayer },
                { &muteClip, triggerguide::Topic::replaceOrLayer } };
    addMouseListener (this, true);

    auto changed = [this] { settingsChanged(); };
    sensitivity.onValueChange = changed;
    gap.onValueChange = changed;
    strength.onValueChange = changed;

    for (auto* cb : { &writeAs, &velocity, &grid })
        cb->onChange = changed;

    for (auto* b : { &keepFeel, &snapTo, &keepPlaying, &muteClip })
        b->onClick = changed;

    hear.onChange = [this] { saveSettings(); restartCountdown = listening ? 4 : 0; };
    listenButton.onClick = [this] { toggleListen(); };
    cancelButton.onClick = [this] { close(); };
    makeButton.onClick = [this] { makeTrack(); };
    helpButton.onClick = [this] { showGuide (hoverTopic); };

    setSize (1000, 620);
    repick();
    engine.addChangeListener (this);
    startTimerHz (30);
}

HitsToMidiWindow::~HitsToMidiWindow()
{
    stopTimer();
    engine.removeChangeListener (this);

    if (listening && engine.isAuditioning())
        engine.stopAudition();
}

//==============================================================================
bool HitsToMidiWindow::isClipAlive() const
{
    return engine.findTrackFor (&clip) == &track;
}

void HitsToMidiWindow::close()
{
    if (closing)
        return;

    closing = true;
    stopListening();

    if (onClose)
        onClose();
}

void HitsToMidiWindow::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (! isClipAlive())
    {
        close();
        return;
    }

    if (! juce::approximatelyEqual (startBeat, clip.startBeat))
    {
        startBeat = clip.startBeat;
        view->repaint();
    }
}

void HitsToMidiWindow::mouseEnter (const juce::MouseEvent& e)
{
    for (const auto& [component, topic] : topicOf)
        if (e.eventComponent == component || component->isParentOf (e.eventComponent))
        {
            hoverTopic = topic;
            return;
        }
}

void HitsToMidiWindow::showGuide (triggerguide::Topic topic)
{
    if (onShowGuide)
        onShowGuide (topic);
}

//==============================================================================
hits::PickSettings HitsToMidiWindow::getPickSettings() const
{
    hits::PickSettings s;
    s.sensitivityDb = sensitivity.getValue();
    s.minGapMs = gap.getValue();

    const int drum = writeAs.getSelectedItemIndex();

    if (drum > 0)
        s.singleDrum = (hits::Drum) (drum - 1);

    const hits::VelocityCurve curves[] = { hits::VelocityCurve::natural, hits::VelocityCurve::gentle,
                                           hits::VelocityCurve::strong, hits::VelocityCurve::same };
    s.curve = curves[juce::jlimit (0, 3, velocity.getSelectedItemIndex())];
    return s;
}

hitstomidi::Timing HitsToMidiWindow::getTiming() const
{
    hitstomidi::Timing t;
    t.snap = snapTo.getToggleState();
    t.gridBeats = gridChoices[juce::jlimit (0, 5, grid.getSelectedItemIndex())];
    t.strength = strength.getValue() / 100.0;
    return t;
}

std::vector<MidiNote> HitsToMidiWindow::getNotes() const
{
    const auto& tempo = engine.getTempoMap();
    return hitstomidi::toClipNotes (hits, tempo, clip.startBeat, clip.getLengthBeats (tempo), getTiming());
}

triggerguide::Part HitsToMidiWindow::getPart() const
{
    const auto& tempo = engine.getTempoMap();
    const double lengthBeats = clip.getLengthBeats (tempo);
    std::vector<triggerguide::DrumNote> notes;

    for (const auto& n : getNotes())
        notes.push_back ({ n.pitch, n.start, (int) std::lround (n.velocity * 127.0f) });

    auto part = triggerguide::getPartFromNotes (notes, lengthBeats, tempo.getAverageBpm (clip.startBeat, clip.startBeat + lengthBeats));
    part.showDrums = writeAs.getSelectedItemIndex() == 0;
    part.sameForAll = velocity.getSelectedItemIndex() == 3;
    return part;
}

void HitsToMidiWindow::repick()
{
    hits = hits::pick (candidates, getPickSettings(), edits);

    faintTimes.clear();

    for (const auto& c : candidates)
    {
        const bool counted = std::any_of (hits.begin(), hits.end(), [&] (const hits::Hit& h) { return std::abs (h.time - c.time) < 0.0005; });

        if (! counted)
            faintTimes.push_back (c.time);
    }

    selected = -1;

    if (selectedTime >= 0.0)
    {
        double best = hits::matchSeconds;

        for (int i = 0; i < (int) hits.size(); ++i)
            if (const double d = std::abs (hits[(size_t) i].time - selectedTime); d <= best)
            {
                best = d;
                selected = i;
            }

        if (selected >= 0)
            selectedTime = hits[(size_t) selected].time;
        else
            selectedTime = -1.0;
    }

    updateEnabled();
    updateStatus();
    view->repaint();

    if (listening && snippetEndSample < 0)
        restartCountdown = 4;
}

void HitsToMidiWindow::settingsChanged()
{
    saveSettings();
    repick();
}

void HitsToMidiWindow::saveSettings() const
{
    auto& settings = *engine.properties.getUserSettings();
    settings.setValue ("hits.sensitivityDb", sensitivity.getValue());
    settings.setValue ("hits.gapMs", gap.getValue());
    settings.setValue ("hits.velocity", velocity.getSelectedItemIndex());
    settings.setValue ("hits.snap", snapTo.getToggleState());
    settings.setValue ("hits.grid", grid.getSelectedItemIndex());
    settings.setValue ("hits.strength", (int) std::lround (strength.getValue()));
    settings.setValue ("hits.original", muteClip.getToggleState() ? 1 : 0);
    settings.setValue ("hits.hear", hear.getSelectedItemIndex());
}

void HitsToMidiWindow::updateEnabled()
{
    const bool snap = snapTo.getToggleState();
    grid.setEnabled (snap);
    strength.setEnabled (snap);
    strengthLabel.setEnabled (snap);
    makeButton.setEnabled (! hits.empty());
}

juce::String HitsToMidiWindow::positionText (double seconds) const
{
    const double beat = engine.getTempoMap().getBeatAfter (startBeat, seconds);
    return "Bar " + juce::String ((int) std::floor (beat / 4.0) + 1) + ", beat " + juce::String ((int) std::floor (std::fmod (beat, 4.0)) + 1);
}

juce::String HitsToMidiWindow::describeHit (int index) const
{
    const auto& h = hits[(size_t) index];
    return "Hit " + juce::String (index + 1) + dot + juce::String (h.time, 2) + " s" + dot + positionText (h.time).toLowerCase()
           + dot + getDrumName (h.drum) + dot + "velocity " + juce::String (h.velocity);
}

juce::String HitsToMidiWindow::getHitTooltip (int index) const
{
    const auto& h = hits[(size_t) index];
    return juce::String (h.time, 2) + " s" + dot + positionText (h.time) + dot + getDrumName (h.drum) + ", velocity " + juce::String (h.velocity);
}

void HitsToMidiWindow::updateStatus()
{
    if (selected >= 0 && selected < (int) hits.size())
        status.setText (describeHit (selected), juce::dontSendNotification);
    else if (hits.empty())
        status.setText ("No hits at this sensitivity. Try a lower value, such as -45 dB.", juce::dontSendNotification);
    else
        status.setText (triggerguide::describeCounts (getPart()), juce::dontSendNotification);
}

//==============================================================================
void HitsToMidiWindow::selectHitNear (double seconds)
{
    selectedTime = seconds;
    selected = -1;
    double best = hits::matchSeconds;

    for (int i = 0; i < (int) hits.size(); ++i)
        if (const double d = std::abs (hits[(size_t) i].time - seconds); d <= best)
        {
            best = d;
            selected = i;
        }

    updateStatus();
    view->repaint();
}

void HitsToMidiWindow::addHit (double seconds, bool exact)
{
    if (! isClipAlive())
        return;

    const auto cand = hits::measureCandidate (clip.audio, sampleRate, seconds, 0.010, candidates, exact);

    // Already a hit there: just select it.
    for (const auto& h : hits)
        if (std::abs (h.time - cand.time) < hits::matchSeconds)
        {
            selectHitNear (h.time);
            return;
        }

    hits::HandEdit e;
    e.kind = hits::HandEdit::Kind::add;
    e.time = cand.time;
    e.target = cand;
    edits.push_back (e);
    selectedTime = cand.time;
    repick();
}

void HitsToMidiWindow::moveHit (int index, double toSeconds, bool exact)
{
    if (! isClipAlive() || index < 0 || index >= (int) hits.size())
        return;

    const auto cand = hits::measureCandidate (clip.audio, sampleRate, toSeconds, 0.010, candidates, exact);
    hits::HandEdit e;
    e.kind = hits::HandEdit::Kind::move;
    e.time = hits[(size_t) index].time;
    e.target = cand;
    edits.push_back (e);
    selectedTime = cand.time;
    repick();
}

void HitsToMidiWindow::removeHit (int index)
{
    if (index < 0 || index >= (int) hits.size())
        return;

    hits::HandEdit e;
    e.kind = hits::HandEdit::Kind::remove;
    e.time = hits[(size_t) index].time;
    edits.push_back (e);
    selectedTime = -1.0;
    repick();
}

void HitsToMidiWindow::setDrum (int index, hits::Drum drum)
{
    if (index < 0 || index >= (int) hits.size())
        return;

    hits::HandEdit e;
    e.kind = hits::HandEdit::Kind::setDrum;
    e.time = hits[(size_t) index].time;
    e.drum = drum;
    edits.push_back (e);
    selectedTime = e.time;
    repick();
}

//==============================================================================
double HitsToMidiWindow::getListenSeconds() const
{
    return engine.isAuditioning() ? engine.getAuditionPosition() / juce::jmax (1.0, engine.getSampleRate()) : -1.0;
}

void HitsToMidiWindow::startListening (int startSample, double endSeconds)
{
    if (! isClipAlive())
        return;

    engine.stop();   // listening and the transport don't play together
    const auto& tempo = engine.getTempoMap();
    const double rate = engine.getSampleRate();
    const hitstomidi::Hear what[] = { hitstomidi::Hear::original, hitstomidi::Hear::both, hitstomidi::Hear::hits };
    auto buffer = hitstomidi::renderListen (clip, getNotes(), tempo, clip.startBeat, rate, what[juce::jlimit (0, 2, hear.getSelectedItemIndex())]);
    snippetEndSample = endSeconds >= 0.0 ? (int) (endSeconds * rate) : -1;
    engine.startAudition (std::move (buffer), juce::jmax (0, startSample));
    listening = true;
    listenButton.setButtonText ("Stop");
}

void HitsToMidiWindow::toggleListen()
{
    if (listening && engine.isAuditioning())
    {
        stopListening();
        return;
    }

    startListening ((int) (listenFrom * engine.getSampleRate()));
}

void HitsToMidiWindow::playSnippet (double fromSeconds)
{
    startListening ((int) (fromSeconds * engine.getSampleRate()), fromSeconds + snippetSeconds);
}

void HitsToMidiWindow::stopListening()
{
    if (listening && engine.isAuditioning())
        engine.stopAudition();

    listening = false;
    snippetEndSample = -1;
    restartCountdown = 0;
    listenButton.setButtonText (juce::String::fromUTF8 ("\xe2\x96\xb6 Listen"));
}

void HitsToMidiWindow::timerCallback()
{
    if (listening && ! engine.isAuditioning())
        stopListening();   // reached the end, or the transport started

    if (listening && snippetEndSample >= 0 && engine.getAuditionPosition() >= snippetEndSample)
        stopListening();

    if (listening && restartCountdown > 0 && --restartCountdown == 0 && snippetEndSample < 0)
        startListening (engine.getAuditionPosition());

    view->setPlayhead (getListenSeconds());
}

//==============================================================================
void HitsToMidiWindow::makeTrack()
{
    if (closing || ! isClipAlive())
        return;

    auto notes = getNotes();

    if (notes.empty())
        return;

    stopListening();
    auto* made = hitstomidi::addDrumTrack (engine, track, clip, std::move (notes), muteClip.getToggleState());

    if (made == nullptr)
        return;

    if (auto* newTrack = engine.findTrackFor (made); newTrack != nullptr && onMade)
        onMade (*newTrack, *made);

    close();
}

//==============================================================================
void HitsToMidiWindow::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void HitsToMidiWindow::resized()
{
    auto area = getLocalBounds();
    auto bottom = area.removeFromBottom (214).reduced (12, 8);
    view->setBounds (area);
    helpButton.setBounds (area.getRight() - 30, 2, 26, 20);

    const int row = 28;
    const int labelW = 72;

    auto next = [&] { auto r = bottom.removeFromTop (row); bottom.removeFromTop (4); return r; };

    auto r = next();
    findLabel.setBounds (r.removeFromLeft (labelW));
    sensLabel.setBounds (r.removeFromLeft (80));
    sensitivity.setBounds (r.removeFromLeft (320));
    r.removeFromLeft (20);
    gapLabel.setBounds (r.removeFromLeft (92));
    gap.setBounds (r.removeFromLeft (320));

    r = next();
    notesLabel.setBounds (r.removeFromLeft (labelW));
    writeLabel.setBounds (r.removeFromLeft (80));
    writeAs.setBounds (r.removeFromLeft (160));
    r.removeFromLeft (20);
    velLabel.setBounds (r.removeFromLeft (92));
    velocity.setBounds (r.removeFromLeft (180));

    r = next();
    timingLabel.setBounds (r.removeFromLeft (labelW));
    keepFeel.setBounds (r.removeFromLeft (110));
    snapTo.setBounds (r.removeFromLeft (84));
    grid.setBounds (r.removeFromLeft (120));
    r.removeFromLeft (20);
    strengthLabel.setBounds (r.removeFromLeft (64));
    strength.setBounds (r.removeFromLeft (260));

    r = next();
    originalLabel.setBounds (r.removeFromLeft (labelW));
    keepPlaying.setBounds (r.removeFromLeft (190));
    muteClip.setBounds (r.removeFromLeft (220));

    status.setBounds (next().withHeight (24));
    bottom.removeFromTop (4);

    r = bottom.removeFromTop (32);
    listenButton.setBounds (r.removeFromLeft (110));
    r.removeFromLeft (16);
    hearLabel.setBounds (r.removeFromLeft (40));
    hear.setBounds (r.removeFromLeft (110));
    makeButton.setBounds (r.removeFromRight (150));
    r.removeFromRight (8);
    cancelButton.setBounds (r.removeFromRight (90));
}
} // namespace daw
