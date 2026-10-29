#include "StructureGuide.h"
#include "engine/Tempo.h"

#include <map>

namespace daw::gen::structure
{
namespace
{
    using T = SectionType;

    /** The type sequence of sections, to compare structures. */
    std::vector<SectionType> typesOf (const SectionList& sections)
    {
        std::vector<SectionType> types;
        for (auto& s : sections)
            types.push_back (s.type);

        return types;
    }

    juce::String barsText (int bars) { return juce::String (bars) + (bars == 1 ? " bar" : " bars"); }
}

//==============================================================================
int Structure::getTotalBars() const
{
    int total = 0;
    for (auto& p : parts)
        total += p.bars;

    return total;
}

SectionList Structure::toSections (int startBar) const
{
    std::map<SectionType, int> count, seen;

    for (auto& p : parts)
        if (p.name.isEmpty())
            ++count[p.type];

    SectionList sections;
    int bar = startBar;

    for (auto& p : parts)
    {
        auto label = p.name;

        // A type that comes back gets numbered: Verse 1, Verse 2.
        if (label.isEmpty() && count[p.type] > 1)
            label = sectionTypes::getName (p.type) + " " + juce::String (++seen[p.type]);

        sections.push_back ({ bar, p.bars, p.type, label });
        bar += p.bars;
    }

    return sections;
}

const std::vector<Structure>& getStructures()
{
    static const std::vector<Structure> structures {
        { "Pop song (verse-chorus)", "Pop, R&B, Afrobeats, Reggaeton, Indie, Country",
          "The most used structure in popular music.",
          "Verses and choruses take turns. The verse tells the story, with new words each time; the chorus repeats the hook, "
          "so by the second time listeners sing along. The pre-chorus lifts the energy so the chorus lands harder. The bridge, "
          "heard only once, breaks the pattern, so the last chorus feels like coming home. Many hits repeat the last chorus or "
          "move it up a key for an extra lift.",
          "Write the chorus first: it is the heart of the song. Give the verse a lower, simpler melody and fewer instruments, "
          "so the chorus has somewhere to go. Change at least one thing in verse 2 (the drums, the bass rhythm, an extra "
          "instrument) so it isn't a copy of verse 1.",
          110.0,
          { { T::intro, 4 }, { T::verse, 8 }, { T::preChorus, 4 }, { T::chorus, 8 }, { T::verse, 8 }, { T::preChorus, 4 },
            { T::chorus, 8 }, { T::bridge, 8 }, { T::chorus, 8 }, { T::outro, 4 } } },

        { "Rock song (with a solo)", "Rock, Indie, Punk, Metal, Pop-rock",
          "Verse and chorus around a riff, with a solo where pop has its bridge.",
          "Like the pop song, but usually without a pre-chorus and with an instrumental solo in the middle. The intro plays "
          "the main guitar riff, which comes back between sections, so the riff becomes a hook of its own. The solo gives "
          "contrast the way a bridge does, and leads into the final choruses, which are played hardest.",
          "Build the intro and the gaps between sections from one strong riff. Play the solo over the verse or chorus chords "
          "so it feels connected to the song. Play the last chorus twice, or with everything at full power.",
          120.0,
          { { T::intro, 4 }, { T::verse, 8 }, { T::chorus, 8 }, { T::verse, 8 }, { T::chorus, 8 }, { T::solo, 8 },
            { T::chorus, 8 }, { T::outro, 4 } } },

        { "Hip-hop and trap (hook and verses)", "Hip-hop, Trap, Drill, Grime",
          "Long rapped verses between a short, catchy hook.",
          "Rap songs often open with the hook, so it's heard straight away, then alternate long verses (16 bars of rapping, "
          "\"a sixteen\") with the hook. The beat is often one loop of 4 or 8 bars for the whole song. The sections differ in "
          "what is added or taken away: the drums stop for a moment before the hook, the hi-hats get busier in the verse, an "
          "extra melody joins in the hook.",
          "Make the 4- or 8-bar loop great first. Then make the sections different by muting and adding parts, not by writing "
          "new music. A drop (a beat or a bar of silence) right before the hook makes it hit harder.",
          90.0,
          { { T::intro, 4 }, { T::chorus, 8, "Hook 1" }, { T::verse, 16 }, { T::chorus, 8, "Hook 2" }, { T::verse, 16 },
            { T::chorus, 8, "Hook 3" }, { T::outro, 4 } } },

        { "EDM (build-up and drop)", "EDM, House, Future bass, Dubstep, Big room",
          "Tension and release instead of verse and chorus.",
          "Electronic dance music replaces verse and chorus with tension and release. The build-up raises the tension, the drop "
          "releases it with the full beat, the bass and the main hook, and the breakdown takes everything away again so that "
          "the second drop hits even harder. Everything happens in blocks of 8 and 16 bars, which dancers feel without "
          "counting.",
          "Make the drop first: the beat, the bass and the hook together. Then build the other sections by taking things away "
          "rather than writing new parts. Try silence, or only a vocal, in the last bar of the build-up: the contrast makes the "
          "drop bigger. For DJs, keep the intro and outro to drums and bass, so they can mix the track in and out.",
          126.0,
          { { T::intro, 16 }, { T::buildUp, 8 }, { T::drop, 16 }, { T::breakdown, 16 }, { T::buildUp, 8 }, { T::drop, 16 },
            { T::outro, 16 } } },

        { "Techno and house (phrases)", "Techno, House, Deep house, Minimal",
          "A groove that keeps going while elements come and go.",
          "Club tracks are long and change slowly. The groove keeps going while elements come in and leave one at a time, "
          "every 8 or 16 bars. Instead of verses and choruses there are phrases, and the story is in the details: a filter "
          "slowly opening, a hi-hat added, the bass taken out for 8 bars. One breakdown in the middle gives the dancers a rest "
          "before the peak.",
          "Start from a loop of 1 or 2 bars that grooves on its own. Every 8 or 16 bars, add or remove one thing; never change "
          "everything at once. Keep 16 bars of drums at the start and the end for DJs.",
          126.0,
          { { T::intro, 16 }, { T::phrase, 32, "Groove" }, { T::breakdown, 16 }, { T::phrase, 32, "Peak" }, { T::outro, 16 } } },

        { "Trance (the long breakdown)", "Trance, Melodic techno, Progressive house",
          "An emotional arc around one long breakdown.",
          "Trance builds an emotional arc around one long breakdown. The drums stop, and the main melody and chords play on "
          "their own over big pads. The build-up rises, and the drop brings the melody back with the full beat: the most "
          "euphoric moment of the track.",
          "The breakdown is where the melody is introduced: let it breathe over long chords. An arpeggio or a pluck keeps "
          "things moving without drums. Bring the melody back in the drop with more layers, or an octave higher.",
          138.0,
          { { T::intro, 16 }, { T::phrase, 16, "Groove" }, { T::breakdown, 32 }, { T::buildUp, 8 }, { T::drop, 32 },
            { T::outro, 16 } } },

        { "Drum & bass", "Drum & bass, Jungle, Liquid",
          "The drop structure at 174 BPM, with long sections.",
          "At 170 to 175 BPM bars go by fast, so the sections are long: a 32-bar intro for DJs and 32-bar drops. Otherwise it is "
          "the EDM drop structure: tension, release, rest, and a bigger release. The second drop often \"switches up\" the "
          "bassline so it doesn't feel like a repeat.",
          "Write the drop's beat and bass first. Make the second drop different: a new bass sound or rhythm over the same "
          "drums.",
          174.0,
          { { T::intro, 32 }, { T::drop, 32 }, { T::breakdown, 16 }, { T::buildUp, 8 }, { T::drop, 32 }, { T::outro, 16 } } },

        { "Twelve-bar blues", "Blues, Rock 'n' roll, Jazz blues, Boogie",
          "One 12-bar chord pattern, played again and again.",
          "The blues repeats one 12-bar chord pattern for the whole song: I I I I, IV IV I I, V IV I V (in C: C C C C, F F C C, "
          "G F C G). Each time through is called a chorus; here the sung ones are verses and the played ones solos. The words "
          "often follow the same plan: a line, the same line again, and an answer. Rock 'n' roll and a lot of early rock use "
          "it too.",
          "Put the Twelve-bar blues on the chord track (Explain chord progressions... in the chord track's menu), once per "
          "section. Play one solo chorus quietly and the next louder, and end with a turnaround in the last two bars that "
          "leads back to the start.",
          100.0,
          { { T::intro, 4 }, { T::verse, 12 }, { T::verse, 12 }, { T::solo, 12 }, { T::solo, 12 }, { T::verse, 12 },
            { T::outro, 4 } } },

        { "AABA (32-bar song form)", "Jazz standards, Swing, Musicals, Classic pop",
          "Two A's, a contrasting bridge, and the A again.",
          "The form of most jazz standards and many classic songs. An 8-bar melody (A) is played twice, then a contrasting "
          "8-bar bridge (B, also called the middle eight) goes somewhere new, and the A comes back to finish. The title is "
          "usually in the A. In jazz, the whole 32 bars (the head) is played first, the players solo over the same 32-bar "
          "form, and the head comes back at the end.",
          "Make the B a real contrast: other chords (it often starts on the IV), a higher or busier melody. The last A can end "
          "differently to close the song.",
          120.0,
          { { T::intro, 4 }, { T::verse, 8, "A1" }, { T::verse, 8, "A2" }, { T::bridge, 8, "B" }, { T::verse, 8, "A3" },
            { T::solo, 32, "Solos (AABA)" }, { T::verse, 8, "A (last)" }, { T::outro, 4 } } },

        { "Strophic (verse after verse)", "Folk, Singer-songwriter, Ballads, Hymns, Country",
          "The same music for every verse, while the words tell the story.",
          "The oldest song form: the same music comes back for every verse while the words tell the story. Often the last line "
          "of each verse returns as a refrain. It works when the story and the words carry the song.",
          "Keep the listener with small changes per verse: a second instrument, a harmony vocal, another strum pattern. A short "
          "instrumental interlude between verses gives a breath.",
          90.0,
          { { T::intro, 4 }, { T::verse, 16 }, { T::verse, 16 }, { T::interlude, 4 }, { T::verse, 16 }, { T::outro, 4 } } },

        { "Lo-fi and beats (two loops)", "Lo-fi, Chillhop, Instrumental hip-hop, Beats",
          "Two 8-bar loops taking turns.",
          "Short instrumental tracks, often two to three minutes, built from two 8-bar loops that take turns: A is the main "
          "loop, B a variation of it (other chords, a melody on top, filtered drums). The mood is relaxed, so small changes "
          "are enough to keep it interesting.",
          "Make A first, then copy it for B and change one or two things. Filtering the drums, or dropping them for two bars "
          "before A comes back, keeps it alive.",
          80.0,
          { { T::intro, 4 }, { T::phrase, 8, "A" }, { T::phrase, 8, "B" }, { T::phrase, 8, "A" }, { T::phrase, 8, "B" },
            { T::outro, 4 } } },

        { "Film and ambient (one arc)", "Cinematic, Ambient, Post-rock, Game music",
          "One big arc: quiet, growing, a climax, and rest.",
          "Without vocals or hooks, film music and ambient pieces are shaped by one big arc: a quiet start, a theme, a "
          "development that adds layers and tension, a climax with everything playing, and a calm resolution. Instead of "
          "repeating sections, the music grows.",
          "Start the theme simply, on one instrument, then repeat it with more layers each time. Make the climax the loudest "
          "and widest moment, and keep the resolution short and quiet.",
          80.0,
          { { T::intro, 8 }, { T::verse, 16, "Theme" }, { T::bridge, 16, "Development" }, { T::chorus, 16, "Climax" },
            { T::outro, 8, "Resolution" } } }
    };

    return structures;
}

//==============================================================================
juce::String getRole (SectionType t)
{
    switch (t)
    {
        case T::intro:      return "sets the mood";
        case T::verse:      return "tells the story";
        case T::preChorus:  return "builds tension";
        case T::chorus:     return "the hook, the high point";
        case T::postChorus: return "an extra hook";
        case T::bridge:     return "contrast";
        case T::solo:       return "an instrument leads";
        case T::interlude:  return "a breather";
        case T::phrase:     return "the groove develops";
        case T::breakdown:  return "space, without drums";
        case T::buildUp:    return "rising tension";
        case T::drop:       return "the release";
        case T::outro:      return "the ending";
    }

    return {};
}

juce::String describeSection (SectionType t)
{
    switch (t)
    {
        case T::intro:
            return "Sets the mood and draws the listener in, often with a piece of the chorus, the main riff, or just drums and "
                   "chords. In songs it is short (2 to 8 bars); dance tracks have long intros (16 or 32 bars) of mostly drums, so DJs "
                   "can mix them in. Energy: low.";
        case T::verse:
            return "Tells the story: new words each time over the same music. Calmer and lower than the chorus, so the chorus has "
                   "somewhere to go. Usually 8 or 16 bars. Energy: medium.";
        case T::preChorus:
            return "A short lift into the chorus that builds tension: a rising melody, chords that don't settle at home (often "
                   "ending on the IV or V), drums building up. 4 or 8 bars. Energy: rising.";
        case T::chorus:
            return "The hook: the part people remember and sing along with. The same words and melody every time, often with the "
                   "song's title, and the fullest arrangement. Usually 8 bars. Energy: the high point.";
        case T::postChorus:
            return "A short extra hook after the chorus: a repeated word, a chant or an instrumental riff (common in modern pop). "
                   "4 or 8 bars. Energy: high.";
        case T::bridge:
            return "Contrast before the last chorus: other chords (often starting away from home, on the IV or vi), a new melody, "
                   "sometimes a key change. It makes the last chorus feel fresh again. 4 or 8 bars, once per song. Energy: medium, "
                   "but different.";
        case T::solo:
            return "An instrument takes the lead (a guitar, sax, piano or synth), often over the verse or chorus chords. 8 or 16 "
                   "bars, or one 12-bar chorus in the blues. Energy: high.";
        case T::interlude:
            return "A short instrumental passage between sections: a breath for the listener, often the intro's riff again. 2 to "
                   "8 bars. Energy: low.";
        case T::phrase:
            return "A block of 8, 16 or 32 bars in a dance track where the groove keeps going while elements come in or drop out "
                   "one at a time (a new hi-hat, the bassline, a filter opening). Techno, house and loop-based beats are built from "
                   "phrases. Energy: medium.";
        case T::breakdown:
            return "The drums and bass drop out, leaving pads, chords or the main melody: a moment of space and emotion in a "
                   "dance track before it builds again. 8, 16 or 32 bars. Energy: low.";
        case T::buildUp:
            return "Raises the tension towards the drop: a snare roll that speeds up, a rising noise sweep, a filter opening, "
                   "sometimes the kick dropping out at the end. 8 or 16 bars. Energy: rising fast.";
        case T::drop:
            return "The release in electronic music: the full beat and the bass come in with the main hook. It is dance music's "
                   "answer to the chorus. 16 or 32 bars. Energy: the high point.";
        case T::outro:
            return "Brings the song to an end: a fade out, the chorus repeating while instruments drop away, or a final chord. "
                   "Dance tracks end with drums only, again so DJs can mix out. 4 to 16 bars. Energy: falling.";
    }

    return {};
}

juce::String getIntroduction()
{
    return "How songs are built\n\n"
           "A song is made of sections: an intro, verses, choruses and so on. Each section has a job, and their order is the "
           "song's structure (or form). Listeners don't count bars, but they feel it when something new starts, so a clear "
           "structure makes a song easy to follow.\n\n"
           "Bars and phrases\n"
           "Sections are almost always 4, 8 or 16 bars long. Music is felt in groups of 4 bars, called phrases, and a section "
           "of 8 or 16 bars is two or four phrases. A change after 8 bars sounds natural; a change after 7 or 9 bars sounds "
           "like a stumble, unless it is done on purpose. In dance music, producers and DJs count in phrases of 8, 16 and 32 "
           "bars, so tracks can be mixed into each other.\n\n"
           "Repetition and contrast\n"
           "A structure balances the familiar and the new. A section that comes back (the chorus, three times) sticks in the "
           "memory; a new section (the bridge, the breakdown) keeps the listener awake. A good rule: repeat something, change "
           "something, repeat again.\n\n"
           "Energy\n"
           "Every section has an energy level: how many instruments play, how loud and busy they are, how high the melody "
           "goes. A song is a journey up and down: low in the intro, building through the verse and pre-chorus, highest in the "
           "chorus or drop, back down for contrast, and highest of all near the end. The line over the coloured strip shows "
           "this curve.\n\n"
           "Get to the point\n"
           "Listeners decide quickly. Most pop songs reach the chorus within 45 to 60 seconds, and dance tracks made for "
           "streaming often reach the first drop within a minute (club versions keep their long DJ intros).\n\n"
           "Using it in DawIt\n"
           "Pick a structure on the left to see it and read why it works, then click Put on section track to lay it out from "
           "the bar you clicked. Or mark your own sections: drag across bars on the Sections row above the chord track. Then "
           "write the chords and parts section by section; click a section to loop it while you work.";
}

juce::String explainSections()
{
    juce::String text ("The sections\n\nEvery section has a job. Pop songs mostly use intro, verse, pre-chorus, chorus, bridge and "
                       "outro; electronic music uses intro, build-up, drop, breakdown and outro, and techno and house think in "
                       "phrases.\n");

    for (auto t : sectionTypes::all)
        text << "\n" << sectionTypes::getName (t) << " (" << getRole (t) << ")\n" << describeSection (t) << "\n";

    return text;
}

juce::String formatDuration (int bars, double bpm)
{
    const int seconds = juce::roundToInt (bars * 4.0 * 60.0 / juce::jmax (1.0, bpm));
    return juce::String (seconds / 60) + ":" + juce::String (seconds % 60).paddedLeft ('0', 2);
}

juce::String explainStructure (const Structure& s, double songBpm)
{
    const int bars = s.getTotalBars();
    juce::String text;
    text << s.name << "\n" << s.summary << "\n\n"
         << "Used in: " << s.styles << ".\n\n"
         << s.story << "\n\n"
         << "The sections\n";

    int bar = 0;
    for (auto& section : s.toSections (0))
    {
        text << "  " << section.getDisplayName() << ", bars " << (bar + 1) << "-" << (bar + section.bars) << " ("
             << barsText (section.bars) << "): " << getRole (section.type) << "\n";
        bar += section.bars;
    }

    text << "\nLength: " << barsText (bars) << ", about " << formatDuration (bars, s.typicalBpm) << " at a typical "
         << juce::roundToInt (s.typicalBpm) << " BPM";

    if (std::abs (songBpm - s.typicalBpm) >= 1.0)
        text << " (" << formatDuration (bars, songBpm) << " at your song's " << formatBpm (songBpm) << " BPM)";

    text << ". Longer songs repeat a chorus or a drop; shorter ones leave out a verse.\n\n"
         << "Where to start\n" << s.tips;

    juce::String templates;
    if (s.name == "Pop song") templates = "Pop ballad, Dance pop, Reggaeton and Afrobeats";
    else if (s.name == "Rock song") templates = "Rock";
    else if (s.name.startsWith ("Hip-hop")) templates = "Hip-hop and Trap";
    else if (s.name.startsWith ("Techno") || s.name.startsWith ("House")) templates = "House and Techno";
    else if (s.name.startsWith ("Trance")) templates = "Trance";
    else if (s.name.startsWith ("Drum & bass")) templates = "Drum & bass";
    else if (s.name.startsWith ("Blues")) templates = "Blues";
    else if (s.name.startsWith ("Lo-fi")) templates = "Lo-fi";
    else if (s.name.startsWith ("Film")) templates = "Film score";

    if (templates.isNotEmpty())
        text << "\n\nTemplates: " << templates << ". File > New Project from Style... makes the whole song.";

    return text;
}

juce::String findStructureName (const SectionList& sections)
{
    const auto types = typesOf (sections);

    for (auto& s : getStructures())
        if (typesOf (s.toSections (0)) == types)
            return s.name;

    return {};
}

juce::String explainSongStructure (const SectionList& sections, double bpm)
{
    if (sections.empty())
        return "Your song's structure\n\n"
               "No sections yet. Drag across bars on the Sections row (above the chord track) to mark one, or pick a structure "
               "on the left and click Put on section track to lay it out.";

    juce::String text ("Your song's structure\n\n");
    const int firstBar = sections.front().startBar;

    for (auto& s : sections)
        text << "  " << s.getDisplayName() << ", bars " << (s.startBar + 1) << "-" << s.getEndBar() << " ("
             << barsText (s.bars) << "), from " << formatDuration (s.startBar, bpm) << ": " << getRole (s.type) << "\n";

    const int totalBars = sections.back().getEndBar() - firstBar;
    text << "\nLength: " << barsText (totalBars) << ", about " << formatDuration (totalBars, bpm) << " at " << formatBpm (bpm) << " BPM.\n";

    if (const auto name = findStructureName (sections); name.isNotEmpty())
        text << "This is the " << name << " structure.\n";

    juce::StringArray notes;

    for (auto& s : sections)
        if (s.bars % 4 != 0)
            notes.add (s.getDisplayName() + " is " + barsText (s.bars) + " long. Sections of 4, 8 or 16 bars feel natural; an odd "
                       "length can work as a surprise, but make sure it is on purpose.");

    for (size_t i = 1; i < sections.size(); ++i)
        if (sections[i].startBar > sections[i - 1].getEndBar())
            notes.add ("Bars " + juce::String (sections[i - 1].getEndBar() + 1) + "-" + juce::String (sections[i].startBar)
                       + " have no section.");

    std::map<SectionType, int> count;
    for (auto& s : sections)
        ++count[s.type];

    const auto peak = std::find_if (sections.begin(), sections.end(), [] (const Section& s)
    {
        return s.type == SectionType::chorus || s.type == SectionType::drop;
    });

    if (peak == sections.end())
    {
        notes.add ("There is no chorus or drop. That is fine for a strophic song, a loop-based beat or a film piece, but most "
                   "songs have a high point that comes back.");
    }
    else
    {
        if (const double seconds = (peak->startBar - firstBar) * 4.0 * 60.0 / juce::jmax (1.0, bpm); seconds > 60.0)
            notes.add ("The first " + sectionTypes::getName (peak->type).toLowerCase() + " starts at " + formatDuration (peak->startBar - firstBar, bpm)
                       + ". Many listeners skip before then: try a shorter intro or first verse.");

        if (count[SectionType::chorus] == 1 && count[SectionType::drop] == 0)
            notes.add ("The chorus comes only once. Bringing it back is what makes it stick.");
    }

    for (size_t i = 2; i < sections.size(); ++i)
        if (sections[i].type == sections[i - 1].type && sections[i].type == sections[i - 2].type)
        {
            notes.add ("Three " + sectionTypes::getName (sections[i].type).toLowerCase() + " sections in a row: vary them, or put "
                       "something else in between for contrast.");
            break;
        }

    if (sections.front().type != SectionType::intro)
        notes.add ("The song starts without an intro, straight into the " + sectionTypes::getName (sections.front().type).toLowerCase()
                   + ". That is common in pop today; it gets to the point.");

    if (sections.back().type != SectionType::outro)
        notes.add ("There is no outro. Decide how it ends: a final chord, a fade out, or the last chorus cut short.");

    if (! notes.isEmpty())
    {
        text << "\nThings to look at\n";
        for (auto& note : notes)
            text << "- " << note << "\n";
    }

    return text;
}
} // namespace daw::gen::structure
