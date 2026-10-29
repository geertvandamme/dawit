#include "ScoreView.h"
#include "GuideButton.h"
#include "generate/ChordSheet.h"

#include <map>

namespace daw
{
namespace
{
    constexpr float minSpace = 6.0f, maxSpace = 16.0f, defaultSpace = 9.0f;

    juce::String sharpSign() { return juce::String::fromUTF8 ("\xe2\x99\xaf"); }
    juce::String flatSign()  { return juce::String::fromUTF8 ("\xe2\x99\xad"); }

    scorepaint::Ink getInk()
    {
        return {};
    }

    /** 0.03, 0.94, 1.5: at most two decimals, no trailing zeros. */
    juce::String formatBeats (double value)
    {
        auto text = juce::String (value, 2);

        if (text.contains ("."))
            text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

        return text.isEmpty() ? juce::String ("0") : text;
    }

    juce::String capitalise (const juce::String& s)
    {
        return s.substring (0, 1).toUpperCase() + s.substring (1);
    }

    juce::String formatPitch (const gen::notation::Pitch& p)
    {
        return juce::String::charToString ("CDEFGAB"[p.letter]) + (p.alter > 0 ? sharpSign() : p.alter < 0 ? flatSign() : juce::String())
               + juce::String (p.octave);
    }

    juce::String getKeyMarkName (const gen::notation::KeyMark& m)
    {
        return juce::String::charToString ("CDEFGAB"[m.letter]) + (m.alter > 0 ? sharpSign() : flatSign());
    }

    juce::String formatBeat (double beatInBar)
    {
        return "bar " + juce::String ((int) std::floor (beatInBar / 4.0 + 1e-9) + 1) + ", beat "
               + formatBeats (std::fmod (beatInBar, 4.0) + 1.0);
    }
} // namespace

//==============================================================================
/** The scrolling surface: it only passes the mouse and painting on to the view. */
class ScoreView::Canvas final : public juce::Component, public juce::TooltipClient
{
public:
    explicit Canvas (ScoreView& v) : owner (v) {}

    void paint (juce::Graphics& g) override { owner.paintScore (g); }

    juce::String getTooltip() override { return owner.getTooltipAt (getMouseXYRelative().toFloat()); }

    void mouseMove (const juce::MouseEvent& e) override { owner.mousePosition = e.position; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
            owner.showMenu();
        else
            owner.handleClick (e);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            owner.handleDoubleClick (e);
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        auto& view = owner.viewport;

        if ((e.mods.isCommandDown() || e.mods.isCtrlDown()) && wheel.deltaY != 0.0f)
        {
            owner.zoomBy (wheel.deltaY > 0 ? 1.1 : 1.0 / 1.1, (float) (e.x - view.getViewPositionX()));
            return;
        }

        // The line runs sideways: the wheel moves along it, unless the staves don't fit and Shift isn't held.
        const bool tallerThanView = getHeight() > view.getMaximumVisibleHeight();
        const bool sideways = e.mods.isShiftDown() || std::abs (wheel.deltaX) > std::abs (wheel.deltaY) || ! tallerThanView;

        if (sideways)
        {
            const float delta = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
            view.setViewPosition (juce::jmax (0, view.getViewPositionX() - (int) (delta * 600.0f)), view.getViewPositionY());
        }
        else
        {
            view.mouseWheelMove (e.getEventRelativeTo (&view), wheel);
        }
    }

private:
    ScoreView& owner;
};

//==============================================================================
ScoreView::ScoreView (Engine& e, Selection& s) : engine (e), selection (s)
{
    canvas = std::make_unique<Canvas> (*this);

    if (auto* settings = engine.properties.getUserSettings())
    {
        space = juce::jlimit (minSpace, maxSpace, (float) settings->getDoubleValue ("view.scoreZoom", defaultSpace));
        showChords = settings->getBoolValue ("view.scoreChords", true);
    }

    viewport.setViewedComponent (canvas.get(), false);
    viewport.setScrollBarsShown (true, true);
    addChildComponent (viewport);

    selection.addChangeListener (this);
    engine.addChangeListener (this);
}

ScoreView::~ScoreView()
{
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

void ScoreView::setSpace (float newSpace)
{
    newSpace = juce::jlimit (minSpace, maxSpace, newSpace);

    if (juce::approximatelyEqual (newSpace, space))
        return;

    space = newSpace;
    engine.properties.getUserSettings()->setValue ("view.scoreZoom", (double) space);
    hitsDirty = true;
    updateCanvasSize();
    canvas->repaint();
}

void ScoreView::zoomBy (double factor, float anchorX)
{
    const float before = space;
    const int contentX = viewport.getViewPositionX() + (int) anchorX;
    setSpace (space * (float) factor);
    const double ratio = (double) space / (double) before;

    if (! juce::approximatelyEqual (ratio, 1.0))
        viewport.setViewPosition (juce::jmax (0, (int) (contentX * ratio) - (int) anchorX), (int) (viewport.getViewPositionY() * ratio));
}

void ScoreView::visibilityChanged()
{
    if (isVisible())
    {
        rebuild();
        startTimerHz (30);
    }
    else
    {
        stopTimer();
    }
}

void ScoreView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (isVisible())
        refresh();
}

void ScoreView::refresh()
{
    rebuild();
    scrollToSelectedClip();
}

void ScoreView::resized()
{
    viewport.setBounds (getLocalBounds());
    updateCanvasSize();
}

float ScoreView::getTopY() const noexcept
{
    return scorepaint::getTopMargin (space);
}

void ScoreView::updateCanvasSize()
{
    if (! hasAnyNotes)
        return;

    const int width = (int) std::ceil (getMargin() + (line.width + 3.0) * space);
    const int height = (int) std::ceil (getTopY() + scorepaint::getStaffBlockHeight (score, space) + scorepaint::getBottomMargin (space));
    canvas->setSize (juce::jmax (width, viewport.getMaximumVisibleWidth()), juce::jmax (height, viewport.getMaximumVisibleHeight()));
}

//==============================================================================
void ScoreView::rebuild()
{
    shownTrack = selection.getTrack();
    hasAnyNotes = false;
    score = {};
    input.notes.clear();
    input.chords.clear();
    input.sections.clear();
    input.clef = gen::notation::ClefChoice::automatic;
    sources.clear();
    barPoints.clear();

    if (shownTrack != nullptr && shownTrack->isMidi())
    {
        // Copy what the score is written from under the lock, then build it outside it.
        const juce::ScopedLock lock (engine.getLock());

        for (auto& clip : shownTrack->clips)
            if (clip->kind == Clip::Kind::midi)
            {
                auto* midi = static_cast<MidiClip*> (clip.get());

                for (int i = 0; i < (int) midi->notes.size(); ++i)
                {
                    auto note = midi->notes[(size_t) i];
                    note.start += midi->startBeat;
                    input.notes.push_back (note);
                    sources.push_back ({ midi, i });
                }
            }

        input.key = engine.getKey();
        input.chords = engine.getChords();
        input.sections = engine.getSections();
        input.tempo = engine.getTempoMap();
        input.clef = (gen::notation::ClefChoice) (int) shownTrack->scoreClef;
    }

    if (! input.notes.empty())
    {
        score = gen::notation::build (input);
        line = gen::notation::layoutLines (score, 1.0e9, false).front();
        hasAnyNotes = true;

        for (int b = 0; b < score.numBars; ++b)
            barPoints.push_back (getBarPoints (b));
    }

    viewport.setVisible (hasAnyNotes);
    hitsDirty = true;
    updateCanvasSize();
    canvas->repaint();
    repaint();
}

std::vector<std::pair<double, double>> ScoreView::getBarPoints (int bar) const
{
    // Where each start in the bar sits: the middle of its note head. The bar's ends bound the ramp.
    std::map<int, double> at;

    for (auto& staff : score.staves)
        for (auto& e : staff.bars[(size_t) bar].events)
            at[e.start] = e.x + 0.59;

    if (at.find (0) == at.end())
        at[0] = 0.5;

    std::vector<std::pair<double, double>> points;

    for (auto& [start, x] : at)
        points.emplace_back (start / (double) gen::notation::unitsPerBeat, x);

    points.emplace_back (4.0, score.barWidths[(size_t) bar] - 0.1);
    return points;
}

float ScoreView::getBarX (int bar) const
{
    return getMargin() + (float) line.barOrigins[(size_t) juce::jlimit (0, score.numBars - 1, bar)] * space;
}

float ScoreView::beatToX (double beat) const
{
    if (! hasAnyNotes)
        return 0.0f;

    const int bar = juce::jlimit (0, score.numBars - 1, (int) std::floor (beat / 4.0));
    const double inBar = juce::jlimit (0.0, 4.0, beat - bar * 4.0);
    const auto& points = barPoints[(size_t) bar];
    double x = points.back().second;

    for (size_t i = 0; i + 1 < points.size(); ++i)
        if (inBar <= points[i + 1].first)
        {
            const double span = points[i + 1].first - points[i].first;
            x = points[i].second + (span > 0.0 ? (inBar - points[i].first) / span : 0.0) * (points[i + 1].second - points[i].second);
            break;
        }

    return getMargin() + (float) ((line.barOrigins[(size_t) bar] + x) * space);
}

double ScoreView::xToBeat (float x) const
{
    if (! hasAnyNotes)
        return 0.0;

    int bar = 0;

    for (int b = 0; b < score.numBars; ++b)
        if (x >= getBarX (b))
            bar = b;

    const double inSpaces = (x - getMargin()) / space - line.barOrigins[(size_t) bar];
    const auto& points = barPoints[(size_t) bar];

    if (inSpaces <= points.front().second)
        return bar * 4.0;

    for (size_t i = 0; i + 1 < points.size(); ++i)
        if (inSpaces <= points[i + 1].second)
        {
            const double span = points[i + 1].second - points[i].second;
            return bar * 4.0 + points[i].first + (span > 0.0 ? (inSpaces - points[i].second) / span : 0.0) * (points[i + 1].first - points[i].first);
        }

    return bar * 4.0 + 4.0;
}

void ScoreView::scrollToSelectedClip()
{
    auto* clip = selection.getMidiClip();

    if (clip == scrolledTo)
        return;

    scrolledTo = clip;

    if (clip == nullptr || ! hasAnyNotes)
        return;

    const int bar = juce::jlimit (0, score.numBars - 1, (int) std::floor (clip->startBeat / 4.0 + 1e-9));
    const int x = (int) (getBarX (bar) - getHeaderEnd() - space);
    viewport.setViewPosition (juce::jmax (0, x), viewport.getViewPositionY());
}

//==============================================================================
void ScoreView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    if (hasAnyNotes)
        return;

    auto area = getLocalBounds();
    const bool noTrack = shownTrack == nullptr || ! shownTrack->isMidi();
    const auto text = noTrack ? juce::String ("Select a MIDI track or clip to see its part written out.")
                              : juce::String ("This track has no notes yet. Draw some in the piano roll, then come back to Score.");

    // A treble clef above the text, in the font the score is drawn in.
    const float clefSpace = 7.0f;
    scorepaint::drawClef (g, gen::notation::Clef::treble, 0, (float) area.getCentreX() - 1.5f * clefSpace,
                          (float) area.getCentreY() - 7.0f * clefSpace * 0.5f - 16.0f, clefSpace, theme::textDim.withAlpha (0.5f));

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (text, area.withTrimmedTop (area.getHeight() / 2 + 6), juce::Justification::centredTop);
}

void ScoreView::paintScore (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    if (! hasAnyNotes)
        return;

    const auto ink = getInk();
    const auto clipBounds = g.getClipBounds();
    const float top = getTopY();
    const float headerWidth = getHeaderEnd();
    const float height = (float) canvas->getHeight();

    // The bars that show.
    int first = 0, last = score.numBars - 1;

    while (first < last && getBarX (first + 1) < (float) clipBounds.getX())
        ++first;

    while (last > first && getBarX (last) > (float) clipBounds.getRight())
        --last;

    // The selected clip's bars get a faint wash.
    if (auto* clip = selection.getMidiClip())
    {
        const int a = juce::jlimit (0, score.numBars - 1, (int) std::floor (clip->startBeat / 4.0 + 1e-9));
        const int b = juce::jlimit (0, score.numBars - 1, (int) std::ceil ((clip->startBeat + clip->lengthBeats) / 4.0 - 1e-9) - 1);
        const float x0 = getBarX (a);
        const float x1 = b + 1 < score.numBars ? getBarX (b + 1) : getMargin() + (float) line.width * space;
        g.setColour (theme::accent.withAlpha (0.09f));
        g.fillRect (x0, 0.0f, juce::jmax (0.0f, x1 - x0), height);
    }

    const double beat = engine.getPositionBeats();
    const bool playing = engine.isPlaying();
    const gen::notation::Score& scoreRef = score;
    const scorepaint::LitFunction lit = [&scoreRef, beat, playing] (int staff, int bar, int index)
    {
        if (! playing)
            return false;

        const auto& e = scoreRef.staves[(size_t) staff].bars[(size_t) bar].events[(size_t) index];
        const double start = bar * 4.0 + e.getStartBeat();
        return ! e.isRest && beat >= start && beat < start + e.getBeats();
    };

    scorepaint::Options options;
    options.header = false;
    options.chordSymbols = showChords;
    options.firstBar = first;
    options.lastBar = last;
    scorepaint::drawSystem (g, score, line, getMargin(), top, space, ink, options, nullptr, lit);

    const float playheadX = beatToX (beat);
    g.setColour (theme::playhead);
    g.fillRect (playheadX - 0.75f, 0.0f, 1.5f, height);

    // The clef and key signature stay at the left edge while the music scrolls under them.
    const float viewX = (float) viewport.getViewPositionX();
    g.setColour (ink.background);
    g.fillRect (viewX, 0.0f, headerWidth, height);

    auto headerLine = line;
    headerLine.numBars = 0;
    headerLine.barOrigins.clear();
    headerLine.width = line.headerWidth;
    scorepaint::Options headerOptions;
    headerOptions.barNumbers = headerOptions.chordSymbols = headerOptions.sectionsAndTempo = false;
    scorepaint::drawSystem (g, score, headerLine, viewX + getMargin(), top, space, ink, headerOptions);
}

//==============================================================================
void ScoreView::ensureHits()
{
    if (! hitsDirty || ! hasAnyNotes)
        return;

    hits.clear();
    headerHits.clear();

    // Draw once into a 1-pixel image, only to learn where everything is.
    juce::Image scratch (juce::Image::ARGB, 1, 1, false);
    juce::Graphics g (scratch);
    scorepaint::Options options;
    options.header = false;
    options.barNumbers = false;
    options.sectionsAndTempo = false;
    options.chordSymbols = showChords;
    scorepaint::drawSystem (g, score, line, getMargin(), getTopY(), space, getInk(), options, &hits);
    scorepaint::drawHeader (g, score, true, getMargin(), getTopY(), space, getInk(), &headerHits);
    hitsDirty = false;
}

const scorepaint::Hit* ScoreView::findHit (juce::Point<float> p, bool onlyNotesAndRests) const
{
    for (int pass = 0; pass < 3; ++pass)
        for (auto& h : hits)
        {
            const bool wanted = pass == 0 ? h.kind == scorepaint::HitKind::note
                              : pass == 1 ? h.kind == scorepaint::HitKind::rest
                                          : ! onlyNotesAndRests && (h.kind == scorepaint::HitKind::chordSymbol || h.kind == scorepaint::HitKind::tripletBracket);

            if (wanted && h.bounds.contains (p))
                return &h;
        }

    return nullptr;
}

const scorepaint::Hit* ScoreView::findHeaderHit (juce::Point<float> p) const
{
    for (auto& h : headerHits)
        if (h.bounds.contains (p))
            return &h;

    return nullptr;
}

juce::String ScoreView::describeNote (const gen::notation::WrittenPitch& p) const
{
    return formatPitch (p);
}

juce::String ScoreView::describeEvent (const gen::notation::Event& e, int staff, int bar) const
{
    const auto position = formatBeat (bar * 4.0 + e.getStartBeat());
    const auto what = scorepaint::describeEvent (e);

    if (e.isRest)
        return capitalise (what) + " \xc2\xb7 " + position;

    juce::StringArray names;

    for (auto& p : e.pitches)
        names.add (describeNote (p));

    juce::String text = names.joinIntoString (" + ") + " \xc2\xb7 " + what + " \xc2\xb7 " + position;

    if (e.pitches.size() == 1)
        if (auto it = input.chords.find (bar); it != input.chords.end())
        {
            const auto chordName = gen::sheet::describe (input.key, it->second).name;
            const bool in = it->second.getPitchClasses().contains (e.pitches.front().midi % 12);
            text << " \xc2\xb7 " << (in ? "in" : "outside") << " the chord " << chordName;
        }

    // A tie makes one sound of several written notes: add them up along the tie.
    if (e.tiedToNext || e.tiedFromPrevious)
    {
        const auto& bars = score.staves[(size_t) staff].bars;
        double beats = e.getBeats();
        int b = bar, i = -1;

        // Walk back to the start of the tie, then forward to its end.
        const auto previousOf = [&] (int& bb, int& ii) -> const gen::notation::Event*
        {
            if (ii > 0)
                return &bars[(size_t) bb].events[(size_t) --ii];

            if (bb > 0 && ! bars[(size_t) bb - 1].events.empty())
            {
                --bb;
                ii = (int) bars[(size_t) bb].events.size() - 1;
                return &bars[(size_t) bb].events[(size_t) ii];
            }

            return nullptr;
        };

        for (size_t k = 0; k < bars[(size_t) bar].events.size(); ++k)
            if (&bars[(size_t) bar].events[k] == &e)
                i = (int) k;

        int bb = b, ii = i;

        while (const auto* prev = previousOf (bb, ii))
        {
            if (! prev->tiedToNext)
                break;

            beats += prev->getBeats();
        }

        bb = b;
        ii = i;
        const gen::notation::Event* current = &e;

        while (current->tiedToNext)
        {
            if (ii + 1 < (int) bars[(size_t) bb].events.size())
                ++ii;
            else if (bb + 1 < (int) bars.size())
            {
                ++bb;
                ii = 0;
            }
            else
                break;

            current = &bars[(size_t) bb].events[(size_t) ii];
            beats += current->getBeats();
        }

        text << " \xc2\xb7 tied: one sound of " << formatBeats (beats) << (juce::approximatelyEqual (beats, 1.0) ? " beat" : " beats");
        return text;
    }

    // What was played, against what is written.
    if (e.sourceNotes.size() == 1 && juce::isPositiveAndBelow (e.sourceNotes.front(), (int) input.notes.size()))
    {
        const auto& played = input.notes[(size_t) e.sourceNotes.front()];
        const double late = played.start - (bar * 4.0 + e.getStartBeat());

        if (std::abs (late) < 0.005)
            text << " \xc2\xb7 played right on it, " << formatBeats (played.length) << " beats long";
        else
            text << " \xc2\xb7 played " << formatBeats (std::abs (late)) << " beats " << (late > 0 ? "late" : "early") << ", "
                 << formatBeats (played.length) << " beats long";
    }
    else if (e.sourceNotes.size() > 1)
    {
        text << " \xc2\xb7 " << (int) e.sourceNotes.size() << " notes played together";
    }

    return text;
}

juce::String ScoreView::getTooltipAt (juce::Point<float> p)
{
    if (! hasAnyNotes)
        return {};

    ensureHits();
    const float viewX = (float) viewport.getViewPositionX();

    if (p.x >= viewX && p.x < viewX + getHeaderEnd())
    {
        if (auto* h = findHeaderHit ({ p.x - viewX, p.y }))
        {
            switch (h->kind)
            {
                case scorepaint::HitKind::clef:
                    return score.staves[(size_t) h->staff].clef == gen::notation::Clef::treble
                               ? "Treble clef: its curl circles the line for G4, the G above middle C"
                               : "Bass clef: its two dots sit either side of the line for F3, the F below middle C";

                case scorepaint::HitKind::keySignature:
                {
                    juce::StringArray names;

                    for (auto& m : gen::notation::getKeySignature (score.keyPosition, score.staves[(size_t) h->staff].clef))
                        names.add (getKeyMarkName (m));

                    juce::String list = names.size() > 1 ? names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1]
                                                         : names.joinIntoString (", ");
                    return "Key signature of " + gen::sheet::getKeyName (input.key) + ": " + list + " in every octave";
                }

                case scorepaint::HitKind::timeSignature:
                    return "4/4: four quarter-note beats in a bar";

                case scorepaint::HitKind::note:
                case scorepaint::HitKind::rest:
                case scorepaint::HitKind::chordSymbol:
                case scorepaint::HitKind::tripletBracket:
                    break;
            }
        }

        return {};
    }

    if (auto* h = findHit (p, false))
    {
        switch (h->kind)
        {
            case scorepaint::HitKind::note:
            case scorepaint::HitKind::rest:
                return describeEvent (score.staves[(size_t) h->staff].bars[(size_t) h->bar].events[(size_t) h->event], h->staff, h->bar);

            case scorepaint::HitKind::chordSymbol:
                return h->text + ", from the chord track";

            case scorepaint::HitKind::tripletBracket:
                return "Triplet: three notes in the time of two";

            case scorepaint::HitKind::clef:
            case scorepaint::HitKind::keySignature:
            case scorepaint::HitKind::timeSignature:
                break;
        }
    }

    return {};
}

//==============================================================================
void ScoreView::handleClick (const juce::MouseEvent& e)
{
    if (! hasAnyNotes)
        return;

    ensureHits();
    const float viewX = (float) viewport.getViewPositionX();

    if (e.position.x < viewX + getHeaderEnd())
        return;

    if (auto* h = findHit (e.position, true); h != nullptr && h->kind == scorepaint::HitKind::note)
    {
        const auto& event = score.staves[(size_t) h->staff].bars[(size_t) h->bar].events[(size_t) h->event];
        float velocity = 0.8f;

        if (! event.sourceNotes.empty() && juce::isPositiveAndBelow (event.sourceNotes.front(), (int) input.notes.size()))
            velocity = input.notes[(size_t) event.sourceNotes.front()].velocity;

        for (auto& p : event.pitches)
            engine.previewNote (p.midi, velocity);

        return;
    }

    // Anywhere else: the playhead goes to that beat, to the nearest sixteenth.
    const double beat = std::round (juce::jmax (0.0, xToBeat (e.position.x)) * 4.0) / 4.0;
    engine.setPositionBeats (beat);
    canvas->repaint();
}

void ScoreView::handleDoubleClick (const juce::MouseEvent& e)
{
    if (! hasAnyNotes || ! onOpenPianoRoll)
        return;

    ensureHits();

    if (e.position.x < (float) viewport.getViewPositionX() + getHeaderEnd())
        return;

    // A clip must still be on the track: the song may have changed since the score was built.
    const auto stillThere = [this] (const Clip* clip)
    {
        return shownTrack != nullptr && std::any_of (shownTrack->clips.begin(), shownTrack->clips.end(),
                                                     [clip] (const auto& c) { return c.get() == clip; });
    };

    if (auto* h = findHit (e.position, true); h != nullptr && h->kind == scorepaint::HitKind::note)
    {
        const auto& event = score.staves[(size_t) h->staff].bars[(size_t) h->bar].events[(size_t) h->event];

        if (! event.sourceNotes.empty())
        {
            const auto& source = sources[(size_t) event.sourceNotes.front()];

            if (stillThere (source.clip))
            {
                onOpenPianoRoll (source.clip, source.note, h->bar * 4.0 + event.getStartBeat());
                return;
            }
        }
    }

    const double beat = juce::jmax (0.0, xToBeat (e.position.x));
    Clip* covering = nullptr;

    if (shownTrack != nullptr)
        for (auto& c : shownTrack->clips)
            if (c->kind == Clip::Kind::midi && beat >= c->startBeat && beat < c->startBeat + static_cast<MidiClip*> (c.get())->lengthBeats)
                covering = c.get();

    onOpenPianoRoll (covering, -1, beat);
}

//==============================================================================
void ScoreView::setClef (ScoreClef clef)
{
    if (shownTrack == nullptr || ! shownTrack->isMidi())
        return;

    const juce::ScopedLock lock (engine.getLock());
    engine.setScoreClef (*shownTrack, clef);
}

void ScoreView::addClefItems (juce::PopupMenu& menu)
{
    const auto current = shownTrack != nullptr ? shownTrack->scoreClef : ScoreClef::automatic;
    const std::pair<ScoreClef, const char*> items[] = {
        { ScoreClef::automatic, "Auto" },
        { ScoreClef::treble, "Treble" },
        { ScoreClef::bass, "Bass" },
        { ScoreClef::grand, "Grand staff" },
        { ScoreClef::guitar, "Guitar: treble, written an octave up" },
        { ScoreClef::bassGuitar, "Bass guitar: bass, written an octave up" } };

    for (auto& item : items)
        menu.addItem (item.second, shownTrack != nullptr && shownTrack->isMidi(), current == item.first,
                      [this, choice = item.first] { setClef (choice); });
}

void ScoreView::showClefMenu (juce::Component& target)
{
    juce::PopupMenu menu;
    addClefItems (menu);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target));
}

void ScoreView::showMenu()
{
    juce::PopupMenu menu, clefMenu;
    addClefItems (clefMenu);
    menu.addSubMenu ("Clef", clefMenu, shownTrack != nullptr && shownTrack->isMidi());

    menu.addItem ("Show chord symbols", true, showChords, [this]
    {
        showChords = ! showChords;
        engine.properties.getUserSettings()->setValue ("view.scoreChords", showChords);
        hitsDirty = true;
        canvas->repaint();
    });

    menu.addSeparator();
    menu.addItem ("Export Sheet Music (PDF)...", hasAnyNotes && onExportPdf != nullptr, false, [this] { if (onExportPdf) onExportPdf(); });
    menu.addItem ("Show in Piano Roll", hasAnyNotes && onOpenPianoRoll != nullptr, false, [this]
    {
        const double beat = engine.getPositionBeats();
        auto* clip = selection.getMidiClip();
        onOpenPianoRoll (clip, -1, clip != nullptr ? juce::jmax (beat, clip->startBeat) : beat);
    });

    menu.addSeparator();
    guideicon::addItem (menu, "How to read music", [this] { if (onShowGuide) onShowGuide(); });
    menu.showMenuAsync (juce::PopupMenu::Options());
}

//==============================================================================
void ScoreView::timerCallback()
{
    if (! hasAnyNotes)
        return;

    const bool playing = engine.isPlaying();
    const double beat = engine.getPositionBeats();

    if (! playing && ! wasPlaying && juce::approximatelyEqual (beat, lastPlayheadBeat))
        return;

    wasPlaying = playing;
    lastPlayheadBeat = beat;

    // Follow the playhead while playing: when it leaves the view, bring it back near the left.
    if (playing)
    {
        const float x = beatToX (beat);
        const int viewX = viewport.getViewPositionX(), viewWidth = viewport.getMaximumVisibleWidth();
        const float left = (float) viewX + getHeaderEnd();

        if (x < left || x > (float) (viewX + viewWidth) - 2.0f * space)
            viewport.setViewPosition (juce::jmax (0, (int) (x - getHeaderEnd() - 3.0f * space)), viewport.getViewPositionY());
    }

    canvas->repaint (viewport.getViewArea());
}
} // namespace daw
