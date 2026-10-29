#include "ArrangementView.h"
#include "BounceActions.h"
#include "ChordSheetExport.h"
#include "engine/Tempo.h"

namespace daw
{
Ruler::Ruler (Engine& e, TimelineView& t) : engine (e), timeline (t) {}

void Ruler::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    // Loop range
    const float x1 = timeline.beatToX (engine.loopStartBeat.load());
    const float x2 = timeline.beatToX (engine.loopEndBeat.load());
    g.setColour (engine.loopEnabled.load() ? theme::loop.withAlpha (0.75f) : theme::textDim.withAlpha (0.25f));
    g.fillRect (x1, 0.0f, x2 - x1, 6.0f);

    // Bars and beats
    int barStep = 1;
    while (barStep * 4.0 * timeline.pixelsPerBeat < 44.0)
        barStep *= 2;

    const bool showBeats = timeline.pixelsPerBeat >= 10.0;
    const int firstBar = juce::jmax (0, (int) (timeline.xToBeat (0) / 4.0) - 1);
    g.setFont (juce::FontOptions (11.0f));

    for (int bar = firstBar; timeline.beatToX (bar * 4.0) < (float) getWidth(); ++bar)
    {
        const float x = timeline.beatToX (bar * 4.0);

        if (bar % barStep == 0)
        {
            g.setColour (theme::gridBar);
            g.fillRect (x, 8.0f, 1.0f, (float) getHeight() - 8.0f);
            g.setColour (theme::text);
            g.drawText (juce::String (bar + 1), juce::Rectangle<float> (x + 4.0f, 7.0f, 40.0f, 14.0f), juce::Justification::centredLeft);
        }

        if (showBeats)
        {
            g.setColour (theme::gridBar.withAlpha (0.6f));
            for (int beat = 1; beat < 4; ++beat)
                g.fillRect (timeline.beatToX (bar * 4.0 + beat), (float) getHeight() - 6.0f, 1.0f, 6.0f);
        }
    }

    // Playhead
    const float px = timeline.beatToX (engine.getPositionBeats());
    juce::Path marker;
    marker.addTriangle (px - 5.0f, (float) getHeight() - 9.0f, px + 5.0f, (float) getHeight() - 9.0f, px, (float) getHeight());
    g.setColour (theme::playhead);
    g.fillPath (marker);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void Ruler::mouseDown (const juce::MouseEvent& e)
{
    dragStartBeat = timeline.xToBeat (e.position.x);
    draggingLoop = false;

    if (e.mods.isPopupMenu() && onShowRhythmGuide != nullptr)
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Note lengths, beats and bars...");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                            [safeThis = juce::Component::SafePointer<Ruler> (this)] (int result)
                            {
                                if (auto* self = safeThis.getComponent(); self != nullptr && result == 1 && self->onShowRhythmGuide)
                                    self->onShowRhythmGuide();
                            });
    }
}

void Ruler::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() || e.getDistanceFromDragStart() < 4)
        return;

    draggingLoop = true;
    auto a = juce::jmax (0.0, std::round (dragStartBeat));
    auto b = juce::jmax (0.0, std::round (timeline.xToBeat (e.position.x)));

    if (b < a)
        std::swap (a, b);

    if (b - a >= 1.0)
    {
        engine.loopStartBeat = a;
        engine.loopEndBeat = b;
        engine.loopEnabled = true;
        repaint();

        if (onLoopChanged)
            onLoopChanged();
    }
}

void Ruler::mouseUp (const juce::MouseEvent& e)
{
    if (! draggingLoop && ! e.mods.isPopupMenu())
    {
        const double beat = timeline.xToBeat (e.position.x);
        engine.setPositionBeats (e.mods.isAltDown() ? beat : std::round (beat));
        repaint();
    }

    draggingLoop = false;
}

//==============================================================================
ArrangementView::ArrangementView (Engine& e, Selection& s, PluginWindowManager& w)
    : engine (e), selection (s), pluginWindows (w)
{
    addAndMakeVisible (ruler);
    addChildComponent (tempoLane);
    addChildComponent (tempoLabel);
    tempoLane.onShowRhythmGuide = [this] { if (onShowRhythmGuide) onShowRhythmGuide(); };
    tempoLabel.onClick = [this] { if (onShowRhythmGuide) onShowRhythmGuide(); };
    addAndMakeVisible (sectionLane);
    sectionLane.onShowGuide = [this] (int bar) { showStructureGuide (bar); };

    addAndMakeVisible (sectionsLabel);
    addAndMakeVisible (chordsHeader);
    sectionsLabel.onClick = [this] { showStructureGuide (getPlayheadBar()); };
    chordsHeader.onOpenGuide = [this] { showChordGuide (getPlayheadBar()); };
    chordsHeader.onExportPdf = [this]
    {
        chordsheet::exportSong (engine, getProjectName != nullptr ? getProjectName() : juce::String ("Untitled"),
                                chordLane.getInstrument() == ChordLane::Instrument::guitar ? chordsheet::Instrument::guitar : chordsheet::Instrument::piano);
    };
    chordsHeader.onPlayAlong = [this] { showPlayAlong(); };
    chordLane.onPlayAlong = [this] { showPlayAlong(); };
    chordLane.onLayoutChanged = [this]
    {
        chordsHeader.update();
        resized();
    };

    addAndMakeVisible (chordLane);
    addAndMakeVisible (lyricsLane);
    addAndMakeVisible (lyricsHeader);
    lyricsLane.onTappingChanged = [this] { lyricsHeader.update(); };
    lyricsLane.onShowGuide = [this] { showLyricsGuide(); };
    lyricsLane.onEditText = [this] { showLyricsEditor(); };
    lyricsHeader.onOpenGuide = [this] { showLyricsGuide(); };
    lyricsHeader.onSingAlong = [this] { showSingAlong(); };
    lyricsLane.onSingLyrics = [this] { singActions.openForLyrics(); };
    singActions.onOpenGuide = [this] (gen::lyricsguide::Topic topic)
    {
        showLyricsGuide();

        if (lyricsExplainer != nullptr)
            lyricsExplainer->showTopic (topic);
    };
    lyricsLane.onSingAlong = [this] { showSingAlong(); };
    lyricsLane.onLayoutChanged = [this]
    {
        lyricsHeader.update();
        resized();
    };
    foldButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb4")));
    foldButton.setTooltip ("Fold the Sections, Chords and Lyrics rows into a thin coloured line each, "
                           "to give the tracks the room (Cmd+3)");
    foldButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    foldButton.setWantsKeyboardFocus (false);
    foldButton.onClick = [this] { setSongRowsFolded (true); };
    addAndMakeVisible (foldButton);
    addChildComponent (foldedRows);
    addChildComponent (foldedHeader);
    foldedRows.onUnfold = [this] { setSongRowsFolded (false); };
    foldedHeader.onUnfold = [this] { setSongRowsFolded (false); };
    setSongRowsFolded (engine.properties.getUserSettings()->getBoolValue ("view.songRowsFolded", false));

    chordLane.onExplainProgressions = [this] (int bar) { showChordGuide (bar); };
    chordLane.onShowHowToPlay = [this] (int bar)
    {
        if (const auto* chord = engine.getChord (bar))
        {
            const auto shown = *chord;
            showChordGuide (bar);

            if (chordExplainer != nullptr)
                chordExplainer->showHowToPlay (shown);
        }
    };
    chordLane.onPlayChords = [this] (int bar)
    {
        // The new track goes below the selected one, or at the end.
        const int index = selection.getTrack() != nullptr ? engine.indexOf (selection.getTrack()) + 1 : -1;
        chordStyles.show (bar, index);
    };
    chordStyles.onPlayed = [this] (Track& t, MidiClip& c) { selection.select (&t, &c); };
    ruler.onLoopChanged = [this] { lanes.repaint(); };
    ruler.onShowRhythmGuide = [this] { if (onShowRhythmGuide) onShowRhythmGuide(); };

    content.addAndMakeVisible (lanes);
    content.addChildComponent (dropMarker);
    trackActions.onEditName = [this] (Track& bus)
    {
        rebuildTracks();

        for (auto* h : headers)
            if (&h->getTrack() == &bus)
                h->editName();
    };
    trackActions.onShowSingingGuide = [this] { showSingingGuide (gen::singing::Topic::voice); };
    trackActions.onShowGuide = [this] (routingguide::Topic topic) { if (onShowRoutingGuide) onShowRoutingGuide (topic); };
    trackActions.onShowRecordingGuide = [this] (recordingguide::Topic topic) { if (onShowRecordingGuide) onShowRecordingGuide (topic); };
    lanes.onShowRecordingGuide = [this] { if (onShowRecordingGuide) onShowRecordingGuide (recordingguide::Topic::takes); };
    lanes.onShowTriggerGuide = [this] { if (onShowTriggerGuide) onShowTriggerGuide (triggerguide::Topic::whatItIs); };
    seenRecordingReport = engine.getRecordingReportVersion();
    trackActions.onDelete = [this] (std::vector<Track*> tracks, bool withItsTracks) { deleteTracks (std::move (tracks), withItsTracks); };
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    hScroll.setAutoHide (false);
    hScroll.addListener (this);
    addAndMakeVisible (hScroll);

    zoomInButton.onClick = [this] { zoomAround (1.4, timeline.xToBeat (lanes.getWidth() * 0.5)); };
    zoomOutButton.onClick = [this] { zoomAround (1.0 / 1.4, timeline.xToBeat (lanes.getWidth() * 0.5)); };

    for (auto* b : { &zoomInButton, &zoomOutButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setTooltip ("Zoom (or Cmd/Ctrl + scroll wheel)");
        addAndMakeVisible (b);
    }

    lanes.onScroll = [this] (double delta) { setViewStart (timeline.viewStartBeat + delta); };
    lanes.onZoom = [this] (double factor, double anchor) { zoomAround (factor, anchor); };
    lanes.onImportRequest = [this] (Track& t, double beat) { importAudio (&t, beat); };
    lanes.onOpenClipEditor = [this] { if (onOpenClipEditor) onOpenClipEditor(); };
    lanes.onSingThis = [this] (Track& t, MidiClip& c) { singActions.open (t, &c); };
    lanes.onBounceClip = [this] (Track& t, Clip& c) { bounceui::bounceInPlace (engine, selection, t, &c); };
    lanes.onSplitIntoStems = [this] (Track& t, AudioClip& c) { stemSplitter.split (t, c); };
    lanes.onRemoveVocals = [this] (Track& t, AudioClip& c) { stemSplitter.removeVocals (t, c); };
    stemSplitter.onStemsAdded = [this] (const std::vector<Track*>& added) { selection.select (added.front()); };
    lanes.onConvertToMidi = [this] (Track& t, AudioClip& c) { audioToMidi.convert (t, c); };
    audioToMidi.onConverted = [this] (Track& t, MidiClip& c) { selection.select (&t, &c); };
    lanes.onHitsToMidi = [this] (Track& t, AudioClip& c) { hitsToMidi.open (t, c); };
    hitsToMidi.onShowGuide = [this] (triggerguide::Topic topic) { if (onShowTriggerGuide) onShowTriggerGuide (topic); };
    lanes.onDetectLyrics = [this] (Track& t, AudioClip& c) { lyricsDetector.detect (t, c); };
    lyricsDetector.onLyricsFound = [this] { setSongRowsFolded (false); };   // show the words that came in
    lanes.onDetectTempo = [this] (Track& t, AudioClip& c) { tempoDetector.detect (t, c); };
    lanes.onDetectChords = [this] (Track& t, AudioClip& c) { chordDetector.detect (t, c); };
    chordDetector.onDetectTempo = [this] (Track& t, AudioClip& c) { tempoDetector.detect (t, c); };
    chordDetector.onChordsSet = [this] { setSongRowsFolded (false); };   // show the chords that came in
    lanes.onDetectStructure = [this] (Track& t, AudioClip& c) { structureDetector.detect (t, c); };
    structureDetector.onDetectTempo = [this] (Track& t, AudioClip& c) { tempoDetector.detect (t, c); };
    structureDetector.onSectionsSet = [this] { setSongRowsFolded (false); };   // show the sections that came in

    engine.addChangeListener (this);
    selection.addChangeListener (this);

    rebuildTracks();
    startTimerHz (30);
}

ArrangementView::~ArrangementView()
{
    engine.removeChangeListener (this);
    selection.removeChangeListener (this);
}

RowLabel::RowLabel (const juce::String& labelText, const juce::String& tooltip) : text (labelText)
{
    setTooltip (tooltip);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void RowLabel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    const bool hovered = isMouseOver();

    g.setColour (hovered ? theme::panelLight.brighter (0.08f) : theme::panelLight);
    g.fillRect (area);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (text, area.withTrimmedLeft (26), juce::Justification::centredLeft);

    // The guide icon sits at the front, in the same column as the Lyrics row's guide button.
    const auto hintArea = juce::Rectangle<float> (1.0f, 0.0f, 22.0f, (float) area.getHeight()).withSizeKeepingCentre (20.0f, 17.0f);
    guideicon::draw (g, hintArea, hovered ? theme::accent : theme::textDim);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (area.removeFromBottom (1));
}

void RowLabel::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onClick != nullptr)
        onClick();
}

//==============================================================================
FoldedSongRows::FoldedSongRows (Engine& e, TimelineView& t) : engine (e), timeline (t)
{
    engine.addChangeListener (this);
}

FoldedSongRows::~FoldedSongRows()
{
    engine.removeChangeListener (this);
}

juce::Rectangle<float> FoldedSongRows::getLine (int index, float width)
{
    return { 0.0f, 3.0f + (float) lineStep * (float) index, width, 4.0f };
}

void FoldedSongRows::updatePlayhead()
{
    const float x = timeline.beatToX (engine.getPositionBeats());

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 5, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 5, getHeight());
}

void FoldedSongRows::paint (juce::Graphics& g)
{
    g.fillAll (theme::laneAlt);

    const auto width = (float) getWidth();
    const double firstBeat = timeline.xToBeat (0.0f), lastBeat = timeline.xToBeat (width);

    // The tempo's line first while it changes; the others move down by one.
    const int first = TempoLane::isNeeded (engine) ? 1 : 0;

    // Where each line runs, so an empty row still shows.
    g.setColour (theme::text.withAlpha (0.05f));
    for (int i = 0; i < 3 + first; ++i)
        g.fillRect (getLine (i, width));

    // A pixel apart, so neighbouring blocks stay two.
    const auto drawBlock = [&] (int row, double startBeat, double endBeat, juce::Colour colour)
    {
        if (endBeat < firstBeat || startBeat > lastBeat)
            return;

        const auto line = getLine (row + first, width);
        const float x1 = timeline.beatToX (startBeat), x2 = timeline.beatToX (endBeat);
        g.setColour (colour);
        g.fillRect (juce::Rectangle<float> (x1, line.getY(), juce::jmax (1.0f, x2 - x1 - 1.0f), line.getHeight()));
    };

    // The tempo: brighter where the band plays faster.
    if (first > 0)
    {
        const auto& changes = engine.getTempoMap().getChanges();
        const double end = juce::jmax (engine.getSongEndBar() * 4.0, changes.back().beat + 4.0);
        const auto range = engine.getTempoMap().getRange (0.0, end);

        for (size_t i = 0; i < changes.size(); ++i)
        {
            const double speed = range.getLength() > 0.0 ? (changes[i].bpm - range.getStart()) / range.getLength() : 1.0;
            drawBlock (-1, changes[i].beat, i + 1 < changes.size() ? changes[i + 1].beat + 0.05 : end,
                       TempoLane::colour.withAlpha ((float) (0.35 + 0.65 * speed)));
        }
    }

    for (auto& s : engine.getSections())
        drawBlock (0, s.startBar * 4.0, s.getEndBar() * 4.0, SectionLane::getColour (s.type).brighter (0.2f));

    // A chord repeated on the following bars is one block, as on the chord track.
    const auto& chords = engine.getChords();

    for (auto it = chords.begin(); it != chords.end();)
    {
        const int startBar = it->first;
        const Chord chord = it->second;
        int endBar = startBar + 1;

        for (++it; it != chords.end() && it->first == endBar && it->second == chord; ++it)
            ++endBar;

        drawBlock (1, startBar * 4.0, endBar * 4.0, ChordLane::getColour (chord).brighter (0.3f));
    }

    // The lyrics' lines, with a notch where each syllable starts when there's room to see it.
    for (auto& line : engine.getLyrics().lines)
    {
        drawBlock (2, line.startBeat, line.endBeat, LyricsLane::colour.withAlpha (0.85f));

        if (line.endBeat < firstBeat || line.startBeat > lastBeat)
            continue;

        const auto area = getLine (2 + first, width);
        float previousX = timeline.beatToX (line.startBeat);
        g.setColour (theme::laneAlt);

        for (size_t i = 1; i < line.syllables.size(); ++i)
        {
            const float x = timeline.beatToX (line.syllables[i].beat);

            if (x - previousX >= 4.0f)
            {
                g.fillRect (juce::Rectangle<float> (std::floor (x), area.getY(), 1.0f, area.getHeight()));
                previousX = x;
            }
        }
    }

    g.setColour (theme::playhead); // where updatePlayhead() last put it, as in TrackLanes
    g.fillRect (playheadX, 0.0f, 1.0f, (float) getHeight());

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

juce::String FoldedSongRows::getTooltip()
{
    const double beat = timeline.xToBeat ((float) getMouseXYRelative().x);

    if (beat < 0.0)
        return {};

    const int bar = (int) std::floor (beat / 4.0);
    juce::StringArray parts { "Bar " + juce::String (bar + 1) };

    if (TempoLane::isNeeded (engine))
        parts.add (formatBpm (engine.getTempoMap().getBpmAt (bar * 4.0)) + " BPM");

    for (auto& s : engine.getSections())
        if (bar >= s.startBar && bar < s.getEndBar())
            parts.add (s.getDisplayName());

    if (const auto* chord = engine.getChord (bar))
        parts.add (chord->getName());

    for (auto& line : engine.getLyrics().lines)
        if (beat >= line.startBeat && beat < line.endBeat)
            parts.add (juce::String::fromUTF8 ("\xe2\x80\x9c") + LyricsLane::getLineText (line) + juce::String::fromUTF8 ("\xe2\x80\x9d"));

    return parts.joinIntoString (juce::String::fromUTF8 ("  \xc2\xb7  "))
           + (TempoLane::isNeeded (engine) ? "\nTempo, sections, chords and lyrics, folded. Double-click to show them in full (Cmd+3)"
                                           : "\nSections, chords and lyrics, folded. Double-click to show them in full (Cmd+3)");
}

void FoldedSongRows::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && ! e.mods.isPopupMenu())
    {
        const double beat = juce::jmax (0.0, timeline.xToBeat (e.position.x));
        engine.setPositionBeats (e.mods.isAltDown() ? beat : std::round (beat));
    }
}

void FoldedSongRows::mouseDoubleClick (const juce::MouseEvent&)
{
    if (onUnfold != nullptr)
        onUnfold();
}

FoldedSongRowsHeader::FoldedSongRowsHeader()
{
    setTooltip ("Sections, chords and lyrics, folded into a line each. Click to show them in full (Cmd+3)");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);

    arrowButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbe")));
    arrowButton.setTooltip ("Show the Sections, Chords and Lyrics rows in full (Cmd+3)");
    arrowButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    arrowButton.setWantsKeyboardFocus (false);
    arrowButton.onClick = [this] { if (onUnfold != nullptr) onUnfold(); };
    addAndMakeVisible (arrowButton);
}

void FoldedSongRowsHeader::paint (juce::Graphics& g)
{
    g.fillAll (isMouseOverOrDragging (true) ? theme::panelLight.brighter (0.08f) : theme::panelLight);

    // A short piece of each line, in the order they're drawn.
    std::vector<juce::Colour> colours { SectionLane::getColour (SectionType::verse).brighter (0.2f),
                                        ChordLane::getColour (Chord {}).brighter (0.3f), LyricsLane::colour };

    if (showsTempo)
        colours.insert (colours.begin(), TempoLane::colour);

    for (size_t i = 0; i < colours.size(); ++i)
    {
        g.setColour (colours[i]);
        g.fillRect (FoldedSongRows::getLine ((int) i, 16.0f).translated (26.0f, 0.0f));
    }

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.5f));
    g.drawText (showsTempo ? "Tempo, sections, chords, lyrics" : "Sections, chords, lyrics", getLocalBounds().withTrimmedLeft (50),
                juce::Justification::centredLeft);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (getLocalBounds().removeFromBottom (1));
}

void FoldedSongRowsHeader::setShowsTempo (bool shouldShow)
{
    if (showsTempo != shouldShow)
    {
        showsTempo = shouldShow;
        repaint();
    }
}

void FoldedSongRowsHeader::resized()
{
    arrowButton.setBounds (getLocalBounds().removeFromLeft (22).reduced (1));
}

void FoldedSongRowsHeader::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onUnfold != nullptr)
        onUnfold();
}

//==============================================================================
ChordTrackHeader::ChordTrackHeader (ChordLane& l) : lane (l)
{
    arrowButton.setTooltip ("Show or hide how to play each chord, on piano or guitar");
    arrowButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    arrowButton.onClick = [this] { lane.setExpanded (! lane.isExpanded()); };
    addAndMakeVisible (arrowButton);

    label.onClick = [this] { if (onOpenGuide != nullptr) onOpenGuide(); };
    addAndMakeVisible (label);

    for (auto* button : { &pianoButton, &guitarButton })
    {
        button->setClickingTogglesState (true);
        button->setRadioGroupId (0xc407d);
        addChildComponent (button);
    }

    pianoButton.setTooltip ("Show each chord on a piano keyboard, voiced near the one before");
    guitarButton.setTooltip ("Show each chord as a guitar chord box, in a shape near the one before");
    pianoButton.onClick = [this] { if (pianoButton.getToggleState()) lane.setInstrument (ChordLane::Instrument::piano); };
    guitarButton.onClick = [this] { if (guitarButton.getToggleState()) lane.setInstrument (ChordLane::Instrument::guitar); };

    pdfButton.setTooltip ("Save the song's chords as a PDF to print or play from: every chord it uses, then bar by bar, "
                          "with the piano or guitar diagrams shown here");
    pdfButton.onClick = [this] { if (onExportPdf != nullptr) onExportPdf(); };
    addChildComponent (pdfButton);

    playAlongButton.setTooltip ("Play along: the chord playing now shown large on piano and guitar, with the next one beside it, "
                                "in a window of its own that follows the song (Cmd+4)");
    playAlongButton.onClick = [this] { if (onPlayAlong != nullptr) onPlayAlong(); };
    addAndMakeVisible (playAlongButton);
    update();
}

void ChordTrackHeader::update()
{
    const bool open = lane.isExpanded();
    arrowButton.setButtonText (open ? juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbe")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb8")));
    pianoButton.setVisible (open);
    guitarButton.setVisible (open);
    pdfButton.setVisible (open);
    pianoButton.setToggleState (lane.getInstrument() == ChordLane::Instrument::piano, juce::dontSendNotification);
    guitarButton.setToggleState (lane.getInstrument() == ChordLane::Instrument::guitar, juce::dontSendNotification);
    repaint();
}

void ChordTrackHeader::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelLight);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (getLocalBounds().removeFromBottom (1));
}

void ChordTrackHeader::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (ChordLane::height);
    arrowButton.setBounds (top.removeFromLeft (22).reduced (1));
    playAlongButton.setBounds (top.removeFromRight (78).reduced (2, 3));
    label.setBounds (top);

    auto buttons = r.reduced (8, 0).withTrimmedLeft (14).removeFromTop (24);
    pianoButton.setBounds (buttons.removeFromLeft (64));
    buttons.removeFromLeft (4);
    guitarButton.setBounds (buttons.removeFromLeft (64));
    buttons.removeFromLeft (8);
    pdfButton.setBounds (buttons.removeFromLeft (56));
}

//==============================================================================
LyricsTrackHeader::LyricsTrackHeader (LyricsLane& l) : lane (l)
{
    arrowButton.setTooltip ("Show or hide every line written out in full under the syllables, readable at any zoom");
    arrowButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    arrowButton.onClick = [this] { lane.setExpanded (! lane.isExpanded()); };
    addAndMakeVisible (arrowButton);

    guideButton.setTooltip ("How lyrics are written: what each part says, rhyme, stress on the beat, words on a melody, "
                            "the hook, and a look at your own lyrics");
    guideButton.onClick = [this] { if (onOpenGuide != nullptr) onOpenGuide(); };
    addAndMakeVisible (guideButton);

    singButton.setTooltip ("Sing along: the line to sing now in large letters, to read while you record a vocal, with the colour "
                           "running over each syllable in time with this row, the next line under it, and a count-in after a rest");
    singButton.onClick = [this] { if (onSingAlong != nullptr) onSingAlong(); };
    addAndMakeVisible (singButton);

    editButton.setTooltip ("Type or edit the whole lyrics: a line per sung line, an empty line between verses, "
                           "[Chorus] above a verse, hyphens between syllables");
    editButton.onClick = [this] { lane.editText(); };
    addAndMakeVisible (editButton);

    tapButton.setTooltip ("Tap along: the song plays from a bar before the playhead's line, and you press any letter key "
                          "the moment each syllable is sung, to put it there. The row shows which syllable is next. Esc or Stop ends it");
    tapButton.onClick = [this]
    {
        if (lane.isTapping())
            lane.stopTapping (true);
        else
            lane.startTapping();
    };
    addAndMakeVisible (tapButton);

    // Clicking them mustn't take the keys from the row: losing them would end tapping before Stop gets the click.
    for (auto* button : std::initializer_list<juce::TextButton*> { &arrowButton, &guideButton, &singButton, &editButton, &tapButton })
    {
        button->setWantsKeyboardFocus (false);
        button->setMouseClickGrabsKeyboardFocus (false);
    }
    update();
}

void LyricsTrackHeader::update()
{
    arrowButton.setButtonText (lane.isExpanded() ? juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbe")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb8")));
    tapButton.setToggleState (lane.isTapping(), juce::dontSendNotification);
    tapButton.setButtonText (lane.isTapping() ? "Stop" : "Tap");
    repaint();
}

void LyricsTrackHeader::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelLight);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (getLocalBounds().removeFromBottom (1));

    auto area = getLocalBounds().removeFromTop (LyricsLane::height).withTrimmedLeft (22);
    g.setColour (LyricsLane::colour);
    g.fillRect (0, 6, 3, LyricsLane::height - 12);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));

    if (lane.isTapping())
    {
        auto text = area.withTrimmedLeft (guideButton.getRight() - 22 + 3).withRight (singButton.getX() - 2);
        g.drawText ("Lyrics", text.removeFromTop (LyricsLane::height / 2).withTrimmedTop (3), juce::Justification::bottomLeft);
        g.setColour (theme::accent);
        g.setFont (juce::FontOptions (10.5f));
        g.drawFittedText ("tapping", text.withTrimmedBottom (3), juce::Justification::topLeft, 1, 0.8f);
    }
    else
    {
        g.drawText ("Lyrics", area.withTrimmedLeft (guideButton.getRight() - 22 + 3), juce::Justification::centredLeft);
    }
}

void LyricsTrackHeader::resized()
{
    auto top = getLocalBounds().removeFromTop (LyricsLane::height);
    arrowButton.setBounds (top.removeFromLeft (22).withSizeKeepingCentre (22, 26).reduced (1));
    auto r = top.reduced (8, 0).withSizeKeepingCentre (top.getWidth() - 16, 24);
    tapButton.setBounds (r.removeFromRight (40));
    r.removeFromRight (3);
    editButton.setBounds (r.removeFromRight (50));
    r.removeFromRight (3);
    singButton.setBounds (r.removeFromRight (72));
    guideButton.setBounds (22, r.getY(), 22, r.getHeight());
}

//==============================================================================
int ArrangementView::getPlayheadBar() const
{
    return juce::jmax (0, (int) std::floor (engine.getPositionBeats() / 4.0));
}

void ArrangementView::showChordGuide (int bar)
{
    // One window: opening it again from another bar moves where "Put on chord track" writes.
    if (chordExplainer != nullptr)
        chordExplainer->setStartBar (bar);

    pluginWindows.show (&chordExplainer, "Chord progressions", [this, bar]
    {
        auto* explainer = new ChordExplainer (engine, bar);
        explainer->onPlayAlong = [this] { showPlayAlong(); };
        chordExplainer = explainer;
        return explainer;
    });
}

void ArrangementView::showStructureGuide (int bar)
{
    if (structureExplainer != nullptr)
        structureExplainer->setStartBar (bar);

    pluginWindows.show (&structureExplainer, "Song structure", [this, bar]
    {
        auto* explainer = new StructureExplainer (engine, bar);
        structureExplainer = explainer;
        return explainer;
    });
}

guideprint::Item ArrangementView::getPrintGuide (gen::guides::Guide guide)
{
    using G = gen::guides::Guide;
    guideprint::Item item;
    item.guide = guide;

    switch (guide)
    {
        case G::chords:
            if (chordExplainer != nullptr)
                item.useOpen (chordExplainer.getComponent());
            else
                item.adopt (std::make_unique<ChordExplainer> (engine, getPlayheadBar()));
            break;

        case G::chordShapes:
        {
            auto explainer = std::make_unique<ChordExplainer> (engine, getPlayheadBar());
            explainer->setPrintChordShapes (true);
            item.adopt (std::move (explainer));
            break;
        }

        case G::structure:
            if (structureExplainer != nullptr)
                item.useOpen (structureExplainer.getComponent());
            else
                item.adopt (std::make_unique<StructureExplainer> (engine, getPlayheadBar()));
            break;

        case G::lyrics:
            if (lyricsExplainer != nullptr)
                item.useOpen (lyricsExplainer.getComponent());
            else
                item.adopt (std::make_unique<LyricsExplainer> (engine));
            break;

        case G::singing:
            if (singingExplainer != nullptr)
                item.useOpen (singingExplainer.getComponent());
            else
                item.adopt (std::make_unique<SingingExplainer> (engine, [this] { return selection.getTrack(); }));
            break;

        case G::readMusic:
        case G::sound:
        case G::midi:
        case G::rhythm:
        case G::styles:
        case G::gainStaging:
        case G::eq:
        case G::compressor:
        case G::gate:
        case G::tune:
        case G::stereo:
        case G::routing:
        case G::reverb:
        case G::delay:
        case G::saturation:
        case G::distortion:
        case G::subtractive:
        case G::additive:
        case G::fm:
        case G::drums:
        case G::physical:
        case G::recording:
        case G::tuning:
        case G::triggers:
            break;
    }

    return item;
}

void ArrangementView::showGuide (gen::guides::Guide guide, int topic)
{
    using G = gen::guides::Guide;
    const int bar = getPlayheadBar();

    switch (guide)
    {
        case G::chords:
            showChordGuide (bar);

            if (chordExplainer != nullptr && topic >= 0)
                chordExplainer->showTopic (juce::jmin (topic, (int) ChordExplainer::firstProgressionRow));
            break;

        case G::chordShapes:
        {
            const auto chord = engine.getChord (bar) != nullptr ? *engine.getChord (bar) : Chord();
            showChordGuide (bar);

            if (chordExplainer != nullptr)
                chordExplainer->showHowToPlay (chord);
            break;
        }

        case G::structure:
            showStructureGuide (bar);

            if (structureExplainer != nullptr && topic >= 0)
                structureExplainer->showTopic (juce::jmin (topic, (int) StructureExplainer::firstStructureRow));
            break;

        case G::lyrics:
            showLyricsGuide();

            if (lyricsExplainer != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (gen::lyricsguide::allTopics)))
                lyricsExplainer->showTopic (gen::lyricsguide::allTopics[topic]);
            break;

        case G::singing:
            showSingingGuide (juce::isPositiveAndBelow (topic, (int) std::size (gen::singing::allTopics)) ? gen::singing::allTopics[topic] : gen::singing::Topic::voice);
            break;

        // The other guides open from the main window.
        case G::readMusic:
        case G::sound:
        case G::midi:
        case G::rhythm:
        case G::styles:
        case G::gainStaging:
        case G::eq:
        case G::compressor:
        case G::gate:
        case G::tune:
        case G::stereo:
        case G::routing:
        case G::reverb:
        case G::delay:
        case G::saturation:
        case G::distortion:
        case G::subtractive:
        case G::additive:
        case G::fm:
        case G::drums:
        case G::physical:
        case G::recording:
        case G::tuning:
        case G::triggers:
            jassertfalse;
            break;
    }
}

void ArrangementView::removeVocals (const AudioClip& clip)
{
    if (auto* track = engine.findTrackFor (&clip))
        for (auto& c : track->clips)
            if (c.get() == &clip)
                stemSplitter.removeVocals (*track, static_cast<AudioClip&> (*c));
}

void ArrangementView::showPlayAlong()
{
    pluginWindows.show (&playAlong, "Play along with the chords", [this]
    {
        auto* window = new ChordPlayAlong (engine);
        window->onOpenGuide = [this]
        {
            showChordGuide (getPlayheadBar());

            if (chordExplainer != nullptr)
                chordExplainer->showPlayingAlong();
        };
        playAlong = window;
        return window;
    }, { 720, 520 });
}

void ArrangementView::showSingAlong()
{
    pluginWindows.show (&singAlong, "Sing along with the lyrics", [this]
    {
        auto* window = new LyricsSingAlong (engine);
        window->onOpenGuide = [this]
        {
            showLyricsGuide();

            if (lyricsExplainer != nullptr)
                lyricsExplainer->showTopic (gen::lyricsguide::Topic::singing);
        };
        window->onOpenSingingGuide = [this] { showSingingGuide (gen::singing::Topic::breathing); };
        singAlong = window;
        return window;
    }, { 640, 460 });
}

void ArrangementView::showLyricsEditor()
{
    // Non-modal: the song keeps playing and the guide can be opened and clicked while it's open.
    pluginWindows.show (&lyricsEditor, "Lyrics", [this]
    {
        auto* window = new LyricsEditWindow (engine, juce::jmax (0, (int) std::floor (engine.getPositionBeats() / 4.0)));
        window->onShowGuide = [this] (gen::lyricsguide::Topic topic)
        {
            showLyricsGuide();

            if (lyricsExplainer != nullptr)
                lyricsExplainer->showTopic (topic);
        };
        window->onClose = [safeThis = juce::Component::SafePointer<ArrangementView> (this)]
        {
            juce::MessageManager::callAsync ([safeThis]
            {
                if (auto* self = safeThis.getComponent())
                    self->pluginWindows.closeFor (&self->lyricsEditor);
            });
        };
        lyricsEditor = window;
        return window;
    }, { 560, 520 });
}

void ArrangementView::showLyricsGuide()
{
    pluginWindows.show (&lyricsExplainer, "How lyrics are written", [this]
    {
        auto* explainer = new LyricsExplainer (engine);
        explainer->onOpenSingingGuide = [this] { showSingingGuide (gen::singing::Topic::breathing); };
        lyricsExplainer = explainer;
        return explainer;
    });
}

void ArrangementView::showSingingGuide (gen::singing::Topic topic)
{
    pluginWindows.show (&singingExplainer, "Singing techniques", [this]
    {
        auto* explainer = new SingingExplainer (engine, [this] { return selection.getTrack(); });
        singingExplainer = explainer;
        return explainer;
    });

    if (singingExplainer != nullptr)
        singingExplainer->showTopic (topic);
}

void ArrangementView::setSongRowsFolded (bool shouldBeFolded)
{
    if (songRowsFolded == shouldBeFolded)
        return;

    songRowsFolded = shouldBeFolded;
    engine.properties.getUserSettings()->setValue ("view.songRowsFolded", songRowsFolded);

    // Tapping needs the Lyrics row.
    if (songRowsFolded && lyricsLane.isTapping())
        lyricsLane.stopTapping (true);

    for (auto* c : std::initializer_list<juce::Component*> { &sectionLane, &chordLane, &lyricsLane,
                                                             &sectionsLabel, &foldButton, &chordsHeader, &lyricsHeader })
        c->setVisible (! songRowsFolded);

    foldedRows.setVisible (songRowsFolded);
    foldedHeader.setVisible (songRowsFolded);
    updateTempoRow();
    resized();
    repaint();

    if (onSongRowsFoldedChanged != nullptr)
        onSongRowsFoldedChanged();
}

void ArrangementView::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::panel);
    g.fillRect (0, 0, headerWidth, rulerHeight);

    // Behind the fold button, left of the Sections label.
    if (! songRowsFolded)
    {
        g.setColour (theme::panelLight);
        g.fillRect (foldButton.getBounds().expanded (1));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (0, sectionsLabel.getBottom() - 1, foldButton.getRight() + 1, 1);
    }
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - scrollBarHeight, headerWidth, scrollBarHeight);
}

void ArrangementView::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (rulerHeight);

    // Headers of the section, chord and lyrics tracks, which sit between the ruler and the tracks.
    if (songRowsFolded)
    {
        foldedHeader.setBounds (r.removeFromTop (foldedRows.getPreferredHeight()).withWidth (headerWidth));
    }
    else
    {
        if (tempoShown)
            tempoLabel.setBounds (r.removeFromTop (TempoLane::height).withWidth (headerWidth));

        auto sections = r.removeFromTop (SectionLane::height).withWidth (headerWidth);
        foldButton.setBounds (sections.removeFromLeft (22).reduced (1));
        sectionsLabel.setBounds (sections);
        chordsHeader.setBounds (r.removeFromTop (chordLane.getPreferredHeight()).withWidth (headerWidth));
        lyricsHeader.setBounds (r.removeFromTop (lyricsLane.getPreferredHeight()).withWidth (headerWidth));
    }

    auto corner = top.removeFromLeft (headerWidth).reduced (4, 3);
    zoomInButton.setBounds (corner.removeFromRight (24));
    corner.removeFromRight (4);
    zoomOutButton.setBounds (corner.removeFromRight (24));

    hScroll.setBounds (r.removeFromBottom (scrollBarHeight).withTrimmedLeft (headerWidth));
    viewport.setBounds (r);
    layoutContent();

    ruler.setBounds (top.withWidth (lanes.getWidth()));
}

void ArrangementView::layoutContent()
{
    const auto rows = routing::getVisibleTracks (engine.getTracks());
    const int height = juce::jmax (viewport.getHeight(), ((int) rows.size() + 1) * TrackLanes::rowHeight);
    content.setSize (viewport.getWidth(), height);

    // Re-read the width: a vertical scroll bar may have appeared.
    const int width = viewport.getMaximumVisibleWidth();
    content.setSize (width, height);

    // A track inside a folded bus has no row.
    for (auto* header : headers)
    {
        const auto it = std::find (rows.begin(), rows.end(), &header->getTrack());
        header->setVisible (it != rows.end());

        if (it != rows.end())
            header->setBounds (0, (int) (it - rows.begin()) * TrackLanes::rowHeight, headerWidth, TrackLanes::rowHeight);
    }

    lanes.setBounds (headerWidth, 0, juce::jmax (0, width - headerWidth), height);
    ruler.setBounds (headerWidth, 0, lanes.getWidth(), rulerHeight);
    tempoLane.setBounds (headerWidth, rulerHeight, lanes.getWidth(), tempoShown ? TempoLane::height : 0);
    sectionLane.setBounds (headerWidth, rulerHeight + (tempoShown ? TempoLane::height : 0), lanes.getWidth(), SectionLane::height);
    chordLane.setBounds (headerWidth, sectionLane.getBottom(), lanes.getWidth(), chordLane.getPreferredHeight());
    lyricsLane.setBounds (headerWidth, chordLane.getBottom(), lanes.getWidth(), lyricsLane.getPreferredHeight());
    foldedRows.setBounds (headerWidth, rulerHeight, lanes.getWidth(), foldedRows.getPreferredHeight());
    updateScrollBar();
}

void ArrangementView::rebuildTracks()
{
    const auto& tracks = engine.getTracks();
    bool same = shownEditGeneration == engine.getEditGeneration() && headers.size() == (int) tracks.size();
    shownEditGeneration = engine.getEditGeneration();

    for (int i = 0; same && i < headers.size(); ++i)
        same = &headers[i]->getTrack() == tracks[(size_t) i].get();

    if (! same)
    {
        headers.clear();

        for (auto& t : tracks)
        {
            auto* header = headers.add (new TrackHeader (engine, *t, selection, pluginWindows, trackActions));
            header->onDragMove = [this] (TrackHeader& h, const juce::MouseEvent& e) { dragTracks (h, e); };
            header->onDragEnd = [this] (TrackHeader& h) { dropTracks (h); };
            header->onDragCancel = [this] { cancelTrackDrag(); };
            content.addAndMakeVisible (header);
        }

        dropMarker.toFront (false);
    }

    // Folding a bus or moving a track changes which rows show and how far they are indented.
    layoutContent();

    for (auto* h : headers)
    {
        h->updateStructure();
        h->updateFxButton();
        h->repaint();
    }

    lanes.repaint();
}

void ArrangementView::deleteTracks (std::vector<Track*> tracks, bool withItsTracks)
{
    // Returns are left out of getOutermost (they are never in a bus), so they are kept apart and put back.
    std::vector<Track*> returns;

    for (auto* t : tracks)
        if (t->isReturn())
            returns.push_back (t);

    tracks = routing::getOutermost (tracks, engine.getTracks());
    tracks.insert (tracks.end(), returns.begin(), returns.end());

    if (tracks.empty())
        return;

    // A bus on its own goes without its tracks, which stay and play where the bus played.
    if (! withItsTracks)
    {
        for (auto* t : tracks)
            if (t->isBus())
                trackActions.ungroup (*t);

        return;
    }

    // Everything that goes: the tracks, and what is inside a bus.
    std::vector<Track*> doomed;

    for (auto& t : engine.getTracks())
        if (std::any_of (tracks.begin(), tracks.end(), [&t] (Track* o) { return o == t.get() || routing::playsInto (*t, *o); }))
            doomed.push_back (t.get());

    auto remove = [safeThis = juce::Component::SafePointer<ArrangementView> (this), tracks]
    {
        if (safeThis == nullptr)
            return;

        auto& self = *safeThis;
        std::vector<Track*> gone;

        for (auto* t : tracks)
            if (self.engine.indexOf (t) >= 0)
                for (auto& other : self.engine.getTracks())
                    if (other.get() == t || routing::playsInto (*other, *t))
                        gone.push_back (other.get());

        for (auto* t : gone)
        {
            for (auto& slot : t->plugins)
                self.pluginWindows.closeFor (*slot->instance);

            self.pluginWindows.closeFor (t); // its built-in synth
            self.selection.forget (t);
        }

        // Drop the headers before the tracks they point at go away.
        for (int i = self.headers.size(); --i >= 0;)
            if (std::find (gone.begin(), gone.end(), &self.headers[i]->getTrack()) != gone.end())
                self.headers.remove (i);

        for (auto* t : tracks)
            self.engine.removeTrack (t);

        self.rebuildTracks();
        self.layoutContent();
    };

    const bool hasContent = std::any_of (doomed.begin(), doomed.end(), [this] (Track* t)
    {
        return ! t->clips.empty() || ! t->plugins.empty() || (t->isReturn() && ! routing::getSenders (*t, engine.getTracks()).empty());
    });

    if (doomed.size() == 1 && ! hasContent)
    {
        juce::MessageManager::callAsync (remove);
        return;
    }

    juce::String question;

    if (tracks.size() == 1 && tracks.front()->isReturn())
    {
        juce::StringArray senders;

        for (auto* sender : routing::getSenders (*tracks.front(), engine.getTracks()))
            senders.add (sender->name);

        question = senders.isEmpty()
                       ? "Delete the return \"" + tracks.front()->name + "\" and its effects?"
                       : "Delete the return " + tracks.front()->name + "? " + (senders.size() == 1 ? senders[0] : senders.joinIntoString (", ", 0, senders.size() - 1) + " and " + senders[senders.size() - 1])
                             + (senders.size() == 1 ? " sends" : " send") + " to it; their sends go too. The tracks keep playing, without this "
                             + (tracks.front()->name.containsIgnoreCase ("delay") || tracks.front()->name.containsIgnoreCase ("echo") ? "delay." : "reverb.");
    }
    else if (tracks.size() == 1 && tracks.front()->isBus())
        question = "Delete the bus \"" + tracks.front()->name + "\" and its " + juce::String ((int) doomed.size() - 1) + " tracks, with everything on them?";
    else if (doomed.size() > 1)
        question = "Delete " + juce::String ((int) doomed.size()) + " tracks and everything on them?";
    else
        question = "Delete \"" + doomed.front()->name + "\" and everything on it?";

    auto options = juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon, "Delete", question, "Delete", "Cancel", this);
    juce::AlertWindow::showAsync (options, [remove] (int result)
    {
        if (result == 1)
            remove();
    });
}

//==============================================================================
std::vector<Track*> ArrangementView::getDraggedTracks (Track& clicked) const
{
    if (clicked.isReturn())
        return { &clicked };   // a return moves by itself, among the returns

    if (selection.isSelected (&clicked))
        return routing::getOutermost (selection.getTracks(), engine.getTracks());

    return { &clicked };
}

void ArrangementView::dragTracks (TrackHeader& header, const juce::MouseEvent& e)
{
    const auto dragged = getDraggedTracks (header.getTrack());
    const auto p = e.getEventRelativeTo (&content).position;
    const auto inViewport = e.getEventRelativeTo (&viewport).getPosition();
    viewport.autoScroll (inViewport.x, inViewport.y, 24, 14);

    const auto rows = routing::getVisibleTracks (engine.getTracks());
    const int numRows = (int) rows.size();
    const float rowPosition = p.y / (float) TrackLanes::rowHeight;
    const int row = (int) std::floor (rowPosition);
    const float inRow = rowPosition - (float) row;

    const auto canTake = [&dragged] (Track* bus)
    {
        return std::all_of (dragged.begin(), dragged.end(), [bus] (Track* t) { return routing::canMoveInto (*t, bus); });
    };

    dropTarget = {};

    // Over the middle of a bus's row: the whole row is outlined, and the tracks go to the end of that bus.
    if (row >= 0 && row < numRows && rows[(size_t) row]->isBus() && inRow >= 0.3f && inRow <= 0.7f && canTake (rows[(size_t) row]))
    {
        dropTarget = { true, { rows[(size_t) row], nullptr } };
        dropMarker.showOutline ({ 0, row * TrackLanes::rowHeight, content.getWidth(), TrackLanes::rowHeight });
        return;
    }

    // Otherwise a line between two rows; further left in the header column goes out of a bus, further right stays in.
    const int gap = juce::jlimit (0, numRows, inRow < 0.5f ? row : row + 1);
    const auto [shallowest, deepest] = routing::getGapLevels (rows, gap);
    const int choices = deepest - shallowest + 1;
    const int level = shallowest + juce::jlimit (0, choices - 1, (int) (p.x / (float) headerWidth * (float) choices));
    auto place = routing::getPlace (rows, gap, level);

    // Returns stay together below everything else: a track dropped among them goes above the first, and a return
    // can only be dropped between or after the returns, at the top level.
    int firstReturnRow = numRows;

    for (int i = 0; i < numRows; ++i)
        if (rows[(size_t) i]->isReturn())
        {
            firstReturnRow = i;
            break;
        }

    const bool draggingReturn = dragged.size() == 1 && dragged.front()->isReturn();

    if (draggingReturn)
        place = { nullptr, gap < numRows ? rows[(size_t) gap] : nullptr };
    else if (place.bus == nullptr && firstReturnRow < numRows && (place.before == nullptr || place.before->isReturn()))
        place.before = rows[(size_t) firstReturnRow];

    bool valid = canTake (place.bus) && (! draggingReturn || gap >= firstReturnRow);

    for (auto* t : dragged)
        valid = valid && place.before != t && (place.before == nullptr || ! routing::playsInto (*place.before, *t));

    if (! valid)
    {
        dropMarker.hideMarker();
        return;
    }

    dropTarget = { true, place };
    const int x = draggingReturn ? 0 : juce::jmin (level, TrackHeader::maxGutters) * TrackHeader::gutterWidth;
    dropMarker.showLine ({ x, gap * TrackLanes::rowHeight - 2, content.getWidth() - x, 4 });
}

void ArrangementView::dropTracks (TrackHeader& header)
{
    dropMarker.hideMarker();
    const auto target = std::exchange (dropTarget, {});

    if (! target.valid)
        return;

    for (auto* t : getDraggedTracks (header.getTrack()))
        engine.moveTrack (*t, target.place.bus, target.place.before);
}

void ArrangementView::cancelTrackDrag()
{
    dropMarker.hideMarker();
    dropTarget = {};
}

void TrackDropMarker::showLine (juce::Rectangle<int> area)
{
    mode = Mode::line;
    setBounds (area);
    setVisible (true);
    repaint();
}

void TrackDropMarker::showOutline (juce::Rectangle<int> area)
{
    mode = Mode::outline;
    setBounds (area);
    setVisible (true);
    repaint();
}

void TrackDropMarker::hideMarker()
{
    mode = Mode::none;
    setVisible (false);
}

void TrackDropMarker::paint (juce::Graphics& g)
{
    if (mode == Mode::line)
    {
        g.setColour (theme::accent);
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (0.0f, 1.0f), 1.5f);
    }
    else if (mode == Mode::outline)
    {
        g.setColour (theme::accent.withAlpha (0.12f));
        g.fillRect (getLocalBounds());
        g.setColour (theme::accent);
        g.drawRect (getLocalBounds().reduced (1), 2);
    }
}

//==============================================================================
double ArrangementView::getTotalBeats() const
{
    double end = 0.0;

    for (auto& t : engine.getTracks())
        for (auto& c : t->clips)
            end = juce::jmax (end, c->getEndBeat (engine.getTempoMap()));

    return juce::jmax (256.0, end + 64.0);
}

void ArrangementView::updateScrollBar()
{
    const double visible = lanes.getWidth() / timeline.pixelsPerBeat;
    hScroll.setRangeLimits (0.0, juce::jmax (getTotalBeats(), timeline.viewStartBeat + visible));
    hScroll.setCurrentRange (timeline.viewStartBeat, visible, juce::dontSendNotification);
}

void ArrangementView::setViewStart (double beat)
{
    timeline.viewStartBeat = juce::jmax (0.0, beat);
    updateScrollBar();
    ruler.repaint();
    sectionLane.repaint();
    chordLane.repaint();
    lyricsLane.repaint();
    foldedRows.repaint();
    lanes.repaint();
}

void ArrangementView::zoomAround (double factor, double anchorBeat)
{
    const float anchorX = timeline.beatToX (anchorBeat);
    timeline.pixelsPerBeat = juce::jlimit (2.0, 400.0, timeline.pixelsPerBeat * factor);
    setViewStart (anchorBeat - anchorX / timeline.pixelsPerBeat);
}

void ArrangementView::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
{
    timeline.viewStartBeat = juce::jmax (0.0, newRangeStart);
    ruler.repaint();
    sectionLane.repaint();
    chordLane.repaint();
    lyricsLane.repaint();
    foldedRows.repaint();
    lanes.repaint();
}

void ArrangementView::updateTempoRow()
{
    const bool needed = TempoLane::isNeeded (engine);
    const bool show = needed && ! songRowsFolded;
    foldedHeader.setShowsTempo (needed);
    tempoLane.setVisible (show);
    tempoLabel.setVisible (show);

    // The folded rows grow by a line too, so lay out again whenever it changes.
    if (show != tempoShown || foldedRows.getHeight() != foldedRows.getPreferredHeight())
    {
        tempoShown = show;
        resized();
    }
}

void ArrangementView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateTempoRow();
    rebuildTracks();
    updateScrollBar();
}

void ArrangementView::timerCallback()
{
    // Keep the playhead in view while playing.
    if (engine.isPlaying())
    {
        const double pos = engine.getPositionBeats();
        const double visible = lanes.getWidth() / timeline.pixelsPerBeat;

        if (pos > timeline.viewStartBeat + visible * 0.95 || pos < timeline.viewStartBeat)
            setViewStart (pos - visible * 0.05);
    }

    lanes.updatePlayhead();
    lanes.updateRecording();
    recordingui::showReportMessages (engine, seenRecordingReport);
    tempoLane.updatePlayhead();
    sectionLane.updatePlayhead();
    chordLane.updatePlayhead();
    lyricsLane.updatePlayhead();
    foldedRows.updatePlayhead();
    ruler.repaint();
}

//==============================================================================
void ArrangementView::importAudio()
{
    auto* track = selection.getTrack();
    importAudio (track != nullptr && track->isAudio() ? track : nullptr, engine.getPositionBeats());
}

void ArrangementView::importAudio (Track* target, double beat)
{
    chooser = std::make_unique<juce::FileChooser> ("Import audio", juce::File(), engine.audioFormats.getWildcardForAllFormats());

    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::canSelectMultipleItems;

    chooser->launchAsync (flags, [this, target, beat] (const juce::FileChooser& fc)
    {
        const auto files = fc.getResults();

        if (files.isEmpty())
            return;

        auto* track = target != nullptr && engine.indexOf (target) >= 0 ? target : engine.addTrack (Track::Type::audio);
        lanes.importAudioFiles (files, *track, beat);
    });
}
} // namespace daw
