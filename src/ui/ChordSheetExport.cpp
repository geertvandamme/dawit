#include "ChordSheetExport.h"
#include "ChordDiagramViews.h"
#include "engine/EffectTools.h"
#include "engine/GuideBooklet.h"
#include "engine/PdfWriter.h"
#include "generate/ChordSheet.h"
#include "Theme.h"

namespace daw::chordsheet
{
namespace
{
    namespace diagrams = gen::diagrams;
    namespace guide = gen::guide;
    using gen::sheet::Entry;

    // Dark on white, for paper (shared with the guide booklets).
    using booklet::ink;
    using booklet::inkDim;
    using booklet::rule;

    /** The chord guide's family colours, darker so they read on white. */
    juce::Colour familyColour (guide::Family family)
    {
        switch (family)
        {
            case guide::Family::home:    return juce::Colour (0xff23944f);
            case guide::Family::away:    return juce::Colour (0xff2a72cc);
            case guide::Family::tension: return juce::Colour (0xffc96a12);
            case guide::Family::outside: return juce::Colour (0xff7d4fd0);
        }

        return inkDim;
    }

    /** The note name for each of the chord's pitch classes, as the sheet spells them ("Eb", not "D#", in Cm). */
    std::array<juce::String, 12> spell (const Entry& e)
    {
        std::array<juce::String, 12> names;
        const auto spelled = juce::StringArray::fromTokens (e.notes, " ", {});
        const auto pitchClasses = e.chord.getPitchClasses();

        for (int i = 0; i < pitchClasses.size() && i < spelled.size(); ++i)
            names[(size_t) pitchClasses[i]] = spelled[i];

        return names;
    }

    juce::String spellNotes (const Entry& e, const std::vector<int>& notes)
    {
        const auto names = spell (e);
        juce::StringArray text;

        for (int n : notes)
            text.add (names[(size_t) ((n % 12 + 12) % 12)]);

        return text.joinIntoString (" ");
    }

    void drawText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, bool bold = false, juce::Colour c = ink,
                   juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (text, r, j, true);
    }

    /** Paints a diagram component into the page at `area` (in points). */
    void paintDiagram (juce::Graphics& g, juce::Component& diagram, juce::Rectangle<float> area)
    {
        diagram.setSize (juce::roundToInt (area.getWidth()), juce::roundToInt (area.getHeight()));
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::translation (area.getX(), area.getY()));
        diagram.paint (g);
    }

    //==============================================================================
    /** A4 pages filled from the top down, in points; a new page starts when the next part doesn't fit. */
    class Pages
    {
    public:
        static constexpr float width = pdf::a4Width, height = pdf::a4Height, margin = 40.0f;
        static constexpr float left = margin, right = width - margin, bottom = height - margin - 18.0f;

        Pages (float pixelsPerPoint, const juce::String& footerText) : scale (pixelsPerPoint), footer (footerText)
        {
            newPage();
        }

        juce::Graphics& g() { return *graphics; }

        /** Starts a new page unless `h` more points fit on this one. */
        void need (float h)
        {
            if (y + h > bottom)
                newPage();
        }

        void newPage()
        {
            graphics.reset();
            pages.emplace_back (juce::Image::RGB, juce::roundToInt (width * scale), juce::roundToInt (height * scale), true);
            graphics = std::make_unique<juce::Graphics> (pages.back());
            graphics->fillAll (juce::Colours::white);
            graphics->addTransform (juce::AffineTransform::scale (scale));
            y = margin;
        }

        /** Wrapped text across the page (or from `x`, `w` wide); returns its height. */
        float paragraph (const juce::String& text, float size, juce::Colour c = inkDim, float x = left, float w = right - left)
        {
            juce::AttributedString s;
            s.setWordWrap (juce::AttributedString::byWord);
            s.append (text, juce::FontOptions (size), c);
            juce::TextLayout layout;
            layout.createLayout (s, w);
            need (layout.getHeight());
            layout.draw (g(), { x, y, w, layout.getHeight() });
            y += layout.getHeight();
            return layout.getHeight();
        }

        /** The pages, each with the title and its number at the foot. */
        std::vector<juce::Image> finish()
        {
            graphics.reset();

            for (size_t i = 0; i < pages.size(); ++i)
            {
                juce::Graphics fg (pages[i]);
                fg.addTransform (juce::AffineTransform::scale (scale));
                fg.setColour (rule);
                fg.fillRect (left, height - margin - 8.0f, right - left, 0.6f);
                const auto line = juce::Rectangle<float> (left, height - margin - 4.0f, right - left, 12.0f);
                drawText (fg, footer + "  \xc2\xb7  made with DawIt", line, 8.5f, false, inkDim);
                drawText (fg, "Page " + juce::String ((int) i + 1) + " of " + juce::String ((int) pages.size()), line, 8.5f, false, inkDim,
                          juce::Justification::centredRight);
            }

            return std::move (pages);
        }

        float y = margin;

    private:
        float scale;
        juce::String footer;
        std::vector<juce::Image> pages;
        std::unique_ptr<juce::Graphics> graphics;
    };

    //==============================================================================
    void drawHeading (Pages& p, const juce::String& title, const juce::String& text, float firstRowHeight)
    {
        p.need (40.0f + firstRowHeight);
        p.y += 10.0f;
        drawText (p.g(), title, { Pages::left, p.y, Pages::right - Pages::left, 20.0f }, 15.0f, true);
        p.y += 22.0f;

        if (text.isNotEmpty())
            p.paragraph (text, 9.5f);

        p.y += 8.0f;
    }

    /** The numeral in its family's colour, the name, and the family and role on the right. */
    void drawEntryHeader (juce::Graphics& g, const Entry& e, juce::Rectangle<float> r, bool roleBelow)
    {
        auto line = r.removeFromTop (18.0f);
        const auto numeralWidth = juce::jmax (30.0f, juce::GlyphArrangement::getStringWidth (juce::FontOptions (13.0f, juce::Font::bold), e.numeral) + 8.0f);
        drawText (g, e.numeral, line.removeFromLeft (numeralWidth), 13.0f, true, familyColour (e.family));
        drawText (g, e.name, roleBelow ? line : line.removeFromLeft (70.0f), 15.0f, true);

        const auto role = guide::getFamilyName (e.family) + (e.role.isNotEmpty() ? ", " + e.role : juce::String());
        const auto roleArea = roleBelow ? r.removeFromTop (12.0f) : line;
        g.setColour (familyColour (e.family));
        g.setFont (juce::FontOptions (roleBelow ? 8.5f : 9.0f));
        g.drawFittedText (role, roleArea.toNearestInt(), roleBelow ? juce::Justification::centredLeft : juce::Justification::centredRight, 1, 0.75f);
    }

    //==============================================================================
    void drawPianoCells (Pages& p, const std::vector<Entry>& entries)
    {
        constexpr int columns = 2;
        constexpr float gap = 22.0f, cellHeight = 104.0f;
        const float cellWidth = (Pages::right - Pages::left - gap * (columns - 1)) / columns;

        // One range for the whole section, so the keyboards line up.
        int low = 48, high = 83;

        for (auto& e : entries)
        {
            low = juce::jmin (low, diagrams::pianoBass (e.chord));

            for (int n : diagrams::pianoInversion (e.chord, 0))
                high = juce::jmax (high, n);
        }

        KeyboardDiagram keyboard;
        keyboard.setPrinted (true);
        keyboard.setRange (low, high);

        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (i % columns == 0)
            {
                if (i > 0)
                    p.y += cellHeight;

                p.need (cellHeight);
            }

            const auto& e = entries[i];
            auto cell = juce::Rectangle<float> (Pages::left + (cellWidth + gap) * (float) (i % columns), p.y, cellWidth, cellHeight - 10.0f);
            drawEntryHeader (p.g(), e, cell.removeFromTop (18.0f), false);

            juce::StringArray inversions;

            for (int inversion = 1; inversion < diagrams::getNumInversions (e.chord); ++inversion)
                inversions.add (spellNotes (e, diagrams::pianoInversion (e.chord, inversion)));

            drawText (p.g(), "Notes " + e.notes + "     inversions " + inversions.joinIntoString (", "), cell.removeFromTop (15.0f), 9.0f, false, inkDim);
            cell.removeFromTop (3.0f);

            keyboard.setChord (diagrams::pianoInversion (e.chord, 0), diagrams::pianoBass (e.chord), e.chord.root);
            keyboard.setNames (spell (e));
            paintDiagram (p.g(), keyboard, cell);
        }

        p.y += cellHeight;
    }

    void drawGuitarCells (Pages& p, const std::vector<Entry>& entries)
    {
        constexpr int columns = 4;
        constexpr float gap = 14.0f, cellHeight = 168.0f;
        const float cellWidth = (Pages::right - Pages::left - gap * (columns - 1)) / columns;

        GuitarDiagram box;
        box.setPrinted (true);

        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (i % columns == 0)
            {
                if (i > 0)
                    p.y += cellHeight;

                p.need (cellHeight);
            }

            const auto& e = entries[i];
            auto cell = juce::Rectangle<float> (Pages::left + (cellWidth + gap) * (float) (i % columns), p.y, cellWidth, cellHeight - 10.0f);
            drawEntryHeader (p.g(), e, cell.removeFromTop (30.0f), true);
            cell.removeFromTop (2.0f);

            const auto shapes = diagrams::getGuitarShapes (e.chord, 2);
            box.setShape (shapes.empty() ? diagrams::GuitarShape() : shapes.front());
            paintDiagram (p.g(), box, cell.removeFromTop (96.0f).withSizeKeepingCentre (juce::jmin (cellWidth, 92.0f), 96.0f));
            cell.removeFromTop (3.0f);

            if (! shapes.empty())
            {
                drawText (p.g(), shapes.front().getTab(), cell.removeFromTop (13.0f), 10.5f, true, ink, juce::Justification::centred);
                drawText (p.g(), diagrams::getDifficultyName (shapes.front().difficulty) + (shapes.size() > 1 ? ";  or " + shapes[1].getTab() : juce::String()),
                          cell.removeFromTop (12.0f), 8.5f, false, inkDim, juce::Justification::centred);
            }
        }

        p.y += cellHeight;
    }

    void drawCells (Pages& p, Instrument instrument, const std::vector<Entry>& entries)
    {
        instrument == Instrument::piano ? drawPianoCells (p, entries) : drawGuitarCells (p, entries);
    }

    //==============================================================================
    void drawProgression (Pages& p, Instrument instrument, const gen::sheet::ProgressionRow& row)
    {
        const bool piano = instrument == Instrument::piano;
        const float diagramHeight = piano ? 48.0f : 92.0f, rowHeight = 14.0f + diagramHeight + 8.0f;
        p.need (33.0f + rowHeight);

        // The name, the numerals (the chord names are over the diagrams) and what it is.
        drawText (p.g(), row.name, { Pages::left, p.y, 200.0f, 16.0f }, 12.0f, true);
        drawText (p.g(), row.numerals, { Pages::left + 200.0f, p.y, Pages::right - Pages::left - 200.0f, 16.0f }, 10.5f, true, ink,
                  juce::Justification::centredRight);
        p.y += 16.0f;
        drawText (p.g(), row.summary, { Pages::left, p.y, Pages::right - Pages::left, 13.0f }, 9.0f, false, inkDim);
        p.y += 17.0f;

        std::vector<Chord> chords;

        for (auto& e : row.chords)
            chords.push_back (e.chord);

        // Long ones, such as the twelve-bar blues, go on in more rows.
        const int perRow = piano ? 4 : 6;
        const float gap = 8.0f;
        const float cellWidth = juce::jmin (piano ? 160.0f : 96.0f, (Pages::right - Pages::left - gap * (float) (perRow - 1)) / (float) perRow);
        auto cellArea = [&] (size_t i)
        {
            if (i > 0 && i % (size_t) perRow == 0)
            {
                p.y += rowHeight;
                p.need (rowHeight);
            }

            return juce::Rectangle<float> (Pages::left + (cellWidth + gap) * (float) (i % (size_t) perRow), p.y, cellWidth, 14.0f + diagramHeight);
        };

        if (piano)
        {
            // Each chord in the inversion nearest the one before, with the keys that stay ringed.
            const auto voicings = diagrams::voiceProgression (chords);
            int low = 127, high = 0;

            for (size_t i = 0; i < row.chords.size(); ++i)
            {
                low = juce::jmin (low, diagrams::pianoBass (chords[i]));

                for (int note : voicings[i])
                    high = juce::jmax (high, note);
            }

            KeyboardDiagram keyboard;
            keyboard.setPrinted (true);
            keyboard.setShowNames (false);
            keyboard.setRange (low, high);

            for (size_t i = 0; i < row.chords.size(); ++i)
            {
                std::vector<int> held;

                if (i > 0)
                    for (int note : voicings[i])
                        if (std::find (voicings[i - 1].begin(), voicings[i - 1].end(), note) != voicings[i - 1].end())
                            held.push_back (note);

                const auto area = cellArea (i);
                drawText (p.g(), row.chords[i].name + "   " + spellNotes (row.chords[i], voicings[i]), area.withHeight (13.0f), 9.5f, true);
                keyboard.setChord (voicings[i], diagrams::pianoBass (chords[i]), chords[i].root, held);
                paintDiagram (p.g(), keyboard, area.withTrimmedTop (14.0f));
            }
        }
        else
        {
            // One shape per chord, each near the one before, with the fingers that stay ringed.
            const auto shapes = diagrams::shapeProgression (chords);
            GuitarDiagram box;
            box.setPrinted (true);
            box.setCompact (true);

            for (size_t i = 0; i < row.chords.size() && i < shapes.size(); ++i)
            {
                const auto area = cellArea (i);
                drawText (p.g(), row.chords[i].name, area.withHeight (13.0f), 10.0f, true, ink, juce::Justification::centred);
                box.setShape (shapes[i], i > 0 ? &shapes[i - 1] : nullptr);
                paintDiagram (p.g(), box, area.withTrimmedTop (14.0f));
            }
        }

        p.y += rowHeight + 6.0f;
    }

    juce::String getInstrumentName (Instrument instrument)
    {
        return instrument == Instrument::piano ? "piano" : "guitar";
    }
}

//==============================================================================
std::vector<juce::Image> renderPages (const MusicalKey& key, Instrument instrument, float pixelsPerPoint)
{
    const auto sheet = gen::sheet::make (key);
    const bool piano = instrument == Instrument::piano;
    Pages p (pixelsPerPoint, "Chords in " + sheet.keyName + " for " + getInstrumentName (instrument));

    // The title, the key signature and how to read the diagrams.
    drawText (p.g(), "Chords in " + sheet.keyName, { Pages::left, p.y, Pages::right - Pages::left, 32.0f }, 26.0f, true);
    p.y += 34.0f;
    drawText (p.g(), "For " + getInstrumentName (instrument) + "   \xc2\xb7   key signature: " + sheet.keySignature,
              { Pages::left, p.y, Pages::right - Pages::left, 14.0f }, 10.5f, false, inkDim);
    p.y += 22.0f;

    p.paragraph (piano ? "Each keyboard shows the chord in root position: the right hand's notes in blue, around middle C, and the left "
                         "hand's bass note, the root, in orange; a filled dot marks the root. An inversion is the same notes starting "
                         "from the next one up; they are spelled out above each keyboard, and in a progression each chord is played "
                         "in the inversion nearest the one before, with the keys that stay down ringed in green."
                       : "Each chord box shows the strings from the low E (left) to the high E (right) and the frets from the top down. "
                         "The numbers are fingers: 1 index, 2 middle, 3 ring, 4 little. o: play the string open; x: don't play it. A "
                         "bar across several strings is a barre: the index finger presses them all. Under each box: the tab (fret per "
                         "string, low E first), how hard the shape is, and another way to play it. In a progression the fingers that "
                         "stay where they were are ringed in green.",
                 9.5f);
    p.y += 6.0f;

    if (! piano)
    {
        if (const auto capo = gen::sheet::findCapo (key); capo.fret > 0)
        {
            const auto shapes = gen::sheet::make (capo.shapesKey);
            juce::StringArray played;

            for (size_t i = 0; i < sheet.chords.size() && i < shapes.chords.size(); ++i)
                played.add (sheet.chords[i].name + " as " + shapes.chords[i].name);

            p.paragraph ("Easier with a capo: put it on fret " + juce::String (capo.fret) + " and play the shapes of " + shapes.keyName
                             + ", and " + juce::String (capo.easyWith) + " of the seven chords are easy shapes instead of "
                             + juce::String (capo.easyWithout) + ". Play " + played.joinIntoString (", ")
                             + ". The boxes below are drawn without a capo.",
                         9.5f, familyColour (guide::Family::away));
            p.y += 6.0f;
        }
    }

    const auto cellHeight = piano ? 104.0f : 168.0f;
    drawHeading (p, "The seven chords of " + sheet.keyName,
                 "One chord on every note of the scale. Home chords feel at rest, away chords move off, and tension chords pull back home. "
                 "Most songs in " + sheet.keyName + " use only these.", cellHeight);
    drawCells (p, instrument, sheet.chords);

    drawHeading (p, "With sevenths", "The same chords with the next note of the scale on top: richer, and the sound of jazz, soul and R&B.", cellHeight);
    drawCells (p, instrument, sheet.sevenths);

    if (! sheet.borrowed.empty())
    {
        drawHeading (p, "Often borrowed", "Chords from outside the key that songs in " + sheet.keyName + " often use anyway, for colour or a stronger pull home.",
                     cellHeight);
        drawCells (p, instrument, sheet.borrowed);
    }

    drawHeading (p, "Progressions in " + sheet.keyName, "Well-known chord sequences in this key, ready to play.", piano ? 98.0f : 142.0f);

    for (auto& row : sheet.progressions)
        drawProgression (p, instrument, row);

    return p.finish();
}

juce::String getTitle (const MusicalKey& key, bool piano, bool guitar)
{
    return "Chords in " + gen::sheet::getKeyName (key) + (piano && guitar ? " (piano and guitar)" : piano ? " (piano)" : " (guitar)");
}

juce::Result exportPdf (const MusicalKey& key, bool piano, bool guitar, const juce::File& file)
{
    std::vector<juce::Image> pages;

    for (auto [wanted, instrument] : { std::pair { piano, Instrument::piano }, std::pair { guitar, Instrument::guitar } })
    {
        if (! wanted)
            continue;

        auto more = renderPages (key, instrument);
        pages.insert (pages.end(), more.begin(), more.end());
    }

    return pdf::write (pages, file, getTitle (key, piano, guitar));
}

//==============================================================================
std::vector<juce::Image> renderSongPages (const SongChords& song, Instrument instrument, float pixelsPerPoint)
{
    const bool piano = instrument == Instrument::piano;
    const auto keyName = gen::sheet::getKeyName (song.key);
    Pages p (pixelsPerPoint, song.title + ": chords for " + getInstrumentName (instrument));

    // As on the chord track: a chord repeated on the following bars is one block, and the blocks are
    // voiced through the song, each near the one before.
    struct Block
    {
        int start = 0, end = 0;
        Entry entry;
    };

    std::vector<Block> blocks;

    for (auto it = song.chords.begin(); it != song.chords.end();)
    {
        Block block { it->first, it->first + 1, gen::sheet::describe (song.key, it->second) };

        for (++it; it != song.chords.end() && it->first == block.end && it->second == block.entry.chord; ++it)
            ++block.end;

        blocks.push_back (block);
    }

    std::vector<Chord> progression;

    for (auto& b : blocks)
        progression.push_back (b.entry.chord);

    const auto voicings = diagrams::voiceProgression (progression);
    const auto shapes = diagrams::shapeProgression (progression);
    int endBar = blocks.empty() ? 0 : blocks.back().end, low = 127, high = 0;

    for (auto& section : song.sections)
        endBar = juce::jmax (endBar, section.getEndBar());

    for (size_t i = 0; i < blocks.size(); ++i)
    {
        low = juce::jmin (low, diagrams::pianoBass (progression[i]));

        for (int n : voicings[i])
            high = juce::jmax (high, n);
    }

    // The title, what the song is in, and how to read the sheet.
    drawText (p.g(), song.title, { Pages::left, p.y, Pages::right - Pages::left, 32.0f }, 26.0f, true);
    p.y += 34.0f;
    drawText (p.g(), "Chords for " + getInstrumentName (instrument) + "   \xc2\xb7   " + keyName + "   \xc2\xb7   " + fx::formatNumber (song.bpm, juce::approximatelyEqual (song.bpm, std::round (song.bpm)) ? 0 : 1)
                         + " BPM   \xc2\xb7   " + juce::String (endBar) + " bars",
              { Pages::left, p.y, Pages::right - Pages::left, 14.0f }, 10.5f, false, inkDim);
    p.y += 22.0f;

    p.paragraph ((piano ? "Each keyboard shows the right hand's notes in blue and the left hand's bass note, the root, in orange; a filled dot "
                          "marks the root. "
                        : "Each chord box shows the strings from the low E (left) to the high E (right); the numbers are fingers (1 index, "
                          "2 middle, 3 ring, 4 little), o an open string, x one not played, and a bar a barre. ")
                     + juce::String ("Bar by bar, every chord is played near the one before, as on DawIt's chord track, with the ")
                     + (piano ? "keys" : "fingers") + " that stay where they were ringed in green. % means the chord goes on; N.C. means no chord.",
                 9.5f);
    p.y += 6.0f;

    // The chords it uses, each once, in the order they come.
    std::vector<Entry> used;

    for (auto& b : blocks)
        if (std::none_of (used.begin(), used.end(), [&b] (const Entry& e) { return e.chord == b.entry.chord; }))
            used.push_back (b.entry);

    drawHeading (p, "The chords in this song", juce::String ((int) used.size()) + (used.size() == 1 ? " chord" : " chords")
                     + ". The numerals and colours say what each one does in " + keyName + ": home chords feel at rest, away chords move off, "
                     "and tension chords pull back home.", piano ? 104.0f : 168.0f);
    drawCells (p, instrument, used);

    // Bar by bar, four to a line, a new line at every section.
    const float diagramHeight = piano ? 48.0f : 92.0f, rowHeight = 16.0f + diagramHeight + 10.0f, gap = 8.0f;
    const float cellWidth = (Pages::right - Pages::left - gap * 3.0f) / 4.0f;
    drawHeading (p, "Bar by bar", {}, rowHeight + 20.0f);

    KeyboardDiagram keyboard;
    keyboard.setPrinted (true);
    keyboard.setShowNames (false);
    keyboard.setRange (low, high);
    GuitarDiagram box;
    box.setPrinted (true);
    box.setCompact (true);

    auto blockAt = [&blocks] (int bar) -> int
    {
        for (size_t i = 0; i < blocks.size(); ++i)
            if (bar >= blocks[i].start && bar < blocks[i].end)
                return (int) i;

        return -1;
    };

    int column = 0;

    for (int bar = 0; bar < endBar; ++bar)
    {
        const auto section = std::find_if (song.sections.begin(), song.sections.end(), [bar] (const Section& s) { return s.startBar == bar; });

        if (section != song.sections.end() || column == 4)
        {
            if (column > 0)
                p.y += rowHeight;

            column = 0;
        }

        if (section != song.sections.end())
        {
            p.need (20.0f + rowHeight);
            drawText (p.g(), section->getDisplayName(), { Pages::left, p.y, 200.0f, 16.0f }, 11.5f, true);
            drawText (p.g(), "bars " + juce::String (section->startBar + 1) + "-" + juce::String (section->getEndBar()),
                      { Pages::left, p.y, Pages::right - Pages::left, 16.0f }, 9.0f, false, inkDim, juce::Justification::centredRight);
            p.y += 20.0f;
        }

        if (column == 0)
            p.need (rowHeight);

        auto cell = juce::Rectangle<float> (Pages::left + (cellWidth + gap) * (float) column, p.y, cellWidth, rowHeight - 10.0f);
        p.g().setColour (rule);
        p.g().fillRect (cell.getX() - gap * 0.5f, cell.getY(), 0.8f, cell.getHeight());   // the bar line

        if (column == 3 || bar == endBar - 1)
            p.g().fillRect (cell.getRight() + gap * 0.5f, cell.getY(), 0.8f, cell.getHeight());

        auto label = cell.removeFromTop (16.0f);
        drawText (p.g(), juce::String (bar + 1), label.removeFromLeft (22.0f).withTrimmedLeft (2.0f), 8.0f, false, inkDim);

        if (const int b = blockAt (bar); b < 0)
        {
            drawText (p.g(), "N.C.", cell, 11.0f, false, inkDim, juce::Justification::centred);
        }
        else if (bar == blocks[(size_t) b].start)
        {
            const auto& block = blocks[(size_t) b];
            drawText (p.g(), block.entry.name, label, 11.5f, true, ink);

            if (piano)   // the notes the right hand plays, after the name
                drawText (p.g(), spellNotes (block.entry, voicings[(size_t) b]),
                          label.withTrimmedLeft (juce::GlyphArrangement::getStringWidth (juce::FontOptions (11.5f, juce::Font::bold), block.entry.name) + 6.0f),
                          8.5f, false, inkDim);
            drawText (p.g(), block.entry.numeral, label, 9.0f, true, familyColour (block.entry.family), juce::Justification::centredRight);

            if (piano)
            {
                std::vector<int> held;

                if (b > 0)
                    for (int note : voicings[(size_t) b])
                        if (std::find (voicings[(size_t) b - 1].begin(), voicings[(size_t) b - 1].end(), note) != voicings[(size_t) b - 1].end())
                            held.push_back (note);

                keyboard.setChord (voicings[(size_t) b], diagrams::pianoBass (block.entry.chord), block.entry.chord.root, held);
                paintDiagram (p.g(), keyboard, cell.withTrimmedRight (4.0f));
            }
            else
            {
                box.setShape (shapes[(size_t) b], b > 0 ? &shapes[(size_t) b - 1] : nullptr);
                paintDiagram (p.g(), box, cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 92.0f), cell.getHeight()));
            }
        }
        else
        {
            drawText (p.g(), blocks[(size_t) b].entry.name, label, 11.5f, false, inkDim);
            drawText (p.g(), "%", cell, 26.0f, false, rule.darker (0.3f), juce::Justification::centred);
        }

        ++column;
    }

    if (column > 0)
        p.y += rowHeight;

    return p.finish();
}

juce::Result exportSongPdf (const SongChords& song, Instrument instrument, const juce::File& file)
{
    return pdf::write (renderSongPages (song, instrument), file, song.title + ": chords for " + getInstrumentName (instrument));
}

//==============================================================================
namespace
{
    std::unique_ptr<juce::FileChooser> chooser;

    /** The key and the instrument, then where to save. */
    class SheetOptions final : public juce::Component
    {
    public:
        SheetOptions (Engine& e, const MusicalKey& key) : engine (e)
        {
            for (int pc = 0; pc < 12; ++pc)
                tonicBox.addItem (MusicalKey::getTonicName (pc), pc + 1);

            for (int s = 0; s < MusicalKey::numScales; ++s)
                scaleBox.addItem (MusicalKey::getScaleName ((MusicalKey::Scale) s), s + 1);

            tonicBox.setSelectedId (key.tonic + 1, juce::dontSendNotification);
            scaleBox.setSelectedId ((int) key.scale + 1, juce::dontSendNotification);

            instrumentBox.addItem ("Piano: keyboards with the inversions", pianoId);
            instrumentBox.addItem ("Guitar: chord boxes with fingers and a capo tip", guitarId);
            instrumentBox.addItem ("Piano and guitar, in one file", bothId);
            instrumentBox.setSelectedId (engine.properties.getUserSettings()->getIntValue ("export.sheetInstrument", bothId), juce::dontSendNotification);

            for (auto [label, text] : { std::pair { &keyLabel, "Key" }, std::pair { &instrumentLabel, "For" } })
            {
                label->setText (text, juce::dontSendNotification);
                label->setColour (juce::Label::textColourId, theme::textDim);
                addAndMakeVisible (label);
            }

            hint.setText ("A4 pages to print or keep on a tablet: the key's seven chords and their sevenths with their numerals and "
                          "families, the chords songs in it often borrow, and its well-known progressions, each one drawn to play.",
                          juce::dontSendNotification);
            hint.setFont (juce::FontOptions (12.5f));
            hint.setColour (juce::Label::textColourId, theme::textDim);
            hint.setJustificationType (juce::Justification::topLeft);

            for (juce::Component* c : { (juce::Component*) &tonicBox, (juce::Component*) &scaleBox, (juce::Component*) &instrumentBox,
                                        (juce::Component*) &hint, (juce::Component*) &exportButton, (juce::Component*) &cancelButton })
                addAndMakeVisible (c);

            exportButton.onClick = [this] { chooseFile(); };
            cancelButton.onClick = [this] { close(); };
            setSize (480, 176);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);
            auto row = r.removeFromTop (28);
            keyLabel.setBounds (row.removeFromLeft (60));
            tonicBox.setBounds (row.removeFromLeft (80));
            row.removeFromLeft (8);
            scaleBox.setBounds (row.removeFromLeft (170));
            r.removeFromTop (8);
            row = r.removeFromTop (28);
            instrumentLabel.setBounds (row.removeFromLeft (60));
            instrumentBox.setBounds (row);
            r.removeFromTop (10);
            auto buttons = r.removeFromBottom (28);
            cancelButton.setBounds (buttons.removeFromRight (90));
            buttons.removeFromRight (8);
            exportButton.setBounds (buttons.removeFromRight (110));
            hint.setBounds (r);
        }

    private:
        enum { pianoId = 1, guitarId = 2, bothId = 3 };

        void close()
        {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);
        }

        void chooseFile()
        {
            MusicalKey key;
            key.tonic = tonicBox.getSelectedId() - 1;
            key.scale = (MusicalKey::Scale) (scaleBox.getSelectedId() - 1);
            const int instrument = instrumentBox.getSelectedId();
            const bool piano = instrument != guitarId, guitar = instrument != pianoId;

            auto& props = *engine.properties.getUserSettings();
            props.setValue ("export.sheetInstrument", instrument);
            const juce::File folder (props.getValue ("export.sheetFolder", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getFullPathName()));
            const auto fileName = juce::File::createLegalFileName (chordsheet::getTitle (key, piano, guitar)) + ".pdf";
            auto& e = engine;
            close();   // deletes this

            chooser = std::make_unique<juce::FileChooser> ("Export chord sheet", folder.getChildFile (fileName), "*.pdf");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [&e, key, piano, guitar] (const juce::FileChooser& fc)
                                  {
                                      auto target = fc.getResult();

                                      if (target == juce::File())
                                          return;

                                      target = target.withFileExtension (".pdf");
                                      auto& p = *e.properties.getUserSettings();
                                      p.setValue ("export.sheetFolder", target.getParentDirectory().getFullPathName());
                                      p.saveIfNeeded();

                                      if (const auto result = exportPdf (key, piano, guitar, target); result.failed())
                                          juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", result.getErrorMessage());
                                  });
        }

        Engine& engine;
        juce::Label keyLabel, instrumentLabel, hint;
        juce::ComboBox tonicBox, scaleBox, instrumentBox;
        juce::TextButton exportButton { "Export..." }, cancelButton { "Cancel" };
    };
}

void exportSong (Engine& engine, const juce::String& projectName, Instrument instrument)
{
    if (engine.getChords().empty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "No chords yet",
                                                "The chord track is empty. Put chords on it first, then export them.");
        return;
    }

    const SongChords song { projectName, engine.getKey(), engine.getTempo(), engine.getChords(), engine.getSections() };
    auto& props = *engine.properties.getUserSettings();
    const juce::File folder (props.getValue ("export.sheetFolder", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getFullPathName()));
    const auto fileName = juce::File::createLegalFileName (projectName + " chords (" + getInstrumentName (instrument) + ")") + ".pdf";

    chooser = std::make_unique<juce::FileChooser> ("Export the song's chords", folder.getChildFile (fileName), "*.pdf");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [&engine, song, instrument] (const juce::FileChooser& fc)
                          {
                              auto target = fc.getResult();

                              if (target == juce::File())
                                  return;

                              target = target.withFileExtension (".pdf");
                              auto& p = *engine.properties.getUserSettings();
                              p.setValue ("export.sheetFolder", target.getParentDirectory().getFullPathName());
                              p.saveIfNeeded();

                              if (const auto result = exportSongPdf (song, instrument, target); result.failed())
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", result.getErrorMessage());
                          });
}

void showDialog (Engine& engine, const MusicalKey& key)
{
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new SheetOptions (engine, key));
    options.dialogTitle = "Export chord sheet (PDF)";
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}
} // namespace daw::chordsheet
