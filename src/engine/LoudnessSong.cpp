#include "LoudnessSong.h"

namespace daw::loudness
{
SongMeasurement measureSong (bounce::Renderer& renderer, double sampleRate, const std::atomic<bool>* cancel, const std::function<void (double)>& progress)
{
    Meter meter;
    meter.prepare (sampleRate);
    SongMeasurement result;

    const auto outcome = renderer.renderToCallback (
        [&meter] (const juce::AudioBuffer<float>& buffer, int start, int count)
        {
            const float* left = buffer.getReadPointer (0, start);
            const float* right = buffer.getReadPointer (1, start);
            for (int i = 0; i < count; ++i)
                meter.push (left[i], right[i], 2);
        },
        [&] (double done)
        {
            if (progress != nullptr) progress (done);
            return cancel == nullptr || ! cancel->load();
        });

    result.result = outcome.result;
    result.cancelled = outcome.cancelled;
    result.readings = meter.getReadings();
    return result;
}
} // namespace daw::loudness
