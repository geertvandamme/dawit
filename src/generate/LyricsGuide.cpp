#include "LyricsGuide.h"
#include "Rhyme.h"
#include "SingAlong.h"
#include "engine/Tempo.h"

#include <map>

namespace daw::gen::lyricsguide
{
namespace
{
    /** The letters only, lower case: "Night," is "night". */
    juce::String lettersOf (const juce::String& text)
    {
        juce::String letters;
        const auto lower = text.toLowerCase();

        for (auto p = lower.getCharPointer(); ! p.isEmpty();)
        {
            const auto c = p.getAndAdvance();

            if (juce::CharacterFunctions::isLetter (c))
                letters += juce::String::charToString (c);
        }

        return letters;
    }

    bool isSung (SectionType type)
    {
        switch (type)
        {
            case SectionType::verse:
            case SectionType::preChorus:
            case SectionType::chorus:
            case SectionType::postChorus:
            case SectionType::bridge:
            case SectionType::phrase:     return true;
            case SectionType::intro:
            case SectionType::solo:
            case SectionType::interlude:
            case SectionType::breakdown:
            case SectionType::buildUp:
            case SectionType::drop:
            case SectionType::outro:      return false;
        }

        return false;
    }

    std::optional<SectionType> getHeadingType (const juce::String& heading)
    {
        const auto name = lettersOf (heading);

        if (name == "hook" || name == "refrain")
            return SectionType::chorus;

        for (auto type : sectionTypes::all)
            if (name.isNotEmpty() && (name == lettersOf (sectionTypes::getName (type)) || name == lettersOf (sectionTypes::getId (type))))
                return type;

        return std::nullopt;
    }

    /** The line's words with hyphens between the syllables of a split word: what `rhyme::countSyllables` reads. */
    juce::String getHyphenatedText (const LyricLine& line)
    {
        juce::String text;

        for (auto& s : line.syllables)
            text << s.text << (s.endsWord ? " " : "-");

        return text.trimEnd();
    }

    juce::String getLineText (const LyricLine& line)
    {
        juce::String text;

        for (auto& s : line.syllables)
            text << s.text << (s.endsWord ? " " : "");

        return text.trimEnd();
    }

    //==============================================================================
    // Building examples.

    Token word (const juce::String& text, int rhyme = -1, bool stressed = false)
    {
        Token t;
        t.text = text;
        t.rhyme = rhyme;
        t.stressed = stressed;
        return t;
    }

    /** A line of text whose last `rhymingWords` words rhyme as `rhyme`. */
    std::vector<Token> words (const juce::String& text, int rhyme = -1, int rhymingWords = 1)
    {
        std::vector<Token> tokens;
        const auto list = juce::StringArray::fromTokens (text, " ", "");

        for (int i = 0; i < list.size(); ++i)
            tokens.push_back (word (list[i], i >= list.size() - rhymingWords ? rhyme : -1));

        return tokens;
    }

    ExampleLine line (const juce::String& label, std::vector<Token> tokens, const juce::String& note = {}, int labelRhyme = -1)
    {
        ExampleLine l;
        l.label = label;
        l.tokens = std::move (tokens);
        l.note = note;
        l.labelRhyme = labelRhyme;
        return l;
    }

    ExampleLine sectionLine (SectionType type, const juce::String& text, const juce::String& note)
    {
        auto l = line (sectionTypes::getName (type), words (text), note);
        l.section = type;
        return l;
    }

    /** Syllables on beats: each is (text, beat, stressed); a stress off the strong beats, or a strong
        beat with a weak syllable, is marked wrong when `markWrong` is set. */
    std::vector<Token> onBeats (std::initializer_list<std::tuple<const char*, double, bool>> syllables, bool markWrong = false)
    {
        std::vector<Token> tokens;

        for (auto& [text, beat, stressed] : syllables)
        {
            Token t;
            t.text = juce::CharPointer_UTF8 (text);
            t.beat = beat;
            t.stressed = stressed;

            const bool strongBeat = std::abs (beat - 2.0 * std::round (beat / 2.0)) < 1.0e-6;
            t.wrong = markWrong && (stressed != strongBeat) && std::abs (beat - std::round (beat)) < 1.0e-6;
            tokens.push_back (t);
        }

        return tokens;
    }

    /** Syllables on melody notes: (text, beat, length, pitch, stressed); empty text holds the syllable before. */
    std::vector<Token> onNotes (std::initializer_list<std::tuple<const char*, double, double, int, bool>> notes)
    {
        std::vector<Token> tokens;

        for (auto& [text, beat, length, pitch, stressed] : notes)
        {
            Token t;
            t.text = juce::CharPointer_UTF8 (text);
            t.beat = beat;
            t.length = length;
            t.pitch = pitch;
            t.stressed = stressed;
            tokens.push_back (t);
        }

        return tokens;
    }

    std::vector<Token> chips (std::initializer_list<SectionType> types)
    {
        std::vector<Token> tokens;

        for (auto type : types)
        {
            Token t;
            t.text = sectionTypes::getName (type);
            t.section = type;
            tokens.push_back (t);
        }

        return tokens;
    }
}

//==============================================================================
juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::sections: return "What each part says";
        case Topic::lines:    return "Lines and phrases";
        case Topic::rhyme:    return "Rhyme";
        case Topic::findRhyme: return "Finding a rhyme";
        case Topic::stress:   return "Stress and the beat";
        case Topic::melody:   return "Words on a melody";
        case Topic::prosody:  return "Words and music agree";
        case Topic::hook:     return "The hook and the title";
        case Topic::craft:    return "Writing tips";
        case Topic::writeWithAi: return "Writing with AI";
        case Topic::forms:    return "Song forms";
        case Topic::singing:  return "Singing along";
        case Topic::computer: return "How a computer sings";
        case Topic::voices:   return "Getting a voice";
        case Topic::yours:    return "Your lyrics";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::sections: return "Verse, pre-chorus, chorus, bridge";
        case Topic::lines:    return "2- and 4-bar phrases, room to breathe";
        case Topic::rhyme:    return "Where rhymes go: AABB, ABAB, XAXA";
        case Topic::findRhyme: return "Perfect, family, assonance: from strong to loose";
        case Topic::stress:   return "Strong syllables on strong beats";
        case Topic::melody:   return "A note per syllable, long notes, pickups";
        case Topic::prosody:  return "The melody says what the words say";
        case Topic::hook:     return "The line everyone remembers";
        case Topic::craft:    return "Show, don't tell, and one idea";
        case Topic::writeWithAi: return "What the model does well, and what you still decide";
        case Topic::forms:    return "Verse-chorus, AABA, blues, folk";
        case Topic::singing:  return "Reading the words while you record";
        case Topic::computer: return "Sounds of words, notes and a voice";
        case Topic::voices:   return "Where to find one, how to add it, the terms";
        case Topic::yours:    return "Rhymes, syllables and gaps in yours";
    }

    return {};
}

//==============================================================================
Example getExample (Topic t, const Lyrics& lyrics)
{
    Example e;

    switch (t)
    {
        case Topic::sections:
            e.lines = { sectionLine (SectionType::verse, "I walked along the river late", "tells the story: who, where, what happened"),
                        sectionLine (SectionType::preChorus, "and every step was leading me", "builds up, leaning towards the chorus"),
                        sectionLine (SectionType::chorus, "hold on, hold on tonight", "the main idea and the hook, the same each time"),
                        sectionLine (SectionType::bridge, "but maybe it was never you", "a new angle, a twist, before the last chorus"),
                        sectionLine (SectionType::outro, "hold on...", "an echo of the hook, or no words at all") };
            break;

        case Topic::lines:
            e.kind = Kind::beats;
            e.endBeat = 8.0;
            e.lines = { line ("Line 1", onBeats ({ { "I", 0.0, false }, { "walked", 1.0, true }, { "a-", 2.0, false }, { "long", 2.5, true },
                                                   { "the", 3.0, false }, { "ri-", 4.0, true }, { "ver", 4.5, false }, { "late", 5.0, true } }),
                              "2 bars, then a breath"),
                        line ("Line 2", onBeats ({ { "the", 0.5, false }, { "ci-", 1.0, true }, { "ty", 1.5, false }, { "lights", 2.0, true },
                                                   { "be-", 3.0, false }, { "hind", 4.0, true }, { "me", 5.0, false } }),
                              "the same length, so it fits the same melody") };
            break;

        case Topic::rhyme:
            e.lines = { line ("X", words ("I walked along the river late"), "XAXA: lines 2 and 4 rhyme", -1),
                        line ("A", words ("the city lights behind me", 0, 2), {}, 0),
                        line ("X", words ("your voice still ringing in my head"), {}, -1),
                        line ("A", words ("the way it used to find me", 0, 2), {}, 0),
                        line ({}, {}),
                        line ("A", words ("hold on, hold on tonight", 0), "AABB: rhyming pairs", 0),
                        line ("A", words ("we could run before the light", 0), {}, 0),
                        line ("B", words ("hold on, don't let it go", 1), {}, 1),
                        line ("B", words ("everything we used to know", 1), {}, 1) };
            break;

        case Topic::findRhyme:
        {
            // One row per kind of rhyme; each word takes that kind's rhyme colour (perfect 0 ... consonance 5).
            auto row = [] (const juce::String& label, std::initializer_list<const char*> list, int kind, const juce::String& note)
            {
                std::vector<Token> tokens;

                for (auto* w : list)
                    tokens.push_back (word (w, kind));

                return line (label, std::move (tokens), note, kind);
            };

            e.lines = { row ("Perfect", { "night", "light", "polite" }, 0, "the vowel and all after it match; the sound before differs"),
                        row ("Identical", { "night", "knight", "tonight" }, 1, "the sound before matches too: it sounds like the same word"),
                        row ("Family", { "night", "ride", "hide" }, 2, "t and d: both plosives"),
                        row ("Additive", { "feel", "heels" }, 3, "an s (a z sound) added"),
                        row ("Assonance", { "night", "time", "mine" }, 4, "the same vowel, other consonants"),
                        row ("Consonance", { "night", "boat" }, 5, "only the t matches"),
                        row ("Two syllables", { "river", "shiver", "giver" }, 0, "the rhyme starts at the stressed syllable") };
            break;
        }

        case Topic::writeWithAi:
        {
            auto original = words ("I feel so sad and lonely now");
            for (auto& tk : original)
                tk.wrong = true;

            const auto same = "8 syllables, like the original" + juce::String::fromUTF8 (" \xc2\xb7 ");

            e.lines = { line ("X", words ("I walked along the river late"), "XAXA: lines 2 and 4 rhyme", -1),
                        line ("A", words ("the city lights behind me", 0, 2), {}, 0),
                        line ("X", std::move (original), "the original: 8 syllables", -1),
                        line ("A", words ("the way it used to find me", 0, 2), {}, 0),
                        line ({}, {}),
                        line ("1", words ("your coat still hangs beside the door"), same + "a picture instead of a feeling"),
                        line ("2", words ("two cups, and one is never full"), same + "a detail only this song has"),
                        line ("3", words ("the empty chair where you would sit"), same + "a place that says they've gone") };

            break;
        }

        case Topic::stress:
            e.kind = Kind::beats;
            e.endBeat = 8.0;
            e.lines = { line ("Right", onBeats ({ { "HOLD", 0.0, true }, { "me", 1.0, false }, { "CLOSE", 2.0, true }, { "and", 3.0, false },
                                                  { "DON'T", 4.0, true }, { "let", 5.0, false }, { "GO", 6.0, true } }, true),
                              "loud syllables on beats 1 and 3"),
                        line ("Wrong", onBeats ({ { "to", 0.0, false }, { "GE", 1.0, true }, { "ther", 2.0, false }, { "we", 3.0, false },
                                                  { "RUN", 4.0, true }, { "a", 5.0, false }, { "WAY", 6.0, true } }, true),
                              "sounds like to-ge-THER"),
                        line ("Fixed", onBeats ({ { "to", 1.0, false }, { "GE", 2.0, true }, { "ther", 3.0, false }, { "we", 3.5, false },
                                                  { "RUN", 4.0, true }, { "a", 5.0, false }, { "WAY", 6.0, true } }, true),
                              "start a beat later: GE lands on beat 3") };
            break;

        case Topic::melody:
            e.kind = Kind::melody;
            e.startBeat = -1.0;
            e.endBeat = 8.0;
            e.lines = { line ("A note each", onNotes ({ { "hold", 0.0, 1.0, 72, true }, { "on,", 1.0, 1.0, 72, false }, { "hold", 2.0, 1.0, 74, true },
                                                        { "on", 3.0, 1.0, 72, false }, { "to-", 4.0, 0.5, 69, false }, { "night", 4.5, 3.5, 67, true } }),
                              "a syllable per note; the long note on an open vowel"),
                        line ("Melisma", onNotes ({ { "oh", 0.0, 1.0, 72, true }, { "", 1.0, 1.0, 74, false }, { "", 2.0, 1.0, 76, false },
                                                    { "", 3.0, 1.0, 74, false }, { "yeah", 4.0, 4.0, 72, true } }),
                              "one syllable over several notes"),
                        line ("Pickup", onNotes ({ { "and", -1.0, 0.5, 67, false }, { "I", -0.5, 0.5, 69, false }, { "LOVE", 0.0, 2.0, 76, true },
                                                   { "you", 2.0, 1.0, 74, false }, { "so", 3.0, 1.0, 72, false }, { "", 4.0, 3.0, 72, false } }),
                              "words before the bar lead in; the key word gets the high note") };
            break;

        case Topic::prosody:
            e.lines = { line ("Rising", words ("we're climbing higher every day"), "the melody goes up with the words"),
                        line ("Falling", words ("and then you said goodbye"), "the melody falls, a minor chord under goodbye"),
                        line ("Pause", words ("... I'm still here"), "a beat of silence before the line that matters"),
                        line ("Question", words ("where do we go from here?"), "ends on the V chord: it doesn't feel finished") };
            break;

        case Topic::hook:
        {
            auto hookLine = [] (std::vector<Token> tokens) { for (size_t i = 0; i < 2 && i < tokens.size(); ++i) tokens[i].stressed = true; return tokens; };
            e.lines = { line ("Chorus", hookLine (words ("hold on, hold on tonight")), "the hook opens the chorus..."),
                        line ({}, words ("we could run before the light")),
                        line ({}, hookLine (words ("hold on, don't let it go"))),
                        line ({}, words ("everything we used to know"), "...and the title is the hook: Hold On") };
            e.lines[0].section = SectionType::chorus;
            break;
        }

        case Topic::craft:
        {
            auto told = [] (const juce::String& text) { auto tokens = words (text); for (auto& tk : tokens) tk.wrong = true; return tokens; };
            e.lines = { line ("Tells", told ("I feel so sad and lonely"), "a feeling named"),
                        line ("Shows", words ("two cups on the table, one still full"), "a picture: the listener feels it"),
                        line ("Tells", told ("I miss you every day"), {}),
                        line ("Shows", words ("your coat still hangs behind the door"), "a detail only this song has") };
            break;
        }

        case Topic::forms:
            e.lines = { line ("Verse-chorus", chips ({ SectionType::verse, SectionType::chorus, SectionType::verse, SectionType::chorus,
                                                        SectionType::bridge, SectionType::chorus }), "pop and rock"),
                        line ("With a pre-chorus", chips ({ SectionType::verse, SectionType::preChorus, SectionType::chorus, SectionType::verse,
                                                             SectionType::preChorus, SectionType::chorus }), "modern pop"),
                        line ("AABA", words ("A  A  B  A"), "standards: the title in each A"),
                        line ("12-bar blues", words ("line,  the same line,  the answer"), "AAB in every verse"),
                        line ("Strophic", chips ({ SectionType::verse, SectionType::verse, SectionType::verse, SectionType::verse }),
                              "folk: new words, the same music") };
            break;

        case Topic::singing:
            e.kind = Kind::beats;
            e.startBeat = -4.0;
            e.endBeat = 8.0;
            e.lines = { line ("Count-in", onBeats ({ { "4", -4.0, false }, { "3", -3.0, false }, { "2", -2.0, false }, { "1", -1.0, false } }),
                              "after a rest the window counts down: breathe in"),
                        line ("Line", onBeats ({ { "hold", 0.0, true }, { "on,", 1.0, false }, { "hold", 2.0, true }, { "on", 3.0, false },
                                                 { "to", 4.0, false }, { "the", 4.5, false }, { "sound", 5.0, true } }),
                              "the colour fills each syllable while it's sung"),
                        line ("Pickup", onBeats ({ { "and", -0.5, false }, { "we", 0.0, true }, { "will", 1.0, false }, { "sing", 2.0, true },
                                                   { "un-", 3.0, false }, { "til", 3.5, false }, { "the", 4.0, false }, { "light", 5.0, true } }),
                              "starts before the bar: come in with the colour") };
            break;

        case Topic::computer:
        {
            // "feel the night" at 120 BPM: a beat is 0.5 s, so a second is 2 beats. The consonants last 0.08 s (0.16 beat).
            auto sound = [] (const char* symbol, double startSeconds, double endSeconds, bool vowel)
            {
                Sound s;
                s.symbol = symbol;
                s.start = startSeconds * 2.0;
                s.length = (endSeconds - startSeconds) * 2.0;
                s.vowel = vowel;
                return s;
            };

            auto note = [] (const char* text, double beat, double length, int pitch, std::vector<Sound> sounds)
            {
                Token token;
                token.text = text;
                token.beat = beat;
                token.length = length;
                token.pitch = pitch;
                token.sounds = std::move (sounds);
                return token;
            };

            e.kind = Kind::sounds;
            e.startBeat = -0.5;
            e.endBeat = 4.0;
            e.lines = { line ("Sung", { note ("feel", 0.0, 1.0, 62, { sound ("f", -0.08, 0.0, false), sound ("iy", 0.0, 0.34, true), sound ("l", 0.34, 0.42, false) }),
                                        note ("the", 1.0, 1.0, 60, { sound ("dh", 0.42, 0.50, false), sound ("ah", 0.50, 0.92, true) }),
                                        note ("night", 2.0, 2.0, 64, { sound ("n", 0.92, 1.00, false), sound ("ay", 1.00, 1.92, true), sound ("t", 1.92, 2.00, false) }) },
                              "the vowels land on the beats; the consonants slip in just before") };
            break;
        }

        case Topic::voices:
            e.lines = { line ("1  Get", words ("download a voice from its maker's page: a .zip"), "Get a voice... in the Sing window opens the page"),
                        line ("2  Add", words ("Add a voice... and pick the .zip or its folder"), "or put the folder in the Voices folder yourself"),
                        line ("3  Check", words ("the voice shows in the Voice menu; Sing lights up"), "if not, the window names the file that is missing"),
                        line ("4  Terms", words ("Terms... shows the maker's rules: credit them"), "read them before you release a song") };
            break;

        case Topic::yours:
        {
            int shown = 0;

            for (auto& verse : analyse (lyrics))
            {
                if (shown >= 13)   // room for the heading and a line at least
                    break;

                auto heading = line (verse.heading, {}, verse.scheme.isNotEmpty() ? "rhymes " + verse.scheme : juce::String());
                heading.section = getHeadingType (verse.heading);
                e.lines.push_back (heading);
                ++shown;

                for (auto& l : verse.lines)
                {
                    if (shown >= 14)
                        break;

                    const int rhyme = l.rhyme == "X" || l.rhyme.isEmpty() ? -1 : (int) (l.rhyme[0] - 'A');
                    auto tokens = words (l.text, rhyme);
                    e.lines.push_back (line (l.rhyme, std::move (tokens),
                                             juce::String (l.syllables) + (l.syllables == 1 ? " syllable" : " syllables")
                                                 + juce::String::fromUTF8 (" \xc2\xb7 ") + juce::String (l.beats, 1).trimCharactersAtEnd ("0").trimCharactersAtEnd (".")
                                                 + " beats",
                                             rhyme));
                    ++shown;
                }
            }

            break;
        }
    }

    return e;
}

//==============================================================================
juce::String explain (Topic t, const Lyrics& lyrics, const SectionList& sections, double bpm)
{
    switch (t)
    {
        case Topic::sections:
            return "Each part of a song has its own job, and the words follow it.\n\n"
                   "The verse tells the story: who, where, what happened. Every verse has new words and moves the story on, "
                   "with details that let the listener see it. Verses are usually calmer and lower, so there's room to listen.\n\n"
                   "The pre-chorus (not every song has one) builds tension: shorter lines, words that lean forward, a melody that climbs "
                   "towards the chorus.\n\n"
                   "The chorus says what the song is about, in a few simple, singable lines, and it comes back with the same words "
                   "each time, so the listener can sing along by the second time. The title usually sits in its first or last line.\n\n"
                   "The bridge comes once, usually after the second chorus: a new angle, a question, a twist or a confession, often over "
                   "new chords, so the last chorus feels fresh again.\n\n"
                   "Intros, solos, breakdowns and drops usually have no words; an outro may repeat the hook as the song fades.\n\n"
                   "In DawIt: put [Verse], [Chorus] or [Bridge] above a verse when you type the lyrics, and it lands on that section "
                   "of the Sections row. A chorus typed once goes on every chorus.";

        case Topic::lines:
            return "Lyrics are written in lines, and a line is sung over a phrase of the music: usually 2 bars, sometimes 1 or 4. "
                   "Phrases come in pairs and fours, so a verse is usually 4 or 8 lines, like the 4- and 8-bar sections it's sung over.\n\n"
                   "Leave room to breathe. A line usually ends before its phrase does, so the singer can take a breath and the band has "
                   "a moment to answer. A line that fills every beat of its bars sounds rushed.\n\n"
                   "Lines sung to the same melody should have about the same number of syllables, with the stresses in the same places. "
                   "That's why the second verse can reuse the melody of the first: its lines are built the same way.\n\n"
                   "Count syllables, not words: \"together\" is three (to-ge-ther), \"night\" is one. In DawIt, type hyphens between "
                   "syllables, and each one gets its own place in time. The picture shows two lines of a verse, each over 2 bars, "
                   "sung in the first bar and a half.";

        case Topic::rhyme:
            return "Rhymes are words that end in the same sound: night and light, behind me and find me. They make lines easier to "
                   "remember and tie them together, and the ear starts to wait for them.\n\n"
                   "Rhymes usually go at the ends of lines, and the pattern is written with letters: lines that rhyme share a letter, "
                   "and X marks a line that doesn't rhyme.\n"
                   "  XAXA: only lines 2 and 4 rhyme. The most common in pop and folk; it leaves room for the story.\n"
                   "  AABB: rhyming pairs (couplets). Punchy and simple, often in choruses.\n"
                   "  ABAB: alternating. A little more literary.\n"
                   "  ABCB: like XAXA, with lines 1 and 3 free.\n\n"
                   "A perfect rhyme matches the last stressed vowel and everything after it, and starts with a different sound "
                   "(night / light, behind me / find me). The Finding a rhyme topic shows the looser kinds. A near rhyme only comes close "
                   "(home / alone, mind / time), and modern songs use them all the time: they sound natural and give you many more words "
                   "to choose from. Internal rhymes sit inside a line (\"the rain in my brain\"), common in rap and fast verses.\n\n"
                   "Don't let the rhyme choose the words: if a line only exists to rhyme, the listener hears it. Change the other line "
                   "instead, or use a near rhyme.";

        case Topic::findRhyme:
            return "Rhyme is about sound, not spelling. Night and bite rhyme, though they're spelt differently. Cough and though are spelt alike and don't.\n\n"
                   "What has to match: find the last stressed syllable of the word, the one you say louder. A perfect rhyme matches that "
                   "syllable's vowel and every sound after it, and starts with a different sound. Night / light, polite / night, river / shiver "
                   "and behind me / find me are all perfect. When the sound before the vowel matches too, it's an identical rhyme: night / knight, "
                   "night / tonight, leave / believe. It sounds like the same word said twice, so it lands weakly.\n\n"
                   "Rhymes from strong to loose. Songwriting teachers such as Pat Pattison of Berklee rank rhymes from the most settled to the least:\n"
                   "  Perfect: night / light.\n"
                   "  Family: the same vowel, with the last consonant swapped for one of the same family: night / ride. T and d are both plosives, "
                   "sounds made by stopping the air.\n"
                   "  Additive and subtractive: the same vowel, with a consonant added or taken away: feel / heels.\n"
                   "  Assonance: the same vowel, with consonants from another family: night / time, home / coat.\n"
                   "  Consonance: different vowels, the same consonants after them: night / boat.\n\n"
                   "The families, as songwriters group them, are these. The plosives stop the air: p, b, t, d, k, g. The fricatives hiss through a gap: "
                   "f, v, th, s, z, sh, the s of measure, ch and j (phonetics calls ch and j affricates: a stop, then a hiss). The nasals hum through the "
                   "nose: m, n, ng. L, r, w and y are in none of the three.\n\n"
                   "Use the scale. A perfect rhyme closes a thought, so it suits the last line of a chorus. A looser rhyme leaves it open, which suits a "
                   "question or a line that leads on. Near rhymes aren't mistakes. Modern songs use them all the time, and they give you many more "
                   "words to choose from.\n\n"
                   "In DawIt, select a word in the Lyrics window and click Rhymes. DawIt looks the word up in the CMU Pronouncing Dictionary: about "
                   "126,000 American English words, with the stressed vowels marked. It lists the perfect rhymes by syllables, then the family rhymes. "
                   "The dictionary is full of names and rare words, so with Ollama running the AI also picks the rhymes that suit your line, and DawIt "
                   "labels each one by its sound. A word the dictionary doesn't know, such as a name or a made-up word, gets rhymes from its spelling, "
                   "marked as guesses.\n\n"
                   "What it can't hear: your accent. The dictionary is American English, and some words rhyme in one accent and not another. "
                   "Spaghetti and already rhyme in most American accents, for example. Say the pair out loud in your own voice before you keep it.";

        case Topic::writeWithAi:
            return "The Write with AI panel in the Lyrics window asks a language model for words. The model runs on your own computer through Ollama, "
                   "so your lyrics never leave it. It can write the whole song for your sections, add or rewrite a section, write the next line, "
                   "rewrite a few words, or find rhymes. It's for getting unstuck and hearing other ways to say a line. It doesn't replace your ear.\n\n"
                   "How it works: a language model writes the next likely words, one after another, from what it learnt reading a great deal of text. "
                   "So it is good at fluent, grammatical lines, and its first idea is often the most familiar one: hearts that fall apart, fire and "
                   "desire. Ask for something specific. More concrete swaps a named feeling for a picture. Your words (\"make it about the night bus "
                   "home\") gives it details only your song has. The more of the song it can read, the better it follows your story, point of view "
                   "and rhyme scheme.\n\n"
                   "What DawIt checks, and why: models are poor at counting syllables, because they read words as chunks of text, not sounds. In a "
                   "2024 test called PhonologyBench, people counted the syllables of a sentence right 90% of the time. The best model managed 55%, and "
                   "open models such as Mistral 7B, the size Ollama runs on a laptop, about 7%. So DawIt counts every suggestion itself with the "
                   "pronouncing dictionary and shows it against the line it replaces (\"8 syllables, like the original\"). It also says which line it "
                   "rhymes with. A line with the same syllables and stresses fits the same melody; a line 3 syllables longer has to be squeezed in.\n\n"
                   "Getting the best from it:\n"
                   "  Write the brief first: what the song is about, the mood, and who is singing to whom.\n"
                   "  Keep the lines you like and ask about the ones that don't work yet. Three alternatives for one line teach you more than a whole "
                   "song written for you.\n"
                   "  Mix and edit: take the picture from one suggestion and the rhyme from another, then make it sound like you.\n"
                   "  Sing every line out loud before you keep it.\n\n"
                   "Who owns the words: in the United States, the Copyright Office said in January 2025 that prompts alone don't make a model's text "
                   "yours to protect. The parts you write, and what you choose, arrange and change, can be. Using AI as a tool doesn't stop a song "
                   "from being protected, and each case is judged on its own. Other countries have their own rules. This isn't legal advice: if you "
                   "release a song, keep the lyrics mostly your own and check the rules where you live.\n\n"
                   "Lines it may have learnt: a model can repeat lyrics it saw while it was trained. In November 2025 a court in Munich found that "
                   "ChatGPT had memorised song lyrics and reproduced them nearly word for word (the ruling isn't final: OpenAI said it would appeal). DawIt asks for new "
                   "lines, but if a suggestion sounds familiar, search for it before you use it.\n\n"
                   "What it doesn't do: it doesn't write words to a melody's notes yet, and its syllable and rhyme checks work for English only.";

        case Topic::stress:
            return "Every word of two or more syllables has a stressed syllable, the one you say louder: to-GE-ther, HAP-py, a-WAY. "
                   "Short words like the, a, and, to and of are usually weak; words that carry meaning (nouns, verbs) are usually strong.\n\n"
                   "Music has strong and weak beats too. In 4/4, beat 1 is the strongest and beat 3 the next; beats 2 and 4 are weaker, "
                   "and the off-beats (the \"and\"s) weaker still.\n\n"
                   "Good lyrics put stressed syllables on strong beats and weak syllables in between, so the words sound the way you'd "
                   "say them. When a stressed syllable falls on a weak beat, or a weak one on a strong beat, the word comes out wrong: "
                   "to-ge-THER. Listeners notice, even if they can't say why.\n\n"
                   "Fix it by moving the line (start it a beat or half a beat later), by choosing another word with the stress in the "
                   "right place, or by changing the melody's rhythm. In the picture, the loud syllables are in capitals and the strong "
                   "beats are shaded; red marks a syllable on the wrong kind of beat.\n\n"
                   "Breaking the rule on purpose is a style of its own (syncopation in R&B and rap), but it works because the singer "
                   "means it.";

        case Topic::melody:
            return "Most pop verses are sung a syllable to a note (syllabic): each syllable gets its own note, so the words are clear. "
                   "Holding one syllable over several notes is a melisma: the runs of soul, gospel and R&B singers, or a held \"oh\" in "
                   "a chorus. Use melismas on open vowels, not in the middle of a word the listener needs to understand.\n\n"
                   "Long notes want open vowels that are easy to hold: oh, ah, ay (day), ee (free), i (night). A long note on a closed "
                   "sound (the \"t\" of \"it\", the \"ng\" of \"sing\") is hard to sing and to hear.\n\n"
                   "The most important word of a line goes where the melody gives it weight: on the downbeat, on the highest note, or on "
                   "the longest one. A melody's peak on \"the\" wastes it.\n\n"
                   "A pickup (anacrusis) is one or more weak syllables before the bar line, such as \"and I...\", leading into the stressed "
                   "word on beat 1. It's how most lines that start with a small word get their stress on the beat.\n\n"
                   "Write the melody's rhythm the way the words would be spoken, then exaggerate it: speak the line out loud with the "
                   "beat and you'll often hear its rhythm. In the picture, notes are drawn as in the piano roll, with the syllable sung "
                   "on each; a line under a note means the syllable before is held.";

        case Topic::prosody:
            return "Prosody is the words and the music saying the same thing. When they do, a song feels true; when they don't, it "
                   "feels off, even if every part is fine on its own.\n\n"
                   "  A melody that rises on hopeful words, and falls on sad or final ones.\n"
                   "  Major chords under happy lines and minor ones under sad lines, or on purpose the other way round, for a bittersweet "
                   "song.\n"
                   "  A line that asks a question ending on a chord that doesn't feel finished, such as the V chord, and the answer "
                   "landing on the I.\n"
                   "  A pause before the line that matters, or a line that's suddenly shorter.\n"
                   "  A restless verse with short, broken lines, and a settled chorus with long ones.\n\n"
                   "Listen to your song with the words in mind: does the music say what the line says? The chord guide's chord families "
                   "(home, away, tension) are a good way to check which feeling a chord gives.";

        case Topic::hook:
            return "The hook is the part people remember and sing in the car: usually a short line of a few words, repeated, in the "
                   "chorus, on the melody's catchiest phrase.\n\n"
                   "In most songs the hook is the title. It sits at the start or the end of the chorus (often both), where the melody "
                   "is highest or most rhythmic, and it comes back every chorus.\n\n"
                   "A good hook is short (two to five words), easy to sing (open vowels, a clear rhythm), and says the song's idea in "
                   "a phrase: Hold On, Let It Go, Someone Like You. Everything else in the song sets it up: the verses tell the story "
                   "that makes the hook mean something.\n\n"
                   "Tip: write the hook first, then ask what story leads to it. That story is your verses.";

        case Topic::craft:
            return "Show, don't tell. \"I feel so sad\" names a feeling; \"two cups on the table, one still full\" lets the listener "
                   "feel it. Concrete things (a coat, a street, a time of day, a smell) make a song vivid and personal.\n\n"
                   "One idea per song. Everything in the lyrics should serve one feeling or story; a second idea belongs in another song.\n\n"
                   "Pick a point of view and keep it: \"I\" (a confession), \"you\" (singing to someone), or \"he\" and \"she\" "
                   "(telling a story). Keep the tense steady too.\n\n"
                   "Contrast the verse and the chorus: detail against the big picture, lower against higher, talking against singing.\n\n"
                   "Use the words people speak. Lyrics are sung once at speed, not read twice: simple words, said naturally, win over "
                   "clever ones.\n\n"
                   "Rewrite. First drafts are for getting it down; then cut every word that doesn't do anything, check the stresses "
                   "against the beat, and sing it out loud. If a word is hard to sing, change it.";

        case Topic::forms:
            return "The song's form decides where the words repeat.\n\n"
                   "Verse-chorus (pop, rock, country): verses with new words that tell the story, a chorus with the same words every "
                   "time, often a bridge before the last chorus.\n\n"
                   "Verse, pre-chorus, chorus (modern pop): the pre-chorus builds between them, so the chorus lands harder.\n\n"
                   "AABA, the 32-bar form of jazz standards and old pop: three A sections with the same melody and new words, the "
                   "title in the first or last line of each A, and a B section (the bridge, or middle eight) with a different melody "
                   "and a new angle.\n\n"
                   "The 12-bar blues: each verse is three lines, AAB: a line, the same line again (sometimes a little changed), and "
                   "an answer line that rhymes with it and resolves it. \"Woke up this morning, my baby was gone / Woke up this "
                   "morning, my baby was gone / ...\"\n\n"
                   "Strophic (folk songs, hymns, ballads): every verse has the same music and new words, often with a refrain, a "
                   "short line that comes back at the end of each verse.\n\n"
                   "The song structure guide (click Sections) shows these forms and more, and puts one on the Sections row.";

        case Topic::singing:
            return singalong::explainSingingAlong (lyrics, bpm);

        case Topic::computer:
            return "A singing voice synthesiser turns three things into a sung vocal: the words, the notes and a voice. Sing this... uses "
                   "DiffSinger, an open-source singing synthesiser, with a voice you add.\n\n"
                   "First the words become phonemes. A phoneme is one sound of a language: \"night\" has five letters but three sounds, n, ay "
                   "and t. English spelling is a poor guide to sound: though, through and tough all end in \"ough\" and sound different. So "
                   "DawIt looks every word up in a pronouncing dictionary, the CMU Pronouncing Dictionary, which has about 135,000 "
                   "entries for about 126,000 English words, using 39 sounds: 15 vowels and 24 consonants. A word it can't find is guessed from its spelling, and the "
                   "Sing window lists it so you can check how it sounds.\n\n"
                   "Every syllable has one vowel sound at its centre, and in singing the vowel carries the note. \"Together\" has three vowel "
                   "sounds, so it takes three notes, one for each syllable (a syllable can also be held over several notes). Consonants are short noises around the vowel. Singers start the vowel "
                   "on the beat and slip the consonant in just before it, so the word lands on time. Choirs are taught the same rule: "
                   "consonants before the beat, vowels on the beat. DawIt places the sounds this way. In the picture, \"feel the night\" is "
                   "sung at 120 BPM. The f of \"feel\" starts just before beat 1, and the n of \"night\" is sung at the end of the note before "
                   "it, so the \"ay\" starts exactly on beat 3.\n\n"
                   "This is why the vowel gets nearly all of a long note. At 120 BPM a beat lasts half a second, so \"night\" on a 2-beat note lasts 1 "
                   "second. If the t takes about a tenth of that, the \"ay\" gets about 0.9 seconds. A word packed with consonants on a short "
                   "note, such as \"strengths\" on a 16th note, leaves almost no time for its vowel and sounds rushed, just as it does when a "
                   "person sings it.\n\n"
                   "The voice is a model trained on recordings of one singer, often a few hours of them, called a voicebank. Its tone and the "
                   "range where it sounds best come from that singer: the voice DawIt suggests, Hoshino Hanami, sings best from about F3 to "
                   "A5. Several models work in turn. One decides how long each sound lasts. One draws the pitch curve with the slides and "
                   "vibrato a singer adds, unless you choose Exact notes. One paints the sound as a spectrogram, a picture of how much energy "
                   "there is at each frequency, from low to high, about 86 times a second for most voices. Last, a vocoder turns that picture into audio. DiffSinger "
                   "paints the spectrogram by starting from noise and removing it step by step. More steps take longer and can sound cleaner.\n\n"
                   "What it doesn't do: it sings the notes and words you give it, and nothing more. It doesn't write the melody or the lyrics, "
                   "it adds breaths only in the rests, and it won't act the song the way a singer would. Every voice comes with its maker's "
                   "terms. Many let you use it in your own released songs with credit; some ask permission or a fee first. Read them before "
                   "you publish, and credit the voice. Only use voices whose singer agreed to become a voicebank: DiffSinger's makers ask that "
                   "nobody's voice is made without their consent.";

        case Topic::voices:
            return "Sing this needs a voice, and DawIt doesn't come with one. A voice is a folder of files made from recordings of one singer "
                   "who agreed to it, and it belongs to its maker, who sets the terms. Voices are big too: the one DawIt suggests is about "
                   "570 MB unpacked. So you download one yourself, once, and add it. It then stays on your computer, and nothing you sing leaves it.\n\n"
                   "1. Get one. Click Get a voice... in the Sing window. DawIt suggests Hoshino Hanami (\"AI-dol\" for DiffSinger), by Lotte V: a "
                   "pop soprano that sings English (about 2 hours of English recordings), among other languages. Her page links to the "
                   "download, a .zip file; download it only from the maker's own page. Sing this sings English only for now, so the "
                   "voice must have English in it. Other English voices exist, each with its own terms. In short, as community wikis summed "
                   "them up in October 2026: Canary, Miyo, TIGER and Peiton DS ask for permission or a bought licence before commercial use; "
                   "Laru Mine asks for permission and credit, and forbids passing the files on; Azure Cobalt's terms vary by release. These are "
                   "summaries written by volunteers, and terms change, so read each maker's own terms before you use a voice. The DiffSinger "
                   "wiki lists the voices (Get a voice... opens it too).\n\n"
                   "2. Add it. Click Add a voice... and pick the .zip, or the folder if you unpacked it. DawIt unpacks a copy into its Voices "
                   "folder (on a Mac: ~/Library/Application Support/DawIt/Voices; in the Voice menu, Show in Finder opens it) and checks it. You "
                   "can also put the unpacked voice folder there yourself, and reopen the Sing window. A voice that came with a separate "
                   "vocoder (a .oudep file) is added the same way.\n\n"
                   "3. What should be in it. A voice in OpenUtau's layout has a dsconfig.yaml (its settings), an acoustic model (an .onnx file "
                   "that paints the sound), a list of its phonemes, and a vocoder: usually an .onnx file in a dsvocoder folder, with a vocoder.yaml; "
                   "a voice that uses a shared vocoder only names it, and that vocoder is added separately. "
                   "Many also have dsdur and dspitch folders: models that decide how long each sound lasts and draw the pitch. A voice may be "
                   "inside one or two extra folders, as zips often are; DawIt finds it.\n\n"
                   "4. How to tell it worked. The voice's name appears in the Voice menu, the line \"Add a voice to sing this\" goes away and "
                   "the Sing button lights up. Sing a short clip first: a voice takes a while to sing. On the Apple silicon Mac it was tested on, "
                   "Good took a little less time than the clip lasts; a slower computer takes longer.\n\n"
                   "5. The terms, in plain words. Terms... opens what the maker wrote. Hanami's say: credit Lotte V when you use the voice; you "
                   "may use it in commercial and non-commercial songs without asking; you may not upload the voice's files elsewhere or sell "
                   "them, or claim the voice as your own; no political or hateful use; and no use for anything illegal. To credit, name the "
                   "voice and its maker, for example in your song's description. One catch: her vocoder file is licensed CC BY-NC-SA 4.0, a "
                   "non-commercial licence. Her vocoder is built from OpenVPI's, and a maintainer of OpenVPI has written that the same licence "
                   "on their vocoder covers passing on and changing the model files, not the sound that comes out of them. That is one "
                   "person's reading, not legal advice, and it isn't Lotte V's. If you plan to release a song commercially, read both and ask the maker.\n\n"
                   "6. If it doesn't work. DawIt names the file. \"dsconfig.yaml is missing\": you picked the wrong folder or a download that isn't "
                   "a voice for OpenUtau; pick the folder that has dsconfig.yaml in it. \"acoustic.onnx is missing\" (or another .onnx file): "
                   "the download is incomplete or a file was left out; unpack the .zip again in full. \"The vocoder is missing\": add the "
                   "voice's vocoder, usually a .oudep file from the same page. \"... has no English\": choose a voice that lists English. "
                   "\"... asks for an input called ..., which this version of DawIt doesn't know how to give it\": the voice was made with a newer or different DiffSinger export than this version "
                   "of DawIt understands. The voice stays in the list only when the problem is the vocoder; for any other problem, "
                   "DawIt removes the copy it made.";

        case Topic::yours:
        {
            if (lyrics.isEmpty())
                return "There are no lyrics yet. Double-click the Lyrics row (or click Edit... in its header) and type them: one sung "
                       "line per line, an empty line between verses, [Verse] or [Chorus] above a verse, and hyphens between syllables "
                       "(ri-ver, to-ge-ther). Then come back here: this page shows each verse's rhyme scheme and how many syllables "
                       "each line has, and points at lines that may be hard to sing.";

            const auto verses = analyse (lyrics);
            juce::String text;
            int lineCount = 0;

            for (auto& v : verses)
                lineCount += (int) v.lines.size();

            text << "Your lyrics have " << lineCount << (lineCount == 1 ? " line" : " lines") << " in " << (int) verses.size()
                 << ((int) verses.size() == 1 ? " verse.\n\n" : " verses.\n\n");

            juce::StringArray notes;

            for (auto& v : verses)
            {
                int fewest = std::numeric_limits<int>::max(), most = 0;

                for (auto& l : v.lines)
                {
                    fewest = juce::jmin (fewest, l.syllables);
                    most = juce::jmax (most, l.syllables);
                }

                text << v.heading << ": " << (int) v.lines.size() << ((int) v.lines.size() == 1 ? " line" : " lines");

                if (v.lines.size() > 1)
                    text << ", rhyme scheme " << v.scheme;

                text << ", " << (fewest == most ? juce::String (most) : juce::String (fewest) + " to " + juce::String (most))
                     << (most == 1 ? " syllable" : " syllables") << " a line.\n";

                if (v.lines.size() >= 3 && most - fewest >= 5)
                    notes.add ("In " + v.heading + ", the lines run from " + juce::String (fewest) + " to " + juce::String (most)
                               + " syllables. Lines sung to the same melody usually have about the same number.");

                if (v.lines.size() >= 3 && v.scheme.containsOnly ("X"))
                    notes.add (v.heading + " has no rhymes at the ends of its lines. That suits some styles, but rhymes make lines "
                               "easier to remember: try making lines 2 and 4 rhyme.");

                for (auto& l : v.lines)
                {
                    const double perBeat = l.beats > 0.0 ? (double) l.syllables / l.beats : 0.0;

                    if (perBeat > 2.5)
                        notes.add (juce::String::fromUTF8 ("\xe2\x80\x9c") + l.text + juce::String::fromUTF8 ("\xe2\x80\x9d has ")
                                   + juce::String (l.syllables) + " syllables in " + juce::String (l.beats, 1).trimCharactersAtEnd ("0").trimCharactersAtEnd (".")
                                   + " beats: at " + formatBpm (bpm) + " BPM that's fast to sing. Give it more beats, or cut words.");
                }
            }

            // Choruses usually repeat their words.
            const VerseReport* firstChorus = nullptr;

            for (auto& v : verses)
            {
                if (getHeadingType (v.heading) != SectionType::chorus)
                    continue;

                auto same = [] (const VerseReport& a, const VerseReport& b)
                {
                    if (a.lines.size() != b.lines.size())
                        return false;

                    for (size_t i = 0; i < a.lines.size(); ++i)
                        if (lettersOf (a.lines[i].text) != lettersOf (b.lines[i].text))
                            return false;

                    return true;
                };

                if (firstChorus == nullptr)
                    firstChorus = &v;
                else if (! same (*firstChorus, v))
                {
                    notes.add ("Not every chorus has the same words. Usually the chorus repeats them, so listeners can sing along; a last "
                               "chorus that changes a line on purpose is fine.");
                    break;
                }
            }

            // Sung sections without words.
            for (auto& s : sections)
            {
                if (! isSung (s.type))
                    continue;

                const double start = s.startBar * 4.0, end = s.getEndBar() * 4.0;
                const bool hasWords = std::any_of (lyrics.lines.begin(), lyrics.lines.end(),
                                                   [&] (const LyricLine& l) { return l.startBeat < end && l.endBeat > start; });

                if (! hasWords)
                    notes.add ("The " + s.getDisplayName() + " at bar " + juce::String (s.startBar + 1) + " has no words yet.");
            }

            // Without hyphens, long words count as one syllable.
            bool anyHyphen = false, anyLongWord = false;

            for (auto& l : lyrics.lines)
            {
                for (auto& s : l.syllables)
                {
                    anyHyphen = anyHyphen || ! s.endsWord;
                    anyLongWord = anyLongWord || lettersOf (s.text).length() >= 7;
                }
            }

            if (! anyHyphen && anyLongWord)
                notes.add ("None of your words are split into syllables, so each word gets one place in time. Type hyphens (ri-ver, "
                           "to-ge-ther) to give each syllable its own.");

            if (! notes.isEmpty())
                text << "\nThings to look at:\n  " << notes.joinIntoString ("\n  ") << "\n";

            text << "\nRhymes and syllables are counted by sound with the pronouncing dictionary (American English; a word it lacks is guessed from its spelling). "
                    "The rhyme scheme counts only perfect rhymes, so trust your ear: near rhymes such as home and alone count as rhymes in songs too.";
            return text;
        }
    }

    return {};
}

//==============================================================================
juce::String getRhymeKey (const juce::String& wordText)
{
    const auto w = lettersOf (wordText);

    if (w.isEmpty())
        return {};

    auto isVowel = [&w] (int i) { return juce::String ("aeiou").containsChar (w[i]) || (w[i] == 'y' && i > 0); };

    // Where the last group of vowels before `from` starts.
    auto groupStart = [&] (int from)
    {
        int i = from;

        while (i >= 0 && ! isVowel (i))
            --i;

        if (i < 0)
            return -1;

        while (i > 0 && isVowel (i - 1))
            --i;

        return i;
    };

    const int last = w.length() - 1;
    int start = groupStart (last);

    if (start < 0)
        return w;

    // A silent e: "late" and "time" rhyme on the vowel before it.
    if (start == last && w[last] == 'e' && last > 0)
        if (const int before = groupStart (last - 1); before >= 0)
            start = before;

    auto key = w.substring (start);

    // Spellings that sound the same.
    if (key.endsWith ("ight"))
        key = key.dropLastCharacters (4) + "ite";
    else if (key == "ow" || key == "oe")
        key = "o";
    else if (key == "ee" || key == "ea")
        key = "e";

    return key;
}

bool rhymes (const juce::String& wordText, const juce::String& other)
{
    // By sound where the dictionary has both words; the same word again counts (repetition).
    if (const auto kind = rhyme::classify (wordText, other); kind != rhyme::Kind::unknown)
        return kind == rhyme::Kind::perfect || kind == rhyme::Kind::identical;

    const auto a = getRhymeKey (wordText), b = getRhymeKey (other);
    return a.isNotEmpty() && a == b;
}

juce::String findRhymeScheme (const juce::StringArray& lastWords)
{
    // Group the lines that rhyme, then letter the groups of two or more in order.
    std::vector<int> group ((size_t) lastWords.size(), -1);
    int groups = 0;

    for (int i = 0; i < lastWords.size(); ++i)
    {
        for (int j = 0; j < i && group[(size_t) i] < 0; ++j)
            if (rhymes (lastWords[i], lastWords[j]))
                group[(size_t) i] = group[(size_t) j];

        if (group[(size_t) i] < 0)
            group[(size_t) i] = groups++;
    }

    std::vector<int> size ((size_t) groups, 0), letter ((size_t) groups, -1);

    for (int g : group)
        ++size[(size_t) g];

    juce::String scheme;
    int nextLetter = 0;

    for (int g : group)
    {
        if (size[(size_t) g] < 2)
        {
            scheme << "X";
            continue;
        }

        if (letter[(size_t) g] < 0)
            letter[(size_t) g] = nextLetter++;

        scheme << juce::String::charToString ((juce::juce_wchar) ('A' + letter[(size_t) g]));
    }

    return scheme;
}

std::vector<VerseReport> analyse (const Lyrics& lyrics)
{
    std::vector<VerseReport> verses;
    int unnamed = 0;

    for (auto& l : lyrics.lines)
    {
        if (verses.empty() || l.startsVerse)
        {
            verses.emplace_back();
            verses.back().heading = l.heading.isNotEmpty() ? l.heading : "Verse " + juce::String (++unnamed);
        }

        verses.back().lines.push_back ({ getLineText (l), rhyme::countSyllables (getHyphenatedText (l)), l.endBeat - l.startBeat, {} });
    }

    for (auto& v : verses)
    {
        juce::StringArray lastWords;

        for (auto& l : v.lines)
            lastWords.add (juce::StringArray::fromTokens (l.text, " ", "").strings.getLast());

        v.scheme = findRhymeScheme (lastWords);

        for (size_t i = 0; i < v.lines.size(); ++i)
            v.lines[i].rhyme = v.scheme.substring ((int) i, (int) i + 1);
    }

    // Name repeated headings: Chorus, Chorus 2...
    std::map<juce::String, int> seen;

    for (auto& v : verses)
        if (const int n = ++seen[v.heading]; n > 1)
            v.heading << " " << n;

    return verses;
}
} // namespace daw::gen::lyricsguide
