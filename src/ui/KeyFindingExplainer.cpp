#include "KeyFindingExplainer.h"
#include "ChordLane.h"
#include "analysis/ChordFinder.h"
#include "generate/ChordGuide.h"

#include <set>

namespace daw
{
namespace kf = gen::keyfinding;
namespace mh = analysis::midiharmony;

namespace
{
    const juce::Colour outsideColour (0xffff9f43);   // a note outside the scale (and a raised or characteristic note)
    const juce::Colour bassColour (0xff5fd38d);      // the bass key of a chord

    int mod12 (int v) { return ((v % 12) + 12) % 12; }

    /** The note names of a picture row, in order: a scale from its home note with consecutive letters (G# in A harmonic
        minor, not Ab), a chord spelled in thirds from its root. */
    juce::StringArray spellRow (const kf::Row& row)
    {
        juce::StringArray names;
        auto pcs = row.pitchClasses;

        if (row.home < 0)
        {
            for (int pc : pcs)
                names.add (mh::getNoteName (pc));

            return names;
        }

        std::sort (pcs.begin(), pcs.end(), [&] (int a, int b) { return mod12 (a - row.home) < mod12 (b - row.home); });

        if (pcs.size() < 5)
        {
            // A chord, with the notes stacked from its root.
            Chord chord;
            chord.root = row.home;
            chord.intervals.clear();

            for (int pc : pcs)
                chord.intervals.add (mod12 (pc - row.home));

            return juce::StringArray::fromTokens (chord.getNoteNames(), " ", {});
        }

        if (pcs.size() == 7)
        {
            static const juce::String letters ("CDEFGAB");
            static const int naturals[] = { 0, 2, 4, 5, 7, 9, 11 };
            const int tonicLetter = letters.indexOfChar (mh::getNoteName (row.home)[0]);

            for (size_t i = 0; i < pcs.size(); ++i)
            {
                const int letter = (tonicLetter + (int) i) % 7;
                const int accidental = (mod12 (pcs[i] - naturals[letter]) + 6) % 12 - 6;
                names.add (juce::String::charToString (letters[letter]) + juce::String::repeatedString (accidental > 0 ? "#" : "b", std::abs (accidental)));
            }

            return names;
        }

        for (int pc : pcs)
            names.add (mh::getNoteName (pc));

        return names;
    }

    /** True for a row that shows a scale (five or more notes, written from home with the steps between) rather than a chord. */
    bool isScaleRow (const kf::Row& row) { return row.pitchClasses.size() >= 5 && row.home >= 0; }

    juce::String formatBeats (double v)
    {
        return std::abs (v - std::round (v)) < 0.005 ? juce::String ((int) std::lround (v)) : juce::String (v, 1);
    }
} // namespace

//==============================================================================
/** The topic's picture: scales and chords on a keyboard with their note names, a profile or the notes of an example
    part, or (Your clip) the selected clip's bars with chords. */
class KeyFindingExplainer::Picture final : public juce::Component,
                                           public juce::TooltipClient
{
public:
    /** Plays a note: pitch, velocity 0 to 1, delay and length in milliseconds. Set by the guide. */
    std::function<void (int, float, int, int)> onPlayNote;

    /** Plays a chord of the clip with its bass note (a pitch class, or -1 for the root). Set by the guide. */
    std::function<void (const Chord&, int)> onPlayChord;

    void setTopic (kf::Topic newTopic, kf::Example newExample)
    {
        topic = newTopic;
        example = std::move (newExample);
        yours = {};
        profile = {};
        keyResult = {};

        if (! example.notes.empty())
        {
            mh::Input input { example.notes, 0.0, 16.0 };

            if (topic == kf::Topic::naming || topic == kf::Topic::between)
                input.endBeat = 4.0;

            keyResult = mh::analyse (input);
            profile = keyResult.profile;

            // The chord's name with its bass: Cmaj7/E, not Cmaj7 (the text speaks of the chord).
            if (topic == kf::Topic::naming && example.rows.size() > 1)
            {
                const auto segments = mh::findChords (input, std::nullopt);

                if (! segments.empty() && segments.front().chord)
                {
                    const auto& s = segments.front();
                    example.rows[1].note = s.chordBass >= 0 ? s.chord->getName() + ", with " + mh::getNoteName (s.bass) + " in the bass: " + s.getName()
                                                            : s.chord->getName();
                }
            }
        }

        repaint();
    }

    void setClip (const YourClip& newClip)
    {
        topic = kf::Topic::yours;
        yours = newClip;
        example = {};
        repaint();
    }

    int getPreferredHeight() const
    {
        if (topic == kf::Topic::yours)
            return yours.found && ! yours.drums ? 270 : 80;

        const auto l = getLayout (600);
        return l.total;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        if (topic == kf::Topic::yours)
        {
            paintYours (g);
            return;
        }

        const auto l = getLayout (getWidth());

        switch (l.chart)
        {
            case Chart::none:         break;
            case Chart::profile:      paintProfile (g, l.chartArea, profile, "How long each note name sounds, in beats", true,
                                                    example.rows.empty() ? std::vector<int>() : example.rows[0].pitchClasses,
                                                    example.rows.empty() ? -1 : example.rows[0].home);
                                      break;
            case Chart::twoProfiles:  paintTwoProfiles (g, l.chartArea); break;
            case Chart::notes:        paintNotes (g, l.chartArea); break;
        }

        for (size_t i = 0; i < example.rows.size() && i < l.rows.size(); ++i)
            paintRow (g, example.rows[i], l.rows[i]);

        // The legend and the caption.
        auto bottom = l.captionArea;
        const auto legend = getLegend();
        paintLegend (g, bottom.removeFromRight (getLegendWidth (legend)), legend);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText (example.caption, bottom, juce::Justification::centredLeft, 2, 0.9f);
    }

    void mouseMove (const juce::MouseEvent& e) override { hover = e.getPosition(); }
    void mouseExit (const juce::MouseEvent&) override { hover = { -1, -1 }; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (topic == kf::Topic::yours)
        {
            if (const auto* s = findSegmentAt (e.getPosition()); s != nullptr && s->chord && onPlayChord)
                onPlayChord (*s->chord, s->chordBass);

            return;
        }

        const auto l = getLayout (getWidth());

        for (size_t i = 0; i < l.rows.size() && i < example.rows.size(); ++i)
            if (l.rows[i].contains (e.getPosition()))
            {
                playRow (example.rows[i]);
                return;
            }

        if (l.chart != Chart::none && l.chartArea.contains (e.getPosition()) && ! example.notes.empty())
            playNotes (example.notes);
    }

    juce::String getTooltip() override
    {
        if (topic == kf::Topic::yours)
        {
            if (! yours.found || yours.drums)
                return "Select a MIDI clip in the arrangement to see its key, scale and chords here.";

            if (const auto* s = findSegmentAt (hover); s != nullptr)
                return describeSegment (*s);

            return "The clip's bars, with the chord of each above it. The notes are in the chord's colour when they belong to it, "
                   "grey when they don't, and orange when they are outside the scale. Click a chord to hear it.";
        }

        const auto l = getLayout (getWidth());

        for (size_t i = 0; i < l.rows.size() && i < example.rows.size(); ++i)
            if (l.rows[i].contains (hover))
                return "Click to hear " + example.rows[i].label + ".";

        if (l.chart != Chart::none && l.chartArea.contains (hover))
            return "The example part. Click to hear it.";

        return "The example. Click a scale or a chord to hear it.";
    }

private:
    enum class Chart { none, profile, twoProfiles, notes };

    static constexpr int rowHeight = 56, captionHeight = 30, labelWidth = 140, keyboardWidth = 154;

    struct Layout
    {
        Chart chart = Chart::none;
        juce::Rectangle<int> chartArea, captionArea;
        std::vector<juce::Rectangle<int>> rows;
        int total = 0;
    };

    int getNumPitchRows() const
    {
        std::set<int> pitches;

        for (auto& n : example.notes)
            pitches.insert (n.pitch);

        return (int) pitches.size();
    }

    Layout getLayout (int w) const
    {
        Layout l;
        int y = 8;

        switch (topic)
        {
            case kf::Topic::key:     l.chart = Chart::profile; y += 0; l.chartArea = { 12, y, w - 24, 150 }; y += 156; break;
            case kf::Topic::howFound: l.chart = Chart::twoProfiles; l.chartArea = { 12, y, w - 24, 140 }; y += 146; break;
            case kf::Topic::naming:
            case kf::Topic::between:
            {
                const int n = juce::jmax (1, getNumPitchRows());
                const int h = n * juce::jmin (22, 120 / n) + 20;
                l.chart = Chart::notes;
                l.chartArea = { 12, y, w - 24, h };
                y += h + 6;
                break;
            }
            case kf::Topic::majorMinor:
            case kf::Topic::modes:
            case kf::Topic::harmonicMelodic:
            case kf::Topic::pentatonic:
            case kf::Topic::susDim:
            case kf::Topic::inversions:
            case kf::Topic::yours:   break;
        }

        for (size_t i = 0; i < example.rows.size(); ++i)
        {
            l.rows.push_back ({ 12, y, w - 24, rowHeight - 4 });
            y += rowHeight;
        }

        l.captionArea = { 12, y, w - 24, captionHeight };
        l.total = y + captionHeight + 4;
        return l;
    }

    //==========================================================================
    /** One octave of keys, C to B, with the notes of a row lit. */
    void paintKeyboard (juce::Graphics& g, juce::Rectangle<float> area, const kf::Row& row) const
    {
        static const int whites[] { 0, 2, 4, 5, 7, 9, 11 };
        static const int blacks[][2] { { 1, 1 }, { 3, 2 }, { 6, 4 }, { 8, 5 }, { 10, 6 } };   // pitch class, white keys to its left
        const float ww = area.getWidth() / 7.0f;

        auto lit = [&] (int pc) { return std::find (row.pitchClasses.begin(), row.pitchClasses.end(), pc) != row.pitchClasses.end(); };
        auto marked = [&] (int pc) { return std::find (row.marked.begin(), row.marked.end(), pc) != row.marked.end(); };
        auto colourOf = [&] (int pc, bool black)
        {
            if (marked (pc))
                return black ? outsideColour.darker (0.2f) : outsideColour;

            if (lit (pc))
                return black ? theme::accent.darker (0.35f) : theme::accent.withAlpha (0.85f);

            return black ? juce::Colour (0xff15161a) : juce::Colour (0xffe9ebf0);
        };
        auto marker = [&] (int pc, juce::Rectangle<float> key, bool black)
        {
            const float cy = black ? key.getBottom() - 6.0f : key.getBottom() - 7.0f;

            if (pc == row.bass && row.bass >= 0)
            {
                g.setColour (bassColour);
                g.fillEllipse (key.getCentreX() - 4.0f, cy - (pc == row.home ? 9.0f : 0.0f), 8.0f, 8.0f);
            }

            if (pc == row.home)
            {
                g.setColour (theme::background);
                g.fillEllipse (key.getCentreX() - 3.5f, cy - 0.5f, 7.0f, 7.0f);
                g.setColour (juce::Colours::white);
                g.fillEllipse (key.getCentreX() - 2.5f, cy + 0.5f, 5.0f, 5.0f);
            }
        };

        for (int i = 0; i < 7; ++i)
        {
            const auto key = juce::Rectangle<float> (area.getX() + (float) i * ww, area.getY(), ww - 1.0f, area.getHeight());
            g.setColour (colourOf (whites[i], false));
            g.fillRoundedRectangle (key, 2.0f);
        }

        for (auto& b : blacks)
        {
            const auto key = juce::Rectangle<float> (area.getX() + (float) b[1] * ww - ww * 0.3f, area.getY(), ww * 0.6f, area.getHeight() * 0.62f);
            g.setColour (colourOf (b[0], true));
            g.fillRoundedRectangle (key, 2.0f);
        }

        for (int i = 0; i < 7; ++i)
            marker (whites[i], { area.getX() + (float) i * ww, area.getY(), ww - 1.0f, area.getHeight() }, false);

        for (auto& b : blacks)
            marker (b[0], { area.getX() + (float) b[1] * ww - ww * 0.3f, area.getY(), ww * 0.6f, area.getHeight() * 0.62f }, true);
    }

    /** A row: its label and what it shows, a keyboard, and the note names (with the steps between a scale's notes). */
    void paintRow (juce::Graphics& g, const kf::Row& row, juce::Rectangle<int> area) const
    {
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
        auto labelArea = area.withWidth (labelWidth - 8);
        g.drawFittedText (row.label, labelArea.removeFromTop (20), juce::Justification::centredLeft, 1, 0.8f);

        if (row.note.isNotEmpty())
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.0f));
            g.drawFittedText (row.note, labelArea, juce::Justification::topLeft, 3, 0.85f);
        }

        paintKeyboard (g, juce::Rectangle<float> ((float) area.getX() + (float) labelWidth, (float) area.getY() + 2.0f, (float) keyboardWidth, (float) area.getHeight() - 4.0f), row);

        // The note names.
        auto names = spellRow (row);
        const bool scale = isScaleRow (row);
        const auto steps = juce::StringArray::fromTokens (row.steps, " ", {});

        if (scale && ! names.isEmpty())
            names.add (names[0]);   // home again, an octave up

        const float x0 = (float) area.getX() + (float) labelWidth + (float) keyboardWidth + 20.0f;
        const float cell = 30.0f, gap = scale ? 14.0f : 6.0f, cy = (float) area.getCentreY();
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));

        for (int i = 0; i < names.size(); ++i)
        {
            const float x = x0 + (float) i * (cell + gap);

            if (x + cell > (float) area.getRight())
                break;

            // The pitch class this cell is, to colour the home note and the marked ones.
            const auto& sortedFrom = row.pitchClasses;
            int pc = -1;

            if (row.home >= 0)
            {
                auto pcs = sortedFrom;
                std::sort (pcs.begin(), pcs.end(), [&] (int a, int b) { return mod12 (a - row.home) < mod12 (b - row.home); });
                pc = (size_t) i < pcs.size() ? pcs[(size_t) i] : row.home;
            }
            else if ((size_t) i < sortedFrom.size())
            {
                pc = sortedFrom[(size_t) i];
            }

            const bool isHome = pc >= 0 && pc == row.home;
            const bool isMarked = std::find (row.marked.begin(), row.marked.end(), pc) != row.marked.end();
            const bool isBass = row.bass >= 0 && pc == row.bass;
            const auto cellArea = juce::Rectangle<float> (x, cy - 11.0f, cell, 22.0f);

            g.setColour (isMarked ? outsideColour : isHome ? theme::accent : theme::panel);
            g.fillRoundedRectangle (cellArea, 4.0f);

            if (isBass)
            {
                g.setColour (bassColour);
                g.drawRoundedRectangle (cellArea.reduced (0.5f), 4.0f, 1.6f);
            }

            g.setColour (isMarked || isHome ? theme::background : theme::text);
            g.drawText (names[i], cellArea, juce::Justification::centred, false);

            if (scale && i < names.size() - 1 && i < steps.size())
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (11.0f));
                g.drawText (steps[i], juce::Rectangle<float> (x + cell, cy - 9.0f, gap, 18.0f), juce::Justification::centred, false);
                g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            }
        }
    }

    /** The marks the rows use, each with what it means. */
    std::vector<std::pair<juce::Colour, juce::String>> getLegend() const
    {
        bool home = false, bass = false, marked = false;

        for (auto& r : example.rows)
        {
            home = home || (r.home >= 0 && (isScaleRow (r) || r.bass >= 0));
            bass = bass || r.bass >= 0;
            marked = marked || ! r.marked.empty();
        }

        std::vector<std::pair<juce::Colour, juce::String>> items;

        if (home)
            items.push_back ({ juce::Colours::white, "home or root" });

        if (bass)
            items.push_back ({ bassColour, "bass" });

        if (marked)
            items.push_back ({ outsideColour, "raised or characteristic" });

        return items;
    }

    static int getLegendWidth (const std::vector<std::pair<juce::Colour, juce::String>>& items)
    {
        const juce::Font font (juce::FontOptions (11.5f));
        float total = 0.0f;

        for (auto& it : items)
            total += juce::GlyphArrangement::getStringWidth (font, it.second) + 20.0f;

        return (int) total;
    }

    void paintLegend (juce::Graphics& g, juce::Rectangle<int> area, const std::vector<std::pair<juce::Colour, juce::String>>& items) const
    {
        g.setFont (juce::FontOptions (11.5f));
        float x = (float) area.getRight();

        for (auto it = items.rbegin(); it != items.rend(); ++it)
        {
            const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), it->second);
            x -= w + 20.0f;
            g.setColour (it->first);
            g.fillEllipse (x, (float) area.getCentreY() - 4.0f, 8.0f, 8.0f);
            g.setColour (theme::textDim);
            g.drawText (it->second, juce::Rectangle<float> (x + 12.0f, (float) area.getY(), w + 6.0f, (float) area.getHeight()), juce::Justification::centredLeft, false);
        }
    }

    //==========================================================================
    /** Twelve bars, one per note name (C to B). The notes in `lit` are drawn in the accent colour. */
    void paintProfile (juce::Graphics& g, juce::Rectangle<int> area, const std::array<double, 12>& values, const juce::String& heading,
                       bool inBeats, const std::vector<int>& lit, int home) const
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (heading, area.removeFromTop (18), juce::Justification::centredLeft, true);
        area.removeFromBottom (20);
        area.removeFromTop (16);

        double most = 0.0;

        for (double v : values)
            most = juce::jmax (most, v);

        if (most <= 0.0)
            return;

        const float bw = (float) area.getWidth() / 12.0f;

        for (int pc = 0; pc < 12; ++pc)
        {
            const float h = (float) (values[(size_t) pc] / most) * (float) area.getHeight();
            const float x = (float) area.getX() + (float) pc * bw;
            const bool isLit = std::find (lit.begin(), lit.end(), pc) != lit.end();
            const auto bar = juce::Rectangle<float> (x + 3.0f, (float) area.getBottom() - h, bw - 6.0f, h);

            g.setColour (isLit ? theme::accent.withAlpha (pc == home ? 1.0f : 0.8f) : theme::textDim.withAlpha (0.45f));
            g.fillRoundedRectangle (bar, 2.0f);

            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f));

            if (values[(size_t) pc] > 0.0)
                g.drawText (inBeats ? formatBeats (values[(size_t) pc]) : juce::String (values[(size_t) pc], 2),
                            juce::Rectangle<float> (x, bar.getY() - 16.0f, bw, 15.0f), juce::Justification::centred, false);

            g.setFont (juce::FontOptions (13.0f, pc == home ? juce::Font::bold : juce::Font::plain));
            g.setColour (pc == home ? theme::accent : isLit ? theme::text : theme::textDim);
            g.drawText (mh::getNoteName (pc), juce::Rectangle<float> (x - 6.0f, (float) area.getBottom() + 2.0f, bw + 12.0f, 18.0f),
                        juce::Justification::centred, false);
        }
    }

    /** How the example part's profile compares with the best key's own profile. */
    void paintTwoProfiles (juce::Graphics& g, juce::Rectangle<int> area) const
    {
        auto left = area.removeFromLeft (area.getWidth() / 2 - 8);
        area.removeFromLeft (16);

        const bool haveKey = ! keyResult.ranking.empty();
        const auto best = haveKey ? keyResult.ranking[0] : mh::KeyScore();
        std::vector<int> lit;

        for (int i : MusicalKey::getIntervals (best.minor ? MusicalKey::Scale::minor : MusicalKey::Scale::major))
            lit.push_back (mod12 (best.tonic + i));

        paintProfile (g, left, profile, "The tune: beats each note name sounds", true, lit, haveKey ? best.tonic : -1);

        std::array<double, 12> keyProfile {};
        const auto& source = analysis::chordfinder::getKeyProfile (best.minor);

        for (int pc = 0; pc < 12; ++pc)
            keyProfile[(size_t) pc] = source[(size_t) mod12 (pc - best.tonic)];

        paintProfile (g, area, keyProfile, (haveKey ? best.getName() : juce::String ("The key")) + "'s own profile (listeners' ratings)", false, lit,
                      haveKey ? best.tonic : -1);
    }

    /** The notes of an example part: pitch up, time across. A note is labelled with its name (naming a chord) or its
        weight (notes between the chords). */
    void paintNotes (juce::Graphics& g, juce::Rectangle<int> area) const
    {
        std::set<int> pitchSet;

        for (auto& n : example.notes)
            pitchSet.insert (n.pitch);

        const std::vector<int> pitches (pitchSet.begin(), pitchSet.end());
        const int n = (int) pitches.size();

        if (n == 0)
            return;

        const float rowH = (float) juce::jmin (22, 120 / n);
        const float left = (float) area.getX() + 40.0f, right = (float) area.getRight() - 4.0f;
        const float top = (float) area.getY() + 4.0f;
        const float beatW = (right - left) / 4.0f;
        const bool weights = topic == kf::Topic::between;
        const auto& chordNotes = example.rows.empty() ? std::vector<int>() : example.rows[0].pitchClasses;

        for (int b = 0; b <= 4; ++b)
        {
            g.setColour (b == 0 || b == 4 ? theme::gridBar : theme::grid);
            g.fillRect (left + (float) b * beatW, top, 1.0f, (float) n * rowH);
        }

        for (int i = 0; i < n; ++i)
        {
            const float y = top + (float) (n - 1 - i) * rowH;
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawText (mh::getNoteName (mod12 (pitches[(size_t) i])) + juce::String (pitches[(size_t) i] / 12 - 1), juce::Rectangle<float> ((float) area.getX(), y, 36.0f, rowH),
                        juce::Justification::centredRight, false);
        }

        for (auto& note : example.notes)
        {
            const int index = (int) (std::find (pitches.begin(), pitches.end(), note.pitch) - pitches.begin());
            const float y = top + (float) (n - 1 - index) * rowH;
            const float x = left + (float) note.start * beatW;
            const float w = juce::jmax (4.0f, (float) note.length * beatW - 2.0f);
            const bool inChord = std::find (chordNotes.begin(), chordNotes.end(), mod12 (note.pitch)) != chordNotes.end();
            const auto rect = juce::Rectangle<float> (x + 1.0f, y + 1.0f, w, rowH - 2.0f);

            g.setColour (weights && ! inChord ? outsideColour : theme::accent);
            g.fillRoundedRectangle (rect, 3.0f);

            const auto label = weights ? juce::String (mh::getWeight ({ note.pitch, note.start, note.length, note.velocity }, 0.0, 4.0), 1)
                                       : mh::getNoteName (mod12 (note.pitch));
            g.setFont (juce::FontOptions (juce::jmin (12.5f, rowH - 4.0f), juce::Font::bold));
            const float textW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), label) + 8.0f;

            if (w >= textW)
            {
                g.setColour (theme::background);
                g.drawText (label, rect.withTrimmedLeft (4.0f), juce::Justification::centredLeft, false);
            }
            else
            {
                g.setColour (theme::text);
                g.drawText (label, juce::Rectangle<float> (rect.getRight() + 2.0f, y, textW, rowH), juce::Justification::centredLeft, false);
            }
        }
    }

    //==========================================================================
    void playRow (const kf::Row& row)
    {
        if (! onPlayNote)
            return;

        if (isScaleRow (row))
        {
            // The scale up from home, one note at a time, and home again an octave up.
            auto pcs = row.pitchClasses;
            std::sort (pcs.begin(), pcs.end(), [&] (int a, int b) { return mod12 (a - row.home) < mod12 (b - row.home); });
            int delay = 0;

            for (int pc : pcs)
            {
                onPlayNote (60 + row.home + mod12 (pc - row.home), 0.75f, delay, 260);
                delay += 280;
            }

            onPlayNote (72 + row.home, 0.75f, delay, 500);
            return;
        }

        std::vector<int> notes = row.pitches;

        if (notes.empty())
        {
            if (row.home >= 0)
                notes.push_back (48 + row.home);

            for (int pc : row.pitchClasses)
                notes.push_back (60 + pc);
        }

        for (int pitch : notes)
            onPlayNote (pitch, pitch < 60 ? 0.65f : 0.75f, 0, 1400);
    }

    void playNotes (const std::vector<mh::Note>& notes)
    {
        // A beat is half a second (120 BPM).
        for (auto& n : notes)
            if (onPlayNote)
                onPlayNote (n.pitch, n.velocity, (int) (n.start * 500.0), juce::jmax (80, (int) (n.length * 500.0) - 40));
    }

    //==========================================================================
    /** Where the Your clip picture puts the bars and the notes. */
    struct ClipGeometry
    {
        juce::Rectangle<float> bars;            // the bars' area (chords above, notes below)
        juce::Rectangle<float> notes;
        juce::Rectangle<float> profile;         // the profile strip
        double firstBeat = 0.0, lastBeat = 0.0;
        int low = 127, high = 0;
        float beatWidth = 1.0f;

        float xOf (double beat) const { return bars.getX() + (float) (beat - firstBeat) * beatWidth; }
    };

    ClipGeometry getClipGeometry() const
    {
        ClipGeometry c;
        const float profileWidth = 150.0f;
        c.profile = juce::Rectangle<float> ((float) getWidth() - 12.0f - profileWidth, 8.0f, profileWidth, (float) getHeight() - 16.0f);
        c.bars = juce::Rectangle<float> (12.0f, 8.0f, c.profile.getX() - 12.0f - 14.0f, (float) getHeight() - 16.0f);
        c.notes = c.bars.withTrimmedTop (44.0f);

        const int firstBar = (int) std::floor (yours.input.startBeat / 4.0 + 1.0e-9);
        const int endBar = (int) std::ceil (yours.input.endBeat / 4.0 - 1.0e-9);
        const int shown = juce::jlimit (1, 16, endBar - firstBar);
        c.firstBeat = firstBar * 4.0;
        c.lastBeat = (firstBar + shown) * 4.0;
        c.beatWidth = c.bars.getWidth() / (float) (c.lastBeat - c.firstBeat);

        for (auto& n : yours.input.notes)
            if (n.start < c.lastBeat && n.getEnd() > c.firstBeat)
            {
                c.low = juce::jmin (c.low, n.pitch);
                c.high = juce::jmax (c.high, n.pitch);
            }

        if (c.high < c.low)
            c.low = c.high = 60;

        return c;
    }

    const mh::Segment* findSegmentAt (juce::Point<int> p) const
    {
        if (topic != kf::Topic::yours || ! yours.found || yours.drums || p.x < 0)
            return nullptr;

        const auto c = getClipGeometry();

        if (! c.bars.contains (p.toFloat()))
            return nullptr;

        const double beat = c.firstBeat + (double) (p.x - c.bars.getX()) / (double) c.beatWidth;

        for (auto& s : yours.result.segments)
            if (beat >= s.startBeat - 1.0e-9 && beat < s.endBeat - 1.0e-9)
                return &s;

        return nullptr;
    }

    juce::String describeSegment (const mh::Segment& s) const
    {
        auto text = "Bar " + juce::String (s.bar + 1) + (s.split ? (s.secondHalf ? " (second half)" : " (first half)") : juce::String()) + " · ";

        if (! s.chord)
            return text + "no chord: nothing sounds together here, or too little to name one.";

        text += s.getName();

        if (yours.numeralKey)
            text += " (" + gen::guide::getNumeral (*yours.numeralKey, *s.chord) + ")";

        if (s.otherChord && s.otherGap < 0.05)
            text += " · or " + mh::getChordName (*s.otherChord, s.otherBass);

        return text + ". Click to hear it.";
    }

    juce::Colour getNoteColour (const mh::Note& note) const
    {
        const int pc = mod12 (note.pitch);

        for (auto& s : yours.result.segments)
            if (note.start >= s.startBeat - 1.0e-9 && note.start < s.endBeat - 1.0e-9)
            {
                if (s.chord && s.chord->getPitchClasses().contains (pc))
                    return ChordLane::getColour (*s.chord).brighter (0.5f);

                break;
            }

        if (! yours.result.tooFew)
        {
            bool inScale = false;

            for (int i : mh::getScaleIntervals (yours.result.key.scale))
                inScale = inScale || mod12 (yours.result.key.tonic + i) == pc;

            if (! inScale)
                return outsideColour;
        }

        return theme::textDim.withAlpha (0.8f);
    }

    void paintYours (juce::Graphics& g)
    {
        if (! yours.found || yours.drums)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (14.0f));
            g.drawFittedText (yours.drums ? yours.trackName + " plays drum sounds, so its notes have no key."
                                          : juce::String ("Select a MIDI clip in the arrangement: its bars and chords appear here."),
                              getLocalBounds().reduced (20), juce::Justification::centred, 3, 0.9f);
            return;
        }

        const auto c = getClipGeometry();
        const auto& result = yours.result;

        // The bars: lines, and their numbers.
        const int firstBar = (int) std::lround (c.firstBeat / 4.0), lastBar = (int) std::lround (c.lastBeat / 4.0);

        for (int bar = firstBar; bar <= lastBar; ++bar)
        {
            const float x = c.xOf (bar * 4.0);
            g.setColour (theme::gridBar);
            g.fillRect (x, c.notes.getY(), 1.0f, c.notes.getHeight());

            if (bar < lastBar && c.beatWidth * 4.0f >= 22.0f)
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (10.5f));
                g.drawText (juce::String (bar + 1), juce::Rectangle<float> (x + 3.0f, c.notes.getY() + 1.0f, 28.0f, 12.0f), juce::Justification::centredLeft, false);
            }
        }

        // The chord over each bar or half bar, and its numeral under it.
        for (auto& s : result.segments)
        {
            if (s.startBeat >= c.lastBeat || s.endBeat <= c.firstBeat)
                continue;

            const auto cell = juce::Rectangle<float> (c.xOf (s.startBeat) + 1.0f, c.bars.getY(), (float) (s.endBeat - s.startBeat) * c.beatWidth - 2.0f, 38.0f);

            if (! s.chord)
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (13.0f));
                g.drawText ("-", cell, juce::Justification::centred, false);
                continue;
            }

            g.setColour (ChordLane::getColour (*s.chord).withAlpha (0.55f));
            g.fillRoundedRectangle (cell, 4.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
            g.drawFittedText (s.getName(), cell.toNearestInt().withTrimmedBottom (14), juce::Justification::centred, 1, 0.6f);

            if (yours.numeralKey)
            {
                g.setColour (theme::text.withAlpha (0.8f));
                g.setFont (juce::FontOptions (11.5f));
                g.drawFittedText (gen::guide::getNumeral (*yours.numeralKey, *s.chord), cell.toNearestInt().withTrimmedTop (22), juce::Justification::centred, 1, 0.6f);
            }
        }

        // The notes.
        const int range = c.high - c.low + 1;
        const float step = juce::jlimit (3.0f, 11.0f, (c.notes.getHeight() - 16.0f) / (float) range);
        const float used = step * (float) range;
        const float yBottom = c.notes.getBottom() - 6.0f - ((c.notes.getHeight() - 12.0f - used) * 0.5f);

        for (auto& n : yours.input.notes)
        {
            if (n.start >= c.lastBeat || n.getEnd() <= c.firstBeat)
                continue;

            const float x0 = c.xOf (juce::jmax (n.start, c.firstBeat)), x1 = c.xOf (juce::jmin (n.getEnd(), c.lastBeat));
            const float y = yBottom - (float) (n.pitch - c.low + 1) * step;
            g.setColour (getNoteColour (n));
            g.fillRoundedRectangle (juce::Rectangle<float> (x0 + 0.5f, y, juce::jmax (2.0f, x1 - x0 - 1.0f), step - 0.5f), 1.5f);
        }

        if (yours.input.endBeat > c.lastBeat + 1.0e-9)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.0f));
            g.drawText ("Showing bars " + juce::String (firstBar + 1) + " to " + juce::String (lastBar), c.notes.withTrimmedTop (c.notes.getHeight() - 14.0f), juce::Justification::centredRight, false);
        }

        paintStrip (g, c);
    }

    /** The profile at the right: one bar per note name, C at the bottom, with the notes of the found scale in bold. */
    void paintStrip (juce::Graphics& g, const ClipGeometry& c) const
    {
        const auto& result = yours.result;
        auto area = c.profile;
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (result.tooFew ? "Note names" : "Note names, " + result.key.name + " lit", area.removeFromTop (16.0f), juce::Justification::centredLeft, false);

        double most = 0.0;

        for (double v : result.profile)
            most = juce::jmax (most, v);

        std::vector<int> inScale;

        if (! result.tooFew)
            for (int i : mh::getScaleIntervals (result.key.scale))
                inScale.push_back (mod12 (result.key.tonic + i));

        const float rowH = area.getHeight() / 12.0f;

        for (int pc = 0; pc < 12; ++pc)
        {
            const float y = area.getBottom() - (float) (pc + 1) * rowH;
            const bool lit = std::find (inScale.begin(), inScale.end(), pc) != inScale.end();
            const bool home = ! result.tooFew && pc == result.key.tonic;
            g.setColour (home ? theme::accent : lit ? theme::text : theme::textDim);
            g.setFont (juce::FontOptions (12.0f, lit ? juce::Font::bold : juce::Font::plain));
            g.drawText (mh::getNoteName (pc), juce::Rectangle<float> (area.getX(), y, 28.0f, rowH), juce::Justification::centredLeft, false);

            if (most > 0.0)
            {
                const float w = (float) (result.profile[(size_t) pc] / most) * (area.getWidth() - 34.0f);
                g.setColour (lit ? theme::accent.withAlpha (home ? 1.0f : 0.75f) : theme::textDim.withAlpha (0.5f));
                g.fillRoundedRectangle (area.getX() + 30.0f, y + 2.0f, juce::jmax (result.profile[(size_t) pc] > 0.0 ? 2.0f : 0.0f, w), rowH - 4.0f, 2.0f);
            }
        }
    }

    //==========================================================================
    kf::Topic topic = kf::Topic::key;
    kf::Example example;
    std::array<double, 12> profile {};
    mh::Result keyResult;
    YourClip yours;
    juce::Point<int> hover { -1, -1 };
};

//==============================================================================
KeyFindingExplainer::KeyFindingExplainer (Engine& e, Selection& s) : engine (e), selection (s)
{
    title.setText ("Finding the key and chords", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    picture->onPlayNote = [this] (int pitch, float velocity, int delayMs, int lengthMs)
    {
        engine.setPreviewSound (BuiltInSound::piano);   // when no MIDI track can play it

        if (delayMs <= 0)
        {
            engine.previewNote (pitch, velocity, lengthMs);
            return;
        }

        juce::Timer::callAfterDelay (delayMs, [safe = juce::Component::SafePointer<KeyFindingExplainer> (this), pitch, velocity, lengthMs]
        {
            if (safe != nullptr)
                safe->engine.previewNote (pitch, velocity, lengthMs);
        });
    };
    picture->onPlayChord = [this] (const Chord& chord, int bass)
    {
        // The bass in octave 3 and the chord in close position just above middle C, for half a bar at the song's tempo.
        const int ms = juce::jlimit (500, 4000, juce::roundToInt (2.0 * 60000.0 / engine.getTempo())) - 60;
        engine.setPreviewSound (BuiltInSound::piano);
        engine.previewNote (48 + (bass >= 0 ? bass : chord.root), 0.65f, ms);

        for (int pc : chord.getPitchClasses())
            engine.previewNote (60 + pc, 0.75f, ms);
    };
    addAndMakeVisible (*picture);

    explanation.setMultiLine (true, true);
    explanation.setReadOnly (true);
    explanation.setCaretVisible (false);
    explanation.setScrollbarsShown (true);
    explanation.setFont (juce::FontOptions (15.0f));
    explanation.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    explanation.setColour (juce::TextEditor::textColourId, theme::text);
    explanation.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    explanation.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    explanation.setBorder (juce::BorderSize<int> (10, 12, 10, 12));
    addAndMakeVisible (explanation);
    footer.onReadChanged = [this] { list.repaint(); };
    addChildComponent (footer);
    addAndMakeVisible (exportButton);

    putButton.setTooltip ("Writes one chord per bar over this clip's bars on the chord track, without the bass note. Asks before replacing chords already there.");
    putButton.onClick = [this] { putOnChordTrack(); };
    keyButton.setTooltip ("The chord menu, the Roman numerals, the scale filter and Generate follow the song's key.");
    keyButton.onClick = [this] { setSongKey(); };
    filterButton.setTooltip ("The piano roll of this track shows only the notes of the song's key. Notes already in clips play as they are.");
    filterButton.setClickingTogglesState (true);
    filterButton.onClick = [this] { toggleScaleFilter(); };

    for (auto* b : { &putButton, &keyButton, &filterButton })
        addChildComponent (b);

    status.setFont (juce::FontOptions (13.0f));
    status.setColour (juce::Label::textColourId, theme::textDim);
    addChildComponent (status);

    engine.addChangeListener (this);
    selection.addChangeListener (this);

    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
}

KeyFindingExplainer::~KeyFindingExplainer()
{
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

juce::String KeyFindingExplainer::getPrintSource() const
{
    const auto y = analyseSelection();
    return y.found && ! y.drums ? "example parts, and the clip on " + y.trackName : juce::String ("example parts");
}

int KeyFindingExplainer::getNumPrintTopics() const
{
    return (int) std::size (kf::allTopics) + 1;
}

booklet::Topic KeyFindingExplainer::showPrintTopic (int index)
{
    if (GuideSourcesTopic::isRow (index, (int) std::size (kf::allTopics)))
        return sources.getPrintTopic();

    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    printHeight = picture->getPreferredHeight();   // all of it, not what fits the window
    return { kf::getTopicName (kf::allTopics[index]), kf::getTopicSummary (kf::allTopics[index]), explanation.getText(),
             { picture->getWidth(), printHeight }, {} };
}

juce::Image KeyFindingExplainer::paintPrintPicture (float scale)
{
    picture->setSize (picture->getWidth(), printHeight);
    auto image = guideprint::snapshot (*picture, scale);
    resized();
    return image;
}

void KeyFindingExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void KeyFindingExplainer::showTopic (kf::Topic topic)
{
    for (int i = 0; i < (int) std::size (kf::allTopics); ++i)
        if (kf::allTopics[i] == topic)
        {
            if (list.getSelectedRow() == i)
                show (i);   // already there: look at the selection again
            else
                list.selectRow (i);
        }
}

int KeyFindingExplainer::getNumRows()
{
    return (int) std::size (kf::allTopics) + 1;
}

void KeyFindingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (kf::allTopics)))
    {
        sources.paintRow (g, w, h, selected);
        return;
    }

    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = kf::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (kf::getTopicName (topic), 10, 5, w - 36, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (kf::getTopicSummary (topic), 10, 23, w - 36, 16, juce::Justification::centredLeft, true);

    GuideFooter::paintTick (g, w, h, gen::guides::Guide::keyFinding, kf::getTopicName (topic));

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void KeyFindingExplainer::selectedRowsChanged (int row)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (kf::allTopics)))
    {
        showSources();
        return;
    }

    if (row >= 0)
        show (row);
}

void KeyFindingExplainer::showSources()
{
    sources.select (*this, list, (int) std::size (kf::allTopics), { &list, &title, &status, &exportButton });
    updateFooter();
}

void KeyFindingExplainer::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // "Your clip" follows the selection and the edits.
    if (source == &selection)
        status.setText ({}, juce::dontSendNotification);

    if (! sources.isShown() && kf::allTopics[shownRow] == kf::Topic::yours)
        show (shownRow);
}

KeyFindingExplainer::YourClip KeyFindingExplainer::analyseSelection() const
{
    YourClip y;
    Track* track = selection.getTrack();
    MidiClip* clip = selection.getMidiClip();

    if (clip == nullptr && track != nullptr && track->isMidi())
    {
        // Only a MIDI track is selected: the clip under the playhead, else its first.
        const juce::ScopedLock lock (engine.getLock());
        const double position = engine.getPositionBeats();
        MidiClip* first = nullptr;

        for (auto& c : track->clips)
        {
            if (c->kind != Clip::Kind::midi)
                continue;

            auto* m = static_cast<MidiClip*> (c.get());

            if (position >= m->startBeat && position < m->getEndBeat (engine.getTempoMap()))
            {
                clip = m;
                break;
            }

            if (first == nullptr || m->startBeat < first->startBeat)
                first = m;
        }

        if (clip == nullptr)
            clip = first;
    }

    if (clip == nullptr || track == nullptr)
        return y;

    y.found = true;
    y.track = track;
    y.trackName = track->name;
    y.drums = track->usesBuiltInSynth() && builtInSounds::isPercussive (track->builtInSound.load());

    if (y.drums)
        return y;

    {
        const juce::ScopedLock lock (engine.getLock());
        y.input = mh::makeInput (*clip, engine.getTimeSignature());
    }

    y.result = mh::analyse (y.input);
    y.numeralKey = y.result.tooFew ? engine.getKey() : y.result.key.songKey;
    return y;
}

void KeyFindingExplainer::show (int index)
{
    sources.close();
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    const auto topic = kf::allTopics[shownRow];

    if (topic == kf::Topic::yours)
    {
        yours = analyseSelection();
        picture->setClip (yours);

        kf::ClipView view;
        view.found = yours.found;
        view.drums = yours.drums;
        view.trackName = yours.trackName;
        view.songKey = engine.getKey();
        view.result = yours.result;
        explanation.setText (kf::explainYourClip (view), false);
    }
    else
    {
        yours = {};
        picture->setTopic (topic, kf::getExample (topic));
        explanation.setText (kf::getText (topic), false);
    }

    explanation.moveCaretToTop (false);
    updateActions();
    resized();
    updateFooter();
}

void KeyFindingExplainer::updateActions()
{
    const bool onYours = kf::allTopics[shownRow] == kf::Topic::yours;
    const bool usable = onYours && yours.found && ! yours.drums;
    const bool hasKey = usable && ! yours.result.tooFew;
    const bool hasChords = usable && ! yours.result.melody && ! kf::getBarChords (yours.result.segments).empty();

    putButton.setVisible (usable);
    putButton.setEnabled (hasChords);

    keyButton.setVisible (hasKey && engine.getKey() != yours.result.key.songKey);
    keyButton.setButtonText ("Set the song's key to " + (hasKey ? yours.result.key.songKey.getName() : juce::String ("the key found")));

    filterButton.setVisible (hasKey);
    filterButton.setButtonText ("Show only notes in " + (hasKey ? yours.result.key.songKey.getName() : juce::String ("the key found")));
    filterButton.setEnabled (hasKey && engine.getKey() == yours.result.key.songKey);
    filterButton.setToggleState (hasKey && yours.track != nullptr && yours.track->scaleFilter.load(), juce::dontSendNotification);

    status.setVisible (onYours);
}

void KeyFindingExplainer::setStatus (const juce::String& text)
{
    status.setText (text, juce::dontSendNotification);
}

void KeyFindingExplainer::putOnChordTrack()
{
    const auto y = analyseSelection();

    if (! y.found || y.drums)
        return;

    const auto merge = kf::mergeIntoChordTrack (y.result.segments, engine.getChords());

    if (merge.toWrite.empty())
    {
        setStatus ("No chords found in this clip, so the chord track stays as it is.");
        return;
    }

    auto apply = [safeThis = juce::Component::SafePointer<KeyFindingExplainer> (this), merge]
    {
        auto* self = safeThis.getComponent();

        if (self == nullptr)
            return;

        self->engine.setChords (kf::applyMerge (merge, self->engine.getChords()));
        self->setStatus ("Put " + juce::String ((int) merge.toWrite.size()) + (merge.toWrite.size() == 1 ? " chord" : " chords")
                         + " on the chord track, in bars " + juce::String (merge.firstBar + 1) + " to " + juce::String (merge.lastBar + 1) + ".");
    };

    const auto question = kf::getReplaceQuestion (merge);

    if (question.isEmpty())
    {
        apply();
        return;
    }

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Replace chords?", question, "Replace", "Cancel", this,
                                        juce::ModalCallbackFunction::create ([apply] (int result) { if (result == 1) apply(); }));
}

void KeyFindingExplainer::setSongKey()
{
    const auto y = analyseSelection();

    if (! y.found || y.drums || y.result.tooFew)
        return;

    engine.setKey (y.result.key.songKey);
    setStatus ("The song is now in " + y.result.key.songKey.getName() + ".");
}

void KeyFindingExplainer::toggleScaleFilter()
{
    const auto y = analyseSelection();

    if (y.track == nullptr || y.drums || y.result.tooFew)
        return;

    engine.setScaleFilter (*y.track, filterButton.getToggleState());
    setStatus (filterButton.getToggleState() ? "The piano roll of " + y.trackName + " now shows only the notes of " + engine.getKey().getName() + "."
                                             : "The piano roll of " + y.trackName + " shows all the notes again.");
}

void KeyFindingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void KeyFindingExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    footer.takeSpace (r);

    if (sources.isShown())
    {
        sources.show (true, r);
        return;
    }

    picture->setBounds (r.removeFromTop (juce::jmin (picture->getPreferredHeight(), (int) ((float) r.getHeight() * 0.6f))));
    r.removeFromTop (10);

    if (putButton.isVisible() || keyButton.isVisible() || filterButton.isVisible())
    {
        auto row = r.removeFromTop (28);

        for (auto* b : { &putButton, &keyButton, &filterButton })
            if (b->isVisible())
            {
                b->setBounds (row.removeFromLeft (b->getBestWidthForHeight (28) + 16));
                row.removeFromLeft (8);
            }

        r.removeFromTop (4);
    }

    if (status.isVisible())
        status.setBounds (r.removeFromTop (20));

    explanation.setBounds (r);
}

void KeyFindingExplainer::updateFooter()
{
    if (footer.update (gen::guides::Guide::keyFinding, sources.isShown() ? gen::guides::sourcesTopicName : kf::getTopicName (kf::allTopics[shownRow])))
        resized();
}

bool KeyFindingExplainer::keyPressed (const juce::KeyPress& key)
{
    return footer.keyPressed (key);
}
} // namespace daw
