#include "ChainPictures.h"

#include "CompressorExplainer.h"
#include "Theme.h"
#include "engine/BuiltInCompressor.h"
#include "ArpeggiatorGuide.h"
#include "engine/BuiltInArpeggiator.h"
#include "engine/BuiltInEq.h"
#include "engine/BuiltInGate.h"
#include "engine/BuiltInSlicer.h"
#include "engine/BuiltInSpectralFreeze.h"
#include "engine/BuiltInStepFilter.h"
#include "SpectralFreezeGuide.h"
#include "SlicerGuide.h"
#include "StepFilterGuide.h"
#include "engine/BuiltInHarmoniser.h"
#include "engine/BuiltInTune.h"
#include "TunePictures.h"
#include "MeterEditor.h"
#include "engine/BuiltInMeter.h"

namespace daw::chainpictures
{
bool hasPicture (const juce::AudioProcessor* plugin)
{
    return dynamic_cast<const BuiltInEq*> (plugin) != nullptr || dynamic_cast<const BuiltInCompressor*> (plugin) != nullptr
           || dynamic_cast<const BuiltInGate*> (plugin) != nullptr || dynamic_cast<const BuiltInTune*> (plugin) != nullptr || dynamic_cast<const BuiltInHarmoniser*> (plugin) != nullptr
           || dynamic_cast<const BuiltInSlicer*> (plugin) != nullptr || dynamic_cast<const BuiltInStepFilter*> (plugin) != nullptr
           || dynamic_cast<const BuiltInSpectralFreeze*> (plugin) != nullptr || dynamic_cast<const BuiltInArpeggiator*> (plugin) != nullptr
           || dynamic_cast<const BuiltInMeter*> (plugin) != nullptr;
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

    // DawIt Meter: the short-term loudness as a number, a thin bar with the target tick, and a red dot over the ceiling.
    if (auto* meter = dynamic_cast<const BuiltInMeter*> (plugin))
    {
        const auto settings = meter->getSettings();
        const auto readings = meter->getReadings();
        auto area = plot.reduced (1.0f, 1.0f);
        auto bar = area.removeFromBottom (5.0f).reduced (2.0f, 0.0f);
        g.setColour (theme::textDim.withAlpha (0.25f));
        g.fillRect (bar);
        auto xFor = [&] (double lufs) { return bar.getX() + (float) juce::jlimit (0.0, 1.0, (lufs + 60.0) / 60.0) * bar.getWidth(); };

        if (readings.shortTerm)
        {
            g.setColour (bypassed ? theme::textDim : meterui::colour);
            g.fillRect (bar.withRight (xFor (*readings.shortTerm)));
        }

        if (settings.showTarget)
        {
            g.setColour (theme::text);
            g.fillRect (xFor (settings.targetLufs) - 0.5f, bar.getY() - 2.0f, 1.0f, bar.getHeight() + 4.0f);
        }

        area.removeFromBottom (2.0f);
        g.setColour (bypassed ? theme::textDim : theme::text);
        g.setFont (juce::FontOptions (juce::jmin (12.0f, area.getHeight() * 0.8f), juce::Font::bold));
        g.drawFittedText (readings.shortTerm ? juce::String (*readings.shortTerm, 1) : juce::String::fromUTF8 ("\xe2\x80\x93"), area.withTrimmedRight (6.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);

        if (readings.truePeak && *readings.truePeak > settings.ceilingDbtp && ! bypassed)
        {
            g.setColour (theme::meterRed);
            g.fillEllipse (area.getRight() - 6.0f, area.getY() + 1.0f, 5.0f, 5.0f);
        }
        return;
    }

    // DawIt Harmoniser: the lead and each voice as a short bar at its note, on a column of rows around the lead.
    if (auto* harmoniser = dynamic_cast<const BuiltInHarmoniser*> (plugin))
    {
        const float lead = harmoniser->liveLead.load();

        if (lead < 0.0f)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (juce::jmin (13.0f, plot.getHeight() * 0.5f), juce::Font::bold));
            g.drawText (juce::String::fromUTF8 ("\xe2\x80\x93"), plot, juce::Justification::centred);
            return;
        }

        const float barWidth = plot.getWidth() / 3.0f - 2.0f;
        auto yFor = [&] (float pitch) { return plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, (pitch - lead) / 14.0f) * (plot.getHeight() * 0.5f - 2.0f); };
        g.setColour (bypassed ? theme::textDim : theme::text);
        g.fillRect (plot.getX() + 1.0f, yFor (lead) - 1.5f, barWidth, 3.0f);

        for (int v = 0; v < harm::numVoices; ++v)
        {
            const float voice = harmoniser->liveVoice[v].load();

            if (voice < 0.0f)
                continue;

            g.setColour (bypassed ? theme::textDim.withAlpha (0.6f) : tunepics::getVoiceColour (v));
            g.fillRect (plot.getX() + 1.0f + (barWidth + 3.0f) * (float) (v + 1), yFor (voice) - 1.5f, barWidth, 3.0f);
        }

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

    // DawIt Arpeggiator: the steps of its pattern, coloured by kind, the playing one lit.
    if (auto* arpeggiator = dynamic_cast<const BuiltInArpeggiator*> (plugin))
    {
        const auto settings = arpeggiator->getSettings();
        const int playing = arpeggiator->playingStep.load();
        const float cell = plot.getWidth() / (float) arp::maxSteps;

        for (int i = 0; i < arp::maxSteps; ++i)
        {
            const bool used = i < settings.steps;
            const auto kind = settings.kinds[(size_t) i];
            auto colour = arptext::getKindColour (kind);
            colour = bypassed ? theme::textDim.withAlpha (0.4f) : colour.withAlpha (used ? 0.95f : 0.25f);

            if (! bypassed && i == playing)
                colour = juce::Colours::white;

            g.setColour (colour);
            const float height = plot.getHeight() * (kind == arp::StepKind::rest ? 0.2f : kind == arp::StepKind::tie ? 0.4f : kind == arp::StepKind::chord ? 0.9f : 0.65f)
                                 + (settings.accents[(size_t) i] ? 3.0f : 0.0f);
            g.fillRect (plot.getX() + cell * (float) i + 0.5f, plot.getBottom() - juce::jmin (plot.getHeight(), height), juce::jmax (1.0f, cell - 1.0f), juce::jmin (plot.getHeight(), height));
        }

        return;
    }

    // DawIt Spectral Freeze: the hold's spectrum, or the live sound's while nothing is held.
    if (auto* freeze = dynamic_cast<const BuiltInSpectralFreeze*> (plugin))
    {
        sfztext::paintMiniSpectrum (g, plot, *freeze, bypassed);
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
