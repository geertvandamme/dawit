#include "BounceDialog.h"
#include "engine/Bounce.h"
#include "engine/Routing.h"
#include "Theme.h"

namespace daw::bounceui
{
namespace
{
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String describeRange (const juce::String& name, juce::Range<double> range, const TempoMap& tempo)
    {
        const int firstBar = (int) std::floor (range.getStart() / 4.0) + 1, lastBar = (int) std::ceil (range.getEnd() / 4.0);
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

    void startBounce (Engine& engine, const bounce::Settings& settings, const juce::File& target)
    {
        if (engine.isRecording())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Recording", "Stop recording before bouncing.");
            return;
        }

        (new BounceJob (engine, settings, target))->launchThread();
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
            whatBox.setSelectedId (pickSelected && ! chosen.empty() ? selectedId : mixId, juce::dontSendNotification);
            whatBox.setTooltip ("What to bounce: the mix, the tracks selected in the headers, or one file per track or per bus.");
            whatBox.onChange = [this] { updateHint(); };

            auto& settings = *engine.properties.getUserSettings();
            const auto& tempo = engine.getTempoMap();
            const auto loop = bounce::getLoopRange (engine), song = bounce::getSongRange (engine);

            rangeBox.addItem (describeRange ("Loop", loop, tempo), loopId);
            rangeBox.addItem (song.isEmpty() ? juce::String ("Whole song (empty)") : describeRange ("Whole song", song, tempo), songId);
            rangeBox.setItemEnabled (loopId, loop.getLength() > 0.01);
            rangeBox.setItemEnabled (songId, ! song.isEmpty());
            rangeBox.setSelectedId (engine.loopEnabled.load() && loop.getLength() > 0.01 ? loopId : songId, juce::dontSendNotification);
            rangeBox.onChange = [this] { tailToggle.setToggleState (rangeBox.getSelectedId() == songId, juce::dontSendNotification); };

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

            for (auto [label, text] : { std::pair { &whatLabel, "What" }, std::pair { &rangeLabel, "Range" }, std::pair { &rateLabel, "Sample rate" }, std::pair { &bitsLabel, "Bit depth" } })
            {
                label->setText (text, juce::dontSendNotification);
                label->setColour (juce::Label::textColourId, theme::textDim);
                addAndMakeVisible (label);
            }

            hint.setFont (juce::FontOptions (12.5f));
            hint.setColour (juce::Label::textColourId, theme::textDim);
            hint.setJustificationType (juce::Justification::topLeft);

            for (juce::Component* c : { (juce::Component*) &whatBox, (juce::Component*) &rangeBox, (juce::Component*) &rateBox, (juce::Component*) &bitsBox,
                                        (juce::Component*) &tailToggle, (juce::Component*) &hint, (juce::Component*) &bounceButton,
                                        (juce::Component*) &cancelButton })
                addAndMakeVisible (c);

            bounceButton.setEnabled (! song.isEmpty() || loop.getLength() > 0.01);
            bounceButton.onClick = [this] { chooseFile(); };
            cancelButton.onClick = [this] { close(); };
            updateHint();
            setSize (520, 256);
        }

        void paint (juce::Graphics& g) override { g.fillAll (theme::panel); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16);

            for (auto [label, box] : { std::pair { &whatLabel, &whatBox }, std::pair { &rangeLabel, &rangeBox }, std::pair { &rateLabel, &rateBox }, std::pair { &bitsLabel, &bitsBox } })
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
            hint.setBounds (r);
        }

    private:
        enum { loopId = 1, songId = 2 };
        enum { mixId = 1, selectedId = 2 };

        void updateHint()
        {
            hint.setText (whatBox.getSelectedId() == selectedId
                              ? "What you hear when you solo " + spokenNames + ": through their buses and the reverb they send to, with the master "
                                "volume. Your solo buttons aren't changed."
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
            const auto range = rangeBox.getSelectedId() == loopId ? bounce::getLoopRange (engine) : bounce::getSongRange (engine);

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

        Engine& engine;
        const juce::String projectName;
        const std::vector<Track*> chosen;   // the outermost selected tracks
        juce::String selectedNames, spokenNames;
        juce::Label whatLabel, rangeLabel, rateLabel, bitsLabel, hint;
        juce::ComboBox whatBox, rangeBox, rateBox, bitsBox;
        juce::ToggleButton tailToggle;
        juce::TextButton bounceButton { "Bounce..." }, cancelButton { "Cancel" };
    };
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
} // namespace daw::bounceui
