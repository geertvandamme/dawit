#include "ReadingGuide.h"

#include "ChordGuide.h"
#include "ChordSheet.h"
#include "Harmony.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace daw::gen::readmusic
{
namespace nt = notation;

namespace
{
    // Texts hold sharp, flat, natural and note signs, so they go through UTF-8.
    juce::String utf (const char* text) { return juce::String::fromUTF8 (text); }

    juce::String sharp() { return utf ("\xe2\x99\xaf"); }
    juce::String flat() { return utf ("\xe2\x99\xad"); }

    /** "F♯4": a MIDI note spelled in the key, as the score writes it. */
    juce::String nameOf (int midi, const MusicalKey& key)
    {
        const auto p = nt::spell (midi, key, nullptr);
        return juce::String::charToString ("CDEFGAB"[p.letter]) + (p.alter > 0 ? sharp() : p.alter < 0 ? flat() : juce::String()) + juce::String (p.octave);
    }

    juce::String getClefName (nt::ClefChoice clef)
    {
        switch (clef)
        {
            case nt::ClefChoice::automatic:
            case nt::ClefChoice::treble:     return "treble clef";
            case nt::ClefChoice::bass:       return "bass clef";
            case nt::ClefChoice::grand:      return "grand staff";
            case nt::ClefChoice::guitar:     return "treble clef, an octave higher than it sounds";
            case nt::ClefChoice::bassGuitar: return "bass clef, an octave higher than it sounds";
        }

        return {};
    }

    /** "no sharps or flats", "1 sharp", "3 flats": counted from the signature the score draws (6 sharps at the top of the circle). */
    juce::String describeSignature (const MusicalKey& key)
    {
        const auto marks = nt::getKeySignature (guide::getCirclePosition (key), nt::Clef::treble);

        if (marks.empty())
            return "no sharps or flats";

        const int n = (int) marks.size();
        return juce::String (n) + (marks[0].alter > 0 ? (n == 1 ? " sharp" : " sharps") : (n == 1 ? " flat" : " flats"));
    }

    juce::String formatBpm (double bpm)
    {
        return juce::String (juce::roundToInt (bpm));
    }

    nt::Score makeScore (std::vector<MidiNote> notes, const MusicalKey& key, nt::ClefChoice clef, ChordMap chords = {},
                         std::vector<Section> sections = {}, double bpm = 120.0)
    {
        nt::Input in;
        in.notes = std::move (notes);
        in.key = key;
        in.clef = clef;
        in.chords = std::move (chords);
        in.sections = std::move (sections);
        in.tempo = TempoMap (bpm);
        return nt::build (in);
    }

    /** One note per beat. */
    std::vector<MidiNote> quarters (const std::vector<int>& pitches, double firstBeat = 0.0)
    {
        std::vector<MidiNote> notes;

        for (size_t i = 0; i < pitches.size(); ++i)
            notes.push_back ({ pitches[i], firstBeat + (double) i, 1.0, 0.8f });

        return notes;
    }

    /** One note per bar. */
    std::vector<MidiNote> wholes (const std::vector<int>& pitches)
    {
        std::vector<MidiNote> notes;

        for (size_t i = 0; i < pitches.size(); ++i)
            notes.push_back ({ pitches[i], 4.0 * (double) i, 4.0, 0.8f });

        return notes;
    }

    Panel makePanel (const juce::String& label, nt::Score score)
    {
        Panel p;
        p.label = label;
        p.score = std::move (score);
        return p;
    }

    /** The key's scale degree as a MIDI note above middle C (degree 7 is the tonic an octave up). */
    int scaleNote (const MusicalKey& key, int degree)
    {
        const auto intervals = key.getIntervals();
        const int n = juce::jmax (1, intervals.size());
        return 60 + key.tonic + intervals[degree % n] + 12 * (degree / n);
    }

    std::vector<int> range (std::initializer_list<int> values) { return std::vector<int> (values); }
} // namespace

//==============================================================================
juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::staff:          return "The staff";
        case Topic::clefs:          return "Treble and bass clefs";
        case Topic::trebleNotes:    return "Notes in the treble clef";
        case Topic::bassNotes:      return "Notes in the bass clef";
        case Topic::ledgerLines:    return "Ledger lines and middle C";
        case Topic::grandStaff:     return "The grand staff and the piano";
        case Topic::accidentals:    return "Sharps, flats and naturals";
        case Topic::keySignatures:  return "Key signatures";
        case Topic::timeAndTempo:   return "Time signature, bars and tempo";
        case Topic::noteValues:     return "Note values on the staff";
        case Topic::ties:           return "Ties and the beat";
        case Topic::chordSymbols:   return "Chord symbols above the staff";
        case Topic::howDawItWrites: return "How DawIt writes your MIDI";
        case Topic::yours:          return "Your part";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::staff:          return "Five lines: higher on the page is higher in pitch";
        case Topic::clefs:          return "The clef says which line is which note";
        case Topic::trebleNotes:    return utf ("E G B D F on the lines, F A C E in the spaces");
        case Topic::bassNotes:      return utf ("G B D F A on the lines, A C E G in the spaces");
        case Topic::ledgerLines:    return "Short extra lines above and below the staff";
        case Topic::grandStaff:     return "Two staves joined: right hand above, left below";
        case Topic::accidentals:    return "Up or down a semitone, until the bar line";
        case Topic::keySignatures:  return "The sharps or flats at the start of every line";
        case Topic::timeAndTempo:   return utf ("4/4, bar lines, bar numbers and \xe2\x99\xa9 = 120");
        case Topic::noteValues:     return "Stems, flags, beams, dots, rests and triplets";
        case Topic::ties:           return "Why one MIDI note can be written as two";
        case Topic::chordSymbols:   return "Am over the bar: what a band plays from";
        case Topic::howDawItWrites: return "Rounded for reading; the MIDI stays as played";
        case Topic::yours:          return "The selected track, written out and explained";
    }

    return {};
}

//==============================================================================
Example getExample (Topic topic, const Part& part)
{
    const MusicalKey cMajor { 0, MusicalKey::Scale::major };
    const auto& songKey = part.input.key;
    Example ex;

    switch (topic)
    {
        case Topic::staff:
        {
            auto p = makePanel ("Treble staff, C4 up to G5, one step at a time",
                                makeScore (wholes (range ({ 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79 })), cMajor, nt::ClefChoice::treble));
            p.showNoteNames = true;
            ex.panels.push_back (std::move (p));
            break;
        }

        case Topic::clefs:
        {
            auto treble = makePanel ("Treble clef: the curl circles G4", makeScore (wholes ({ 67 }), cMajor, nt::ClefChoice::treble));
            treble.accentNotes = { 67 };
            auto bass = makePanel ("Bass clef: the dots surround F3", makeScore (wholes ({ 53 }), cMajor, nt::ClefChoice::bass));
            bass.accentNotes = { 53 };
            auto middle = makePanel ("Middle C (C4) sits between the two", makeScore (wholes ({ 60 }), cMajor, nt::ClefChoice::grand));
            middle.accentNotes = { 60 };
            ex.panels = { std::move (treble), std::move (bass), std::move (middle) };
            break;
        }

        case Topic::trebleNotes:
        {
            const auto pitches = range ({ 64, 65, 67, 69, 71, 72, 74, 76, 77 });
            auto p = makePanel ("Lines: E4 G4 B4 D5 F5. Spaces: F4 A4 C5 E5", makeScore (wholes (pitches), cMajor, nt::ClefChoice::treble));
            p.showNoteNames = true;
            p.keyboardNotes = pitches;
            ex.panels.push_back (std::move (p));
            break;
        }

        case Topic::bassNotes:
        {
            const auto pitches = range ({ 43, 45, 47, 48, 50, 52, 53, 55, 57 });
            auto p = makePanel ("Lines: G2 B2 D3 F3 A3. Spaces: A2 C3 E3 G3", makeScore (wholes (pitches), cMajor, nt::ClefChoice::bass));
            p.showNoteNames = true;
            p.keyboardNotes = pitches;
            ex.panels.push_back (std::move (p));
            break;
        }

        case Topic::ledgerLines:
        {
            auto t = makePanel ("Treble staff: C4, A3, A5, C6", makeScore (quarters ({ 60, 57, 81, 84 }), cMajor, nt::ClefChoice::treble));
            t.showNoteNames = true;
            t.accentNotes = { 60 };
            auto b = makePanel ("Bass staff: C4, E4, E2, C2", makeScore (quarters ({ 60, 64, 40, 36 }), cMajor, nt::ClefChoice::bass));
            b.showNoteNames = true;
            b.accentNotes = { 60 };
            ex.panels = { std::move (t), std::move (b) };
            break;
        }

        case Topic::grandStaff:
        {
            const auto pitches = range ({ 48, 55, 60, 64, 67, 72 });
            auto p = makePanel ("Grand staff: C3 G3 C4 E4 G4 C5", makeScore (quarters (pitches), cMajor, nt::ClefChoice::grand));
            p.keyboardNotes = pitches;
            p.accentNotes = { 53, 60, 67 };     // the landmarks F3, C4 and G4
            p.showNoteNames = true;
            ex.panels.push_back (std::move (p));
            break;
        }

        case Topic::accidentals:
        {
            ChordMap chords;
            chords[0] = Chord { 2, { 0, 4, 7 } };   // D, so 66 is spelled F sharp
            const std::vector<MidiNote> notes { { 65, 0, 1, 0.8f }, { 66, 1, 1, 0.8f }, { 66, 2, 1, 0.8f }, { 65, 3, 1, 0.8f },
                                                { 70, 4, 1, 0.8f }, { 71, 5, 1, 0.8f }, { 65, 6, 2, 0.8f } };
            ex.panels.push_back (makePanel ("Bar 1: a sharp lasts until the natural. Bar 2: the bar line cleared it",
                                            makeScore (notes, cMajor, nt::ClefChoice::treble, chords)));
            ex.playNotes = notes;
            break;
        }

        case Topic::keySignatures:
        {
            const auto tonic = [] (const MusicalKey& k, int base) { return base + k.tonic; };
            const auto one = [&] (const juce::String& label, const MusicalKey& k, nt::ClefChoice clef)
            {
                return makePanel (label, makeScore (wholes ({ tonic (k, clef == nt::ClefChoice::bass ? 48 : 60) }), k, clef));
            };
            ex.panels.push_back (one ("Your song, treble clef", songKey, nt::ClefChoice::treble));
            ex.panels.push_back (one ("Your song, bass clef", songKey, nt::ClefChoice::bass));
            ex.panels.push_back (one ("G major: F" + sharp(), { 7, MusicalKey::Scale::major }, nt::ClefChoice::treble));
            ex.panels.push_back (one ("D major: F" + sharp() + " C" + sharp(), { 2, MusicalKey::Scale::major }, nt::ClefChoice::treble));
            ex.panels.push_back (one ("F major: B" + flat(), { 5, MusicalKey::Scale::major }, nt::ClefChoice::treble));
            ex.panels.push_back (one ("B" + flat() + " major: B" + flat() + " E" + flat(), { 10, MusicalKey::Scale::major }, nt::ClefChoice::treble));
            break;
        }

        case Topic::timeAndTempo:
        {
            std::vector<MidiNote> notes = quarters ({ scaleNote (songKey, 0), scaleNote (songKey, 1), scaleNote (songKey, 2), scaleNote (songKey, 3) });
            notes.push_back ({ scaleNote (songKey, 4), 4.0, 4.0, 0.8f });
            ex.panels.push_back (makePanel ("The start of a line: clef, key signature, time signature, tempo, bars 1 and 2, the final bar line",
                                            makeScore (notes, songKey, nt::ClefChoice::treble, {}, {}, part.input.tempo.getBpmAt (0.0))));
            ex.panels.back().numBars = 2;
            break;
        }

        case Topic::noteValues:
        {
            const double third = 1.0 / 3.0;
            const std::vector<MidiNote> notes { { 67, 0.0, 2.0, 0.8f }, { 74, 2.0, 1.0, 0.8f }, { 72, 3.0, 0.5, 0.8f }, { 71, 3.5, 0.5, 0.8f },
                                                { 64, 4.0, 1.5, 0.8f }, { 65, 5.5, 0.5, 0.8f },
                                                { 67, 6.0, third, 0.8f }, { 69, 6.0 + third, third, 0.8f }, { 71, 6.0 + 2.0 * third, third, 0.8f } };
            ex.panels.push_back (makePanel ("Bar 1: half, quarter, two beamed eighths. Bar 2: dotted quarter, eighth, triplet, quarter rest",
                                            makeScore (notes, cMajor, nt::ClefChoice::treble)));
            ex.playNotes = notes;
            break;
        }

        case Topic::ties:
        {
            const std::vector<MidiNote> notes { { 69, 0.0, 0.5, 0.8f }, { 72, 0.5, 1.0, 0.8f }, { 71, 1.5, 0.5, 0.8f }, { 69, 2.0, 1.5, 0.8f }, { 67, 3.5, 1.0, 0.8f } };
            auto p = makePanel ("The MIDI notes above, and the same rhythm written below", makeScore (notes, cMajor, nt::ClefChoice::treble));

            for (auto& n : notes)
                p.played.push_back ({ n.start, n.length, n.pitch });

            ex.panels.push_back (std::move (p));
            ex.playNotes = notes;
            break;
        }

        case Topic::chordSymbols:
        {
            const MusicalKey aMinor { 9, MusicalKey::Scale::minor };
            ChordMap chords;
            chords[0] = Chord { 9, { 0, 3, 7 } };   // Am
            chords[1] = Chord { 5, { 0, 4, 7 } };   // F
            auto notes = quarters ({ 69, 72, 76, 72 });
            const auto second = quarters ({ 65, 69, 72, 69 }, 4.0);
            notes.insert (notes.end(), second.begin(), second.end());
            ex.panels.push_back (makePanel ("Am in bar 1, F in bar 2, and the section name in a box",
                                            makeScore (notes, aMinor, nt::ClefChoice::treble, chords, { { 0, 2, SectionType::chorus, "Chorus" } })));
            break;
        }

        case Topic::howDawItWrites:
        {
            const std::vector<MidiNote> played { { 64, 0.03, 0.46, 0.8f }, { 67, 0.52, 0.41, 0.8f }, { 69, 0.98, 0.95, 0.8f },
                                                 { 67, 2.04, 0.47, 0.8f }, { 64, 2.49, 1.40, 0.8f } };
            auto p = makePanel ("As played above (a little off the grid), as written below", makeScore (played, cMajor, nt::ClefChoice::treble));

            for (auto& n : played)
                p.played.push_back ({ n.start, n.length, n.pitch });

            ex.panels.push_back (std::move (p));
            ex.playNotes = played;
            break;
        }

        case Topic::yours:
        {
            if (! part.hasTrack || part.input.notes.empty())
                break;

            auto score = nt::build (part.input);
            const int bar = juce::jmax (0, (int) std::floor (part.playheadBeat / 4.0));
            const int numBars = score.numBars;
            auto p = makePanel (part.trackName, std::move (score));
            p.firstBar = juce::jmax (0, juce::jmin (bar - 1, numBars - 4));
            p.numBars = 4;
            ex.panels.push_back (std::move (p));
            break;
        }
    }

    return ex;
}

//==============================================================================
PartReport analysePart (const Part& part)
{
    PartReport r;
    r.hasTrack = part.hasTrack;

    if (! part.hasTrack)
    {
        r.text = "Select a MIDI track to see its part written out here.";
        return r;
    }

    if (part.input.notes.empty())
    {
        r.text = part.trackName + " has no notes yet. Draw some in the piano roll and they'll appear here.";
        return r;
    }

    const auto& notes = part.input.notes;
    const auto& key = part.input.key;
    const auto score = nt::build (part.input);
    r.numNotes = (int) notes.size();
    r.clef = score.clefUsed;

    double firstStart = 1.0e9, lastStart = 0.0;
    int low = 127, high = 0;

    for (auto& n : notes)
    {
        firstStart = std::min (firstStart, n.start);
        lastStart = std::max (lastStart, n.start);
        low = std::min (low, n.pitch);
        high = std::max (high, n.pitch);
    }

    r.firstBar = (int) std::floor (firstStart / 4.0 + 1.0e-9) + 1;
    r.lastBar = (int) std::floor (lastStart / 4.0 + 1.0e-9) + 1;

    // Where each source note is written, and the ledger lines of the highest and lowest.
    std::vector<double> writtenStart (notes.size(), -1.0);
    std::vector<int> writtenBar (notes.size(), 0);
    int highStep = 0, lowStep = 0, highestMidi = -1, lowestMidi = 1000;
    std::set<std::pair<int, int>> tripletBeatSet;

    for (auto& staff : score.staves)
    {
        std::vector<std::pair<double, const nt::Event*>> firsts;

        for (size_t b = 0; b < staff.bars.size(); ++b)
        {
            for (auto& e : staff.bars[b].events)
            {
                if (e.isRest)
                    continue;

                if (e.triplet)
                    tripletBeatSet.insert ({ (int) b, e.start / nt::unitsPerBeat });

                for (auto& p : e.pitches)
                {
                    if (p.midi > highestMidi) { highestMidi = p.midi; highStep = p.step; }
                    if (p.midi < lowestMidi)  { lowestMidi = p.midi; lowStep = p.step; }
                }

                if (e.tiedFromPrevious)
                    continue;

                const double start = (double) b * 4.0 + e.getStartBeat();
                firsts.push_back ({ start, &e });

                for (int idx : e.sourceNotes)
                    if (idx >= 0 && idx < (int) notes.size())
                    {
                        writtenStart[(size_t) idx] = start;
                        writtenBar[(size_t) idx] = (int) b + 1;
                    }
            }
        }

        std::sort (firsts.begin(), firsts.end(), [] (auto& a, auto& b) { return a.first < b.first; });

        for (size_t i = 0; i + 1 < firsts.size(); ++i)
        {
            size_t next = i + 1;

            while (next < firsts.size() && firsts[next].first <= firsts[i].first + 1.0e-9)
                ++next;

            if (next >= firsts.size())
                continue;

            for (int idx : firsts[i].second->sourceNotes)
                if (idx >= 0 && idx < (int) notes.size() && notes[(size_t) idx].getEnd() > firsts[next].first + 0.0625)
                    ++r.held;
        }
    }

    r.tripletBeats = (int) tripletBeatSet.size();

    for (size_t i = 0; i < notes.size(); ++i)
    {
        if (writtenStart[i] < 0.0)
            continue;

        const double moved = std::abs (notes[i].start - writtenStart[i]);

        if (moved > 0.0625)
            ++r.moved;

        if (moved > r.maxMoved)
        {
            r.maxMoved = moved;
            r.maxMovedBar = writtenBar[i];
        }
    }

    // Notes outside the key, and the raised 7th of a minor key.
    const auto intervals = key.getIntervals();
    const int raisedPc = (key.tonic + 11) % 12;
    int raisedMidi = -1;

    for (auto& n : notes)
    {
        const int pc = ((n.pitch % 12) + 12) % 12;
        bool inKey = false;

        for (int interval : intervals)
            inKey = inKey || (key.tonic + interval) % 12 == pc;

        if (inKey)
            continue;

        ++r.outsideKey;

        if (harmony::isMinorKey (key) && pc == raisedPc)
        {
            ++r.raisedSeventh;
            raisedMidi = n.pitch;
        }
    }

    const auto lowName = nameOf (low, key), highName = nameOf (high, key);
    juce::String text;
    text << part.trackName << " has " << r.numNotes << (r.numNotes == 1 ? " note" : " notes") << " in bars " << r.firstBar
         << utf ("\xe2\x80\x93") << r.lastBar << ". It's written in the " << getClefName (r.clef) << " because ";

    if (! part.autoClef)
        text << "you chose it in the Clef menu.";
    else if (r.clef == nt::ClefChoice::treble)
        text << "its lowest note, " << lowName << ", is F3 or higher.";
    else if (r.clef == nt::ClefChoice::bass)
        text << "its highest note, " << highName << ", is G4 or lower.";
    else
    {
        int treble = 0;

        for (auto& n : notes)
            treble += n.pitch >= 60 ? 1 : 0;

        text << "it runs from " << lowName << " to " << highName << ", too wide for one staff: " << treble
             << " notes from middle C up are in the treble staff and " << ((int) notes.size() - treble) << " in the bass staff.";
    }

    const int highLedger = nt::getLedgerLines (highStep), lowLedger = nt::getLedgerLines (lowStep);

    if (highLedger > 0 || lowLedger > 0)
    {
        text << "\n\n";

        if (highLedger > 0 && lowLedger > 0)
            text << "The highest note, " << highName << ", needs " << highLedger << " ledger " << (highLedger == 1 ? "line" : "lines")
                 << "; the lowest, " << lowName << ", needs " << lowLedger << ".";
        else if (highLedger > 0)
            text << "The highest note, " << highName << ", needs " << highLedger << " ledger " << (highLedger == 1 ? "line" : "lines") << ".";
        else
            text << "The lowest note, " << lowName << ", needs " << lowLedger << " ledger " << (lowLedger == 1 ? "line" : "lines") << ".";
    }

    text << "\n\nThe key is " << sheet::getKeyName (key) << ", with " << describeSignature (key)
         << " at the start of every line.";

    if (r.outsideKey > 0)
    {
        text << " " << r.outsideKey << " of the " << r.numNotes << " notes are outside the key, so they carry a sharp, flat or natural.";

        if (r.raisedSeventh > 0)
            text << " " << r.raisedSeventh << " of them are " << nameOf (raisedMidi, key).dropLastCharacters (1)
                 << ", the raised seventh that minor keys borrow to lead home.";
    }
    else
        text << " Every note is in the key, so the score needs no accidentals.";

    text << "\n\n";

    if (r.moved == 0)
        text << "Every note starts on the grid, so the score shows exactly what plays.";
    else
        text << r.moved << " notes start more than a quarter of a sixteenth (0.0625 beats) away from where they're written; the most is "
             << juce::String (r.maxMoved, 2) << " beats, in bar " << r.maxMovedBar << ".";

    if (r.tripletBeats > 0)
        text << " " << r.tripletBeats << " beats are written as triplets.";

    if (r.held > 0)
        text << "\n\n" << r.held << " notes are still held when the next one starts; the score writes them up to that next note.";

    // Advice, each only when its condition holds.
    if (r.numNotes >= 8 && r.moved * 4 > r.numNotes)
        r.advice.add ("This part was played freely, so the score had to round a lot. That's fine for listening. For a cleaner score, put the notes on the grid in the piano roll with Snap at 1/16, but listen first: the feel changes too.");

    const auto name = part.trackName.toLowerCase();

    if (part.autoClef && r.clef == nt::ClefChoice::bass && lowLedger >= 4 && low < 40 && name.contains ("bass"))
        r.advice.add ("The lowest notes need " + juce::String (lowLedger) + " ledger lines. Bass guitar parts are written an octave higher than they sound: choose Clef > Bass guitar in the right-click menu to read it as bass players do.");

    if (part.autoClef && r.clef == nt::ClefChoice::grand && low >= 40 && (name.contains ("guitar") || name.contains ("gtr")))
        r.advice.add ("Guitar music is written on one treble staff, an octave higher than it sounds: choose Clef > Guitar to read it that way.");

    if (r.clef == nt::ClefChoice::treble && highLedger >= 4)
        r.advice.add ("The highest notes need " + juce::String (highLedger) + " ledger lines. Printed music would use 8va (play an octave higher) here; DawIt doesn't write it yet, so count the ledger lines in pairs.");

    for (auto& a : r.advice)
        text << "\n\n" << a;

    r.text = text;
    return r;
}

//==============================================================================
juce::String explain (Topic topic, const Part& part)
{
    const auto& key = part.input.key;
    const auto keyName = sheet::getKeyName (key);
    const auto signature = describeSignature (key);
    const auto bpm = formatBpm (part.input.tempo.getBpmAt (0.0));
    juce::String body;

    switch (topic)
    {
        case Topic::staff:
            body = utf ("Written music is drawn on a staff (in Britain often called a stave): five lines with four spaces between them. Each line and each space is one note. A note sits either on a line (the line runs through it) or in a space (between two lines).\n\n"
                        "Higher on the staff means higher in pitch. From a line to the space just above it is one step up the white keys of a piano: E to F, F to G. From a line to the next line skips a note: E to G. So you can see a tune's shape before you know a single note name: up, down, a leap, a repeated note.\n\n"
                        "Music uses seven letters, A B C D E F G, and after G they start again at A. The black keys have no letters of their own: they're written with sharps and flats (see Sharps, flats and naturals).\n\n"
                        "The staff alone doesn't say which line is which note. That's the job of the clef, the sign at the start of every line. Notes too high or too low for the five lines get short lines of their own, called ledger lines.\n\n"
                        "In the picture, each note is one step higher than the one before, from a line to a space and on. Click a note to hear it.");
            break;

        case Topic::clefs:
            body = utf ("The clef at the start of a staff says which notes the lines stand for. Without it, the same dot could be any note.\n\n"
                        "The treble clef is also called the G clef. It grew out of a letter G, and its curl wraps round the second line from the bottom. That line is G4, the G just above middle C. High instruments and voices read it: the piano's right hand, singers, guitar, violin, flute, trumpet.\n\n"
                        "The bass clef is the F clef. It grew out of a letter F, and its two dots sit either side of the second line from the top. That line is F3, the F just below middle C. Low instruments read it: the piano's left hand, bass guitar, cello, trombone, and low voices.\n\n"
                        "G4 is seven semitones (a fifth) above middle C, and F3 seven semitones below it, so the two clefs sit either side of middle C. The number after a note counts octaves from C. Middle C is C4, MIDI note 60 in DawIt (see What is MIDI?), and every note from C4 up to the B above it has a 4.\n\n"
                        "You may meet a treble clef with a small 8 under it: the music sounds an octave lower than written. Guitar music is written an octave higher than it sounds, so it fits on the treble staff; bass guitar music is written an octave higher in the bass clef. Many printed parts leave the 8 off, because guitarists and bassists know.\n\n"
                        "Other clefs exist (the viola's alto clef puts middle C on the middle line), but almost all the music you'll meet uses treble and bass.");
            break;

        case Topic::trebleNotes:
            body = utf ("From the bottom line up, the five lines of the treble staff are E G B D F: E4, G4, B4, D5 and F5. A sentence helps you remember them: Every Good Boy Deserves Football (or Fudge, or Does Fine; there are many versions).\n\n"
                        "The four spaces, from the bottom up, are F A C E: F4, A4, C5 and E5. They spell FACE.\n\n"
                        "Mnemonics are a start, but fluent readers don't spell out a sentence for every note. They know a few landmark notes by heart and count steps from them:\n"
                        "  G4, the line the clef curls round;\n"
                        "  middle C (C4), on one ledger line below the staff;\n"
                        "  C5, in the third space, an octave above middle C;\n"
                        "  F5, the top line.\n"
                        "A note one step higher is the next letter; a note on the next line up skips a letter.\n\n"
                        "Try it: the second space is A4, the A that orchestras tune to (440 Hz). Two spaces higher is E5.\n\n"
                        "In the picture, the lines and spaces are named and the keyboard below lights each note. Click one to hear it.");
            break;

        case Topic::bassNotes:
            body = utf ("From the bottom line up, the five lines of the bass staff are G B D F A: G2, B2, D3, F3 and A3. A sentence to remember them: Good Boys Do Fine Always (or Deserve Fudge Always).\n\n"
                        "The four spaces, from the bottom up, are A C E G: A2, C3, E3 and G3. All Cows Eat Grass.\n\n"
                        "The bass clef is not the treble clef moved down: the same line is a different letter. The treble staff's bottom line is E, the bass staff's is G. That's why people who read only treble at first misread the bass clef by two letters.\n\n"
                        "Landmarks to read from: F3, the line between the clef's dots; middle C (C4), on one ledger line above the staff; and C3, in the second space, an octave below middle C.\n\n"
                        "A bass guitar's open strings sound E1, A1, D2 and G2. They are written an octave higher, as E2, A2, D3 and G3, so they sit on and just below the staff instead of far under it.");
            break;

        case Topic::ledgerLines:
            body = utf ("When a note is too high or too low for the staff, short lines are drawn just for it: ledger lines. They work like the staff's own lines, so the letters carry on in order.\n\n"
                        "Below the treble staff: D4 hangs just under the bottom line, middle C (C4) sits on one ledger line, B3 hangs under that, and A3 sits on two ledger lines.\n"
                        "Above the bass staff: B3 sits just above the top line, middle C (C4) on one ledger line, D4 just above that, and E4 on two ledger lines.\n\n"
                        "So middle C is written in both clefs, on one ledger line: below the treble staff and above the bass staff. It's the same key on the piano, the C nearest the middle of the keyboard.\n\n"
                        "Ledger lines are easy to miscount. Learn two more landmarks: two ledger lines above the treble staff is C6, and two below the bass staff is C2.\n\n"
                        "A part that keeps needing three or more ledger lines is usually written in the other clef, or with 8va over it (play an octave higher than written). DawIt picks the clef that needs the fewest ledger lines; it doesn't write 8va yet.");
            break;

        case Topic::grandStaff:
            body = utf ("Piano music needs high and low notes at once, so it uses two staves joined by a brace, the curly bracket at the start: the treble staff on top and the bass staff below. Together they are the grand staff (the great stave in Britain).\n\n"
                        "Usually the top staff is for the right hand and the bottom staff for the left. That's a habit, not a law: pianists play what's written with whichever hand suits, and composers move notes between the staves.\n\n"
                        "Middle C sits between the two staves, on one ledger line under the treble staff or one above the bass staff. It's the same note either way; where it's written usually says which hand plays it.\n\n"
                        "Bar lines run through both staves, and notes lined up above each other are played together: a chord in the left hand and a melody note in the right hand at the same place sound at the same moment.\n\n"
                        "When DawIt writes a part on a grand staff, it puts every note from middle C up in the treble staff and every note below in the bass staff. Real piano music splits by hand, not at a fixed note, so a melody that dips below middle C may jump between staves.\n\n"
                        "In the picture, the notes on the grand staff light up on the keyboard below.");
            break;

        case Topic::accidentals:
            body = utf ("A sharp (\xe2\x99\xaf) raises a note by a semitone, the next key up on the piano: F\xe2\x99\xaf is the black key just above F. A flat (\xe2\x99\xad) lowers it by a semitone: B\xe2\x99\xad is the black key just below B. A natural (\xe2\x99\xae) cancels a sharp or flat and takes the note back.\n\n"
                        "These signs are called accidentals. How to read them:\n"
                        "  They're written before the note but said after it: \"F sharp\".\n"
                        "  An accidental lasts until the next bar line, for every later note on the same line or space in that bar. After the bar line it's gone.\n"
                        "  It only counts for that line or space, not the same letter in another octave. Many publishers print it again in the other octave anyway, to be clear.\n"
                        "  A note tied over the bar line keeps its accidental; a new note in the next bar doesn't.\n\n"
                        "One key can have two names. The black key between F and G is F\xe2\x99\xaf and also G\xe2\x99\xad. Which is written depends on the key: in D major it's F\xe2\x99\xaf, because a scale uses each letter once (D E F\xe2\x99\xaf G A B C\xe2\x99\xaf). DawIt picks the spelling from the song's key and the bar's chord.\n\n"
                        "Rarer: a double sharp (\xf0\x9d\x84\xaa) raises a note by two semitones and a double flat (\xe2\x99\xad\xe2\x99\xad) lowers it by two.\n\n"
                        "In the picture, the first bar has a sharp that lasts until a natural cancels it. In the second, the bar line has cleared it.");
            break;

        case Topic::keySignatures:
            body = utf ("Most songs use the seven notes of one key. Instead of writing the same sharps or flats on every note, they're written once at the start of every line, after the clef: the key signature. A sharp on the F line means every F is F\xe2\x99\xaf, in every octave, unless an accidental says otherwise.\n\n"
                        "Sharps always come in the same order, F C G D A E B, and flats in the opposite order, B E A D G C F. So two sharps are always F\xe2\x99\xaf and C\xe2\x99\xaf, and three flats are always B\xe2\x99\xad, E\xe2\x99\xad and A\xe2\x99\xad. (A sentence for the order: Father Charles Goes Down And Ends Battle; read it backwards for flats.)\n\n"
                        "To find the key:\n"
                        "  With sharps, the major key is a semitone above the last sharp: F\xe2\x99\xaf and C\xe2\x99\xaf mean D major.\n"
                        "  With flats, the major key is the second-last flat: B\xe2\x99\xad, E\xe2\x99\xad, A\xe2\x99\xad mean E\xe2\x99\xad major. One flat (B\xe2\x99\xad) is F major.\n"
                        "  No sharps or flats is C major.\n\n"
                        "Each key signature also fits one minor key, the relative minor, three semitones below the major: D major's signature is also B minor's, and C major's (none) is A minor's. The music tells you which. A song in A minor often ends on A, and often has G\xe2\x99\xaf written as an accidental: the raised seventh, which the key signature leaves out.\n\n"
                        "Your song is in {key}: {signature}. In the score, the key signature is on every line; the time signature only on the first. The circle of fifths in the chord guide shows all twelve keys and their signatures.")
                       .replace ("{key}", keyName).replace ("{signature}", signature);
            break;

        case Topic::timeAndTempo:
            body = utf ("After the clef and key signature comes the time signature: two numbers stacked like a fraction. 4/4 means four beats in a bar, each a quarter note; it's sometimes written as a C, for common time. The rhythm guide explains other time signatures; DawIt works in 4/4.\n\n"
                        "Bar lines divide the music into bars (measures in America). In 4/4 every bar adds up to exactly four beats, notes and rests together. A double bar line, thin then thick, marks the end.\n\n"
                        "Bar numbers are printed small above the staff so musicians can say \"from bar 17\". In DawIt's score they match the ruler.\n\n"
                        "The tempo mark \xe2\x99\xa9 = {bpm} at the top means {bpm} quarter notes a minute: the same as {bpm} BPM. Older music often uses Italian words instead, such as Andante (at a walking pace) or Allegro (fast).\n\n"
                        "Section names such as Verse and Chorus are printed in boxes above the bar where they start. They're rehearsal marks: places a band can start from.\n\n"
                        "In the picture: the start of a line with your song's tempo, two bars and the end.")
                       .replace ("{bpm}", bpm);
            break;

        case Topic::noteValues:
            body = utf ("The rhythm guide says how long each note value lasts. On the staff, you read the length from the note's shape and the pitch from where its head sits.\n\n"
                        "Stems: notes below the middle line have their stem going up, on the right of the head; notes on or above the middle line have it going down, on the left. It changes nothing in the sound: it keeps the stems inside the staff so they don't collide with other lines. A stem is about an octave long.\n\n"
                        "Flags and beams: an eighth note has one flag and a sixteenth two. Eighths and sixteenths in the same beat have their flags joined into beams, so you can see each beat as a group: four sixteenths under a double beam make one beat. DawIt beams by the beat. Some printed music beams four eighths across beats 1 and 2 or 3 and 4, but beams don't usually cross the middle of a 4/4 bar.\n\n"
                        "Dots: a dot after the head makes the note half as long again. A dotted quarter is 1.5 beats.\n\n"
                        "Rests: a whole rest hangs from the fourth line and a half rest sits on the middle line, like a hat. Quarter, eighth and sixteenth rests have shapes of their own. A whole rest also stands for a whole empty bar.\n\n"
                        "Triplets: three notes with a 3 over them fill the time of two. Three eighth-note triplets fill one beat.\n\n"
                        "In the picture: a half note, a quarter and two beamed eighths in the first bar; a dotted quarter, an eighth, a triplet and a quarter rest in the second. Press Play to hear it.");
            break;

        case Topic::ties:
            body = utf ("A tie is a curve joining two notes of the same pitch: play the first and hold it through the second, without playing again. A slur looks similar but joins different notes, and means \"play them smoothly\". DawIt writes ties but not slurs, because MIDI doesn't record phrasing.\n\n"
                        "A note that crosses a bar line needs a tie, because a 4/4 bar holds four beats and no more. A note that starts on beat 4 and lasts two beats is written as a quarter tied to a quarter in the next bar.\n\n"
                        "Ties are also used inside a bar, so you can see where the beats are:\n"
                        "  A note that starts between beats and lasts past the next beat is split at that beat. An eighth note on the \"and\" of 1 that lasts a whole beat is written as two tied eighths, so beat 2 is visible.\n"
                        "  In 4/4 the middle of the bar, beat 3, stays visible. A half note on beat 2 is written as two tied quarters. Only notes that start on beat 1 (a half, a dotted half or a whole note) hold through beat 3 without a tie.\n\n"
                        "This is a convention, not a law. Many writers use short forms for common syncopations (eighth, quarter, eighth), and some allow a half note on beat 2. DawIt writes the strict form, which is the easiest to count when you're learning.\n\n"
                        "In the picture, the top shows the MIDI notes as the piano roll does; below is how the score writes the same rhythm, with ties at the beat and over the bar line.");
            break;

        case Topic::chordSymbols:
            body = utf ("Chord names above the staff, such as Am, F, C or G7, tell a guitarist or pianist which chord to play from that point until the next one. They're the same names as on DawIt's chord track: a letter for the root, m for minor, 7 for a seventh (the chord guide explains them).\n\n"
                        "A page with a melody and chord symbols, often with the lyrics under the notes, is a lead sheet. It's a common way to write down a pop or jazz song for a band: the singer reads the melody, and the others make their parts from the chords.\n\n"
                        "Chord symbols are spelled in the song's key, like the notes: B\xe2\x99\xad in F major, A\xe2\x99\xaf" "m in F\xe2\x99\xaf major. DawIt takes them from the chord track and shows a symbol where the chord changes and at the start of each section, not on every bar.\n\n"
                        "Melody notes that belong to the chord sound settled; others add colour or lead to the next note. Hover a note in the score to see whether it's in the bar's chord.");
            break;

        case Topic::howDawItWrites:
            body = utf ("MIDI records when each key went down and came up, very precisely. Nobody plays exactly on the beat, so a note played \"on beat 2\" may start 0.03 beats late and last 0.94 beats. Written exactly, that would need tiny rests and odd ties nobody could read. So the score rounds, the way a musician writing a part down by ear would:\n"
                        "  Each beat is read either as sixteenths or as eighth-note triplets, whichever fits its notes better, and every start and end moves to the nearest of those points.\n"
                        "  Notes that start together on one staff become a chord. The score writes one line of notes per staff, so a note still held when the next one starts is written up to that next note.\n"
                        "  Long notes are split with ties at bar lines and beats (see Ties and the beat).\n"
                        "  Sharps and flats come from the song's key and the bar's chord: G\xe2\x99\xaf in A minor, not A\xe2\x99\xad.\n"
                        "  The clef is the one that needs the fewest ledger lines: treble when the lowest note is F3 or higher, bass when the highest is G4 or lower, and a grand staff, split at middle C, when the part needs both.\n\n"
                        "The rounding is only for the score. The MIDI isn't changed and plays exactly as recorded. Hover a note to see how far it moved.\n\n"
                        "DawIt doesn't write everything a printed part has yet. Swung eighths are usually written as triplets (a quarter and an eighth in each beat), and velocity isn't written as dynamics such as p (soft) and f (loud).\n\n"
                        "In the picture: a rhythm as it was played, slightly off the grid, and how the score writes it.");
            break;

        case Topic::yours:
            return getTopicName (topic) + "\n\n" + analysePart (part).text;
    }

    return getTopicName (topic) + "\n\n" + body;
}
} // namespace daw::gen::readmusic
