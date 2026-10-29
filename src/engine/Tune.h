#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>
#include <vector>

/**
    The rules of DawIt Tune with no audio in them, so the processor, the guide's pictures and the
    tests all use the same numbers: cents, which note a sung note is pulled to, how fast, how far,
    the middle of a note with vibrato, and made-up sung takes to try it on.
*/
namespace daw::tune
{
//==============================================================================
// Cents and zones

/** A semitone is 100 cents; an octave is 1,200. The number of cents from frequency `f1` up to `f2`. */
inline double getCents (double f2, double f1) noexcept { return 1200.0 * std::log2 (f2 / f1); }

/** The frequency `cents` above `hz`. */
inline double shiftHz (double hz, double cents) noexcept { return hz * std::pow (2.0, cents / 1200.0); }

/** The frequency of a MIDI note (69 = A4 = 440 Hz); fractional notes are fine. */
inline double getHz (double midi) noexcept { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }

/** The factor that moves a frequency `cents` up: 1.059463 for 100 cents. */
inline double getRatio (double cents) noexcept { return std::pow (2.0, cents / 1200.0); }

/** DawIt's bands, from how far a note is from a note: in tune to 10 cents, close to 25, out beyond. */
constexpr double inTuneCents = 10.0, closeCents = 25.0;

enum class Zone { inTune, close, out };

/** The zone of a distance in cents (the sign doesn't matter). */
Zone getZone (double cents) noexcept;

/** "In tune", "Close" or "Out". */
juce::String getZoneName (Zone);

//==============================================================================
// Targets

/** The pitch classes of a scale or chord as bits (bit 0 = C). */
constexpr int allNotes = 0xfff;

/** The note a sung pitch (fractional MIDI) is pulled to: the nearest note whose pitch class is in `mask`,
    ties to the lower one. `previous` (-1 for none) is kept until the pitch is 15 cents past the halfway
    point to another note, so a note that hovers there doesn't flip. */
int chooseTarget (double midi, int mask, int previous);

/** Nothing is touched when the error is within the tolerance; beyond it the whole error counts. */
double applyTolerance (double errorCents, double toleranceCents) noexcept;

/** The part of the error that is removed: 60% of 30 cents is 18. */
double applyAmount (double errorCents, double amountPercent) noexcept;

//==============================================================================
// Moving

/** One pole towards a target. `speedMs` is the time to get 95% of the way, so the time constant is a third of it;
    0 jumps at once. */
class Glide
{
public:
    void reset (double value = 0.0) noexcept { current = value; }
    double advance (double target, double seconds, double speedMs) noexcept;
    double get() const noexcept { return current; }

private:
    double current = 0.0;
};

/** The middle of a note with vibrato: a 2-pole Butterworth low-pass at 1.5 Hz on the pitch, run once per frame. */
class CentreFilter
{
public:
    static constexpr double cutoffHz = 1.5;

    void prepare (double framesPerSecond);

    /** Starts afresh as if the pitch had always been `value`. */
    void restart (double value) noexcept;
    double process (double value) noexcept;

private:
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

//==============================================================================
// Settings

enum class Mode { check, correct };
enum class Target { key, chords, chromatic };

/** The controls of DawIt Tune. */
struct Settings
{
    Mode mode = Mode::check;
    Target target = Target::key;
    float speedMs = 50.0f;
    float amountPercent = 100.0f;
    float toleranceCents = 10.0f;
    bool keepVibrato = true;
};

/** A starting point and why. */
struct Preset
{
    juce::String name, why;
    Settings settings;
};

const std::vector<Preset>& getPresets();

/** The notes a target allows: the key's, the chord's (the key's in a bar without a chord, where `chordMask` is 0), or all twelve. */
int getTargetMask (Target, int keyMask, int chordMask) noexcept;

/** What the correction made of one frame. */
struct Step
{
    bool voiced = false;
    double correctionCents = 0.0;   // how far the note is moved: the pitch plus this is what is heard
    double centreMidi = 0.0;        // the middle of the note (the pitch itself without Keep vibrato)
    int target = -1;
};

/** The correction, one frame of pitch at a time. */
class Corrector
{
public:
    /** `framesPerSecond`: how often step() is called (172 in the effect, 200 in the pictures). */
    void prepare (double framesPerSecond);
    void reset() noexcept;

    /** One frame: the sung pitch (fractional MIDI, below 0 for no clear pitch), the settings and the notes allowed. */
    Step step (double midi, const Settings&, int mask) noexcept;

private:
    double fps = 200.0;
    int gapFrames = 0, maxGapFrames = 5;
    bool active = false;
    double lastMidi = 0.0;
    int previousTarget = -1;
    CentreFilter filter;
    Glide glide;
};

/** The corrected pitch of a line of frames (fractional MIDI, below 0 for a gap), as the effect would make it. */
std::vector<float> simulate (const std::vector<float>& sungMidi, const Settings&, int mask, double framesPerSecond);

//==============================================================================
// What was heard

/** What the tracker heard at one moment (every few milliseconds), for the picture and for "Your track". */
struct Frame
{
    float beat = -1.0f;        // where in the song, or -1 while the song isn't playing
    float sung = -1.0f;        // the pitch as a fractional MIDI note, or -1 for no clear pitch
    float corrected = -1.0f;   // the pitch after the correction (the same as sung in Check mode)
    float rmsDb = -100.0f;     // how loud the sound was
};

//==============================================================================
// Made-up takes

/** One sung note: a MIDI pitch, when it starts and ends, and how far from it the singer is over time. */
struct TakeNote
{
    double midi = 69.0, start = 0.0, end = 1.0;
    std::function<double (double)> cents;   // cents off midi, at the seconds since the note began; flat when empty
    double level = 1.0;
};

/** A mono "ah" sung through the notes, one after another, for `seconds`. */
juce::AudioBuffer<float> makeTake (const std::vector<TakeNote>&, double sampleRate, double seconds);

/** The pitch of the notes at every frame (fractional MIDI, -1 between notes), for the pictures. */
std::vector<float> makeFrames (const std::vector<TakeNote>&, double seconds, double frameSeconds = 0.005);

/** The off-note test sound: A3 starting 150 cents low, sliding up to 30 cents flat and holding there with a gentle vibrato. */
TakeNote getOffNote();

/** The notes of the guide's pictures (see the plan: example phrase, scoop, vibrato, hard tune). */
std::vector<TakeNote> getExamplePhraseNotes();
std::vector<TakeNote> getScoopNotes();
std::vector<TakeNote> getVibratoNotes();
std::vector<TakeNote> getHardTuneNotes();

constexpr double exampleFrameSeconds = 0.005;
}
