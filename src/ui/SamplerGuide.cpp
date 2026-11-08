#include "SamplerGuide.h"
#include "SamplerPictures.h"

#include <cmath>
#include <set>

namespace daw::samplertext
{
using samplerguide::Page;

namespace
{
    std::function<TrackNotes (const juce::AudioProcessor&)>& getLookup()
    {
        static std::function<TrackNotes (const juce::AudioProcessor&)> lookup;
        return lookup;
    }

    double now()
    {
        return juce::Time::getMillisecondCounterHiRes() / 1000.0;
    }

    void drawCaption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& left, const juce::String& right = {})
    {
        auto line = r.removeFromTop (18.0f);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::text);
        g.drawText (left, line, juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.drawText (right, line, juce::Justification::centredRight, true);
        r.removeFromTop (4.0f);
    }

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Colour colour = theme::textDim,
                    juce::Justification justification = juce::Justification::centredLeft, float size = 11.0f)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions (size));
        g.drawText (text, r, justification, true);
    }

    juce::String formatSeconds (double seconds)
    {
        return juce::String (seconds, seconds < 0.1 ? 3 : 2) + " s";
    }

    juce::String formatMb (double bytes)
    {
        const double mb = bytes / 1.0e6;
        return juce::String (mb, mb < 10.0 ? 1 : 0) + " MB";
    }

    //==============================================================================
    /** What a picture draws from: the sampler's real zones and recordings when its font is ready, otherwise an example. */
    struct Source
    {
        bool real = false;
        std::shared_ptr<soundfonts::Font> font;
        std::vector<soundfonts::Zone> distinct;   // each key and velocity range once
        std::vector<soundfonts::Zone> all;        // as the font lists them
        juce::String presetName;
        bool isDrumKit = false;
        int key = 60, velocity = 100;             // the last note played, or middle C
        int bank = 0, program = 0;
    };

    /** A made-up preset: a recording every 6 keys, each in a soft and a loud layer. */
    std::vector<soundfonts::Zone> makeExampleZones()
    {
        std::vector<soundfonts::Zone> zones;

        for (int low = 36; low < 96; low += 6)
            for (int layer = 0; layer < 2; ++layer)
            {
                soundfonts::Zone z;
                z.lowKey = low;
                z.highKey = low + 5;
                z.rootKey = low + 3;
                z.lowVelocity = layer == 0 ? 1 : 64;
                z.highVelocity = layer == 0 ? 63 : 127;
                z.loop = soundfonts::LoopMode::none;
                z.sampleEnd = 44100;
                z.attack = 0.005;
                z.decay = 1.5;
                z.sustain = 0.4;
                z.release = 0.4;
                zones.push_back (z);
            }

        return zones;
    }

    Source getSource (BuiltInSampler& sampler)
    {
        Source s;
        s.bank = sampler.getBank();
        s.program = sampler.getProgram();
        s.isDrumKit = s.bank == soundfonts::drumBank;
        const int last = sampler.lastNote.load();
        const int lastVelocity = sampler.lastVelocity.load();
        s.key = last >= 0 ? last : (s.isDrumKit ? sampler::testDrumNote : sampler::testNote);
        s.velocity = lastVelocity > 0 ? lastVelocity : 100;

        if (sampler.getStatus() == BuiltInSampler::Status::ready && sampler.getFont() != nullptr && ! sampler.getZones().empty())
        {
            s.real = true;
            s.font = sampler.getFont();
            s.all = sampler.getZones();
            s.distinct = samplerpics::getDistinctZones (s.all);
            s.presetName = sampler.getPresetName();
        }
        else
        {
            s.all = makeExampleZones();
            s.distinct = s.all;
            s.presetName = "an example preset";
            s.isDrumKit = false;
            s.key = last >= 0 ? last : 60;
        }

        return s;
    }

    /** The first zone that plays the source's last note, or null. */
    const soundfonts::Zone* findZone (const Source& s)
    {
        auto found = samplerpics::findZonesAt (s.distinct, s.key, s.velocity);

        if (found.empty())
            found = samplerpics::findZonesAt (s.distinct, s.key, -1);

        return found.empty() ? nullptr : &s.distinct[(size_t) found.front()];
    }

    void drawExampleNote (juce::Graphics& g, juce::Rectangle<float> r)
    {
        drawLabel (g, "An example: choose a SoundFont in DawIt Sampler's window to see yours.", r.removeFromBottom (16.0f), theme::textDim, juce::Justification::centredLeft, 11.5f);
    }

    juce::String zoneLabel (const soundfonts::Zone& z)
    {
        return "zone " + samplerguide::getNoteName (z.lowKey) + "-" + samplerguide::getNoteName (z.highKey) + ", root " + samplerguide::getNoteName (z.rootKey);
    }

    //==============================================================================
    /** A made-up recording of a plucked note: `cycles` cycles that fade away, drawn across `r` (the same cycles in a
        shorter stretch when `lengthFraction` is less than 1). */
    void drawSyntheticWave (juce::Graphics& g, juce::Rectangle<float> r, double cycles, double lengthFraction, juce::Colour colour)
    {
        g.setColour (theme::grid);
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
        const float width = r.getWidth() * (float) lengthFraction;
        const int steps = juce::jmax (2, (int) width * 2);
        juce::Path path;

        for (int i = 0; i <= steps; ++i)
        {
            const double t = (double) i / (double) steps;
            const double envelope = std::exp (-3.0 * t) * (1.0 - std::exp (-60.0 * t));
            const double value = envelope * (std::sin (juce::MathConstants<double>::twoPi * cycles * t)
                                             + 0.4 * std::sin (juce::MathConstants<double>::twoPi * cycles * 2.0 * t + 0.6));
            const float x = r.getX() + width * (float) t;
            const float y = r.getCentreY() - (float) value * r.getHeight() * 0.36f;
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.4f));
    }

    /** The recording of a zone as its loudest and quietest values in each pixel column. Returns the x of a sample position. */
    std::function<float (int)> drawRecording (juce::Graphics& g, juce::Rectangle<float> r, const soundfonts::Font& font, const soundfonts::Zone& z, juce::Colour colour)
    {
        const float* data = font.getSamples();
        const int length = z.sampleEnd - z.sampleStart;
        g.setColour (theme::grid);
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());

        auto xOf = [r, z, length] (int position)
        {
            return r.getX() + r.getWidth() * (float) (position - z.sampleStart) / (float) juce::jmax (1, length);
        };

        if (data == nullptr || length < 2)
            return xOf;

        const int columns = juce::jmax (1, (int) r.getWidth());
        float peak = 0.0001f;
        std::vector<float> low ((size_t) columns), high ((size_t) columns);

        for (int c = 0; c < columns; ++c)
        {
            const int from = z.sampleStart + (int) ((juce::int64) length * c / columns);
            const int to = juce::jmax (from + 1, z.sampleStart + (int) ((juce::int64) length * (c + 1) / columns));
            float lo = 0.0f, hi = 0.0f;

            for (int i = from; i < to && i < z.sampleEnd; ++i)
            {
                lo = juce::jmin (lo, data[i]);
                hi = juce::jmax (hi, data[i]);
            }

            low[(size_t) c] = lo;
            high[(size_t) c] = hi;
            peak = juce::jmax (peak, juce::jmax (-lo, hi));
        }

        const float scale = r.getHeight() * 0.46f / peak;
        g.setColour (colour);

        for (int c = 0; c < columns; ++c)
        {
            const float x = r.getX() + (float) c + 0.5f;
            g.drawVerticalLine ((int) x, r.getCentreY() - high[(size_t) c] * scale, juce::jmax (r.getCentreY() - high[(size_t) c] * scale + 1.0f, r.getCentreY() - low[(size_t) c] * scale));
        }

        return xOf;
    }

    /** Draws the zone's volume envelope, with the times of its stages under it. */
    void drawEnvelope (juce::Graphics& g, juce::Rectangle<float> r, const soundfonts::Zone& z, juce::Colour colour)
    {
        auto labels = r.removeFromBottom (16.0f);
        const double times[4] = { z.attack, z.hold, z.decay, z.release };
        double total = 0.0;

        for (double t : times)
            total += t;

        std::array<float, 5> widths {};   // attack, hold, decay, sustain, release
        const float sustainShare = 0.22f;
        const float stageShare = 1.0f - sustainShare;
        widths[3] = sustainShare;
        const int map[4] = { 0, 1, 2, 4 };

        for (int i = 0; i < 4; ++i)
            widths[(size_t) map[i]] = total > 0.0 ? juce::jmax (0.05f, (float) (times[i] / total) * stageShare) : stageShare / 4.0f;

        float sum = 0.0f;

        for (float w : widths)
            sum += w;

        for (float& w : widths)
            w = w / sum * r.getWidth();

        const float top = r.getY() + 6.0f, bottom = r.getBottom() - 4.0f;
        const auto sustainY = bottom - (bottom - top) * (float) juce::jlimit (0.0, 1.0, z.sustain);
        float x = r.getX();
        juce::Path p;
        p.startNewSubPath (x, bottom);
        x += widths[0];
        p.lineTo (x, top);
        x += widths[1];
        p.lineTo (x, top);
        x += widths[2];
        p.lineTo (x, sustainY);
        x += widths[3];
        p.lineTo (x, sustainY);
        x += widths[4];
        p.lineTo (x, bottom);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.8f));

        const juce::String names[5] = { "attack " + formatSeconds (z.attack), "hold " + formatSeconds (z.hold), "decay " + formatSeconds (z.decay),
                                        "sustain " + juce::String (juce::roundToInt (z.sustain * 100.0)) + "%", "release " + formatSeconds (z.release) };
        float lx = r.getX();

        for (size_t i = 0; i < 5; ++i)
        {
            if (widths[i] > 56.0f)
                drawLabel (g, names[i], juce::Rectangle<float> (lx, labels.getY(), widths[i], labels.getHeight()), theme::textDim, juce::Justification::centred, 10.5f);

            lx += widths[i];
        }
    }

    /** A zone map with the keyboard under it, lined up, drawn in `r`. Returns the plot area. */
    juce::Rectangle<float> drawMapAndKeyboard (juce::Graphics& g, juce::Rectangle<float> r, const Source& s, int litKey)
    {
        auto keyboard = r.removeFromBottom (24.0f);
        r.removeFromBottom (2.0f);
        samplerpics::paintZoneMap (g, r, s.distinct, -1, litKey);
        samplerpics::paintKeyboard (g, keyboard, -1, litKey);
        return r;
    }

    //==============================================================================
    /** The General MIDI drum names the guide knows (the plan's list); the other notes show their number only. */
    juce::String getDrumName (int note)
    {
        switch (note)
        {
            case 36: return "kick";
            case 38: return "snare";
            case 42: return "closed hat";
            case 46: return "open hat";
            case 49: return "crash";
            case 51: return "ride";
            default: break;
        }

        return {};
    }

    void drawProgramGrid (juce::Graphics& g, juce::Rectangle<float> r, const Source& s)
    {
        const float labelWidth = 150.0f;
        const float cellW = (r.getWidth() - labelWidth) / 8.0f;
        const float cellH = r.getHeight() / 16.0f;
        const int litProgram = s.real && ! s.isDrumKit ? s.program : -1;

        for (int family = 0; family < samplerguide::numFamilies; ++family)
        {
            const float y = r.getY() + cellH * (float) family;
            drawLabel (g, samplerguide::getFamilyName (family) + "  " + juce::String (samplerguide::getFamilyFirstProgram (family)) + "-" + juce::String (samplerguide::getFamilyLastProgram (family)),
                       juce::Rectangle<float> (r.getX(), y, labelWidth - 6.0f, cellH), theme::textDim, juce::Justification::centredRight, 10.5f);

            for (int i = 0; i < 8; ++i)
            {
                const int program = family * 8 + i;   // 0 to 127, shown 1 to 128
                const auto cell = juce::Rectangle<float> (r.getX() + labelWidth + cellW * (float) i, y, cellW, cellH).reduced (1.0f);
                const bool lit = program == litProgram;
                g.setColour (lit ? samplerpics::colour : theme::panelLight);
                g.fillRoundedRectangle (cell, 2.0f);
                drawLabel (g, juce::String (program + 1), cell, lit ? theme::background : theme::textDim, juce::Justification::centred, 10.0f);
            }
        }
    }

    void drawDrumMap (juce::Graphics& g, juce::Rectangle<float> r, const Source& s)
    {
        const int first = 35, last = 81, columns = 12;
        const int rows = (last - first + columns) / columns;
        const float cellW = r.getWidth() / (float) columns, cellH = juce::jmin (r.getHeight() / (float) rows, 56.0f);

        for (int note = first; note <= last; ++note)
        {
            const int index = note - first;
            const auto cell = juce::Rectangle<float> (r.getX() + cellW * (float) (index % columns), r.getY() + cellH * (float) (index / columns), cellW, cellH).reduced (2.0f);
            const bool played = s.real && ! samplerpics::findZonesAt (s.distinct, note, -1).empty();
            g.setColour (played ? samplerpics::colour.withAlpha (0.85f) : theme::panelLight);
            g.fillRoundedRectangle (cell, 3.0f);
            const auto name = getDrumName (note);
            drawLabel (g, juce::String (note), cell.withTrimmedBottom (name.isEmpty() ? 0.0f : cell.getHeight() * 0.45f), played ? theme::background : theme::text,
                       juce::Justification::centred, 12.0f);

            if (name.isNotEmpty())
                drawLabel (g, name, cell.withTop (cell.getCentreY()), played ? theme::background : theme::textDim, juce::Justification::centred, 10.5f);
        }
    }

    //==============================================================================
    void drawBar (juce::Graphics& g, juce::Rectangle<float> r, double value, double maxValue, const juce::String& name, const juce::String& size, juce::Colour colour)
    {
        auto label = r.removeFromLeft (200.0f);
        drawLabel (g, name, label, theme::text, juce::Justification::centredLeft, 12.5f);
        const float width = (r.getWidth() - 90.0f) * (float) (value / juce::jmax (1.0, maxValue));
        g.setColour (colour);
        g.fillRoundedRectangle (r.getX(), r.getY() + 4.0f, juce::jmax (3.0f, width), r.getHeight() - 8.0f, 3.0f);
        drawLabel (g, size, juce::Rectangle<float> (r.getX() + width + 8.0f, r.getY(), 90.0f, r.getHeight()), theme::text, juce::Justification::centredLeft, 12.5f);
    }
}

//==============================================================================
void setTrackLookup (std::function<TrackNotes (const juce::AudioProcessor&)> lookup)
{
    getLookup() = std::move (lookup);
}

samplerguide::TrackView getTrackView (const BuiltInSampler& sampler, const std::vector<samplerguide::Note>& notes)
{
    samplerguide::TrackView view;
    const auto status = sampler.getStatus();
    view.fontName = sampler.getFontReference().fileName;
    view.fallbackSound = sampler.getFallbackSound();
    view.isDrumKit = sampler.getBank() == soundfonts::drumBank;
    view.presetName = sampler.getPresetName();
    const int transpose = juce::roundToInt (sampler.getSettings().transpose);

    switch (status)
    {
        case BuiltInSampler::Status::missing:
        case BuiltInSampler::Status::noFont:
        case BuiltInSampler::Status::failed:
            view.font = samplerguide::TrackView::Font::missing;
            break;

        case BuiltInSampler::Status::loading:
            view.font = samplerguide::TrackView::Font::loading;
            break;

        case BuiltInSampler::Status::noPreset:
        case BuiltInSampler::Status::ready:
            view.font = samplerguide::TrackView::Font::ready;
            view.zones = sampler.getZones();
            break;
    }

    for (auto& n : notes)
        view.notes.push_back ({ juce::jlimit (0, 127, n.key + transpose), n.velocity });

    return view;
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInSampler& sampler)
{
    BuiltInSampler* processor = &sampler;
    std::vector<fxui::Guide::Page> pages;

    for (auto page : samplerguide::allPages)
    {
        if (page == Page::sources)
            continue;

        pages.push_back ({ samplerguide::getPageName (page), samplerguide::getPageSummary (page), samplerguide::explain (page), {} });
    }

    // Alive while the guide is: a delayed second note checks it before it plays.
    auto alive = std::make_shared<int> (0);
    auto selected = std::make_shared<int> (0);   // the interval the speed picture shows, by place in getPictureIntervals()

    // 1. A sampler plays recordings: the recording that played for the last note.
    pages[0].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        const auto* z = findZone (s);
        drawCaption (g, r, z != nullptr ? "The recording for " + samplerguide::getNoteName (s.key) + " (" + zoneLabel (*z) + ")" : "The recording for " + samplerguide::getNoteName (s.key),
                     s.presetName);

        if (! s.real)
        {
            drawExampleNote (g, r);
            drawSyntheticWave (g, r.withTrimmedBottom (18.0f), 9.0, 1.0, samplerpics::colour);
            return;
        }

        if (z == nullptr)
        {
            drawLabel (g, "No recording plays " + samplerguide::getNoteName (s.key) + " in this sound: that key is silent.", r, theme::textDim, juce::Justification::centred, 14.0f);
            return;
        }

        drawRecording (g, r.withTrimmedBottom (18.0f), *s.font, *z, samplerpics::colour);
        drawLabel (g, juce::String (formatSeconds ((double) (z->sampleEnd - z->sampleStart) / (double) juce::jmax (1, z->sampleRate))) + " of recording, made at "
                          + samplerguide::getNoteName (z->rootKey) + ", played here at " + samplerguide::getNoteName (s.key),
                   r.removeFromBottom (16.0f), theme::textDim, juce::Justification::centredLeft, 11.5f);
    };

    // 2. Pitch from playback speed.
    pages[1].paint = [selected] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto& intervals = samplerguide::getPictureIntervals();
        const int n = intervals[(size_t) juce::jlimit (0, (int) intervals.size() - 1, *selected)];
        const auto sign = n > 0 ? "+" : "";
        drawCaption (g, r, "A3 and A3 " + juce::String (sign) + juce::String (n) + " semitones (" + samplerguide::getNoteName (57 + n) + ")",
                     "speed " + juce::String (samplerguide::getSpeed (n), 3) + " x, length " + juce::String (samplerguide::getLengthFactor (n), 3));

        auto chips = r.removeFromBottom (26.0f);
        const float chipW = chips.getWidth() / (float) intervals.size();

        for (size_t i = 0; i < intervals.size(); ++i)
        {
            const auto chip = juce::Rectangle<float> (chips.getX() + chipW * (float) i, chips.getY(), chipW, chips.getHeight()).reduced (3.0f);
            const bool on = (int) i == *selected;
            g.setColour (on ? samplerpics::colour : theme::panelLight);
            g.fillRoundedRectangle (chip, 4.0f);
            drawLabel (g, (intervals[i] > 0 ? "+" : "") + juce::String (intervals[i]), chip, on ? theme::background : theme::text, juce::Justification::centred, 12.0f);
        }

        const double cycles = 9.0;
        const float half = (r.getHeight() - 8.0f) / 2.0f;
        auto top = r.removeFromTop (half);
        r.removeFromTop (8.0f);
        drawLabel (g, "At its root, A3: 1.000 x", top.removeFromTop (14.0f), theme::textDim);
        drawSyntheticWave (g, top, cycles, 1.0, theme::textDim);
        drawLabel (g, "Moved " + juce::String (sign) + juce::String (n) + ": the same wave, " + (n > 0 ? "squeezed" : "stretched") + " to " + juce::String (juce::roundToInt (samplerguide::getLengthFactor (n) * 100.0)) + "% of the length",
                   r.removeFromTop (14.0f), samplerpics::colour);
        drawSyntheticWave (g, r, cycles, juce::jmin (1.0, samplerguide::getLengthFactor (n)), samplerpics::colour);
    };

    pages[1].clapText = "Hear both";
    pages[1].clapTooltip = "Plays A3, then the note shown moved by the interval, with this sampler's sound. Each click steps to the next interval.";
    pages[1].ownClap = [processor, selected, weak = std::weak_ptr<int> (alive)]
    {
        const auto& intervals = samplerguide::getPictureIntervals();
        const int n = intervals[(size_t) juce::jlimit (0, (int) intervals.size() - 1, *selected)];
        processor->playKey (57);
        juce::Timer::callAfterDelay (900, [processor, weak, selected, n]
        {
            if (weak.lock() == nullptr)
                return;

            processor->playKey (juce::jlimit (0, 127, 57 + n));
            *selected = (*selected + 1) % (int) samplerguide::getPictureIntervals().size();
        });
    };

    // 3. Key zones.
    pages[2].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        drawCaption (g, r, s.real ? "The key zones of " + s.presetName : "Key zones: a recording every 6 keys, in two layers",
                     samplerpics::describeKey (s.distinct, s.key, s.velocity));

        if (! s.real)
            drawExampleNote (g, r);

        drawMapAndKeyboard (g, r.withTrimmedBottom (s.real ? 0.0f : 18.0f), s, processor->lastNote.load());
    };

    // 4. Velocity layers: the layers under the last key.
    pages[3].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        const auto atKey = samplerpics::findZonesAt (s.distinct, s.key, -1);
        drawCaption (g, r, "The layers under " + samplerguide::getNoteName (s.key) + (atKey.empty() ? ": none" : ": " + juce::String ((int) atKey.size())),
                     "last velocity " + juce::String (s.velocity));

        if (! s.real)
            drawExampleNote (g, r);

        auto plot = r.withTrimmedBottom (s.real ? 0.0f : 18.0f).reduced (40.0f, 0.0f);
        g.setColour (theme::background.withAlpha (0.7f));
        g.fillRect (plot);

        for (int velocity : { 1, 32, 64, 96, 127 })
        {
            const float y = plot.getBottom() - plot.getHeight() * (float) velocity / 127.0f;
            g.setColour (theme::grid);
            g.fillRect (plot.getX(), y, plot.getWidth(), 1.0f);
            drawLabel (g, juce::String (velocity), juce::Rectangle<float> (plot.getX() - 36.0f, y - 7.0f, 32.0f, 14.0f), theme::textDim, juce::Justification::centredRight);
        }

        if (atKey.empty())
        {
            drawLabel (g, "No recording plays this key.", plot, theme::textDim, juce::Justification::centred, 14.0f);
            return;
        }

        for (int index : atKey)
        {
            const auto& z = s.distinct[(size_t) index];
            const float top = plot.getBottom() - plot.getHeight() * (float) z.highVelocity / 127.0f;
            const float bottom = plot.getBottom() - plot.getHeight() * (float) (z.lowVelocity - 1) / 127.0f;
            const auto band = juce::Rectangle<float> (plot.getX() + 2.0f, top + 1.0f, plot.getWidth() - 4.0f, bottom - top - 2.0f);
            g.setColour (samplerpics::getZoneColour (index).withAlpha (0.85f));
            g.fillRoundedRectangle (band, 3.0f);
            drawLabel (g, "velocity " + juce::String (z.lowVelocity) + "-" + juce::String (z.highVelocity) + ", root " + samplerguide::getNoteName (z.rootKey), band.reduced (8.0f, 0.0f),
                       theme::background, juce::Justification::centredLeft, 12.0f);
        }

        const float y = plot.getBottom() - plot.getHeight() * (float) s.velocity / 127.0f;
        g.setColour (theme::text);
        g.fillRect (plot.getX(), y - 1.0f, plot.getWidth(), 2.0f);
    };

    // 5. Loops and the envelope.
    pages[4].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        const auto* z = findZone (s);

        if (z == nullptr)
        {
            drawCaption (g, r, "The recording for " + samplerguide::getNoteName (s.key));
            drawLabel (g, "No recording plays this key.", r, theme::textDim, juce::Justification::centred, 14.0f);
            return;
        }

        const bool looped = z->loop != soundfonts::LoopMode::none;
        const juce::String loopName = z->loop == soundfonts::LoopMode::continuous ? "loops for as long as the note sounds"
                                    : z->loop == soundfonts::LoopMode::untilRelease ? "loops while the key is held, then plays on" : "plays once to its end";
        drawCaption (g, r, "The recording for " + samplerguide::getNoteName (s.key) + " " + loopName, zoneLabel (*z));

        if (! s.real)
            drawExampleNote (g, r);

        auto bottomPart = r.withTrimmedBottom (s.real ? 0.0f : 18.0f);
        auto envelopeArea = bottomPart.removeFromBottom (bottomPart.getHeight() * 0.38f);
        bottomPart.removeFromBottom (8.0f);
        std::function<float (int)> xOf;

        if (s.real)
            xOf = drawRecording (g, bottomPart, *s.font, *z, samplerpics::colour);
        else
        {
            drawSyntheticWave (g, bottomPart, 12.0, 1.0, samplerpics::colour);
            xOf = [bottomPart, z] (int position) { return bottomPart.getX() + bottomPart.getWidth() * (float) position / (float) juce::jmax (1, z->sampleEnd); };
        }

        float position = 0.0f;

        if (looped && z->loopEnd > z->loopStart)
        {
            const auto left = xOf (z->loopStart), right = xOf (z->loopEnd);
            g.setColour (theme::loop.withAlpha (0.16f));
            g.fillRect (left, bottomPart.getY(), right - left, bottomPart.getHeight());
            g.setColour (theme::loop);
            g.drawVerticalLine ((int) left, bottomPart.getY(), bottomPart.getBottom());
            g.drawVerticalLine ((int) right, bottomPart.getY(), bottomPart.getBottom());
            drawLabel (g, "loop start", juce::Rectangle<float> (left + 3.0f, bottomPart.getY(), 80.0f, 14.0f), theme::loop);
            drawLabel (g, "loop end", juce::Rectangle<float> (right - 83.0f, bottomPart.getY() + 14.0f, 80.0f, 14.0f), theme::loop, juce::Justification::centredRight);

            // The play position runs on at the recording's own speed, and goes round the loop.
            const double seconds = std::fmod (now(), 5.0);
            double sample = (double) z->sampleStart + seconds * (double) z->sampleRate;
            const double span = (double) (z->loopEnd - z->loopStart);

            if (sample >= (double) z->loopEnd)
                sample = (double) z->loopStart + std::fmod (sample - (double) z->loopStart, span);

            position = xOf ((int) sample);
        }
        else
        {
            const double length = juce::jmax (0.5, (double) (z->sampleEnd - z->sampleStart) / (double) juce::jmax (1, z->sampleRate));
            position = bottomPart.getX() + bottomPart.getWidth() * (float) juce::jlimit (0.0, 1.0, std::fmod (now(), length + 1.0) / length);
        }

        g.setColour (theme::playhead);
        g.drawVerticalLine ((int) position, bottomPart.getY(), bottomPart.getBottom());
        drawLabel (g, "volume envelope of this zone", envelopeArea.removeFromTop (14.0f), theme::textDim);
        drawEnvelope (g, envelopeArea, *z, soundfonts::LoopMode::none == z->loop ? samplerpics::colour : theme::meterGreen);
    };

    // 6. Inside a SoundFont.
    pages[5].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        drawCaption (g, r, s.real ? s.font->getName() : "A SoundFont, from the top down", s.real ? formatMb ((double) s.font->getFileSize()) + " on disk" : "");

        if (! s.real)
            drawExampleNote (g, r);

        std::set<std::pair<int, int>> recordings;

        for (auto& z : s.all)
            recordings.insert ({ z.sampleStart, z.sampleEnd });

        const int presets = s.real ? (int) s.font->getPresets().size() : 128;
        const juce::String lines[3] = {
            s.real ? juce::String (presets) + (presets == 1 ? " preset" : " presets") + " in the file: you choose one, such as " + s.presetName
                   : "Presets: what you choose, such as Acoustic Grand Piano (numbered in banks of up to 128)",
            juce::String ((int) s.all.size()) + (s.all.size() == 1 ? " zone" : " zones") + " in " + (s.real ? "this preset" : "the example") + ": a key range, a velocity range and a recording each",
            juce::String ((int) recordings.size()) + (recordings.size() == 1 ? " recording" : " recordings")
                + (s.real ? " used by this preset, out of " + juce::String (s.font->getSampleCount() / 1000) + " thousand sample values in the whole file" : ": mono, shared between zones and presets") };
        const juce::String titles[3] = { "Presets", "Zones", "Samples" };
        const juce::Colour colours[3] = { theme::accent, samplerpics::colour, theme::meterGreen };
        const float boxHeight = juce::jmin (70.0f, (r.getHeight() - 20.0f) / 3.0f);

        for (int i = 0; i < 3; ++i)
        {
            const auto box = juce::Rectangle<float> (r.getX() + 60.0f * (float) i, r.getY() + (boxHeight + 10.0f) * (float) i, r.getWidth() - 120.0f, boxHeight);
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (box, 5.0f);
            g.setColour (colours[i]);
            g.fillRoundedRectangle (box.withWidth (6.0f), 3.0f);
            drawLabel (g, titles[i], box.reduced (16.0f, 8.0f).removeFromTop (18.0f), colours[i], juce::Justification::centredLeft, 14.0f);
            drawLabel (g, lines[i], box.reduced (16.0f, 8.0f).withTrimmedTop (22.0f), theme::text, juce::Justification::centredLeft, 12.5f);
        }
    };

    // 7. General MIDI.
    pages[6].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);

        if (s.isDrumKit)
        {
            drawCaption (g, r, "The drum map: each note is a drum", "lit: notes this kit plays");
            drawDrumMap (g, r, s);
            return;
        }

        drawCaption (g, r, "The 128 General MIDI sounds, 16 families of 8", s.real ? "lit: program " + juce::String (s.program + 1) + ", " + s.presetName : "choose a sound to light it");
        drawProgramGrid (g, r, s);
    };

    // 8. Memory and loading.
    pages[7].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        double disk = 0.0, decoded = 0.0, held = 0.0;
        juce::String name;

        if (s.real)
        {
            name = s.font->getName();
            disk = (double) s.font->getFileSize();
            decoded = (double) s.font->getSampleCount() * 2.0;
            held = (double) s.font->getMemoryBytes();
        }
        else
        {
            name = "MuseScore General (an example)";
            disk = samplerguide::museScoreSf3Mb * 1.0e6;
            decoded = samplerguide::museScoreSf2Mb * 1.0e6;
            held = samplerguide::museScoreMemoryMb * 1.0e6;
        }

        drawCaption (g, r, "Where the memory goes: " + name);

        if (! s.real)
            drawExampleNote (g, r);

        const double maxValue = juce::jmax (disk, juce::jmax (decoded, held));
        const float row = juce::jmin (64.0f, r.getHeight() / 4.0f);
        r.removeFromTop (10.0f);
        drawBar (g, r.removeFromTop (row), disk, maxValue, "The file on disk", formatMb (disk), theme::textDim);
        drawBar (g, r.removeFromTop (row), decoded, maxValue, "Decoded, 16 bits a sample", formatMb (decoded), theme::accent);
        drawBar (g, r.removeFromTop (row), held, maxValue, "Held by DawIt, 32 bits", formatMb (held), samplerpics::colour);
    };

    // 9. Sampled or synthesised?
    pages[8].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = getSource (*processor);
        drawCaption (g, r, "Synthesised and sampled, side by side", s.real ? s.presetName : "");

        if (! s.real)
            drawExampleNote (g, r);

        r = r.withTrimmedBottom (s.real ? 0.0f : 18.0f);
        auto left = r.removeFromLeft ((r.getWidth() - 16.0f) / 2.0f);
        r.removeFromLeft (16.0f);
        drawLabel (g, "DawIt Synth: built from oscillators and a filter", left.removeFromTop (16.0f), theme::text, juce::Justification::centredLeft, 12.5f);
        drawLabel (g, "Sampler: a recording of the real thing", r.removeFromTop (16.0f), theme::text, juce::Justification::centredLeft, 12.5f);

        // A synth wave repeats exactly; a recording never does.
        g.setColour (theme::grid);
        g.drawHorizontalLine ((int) left.getCentreY(), left.getX(), left.getRight());
        juce::Path path;
        const int steps = juce::jmax (2, (int) left.getWidth() * 2);

        for (int i = 0; i <= steps; ++i)
        {
            const double t = (double) i / (double) steps;
            const double value = 0.55 * (2.0 * std::fmod (14.0 * t, 1.0) - 1.0) * std::exp (-1.5 * t);   // a fading saw
            const float x = left.getX() + left.getWidth() * (float) t;
            const float y = left.getCentreY() - (float) value * left.getHeight() * 0.8f;
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }

        g.setColour (theme::accent);
        g.strokePath (path, juce::PathStrokeType (1.4f));

        if (const auto* z = findZone (s); z != nullptr && s.real)
            drawRecording (g, r, *s.font, *z, samplerpics::colour);
        else
            drawSyntheticWave (g, r, 14.0, 1.0, samplerpics::colour);
    };

    pages[8].clapText = "Hear a short tune";
    pages[8].clapTooltip = "Plays four notes with this track's sound: the DawIt Synth until a SoundFont is ready, then the sampled sound.";
    pages[8].ownClap = [processor, weak = std::weak_ptr<int> (alive)]
    {
        const int keys[4] = { 60, 64, 67, 72 };
        processor->playKey (keys[0]);

        for (int i = 1; i < 4; ++i)
            juce::Timer::callAfterDelay (350 * i, [processor, weak, key = keys[i]]
            {
                if (weak.lock() != nullptr)
                    processor->playKey (key);
            });
    };

    // 10. Your track.
    const auto notesOf = [processor]
    {
        TrackNotes found;

        if (const auto& lookup = getLookup())
            found = lookup (*processor);

        return found;
    };

    pages[9].paint = [processor, notesOf] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto track = notesOf();
        const auto s = getSource (*processor);
        drawCaption (g, r, track.found ? track.trackName + ": its notes against " + s.presetName : "Your track", s.real ? "outside the zones in red" : "");

        if (! s.real)
        {
            drawLabel (g, processor->getStatus() == BuiltInSampler::Status::loading ? "The SoundFont is still loading." : "The pictures show the zones once a SoundFont is ready.",
                       r.removeFromBottom (16.0f), theme::textDim, juce::Justification::centredLeft, 11.5f);
        }

        auto plot = drawMapAndKeyboard (g, r, s, -1);

        if (! track.found)
            return;

        const int transpose = juce::roundToInt (processor->getSettings().transpose);

        for (auto& n : track.notes)
        {
            const int key = juce::jlimit (0, 127, n.key + transpose);
            const bool inside = ! samplerpics::findZonesAt (s.distinct, key, -1).empty();
            const float x = samplerpics::getKeyX (plot, key) + plot.getWidth() / 256.0f;
            const float y = plot.getBottom() - plot.getHeight() * (float) n.velocity / 127.0f;
            g.setColour ((inside || ! s.real ? theme::text : theme::meterRed).withAlpha (0.85f));
            g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
        }
    };

    pages[9].liveText = [processor, notesOf]
    {
        const auto track = notesOf();
        const auto status = processor->getStatus();

        if (! track.found)
            return samplerguide::explain (Page::yourTrack, nullptr);

        if (status == BuiltInSampler::Status::noFont)
            return juce::String ("This track has no SoundFont chosen yet, so the DawIt Synth's ")
                   + builtInSounds::getName (processor->getFallbackSound()).toLowerCase() + " plays. Choose one in the sampler's window (or from the Sampled sounds menu) and this page looks at your notes against its zones.";

        if (status == BuiltInSampler::Status::failed)
            return "This track's SoundFont couldn't be read (" + processor->getFailure() + "), so the DawIt Synth plays instead.";

        const auto view = getTrackView (*processor, track.notes);
        return samplerguide::explain (Page::yourTrack, &view);
    };

    pages[9].text = pages[9].liveText();
    return pages;
}
} // namespace daw::samplertext
