#include "WriteLyrics.h"

#include "Rhyme.h"

#include <cmath>

namespace daw::gen::writelyrics
{
using guides::Guide;

std::vector<Step> getSteps()
{
    return {
        { "Pick a section", "Choose the part your line is for",
          "Each part of a song has its own job, and the words follow it. A verse tells the story, with new words each time. A chorus says what the song is about, with the same words every time it comes back. "
          "Intros, solos and drops usually have no words, so DawIt looks for a part that is usually sung: a verse, pre-chorus, chorus, post-chorus, bridge or phrase.",
          { { Guide::lyrics, "What each part says" }, { Guide::structure, "How songs are built" } } },
        { "Write a line", "One sung line for your section",
          "A line of lyrics is sung over a phrase of the music, usually 2 bars, sometimes 1 or 4. Leave room to breathe: a line usually ends before its phrase does. "
          "Everyday words, said naturally, work better than clever ones, and a concrete picture, two cups on the table, one still full, says more than naming a feeling. Count syllables, not words: \"together\" is three.",
          { { Guide::lyrics, "Lines and phrases" }, { Guide::lyrics, "Writing tips" } } },
        { "Time it by tapping", "Put each syllable where it's sung",
          "Until you place them, DawIt spreads a line's syllables evenly, which is rarely how words are sung. Words have a stressed syllable, to-GE-ther, and stressed syllables belong on strong beats: in 4/4, beat 1 is the strongest and beat 3 the next. "
          "Tapping is the quickest way to put each syllable where you hear it. DawIt takes your sound card's delay off each tap, and snaps it to the nearest quarter, half or whole beat, finer as you zoom in. "
          "Most people tap a little early, a few hundredths of a second ahead of the beat, and the snapping usually puts those taps back on it. "
          "A line may start just before the bar line on a small word, a pickup: it still belongs to its section.",
          { { Guide::lyrics, "Stress and the beat" }, { Guide::rhythm, "Bars and beats" } } },
        { "Sing it", "Sing your line with the words in front of you",
          "Singing a line out loud is its real test. A word that's hard to sing, or a stress on the wrong beat, shows at once: go back to step 3, or change a word. "
          "Breathe in the rest before the line, not in the middle of a word, and come in when the colour starts, which may be just before the bar line. "
          "DawIt doesn't listen to you: the tick only says the song played your line with the words in front of you.",
          { { Guide::lyrics, "Singing along" }, { Guide::singing, "Posture and breathing" } } }
    };
}

juce::String getTitle() { return "Write lyrics to your song"; }

juce::String getIntro()
{
    return "Four steps to a first sung line: pick a part of your song, write a line for it, time it by tapping along, then sing it with the words in front of you. "
           "Do each step in the main window: the tick appears by itself.";
}

juce::String getHelpDescription() { return "Pick a section, write a line for it, time it by tapping and sing it, in four steps"; }

juce::String getCompletionText()
{
    return "Done. You wrote a line for your song, put each syllable in time and sang it. Next, write the line that answers it: lines come in pairs, and in many songs line 2 rhymes with line 4. "
           "Then fill the section, and record your voice on an audio track while Sing along shows the words.";
}

int clampStep (int stored) noexcept { return stored >= 0 && stored < numSteps ? stored : 0; }

//==============================================================================
std::optional<Section> findChosenSection (const SectionList& sections, const Loop& loop, double barLength, int rememberedBar)
{
    constexpr double eps = 1.0e-6;

    if (loop.on && loop.endBeat > loop.startBeat + eps)
        for (auto& s : sections)
            if (lyrics::isSung (s.type) && loop.startBeat >= s.startBar * barLength - eps && loop.endBeat <= s.getEndBar() * barLength + eps)
                return s;

    if (rememberedBar >= 0)
        for (auto& s : sections)
            if (s.startBar == rememberedBar && lyrics::isSung (s.type))
                return s;

    return std::nullopt;
}

bool lineCountsFor (const LyricLine& line, const Section& section, double barLength)
{
    const double middle = (line.startBeat + line.endBeat) * 0.5;
    return middle >= section.startBar * barLength - 1.0e-9 && middle < section.getEndBar() * barLength - 1.0e-9;
}

int countUnplaced (const LyricLine& line)
{
    int count = 0;

    for (size_t i = 1; i < line.syllables.size(); ++i)
        if (! line.syllables[i].pinned)
            ++count;

    return count;
}

bool isLineTimed (const LyricLine& line) { return line.syllables.size() >= 2 && countUnplaced (line) == 0; }

int findLine (const Lyrics& lyrics, const Section& section, double barLength)
{
    int firstCounting = -1;

    for (size_t i = 0; i < lyrics.lines.size(); ++i)
    {
        if (! lineCountsFor (lyrics.lines[i], section, barLength))
            continue;

        if (isLineTimed (lyrics.lines[i]))
            return (int) i;

        if (firstCounting < 0)
            firstCounting = (int) i;
    }

    return firstCounting;
}

juce::String getLineText (const LyricLine& line)
{
    juce::String text;

    for (auto& s : line.syllables)
        text << s.text << (s.endsWord ? " " : "");

    return text.trim();
}

/** Letters and apostrophes only, lower case: "River," gives "river". */
static juce::String cleanWord (const juce::String& word)
{
    juce::String clean;
    const auto lower = word.toLowerCase();

    for (auto p = lower.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();

        if (juce::CharacterFunctions::isLetter (c) || c == '\'')
            clean += juce::String::charToString (c);
    }

    return clean;
}

juce::StringArray findUnsplitWords (const LyricLine& line)
{
    juce::StringArray words;
    size_t i = 0;

    while (i < line.syllables.size())
    {
        size_t last = i;

        while (last + 1 < line.syllables.size() && ! line.syllables[last].endsWord)
            ++last;

        if (last == i)
        {
            const auto clean = cleanWord (line.syllables[i].text);

            if (clean.isNotEmpty() && rhyme::countSyllables (clean) >= 2)
                words.add (clean);
        }

        i = last + 1;
    }

    return words;
}

int countSungSyllables (const LyricLine& line)
{
    juce::String text;

    for (auto& s : line.syllables)
        text << s.text << (s.endsWord ? " " : "-");

    return rhyme::countSyllables (text.trimEnd());
}

bool isLineFast (const LyricLine& line)
{
    const double beats = line.endBeat - line.startBeat;
    return beats > 0.0 && (double) countSungSyllables (line) / beats > fastSyllablesPerBeat;
}

static juce::String joinNames (const juce::StringArray& names);

/** The hyphen tip for the line's unsplit multi-syllable words, or nothing when there are none. */
static juce::String getHyphenTip (const LyricLine& line)
{
    const auto unsplit = findUnsplitWords (line);

    if (unsplit.size() == 1)
        return "Tip: " + unsplit[0] + " sounds like more than one syllable but is one block, so it gets one tap. In the Lyrics window, put hyphens between its syllables to tap each one.";

    if (unsplit.size() > 1)
        return "Tip: " + joinNames (unsplit) + " sound like more than one syllable but are one block each, so they get one tap. In the Lyrics window, put hyphens between their syllables (ri-ver) to tap each one.";

    return {};
}

bool hasSoundIn (const Engine& engine, const Section& section)
{
    const double barLength = engine.getBarLength();
    const double from = section.startBar * barLength, to = section.getEndBar() * barLength;
    const auto& tempo = engine.getTempoMap();

    for (auto& t : engine.getTracks())
    {
        if (t->muted.load())
            continue;

        for (auto& c : t->clips)
            if (! c->muted && c->startBeat < to - 1.0e-9 && from < c->getEndBeat (tempo) - 1.0e-9)
                return true;
    }

    return false;
}

//==============================================================================
void SungTracker::startNewPass() { seenFirstHalf = false; }

void SungTracker::reset()
{
    done = false;
    doneText = {};
    passText = {};
    lastBeat = -1.0e9;
    startNewPass();
}

void SungTracker::feed (double heardBeat, bool playing, bool singAlongShowing, const LineSpan* line, double secondsPerBeat)
{
    if (! playing || ! singAlongShowing || line == nullptr)
    {
        startNewPass();
        lastBeat = -1.0e9;
        return;
    }

    if (line->text != passText)
    {
        startNewPass();
        passText = line->text;
    }

    if (heardBeat < lastBeat - 1.0e-9)
        startNewPass();   // a loop wrap

    lastBeat = heardBeat;

    const double start = line->startBeat, length = line->endBeat - line->startBeat;

    if (heardBeat < start - 1.0e-9 || heardBeat > line->endBeat + 1.0e-9)
        return;

    if (length * secondsPerBeat < shortLineSeconds)
    {
        done = true;
        doneText = line->text;
        return;
    }

    if (heardBeat < start + length * 0.5)
    {
        seenFirstHalf = true;
    }
    else if (seenFirstHalf)
    {
        done = true;
        doneText = line->text;
    }
}

//==============================================================================
Loop getLoop (const Engine& engine) { return { engine.loopEnabled.load(), engine.loopStartBeat.load(), engine.loopEndBeat.load() }; }

std::optional<Section> findChosenSection (const Engine& engine, const Progress& progress)
{
    return findChosenSection (engine.getSections(), getLoop (engine), engine.getBarLength(), progress.rememberedBar);
}

std::optional<Section> findFirstSungSection (const SectionList& sections)
{
    for (auto& s : sections)
        if (lyrics::isSung (s.type))
            return s;

    return std::nullopt;
}

juce::String getBarsText (const Section& s) { return "bars " + juce::String (s.startBar + 1) + " to " + juce::String (s.getEndBar()); }

/** A number to one decimal without a trailing ".0": 8, 2.5. */
static juce::String formatNumber (double value)
{
    auto text = juce::String (value, 1);
    return text.endsWith (".0") ? text.dropLastCharacters (2) : text;
}

static juce::String joinNames (const juce::StringArray& names)
{
    if (names.size() < 2)
        return names.joinIntoString ("");

    return names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1];
}

/** What the exercise knows about the song now. The line pointer is only good inside the call that asked. */
struct Situation
{
    std::optional<Section> section;
    const LyricLine* line = nullptr;
};

static Situation getSituation (const Engine& engine, const Progress& progress)
{
    Situation s;
    s.section = findChosenSection (engine, progress);

    if (s.section.has_value())
    {
        const int index = findLine (engine.getLyrics(), *s.section, engine.getBarLength());

        if (index >= 0)
            s.line = &engine.getLyrics().lines[(size_t) index];
    }

    return s;
}

static bool isSungDone (const Situation& s, const Progress& progress)
{
    return s.line != nullptr && progress.sung.done && progress.sung.text == getLineText (*s.line);
}

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    const auto s = getSituation (engine, progress);

    switch (step)
    {
        case stepSection: return s.section.has_value();
        case stepWrite:   return s.line != nullptr;
        case stepTap:     return s.line != nullptr && isLineTimed (*s.line);
        case stepSing:    return isSungDone (s, progress);
        default:          break;
    }

    return false;
}

juce::String getDoText (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepSection:
            return "Press Open the Sections row, click a verse or a chorus there and choose Loop this section. The loop plays that part over and over, so you can listen, write, tap and sing without rewinding. "
                   "A chorus is a good place to start: its few lines say what the song is about.";

        case stepWrite:
        {
            const auto section = findChosenSection (engine, progress);
            const auto name = section.has_value() ? section->getDisplayName() : juce::String ("your section");
            return "Press Open Lyrics... and type one line for " + name + ", the way you would sing it. Put hyphens between the syllables of longer words, ri-ver, to-ge-ther, so each gets its own place in time. "
                   "Above it, type the part's name in brackets, such as [Chorus], to put it on that kind of section. Then press Apply lyrics to song. "
                   "If Ollama is installed, the window's Write with AI panel can suggest lines, but your own words are the point here.";
        }

        case stepTap:
            return "Press Tap this line. The song plays from a bar before your line. Sing it in your head with the music and press any letter key the moment each syllable comes: each tap puts the next syllable there, and the first moves the whole line. "
                   "Tapping stops after the last syllable, and with the loop on you can tap it again on the next pass. You can also drag a syllable on the Lyrics row; Alt drags without snapping.";

        case stepSing:
            return "Press Open Sing along (Cmd+5). It shows the line to sing now in large letters, and the colour runs over each syllable as it's sung, just as you tapped it. "
                   "Press Space and sing when your line comes round: with the section looping, you get another go each pass. The tick comes once the song has played through your line with Sing along open.";

        default: break;
    }

    return {};
}

juce::String getActionLabel (int step)
{
    switch (step)
    {
        case stepSection: return "Open the Sections row";
        case stepWrite:   return "Open Lyrics...";
        case stepTap:     return "Tap this line";
        case stepSing:    return "Open Sing along";
        default:          break;
    }

    return {};
}

juce::String getActionTooltip (int step, const juce::String& sectionName)
{
    switch (step)
    {
        case stepSection:
            return sectionName.isNotEmpty() ? "Shows the Sections row and points at " + sectionName + ". Click a section and choose Loop this section."
                                            : juce::String ("Shows the Sections row. Click a section and choose Loop this section.");

        case stepWrite:
            return "Opens the Lyrics window to type your line. Nothing in the song changes until you press Apply lyrics to song.";

        case stepTap:
            return "Plays from a bar before your line. Press any letter key the moment each syllable is sung. Tapping stops after the last syllable; Esc stops it sooner.";

        case stepSing:
            return "Opens Sing along (Cmd+5): your words in large letters as the song plays. Press Space to play.";

        default: break;
    }

    return {};
}

juce::String getFixLabel (Fix fix)
{
    switch (fix)
    {
        case Fix::openSections:   return getActionLabel (stepSection);
        case Fix::openLyrics:     return getActionLabel (stepWrite);
        case Fix::audioSettings:  return "Audio/MIDI Settings...";
        case Fix::detectLyrics:
        case Fix::addRecordingTrack:
        case Fix::none:
        case Fix::importAudio:
        case Fix::detectTempo:
        case Fix::detectChords:
        case Fix::detectSections: break;
    }

    return {};
}

juce::String getFixTooltip (Fix fix, const Engine& engine, const Progress& progress)
{
    switch (fix)
    {
        case Fix::openSections:
        {
            auto section = findChosenSection (engine, progress);

            if (! section.has_value())
                section = findFirstSungSection (engine.getSections());

            return getActionTooltip (stepSection, section.has_value() ? section->getDisplayName() : juce::String());
        }

        case Fix::openLyrics:     return getActionTooltip (stepWrite, {});
        case Fix::audioSettings:  return "Opens the audio settings, where you choose where the sound comes out.";
        case Fix::detectLyrics:
        case Fix::addRecordingTrack:
        case Fix::none:
        case Fix::importAudio:
        case Fix::detectTempo:
        case Fix::detectChords:
        case Fix::detectSections: break;
    }

    return {};
}

static juce::String describeUnsung (const SectionList& sections)
{
    juce::StringArray names;

    for (auto& s : sections)
        names.addIfNotAlreadyThere (s.getDisplayName());

    return joinNames (names);
}

Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    const auto& sections = engine.getSections();
    const auto s = getSituation (engine, progress);

    if (step == stepSection)
    {
        if (s.section.has_value())
            return {};

        if (sections.empty())
            return { "Your song has no sections yet. Click an empty part of the Sections row, above the chords, and choose Verse or Chorus, or start from a song sketch with File > New Project from Style....", Fix::openSections };

        if (! findFirstSungSection (sections).has_value())
            return { "Your song's sections, " + describeUnsung (sections) + ", usually have no words. Click one, choose Type and make it a Verse or a Chorus, or add one in an empty part of the row.", Fix::openSections };

        const auto loop = getLoop (engine);

        if (loop.on)
        {
            const double barLength = engine.getBarLength();

            for (auto& sec : sections)
                if (loop.startBeat >= sec.startBar * barLength - 1.0e-6 && loop.endBeat <= sec.getEndBar() * barLength + 1.0e-6)
                    return { "The loop is in " + sec.getDisplayName() + ", which usually has no words. Click a verse or a chorus and choose Loop this section.", Fix::openSections };

            return { "The loop runs across more than one section. Click a section and choose Loop this section.", Fix::openSections };
        }

        return {};
    }

    if (step < stepWrite || step > stepSing)
        return {};

    if (! s.section.has_value())
        return { "Pick a section first (step 1): the line is written for it.", Fix::openSections };

    if (step == stepWrite)
    {
        if (s.line == nullptr && ! engine.getLyrics().isEmpty())
            return { "Your lyrics have " + juce::String ((int) engine.getLyrics().lines.size()) + (engine.getLyrics().lines.size() == 1 ? " line" : " lines")
                     + ", but none is sung in " + s.section->getDisplayName() + ", " + getBarsText (*s.section)
                     + ". On the Lyrics row, drag the bar above a line's words into the section, or type a new line.", Fix::none };

        return {};
    }

    if (s.line == nullptr)
        return { "Write a line first (step 2).", Fix::openLyrics };

    if (isStepDone (step, engine, progress))
        return {};

    juce::StringArray parts;
    auto fix = Fix::none;

    if (step == stepSing && ! progress.hasOutputDevice)
    {
        parts.add ("No audio output is chosen, so you won't hear the song. Choose one in Audio/MIDI Settings.");
        fix = Fix::audioSettings;
    }

    if (step == stepTap)
    {
        const int unplaced = countUnplaced (*s.line);

        if (s.line->syllables.size() < 2)
            parts.add ("Your line is a single syllable block. Write a longer line, with hyphens between syllables, so there's something to tap.");
        else if (unplaced > 0)
            parts.add (juce::String (unplaced) + " of the line's " + juce::String ((int) s.line->syllables.size())
                       + " syllables aren't placed yet. Tap the line again from its start.");

        const auto tip = getHyphenTip (*s.line);

        if (tip.isNotEmpty())
            parts.add (tip);
    }

    if (! hasSoundIn (engine, *s.section))
        parts.add ("Nothing plays in " + s.section->getDisplayName() + " yet, so there's nothing to tap or sing to. Add chords and a beat first (Help > First Song Exercise...), or turn on the metronome in the transport bar and tap to its click.");

    if (step == stepSing && progress.singAlongOpen)
        parts.add ("Sing along is open: press Space and sing when your line comes round.");

    return { parts.joinIntoString ("\n"), fix };
}

juce::String getDoneNote (int step, const Engine& engine, const Progress& progress)
{
    if (! isStepDone (step, engine, progress))
        return {};

    const auto s = getSituation (engine, progress);

    switch (step)
    {
        case stepSection:
            return s.section->getDisplayName() + ", " + getBarsText (*s.section) + ", is your section.";

        case stepWrite:
        {
            const int count = countSungSyllables (*s.line);
            const double beats = s.line->endBeat - s.line->startBeat;
            auto note = "\"" + getLineText (*s.line) + "\": " + juce::String (count) + (count == 1 ? " syllable" : " syllables") + " over " + formatNumber (beats) + " beats.";

            if (isLineFast (*s.line))
                note << " At " << formatNumber (engine.getTempo()) << " BPM that's fast to sing: give it more beats, or cut words.";

            return note;
        }

        case stepTap:
        {
            // The tip stays while a multi-syllable word is one block, though the step ticks from the timing.
            const auto tip = getHyphenTip (*s.line);
            return "All " + juce::String ((int) s.line->syllables.size()) + " syllables of your line are placed." + (tip.isNotEmpty() ? "\n" + tip : juce::String());
        }


        case stepSing:  return "The song played through your line with Sing along open.";
        default:        break;
    }

    return {};
}

juce::String getShowMeText (int step, const juce::String& sectionName)
{
    switch (step)
    {
        case stepSection:
            return sectionName.isNotEmpty() ? sectionName + ": click it and choose Loop this section." : juce::String ("Click here to add a section, then choose Verse or Chorus.");

        case stepWrite: return "Edit...: type your line, then press Apply lyrics to song.";
        case stepTap:   return "Your line. Press Tap this line and tap a letter key on each syllable, or drag a syllable to place it.";
        case stepSing:  return "Sing along (Cmd+5): your words, large, as the song plays.";
        default:        break;
    }

    return {};
}
} // namespace daw::gen::writelyrics
