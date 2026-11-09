#include "LoudnessActions.h"
#include "engine/Bounce.h"
#include "BounceDialog.h"
#include "engine/LoudnessSong.h"

namespace daw::loudnessui
{
namespace
{
    std::optional<metertext::SongResult> last;
    constexpr double measureRate = 48000.0;

    /** "The whole song (bar 1 to bar 37, 3:12):" or "The loop (bars 9 to 16, 0:16):". */
    juce::String describe (const Engine& engine, juce::Range<double> range, bool loop)
    {
        const double barLength = engine.getBarLength();
        const int first = (int) std::floor (range.getStart() / barLength + 1.0e-6) + 1, lastBar = juce::jmax (first, (int) std::round (range.getEnd() / barLength));   // the last bar inside the range
        const auto length = loudness::formatClock (engine.getTempoMap().getSecondsBetween (range.getStart(), range.getEnd()));
        return loop ? "The loop (bars " + juce::String (first) + " to " + juce::String (lastBar) + ", " + length + "):"
                    : "The whole song (bar " + juce::String (first) + " to bar " + juce::String (lastBar) + ", " + length + "):";
    }

    bool getRange (Engine& engine, juce::Range<double>& range, bool& loop)
    {
        if (engine.isRecording())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Recording", "Stop recording before measuring the loudness.");
            return false;
        }

        range = bounce::getDefaultRange (engine);
        loop = engine.loopEnabled.load() && bounce::getLoopRange (engine).getLength() > 0.01;

        if (range.getLength() < 0.01)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Nothing to measure", "The song is empty. Add some notes or audio, then measure its loudness.");
            return false;
        }

        return true;
    }

    bounce::Settings makeSettings (juce::Range<double> range)
    {
        bounce::Settings settings;
        settings.startBeat = range.getStart();
        settings.endBeat = range.getEnd();
        settings.sampleRate = measureRate;
        return settings;
    }

    /** Measures on its own thread behind a progress bar. Deletes itself. */
    class MeasureJob final : public juce::ThreadWithProgressWindow
    {
    public:
        MeasureJob (Engine& e, const bounce::Settings& settings, juce::String scopeText, std::function<void (const metertext::SongResult&)> done)
            : juce::ThreadWithProgressWindow ("Measuring loudness...", true, true, 10000),
              scope (std::move (scopeText)), onFinished (std::move (done))
        {
            renderer = std::make_unique<bounce::Renderer> (e, settings);   // here, on the message thread
        }

        void run() override
        {
            outcome = loudness::measureSong (*renderer, measureRate, &cancel, [this] (double amountDone)
            {
                setProgress (amountDone);
                if (threadShouldExit()) cancel = true;
            });
        }

        void threadComplete (bool) override
        {
            renderer.reset();   // back to normal playback, on the message thread

            if (! outcome.result.wasOk())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Measuring failed", outcome.result.getErrorMessage());
            }
            else if (! outcome.cancelled)
            {
                last = metertext::SongResult { outcome.readings, scope };

                if (onFinished != nullptr)
                    onFinished (*last);
            }

            delete this;
        }

    private:
        std::unique_ptr<bounce::Renderer> renderer;
        std::atomic<bool> cancel { false };
        loudness::SongMeasurement outcome;
        juce::String scope;
        std::function<void (const metertext::SongResult&)> onFinished;
    };
}

void measureSong (Engine& engine, std::function<void (const metertext::SongResult&)> onFinished)
{
    juce::Range<double> range;
    bool loop = false;

    if (! getRange (engine, range, loop))
        return;

    bounceui::goUnlessFallbacks (engine, [&engine, settings = makeSettings (range), text = describe (engine, range, loop), done = std::move (onFinished)]() mutable
    {
        (new MeasureJob (engine, settings, text, std::move (done)))->launchThread();
    });
}

std::optional<metertext::SongResult> measureSongNow (Engine& engine)
{
    juce::Range<double> range;
    bool loop = false;

    if (! getRange (engine, range, loop))
        return std::nullopt;

    std::optional<metertext::SongResult> result;
    {
        bounce::Renderer renderer (engine, makeSettings (range));
        const auto outcome = loudness::measureSong (renderer, measureRate);

        if (outcome.result.wasOk() && ! outcome.cancelled)
            result = metertext::SongResult { outcome.readings, describe (engine, range, loop) };
    }

    if (result)
        last = result;

    return result;
}

std::optional<metertext::SongResult> getLast() { return last; }

void clearLast() { last.reset(); }
} // namespace daw::loudnessui
