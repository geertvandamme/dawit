#include "LyricsFinder.h"
#include "MonoAudio.h"

#include <whisper.h>

#include <map>
#include <thread>

namespace daw::analysis::lyricsfinder
{
namespace
{
    // OpenAI's English-only Whisper models converted for whisper.cpp, at a fixed revision.
    const models::Model smallModel { "ggml-small.en.bin",
                                     "https://huggingface.co/ggerganov/whisper.cpp/resolve/"
                                     "5359861c739e955e79d9a303bcbc70fb988958b1/ggml-small.en.bin",
                                     "c6138d6d58ecc8322097e0f987c32f1be8bb0a18532a3f88f734d1bbf9c41e5d",
                                     487614201 };

    const models::Model mediumModel { "ggml-medium.en.bin",
                                      "https://huggingface.co/ggerganov/whisper.cpp/resolve/"
                                      "5359861c739e955e79d9a303bcbc70fb988958b1/ggml-medium.en.bin",
                                      "cc37e93478338ec7700281a7ac30a10128929eb8f427dda2e865faa8f6da4356",
                                      1533774781 };

    constexpr double frameSeconds = 0.01;      // the level is measured every 10 ms
    constexpr double windowSeconds = 0.02;     // over 20 ms
    constexpr double singingRangeDb = 30.0;    // singing is within this much of the loudest part
    constexpr double quietestSingingDb = -50.0;
    constexpr double longestBreath = 0.25;     // shorter gaps are bridged
    constexpr double shortestSinging = 0.1;    // shorter blips are left out

    constexpr double dtwDelay = 0.1;           // Whisper's word times (DTW) land this long after the word starts, measured on speech
    constexpr double phraseStartSnap = 0.3;    // the first word of a phrase starts with the singing when it's this close

    constexpr double minWordGap = 0.02;        // words start at least this far apart
    constexpr double minWordLength = 0.05;
    constexpr double onsetEdge = 0.04;         // an onset this close to a word's edge is the word's own start or the next's
    constexpr double minSyllableGap = 0.04;

    constexpr double lineBreakGap = 0.4;       // a pause this long starts a new line
    constexpr double verseBreakGap = 3.0;      // and this long a new verse, where there are no sections
    constexpr int longestLine = 12;            // words; longer lines are split at their longest pause

    bool hasLetters (const juce::String& text)
    {
        for (auto p = text.getCharPointer(); ! p.isEmpty();)
            if (juce::CharacterFunctions::isLetterOrDigit (p.getAndAdvance()))
                return true;

        return false;
    }

    /** Lower-case letters and single spaces only: "Thanks for watching!" gives "thanks for watching". */
    juce::String normalise (const juce::String& text)
    {
        juce::String out;
        const auto lower = text.toLowerCase();

        for (auto p = lower.getCharPointer(); ! p.isEmpty();)
        {
            const auto c = p.getAndAdvance();
            out += juce::CharacterFunctions::isLetterOrDigit (c) ? juce::String::charToString (c) : juce::String (" ");
        }

        return out.trim().replace ("  ", " ").replace ("  ", " ");
    }

    /** A sound Whisper writes in brackets instead of words: "(upbeat music)", "(laughs)". Backing
        vocals in brackets, "(yeah)", are words and stay. */
    bool isSoundDescription (const juce::String& text)
    {
        const auto words = juce::StringArray::fromTokens (normalise (text), " ", "");

        for (auto* sound : { "music", "playing", "plays", "laughs", "laughing", "applause", "instrumental", "humming", "hums",
                             "vocalizing", "vocalising", "sighs", "inaudible", "silence", "cheering", "chuckles", "breathing",
                             "gasps", "beeping", "noise", "static", "whistling", "clapping", "singing", "sings" })
            if (words.contains (sound))
                return true;

        return false;
    }

    double overlapWith (const std::vector<juce::Range<double>>& ranges, juce::Range<double> span, double widen)
    {
        double total = 0.0;

        for (const auto& r : ranges)
            total += r.expanded (widen).getIntersectionWith (span).getLength();

        return total;
    }

    /** A word as it goes in the lyrics: no quotes, full stops or colons, and at the end of a
        line no punctuation at all. */
    juce::String tidyWord (const juce::String& text, bool endsLine)
    {
        auto t = text.trim().removeCharacters ("\".;:");

        if (endsLine)
            while (t.isNotEmpty() && juce::String (",!?").containsChar (t.getLastCharacter()))
                t = t.dropLastCharacters (1);

        return t;
    }

    bool isI (const juce::String& word)
    {
        return word == "I" || word.startsWith ("I'") || word.startsWith (juce::String::fromUTF8 ("I\xe2\x80\x99"));
    }

    void ignoreLog (ggml_log_level, const char*, void*) {}
}

//==============================================================================
const models::Model& getModel (ModelSize size)
{
    switch (size)
    {
        case ModelSize::small:  return smallModel;
        case ModelSize::medium: return mediumModel;
    }

    return smallModel;
}

juce::String getModelName (ModelSize size)
{
    switch (size)
    {
        case ModelSize::small:  return "Whisper small (English)";
        case ModelSize::medium: return "Whisper medium (English)";
    }

    return {};
}

//==============================================================================
juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    return toMono (audio, sourceSampleRate, sampleRate);
}

std::vector<juce::Range<double>> findSinging (const juce::AudioBuffer<float>& mono, double rate)
{
    const int hop = juce::jmax (1, juce::roundToInt (rate * frameSeconds));
    const int window = juce::jmax (1, juce::roundToInt (rate * windowSeconds));
    const int numSamples = mono.getNumSamples();
    const int numFrames = numSamples / hop;

    std::vector<double> levelDb ((size_t) numFrames, -200.0);
    double loudest = -200.0;

    for (int f = 0; f < numFrames; ++f)
    {
        const int start = f * hop;
        const int length = juce::jmin (window, numSamples - start);
        const double rms = mono.getRMSLevel (0, start, length);
        levelDb[(size_t) f] = rms > 0.0 ? 20.0 * std::log10 (rms) : -200.0;
        loudest = juce::jmax (loudest, levelDb[(size_t) f]);
    }

    const double threshold = juce::jmax (quietestSingingDb, loudest - singingRangeDb);
    std::vector<juce::Range<double>> ranges;

    for (int f = 0; f < numFrames;)
    {
        if (levelDb[(size_t) f] < threshold)
        {
            ++f;
            continue;
        }

        int end = f;

        while (end < numFrames && levelDb[(size_t) end] >= threshold)
            ++end;

        const juce::Range<double> range (f * hop / rate, juce::jmin (numSamples, end * hop + window) / rate);

        if (! ranges.empty() && range.getStart() - ranges.back().getEnd() < longestBreath)
            ranges.back() = ranges.back().getUnionWith (range);
        else
            ranges.push_back (range);

        f = end;
    }

    ranges.erase (std::remove_if (ranges.begin(), ranges.end(), [] (const juce::Range<double>& r) { return r.getLength() < shortestSinging; }),
                  ranges.end());
    return ranges;
}

void muteOutside (juce::AudioBuffer<float>& mono, double rate, const std::vector<juce::Range<double>>& singing)
{
    const int numSamples = mono.getNumSamples();
    const int fade = juce::roundToInt (rate * 0.02);
    const int margin = juce::roundToInt (rate * 0.05);
    std::vector<float> gain ((size_t) numSamples, 0.0f);

    for (const auto& r : singing)
    {
        const int start = juce::jmax (0, juce::roundToInt (r.getStart() * rate) - margin);
        const int end = juce::jmin (numSamples, juce::roundToInt (r.getEnd() * rate) + margin);

        for (int i = juce::jmax (0, start - fade); i < juce::jmin (numSamples, end + fade); ++i)
        {
            const float g = i < start ? (float) (i - (start - fade)) / (float) fade
                                      : (i >= end ? (float) (end + fade - i) / (float) fade : 1.0f);
            gain[(size_t) i] = juce::jmax (gain[(size_t) i], g);
        }
    }

    auto* data = mono.getWritePointer (0);

    for (int i = 0; i < numSamples; ++i)
        data[i] *= gain[(size_t) i];
}

bool isMadeUpPhrase (const juce::String& text)
{
    const auto t = " " + normalise (text) + " ";

    for (auto* phrase : { "thanks for watching", "thank you for watching", "thanks for listening", "thank you for listening",
                          "please subscribe", "like and subscribe", "subscribe to", "subtitles by", "subtitled by", "captions by",
                          "transcribed by", "transcription by", "translated by", "amara org", "see you in the next video",
                          "see you next time", "www" })
        if (t.contains (" " + juce::String (phrase) + " "))
            return true;

    return false;
}

std::vector<Word> cleanUp (std::vector<Word> words, const std::vector<juce::Range<double>>& singing)
{
    // Phrases Whisper makes up, by its own phrases.
    std::map<int, juce::String> phrases;

    for (const auto& w : words)
        phrases[w.segment] += " " + w.text;

    std::vector<Word> kept;

    for (size_t i = 0; i < words.size(); ++i)
    {
        auto w = words[i];
        w.text = w.text.trim();

        // Brackets: "[Music]" always goes; "(soft music)" goes, "(yeah)" stays without the brackets.
        if (w.text.startsWithChar ('[') || w.text.startsWithChar ('('))
        {
            const auto close = w.text.startsWithChar ('[') ? ']' : ')';
            size_t end = i;
            juce::String inside;

            while (end < words.size())
            {
                inside += " " + words[end].text;

                if (words[end].text.containsChar (close))
                    break;

                ++end;
            }

            if (close == ']' || isSoundDescription (inside))
            {
                i = juce::jmin (end, words.size() - 1);
                continue;
            }
        }

        w.text = w.text.removeCharacters ("[]()");

        if (! hasLetters (w.text) || isMadeUpPhrase (phrases[w.segment]))
            continue;

        // The singing the word starts in, or else the one it overlaps most. A word mostly outside
        // the singing is made up from leftovers of the band, or from silence.
        const juce::Range<double> span (w.start, juce::jmax (w.end, w.start + 0.02));
        const auto home = std::find_if (singing.begin(), singing.end(), [&w] (const juce::Range<double>& r) { return r.expanded (0.15).contains (w.start); });
        const juce::Range<double>* within = home != singing.end() ? &*home : nullptr;

        if (within == nullptr)
        {
            if (overlapWith (singing, span, 0.15) < 0.3 * span.getLength())
                continue;

            for (const auto& r : singing)
                if (within == nullptr || r.getIntersectionWith (span).getLength() > within->getIntersectionWith (span).getLength())
                    within = &r;
        }

        // Pulled in to that singing: Whisper often stretches a phrase's last word over the pause after it.
        if (within != nullptr)
        {
            w.start = juce::jmax (w.start, within->getStart());

            if (w.end > within->getEnd() + 0.1)
                w.end = within->getEnd();
        }

        kept.push_back (w);
    }

    std::stable_sort (kept.begin(), kept.end(), [] (const Word& a, const Word& b) { return a.start < b.start; });

    // The first word of each phrase starts where the singing does.
    const juce::Range<double>* previousPhrase = nullptr;

    for (auto& w : kept)
    {
        const auto phrase = std::find_if (singing.begin(), singing.end(), [&w] (const juce::Range<double>& r) { return r.expanded (0.15).contains (w.start); });

        if (phrase == singing.end())
            continue;

        if (&*phrase != previousPhrase && w.start > phrase->getStart() && w.start - phrase->getStart() < phraseStartSnap)
            w.start = phrase->getStart();

        previousPhrase = &*phrase;
    }

    for (size_t i = 0; i < kept.size(); ++i)
    {
        auto& w = kept[i];

        if (i > 0)
            w.start = juce::jmax (w.start, kept[i - 1].start + 0.02);

        w.end = juce::jmax (w.end, w.start + 0.05);

        if (i + 1 < kept.size())
            w.end = juce::jmin (w.end, juce::jmax (kept[i + 1].start, w.start + 0.02));
    }

    return kept;
}

//==============================================================================
void snapToOnsets (std::vector<Word>& words, const std::vector<double>& onsets)
{
    if (onsets.empty())
        return;

    for (size_t i = 0; i < words.size(); ++i)
    {
        auto& w = words[i];
        const double minimum = i > 0 ? words[i - 1].start + minWordGap : -1.0e9;
        const double maximum = i + 1 < words.size() ? words[i + 1].start - minWordGap : 1.0e9;
        const double latest = juce::jmin (maximum, w.end - minWordLength);

        auto it = std::lower_bound (onsets.begin(), onsets.end(), w.start - longestSnap);
        const double* best = nullptr;

        for (; it != onsets.end() && *it <= w.start + longestSnap; ++it)
        {
            if (*it < minimum || *it > latest)
                continue;

            // An onset nearer the next word's start belongs to that word.
            if (i + 1 < words.size() && std::abs (*it - words[i + 1].start) < std::abs (*it - w.start))
                continue;

            if (best == nullptr || std::abs (*it - w.start) < std::abs (*best - w.start))
                best = &*it;
        }

        if (best == nullptr)
            continue;

        w.start = *best;

        if (i > 0)
            words[i - 1].end = juce::jmin (words[i - 1].end, juce::jmax (w.start, words[i - 1].start + minWordLength));
    }

    for (auto& w : words)
    {
        w.end = juce::jmax (w.end, w.start + minWordLength);
        w.onsets.clear();

        for (auto it = std::upper_bound (onsets.begin(), onsets.end(), w.start + onsetEdge); it != onsets.end() && *it < w.end - onsetEdge; ++it)
            w.onsets.push_back (*it);
    }

    for (size_t i = 0; i + 1 < words.size(); ++i)
        words[i].end = juce::jmin (words[i].end, juce::jmax (words[i + 1].start, words[i].start + minWordLength));
}

std::vector<double> getSyllableTimes (const Word& w, int count)
{
    std::vector<double> times;
    const double length = w.end - w.start;
    double last = w.start;

    for (int k = 0; k < count; ++k)
    {
        double t = w.start + length * k / count;

        if (k > 0)
        {
            const double reach = juce::jmin (longestSnap, 0.4 * length / count);
            const double* best = nullptr;

            for (const auto& o : w.onsets)
                if (std::abs (o - t) <= reach && o >= last + minSyllableGap && (best == nullptr || std::abs (o - t) < std::abs (*best - t)))
                    best = &o;

            if (best != nullptr)
                t = *best;
        }

        t = juce::jmax (t, last);
        times.push_back (t);
        last = t;
    }

    return times;
}

Lyrics toLyrics (const std::vector<Word>& words, double startBeat, const TempoMap& tempo, const SectionList& sections)
{
    auto beatOf = [startBeat, &tempo] (double seconds) { return tempo.getBeatAfter (startBeat, seconds); };

    // The section a word is sung in; a word just before the bar line is a pick-up into the next one.
    auto sectionOf = [&] (const Word& w)
    {
        const double beat = beatOf (w.start) + 0.5;

        for (size_t i = 0; i < sections.size(); ++i)
            if (beat >= sections[i].startBar * 4.0 && beat < sections[i].getEndBar() * 4.0)
                return (int) i;

        return -1;
    };

    struct Line
    {
        std::vector<Word> words;
        bool startsVerse = false;
        int section = -1;
    };

    std::vector<Line> lines;

    for (size_t i = 0; i < words.size(); ++i)
    {
        const auto& w = words[i];
        const int section = sectionOf (w);
        bool newLine = lines.empty(), newVerse = lines.empty();

        if (! lines.empty())
        {
            const auto& previous = words[i - 1];
            const double gap = w.start - previous.end;

            if (section != lines.back().section)
                newLine = newVerse = true;
            else if (section < 0 && gap >= verseBreakGap)
                newLine = newVerse = true;
            else if (gap >= lineBreakGap)
                newLine = true;
            else if (w.segment != previous.segment && (gap >= 0.15 || lines.back().words.size() >= 3))
                newLine = true;
        }

        if (newLine)
            lines.push_back ({ {}, newVerse, section });

        lines.back().words.push_back (w);
    }

    // Long lines split at their longest pause, with at least three words on each side.
    for (size_t i = 0; i < lines.size(); ++i)
    {
        auto& line = lines[i];

        if ((int) line.words.size() <= longestLine)
            continue;

        size_t at = 3;
        double longest = -1.0;

        for (size_t j = 3; j + 3 <= line.words.size(); ++j)
        {
            const double gap = line.words[j].start - line.words[j - 1].end;

            if (gap > longest)
            {
                longest = gap;
                at = j;
            }
        }

        Line rest { { line.words.begin() + (std::ptrdiff_t) at, line.words.end() }, false, line.section };
        line.words.resize (at);
        lines.insert (lines.begin() + (std::ptrdiff_t) i + 1, std::move (rest));
        --i;   // the first half may still be too long
    }

    Lyrics lyrics;

    for (const auto& line : lines)
    {
        LyricLine out;
        out.startsVerse = line.startsVerse;

        if (line.startsVerse && line.section >= 0)
            out.heading = sections[(size_t) line.section].getDisplayName();

        bool afterFullStop = false;

        for (size_t j = 0; j < line.words.size(); ++j)
        {
            const auto& w = line.words[j];
            auto text = tidyWord (w.text, j + 1 == line.words.size());

            // Lines start with a capital; a capital after a full stop that's gone becomes small.
            if (j == 0)
                text = text.substring (0, 1).toUpperCase() + text.substring (1);
            else if (afterFullStop && ! isI (text))
                text = text.substring (0, 1).toLowerCase() + text.substring (1);

            afterFullStop = w.text.trim().endsWithChar ('.');

            if (text.isEmpty())
                continue;

            const auto parts = lyrics::splitEnglishWord (text);

            const auto times = getSyllableTimes (w, parts.size());

            for (int k = 0; k < parts.size(); ++k)
            {
                LyricSyllable s;
                s.text = parts[k];
                s.endsWord = k + 1 == parts.size();
                s.pinned = true;
                s.beat = beatOf (times[(size_t) k]);
                out.syllables.push_back (s);
            }
        }

        if (out.syllables.empty())
            continue;

        out.startBeat = out.syllables.front().beat;
        out.endBeat = beatOf (line.words.back().end);
        lyrics.lines.push_back (std::move (out));
    }

    lyrics::tidy (lyrics);
    return lyrics;
}

Lyrics replaceBetween (const Lyrics& current, const Lyrics& found, double fromBeat, double toBeat)
{
    Lyrics result;

    for (const auto& line : current.lines)
        if (line.endBeat <= fromBeat || line.startBeat >= toBeat)
            result.lines.push_back (line);

    result.lines.insert (result.lines.end(), found.lines.begin(), found.lines.end());
    std::stable_sort (result.lines.begin(), result.lines.end(),
                      [] (const LyricLine& a, const LyricLine& b) { return a.startBeat < b.startBeat; });
    return result;
}

//==============================================================================
Transcriber::Transcriber() = default;

Transcriber::~Transcriber()
{
    if (context != nullptr)
        whisper_free (context);
}

bool Transcriber::load (const juce::File& file, ModelSize size, juce::String& error)
{
    whisper_log_set (ignoreLog, nullptr);   // it reports every step on the console

    if (context != nullptr)
        whisper_free (context);

    auto params = whisper_context_default_params();
    params.use_gpu = true;
    params.flash_attn = false;              // the word timing (DTW) doesn't work with it
    params.dtw_token_timestamps = true;
    params.dtw_aheads_preset = size == ModelSize::small ? WHISPER_AHEADS_SMALL_EN : WHISPER_AHEADS_MEDIUM_EN;

    const auto path = file.getFullPathName();
    context = whisper_init_from_file_with_params (path.toRawUTF8(), params);

    if (context == nullptr)
    {
        error = "Couldn't load the Whisper model " + file.getFileName() + ". Delete it from "
              + file.getParentDirectory().getFullPathName() + " to download it again.";
        return false;
    }

    return true;
}

std::optional<std::vector<Word>> Transcriber::transcribe (const juce::AudioBuffer<float>& mono, const ProgressFn& progress, juce::String& error)
{
    if (context == nullptr)
    {
        error = "The Whisper model isn't loaded.";
        return std::nullopt;
    }

    struct Callbacks
    {
        const ProgressFn& progress;
        std::atomic<bool> stop { false };
    };

    Callbacks callbacks { progress };

    auto params = whisper_full_default_params (WHISPER_SAMPLING_BEAM_SEARCH);
    params.n_threads = juce::jlimit (1, 8, (int) std::thread::hardware_concurrency());
    params.language = "en";
    params.translate = false;
    params.token_timestamps = true;
    params.suppress_nst = true;             // no "♪" or "(music)" for the instruments
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.print_special = false;

    params.progress_callback = [] (whisper_context*, whisper_state*, int percent, void* user)
    {
        auto* c = static_cast<Callbacks*> (user);

        if (! c->progress (percent / 100.0))
            c->stop = true;
    };
    params.progress_callback_user_data = &callbacks;
    params.encoder_begin_callback = [] (whisper_context*, whisper_state*, void* user) { return ! static_cast<Callbacks*> (user)->stop.load(); };
    params.encoder_begin_callback_user_data = &callbacks;
    params.abort_callback = [] (void* user) { return static_cast<Callbacks*> (user)->stop.load(); };
    params.abort_callback_user_data = &callbacks;

    const int result = whisper_full (context, params, mono.getReadPointer (0), mono.getNumSamples());

    if (callbacks.stop)
        return std::nullopt;

    if (result != 0)
    {
        error = "Whisper couldn't listen to the vocals (error " + juce::String (result) + ").";
        return std::nullopt;
    }

    // Tokens are pieces of words; one starting with a space starts a new word. The text is UTF-8
    // bytes that may split a character between tokens, so it's joined before it's decoded.
    std::vector<Word> words;
    std::string text;
    Word word;
    const auto lastTextToken = whisper_token_eot (context);

    auto finishWord = [&]
    {
        if (! text.empty())
        {
            word.text = juce::String::fromUTF8 (text.c_str()).trim();
            words.push_back (word);
        }

        text.clear();
    };

    for (int i = 0; i < whisper_full_n_segments (context); ++i)
    {
        for (int j = 0; j < whisper_full_n_tokens (context, i); ++j)
        {
            const auto data = whisper_full_get_token_data (context, i, j);

            if (data.id >= lastTextToken)
                continue;   // timestamps and other special tokens

            const std::string piece = whisper_full_get_token_text (context, i, j);

            if (piece.empty())
                continue;

            const double t0 = (double) data.t0 * 0.01, t1 = (double) data.t1 * 0.01;

            if (text.empty() || piece.front() == ' ' || word.segment != i)
            {
                finishWord();
                word.segment = i;
                word.start = data.t_dtw >= 0 ? juce::jmax (0.0, (double) data.t_dtw * 0.01 - dtwDelay) : t0;
            }

            text += piece;
            word.end = juce::jmax (t1, word.start + 0.05);
        }
    }

    finishWord();
    return words;
}
} // namespace daw::analysis::lyricsfinder
