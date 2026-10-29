#include "BounceActions.h"
#include "engine/Bounce.h"

namespace daw::bounceui
{
namespace
{
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String clipFileName (const Track& track, const AudioClip& clip)
    {
        return juce::File::createLegalFileName (track.name + (clip.name.isNotEmpty() ? " - " + clip.name : juce::String())) + ".wav";
    }

    //==============================================================================
    /** Renders on its own thread behind a progress bar, then adds the new track on the message thread. Deletes itself. */
    class InPlaceJob final : public juce::ThreadWithProgressWindow
    {
    public:
        InPlaceJob (Engine& e, Selection& s, const bounce::InPlacePlan& p)
            : juce::ThreadWithProgressWindow ("Bouncing " + p.track->name + " in place...", true, true, 10000),
              engine (e), selection (s), plan (p)
        {
            renderer = std::make_unique<bounce::Renderer> (e, bounce::getInPlaceSettings (p));   // here, on the message thread
        }

        void run() override
        {
            outcome = renderer->renderToBuffer (audio, [this] (double progress)
            {
                setProgress (progress);
                return ! threadShouldExit();
            });
        }

        void threadComplete (bool) override
        {
            renderer.reset();   // back to normal playback, on the message thread

            if (! outcome.cancelled)
            {
                if (! outcome.result.wasOk())
                    fail (outcome.result.getErrorMessage());
                else
                {
                    const auto result = bounce::addBouncedTrack (engine, plan, std::move (audio));

                    if (! result.result.wasOk())
                        fail (result.result.getErrorMessage());
                    else
                    {
                        selection.select (result.track, result.clip);

                        if (result.note.isNotEmpty())
                            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Bounced in place", result.note);
                    }
                }
            }

            delete this;
        }

    private:
        static void fail (const juce::String& message)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Bounce in place failed", message);
        }

        Engine& engine;
        Selection& selection;
        const bounce::InPlacePlan plan;
        std::unique_ptr<bounce::Renderer> renderer;
        juce::AudioBuffer<float> audio;
        bounce::Outcome outcome;
    };
}

void bounceInPlace (Engine& engine, Selection& selection, Track& track, Clip* clip)
{
    if (engine.isRecording())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Recording", "Stop recording before bouncing.");
        return;
    }

    const auto plan = bounce::planInPlace (engine, track, clip);

    if (! plan.isOk())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Bounce in place", plan.problem);
        return;
    }

    (new InPlaceJob (engine, selection, plan))->launchThread();
}

void exportClipAsWav (Engine& engine, const Track& track, const AudioClip& clip)
{
    auto& settings = *engine.properties.getUserSettings();
    const juce::File folder (settings.getValue ("export.audioFolder",
                                                juce::File::getSpecialLocation (juce::File::userMusicDirectory).getFullPathName()));

    // The clip's audio is copied now, so a later edit while the dialog is open doesn't matter.
    auto copy = std::make_shared<AudioClip>();
    copy->audio = clip.audio;
    copy->sourceSampleRate = clip.sourceSampleRate;
    copy->gainDb = clip.gainDb;

    chooser = std::make_unique<juce::FileChooser> ("Export clip as WAV file", folder.getChildFile (clipFileName (track, clip)), "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [&settings, copy] (const juce::FileChooser& fc)
                          {
                              auto target = fc.getResult();

                              if (target == juce::File())
                                  return;

                              target = target.withFileExtension (".wav");
                              settings.setValue ("export.audioFolder", target.getParentDirectory().getFullPathName());
                              settings.saveIfNeeded();

                              if (const auto result = bounce::writeClipToWav (*copy, target); result.failed())
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", result.getErrorMessage());
                          });
}

void dragClipOut (Engine&, const Track& track, const AudioClip& clip, juce::Component& source)
{
    // A fresh folder per drag, as the receiving app may still be reading the previous file.
    // Folders from earlier drags are cleaned up once they are an hour old.
    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DawIt Audio");

    for (auto& old : root.findChildFiles (juce::File::findDirectories, false))
        if (old.getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::hours (1))
            old.deleteRecursively();

    const auto folder = root.getNonexistentChildFile ("drag", {}, false);
    folder.createDirectory();
    const auto file = folder.getChildFile (clipFileName (track, clip));

    if (const auto result = bounce::writeClipToWav (clip, file); result.failed())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed", result.getErrorMessage());
        return;
    }

    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, &source);
}
} // namespace daw::bounceui
