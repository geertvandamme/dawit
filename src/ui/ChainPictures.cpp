#include "ChainPictures.h"

#include "CompressorExplainer.h"
#include "Theme.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInEq.h"
#include "engine/BuiltInGate.h"
#include "engine/BuiltInSlicer.h"
#include "engine/BuiltInStepFilter.h"
#include "SlicerGuide.h"
#include "StepFilterGuide.h"
#include "engine/BuiltInTune.h"
#include "TunePictures.h"

namespace daw::chainpictures
{
bool hasPicture (const juce::AudioProcessor* plugin)
{
    return dynamic_cast<const BuiltInEq*> (plugin) != nullptr || dynamic_cast<const BuiltInCompressor*> (plugin) != nullptr
           || dynamic_cast<const BuiltInGate*> (plugin) != nullptr || dynamic_cast<const BuiltInTune*> (plugin) != nullptr
           || dynamic_cast<const BuiltInSlicer*> (plugin) != nullptr || dynamic_cast<const BuiltInStepFilter*> (plugin) != nullptr;
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

    // DawIt Slicer: the steps of its pattern, coloured by mode.
    if (auto* slicer = dynamic_cast<const BuiltInSlicer*> (plugin))
    {
        const auto settings = slicer->getSettings();
        const float cell = plot.getWidth() / (float) slc::maxSteps;

        for (int i = 0; i < slc::maxSteps; ++i)
        {
            const bool used = i < settings.steps;
            const auto mode = settings.modes[(size_t) i];
            auto colour = mode == slc::Mode::mute ? theme::gridBar : slctext::getModeColour (mode);
            colour = bypassed ? theme::textDim.withAlpha (0.4f) : colour.withAlpha (used ? 0.95f : 0.25f);
            g.setColour (colour);
            const float height = plot.getHeight() * (mode == slc::Mode::mute ? 0.25f : 0.6f);
            g.fillRect (plot.getX() + cell * (float) i + 0.5f, plot.getBottom() - plot.getHeight() * 0.2f - height, juce::jmax (1.0f, cell - 1.0f), height);
        }

        return;
    }

    // DawIt Step Filter: the bars of its pattern, each as high as its cutoff, the playing one lit.
    if (auto* stepFilter = dynamic_cast<const BuiltInStepFilter*> (plugin))
    {
        const auto settings = stepFilter->getSettings();
        const int playing = stepFilter->playingStep.load();
        const float cell = plot.getWidth() / (float) stf::maxSteps;

        for (int i = 0; i < stf::maxSteps; ++i)
        {
            const bool used = i < settings.steps;
            auto barColour = bypassed ? theme::textDim.withAlpha (0.4f) : stftext::colour.withAlpha (used ? 0.8f : 0.2f);

            if (! bypassed && i == playing)
                barColour = juce::Colours::white;

            const float height = juce::jmax (2.0f, (plot.getHeight() - 4.0f) * settings.heights[(size_t) i] / 100.0f);
            g.setColour (barColour);
            g.fillRect (plot.getX() + cell * (float) i + 0.5f, plot.getBottom() - 2.0f - height, juce::jmax (1.0f, cell - 1.0f), height);
        }

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
