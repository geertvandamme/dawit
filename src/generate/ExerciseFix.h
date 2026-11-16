#pragma once

namespace daw::gen::exercises
{
/** What the way forward is when an exercise step can't be done yet (the action button turns into it), and what a step's
    own action is where it has a name of its own (`detectTempo`, `detectSections`, `detectLyrics`, and `addRecordingTrack` for the karaoke exercise). `none` is the step's own action.
    Shared by the exercises; it lives in its own header so `gen::songchords` can use it without including the model. */
enum class Fix { none, importAudio, detectTempo, detectChords, detectSections, audioSettings, openSections, openLyrics,
                 detectLyrics, addRecordingTrack };
} // namespace daw::gen::exercises
