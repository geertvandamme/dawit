#include "EqExplainer.h"
#include "EffectWindow.h"

namespace daw
{
namespace eqtext
{
    using T = eq::BandType;

    juce::Colour getBandColour (int band)
    {
        static const juce::Colour colours[] = { juce::Colour (0xffff7a59), juce::Colour (0xffffc145), juce::Colour (0xff8bd450),
                                                juce::Colour (0xff3fc1c9), juce::Colour (0xff6a8dff), juce::Colour (0xffc77dff) };
        return colours[juce::jlimit (0, eq::numBands - 1, band)];
    }

    juce::String formatFrequency (double hz)
    {
        if (hz < 1000.0)
            return juce::String (juce::roundToInt (hz)) + " Hz";

        return juce::String (hz / 1000.0, hz < 10000.0 ? 2 : 1) + " kHz";
    }

    juce::String formatGain (double db)
    {
        if (db < -60.0)
            return "below -60 dB";

        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    namespace
    {
        juce::String percentOfLevel (double db)
        {
            return juce::String (juce::roundToInt (100.0 * std::pow (10.0, db / 20.0))) + "%";
        }
    }

    juce::String describeRegion (double hz)
    {
        if (hz < 60.0)    return "Sub-bass (20-60 Hz): felt more than heard, the rumble under a kick or a synth bass. Small speakers don't play it.";
        if (hz < 250.0)   return "Bass (60-250 Hz): the weight of kick and bass, the low notes of piano and guitar. Too much sounds boomy.";
        if (hz < 500.0)   return "Low mids (250-500 Hz): body and warmth. Many instruments pile up here, and too much sounds muddy or boxy.";
        if (hz < 2000.0)  return "Mids (500 Hz-2 kHz): the core of most instruments and voices. Too much sounds honky, nasal or like a telephone; too little sounds hollow.";
        if (hz < 6000.0)  return "Upper mids (2-6 kHz): presence, clarity and attack. The ear is most sensitive here, so small changes are easy to hear and too much is harsh.";
        if (hz < 10000.0) return "Brilliance (6-10 kHz): the s and t sounds of a voice, cymbals, string and breath noise.";
        return "Air (10-20 kHz): openness and sparkle. Many adults hear only up to 15-17 kHz.";
    }

    juce::String describeNote (double hz)
    {
        const int note = juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0));

        if (note < 21 || note > 108)
            return {};

        return "close to the note " + juce::MidiMessage::getMidiNoteName (note, true, true, 4);
    }

    juce::String describeType (T t)
    {
        switch (t)
        {
            case T::lowCut:    return "Removes everything below its frequency, more the further below it goes. Also called a high-pass filter, because it lets the highs pass.";
            case T::highCut:   return "Removes everything above its frequency, more the further above it goes. Also called a low-pass filter, because it lets the lows pass.";
            case T::lowShelf:  return "Raises or lowers everything below its frequency by the same amount, like the bass knob of a stereo. The change fades in around the frequency: at the frequency itself it's halfway.";
            case T::highShelf: return "Raises or lowers everything above its frequency by the same amount, like the treble knob of a stereo. At the frequency itself it's halfway.";
            case T::bell:      return "Raises or lowers a range around its frequency: most at the centre, fading out on both sides in the shape of a bell. The most used band of an EQ. Also called a peak filter.";
            case T::notch:     return "Removes a very narrow range around its frequency almost completely, and leaves the rest alone. Also called a band-stop filter.";
        }

        return {};
    }

    juce::String howToUse (T t)
    {
        switch (t)
        {
            case T::lowCut:    return "Almost every track except bass and kick carries rumble and boom it doesn't need. Typical: vocals at 80-100 Hz, guitars and keys at 80-150 Hz, hi-hats and overheads at 200-400 Hz. It clears the lows for bass and kick, and you mostly don't hear it go: solo the band to hear what you remove. Too high makes a part thin.";
            case T::highCut:   return "Tames hiss and fizz (distorted guitars above 6-10 kHz), takes noise off a bass or kick, or makes a part darker and further away so it sits behind the vocal. Careful on vocals and cymbals: their air lives above 10 kHz.";
            case T::lowShelf:  return "+2 to +4 dB at 80-120 Hz adds weight to a kick, a bass or a thin mix. -2 to -3 dB at 150-250 Hz takes the woolliness off an acoustic guitar or a full mix. To remove rumble, a low cut is better: a shelf keeps some of it.";
            case T::highShelf: return "+2 to +3 dB at 10-12 kHz adds air to vocals and acoustic instruments. -2 to -4 dB at 6-8 kHz softens harsh cymbals or a bright recording. Big boosts bring up hiss as well.";
            case T::bell:      return "Cut to fix: mud at 200-500 Hz, boxiness at 300-600 Hz, honk at 800 Hz-1.5 kHz, harshness at 2-5 kHz. Boost to bring out: the body of a snare (150-250 Hz), the presence of a vocal or the click of a kick (3-5 kHz). Cut narrow, boost wide: a narrow cut removes one problem and leaves the rest, a narrow boost sounds unnatural. Not sure where? Try Find a bad frequency.";
            case T::notch:     return "For mains hum at 50 or 60 Hz (and twice and three times that), the ring of one drum, a feedback whistle, one booming note of a room. Find the exact frequency by sweeping a narrow bell boost first, then switch the band to a notch.";
        }

        return {};
    }

    juce::String whatQDoes (T t)
    {
        switch (t)
        {
            case T::lowCut:
            case T::highCut:   return "Q sets the shape at the corner. 0.71 is the smoothest cut, without a bump (called Butterworth). Higher Q adds a bump just before the cut: a resonant, synth-like sound. Lower Q starts the cut gently, well before the frequency.";
            case T::lowShelf:
            case T::highShelf: return "Q sets how sudden the step is. 0.71 is a smooth step. Higher Q makes it steeper, with a small dip or bump on the other side, which some engineers like for a tighter low end. Lower Q spreads the change over several octaves.";
            case T::bell:      return "Q is the width. Low Q (0.5-1) is broad and musical, for shaping the tone. High Q (4-10) is narrow and surgical, for one problem frequency. Q 1.4 is about one octave wide, Q 0.7 two octaves, Q 4 a third of an octave.";
            case T::notch:     return "Q is the width of the notch: the higher, the narrower, and the less it touches the rest. Hum needs a high Q (10 or more); a wider notch takes away more of the music too.";
        }

        return {};
    }

    juce::String describeSettings (const eq::Band& b, double rate)
    {
        const auto f = formatFrequency (b.freq);
        const auto name = eq::getTypeName (b.type);

        if (! b.on)
            return name + " at " + f + ", switched off: it changes nothing until you switch it on.";

        auto at = [&] (double hz) { return eq::getResponseDb (b, hz, rate); };
        juce::String s;

        switch (b.type)
        {
            case T::bell:
            {
                const double octaves = eq::getBandwidthOctaves (b.q), half = std::pow (2.0, octaves / 2.0);
                s << "Bell at " << f << ", " << formatGain (b.gainDb) << ", Q " << juce::String (b.q, 2) << ": works on about "
                  << formatFrequency (b.freq / half) << " to " << formatFrequency (b.freq * half) << " (" << juce::String (octaves, 1)
                  << (octaves >= 1.05 ? " octaves)" : " octave)");

                if (std::abs (b.gainDb) < 0.05f)
                    s << ", but at 0 dB it changes nothing yet: drag its dot up or down.";
                else
                    s << (b.gainDb > 0 ? ", raising " : ", lowering ") << f << " by " << juce::String (std::abs (b.gainDb), 1)
                      << " dB (to " << percentOfLevel (b.gainDb) << " of its level).";

                break;
            }

            case T::notch:
            {
                const double half = std::pow (2.0, eq::getBandwidthOctaves (b.q) / 2.0);
                s << "Notch at " << f << ", Q " << juce::String (b.q, 1) << ": takes out " << f << " and lowers "
                  << formatFrequency (b.freq / half) << " to " << formatFrequency (b.freq * half) << " by 3 dB or more; "
                  << formatFrequency (b.freq / (half * half)) << " and " << formatFrequency (b.freq * half * half) << " are "
                  << formatGain (at (b.freq * half * half)) << ".";
                break;
            }

            case T::lowCut:
            case T::highCut:
            {
                const bool low = b.type == T::lowCut;
                s << name << " at " << f << ", " << b.slope << " dB/oct: " << formatGain (at (b.freq)) << " at " << f;

                for (double factor : { 2.0, 4.0 })
                {
                    const double hz = low ? b.freq / factor : b.freq * factor;

                    if (hz >= 10.0 && hz < rate * 0.45)
                        s << ", " << formatGain (at (hz)) << " at " << formatFrequency (hz);
                }

                s << ".";
                double peakDb = 0.0, peakHz = b.freq;

                for (int i = 0; i <= 240; ++i)
                {
                    const double hz = b.freq * std::pow (2.0, -2.0 + 4.0 * i / 240.0);

                    if (hz < rate * 0.45 && at (hz) > peakDb)
                    {
                        peakDb = at (hz);
                        peakHz = hz;
                    }
                }

                if (peakDb > 0.5)
                    s << " Its Q of " << juce::String (b.q, 2) << " adds a bump of " << formatGain (peakDb) << " at " << formatFrequency (peakHz) << ".";
                else
                    s << (low ? " Everything well above " : " Everything well below ") << f << " stays as it is.";

                break;
            }

            case T::lowShelf:
            case T::highShelf:
            {
                const bool low = b.type == T::lowShelf;

                if (std::abs (b.gainDb) < 0.05f)
                {
                    s << name << " at " << f << ", 0 dB: changes nothing yet. Drag its dot up to raise everything "
                      << (low ? "below " : "above ") << f << ", or down to lower it.";
                    break;
                }

                const double otherSide = low ? b.freq * 4.0 : b.freq / 4.0;
                s << name << " at " << f << ", " << formatGain (b.gainDb) << ": everything well " << (low ? "below" : "above") << " it is "
                  << juce::String (std::abs (b.gainDb), 1) << " dB " << (b.gainDb > 0 ? "louder" : "quieter") << " (" << percentOfLevel (b.gainDb)
                  << " of its level), " << formatGain (at (b.freq)) << " at " << f << " itself, and " << formatGain (at (otherSide))
                  << " two octaves " << (low ? "above" : "below") << ", at " << formatFrequency (otherSide) << ".";
                break;
            }
        }

        return s;
    }
} // namespace eqtext

//==============================================================================
namespace
{
    using eqtext::Topic;

    void addTitle (juce::AttributedString& s, const juce::String& text, juce::Colour colour = theme::text)
    {
        s.append (text + "\n", juce::Font (juce::FontOptions (16.0f, juce::Font::bold)), colour);
    }

    void addLead (juce::AttributedString& s, const juce::String& text)
    {
        s.append (text + "\n\n", juce::Font (juce::FontOptions (13.5f)), theme::accent.brighter (0.4f));
    }

    void addSection (juce::AttributedString& s, const juce::String& heading, const juce::String& text)
    {
        if (heading.isNotEmpty())
            s.append (heading + "\n", juce::Font (juce::FontOptions (12.5f, juce::Font::bold)), theme::text);

        s.append (text + "\n\n", juce::Font (juce::FontOptions (12.5f)), theme::textDim.brighter (0.35f));
    }

    juce::String bandTitle (int band, const eq::Band& b)
    {
        return "Band " + juce::String (band + 1) + "  -  " + eq::getTypeName (b.type) + " (" + eq::getTypeAlias (b.type) + ")";
    }

    juce::String kilo (float hz) { return juce::String (hz / 1000.0f, 1).trimCharactersAtEnd ("0").trimCharactersAtEnd ("."); }

    /** "80 Hz", "5 kHz", "1.5 kHz": for typical values, without false precision. */
    juce::String formatRounded (float hz)
    {
        return hz < 1000.0f ? juce::String (juce::roundToInt (hz)) + " Hz" : kilo (hz) + " kHz";
    }

    /** "50-80 Hz", "700 Hz-1.5 kHz", "2-5 kHz". */
    juce::String formatRange (float from, float to)
    {
        if (to < 1000.0f)
            return juce::String (juce::roundToInt (from)) + "-" + juce::String (juce::roundToInt (to)) + " Hz";

        if (from < 1000.0f)
            return juce::String (juce::roundToInt (from)) + " Hz-" + kilo (to) + " kHz";

        return kilo (from) + "-" + kilo (to) + " kHz";
    }

    juce::String noteName (float hz)
    {
        return juce::MidiMessage::getMidiNoteName (juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0)), true, true, 4);
    }

    juce::String capitalised (const juce::String& text)
    {
        return text.substring (0, 1).toUpperCase() + text.substring (1);
    }
}

EqExplainer::EqExplainer (BuiltInEq& p) : processor (p)
{
    for (auto* b : { &lessonNext, &lessonCancel })
    {
        b->setWantsKeyboardFocus (false);
        addChildComponent (b);
    }

    lessonNext.onClick = [this]
    {
        if (lessonStep == 1)
        {
            // Found it: turn the boost into a modest cut at the same place.
            auto band = processor.getBand (lessonBand);
            band.gainDb = -4.0f;
            band.q = 4.0f;
            processor.setBand (lessonBand, band);
            lessonStep = 2;
            lessonNext.setButtonText ("Done");
            lessonCancel.setButtonText ("Undo");
            refresh();
        }
        else
        {
            endLesson (false);
        }
    };

    lessonCancel.onClick = [this] { endLesson (true); };
    showOverview();
}

EqExplainer::~EqExplainer()
{
    // Closing the window halfway through the sweep shouldn't leave a +10 dB boost behind.
    if (lessonStep == 1)
        processor.setBand (lessonBand, lessonSaved);
}

void EqExplainer::showOverview()
{
    if (isInLesson())
        return;

    topic = Topic::overview;
    focusBand = -1;
    refresh();
}

void EqExplainer::showBand (int band, Topic t)
{
    if (isInLesson())
        return;

    topic = t;
    focusBand = band;
    refresh();
}

void EqExplainer::showTopic (Topic t)
{
    if (isInLesson())
        return;

    topic = t;

    if (t == Topic::instruments)
        instrumentIndex = instrumentZone = -1;

    refresh();
}

void EqExplainer::showInstrument (int instrument, int zone)
{
    if (isInLesson())
        return;

    topic = Topic::instruments;
    instrumentIndex = instrument;
    instrumentZone = zone;
    refresh();
}

void EqExplainer::showPreset (int index)
{
    if (isInLesson())
        return;

    topic = Topic::presets;
    presetIndex = index;
    refresh();
}

void EqExplainer::startLesson (int band)
{
    if (isInLesson())
        return;

    lessonBand = juce::jlimit (0, eq::numBands - 1, band);
    lessonSaved = processor.getBand (lessonBand);

    // A narrow, loud boost makes problem frequencies jump out while it moves over them.
    eq::Band sweep { true, eq::BandType::bell, eq::isCut (lessonSaved.type) ? 1000.0f : lessonSaved.freq, 10.0f, 5.0f, 12 };
    processor.setBand (lessonBand, sweep);
    lessonStep = 1;
    lessonNext.setButtonText ("Found it");
    lessonCancel.setButtonText ("Cancel");
    lessonNext.setVisible (true);
    lessonCancel.setVisible (true);
    refresh();

    if (onLessonChanged)
        onLessonChanged();
}

void EqExplainer::endLesson (bool restore)
{
    if (restore)
        processor.setBand (lessonBand, lessonSaved);

    focusBand = lessonBand;
    topic = Topic::band;
    lessonStep = 0;
    lessonNext.setVisible (false);
    lessonCancel.setVisible (false);
    refresh();

    if (onLessonChanged)
        onLessonChanged();
}

juce::AttributedString EqExplainer::buildText() const
{
    juce::AttributedString s;
    s.setWordWrap (juce::AttributedString::byWord);
    const double rate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;

    if (lessonStep > 0)
    {
        const auto band = processor.getBand (lessonBand);
        const auto where = eqtext::formatFrequency (band.freq);
        addTitle (s, "Find a bad frequency", eqtext::getBandColour (lessonBand));

        if (lessonStep == 1)
        {
            addLead (s, "Band " + juce::String (lessonBand + 1) + " is now a narrow +10 dB boost, at " + where + ".");
            addSection (s, "1. Sweep", "Start playback (Space) and drag the band's dot slowly left and right. Most places just sound "
                                       "louder. A problem frequency jumps out: it rings, honks, booms or hurts. Stop there.");
            addSection (s, "Where you are now", eqtext::describeRegion (band.freq));
            addSection (s, "2. Found it?", "Click Found it to turn the boost into a cut at the same place.");
        }
        else
        {
            addLead (s, "Band " + juce::String (lessonBand + 1) + " now cuts 4 dB at " + where + " with Q 4.");
            addSection (s, "Compare", "Switch the band off and on while the track plays. The sound should be clearer, with less of what "
                                      "bothered you, and not thinner. Too much? A cut of 2-3 dB is often enough.");
            addSection (s, "Nothing jumped out?", "Then the track may not need it, which is fine too: click Undo.");
            addSection (s, "Why it works", "A boost makes a range easy to hear, so you find it fast; the cut then fixes it. Sweeping "
                                           "boosts for a long time tires your ears, so keep it short.");
        }

        return s;
    }

    switch (topic)
    {
        case Topic::overview:
            addTitle (s, "DawIt EQ");
            addLead (s, "An equaliser (EQ) makes some frequencies louder or quieter. Every sound is a mix of low and high "
                        "frequencies; the EQ changes that balance: the tone.");
            addSection (s, "When to use it", "On almost every track in a mix: to take away what a sound doesn't need (rumble below its "
                                             "lowest note, a boxy or muddy range, a harsh ring), to make parts that cover each other "
                                             "fit together (kick and bass, guitars and the vocal), and to shape the tone (air on a "
                                             "vocal, click on a kick, less boom on an acoustic guitar). Also before a compressor or "
                                             "distortion, to set what they react to, and after them, to shape the result.");
            addSection (s, "How it's usually set", "First a low cut on everything that doesn't need deep lows (most things but kick and "
                                                   "bass, at 80-150 Hz). Then cuts: find what bothers you with Find a bad frequency and cut "
                                                   "it 2-6 dB. Last, gentle, wide boosts (2-3 dB, a low Q) for character. Pick a starting "
                                                   "point for the instrument, or open Instruments to see where it lives.");
            addSection (s, "Reading the graph", "Left to right goes from low (20 Hz, deep bass) to high (20 kHz, the highest you can hear); "
                                                "each grid step to the right doubles the frequency, one octave up. Up is louder, down quieter; the "
                                                "middle line is 0 dB, unchanged. The white line is what the EQ does in total, the coloured lines "
                                                "are its six bands. Behind them, the spectrum of the track as it plays: grey before the EQ, blue after.");
            addSection (s, "Using it", "Drag a dot to move a band: sideways for the frequency, up and down for the gain; set Q with "
                                       "the band's Q below. Right-click to change its type, double-click to reset it. Hold Alt while dragging to hear "
                                       "only that band. Hover anything to read about it here. Instruments shows where common "
                                       "instruments live, to see what your bands touch.");
            addSection (s, "Good habits", "Cut before you boost. Make small moves (2-3 dB). Compare with the band off, at the same loudness "
                                          "(Match level). Listen in the mix rather than with the track solo: what counts is how it sounds "
                                          "with the rest.");
            fxui::addPluginParagraph (s, processor);
            break;

        case Topic::presets:
        {
            const auto& presets = eq::getPresets();

            if (! juce::isPositiveAndBelow (presetIndex, (int) presets.size()))
            {
                addTitle (s, "Starting points");
                addSection (s, {}, "Settings for common instruments, each explaining why every band is set the way it is. They are a "
                                   "start, not an answer: every recording is different, so listen and adjust.");
                break;
            }

            const auto& preset = presets[(size_t) presetIndex];
            addTitle (s, preset.name);
            addLead (s, preset.why);

            for (int i = 0; i < eq::numBands; ++i)
            {
                const auto& band = preset.bands[(size_t) i];

                if (preset.notes[(size_t) i].isNotEmpty())
                    addSection (s, "Band " + juce::String (i + 1) + ": " + eq::getTypeName (band.type) + " " + eqtext::formatFrequency (band.freq)
                                       + (eq::hasGain (band.type) ? ", " + eqtext::formatGain (band.gainDb) : juce::String()),
                                preset.notes[(size_t) i]);
            }

            addSection (s, {}, "Now listen: switch bands off and on, and move them until it sounds right for this recording.");
            break;
        }

        case Topic::instruments:
        {
            const auto& list = eq::getInstruments();

            if (! juce::isPositiveAndBelow (instrumentIndex, (int) list.size()))
            {
                addTitle (s, "Where instruments live");
                addLead (s, "Common instruments on the same frequency scale as the graph, so you can see which of them your bands touch.");
                addSection (s, "Reading it", "The thick blue bar is the range of an instrument's notes: its fundamentals, the pitch you hear. "
                                             "For a drum it's the note of the drum itself. The thin bar is its overtones, the higher frequencies "
                                             "that sound along with every note: they give it its character and reach much higher than the "
                                             "notes. Red boxes are where problems usually hide: cut there if it bothers you. Green boxes are "
                                             "what a boost brings out.");
                addSection (s, "Why it helps", "Instruments that share a range cover each other up (masking). Kick and bass both want "
                                               "50-100 Hz; guitars, keys and voices pile up at 200-500 Hz; every lead part wants 2-5 kHz. "
                                               "Give each its own spot: a small cut in one where another matters most often makes both "
                                               "clearer, more than boosting either.");
                addSection (s, "Using it", "Hover an instrument for its range and what to do where; the others turn grey. Click it to "
                                           "keep it that way while you work on the bands, and click again to let go, or click "
                                           "another instrument to switch. Hover one of its "
                                           "boxes to light that range up over the whole graph and read what your EQ does there. The dots "
                                           "work as always: drag one into a box to try it.");
                addSection (s, {}, "These are typical numbers, not exact ones: every instrument, player and recording differs. Use them "
                                   "to know where to start listening.");
                break;
            }

            const auto& instrument = list[(size_t) instrumentIndex];
            const bool hasZone = juce::isPositiveAndBelow (instrumentZone, (int) instrument.zones.size());
            addTitle (s, instrument.name);

            if (hasZone)
            {
                const auto& zone = instrument.zones[(size_t) instrumentZone];
                addLead (s, capitalised (zone.name) + ", " + formatRange (zone.from, zone.to) + ": " + zone.text);

                // What the EQ does in the middle of the range (on the octave scale), and which bands sit in it.
                const double centre = std::sqrt ((double) zone.from * zone.to);
                double db = 0.0;
                juce::StringArray inside;

                for (int i = 0; i < eq::numBands; ++i)
                {
                    const auto band = processor.getBand (i);
                    db += eq::getResponseDb (band, centre, rate);

                    if (band.on && band.freq >= zone.from && band.freq <= zone.to)
                        inside.add ("band " + juce::String (i + 1) + " (" + eq::getTypeName (band.type).toLowerCase() + " at "
                                    + eqtext::formatFrequency (band.freq) + ")");
                }

                juce::String here = std::abs (db) < 0.1 ? "Your EQ leaves the middle of this range alone right now."
                                                        : "Your EQ changes the middle of this range (" + eqtext::formatFrequency (centre) + ") by "
                                                              + eqtext::formatGain (db) + " right now.";

                if (! inside.isEmpty())
                    here << " In it: " << inside.joinIntoString (", ") << ".";
                else
                    here << (zone.problem ? " To try a cut, drag a bell into the box, lower it 3 dB and switch it off and on while the track plays."
                                          : " To try a boost, drag a bell into the box, raise it 2-3 dB and switch it off and on while the track plays.");

                addSection (s, "Your EQ here", here);
            }
            else
            {
                addLead (s, instrument.summary);
            }

            if (instrument.pitched)
                addSection (s, "Notes: " + formatRange (instrument.notesFrom, instrument.notesTo) + " (" + noteName (instrument.notesFrom)
                                   + " to " + noteName (instrument.notesTo) + ")",
                            "The fundamentals: the pitch of each note. Its overtones reach up to about " + formatRounded (instrument.overtonesTo)
                                + ", giving it its character.");
            else
                addSection (s, "Body: " + formatRange (instrument.notesFrom, instrument.notesTo),
                            "The note of the drum itself. Its attack and overtones reach up to about " + formatRounded (instrument.overtonesTo) + ".");

            for (int z = 0; z < (int) instrument.zones.size(); ++z)
            {
                const auto& zone = instrument.zones[(size_t) z];

                if (z != instrumentZone)
                    addSection (s, capitalised (zone.name) + ", " + formatRange (zone.from, zone.to)
                                       + (zone.problem ? ": cut if it bothers you" : ": boost to bring it out"),
                                zone.text);
            }

            if (instrument.lowCutHz > 0.0f)
                addSection (s, "Low cut up to about " + formatRounded (instrument.lowCutHz),
                            instrument.lowCutHz < 50.0f ? "Below it there's only rumble you can't hear, which still uses up headroom: a low cut there is safe."
                                                        : "Below it there's little it needs, mostly rumble and other instruments bleeding into the mic: "
                                                          "a low cut there clears room for bass and kick.");
            break;
        }

        case Topic::output:
            addTitle (s, "Output");
            addSection (s, {}, "Raises or lowers the whole track after the EQ. Use it to bring the level back after a big boost or cut, so "
                               "the next effect and the fader get the level they had before.");
            break;

        case Topic::match:
            addTitle (s, "Match level");
            addLead (s, processor.isMatchingLevel() ? "On: it changes the level by " + eqtext::formatGain (processor.matchGainDb.load()) + " right now."
                                                    : "Off. Turned on, it would change the level by " + eqtext::formatGain (processor.matchGainDb.load()) + ".");
            addSection (s, {}, "Keeps the output as loud as what comes in, by comparing the two over the last half second. Louder almost "
                               "always sounds better at first, so an EQ that boosts seems to improve things even when it doesn't. With match "
                               "level on, switching a band or the whole EQ off and on compares only the tone.");
            break;

        case Topic::band:
        case Topic::type:
        case Topic::frequency:
        case Topic::gain:
        case Topic::q:
        case Topic::slope:
        case Topic::power:
        case Topic::solo:
        {
            if (! juce::isPositiveAndBelow (focusBand, eq::numBands))
                break;

            const auto band = processor.getBand (focusBand);
            addTitle (s, bandTitle (focusBand, band), eqtext::getBandColour (focusBand));
            addLead (s, eqtext::describeSettings (band, rate));

            switch (topic)
            {
                case Topic::frequency:
                {
                    const auto note = eqtext::describeNote (band.freq);
                    addSection (s, "Frequency: " + eqtext::formatFrequency (band.freq) + (note.isNotEmpty() ? ", " + note : juce::String()),
                                "Where the band works. Further left is lower (bass), further right higher (treble); each grid step doubles "
                                "the frequency, one octave up.");
                    addSection (s, {}, eqtext::describeRegion (band.freq));
                    break;
                }

                case Topic::gain:
                    addSection (s, "Gain", eq::hasGain (band.type)
                                              ? juce::String ("How much the band raises (+) or lowers (-) its range. +6 dB is about twice the level, "
                                                              "-6 dB half; 2-3 dB is already clearly audible. Cut before you boost: taking away what's "
                                                              "in the way usually sounds more natural than adding.")
                                              : "A " + eq::getTypeName (band.type).toLowerCase() + " has no gain: it always removes. "
                                                    "Pick a shelf or bell to raise or lower instead.");
                    break;

                case Topic::q:
                    addSection (s, "Q: " + juce::String (band.q, 2), eqtext::whatQDoes (band.type));
                    break;

                case Topic::slope:
                    addSection (s, "Slope: " + juce::String (band.slope) + " dB/oct",
                                eq::isCut (band.type)
                                    ? juce::String ("How steeply the cut falls. 12 dB/oct: an octave past the frequency (half or double it) the level is "
                                                    "12 dB lower, about a quarter. Gentle and natural. 24 dB/oct is twice as steep, to remove a lot "
                                                    "just past the frequency and keep what's just inside it.")
                                    : juce::String ("Only cuts have a slope. Pick Low cut or High cut to use it."));
                    break;

                case Topic::power:
                    addSection (s, "On / off", "Switches the band on or off. Flick it while the track plays to hear what the band does: the most "
                                               "honest test there is.");
                    break;

                case Topic::solo:
                    addSection (s, "Solo", "Plays only the frequencies this band works on, so you hear what you're cutting or boosting: below "
                                           "the frequency for a low cut or low shelf, above it for a high cut or high shelf, the range around it "
                                           "for a bell or notch. Holding Alt while dragging a dot does the same.");
                    break;

                case Topic::band:
                case Topic::type:
                case Topic::overview:
                case Topic::output:
                case Topic::match:
                case Topic::presets:
                case Topic::instruments:
                    addSection (s, "What it does", eqtext::describeType (band.type));
                    addSection (s, "How to use it", eqtext::howToUse (band.type));
                    break;
            }

            break;
        }
    }

    return s;
}

void EqExplainer::refresh()
{
    const auto text = buildText();

    // The picture of Q only shows with a band; it says nothing new during the lesson.
    const bool picture = lessonStep == 0 && juce::isPositiveAndBelow (focusBand, eq::numBands)
                         && topic != Topic::overview && topic != Topic::presets && topic != Topic::output && topic != Topic::match
                         && topic != Topic::instruments;
    const auto key = text.getText() + (picture ? juce::String (processor.getBand (focusBand).q) : juce::String());
    const auto page = juce::String ((int) topic) + "/" + juce::String (focusBand) + "/" + juce::String (presetIndex) + "/"
                      + juce::String (instrumentIndex) + "/" + juce::String (instrumentZone) + "/" + juce::String (lessonStep);

    // A new page starts at the top; the same page keeps its place while its numbers change.
    if (page != shownPage)
    {
        shownPage = page;
        scroll = 0.0f;
    }
    else if (key == shownText)
    {
        return;
    }

    shownText = key;
    shownAttributed = text;
    showPicture = picture;
    resized();
    repaint();
}

void EqExplainer::resized()
{
    auto r = getLocalBounds().reduced (14);

    if (lessonStep > 0)
    {
        auto buttons = r.removeFromBottom (28);
        lessonCancel.setBounds (buttons.removeFromRight (90));
        buttons.removeFromRight (8);
        lessonNext.setBounds (buttons.removeFromRight (110));
        r.removeFromBottom (8);
    }

    pictureArea = showPicture ? r.removeFromBottom (130) : juce::Rectangle<int>();

    if (showPicture)
        r.removeFromBottom (8);

    textArea = r;
    layout.createLayout (shownAttributed, (float) juce::jmax (10, textArea.getWidth()));
    scroll = juce::jlimit (0.0f, juce::jmax (0.0f, layout.getHeight() - (float) textArea.getHeight()), scroll);
}

void EqExplainer::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const float most = juce::jmax (0.0f, layout.getHeight() - (float) textArea.getHeight());

    if (most <= 0.0f)
    {
        Component::mouseWheelMove (e, wheel);
        return;
    }

    scroll = juce::jlimit (0.0f, most, scroll - wheel.deltaY * (wheel.isReversed ? -160.0f : 160.0f));
    repaint();
}

void EqExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, 0, 1, getHeight());

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (textArea);
        layout.draw (g, textArea.toFloat().translated (0.0f, -scroll).withHeight (juce::jmax ((float) textArea.getHeight(), layout.getHeight())));
    }

    // Fade the text out where there's more to scroll to.
    const auto area = textArea.toFloat();

    if (layout.getHeight() - scroll > area.getHeight() + 1.0f)
    {
        g.setGradientFill (juce::ColourGradient (theme::panel.withAlpha (0.0f), 0.0f, area.getBottom() - 34.0f, theme::panel, 0.0f, area.getBottom(), false));
        g.fillRect (area.withTop (area.getBottom() - 34.0f));
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("scroll for more", area.withTop (area.getBottom() - 13.0f), juce::Justification::centredRight);
    }

    if (scroll > 0.5f)
    {
        g.setGradientFill (juce::ColourGradient (theme::panel, 0.0f, area.getY(), theme::panel.withAlpha (0.0f), 0.0f, area.getY() + 24.0f, false));
        g.fillRect (area.withHeight (24.0f));
    }

    if (! pictureArea.isEmpty())
        drawQPicture (g, pictureArea.toFloat());
}

void EqExplainer::drawQPicture (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto band = processor.getBand (focusBand);
    const auto colour = eqtext::getBandColour (focusBand);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (area, 4.0f);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    auto header = area.removeFromTop (18.0f).reduced (6.0f, 0.0f);
    g.drawText ("WHAT Q DOES TO A " + eq::getTypeName (band.type).toUpperCase(), header, juce::Justification::centredLeft);
    area = area.reduced (6.0f, 4.0f);

    // Four octaves either side of a 1 kHz example, boosting or cutting 12 dB like the band, so the shape shows.
    const double centre = 1000.0, rate = 48000.0;
    const float gain = eq::hasGain (band.type) ? (band.gainDb < 0.0f ? -12.0f : 12.0f) : 0.0f;
    const float range = 18.0f;
    auto toY = [&] (double db) { return area.getCentreY() - (float) juce::jlimit (-1.0, 1.0, db / range) * area.getHeight() * 0.5f; };

    g.setColour (theme::gridBar.withAlpha (0.6f));
    g.fillRect (area.getX(), area.getCentreY(), area.getWidth(), 1.0f);

    auto curve = [&] (float q)
    {
        eq::Band b = band;
        b.on = true;
        b.freq = (float) centre;
        b.gainDb = gain;
        b.q = q;
        const auto stages = eq::makeStages (b, rate);
        juce::Path p;

        for (int x = 0; x <= (int) area.getWidth(); x += 2)
        {
            const double hz = centre * std::pow (2.0, -4.0 + 8.0 * x / area.getWidth());
            const float y = toY (juce::Decibels::gainToDecibels (stages.getMagnitude (hz, rate), -100.0));
            x == 0 ? p.startNewSubPath (area.getX() + (float) x, y) : p.lineTo (area.getX() + (float) x, y);
        }

        return p;
    };

    const std::array<float, 3> examples = band.type == eq::BandType::notch ? std::array<float, 3> { 1.0f, 4.0f, 15.0f }
                                         : band.type == eq::BandType::bell ? std::array<float, 3> { 0.5f, 1.4f, 5.0f }
                                                                           : std::array<float, 3> { 0.5f, eq::flatQ, 3.0f };
    g.setFont (juce::FontOptions (10.5f));

    for (size_t i = 0; i < examples.size(); ++i)
    {
        g.setColour (theme::textDim.withAlpha (0.55f));
        g.strokePath (curve (examples[i]), juce::PathStrokeType (1.0f));
    }

    g.setColour (colour);
    g.strokePath (curve (band.q), juce::PathStrokeType (2.2f));

    g.setColour (theme::textDim);
    juce::String legend;
    for (auto q : examples)
        legend << "Q " << juce::String (q, q < 1.0f ? 2 : 1) << "   ";

    g.drawText (legend + "(grey)", area.removeFromBottom (13.0f), juce::Justification::bottomLeft);
    g.setColour (colour);
    g.drawText ("Q " + juce::String (band.q, 2) + " (yours)", header, juce::Justification::centredRight);
}
} // namespace daw
