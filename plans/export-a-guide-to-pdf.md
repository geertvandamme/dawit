# Export a guide to PDF

Status: Built, not committed
Idea: [Export a guide to PDF](../FEATURE_IDEAS.html#export-a-guide-to-pdf)

## Goal

*Export PDF...* (Cmd+P) in any guide window saves that guide as a booklet. The booklet has a cover with the contents, then a page per topic with its picture and its text, exactly as the guide shows them now. A "your song" page shows the user's own song, and an effect's guide shows the settings in its window. The Theory Guides window (and *File > Export Theory Guides (PDF)...*) does the same for a set: *Start here*, an area, every guide, or the guides a search found. A beginner can read the theory away from the screen, print it for a lesson, or give a class the *Start here* path on paper. The pictures are made light for paper, with the colours that carry meaning kept.

## Decisions for you

1. **Scope of this plan.** Options: (a) only *Export PDF...* in each guide window; (b) also sets of guides from the Theory Guides window and the File menu: a path, an area, all guides, or a search's results. **Recommended:** (b). The idea names both, and a teacher wants the *Start here* path as one booklet. Sets add one step (step 7: making a guide that isn't open, and a set cover). The export job, the layout and the dialog are shared with (a).
   Answer: (b) include sets (recommended).
2. **Pictures on paper.** The pictures are drawn in the app's dark theme. Options: (a) print them as they are, as dark cards on white pages (uses a lot of ink); (b) map every pixel to paper colours: the dark backgrounds become white, the light text becomes near-black, and colours keep their hue, darkened enough to read on white (see *Theory and numbers*), with *As on screen* still offered in the dialog; (c) a real print palette that every picture draws with, which means making `theme::` switchable across about 930 uses. **Recommended:** (b), with *Light, for printing* as the default. It works for all 19 guides at once and can be tested. (c) is better but much bigger, and belongs with a light app theme later.
   Answer: (b) light paper colours, with *As on screen* still in the dialog (recommended).
3. **Text as image or real PDF text.** Options: (a) the text is part of each page's picture at 300 dpi, through the existing `PdfWriter`; (b) real PDF text: searchable, selectable, sharper and much smaller, but it needs an embedded font (IBM Plex Sans, OFL, as on the website), a font writer and the same text measuring on both sides. That is about as big as the rest of this plan. **Recommended:** (a) now and (b) later, as the idea itself suggests. To keep files smaller, `PdfWriter` gets PNG predictors (step 1).
   Answer: (a) text as part of the page image for now (recommended).
4. **Resolution.** Options: 300 dpi (the idea, the usual print resolution); 200 dpi (as the chord sheets use, about 45% of the pixels); or a choice in the dialog. **Recommended:** 300 dpi for everything. Step 9 measures the file sizes: if *All guides* comes out above about 150 MB, sets drop to 200 dpi and single guides stay at 300.
   Answer: 300 dpi for everything, sets at 200 dpi if *All guides* measures above about 150 MB (recommended).
5. **Default paper size.** Options: always A4 until changed; or Letter in the countries that use it (from the system's region, by Unicode CLDR's list) and A4 elsewhere. In both cases the last choice is remembered. **Recommended:** follow the region, so a first export in the US or Canada is already right.
   Answer: Letter where the system region uses it, A4 elsewhere; the last choice is remembered (recommended).
6. **"Piano and guitar chords" in a set.** That guide has no topic list: it is the chord guide showing one chord. Options: (a) give it a page per chord of the song's chord track (each distinct chord once, in order, or the key's seven chords when the track is empty), each with the chord's piano and guitar diagrams and how to play it; (b) leave it out of sets and point to *Chord sheet (PDF)...*. **Recommended:** (a). It's the "your song" version of that guide, and it reuses `ChordExplainer::showChord`.
   Answer: (a) a page per chord of the song's chord track (recommended).
7. **The button's name and shortcut.** Options: *Export PDF...* with Cmd+P (the usual print shortcut, free in DawIt); or *Print...*, which promises a print dialog DawIt can't show (JUCE has no printing API). **Recommended:** *Export PDF...* and Cmd+P. The tooltip says that it saves a PDF to print.
   Answer: *Export PDF...* with Cmd+P (recommended).

## What the user does and sees

### In a guide window

Every guide window gets an **Export PDF...** button at the right end of its title row. That covers the 7 explainer windows (*What is sound?*, *What is MIDI?*, *Note lengths, beats and bars*, *Chord progressions*, *Song structure*, *How lyrics are written*, *Gain staging*) and the 11 "What is ...?" windows of the built-in effects and synths, which share `fxui::Guide`. Cmd+P anywhere in the window does the same.

```
+-------------------------------------------------------------------------------+
| What is reverb?          The pictures follow the settings in ...  [Export PDF...] |
| +-----------+ +---------------------------------------------------------------+ |
| | topics    | |  picture                                                      | |
```

- Tooltip: "Save this guide as a PDF to print or read on a tablet: a cover with the contents, then a page per topic with its picture and text as they show now (Cmd+P)".
- In the chord guide, the title row already holds the key controls. The button sits between the title and *Key*. Its tooltip tells it apart from *Chord sheet (PDF)...*: "Save this guide as a PDF... (the chord sheet button saves a key's chords to play from instead)".
- The button doesn't take keyboard focus (`setWantsKeyboardFocus (false)`, `setMouseClickGrabsKeyboardFocus (false)`), so the topic list keeps the arrow keys.

### The dialog

A small dialog, like *Export chord sheet (PDF)*, titled "Export guide (PDF)" or, for a set, "Export guides (PDF)":

```
 Guides    [ Start here: 7 guides                         v ]   (sets only)
 Paper     [ A4 (210 x 297 mm)                            v ]
 Pictures  [ Light, for printing                          v ]
 A cover with the contents, then a page per topic: its picture and text as the
 guide shows them now, so "Your lyrics" shows your song. 11 topics.
                                                   [ Export... ] [ Cancel ]
```

- **Guides** (sets only): *Start here (7 guides)*, then each area (*Sound (1 guide)* ... *Synthesis (5 guides)*), *All guides (19 guides)*, and, when a search is shown, *The 3 guides your search found*. It starts on what the Theory Guides window shows, or on *Start here* from the File menu. Tooltip: "Which guides go in the booklet, in the order the Theory Guides window lists them".
- **Paper:** *A4 (210 x 297 mm)* or *Letter (8.5 x 11 in)*. Tooltip: "A4 is used almost everywhere; Letter in the US, Canada and parts of Latin America".
- **Pictures:** *Light, for printing* or *As on screen (dark)*. Tooltip: "Light turns the dark backgrounds white and keeps the colours that mean something, to save ink and read well on paper. As on screen keeps the pictures as the window shows them".
- The hint line counts the topics (or the guides and topics for a set), and says that the pictures show the song as it is now.
- *Export...* opens a save dialog. The file name is the guide's name, made legal ("What is reverb.pdf"), or the set's name ("DawIt guides - Start here.pdf"), in the last folder used.
- Then a progress window, "Exporting What is reverb?...", with a bar that moves page by page and a *Cancel* button. While it runs, the guide window turns through its topics: it is being photographed. Afterwards it goes back to the topic it showed.
- When it's done: "Saved 15 pages (6.2 MB) to What is reverb.pdf." with *Show in Finder* and *OK*, as *Bounce to WAV* does. If the guide window is closed during the export, it stops: "The guide was closed before its pages were all taken. Nothing was saved."

### In the Theory Guides window

- An **Export PDF...** button sits left of the search field. It opens the dialog for whatever the window shows. Tooltip: "Save the guides shown here as one PDF booklet: Start here's seven guides, an area's guides, all of them, or the ones your search found (Cmd+P)". It is disabled when a search found nothing.
- **Right-click on a guide's card:** a menu with *Export "What is EQ?" as PDF...*, a separator, then *Open "What is EQ?"* as the last item (the guide, as everywhere).
- Cmd+P in the window does what the button does.

### In the File menu

*File > Export Theory Guides (PDF)...*, right under *Export Chord Sheet (PDF)...*, opens the set dialog on *Start here*. It has no shortcut, because Cmd+P belongs to the guide windows.

### The booklet

A4 or Letter, portrait, 40 pt margins, dark text on white:

- **Cover of a single guide (page 1):** a stripe in the guide's area colour across the top; "DawIt guide · Effects" (9 pt, dim); the guide's name (30 pt bold); its summary from `gen::guides` (13 pt); "Pictures from: DawIt Reverb on Vocals, as set in its window" and "Song: My Song · A minor · 96 BPM · 26 October 2026" (10 pt). Then **In this guide**: one line per topic, with the name in bold, the summary dim after it, and the page number on the right. Each line is a link to its page. When the topics don't fit, the contents go on to a second page ("Contents, continued").
- **Cover of a set:** the set's name ("Start here"), its line ("Seven guides in an order that builds up..."), the song line, then **In this booklet**: one line per guide, with its area colour dot, name, area and page, each a link.
- **A guide's cover inside a set:** as the cover of a single guide, without the song line (it's on the set's cover).
- **Topic page:** a header line with the guide's name on the left (8.5 pt, dim) and the area on the right (in its colour), above a hairline. Then the topic's name (18 pt bold), its summary (11 pt, dim), the picture across the text width with rounded corners and its on-screen proportions (at most 45% of the page's height, then scaled down and centred), and the text at 10.5 pt in paragraphs. Text that doesn't fit goes on to the next page under "<Topic> (continued)".
- **Footer on every page:** a hairline, then "What is reverb? · My Song · made with DawIt" on the left and "Page 3 of 15" on the right, as on the chord sheets.
- **Reader sidebar:** the PDF has bookmarks. For a single guide there is one per topic. For a set there is one per guide, with its topics under it.

### Remembered between sessions (`engine.properties`)

- `export.guidePaper`: `"a4"` or `"letter"`. When it's missing, the default comes from the system's region.
- `export.guidePictures`: `"paper"` or `"screen"`, default `"paper"`.
- `export.guideFolder`: the folder last saved to, default the Documents folder.

## Data and saving

Nothing goes into the `.dawit` project. The booklet is made from the guides' windows as they are, and only the three app settings above are remembered. Older projects aren't affected, so there's no project test.

## Theory and numbers

There is no music theory in this feature. The numbers are about paper, PDF and colour:

- **Paper sizes** (ISO 216 for A4; US Letter is ANSI A). 1 pt = 1/72 in. A4 is 210 x 297 mm = **595.28 x 841.89 pt**. Letter is 8.5 x 11 in = **612 x 792 pt**. *Tested.*
- **300 dpi**, the usual resolution for printed images. A full page is **2480 x 3508 px** for A4 and **2550 x 3300 px** for Letter. At 36 dpi (used in the tests to keep them fast) A4 is **298 x 421 px** and Letter **306 x 396 px**. *Tested.*
- **Which countries use Letter:** Unicode CLDR's `supplementalData.xml` has `<paperSize type="US-Letter" territories="BZ CA CL CO CR GT MX NI PA PH PR SV US VE"/>`, and A4 for every other region ([CLDR supplemental data](https://github.com/unicode-org/cldr/blob/main/common/supplemental/supplementalData.xml)). `juce::SystemStats::getUserRegion()` gives the region code. *Tested:* US, CA, MX and PH give Letter; GB, BE, DE and an empty region give A4.
- **Contrast** (WCAG 2.x, [contrast ratio](https://www.w3.org/TR/WCAG21/#dfn-contrast-ratio)). The ratio is (L1 + 0.05) / (L2 + 0.05), from the relative luminance of linearised sRGB (0.2126 R + 0.7152 G + 0.0722 B). Normal text needs 4.5:1 (AA) or 7:1 (AAA). Graphics need 3:1 ([SC 1.4.11](https://www.w3.org/TR/WCAG21/#non-text-contrast)). These are the checks for the paper colours.
- **Paper colours (decision 2b).** They use OKLab, a colour space where equal steps in lightness look equal ([Björn Ottosson, *A perceptual color space for image processing*, 2020](https://bottosson.github.io/posts/oklab/)). In HSL, blue and yellow of the same "lightness" look very different. For each colour:
  1. Convert sRGB to OKLab: lightness L, and a and b. Chroma (how colourful it is) is C = sqrt(a² + b²).
  2. *Neutral lightness:* t = (L − L<sub>dark</sub>) / (L<sub>light</sub> − L<sub>dark</sub>), with L<sub>dark</sub> the OKLab L of `theme::background` (about 0.227) and L<sub>light</sub> that of `theme::text` (about 0.894). Then L<sub>n</sub> = clamp (1 − 0.85 t, 0, 1). So the background becomes white and the text becomes L 0.15.
  3. *How colourful:* w = smoothstep (0.06, 0.12, C). Greys (C below about 0.02) and dim tints (around 0.05) count as neutral. The accent (C 0.158) and the area colours count as colours.
  4. L' = (1 − w) L<sub>n</sub> + w · min (L, 0.62). Bright colours are darkened to L 0.62 at most, so they read on white, and they keep their a and b (hue and chroma).
  5. If (L', a, b) falls outside sRGB, scale a and b down in 1% steps until it fits. Alpha is kept.

  This was worked out for the plan with the app's colours. `theme::background` becomes white. `panelLight` becomes #d4d8e1 (L 0.88). `theme::text` becomes #14161b, at **13.8:1** on the mapped `panelLight`. `textDim` reaches **4.9:1** (on screen it is 3.9:1). Every area colour and the meter colours keep their hue within **6°**, at **3.4:1 or more** against white (meter green #1d9f5d, the lowest). The accent at 35% over `panelLight`, a typical selection tint, becomes the light blue #7fa0c9 (L about 0.69) instead of a dark block. *Tested,* with margins (see *Tests*).
- **PDF image compression** (ISO 32000-1:2008, 7.4.4.4, *LZW and Flate predictor functions*, Table 8; [PDF 32000-1:2008](https://opensource.adobe.com/dc-acrobat-sdk-docs/pdfstandards/PDF32000_2008.pdf)). `/DecodeParms << /Predictor 15 /Colors 3 /BitsPerComponent 8 /Columns W >>` means that each row starts with a byte naming its PNG filter: 0 None, 1 Sub, 2 Up, 3 Average, 4 Paeth ([PNG specification, 9 Filtering](https://www.w3.org/TR/png/#9Filters)). Each row takes the filter whose output has the smallest sum of absolute values, read as signed bytes; that is the PNG specification's suggested heuristic. Average is left out. Smooth areas and text on white then compress far better. *Tested:* the pixels come back exactly, and a gradient's stream is smaller than without the predictor.
- **PDF outline and links** (ISO 32000-1, 12.3.3 *Document outline*, 12.5.6.5 *Link annotations*). These are the sidebar bookmarks and the clickable contents. Link rectangles are in points from the bottom left, so a line at (40, 100) of size 200 x 20 from the top left of an A4 page becomes `/Rect [40 721.89 240 741.89]`. *Tested.*

## The guide

- **No new guide and no new topics.** The feature has no theory of its own to explain. It is explained where it's used:
  - the tooltips above, on the buttons, the card menu and every control of the dialog;
  - the dialog's hint, which says what the booklet holds and that the pictures show the song as it is now;
  - the cover's "Pictures from" and "Song" lines, which say which song and settings the pictures come from;
  - the Theory Guides window's intro line, which gets a second sentence: "Export PDF... saves the guides shown as a booklet to print."
- **`gen::guides`:** no new entry, so nothing changes on *Start here*. The entries' area, name and summary are used for the covers. `TheoryGuidesView::getColour (Area)` gives the area colours, through `booklet::toPaper` when the pictures are light.
- **Every guide's "your song" topic** goes into the booklet as it shows now: *Your lyrics*, *Your chord track*, *Your song's structure*, MIDI's *Your clip*, the sound guide's clip in the wave editor, and gain staging's chain of the selected track.

## Code

### New: `src/engine/GuideBooklet.{h,cpp}`, namespace `daw::booklet` (in both targets, no UI)

Laying out and drawing the booklet's pages from plain data, plus the paper colours. It is pure enough to test headless.

```cpp
namespace daw::booklet
{
enum class Paper { a4, letter };
juce::Point<float> getSize (Paper);                 // points: 595.28 x 841.89, 612 x 792
juce::String getName (Paper);                       // "A4 (210 x 297 mm)", "Letter (8.5 x 11 in)"
Paper getDefaultPaper (const juce::String& region); // Letter in CLDR's US-Letter regions, else A4
juce::Point<int> getPixelSize (Paper, float dpi);   // A4 at 300: 2480 x 3508
constexpr float printDpi = 300.0f;

/** Paper colours: see "Theory and numbers". */
juce::Colour toPaper (juce::Colour);
void toPaper (juce::Image&);                        // every pixel, memoised per distinct colour
float getContrastRatio (juce::Colour, juce::Colour);// WCAG 2.x
float getOkLightness (juce::Colour);                // OKLab L, 0 to 1

/** A topic as the guide shows it. The picture may stay empty until its pages are drawn. */
struct Topic
{
    juce::String name, summary, text;               // text: paragraphs separated by blank lines
    juce::Point<int> pictureSize;                   // on screen, in pixels; {0, 0}: no picture
    juce::Image picture;                            // at print resolution, filled just before drawing
};

struct Guide
{
    juce::String name, summary, area, source;       // source: "DawIt Reverb on Vocals, as set in its window"
    juce::Colour colour;                            // its area's colour, as on screen
    std::vector<Topic> topics;
};

struct Booklet
{
    juce::String title, summary;                    // a set's name and line; a single guide uses its own
    juce::String songLine, date;                    // "My Song · A minor · 96 BPM", "26 October 2026"
    std::vector<Guide> guides;                      // one: a guide's booklet; more: a set
};

struct Options { Paper paper = Paper::a4; float dpi = printDpi; bool paperColours = true; };

/** What goes on every page, worked out from the text alone, before any picture is taken. */
struct PageInfo
{
    enum class Kind { cover, contents, guideCover, topic };
    Kind kind = Kind::cover;
    int guide = -1, topic = -1;
    juce::Range<int> text;                          // a topic page: the characters of the topic's text on it
    bool continued = false;
    juce::Rectangle<float> pictureArea;             // points from the top left; empty when none
    std::vector<pdf::Link> links;                   // the contents lines on it
};

struct Plan
{
    std::vector<PageInfo> pages;
    std::vector<int> guidePages;                    // a set: the page each guide's cover is on
    std::vector<std::vector<int>> topicPages;       // [guide][topic]: the page the topic starts on
    std::vector<pdf::Bookmark> bookmarks;
};

Plan makePlan (const Booklet&, const Options&);
juce::Image renderPage (const Booklet&, const Plan&, int page, const Options&);   // juce::SoftwareImageType
juce::Result write (const Booklet&, const Options&, juce::OutputStream&);           // everything at once (tests)
}
```

- `makePlan` measures with `juce::TextLayout` at the text width (page width minus 80 pt) and fills the pages top-down, as `chordsheet`'s `Pages` does. A paragraph that doesn't fit is split between lines (`TextLayout::Line::stringRange`). The covers' contents lines are a fixed 24 pt tall, so their number of pages is known first. The topic pages are then numbered after them.
- `renderPage` draws in points through `AffineTransform::scale (dpi / 72)`. On topic pages it draws the topic's `picture` into `pictureArea`, through `toPaper` on a copy when `paperColours` is set, and keeps its proportions. It also draws the footer with "Page n of N".
- Fonts: `juce::FontOptions` with the default sans, as the chord sheets use. Ink colours come from the chord sheet: ink #1d1e22, dim #6b6f78, rule #d5d8de. Move these three into `booklet` as `booklet::ink`, `inkDim` and `rule`, and use them from `ChordSheetExport.cpp` too.

### Changed: `src/engine/PdfWriter.{h,cpp}`

- `constexpr float letterWidth = 612.0f, letterHeight = 792.0f;`
- `struct Link { juce::Rectangle<float> area; int targetPage; };`: points from the page's top left.
- `struct Bookmark { juce::String title; int page = 0; int level = 0; };`: level 1 sits under the level-0 bookmark before it.
- `class Document final`: `explicit Document (const juce::String& title = {})`, `void addPage (const juce::Image&, float pageWidth, float pageHeight, std::vector<Link> = {})`, `void setBookmarks (std::vector<Bookmark>)`, `int getNumPages() const`, and `juce::Result write (juce::OutputStream&) const` / `write (const juce::File&) const` (through `juce::TemporaryFile`, as now). `addPage` compresses the image at once (PNG predictor rows, `GZIPCompressorOutputStream` at level 6), so a 200-page booklet holds only compressed pages. Object numbers are given out in `write`. The catalog gets `/Outlines` only when there are bookmarks. Each link is its own object, listed in the page's `/Annots`, with `/Border [0 0 0]` and `/Dest [<page> 0 R /Fit]`.
- The existing `write (pages, ...)` functions become wrappers around `Document`, so the chord sheets don't change, apart from smaller files.

### New: `src/ui/GuidePrint.{h,cpp}`, namespace `daw::guideprint`

What every guide window gives for its PDF, the shared button, the dialog and the export job.

```cpp
namespace daw::guideprint
{
/** A guide window that can be saved as a booklet: it shows its topics one by one, and says what its pictures show. */
class Source
{
public:
    virtual ~Source() = default;
    virtual juce::String getPrintTitle() const = 0;   // "What is reverb?"
    virtual juce::String getPrintSource() const = 0;  // "the clip Vocals in the wave editor"
    virtual int getNumPrintTopics() const = 0;
    /** Shows topic `index` as a click in its list would (nothing plays). Returns its name, summary, text and
        the picture's size on screen; the picture itself stays empty. */
    virtual booklet::Topic showPrintTopic (int index) = 0;
    /** The picture of the topic shown, `scale` times its size on screen. */
    virtual juce::Image paintPrintPicture (float scale) = 0;
    /** Back to what it showed before the export. */
    virtual void endPrinting() = 0;
};

/** A guide to export: its window's content, or one made for the export and deleted after it. */
struct Item
{
    gen::guides::Guide guide;
    juce::Component::SafePointer<juce::Component> component;   // also the Source
    Source* source = nullptr;
    std::unique_ptr<juce::Component> owned;
    std::unique_ptr<juce::AudioPluginInstance> plugin;         // a fresh built-in for an effect's guide
};

/** What the export needs from the app, set by MainComponent while it exists. */
struct App
{
    Engine* engine = nullptr;
    std::function<juce::String()> getProjectName;
    std::function<Item (gen::guides::Guide)> getGuide;          // the open guide, or one made for the export
};
void setApp (App);

/** "Export PDF...": opens the dialog for its Source. On its window it also handles Cmd+P: as a
    juce::KeyListener on the top-level component, added when the button is put in a window and removed with it. */
class ExportButton final : public juce::TextButton, private juce::KeyListener { ... };

void exportGuide (Source&, gen::guides::Guide);                    // the dialog for one guide
void exportGuides (std::vector<gen::guides::Guide> shown, const juce::String& shownName); // the set dialog
bool isExportKey (const juce::KeyPress&);                          // Cmd+P

/** No dialogs: exports the items to a file and calls back with the result (the developer flag uses it). */
void exportToFile (std::vector<Item>, const juce::String& title, const juce::String& summary,
                   const booklet::Options&, const juce::File&, std::function<void (juce::Result, int pages)> done);
}
```

- **The job** (`class ExportJob final : public juce::ThreadWithProgressWindow`, deleting itself like `BounceJob`):
  1. On the message thread, before the thread starts, it collects every guide's text: `showPrintTopic (i)` for each topic, then `endPrinting()`. It also builds the `booklet::Booklet`, with the names, summaries, area colours, sources, the song line (project name, `engine.getKey().getName()`, `engine.getTempo()` rounded, and the date as `"%e %B %Y"`).
  2. On the worker, `booklet::makePlan`.
  3. On the worker, page by page: when a page belongs to a guide whose pictures aren't taken yet, it asks the message thread (`juce::MessageManager::callAsync` plus a `juce::WaitableEvent`, waiting in 100 ms steps while checking `threadShouldExit()`) to take that guide's pictures. That means `showPrintTopic (i)` then `paintPrintPicture (scale)` for each topic, with scale = picture width in pixels at the dpi divided by the width on screen. Then `endPrinting()`. Before every call, the item's `SafePointer` is checked. The previous guide's pictures are freed. Then `renderPage` and `Document::addPage`, with `setProgress`.
  4. `Document::write` to the chosen file. In `threadComplete` (message thread), the items are destroyed, including any components and plugins made for the export, and the result is reported.
- **The dialog** (`ExportOptions`, laid out like `chordsheet`'s `SheetOptions`): reads and writes the three settings, then opens the `juce::FileChooser` with `warnAboutOverwriting`.

### Changed: the guide windows (each becomes a `guideprint::Source` and gets an `ExportButton`)

- **`src/ui/EffectWindow.{h,cpp}`, `fxui::Guide`** (11 guides at once):
  - `Page` gets `double printSeconds = 1.0;`. This is the moment after the clap that a moving picture is printed at. `printSeconds` is under 30, so the moving pictures' `since < 30` path draws a fixed moment rather than the clock-driven loop.
  - `setPrintSource (juce::String)`.
  - `showPrintTopic` calls `showPage (i)`. `paintPrintPicture` paints `pages[i].paint` into an ARGB image at the scale, over the `panelLight` rounded card, inside `reduced (16, 12)` as `Picture::paint` does, with `printSeconds`. `endPrinting` goes back to the page shown before.
  - The button goes at the right of the header row, and the hint label shrinks.
  - `EffectEditor::openGuide`, `BuiltInEqEditor::openGuide` and `BuiltInCompressorEditor::openGuide` set the print source to "the settings in " + the editor's window name ("Vocals - DawIt Reverb").
- **`SoundExplainer`, `MidiExplainer`, `RhythmExplainer`, `GainStagingExplainer`, `StructureExplainer`, `LyricsExplainer`:**
  - `showPrintTopic` goes through their own `showTopic` or `showRow`, then returns the topic's name and summary (from the guide's `getTopicName` and `getTopicSummary`, or the list row's text for Structure), `explanation.getText()`, and the picture's size.
  - `paintPrintPicture` is `createComponentSnapshot (bounds, true, scale)` of the picture component: `picture`, `rowsView`, `strip`. For Lyrics, the picture is first set to its `getPreferredHeight()`, so nothing is cut off, and then laid out again.
  - Print sources:
    - Sound: "the clip Vocals in the wave editor, at bar 9", or "a made-up tone (no clip in the wave editor)".
    - MIDI: "the clip Lead in the piano roll", or "a made-up example".
    - Rhythm: "the song's tempo, 96 BPM".
    - Gain staging: "the chain of the track Bass", or "no track selected".
    - Structure: "the song's 8 sections".
    - Lyrics: "the song's lyrics, 24 lines".
- **`ChordExplainer`:**
  - Its topics are its list rows: introduction, circle of fifths, your chord track, playing along, then each well-known progression.
  - Its picture is made of parts. `paintPrintPicture` snapshots, top to bottom, with 10 px between them on `theme::panel`: the key's tiles with the legend (the window's area from the first key tile to the bottom of `legendArea`), the row's chord tiles when there are any, the diagram panel when it shows something, and the circle when it's visible. They are stacked into one image as wide as the widest part.
  - Print source: "A minor, with the song's chord track (16 bars)".
  - A second mode for decision 6 is turned on by `setPrintChordShapes (true)` on an instance made for a set. Its title is "Piano and guitar chords". Its topics are the song's distinct chords in order of first use (or the key's seven when the chord track is empty), named "How to play Am", with the summary "On piano and guitar". Each is shown through `showChord`, and its picture is the diagram panel's snapshot.
- **`TheoryGuidesWindow.{h,cpp}`:**
  - `TheoryGuidesView` gets an `ExportButton`-style button (not a `Source`). Its click calls `guideprint::exportGuides` with the guides shown: the path, an area, all, or the search's matches.
  - `Card::mouseUp` shows the right-click menu when `e.mods.isPopupMenu()`. *Export as PDF...* calls `guideprint::exportGuides ({ guide }, name)`.
  - Cmd+P goes through `keyPressed`.
  - `BuiltInGuides::makePages (juce::AudioProcessor&)` is taken out of `getPageNames`. `getPageNames` then uses it.
  - New `BuiltInGuides::getPrintGuide (const juce::String& identifier)` makes the item for an effect's or synth's guide: a new `fxui::Guide` on `find (identifier)` with the source "DawIt Reverb on Vocals, as set in its window". When the song has none, it uses a fresh `createBuiltIn (identifier)`, with `prepareToPlay (44100.0, 512)` called and owned by the item, and the source "a DawIt Reverb with its starting settings (none in the song)".

### Changed: making guides for a set

- **`MainComponent`:**
  - calls `guideprint::setApp` in its constructor and `setApp ({})` in its destructor;
  - `getGuide` returns the open window's component for sound, MIDI and rhythm (`soundGuide`, `midiGuide`, `rhythmGuide`), or a new one built as in `showSoundGuide` and the others (without `onRemoveVocals`). It hands the chord, chord shapes, structure and lyrics guides to `arrangement.getPrintGuide (guide)`, gain staging to `deviceBar.getPrintGuide()`, and the effects to `builtInGuides.getPrintGuide (entry.plugin)`;
  - new command `exportGuidesCommand`, *Export Theory Guides (PDF)...*, in `getAllCommands`, `getCommandInfo` ("Save guides as a booklet to print: Start here, an area or all of them"), `perform` and the File menu.
- **`ArrangementView::getPrintGuide (gen::guides::Guide)`:** the open `chordExplainer`, `structureExplainer` or `lyricsExplainer`, or a new one at `getPlayheadBar()`. For `chordShapes` it always makes a new `ChordExplainer` with `setPrintChordShapes (true)`.
- **`DeviceBar::getPrintGuide()`:** the open `guide`, or a new `GainStagingExplainer` with the same snapshot lambda as `showGuide`.

### Changed: `src/Main.cpp`, a developer flag

`--export-guides <folder>` runs after `--demo` or a project has loaded, 1.5 s in, like `--screenshot`. It exports every guide in `gen::guides::allGuides` to `<folder>/<name>.pdf`, then *Start here* to `<folder>/Start here.pdf`, one after another through `guideprint::exportToFile`. For each it prints a line:

```
What is reverb?: 14 topics, 15 pages, 6.2 MB, 4.1 s
```

Then it quits. The return value is 1 (`setApplicationReturnValue`) if any export failed, if any topic didn't start its own page, or if a guide whose `gen::guides` entry lists its own topics produced different topic names. `moreThanOneInstanceAllowed()` also allows it. The real-app check uses it, and dawWebsite can use it to make booklets to offer for download.

### Threading

- The edit isn't changed.
- The guide windows (the Sources) are only touched on the message thread: collecting the text before the job, and taking the pictures through `callAsync` while it runs. They already read the song there.
- The plan, the page drawing (`SoftwareImageType`), the paper colours, the compression and the file writing happen on the job's thread.
- Cancelling sets `threadShouldExit`. The job stops between pages and nothing is written (`TemporaryFile`).

### Files

- **New:** `src/engine/GuideBooklet.{h,cpp}` and `src/ui/GuidePrint.{h,cpp}`.
- **Changed:**
  - `src/engine/PdfWriter.{h,cpp}`, `src/ui/ChordSheetExport.cpp` (the shared ink colours)
  - `src/ui/EffectWindow.{h,cpp}`, `EqEditor.cpp`, `CompressorEditor.cpp`
  - the 7 explainers
  - `TheoryGuidesWindow.{h,cpp}`, `MainComponent.{h,cpp}`, `ArrangementView.{h,cpp}`, `DeviceBar.{h,cpp}`
  - `src/Main.cpp`, `CMakeLists.txt`: `GuideBooklet.cpp` in `DawIt` and `DawItTests`, `GuidePrint.cpp` in `DawIt`
  - `tests/EngineTests.cpp`

## Libraries and models

None. The PDF writer, the predictors and the OKLab conversion are a few hundred lines of plain C++ on JUCE.

## Steps

- [x] 1. **PDF writer: pages one at a time, smaller pictures, bookmarks and links.** Add `pdf::Document`, `Link`, `Bookmark` and the Letter constants. Write each picture with the PNG predictors (per-row choice of None, Sub, Up or Paeth) and `/DecodeParms`. Make the old `write` functions wrappers. Files: `src/engine/PdfWriter.{h,cpp}`, `tests/EngineTests.cpp`. Tests: new `runPdfTests()` (see *Tests*). In `runChordSheetTests`, the existing check that unpacks the first picture now unfilters its rows first. Check: *File > Export Chord Sheet (PDF)...* still opens in Preview looking the same, and the file is smaller than before (write both sizes in *Changes while building*). Done when: `./build.sh --test` passes without warnings.
- [x] 2. **Paper sizes and paper colours.** Add `booklet::Paper`, `getSize`, `getName`, `getDefaultPaper`, `getPixelSize`, `toPaper` (colour and image), `getContrastRatio` and `getOkLightness`. Add `ink`, `inkDim` and `rule`, and use them in `ChordSheetExport.cpp`. Files: `src/engine/GuideBooklet.{h,cpp}` (new), `CMakeLists.txt` (both targets), `src/ui/ChordSheetExport.cpp`, `tests/EngineTests.cpp`. Tests: the paper and colour checks in `runGuideBookletTests()`. Check: the tests print the mapped hex values of `theme::text`, `textDim`, `panelLight` and `accent` in their messages. Done when: `./build.sh --test` passes.
- [x] 3. **The booklet's layout and pages.** Add `Topic`, `Guide`, `Booklet`, `Options`, `PageInfo`, `Plan`, `makePlan`, `renderPage` and `write`: covers with contents and links, overflow contents pages, topic pages with the picture capped at 45% of the page height, text flowing on to "(continued)" pages, footers with page numbers, and bookmarks. Files: `src/engine/GuideBooklet.{h,cpp}`, `tests/EngineTests.cpp`. Tests: the layout, render and write checks in `runGuideBookletTests()`, including a booklet for every `gen::guides` entry with listed topics. Check: temporarily write a sample booklet from the test to `/tmp` and open it in Preview, then remove that line before finishing the step. Done when: `./build.sh --test` passes.
- [x] 4. **Export a "What is ...?" guide.**
  - Add `guideprint::Source`, `Item`, `App`, `setApp`, `ExportButton` (with Cmd+P through a `KeyListener` on the top-level window), `isExportKey`, `exportGuide`, `exportToFile`, the `ExportOptions` dialog and `ExportJob` (two passes, pictures through `callAsync` and `WaitableEvent`, progress, cancel, the "Saved ..." alert with *Show in Finder*).
  - Make `fxui::Guide` a `Source`, with `Page::printSeconds`, `setPrintSource` and the button.
  - Set the print source in `EffectEditor::openGuide`, `BuiltInEqEditor::openGuide` and `BuiltInCompressorEditor::openGuide`.
  - Call `setApp` in `MainComponent` (with `getGuide` returning an empty item for now).

  Files: `src/ui/GuidePrint.{h,cpp}` (new), `CMakeLists.txt` (`DawIt`), `src/ui/EffectWindow.{h,cpp}`, `src/ui/EqEditor.cpp`, `src/ui/CompressorEditor.cpp`, `src/ui/MainComponent.{h,cpp}`. Tests: none new (UI); the booklet behind it is tested. Check: run `./run.sh -- --demo`, put DawIt Reverb on a track, open *What is reverb?*, and press Cmd+P: the dialog opens. Export an A4 light PDF. In Preview: a cover with the contents and working links, a page per topic, light pictures with the reverb's colours, the sidebar bookmarks, and "Page n of N". Export again *As on screen* and on Letter. Cancel one halfway: no file. Done when: `./build.sh --test` passes and all 11 effect and synth guides export.
- [x] 5. **Sound, MIDI, rhythm and gain staging guides.** Make `SoundExplainer`, `MidiExplainer`, `RhythmExplainer` and `GainStagingExplainer` `Source`s, with the button in their title rows and their print sources. Files: those four `.h/.cpp` pairs. Check: export each with the demo song. *What is MIDI?*'s *Your clip* shows the piano roll's clip, and *Gain staging* shows the selected track's chain. Close a guide window during its export: the export stops with the message and no file. Done when: `./build.sh --test` passes.
- [x] 6. **Chord, structure and lyrics guides.** Make `StructureExplainer`, `LyricsExplainer` and `ChordExplainer` `Source`s: the chord guide's composite picture, its print source, and `setPrintChordShapes`. Lyrics' picture is printed at its preferred height. Files: those three `.h/.cpp` pairs. Check: with the A minor demo song's sections, chords and lyrics, export each. *Your lyrics*, *Your song's structure* and *Your chord track* show the song. The circle of fifths page shows the circle, and a progression page its tiles and diagrams. Done when: `./build.sh --test` passes.
- [x] 7. **Sets from the Theory Guides window and the File menu.**
  - Add `guideprint::exportGuides` with the *Guides* combo, the set covers (already in `booklet`), and `MainComponent`'s `getGuide`.
  - Add `ArrangementView::getPrintGuide`, `DeviceBar::getPrintGuide`, `BuiltInGuides::makePages` and `getPrintGuide`.
  - In the Theory Guides window: the button, Cmd+P, the card's right-click menu, and the intro's second sentence.
  - Add `exportGuidesCommand` in the File menu.

  Files: `src/ui/GuidePrint.{h,cpp}`, `src/ui/TheoryGuidesWindow.{h,cpp}`, `src/ui/MainComponent.{h,cpp}`, `src/ui/ArrangementView.{h,cpp}`, `src/ui/DeviceBar.{h,cpp}`. Check: export *Start here* from the window, *Mixing* from the File menu, and a search for "attack". Right-click the EQ's card and export it with no EQ in the song: its cover says "a DawIt EQ with its starting settings". Guide windows that were closed stay closed, and open ones go back to their topic. Done when: `./build.sh --test` passes.
- [x] 8. **The `--export-guides <folder>` developer flag.** Files: `src/Main.cpp`, `src/ui/GuidePrint.cpp` if `exportToFile` needs anything more. Check: `perl -e 'alarm 600; exec @ARGV' DawIt.app/Contents/MacOS/DawIt --demo --export-guides /tmp/dawit-guides` prints 20 lines and returns 0. Done when: `./build.sh --test` passes and the run returns 0.
- [x] 9. **Real-app check.**
  - Copy `~/Library/Application Support/DawIt/DawIt.settings` aside first and put it back afterwards (never read it).
  - Patch `src/Main.cpp` temporarily with a hook behind `DAWIT_HOOK`: set up the A minor pop song with sections, chords and lyrics (as for the website's lyrics screenshots), and save 2x PNGs with `createComponentSnapshot (..., 2.0f)` of:
    1. *What is reverb?* with its *Export PDF...* button;
    2. the export dialog (single guide) and the set dialog on *Start here*;
    3. the Theory Guides window with its button, and a card's right-click menu;
    4. page images from `booklet::renderPage` at 100 dpi, saved as PNG: the lyrics guide's cover, its *Your lyrics* page, a *What is EQ?* page in light and in on-screen colours, a "(continued)" page, a Letter cover and *Start here*'s cover.
  - Look at them for: no clipped or overlapping text; light pictures where every colour still reads (section and chord colours, meters, curves); dim text readable; page numbers and contents right.
  - Click the contents links and the sidebar in Preview.
  - Run `--export-guides` with `--demo`, and again on an empty song (every effect guide on a fresh instance). Write the times and sizes per guide and for a manual *All guides* export in *Changes while building*. If *All guides* is above about 150 MB, apply decision 4's fallback (sets at 200 dpi).
  - Then `git checkout -- src/Main.cpp` and rebuild cleanly.

  Done when: every screenshot looks right and `./build.sh --test` passes on the clean tree.
- [x] 10. **Docs.**
  - README: a *Features* bullet "**Guides as PDF booklets**" saying where (*Export PDF...* in every guide window, Cmd+P, the Theory Guides window's button and the card menu, *File > Export Theory Guides (PDF)...*), what's in the booklet and the paper and picture choices. Add *Tests* lines for the PDF writer additions and the booklet. Add *Code layout* lines for `GuideBooklet` ("a guide's or a set of guides' booklet: covers with contents, a page per topic, paper colours") and `GuidePrint` ("Export PDF... in the guide windows and the Theory Guides window: the dialog and the export job"). Update `PdfWriter`'s line (pages one at a time, PNG predictors, bookmarks and links). Mention `--export-guides` next to `--screenshot` in *Building*.
  - FEATURE_IDEAS.html: a `<p class="status partly"><strong>Partly done:</strong> ...</p>` (or *Done*, if the user dropped the open items) under *Export a guide to PDF*, saying what's built and what's open (see *Left for later*). Under *Export to PDF: sheet music and guitar chord sheets*, nothing changes.
  - CLAUDE.md: in *Theory blocks (guides)*, add that a new guide window is also a `guideprint::Source` with an *Export PDF...* button (and a new "What is ...?" guide gets it through `fxui::Guide`), and that `--export-guides` checks it. Add a line to *Building and testing* about `--export-guides <folder>`.
  - Tick the steps here and set `Status: Built, not committed`.

## Tests

`runPdfTests()` (new):
- Predictor round trip: a 37 x 5 RGB image with a horizontal gradient, a vertical gradient and a few odd pixels is written. Its stream is inflated, every row's filter byte (0, 1, 2 or 4) is undone with a small test helper, and the result is the image's pixels exactly. The stream has `/Predictor 15 /Colors 3 /BitsPerComponent 8 /Columns 37`.
- A 256 x 64 smooth gradient's compressed stream is smaller with the predictors than the same pixels deflated without them (the test compresses the raw rows itself).
- Bookmarks: a 3-page document with bookmarks "A" (page 0, level 0), "A1" (page 1, level 1) and "B" (page 2, level 0) has `/Type /Outlines`. The root counts 3. "A" has `/First` and `/Last` pointing at "A1". Each `/Dest` names the right page object, checked through the cross-reference table as the existing test does.
- Links: a link at (40, 100, 200 x 20) on an A4 page to page 2 writes `/Subtype /Link`, `/Rect [40 721.89 240 741.89]` (within 0.01) and a `/Dest` naming page 2's object, and page 0's dictionary lists it in `/Annots`.
- Without bookmarks or links, the catalog has no `/Outlines` and pages have no `/Annots`. The existing two-page test still counts 10 objects with every offset right.
- `Document::getNumPages()` counts the pages added. Writing a document with no pages fails with a message.

`runGuideBookletTests()` (new):
- **Paper:**
  - A4 is 595.28 x 841.89 pt and Letter 612 x 792 (within 0.01);
  - at 300 dpi, 2480 x 3508 and 2550 x 3300 px; at 36 dpi, 298 x 421 and 306 x 396;
  - `getDefaultPaper` gives Letter for "US", "CA", "MX" and "PH", and A4 for "GB", "BE", "DE" and "".
- **Contrast:** black on white is 21:1 (within 0.01), a colour against itself 1:1, and #767676 on white is 4.54:1 (within 0.02).
- **Paper colours** (from `ui/Theme.h`, which is header-only like `ui/PitchRows.h`):
  - `theme::background` becomes white (every channel at least 250);
  - `theme::panelLight` has OKLab L between 0.85 and 0.92;
  - `theme::text` is at least 7:1 against the mapped `panelLight` and against white;
  - `theme::textDim` is at least 4.5:1 against the mapped `panelLight`;
  - the coloured ones, `theme::accent`, `meterGreen`, `meterYellow`, `meterRed` and the eight area colours (#5cc8ff #7d8bff #b38cff #ff8fb1 #ffa94d #5fd38d #4fd1c5 #e0b26b, as in `TheoryGuidesView::getColour`), keep their hue within 8° (`getHue() * 360`, wrapping round) and are at least 3:1 against white;
  - the accent at 35% over `panelLight` maps to OKLab L of at least 0.65, with blue its largest channel;
  - a 3-pixel image of background, text and accent maps each pixel as `toPaper (Colour)` does, and alpha is kept.
- **Layout of a single guide:** 7 topics, each with 3 short paragraphs and a 774 x 360 picture, give 8 pages. Page 0 is the cover with 7 links targeting pages 1 to 7. `topicPages[0]` is {1, ..., 7}. There are 7 bookmarks at level 0 on pages 1 to 7. Every topic page's text range covers its whole text.
- **Long text:** a topic of 60 paragraphs of 40 words spans at least 3 pages. Its ranges follow each other without gaps or overlaps, and together they cover the text exactly. The pages after the first are `continued`. The next topic starts on the page after its last, and the cover's link for it targets that page.
- **Pictures:**
  - a 400 x 1000 picture is at most 45% of the page's height (378.85 pt on A4), with width over height 0.4 (within 0.01), centred;
  - a topic without a picture has an empty `pictureArea`, and its text starts higher than on a topic with one;
  - a 774 x 360 picture fills the text width (515.28 pt on A4).
- **A set:** two guides of 3 and 4 topics give 10 pages: the cover, guide A's cover, 3 topics, guide B's cover, 4 topics. `guidePages` is {1, 5}. The cover's 2 links target 1 and 5. The bookmarks are A (1), its topics at level 1 (2, 3, 4), B (5) and its topics (6 to 9).
- **Contents overflow:** a single guide with 40 topics has more than one contents page, the first topic starts after the last of them, and the covers' links together number 40, each on its topic's start page.
- **Every guide's booklet:** for each `gen::guides` entry with listed topics, a booklet with short texts and no pictures has 1 + the topics' count pages, and its cover's contents name every topic.
- **Drawing:**
  - `renderPage` at 36 dpi gives 298 x 421 px on A4 and 306 x 396 on Letter;
  - the corner pixel is white;
  - the footer's strip has dark pixels (the page number);
  - a topic page whose picture is filled with `theme::panelLight` is light in the middle of `pictureArea` with paper colours (every channel above 200), and dark without (below 80).
- **Writing:** `booklet::write` of the 7-topic guide gives a PDF with `/Count 8`, `/Type /Outlines`, 7 `/Subtype /Link` and the guide's name in `/Title`.

## Website hand-off

For dawWebsite, which asks the user first:

- **Features page:** a section `id="guide-pdf"` right after `theory-guides`, with the step label "Take a guide with you" and the h2 "Print a guide, or a whole path of them, as a booklet". The intro: read away from the screen, print for a lesson, hand out in a class. List items:
  - *Export PDF...* in every guide window (Cmd+P);
  - sets from the Theory Guides window (*Start here*, an area, all, a search) and *File > Export Theory Guides (PDF)...*;
  - a cover with contents you can click, a page per topic with its picture and text;
  - "your song" pages showing your song, effects' pages showing their settings;
  - light pictures for paper or as on screen;
  - A4 or Letter.

  Then a `sheet-pair` figure like the chord sheets' section, and a wide shot.
- **Screenshots** (the A minor pop song with verses, choruses and lyrics; hook method; page images from `booklet::renderPage` at the chord sheets' size, about 989 x 1400):
  1. the cover of *How lyrics are written*, with its contents and "Song: ... · A minor ...";
  2. its *Your lyrics* page in light colours;
  3. a *What is EQ?* topic page with the EQ on the vocal, to show an effect's picture on paper;
  4. the export dialog over the Theory Guides window on *Start here* (wide shot).
- **Main page:** maybe a list item in step *1 Explore music theory* ("Print any guide, or the Start here path, as a booklet"). Ask the user.
- **Booklets to download (a question for the user):** `--export-guides` on the demo song makes every guide's PDF and *Start here.pdf*. The site could offer them (sizes from step 9). They would show the demo song, not the reader's. They'd need regenerating whenever a guide changes.
- **How it's made:** no change (no new library, model or tool).

## Left for later

- Real PDF text (searchable, selectable, sharper and smaller), with an embedded font such as IBM Plex Sans (OFL).
- A real print palette that the pictures draw with, instead of mapping their pixels. It would come with a light theme for the app.
- A preview of the pages in the app before saving.
- Choosing which topics go in; two pages on one sheet to save paper.
- Moving pictures printed as a strip of moments rather than one.
- Links in the booklet's text to other guides' pages, once guides link to each other (*Theory guides in one place*'s *See also*).
- The paths by goal (*Guide paths by goal*) as sets, once they exist.
- Note-reading exercise sheets (*Learn to read notes on the piano*) built on `booklet` and `pdf::Document`.
- The booklets on the website to download.
- Compressing pages on several threads, to make *All guides* faster.

## Changes while building

- `fxui::Guide` has one `printSeconds` constant (1.0) instead of a field per `Page`, which would have caused missing-field-initialiser warnings in 11 places.
- `PageInfo` also has `textTop`, `firstLine`, `numLines` and `contentsContinued`; `Item` has `sourceText` and `useOpen`/`adopt`; the export's done callback gets a `Report` (pages, topic names, whether topics start their own pages) instead of a page count.
- The print source of a window's own export says "the settings in DawIt Reverb's window" (the editor doesn't know its track); sets say "DawIt Reverb on Vocals, as set in its window".
- `--export-guides` accepts the chord and structure guides' grouped topics (the Theory Guides list names the series as one topic).
- `getEntry` for a one-guide set (a card's export) uses the guide's own summary on its cover.
- Chord sheet file size before/after the PNG predictors was not measured. Page sizes at 300 dpi: 2.4 to 7.7 MB per guide; *Start here* (82 pages) 35 MB; all 19 single guides together 132 MB, so *All guides* is under 150 MB and sets stay at 300 dpi (`setDpi` in GuidePrint.cpp).
- Timings (demo song): 1.3 to 3.2 s per guide, *Start here* 15 s.
- Checked in the real app (A minor demo song with sections, chords, lyrics, EQ and reverb on Vocals): the guide window, both dialogs, the Theory Guides window, and pages of the PDFs (lyrics, EQ light and screen, chord track, Letter). The card's right-click menu could not be shown by a simulated click, so it is untested visually; cancelling halfway, closing a guide window during an export and Cmd+P were not exercised by hand.
- Because `src/Main.cpp` now holds the permanent `--export-guides` flag, the temporary hook was removed by restoring a saved copy instead of `git checkout`.
