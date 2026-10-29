#include "SheetMusicExport.h"
#include "GuideButton.h"
#include "ScorePainter.h"
#include "Theme.h"
#include "engine/PdfWriter.h"
#include "generate/ChordSheet.h"

namespace daw::sheetmusic
{
namespace
{
    namespace notation = gen::notation;

    constexpr float margin = 48.0f, staffSpace = 5.6f, braceRoom = 2.5f;

    juce::Point<float> getPaperSize (Paper paper)
    {
        return paper == Paper::letter ? juce::Point<float> (pdf::letterWidth, pdf::letterHeight)
                                      : juce::Point<float> (pdf::a4Width, pdf::a4Height);
    }

    scorepaint::Ink getPrintInk()
    {
        return { juce::Colours::black, juce::Colour (0xff666666), juce::Colours::black, juce::Colours::white };
    }

    /** Where a line of music sits on its page. */
    struct Placed
    {
        notation::Line line;
        float top = 0.0f;       // the first staff's top line
        float height = 0.0f;    // everything the line takes, with the gap below it
    };

    void drawText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size, bool bold, juce::Colour c,
                   juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (text, r, j, true);
    }
} // namespace

gen::readmusic::Part getPart (Engine& engine, Track& track, double playheadBeat)
{
    gen::readmusic::Part part;
    part.hasTrack = track.isMidi();
    part.trackName = track.name;
    part.playheadBeat = playheadBeat;
    part.autoClef = track.scoreClef == ScoreClef::automatic;

    if (! track.isMidi())
        return part;

    const juce::ScopedLock lock (engine.getLock());

    for (auto& clip : track.clips)
        if (clip->kind == Clip::Kind::midi)
        {
            auto* midi = static_cast<MidiClip*> (clip.get());

            for (auto note : midi->notes)
            {
                note.start += midi->startBeat;
                part.input.notes.push_back (note);
            }
        }

    part.input.key = engine.getKey();
    part.input.chords = engine.getChords();
    part.input.sections = engine.getSections();
    part.input.tempo = engine.getTempoMap();
    part.input.clef = (notation::ClefChoice) (int) track.scoreClef;
    return part;
}

std::vector<juce::Image> renderPages (const gen::readmusic::Part& part, const Header& header, Paper paper, float pixelsPerPoint)
{
    std::vector<juce::Image> images;

    if (part.input.notes.empty())
        return images;

    const auto score = notation::build (part.input);
    const auto size = getPaperSize (paper);
    const float space = staffSpace;
    const float contentWidth = size.x - 2.0f * margin;
    const auto lines = notation::layoutLines (score, (double) ((contentWidth - braceRoom * space) / space), false);
    const float block = scorepaint::getStaffBlockHeight (score, space);
    const float footHeight = 22.0f, headerHeight = 96.0f;

    // Which lines go on which page.
    std::vector<std::vector<Placed>> pages (1);
    float y = margin + headerHeight;

    for (auto& line : lines)
    {
        Placed p { line, 0.0f, 0.0f };
        const float topSpace = scorepaint::getLineTopMargin (score, line, space);
        p.height = topSpace + block + 4.0f * space + 2.0f * space;

        if (y + p.height > size.y - margin - footHeight && ! pages.back().empty())
        {
            pages.emplace_back();
            y = margin;
        }

        p.top = y + topSpace;
        pages.back().push_back (p);
        y += p.height;
    }

    const auto ink = getPrintInk();
    const int numPages = (int) pages.size();

    for (int pageIndex = 0; pageIndex < numPages; ++pageIndex)
    {
        juce::Image image (juce::Image::RGB, juce::roundToInt (size.x * pixelsPerPoint), juce::roundToInt (size.y * pixelsPerPoint), true);
        juce::Graphics g (image);
        g.fillAll (juce::Colours::white);
        g.addTransform (juce::AffineTransform::scale (pixelsPerPoint));

        if (pageIndex == 0)
        {
            drawText (g, header.title, { margin, margin, contentWidth, 30.0f }, 24.0f, true, ink.ink, juce::Justification::centred);
            drawText (g, header.part, { margin, margin + 40.0f, contentWidth, 18.0f }, 13.0f, true, ink.ink);
            drawText (g, juce::String::fromUTF8 ("\xe2\x99\xa9") + " = " + juce::String (header.bpm) + "     Key: " + header.keyName,
                      { margin, margin + 60.0f, contentWidth, 16.0f }, 10.5f, false, ink.dim);
        }

        for (auto& p : pages[(size_t) pageIndex])
            scorepaint::drawSystem (g, score, p.line, margin + braceRoom * space, p.top, space, ink, {});

        drawText (g, "Made with DawIt", { margin, size.y - margin + 6.0f, contentWidth / 2.0f, 14.0f }, 8.5f, false, ink.dim);
        drawText (g, "Page " + juce::String (pageIndex + 1) + " of " + juce::String (numPages),
                  { margin + contentWidth / 2.0f, size.y - margin + 6.0f, contentWidth / 2.0f, 14.0f }, 8.5f, false, ink.dim, juce::Justification::centredRight);

        images.push_back (image);
    }

    return images;
}

juce::Result exportPdf (const gen::readmusic::Part& part, const Header& header, Paper paper, const juce::File& file)
{
    const auto pages = renderPages (part, header, paper);

    if (pages.empty())
        return juce::Result::fail ("This track has no notes to write out.");

    const auto size = getPaperSize (paper);
    pdf::Document document (header.title + " - " + header.part);

    for (auto& page : pages)
        document.addPage (page, size.x, size.y);

    return document.write (file);
}

juce::String getFileName (const juce::String& title, const juce::String& part)
{
    return juce::File::createLegalFileName (title + " - " + part);
}

Paper getSavedPaper (Engine& engine)
{
    return engine.properties.getUserSettings()->getValue ("export.sheetPaper", "a4") == "letter" ? Paper::letter : Paper::a4;
}

//==============================================================================
namespace
{
    std::unique_ptr<juce::FileChooser> chooser;

    /** The dialog's content: the paper, then Export... asks where to save. */
    class SheetMusicOptions final : public juce::Component
    {
    public:
        SheetMusicOptions (Engine& e, gen::readmusic::Part p, Header h, std::function<void()> showGuide)
            : engine (e), part (std::move (p)), header (std::move (h)), onShowGuide (std::move (showGuide))
        {
            paperLabel.setText ("Paper", juce::dontSendNotification);
            paperBox.addItem ("A4", 1);
            paperBox.addItem ("US Letter", 2);
            paperBox.setSelectedId (getSavedPaper (engine) == Paper::letter ? 2 : 1, juce::dontSendNotification);
            paperBox.setTooltip ("The size of the pages: A4 for most of the world, US Letter mainly in the United States and Canada");

            hint.setText ("Saves " + header.part + " as sheet music on portrait pages, black on white, to print or read on a tablet. "
                          "Your MIDI isn't changed: the notes are written rounded for reading.",
                          juce::dontSendNotification);
            hint.setFont (juce::FontOptions (13.0f));
            hint.setColour (juce::Label::textColourId, theme::textDim);
            hint.setJustificationType (juce::Justification::topLeft);

            exportButton.setTooltip ("Choose where to save the PDF");
            cancelButton.setTooltip ("Close without saving");

            for (auto* c : { (juce::Component*) &paperLabel, (juce::Component*) &paperBox, (juce::Component*) &hint,
                             (juce::Component*) &exportButton, (juce::Component*) &cancelButton })
                addAndMakeVisible (c);

            exportButton.onClick = [this] { chooseFile(); };
            cancelButton.onClick = [this] { close(); };
            guideButton.setTooltip ("Open the guide How to read music: what the staff, clefs and notes mean");
            guideButton.onClick = [this]
            {
                auto show = onShowGuide;
                close();   // deletes this
                if (show)
                    show();
            };
            addChildComponent (guideButton);
            guideButton.setVisible (onShowGuide != nullptr);
            setSize (480, 190);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);
            auto row = r.removeFromTop (28);
            paperLabel.setBounds (row.removeFromLeft (60));
            paperBox.setBounds (row.removeFromLeft (150));
            r.removeFromTop (10);
            auto buttons = r.removeFromBottom (28);
            guideButton.setBounds (buttons.removeFromLeft (guideButton.getBestWidthForHeight (buttons.getHeight())));
            cancelButton.setBounds (buttons.removeFromRight (90));
            buttons.removeFromRight (8);
            exportButton.setBounds (buttons.removeFromRight (110));
            hint.setBounds (r);
        }

    private:
        void close()
        {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);
        }

        void chooseFile()
        {
            const auto paper = paperBox.getSelectedId() == 2 ? Paper::letter : Paper::a4;
            auto& props = *engine.properties.getUserSettings();
            props.setValue ("export.sheetPaper", paper == Paper::letter ? "letter" : "a4");
            const juce::File folder (props.getValue ("export.sheetFolder", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getFullPathName()));
            const auto fileName = getFileName (header.title, header.part) + ".pdf";
            auto& e = engine;
            const auto p = part;
            const auto h = header;
            close();   // deletes this

            chooser = std::make_unique<juce::FileChooser> ("Export sheet music", folder.getChildFile (fileName), "*.pdf");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [&e, p, h, paper] (const juce::FileChooser& fc)
                                  {
                                      auto target = fc.getResult();

                                      if (target == juce::File())
                                          return;

                                      target = target.withFileExtension (".pdf");
                                      auto& props2 = *e.properties.getUserSettings();
                                      props2.setValue ("export.sheetFolder", target.getParentDirectory().getFullPathName());
                                      props2.saveIfNeeded();

                                      if (const auto result = exportPdf (p, h, paper, target); result.failed())
                                          juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", result.getErrorMessage());
                                  });
        }

        Engine& engine;
        gen::readmusic::Part part;
        Header header;
        juce::Label paperLabel, hint;
        juce::ComboBox paperBox;
        juce::TextButton exportButton { "Export..." }, cancelButton { "Cancel" };
        std::function<void()> onShowGuide;
        GuideButton guideButton { "How to read music" };
    };
} // namespace

void showDialog (Engine& engine, Track* track, const juce::String& projectName, std::function<void()> onShowGuide)
{
    if (track == nullptr || ! track->isMidi())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "No MIDI track selected",
                                                "Select a MIDI track, then export it as sheet music.");
        return;
    }

    auto part = getPart (engine, *track, 0.0);

    if (part.input.notes.empty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "No notes yet",
                                                track->name + " has no notes. Draw some in the piano roll, then export it.");
        return;
    }

    Header header;
    header.title = projectName.isEmpty() ? juce::String ("Untitled") : projectName;
    header.part = track->name;
    header.keyName = gen::sheet::getKeyName (part.input.key);
    header.bpm = juce::roundToInt (part.input.tempo.getBpmAt (0.0));

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new SheetMusicOptions (engine, std::move (part), header, std::move (onShowGuide)));
    options.dialogTitle = "Export sheet music (PDF)";
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}
} // namespace daw::sheetmusic
