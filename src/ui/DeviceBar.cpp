#include <juce_dsp/juce_dsp.h>

#include "GuideButton.h"
#include "DeviceBar.h"
#include "ChainPictures.h"
#include "CompressorExplainer.h"
#include "PluginActions.h"
#include "engine/BuiltInGate.h"
#include "engine/BuiltInStereo.h"
#include "engine/BuiltInTune.h"
#include "StereoScope.h"
#include "engine/Routing.h"

namespace daw
{
namespace
{
    /** How many tracks (not buses) play into the bus, directly or through buses in it; for a return, how many send to it. */
    int countTracksInto (const Track& bus, const Tracks& all)
    {
        if (bus.isReturn())
            return (int) routing::getSenders (bus, all).size();

        return (int) std::count_if (all.begin(), all.end(), [&bus] (auto& t) { return ! t->hasInput() && routing::playsInto (*t, bus); });
    }

    /** "Kick, Snare, Hats" or "Kick, Snare, Hats +2": the names of the bus's own tracks, or of the tracks that send to a return. */
    juce::String describeFeed (const Track& bus, const Tracks& all)
    {
        const auto members = bus.isReturn() ? routing::getSenders (bus, all) : routing::getMembers (bus, all);
        juce::StringArray names;

        for (size_t i = 0; i < members.size() && i < 3; ++i)
            names.add (members[i]->name);

        auto text = names.joinIntoString (", ");

        if (members.size() > 3)
            text << " +" << juce::String ((int) members.size() - 3);

        return text;
    }
}

/** One step of the chain: the built-in synth (DawIt Synth), a plugin, the track's fader, a bus it plays into, or
    the master. A bus's own chain starts with what feeds it (its tracks added up). */
class DeviceBar::Chip final : public juce::Component, public juce::SettableTooltipClient
{
public:
    static constexpr int tagWidth = 24, barWidth = 28, meterWidth = 8 + tagWidth + barWidth + 48, textRefreshTicks = 10;
    static constexpr int pictureWidth = 50, volumeTextWidth = 92, badgeWidth = 54;

    enum class Kind { synth, plugin, fader, master, busInput, bus, send };

    Chip (DeviceBar& o, Track& t, const PluginSlot* s, Kind k, Track* busOnTheWay = nullptr)
        : owner (o), track (t), slot (s), kind (k), busTrack (busOnTheWay)
    {
        switch (kind)
        {
            case Kind::synth:
                setTooltip ("DawIt Synth, the built-in instrument - click to pick its sound. Right-click: load an instrument plugin");
                break;
            case Kind::plugin:
                setTooltip (slot->instance->getName() + " (" + plugins::getFormatLabel (slot->instance->getPluginDescription())
                            + ") - click to open the editor. Dot: bypass, x: remove, "
                            + (slot->isInstrument ? "" : "drag to reorder, ") + "right-click: more");
                break;
            case Kind::fader:
                setTooltip ("The track's volume (fader), after the effects: the level going in and coming out. Click to show the mixer");
                break;
            case Kind::master:
                setTooltip ("The master: all tracks together, then the master volume, out to your speakers. Click to show the mixer");
                break;
            case Kind::busInput:
            case Kind::bus:
            case Kind::send:
                break;   // worded by getTooltip(), as the tracks in the bus and the sends change
        }
    }

    /** Kind::send: the send this chip shows (null once it is gone). */
    Send* getSend() const { return kind == Kind::send ? routing::findSend (track, *busTrack) : nullptr; }

    Kind getKind() const noexcept { return kind; }

    /** Kind::send: the return it goes to. */
    Track& getReturn() const noexcept { return *busTrack; }

    juce::String getTooltip() override
    {
        if (kind == Kind::send)
        {
            const auto* send = getSend();
            return "Send to " + busTrack->name + ": a copy of this track, taken " + (send != nullptr && send->preFader.load() ? "before" : "after")
                   + " its fader, at " + (send != nullptr ? juce::String (send->levelDb.load(), 1) : juce::String ("-60.0")) + " dB, goes to the " + busTrack->name
                   + " return. Drag up or down to change the level; click to see the return's chain; right-click for pre- or post-fader.";
        }

        if (kind == Kind::busInput && track.isReturn())
            return "What comes into " + track.name + ": the sends of " + juce::String (countTracksInto (track, owner.engine.getTracks()))
                   + (countTracksInto (track, owner.engine.getTracks()) == 1 ? " track" : " tracks") + ", added up. Click for a list of them.";

        if (kind == Kind::busInput)
            return "What comes into " + track.name + ": its " + juce::String (countTracksInto (track, owner.engine.getTracks()))
                   + " tracks added up. Click for a list of them.";

        if (kind == Kind::bus)
        {
            juce::StringArray others;

            for (auto* m : routing::getMembers (*busTrack, owner.engine.getTracks()))
                if (m != &track && ! routing::playsInto (track, *m))
                    others.add (m->name);

            const auto together = others.isEmpty() ? juce::String() : ", together with " + others.joinIntoString (", ", 0, others.size() - 1)
                                                                       + (others.size() > 1 ? " and " : "") + others[others.size() - 1];
            return busTrack->name + ": " + (track.isBus() ? "this bus plays into the " : "this track plays into the ") + busTrack->name + " bus" + together
                   + ". In: all of them added up; out: after the bus's effects and fader. Click to see the bus's own chain.";
        }

        if (getBadgeArea().contains (getMouseXYRelative()) && hasBadge() && shownChange.has_value())
            return getBadgeTooltip();

        return SettableTooltipClient::getTooltip();
    }

    /** Null for the built-in synth, or once the plugin has been removed from the track. */
    PluginSlot* getSlot() const
    {
        for (auto& p : track.plugins)
            if (p.get() == slot)
                return p.get();

        return nullptr;
    }

    int getPreferredWidth() const
    {
        if (kind == Kind::send)
        {
            const auto font = juce::Font (juce::FontOptions (11.5f, juce::Font::bold));
            const auto* send = getSend();
            return 14 + juce::GlyphArrangement::getStringWidthInt (font, getSendText()) + 8 + (send != nullptr && send->preFader.load() ? 28 : 0) + 8 + sendMeterWidth + 8;
        }

        if (kind == Kind::fader || kind == Kind::master)
            return 10 + volumeTextWidth + 8 + meterWidth;

        if (kind == Kind::busInput || kind == Kind::bus)
        {
            const auto font = juce::Font (juce::FontOptions (13.0f, juce::Font::bold));
            return juce::jlimit (130, 240, juce::GlyphArrangement::getStringWidthInt (font, getDisplayName()) + 52) + meterWidth;
        }

        const auto font = juce::Font (juce::FontOptions (13.0f, juce::Font::bold));
        return juce::jlimit (110, 220, juce::GlyphArrangement::getStringWidthInt (font, getDisplayName()) + 52) + meterWidth
               + (hasPicture() ? pictureWidth + 6 : 0) + (hasBadge() ? 40 : 0);
    }

    /** Effects show how much louder or quieter they make the sound. */
    bool hasBadge() const
    {
        auto* s = getSlot();
        return kind == Kind::plugin && s != nullptr && ! s->isInstrument;
    }

    /** DawIt EQ, Compressor and Gate can keep their output as loud as their input. */
    bool canMatch() const { return getEq() != nullptr || getCompressor() != nullptr || getGate() != nullptr || getStereo() != nullptr; }

    bool isMatching() const
    {
        if (auto* eq = getEq()) return eq->isMatchingLevel();
        if (auto* c = getCompressor()) return c->isMatchingLevel();
        if (auto* gate = getGate()) return gate->isMatchingLevel();
        if (auto* stereo = getStereo()) return stereo->isMatchingLevel();
        return false;
    }

    void setMatching (bool on)
    {
        if (auto* eq = getEq()) eq->setMatchingLevel (on);
        if (auto* c = getCompressor()) c->setMatchingLevel (on);
        if (auto* gate = getGate()) gate->setMatchingLevel (on);
        if (auto* stereo = getStereo()) stereo->setMatchingLevel (on);
        repaint();
    }

    /** True while the track is muted, a bus it plays into is muted, or another track is soloed. */
    bool isSilenced() const
    {
        return routing::getSilence (track, owner.engine.getTracks()) != routing::Silence::none;
    }

    /** This step's levels of the last third of a second, for the gain staging. */
    gainstaging::Stop getStop() const
    {
        using gainstaging::StopKind;
        gainstaging::Stop stop;
        auto* s = getSlot();
        const auto toDb = [] (float level) { return juce::Decibels::gainToDecibels (level, -100.0f); };

        stop.name = kind == Kind::fader ? juce::String ("Fader") : kind == Kind::master ? juce::String ("Master")
                  : kind == Kind::busInput ? track.name : (kind == Kind::bus || kind == Kind::send) ? busTrack->name : getDisplayName();
        stop.kind = kind == Kind::fader ? StopKind::fader : kind == Kind::master ? StopKind::master
                  : kind == Kind::busInput ? StopKind::busInput : kind == Kind::bus ? StopKind::bus : kind == Kind::send ? StopKind::send
                  : (kind == Kind::synth || (s != nullptr && s->isInstrument)) ? StopKind::instrument : StopKind::effect;
        stop.inDb = stop.kind == StopKind::instrument || stop.kind == StopKind::busInput ? -100.0f : toDb (textIn);
        stop.numTracks = kind == Kind::busInput ? countTracksInto (track, owner.engine.getTracks())
                       : kind == Kind::bus ? countTracksInto (*busTrack, owner.engine.getTracks()) : 0;
        stop.busHasEffects = kind == Kind::busInput ? ! track.plugins.empty() : kind == Kind::bus ? ! busTrack->plugins.empty() : true;
        stop.outDb = toDb (textOut);
        stop.changeDb = hasBadge() ? shownChange : std::nullopt;
        stop.settingDb = kind == Kind::fader ? track.volumeDb.load() : kind == Kind::master ? owner.engine.masterVolumeDb.load()
                       : kind == Kind::bus ? busTrack->volumeDb.load() : 0.0f;
        stop.bypassed = s != nullptr && s->bypassed.load();
        stop.canMatch = canMatch();
        stop.matching = isMatching();
        stop.isCompressor = getCompressor() != nullptr;
        stop.isReturn = kind == Kind::busInput && track.isReturn();

        if (kind == Kind::send)
        {
            const auto* send = getSend();
            stop.settingDb = send != nullptr ? send->levelDb.load() : sendOffDb;
            stop.preFader = send != nullptr && send->preFader.load();
            stop.inDb = -100.0f;
        }

        return stop;
    }

    float getOutLevel() const { return textOut; }

    /** DawIt EQ or DawIt Compressor: they show a picture of their settings. */
    BuiltInEq* getEq() const { auto* s = getSlot(); return s != nullptr ? dynamic_cast<BuiltInEq*> (s->instance.get()) : nullptr; }
    BuiltInCompressor* getCompressor() const { auto* s = getSlot(); return s != nullptr ? dynamic_cast<BuiltInCompressor*> (s->instance.get()) : nullptr; }
    BuiltInGate* getGate() const { auto* s = getSlot(); return s != nullptr ? dynamic_cast<BuiltInGate*> (s->instance.get()) : nullptr; }
    BuiltInStereo* getStereo() const { auto* s = getSlot(); return s != nullptr ? dynamic_cast<BuiltInStereo*> (s->instance.get()) : nullptr; }
    BuiltInTune* getTune() const { auto* s = getSlot(); return s != nullptr ? dynamic_cast<BuiltInTune*> (s->instance.get()) : nullptr; }
    bool hasPicture() const { return getEq() != nullptr || getCompressor() != nullptr || getGate() != nullptr || getStereo() != nullptr || getTune() != nullptr; }

    /** Follows the settings and the signal, and redraws the picture when either changed. Called by
        the bar's timer. */
    void updatePicture()
    {
        if (getStereo() != nullptr || getTune() != nullptr) repaint (getPictureArea());
        if (updateSignal())
            repaint (getPictureArea());

        std::vector<float> settings;

        if (auto* eq = getEq())
        {
            for (int i = 0; i < eq::numBands; ++i)
            {
                const auto b = eq->getBand (i);
                settings.insert (settings.end(), { b.on ? 1.0f : 0.0f, (float) b.type, b.freq, b.gainDb, b.q, (float) b.slope });
            }

            settings.push_back (eq->getOutputDb());
        }
        else if (auto* c = getCompressor())
        {
            const auto s = c->getSettings();
            settings = { s.thresholdDb, s.ratio, s.kneeDb };
        }
        else if (auto* gate = getGate())
        {
            const auto s = gate->getSettings();
            settings = { s.thresholdDb, s.hysteresisDb, s.rangeDb, s.ratio };
        }

        if (auto* s = getSlot())
            settings.push_back (s->bypassed.load() ? 1.0f : 0.0f);

        if (settings != shownSettings)
        {
            shownSettings = std::move (settings);
            repaint (getPictureArea());
        }
    }

    /** Reads the levels the audio thread measured since the last call. Called by the bar's timer. */
    void updateMeters()
    {
        auto* s = getSlot();

        if (kind == Kind::plugin && s == nullptr)
            return;

        float in = 0.0f, out = 0.0f;

        switch (kind)
        {
            case Kind::synth:  out = track.builtInSynthPeak.exchange (0.0f); break;
            case Kind::plugin: in = s->inputPeak.exchange (0.0f); out = s->outputPeak.exchange (0.0f); break;
            case Kind::fader:  in = track.faderInputPeak.exchange (0.0f); out = track.faderOutputPeak.exchange (0.0f); break;
            case Kind::master: in = owner.engine.masterInputPeak.exchange (0.0f); out = owner.engine.masterOutputPeak.exchange (0.0f); break;
            case Kind::busInput: out = track.busInputPeak.exchange (0.0f); break;
            case Kind::bus:    in = busTrack->busInputPeak.exchange (0.0f); out = busTrack->faderOutputPeak.exchange (0.0f); break;
            case Kind::send:   if (auto* send = getSend()) out = send->peak.exchange (0.0f); break;
        }

        // The fader and master show their volume, which the mixer and track header change.
        if (kind == Kind::fader || kind == Kind::master)
        {
            const auto volumeText = getDisplayName();
            const bool silenced = kind == Kind::fader && isSilenced();

            if (volumeText != shownVolume || silenced != shownSilenced)
            {
                shownVolume = volumeText;
                shownSilenced = silenced;
                repaint();
            }
        }

        // How much louder the effect makes it: the energy out against in, over about the last half second.
        if (hasBadge())
        {
            energyIn = energyIn * 0.93f + s->inputEnergy.exchange (0.0f);
            energyOut = energyOut * 0.93f + s->outputEnergy.exchange (0.0f);
        }

        // Bars fall back smoothly; the numbers show the highest peak of the last third of a second,
        // so they stay readable.
        barIn = juce::jmax (in, barIn * 0.8f);
        barOut = juce::jmax (out, barOut * 0.8f);
        holdIn = juce::jmax (holdIn, in);
        holdOut = juce::jmax (holdOut, out);

        if (++ticksSinceText >= textRefreshTicks)
        {
            textIn = holdIn;
            textOut = holdOut;
            holdIn = holdOut = 0.0f;
            ticksSinceText = 0;

            const bool playing = textIn >= juce::Decibels::decibelsToGain (gainstaging::silentBelowDb) && energyIn > 1.0e-9f;
            const auto change = playing ? std::optional<float> (10.0f * std::log10 (juce::jmax (1.0e-12f, energyOut) / energyIn)) : std::nullopt;

            if (change != shownChange)
            {
                shownChange = change;
                repaint (getBadgeArea());
            }
        }

        repaint (getMeterArea());
    }

    juce::String getDisplayName() const
    {
        if (kind == Kind::fader)
            return gainstaging::formatDb (track.volumeDb.load());

        if (kind == Kind::master)
            return gainstaging::formatDb (owner.engine.masterVolumeDb.load());

        if (kind == Kind::synth)
            return "DawIt Synth - " + builtInSounds::getName (track.builtInSound);

        if (kind == Kind::busInput)
            return "From " + describeFeed (track, owner.engine.getTracks());

        if (kind == Kind::bus)
            return busTrack->name;

        if (kind == Kind::send)
            return getSendText();

        auto* s = getSlot();
        return s != nullptr ? s->instance->getName() : juce::String();
    }

    /** Kind::send: an arrow, the return's name and the level. */
    juce::String getSendText() const
    {
        const auto* send = getSend();
        const auto arrow = juce::String::fromUTF8 ("\xe2\x86\x92 ");
        return arrow + busTrack->name + "  " + (send != nullptr && send->isOn() ? juce::String (send->levelDb.load(), 1) + " dB" : juce::String ("Off"));
    }

    /** Kind::send: the whole half-height chip. */
    void paintSend (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        auto fill = busTrack->colour.withAlpha (0.35f);

        if (isMouseOver (true))
            fill = fill.brighter (0.12f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (busTrack->colour.withAlpha (0.8f));
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

        auto area = getLocalBounds().reduced (7, 0);
        auto meter = area.removeFromRight (sendMeterWidth).withSizeKeepingCentre (sendMeterWidth, 5);
        area.removeFromRight (8);
        const auto* send = getSend();

        if (send != nullptr && send->preFader.load())
        {
            g.setColour (theme::meterYellow);
            g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.drawText ("PRE", area.removeFromRight (28), juce::Justification::centredRight, false);
        }

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (getSendText(), area, juce::Justification::centredLeft, true);
        drawLevelBar (g, meter.toFloat(), barOut);
    }

    void paint (juce::Graphics& g) override
    {
        if (kind == Kind::send)
        {
            paintSend (g);
            return;
        }

        auto* s = getSlot();
        const bool builtIn = kind != Kind::plugin;
        const bool volume = kind == Kind::fader || kind == Kind::master;

        if (! builtIn && s == nullptr)
            return;

        const bool instrument = kind == Kind::synth || kind == Kind::busInput || (s != nullptr && s->isInstrument);
        const bool bypassed = s != nullptr && s->bypassed.load();
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);

        // A bus on the way has the bus's colour, like the instrument has the track's.
        const auto tint = kind == Kind::bus ? busTrack->colour : track.colour;
        auto fill = instrument || kind == Kind::bus ? tint.withAlpha (0.35f) : volume ? theme::lane : theme::panelLight;
        if (isMouseOver (true))
            fill = fill.brighter (0.12f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (instrument || kind == Kind::bus ? tint.withAlpha (0.8f) : theme::gridBar);
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

        // What feeds a bus: its tracks' colours as a small stack.
        if (kind == Kind::busInput)
        {
            const auto members = routing::getMembers (track, owner.engine.getTracks());
            const int shown = juce::jmin (5, (int) members.size());
            const float top = (float) getHeight() * 0.5f - (float) shown * 2.75f;

            for (int i = 0; i < shown; ++i)
            {
                g.setColour (members[(size_t) i]->colour);
                g.fillRoundedRectangle (7.0f, top + 5.5f * (float) i, 12.0f, 4.0f, 1.5f);
            }
        }

        // Power dot (bypass)
        if (! builtIn)
        {
            const auto dot = getPowerArea().toFloat().withSizeKeepingCentre (10.0f, 10.0f);
            g.setColour (bypassed ? theme::textDim.withAlpha (0.5f) : theme::meterGreen);
            g.fillEllipse (dot);
        }

        // Remove cross
        if (! builtIn)
        {
            const auto x = getRemoveArea().toFloat().withSizeKeepingCentre (8.0f, 8.0f);
            g.setColour (isMouseOver (true) ? theme::text : theme::textDim);
            g.drawLine ({ x.getTopLeft(), x.getBottomRight() }, 1.5f);
            g.drawLine ({ x.getBottomLeft(), x.getTopRight() }, 1.5f);
        }

        paintMeters (g, instrument);
        paintPicture (g, bypassed);

        auto text = getTextArea();
        auto label = text.removeFromTop (getHeight() / 2).withTrimmedTop (3);

        if (hasBadge())
            paintBadge (g, label.removeFromRight (badgeWidth), bypassed);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (kind == Kind::fader    ? "FADER" + juce::String (isSilenced() ? " - SILENT" : "")
                    : kind == Kind::master ? juce::String ("MASTER")
                    : kind == Kind::busInput ? juce::String (track.isReturn() ? "SENDS ADDED UP" : "TRACKS ADDED UP")
                    : kind == Kind::bus    ? juce::String ("BUS")
                    : instrument           ? juce::String ("INSTRUMENT")
                                           : "EFFECT" + juce::String (bypassed ? " - OFF" : ""),
                    label, juce::Justification::bottomLeft, true);

        g.setColour (bypassed ? theme::textDim : theme::text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (getDisplayName(), text.withTrimmedBottom (3), juce::Justification::topLeft, true);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseMove (const juce::MouseEvent& e) override
    {
        // The badge of DawIt EQ and DawIt Compressor is a button.
        const bool onBadge = hasBadge() && canMatch() && getBadgeArea().contains (e.getPosition());

        if (onBadge != mouseOnBadge)
        {
            mouseOnBadge = onBadge;
            setMouseCursor (onBadge ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
            repaint (getBadgeArea());
        }
    }

    /** True for effects, the only chips that can be dragged: instruments always come first. */
    bool isMovable() const
    {
        auto* s = getSlot();
        return s != nullptr && ! s->isInstrument;
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        if (auto* send = getSend())
            dragStartDb = send->levelDb.load();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        // A send's chip is a knob lying down: up or down changes its level, a quarter of a dB per pixel.
        if (kind == Kind::send && ! e.mods.isPopupMenu())
        {
            if (auto* send = getSend(); send != nullptr && e.getDistanceFromDragStart() > 2)
            {
                const float level = juce::jlimit (sendOffDb, 6.0f, std::round ((dragStartDb - (float) e.getDistanceFromDragStartY() * 0.25f) * 10.0f) / 10.0f);
                send->levelDb = level;
                repaint();
            }

            return;
        }

        if (isMovable() && ! e.mods.isPopupMenu() && (dragging || e.getDistanceFromDragStart() > 4))
        {
            dragging = true;
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
            owner.dragChip (*this, e.getEventRelativeTo (&owner.chipRow));
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragging)
        {
            dragging = false;
            setMouseCursor (juce::MouseCursor::NormalCursor);
            owner.endChipDrag();
            return;
        }

        if (e.mouseWasDraggedSinceMouseDown() || ! getLocalBounds().contains (e.getPosition()))
            return;

        auto& dawEngine = owner.engine;
        auto& windows = owner.pluginWindows;

        if (kind == Kind::send)
        {
            if (e.mods.isPopupMenu())
            {
                owner.actions.showSendMenu (track, *busTrack, this);
                return;
            }

            // The return's own chain. Selecting rebuilds the bar, which deletes this chip: do it after this callback returns.
            juce::MessageManager::callAsync ([bar = juce::Component::SafePointer<DeviceBar> (&owner), ret = busTrack]
            {
                if (bar != nullptr && bar->engine.indexOf (ret) >= 0)
                    bar->selection.select (ret);
            });
            return;
        }

        if (kind == Kind::fader || kind == Kind::master)
        {
            if (owner.onShowMixer != nullptr)
                owner.onShowMixer();

            return;
        }

        // The bus on the way: show its own chain. Selecting rebuilds the bar, which deletes this chip: do it after
        // this callback returns.
        if (kind == Kind::bus)
        {
            juce::MessageManager::callAsync ([bar = juce::Component::SafePointer<DeviceBar> (&owner), bus = busTrack]
            {
                if (bar != nullptr && bar->engine.indexOf (bus) >= 0)
                    bar->selection.select (bus);
            });
            return;
        }

        // What feeds the bus: a list of its tracks, each opening its own chain.
        if (kind == Kind::busInput)
        {
            const auto members = track.isReturn() ? routing::getSenders (track, dawEngine.getTracks()) : routing::getMembers (track, dawEngine.getTracks());
            juce::PopupMenu menu;
            menu.addSectionHeader (track.isReturn() ? "Sends into " + track.name : "Tracks in " + track.name);

            for (size_t i = 0; i < members.size(); ++i)
            {
                auto text = "Show " + members[i]->name + juce::String::fromUTF8 ("\xe2\x80\x99s chain");

                if (auto* send = track.isReturn() ? routing::findSend (*members[i], track) : nullptr)
                    text << " (" << juce::String (send->levelDb.load(), 1) << " dB)";

                menu.addItem ((int) i + 1, text);
            }

            if (members.empty())
                menu.addItem (-1, track.isReturn() ? "No track sends to it yet" : "No tracks play into it yet", false);

            menu.addSeparator();
            guideicon::addItem (menu, 1000, "About buses, sends and routing...");
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                                [bar = juce::Component::SafePointer<DeviceBar> (&owner), members, isReturn = track.isReturn()] (int result)
                                {
                                    if (bar == nullptr || result <= 0)
                                        return;

                                    if (result == 1000)
                                    {
                                        if (bar->actions.onShowGuide != nullptr)
                                            bar->actions.onShowGuide (isReturn ? routingguide::Topic::sends : routingguide::Topic::what);
                                    }
                                    else if (result <= (int) members.size() && bar->engine.indexOf (members[(size_t) result - 1]) >= 0)
                                    {
                                        bar->selection.select (members[(size_t) result - 1]);
                                    }
                                });
            return;
        }

        if (kind == Kind::synth)
        {
            if (e.mods.isPopupMenu())
                plugins::showAddMenu (dawEngine, windows, track, true, this);
            else
                plugins::openBuiltInSynth (dawEngine, windows, track);

            return;
        }

        auto* s = getSlot();

        if (s == nullptr)
            return;

        if (e.mods.isPopupMenu())
            plugins::showSlotMenu (dawEngine, windows, track, *s, this);
        else if (hasBadge() && canMatch() && (isMatching() || shownChange.has_value()) && getBadgeArea().contains (e.getPosition()))
            setMatching (! isMatching());
        else if (getPowerArea().contains (e.getPosition()))
            dawEngine.setPluginBypassed (*s, ! s->bypassed.load());
        else if (getRemoveArea().contains (e.getPosition()))
            // Removing rebuilds the bar, which deletes this chip: do it after this callback returns.
            juce::MessageManager::callAsync ([&dawEngine, &windows, trackPtr = &track, s]
            {
                if (dawEngine.indexOf (trackPtr) >= 0 && plugins::contains (*trackPtr, s))
                    plugins::remove (dawEngine, windows, *trackPtr, *s);
            });
        else
            plugins::open (windows, track, *s);
    }

private:
    juce::Rectangle<int> getPowerArea() const { return getLocalBounds().removeFromLeft (22); }
    juce::Rectangle<int> getRemoveArea() const { return getLocalBounds().removeFromRight (20); }

    juce::Rectangle<int> getMeterArea() const
    {
        return getLocalBounds().withTrimmedRight (kind != Kind::plugin ? 8 : 20).removeFromRight (meterWidth).reduced (0, 4);
    }

    static constexpr int sendMeterWidth = 36;

    /** Where the chip's text starts: the plugins make room for their power dot, a bus's feed for its colours. */
    int getTextLeft() const { return kind == Kind::plugin ? 24 : kind == Kind::busInput ? 26 : 10; }

    /** The name, with the kind of chip above it. */
    juce::Rectangle<int> getTextArea() const
    {
        const bool plugin = kind == Kind::plugin;
        return getLocalBounds().withTrimmedLeft (getTextLeft()).withTrimmedRight ((plugin ? 20 : 8) + meterWidth
                                                                                    + (hasPicture() ? pictureWidth + 6 : 0));
    }

    /** At the right of the line above the name. */
    juce::Rectangle<int> getBadgeArea() const
    {
        return getTextArea().removeFromTop (getHeight() / 2).withTrimmedTop (3).removeFromRight (badgeWidth);
    }

    /** How much louder or quieter the effect makes the sound while it plays, or that match level is on. */
    void paintBadge (juce::Graphics& g, juce::Rectangle<int> area, bool bypassed)
    {
        if (bypassed)
            return;

        juce::String text;
        auto colour = theme::textDim;

        // Match level shows as such while it keeps the loudness; DawIt EQ's output gain comes after it.
        if (isMatching() && (! shownChange.has_value() || std::abs (*shownChange) < 1.0f))
        {
            text = "MATCHED";
            colour = theme::accent;
        }
        else if (shownChange.has_value())
        {
            const float change = *shownChange;
            text = std::abs (change) < 0.05f ? juce::String ("0.0 dB") : gainstaging::formatDb (change);
            colour = change >= gainstaging::louderHintDb ? theme::meterYellow : std::abs (change) >= 1.0f ? theme::text : theme::textDim;
        }
        else
        {
            return;
        }

        const auto pill = area.toFloat().withSizeKeepingCentre ((float) area.getWidth(), 13.0f).withTrimmedTop (1.0f);
        g.setColour (colour.withAlpha (0.16f));
        g.fillRoundedRectangle (pill, 6.0f);

        if (mouseOnBadge && isMouseOver (true))
        {
            g.setColour (colour.withAlpha (0.6f));
            g.drawRoundedRectangle (pill, 6.0f, 1.0f);
        }

        g.setColour (colour);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (text, pill, juce::Justification::centred, false);
    }

    juce::String getBadgeTooltip() const
    {
        const float change = shownChange.value_or (0.0f);

        if (isMatching())
            return std::abs (change) < 1.0f
                       ? "Match level is on: this effect keeps the sound as loud as what comes in, so on and off compare fairly. Click to switch it off"
                       : "Match level is on, but the effect still makes the sound " + juce::String (change, 1) + " dB "
                             + (change > 0.0f ? "louder" : "quieter") + ": its output gain is not 0 dB. Click to switch match level off";

        juce::String tip = std::abs (change) < 0.05f ? juce::String ("This effect doesn't change the loudness.")
                         : "This effect makes the sound " + juce::String (std::abs (change), 1) + " dB " + (change > 0.0f ? "louder" : "quieter")
                               + " while it plays.";

        if (canMatch())
            return tip + " Click to switch on match level: it keeps the output as loud as the input, so you hear only what the effect changes";

        return tip + (change >= 1.0f ? " Louder tends to sound better: turn its output down by about that much to compare it fairly" : "");
    }

    /** The signal for the picture: the EQ's spectrum after it, or the compressor's loudest level and
        reduction since the last tick. Both sink away when nothing plays. True if it should redraw. */
    bool updateSignal()
    {
        if (auto* eqPlugin = getEq())
        {
            if (spectrum == nullptr)
                spectrum = std::make_unique<Spectrum>();

            auto& sp = *spectrum;
            const int columns = pictureWidth - 4;
            sp.levels.resize ((size_t) columns, -120.0f);
            const auto written = eqPlugin->readPreview (sp.data.data(), Spectrum::size);
            const bool fresh = written != sp.written;
            sp.written = written;

            if (fresh)
            {
                std::fill (sp.data.begin() + Spectrum::size, sp.data.end(), 0.0f);
                sp.window.multiplyWithWindowingTable (sp.data.data(), (size_t) Spectrum::size);
                sp.fft.performFrequencyOnlyForwardTransform (sp.data.data());
            }

            const double rate = eqPlugin->getSampleRate() > 0.0 ? eqPlugin->getSampleRate() : 48000.0;
            bool visible = false;

            for (int x = 0; x < columns; ++x)
            {
                auto& level = sp.levels[(size_t) x];

                if (fresh)
                {
                    // The loudest bin under the column; a full-scale sine comes out at a quarter of the size.
                    const auto hzAt = [] (double c) { return eq::minFreq * std::pow (eq::maxFreq / eq::minFreq, c / columns); };
                    const int last = Spectrum::size / 2 - 1;
                    const int b0 = juce::jlimit (0, last, (int) (hzAt (x) * Spectrum::size / rate));
                    const int b1 = juce::jlimit (b0, last, (int) (hzAt (x + 1) * Spectrum::size / rate));
                    float peak = 0.0f;

                    for (int b = b0; b <= b1; ++b)
                        peak = juce::jmax (peak, sp.data[(size_t) b]);

                    const float db = juce::Decibels::gainToDecibels (peak * 4.0f / (float) Spectrum::size, -120.0f);
                    level = db > level ? db : juce::jmax (db, level - 1.5f);   // jump up, fall back about 45 dB a second
                }
                else
                {
                    level = juce::jmax (-120.0f, level - 1.5f);
                }

                visible = visible || level > -Spectrum::rangeDb;
            }

            const bool redraw = visible || sp.shown;
            sp.shown = visible;
            return redraw;
        }

        if (auto* compressor = getCompressor())
        {
            for (auto& p : trail)
                ++p.age;

            trail.erase (std::remove_if (trail.begin(), trail.end(), [] (const DotPoint& p) { return p.age > DotPoint::maxAge; }), trail.end());
            const auto now = compressor->takePreview();

            if (now.detectorDb > comp::minThreshold)
                trail.push_back ({ now.detectorDb, now.reductionDb, 0 });

            const bool redraw = ! trail.empty() || trailShown;
            trailShown = ! trail.empty();
            return redraw;
        }

        if (auto* gate = getGate())
        {
            for (auto& p : trail)
                ++p.age;

            trail.erase (std::remove_if (trail.begin(), trail.end(), [] (const DotPoint& p) { return p.age > DotPoint::maxAge; }), trail.end());
            const auto now = gate->takePreview();

            if (now.detectorDb > gate::minThreshold)
                trail.push_back ({ now.detectorDb, now.reductionDb, 0 });

            const bool redraw = ! trail.empty() || trailShown;
            trailShown = ! trail.empty();
            return redraw;
        }

        return false;
    }

    /** Just left of the meters. */
    juce::Rectangle<int> getPictureArea() const
    {
        const auto meters = getMeterArea();
        return { meters.getX() - pictureWidth - 6, meters.getY(), pictureWidth, meters.getHeight() };
    }

    /** The EQ curve (20 Hz to 20 kHz across, +-12 dB up and down, so small moves show), or the
        compressor's curve (level in across, level out up, from -60 to 0 dB): the same pictures as in
        their windows, tiny. */
    void paintPicture (juce::Graphics& g, bool bypassed)
    {
        auto* eqPlugin = getEq();
        auto* compressor = getCompressor();
        auto* gatePlugin = getGate();
        auto* stereoPlugin = getStereo();

        if (eqPlugin == nullptr && compressor == nullptr && gatePlugin == nullptr && stereoPlugin == nullptr && getTune() == nullptr)
            return;

        const auto area = getPictureArea().toFloat();
        g.setColour (theme::background.withAlpha (0.7f));
        g.fillRoundedRectangle (area, 3.0f);

        const auto plot = area.reduced (2.0f);

        // What comes out of the EQ, behind the curve, as in the EQ's window.
        if (eqPlugin != nullptr && spectrum != nullptr && spectrum->shown && ! bypassed)
        {
            juce::Path fill;
            fill.startNewSubPath (plot.getBottomLeft());

            for (size_t x = 0; x < spectrum->levels.size(); ++x)
                fill.lineTo (plot.getX() + (float) x + 0.5f,
                             plot.getBottom() - juce::jlimit (0.0f, 1.0f, 1.0f + spectrum->levels[x] / Spectrum::rangeDb) * plot.getHeight());

            fill.lineTo (plot.getBottomRight());
            fill.closeSubPath();
            g.setColour (theme::accent.withAlpha (0.35f));
            g.fillPath (fill);
        }

        if (stereoPlugin != nullptr)
        {
            if (! bypassed) stereoScope::drawGoniometer (g, plot, stereoPlugin->outputMeasurement.getPoints(), false, stereoPlugin->getSampleRate());
            const int listen = stereoPlugin->getSettings().listen;
            if (listen != 0)
            {
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
                g.drawText (listen == 1 ? "MONO" : "SIDE", plot, juce::Justification::centred);
            }
            return;
        }
        chainpictures::paintCurve (g, plot, getSlot()->instance.get(), bypassed);

        // The track on the compressor's curve: a dot at the level it hears and what it turned that
        // down to (attack and release keep it off the curve for a moment), with a fading trail, and in
        // red how far the newest one is turned down.
        if ((compressor != nullptr || gatePlugin != nullptr) && ! trail.empty() && ! bypassed)
        {
            const float lowest = gatePlugin != nullptr ? gate::minThreshold : comp::minThreshold;
            auto at = [&] (const DotPoint& p)
            {
                const float in = juce::jlimit (lowest, 0.0f, p.inDb);
                const float out = juce::jmax (lowest, in - p.reductionDb);
                const float span = -lowest;
                return juce::Point<float> { plot.getX() + (in - lowest) / span * plot.getWidth(),
                                            plot.getBottom() - (out - lowest) / span * plot.getHeight() };
            };

            for (auto& p : trail)
            {
                g.setColour (juce::Colours::white.withAlpha (0.5f * (1.0f - (float) p.age / (float) (DotPoint::maxAge + 1))));
                g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (at (p)));
            }

            const auto& newest = trail.back();
            const auto dot = at (newest);
            const float in = juce::jlimit (lowest, 0.0f, newest.inDb);
            const float unchangedY = plot.getBottom() - (in - lowest) / -lowest * plot.getHeight();

            if (unchangedY < dot.y - 1.0f)
            {
                g.setColour (theme::meterRed.withAlpha (0.9f));
                g.fillRect (juce::Rectangle<float>::leftTopRightBottom (dot.x - 0.75f, unchangedY, dot.x + 0.75f, dot.y));
            }

            g.setColour (juce::Colours::white.withAlpha (1.0f - (float) newest.age / (float) (DotPoint::maxAge + 1)));
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (dot));
        }
    }

    static juce::String formatDb (float level)
    {
        if (level < juce::Decibels::decibelsToGain (-60.0f))
            return "-inf dB";

        const float db = juce::Decibels::gainToDecibels (level);
        return (db > 0.05f ? "+" : "") + juce::String (db, 1) + " dB";
    }

    /** Instruments get only an output meter: their input is MIDI. */
    void paintMeters (juce::Graphics& g, bool instrument)
    {
        auto area = getMeterArea();

        g.setColour (theme::gridBar.withAlpha (0.6f));
        g.fillRect (area.getX(), area.getY(), 1, area.getHeight());
        area.removeFromLeft (8);

        auto paintRow = [&g] (juce::Rectangle<int> row, const juce::String& tag, float bar, float text)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText (tag, row.removeFromLeft (tagWidth), juce::Justification::centredLeft, false);

            drawLevelBar (g, row.removeFromLeft (barWidth).toFloat().withSizeKeepingCentre ((float) barWidth, 5.0f), bar);

            g.setColour (text > 1.0f ? theme::meterRed : theme::text);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (formatDb (text), row, juce::Justification::centredRight, false);
        };

        if (instrument)
        {
            paintRow (area.withSizeKeepingCentre (area.getWidth(), area.getHeight() / 2), "OUT", barOut, textOut);
            return;
        }

        paintRow (area.removeFromTop (area.getHeight() / 2), "IN", barIn, textIn);
        paintRow (area, "OUT", barOut, textOut);
    }

    DeviceBar& owner;
    Track& track;
    const PluginSlot* slot;
    const Kind kind;
    Track* const busTrack;   // Kind::bus: the bus on the way; Kind::busInput: the bus being fed; Kind::send: the return

    float barIn = 0.0f, barOut = 0.0f, holdIn = 0.0f, holdOut = 0.0f, textIn = 0.0f, textOut = 0.0f;
    float energyIn = 0.0f, energyOut = 0.0f;
    std::optional<float> shownChange;
    bool mouseOnBadge = false;
    juce::String shownVolume;
    bool shownSilenced = false;
    std::vector<float> shownSettings;

    // DawIt EQ's spectrum: the newest samples after it, analysed per column of the picture, in dB.
    struct Spectrum
    {
        static constexpr int order = 10, size = 1 << order;
        static constexpr float rangeDb = 80.0f;   // from the bottom of the picture to 0 dB at the top
        juce::dsp::FFT fft { order };
        juce::dsp::WindowingFunction<float> window { (size_t) size, juce::dsp::WindowingFunction<float>::hann };
        std::vector<float> data = std::vector<float> ((size_t) size * 2), levels;
        juce::int64 written = -1;
        bool shown = false;
    };
    std::unique_ptr<Spectrum> spectrum;

    // DawIt Compressor's and DawIt Gate's dot: the loudest level per tick and its reduction, kept for a short trail.
    struct DotPoint
    {
        static constexpr int maxAge = 12;
        float inDb, reductionDb;
        int age;
    };
    std::vector<DotPoint> trail;
    bool trailShown = false;
    int ticksSinceText = textRefreshTicks;
    bool dragging = false;
    float dragStartDb = 0.0f;   // a send: its level when the mouse went down
};

//==============================================================================
DeviceBar::DeviceBar (Engine& e, Selection& s, PluginWindowManager& w, TrackActions& a)
    : engine (e), selection (s), pluginWindows (w), actions (a)
{
    addInstrumentButton.setTooltip ("Pick the instrument: the DawIt Synth, a built-in synth or a plugin (replaces the current instrument)");
    addInstrumentButton.onClick = [this]
    {
        if (shownTrack != nullptr)
            plugins::showAddMenu (engine, pluginWindows, *shownTrack, true, &addInstrumentButton);
    };

    addEffectButton.setTooltip ("Add an effect plugin to the end of the chain");
    addEffectButton.onClick = [this]
    {
        if (shownTrack != nullptr)
            plugins::showAddMenu (engine, pluginWindows, *shownTrack, false, &addEffectButton);
    };

    guideButton.setTooltip ("Gain staging: how loud the sound is at every step of the chain, and why it matters");
    guideButton.onClick = [this] { showGuide (gainstaging::Topic::what); };

    for (auto* b : std::initializer_list<juce::TextButton*> { &addInstrumentButton, &addEffectButton, &guideButton })
    {
        b->setWantsKeyboardFocus (false);
        addChildComponent (b);
    }

    // A long chain scrolls sideways (scroll wheel or trackpad) with a thin bar in the bottom margin.
    chipRow.onPaint = [this] (juce::Graphics& g) { paintArrows (g); };
    chipRow.onPaintOverChildren = [this] (juce::Graphics& g) { paintDropMarker (g); };

    // The ! under an arrow tells what it is about; a click opens the guide there.
    chipRow.onTooltip = [this] (juce::Point<int> p)
    {
        for (int i = 0; i + 1 < chips.size(); ++i)
            if (auto* hint = getArrowHint (i); hint != nullptr && getMarkerArea (i).expanded (3).contains (p))
                return hint->text + " (click to read more)";

        return juce::String();
    };

    chipRow.onClick = [this] (juce::Point<int> p)
    {
        for (int i = 0; i + 1 < chips.size(); ++i)
            if (auto* hint = getArrowHint (i); hint != nullptr && getMarkerArea (i).expanded (3).contains (p))
                return showGuide (hint->topic);
    };

    viewport.setViewedComponent (&chipRow, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (scrollBarThickness);
    addAndMakeVisible (viewport);

    selection.addChangeListener (this);
    engine.addChangeListener (this);
    refresh();
    startTimerHz (30);
}

DeviceBar::~DeviceBar()
{
    pluginWindows.closeFor (&guideKey);
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

void DeviceBar::showGuide (gainstaging::Topic topic)
{
    pluginWindows.show (&guideKey, "Gain staging", [this]
    {
        auto* explainer = new GainStagingExplainer ([bar = juce::Component::SafePointer<DeviceBar> (this)]
        {
            return bar != nullptr ? bar->getSnapshot() : gainstaging::Snapshot();
        });

        guide = explainer;
        return explainer;
    });

    if (guide != nullptr)
        guide->showTopic (topic);
}

guideprint::Item DeviceBar::getPrintGuide()
{
    guideprint::Item item;
    item.guide = gen::guides::Guide::gainStaging;

    if (guide != nullptr)
        item.useOpen (guide.getComponent());
    else
        item.adopt (std::make_unique<GainStagingExplainer> ([bar = juce::Component::SafePointer<DeviceBar> (this)]
                                                           {
                                                               return bar != nullptr ? bar->getSnapshot() : gainstaging::Snapshot();
                                                           }));

    return item;
}

gainstaging::Snapshot DeviceBar::getSnapshot() const
{
    gainstaging::Snapshot snapshot;

    if (shownTrack != nullptr)
    {
        snapshot.trackName = shownTrack->name;
        snapshot.stops = stops;
        snapshot.stops.insert (snapshot.stops.end(), sendStops.begin(), sendStops.end());   // the sends, off the line
        snapshot.hints = hints;
    }

    return snapshot;
}

void DeviceBar::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void DeviceBar::timerCallback()
{
    // A send turned on or off (a knob in the mixer, a menu) changes the chips.
    if (shownTrack != nullptr && getSends (shownTrack) != shownSends)
        refresh();

    for (auto* chip : chips)
    {
        chip->updateMeters();
        chip->updatePicture();
    }

    for (auto* chip : sendChips)
        chip->updateMeters();

    updateGainStaging();
}

std::vector<std::pair<const Track*, bool>> DeviceBar::getSends (const Track* track) const
{
    std::vector<std::pair<const Track*, bool>> result;

    if (track != nullptr && engine.indexOf (track) >= 0)
        for (auto& send : track->sends)
            if (send->isOn())
                result.push_back ({ send->target, send->preFader.load() });

    return result;
}

int DeviceBar::getFaderIndex() const
{
    for (int i = 0; i < chips.size(); ++i)
        if (chips[i]->getKind() == Chip::Kind::fader)
            return i;

    return -1;
}

void DeviceBar::updateGainStaging()
{
    stops.clear();

    for (auto* chip : chips)
        stops.push_back (chip->getStop());

    hints = gainstaging::findHints (stops);

    // The sends, as stops off the line: each is taken after the fader's stop, or the one before it when pre-fader.
    sendStops.clear();
    const int faderIndex = getFaderIndex();

    for (auto* chip : sendChips)
    {
        auto stop = chip->getStop();
        stop.tapAfter = faderIndex >= 0 ? (stop.preFader ? faderIndex - 1 : faderIndex) : -1;
        sendStops.push_back (std::move (stop));
    }

    // Repaint the arrows whose level or ! changed.
    shownArrows.resize ((size_t) juce::jmax (0, chips.size() - 1));

    for (int i = 0; i + 1 < chips.size(); ++i)
    {
        const auto* hint = getArrowHint (i);
        const auto shown = juce::String (juce::roundToInt (juce::Decibels::gainToDecibels (chips[i]->getOutLevel(), -100.0f)))
                           + (hint != nullptr ? (hint->serious ? "!!" : "!") : "");

        if (shown != shownArrows[(size_t) i])
        {
            shownArrows[(size_t) i] = shown;
            chipRow.repaint (getArrowArea (i));
        }
    }
}

juce::Rectangle<int> DeviceBar::getArrowArea (int arrow) const
{
    if (arrow < 0 || arrow + 1 >= chips.size())
        return {};

    const auto* left = chips[arrow];
    return { left->getRight(), 0, chips[arrow + 1]->getX() - left->getRight(), chipRow.getHeight() };
}

juce::Rectangle<int> DeviceBar::getMarkerArea (int arrow) const
{
    const auto area = getArrowArea (arrow);
    return juce::Rectangle<int> (11, 11).withCentre ({ area.getCentreX(), area.getBottom() - 7 });
}

const gainstaging::Hint* DeviceBar::getArrowHint (int arrow) const
{
    const gainstaging::Hint* found = nullptr;

    for (auto& hint : hints)
        if ((hint.stop == arrow + 1 || (arrow == 0 && hint.stop == 0)) && (found == nullptr || (hint.serious && ! found->serious)))
            found = &hint;

    return found;
}

void DeviceBar::refresh()
{
    auto* track = selection.getTrack();

    if (track != nullptr && engine.indexOf (track) < 0)
        track = nullptr;

    std::vector<const PluginSlot*> slots;
    if (track != nullptr)
        for (auto& p : track->plugins)
            slots.push_back (p.get());

    const bool builtInSynth = track != nullptr && track->usesBuiltInSynth();
    const auto sound = track != nullptr ? track->builtInSound.load() : BuiltInSound::piano;

    // The buses on the way to the master, and for a bus what feeds it.
    std::vector<const Track*> buses;

    if (track != nullptr)
        for (auto* b = track->bus; b != nullptr; b = b->bus)
            buses.push_back (b);

    const auto feed = track != nullptr && track->hasInput() ? describeFeed (*track, engine.getTracks()) : juce::String();
    const auto sends = getSends (track);

    // A new sound changes the built-in synth chip's name, and so its width.
    if (track == shownTrack && builtInSynth && sound != shownSound)
    {
        shownSound = sound;
        resized();
    }

    if (track != shownTrack || slots != shownSlots || builtInSynth != shownBuiltInSynth || buses != shownBuses || feed != shownFeed || sends != shownSends)
    {
        // An effect added to the end of a long chain scrolls into view; another track starts at its instrument.
        const bool otherTrack = track != shownTrack;
        const bool added = ! otherTrack && slots.size() > shownSlots.size() && ! slots.empty()
                           && std::find (shownSlots.begin(), shownSlots.end(), slots.back()) == shownSlots.end();

        const int heightBefore = getPreferredHeight();
        draggedChip = nullptr;
        dropIndex = dropMarkerX = -1;
        chips.clear();
        sendChips.clear();
        shownSends = sends;
        shownTrack = track;
        shownSlots = slots;
        shownBuiltInSynth = builtInSynth;
        shownSound = sound;
        shownBuses = buses;
        shownFeed = feed;

        if (track != nullptr)
        {
            if (track->hasInput())
                chipRow.addAndMakeVisible (chips.add (new Chip (*this, *track, nullptr, Chip::Kind::busInput, track)));
            else if (builtInSynth)
                chipRow.addAndMakeVisible (chips.add (new Chip (*this, *track, nullptr, Chip::Kind::synth)));

            for (auto* slot : slots)
                chipRow.addAndMakeVisible (chips.add (new Chip (*this, *track, slot, Chip::Kind::plugin)));

            // Where the chain goes after the effects: the fader, the buses it plays into, and the master.
            chipRow.addAndMakeVisible (chips.add (new Chip (*this, *track, nullptr, Chip::Kind::fader)));

            for (auto* b : buses)
                chipRow.addAndMakeVisible (chips.add (new Chip (*this, *track, nullptr, Chip::Kind::bus, const_cast<Track*> (b))));

            chipRow.addAndMakeVisible (chips.add (new Chip (*this, *track, nullptr, Chip::Kind::master)));

            // The sends that are on, in a row of their own below the line.
            for (auto& [target, pre] : sends)
            {
                juce::ignoreUnused (pre);
                chipRow.addAndMakeVisible (sendChips.add (new Chip (*this, *track, nullptr, Chip::Kind::send, const_cast<Track*> (target))));
            }
        }

        shownArrows.clear();
        stops.clear();
        hints.clear();

        if (getPreferredHeight() != heightBefore && onPreferredHeightChanged != nullptr)
            onPreferredHeightChanged();

        addInstrumentButton.setButtonText (track != nullptr && track->hasInstrument() ? "Replace Instrument" : "+ Instrument");
        addInstrumentButton.setVisible (track != nullptr && track->isMidi());
        addEffectButton.setVisible (track != nullptr);
        guideButton.setVisible (track != nullptr);
        resized();

        if (added)
            viewport.setViewPosition (chipRow.getWidth(), 0);
        else if (otherTrack)
            viewport.setViewPosition (0, 0);
    }

    for (auto* chip : chips)
        chip->repaint();

    for (auto* chip : sendChips)
        chip->repaint();

    repaint();
}

void DeviceBar::resized()
{
    auto r = getLocalBounds().reduced (8, 6);
    r.removeFromLeft (titleWidth);

    // Buttons on the right, chips fill the rest.
    if (guideButton.isVisible())
    {
        guideButton.setBounds (r.removeFromRight (guideButton.getBestWidthForHeight (r.getHeight()) + 14));
        r.removeFromRight (6);
    }

    if (addEffectButton.isVisible())
    {
        addEffectButton.setBounds (r.removeFromRight (84));
        r.removeFromRight (6);
    }

    if (addInstrumentButton.isVisible())
    {
        addInstrumentButton.setBounds (r.removeFromRight (addInstrumentButton.getBestWidthForHeight (r.getHeight()) + 14));
        r.removeFromRight (12);
    }

    // The chips keep their size; a chain longer than the space scrolls. The viewport reaches into
    // the bottom margin, where its scroll bar shows when needed.
    const int chipHeight = preferredHeight - 12;
    int x = 0;

    for (auto* chip : chips)
    {
        if (chip != draggedChip)
            chip->setBounds (x, 0, chip->getPreferredWidth(), chipHeight);
        else
            chip->setSize (chip->getPreferredWidth(), chipHeight);

        x += chip->getPreferredWidth() + arrowGap;
    }

    // The sends hang below the line: each starts under the gap it is taken from (before the fader when pre-fader,
    // after it otherwise), a little to the right of the branch, and never on top of the one before.
    const int faderIndex = getFaderIndex();
    int sendX = 0, rowWidth = juce::jmax (0, x - arrowGap);

    for (auto* chip : sendChips)
    {
        const auto* send = chip->getSend();
        const bool pre = send != nullptr && send->preFader.load();
        int tapX = 0;

        if (faderIndex >= 0)
            tapX = pre ? (faderIndex > 0 ? chips[faderIndex - 1]->getRight() + arrowGap / 2 : chips[faderIndex]->getX() + 6)
                       : chips[faderIndex]->getRight() + arrowGap / 2;

        sendX = juce::jmax (sendX, tapX + 12);
        chip->setBounds (sendX, chipHeight + sendRowGap, chip->getPreferredWidth(), sendRowHeight);
        sendX += chip->getPreferredWidth() + 8;
        rowWidth = juce::jmax (rowWidth, sendX - 8);
    }

    viewport.setBounds (r.withHeight (getHeight() - r.getY()));
    chipRow.setSize (rowWidth, r.getHeight());
}

//==============================================================================
std::vector<DeviceBar::Chip*> DeviceBar::getEffectChipsExcept (const Chip* except) const
{
    std::vector<Chip*> effects;

    for (auto* chip : chips)
        if (chip != except && chip->isMovable())
            effects.push_back (chip);

    return effects;
}

void DeviceBar::dragChip (Chip& chip, const juce::MouseEvent& e)
{
    if (shownTrack == nullptr || ! chip.isMovable())
        return;

    if (draggedChip != &chip)
    {
        draggedChip = &chip;
        dragGrabX = e.getMouseDownX() - chip.getX();
        chip.toFront (false);
    }

    // Near the edges of the view, the row scrolls along.
    const auto inView = viewport.getLocalPoint (&chipRow, e.getPosition());
    viewport.autoScroll (inView.x, inView.y, 30, 10);

    // The chip follows the mouse, kept between the instrument and the fader.
    const int left = chips.getFirst()->getX();
    const Chip* fader = chips.getLast();

    for (auto* other : chips)
        if (other->getKind() == Chip::Kind::fader)
            fader = other;

    const int right = juce::jmax (left, fader->getX() - arrowGap - chip.getWidth());
    chip.setTopLeftPosition (juce::jlimit (left, right, e.x - dragGrabX), chip.getY());

    // Its new place among the other effects follows the mouse; instruments stay in front.
    int instruments = 0;
    for (auto& p : shownTrack->plugins)
        instruments += p->isInstrument ? 1 : 0;

    const auto others = getEffectChipsExcept (&chip);
    int before = 0;

    for (auto* other : others)
        before += other->getBounds().getCentreX() < e.x ? 1 : 0;

    dropIndex = instruments + before;

    // The marker sits in the gap the chip will land in (none if there is nothing to reorder against).
    dropMarkerX = others.empty() ? -1
                : before == 0    ? others.front()->getX() - arrowGap / 2
                                 : others[(size_t) before - 1]->getRight() + arrowGap / 2;
    repaint();
}

void DeviceBar::endChipDrag()
{
    auto* chip = std::exchange (draggedChip, nullptr);
    auto* slot = chip != nullptr ? chip->getSlot() : nullptr;
    const int target = std::exchange (dropIndex, -1);
    dropMarkerX = -1;

    resized();
    repaint();

    // Moving rebuilds the chips once the change message arrives, after this mouse callback.
    if (slot != nullptr && target >= 0 && shownTrack != nullptr)
        engine.movePlugin (*shownTrack, slot, target);
}

void DeviceBar::paintDropMarker (juce::Graphics& g)
{
    if (draggedChip == nullptr || dropMarkerX < 0)
        return;

    g.setColour (theme::accent);
    g.fillRoundedRectangle (juce::Rectangle<float> ((float) dropMarkerX - 1.5f, (float) draggedChip->getY() - 2.0f,
                                                    3.0f, (float) draggedChip->getHeight() + 4.0f), 1.5f);
}

void DeviceBar::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);

    auto title = getLocalBounds().reduced (8, 6).removeFromLeft (titleWidth - 10);

    if (shownTrack == nullptr)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("Select a track to see its instrument and effects", getLocalBounds().withTrimmedLeft (12),
                    juce::Justification::centredLeft);
        return;
    }

    g.setColour (shownTrack->colour);
    g.fillRoundedRectangle (title.removeFromLeft (4).toFloat(), 2.0f);
    title.removeFromLeft (8);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (shownTrack->isReturn() ? "RETURN" : shownTrack->isBus() ? "BUS" : shownTrack->isMidi() ? "MIDI TRACK" : "AUDIO TRACK", title.removeFromTop (title.getHeight() / 2).withTrimmedTop (3),
                juce::Justification::bottomLeft, true);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (shownTrack->name, title.withTrimmedBottom (3), juce::Justification::topLeft, true);
}

void DeviceBar::paintBranches (juce::Graphics& g)
{
    // A short branch drops from the line, at the gap the send is taken from, and turns right into its chip.
    const int faderIndex = getFaderIndex();

    if (faderIndex < 0)
        return;

    for (auto* chip : sendChips)
    {
        const auto* send = chip->getSend();
        const bool pre = send != nullptr && send->preFader.load();
        const int gap = pre ? faderIndex - 1 : faderIndex;
        const float x = gap >= 0 ? (float) chips[gap]->getRight() + (float) arrowGap * 0.5f : (float) chips[faderIndex]->getX() + 6.0f;
        const float lineY = (float) (preferredHeight - 12) * 0.5f + 1.0f;
        const float chipY = (float) chip->getBounds().getCentreY();

        g.setColour (chip->getReturn().colour.withAlpha (0.9f));
        juce::Path branch;
        branch.startNewSubPath (x, lineY);
        branch.lineTo (x, chipY - 3.0f);
        branch.quadraticTo (x, chipY, x + 3.0f, chipY);
        g.strokePath (branch, juce::PathStrokeType (1.5f));
        g.drawArrow ({ x + 3.0f, chipY, (float) chip->getX() - 1.0f, chipY }, 1.5f, 5.0f, 5.0f);
        g.fillEllipse (x - 2.0f, lineY - 2.0f, 4.0f, 4.0f);
    }
}

void DeviceBar::paintArrows (juce::Graphics& g)
{
    paintBranches (g);

    // Signal-flow arrows between chips, in the colour of the level passing through, with its peak
    // above and a ! below where something is worth a look.
    for (int i = 0; i + 1 < chips.size(); ++i)
    {
        if (chips[i] == draggedChip || chips[i + 1] == draggedChip)
            continue;

        const auto area = getArrowArea (i).toFloat();
        const float x1 = area.getX() + 4.0f, x2 = area.getRight() - 4.0f;
        const float y = area.getCentreY() + 1.0f;

        if (x2 - x1 < 6.0f)
            continue;

        const float db = juce::Decibels::gainToDecibels (chips[i]->getOutLevel(), -100.0f);
        const auto zone = gainstaging::getZone (db);

        g.setColour (zone == gainstaging::Zone::silent ? theme::textDim : gainstaging::getZoneColour (zone));
        g.drawArrow ({ x1, y, x2, y }, zone == gainstaging::Zone::silent ? 1.5f : 2.0f, 6.0f, 6.0f);

        if (zone != gainstaging::Zone::silent)
        {
            const int rounded = juce::roundToInt (db);
            g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.drawText ((rounded > 0 ? "+" : "") + juce::String (rounded), area.withHeight (y - 5.0f - area.getY()),
                        juce::Justification::centredBottom, false);
        }

        if (auto* hint = getArrowHint (i))
        {
            const auto marker = getMarkerArea (i).toFloat();
            g.setColour (hint->serious ? theme::meterRed : theme::meterYellow);
            g.fillEllipse (marker);
            g.setColour (theme::background);
            g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.drawText ("!", marker, juce::Justification::centred, false);
        }
    }
}
} // namespace daw
