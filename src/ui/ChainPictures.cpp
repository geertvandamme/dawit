#include "ChainPictures.h"

#include "CompressorExplainer.h"
#include "Theme.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInEq.h"
#include "engine/BuiltInGate.h"
#include "engine/BuiltInTune.h"
#include "TunePictures.h"

namespace daw::chainpictures
{
bool hasPicture (const juce::AudioProcessor* plugin)
{
    return dynamic_cast<const BuiltInEq*> (plugin) != nullptr || dynamic_cast<const BuiltInCompressor*> (plugin) != nullptr
           || dynamic_cast<const BuiltInGate*> (plugin) != nullptr || dynamic_cast<const BuiltInTune*> (plugin) != nullptr;
}

float getEqY (juce::Rectangle<float> plot, double db)
{
    return plot.getCentreY() - (float) (juce::jlimit (-12.0, 12.0, db) / 12.0) * plot.getHeight() * 0.5f;
}

void paintCurve (juce::Graphics& g, juce::Rectangle<float> plot, const juce::AudioProcessor* plugin, bool bypassed)
{
    auto* eqPlugin = dynamic_cast<const BuiltInEq*> (plugin);
    auto* compressor = dynamic_cast<const BuiltInCompressor*> (plugin);
    auto* gatePlugin = dynamic_cast<const BuiltInGate*> (plugin);

    // DawIt Tune: the note being sung and how far it is out, live.
    if (auto* tuner = dynamic_cast<const BuiltInTune*> (plugin))
    {
        tunepics::paintMiniTuner (g, plot, tuner->liveSung.load(), bypassed);
        return;
    }

    if (eqPlugin == nullptr && compressor == nullptr && gatePlugin == nullptr)
        return;

    const int points = juce::jmax (2, (int) plot.getWidth());
    juce::Path curve;

    if (eqPlugin != nullptr)
    {
        const double rate = eqPlugin->getSampleRate() > 0.0 ? eqPlugin->getSampleRate() : 48000.0;
        std::array<eq::Band, eq::numBands> bands;

        for (int i = 0; i < eq::numBands; ++i)
            bands[(size_t) i] = eqPlugin->getBand (i);

        g.setColour (theme::gridBar);
        g.fillRect (plot.getX(), getEqY (plot, 0.0), plot.getWidth(), 1.0f);

        for (int x = 0; x <= points; ++x)
        {
            const double hz = eq::minFreq * std::pow (eq::maxFreq / eq::minFreq, (double) x / points);
            double db = eqPlugin->getOutputDb();

            for (auto& b : bands)
                db += eq::getResponseDb (b, hz, rate);

            const juce::Point<float> p { plot.getX() + (float) x * plot.getWidth() / (float) points, getEqY (plot, db) };
            x == 0 ? curve.startNewSubPath (p) : curve.lineTo (p);
        }
    }
    else if (gatePlugin != nullptr)
    {
        // Level in across, out up, from -80 to 0 dB: open, the line follows the diagonal; closed, it drops.
        const auto settings = gatePlugin->getSettings();
        auto toX = [&] (double db) { return plot.getX() + (float) ((db - gate::minThreshold) / -gate::minThreshold) * plot.getWidth(); };
        auto toY = [&] (double db) { return plot.getBottom() - (float) ((juce::jmax (db, (double) gate::minThreshold) - gate::minThreshold) / -gate::minThreshold) * plot.getHeight(); };

        g.setColour (theme::gridBar);
        g.drawLine (toX (gate::minThreshold), toY (gate::minThreshold), toX (0.0), toY (0.0), 1.0f);

        for (int x = 0; x <= points; ++x)
        {
            const double in = gate::minThreshold - gate::minThreshold * x / points;
            const juce::Point<float> p { toX (in), toY (in - gate::getReductionDb (settings, in, in >= settings.thresholdDb)) };
            x == 0 ? curve.startNewSubPath (p) : curve.lineTo (p);
        }
    }
    else
    {
        const auto settings = compressor->getSettings();
        auto toX = [&] (double db) { return plot.getX() + (float) ((db - comp::minThreshold) / -comp::minThreshold) * plot.getWidth(); };
        auto toY = [&] (double db) { return plot.getBottom() - (float) ((db - comp::minThreshold) / -comp::minThreshold) * plot.getHeight(); };

        // The diagonal: what comes out without compression.
        g.setColour (theme::gridBar);
        g.drawLine (toX (comp::minThreshold), toY (comp::minThreshold), toX (0.0), toY (0.0), 1.0f);

        for (int x = 0; x <= points; ++x)
        {
            const double in = comp::minThreshold - comp::minThreshold * x / points;
            const juce::Point<float> p { toX (in), toY (in - comp::getReductionDb (settings, in)) };
            x == 0 ? curve.startNewSubPath (p) : curve.lineTo (p);
        }
    }

    const auto colour = eqPlugin != nullptr ? juce::Colours::white : gatePlugin != nullptr ? juce::Colour (0xff2ec4b6) : comptext::curveColour;   // as in their windows
    g.setColour (bypassed ? theme::textDim.withAlpha (0.6f) : colour.withAlpha (0.9f));
    g.strokePath (curve, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace daw::chainpictures
