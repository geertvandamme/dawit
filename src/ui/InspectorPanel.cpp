#include "InspectorPanel.h"
#include "PluginActions.h"
#include "RecordingActions.h"
#include "engine/BuiltInSynth.h"
#include "engine/Routing.h"

namespace daw
{
namespace
{
    constexpr int titleHeight = 28, rowHeight = 24, headingHeight = 22, sidePad = 10;

    juce::String formatPan (double v)
    {
        const int percent = juce::roundToInt (std::abs (v) * 100.0);
        return percent == 0 ? juce::String ("C") : juce::String (percent) + (v < 0 ? " L" : " R");
    }

    juce::String dot() { return juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")); }
}

//==============================================================================
InspectorEdge::InspectorEdge()
{
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    setTooltip ("Drag to make the inspector wider or narrower");
}

void InspectorEdge::paint (juce::Graphics& g)
{
    g.fillAll (isMouseOverOrDragging() ? theme::accent.withAlpha (0.5f) : theme::background);
}

void InspectorEdge::mouseDown (const juce::MouseEvent&)
{
    startWidth = getPanelWidth ? getPanelWidth() : InspectorPanel::defaultWidth;
}

void InspectorEdge::mouseDrag (const juce::MouseEvent& e)
{
    if (onResize)
        onResize (startWidth + e.getDistanceFromDragStartX());
}

void InspectorEdge::mouseUp (const juce::MouseEvent&)
{
    if (onResizeEnd)
        onResizeEnd();
}

//==============================================================================
/** One effect (or the instrument) in the list: a power dot, the name, and an x under the mouse. */
class InspectorPanel::EffectRow final : public juce::Component, public juce::SettableTooltipClient
{
public:
    /** `slot` is null for the built-in synth. */
    EffectRow (InspectorPanel& o, PluginSlot* s) : owner (o), slot (s)
    {
        if (isInstrument())
            setTooltip ("The instrument that plays this track's notes: click to open it, right-click to choose another");
        else
            setTooltip (s->instance->getName() + ": click to open, click the dot to switch it off, drag up or down to change the order, right-click for more.");
    }

    PluginSlot* getSlot() const noexcept { return slot; }
    bool isInstrument() const noexcept { return slot == nullptr || slot->isInstrument; }

    /** Repaints when the bypass switch or the synth's sound changed. */
    void refresh()
    {
        const bool bypassed = slot != nullptr && slot->bypassed.load();
        const auto text = getText();

        if (bypassed != shownBypassed || text != shownText)
        {
            shownBypassed = bypassed;
            shownText = text;
            repaint();
        }
    }

    juce::String getText() const
    {
        if (slot == nullptr)
            return "Instrument: DawIt Synth" + dot() + (owner.shownTrack != nullptr ? builtInSounds::getName (owner.shownTrack->builtInSound.load()) : juce::String());

        if (slot->isInstrument)
            return "Instrument: " + slot->instance->getName();

        return slot->instance->getName();
    }

    void paint (juce::Graphics& g) override
    {
        const bool bypassed = slot != nullptr && slot->bypassed.load();

        if (isMouseOver() && draggedNot())
        {
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f, 1.0f), 3.0f);
        }

        auto text = getLocalBounds().withTrimmedLeft (isInstrument() ? sidePad : 28).withTrimmedRight (isInstrument() ? sidePad : 24);

        if (! isInstrument())
        {
            const auto centre = juce::Point<float> (17.0f, (float) getHeight() * 0.5f);
            g.setColour (bypassed ? theme::textDim.withAlpha (0.5f) : theme::meterGreen);

            if (bypassed)
                g.drawEllipse (centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f, 1.2f);
            else
                g.fillEllipse (centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);

            if (isMouseOver())
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (14.0f));
                g.drawText (juce::String (juce::CharPointer_UTF8 ("\xc3\x97")), getRemoveArea(), juce::Justification::centred);
            }
        }

        g.setColour (bypassed ? theme::textDim : theme::text);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText (getText(), text, juce::Justification::centredLeft, 1, 0.8f);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu() || ! owner.isShownAlive())
            return;

        auto& track = *owner.shownTrack;

        if (isInstrument())
            plugins::showAddMenu (owner.engine, owner.pluginWindows, track, true, this);
        else if (plugins::contains (track, slot))
            plugins::showSlotMenu (owner.engine, owner.pluginWindows, track, *slot, this);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! isInstrument() && ! e.mods.isPopupMenu() && e.getDistanceFromDragStart() > 4)
            owner.dragRow (*this, e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (owner.endRowDrag() || e.mods.isPopupMenu() || e.mouseWasDraggedSinceMouseDown() || ! owner.isShownAlive())
            return;

        auto& eng = owner.engine;
        auto& windows = owner.pluginWindows;
        auto* trackPtr = owner.shownTrack;

        if (slot == nullptr)
        {
            plugins::openBuiltInSynth (eng, windows, *trackPtr);
            return;
        }

        if (! plugins::contains (*trackPtr, slot))
            return;

        if (! isInstrument() && getPowerArea().contains (e.getPosition()))
            eng.setPluginBypassed (*slot, ! slot->bypassed.load());
        else if (! isInstrument() && getRemoveArea().contains (e.getPosition()))
            // Removing rebuilds the list, which deletes this row: do it after this callback returns.
            juce::MessageManager::callAsync ([&eng, &windows, trackPtr, s = slot]
            {
                if (eng.indexOf (trackPtr) >= 0 && plugins::contains (*trackPtr, s))
                    plugins::remove (eng, windows, *trackPtr, *s);
            });
        else
            plugins::open (windows, *trackPtr, *slot);
    }

private:
    bool draggedNot() const { return owner.draggedRow != this; }
    juce::Rectangle<int> getPowerArea() const { return getLocalBounds().removeFromLeft (28); }
    juce::Rectangle<int> getRemoveArea() const { return getLocalBounds().removeFromRight (24); }

    InspectorPanel& owner;
    PluginSlot* const slot;
    bool shownBypassed = false;
    juce::String shownText;
};

//==============================================================================
/** A track that sends to the shown return: its name in its colour and the send's level. Click selects it. */
class InspectorPanel::FedRow final : public juce::Component, public juce::TooltipClient
{
public:
    FedRow (InspectorPanel& o, Track& s, Track& r) : owner (o), sender (&s), returnTrack (&r) {}

    void refresh()
    {
        const auto text = getLevelText();

        if (text != shownText)
        {
            shownText = text;
            repaint();
        }
    }

    juce::String getTooltip() override
    {
        if (owner.engine.indexOf (sender) < 0)
            return {};

        return sender->name + " sends to this return at " + getLevelText() + ". Click to select it.";
    }

    void paint (juce::Graphics& g) override
    {
        if (owner.engine.indexOf (sender) < 0)
            return;

        if (isMouseOver())
        {
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f, 1.0f), 3.0f);
        }

        auto area = getLocalBounds().reduced (sidePad, 0);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (getLevelText(), area.removeFromRight (56), juce::Justification::centredRight);

        g.setColour (sender->colour);
        g.fillRect (area.removeFromLeft (4).reduced (0, 5));
        area.removeFromLeft (6);
        g.setColour (sender->colour.brighter (0.3f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (sender->name, area, juce::Justification::centredLeft, true);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu() && ! e.mouseWasDraggedSinceMouseDown() && owner.engine.indexOf (sender) >= 0)
            owner.selection.select (sender);
    }

private:
    juce::String getLevelText() const
    {
        if (owner.engine.indexOf (sender) < 0 || owner.engine.indexOf (returnTrack) < 0)
            return {};

        const auto* send = routing::findSend (*sender, *returnTrack);
        return send != nullptr && send->isOn() ? juce::String (send->levelDb.load(), 1) + " dB" : juce::String ("Off");
    }

    InspectorPanel& owner;
    Track* const sender;
    Track* const returnTrack;
    juce::String shownText;
};

//==============================================================================
/** A text row that does something when clicked: + Effect, + Send to a new return. */
class InspectorPanel::AddRow final : public juce::Button
{
public:
    explicit AddRow (const juce::String& text) : juce::Button (text)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
    }

    void paintButton (juce::Graphics& g, bool isOver, bool isDown) override
    {
        if (isOver || isDown)
        {
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f, 1.0f), 3.0f);
        }

        g.setColour (isOver ? theme::accent : theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText (getButtonText(), getLocalBounds().reduced (sidePad, 0), juce::Justification::centredLeft, 1, 0.8f);
    }
};

//==============================================================================
InspectorPanel::InspectorPanel (Engine& e, Selection& s, PluginWindowManager& w, TrackActions& a)
    : engine (e), selection (s), pluginWindows (w), actions (a)
{
    guideButton.setTooltip ("What the inspector shows, and why sends are separate from effects (opens Buses, sends and routing)");
    guideButton.onClick = [this] { if (actions.onShowGuide) actions.onShowGuide (routingguide::Topic::inspector); };

    closeButton.setTooltip ("Hide the inspector (View > Inspector, I)");
    closeButton.onClick = [this] { if (onClose) onClose(); };

    // The same buttons, tooltips and colours as the mixer's strip.
    muteButton.setClickingTogglesState (true);
    muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0a030));
    muteButton.onClick = [this] { if (isShownAlive() && shownTrack != nullptr) shownTrack->muted = muteButton.getToggleState(); };

    soloButton.setClickingTogglesState (true);
    soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4fc06f));
    soloButton.onClick = [this] { if (isShownAlive() && shownTrack != nullptr) shownTrack->soloed = soloButton.getToggleState(); };

    outputButton.onClick = [this] { if (isShownAlive() && shownTrack != nullptr) actions.showOutputMenu (*shownTrack, &outputButton); };

    for (auto* b : std::initializer_list<juce::Button*> { &guideButton, &closeButton, &muteButton, &soloButton, &outputButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    panKnob.setScrollWheelEnabled (false);
    panKnob.setRange (-1.0, 1.0, 0.01);
    panKnob.setDoubleClickReturnValue (true, 0.0);
    panKnob.textFromValueFunction = formatPan;
    panKnob.setPopupDisplayEnabled (true, true, nullptr);
    panKnob.setWantsKeyboardFocus (false);
    panKnob.setTooltip ("Pan: where the track sits between the left and right speaker (double-click to centre)");
    panKnob.onValueChange = [this]
    {
        if (isShownAlive() && shownTrack != nullptr)
            shownTrack->pan = (float) panKnob.getValue();

        panLabel.setText (formatPan (panKnob.getValue()), juce::dontSendNotification);
    };
    addAndMakeVisible (panKnob);

    panLabel.setFont (juce::FontOptions (11.0f));
    panLabel.setColour (juce::Label::textColourId, theme::textDim);
    panLabel.setJustificationType (juce::Justification::centredLeft);
    panLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (panLabel);

    // The same range and feel as the mixer's fader.
    fader.textFromValueFunction = [] (double v) { return v <= -60.0 ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; };
    fader.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
    fader.setScrollWheelEnabled (false);
    fader.setRange (-60.0, 6.0, 0.1);
    fader.setSkewFactorFromMidPoint (-12.0);
    fader.setDoubleClickReturnValue (true, 0.0);
    fader.setSliderSnapsToMousePosition (false);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    fader.onValueChange = [this]
    {
        if (shownTrack == nullptr)
            engine.masterVolumeDb = (float) fader.getValue();
        else if (isShownAlive())
            shownTrack->volumeDb = (float) fader.getValue();
    };
    addAndMakeVisible (fader);

    content.onPaint = [this] (juce::Graphics& g)
    {
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));

        for (auto& [y, text] : headings)
        {
            g.setColour (theme::textDim);
            g.drawText (text, sidePad, y, content.getWidth() - 2 * sidePad, headingHeight - 4, juce::Justification::bottomLeft);
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText (g.getCurrentFont(), text, 0.0f, 0.0f);
            const int textWidth = (int) std::ceil (glyphs.getBoundingBox (0, -1, true).getWidth());
            g.setColour (theme::gridBar);
            g.fillRect (sidePad + textWidth + 6, y + headingHeight - 9, juce::jmax (0, content.getWidth() - 2 * sidePad - textWidth - 6), 1);
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));

        for (auto& [y, text] : notes)
            g.drawFittedText (text, sidePad, y, content.getWidth() - 2 * sidePad, rowHeight, juce::Justification::centredLeft, 1, 0.8f);
    };
    content.onPaintOverChildren = [this] (juce::Graphics& g)
    {
        if (draggedRow != nullptr && dropMarkerY >= 0)
        {
            g.setColour (theme::accent);
            g.fillRoundedRectangle (juce::Rectangle<float> (4.0f, (float) dropMarkerY - 1.5f, (float) content.getWidth() - 8.0f, 3.0f), 1.5f);
        }
    };
    content.addMouseListener (this, false);   // a right-click on the empty list opens the panel's menu
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (6);
    addAndMakeVisible (viewport);

    engine.addChangeListener (this);
    selection.addChangeListener (this);
    refresh();
    startTimerHz (30);
}

InspectorPanel::~InspectorPanel()
{
    stopTimer();
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
}

Track* InspectorPanel::getShownTrack() const
{
    auto* track = selection.getTrack();
    return track != nullptr && engine.indexOf (track) >= 0 ? track : nullptr;
}

bool InspectorPanel::isShownAlive() const
{
    return shownTrack == nullptr || engine.indexOf (shownTrack) >= 0;
}

void InspectorPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void InspectorPanel::timerCallback()
{
    if (! isShowing())
        return;

    refresh();
    updateValues();
}

void InspectorPanel::refresh()
{
    auto* track = getShownTrack();

    std::vector<const PluginSlot*> slots;
    std::vector<Track*> targets, senders;

    if (track != nullptr)
    {
        for (auto& p : track->plugins)
            slots.push_back (p.get());

        targets = routing::getSendTargets (track, engine.getTracks());

        if (track->isReturn())
            senders = routing::getSenders (*track, engine.getTracks());
    }

    const bool synth = track != nullptr && track->usesBuiltInSynth();

    if (track != shownTrack || slots != shownSlots || targets != shownTargets || senders != shownSenders || synth != shownBuiltInSynth
        || engine.getEditGeneration() != shownGeneration)
    {
        draggedRow = nullptr;
        dropIndex = dropMarkerY = -1;
        shownTrack = track;
        shownSlots = slots;
        shownTargets = targets;
        shownSenders = senders;
        shownBuiltInSynth = synth;
        shownGeneration = engine.getEditGeneration();
        shownName = {};
        shownType = {};
        shownOutput = {};
        shownSelectedCount = -1;
        rebuildRows();
        resized();
        repaint();
    }
}

void InspectorPanel::rebuildRows()
{
    // Clearing deletes the rows, so nothing may still point at them.
    sendKnobs.clear();
    fedRows.clear();
    addRows.clear();
    rows.clear();
    headings.clear();
    notes.clear();

    const bool isMaster = shownTrack == nullptr;

    muteButton.setVisible (! isMaster);
    soloButton.setVisible (! isMaster);
    outputButton.setVisible (! isMaster);
    panKnob.setVisible (! isMaster);
    panLabel.setVisible (! isMaster);
    viewport.setVisible (! isMaster);

    if (isMaster)
    {
        fader.setTooltip ("Master volume: the whole mix, as in the transport bar (double-click resets to 0 dB)");
        return;
    }

    auto& track = *shownTrack;
    fader.setTooltip ("Volume (double-click resets to 0 dB)");
    panKnob.setTooltip ("Pan: where the track sits between the left and right speaker (double-click to centre)");

    muteButton.setTooltip (track.isReturn() ? "Mute: take this return's effect out of the mix. The tracks keep playing, dry."
                           : track.isBus() ? "Mute: silence every track in this bus" : "Mute: silence this track, and its sends");
    soloButton.setTooltip (track.isReturn() ? "Solo: hear only this return. The tracks stay silent but keep sending to it."
                           : track.isBus() ? "Solo: hear this bus, with all its tracks" : "Solo: hear only the soloed tracks");

    if (track.isReturn())
    {
        outputButton.setTooltip ("A return plays into the master.");
        outputButton.setEnabled (false);
    }
    else
    {
        outputButton.setTooltip ("Where this track's sound goes: straight to the master, or into a bus. Click to change it.");
        outputButton.setEnabled (true);
    }

    auto addRow = [this] (EffectRow* row)
    {
        content.addAndMakeVisible (rows.add (row));
    };

    if (track.usesBuiltInSynth())
        addRow (new EffectRow (*this, nullptr));

    for (auto& p : track.plugins)
        if (p->isInstrument)
            addRow (new EffectRow (*this, p.get()));

    for (auto& p : track.plugins)
        if (! p->isInstrument)
            addRow (new EffectRow (*this, p.get()));

    auto* addEffect = addRows.add (new AddRow ("+ Effect"));
    addEffect->setTooltip ("Add an effect at the end of the chain");
    addEffect->onClick = [this] { if (isShownAlive() && shownTrack != nullptr) plugins::showAddMenu (engine, pluginWindows, *shownTrack, false, addRows.getFirst()); };
    content.addAndMakeVisible (addEffect);

    if (track.isReturn())
    {
        for (auto* sender : shownSenders)
            content.addAndMakeVisible (fedRows.add (new FedRow (*this, *sender, track)));
    }
    else
    {
        for (auto* target : shownTargets)
        {
            auto* knob = sendKnobs.add (new SendKnob (engine, actions, track, *target, SendKnob::Look::wide));
            content.addAndMakeVisible (knob);
        }

        auto* addSend = addRows.add (new AddRow ("+ Send to a new return"));
        addSend->setTooltip ("Make a return track (Reverb, Delay or empty) and send this track to it at -10 dB, about as much reverb as a 20% mix on the track.");
        addSend->onClick = [this, addSend] { showNewReturnMenu (*addSend); };
        content.addAndMakeVisible (addSend);
    }

    layoutRows();
}

void InspectorPanel::layoutRows()
{
    const int width = juce::jmax (50, viewport.getMaximumVisibleWidth());
    int y = 0;
    headings.clear();
    notes.clear();

    if (shownTrack == nullptr)
        return;

    headings.emplace_back (y, "EFFECTS");
    y += headingHeight;

    for (auto* row : rows)
    {
        row->setBounds (4, y, width - 8, rowHeight);
        y += rowHeight;
    }

    addRows.getFirst()->setBounds (4, y, width - 8, rowHeight);
    y += rowHeight + 4;

    if (shownTrack->isReturn())
    {
        headings.emplace_back (y, "FED BY");
        y += headingHeight;

        for (auto* row : fedRows)
        {
            row->setBounds (4, y, width - 8, rowHeight);
            y += rowHeight;
        }

        if (fedRows.isEmpty())
        {
            notes.emplace_back (y, "Nothing sends to it yet.");
            y += rowHeight;
        }
    }
    else
    {
        headings.emplace_back (y, "SENDS");
        y += headingHeight;

        for (auto* knob : sendKnobs)
        {
            knob->setBounds (10, y, width - 20, rowHeight);
            y += rowHeight;
        }

        if (sendKnobs.isEmpty())
        {
            notes.emplace_back (y, "No return in the song yet.");
            y += rowHeight;
        }

        addRows.getLast()->setBounds (4, y, width - 8, rowHeight);
        y += rowHeight;
    }

    content.setSize (width, y + 6);
}

void InspectorPanel::resized()
{
    auto r = getLocalBounds().withTrimmedRight (1);
    auto title = r.removeFromTop (titleHeight);
    closeButton.setBounds (title.removeFromRight (30).reduced (4, 4));
    guideButton.setBounds (title.removeFromRight (28).reduced (2, 4));

    const bool isMaster = shownTrack == nullptr;
    r.removeFromTop (6);
    nameArea = r.removeFromTop (24);
    typeArea = r.removeFromTop (16);
    selectionArea = {};
    noteArea = {};

    if (selection.getTracks().size() > 1 && ! isMaster)
        selectionArea = r.removeFromTop (16);

    if (isMaster)
    {
        noteArea = r.removeFromTop (46).reduced (sidePad, 4);
    }
    else
    {
        r.removeFromTop (4);
        auto buttons = r.removeFromTop (24).reduced (sidePad, 0);
        muteButton.setBounds (buttons.removeFromLeft (28));
        buttons.removeFromLeft (4);
        soloButton.setBounds (buttons.removeFromLeft (28));
        buttons.removeFromLeft (8);
        outputButton.setBounds (buttons);
        r.removeFromTop (4);
    }

    // Pan and the volume stay at the bottom; the effects and sends take the rest.
    const int volumeHeight = juce::jlimit (150, 260, r.getHeight() / 3);
    volumeArea = r.removeFromBottom (volumeHeight);

    if (! isMaster)
    {
        panArea = r.removeFromBottom (50);
        panKnob.setBounds (panArea.getX() + 64, panArea.getCentreY() - 19, 38, 38);
        panLabel.setBounds (panArea.getX() + 112, panArea.getCentreY() - 10, 60, 20);
    }
    else
        panArea = {};

    auto vol = volumeArea.withTrimmedTop (18);
    const int groupWidth = 70 + 8 + 14, left = vol.getX() + (vol.getWidth() - groupWidth) / 2;
    fader.setBounds (left, vol.getY(), 70, vol.getHeight() - 4);
    meterArea = juce::Rectangle<int> (left + 78, vol.getY() + 4, 14, vol.getHeight() - 4 - 24);

    viewport.setBounds (r);
    layoutRows();
}

void InspectorPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (theme::gridBar);
    g.fillRect (getWidth() - 1, 0, 1, getHeight());

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("Inspector", sidePad, 0, getWidth() - 80, titleHeight, juce::Justification::centredLeft);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (0, titleHeight - 1, getWidth() - 1, 1);

    const bool isMaster = shownTrack == nullptr;
    const auto colour = isMaster ? theme::text : shownTrack->colour;
    const auto name = isMaster ? juce::String ("Master") : shownTrack->name;

    auto nameBox = nameArea.reduced (sidePad, 0);
    g.setColour (colour);
    g.fillRect (nameBox.removeFromLeft (4).reduced (0, 2));
    nameBox.removeFromLeft (6);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawFittedText (name, nameBox, juce::Justification::centredLeft, 1, 0.6f);

    juce::String type;

    if (isMaster)
        type = "THE MIX BUS";
    else
    {
        type = shownTrack->isReturn() ? "RETURN" : shownTrack->isBus() ? "BUS" : shownTrack->isMidi() ? "MIDI TRACK" : "AUDIO TRACK";

        if (shownTrack->isReturn())
            type << dot() << "plays into the master";
        else if (shownTrack->bus != nullptr)
            type << dot() << "in " << shownTrack->bus->name;
    }

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f));
    g.drawFittedText (type, typeArea.reduced (sidePad + 10, 0), juce::Justification::centredLeft, 1, 0.8f);

    if (! selectionArea.isEmpty())
        g.drawFittedText (juce::String ((int) selection.getTracks().size()) + " selected; showing " + shownTrack->name,
                          selectionArea.reduced (sidePad + 10, 0), juce::Justification::centredLeft, 1, 0.8f);

    if (! noteArea.isEmpty())
        g.drawFittedText ("The master has no pan, effects or sends. It's the sum of everything.", noteArea, juce::Justification::topLeft, 3, 0.9f);

    auto heading = [&g] (const juce::String& text, juce::Rectangle<int> area)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (text, area, juce::Justification::centredLeft);
    };

    if (! panArea.isEmpty())
    {
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRect (0, panArea.getY(), getWidth() - 1, 1);
        heading ("PAN", panArea.withTrimmedLeft (sidePad).withWidth (50));
    }

    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRect (0, volumeArea.getY(), getWidth() - 1, 1);
    heading ("VOLUME", volumeArea.withTrimmedLeft (sidePad).withHeight (18));

    drawStereoMeter (g, meterArea.toFloat(), meterLeft, meterRight);
    g.setColour (theme::textDim.withAlpha (0.7f));
    const float zeroY = (float) meterArea.getBottom() - (float) meterArea.getHeight() * meterProportion (1.0f);
    g.fillRect ((float) meterArea.getX() - 3.0f, zeroY, 3.0f, 1.0f);
}

void InspectorPanel::updateValues()
{
    if (! isShownAlive())
        return;

    const float volume = shownTrack != nullptr ? shownTrack->volumeDb.load() : engine.masterVolumeDb.load();

    if (! fader.isMouseButtonDown() && ! juce::approximatelyEqual ((float) fader.getValue(), volume))
        fader.setValue (volume, juce::dontSendNotification);

    if (shownTrack != nullptr)
    {
        auto& track = *shownTrack;

        if (! panKnob.isMouseButtonDown() && ! juce::approximatelyEqual ((float) panKnob.getValue(), track.pan.load()))
            panKnob.setValue (track.pan.load(), juce::dontSendNotification);

        panLabel.setText (formatPan (panKnob.getValue()), juce::dontSendNotification);
        muteButton.setToggleState (track.muted.load(), juce::dontSendNotification);
        soloButton.setToggleState (track.soloed.load(), juce::dontSendNotification);

        const auto output = juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x92 ")) + TrackActions::getOutputName (track);

        if (output != shownOutput)
        {
            shownOutput = output;
            outputButton.setButtonText (output);
        }

        for (auto* knob : sendKnobs)
            knob->refresh();

        for (auto* row : fedRows)
            row->refresh();

        for (auto* row : rows)
            row->refresh();
    }

    // Name, type and the count of selected tracks: repaint the header when one changes.
    const auto name = shownTrack != nullptr ? shownTrack->name : juce::String();
    const auto busName = shownTrack != nullptr && shownTrack->bus != nullptr ? shownTrack->bus->name : juce::String();
    const int selected = (int) selection.getTracks().size();

    if (name != shownName || busName != shownType || selected != shownSelectedCount)
    {
        const bool relayout = (selected > 1) != (shownSelectedCount > 1);
        shownName = name;
        shownType = busName;
        shownSelectedCount = selected;

        if (relayout)
            resized();

        repaint();
    }

    const float left = shownTrack != nullptr ? shownTrack->peakLeft.load() : engine.masterPeakLeft.load();
    const float right = shownTrack != nullptr ? shownTrack->peakRight.load() : engine.masterPeakRight.load();

    if (! juce::approximatelyEqual (left, meterLeft) || ! juce::approximatelyEqual (right, meterRight))
    {
        meterLeft = left;
        meterRight = right;
        repaint (meterArea);
    }
}

juce::String InspectorPanel::getTooltip()
{
    const auto pos = getMouseXYRelative();

    if (pos.y < titleHeight)
        return "Inspector: the selected track's effects, sends, pan and volume. Click a track to show it here.";

    if (nameArea.contains (pos) || typeArea.contains (pos))
    {
        if (shownTrack == nullptr)
            return "The master: the mix bus, where every track, bus and return ends. Right-click for the guide.";

        if (! isShownAlive())
            return {};

        const auto kind = shownTrack->isReturn() ? "a return" : shownTrack->isBus() ? "a bus" : shownTrack->isMidi() ? "a MIDI track" : "an audio track";
        const auto where = shownTrack->isReturn() ? juce::String ("plays into the master")
                           : shownTrack->bus != nullptr ? "plays into " + shownTrack->bus->name : juce::String ("plays into the master");
        return shownTrack->name + ", " + kind + " that " + where;
    }

    return {};
}

void InspectorPanel::showMenu()
{
    juce::PopupMenu menu;
    menu.addItem ("Hide Inspector", [this] { if (onClose) onClose(); });
    menu.addSeparator();
    guideicon::addItem (menu, "About the inspector...", [this] { if (actions.onShowGuide) actions.onShowGuide (routingguide::Topic::inspector); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this));
}

void InspectorPanel::showNewReturnMenu (juce::Component& target)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Reverb");
    menu.addItem (2, "Delay");
    menu.addItem (3, "Empty");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                        [panel = juce::Component::SafePointer<InspectorPanel> (this), track = shownTrack] (int result)
                        {
                            if (panel == nullptr || result <= 0 || track == nullptr || panel->engine.indexOf (track) < 0)
                                return;

                            panel->actions.addReturn (result == 1 ? TrackActions::ReturnKind::reverb
                                                      : result == 2 ? TrackActions::ReturnKind::delay : TrackActions::ReturnKind::empty, { track });
                        });
}

void InspectorPanel::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;

    // Rows, knobs and buttons have their own menus; the rest of the panel has this one.
    auto* c = e.originalComponent;

    if (dynamic_cast<juce::Button*> (c) != nullptr || dynamic_cast<juce::Slider*> (c) != nullptr || dynamic_cast<EffectRow*> (c) != nullptr
        || dynamic_cast<FedRow*> (c) != nullptr || dynamic_cast<juce::Label*> (c) != nullptr)
        return;

    showMenu();
}

void InspectorPanel::dragRow (EffectRow& row, const juce::MouseEvent& e)
{
    if (! isShownAlive() || shownTrack == nullptr)
        return;

    draggedRow = &row;
    const auto inContent = e.getEventRelativeTo (&content).getPosition();
    viewport.autoScroll (viewport.getLocalPoint (&content, inContent).x, viewport.getLocalPoint (&content, inContent).y, 20, 8);

    int instruments = 0;

    for (auto& p : shownTrack->plugins)
        instruments += p->isInstrument ? 1 : 0;

    std::vector<EffectRow*> others;

    for (auto* other : rows)
        if (other != &row && ! other->isInstrument())
            others.push_back (other);

    int before = 0;

    for (auto* other : others)
        before += other->getBounds().getCentreY() < inContent.y ? 1 : 0;

    dropIndex = instruments + before;
    dropMarkerY = others.empty() ? -1 : before == 0 ? others.front()->getY() : others[(size_t) before - 1]->getBottom();
    content.repaint();
    row.repaint();
}

bool InspectorPanel::endRowDrag()
{
    auto* row = std::exchange (draggedRow, nullptr);

    if (row == nullptr)
        return false;

    const int target = std::exchange (dropIndex, -1);
    dropMarkerY = -1;
    content.repaint();
    row->repaint();

    // Moving rebuilds the rows once the change message arrives, after this mouse callback.
    if (auto* slot = row->getSlot(); slot != nullptr && target >= 0 && isShownAlive() && shownTrack != nullptr && plugins::contains (*shownTrack, slot))
        engine.movePlugin (*shownTrack, slot, target);

    return true;
}
} // namespace daw
