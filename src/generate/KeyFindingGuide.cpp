#include "KeyFindingGuide.h"
#include "ChordGuide.h"

namespace daw::gen::keyfinding
{
namespace mh = analysis::midiharmony;

namespace
{
    int mod12 (int v) { return ((v % 12) + 12) % 12; }

    mh::Note note (int pitch, double start, double length, float velocity = 1.0f) { return { pitch, start, length, velocity }; }

    juce::String percent (double share)
    {
        const double p = share * 100.0;
        return std::abs (p - std::round (p)) < 0.05 ? juce::String ((int) std::lround (p)) : juce::String (p, 1);
    }

    juce::String withArticle (const juce::String& name)
    {
        return (juce::String ("AEFHILMNORSX").containsChar (name[0]) ? "an " : "a ") + name;
    }

    juce::String barName (const mh::Segment& s)
    {
        return "Bar " + juce::String (s.bar + 1) + (s.split ? (s.secondHalf ? " (second half)" : " (first half)") : juce::String());
    }

    Row makeRow (const juce::String& label, std::vector<int> pcs, int home, std::vector<int> marked = {}, const juce::String& steps = {})
    {
        Row r;
        r.label = label;
        r.pitchClasses = std::move (pcs);
        r.home = home;
        r.marked = std::move (marked);
        r.steps = steps;
        return r;
    }

    Row scaleRow (int tonic, mh::Scale scale, std::vector<int> marked = {}, const juce::String& steps = {})
    {
        std::vector<int> pcs;

        for (int i : mh::getScaleIntervals (scale))
            pcs.push_back (mod12 (tonic + i));

        return makeRow (mh::getKeyName (tonic, scale), pcs, tonic, std::move (marked), steps);
    }

    Row chordRow (const Chord& c, std::vector<int> pitches, int bass, const juce::String& note = {})
    {
        Row r;
        r.label = c.getName();
        r.home = c.root;
        r.bass = bass;
        r.note = note;
        r.pitches = std::move (pitches);

        for (int p : r.pitches)
            if (std::find (r.pitchClasses.begin(), r.pitchClasses.end(), mod12 (p)) == r.pitchClasses.end())
                r.pitchClasses.push_back (mod12 (p));

        return r;
    }

    Row chordRow (const juce::String& label, int root, juce::Array<int> intervals)
    {
        Chord c { root, intervals };
        auto r = chordRow (c, {}, root);
        r.label = label;
        r.pitchClasses.clear();

        for (int pc : c.getPitchClasses())
            r.pitchClasses.push_back (pc);

        return r;
    }
} // namespace

juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::key:             return "What a key is";
        case Topic::majorMinor:      return "Major and minor scales";
        case Topic::modes:           return "The modes";
        case Topic::harmonicMelodic: return "Harmonic and melodic minor";
        case Topic::pentatonic:      return "Pentatonic and blues scales";
        case Topic::howFound:        return "How DawIt finds the key";
        case Topic::naming:          return "Naming a chord from its notes";
        case Topic::inversions:      return "Inversions and slash chords";
        case Topic::susDim:          return "Sus, diminished and added notes";
        case Topic::between:         return "Notes between the chords";
        case Topic::yours:           return "Your clip";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::key:             return "The notes a part uses, and the one that feels like home.";
        case Topic::majorMinor:      return "The two most common scales, and why C major and A minor share their notes.";
        case Topic::modes:           return "Dorian, Phrygian, Lydian, Mixolydian and Locrian: the same seven notes with another home.";
        case Topic::harmonicMelodic: return "Minor scales with a raised seventh (and sixth), for a stronger pull home.";
        case Topic::pentatonic:      return "Five-note scales, and the blues scale's extra note.";
        case Topic::howFound:        return "Counting how long each note sounds and comparing it with every key.";
        case Topic::naming:          return "Collect the notes, stack them in thirds, and the bottom of the stack is the root.";
        case Topic::inversions:      return "The bass note: C, C/E and C/G are the same chord over different notes.";
        case Topic::susDim:          return "Chords with a note swapped, squeezed or added.";
        case Topic::between:         return "Passing and neighbour notes: why not every note belongs to the chord.";
        case Topic::yours:           return "The key, scale and chords of the selected MIDI clip, and what to do with them.";
    }

    return {};
}

std::vector<mh::Note> getExampleTune()
{
    // C E G E | F A G(2 beats) | E D C D | C (4 beats)
    return { note (60, 0, 1), note (64, 1, 1), note (67, 2, 1), note (64, 3, 1),
             note (65, 4, 1), note (69, 5, 1), note (67, 6, 2),
             note (64, 8, 1), note (62, 9, 1), note (60, 10, 1), note (62, 11, 1),
             note (60, 12, 4) };
}

Example getExample (Topic t)
{
    using S = mh::Scale;
    Example e;

    switch (t)
    {
        case Topic::key:
            e.notes = getExampleTune();
            e.rows = { scaleRow (0, S::major) };
            e.rows[0].note = "C sounds for 6 beats, E and G for 3, D for 2, F and A for 1: every note is one of C major's seven.";
            e.caption = "The tune's notes by how long they sound, with C major's notes lit and C marked as home.";
            break;

        case Topic::majorMinor:
            e.rows = { scaleRow (0, S::major, {}, "W W H W W W H"), scaleRow (9, S::minor, {}, "W H W W H W W") };
            e.caption = "C major and A minor use the same seven notes.";
            break;

        case Topic::modes:
        {
            const int homes[] { 0, 2, 4, 5, 7, 9, 11 };
            const S scales[] { S::major, S::dorian, S::phrygian, S::lydian, S::mixolydian, S::minor, S::locrian };
            const int characteristic[] { -1, 11, 5, 6, 10, -1, 6 };    // Dorian B, Phrygian F, Lydian B, Mixolydian F, Locrian F
            const int marks[] { -1, 11, 5, 11, 5, -1, 5 };

            for (int i = 0; i < 7; ++i)
            {
                auto r = scaleRow (homes[i], scales[i], marks[i] >= 0 ? std::vector<int> { marks[i] } : std::vector<int>());
                r.label = r.label + (i == 0 ? " (Ionian)" : i == 5 ? " (Aeolian)" : "");
                r.note = characteristic[i] >= 0 ? "the characteristic note is lit" : "";
                e.rows.push_back (r);
            }

            e.caption = "The white keys from each note: seven modes of the same seven notes.";
            break;
        }

        case Topic::harmonicMelodic:
            e.rows = { scaleRow (9, S::minor), scaleRow (9, S::harmonicMinor, { 8 }), scaleRow (9, S::melodicMinor, { 6, 8 }),
                       chordRow ("Em (E G B)", 4, { 0, 3, 7 }), chordRow ("E (E G# B)", 4, { 0, 4, 7 }) };
            e.rows[3].marked = { 7 };
            e.rows[4].marked = { 8 };
            e.caption = "The raised notes are lit; the five chord is major in harmonic minor.";
            break;

        case Topic::pentatonic:
            e.rows = { scaleRow (0, S::majorPentatonic), scaleRow (9, S::minorPentatonic), scaleRow (9, S::blues, { 3 }) };
            e.caption = "Five notes, and the blues scale's extra note, the blue note, marked.";
            break;

        case Topic::howFound:
        {
            e.notes = getExampleTune();
            const auto result = mh::analyse ({ e.notes, 0.0, 16.0 });

            for (auto& k : result.ranking)
            {
                auto r = scaleRow (k.tonic, k.minor ? S::minor : S::major);
                r.note = "score " + juce::String (k.score, 2);
                e.rows.push_back (r);
            }

            e.caption = "The tune's profile beside the key profile of the best key, with the top three keys and their scores.";
            break;
        }

        case Topic::naming:
            e.notes = { note (52, 0, 4), note (55, 0, 4), note (60, 0, 4), note (71, 0, 4) };   // E3 G3 C4 B4
            e.rows = { makeRow ("Note names", { 4, 7, 0, 11 }, -1), makeRow ("Stacked in thirds", { 0, 4, 7, 11 }, 0) };
            e.rows[1].note = "Cmaj7";
            e.caption = "E G C B in a bar: the note names, stacked in thirds from C, make Cmaj7.";
            break;

        case Topic::inversions:
            e.rows = { chordRow (Chord { 0, { 0, 4, 7 } }, { 48, 52, 55 }, 0),
                       chordRow (Chord { 0, { 0, 4, 7 } }, { 40, 55, 60, 64 }, 4, "C/E"),
                       chordRow (Chord { 0, { 0, 4, 7 } }, { 43, 60, 64 }, 7, "C/G"),
                       chordRow (Chord { 9, { 0, 3, 7, 10 } }, { 45, 60, 64, 67 }, 9, "Am7, or C/A") };
            e.rows[1].label = "C/E";
            e.rows[2].label = "C/G";
            e.rows[3].label = "A C E G over A";
            e.caption = "The same chord over different bass notes (the lit key).";
            break;

        case Topic::susDim:
            e.rows = { chordRow ("Csus2", 0, { 0, 2, 7 }), chordRow ("Csus4", 0, { 0, 5, 7 }), chordRow ("Bdim", 11, { 0, 3, 6 }),
                       chordRow ("Bdim7", 11, { 0, 3, 6, 9 }), chordRow ("Caug", 0, { 0, 4, 8 }), chordRow ("Cadd9", 0, { 0, 4, 7, 14 }) };
            e.caption = "Six chords with a note swapped, squeezed or added.";
            break;

        case Topic::between:
            e.notes = { note (48, 0, 4, 0.8f), note (52, 0, 4, 0.8f), note (55, 0, 4, 0.8f),
                        note (60, 0, 0.5, 0.8f), note (62, 0.5, 0.5, 0.8f), note (64, 1, 0.5, 0.8f), note (65, 1.5, 0.5, 0.8f), note (67, 2, 0.5, 0.8f) };
            e.rows = { chordRow ("C", 0, { 0, 4, 7 }) };
            e.rows[0].note = "Each held chord note weighs 3.2; D and F weigh 0.4 each.";
            e.caption = "One bar: a held C chord and the tune C D E F G.";
            break;

        case Topic::yours:
            e.notes = getExampleTune();
            e.rows = { scaleRow (0, S::major) };
            e.caption = "The key, scale and chords of the selected MIDI clip.";
            break;
    }

    return e;
}

juce::String getText (Topic t)
{
    switch (t)
    {
        case Topic::key:
            return juce::String::fromUTF8 (R"txt(A key is two things: a set of notes that a piece mostly uses, and one note among them that feels like home. A song in C major uses mainly the seven notes C, D, E, F, G, A and B (the white keys of a piano), and C is home: tunes come to rest on it, and songs often end on it.

Music has 12 different note names: C, C#, D, Eb, E, F, F#, G, Ab, A, Bb and B. After B comes C again, an octave higher. For finding a key the octave doesn't matter: a low C and a high C count as the same note name.

The picture shows a short tune: C E G E | F A G | E D C D | C. Each bar is how long one note name sounds, in beats: C sounds for 6 beats, E and G for 3, D for 2, F and A for 1. Every note is one of C major's seven, and C sounds longest and comes last. That's why the tune is in C major.

A key isn't a rule. Songs use notes from outside their key all the time, for colour or tension, and some change key halfway. A key describes what a part mostly does; it doesn't forbid anything.

This guide doesn't explain how the chords of a key work together (I, IV, V and so on): that's the *Chord progressions* guide.)txt");

        case Topic::majorMinor:
            return juce::String::fromUTF8 (R"txt(A scale is the notes of a key in order, from home up to home an octave higher. What makes scales differ is the pattern of steps between the notes. A whole step (W) is two semitones, like C to D. A half step (H) is one semitone, like E to F: two piano keys right next to each other.

The major scale's pattern is W W H W W W H. From C it gives C D E F G A B C, all white keys. From G the same pattern needs one black key: G A B C D E F# G. Every major key uses this pattern, so they all sound alike, just higher or lower.

The natural minor scale's pattern is W H W W H W W. From A it gives A B C D E F G A, again all white keys. Its third note is a semitone lower than in A major (C instead of C#), and that gives minor its darker sound.

C major and A minor use exactly the same seven notes. They're called relative keys. Every major key has a relative minor whose home is three semitones lower: C major and A minor, G major and E minor, F major and D minor. What tells them apart is which note feels like home: where the tune rests, which chord the song starts and ends on, which note the bass keeps coming back to. That's also one of the commonest mix-ups of a key finder.

Click a scale to hear it.)txt");

        case Topic::modes:
            return juce::String::fromUTF8 (R"txt(Play the white keys from C to C and you hear the major scale. Play the same white keys from D to D and it sounds different: minor, but with one brighter note near the top. That's the Dorian mode. Each white key can be home, which gives seven modes:
• Ionian, from C: the major scale.
• Dorian, from D: minor with a major sixth (B in D Dorian, where D minor has Bb).
• Phrygian, from E: minor with a minor second (F, just a semitone above home E).
• Lydian, from F: major with a raised fourth (B in F Lydian, where F major has Bb).
• Mixolydian, from G: major with a minor seventh (F in G Mixolydian, where G major has F#).
• Aeolian, from A: the natural minor scale.
• Locrian, from B: its home chord (B D F) is diminished, so it hardly feels at rest. It's more theory than practice.

Players often hear Dorian as minor but warmer, Lydian as bright and floating, Mixolydian as major but relaxed, and Phrygian as dark. Those are impressions, not rules.

A mode can start on any note. D Dorian has the notes of C major, but A Dorian has those of G major: A B C D E F# G. To tell a mode, compare it with the major or minor scale on the same home note and look for the one note that differs, its characteristic note. A part in D that uses B, never Bb, is D Dorian rather than D minor.

Modes only show when home is clear. White-key notes that keep landing on C are C major; the same notes landing on D are D Dorian. When home is unclear, DawIt says so and names both.)txt");

        case Topic::harmonicMelodic:
            return juce::String::fromUTF8 (R"txt(In natural minor, the seventh note is a whole step below home: G in A minor. The five chord is then minor (E G B), and it pulls home only softly. Raise the seventh by a semitone, to G#, and the five chord becomes major (E G# B). G# sits a semitone below A and leans towards it. It's called the leading note.

The harmonic minor scale is natural minor with that raised seventh: A B C D E F G# A. Between F and G# there's now a gap of three semitones (an augmented second), which gives the scale its exotic sound.

The melodic minor scale also raises the sixth, to smooth that gap: A B C D E F# G# A. In classical music it's used like this going up, and coming down it returns to natural minor (A G F E D C B A). Jazz uses the raised form both ways, and that's the form DawIt shows and the song key uses.

Most minor songs mix these: natural minor for most of the tune, and the raised seventh near the end of a phrase, in the five chord. So a G# in a part in A minor doesn't mean a different key. DawIt names the key A minor and says where the G# comes from.)txt");

        case Topic::pentatonic:
            return juce::String::fromUTF8 (R"txt(A pentatonic scale has five notes. The major pentatonic is the major scale without its fourth and seventh: C D E G A. The minor pentatonic is natural minor without its second and sixth: A C D E G. They share their notes, just as C major and A minor do.

They have no half steps, so no two of their notes rub against each other. That's why many guitarists and singers improvise with them first, and why so many folk tunes and riffs fit in them. The black keys of a piano form one: F# G# A# C# D#, the F# major pentatonic.

The blues scale is the minor pentatonic with one note added, the flattened fifth: Eb in A, so A C D Eb E G. It's the "blue note", usually a quick passing note between D and E. On a guitar or a voice, blue notes are often bent somewhere between two notes; MIDI notes are whole semitones, so DawIt sees the nearest one. There's no single agreed blues scale: some books use a major blues scale (C D Eb E G A) or a nine-note one. DawIt looks for the six-note one.

A part that uses just these five or six notes is shown as pentatonic or blues. The song's key can only be one of the seven-note scales, so *Set the song's key* sets A minor for A minor pentatonic or A blues, and C major for C major pentatonic.)txt");

        case Topic::howFound:
            return juce::String::fromUTF8 (R"txt(DawIt adds up how long each of the 12 note names sounds in the clip, in beats, times its velocity. A whole note played firmly counts for more than a quick, soft grace note. That gives a profile like the bars in the picture.

It compares that profile with one for each of the 24 major and minor keys. Those come from listening tests by Carol Krumhansl and Edward Kessler (1982): listeners heard a key set up, then rated how well each of the 12 notes fitted it. Home scored highest (6.35 in major), then the other notes of the home chord (the fifth 5.19, the third 4.38), then the rest of the scale, and the notes outside the scale lowest (2.2 to 2.5). The key whose profile rises and falls most like the clip's wins. This is the Krumhansl-Schmuckler method, and the score is a correlation from -1 to 1.

For the example tune, C major scores 0.95 and the runner-up, A minor, 0.59: a clear result. When the top two are less than 0.1 apart, DawIt also looks at where the part starts and ends, because the first chord and the last bass note usually sit on home. A major key and its relative minor get this check whatever their scores, when both scales hold the part's notes equally well: then the start (counted double) and the end decide. An Am F C G loop that starts and ends on Am is A minor, although C major's profile matches its notes better (0.93 against 0.83).

Then it checks the scale. Of the scales on that home note (major, minor, the modes, harmonic and melodic minor, the pentatonics and blues), it picks the one that holds the most of the notes played. The fit is that share, by length and velocity. When two scales fit within 3 points of each other, the simpler one wins (major or minor before a mode), and the few notes outside it are explained instead, like a G# in A minor.

It isn't perfect. The usual mistakes of key finders are a key a fifth away (G major for C major), the relative key (A minor for C major) and the key on the same home note (C minor for C major). Research tests even give part marks for them: half a point for a fifth, 0.3 for the relative key, 0.2 for the parallel one. A short clip, a part with only a few notes, or a riff over one chord gives little to go on. Your ears decide: if the music feels at home on another note, trust that.)txt");

        case Topic::naming:
            return juce::String::fromUTF8 (R"txt(A chord is named after its root and its kind: Am is a minor chord on A. To name a chord from the notes played, DawIt does what a musician does:
1. Collect the note names that sound in the bar (or half bar), in any octave, weighted by how long and how firmly they sound.
2. Find the chord whose notes match them best. A chord's notes stack in thirds from its root: C, a third up E, a third up G, a third up B.
3. Look at the lowest note, the bass. It often decides between chords that share notes.

In the picture a bar plays E, G, C and B in different octaves. As note names that's C E G B, and stacked in thirds from C it's C E G B: Cmaj7. Its lowest note is E, the third, so DawIt writes it Cmaj7/E (see *Inversions and slash chords*).

A chord doesn't need all its notes. The fifth is often left out because it adds little colour: C and E with a C in the bass is still C. The third decides major or minor, so with only a root and a fifth (a power chord) DawIt picks the major or minor chord that's in the key and says the third is missing.

DawIt names the chords the chord track can hold: major, m, 7, maj7, m7, m7b5, dim, dim7, aug, sus2, sus4, add9, m(add9), 9, maj9 and m9. A sixth chord (C E G A) has the same notes as Am7, so it's named Am7 with C in the bass, Am7/C. Other notes it can't place are listed as notes between the chords.

A part that plays one note at a time gets a chord only where it spells one out, as an arpeggio does (A C E C for Am): at least three of a plain chord's note names, holding 85% of the bar's weight. Its bass is then the lowest note. A tune that just moves along the scale gets no chords, because a tune's notes alone don't fix the harmony.)txt");

        case Topic::inversions:
            return juce::String::fromUTF8 (R"txt(A chord's notes can be stacked in any order. What counts is the lowest one, the bass:
• root position: the root in the bass (C E G over C);
• first inversion: the third in the bass (C over E), written C/E;
• second inversion: the fifth in the bass, C/G;
• third inversion, for seventh chords only: the seventh in the bass, C7/Bb.
The slash means "over". C/E is a C chord with E in the bass, not "C or E".

Inversions make a bass line smoother. C, G/B, Am walks the bass down C, B, A while the chords stay simple.

A slash chord's bass can also be a note that isn't in the chord at all: C/D is C E G over a D.

Sometimes the bass decides the name. A, C, E and G over an A are Am7. You could also read them as a C chord over A, C/A: the same notes. Because A is Am7's root, DawIt says Am7 and mentions C/A as the other reading. F/D and Dm7 are another such pair.

Some chords are symmetrical. A diminished seventh's four notes are each three semitones apart, so Cdim7, Ebdim7, F#dim7 and Adim7 are the same notes, and there are only three different dim7 chords. An augmented chord's notes are four semitones apart: Caug, Eaug and G#aug share their notes. For these, DawIt names the chord on the bass note.

The chord track holds one chord per bar without its bass, so C/E goes on it as C. *Your clip* shows the bass.)txt");

        case Topic::susDim:
            return juce::String::fromUTF8 (R"txt(Most chords are major or minor, but a MIDI part shows other kinds clearly:
• sus2 and sus4: the third is swapped for the second or the fourth. Csus2 is C D G, Csus4 is C F G. With no third they're neither major nor minor, and they sound open, waiting to move. Csus2 and Gsus4 have the same notes (C D G), so the bass decides again.
• diminished (dim): a minor third and a fifth a semitone lower, B D F. Tense; in C major it's the seven chord.
• half-diminished (m7b5): a diminished chord with a minor seventh, B D F A.
• diminished seventh (dim7): three minor thirds stacked, B D F Ab. Very tense; it often leads to the chord a semitone above its root (Bdim7 to C).
• augmented (aug): two major thirds stacked, C E G#. Dreamy and unresolved.
• add9: a major chord plus the ninth (the second, an octave up) without a seventh: Cadd9 is C E G D. With a seventh as well, it's a ninth chord (C9, Cmaj9).)txt");

        case Topic::between:
            return juce::String::fromUTF8 (R"txt(A tune moves between the notes of its chords. Hold a C chord for a bar while the tune runs C D E F G: D and F aren't in the chord. They're passing notes, stepping from one chord note to the next in the same direction. A neighbour note steps away and comes back (E F E). These usually fall between the beats and are short.

Some outside notes land on the beat on purpose. A suspension holds a note over from the previous chord, then falls a step to a chord note (Csus4 becoming C). An appoggiatura leaps to an outside note on the beat, then steps to a chord note. Both add tension that then resolves.

That's why DawIt weighs notes by length and velocity. In the picture, each note of the held C chord (4 beats at velocity 80%) counts 3.2, while each eighth-note D and F counts 0.4 (half a beat at 80%). The chord's notes outweigh the passing notes eight to one, so the bar is C, and D and F are listed as notes between the chords.

When outside notes are long and loud, they probably belong to the chord, and DawIt picks a richer one: Cadd9 for a held D.)txt");

        case Topic::yours:
            return "Select a MIDI clip (click it in the arrangement), and this topic shows its key, scale and chords.";
    }

    return {};
}

//==============================================================================
std::vector<BarChord> getBarChords (const std::vector<mh::Segment>& segments)
{
    std::vector<BarChord> bars;
    int lastBar = -1;

    for (auto& s : segments)
    {
        if (s.bar == lastBar)
            continue;

        lastBar = s.bar;

        if (s.chord)
            bars.push_back ({ s.bar, *s.chord });
    }

    return bars;
}

Merge mergeIntoChordTrack (const std::vector<mh::Segment>& segments, const ChordMap& existing)
{
    Merge m;
    m.toWrite = getBarChords (segments);

    if (! segments.empty())
    {
        m.firstBar = segments.front().bar;
        m.lastBar = segments.back().bar;
    }

    for (auto& b : m.toWrite)
    {
        const auto it = existing.find (b.bar);

        if (it != existing.end() && ! (it->second == b.chord))
            m.replaced.push_back ({ b.bar, it->second });
    }

    return m;
}

ChordMap applyMerge (const Merge& m, ChordMap existing)
{
    for (auto& b : m.toWrite)
        existing[b.bar] = b.chord;

    return existing;
}

juce::String getReplaceQuestion (const Merge& m)
{
    if (m.replaced.empty())
        return {};

    juce::StringArray list;

    for (size_t i = 0; i < m.replaced.size() && i < 3; ++i)
        list.add ("bar " + juce::String (m.replaced[i].bar + 1) + ": " + m.replaced[i].chord.getName());

    return "Replace the chords in bars " + juce::String (m.firstBar + 1) + " to " + juce::String (m.lastBar + 1) + "? "
           + juce::String ((int) m.replaced.size()) + " of them already have other chords (" + list.joinIntoString (", ")
           + (m.replaced.size() > 3 ? ", ..." : "") + ").";
}

//==============================================================================
namespace
{
    juce::String describeBass (const Chord& chord, int bass)
    {
        const int d = mod12 (bass - chord.root);
        const auto name = mh::getNoteName (bass);

        // Only a note the chord holds is its third or fifth (C over Eb: Eb isn't C's third).
        if ((d == 3 || d == 4) && chord.intervals.contains (d))
            return "its third, " + name + ", in the bass";

        if (d == 7 && chord.intervals.contains (d))
            return "its fifth, " + name + ", in the bass";

        if (chord.intervals.contains (d))
            return name + " in the bass";

        return name + ", which isn't in the chord, in the bass";
    }
} // namespace

juce::String explainYourClip (const ClipView& view)
{
    if (! view.found)
        return getText (Topic::yours);

    if (view.drums)
        return view.trackName + " plays drum sounds: its notes pick drums, not pitches, so it has no key.";

    const auto& r = view.result;
    juce::StringArray paragraphs;

    if (r.tooFew)
        paragraphs.add ("Only " + juce::String (r.numNotes) + (r.numNotes == 1 ? " note" : " notes")
                        + ": too few to tell the key. Chords are still named where notes sound together.");

    std::optional<MusicalKey> found;

    if (! r.tooFew)
    {
        found = r.key.songKey;
        auto text = r.key.name + " fits best: " + percent (r.key.fit) + "% of the notes, by length and velocity, are in it.";

        if (r.key.scale == mh::Scale::majorPentatonic || r.key.scale == mh::Scale::minorPentatonic || r.key.scale == mh::Scale::blues)
            text += " On the chord track and in the song's key this counts as " + r.key.songKey.getName() + ".";

        paragraphs.add (text);

        if (r.close)
        {
            auto close = "It's close: " + r.runnerUp.getName() + " fits almost as well. ";

            const bool plainScale = r.key.scale == mh::Scale::major || r.key.scale == mh::Scale::minor;

            if (r.relative && plainScale)
                close += "They share all their notes. ";

            // A mode or other scale is named in full, so the winner's name matches the sentence above.
            paragraphs.add (close + (plainScale ? r.ranking[0].getName() : r.key.name) + " wins because " + r.closeReason + ".");
        }

        if (r.key.fit < 0.85)
            paragraphs.add ("Only " + percent (r.key.fit) + "% of the notes fit any one key. The part may change key, or use many notes from outside it. "
                            "Try splitting the clip where it changes and detecting each half.");

        juce::StringArray outside;

        for (size_t i = 0; i < r.outside.size() && i < 5; ++i)
            outside.add (mh::getSpelledName (r.outside[i].pitchClass, r.key.tonic) + " in bar " + juce::String (r.outside[i].firstBar + 1) + ": " + r.outside[i].reason + ".");

        if (! outside.isEmpty())
            paragraphs.add (outside.joinIntoString (" "));

        if (view.songKey && *view.songKey != r.key.songKey)
            paragraphs.add ("The song is set to " + view.songKey->getName() + ". Set it to " + r.key.songKey.getName()
                            + " so the chord menu, the numerals and the scale filter follow this part.");
        else if (view.songKey)
            paragraphs.add ("The song is already in " + view.songKey->getName() + ".");
    }

    if (! r.melody)
    {
        const auto bars = getBarChords (r.segments);
        const auto numeralKey = found ? *found : view.songKey.value_or (MusicalKey { 0, MusicalKey::Scale::major });
        juce::StringArray names, numerals;
        std::vector<Chord> chords;

        for (auto& s : r.segments)
        {
            if (s.secondHalf)
                continue;

            names.add (s.chord ? s.chord->getName() : "-");

            if (s.chord)
            {
                numerals.add (guide::getNumeral (numeralKey, *s.chord));
                chords.push_back (*s.chord);
            }
        }

        auto text = "Chords: " + names.joinIntoString (" | ");

        if (found)
            text += " (" + numerals.joinIntoString (" ") + ")";

        paragraphs.add (text + ".");

        juce::StringArray slash, close, noThird, halves;

        for (auto& s : r.segments)
        {
            if (! s.chord)
                continue;

            if (s.chordBass >= 0)
                slash.add (barName (s) + " is " + s.getName() + ": " + withArticle (s.chord->getName()) + " chord with " + describeBass (*s.chord, s.chordBass) + ".");

            if (s.otherChord && s.otherGap < 0.05)
            {
                if (s.otherGap < 1.0e-6)
                    close.add (barName (s) + ": " + s.getName() + ", or " + mh::getChordName (*s.otherChord, s.otherBass)
                               + ": the same notes; the bass, " + mh::getNoteName (s.bass) + ", decides.");
                else
                    close.add (barName (s) + ": " + s.getName() + ", or " + mh::getChordName (*s.otherChord, s.otherBass)
                               + ": the notes fit both almost equally.");
            }

            if (s.noThird)
                noThird.add (barName (s) + ": " + mh::getNoteName (s.chord->root) + "5 played as " + mh::getNoteName (s.chord->root) + " and "
                             + mh::getNoteName (mod12 (s.chord->root + 7)) + " only; " + s.chord->getName() + " is the chord in the key.");

            if (s.secondHalf)
            {
                for (auto& first : r.segments)
                    if (first.bar == s.bar && ! first.secondHalf && first.chord)
                        halves.add ("Bar " + juce::String (s.bar + 1) + " changes chord halfway (" + first.chord->getName() + ", then " + s.chord->getName()
                                    + "). The chord track holds one chord per bar, so it gets " + first.chord->getName() + ".");
            }
        }

        for (auto* list : { &slash, &close, &noThird })
            if (! list->isEmpty())
                paragraphs.add (juce::StringArray (list->strings.getRawDataPointer(), juce::jmin (3, list->size())).joinIntoString (" "));

        if (found && ! chords.empty())
        {
            // The name looks at the chord a bar, as the chord track holds them.
            std::vector<Chord> barChords;

            for (auto& b : bars)
                barChords.push_back (b.chord);

            const auto name = guide::findProgressionName (*found, barChords);

            if (name.isNotEmpty())
                paragraphs.add ("This is the " + name + ".");
        }

        if (! halves.isEmpty())
            paragraphs.add (juce::StringArray (halves.strings.getRawDataPointer(), juce::jmin (3, halves.size())).joinIntoString (" "));
    }
    else if (! r.tooFew)
    {
        paragraphs.add ("This part plays one note at a time and outlines no chords, so DawIt names its key but not chords: a tune's notes alone don't fix the harmony. "
                        "Detect the chords of the chord part instead.");
    }

    paragraphs.add ("Check by ear: play the clip with the chord track and listen whether each chord feels right.");
    return paragraphs.joinIntoString ("\n\n");
}
} // namespace daw::gen::keyfinding
