#include "StylesGuide.h"

#include "ChordGuide.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInReverb.h"
#include "engine/RoutingGuide.h"

namespace daw::gen::stylesguide
{
namespace
{
    juce::String styleText (const juce::String& id)
    {
        if (id == "pop-ballad") return R"guide(A ballad is a slow song that leaves room for the voice and the words: a piano and a singer, with strings joining for the chorus.

Tempo: 60 to 80 BPM; this template uses 70. A bar then lasts 3.4 seconds, so a long note can really ring.

Structure: the pop song (verse, pre-chorus, chorus, a bridge, a last chorus). 64 bars, about 3:39 at 70 BPM.

Chords, in C major: the verse plays the fifties progression, C Am F G. The pre-chorus plays Am F C G, which ends on G and pulls into the chorus. The chorus plays the pop progression, C G Am F: the same four chords in another order, so the chorus feels like an arrival. The bridge plays the Canon, C G Am Em F C F G, eight chords whose bass walks down: something new before the last chorus.

Groove: soft and sparse. The kick on beats 1 and 3, a quiet snare on 2 and 4, and soft hi-hats on every eighth note.

Sounds: piano playing broken chords (Ballad arpeggio), one held bass note per chord, and strings (Sustained) as the pad in the pre-chorus and chorus.

Mix: a hall reverb, because slow, long notes can carry a long tail.

Try: mute the drums in verse 1, as many ballads start, and write the chorus melody higher than the verse's.)guide";

        if (id == "dance-pop") return R"guide(Dance pop is a pop song with a club beat: verses and a chorus to sing, over a kick on every beat.

Tempo: pop runs from about 100 to 130 BPM, and dance pop sits near the top. This template uses 120: a beat every half second.

Structure: the pop song. 64 bars, about 2:08 at 120 BPM.

Chords, in A minor: the verse plays Am Dm F G (the minor loop). The pre-chorus climbs F G Am. The chorus plays Am F C G (epic minor): three of its four chords are major, so it sounds hopeful though the key is minor. The bridge walks down Am G F E (the Andalusian cadence); E major borrows a note from outside the key and pulls hard back to Am.

Groove: four on the floor, the kick on every beat. A clap on 2 and 4, and an open hi-hat on every off-beat (the "and" between the beats): the "tss" that makes it bounce.

Sounds: chords as short off-beat plucks, a bass jumping octaves on the eighths (Octave disco), and a pad in the pre-chorus and chorus.

Mix: a plate on the clap, chords, pad and melody.

Try: make the chorus melody higher and simpler than the verse's, with fewer words.)guide";

        if (id == "rock") return R"guide(Rock is built on guitars, bass and drums playing together, often around one strong riff.

Tempo: most rock songs run at about 100 to 130 BPM; punk goes much faster. This template uses 120.

Structure: the rock song: verse and chorus twice, an 8-bar solo where pop has its bridge, and a last chorus. 56 bars, about 1:52 at 120 BPM. Rock songs often play the last chorus twice: copy it if you want more.

Chords, in E major: guitar bands like E, A, D and G, because the guitar's open strings are E, A, D, G, B and E, and in those keys they ring. The verse plays E A B E (the three-chord song) and the chorus E B C#m A (the pop progression). The solo plays over the verse chords, so it sounds connected to the song.

Groove: the backbeat, the snare on beats 2 and 4. The kick on 1, 3 and the "and" of 3, and the hi-hats on every eighth note, the beats a little louder.

Sounds: power chords on the eighths (the root, the fifth and the octave, no third) and the bass on the root in eighths, locked to the guitar. DawIt has no guitar sound yet, so the guitar part plays on the Pluck sound: treat it as a sketch, or put a guitar plugin on the Chords track.

Mix: a short room, so the band sounds close and played in one space.

Try: write a riff for the intro and bring it back between the sections.)guide";

        if (id == "hip-hop") return R"guide(Hip-hop is built for the voice: a beat that loops, with room for rapped verses and a hook to sing along to. This template is boom bap, the sound of East Coast hip-hop in the late 1980s and early 1990s. The name copies the sound of the kick (boom) and the snare (bap).

Tempo: 80 to 100 BPM; this template uses 90.

Structure: hooks and verses: an intro, the hook, then 16-bar verses between hooks. 64 bars, about 2:51 at 90 BPM.

Chords, in C minor: one loop for the whole song, Cm Fm Ab Bb (the minor loop). That's normal in hip-hop: the sections differ in what plays, not in their chords.

Groove: the snare on 2 and 4; the kick on 1, on the last sixteenth of beat 2 and on the "and" of 3; hi-hats on the eighths. This is one common pattern. Real boom bap swings: the off-beat notes come a little late, as the samplers it was made on (such as the Akai MPC) could do. DawIt's patterns are straight for now: drag the off-beat hi-hats a little later in the piano roll and hear the difference.

Sounds: DawIt Drums' Boom bap kit, jazz piano chords (Jazz comping) where a producer would use a sample from a record, and one held bass note per chord.

Mix: a short room keeps it dry and close.

Try: mute the drums for the last bar before each hook, so the hook hits harder.)guide";

        if (id == "trap") return R"guide(Trap began in the hip-hop of Atlanta and the American South in the early 2000s and has spread into pop. Its sound: a deep bass, a clap once a bar, and fast hi-hats with rolls.

Tempo: written at 130 to 150 BPM, here 140, but it feels half as fast, 70, because the clap plays only once per bar, on beat 3. This is half time (see the rhythm guide). The fast tempo gives the hi-hats room for quick rolls.

Structure: hooks and verses, as in hip-hop. At 140 BPM a bar goes by in 1.7 seconds, so the 64 bars last only about 1:50: make the verses longer if you need more room.

Chords, in F minor: one dark loop, two bars per chord: Fm Eb Db C (the Andalusian cadence). The last chord, C major, borrows a note from outside the key, which pulls it strongly back to Fm.

Groove: the kick on 1 and on an off-beat or two, the clap on 3, the hi-hats on the eighths. At the end of every second bar the hi-hats roll: 8 hits in one beat (32nd notes).

Sounds: DawIt Drums' 808-style kit, a pad for the chords, and a bass holding each root. In trap the bass is often a long 808 kick tuned to the song's notes (DawIt Drums' Trap: a long 808 kit). DawIt Drums plays one pitch per drum, so this template keeps the bass on its own track.

Mix: a plate on the clap, the chords and the melody.

Try: change the hi-hat rolls every few bars: triplets, sixteenths, 32nds.)guide";

        if (id == "house") return R"guide(House began in Chicago clubs in the early 1980s; its name is usually traced to a club called the Warehouse. It's music for DJs and dancing: a steady groove where parts come and go.

Tempo: 120 to 128 BPM; this template uses 124.

Structure: phrases of 16 and 32 bars: an intro, the groove, a breakdown, the peak, an outro. 112 bars, about 3:37 at 124 BPM.

Chords, in A minor: one loop all the way, Am Dm F G (the minor loop).

Groove: four on the floor, a clap on 2 and 4, and an open hi-hat on every off-beat. The intro and outro have only the kick, hi-hats and bass, so a DJ can mix them with the records before and after.

Sounds: DawIt Drums' 909-style kit, after the Roland TR-909 drum machine (1983); house piano stabs; a bass jumping octaves; a pad in the breakdown and the peak.

Mix: a plate on the clap, chords, pad and melody.

Try: every 8 bars, add or take away one thing; never change everything at once.)guide";

        if (id == "techno") return R"guide(Techno began in Detroit in the mid-1980s. It's hypnotic: a few sounds repeated, changing slowly over minutes.

Tempo: 125 to 135 BPM; this template uses 130.

Structure: as house: intro, groove, breakdown, peak, outro. 112 bars, about 3:27 at 130 BPM.

Chords, in F minor: few chords, two bars each: Db Eb Fm Fm (rising to home). Many techno tracks use one chord or none at all: the groove and the sounds matter more than the harmony.

Groove: four on the floor; a quieter clap on 2 and 4; closed hi-hats on the beats and open ones on the off-beats.

Sounds: DawIt Drums' Techno kit, an arpeggio going up and down, a bass on the eighths, and a pad in the breakdown and the peak.

Mix: a hall, a big dark space around the arpeggio and the pad.

Try: take the kick out for the last 8 bars before the peak.)guide";

        if (id == "trance") return R"guide(Trance took shape in the club scene of the early 1990s, notably in Germany. It's about emotion: big chords, a soaring melody, and a long breakdown before the drop.

Tempo: 136 to 140 BPM; this template uses 138.

Structure: intro, groove, a 32-bar breakdown without drums, an 8-bar build-up, a 32-bar drop, outro. 120 bars, about 3:29 at 138 BPM.

Chords, in A minor: Am F C G (epic minor), two bars each. These are the four chords of C major's pop progression (C G Am F) in another order: starting on Am makes them bittersweet.

Groove: four on the floor, a clap on 2 and 4, closed hi-hats on the beats and open ones on the off-beats. The build-up drops the kick and plays a clap roll that speeds up: quarter notes for 4 bars, eighths for 2, sixteenths for the last 2.

Sounds: chords chopped into sixteenths (Trance gate), a bass on the eighths, a pad that carries the breakdown. The melody has clips in the breakdown, build-up and drop: the breakdown is where a trance melody is usually introduced.

Mix: a hall: long, wide tails are part of the sound.

Try: play the melody in the breakdown over the pad alone, then an octave higher in the drop.)guide";

        if (id == "drum-and-bass") return R"guide(Drum & bass came out of the UK's jungle scene in the early 1990s: fast drums over deep bass lines that move slowly.

Tempo: about 160 to 180 BPM, most tracks at 170 to 175; this template uses 174. The bass and chords change slowly, so the music feels fast and calm at once.

Structure: long sections: a 32-bar intro for DJs, a 32-bar drop, a breakdown, a build-up and a second drop. 136 bars, about 3:08 at 174 BPM.

Chords, in E minor: Em Am C D (the minor loop), two bars per chord.

Groove: the two-step: the kick on 1 and the "and" of 3, the snare on 2 and 4, hi-hats on the eighths. Jungle and much drum & bass chop up sampled breakbeats instead, most famously the Amen break; the two-step is the simple place to start.

Sounds: a pad for the chords and a deep held bass.

Mix: a plate on the snare, the chords and the melody.

Try: make the second drop different from the first: a new bass rhythm over the same drums.)guide";

        if (id == "lo-fi") return R"guide(Lo-fi (low fidelity) beats are relaxed instrumental tracks made to sound a little old and worn: soft drums, jazzy chords, the crackle of a record.

Tempo: 70 to 90 BPM; this template uses 80.

Structure: two 8-bar loops, A and B, taking turns. 40 bars: exactly 2:00 at 80 BPM.

Chords, in F major: A plays Gm7 C7 Fmaj7 (the ii-V-I of jazz) and B plays F Dm Bb C (the fifties progression). A seventh chord adds a fourth note on top of a chord, which gives lo-fi its soft, jazzy colour.

Groove: like boom bap but softer: the kick on 1 and around 3, a quiet snare on 2 and 4, soft hi-hats on the eighths. Lo-fi usually swings and sits a little behind the beat; DawIt's patterns are straight for now.

Sounds: DawIt Drums' Boom bap kit, piano chords comping like a jazz pianist, and a held bass.

Mix: a short room, so it stays close and intimate.

Try: mute the drums for two bars before A comes back.)guide";

        if (id == "blues") return R"guide(The blues grew out of African American music in the southern United States, and jazz, rock 'n' roll and rock all borrow from it.

Tempo: from slow blues around 60 to 75 BPM to shuffles of about 100 to 130. This template is a medium shuffle at 100.

Structure: one 12-bar pattern again and again, as sung verses and played solos. 68 bars, about 2:43 at 100 BPM.

Chords, in A major: A A A A, D D A A, E D A E: only the I, IV and V chords. Blues players often make every chord a seventh chord (A7, D7, E7) for a grittier sound.

Groove: the shuffle. Each beat splits in three, and the hi-hat plays the first and the third part: long-short, long-short. The kick on 1 and 3, the snare on 2 and 4.

Sounds: organ chords held for each bar, and a walking bass that steps through the chord on every beat. Both play on the beat, so they fit the shuffle.

Mix: a short room.

Try: sing a line, sing it again, then answer it: the classic plan of a blues verse.)guide";

        if (id == "reggaeton") return R"guide(Reggaeton took shape in Puerto Rico in the 1990s, from Jamaican dancehall, hip-hop and Latin music.

Tempo: about 85 to 100 BPM; this template uses 95.

Structure: the pop song. 64 bars, about 2:42 at 95 BPM.

Chords, in D minor: the verse plays Dm Gm Bb C (the minor loop), the pre-chorus climbs Bb C Dm, the chorus plays Dm Bb F C (epic minor), and the bridge walks down Dm C Bb A.

Groove: the dembow, named after Shabba Ranks's 1990 song "Dem Bow". The kick plays every beat. The snare plays on the "a" of 1, the "and" of 2, the "a" of 3 and the "and" of 4. With the kick on 1 and 3, each half bar is 3 + 3 + 2 sixteenths: the tresillo rhythm, heard all over Latin music.

Sounds: DawIt Drums' 808-style kit, off-beat plucked chords, a held bass, and a pad in the pre-chorus and chorus.

Mix: a plate on the snare, chords, pad and melody.

Try: keep the dembow all the way through and change only what plays around it.)guide";

        if (id == "afrobeats") return R"guide(Afrobeats is the pop music of West Africa, grown in Nigeria, Ghana and the UK in the 2000s and 2010s. It isn't the same as Afrobeat (without the s), the big-band style of Fela Kuti in the 1960s and 70s.

Tempo: about 95 to 120 BPM, most songs around 100 to 115. This template uses 108.

Structure: the pop song. 64 bars, about 2:22 at 108 BPM.

Chords, in G major: the verse plays Em C G D, the pre-chorus G Em C D, the chorus G D Em C (the pop progression) and the bridge G C D G.

Groove: the drums are the heart of afrobeats, and grooves vary a lot; this is one common pattern. A rim (a soft snare here) plays a clave, 3 + 2 hits: on 1, the "a" of 1, the "and" of 2, the "and" of 3 and on 4. The kick avoids four on the floor: on 1, the "and" of 2 and the "and" of 3. A shaker (a closed hi-hat here) plays every sixteenth, louder on the off-beats.

Sounds: funky sixteenth-note guitar chords (on the Pluck sound), a bass on the root and the fifth.

Mix: a plate on the rim, chords and melody.

Try: add a percussion track that answers the clave; afrobeats songs often layer several.)guide";

        return R"guide(Film music serves the picture: it has no fixed tempo or form, and it grows and falls with the story. This template is one big arc: a quiet start, a theme, a development that adds layers, a climax, and rest.

Tempo: whatever the scene needs. This template uses 80, slow enough for long notes.

Structure: one arc. 64 bars, about 3:12 at 80 BPM.

Chords, in D minor, two bars each: the theme and the climax play Dm Bb F C (epic minor). The development plays Bb C Dm (rising to home): two major chords climbing to the minor home chord, which sounds heroic.

Groove: low and high drums, like the big drum ensembles of action scenes, only in the development and the climax, with a cymbal at the start of the climax. The theme has no drums at all.

Sounds: held strings for the chords, a held bass, and the melody track for the theme.

Mix: a hall: orchestras record in big rooms.

Try: play the theme simply on one instrument first, then again with more layers each time.)guide";
    }

    juce::String names (const std::vector<Chord>& chords)
    {
        juce::StringArray result;
        for (const auto& chord : chords) result.add (chord.getName());
        return result.joinIntoString (" ");
    }

    std::vector<Chord> sectionChords (const Engine& engine, const Section& section)
    {
        std::vector<Chord> result;
        for (int bar = section.startBar; bar < section.getEndBar(); ++bar)
            if (const auto* chord = engine.getChord (bar); chord != nullptr && (result.empty() || ! (result.back() == *chord)))
                result.push_back (*chord);
        return result;
    }

    bool hasSoundIn (const Engine& engine, const Track& track, const Section& section)
    {
        for (const auto& clip : track.clips)
        {
            if (clip->startBeat >= section.getEndBar() * 4.0 || clip->getEndBeat (engine.getTempoMap()) <= section.startBar * 4.0)
                continue;
            if (clip->kind == Clip::Kind::audio || ! static_cast<const MidiClip&> (*clip).notes.empty())
                return true;
        }
        return false;
    }
} // namespace

juce::String getStyleId (Topic topic)
{
    const int index = (int) topic - (int) Topic::popBallad;
    return juce::isPositiveAndBelow (index, (int) templates::getStyles().size()) ? templates::getStyles()[(size_t) index].id : juce::String();
}

juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::styles: return "Styles of music";
        case Topic::parts: return "The parts of a song";
        case Topic::routing: return "Drum bus and reverb return";
        case Topic::yourSong: return "Your song";
        case Topic::popBallad: case Topic::dancePop: case Topic::rock: case Topic::hipHop: case Topic::trap: case Topic::house:
        case Topic::techno: case Topic::trance: case Topic::drumAndBass: case Topic::loFi: case Topic::blues: case Topic::reggaeton:
        case Topic::afrobeats: case Topic::filmScore:
            if (const auto* style = templates::findStyle (getStyleId (topic))) return style->name;
    }
    return {};
}

juce::String getTopicSummary (Topic topic)
{
    if (topic == Topic::styles) return "What makes a pop song sound different from a techno track.";
    if (topic == Topic::parts) return "Drums, bass, chords, pad and melody, and who plays where.";
    if (topic == Topic::routing) return "Why every template groups the drums and shares one reverb.";
    if (topic == Topic::yourSong) return "Your tempo, chords and parts against the styles.";
    if (const auto* style = templates::findStyle (getStyleId (topic))) return style->summary;
    return {};
}

templates::SongPlan getExample (Topic topic)
{
    const auto id = getStyleId (topic).isNotEmpty() ? getStyleId (topic) : juce::String ("dance-pop");
    const auto* style = templates::findStyle (id);
    return style != nullptr ? templates::makePlan (*style, std::nullopt, style->defaultBpm, 0, true) : templates::SongPlan();
}

juce::String analyse (const Engine& engine)
{
    juce::StringArray advice;
    const double bpm = engine.getTempo();
    juce::StringArray matches;
    for (const auto& style : templates::getStyles())
        if (bpm >= style.minimumBpm && bpm <= style.maximumBpm)
            matches.add (style.name.toLowerCase() + " (" + juce::String (style.minimumBpm) + " to " + juce::String (style.maximumBpm) + ")");

    if (! matches.isEmpty())
        advice.add ("At " + juce::String (bpm, juce::exactlyEqual (bpm, std::floor (bpm)) ? 0 : 1) + " BPM your song is in the usual range of " + matches.joinIntoString (", ") + ".");
    else
    {
        const double half = bpm * 0.5;
        juce::StringArray halfMatches;
        for (const auto& style : templates::getStyles())
            if (half >= style.minimumBpm && half <= style.maximumBpm)
                halfMatches.add (style.name.toLowerCase());
        advice.add (halfMatches.isEmpty()
                        ? "No style here usually sits at " + juce::String (bpm, 1) + " BPM. That's fine: these are habits, not rules."
                        : "No style here usually sits at " + juce::String (bpm, 1) + " BPM. Half of it, " + juce::String (half, 1)
                            + ", is in the range of " + halfMatches.joinIntoString (", ")
                            + ": if your snare or clap plays only once a bar, listeners feel it at that half tempo.");
    }

    if (engine.getSections().empty())
        advice.add ("No sections yet: File > New Project from Style... makes a song with them, or mark them on the Sections row.");
    else if (const auto structureName = structure::findStructureName (engine.getSections()); structureName.isNotEmpty())
    {
        juce::StringArray uses;
        for (const auto& style : templates::getStyles()) if (style.structureName == structureName) uses.add (style.name.toLowerCase());
        if (! uses.isEmpty()) advice.add ("Your sections follow the " + structureName.upToFirstOccurrenceOf (" (", false, false)
                                          + " structure, which the " + uses.joinIntoString (", ") + " templates use.");
    }

    const Section* verse = nullptr;
    const Section* chorus = nullptr;
    for (const auto& section : engine.getSections())
    {
        if (verse == nullptr && (section.type == SectionType::verse || section.type == SectionType::phrase)) verse = &section;
        if (verse != nullptr && chorus == nullptr && (section.type == SectionType::chorus || section.type == SectionType::drop)) chorus = &section;
    }

    if (verse != nullptr && chorus != nullptr)
    {
        const auto verseChords = sectionChords (engine, *verse);
        const auto chorusChords = sectionChords (engine, *chorus);
        if (! verseChords.empty() && ! chorusChords.empty())
        {
            auto sorted = [] (std::vector<Chord> list)
            {
                std::sort (list.begin(), list.end(), [] (const Chord& a, const Chord& b) { return a.getName() < b.getName(); });
                return list;
            };
            if (verseChords == chorusChords)
                advice.add ("Your verse and chorus play the same chords (" + names (verseChords) + "). That's normal in hip-hop and club music, where one loop runs through the song. In a pop song, try the chorus on the same chords in another order, so it feels like an arrival.");
            else if (sorted (verseChords) == sorted (chorusChords))
                advice.add ("Your verse and chorus use the same chords in another order, so the chorus feels like an arrival.");
            else
                advice.add ("Your chorus moves to other chords than the verse, so it sounds like a new place.");
        }

        int verseParts = 0, chorusParts = 0;
        for (const auto& track : engine.getTracks())
            if (! track->hasInput())
            {
                verseParts += hasSoundIn (engine, *track, *verse) ? 1 : 0;
                chorusParts += hasSoundIn (engine, *track, *chorus) ? 1 : 0;
            }
        if (chorusParts < verseParts)
            advice.add ("Your chorus has " + juce::String (chorusParts) + " parts playing and the verse " + juce::String (verseParts) + ". A chorus usually has the most energy: try bringing a part in at the chorus, or leaving one out of the verse.");
        else if (chorusParts > verseParts)
            advice.add ("Your chorus has " + juce::String (chorusParts) + " parts and the verse " + juce::String (verseParts) + ": it lifts.");
    }

    std::vector<const Track*> drumTracks;
    for (const auto& track : engine.getTracks()) if (! track->hasInput() && routingguide::getFamily (*track) == routingguide::Family::drums) drumTracks.push_back (track.get());
    if (drumTracks.size() >= 2)
    {
        Track* bus = drumTracks.front()->bus;
        const bool together = bus != nullptr && std::all_of (drumTracks.begin(), drumTracks.end(), [bus] (const Track* track) { return track->bus == bus; });
        if (! together)
            advice.add ("Your " + juce::String ((int) drumTracks.size()) + " drum tracks play straight into the master. Put them on a Drums bus with DawIt Compressor's Drum bus starting point: the routing guide's Your song's routing does it in one click.");
        else if (std::none_of (bus->plugins.begin(), bus->plugins.end(), [] (const auto& slot) { return slot->instance->getName() == BuiltInCompressor::identifier; }))
            advice.add ("Your Drums bus has no compressor: try DawIt Compressor's Drum bus starting point for glue.");
    }

    const Track* reverb = nullptr;
    int soundingTracks = 0;
    for (const auto& track : engine.getTracks())
    {
        if (! track->hasInput() && ! track->clips.empty()) ++soundingTracks;
        if (track->isReturn() && std::any_of (track->plugins.begin(), track->plugins.end(), [] (const auto& slot) { return slot->instance->getName() == BuiltInReverb::identifier; })) reverb = track.get();
    }
    if (reverb == nullptr && soundingTracks >= 3)
        advice.add ("Your song has no shared reverb. Track > New Return Track > Reverb makes one; send the chords, pad and melody to it, and the snare a little.");
    else if (reverb != nullptr && std::none_of (engine.getTracks().begin(), engine.getTracks().end(), [reverb] (const auto& track)
             { return std::any_of (track->sends.begin(), track->sends.end(), [reverb] (const auto& send) { return send->target == reverb && send->isOn(); }); }))
        advice.add ("Nothing sends to Reverb yet: turn up the Reverb knob of a track in the mixer.");

    return advice.joinIntoString ("\n\n");
}

juce::String explain (Topic topic, const Engine& engine)
{
    if (topic == Topic::styles) return R"guide(A style of music (a genre) is a set of habits that musicians share. Nobody enforces them, but listeners recognise them within seconds. Six things set most styles apart:

- Tempo: how fast the beat is. A ballad moves at 60 to 80 BPM, house at 120 to 128, drum & bass at 170 to 175.
- Groove: what the drums play. A kick on every beat is house and techno. A snare on beats 2 and 4 is rock and pop. One clap on beat 3 of every bar is trap.
- Harmony: the key and the chords. Trap, techno and trance are often in minor keys; many pop songs and the blues are in major. Some styles change chords every bar, others ride one loop for the whole song.
- Sounds: an organ and a walking bass say blues; chords chopped into fast sixteenths say trance.
- Structure: verses and choruses in pop, long 16- and 32-bar phrases in club music, one 12-bar pattern in the blues.
- The mix: how loud the drums are, how much reverb, how wide.

The picture shows each style's usual tempo range, with your song's tempo as a red line. The ranges overlap: a song at 96 BPM could be hip-hop, reggaeton or afrobeats. What tells them apart is the groove and the sounds, not the tempo alone.

These are habits, not rules. Most songs mix styles, and many famous songs break a habit of their own style on purpose. Use a template to hear the habits, then change whatever you like.

Every style below has a template. File > New Project from Style... makes a whole song in it: tempo, key, sections, chords, drums, bass and effects. A template is a sketch to start from, not a finished song: the melody is yours to write.)guide";

    if (topic == Topic::parts) return R"guide(Almost every template has the same five parts, and each has a job:

- Drums keep time and set the groove. Each drum has its own track (Kick, Snare, Hats), so you can see, mute and mix each one.
- Bass joins the drums to the harmony. It usually plays the root of each chord (the note the chord is named after), low down.
- Chords fill the middle. The same chords played another way change the style: held, broken into an arpeggio, or chopped into short stabs.
- Pad: long, soft chords behind everything. They make a section sound fuller without adding rhythm.
- Melody: the tune, sung or played. A template leaves its clips empty for you: write it in the piano roll, or let Generate write one.

Not every part plays all the time. Most of a song's energy comes from how many parts play. In the picture each row is a part, and each block a section where it plays:

- The intro and outro are thin: the chords alone, or in club music the kick, hi-hats and bass, so a DJ can mix the track in and out.
- The verse brings in the drums and the bass.
- The chorus or drop has everything.
- A breakdown takes the drums and bass out, so the next drop hits harder.
- A build-up drops the kick and plays a snare roll that gets faster: quarter notes, then eighths, then sixteenths.

This follows the energy curve of the Song structure guide, from 1 for a calm outro to 5 for a chorus or drop. If your chorus doesn't feel bigger than your verse, count the parts playing in each.)guide";

    if (topic == Topic::routing) return R"guide(Every template sets up two things a mix engineer often adds first.

The Drums bus. The drum tracks play into one bus called Drums instead of straight into the master. One fader turns the whole kit up or down without changing its balance. One compressor, at DawIt Compressor's Drum bus starting point, makes the drums move as one instrument: when the snare hits, everything dips a little together. This is glue compression, and it's gentle: 2:1, with a 30 ms attack so every hit keeps its click.

The Reverb return. A return track called Reverb holds one DawIt Reverb at 100% mix. The chords, the pad, the melody and the snare send a copy of themselves to it. Because they share one reverb, they sound as if they were played in one room, and changing the reverb once changes it for all of them. The send levels set how much each gets: the pad -8 dB (a lot, so it floats behind), the melody -10 dB, the chords -12 dB and the snare -14 dB (a touch). A send at -10 dB gives about as much reverb as a 20% mix on the track itself.

The kick, the hi-hats and the bass don't send. Reverb on the lowest sounds makes the low end muddy, and the return's own low cut (200 Hz on a room or hall, 250 Hz on the plate) keeps the lows of the rest out too.

Which reverb: a plate for most styles, because it is bright and dense and suits vocals, snare and keys. A hall for the ballad, techno, trance and the film score, whose long notes can carry a long tail. A short room for rock, hip-hop, lo-fi and blues, which sound best close and dry. These are starting points: change the return's reverb and every part follows.

The bass also gets DawIt Compressor's Bass starting point, which evens out its notes, and the chords get DawIt EQ's Make room for a vocal: a low cut at 150 Hz leaves the lows to the bass and kick, and a 3 dB dip at 3 kHz leaves room for the melody. The Buses, sends and routing guide explains all of this in more detail.)guide";

    if (topic == Topic::yourSong)
        return "How your song compares with the styles' habits. They are habits, not rules: use this to decide on purpose.\n\n" + analyse (engine);

    return styleText (getStyleId (topic));
}
} // namespace daw::gen::stylesguide
