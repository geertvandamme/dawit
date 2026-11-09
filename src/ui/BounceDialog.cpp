#include "BounceDialog.h"
#include "engine/Bounce.h"
#include "engine/Routing.h"
#include "GuideButton.h"
#include "Theme.h"

namespace daw::bounceui
{
namespace
{
    std::function<void (bool)> guideOpener;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::String describeRange (const juce::String& name, juce::Range<double> range, const TempoMap& tempo, double barLength)
    {
        const int firstBar = (int) std::floor (range.getStart() / barLength) + 1, lastBar = (int) std::ceil (range.getEnd() / barLength);
        return name + ": bars " + juce::String (firstBar) + (lastBar > firstBar ? "-" + juce::String (lastBar) : juce::String())
               + " (" + juce::String (tempo.getSecondsBetween (range.getStart(), range.getEnd()), 1) + " s)";
    }

    //==============================================================================
    /** Bounces on its own thread behind a progress bar, then says how it went. Deletes itself. */
    class BounceJob final : public juce::ThreadWithProgressWindow
    {
    public:
        BounceJob (Engine& e, const bounce::Settings& s, const juce::File& f)
            : juce::ThreadWithProgressWindow ("Bouncing " + f.getFileName() + "...", true, true, 10000),
              settings (s), target (f)
        {
            renderer = std::make_unique<bounce::Renderer> (e, s);   // here, on the message thread
        }

        void run() override
        {
            outcome = renderer->render (target, [this] (double progress)
            {
                setProgress (progress);
                return ! threadShouldExit();
            });
        }

        void threadComplete (bool) override
        {
            renderer.reset();   // back to normal playback, on the message thread
            report();
            delete this;
        }

    private:
        void report() const
        {
            if (outcome.cancelled)
                return;

            if (! outcome.result.wasOk())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Bounce failed", outcome.result.getErrorMessage());
                return;
            }

            juce::String text;
            text << "Bounced " << juce::String (outcome.seconds, 1) << " seconds to " << target.getFileName() << ".";

            // The peak tells whether the mix fits: above 0 dB a 16 or 24-bit file clips.
            const float peakDb = juce::Decibels::gainToDecibels (outcome.peak, -100.0f);

            if (outcome.peak > 1.0f && settings.bitDepth != 32)
                text << "\n\nThe mix peaks at +" << juce::String (peakDb, 1) << " dB, above the 0 dB a WAV file can hold, so the loudest moments "
                     << "are clipped (cut off), which sounds harsh. Turn the master or the loudest tracks down by about "
                     << juce::String (std::ceil (peakDb + 1.0f), 0) << " dB and bounce again.";
            else if (outcome.peak > 1.0f)
                text << "\n\nThe mix peaks at +" << juce::String (peakDb, 1) << " dB. A 32-bit float file keeps that without clipping, "
                     << "but turn it down before playing it elsewhere.";
            else if (outcome.peak > 0.0f)
                text << "\n\nPeak level: " << juce::String (peakDb, 1) << " dB.";
            else
                text << "\n\nThe file is silent: nothing plays in this range.";

            juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::InfoIcon, "Bounce finished", text, "Show in Finder", "OK", nullptr,
                                                juce::ModalCallbackFunction::create ([file = target] (int result)
                                                {
                                                    if (result == 1)
                                                        file.revealToUser();
                                                }));
        }

        const bounce::Settings settings;
        const juce::File target;
        std::unique_ptr<bounce::Renderer> renderer;
        bounce::Outcome outcome;
    };

    //==============================================================================
    /** Runs a function on the message thread and waits for it, from the job's thread. */
    void onMessageThread (std::function<void()> function)
    {
        juce::WaitableEvent done;
        juce::MessageManager::callAsync ([&] { function(); done.signal(); });
        done.wait();
    }

    /** Writes the files of a multitrack or stem export one after the other behind one progress bar, then says how it
        went. Each file needs the message thread to start and finish, so the job thread asks for it. Deletes itself. */
    class FilesJob final : public juce::ThreadWithProgressWindow
    {
    public:
        FilesJob (Engine& e, const bounce::FilesPlan& p, const bounce::Settings& common, const juce::File& f)
            : juce::ThreadWithProgressWindow ((p.kind == bounce::FilesKind::stems ? "Bouncing stems to " : "Bouncing multitracks to ") + f.getFileName() + "...",
                                              true, true, 10000),
              folder (f), bits (common.bitDepth), exporter (std::make_unique<bounce::FilesExport> (e, p, common, f)), plan (p)
        {
        }

        void run() override
        {
            const int n = exporter->getNumFiles();

            for (int i = 0; i < n && ! threadShouldExit(); ++i)
            {
                setStatusMessage (plan.files[(size_t) i].fileName + "  (" + juce::String (i + 1) + " of " + juce::String (n) + ")");
                onMessageThread ([this, i] { exporter->start (i); });
                const auto outcome = exporter->renderStarted ([this, i, n] (double progress)
                {
                    setProgress ((i + progress) / n);
                    return ! threadShouldExit();
                });
                onMessageThread ([this] { exporter->finishFile(); });

                if (! outcome.result.wasOk())
                    break;
            }
        }

        void threadComplete (bool userPressedCancel) override
        {
            auto outcome = exporter->finish();   // the two small files; or removes everything after a cancel or a failure
            exporter.reset();

            if (! userPressedCancel && ! outcome.cancelled)
                report (outcome);

            delete this;
        }

    private:
        void report (const bounce::FilesOutcome& outcome) const
        {
            if (! outcome.result.wasOk())
            {
                if (outcome.result.getErrorMessage() != "Cancelled")
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Bounce failed", outcome.result.getErrorMessage());

                return;
            }

            juce::String text;
            text << "Wrote " << juce::String ((int) outcome.written.size()) << (outcome.written.size() == 1 ? " file" : " files") << ", each "
                 << juce::String (outcome.seconds, 1) << " seconds long, to " << folder.getFileName() << ". They all start at the same moment, so "
                 << "put them in another program at the start and they line up.";

            juce::StringArray loud;

            for (auto& w : outcome.written)
                if (w.peak > 1.0f)
                    loud.add (w.fileName.upToLastOccurrenceOf (".wav", false, true) + " (+" + juce::String (juce::Decibels::gainToDecibels (w.peak), 1) + " dB)");

            if (loud.isEmpty())
                text << "\n\nNo file peaks over 0 dB.";
            else if (bits == 32)
                text << "\n\nThese peak over 0 dB: " << loud.joinIntoString (", ") << ". A 32-bit float file keeps that without clipping, "
                     << "but turn them down before playing them elsewhere.";
            else
                text << "\n\nThese peak over 0 dB, so the file clips (is cut off) there, which sounds harsh: " << loud.joinIntoString (", ")
                     << ". Turn that part down and bounce again.";

            if (! outcome.skipped.empty())
            {
                juce::StringArray left;

                for (auto& s : outcome.skipped)
                    left.add (s.name + ": " + s.reason);

                text << "\n\nLeft out: " << left.joinIntoString (", ") << ".";
            }

            juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::InfoIcon, "Bounce finished", text, "Show in Finder", "OK", nullptr,
                                                juce::ModalCallbackFunction::create ([f = folder] (int result)
                                                {
                                                    if (result == 1)
                                                        f.revealToUser();
                                                }));
        }

        const juce::File folder;
        const int bits;
        std::unique_ptr<bounce::FilesExport> exporter;
        const bounce::FilesPlan plan;
    };

    void startFilesBounce (Engine& engine, const bounce::FilesPlan& plan, const bounce::Settings& common, const juce::File& folder)
    {
        if (engine.isRecording())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Recording", "Stop recording before bouncing.");
            return;
        }

        goUnlessFallbacks (engine, [&engine, plan, common, folder] { (new FilesJob (engine, plan, common, folder))->launchThread(); });
    }

    void startBounce (Engine& engine, const bounce::Settings& settings, const juce::File& target)
    {
        if (engine.isRecording())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Recording", "Stop recording before bouncing.");
            return;
        }

        goUnlessFallbacks (engine, [&engine, settings, target] { (new BounceJob (engine, settings, target))->launchThread(); });
    }

    //==============================================================================
    /** The options: range, sample rate, bit depth and tail. */
    class BounceOptions final : public juce::Component
    {
    public:
        BounceOptions (Engine& e, const juce::String& name, const std::vector<Track*>& selectedTracks, bool pickSelected)
            : engine (e), projectName (name), chosen (routing::getOutermost (selectedTracks, e.getTracks()))
        {
            juce::StringArray names;

            for (auto* t : chosen)
                names.add (t->name);

            spokenNames = chosen.size() > 3 ? "the " + juce::String ((int) chosen.size()) + " selected tracks"
                                            : names.size() > 1 ? names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1]
                                                               : names.joinIntoString (", ");
            selectedNames = chosen.size() > 3 ? juce::String ((int) chosen.size()) + " tracks" : names.joinIntoString (", ");
            whatBox.addItem ("The mix", mixId);
            whatBox.addItem (chosen.empty() ? juce::String ("Selected tracks (select them in the track headers)")
                                            : "Selected tracks: " + selectedNames, selectedId);
            whatBox.setItemEnabled (selectedId, ! chosen.empty());
            whatBox.addItem ("Every track, one file each (multitracks)", multitracksId);
            whatBox.addItem ("Every bus and lone track, one file each (stems)", stemsId);

            auto& settings = *engine.properties.getUserSettings();
            const auto remembered = settings.getValue ("export.bounceWhat", "mix");   // never "selected": it depends on the song's selection
            whatBox.setSelectedId (pickSelected && ! chosen.empty() ? selectedId : remembered == "tracks" ? multitracksId : remembered == "stems" ? stemsId : mixId,
                                   juce::dontSendNotification);
            whatBox.setTooltip ("What to bounce: the mix, the tracks selected in the headers, or one file per track or per bus.");
            whatBox.onChange = [this] { updateCounts(); };

            soundBox.addItem ("As you hear it (effects, bus, reverb, fader)", asHeardId);
            soundBox.addItem ("Dry: instrument and clips only", dryId);
            soundBox.setSelectedId (settings.getBoolValue ("export.bounceDry", false) ? dryId : asHeardId, juce::dontSendNotification);
            soundBox.setTooltip ("As you hear it: with its effects, bus, reverb and fader, so the files play back your mix (not exactly when a bus has a "
                                 "compressor: see the routing guide). Dry: only the clips and the instrument at 0 dB, centred, for an engineer who mixes it again.");
            const auto& tempo = engine.getTempoMap();
            const auto loop = bounce::getLoopRange (engine), song = bounce::getSongRange (engine);

            rangeBox.addItem (describeRange ("Loop", loop, tempo, engine.getBarLength()), loopId);
            rangeBox.addItem (song.isEmpty() ? juce::String ("Whole song (empty)") : describeRange ("Whole song", song, tempo, engine.getBarLength()), songId);
            rangeBox.setItemEnabled (loopId, loop.getLength() > 0.01);
            rangeBox.setItemEnabled (songId, ! song.isEmpty());
            rangeBox.setSelectedId (engine.loopEnabled.load() && loop.getLength() > 0.01 ? loopId : songId, juce::dontSendNotification);
            rangeBox.onChange = [this]
            {
                tailToggle.setToggleState (rangeBox.getSelectedId() == songId, juce::dontSendNotification);
                updateCounts();
            };

            rateBox.addItem ("44.1 kHz (CD, streaming)", 1);
            rateBox.addItem ("48 kHz (video)", 2);
            rateBox.setSelectedId (juce::approximatelyEqual (settings.getDoubleValue ("export.bounceRate", 44100.0), 48000.0) ? 2 : 1, juce::dontSendNotification);

            bitsBox.addItem ("16-bit (CD)", 16);
            bitsBox.addItem ("24-bit (studio, the usual choice)", 24);
            bitsBox.addItem ("32-bit float (no clipping, larger)", 32);
            bitsBox.setSelectedId (settings.getIntValue ("export.bounceBits", 24), juce::dontSendNotification);

            tailToggle.setButtonText ("Let the end ring out (reverb and released notes, up to 4 s)");
            tailToggle.setToggleState (rangeBox.getSelectedId() == songId, juce::dontSendNotification);
            tailToggle.setTooltip ("Leave it off for a loop, so it repeats seamlessly in another app");

            for (auto [label, text] : { std::pair { &whatLabel, "What" }, std::pair { &soundLabel, "Sound" }, std::pair { &rangeLabel, "Range" }, std::pair { &rateLabel, "Sample rate" }, std::pair { &bitsLabel, "Bit depth" } })
            {
                label->setText (text, juce::dontSendNotification);
                label->setColour (juce::Label::textColourId, theme::textDim);
                addAndMakeVisible (label);
            }

            hint.setFont (juce::FontOptions (12.5f));
            hint.setColour (juce::Label::textColourId, theme::textDim);
            hint.setJustificationType (juce::Justification::topLeft);

            for (juce::Component* c : { (juce::Component*) &whatBox, (juce::Component*) &soundBox, (juce::Component*) &rangeBox, (juce::Component*) &rateBox, (juce::Component*) &bitsBox,
                                        (juce::Component*) &tailToggle, (juce::Component*) &hint, (juce::Component*) &bounceButton,
                                        (juce::Component*) &cancelButton })
                addAndMakeVisible (c);

            guideButton.setTooltip ("Sending stems to an engineer: what a mixing or mastering engineer wants, how to prepare the files and how to check them. Opens the guide.");
            guideButton.onClick = [this] { if (guideOpener) guideOpener (isFiles()); };
            addAndMakeVisible (guideButton);
            bounceButton.setEnabled (! song.isEmpty() || loop.getLength() > 0.01);
            bounceButton.onClick = [this] { chooseFile(); };
            cancelButton.onClick = [this] { close(); };
            updateCounts();
            setSize (520, 320);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);

            for (auto [label, box] : { std::pair { &whatLabel, &whatBox }, std::pair { &soundLabel, &soundBox }, std::pair { &rangeLabel, &rangeBox }, std::pair { &rateLabel, &rateBox }, std::pair { &bitsLabel, &bitsBox } })
            {
                auto row = r.removeFromTop (28);
                label->setBounds (row.removeFromLeft (100));
                box->setBounds (row);
                r.removeFromTop (8);
            }

            tailToggle.setBounds (r.removeFromTop (24).withTrimmedLeft (96));
            r.removeFromTop (4);
            auto buttons = r.removeFromBottom (28);
            cancelButton.setBounds (buttons.removeFromRight (90));
            buttons.removeFromRight (8);
            bounceButton.setBounds (buttons.removeFromRight (110));
            guideButton.setBounds (buttons.removeFromLeft (guideButton.getBestWidthForHeight (28)).withHeight (28));
            hint.setBounds (r);
        }

    private:
        enum { loopId = 1, songId = 2 };
        enum { mixId = 1, selectedId = 2, multitracksId = 3, stemsId = 4 };
        enum { asHeardId = 1, dryId = 2 };

        juce::Range<double> currentRange() const
        {
            return rangeBox.getSelectedId() == loopId ? bounce::getLoopRange (engine) : bounce::getSongRange (engine);
        }

        bool isFiles() const { return whatBox.getSelectedId() == multitracksId || whatBox.getSelectedId() == stemsId; }

        bounce::FilesPlan makePlan() const
        {
            return bounce::planFiles (engine, whatBox.getSelectedId() == stemsId ? bounce::FilesKind::stems : bounce::FilesKind::multitracks,
                                      soundBox.getSelectedId() == dryId, currentRange(), projectName);
        }

        /** The number of files in the two file choices, for the current range, and the hint. */
        void updateCounts()
        {
            const auto range = currentRange();
            const int picked = whatBox.getSelectedId() != 0 ? whatBox.getSelectedId() : (int) lastWhat;
            const auto count = [&] (bounce::FilesKind kind) { return (int) bounce::planFiles (engine, kind, false, range, projectName).files.size(); };
            const auto noun = [] (int n) { return juce::String (n) + (n == 1 ? " file" : " files"); };
            whatBox.changeItemText (multitracksId, "Every track, one file each (multitracks, " + noun (count (bounce::FilesKind::multitracks)) + ")");
            whatBox.changeItemText (stemsId, "Every bus and lone track, one file each (stems, " + noun (count (bounce::FilesKind::stems)) + ")");
            whatBox.setSelectedId (picked, juce::dontSendNotification);   // the box shows the text it had, so a changed text needs this
            lastWhat = picked;
            updateHint();
        }

        void updateHint()
        {
            const auto what = whatBox.getSelectedId();
            soundBox.setEnabled (what == multitracksId);
            soundLabel.setAlpha (what == multitracksId ? 1.0f : 0.4f);
            tailToggle.setEnabled (! isFiles());
            hint.setText (what == selectedId
                              ? "What you hear when you solo " + spokenNames + ": through their buses and the reverb they send to, with the master "
                                "volume. Your solo buttons aren't changed."
                              : what == multitracksId
                                  ? juce::String ("One WAV per track, all starting at the start of the range and the same length, in a new folder with the tempo "
                                                  "and key. Muted tracks and tracks with no clips are left out.")
                              : what == stemsId
                                  ? juce::String ("One WAV per bus at the top and per track outside a bus, each as you hear it when soloed. Together they "
                                                  "play back the mix.")
                              : juce::String ("The mix as you hear it: every track, bus, instrument and effect, and the master volume. "
                                              "The metronome is left out."),
                          juce::dontSendNotification);
        }

        void close()
        {
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState (0);
        }

        void chooseFile()
        {
            const auto range = currentRange();

            bounce::Settings settings;
            settings.startBeat = range.getStart();
            settings.endBeat = range.getEnd();
            settings.sampleRate = rateBox.getSelectedId() == 2 ? 48000.0 : 44100.0;
            settings.bitDepth = bitsBox.getSelectedId();
            settings.tailSeconds = tailToggle.getToggleState() ? 4.0 : 0.0;
            const bool selectedOnly = whatBox.getSelectedId() == selectedId;

            if (selectedOnly)
            {
                settings.focus.mode = BounceFocus::Mode::heardAsSoloed;

                for (auto* t : chosen)
                    settings.focus.tracks.push_back (t);
            }

            auto& props = *engine.properties.getUserSettings();
            props.setValue ("export.bounceRate", settings.sampleRate);
            props.setValue ("export.bounceBits", settings.bitDepth);

            if (! selectedOnly)
                props.setValue ("export.bounceWhat", whatBox.getSelectedId() == multitracksId ? "tracks" : whatBox.getSelectedId() == stemsId ? "stems" : "mix");

            props.setValue ("export.bounceDry", soundBox.getSelectedId() == dryId);

            if (isFiles())
            {
                chooseFolder (settings);
                return;
            }

            const juce::File folder (props.getValue ("export.audioFolder",
                                                     juce::File::getSpecialLocation (juce::File::userMusicDirectory).getFullPathName()));
            const auto fileName = juce::File::createLegalFileName (projectName + (selectedOnly ? " (" + selectedNames + ")" : juce::String())
                                                                         + (rangeBox.getSelectedId() == loopId ? " (loop)" : "")) + ".wav";
            auto& e = engine;
            close();   // deletes this

            chooser = std::make_unique<juce::FileChooser> ("Bounce to WAV", folder.getChildFile (fileName), "*.wav");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [&e, settings] (const juce::FileChooser& fc)
                                  {
                                      auto target = fc.getResult();

                                      if (target == juce::File())
                                          return;

                                      target = target.withFileExtension (".wav");
                                      auto& p = *e.properties.getUserSettings();
                                      p.setValue ("export.audioFolder", target.getParentDirectory().getFullPathName());
                                      p.saveIfNeeded();
                                      startBounce (e, settings, target);
                                  });
        }

        /** The save dialog names the new folder the files go into ("Pop song stems"); it is made when the files are written. */
        void chooseFolder (bounce::Settings common)
        {
            auto plan = makePlan();

            if (! plan.isOk())
            {
                const auto problem = plan.problem.isNotEmpty() ? plan.problem : juce::String ("Nothing to bounce.");
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Bounce to WAV", problem);
                return;
            }

            common.tailSeconds = bounce::filesTailSeconds;
            auto& props = *engine.properties.getUserSettings();
            const juce::File parent (props.getValue ("export.audioFolder", juce::File::getSpecialLocation (juce::File::userMusicDirectory).getFullPathName()));
            const auto name = juce::File::createLegalFileName (projectName + (plan.kind == bounce::FilesKind::stems ? " stems" : " multitracks")
                                                               + (rangeBox.getSelectedId() == loopId ? " (loop)" : ""));
            auto& e = engine;
            close();   // deletes this

            chooser = std::make_unique<juce::FileChooser> (plan.kind == bounce::FilesKind::stems ? "Save the stems in a new folder" : "Save the multitracks in a new folder",
                                                           parent.getChildFile (name), "");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [&e, plan, common] (const juce::FileChooser& fc)
                                  {
                                      const auto folder = fc.getResult();

                                      if (folder == juce::File())
                                          return;

                                      auto& p = *e.properties.getUserSettings();
                                      p.setValue ("export.audioFolder", folder.getParentDirectory().getFullPathName());
                                      p.saveIfNeeded();
                                      startFilesBounce (e, plan, common, folder);
                                  });
        }

        Engine& engine;
        int lastWhat = mixId;
        const juce::String projectName;
        const std::vector<Track*> chosen;   // the outermost selected tracks
        juce::String selectedNames, spokenNames;
        juce::Label whatLabel, soundLabel, rangeLabel, rateLabel, bitsLabel, hint;
        juce::ComboBox whatBox, soundBox, rangeBox, rateBox, bitsBox;
        juce::ToggleButton tailToggle;
        juce::TextButton bounceButton { "Bounce..." }, cancelButton { "Cancel" };
        GuideButton guideButton;
    };
}

void setGuideOpener (std::function<void (bool)> opener)
{
    guideOpener = std::move (opener);
}

void showDialog (Engine& engine, const juce::String& projectName, const std::vector<Track*>& selected, bool pickSelected)
{
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new BounceOptions (engine, projectName, selected, pickSelected));
    options.dialogTitle = "Bounce to WAV";
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}
/** Runs `go` at once, or, when a DawIt Sampler would play the DawIt Synth in the bounce (its SoundFont is missing), first
    lists those tracks and asks whether to bounce anyway: nobody should be surprised by a synthetic piano. */
void goUnlessFallbacks (const Engine& engine, std::function<void()> go)
{
    const auto lines = bounce::getFallbackTracks (engine);

    if (lines.isEmpty())
    {
        go();
        return;
    }

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::WarningIcon, "Some tracks play a stand-in sound",
                                        "These tracks would play the DawIt Synth in the bounce, because their SoundFont isn't there:\n\n"
                                            + lines.joinIntoString ("\n") + "\n\nAdd the SoundFont first (Add SoundFont... in the instrument menu) to hear the real instrument.",
                                        "Bounce anyway", "Cancel", nullptr,
                                        juce::ModalCallbackFunction::create ([start = std::move (go)] (int result) { if (result == 1) start(); }));
}
} // namespace daw::bounceui
