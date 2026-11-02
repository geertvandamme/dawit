#include "ArpeggiatorGuide.h"
#include "engine/MusicalKey.h"

#include <algorithm>
#include <set>

namespace daw::arptext
{
using arp::Knob;
using arp::StepKind;

namespace
{
    const char* const knobNames[] = { "Order", "Octaves", "Rate", "Gate", "Swing", "Steps", "Accent", "Latch" };

    juce::String getRateName (const arp::Settings& s)
    {
        return stepgrid::stepLengths[juce::jlimit (0, stepgrid::numStepLengths - 1, s.rate)].name;
    }

    juce::String formatMs (double ms)
    {
        return ms >= 100.0 ? juce::String (juce::roundToInt (ms)) + " ms" : juce::String (ms, 1) + " ms";
    }

    /** The pitch class (0 = C) a chord name starts with, or -1. */
    int getRootOfName (const juce::String& name)
    {
        if (name.isEmpty() || name.containsChar (' '))
            return -1;

        static const int natural[] = { 9, 11, 0, 2, 4, 5, 7 };   // A to G
        const auto first = name[0];

        if (first < 'A' || first > 'G')
            return -1;

        int pc = natural[first - 'A'];

        if (name.length() > 1 && name[1] == '#')
            ++pc;
        else if (name.length() > 1 && name[1] == 'b')
            --pc;

        return (pc + 12) % 12;
    }

    int getPitchClasses (const std::array<std::uint64_t, 2>& mask)
    {
        int bits = 0;

        for (int p = 0; p < 128; ++p)
            if (((mask[(size_t) (p / 64)] >> (p % 64)) & 1ULL) != 0)
                bits |= 1 << (p % 12);

        return bits;
    }

    int getLowest (const std::array<std::uint64_t, 2>& mask)
    {
        for (int p = 0; p < 128; ++p)
            if (((mask[(size_t) (p / 64)] >> (p % 64)) & 1ULL) != 0)
                return p;

        return -1;
    }

    int getHighest (const std::array<std::uint64_t, 2>& mask)
    {
        for (int p = 127; p >= 0; --p)
            if (((mask[(size_t) (p / 64)] >> (p % 64)) & 1ULL) != 0)
                return p;

        return -1;
    }

    bool isOn (const std::array<std::uint64_t, 2>& mask, int p) { return p >= 0 && p < 128 && ((mask[(size_t) (p / 64)] >> (p % 64)) & 1ULL) != 0; }

    bool isBlackKey (int pitch)
    {
        const int pc = pitch % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    /** "R", "3", "5", "7" and so on: a note's place in the chord with this root. */
    juce::String getRole (int interval)
    {
        switch (interval % 12)
        {
            case 0: return "R";
            case 1: case 2: return "2";
            case 3: case 4: return "3";
            case 5: return "4";
            case 6: case 7: case 8: return "5";
            case 9: return "6";
            default: return "7";
        }
    }
}

juce::Colour getKindColour (StepKind kind)
{
    switch (kind)
    {
        case StepKind::note:  return colour;
        case StepKind::chord: return juce::Colour (0xff9b7bff);
        case StepKind::bass:  return juce::Colour (0xff4f8bff);
        case StepKind::rest:  return theme::gridBar;
        case StepKind::tie:   return juce::Colour (0xff7d8794);
    }

    return colour;
}

juce::String getTitle (int topic)
{
    if (topic >= 0 && topic < arp::numKnobs)
        return knobNames[topic];

    if (topic >= kindButton && topic < kindButton + arp::numStepKinds)
        return juce::String (arp::getKindName ((StepKind) (topic - kindButton))) + " brush";

    switch (topic)
    {
        case keyboard:  return "The keys you hold";
        case stepRow:   return "The steps of the pattern";
        case notesView: return "The last 2 bars";
        case fxui::overview: return "DawIt Arpeggiator";
        case fxui::presets:  return "Starting points";
        case fxui::clap:     return "Test chord";
        case fxui::guide:    return "What is an arpeggiator?";
        default: break;
    }

    return "DawIt Arpeggiator";
}

juce::String describe (int topic)
{
    switch (topic)
    {
        case (int) Knob::rate:    return "How long a step lasts, as a note value: 1/16 is four steps a beat. Follows the song's tempo.";
        case (int) Knob::order:   return "The path through the notes you hold: Up, Down, Up and down, Down and up, Outside in, As played, Alberti or Random.";
        case (int) Knob::octaves: return "How many octaves it climbs through: 1 plays the notes you hold, 2 adds them an octave higher.";
        case (int) Knob::gate:    return "How long each note lasts, as a share of its step: 50% short and detached, 100% joined, above 100% overlapping.";
        case (int) Knob::swing:   return "Delays every second step: 50% is straight, about 67% a full triplet shuffle.";
        case (int) Knob::steps:   return "How many steps the pattern has before it starts again. 16 steps of 1/16 fill one bar.";
        case (int) Knob::accent:  return "How much louder an accented step plays, in MIDI velocity (1 to 127). Shift-click a step to accent it.";
        case (int) Knob::latch:   return "Keep playing the chord after you let go of the keys, until you play the next one.";
        case fxui::clap:          return "Play the chord track's chord of this bar (C major where there's none) through the arpeggiator for 2 bars.";
        case keyboard:            return "The keys you hold, named by their place in the chord, and the note playing now.";
        case notesView:           return "The last 2 bars of notes it played, against the song's beats.";
        case stepRow:             return "The steps of the pattern over the song's beats. Pick a brush, then click or drag across steps; Shift-click accents a step.";
        default: break;
    }

    if (topic >= kindButton && topic < kindButton + arp::numStepKinds)
        return arp::getKindHint ((StepKind) (topic - kindButton));

    return {};
}

juce::String getTypical (int topic)
{
    switch (topic)
    {
        case (int) Knob::order:
            return "Up is the classic climbing arpeggio, Down is often more relaxed. Up and down, Outside in and Alberti circle round the chord instead of climbing it. "
                   "Try one chord in each: the harmony stays the same, only the shape of the line changes. The order starts again from its first note when you play "
                   "a new chord after letting go of the last one.";
        case (int) Knob::octaves:
            return "1 or 2 octaves is the usual choice, 3 or 4 for big sweeping trance or film arpeggios. A line that climbs into the top of the keyboard gets thin "
                   "and piercing, and one that dives low gets muddy and gets in the way of the bass. Hold one note and set 2 octaves for the octave bounce of disco "
                   "and synth-pop basslines.";
        case (int) Knob::rate:
            return "1/16 is the everyday choice, 1/8 for slower, calmer parts, 1/32 for shimmering runs. Triplets fit three in the time of two: 1/8T is a third of "
                   "a beat. The rate follows the song's tempo, so the arpeggio stays on the beat when the tempo changes.";
        case (int) Knob::gate:
            return "At 100% a note lasts until the next one starts: joined, legato. At 50% it leaves a gap: short, detached, staccato, which keeps a fast pattern "
                   "clear. Above 100% the notes overlap, like harp strings ringing on. How long you hear a note also depends on the instrument: if the gate seems "
                   "to do nothing, look at its release.";
        case (int) Knob::swing:
            return "Swing delays every second step, so pairs of steps become long-short. 50% is straight. Small settings such as 54% loosen the feel without it "
                   "sounding like swing; about 67% is the full triplet shuffle. It swings the pairs of steps: at 1/16 the sixteenths, at 1/8 the eighths.";
        case (int) Knob::steps:
            return "16 steps of 1/16, 12 of 1/8T, 8 of 1/8 or 4 of 1/4 fill exactly one bar, so step 1 is always on a bar line. Lengths that divide the bar, "
                   "such as 8 steps of 1/16, keep it there too. Other lengths cross the bar line, so the pattern starts at a different place in each bar: restless and hypnotic rather than square.";
        case (int) Knob::accent:
            return "Accents say where the weight of the bar is. Accent steps 1, 5, 9 and 13 of a 1/16 pattern and the arpeggio marks the beat, as trance arpeggios "
                   "do. What louder means is up to the instrument: most get louder, many also brighter.";
        case (int) Knob::latch:
            return "Latch lets you play a chord and take your hands off the keyboard. The pattern goes on until you play the next chord. The sustain pedal does "
                   "the same while it is down, without Latch.";
        default: break;
    }

    return {};
}

juce::String describeSettings (int topic, const arp::Settings& s, double bpm)
{
    const double stepMs = stepgrid::getStepMs (s.rate, bpm);
    const auto rate = getRateName (s);

    switch (topic)
    {
        case (int) Knob::order:
            return juce::String (arp::getOrderName (s.order)) + ": " + juce::String (arp::getOrderHint (s.order)).toLowerCase() + ". Three held notes over "
                   + juce::String (s.octaves) + (s.octaves == 1 ? " octave" : " octaves") + " come round every " + juce::String (arp::getCycleLength (3, s.order, s.octaves)) + " steps.";
        case (int) Knob::octaves:
            return juce::String (s.octaves) + (s.octaves == 1 ? " octave" : " octaves") + (s.octaves == 1 ? ": it plays only the notes you hold." : ": three held notes make a line of ")
                   + (s.octaves == 1 ? juce::String() : juce::String (3 * s.octaves) + " notes.");
        case (int) Knob::rate:
            return rate + ": a step every " + formatMs (stepMs) + " at " + juce::String (juce::roundToInt (bpm)) + " BPM, " + juce::String (1000.0 / stepMs, 1) + " steps a second.";
        case (int) Knob::gate:
            return juce::String (juce::roundToInt (s.gatePercent)) + "%: each note lasts " + formatMs (stepMs * (double) s.gatePercent / 100.0) + " of its " + formatMs (stepMs)
                   + " step" + (s.gatePercent > 100.5f ? ", so it is still sounding when the next one starts." : s.gatePercent > 99.5f ? ", so the notes join." : ", leaving a gap before the next.");
        case (int) Knob::swing:
        {
            const double secondMs = stepMs * 2.0 * (double) s.swingPercent / 100.0;
            return juce::String (s.swingPercent, s.swingPercent - std::floor (s.swingPercent) < 0.005f ? 0 : 1) + "%: "
                   + (s.swingPercent < 50.5f ? juce::String ("straight, both steps of a pair the same length.")
                                             : "the second step of each pair starts " + formatMs (secondMs - stepMs) + " late, at " + formatMs (secondMs) + " into the pair.");
        }
        case (int) Knob::steps:
        {
            const double beats = (double) s.steps * stepgrid::getStepBeats (s.rate);
            return juce::String (s.steps) + " steps of " + rate + " are " + juce::String (beats, 2) + " beats, " + juce::String (beats / 4.0, 2) + " bars long.";
        }
        case (int) Knob::accent:
            return "An accented step plays " + juce::String (s.accent) + " louder: velocity 100 becomes " + juce::String (arp::getAccentedVelocity (100, s.accent)) + ".";
        case (int) Knob::latch:
            return s.latch ? "On: the chord keeps playing after you let go." : "Off: it stops when you let go of the keys.";
        default: break;
    }

    return {};
}

//==============================================================================
int StepGrid::getStepAt (juce::Point<float> p) const
{
    if (! cells.expanded (0.0f, 4.0f).contains (p))
        return -1;

    return juce::jlimit (0, arp::maxSteps - 1, (int) ((p.x - cells.getX()) / cellWidth));
}

StepGrid layoutSteps (juce::Rectangle<float> bounds)
{
    StepGrid grid;
    grid.cells = bounds.withTrimmedTop (22.0f).withTrimmedBottom (20.0f);
    grid.cellWidth = grid.cells.getWidth() / (float) arp::maxSteps;
    return grid;
}

void drawSteps (juce::Graphics& g, const StepGrid& grid, const arp::Settings& s, int playingStep, int hovered, bool labels)
{
    const double sb = stepgrid::getStepBeats (s.rate);
    const auto row = grid.cells;
    const auto bandTop = row.getY() - 20.0f, bandBottom = row.getBottom() + 18.0f;

    // The beat and bar lines: a line per beat, stronger at each bar.
    for (int beat = 0; beat * (1.0 / sb) <= arp::maxSteps + 1.0e-6; ++beat)
    {
        const float x = row.getX() + grid.cellWidth * (float) (beat / sb);

        if (x > row.getRight() + 1.0f)
            break;

        g.setColour (beat % 4 == 0 ? theme::gridBar : theme::grid);
        g.fillRect (x - 0.5f, bandTop, 1.0f, bandBottom - bandTop);
    }

    for (int i = 0; i < arp::maxSteps; ++i)
    {
        const auto cell = grid.getCell (i).reduced (1.5f, 0.0f);
        const bool used = i < s.steps;
        const auto kind = s.kinds[(size_t) i];
        const bool accented = s.accents[(size_t) i];
        const auto c = getKindColour (kind);
        const float alpha = used ? 1.0f : 0.28f;

        g.setColour ((kind == StepKind::rest ? theme::lane : c.withAlpha (kind == StepKind::tie ? 0.45f : 0.8f)).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (cell, 4.0f);
        g.setColour ((kind == StepKind::rest ? theme::gridBar : c.withAlpha (0.8f)).withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (cell, 4.0f, 1.0f);

        // An accent: a bright bar across the top of the cell.
        if (accented)
        {
            g.setColour (juce::Colours::white.withMultipliedAlpha (used ? 0.95f : 0.3f));
            g.fillRoundedRectangle (cell.getX() + 3.0f, cell.getY() + 3.0f, cell.getWidth() - 6.0f, 3.0f, 1.5f);
        }

        // The letter of the kind: N, C, B, R, T.
        g.setColour (juce::Colours::white.withMultipliedAlpha (used ? (kind == StepKind::rest ? 0.45f : 0.95f) : 0.3f));
        g.setFont (juce::FontOptions (juce::jmin (15.0f, cell.getWidth() * 0.5f), juce::Font::bold));
        g.drawText (juce::String::charToString ((juce::juce_wchar) arp::getKindLetter (kind)), cell.withTrimmedTop (6.0f), juce::Justification::centred);

        if (labels)
        {
            g.setFont (juce::FontOptions (10.0f));
            g.setColour (theme::text.withMultipliedAlpha (used ? 0.8f : 0.3f));
            g.drawText (juce::String (i + 1), grid.getCell (i).withY (row.getY() - 16.0f).withHeight (14.0f), juce::Justification::centred);

            // What beat the step falls on: bar.beat on the beat, "&" on the half.
            const double beats = i * sb, frac = beats - std::floor (beats);
            juce::String label;

            if (frac < 1.0e-6 || frac > 1.0 - 1.0e-6)
                label = juce::String ((int) (std::floor (beats / 4.0 + 1.0e-6)) + 1) + "." + juce::String ((int) std::floor (beats + 1.0e-6) % 4 + 1);
            else if (std::abs (frac - 0.5) < 1.0e-6)
                label = "&";

            g.setFont (juce::FontOptions (10.5f));
            g.setColour (theme::textDim.withMultipliedAlpha (used ? 1.0f : 0.4f));
            g.drawText (label, grid.getCell (i).withY (row.getBottom() + 2.0f).withHeight (14.0f), juce::Justification::centred);
        }
    }

    for (const int step : { hovered, playingStep })
    {
        if (! juce::isPositiveAndBelow (step, arp::maxSteps))
            continue;

        const auto cell = grid.getCell (step).reduced (1.5f, 0.0f);
        g.setColour (step == playingStep ? juce::Colours::white : theme::text.withAlpha (0.7f));
        g.drawRoundedRectangle (cell.expanded (1.0f), 5.0f, step == playingStep ? 2.5f : 1.5f);
    }
}

juce::String getStepDescription (const arp::Settings& s, int step, StepKind brush)
{
    if (! juce::isPositiveAndBelow (step, arp::maxSteps))
        return "The steps of the pattern. Click one to give it the chosen brush.";

    const auto kind = s.kinds[(size_t) step];
    const bool accented = s.accents[(size_t) step];
    const double beats = step * stepgrid::getStepBeats (s.rate);
    const auto dot = juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 "));
    auto text = "Step " + juce::String (step + 1) + " of " + juce::String (s.steps) + dot + arp::getKindName (kind) + (accented ? dot + "accented" : juce::String());
    text << ". " << arp::getKindHint (kind) << ": starts " << juce::String (beats, beats - std::floor (beats) < 1.0e-6 ? 0 : 2) << (juce::exactlyEqual (beats, 1.0) ? " beat" : " beats") << " into the pattern. ";
    text << "Click to give it the " << arp::getKindName (brush) << " brush (a step already in it becomes Note), drag to paint, Shift-click to accent, double-click for Note.";

    if (step >= s.steps)
        text << " It is past the last step, so it isn't played.";

    return text;
}

//==============================================================================
KeyboardState getKeyboardState (const BuiltInArpeggiator& p)
{
    KeyboardState k;
    k.held = { p.heldKeys[0].load(), p.heldKeys[1].load() };
    k.sounding = { p.soundingNotes[0].load(), p.soundingNotes[1].load() };
    k.chordPitchClasses = p.chordPitchClasses.load();
    return k;
}

juce::String getHeldChordName (const KeyboardState& k)
{
    const int bits = getPitchClasses (k.held);

    if (bits == 0)
        return {};

    return arp::nameChord (bits, getLowest (k.held) % 12);
}

void drawKeyboard (juce::Graphics& g, juce::Rectangle<float> area, const KeyboardState& k)
{
    int lo = juce::jmin (48, getLowest (k.held) < 0 ? 48 : getLowest (k.held), getLowest (k.sounding) < 0 ? 48 : getLowest (k.sounding));
    int hi = juce::jmax (83, getHighest (k.held), getHighest (k.sounding));
    lo = juce::jmax (0, lo - lo % 12);
    hi = juce::jmin (127, hi + (11 - hi % 12));

    int whites = 0;

    for (int p = lo; p <= hi; ++p)
        whites += isBlackKey (p) ? 0 : 1;

    const float keyWidth = area.getWidth() / (float) juce::jmax (1, whites);
    const float blackWidth = keyWidth * 0.62f, blackHeight = area.getHeight() * 0.6f;

    // Where each key sits: whites side by side, blacks on the line between two whites.
    auto keyRect = [&] (int pitch)
    {
        int index = 0;

        for (int p = lo; p < pitch; ++p)
            index += isBlackKey (p) ? 0 : 1;

        if (isBlackKey (pitch))
            return juce::Rectangle<float> (area.getX() + (float) index * keyWidth - blackWidth * 0.5f, area.getY(), blackWidth, blackHeight);

        return juce::Rectangle<float> (area.getX() + (float) index * keyWidth, area.getY(), keyWidth, area.getHeight());
    };

    const int bits = getPitchClasses (k.held);
    const int root = getRootOfName (arp::nameChord (bits, getLowest (k.held) % 12));

    for (int pass = 0; pass < 2; ++pass)
    {
        for (int p = lo; p <= hi; ++p)
        {
            const bool black = isBlackKey (p);

            if ((pass == 0) == black)
                continue;

            const auto r = keyRect (p);
            const bool held = isOn (k.held, p), sounding = isOn (k.sounding, p);
            const bool inChord = k.chordPitchClasses != 0 && ((k.chordPitchClasses >> (p % 12)) & 1) != 0;

            auto fill = black ? juce::Colour (0xff15161a) : juce::Colour (0xffd9dce3);

            if (inChord)
                fill = black ? fill.interpolatedWith (colour, 0.25f) : fill.interpolatedWith (colour, 0.18f);

            if (sounding)
                fill = colour.brighter (black ? 0.0f : 0.3f);

            g.setColour (fill);
            g.fillRect (r.reduced (0.5f, 0.0f));
            g.setColour (black ? juce::Colours::black : theme::gridBar);
            g.drawRect (r.reduced (0.5f, 0.0f), 1.0f);

            if (held)
            {
                g.setColour (juce::Colours::white);
                g.drawRect (r.reduced (1.5f), 2.0f);

                if (root >= 0)
                {
                    g.setColour (black && ! sounding ? juce::Colours::white : juce::Colours::black);
                    g.setFont (juce::FontOptions (juce::jmin (13.0f, keyWidth * 0.7f), juce::Font::bold));
                    g.drawText (getRole ((p - root + 120) % 12), r.withTrimmedTop (r.getHeight() - (black ? 22.0f : 24.0f)).withHeight (18.0f), juce::Justification::centred);
                }
            }

            // C marks its octave, small, at the foot of the key.
            if (! black && p % 12 == 0 && ! held)
            {
                g.setColour (juce::Colours::black.withAlpha (0.55f));
                g.setFont (juce::FontOptions (9.5f));
                g.drawText (juce::MidiMessage::getMidiNoteName (p, true, true, 4), r.withTrimmedTop (r.getHeight() - 15.0f), juce::Justification::centred);
            }
        }
    }
}

//==============================================================================
std::vector<arp::StepRecord> getLastBars (BuiltInArpeggiator& p)
{
    std::vector<arp::StepRecord> all ((size_t) arp::StepHistory::size);
    const int n = p.history.getRecent (all.data(), (int) all.size());
    all.resize ((size_t) juce::jmax (0, n));

    const double now = p.clockBeat.load();
    std::vector<arp::StepRecord> kept;

    for (auto& r : all)
        if (r.beat >= now - 8.0 - 1.0e-6 && r.beat <= now + 8.0)
            kept.push_back (r);

    return kept;
}

void drawLastBars (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<arp::StepRecord>& records, const arp::Settings& s, double nowBeat)
{
    const double stepBeats = stepgrid::getStepBeats (s.rate);
    double newestEnd = nowBeat;

    for (auto& r : records)
        newestEnd = juce::jmax (newestEnd, r.beat + stepBeats);

    const double right = std::ceil (newestEnd - 1.0e-6), left = right - 8.0;
    const auto plot = area.withTrimmedTop (16.0f);
    auto toX = [&] (double beat) { return plot.getX() + (float) ((beat - left) / 8.0) * plot.getWidth(); };

    // The beats of the song, a stronger line and the bar's number at each bar.
    for (int beat = (int) std::floor (left); beat <= (int) right; ++beat)
    {
        const float x = toX ((double) beat);

        if (x < plot.getX() - 0.5f || x > plot.getRight() + 0.5f)
            continue;

        if (beat < 0)
            continue;

        const bool bar = beat % 4 == 0;
        g.setColour (bar ? theme::gridBar : theme::grid);
        g.fillRect (x - 0.5f, plot.getY(), 1.0f, plot.getHeight());

        if (bar)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText ("Bar " + juce::String (beat / 4 + 1), juce::Rectangle<float> (x + 3.0f, area.getY(), 60.0f, 14.0f), juce::Justification::centredLeft);
        }
    }

    int lowest = 127, highest = 0;

    for (auto& r : records)
        for (int i = 0; i < r.numPlayed; ++i)
        {
            lowest = juce::jmin (lowest, (int) r.played[(size_t) i]);
            highest = juce::jmax (highest, (int) r.played[(size_t) i]);
        }

    if (lowest > highest)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Hold a chord, or press Test chord, to see the notes it plays here.", plot, juce::Justification::centred);
        return;
    }

    int span = highest - lowest;

    if (span < 12)
    {
        lowest -= (12 - span) / 2;
        span = 12;
    }

    lowest -= 1;
    const int rows = span + 3;
    const float rowHeight = plot.getHeight() / (float) rows;
    auto toY = [&] (int pitch) { return plot.getBottom() - rowHeight * (float) (pitch - lowest + 1); };

    for (int p = lowest; p < lowest + rows; ++p)
    {
        if (p % 12 != 0)
            continue;

        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (p) + rowHeight * 0.5f, plot.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (9.5f));
        g.drawText (juce::MidiMessage::getMidiNoteName (p, true, true, 4), juce::Rectangle<float> (plot.getX() + 2.0f, toY (p) - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    }

    for (auto& r : records)
    {
        if (r.numPlayed == 0)
            continue;

        const int tied = r.kind == StepKind::note || r.kind == StepKind::chord || r.kind == StepKind::bass ? arp::getTiedSteps (s, r.step) : 1;
        const double length = (double) (tied - 1) * stepBeats + stepBeats * (double) s.gatePercent / 100.0;
        const float x0 = toX (r.beat), x1 = juce::jmax (x0 + 2.0f, toX (r.beat + length));
        const auto kindColour = getKindColour (r.kind);

        for (int i = 0; i < r.numPlayed; ++i)
        {
            const int pitch = r.played[(size_t) i];
            const bool outside = r.chordPitchClasses != 0 && ((r.chordPitchClasses >> (pitch % 12)) & 1) == 0;
            const juce::Rectangle<float> note (x0, toY (pitch) + 1.0f, juce::jmin (x1, plot.getRight()) - x0, juce::jmax (2.0f, rowHeight - 2.0f));
            g.setColour (kindColour);
            g.fillRoundedRectangle (note, 2.0f);

            if (outside)
            {
                g.setColour (theme::meterYellow);
                g.drawRoundedRectangle (note, 2.0f, 1.2f);
            }
        }
    }

    // Where the song is now.
    const float nowX = toX (nowBeat);

    if (nowX >= plot.getX() && nowX <= plot.getRight())
    {
        g.setColour (theme::playhead.withAlpha (0.7f));
        g.fillRect (nowX - 0.5f, plot.getY(), 1.0f, plot.getHeight());
    }
}

//==============================================================================
namespace
{
    /** A note of a picture: where it starts and how long it lasts in quarter notes, its pitch and velocity. */
    struct RollNote
    {
        double start = 0.0, length = 0.25;
        int pitch = 60, velocity = 100;
    };

    void drawCaption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& left, const juce::String& right = {})
    {
        auto line = r.removeFromTop (18.0f);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::text);
        g.drawText (left, line, juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.drawText (right, line, juce::Justification::centredRight, true);
        r.removeFromTop (6.0f);
    }

    void drawNote (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text)
    {
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::textDim);
        g.drawFittedText (text, r.toNearestInt(), juce::Justification::centredLeft, 2);
    }

    juce::Rectangle<float> drawBox (juce::Graphics& g, juce::Rectangle<float> box, const juce::String& name, const juce::String& detail, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.18f));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (c);
        g.drawRoundedRectangle (box, 5.0f, 1.2f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (theme::text);
        g.drawText (name, box.withTrimmedBottom (detail.isNotEmpty() ? box.getHeight() / 2.0f - 2.0f : 0.0f), juce::Justification::centred);

        if (detail.isNotEmpty())
        {
            g.setFont (juce::FontOptions (10.5f));
            g.setColour (theme::textDim);
            g.drawFittedText (detail, box.withTrimmedTop (box.getHeight() / 2.0f - 2.0f).reduced (4.0f, 2.0f).toNearestInt(), juce::Justification::centredTop, 2);
        }

        return box;
    }

    void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow ({ from, to }, 1.2f, 7.0f, 7.0f);
    }

    juce::String getNoteName (int pitch) { return juce::MidiMessage::getMidiNoteName (pitch, true, true, 4); }

    int findRate (const char* name)
    {
        for (int i = 0; i < stepgrid::numStepLengths; ++i)
            if (juce::String (stepgrid::stepLengths[i].name) == name)
                return i;

        return stepgrid::defaultStepLength;
    }

    /** The plain arpeggio the pictures start from: Up, 1 octave, 1/16, gate 80, 16 Note steps. */
    arp::Settings getPlain() { return {}; }

    /** The pitches held now, low to high; C4 E4 G4 when nothing is held. */
    std::vector<int> getHeldPitches (const KeyboardState& k, bool& isExample)
    {
        std::vector<int> pitches;

        for (int p = 0; p < 128 && (int) pitches.size() < arp::maxHeld; ++p)
            if (((k.held[(size_t) (p / 64)] >> (p % 64)) & 1ULL) != 0)
                pitches.push_back (p);

        isExample = pitches.empty();

        if (isExample)
            pitches = { 60, 64, 67 };

        return pitches;
    }

    std::vector<arp::HeldNote> makeHeld (const std::vector<int>& pitches)
    {
        std::vector<arp::HeldNote> held;

        for (size_t i = 0; i < pitches.size(); ++i)
            held.push_back ({ pitches[i], 100, (int) i, true });

        return held;
    }

    /** The notes the settings make from these pitches in the first `beats` quarter notes, worked out with the same functions the processor uses. */
    std::vector<RollNote> simulate (const arp::Settings& s, const std::vector<int>& pitches, double beats, double offset = 0.0)
    {
        std::vector<RollNote> notes;
        std::vector<arp::SeqNote> seq;
        arp::buildSequence (makeHeld (pitches), s.order, s.octaves, seq);

        if (seq.empty())
            return notes;

        const int lowest = *std::min_element (pitches.begin(), pitches.end());
        int counter = 0;

        for (long long k = 0; arp::getStepStartBeats (k, s.rate, s.swingPercent) < beats - 1.0e-9; ++k)
        {
            const int index = (int) (k % juce::jmax (1, s.steps));
            const auto kind = s.kinds[(size_t) index];

            if (kind == StepKind::rest || kind == StepKind::tie)
                continue;

            RollNote n;
            n.start = arp::getStepStartBeats (k, s.rate, s.swingPercent) + offset;
            n.length = arp::getNoteEndBeats (k, arp::getTiedSteps (s, index), s.rate, s.swingPercent, s.gatePercent) - (n.start - offset);
            n.velocity = s.accents[(size_t) index] ? arp::getAccentedVelocity (100, s.accent) : 100;

            if (kind == StepKind::note)
            {
                const size_t i = s.order == arp::Order::random ? (size_t) arp::getRandomIndex (k, (int) seq.size()) : (size_t) counter % seq.size();
                ++counter;
                n.pitch = seq[i].pitch;
                notes.push_back (n);
            }
            else if (kind == StepKind::chord)
            {
                for (const int p : pitches)
                {
                    n.pitch = p;
                    notes.push_back (n);
                }
            }
            else
            {
                n.pitch = arp::getBassPitch (lowest);
                notes.push_back (n);
            }
        }

        return notes;
    }

    /** A small piano roll: the notes as bars on their pitch rows against `beats` quarter notes, with the beat and bar lines. */
    void drawRoll (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<RollNote>& notes, double beats, juce::Colour c, bool labels = true, int minRows = 8)
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (area);

        if (notes.empty())
            return;

        int lo = 127, hi = 0;

        for (auto& n : notes)
        {
            lo = juce::jmin (lo, n.pitch);
            hi = juce::jmax (hi, n.pitch);
        }

        while (hi - lo + 1 < minRows)
        {
            if (lo > 0 && (hi - lo) % 2 == 0)
                --lo;
            else
                ++hi;
        }

        const float labelWidth = labels ? 34.0f : 0.0f;
        const auto plot = area.withTrimmedLeft (labelWidth).reduced (0.0f, 3.0f);
        const float rowHeight = plot.getHeight() / (float) (hi - lo + 1);
        auto toX = [&] (double beat) { return plot.getX() + (float) (beat / beats) * plot.getWidth(); };

        for (int b = 0; b <= (int) std::floor (beats + 1.0e-9); ++b)
        {
            g.setColour (b % 4 == 0 ? theme::gridBar : theme::grid);
            g.fillRect (toX ((double) b) - 0.5f, area.getY(), 1.0f, area.getHeight());
        }

        std::set<int> used;

        for (auto& n : notes)
            used.insert (n.pitch);

        g.setFont (juce::FontOptions (10.0f));

        for (const int p : used)
        {
            const float y = plot.getBottom() - rowHeight * (float) (p - lo + 1);

            if (labels)
            {
                g.setColour (theme::textDim);
                g.drawText (getNoteName (p), juce::Rectangle<float> (area.getX() + 2.0f, y, labelWidth - 4.0f, rowHeight), juce::Justification::centredRight);
            }

            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRect (plot.getX(), y, plot.getWidth(), rowHeight);
        }

        for (auto& n : notes)
        {
            const float x0 = toX (n.start), x1 = juce::jmin (plot.getRight(), toX (n.start + n.length));
            const float y = plot.getBottom() - rowHeight * (float) (n.pitch - lo + 1);
            const float alpha = juce::jlimit (0.45f, 1.0f, 0.45f + (float) (n.velocity - 80) / 60.0f * 0.55f);
            g.setColour (c.withAlpha (alpha));
            g.fillRect (juce::Rectangle<float> (x0, y + 1.0f, juce::jmax (1.5f, x1 - x0 - 0.5f), juce::jmax (1.0f, rowHeight - 2.0f)));
        }
    }

    /** Dots of a line of pitches over steps, numbered, with a line from dot to dot. Returns nothing: it draws into `area`. */
    void drawDots (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<int>& pitchPerStep, int lo, int hi, bool numbers)
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (area);
        const int n = (int) pitchPerStep.size();

        if (n == 0)
            return;

        const auto plot = area.reduced (14.0f, 12.0f);
        auto toPoint = [&] (int i)
        {
            const float fy = hi > lo ? (float) (pitchPerStep[(size_t) i] - lo) / (float) (hi - lo) : 0.5f;
            return juce::Point<float> (plot.getX() + plot.getWidth() * (n > 1 ? (float) i / (float) (n - 1) : 0.5f), plot.getBottom() - plot.getHeight() * fy);
        };

        juce::Path line;

        for (int i = 0; i < n; ++i)
        {
            if (i == 0)
                line.startNewSubPath (toPoint (i));
            else
                line.lineTo (toPoint (i));
        }

        g.setColour (colour.withAlpha (0.35f));
        g.strokePath (line, juce::PathStrokeType (1.2f));
        const float radius = numbers ? 8.0f : 4.5f;

        for (int i = 0; i < n; ++i)
        {
            const auto pt = toPoint (i);
            g.setColour (colour);
            g.fillEllipse (pt.x - radius, pt.y - radius, radius * 2.0f, radius * 2.0f);

            if (numbers)
            {
                g.setColour (juce::Colours::black);
                g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
                g.drawText (juce::String (i + 1), juce::Rectangle<float> (pt.x - radius, pt.y - radius, radius * 2.0f, radius * 2.0f), juce::Justification::centred);
            }
        }
    }

    arp::Example makeExample (const arp::Settings& s, const std::vector<int>& pitches, double beats)
    {
        arp::Example e;
        e.settings = s;
        e.numPitches = (int) juce::jmin ((size_t) arp::maxHeld, pitches.size());

        for (int i = 0; i < e.numPitches; ++i)
            e.pitches[(size_t) i] = pitches[(size_t) i];

        e.beats = beats;
        return e;
    }

    /** Gives a page its listening example: the button plays what `make` returns into the instrument. Without a track to play it on, the button does nothing and says why. */
    void addExample (fxui::Guide::Page& page, BuiltInArpeggiator* processor, std::function<arp::Example()> make, const juce::String& what)
    {
        page.noClap = true;
        const bool hasHost = fxui::getHostTrack (*processor).name.isNotEmpty();
        page.clapText = "Hear this";

        if (hasHost)
        {
            page.ownClap = [processor, make] { processor->playExample (make()); };
            page.clapTooltip = what + " The example has its own settings, so yours stay as they are.";
        }
        else
        {
            page.ownClap = [] {};
            page.clapTooltip = "Put DawIt Arpeggiator on a MIDI track to hear this";
        }
    }
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInArpeggiator& arpeggiator)
{
    BuiltInArpeggiator* processor = &arpeggiator;
    std::vector<fxui::Guide::Page> pages;

    struct PageText { const char* name; const char* summary; const char* text; };
    static const PageText texts[] = {
        { "Chords, one note at a time", "What an arpeggio is, and what an arpeggiator does", R"arp(An arpeggio is a chord played one note after another instead of all at once. Hold C, E and G on a piano together and you hear a block chord, C major. Play them C, E, G, C, E, G in a steady rhythm and you hear the same chord as a moving line. The word comes from the Italian arpeggiare, to play on a harp, where chords are naturally played this way.

An arpeggiator does it for you. You hold a chord, or a MIDI clip holds it, and the arpeggiator plays its notes in turn, in time with the song: four notes a beat at 1/16, in the order you choose, over one or more octaves. You choose the chord; the arpeggiator chooses the rhythm and the order.

Arpeggios are everywhere: the opening of Beethoven's "Moonlight" Sonata, a fingerpicked guitar ballad, the fast sixteenths of trance. Synthesisers have had arpeggiators for a long time. Roland's Juno-60, from 1982, had three orders (Up, Up and down, Down), a range of 1 to 3 octaves and a Hold switch, and those are still the basic controls.

DawIt Arpeggiator goes further: a step of its pattern can also play the whole chord, the bass note, a rest or a longer note, so the same chord can become a broken chord, a stab or a piano part.

It's a MIDI effect: it changes notes, not sound. It sits before the instrument, so what you hear depends on the instrument after it. A short pluck makes each note clear; a slow pad blurs them into a shimmer.)arp" },
        { "Chord tones", "The notes of the chord: root, third, fifth", R"arp(The notes an arpeggiator plays are the notes you hold, so the first thing that decides how it sounds is the chord.

A triad, the most common chord, has three notes. The root names it. The third is 3 or 4 semitones above the root: 4 for a major chord (C to E), 3 for a minor one (A to C). The fifth is 7 semitones above the root (C to G, A to E). So C major is C E G and A minor is A C E. Add the note 10 or 11 semitones above the root and you have a seventh chord: Am7 is A C E G, Cmaj7 is C E G B.

Because every note it plays is a chord tone, an arpeggio fits wherever its chord fits. That's why it's an easy way to fill a song: hold the chord of the bar and its notes agree with the harmony. They clash when the chord you hold isn't the one the rest of the song plays; the Your track page checks that against the chord track.

Which notes you hold, and where, is called the voicing, and it changes the sound. C4 E4 G4 ripples close together; C3 G3 E4, spread over more than an octave, sounds wider and calmer. A note more makes the cycle longer: a 4-note chord in Up comes round every 4 steps, a 3-note one every 3.

The picture shows the keys you hold, each named by its job in the chord (R for the root, then 3, 5 and 7), and the note playing now, lit.)arp" },
        { "Note order", "Up, down, and the patterns in between", R"arp(The order is the path the arpeggiator takes through the notes you hold. With C4 E4 G4 held:

- Up: C E G, C E G. The classic climbing arpeggio.
- Down: G E C, G E C. Falling, often more relaxed.
- Up and down: C E G E, C E G E. Up, then back down without playing the top and the bottom twice, so 3 notes come round every 4 steps.
- Down and up: G E C E, G E C E.
- Outside in: C G E. The lowest, the highest, then inwards. With four notes, C E G B, it plays C B E G.
- As played: in the order you pressed the keys. Press E, then C, then G, and it plays E C G. Notes of a clip that start together count from low to high.
- Alberti: low, high, middle, high: C G E G. It's named after Domenico Alberti, an Italian composer of the 18th century who used it a great deal, though he didn't invent it. It's the left hand at the start of Mozart's Piano Sonata in C, K. 545.
- Random: one of the held notes, in any of the octaves you set, picked by chance for each step; the same note can come twice. DawIt's chance is the same each time the song plays the same bar, so a part you like stays the way it is.

The order starts again from its first note when you play a new chord after letting go of the last one. Add or lift a note while others stay held and it carries on where it was.

Up and Down sound like a direction; Up and down, Outside in and Alberti like a figure that circles. Try one chord in each: the harmony stays the same, only the shape of the line changes.)arp" },
        { "Octaves", "Spreading the chord over more of the keyboard", R"arp(Octaves sets how many octaves the arpeggio climbs through. At 1 it plays only the notes you hold. At 2 it plays them, then the same notes 12 semitones (an octave) higher: C4 E4 G4 C5 E5 G5. At 3 it adds another octave, up to G6.

A note an octave higher has the same name at twice the frequency: A4 is 440 Hz, A5 880 Hz. So the harmony doesn't change, only the range. More octaves make a longer, sweeping line: 3 notes over 2 octaves come round every 6 steps, over 3 octaves every 9.

More range isn't always better. A line that climbs into the top of the keyboard gets thin and piercing, and one that dives low gets muddy and gets in the way of the bass. 1 or 2 octaves is the usual choice, 3 or 4 for big sweeping trance or film arpeggios.

Hold one note and set 2 octaves, and it jumps between the note and its octave, C3 C4 C3 C4: the octave bounce of disco and synth-pop basslines.)arp" },
        { "Rate and note length", "How fast it steps, and how long each note lasts", R"arp(Rate is the length of a step, as a note value, the same as in the piano roll: 1/4 is one beat, 1/8 half a beat, 1/16 a quarter of a beat and 1/32 an eighth of a beat. Triplets fit three in the time of two: 1/8T is a third of a beat, 1/16T a sixth.

At 120 BPM a beat lasts 0.5 seconds, so 1/16 steps come every 125 ms, 8 notes a second. At 140 BPM 1/16 is 9.3 notes a second and 1/32 is 18.7. The rate follows the song's tempo, so the arpeggio stays on the beat when the tempo changes.

Gate is how long each note lasts, as a share of its step. At 100% a note lasts until the next one starts: joined notes, called legato. At 50% it lasts half the step and leaves a gap: short, detached notes, called staccato, which keep a fast pattern clear. Above 100% the notes overlap, each still sounding when the next starts, like harp strings ringing on or a pianist holding the sustain pedal. An instrument that plays one note at a time may glide from one to the next instead, depending on how it's set.

How long you hear a note also depends on the instrument. A pluck that dies away in 200 ms sounds short at any gate; a pad with a slow release goes on sounding after the note ends. If the gate seems to do nothing, look at the instrument's release.)arp" },
        { "Chords, bass notes and rests", "The steps of the pattern", R"arp(Each step of the pattern says what to play when its turn comes:

- Note: the next note of the order.
- Chord: the whole chord you hold, at once. A pattern of only Chord steps turns a held chord into a rhythm of block chords; other arpeggiators call this a chord trigger.
- Bass: the lowest note you hold, an octave lower, as a pianist's left hand would play it.
- Rest: nothing. A gap is rhythm too.
- Tie: the step before goes on sounding through this one, for a longer note.

Chord, Bass, Rest and Tie don't move the order on: Note, Chord, Note plays the order's first note, the chord, then its second note.

Mixing them is how an arpeggiator plays like a person. Bass, Note, Note, Note in eighths is a ballad piano's left hand. Bass, Chord, Bass, Chord in quarters is the oom-pah of stride piano or a march. Rest, Chord, Rest, Chord in eighths puts the chord on the off-beats, as in ska and house. Chord, Tie, then notes is a hit followed by a ripple.

Steps sets how many steps the pattern has before it starts again, from 1 to 16. It counts its steps from the start of the song, so it stays in place when you play from the middle or loop a section. 16 steps of 1/16, 12 of 1/8T, 8 of 1/8 or 4 of 1/4 fill exactly one bar, so step 1 is always on a bar line. Lengths that divide the bar, such as 8 or 4 steps of 1/16, stay in line too, starting two or four times a bar; other lengths, such as 6 steps of 1/16, cross the bar line (see Patterns that cross the bar).)arp" },
        { "Accents, swing and syncopation", "Where the weight falls", R"arp(An accented step plays louder: its velocity goes up by the Accent amount (20 at the start, on MIDI's scale of 1 to 127). Shift-click a step to accent it. What louder means is up to the instrument: most get louder, many also brighter.

Accents say where the weight of the bar is. In 4/4 the beats, 1, 2, 3 and 4, are the strong places, and 1 the strongest. Accent steps 1, 5, 9 and 13 of a 1/16 pattern and the arpeggio marks the beat, as trance arpeggios do.

Syncopation puts the weight where it isn't expected: on the off-beats, the "and"s between the beats, or on a weak beat while the strong one stays silent. A rest on beat 1 with a chord just after it, or an accent on the last sixteenth before a beat, pushes the rhythm forward. Dance producers count it among the things that make a track move.

The best-known syncopated pattern is the 3-3-2. Of the 8 eighth notes in a bar, play the 1st, 4th and 7th: on 1, on the "and" of 2, and on 4. The middle hit falls between the beats, and the gaps are 3, 3 and 2 eighths long. It's the tresillo, which came from African music into Cuban and other Latin American music, and it's common in pop and in EDM drops. The 3-3-2 stabs starting point plays it.

Swing delays every second step, so pairs of steps become long-short. At 50% both steps are the same length: straight. At about 67% the first of each pair takes two thirds of the time and the second falls on a triplet: a full shuffle. Roger Linn, who designed the swing of the Linn and MPC drum machines, describes small settings such as 54% as loosening the feel without it sounding like swing. Swing works on pairs of steps, so at 1/16 it swings the sixteenths, at 1/8 the eighths.)arp" },
        { "Patterns that cross the bar", "When the notes and the steps don't line up", R"arp(Hold three notes, set Up and 16 steps of 1/16, and listen to where the C falls. The notes come round every 3 steps and the bar every 16, so the C is on steps 1, 4, 7, 10, 13 and 16, and in the next bar on step 3. The line shifts against the beat, which makes even a plain arpeggio sound restless.

They meet again after the smallest number of steps that both 3 and 16 divide: 48 steps, 3 bars of 1/16. With four notes the cycle is 4, which divides 16, so the C is on every beat and the arpeggio sounds square and settled.

The pattern's own length does the same. A pattern of 6 steps of 1/16 is a beat and a half long, so it starts at a different place in each bar, and lines up with the bar again only after 12 beats: 8 patterns, 3 bars. Cycles of different lengths over the same steady pulse are called polymetre.

Neither is wrong. Lined-up patterns sound steady; crossing ones restless and hypnotic. That's an opinion, but a common one. To make a crossing pattern line up, hold one note more or less, or choose Up and down: at 1 octave, 3 notes then come round every 4 steps.)arp" },
        { "How a pattern fits the song", "Chord changes, range and space", R"arp(An arpeggio is one part of the arrangement, so it shares the song with everything else.

Follow the chords. The arpeggiator plays what you hold, so when the song moves from C to Am, hold Am. With a MIDI clip on the track, write the chord track's chords into it as block chords; the arpeggiator turns each into the pattern. Change chord on a bar line or a beat: the order starts again on the new chord, so a change right on step 1 sounds cleanest.

Give it a range of its own. Busy parts in the same range get in each other's way. If the vocal sits around C4 to C5, an arpeggio an octave above or below leaves it room. Keep fast arpeggios out of the bass's range, below about C3, where quick notes tend to blur.

Leave space. A fast arpeggio fills every gap. Many producers bring one in for the build-up or the chorus and leave it out of the verse, or make it sparser there with rests or slower steps. That's taste, not a rule.

Velocity and length change it as much as speed. The same 1/16 pattern sounds gentle with a 50% gate on a soft pluck, and hard at 100% on a bright saw wave.

DawIt Arpeggiator or Play chords as...? Both turn chords into patterns. Play chords as... on the chord track writes the notes into a clip you can edit one by one. The arpeggiator plays them live: change the order or the rate while the song plays, and every chord you hold follows.)arp" },
        { "How DawIt Arpeggiator works", "Before the instrument, on the song's clock", R"arp(1. It's a MIDI effect. On a MIDI track it sits before the instrument, first in the device bar. The notes of the clips, your MIDI keyboard and the computer keyboard go into it, and what comes out goes to the instrument: the DawIt Synth, a built-in instrument or a plugin. It only goes on MIDI tracks, because an audio track has no notes.

2. It follows the song's clock. DawIt tells it the tempo and where the playhead is for every block of sound, and it counts its steps from the start of the song. While the song is stopped and you play, it runs a clock of its own at the song's tempo, starting at step 1 with your first chord.

3. A new chord starts a step. Notes held together make the chord. When a chord arrives a little after a step began, up to a quarter of a step late, that step plays at once, so a chord played or recorded slightly late isn't skipped.

4. It ends every note it starts: when you let go, stop, jump to another place or switch it off. Latch keeps the chord going after you let go, until you play the next one. The sustain pedal holds the keys you let go of while it's down.

5. Recording keeps what you play, not what it plays. A take holds your chords, and the arpeggiator turns them into the pattern each time the song plays, so you can still change the pattern, the rate or the order afterwards. For a written-out part, use Play chords as... on the chord track; for audio, bounce the track.

6. Switched off, it lets the notes through unchanged: you hear the chord you hold.)arp" },
        { "Your track", "What it played on this track", R"arp(This page looks at the last 2 bars the arpeggiator played on this track, and at the chord track, and says what to try.

Play the track, or hold a chord, and the picture follows.)arp" },
    };

    for (auto& t : texts)
    {
        pages.push_back ({ t.name, t.summary, t.text, nullptr });
        pages.back().noClap = true;
    }

    const auto bpmOf = [processor] { const double b = processor->getUsedBpm(); return b > 0.0 ? b : 120.0; };

    // 1. Chords, one note at a time: a block chord and the same chord as an arpeggio.
    pages[0].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "C4 E4 G4 as a block chord, then as an arpeggio", "4 beats, 1/16, Up");
        const float laneHeight = (r.getHeight() - 24.0f) / 2.0f;
        const std::vector<int> pitches { 60, 64, 67 };

        std::vector<RollNote> block;

        for (const int p : pitches)
            block.push_back ({ 0.0, 4.0, p, 100 });

        const auto top = r.withHeight (laneHeight), bottom = r.withHeight (laneHeight).translated (0.0f, laneHeight + 24.0f);
        drawRoll (g, top.withTrimmedTop (16.0f), block, 4.0, getKindColour (StepKind::chord));
        drawRoll (g, bottom.withTrimmedTop (16.0f), simulate (getPlain(), pitches, 4.0), 4.0, colour);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (theme::text);
        g.drawText ("Held together: a block chord", top.withHeight (16.0f), juce::Justification::centredLeft);
        g.drawText ("One after another: an arpeggio", bottom.withHeight (16.0f), juce::Justification::centredLeft);
    };
    addExample (pages[0], processor, [] { return makeExample (getPlain(), { 60, 64, 67 }, 4.0); }, "Plays C4 E4 G4 as a 1/16 arpeggio, Up, for one bar.");

    // 2. Chord tones: the keyboard with the keys held, named by their place in the chord.
    pages[1].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        auto state = getKeyboardState (*processor);
        bool example = false;
        const auto pitches = getHeldPitches (state, example);

        if (example)
            for (const int p : pitches)
                state.held[(size_t) (p / 64)] |= 1ULL << (p % 64);

        drawCaption (g, r, example ? "C major: C4 E4 G4 (hold keys to see your own)" : "The keys you hold: " + getHeldChordName (state),
                     "R root, 3 third, 5 fifth, 7 seventh");
        drawKeyboard (g, r.withTrimmedBottom (46.0f), state);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 40.0f), example ? "In C major the third is 4 semitones above the root (C to E) and the fifth 7 (C to G). In A minor the third is 3 (A to C)."
                                                                    : "Each key is named by its job in the chord. The notes lit are the ones playing now.");
    };
    addExample (pages[1], processor, [] { auto s = getPlain(); s.rate = findRate ("1/8"); return makeExample (s, { 57, 60, 64, 67 }, 8.0); },
                "Plays Am7 (A3 C4 E4 G4) one note at a time, in eighths.");

    // 3. Note order: the order over the held notes as numbered dots.
    pages[2].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        bool example = false;
        const auto pitches = getHeldPitches (getKeyboardState (*processor), example);
        const auto seq = arp::getSequence (makeHeld (pitches), s.order, s.octaves);
        drawCaption (g, r, juce::String (arp::getOrderName (s.order)) + " over " + (example ? juce::String ("C E G (an example)") : juce::String (pitches.size()) + " held notes"),
                     "comes round every " + juce::String (arp::getCycleLength ((int) pitches.size(), s.order, s.octaves)) + " steps");
        std::vector<int> perStep;
        int lo = 127, hi = 0;

        for (auto& n : seq)
        {
            lo = juce::jmin (lo, n.pitch);
            hi = juce::jmax (hi, n.pitch);
        }

        for (int k = 0; k < 16 && ! seq.empty(); ++k)
            perStep.push_back (seq[(size_t) (s.order == arp::Order::random ? arp::getRandomIndex (k, (int) seq.size()) : k % (int) seq.size())].pitch);

        drawDots (g, r.withTrimmedBottom (30.0f), perStep, lo, hi, true);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 28.0f), "Each dot is a step, numbered from 1; its height is the note. The line shows the path. Change the Order and the picture follows.");
    };
    addExample (pages[2], processor, [processor] { auto s = processor->getSettings(); s.rate = findRate ("1/8"); s.steps = 16; s.kinds.fill (StepKind::note); s.accents.fill (false); return makeExample (s, { 60, 64, 67 }, 8.0); },
                "Plays C4 E4 G4 in the order chosen in the window, in eighths.");

    // 4. Octaves: the sequence at 1, 2 and 3 octaves.
    pages[3].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        bool example = false;
        const auto pitches = getHeldPitches (getKeyboardState (*processor), example);
        drawCaption (g, r, "One cycle of " + juce::String (arp::getOrderName (s.order)) + " at 1, 2 and 3 octaves", example ? "over C E G" : "over your held notes");
        const auto widest = arp::getSequence (makeHeld (pitches), s.order, 3);
        int lo = 127, hi = 0;

        for (auto& n : widest)
        {
            lo = juce::jmin (lo, n.pitch);
            hi = juce::jmax (hi, n.pitch);
        }

        const float gap = 12.0f, width = (r.getWidth() - 2.0f * gap) / 3.0f;
        const auto area = r.withTrimmedBottom (34.0f);

        for (int o = 1; o <= 3; ++o)
        {
            const auto cell = juce::Rectangle<float> (area.getX() + (width + gap) * (float) (o - 1), area.getY(), width, area.getHeight());
            std::vector<int> perStep;

            for (auto& n : arp::getSequence (makeHeld (pitches), s.order, o))
                perStep.push_back (n.pitch);

            drawDots (g, cell.withTrimmedTop (18.0f), perStep, lo, hi, perStep.size() <= 12);
            const bool chosen = o == s.octaves;
            g.setFont (juce::FontOptions (12.0f, chosen ? juce::Font::bold : juce::Font::plain));
            g.setColour (chosen ? colour : theme::text);
            g.drawText (juce::String (o) + (o == 1 ? " octave: " : " octaves: ") + juce::String ((int) perStep.size()) + " notes", cell.withHeight (18.0f), juce::Justification::centredLeft);

            if (chosen)
            {
                g.setColour (colour);
                g.drawRect (cell.withTrimmedTop (18.0f), 1.5f);
            }
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "The octave your setting uses is outlined. Higher dots are the same notes 12 semitones up.");
    };
    addExample (pages[3], processor, [processor] { auto s = getPlain(); s.order = processor->getSettings().order; s.octaves = 3; return makeExample (s, { 60, 64, 67 }, 8.0); },
                "Plays C4 E4 G4 over 3 octaves, in the order chosen in the window.");

    // 5. Rate and note length: three gates at the current rate.
    pages[4].paint = [processor, bpmOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const double bpm = bpmOf(), stepBeats = stepgrid::getStepBeats (s.rate), stepMs = stepgrid::getStepMs (s.rate, bpm);
        drawCaption (g, r, getRateName (s) + " at " + juce::String (juce::roundToInt (bpm)) + " BPM: " + formatMs (stepMs) + " a step", juce::String (1000.0 / stepMs, 1) + " notes a second");
        const float laneHeight = (r.getHeight() - 36.0f) / 3.0f;
        const double beats = stepBeats * 8.0;
        const int pitches[] = { 60, 64, 67 };
        const char* const labels[] = { "Gate 50%: staccato, detached", "Gate 100%: legato, joined", "Gate 150%: overlapping, ringing on" };
        const double gates[] = { 0.5, 1.0, 1.5 };

        for (int row = 0; row < 3; ++row)
        {
            const auto lane = juce::Rectangle<float> (r.getX(), r.getY() + (laneHeight + 4.0f) * (float) row, r.getWidth(), laneHeight);
            std::vector<RollNote> notes;

            for (int i = 0; i < 8; ++i)
                notes.push_back ({ stepBeats * i, stepBeats * gates[row], pitches[i % 3], 100 });

            drawRoll (g, lane.withTrimmedTop (14.0f), notes, beats, colour, false, 6);
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (juce::String (labels[row]) + "  (a note lasts " + formatMs (stepMs * gates[row]) + ")", lane.withHeight (14.0f), juce::Justification::centredLeft);
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 28.0f), "The first 8 steps at your rate, on C E G. How long you hear a note also depends on the instrument's release.");
    };
    addExample (pages[4], processor, [processor] { auto s = getPlain(); const auto now = processor->getSettings(); s.rate = now.rate; s.gatePercent = now.gatePercent; return makeExample (s, { 60, 64, 67 }, 8.0); },
                "Plays C4 E4 G4 at the rate and gate set in the window.");

    // 6. Chords, bass notes and rests: the step row above the notes it makes.
    pages[5].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        bool example = false;
        const auto pitches = getHeldPitches (getKeyboardState (*processor), example);
        drawCaption (g, r, "Your pattern: " + arp::patternToText (s).fromFirstOccurrenceOf (": ", false, false), example ? "notes on C E G" : "notes on your held keys");
        const auto grid = layoutSteps (r.withHeight (70.0f));
        drawSteps (g, grid, s, processor->playingStep.load(), -1, true);
        const double beats = stepgrid::getStepBeats (s.rate) * 16.0;
        const auto lane = juce::Rectangle<float> (grid.cells.getX(), r.getY() + 76.0f, grid.cells.getWidth(), r.getHeight() - 76.0f - 30.0f);
        drawRoll (g, lane, simulate (s, pitches, beats), beats, colour, false, 8);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 28.0f), "Note steps follow the order, Chord plays everything held, Bass the lowest note an octave down. Rests and Ties leave gaps and longer notes.");
    };
    addExample (pages[5], processor, [processor] { return makeExample (processor->getSettings(), { 60, 64, 67 }, 8.0); }, "Plays C4 E4 G4 with the pattern in the window.");

    // 7. Accents, swing and syncopation: the 3-3-2, and swing at three settings.
    pages[6].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "The 3-3-2: hits on eighths 1, 4 and 7 of a bar", "gaps of 3, 3 and 2 eighths");
        const auto top = r.removeFromTop (juce::jmin (70.0f, r.getHeight() * 0.3f));
        const float cell = top.getWidth() / 8.0f;

        for (int i = 0; i < 8; ++i)
        {
            const bool hit = i == 0 || i == 3 || i == 6;
            const auto c = juce::Rectangle<float> (top.getX() + cell * (float) i, top.getY(), cell, top.getHeight()).reduced (3.0f);
            g.setColour (hit ? getKindColour (StepKind::chord) : juce::Colours::black.withAlpha (0.25f));
            g.fillRoundedRectangle (c, 4.0f);
            g.setColour (hit ? juce::Colours::black : theme::textDim);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (juce::String (i + 1), c, juce::Justification::centred);
        }

        r.removeFromTop (14.0f);
        drawCaption (g, r, "Swing: the second of a pair of 1/16 steps, at 120 BPM", "the first step is at 0");
        const int rate = findRate ("1/16");
        const float percents[] = { 50.0f, 54.0f, 66.67f };
        const float laneHeight = (r.getHeight() - 32.0f) / 3.0f;
        const double pairBeats = 0.5;

        for (int row = 0; row < 3; ++row)
        {
            const auto lane = juce::Rectangle<float> (r.getX(), r.getY() + laneHeight * (float) row, r.getWidth(), laneHeight).reduced (0.0f, 3.0f);
            const double second = arp::getStepStartBeats (1, rate, percents[row]);
            const auto plot = lane.withTrimmedLeft (260.0f);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (plot);
            g.setFont (juce::FontOptions (12.0f));
            g.setColour (theme::text);
            g.drawText (juce::String (percents[row], row == 2 ? 1 : 0) + "%: second step at " + juce::String (second, 3) + " beat = " + formatMs (second * 500.0),
                        lane.withWidth (255.0f), juce::Justification::centredLeft, true);

            for (const double beat : { 0.0, second, pairBeats })
            {
                const float x = plot.getX() + (float) (beat / pairBeats) * (plot.getWidth() - 2.0f);
                g.setColour (beat < pairBeats - 1.0e-9 ? colour : theme::gridBar);
                g.fillRect (x, plot.getY() + 2.0f, 2.0f, plot.getHeight() - 4.0f);
            }
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 28.0f), "At 66.7% the second step lands on the triplet: a full shuffle. 54% only loosens the feel.");
    };
    addExample (pages[6], processor, []
    {
        for (auto& p : arp::getPresets())
            if (p.name.containsIgnoreCase ("3-3-2"))
                return makeExample (p.settings, { 60, 64, 67 }, 8.0);

        return makeExample (getPlain(), { 60, 64, 67 }, 8.0);
    }, "Plays C4 E4 G4 with the 3-3-2 stabs starting point.");

    // 8. Patterns that cross the bar: 3 bars of a 3-note Up over 16 steps.
    pages[7].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "3 bars of 3-note Up over 16 steps of 1/16", "the C marked each time it comes round");
        const float rowHeight = (r.getHeight() - 36.0f) / 3.0f, labelWidth = 52.0f;
        const float cell = (r.getWidth() - labelWidth) / 16.0f;
        const char* const letters[] = { "C", "E", "G" };

        for (int bar = 0; bar < 3; ++bar)
        {
            const float y = r.getY() + rowHeight * (float) bar;
            g.setFont (juce::FontOptions (11.5f));
            g.setColour (theme::textDim);
            g.drawText ("Bar " + juce::String (bar + 1), juce::Rectangle<float> (r.getX(), y, labelWidth - 4.0f, rowHeight), juce::Justification::centredLeft);

            for (int col = 0; col < 16; ++col)
            {
                const int n = bar * 16 + col;
                const bool isC = n % 3 == 0;
                const auto c = juce::Rectangle<float> (r.getX() + labelWidth + cell * (float) col, y, cell, rowHeight).reduced (1.5f, 3.0f);
                g.setColour (isC ? colour : juce::Colours::black.withAlpha (0.25f));
                g.fillRoundedRectangle (c, 3.0f);

                if (col % 4 == 0)
                {
                    g.setColour (theme::text.withAlpha (0.5f));
                    g.fillRect (c.getX() - 1.5f, c.getY() - 2.0f, 1.5f, c.getHeight() + 4.0f);
                }

                g.setColour (isC ? juce::Colours::black : theme::textDim);
                g.setFont (juce::FontOptions (juce::jmin (13.0f, cell * 0.6f), juce::Font::bold));
                g.drawText (letters[n % 3], c, juce::Justification::centred);
            }
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "The C falls on steps 1, 4, 7, 10, 13 and 16, then on step 3 of the next bar. It is back where it began after 48 steps, 3 bars. Marks are the beats.");
    };
    addExample (pages[7], processor, [] { return makeExample (getPlain(), { 60, 64, 67 }, 12.0); }, "Plays C4 E4 G4, Up, 16 steps of 1/16, for 3 bars.");

    // 9. How a pattern fits the song: four bars of an example progression, with the arpeggio on each chord.
    pages[8].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "An example progression, one bar each: C, Am, F, G", "your pattern on the chord of each bar");
        struct Bar { const char* name; std::vector<int> pitches; };
        const Bar bars[] = { { "C", { 60, 64, 67 } }, { "Am", { 57, 60, 64 } }, { "F", { 53, 57, 60 } }, { "G", { 55, 59, 62 } } };
        std::vector<RollNote> notes;

        for (int b = 0; b < 4; ++b)
            for (auto& n : simulate (s, bars[b].pitches, 4.0, 4.0 * b))
                notes.push_back (n);

        const auto lane = r.withTrimmedTop (18.0f).withTrimmedBottom (34.0f);
        drawRoll (g, lane, notes, 16.0, colour, true, 10);

        const float barWidth = (lane.getWidth() - 34.0f) / 4.0f;
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));

        for (int b = 0; b < 4; ++b)
        {
            g.setColour (theme::text);
            g.drawText (bars[b].name, juce::Rectangle<float> (lane.getX() + 34.0f + barWidth * (float) b + 4.0f, r.getY(), 60.0f, 16.0f), juce::Justification::centredLeft);
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "Hold each bar's chord and every note it plays is a tone of that chord. Hold the wrong chord and the notes clash with the harmony. Your track checks that against your chord track.");
    };
    addExample (pages[8], processor, [processor] { auto s = processor->getSettings(); return makeExample (s, { 57, 60, 64 }, 8.0); }, "Plays A3 C4 E4 with the pattern in the window.");

    // 10. How DawIt Arpeggiator works: the chain, and the playhead's step.
    pages[9].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const int step = processor->playingStep.load();
        drawCaption (g, r, "Before the instrument, on the song's clock", processor->usingSongClock.load() ? "following the song" : "running its own clock");
        const float midY = r.getCentreY() + 14.0f, boxH = 56.0f, gap = 40.0f;
        const float boxW = (r.getWidth() - 3.0f * gap) / 4.0f;
        const juce::String names[] = { "Notes in", "DawIt Arpeggiator", "Instrument", "Audio effects" };
        const juce::String details[] = { "clips, keyboard, pedal", step >= 0 ? "playing step " + juce::String (step + 1) : "waiting for a chord", "synth, built-in or plugin", "reverb, delay and so on" };
        const juce::Colour colours[] = { theme::textDim, colour, theme::accent, theme::textDim };
        juce::Rectangle<float> boxes[4];

        for (int i = 0; i < 4; ++i)
        {
            boxes[i] = drawBox (g, juce::Rectangle<float> (r.getX() + (boxW + gap) * (float) i, midY - boxH / 2.0f, boxW, boxH), names[i], details[i], colours[i]);

            if (i > 0)
                arrow (g, { boxes[i - 1].getRight() + 3.0f, midY }, { boxes[i].getX() - 3.0f, midY });
        }

        const auto clock = drawBox (g, juce::Rectangle<float> (boxes[1].getX() - 10.0f, midY - 110.0f, boxes[1].getWidth() + 20.0f, 44.0f), "The song's clock", "tempo and position", theme::accent);
        arrow (g, { clock.getCentreX(), clock.getBottom() }, { boxes[1].getCentreX(), boxes[1].getY() });
        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "Notes go in, patterns come out, and the instrument plays them. A MIDI effect changes notes, not sound: it has no audio of its own.");
    };
    addExample (pages[9], processor, [processor] { return makeExample (processor->getSettings(), { 60, 64, 67 }, 8.0); }, "Plays C4 E4 G4 through the pattern in the window, into the instrument after it.");

    // 11. Your track: the last 2 bars, and the advice.
    pages[10].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto host = fxui::getHostTrack (*processor);
        const auto s = processor->getSettings();
        drawCaption (g, r, (host.name.isEmpty() ? juce::String ("Stand-in arpeggiator") : host.name) + ": the last 2 bars", getRateName (s) + ", " + juce::String (s.steps) + " steps");
        drawLastBars (g, r, getLastBars (*processor), s, processor->clockBeat.load());
    };
    const auto introduction = pages[10].text;
    pages[10].liveText = [processor, introduction, bpmOf]
    {
        arp::AnalysisInput input;
        input.settings = processor->getSettings();
        input.records = getLastBars (*processor);
        input.bpm = bpmOf();
        juce::StringArray lines;

        for (auto& line : arp::analyse (input))
            lines.add (line);

        return introduction + "\n\n" + lines.joinIntoString ("\n\n");
    };

    return pages;
}
} // namespace daw::arptext
