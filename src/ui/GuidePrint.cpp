#include "GuidePrint.h"
#include "TheoryGuidesWindow.h"
#include "Theme.h"

#include <optional>

namespace daw::guideprint
{
namespace
{
    namespace guides = gen::guides;

    App app;

    constexpr float setDpi = booklet::printDpi;   // lowered if a booklet of all guides turns out too large

    juce::String getDateText()
    {
        const auto now = juce::Time::getCurrentTime();
        return juce::String (now.getDayOfMonth()) + " " + now.getMonthName (false) + " " + juce::String (now.getYear());
    }

    juce::String getSongLine()
    {
        if (app.engine == nullptr)
            return {};

        const auto name = app.getProjectName != nullptr ? app.getProjectName() : juce::String();
        const auto dot = juce::String::fromUTF8 (" \xc2\xb7 ");
        return (name.isNotEmpty() ? name + dot : juce::String()) + app.engine->getKey().getName() + dot
               + juce::String (juce::roundToInt (app.engine->getTempo())) + " BPM";
    }

    juce::String getMegabytes (juce::int64 bytes)
    {
        return juce::String ((double) bytes / (1024.0 * 1024.0), 1) + " MB";
    }

    //==============================================================================
    /** Saves the booklet on its own thread behind a progress bar, taking each guide's pictures on the message thread
        while it goes, then says how it went. Deletes itself. */
    class ExportJob final : public juce::ThreadWithProgressWindow
    {
    public:
        ExportJob (std::vector<Item> i, booklet::Booklet b, const booklet::Options& o, const juce::File& f, bool report,
                   std::function<void (juce::Result, const Report&)> d)
            : juce::ThreadWithProgressWindow ("Exporting " + (b.guides.size() == 1 ? b.guides.front().name : b.title) + "...", true, true, 20000),
              items (std::move (i)), book (std::move (b)), options (o), target (f), showReport (report), done (std::move (d))
        {
        }

        void run() override
        {
            plan = booklet::makePlan (book, options);

            for (size_t g = 0; g < book.guides.size(); ++g)
            {
                juce::StringArray names;
                int previous = -1;

                for (size_t t = 0; t < book.guides[g].topics.size(); ++t)
                {
                    const int page = plan.topicPages[g][t];
                    names.add (book.guides[g].topics[t].name);
                    made.topicsOnOwnPages = made.topicsOnOwnPages && page > previous && plan.pages[(size_t) page].kind == booklet::PageInfo::Kind::topic
                                              && ! plan.pages[(size_t) page].continued;
                    previous = page;
                }

                made.topicNames.push_back (names);
            }

            const auto size = booklet::getSize (options.paper);
            pdf::Document document (book.guides.size() == 1 ? book.guides.front().name : book.title);
            int picturesOf = -1;

            for (int page = 0; page < (int) plan.pages.size(); ++page)
            {
                if (threadShouldExit())
                {
                    cancelled = true;
                    return;
                }

                const auto& info = plan.pages[(size_t) page];

                if (info.kind == booklet::PageInfo::Kind::topic && info.guide != picturesOf)
                {
                    if (picturesOf >= 0)
                        for (auto& topic : book.guides[(size_t) picturesOf].topics)
                            topic.picture = {};

                    picturesOf = info.guide;

                    if (! takePictures (info.guide))
                        return;   // cancelled or closed
                }

                document.addPage (booklet::renderPage (book, plan, page, options), size.x, size.y, info.links);
                setProgress ((page + 1.0) / (double) plan.pages.size());
            }

            document.setBookmarks (plan.bookmarks);
            outcome = document.write (target);
            made.pages = document.getNumPages();
        }

        void threadComplete (bool) override
        {
            state->cancelled = true;
            items.clear();   // the guides and plugins made for the export, on the message thread
            report();

            if (done != nullptr)
                done (cancelled ? juce::Result::fail ("Cancelled.") : (problem.isNotEmpty() ? juce::Result::fail (problem) : outcome), made);

            delete this;
        }

    private:
        struct State
        {
            juce::WaitableEvent finished;
            std::atomic<bool> cancelled { false };
        };

        /** Asks the message thread to photograph a guide's topics, and waits for it. False when the guide went away
            or the user cancelled. */
        bool takePictures (int guide)
        {
            bool ok = true;
            auto s = state;
            s->finished.reset();

            juce::MessageManager::callAsync ([this, s, guide, &ok]
            {
                if (! s->cancelled)
                    ok = photograph (guide);

                s->finished.signal();
            });

            while (! s->finished.wait (100))
            {
                if (threadShouldExit())
                {
                    s->cancelled = true;
                    cancelled = true;
                    return false;
                }
            }

            if (! ok)
                return false;

            return ! threadShouldExit();
        }

        /** On the message thread: each topic shown and drawn at the size its page needs. */
        bool photograph (int guide)
        {
            auto& item = items[(size_t) guide];
            auto& g = book.guides[(size_t) guide];

            for (size_t t = 0; t < g.topics.size(); ++t)
            {
                if (item.component == nullptr || item.source == nullptr)
                {
                    problem = "The guide was closed before its pages were all taken. Nothing was saved.";
                    return false;
                }

                auto& topic = g.topics[t];

                if (topic.pictureSize.x <= 0 || topic.pictureSize.y <= 0)
                    continue;

                const auto& area = plan.pages[(size_t) plan.topicPages[(size_t) guide][t]].pictureArea;
                const float scale = area.getWidth() * options.dpi / 72.0f / (float) topic.pictureSize.x;
                item.source->showPrintTopic ((int) t);
                topic.picture = item.source->paintPrintPicture (scale);
            }

            if (item.component != nullptr && item.source != nullptr)
                item.source->endPrinting();

            return true;
        }

        void report()
        {
            if (cancelled || ! showReport)
                return;

            if (problem.isNotEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export stopped", problem);
                return;
            }

            if (outcome.failed())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", outcome.getErrorMessage());
                return;
            }

            const auto text = "Saved " + juce::String (made.pages) + (made.pages == 1 ? " page (" : " pages (") + getMegabytes (target.getSize()) + ") to "
                              + target.getFileName() + ".";
            juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::InfoIcon, "Guide exported", text, "Show in Finder", "OK", nullptr,
                                                juce::ModalCallbackFunction::create ([file = target] (int result)
                                                {
                                                    if (result == 1)
                                                        file.revealToUser();
                                                }));
        }

        std::vector<Item> items;
        booklet::Booklet book;
        booklet::Options options;
        booklet::Plan plan;
        juce::File target;
        bool showReport;
        std::function<void (juce::Result, const Report&)> done;
        std::shared_ptr<State> state = std::make_shared<State>();
        juce::Result outcome = juce::Result::ok();
        juce::String problem;
        bool cancelled = false;
        Report made;
    };

    /** On the message thread: every guide's text, one topic after another, and the booklet around it. Items with no
        window are left out. */
    booklet::Booklet collect (std::vector<Item>& items, const juce::String& title, const juce::String& summary, bool wantSong)
    {
        booklet::Booklet b;
        b.title = title;
        b.summary = summary;
        b.date = getDateText();

        if (wantSong)
            b.songLine = getSongLine();

        for (auto& item : items)
        {
            if (item.source == nullptr || item.component == nullptr)
                continue;

            const auto entry = guides::getEntry (item.guide);
            booklet::Guide g;
            g.name = entry.name;
            g.summary = entry.summary;
            g.area = guides::getAreaName (entry.area);
            g.colour = TheoryGuidesView::getColour (entry.area);
            g.source = item.sourceText.isNotEmpty() ? item.sourceText : item.source->getPrintSource();

            for (int i = 0; i < item.source->getNumPrintTopics(); ++i)
                g.topics.push_back (item.source->showPrintTopic (i));

            item.source->endPrinting();
            b.guides.push_back (std::move (g));
        }

        return b;
    }

    void start (std::vector<Item> items, const juce::String& title, const juce::String& summary, const booklet::Options& options,
                const juce::File& file, bool report, std::function<void (juce::Result, const Report&)> done)
    {
        // The guides left out (no window) are dropped from the items too, so booklet and items stay in step.
        items.erase (std::remove_if (items.begin(), items.end(), [] (const Item& i) { return i.source == nullptr || i.component == nullptr; }), items.end());
        auto b = collect (items, title, summary, true);

        if (b.guides.empty())
        {
            if (done != nullptr)
                done (juce::Result::fail ("There is no guide to export."), Report());

            return;
        }

        (new ExportJob (std::move (items), std::move (b), options, file, report, std::move (done)))->launchThread();
    }

    //==============================================================================
    struct Choice
    {
        juce::String name, label;
        std::vector<guides::Guide> guides;
    };

    /** The choices of the set dialog. */
    std::vector<Choice> makeChoices (const std::vector<guides::Guide>& shown, const juce::String& shownName, int& selected)
    {
        std::vector<Choice> choices;
        auto count = [] (size_t n) { return juce::String ((int) n) + (n == 1 ? " guide" : " guides"); };
        auto add = [&] (const juce::String& name, std::vector<guides::Guide> list)
        {
            choices.push_back ({ name, name + ": " + count (list.size()), std::move (list) });
        };

        add ("Start here", guides::getStartPath());

        for (auto area : guides::allAreas)
        {
            std::vector<guides::Guide> list;

            for (auto& e : guides::getEntries())
                if (e.area == area)
                    list.push_back (e.guide);

            add (guides::getAreaName (area), std::move (list));
        }

        add ("All guides", std::vector<guides::Guide> (std::begin (guides::allGuides), std::end (guides::allGuides)));

        selected = 0;

        for (size_t i = 0; i < choices.size(); ++i)
            if (choices[i].name == shownName)
                selected = (int) i;

        if (selected == 0 && shownName != "Start here" && ! shown.empty())
        {
            const auto name = shown.size() == 1 ? shownName : "The " + juce::String ((int) shown.size()) + " guides your search found";
            choices.push_back ({ shownName, name, shown });
            selected = (int) choices.size() - 1;
        }

        return choices;
    }

    class ExportOptions final : public juce::Component
    {
    public:
        /** `source` set: one guide's dialog. Otherwise the set dialog over `choices`. */
        ExportOptions (Source* s, guides::Guide g, std::vector<Choice> c, int selected) : source (s), guide (g), choices (std::move (c))
        {
            auto& props = *app.engine->properties.getUserSettings();
            setCombo (guidesBox, guidesLabel, "Guides", "Which guides go in the booklet, in the order the Theory Guides window lists them");

            for (size_t i = 0; i < choices.size(); ++i)
                guidesBox.addItem (choices[i].label, (int) i + 1);

            guidesBox.setSelectedId (selected + 1, juce::dontSendNotification);
            guidesBox.onChange = [this] { updateHint(); };

            setCombo (paperBox, paperLabel, "Paper", "A4 is used almost everywhere; Letter in the US, Canada and parts of Latin America");

            for (auto paper : { booklet::Paper::a4, booklet::Paper::letter })
                paperBox.addItem (booklet::getName (paper), (int) paper + 1);

            const auto saved = props.getValue ("export.guidePaper");
            const auto paper = saved == "a4" ? booklet::Paper::a4 : saved == "letter" ? booklet::Paper::letter
                                                                                       : booklet::getDefaultPaper (juce::SystemStats::getUserRegion());
            paperBox.setSelectedId ((int) paper + 1, juce::dontSendNotification);

            setCombo (picturesBox, picturesLabel, "Pictures",
                      "Light turns the dark backgrounds white and keeps the colours that mean something, to save ink and read well on paper. "
                      "As on screen keeps the pictures as the window shows them");
            picturesBox.addItem ("Light, for printing", 1);
            picturesBox.addItem ("As on screen (dark)", 2);
            picturesBox.setSelectedId (props.getValue ("export.guidePictures", "paper") == "screen" ? 2 : 1, juce::dontSendNotification);

            hint.setFont (juce::FontOptions (12.5f));
            hint.setColour (juce::Label::textColourId, theme::textDim);
            hint.setJustificationType (juce::Justification::topLeft);
            addAndMakeVisible (hint);
            updateHint();

            exportButton.setTooltip ("Choose where to save the PDF");
            exportButton.onClick = [this] { chooseFile(); };
            cancelButton.onClick = [this] { close(); };
            addAndMakeVisible (exportButton);
            addAndMakeVisible (cancelButton);
            setSize (520, source == nullptr ? 216 : 180);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);

            auto row = [&] (juce::Label& label, juce::ComboBox& box)
            {
                auto line = r.removeFromTop (28);
                label.setBounds (line.removeFromLeft (80));
                box.setBounds (line);
                r.removeFromTop (8);
            };

            if (source == nullptr)
                row (guidesLabel, guidesBox);

            row (paperLabel, paperBox);
            row (picturesLabel, picturesBox);
            auto buttons = r.removeFromBottom (28);
            cancelButton.setBounds (buttons.removeFromRight (90));
            buttons.removeFromRight (8);
            exportButton.setBounds (buttons.removeFromRight (110));
            hint.setBounds (r);
        }

    private:
        void setCombo (juce::ComboBox& box, juce::Label& label, const juce::String& text, const juce::String& tip)
        {
            label.setText (text, juce::dontSendNotification);
            label.setColour (juce::Label::textColourId, theme::textDim);
            box.setTooltip (tip);
            addChildComponent (label);
            addChildComponent (box);
            const bool shown = &box != &guidesBox || source == nullptr;
            label.setVisible (shown);
            box.setVisible (shown);
        }

        void updateHint()
        {
            juce::String text = "A cover with the contents, then a page per topic: its picture and text as the guide shows them now, so the pages about your song show your song.";

            if (source != nullptr)
                text << " " << source->getNumPrintTopics() << " topics.";
            else if (const auto index = guidesBox.getSelectedId() - 1; juce::isPositiveAndBelow (index, (int) choices.size()))
                text = describeSet (choices[(size_t) index].name, choices[(size_t) index].guides) + " " + text;

            hint.setText (text, juce::dontSendNotification);
        }

        void close()
        {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);
        }

        void chooseFile()
        {
            const auto paper = (booklet::Paper) (paperBox.getSelectedId() - 1);
            const bool light = picturesBox.getSelectedId() == 1;

            auto& props = *app.engine->properties.getUserSettings();
            props.setValue ("export.guidePaper", paper == booklet::Paper::letter ? "letter" : "a4");
            props.setValue ("export.guidePictures", light ? "paper" : "screen");

            booklet::Options options;
            options.paper = paper;
            options.paperColours = light;
            options.dpi = source == nullptr ? setDpi : booklet::printDpi;

            const juce::File folder (props.getValue ("export.guideFolder", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getFullPathName()));
            Choice choice;

            if (source == nullptr)
                choice = choices[(size_t) (guidesBox.getSelectedId() - 1)];

            const auto fileName = juce::File::createLegalFileName (source != nullptr ? source->getPrintTitle()
                                                                   : choice.guides.size() == 1 ? choice.name : "DawIt guides - " + choice.name) + ".pdf";
            auto* sourcePointer = source;
            juce::Component::SafePointer<juce::Component> owner (dynamic_cast<juce::Component*> (source));
            const auto g = guide;
            close();   // deletes this

            chooser = std::make_shared<juce::FileChooser> (source != nullptr ? "Export guide" : "Export guides", folder.getChildFile (fileName), "*.pdf");
            auto keep = chooser;
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [keep, sourcePointer, owner, g, choice, options] (const juce::FileChooser& fc)
                                  {
                                      auto file = fc.getResult();

                                      if (file == juce::File())
                                          return;

                                      file = file.withFileExtension (".pdf");

                                      if (app.engine != nullptr)
                                      {
                                          auto& p = *app.engine->properties.getUserSettings();
                                          p.setValue ("export.guideFolder", file.getParentDirectory().getFullPathName());
                                          p.saveIfNeeded();
                                      }

                                      std::vector<Item> items;
                                      juce::String title, summary;

                                      if (sourcePointer != nullptr)
                                      {
                                          if (owner == nullptr)
                                              return;

                                          Item item;
                                          item.guide = g;
                                          item.component = owner;
                                          item.source = sourcePointer;
                                          items.push_back (std::move (item));
                                      }
                                      else
                                      {
                                          title = choice.name;
                                          summary = describeSet (choice.name, choice.guides);

                                          for (auto member : choice.guides)
                                              if (app.getGuide != nullptr)
                                                  items.push_back (app.getGuide (member));
                                      }

                                      start (std::move (items), title, summary, options, file, true, nullptr);
                                  });
        }

        Source* source;
        guides::Guide guide;
        std::vector<Choice> choices;
        juce::Label guidesLabel, paperLabel, picturesLabel, hint;
        juce::ComboBox guidesBox, paperBox, picturesBox;
        juce::TextButton exportButton { "Export..." }, cancelButton { "Cancel" };
        std::shared_ptr<juce::FileChooser> chooser;
    };

    void showDialog (Source* source, guides::Guide guide, std::vector<Choice> choices, int selected, const juce::String& title)
    {
        if (app.engine == nullptr)
            return;

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (new ExportOptions (source, guide, std::move (choices), selected));
        options.dialogTitle = title;
        options.dialogBackgroundColour = theme::panel;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    }
}

void setApp (App a)
{
    app = std::move (a);
}

std::optional<guides::Guide> findGuideForPlugin (const juce::String& identifier)
{
    for (auto& e : guides::getEntries())
        if (e.plugin == identifier)
            return e.guide;

    return std::nullopt;
}

juce::Image snapshot (juce::Component& component, float scale)
{
    return component.createComponentSnapshot (component.getLocalBounds(), true, scale);
}

bool isExportKey (const juce::KeyPress& key)
{
    return key == juce::KeyPress ('p', juce::ModifierKeys::commandModifier, 0);
}

juce::String describeSet (const juce::String& name, const std::vector<guides::Guide>& list)
{
    static const char* numbers[] { "No", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten" };
    const auto count = list.size() <= 10 ? juce::String (numbers[list.size()]) : juce::String ((int) list.size());
    const auto guidesText = count + (list.size() == 1 ? " guide" : " guides");

    if (list.size() == 1)
        return guides::getEntry (list.front()).summary;

    if (name == "Start here")
        return guidesText + " in an order that builds up, from sound and rhythm to chords, structure and the mixer.";

    if (name == "All guides")
        return "Every guide in DawIt (" + juce::String ((int) list.size()) + "), in the order the Theory Guides window lists them.";

    for (auto area : guides::allAreas)
        if (guides::getAreaName (area) == name)
            return guidesText + ": " + guides::getAreaSummary (area);

    return guidesText + " your search found.";
}

//==============================================================================
bool ExportButton::Keys::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (! isExportKey (key))
        return false;

    button.triggerClick();
    return true;
}

ExportButton::ExportButton (Source& s, guides::Guide g, const juce::String& tooltip) : juce::TextButton ("Export PDF..."), source (s), guide (g)
{
    setTooltip (tooltip.isNotEmpty() ? tooltip
                                     : juce::String ("Save this guide as a PDF to print or read on a tablet: a cover with the contents, then a page per topic "
                                                     "with its picture and text as they show now (Cmd+P)"));
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    onClick = [this] { exportGuide (source, guide); };
}

ExportButton::~ExportButton()
{
    if (listeningTo != nullptr)
        listeningTo->removeKeyListener (&keys);
}

void ExportButton::parentHierarchyChanged()
{
    auto* top = getTopLevelComponent();

    if (top == listeningTo.getComponent())
        return;

    if (listeningTo != nullptr)
        listeningTo->removeKeyListener (&keys);

    listeningTo = top;

    if (top != nullptr && top != this)
        top->addKeyListener (&keys);
}

void exportGuide (Source& source, guides::Guide guide)
{
    showDialog (&source, guide, {}, 0, "Export guide (PDF)");
}

void exportGuides (std::vector<guides::Guide> shown, const juce::String& shownName)
{
    int selected = 0;
    auto choices = makeChoices (shown, shownName, selected);
    showDialog (nullptr, guides::Guide::sound, std::move (choices), selected, "Export guides (PDF)");
}

void exportToFile (std::vector<Item> items, const juce::String& title, const juce::String& summary, const booklet::Options& options,
                   const juce::File& file, std::function<void (juce::Result, const Report&)> done)
{
    start (std::move (items), title, summary, options, file, false, std::move (done));
}
} // namespace daw::guideprint
