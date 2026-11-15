#pragma once

#include "MeterGuide.h"
#include "engine/Engine.h"

namespace daw::loudnessui
{
/** File > Measure Loudness...: renders the loop (when it is on) or the whole song offline, without playing it, behind a
    progress window with Cancel, and reads its loudness the way DawIt Meter does. The result is kept for the guide's
    *Your track* page, and `onFinished` is called on the message thread with it (not after a cancel or a failure).
    With `wholeSong` true it measures the whole song even when the loop is on (the mix and export exercise). */
void measureSong (Engine&, std::function<void (const metertext::SongResult&)> onFinished = {}, bool wholeSong = false);

/** The range the last measurement covered: the loop or the whole song, in beats. */
struct MeasuredRange
{
    bool wholeSong = true;
    double startBeat = 0.0, endBeat = 0.0;
};

/** The range of the last measurement; empty when none was made yet. */
std::optional<MeasuredRange> getLastRange();

/** The same, here and now, without a progress window: for screenshot scenes. Empty for an empty song, while recording or on a failure. */
std::optional<metertext::SongResult> measureSongNow (Engine&);

/** The last measurement, or none yet. */
std::optional<metertext::SongResult> getLast();

/** Forgets the last measurement (a different project is a different song). */
void clearLast();
} // namespace daw::loudnessui
