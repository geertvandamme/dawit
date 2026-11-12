#pragma once

#include "engine/Engine.h"

#include <utility>
#include <vector>

namespace daw::gen::staging
{
/** Chords a take proposes for the chord track, split by what Apply would do: bars that are empty get the chord, bars that
    already have a chord keep it. Bars are song bars (the first bar is 0). */
struct ChordProposal
{
    std::vector<std::pair<int, Chord>> added;
    std::vector<int> kept;
};

/** The take's chords, one per bar of the clip from `firstBar` on, against the chord track: only empty bars are ever filled. */
ChordProposal splitChordProposal (const ChordMap& existing, int firstBar, const std::vector<Chord>& proposed);

/** The notes a clip would hold after Apply: `result` alone (replace), or the clip's own followed by `result` (add). Notes
    starting at or after the clip's end are dropped and one that crosses it is shortened to end there. Beats relative to the clip. */
std::vector<MidiNote> fitNotes (const std::vector<MidiNote>& clipNotes, const std::vector<MidiNote>& result,
                                double clipLengthBeats, bool replace);

/**
    What a take was made for: the clip and everything in the song its notes depend on. A take made for one target must not
    reach another, so Apply (and the preview) check it first. UI-free; never saved.
*/
struct Target
{
    const MidiClip* clip = nullptr;
    int editGeneration = 0;                  // changes when a new project is started or loaded, or the time signature changes
    double startBeat = 0.0, lengthBeats = 0.0;
    std::vector<MidiNote> notes;
    ChordMap chordsInBars;                   // the chords of the song bars the clip covers
    MusicalKey key;
    TimeSignature timeSignature;
    int firstBar = 0, numBars = 0;           // song bars the clip covers

    /** Takes the target's state from the engine now. */
    static Target capture (const Engine&, const MidiClip&);

    /** False when the clip is gone, or its notes, start, length, chords in its bars, the key, the time signature or the
        project changed since capture. A tempo change, or a chord outside the clip's bars, doesn't count. */
    bool isValid (const Engine&) const;
};

/** What a take is: notes (already fitted to the clip) and, for a request that proposes them, one chord per bar of the clip. */
struct Take
{
    std::vector<MidiNote> notes;
    std::vector<Chord> chords;
};

/**
    The life of one request and the take it makes: begin, deliver (or cancel), then discard or end. Only the newest request's
    result is accepted, a cancelled request stages nothing, and a staged take that no longer matches its target is dropped.
    Never changes the song and never touches the engine's preview itself; the panel shows the take with `Engine::setClipPreview`.
*/
class Staging
{
public:
    enum class Outcome { staged, oldRequest, cancelled, stale };

    /** Starts a request for `clip`, forgetting any staged take, and returns its id (the next one each time). */
    int begin (const Engine&, const MidiClip&);

    /** Stops the running request: its result, even takes that already arrived, is never staged. */
    void cancel();

    /** A request's result arrives. `replace` says whether it replaces the clip's notes or adds to them; the notes are fitted
        to the clip's length. Rejected when the id isn't the newest, the request was cancelled, or the target changed. */
    Outcome deliver (const Engine&, int requestId, const std::vector<MidiNote>& notes, const std::vector<Chord>& chords, bool replace);

    /** Changes a staged take between replace and add (the clip's own notes come from the target). Needs a staged take. */
    void setReplace (bool replace);

    bool isStaged() const noexcept { return staged; }
    bool isReplacing() const noexcept { return replacing; }
    int getRequestId() const noexcept { return requestId; }
    const Take& getTake() const noexcept { return take; }
    const Target& getTarget() const noexcept { return target; }

    /** The staged take's notes as they would be after Apply: `getTake().notes` fitted (replace) or the clip's own plus them (add). */
    std::vector<MidiNote> getNotesAfterApply() const;

    /** True when a take is staged and its target changed: the caller discards it. */
    bool isStale (const Engine&) const;

    /** Removes the staged take; the song was never changed. */
    void discard();

private:
    Target target;
    Take take;
    std::vector<MidiNote> rawNotes;
    bool staged = false, replacing = true, cancelled = false;
    int requestId = 0;
};
} // namespace daw::gen::staging
