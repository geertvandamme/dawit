#include "GuideButton.h"
#include "TrackLanes.h"
#include "SectionLane.h"
#include "BounceActions.h"
#include "MidiExportActions.h"
#include "RecordingActions.h"
#include "engine/Routing.h"

namespace daw
{
TrackLanes::TrackLanes (Engine& e, Selection& s, TimelineView& t)
    : engine (e), selection (s), timeline (t)
{
    setWantsKeyboardFocus (true);
}

//==============================================================================
juce::Rectangle<float> TrackLanes::getClipBounds (int row, const Clip& clip) const
{
    const float x = timeline.beatToX (clip.startBeat);
    const float w = juce::jmax (4.0f, (float) (clip.getLengthBeats (engine.getTempoMap()) * timeline.pixelsPerBeat));
    return { x, (float) (row * rowHeight + 4), w, (float) (rowHeight - 8) };
}

const std::vector<Track*>& TrackLanes::getRows() const
{
    rows = routing::getVisibleTracks (engine.getTracks());
    return rows;
}

int TrackLanes::getRowOf (const Track* track) const
{
    const auto& shown = getRows();
    const auto it = std::find (shown.begin(), shown.end(), track);
    return it != shown.end() ? (int) (it - shown.begin()) : -1;
}

Track* TrackLanes::trackAtY (float y) const
{
    const auto& shown = getRows();
    const int row = (int) std::floor (y / (float) rowHeight);
    return row >= 0 && row < (int) shown.size() ? shown[(size_t) row] : nullptr;
}

juce::String TrackLanes::getTooltip()
{
    const auto p = getMouseXYRelative().toFloat();
    auto* track = trackAtY (p.y);

    if (track != nullptr && ! track->hasInput())
    {
        // Over one of several takes of the same stretch: which one it is and how to choose.
        const auto hit = findClipAt (p);

        if (hit.clip != nullptr && hit.clip->kind == Clip::Kind::audio)
        {
            const auto group = takes::findOverlapping (track->clips, static_cast<const AudioClip&> (*hit.clip), engine.getTempoMap());

            if (group.size() > 1)
            {
                const int index = (int) (std::find (group.begin(), group.end(), static_cast<AudioClip*> (hit.clip)) - group.begin());
                return hit.clip->name + (hit.clip->muted ? " (muted)" : " (playing)") + juce::String::fromUTF8 (" \xc2\xb7 ")
                       + juce::String (index + 1) + " of " + juce::String ((int) group.size()) + " takes here: right-click > Takes to choose";
            }
        }

        return {};
    }

    if (track == nullptr)
        return {};

    const double beat = timeline.xToBeat (p.x);

    if (beat < 0.0)
        return {};

    juce::StringArray names;

    for (auto* t : getLaneTracks (*track))
    {

        for (auto& c : t->clips)
            if (beat >= c->startBeat && beat < c->getEndBeat (engine.getTempoMap()))
            {
                names.add (t->name);
                break;
            }
    }

    juce::String text = "Bar " + juce::String ((int) std::floor (beat / 4.0) + 1) + juce::String::fromUTF8 (" \xc2\xb7 ") + track->name;

    if (track->isReturn())
    {
        if (! names.isEmpty())
            text << juce::String::fromUTF8 (" \xc2\xb7 ") << names.joinIntoString (", ", 0, names.size() - 1) << (names.size() > 1 ? " and " : "")
                 << names[names.size() - 1] << (names.size() > 1 ? " send here" : " sends here");

        return text + "\nA return has no clips of its own: this shows the tracks that send to it";
    }

    if (! names.isEmpty())
        text << juce::String::fromUTF8 (" \xc2\xb7 ") << names.joinIntoString (", ", 0, names.size() - 1) << (names.size() > 1 ? " and " : "")
             << names[names.size() - 1] << (names.size() > 1 ? " play here" : " plays here");

    return text + "\nA bus has no clips of its own: this shows its tracks. Double-click to fold or unfold it";
}

TrackLanes::Hit TrackLanes::findClipAt (juce::Point<float> p) const
{
    auto* track = trackAtY (p.y);

    if (track == nullptr)
        return {};

    const int row = getRowOf (track);

    Hit found { track, nullptr, false };

    for (auto it = track->clips.rbegin(); it != track->clips.rend(); ++it)
    {
        const auto r = getClipBounds (row, **it);

        if (! r.contains (p))
            continue;

        const Hit hit { track, it->get(), (*it)->kind == Clip::Kind::midi && p.x > r.getRight() - 6.0f };

        // Muted takes are drawn under the playing one, so a click reaches the one on top.
        if (! isHiddenTake (*track, **it))
            return hit;

        if (found.clip == nullptr)
            found = hit;
    }

    return found;
}

bool TrackLanes::isHiddenTake (const Track& track, const Clip& clip) const
{
    return clip.muted && clip.kind == Clip::Kind::audio
           && takes::findOverlapping (track.clips, static_cast<const AudioClip&> (clip), engine.getTempoMap()).size() > 1;
}

double TrackLanes::snapStep() const
{
    return timeline.pixelsPerBeat < 10.0 ? 4.0 : 1.0;
}

double TrackLanes::snap (double beat, const juce::ModifierKeys& mods) const
{
    if (mods.isAltDown())
        return beat;

    const double step = snapStep();
    return std::round (beat / step) * step;
}

//==============================================================================
void TrackLanes::paint (juce::Graphics& g)
{
    const auto area = g.getClipBounds();
    const auto tracks = getRows();

    g.fillAll (theme::background);

    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        const auto row = juce::Rectangle<int> (0, i * rowHeight, getWidth(), rowHeight);
        g.setColour (i % 2 == 0 ? theme::lane : theme::laneAlt);
        g.fillRect (row);

        // The tracks in a bus sit on a faint tint of its colour, one for each level, so the group reads as a block.
        for (auto* b = tracks[(size_t) i]->bus; b != nullptr; b = b->bus)
        {
            g.setColour (b->colour.withAlpha (0.04f));
            g.fillRect (row);
        }

        if (selection.isSelected (tracks[(size_t) i]))
        {
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            g.fillRect (row);
        }

        if (i == dropRow)
        {
            g.setColour (theme::accent.withAlpha (0.15f));
            g.fillRect (row);
        }

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (0, row.getBottom() - 1, getWidth(), 1);
    }

    drawGrid (g, area);

    // Where each section starts, in its colour, so the song's structure shows behind the clips.
    for (auto& section : engine.getSections())
    {
        g.setColour (SectionLane::getColour (section.type).withAlpha (0.6f));
        g.fillRect (timeline.beatToX (section.startBar * 4.0), 0.0f, 1.5f, (float) getHeight());
    }

    {
        const float x1 = timeline.beatToX (engine.loopStartBeat.load());
        const float x2 = timeline.beatToX (engine.loopEndBeat.load());

        if (engine.loopEnabled.load())
        {
            g.setColour (theme::loop.withAlpha (0.07f));
            g.fillRect (x1, 0.0f, x2 - x1, (float) getHeight());
        }
    }

    // While copying, the original stays where it was; the dragged clip is the copy.
    if (copying && dragClip != nullptr)
    {
        if (const int row = getRowOf (dragOriginalTrack); row >= 0)
            drawClip (g, *dragOriginalTrack, *dragClip,
                      getClipBounds (row, *dragClip).withX (timeline.beatToX (dragOriginalStart)), false);
    }

    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        auto& track = *tracks[(size_t) i];

        if (track.hasInput())
        {
            drawBusSummary (g, track, i);
            continue;
        }

        // Muted takes first, so the one that plays is drawn on top of them.
        for (const bool hidden : { true, false })
            for (auto& clip : track.clips)
            {
                const auto r = getClipBounds (i, *clip);

                const bool onTop = ! hidden && ! isHiddenTake (track, *clip) && clip->kind == Clip::Kind::audio
                                   && takes::findOverlapping (track.clips, static_cast<const AudioClip&> (*clip), engine.getTempoMap()).size() > 1;

                // The playing take hides the muted ones under it (the clip is drawn see-through), so their names do not show through.
                if (onTop && r.intersects (area.toFloat()))
                {
                    g.setColour (i % 2 == 0 ? theme::lane : theme::laneAlt);
                    g.fillRoundedRectangle (r, 4.0f);
                }

                if (r.intersects (area.toFloat()) && isHiddenTake (track, *clip) == hidden)
                    drawClip (g, track, *clip, r, clip.get() == selection.getClip());
            }
    }

    drawLiveTakes (g);

    const int hintTop = (int) tracks.size() * rowHeight;

    if (dropRow >= (int) tracks.size())
    {
        g.setColour (theme::accent.withAlpha (0.15f));
        g.fillRect (0, hintTop, getWidth(), rowHeight);
    }

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (tracks.empty() ? "Add a track with \"+ Audio Track\" or \"+ MIDI Track\", or drop audio files here."
                               : "Double-click a MIDI lane to create a clip, or a clip to edit it.  Drop audio files on a lane to import.  "
                                 "Drag a clip out of the window to drop it into another app as a MIDI or WAV file.",
                juce::Rectangle<int> (12, hintTop + 8, getWidth() - 24, 24), juce::Justification::centredLeft);

    // Drawn where updatePlayhead() last put it: a partial repaint must not move the line, or the
    // old one is never erased.
    g.setColour (theme::playhead);
    g.fillRect (playheadX, 0.0f, 1.5f, (float) getHeight());
}

std::vector<const Track*> TrackLanes::getLaneTracks (const Track& bus) const
{
    std::vector<const Track*> result;

    // The tracks with clips that play into a bus, or send to a return (a bus that sends counts as the tracks in it).
    auto addLeaves = [&] (const Track& t, auto& self) -> void
    {
        if (! t.hasInput())
        {
            if (std::find (result.begin(), result.end(), &t) == result.end())
                result.push_back (&t);

            return;
        }

        for (auto& other : engine.getTracks())
            if (other->bus == &t)
                self (*other, self);
    };

    if (bus.isReturn())
    {
        for (auto* sender : routing::getSenders (bus, engine.getTracks()))
            addLeaves (*sender, addLeaves);
    }
    else
    {
        for (auto& t : engine.getTracks())
            if (! t->hasInput() && routing::playsInto (*t, bus))
                result.push_back (t.get());
    }

    return result;
}

void TrackLanes::drawBusSummary (juce::Graphics& g, const Track& bus, int row)
{
    // One line per track, up to 10 px tall, stacked in the row; the clips as thin bars in the tracks' colours.
    // A return shows the tracks that send to it.
    std::vector<const Track*> members;

    for (auto* t : getLaneTracks (bus))
        members.push_back (t);

    const auto area = juce::Rectangle<float> (0.0f, (float) (row * rowHeight + 6), (float) getWidth(), (float) (rowHeight - 12));

    if (members.empty())
    {
        g.setColour (theme::textDim.withAlpha (0.6f));
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (bus.isReturn() ? "Turn up the " + bus.name + " knob on a mixer strip to send a track here" : juce::String ("Drag tracks here to put them in this bus"),
                    area.withTrimmedLeft (12.0f), juce::Justification::centredLeft, true);
        return;
    }

    const float lineHeight = juce::jmin (10.0f, area.getHeight() / (float) members.size());
    const float barHeight = juce::jmax (1.0f, lineHeight - (lineHeight > 4.0f ? 2.0f : 0.5f));
    const float top = area.getY() + (area.getHeight() - lineHeight * (float) members.size()) * 0.5f;
    const auto visible = g.getClipBounds().toFloat();

    for (size_t i = 0; i < members.size(); ++i)
    {
        const float y = top + lineHeight * (float) i;

        for (auto& clip : members[i]->clips)
        {
            const float x = timeline.beatToX (clip->startBeat);
            const float w = juce::jmax (2.0f, (float) (clip->getLengthBeats (engine.getTempoMap()) * timeline.pixelsPerBeat) - 1.0f);

            if (x + w < visible.getX() || x > visible.getRight())
                continue;

            g.setColour ((clip->muted ? members[i]->colour.withSaturation (0.0f) : members[i]->colour).withAlpha (0.75f));
            g.fillRoundedRectangle (x, y, w, barHeight, juce::jmin (2.0f, barHeight * 0.5f));
        }
    }
}

void TrackLanes::drawGrid (juce::Graphics& g, juce::Rectangle<int> area)
{
    double step = 1.0;

    if (timeline.pixelsPerBeat < 14.0)
    {
        step = 4.0;
        while (step * timeline.pixelsPerBeat < 24.0)
            step *= 2.0;
    }

    const double firstBeat = std::floor (timeline.xToBeat (area.getX()) / step) * step;

    for (double beat = juce::jmax (0.0, firstBeat); timeline.beatToX (beat) <= (float) area.getRight(); beat += step)
    {
        const bool isBar = std::fmod (beat, 4.0) == 0.0;
        g.setColour (isBar ? theme::gridBar.withAlpha (0.55f) : theme::grid.withAlpha (0.6f));
        g.fillRect (timeline.beatToX (beat), (float) area.getY(), 1.0f, (float) area.getHeight());
    }
}

void TrackLanes::drawClip (juce::Graphics& g, const Track& track, const Clip& clip, juce::Rectangle<float> r, bool selected)
{
    // A muted clip is drawn in grey.
    const auto colour = clip.muted ? track.colour.withSaturation (0.0f).darker (0.3f) : track.colour;

    g.setColour (colour.withAlpha (0.28f));
    g.fillRoundedRectangle (r, 4.0f);

    auto header = r.withHeight (16.0f);
    g.setColour (colour.withAlpha (0.65f));
    g.fillRoundedRectangle (header, 4.0f);
    g.fillRect (header.withTrimmedTop (8.0f));

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    auto label = clip.muted ? clip.name + " (muted)" : clip.name;

    if (clip.kind == Clip::Kind::audio)
        if (const float gainDb = static_cast<const AudioClip&> (clip).gainDb; ! juce::exactlyEqual (gainDb, 0.0f))
            label << "  " << (gainDb > 0.0f ? "+" : "") << juce::String (gainDb, 1) << " dB";

    g.drawText (label, header.reduced (6.0f, 0.0f), juce::Justification::centredLeft, true);

    auto body = r.withTrimmedTop (17.0f).reduced (0.0f, 2.0f);
    const auto visible = body.getIntersection (g.getClipBounds().toFloat());

    if (clip.kind == Clip::Kind::midi)
    {
        auto& midiClip = static_cast<const MidiClip&> (clip);

        // With the track's scale filter on, notes outside the song key are left out, as in the piano roll.
        const auto& key = engine.getKey();
        const auto isShown = [&track, &key] (const MidiNote& n) { return ! track.scaleFilter || key.contains (n.pitch); };

        if (std::any_of (midiClip.notes.begin(), midiClip.notes.end(), isShown))
        {
            int lo = 127, hi = 0;
            for (auto& n : midiClip.notes)
            {
                if (isShown (n))
                {
                    lo = juce::jmin (lo, n.pitch);
                    hi = juce::jmax (hi, n.pitch);
                }
            }

            // Show at least an octave so single notes don't fill the whole clip.
            const int centre = (lo + hi) / 2;
            lo = juce::jmin (lo, centre - 6);
            hi = juce::jmax (hi, centre + 6);
            const float noteH = juce::jmax (1.5f, body.getHeight() / (float) (hi - lo + 1));

            g.setColour (colour.brighter (0.6f));

            for (auto& n : midiClip.notes)
            {
                if (n.start >= midiClip.lengthBeats || ! isShown (n))
                    continue;

                const float x = r.getX() + (float) (n.start * timeline.pixelsPerBeat);
                const float w = juce::jmax (1.5f, (float) (juce::jmin (n.length, midiClip.lengthBeats - n.start) * timeline.pixelsPerBeat));
                const float y = body.getBottom() - (float) (n.pitch - lo + 1) * noteH;
                g.fillRect (x, y, w, juce::jmax (1.5f, noteH - 0.5f));
            }
        }
    }
    else
    {
        auto& audioClip = static_cast<const AudioClip&> (clip);
        const int numBlocks = (int) audioClip.peakMax.size();

        if (numBlocks > 0 && ! visible.isEmpty())
        {
            // Each column's samples through the tempo map, so the waveform stretches with tempo changes.
            const auto& tempo = engine.getTempoMap();
            const double drawnStart = timeline.xToBeat (r.getX());
            const auto sampleAt = [&] (int x) { return tempo.getSecondsBetween (drawnStart, timeline.xToBeat ((float) x)) * audioClip.sourceSampleRate; };
            const float midY = body.getCentreY(), halfH = body.getHeight() * 0.5f, gain = audioClip.getGain();

            g.setColour (colour.brighter (0.5f).withAlpha (0.9f));

            for (int x = (int) visible.getX(); x < (int) visible.getRight(); ++x)
            {
                const double s0 = sampleAt (x);
                const int b0 = (int) (s0 / AudioClip::peakBlockSize);

                if (b0 >= numBlocks)
                    break;

                if (b0 < 0)
                    continue;

                const int b1 = juce::jlimit (b0 + 1, numBlocks, (int) (sampleAt (x + 1) / AudioClip::peakBlockSize));
                float lo = 0.0f, hi = 0.0f;

                for (int b = b0; b < b1; ++b)
                {
                    lo = juce::jmin (lo, audioClip.peakMin[(size_t) b]);
                    hi = juce::jmax (hi, audioClip.peakMax[(size_t) b]);
                }

                // Drawn at the clip's gain, so a louder clip looks bigger.
                lo *= gain;
                hi *= gain;
                g.drawVerticalLine (x, midY - juce::jmin (1.0f, hi) * halfH, midY - juce::jmax (-1.0f, lo) * halfH + 1.0f);
            }
        }
    }

    g.setColour (selected ? juce::Colours::white : colour.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, selected ? 2.0f : 1.0f);
}

void TrackLanes::drawLiveTakes (juce::Graphics& g)
{
    const auto& tempo = engine.getTempoMap();
    const double rate = engine.getSampleRate();

    for (auto& live : engine.getLiveTakes())
    {
        const int row = getRowOf (live.track);

        if (row < 0 || live.peaks == nullptr)
            continue;

        const double endBeat = tempo.getBeatAfter (live.startBeat, live.seconds);
        const float x = timeline.beatToX (live.startBeat);
        const juce::Rectangle<float> r (x, (float) (row * rowHeight + 4), juce::jmax (4.0f, timeline.beatToX (endBeat) - x), (float) (rowHeight - 8));

        // The next take number, as the clip will be named when recording stops.
        int number = 1;

        for (auto& c : live.track->clips)
            if (c->name.startsWith (live.track->name + " take "))
                number = juce::jmax (number, c->name.fromLastOccurrenceOf (" ", false, false).getIntValue() + 1);

        g.setColour (theme::meterRed.withAlpha (0.22f));
        g.fillRoundedRectangle (r, 4.0f);
        auto header = r.withHeight (16.0f);
        g.setColour (theme::meterRed.withAlpha (0.7f));
        g.fillRoundedRectangle (header, 4.0f);
        g.fillRect (header.withTrimmedTop (8.0f));
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (live.track->name + " take " + juce::String (number), header.reduced (6.0f, 0.0f), juce::Justification::centredLeft, true);

        // The waveform as it comes in: the loudest sample of each block of the take so far.
        const auto body = r.withTrimmedTop (17.0f).reduced (0.0f, 2.0f);
        const auto visible = body.getIntersection (g.getClipBounds().toFloat());
        const float midY = body.getCentreY(), halfH = body.getHeight() * 0.5f;
        g.setColour (juce::Colour (0xffffb0b0).withAlpha (0.9f));

        for (int px = (int) visible.getX(); px < (int) visible.getRight(); ++px)
        {
            const double s0 = tempo.getSecondsBetween (live.startBeat, timeline.xToBeat ((float) px)) * rate;
            const double s1 = tempo.getSecondsBetween (live.startBeat, timeline.xToBeat ((float) px + 1.0f)) * rate;
            const int b0 = (int) juce::jmax (0.0, s0 / Engine::LiveTake::peakBlockSize);
            const int b1 = juce::jmax (b0 + 1, (int) (s1 / Engine::LiveTake::peakBlockSize));
            float peak = 0.0f;

            for (int b = b0; b < b1 && b < (int) live.peaks->size(); ++b)
                peak = juce::jmax (peak, (*live.peaks)[(size_t) b]);

            if (peak > 0.0f)
                g.drawVerticalLine (px, midY - juce::jmin (1.0f, peak) * halfH, midY + juce::jmin (1.0f, peak) * halfH + 1.0f);
        }

        g.setColour (theme::meterRed);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    }
}

void TrackLanes::updateRecording()
{
    const auto live = engine.getLiveTakes();

    if (live.empty() && ! drewLiveTakes)
        return;

    drewLiveTakes = ! live.empty();
    repaint();
}

void TrackLanes::updatePlayhead()
{
    const float x = timeline.beatToX (engine.getPositionBeats());

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 6, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 6, getHeight());
}

//==============================================================================
void TrackLanes::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    drag = Drag::none;
    dragClip = nullptr;

    const auto hit = findClipAt (e.position);
    selection.select (hit.track, hit.clip);
    repaint();

    if (hit.clip == nullptr)
        return;

    if (e.mods.isPopupMenu())
    {
        showClipMenu (*hit.track, *hit.clip);
        return;
    }

    dragTrack = dragOriginalTrack = hit.track;
    dragClip = hit.clip;
    dragOriginalStart = hit.clip->startBeat;
    dragOriginalLength = hit.clip->getLengthBeats (engine.getTempoMap());
    dragMouseStartBeat = timeline.xToBeat (e.position.x);
    drag = hit.onRightEdge ? Drag::resize : Drag::move;
}

void TrackLanes::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::none || dragClip == nullptr)
        return;

    const double delta = timeline.xToBeat (e.position.x) - dragMouseStartBeat;

    // Dragging a clip out of the window hands it to other apps (Logic Pro, the Finder...) as a .mid or .wav file.
    if (drag == Drag::move && ! getTopLevelComponent()->getScreenBounds().contains (e.getScreenPosition()))
    {
        setCopying (false);
        {
            const juce::ScopedLock sl (engine.getLock());
            dragClip->startBeat = dragOriginalStart;
        }

        if (dragTrack != dragOriginalTrack && engine.indexOf (dragOriginalTrack) >= 0)
        {
            engine.moveClip (*dragTrack, dragClip, *dragOriginalTrack);
            dragTrack = dragOriginalTrack;
            selection.select (dragTrack, dragClip);
        }

        engine.notesChanged();
        repaint();

        auto* clip = dragClip;
        drag = Drag::none;
        dragClip = nullptr;

        if (clip->kind == Clip::Kind::midi)
            midiactions::dragClipOut (engine, *dragTrack, *static_cast<MidiClip*> (clip), *this);
        else
            bounceui::dragClipOut (engine, *dragTrack, *static_cast<AudioClip*> (clip), *this);
        return;
    }

    if (drag == Drag::move)
    {
        setCopying (e.mods.isCommandDown());
        const double newStart = juce::jmax (0.0, snap (dragOriginalStart + delta, e.mods));
        {
            const juce::ScopedLock sl (engine.getLock());
            dragClip->startBeat = newStart;
        }

        if (auto* target = trackAtY (e.position.y); target != nullptr && target != dragTrack && target->type == dragTrack->type)
        {
            engine.moveClip (*dragTrack, dragClip, *target);
            dragTrack = target;
            selection.select (target, dragClip);
        }
    }
    else
    {
        const double newEnd = snap (dragOriginalStart + dragOriginalLength + delta, e.mods);
        const juce::ScopedLock sl (engine.getLock());
        static_cast<MidiClip&> (*dragClip).lengthBeats = juce::jmax (snapStep(), newEnd - dragClip->startBeat);
    }

    engine.notesChanged();
    repaint();
}

void TrackLanes::mouseUp (const juce::MouseEvent& e)
{
    // Cmd-drag (Ctrl-drag on Windows): put the original back and drop a copy where the clip was dragged to.
    if (drag == Drag::move && dragClip != nullptr && e.mods.isCommandDown()
        && (! juce::exactlyEqual (dragClip->startBeat, dragOriginalStart) || dragTrack != dragOriginalTrack)
        && engine.indexOf (dragOriginalTrack) >= 0)
    {
        auto* copy = engine.duplicateClip (*dragTrack, *dragClip);
        {
            const juce::ScopedLock sl (engine.getLock());
            copy->startBeat = dragClip->startBeat;
            dragClip->startBeat = dragOriginalStart;
        }

        engine.moveClip (*dragTrack, dragClip, *dragOriginalTrack);
        selection.select (dragTrack, copy);
        engine.notesChanged();
    }

    drag = Drag::none;
    dragClip = nullptr;
    setCopying (false);
    repaint();
}

void TrackLanes::modifierKeysChanged (const juce::ModifierKeys& mods)
{
    if (drag == Drag::move && dragClip != nullptr)
        setCopying (mods.isCommandDown());
}

void TrackLanes::setCopying (bool shouldCopy)
{
    if (copying == shouldCopy)
        return;

    copying = shouldCopy;
    setMouseCursor (copying ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
    repaint();
}

void TrackLanes::mouseMove (const juce::MouseEvent& e)
{
    const auto hit = findClipAt (e.position);
    setMouseCursor (hit.onRightEdge ? juce::MouseCursor::LeftRightResizeCursor
                                    : hit.clip != nullptr ? juce::MouseCursor::DraggingHandCursor
                                                          : juce::MouseCursor::NormalCursor);
}

void TrackLanes::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto hit = findClipAt (e.position);

    if (hit.track == nullptr)
        return;

    if (hit.clip != nullptr)
    {
        // The piano roll for a MIDI clip, the wave editor for an audio clip.
        selection.select (hit.track, hit.clip);

        if (onOpenClipEditor)
            onOpenClipEditor();

        return;
    }

    const double beat = juce::jmax (0.0, std::floor (timeline.xToBeat (e.position.x) / 4.0) * 4.0);

    if (hit.track->isBus())
    {
        engine.setFolded (*hit.track, ! hit.track->folded);
    }
    else if (hit.track->isMidi())
    {
        auto* clip = engine.addMidiClip (*hit.track, beat, 16.0);
        selection.select (hit.track, clip);
        repaint();

        if (onOpenClipEditor)
            onOpenClipEditor();
    }
    else if (onImportRequest && hit.track->isAudio())
    {
        onImportRequest (*hit.track, beat);
    }
}

void TrackLanes::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        if (onZoom && wheel.deltaY != 0.0f)
            onZoom (wheel.deltaY > 0 ? 1.12 : 1.0 / 1.12, timeline.xToBeat (e.position.x));

        return;
    }

    if (std::abs (wheel.deltaX) > std::abs (wheel.deltaY) || e.mods.isShiftDown())
    {
        const float delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;

        if (onScroll)
            onScroll (-delta * 400.0 / timeline.pixelsPerBeat);

        return;
    }

    juce::Component::mouseWheelMove (e, wheel); // vertical: let the viewport scroll
}

bool TrackLanes::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedClip();
        return true;
    }

    if (key == juce::KeyPress ('d', juce::ModifierKeys::commandModifier, 0))
    {
        duplicateSelectedClip();
        return true;
    }

    if (key == juce::KeyPress ('m'))
    {
        toggleSelectedClipMute();
        return true;
    }

    return false;
}

void TrackLanes::deleteSelectedClip()
{
    auto* track = selection.getTrack();
    auto* clip = selection.getClip();

    if (track == nullptr || clip == nullptr)
        return;

    selection.select (track, nullptr);
    engine.removeClip (*track, clip);
    repaint();
}

void TrackLanes::toggleSelectedClipMute()
{
    if (auto* clip = selection.getClip())
    {
        engine.setClipMuted (*clip, ! clip->muted);
        repaint();
    }
}

void TrackLanes::duplicateSelectedClip()
{
    auto* track = selection.getTrack();
    auto* clip = selection.getClip();

    if (track == nullptr || clip == nullptr)
        return;

    selection.select (track, engine.duplicateClip (*track, *clip));
    repaint();
}

void TrackLanes::showClipMenu (Track& track, Clip& clip)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Duplicate");
    menu.addItem (2, "Delete");
    menu.addItem (13, clip.muted ? "Unmute clip" : "Mute clip");

    const bool editable = ! engine.isRecording();
    menu.addSeparator();
    menu.addItem (7, "Split at playhead", editable && engine.canSplit (clip, engine.getPositionBeats()));
    menu.addItem (8, "Join with previous clip", editable && Engine::findNeighbourClip (track, clip, false) != nullptr);
    menu.addItem (9, "Join with next clip", editable && Engine::findNeighbourClip (track, clip, true) != nullptr);

    if (clip.kind == Clip::Kind::audio)
    {
        menu.addSeparator();
        menu.addItem (10, "Reverse", editable);
        menu.addItem (20, "Bounce clip in place", editable && ! clip.muted && onBounceClip != nullptr);
        menu.addItem (21, "Export clip as WAV file...");
        menu.addItem (11, "Split into stems...", editable && onSplitIntoStems != nullptr);
        menu.addItem (18, "Remove vocals (karaoke version)...", editable && onRemoveVocals != nullptr);
        menu.addItem (12, "Convert to MIDI...", editable && onConvertToMidi != nullptr);
        menu.addItem (22, "Hits to MIDI...", editable && onHitsToMidi != nullptr);
        menu.addItem (15, "Detect tempo...", editable && onDetectTempo != nullptr);
        menu.addItem (16, "Detect chords...", editable && onDetectChords != nullptr);
        menu.addItem (17, "Detect song structure...", editable && onDetectStructure != nullptr);
        menu.addItem (14, "Detect lyrics (English only)...", editable && onDetectLyrics != nullptr);

        // Several takes of the same stretch: choose the one that plays.
        const auto group = takes::findOverlapping (track.clips, static_cast<const AudioClip&> (clip), engine.getTempoMap());

        if (group.size() > 1)
        {
            menu.addSeparator();
            juce::PopupMenu choose;

            for (size_t i = 0; i < group.size(); ++i)
                choose.addItem (100 + (int) i, "Take " + juce::String ((int) i + 1) + (group[i]->muted ? "" : " (playing)"), editable, ! group[i]->muted);

            menu.addSubMenu ("Takes", choose);
            menu.addItem (30, "Delete other takes", editable);
        }

        menu.addSeparator();
        guideicon::addItem (menu, 31, "About recording audio...", onShowRecordingGuide != nullptr);
        guideicon::addItem (menu, 32, "About drum triggers...", onShowTriggerGuide != nullptr);
    }

    if (clip.kind == Clip::Kind::midi)
    {
        auto& midiClip = static_cast<MidiClip&> (clip);
        menu.addSeparator();
        menu.addItem (5, "Transpose octave up", Engine::canTranspose (midiClip, 12));
        menu.addItem (6, "Transpose octave down", Engine::canTranspose (midiClip, -12));
        menu.addItem (19, "Sing this melody...", ! midiClip.notes.empty() && onSingThis != nullptr);
        menu.addSeparator();
        menu.addItem (20, "Bounce clip in place", editable && ! clip.muted && onBounceClip != nullptr);
        menu.addSeparator();
        menu.addItem (3, "Export clip as MIDI file...");
        menu.addItem (4, "Export track as MIDI file...");
    }

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, &track, &clip] (int result)
    {
        // The clip may have gone in the meantime; make sure it is still the selected one.
        if (selection.getClip() != &clip || selection.getTrack() != &track)
            return;

        if (result == 1)
            duplicateSelectedClip();
        else if (result == 2)
            deleteSelectedClip();
        else if (result == 13)
            toggleSelectedClipMute();
        else if (result == 31 && onShowRecordingGuide != nullptr)
            onShowRecordingGuide();
        else if (result == 32 && onShowTriggerGuide != nullptr)
            onShowTriggerGuide();
        else if (result == 30)
        {
            // The clip stays; the others go, and it plays.
            const auto doomed = takes::findOthers (track.clips, static_cast<AudioClip&> (clip), engine.getTempoMap());

            for (auto* other : doomed)
                selection.forget (&track, other);

            engine.deleteOtherTakes (track, static_cast<AudioClip&> (clip));
        }
        else if (result >= 100 && result < 200)
        {
            const auto group = takes::findOverlapping (track.clips, static_cast<AudioClip&> (clip), engine.getTempoMap());

            if (const size_t index = (size_t) (result - 100); index < group.size())
            {
                engine.chooseTake (track, *group[index]);
                selection.select (&track, group[index]);
            }
        }
        else if (result == 3)
            midiactions::exportClip (engine, track, static_cast<MidiClip&> (clip));
        else if (result == 4)
            midiactions::exportTrack (engine, track);
        else if (result == 20)
            onBounceClip (track, clip);
        else if (result == 21)
            bounceui::exportClipAsWav (engine, track, static_cast<AudioClip&> (clip));
        else if (result == 19)
            onSingThis (track, static_cast<MidiClip&> (clip));
        else if (result == 5 || result == 6)
            engine.transposeClip (static_cast<MidiClip&> (clip), result == 5 ? 12 : -12);
        else if (result == 10)
            engine.reverseClip (static_cast<AudioClip&> (clip));
        else if (result == 11)
            onSplitIntoStems (track, static_cast<AudioClip&> (clip));
        else if (result == 18)
            onRemoveVocals (track, static_cast<AudioClip&> (clip));
        else if (result == 12)
            onConvertToMidi (track, static_cast<AudioClip&> (clip));
        else if (result == 22)
            onHitsToMidi (track, static_cast<AudioClip&> (clip));
        else if (result == 14)
            onDetectLyrics (track, static_cast<AudioClip&> (clip));
        else if (result == 15)
            onDetectTempo (track, static_cast<AudioClip&> (clip));
        else if (result == 16)
            onDetectChords (track, static_cast<AudioClip&> (clip));
        else if (result == 17)
            onDetectStructure (track, static_cast<AudioClip&> (clip));
        else if (result == 7)
            engine.splitClip (track, clip, engine.getPositionBeats());
        else if (result == 8 || result == 9)
        {
            if (auto* other = Engine::findNeighbourClip (track, clip, result == 9))
            {
                // The earlier clip stays; the other one goes.
                auto* gone = other->startBeat < clip.startBeat ? static_cast<Clip*> (&clip) : other;
                selection.forget (&track, gone);
                selection.select (&track, engine.joinClips (track, clip, *other));
            }
        }

        repaint();
    });
}

//==============================================================================
void TrackLanes::importAudioFiles (const juce::Array<juce::File>& files, Track& track, double startBeat)
{
    juce::StringArray errors;
    Clip* last = nullptr;
    double beat = startBeat;

    for (auto& file : files)
    {
        juce::String error;

        if (auto* clip = engine.addAudioClip (track, file, beat, error))
        {
            beat = clip->getEndBeat (engine.getTempoMap());
            last = clip;
        }
        else
        {
            errors.add (error);
        }
    }

    if (last != nullptr)
        selection.select (&track, last);

    if (! errors.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Import failed",
                                                errors.joinIntoString ("\n"));

    repaint();
}

bool TrackLanes::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (engine.audioFormats.findFormatForFileExtension (juce::File (f).getFileExtension()) != nullptr)
            return true;

    return false;
}

void TrackLanes::fileDragMove (const juce::StringArray&, int, int y)
{
    auto* track = trackAtY ((float) y);
    const int row = track != nullptr && ! track->isMidi() ? getRowOf (track) : (int) getRows().size();

    if (row != dropRow)
    {
        dropRow = row;
        repaint();
    }
}

void TrackLanes::fileDragExit (const juce::StringArray&)
{
    dropRow = -1;
    repaint();
}

void TrackLanes::filesDropped (const juce::StringArray& paths, int x, int y)
{
    dropRow = -1;

    juce::Array<juce::File> files;
    for (auto& p : paths)
        if (engine.audioFormats.findFormatForFileExtension (juce::File (p).getFileExtension()) != nullptr)
            files.add (juce::File (p));

    auto* track = trackAtY ((float) y);

    if (track != nullptr && track->isReturn())
        return;   // a return has no clips: it only plays what is sent to it

    if (track != nullptr && track->isBus())
    {
        // Audio dropped on a bus becomes a new audio track inside it.
        auto* bus = track;
        track = engine.addTrack (Track::Type::audio);
        engine.moveTrack (*track, bus);
    }
    else if (track == nullptr || track->isMidi())
    {
        track = engine.addTrack (Track::Type::audio);
    }

    importAudioFiles (files, *track, juce::jmax (0.0, snap (timeline.xToBeat (x), {})));
}
} // namespace daw
