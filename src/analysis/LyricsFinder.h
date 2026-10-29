#pragma once

#include "engine/Lyrics.h"
#include "engine/ModelDownload.h"
#include "engine/TempoMap.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <optional>

struct whisper_context;

namespace daw::analysis
{
/**
    Finding the sung words in a recording and putting them on the lyrics track, timed to the
    singing. English vocals only for now.

    The steps:
    1. The vocals: the clip itself when it's a vocal track, otherwise its vocal stem from Demucs
       (see stems::Separator), which keeps the band from being heard as words.
    2. Where someone sings, from the level of the vocals. The rest is muted, because Whisper makes
       up words in long quiet or instrumental parts.
    3. The words, with when each starts and ends, from OpenAI's Whisper (through whisper.cpp) at
       16 kHz, with its English-only models.
    4. Clean-up: words outside the singing, sounds in brackets ("[Music]") and phrases Whisper
       is known to make up ("Thanks for watching") go.
    5. Word starts snapped to the nearest onset of the singing (onsets::findOnsets), when that is
       within 0.15 s, and the onsets inside a word kept for its syllables.
    6. Lines at breaths and pauses and where a new section starts, a verse per section with the
       section's name as its heading, and each word split into sung syllables by English spelling
       rules (lyrics::splitEnglishWord), its time shared out over them.
    7. Each syllable becomes a pinned LyricSyllable at its beat, so the lyrics track shows the
       real timing and the text editor and the guide work on it as on typed lyrics.

    English only: the models are Whisper's English ones (".en"), and the syllables follow English
    spelling. Vocals in another language come out as English words that sound alike.
*/
namespace lyricsfinder
{
    /** Whisper listens to mono audio at this rate. */
    constexpr double sampleRate = 16000.0;

    /** The English Whisper models to choose from: small is quicker, medium hears more. */
    enum class ModelSize { small, medium };

    const models::Model& getModel (ModelSize);

    /** "Whisper small (English)". */
    juce::String getModelName (ModelSize);

    /** Reports progress from 0 to 1; returning false cancels. */
    using ProgressFn = std::function<bool (double)>;

    /** A word Whisper heard, with its time in seconds from the start of the audio. */
    struct Word
    {
        Word() = default;
        Word (juce::String t, double s, double e, int seg, std::vector<double> o = {})
            : text (std::move (t)), start (s), end (e), segment (seg), onsets (std::move (o)) {}

        juce::String text;       // as Whisper wrote it, with punctuation: "night,"
        double start = 0.0, end = 0.0;
        int segment = 0;         // Whisper's phrase it came in; a hint for where lines break
        std::vector<double> onsets;   // where the singing has an onset inside the word (set by snapToOnsets), for its syllables
    };

    //==============================================================================
    /** The audio as mono at 16 kHz. */
    juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>&, double sourceSampleRate);

    /** Where someone sings, in seconds: where the level of the vocals is within 30 dB of its
        loudest (and above -50 dBFS), with breaths shorter than 0.25 s bridged and blips shorter
        than 0.1 s left out. */
    std::vector<juce::Range<double>> findSinging (const juce::AudioBuffer<float>& mono, double rate);

    /** Silences the audio outside the singing, with short fades. */
    void muteOutside (juce::AudioBuffer<float>& mono, double rate, const std::vector<juce::Range<double>>& singing);

    /** True for a phrase Whisper is known to make up from silence or music ("Thanks for watching",
        "Subtitles by ..."). */
    bool isMadeUpPhrase (const juce::String& text);

    /** Keeps the sung words: drops sounds in brackets, signs without letters (♪), made-up phrases
        and words mostly outside the singing, and puts the rest in order without overlaps. */
    std::vector<Word> cleanUp (std::vector<Word>, const std::vector<juce::Range<double>>& singing);

    /** The furthest a word's start is moved to meet an onset of the singing, in seconds. Whisper's word
        times are typically off by up to about 0.1 to 0.2 s on singing; much more and the nearest onset
        is more likely another word's, and at 120 BPM an eighth note is only 0.25 s. */
    constexpr double longestSnap = 0.15;

    /** Moves each word's start to the nearest onset of the singing (see onsets::findOnsets) within
        `longestSnap`, so the words begin where the voice does. A word stays where it is when no onset
        is that close, when the onset is nearer the next word, or when moving would take it past
        its neighbours (order is kept, 20 ms between starts, at least 50 ms long). A word's end moves
        only as far as needed to stay clear of the next start. Onsets inside a word (at least 40 ms
        from its edges) are kept in `Word::onsets` for its syllables. */
    void snapToOnsets (std::vector<Word>& words, const std::vector<double>& onsets);

    /** The times, in seconds, at which a word's `count` syllables start: shared out evenly over the
        word, each moved to the nearest of the word's own onsets when one is within 40% of a
        syllable's length (and at most `longestSnap`), keeping the order. The first is the word's start. */
    std::vector<double> getSyllableTimes (const Word&, int count);

    /** The words as lyrics on the song's timeline, for audio that starts at `startBeat`, through
        the song's tempo (and its changes):
        lines at pauses of 0.4 s or more, at Whisper's phrases and at sections, a new verse per
        section (headed with its name) or after 3 s without singing, and every syllable pinned at
        its time. Ends of lines lose their punctuation and lines start with a capital. */
    Lyrics toLyrics (const std::vector<Word>&, double startBeat, const TempoMap&, const SectionList&);

    /** `current` with every line that overlaps `from` to `to` (in beats) replaced by `found`. */
    Lyrics replaceBetween (const Lyrics& current, const Lyrics& found, double fromBeat, double toBeat);

    //==============================================================================
    /** A Whisper model, loaded with whisper.cpp; on the GPU (Metal) where there is one. */
    class Transcriber
    {
    public:
        Transcriber();
        ~Transcriber();

        bool load (const juce::File&, ModelSize, juce::String& error);

        /** The English words in 16 kHz mono audio, with their times. Blocks; returns nullopt when
            cancelled or when it fails (then with `error` set). */
        std::optional<std::vector<Word>> transcribe (const juce::AudioBuffer<float>& mono, const ProgressFn&, juce::String& error);

    private:
        whisper_context* context = nullptr;

        JUCE_DECLARE_NON_COPYABLE (Transcriber)
    };
} // namespace lyricsfinder
} // namespace daw::analysis
