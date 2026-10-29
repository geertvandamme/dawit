#include "ChordPlayAlong.h"
#include "engine/Tempo.h"
#include "ChordLane.h"
#include "SectionLane.h"

namespace daw
{
namespace playalong = gen::playalong;
namespace diagrams = gen::diagrams;

namespace
{
    bool isBlackKey (int note)
    {
        const int pc = (note % 12 + 12) % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    /** The chord's spelled note name for each of its pitch classes ("Eb", not "D#", in Cm). */
    std::array<juce::String, 12> spell (const Chord& chord)
    {
        std::array<juce::String, 12> names;
        const auto spelled = juce::StringArray::fromTokens (chord.getNoteNames(), " ", {});
        const auto pitchClasses = chord.getPitchClasses();

        for (int i = 0; i < pitchClasses.size() && i < spelled.size(); ++i)
            names[(size_t) pitchClasses[i]] = spelled[i];

        return names;
    }

    juce::String getInstrumentId (ChordPlayAlong::Instrument instrument)
    {
        switch (instrument)
        {
            case ChordPlayAlong::Instrument::piano:  return "piano";
            case ChordPlayAlong::Instrument::guitar: return "guitar";
            case ChordPlayAlong::Instrument::both:   return "both";
        }

        return "both";
    }

    constexpr auto settingInstrument = "playAlong.instrument", settingCapo = "playAlong.capo", settingNoteNames = "playAlong.noteNames";
}

//==============================================================================
ChordPlayAlong::ChordPlayAlong (Engine& e) : engine (e)
{
    auto* settings = engine.properties.getUserSettings();
    const auto savedInstrument = settings->getValue (settingInstrument, "both");
    instrument = savedInstrument == "piano" ? Instrument::piano : savedInstrument == "guitar" ? Instrument::guitar : Instrument::both;
    capo = juce::jlimit (0, 7, settings->getIntValue (settingCapo, 0));
    noteNames = settings->getBoolValue (settingNoteNames, true);

    playButton.setTooltip ("Play or stop the song (Space), from the playhead");
    playButton.onClick = [this] { engine.togglePlay(); };

    for (auto* button : { &pianoButton, &guitarButton, &bothButton })
    {
        button->setClickingTogglesState (true);
        button->setRadioGroupId (0x91a7);
    }

    pianoButton.setTooltip ("Show each chord on a piano keyboard: blue keys for the right hand, orange for the left hand's bass note");
    guitarButton.setTooltip ("Show each chord as a guitar chord box, in a shape near the one before");
    bothButton.setTooltip ("Show the piano keyboard and the guitar chord box side by side");
    pianoButton.onClick = [this] { if (pianoButton.getToggleState()) setInstrument (Instrument::piano); };
    guitarButton.onClick = [this] { if (guitarButton.getToggleState()) setInstrument (Instrument::guitar); };
    bothButton.onClick = [this] { if (bothButton.getToggleState()) setInstrument (Instrument::both); };

    capoBox.onChange = [this] { setCapo (capoBox.getSelectedId() - 1); };

    namesToggle.setTooltip ("Write the note names under the piano keys");
    namesToggle.setToggleState (noteNames, juce::dontSendNotification);
    namesToggle.onClick = [this]
    {
        noteNames = namesToggle.getToggleState();
        engine.properties.getUserSettings()->setValue (settingNoteNames, noteNames);
        showDiagrams();
    };

    tipsButton.setTooltip ("How to play along: what to look at, which fingers to keep down, and what to do when a change is late");
    tipsButton.onClick = [this] { if (onOpenGuide != nullptr) onOpenGuide(); };

    // Clicking them mustn't take the keys away: Space plays and stops the song.
    for (juce::Component* c : std::initializer_list<juce::Component*> { &playButton, &pianoButton, &guitarButton, &bothButton, &capoBox, &namesToggle, &tipsButton })
    {
        c->setWantsKeyboardFocus (false);
        c->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (c);
    }

    // The diagrams show what to play; they don't play themselves, the song does.
    for (auto* keys : { &currentKeys, &nextKeys })
    {
        keys->setLarge (true);
        keys->setMouseCursor (juce::MouseCursor::NormalCursor);
        addChildComponent (keys);
    }

    for (auto* box : { &currentBox, &nextBox })
    {
        box->setLarge (true);
        box->setMouseCursor (juce::MouseCursor::NormalCursor);
        addChildComponent (box);
    }

    currentKeys.setTooltip ("The chord to play now. Blue keys: the right hand. Orange: the left hand's bass note. A filled dot marks the root");
    currentBox.setTooltip ("The chord to play now: strings from the low E (left) to the high E (right), frets downwards. "
                           "Numbers are fingers (1 index, 2 middle, 3 ring, 4 little), o an open string, x a string you don't play");
    nextKeys.setTooltip ("The next chord, lit on the last beat before it. Green rings: keys you keep down from the chord now");
    nextBox.setTooltip ("The next chord, lit on the last beat before it. Green rings: fingers that stay where they are");

    setInstrument (instrument);
    setSize (width, height);
    rebuild();
    update (true);

    engine.addChangeListener (this);
    startTimerHz (30);
}

ChordPlayAlong::~ChordPlayAlong()
{
    engine.removeChangeListener (this);
}

void ChordPlayAlong::setInstrument (Instrument newInstrument)
{
    instrument = newInstrument;
    engine.properties.getUserSettings()->setValue (settingInstrument, getInstrumentId (instrument));
    pianoButton.setToggleState (instrument == Instrument::piano, juce::dontSendNotification);
    guitarButton.setToggleState (instrument == Instrument::guitar, juce::dontSendNotification);
    bothButton.setToggleState (instrument == Instrument::both, juce::dontSendNotification);
    capoBox.setEnabled (instrument != Instrument::piano);
    resized();
    update (true);
}

void ChordPlayAlong::setCapo (int newCapo)
{
    if (newCapo < 0 || newCapo == capo)
        return;

    capo = newCapo;
    engine.properties.getUserSettings()->setValue (settingCapo, capo);
    rebuild();
    update (true);
}

void ChordPlayAlong::rebuild()
{
    changesFrom = engine.getChords();
    changes = playalong::getChanges (changesFrom);

    // Voiced and shaped through the whole song, as the chord track does: each near the one before.
    std::vector<Chord> chords, shapeChords;
    for (auto& c : changes)
    {
        chords.push_back (c.chord);
        shapeChords.push_back (playalong::getShapeChord (c.chord, capo));
    }

    voicings = diagrams::voiceProgression (chords);
    shapes = diagrams::shapeProgression (shapeChords);

    pianoLow = 127;
    pianoHigh = 0;

    for (size_t i = 0; i < changes.size() && i < voicings.size(); ++i)
    {
        pianoLow = juce::jmin (pianoLow, diagrams::pianoBass (changes[i].chord));

        for (int n : voicings[i])
        {
            pianoLow = juce::jmin (pianoLow, n);
            pianoHigh = juce::jmax (pianoHigh, n);
        }
    }

    if (pianoHigh < pianoLow)
    {
        pianoLow = 48;
        pianoHigh = 76;
    }

    // As the keyboard widens it: from a white key to a white key.
    while (isBlackKey (pianoLow))
        --pianoLow;

    while (isBlackKey (pianoHigh))
        ++pianoHigh;

    // The capos, with the one that makes the most chords open shapes marked.
    easiestCapo = playalong::findEasiestCapo (changes);
    capoBox.clear (juce::dontSendNotification);

    for (int fret = 0; fret <= 7; ++fret)
        capoBox.addItem ((fret == 0 ? juce::String ("No capo") : "Capo " + juce::String (fret))
                             + (fret == easiestCapo && fret > 0 ? juce::String (juce::CharPointer_UTF8 (" \xe2\x98\x85 easiest")) : juce::String()),
                         fret + 1);

    capoBox.setSelectedId (capo + 1, juce::dontSendNotification);
    capoBox.setEnabled (instrument != Instrument::piano);
    capoBox.setTooltip ("A capo clamps all the strings at a fret, so open chord shapes sound higher: with a capo on fret 3, "
                        "an Em shape sounds as Gm. The chord boxes then show the shapes to play, counted from the capo."
                        + (easiestCapo > 0 ? " For this song, capo " + juce::String (easiestCapo) + " gives the most open chords (marked with a star)."
                                           : juce::String (" For this song, no capo gives more open chords than none.")));
}

void ChordPlayAlong::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (engine.getChords() != changesFrom)
        rebuild();

    update (true);
}

void ChordPlayAlong::timerCallback()
{
    update (false);
}

void ChordPlayAlong::update (bool force)
{
    const playalong::Loop loop { engine.loopEnabled.load(), engine.loopStartBeat.load(), engine.loopEndBeat.load() };
    const auto m = playalong::findMoment (changes, engine.getSections(), engine.getHeardPositionBeats(), loop, 8);
    const bool about = playalong::isAboutToChange (m);
    const bool playing = engine.isPlaying();

    const bool chordChanged = force || m.current != moment.current || m.next != moment.next || about != aboutToChange;
    const bool beatChanged = chordChanged || m.bar != moment.bar || m.beatInBar != moment.beatInBar || m.section != moment.section;
    const bool countdownChanged = beatChanged || std::abs (m.beatsLeft - moment.beatsLeft) > 1.0e-3;

    moment = m;
    aboutToChange = about;

    if (force || playing != wasPlaying)
    {
        wasPlaying = playing;
        playButton.setButtonText (playing ? "Stop" : "Play");
    }

    if (chordChanged)
    {
        showDiagrams();
        repaint();
    }
    else if (beatChanged)
    {
        // The text that counts the beats; the diagrams stay as they are.
        repaint (positionArea);
        repaint (nextNameArea);
        repaint (comingArea);
        repaint (countdownArea);
    }
    else if (countdownChanged)
    {
        repaint (countdownArea);
    }
}

void ChordPlayAlong::showDiagrams()
{
    const bool piano = instrument != Instrument::guitar, guitar = instrument != Instrument::piano;

    const auto show = [&] (int index, KeyboardDiagram& keys, GuitarDiagram& box, int heldFrom)
    {
        const bool has = index >= 0 && index < (int) changes.size() && index < (int) voicings.size() && index < (int) shapes.size();
        keys.setVisible (has && piano);
        box.setVisible (has && guitar);

        if (! has)
            return;

        const auto& chord = changes[(size_t) index].chord;
        const bool held = heldFrom >= 0 && heldFrom < (int) changes.size() && heldFrom != index;
        keys.setRange (pianoLow, pianoHigh);
        keys.setNames (spell (chord));
        keys.setShowNames (noteNames);
        keys.setChord (voicings[(size_t) index], diagrams::pianoBass (chord), chord.root, held ? voicings[(size_t) heldFrom] : std::vector<int>());
        box.setShape (shapes[(size_t) index], held ? &shapes[(size_t) heldFrom] : nullptr);
    };

    show (moment.current, currentKeys, currentBox, -1);
    show (moment.next, nextKeys, nextBox, moment.current);

    // The next chord waits dimmed, and lights up when it's time to move.
    for (juce::Component* c : std::initializer_list<juce::Component*> { &nextKeys, &nextBox })
        c->setAlpha (aboutToChange ? 1.0f : 0.6f);

    nextKeys.setLit (aboutToChange);
    nextBox.setLit (aboutToChange);
}

//==============================================================================
void ChordPlayAlong::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto bar = r.removeFromTop (28);
    playButton.setBounds (bar.removeFromLeft (72));
    bar.removeFromLeft (16);

    for (auto* button : { &pianoButton, &guitarButton, &bothButton })
    {
        button->setBounds (bar.removeFromLeft (68));
        bar.removeFromLeft (4);
    }

    bar.removeFromLeft (12);
    capoBox.setBounds (bar.removeFromLeft (150));
    bar.removeFromLeft (12);
    namesToggle.setBounds (bar.removeFromLeft (120));
    tipsButton.setBounds (bar.removeFromRight (80));

    r.removeFromTop (10);
    positionArea = r.removeFromTop (30);
    r.removeFromTop (8);
    comingArea = r.removeFromBottom (62);
    r.removeFromBottom (12);

    nextArea = r.removeFromRight (r.getWidth() * 34 / 100);
    r.removeFromRight (14);
    currentArea = r;

    auto inner = currentArea.reduced (18, 14);
    currentNameArea = inner.removeFromTop (juce::jlimit (54, 130, currentArea.getHeight() / 5));
    countdownArea = inner.removeFromBottom (40);
    inner.removeFromBottom (12);
    inner.removeFromTop (26);   // the capo line, under the name
    currentDiagramArea = inner;

    auto nextInner = nextArea.reduced (16, 14);
    nextNameArea = nextInner.removeFromTop (22 + juce::jlimit (36, 80, nextArea.getHeight() / 8));
    nextInner.removeFromTop (26);
    nextDiagramArea = nextInner.withTrimmedBottom (52);   // level with the current diagrams, above the countdown

    layoutDiagrams (currentDiagramArea, currentKeys, currentBox, false);
    layoutDiagrams (nextDiagramArea, nextKeys, nextBox, true);
}

void ChordPlayAlong::layoutDiagrams (juce::Rectangle<int> area, KeyboardDiagram& keys, GuitarDiagram& box, bool stacked)
{
    int whites = 0;
    for (int n = pianoLow; n <= pianoHigh; ++n)
        if (! isBlackKey (n))
            ++whites;

    // A keyboard as wide as it can be with keys about 4.6 times as long as wide, names under them.
    const auto fitKeys = [whites] (juce::Rectangle<int> a)
    {
        const float keyWidth = juce::jmin ((float) a.getWidth() / (float) juce::jmax (1, whites), (float) a.getHeight() / 5.2f);
        return a.withSizeKeepingCentre ((int) (keyWidth * (float) whites), (int) (keyWidth * 5.2f));
    };

    // A chord box a bit taller than wide, like a printed one.
    const auto fitBox = [] (juce::Rectangle<int> a)
    {
        const int h = juce::jmin (a.getHeight(), (int) ((float) a.getWidth() * 1.3f));
        return a.withSizeKeepingCentre ((int) ((float) h * 0.77f), h);
    };

    if (instrument == Instrument::both && stacked)
    {
        // In the narrow panel: the keyboard above the chord box.
        auto keysArea = area.removeFromTop (area.getHeight() * 30 / 100);
        area.removeFromTop (12);
        keys.setBounds (fitKeys (keysArea));
        box.setBounds (fitBox (area));
    }
    else if (instrument == Instrument::both)
    {
        auto boxArea = area.removeFromRight (juce::jmin (area.getWidth() * 36 / 100, (int) ((float) area.getHeight() * 0.77f)));
        area.removeFromRight (16);
        keys.setBounds (fitKeys (area));
        box.setBounds (fitBox (boxArea));
    }
    else
    {
        keys.setBounds (fitKeys (area));
        box.setBounds (fitBox (area));
    }
}

//==============================================================================
void ChordPlayAlong::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    if (changes.empty())
    {
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        auto r = getLocalBounds().reduced (60).withTrimmedTop (40);
        g.drawFittedText ("The song has no chords yet", r.removeFromTop (40), juce::Justification::centred, 1);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("Put chords on the Chords row (click a bar), pick a progression in the chord guide and put it on the chord track, "
                          "or detect the chords of a recording (right-click an audio clip, Detect chords...). They show here, large, as the song plays.",
                          r.removeFromTop (80), juce::Justification::centredTop, 4);
        return;
    }

    // Where the song is.
    const auto& sections = engine.getSections();
    g.setColour (moment.section >= 0 ? SectionLane::getColour (sections[(size_t) moment.section].type).brighter (0.4f) : theme::text);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawFittedText (playalong::describePosition (moment, sections), positionArea.withTrimmedRight (260), juce::Justification::centredLeft, 1);

    juce::String info = formatBpm (engine.getTempo()) + " BPM";
    if (engine.loopEnabled.load())
        info << juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 looping bars ")) << juce::String ((int) std::floor (engine.loopStartBeat.load() / 4.0) + 1)
             << "-" << juce::String ((int) std::ceil (engine.loopEndBeat.load() / 4.0 - 1.0e-6));

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (info, positionArea, juce::Justification::centredRight);

    paintCurrent (g);
    paintNext (g);
    paintCountdown (g);
    paintComing (g);
}

void ChordPlayAlong::paintCurrent (juce::Graphics& g)
{
    g.setColour (theme::panel);
    g.fillRoundedRectangle (currentArea.toFloat(), 8.0f);

    if (moment.current >= 0)
    {
        const auto& chord = changes[(size_t) moment.current].chord;
        g.setColour (ChordLane::getColour (chord).brighter (0.5f));
        g.setFont (juce::FontOptions ((float) currentNameArea.getHeight() * 0.88f, juce::Font::bold));
        g.drawFittedText (chord.getName(), currentNameArea, juce::Justification::centredLeft, 1);

        juce::String detail = chord.getNoteNames();
        if (instrument != Instrument::piano && capo > 0)
            detail << "   " << playalong::describeCapo (chord, capo);
        else if (instrument != Instrument::piano && ! shapes.empty() && ! shapes[(size_t) moment.current].isEmpty())
            detail << "   " << shapes[(size_t) moment.current].getTab();

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText (detail, currentNameArea.withY (currentNameArea.getBottom()).withHeight (24), juce::Justification::centredLeft, 1);
        return;
    }

    // A bar without a chord: say so, and when the next one comes.
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions ((float) currentNameArea.getHeight() * 0.6f, juce::Font::bold));
    g.drawFittedText ("No chord", currentNameArea, juce::Justification::centredLeft, 1);

    g.setFont (juce::FontOptions (22.0f));
    const auto text = moment.next >= 0 ? changes[(size_t) moment.next].chord.getName() + " " + playalong::describeWait (moment.beatsToNext)
                                       : juce::String ("The chords have ended");
    g.setColour (theme::text);
    g.drawFittedText (text, currentDiagramArea.withHeight (40), juce::Justification::centredLeft, 1);
}

void ChordPlayAlong::paintNext (juce::Graphics& g)
{
    g.setColour (theme::panel.withAlpha (0.7f));
    g.fillRoundedRectangle (nextArea.toFloat(), 8.0f);

    if (aboutToChange)
    {
        g.setColour (theme::accent);
        g.drawRoundedRectangle (nextArea.toFloat().reduced (1.0f), 8.0f, 2.5f);
    }

    auto nameArea = nextNameArea;
    const auto header = nameArea.removeFromTop (22);
    g.setColour (aboutToChange ? theme::accent : theme::textDim);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));

    if (moment.next < 0)
    {
        g.drawText ("Next", header, juce::Justification::centredLeft);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (18.0f));
        g.drawFittedText ("Nothing after this: the song's last chord", nameArea, juce::Justification::centredLeft, 2);
        return;
    }

    g.drawText ("Next " + playalong::describeWait (moment.beatsToNext), header, juce::Justification::centredLeft);

    const auto& chord = changes[(size_t) moment.next].chord;
    g.setColour (ChordLane::getColour (chord).brighter (0.5f).withAlpha (aboutToChange ? 1.0f : 0.6f));
    g.setFont (juce::FontOptions ((float) nameArea.getHeight() * 0.85f, juce::Font::bold));
    g.drawFittedText (chord.getName(), nameArea, juce::Justification::centredLeft, 1);

    juce::String detail;
    if (instrument != Instrument::piano && capo > 0)
        detail = "Play the " + playalong::getShapeChord (chord, capo).getName() + " shape";
    else
        detail = chord.getNoteNames();

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (14.0f));
    g.drawFittedText (detail, nameArea.withY (nameArea.getBottom()).withHeight (24), juce::Justification::centredLeft, 1);
}

void ChordPlayAlong::paintCountdown (juce::Graphics& g)
{
    auto r = countdownArea;

    // The bar's beats, the one playing lit.
    const float dot = 26.0f;
    for (int beat = 0; beat < 4; ++beat)
    {
        const auto circle = r.removeFromLeft ((int) dot + 8).toFloat().withSizeKeepingCentre (dot, dot);
        const bool now = beat == moment.beatInBar;
        g.setColour (now ? theme::accent : theme::panelLight);
        g.fillEllipse (circle);
        g.setColour (now ? juce::Colour (0xff15161a) : theme::textDim);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (juce::String (beat + 1), circle, juce::Justification::centred);
    }

    r.removeFromLeft (12);
    const auto label = r.removeFromRight (170);
    const auto barArea = r.toFloat().withSizeKeepingCentre ((float) r.getWidth(), 12.0f);

    g.setColour (theme::panelLight);
    g.fillRoundedRectangle (barArea, 6.0f);

    juce::String text = "Held to the end";

    if (moment.beatsLeft >= 0.0 && moment.runBeats > 0.0)
    {
        // It runs out towards the change, and turns yellow on the last beat.
        const auto left = (float) juce::jlimit (0.0, 1.0, moment.beatsLeft / moment.runBeats);
        g.setColour (moment.beatsLeft <= 1.0 ? theme::meterYellow : theme::accent);
        g.fillRoundedRectangle (barArea.withWidth (juce::jmax (barArea.getHeight(), barArea.getWidth() * left)), 6.0f);
        text = "Change " + playalong::describeWait (moment.beatsLeft);
    }

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (16.0f));
    g.drawText (text, label, juce::Justification::centredRight);
}

void ChordPlayAlong::paintComing (juce::Graphics& g)
{
    auto r = comingArea;
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText ("Coming up", r.removeFromLeft (70), juce::Justification::centredLeft, 2);

    if (moment.coming.empty())
    {
        g.drawText ("No more chords", r, juce::Justification::centredLeft);
        return;
    }

    const int blockWidth = juce::jmin (140, r.getWidth() / 8);

    for (auto& c : moment.coming)
    {
        if (r.getWidth() < blockWidth / 2)
            break;

        auto block = r.removeFromLeft (blockWidth).reduced (3, 0).toFloat();
        const auto& chord = changes[(size_t) c.change].chord;
        g.setColour (ChordLane::getColour (chord));
        g.fillRoundedRectangle (block, 6.0f);

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.drawFittedText (chord.getName(), block.withTrimmedBottom (20.0f).toNearestInt(), juce::Justification::centred, 1);
        g.setColour (theme::text.withAlpha (0.8f));
        g.setFont (juce::FontOptions (12.5f));
        g.drawFittedText (playalong::describeWait (c.inBeats), block.removeFromBottom (24.0f).toNearestInt(), juce::Justification::centred, 1);
    }
}
} // namespace daw
