#include "CompressorExplainer.h"
#include "EffectWindow.h"
#include "engine/Tempo.h"

namespace daw
{
namespace comptext
{
    using comp::Knob;

    namespace
    {
        juce::String formatDb (double db)
        {
            return juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
        }

        juce::String formatTime (double ms)
        {
            if (ms >= 1000.0)
                return juce::String (ms / 1000.0, 2) + " s";

            return juce::String (ms, ms < 1.0 ? 2 : ms < 10.0 ? 1 : 0) + " ms";
        }

        juce::String formatRatio (double r)
        {
            return juce::String (r, r < 10.0 ? 1 : 0) + ":1";
        }
    }

    juce::String getTitle (Topic t)
    {
        switch (t)
        {
            case Topic::threshold: return "Threshold";
            case Topic::ratio:     return "Ratio";
            case Topic::knee:      return "Knee";
            case Topic::attack:    return "Attack";
            case Topic::release:   return "Release";
            case Topic::makeup:    return "Makeup";
            case Topic::mix:       return "Mix";
            case Topic::sidechain: return "Sidechain filter";
            case Topic::detection: return "Detection: peak or RMS";
            case Topic::overview:  return "DawIt Compressor";
            case Topic::curve:     return "The curve";
            case Topic::history:   return "The last 4 seconds";
            case Topic::meter:     return "Meters";
            case Topic::match:     return "Match level";
            case Topic::listen:    return "Hear what it removes";
            case Topic::presets:   return "Starting points";
        }

        return {};
    }

    juce::String describe (Topic t)
    {
        switch (t)
        {
            case Topic::threshold:
                return "The level where the compressor starts working. Whatever is louder than the threshold is turned down; whatever "
                       "is quieter passes untouched. The lower the threshold, the more of the track is compressed. Together with the "
                       "ratio, it decides how much the compressor does.";
            case Topic::ratio:
                return "How strongly what goes over the threshold is turned down. At 4:1, a level 4 dB over the threshold comes out "
                       "1 dB over, and 8 dB over comes out 2 dB over. 1:1 does nothing. From about 20:1 hardly anything gets over: "
                       "that's a limiter.";
            case Topic::knee:
                return "How the compression starts at the threshold. A hard knee (0 dB) does nothing just under the threshold and the "
                       "full ratio just over it: decisive, audible, punchy. A soft knee fades the ratio in from half the knee under the "
                       "threshold to half the knee over it: smoother and harder to hear.";
            case Topic::attack:
                return "How quickly the compressor turns down once the level goes over the threshold. After the attack time it has done "
                       "about two thirds of its work. A slow attack lets the start of each note or hit through before it clamps down: "
                       "more punch and snap. A fast attack catches the start too: smoother, softer and further away.";
            case Topic::release:
                return "How quickly the compressor lets go once the level drops back under the threshold. After the release time it "
                       "has let go about two thirds. Too fast, and it lets go between the waves of low notes (distortion) or brings "
                       "breaths and room up between hits (pumping). Too slow, and it never lets go, so everything after a loud part "
                       "stays turned down. At its best it comes back just before the next note or hit, so it breathes with the music.";
            case Topic::makeup:
                return "Compression makes the loud parts quieter, so the track ends up quieter as a whole. Makeup turns the result back "
                       "up. The loud parts are now held back, so the quiet parts end up louder than before: that's how compression "
                       "makes a track sound fuller and closer.";
            case Topic::mix:
                return "Blends the compressed track with the untouched one. 100% is only compressed. Lower is parallel compression: "
                       "the untouched part keeps the natural attack and dynamics, the compressed part fills in underneath. You can "
                       "compress hard without the squashed sound.";
            case Topic::sidechain:
                return "A low cut on what the compressor listens to, not on what you hear. The lows carry most of the energy, so a kick "
                       "or a bass note would drive most of the compression and pull everything else down with it (pumping). With the "
                       "filter, the compressor reacts more to the mids and highs, more like the ear. The sound keeps its lows.";
            case Topic::detection:
                return "How the compressor measures the level. Peak follows every peak of the waveform: quick to catch the start of a "
                       "hit, good for drums and for stopping peaks. RMS averages the level over 10 ms, which is closer to how loud it "
                       "sounds: smoother and more musical on vocals, bass and buses. A steady tone measures 3 dB lower as RMS than as "
                       "peak, so at the same threshold RMS compresses a little less.";
            case Topic::curve:
                return "What comes out (up) for each level that goes in (left to right), before makeup. The dashed diagonal is 1:1: "
                       "unchanged. Under the threshold the curve follows it; over it, the curve bends flatter, the more the higher the "
                       "ratio. The red area is how much is turned down.\n\nThe dot is the track right now. Above the curve, the "
                       "compressor is still catching up (attack); below it, it's letting go (release).\n\nDrag the threshold dot "
                       "sideways and the ratio dot at the right end up and down. Double-click to reset.";
            case Topic::history:
                return "The last 4 seconds, the newest on the right. Grey is the level coming in, red is what the compressor took off, "
                       "the blue line is what comes out, with makeup and mix. The orange dashed line is the threshold: wherever the "
                       "grey goes over it, red appears.\n\nWatch the red: it rises at the start of each loud part as fast as the attack "
                       "allows, and falls back afterwards as fast as the release allows. With RMS detection or the sidechain filter, a "
                       "thin white line shows what the compressor hears.";
            case Topic::meter:
                return "GR is the gain reduction: how many dB the compressor turns down right now. IN and OUT are the levels going in "
                       "and coming out. Most of the time the reduction should move: a needle stuck at the same value means the "
                       "compressor never lets go (release too long, or threshold far too low).";
            case Topic::match:
                return "Keeps the output as loud as what comes in, by setting the makeup itself from the last half second. Louder "
                       "almost always sounds better at first, so a compressor with makeup seems to improve things even when it "
                       "doesn't. With match level on, switching the compressor off and on compares only the sound. The makeup knob "
                       "waits while it's on.";
            case Topic::listen:
                return "Plays only what the compressor takes away: the input minus the compressed track. With a fast attack you hear "
                       "the clicks and cracks of the hits, with a slow one only what comes after them; under the threshold, nothing at "
                       "all. The quickest way to hear what attack and release do. Click again to hear the track.";
            case Topic::overview:
            case Topic::presets:
                break;
        }

        return {};
    }

    juce::String getTypical (Topic t)
    {
        switch (t)
        {
            case Topic::threshold:
                return "It depends on how loud the track is, so set it by ear and by the meter, not by number: lower it until the "
                       "loudest parts are turned down by about\n- vocals 3-6 dB, backing vocals 6-8 dB\n- kick, snare and bass 3-6 dB\n"
                       "- acoustic guitar 3-5 dB, piano and pads 2-3 dB\n- drum bus 2-3 dB, mix bus 1-2 dB\n- parallel compression 10-15 dB";
            case Topic::ratio:
                return "- 1.5:1 to 2:1, gentle: mix bus, piano, pads, the glue on a drum bus\n- 3:1 to 4:1, medium: vocals, bass, guitars, "
                       "kick and snare\n- 5:1 to 10:1, heavy: backing vocals that must sit still, parallel compression\n- 20:1: limiting peaks";
            case Topic::knee:
                return "- Hard (0-2 dB) on drums and for limiting, where you want the grab\n- Soft (6-12 dB) on vocals, bass, piano, pads "
                       "and buses: wherever the compression should go unnoticed";
            case Topic::attack:
                return "- Kick 20-30 ms, snare 5-20 ms: lets the click and crack through (a kick's click lasts 5-10 ms)\n- Vocals 2-10 ms, "
                       "electric guitar 5-20 ms, acoustic guitar 10-20 ms\n- Bass 10-30 ms, piano 20-30 ms, pads 30-80 ms\n- Drum bus and "
                       "mix bus 10-30 ms\n- Parallel squash 0.1-1 ms, limiting 0.1 ms";
            case Topic::release:
                return "- Kick 40-80 ms, snare 80-150 ms\n- Vocals 40-120 ms, guitars 100-150 ms\n- Bass 100-300 ms (shorter distorts low "
                       "notes)\n- Piano 200-300 ms, pads 300-500 ms\n- Drum bus and mix bus 100-300 ms, or fitted to the tempo\n- Limiting 30-100 ms";
            case Topic::makeup:
                return "About the reduction the meter shows on the loud parts, or a little less. Then compare with the compressor off at "
                       "the same loudness: Match level does the matching for you.";
            case Topic::mix:
                return "- 100% for most uses\n- 20-50% for parallel compression on drums (New York compression), vocals and bass\n"
                       "- 70-90% to keep some life on a vocal";
            case Topic::sidechain:
                return "- Off on kick, bass and limiters: their lows should drive the compressor\n- 80-150 Hz on drum buses and mix buses "
                       "(against pumping), vocals (against plosives) and guitars (against the body boom)";
            case Topic::detection:
                return "- Peak: drums, plucked and picked parts, limiting\n- RMS: bass, voice, piano, pads and buses";
            case Topic::overview:
            case Topic::curve:
            case Topic::history:
            case Topic::meter:
            case Topic::match:
            case Topic::listen:
            case Topic::presets:
                break;
        }

        return {};
    }

    juce::String describeSettings (Topic t, const comp::Settings& s, const Recent& r, double bpm)
    {
        juce::String text;
        const double over = r.loudestDb - s.thresholdDb, reduction = comp::getReductionDb (s, r.loudestDb);

        switch (t)
        {
            case Topic::threshold:
                text << "Threshold " << formatDb (s.thresholdDb) << ": whatever the compressor hears louder than that is turned down. ";

                if (! r.hasSignal())
                    text << "Play the track to see how far it goes over.";
                else if (over <= 0.0 && reduction < 0.1)
                    text << "In the last 4 seconds the track reached " << formatDb (r.loudestDb) << ", under the threshold: nothing was "
                         << "compressed. Lower the threshold below " << formatDb (r.loudestDb) << " to start.";
                else
                    text << "In the last 4 seconds the track reached " << formatDb (r.loudestDb) << ", " << formatDb (std::abs (over))
                         << (over >= 0.0 ? " over" : " under") << " it, so its loudest part was turned down by about " << formatDb (reduction) << ".";

                break;

            case Topic::ratio:
                if (s.ratio < 1.05f)
                {
                    text << "1:1: nothing is turned down, whatever the threshold.";
                    break;
                }

                text << formatRatio (s.ratio) << ": for every " << juce::String (s.ratio, 1) << " dB over the threshold, 1 dB comes out. ";

                if (r.hasSignal() && over > 0.5)
                    text << "The loudest part of the last 4 seconds, " << formatDb (over) << " over, comes out "
                         << formatDb (over - reduction) << " over: " << formatDb (reduction) << " quieter.";
                else
                    text << "A peak 12 dB over comes out " << formatDb (12.0 - 12.0 * (1.0 - 1.0 / s.ratio)) << " over.";

                break;

            case Topic::knee:
                if (s.kneeDb < 0.5f)
                    text << "Hard knee: whatever goes over " << formatDb (s.thresholdDb) << " gets the full " << formatRatio (s.ratio) << " at once.";
                else
                    text << "Knee " << formatDb (s.kneeDb) << ": the compression begins at " << formatDb (s.thresholdDb - s.kneeDb / 2.0f)
                         << " and reaches the full " << formatRatio (s.ratio) << " at " << formatDb (s.thresholdDb + s.kneeDb / 2.0f)
                         << ". At the threshold itself it turns down by " << formatDb (comp::getReductionDb (s, s.thresholdDb)) << ".";

                break;

            case Topic::attack:
                text << "Attack " << formatTime (s.attackMs) << ": the first " << formatTime (0.29 * s.attackMs) << " of a loud note get less "
                     << "than a quarter of the turning down, after " << formatTime (s.attackMs) << " it's two thirds, after "
                     << formatTime (3.0 * s.attackMs) << " 95%.";

                if (s.attackMs >= 15.0f)
                    text << " Slow: the start of each hit comes through.";
                else if (s.attackMs <= 2.0f)
                    text << " Fast: even the start of each hit is caught.";

                break;

            case Topic::release:
            {
                const double settled = 3.0 * s.releaseMs;
                text << "Release " << formatTime (s.releaseMs) << ": two thirds let go after " << formatTime (s.releaseMs) << ", 95% after "
                     << formatTime (settled) << ".";

                if (bpm > 0.0)
                {
                    const double beat = 60000.0 / bpm, eighth = beat / 2.0;
                    text << " At " << formatBpm (bpm) << " BPM a beat lasts " << formatTime (beat) << " and an eighth note "
                         << formatTime (eighth) << ": ";

                    if (settled <= eighth)
                        text << "it has let go before the next eighth note.";
                    else if (settled <= beat)
                        text << "it lets go within a beat, but not between eighth notes.";
                    else
                        text << "on a part that hits every beat, it doesn't fully let go before the next one.";
                }

                if (s.releaseMs < 40.0f)
                    text << " Very short: on low notes it starts following the wave itself, which distorts.";

                break;
            }

            case Topic::makeup:
                if (std::abs (s.makeupDb) < 0.05f)
                    text << "No makeup: the compressed track comes out quieter than it went in.";
                else
                    text << "Makeup " << comp::formatKnob (Knob::makeup, s) << ": the compressed track comes out " << formatDb (std::abs (s.makeupDb))
                         << (s.makeupDb > 0.0f ? " louder." : " quieter.");

                break;

            case Topic::mix:
                if (s.mixPercent >= 99.5f)
                    text << "Mix 100%: you hear only the compressed track.";
                else if (s.mixPercent <= 0.5f)
                    text << "Mix 0%: you hear only the untouched track; the compressor does nothing you can hear.";
                else
                    text << "Mix " << comp::formatKnob (Knob::mix, s) << ": " << comp::formatKnob (Knob::mix, s) << " compressed under "
                         << juce::String (100 - juce::roundToInt (s.mixPercent)) << "% untouched.";

                break;

            case Topic::sidechain:
                if (! comp::isSidechainFilterOn (s.sidechainHz))
                    text << "Off: the compressor listens to the whole track.";
                else
                    text << comp::formatKnob (Knob::sidechain, s) << ": the compressor hears the track with less below "
                         << comp::formatKnob (Knob::sidechain, s) << ": 12 dB less an octave under it (" << juce::String (juce::roundToInt (s.sidechainHz / 2.0f))
                         << " Hz), 24 dB less two octaves under (" << juce::String (juce::roundToInt (s.sidechainHz / 4.0f)) << " Hz). You still hear all of it.";

                break;

            case Topic::detection:
                text << (s.detection == comp::Detection::rms ? "RMS: the compressor follows the level averaged over 10 ms."
                                                             : "Peak: the compressor follows every peak of the waveform.");
                break;

            case Topic::history:
            case Topic::meter:
            case Topic::curve:
                if (r.hasSignal())
                    text << "In the last 4 seconds the compressor heard up to " << formatDb (r.loudestDb) << " and turned down by up to "
                         << formatDb (r.mostReductionDb) << ".";
                else
                    text << "Play the track to see it move.";

                break;

            case Topic::overview:
            case Topic::match:
            case Topic::listen:
            case Topic::presets:
                break;
        }

        return text;
    }
} // namespace comptext

//==============================================================================
namespace
{
    using comptext::Topic;
    using comp::Knob;

    void addTitle (juce::AttributedString& s, const juce::String& text, juce::Colour colour = theme::text)
    {
        s.append (text + "\n", juce::Font (juce::FontOptions (16.0f, juce::Font::bold)), colour);
    }

    void addLead (juce::AttributedString& s, const juce::String& text)
    {
        s.append (text + "\n\n", juce::Font (juce::FontOptions (13.5f)), theme::accent.brighter (0.4f));
    }

    void addSection (juce::AttributedString& s, const juce::String& heading, const juce::String& text)
    {
        if (heading.isNotEmpty())
            s.append (heading + "\n", juce::Font (juce::FontOptions (12.5f, juce::Font::bold)), theme::text);

        s.append (text + "\n\n", juce::Font (juce::FontOptions (12.5f)), theme::textDim.brighter (0.35f));
    }

    constexpr float targetWithoutPreset = 4.0f;
}

CompressorExplainer::CompressorExplainer (BuiltInCompressor& p) : processor (p)
{
    for (auto* b : { &lessonHelper, &lessonNext, &lessonCancel })
    {
        b->setWantsKeyboardFocus (false);
        addChildComponent (b);
    }

    lessonNext.onClick = [this]
    {
        if (lessonStep < 4)
            setLessonStep (lessonStep + 1);
        else
            endLesson (false);
    };

    lessonCancel.onClick = [this] { endLesson (true); };

    lessonHelper.onClick = [this]
    {
        const auto s = processor.getSettings();

        switch (lessonStep)
        {
            case 1:
            {
                const float target = juce::isPositiveAndBelow (presetIndex, (int) comp::getPresets().size())
                                         ? comp::getPresets()[(size_t) presetIndex].targetReductionDb : targetWithoutPreset;
                if (const auto threshold = findThreshold != nullptr ? findThreshold (s, target) : std::nullopt)
                    processor.setKnob (Knob::threshold, *threshold);

                break;
            }

            case 2: processor.listenToRemoved = ! processor.listenToRemoved.load(); break;
            case 3: processor.setKnob (Knob::release, juce::jlimit (comp::minRelease, comp::maxRelease, (float) (30000.0 / processor.tempoBpm.load()))); break;
            case 4: processor.setKnob (Knob::makeup, juce::jlimit (comp::minMakeup, comp::maxMakeup, std::round (processor.matchGainDb.load() * 10.0f) / 10.0f)); break;
            default: break;
        }

        refresh();
    };

    showOverview();
}

void CompressorExplainer::showOverview()
{
    showTopic (Topic::overview);
}

void CompressorExplainer::showTopic (Topic t)
{
    if (isInLesson())
        return;

    // Back on the starting points, the one picked last is what to read about.
    topic = t;
    showingPreset = t == Topic::presets && presetIndex >= 0;
    refresh();
}

void CompressorExplainer::showPreset (int index, bool thresholdFromTrack)
{
    if (isInLesson())
        return;

    topic = Topic::presets;
    presetIndex = index;
    presetThresholdFromTrack = thresholdFromTrack;
    showingPreset = true;
    refresh();
}

void CompressorExplainer::startLesson()
{
    if (isInLesson())
        return;

    lessonSaved = processor.getSettings();
    lessonSavedMatch = processor.isMatchingLevel();

    // From the starting point picked last, or the neutral one at 4:1, with nothing compressed yet.
    const auto& presets = comp::getPresets();
    auto start = juce::isPositiveAndBelow (presetIndex, (int) presets.size()) ? presets[(size_t) presetIndex].settings : presets.front().settings;

    if (! juce::isPositiveAndBelow (presetIndex, (int) presets.size()))
        start.ratio = 4.0f;

    start.thresholdDb = comp::maxThreshold;
    start.makeupDb = 0.0f;
    processor.setSettings (start);
    processor.setMatchingLevel (false);
    processor.listenToRemoved = false;

    for (auto* b : { &lessonHelper, &lessonNext, &lessonCancel })
        b->setVisible (true);

    lessonCancel.setButtonText ("Cancel");
    setLessonStep (1);

    if (onLessonChanged)
        onLessonChanged();
}

void CompressorExplainer::setLessonStep (int step)
{
    lessonStep = step;
    processor.listenToRemoved = false;
    lessonNext.setButtonText (step < 4 ? "Next" : "Done");
    refresh();
}

void CompressorExplainer::endLesson (bool restore)
{
    if (restore)
    {
        processor.setSettings (lessonSaved);
        processor.setMatchingLevel (lessonSavedMatch);
    }

    processor.listenToRemoved = false;
    lessonStep = 0;

    for (auto* b : { &lessonHelper, &lessonNext, &lessonCancel })
        b->setVisible (false);

    topic = Topic::overview;
    showingPreset = false;
    refresh();

    if (onLessonChanged)
        onLessonChanged();
}

void CompressorExplainer::updateHelperButton()
{
    if (lessonStep == 0)
        return;

    const double bpm = processor.tempoBpm.load();
    lessonHelper.setVisible (lessonStep != 3 || bpm > 0.0);

    switch (lessonStep)
    {
        case 1:
            lessonHelper.setButtonText ("Set it for me");
            lessonHelper.setEnabled (recent().hasSignal());
            lessonHelper.setTooltip (recent().hasSignal() ? "Sets the threshold from what just played" : "Play the track first");
            break;
        case 2:
            lessonHelper.setButtonText (processor.listenToRemoved.load() ? "Hear the track" : "Hear what it removes");
            lessonHelper.setEnabled (true);
            break;
        case 3:
            lessonHelper.setButtonText ("Fit to the tempo");
            lessonHelper.setEnabled (true);
            lessonHelper.setTooltip ("Sets the release to an eighth note at the song's tempo");
            break;
        default:
            lessonHelper.setButtonText ("Match it for me");
            lessonHelper.setEnabled (recent().hasSignal());
            lessonHelper.setTooltip (recent().hasSignal() ? "Sets the makeup so the track is as loud as without compression" : "Play the track first");
            break;
    }
}

CompressorExplainer::Picture CompressorExplainer::getPicture() const
{
    if (lessonStep == 2 || lessonStep == 3)
        return Picture::timing;

    if (lessonStep > 0 || showingPreset)
        return Picture::none;

    if (topic == Topic::attack || topic == Topic::release || topic == Topic::detection)
        return Picture::timing;

    if (topic == Topic::ratio || topic == Topic::knee)
        return Picture::curve;

    return Picture::none;
}

juce::AttributedString CompressorExplainer::buildText() const
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = processor.getSettings();
    const auto r = recent();
    const double bpm = processor.tempoBpm.load();
    const auto& presets = comp::getPresets();
    const comp::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) presets.size()) ? &presets[(size_t) presetIndex] : nullptr;
    const float target = preset != nullptr ? preset->targetReductionDb : targetWithoutPreset;
    const auto targetText = juce::String (target, 1) + " dB";

    if (lessonStep > 0)
    {
        addTitle (text, "Set it up step by step, " + juce::String (lessonStep) + " of 4", comptext::curveColour);

        switch (lessonStep)
        {
            case 1:
                addLead (text, "1. Threshold. Ratio " + comp::formatKnob (Knob::ratio, s) + ", threshold " + comp::formatKnob (Knob::threshold, s)
                                   + (s.thresholdDb > -0.5f ? ": nothing is compressed yet." : "."));
                addSection (text, "Do this", "Play the track (Space) and turn Threshold down slowly. Watch the red in the history and the GR "
                                             "meter, and stop when the loudest parts are turned down by about " + targetText + ". Or click Set it for me.");
                addSection (text, "Now", r.hasSignal() ? "The loudest part of the last 4 seconds is turned down by "
                                                             + juce::String (r.mostReductionDb, 1) + " dB."
                                                       : juce::String ("The track isn't playing."));
                addSection (text, "Why first", "The threshold decides which parts the compressor touches at all. Attack and release can only "
                                               "be judged once it's working.");
                break;

            case 2:
                addLead (text, "2. Attack, now " + comp::formatKnob (Knob::attack, s) + ".");
                addSection (text, "Do this", "Turn Attack all the way down and listen to the start of each note or hit: it gets softer and "
                                             "duller. Now turn it up slowly until the snap comes back. For punch, stop there; for a smoother, "
                                             "more even sound, stay a little lower.");
                addSection (text, "Hear it better", "Hear what it removes plays only what the compressor takes away: with a fast attack you "
                                                    "hear the clicks of the hits, with a slow one only what comes after them.");
                addSection (text, "Now", comptext::describeSettings (Topic::attack, s, r, bpm));
                break;

            case 3:
                addLead (text, "3. Release, now " + comp::formatKnob (Knob::release, s) + ".");
                addSection (text, "Do this", "Watch the red in the history after each loud part: it should fall back to almost nothing "
                                             "before the next note or hit. Too short, and the sound pumps: breaths and room rise up, low notes "
                                             "distort. Too long, and the red never falls back: the compressor just turns everything down.");
                addSection (text, "Now", comptext::describeSettings (Topic::release, s, r, bpm));

                if (bpm > 0.0)
                    addSection (text, "Fit to the tempo", "Sets the release to an eighth note, " + juce::String (juce::roundToInt (30000.0 / bpm))
                                                              + " ms at " + formatBpm (bpm) + " BPM: it then lets go in time with the song.");
                break;

            default:
                addLead (text, "4. Level. Makeup " + comp::formatKnob (Knob::makeup, s) + ".");
                addSection (text, "Do this", "The compressor made the loud parts quieter. Raise Makeup until the track is as loud as with the "
                                             "compressor off, or click Match it for me. Then switch the compressor off and on with its dot in the "
                                             "device bar while the track plays.");
                addSection (text, "Listen for", "Steadier, fuller, closer: good. Only smaller, flatter, or breathing: raise the threshold a "
                                                "little or lower the ratio. Done means done; Cancel brings back what you had before.");
                addSection (text, "Now", r.hasSignal() ? "To be as loud as without compression, the makeup would be "
                                                             + juce::String (processor.matchGainDb.load(), 1) + " dB."
                                                       : juce::String ("The track isn't playing."));
                break;
        }

        return text;
    }

    if (showingPreset && preset != nullptr)
    {
        addTitle (text, preset->name);
        addLead (text, preset->why);

        if (presetThresholdFromTrack)
            addSection (text, {}, "The threshold is set from what just played, so the loudest part gets about " + targetText
                                      + " of reduction. Play on and adjust by ear.");
        else
            addSection (text, {}, "The threshold is a guess, because it depends on how loud the track is: play the track and lower it until "
                                  "the loudest parts get about " + targetText + " of reduction, or pick this starting point again while it plays.");

        // The threshold as set from the track; the rest as the starting point has them.
        auto shown = preset->settings;

        if (presetThresholdFromTrack)
            shown.thresholdDb = s.thresholdDb;

        for (int k = 0; k < comp::numKnobs; ++k)
            addSection (text, comptext::getTitle ((Topic) k).upToFirstOccurrenceOf (":", false, false) + ": " + comp::formatKnob ((Knob) k, shown),
                        preset->notes[(size_t) k]);

        return text;
    }

    switch (topic)
    {
        case Topic::overview:
            addTitle (text, comptext::getTitle (topic));
            addLead (text, "A compressor turns down the parts of a track that get too loud, by itself, while the track plays. It makes "
                           "the difference between loud and quiet smaller: the track sits steadier in the mix, and with makeup it sounds "
                           "fuller and closer.");
            addSection (text, "When to use it", "When a part's level jumps around too much to sit in the mix: a vocal whose loud words "
                                                "jump out and quiet ones get lost, a bass whose notes aren't even, a kick or snare that "
                                                "needs more punch or body. Gently on a group of drums or the whole mix, to glue the parts "
                                                "together. In parallel (mix below 100%) to make drums or vocals dense while keeping their "
                                                "hits. At 20:1 with a fast attack, as a limiter against peaks. Not every track needs one: "
                                                "a part that already sits steady doesn't.");
            addSection (text, "How it's usually set", "Ratio 2:1 to 4:1, the threshold low enough that the loud parts come down 3-6 dB "
                                                      "(watch the red), an attack slow enough to let the start of each note through (10-30 "
                                                      "ms on drums, a few ms on vocals), a release that lets go before the next loud part, "
                                                      "then makeup to match the level. After an EQ that removes rumble, before reverb and "
                                                      "delay.");
            addSection (text, "How it works", "It listens to the level all the time. When the level goes over the Threshold, it turns "
                                              "down by the Ratio. Attack is how fast it reacts, Release how fast it lets go. Makeup brings the "
                                              "level back up afterwards.");
            addSection (text, "Reading the window", "Left, the curve: what goes in and what comes out. In the middle, the last 4 seconds: "
                                                    "grey what came in, red what was turned down, blue what came out. Right, the meters. "
                                                    "Hover anything to read about it here, and scroll here when the text runs long.");
            addSection (text, "Getting started", "Pick a starting point for your instrument; each tells why every knob is set the way it is. "
                                                 "Or click Set it up step by step. Make small moves, and compare with the compressor off at "
                                                 "the same loudness (Match level).");
            fxui::addPluginParagraph (text, processor);
            break;

        case Topic::presets:
            addTitle (text, comptext::getTitle (topic));
            addSection (text, {}, "Settings for common instruments, each explaining why every knob is set the way it is. Pick one while the "
                                  "track plays: the threshold is then set from the track itself. They are a start, not an answer: listen "
                                  "and adjust.");
            break;

        case Topic::match:
            addTitle (text, comptext::getTitle (topic));
            addLead (text, processor.isMatchingLevel()
                               ? "On: the makeup is " + juce::String (processor.matchGainDb.load(), 1) + " dB right now."
                               : "Off. Turned on, the makeup would be " + juce::String (processor.matchGainDb.load(), 1) + " dB right now.");
            addSection (text, {}, comptext::describe (topic));
            break;

        case Topic::listen:
            addTitle (text, comptext::getTitle (topic));
            addLead (text, processor.listenToRemoved.load() ? "On: you hear only what the compressor takes away." : "Off: you hear the compressed track.");
            addSection (text, {}, comptext::describe (topic));
            break;

        case Topic::curve:
        case Topic::history:
        case Topic::meter:
            addTitle (text, comptext::getTitle (topic));
            addLead (text, comptext::describeSettings (topic, s, r, bpm));
            addSection (text, {}, comptext::describe (topic));
            break;

        case Topic::threshold:
        case Topic::ratio:
        case Topic::knee:
        case Topic::attack:
        case Topic::release:
        case Topic::makeup:
        case Topic::mix:
        case Topic::sidechain:
        case Topic::detection:
        {
            const auto knob = (Knob) (int) topic;
            addTitle (text, comptext::getTitle (topic), comptext::curveColour);
            auto lead = comptext::describeSettings (topic, s, r, bpm);

            if (topic == Topic::makeup && processor.isMatchingLevel())
                lead = "Match level is on: it sets the makeup itself, " + juce::String (processor.matchGainDb.load(), 1) + " dB right now.";

            addLead (text, lead);
            addSection (text, "What it does", comptext::describe (topic));
            addSection (text, "When and how it's set", comptext::getTypical (topic));

            if (preset != nullptr)
                addSection (text, "In the " + preset->name + " starting point: " + comp::formatKnob (knob, preset->settings), preset->notes[(size_t) knob]);
            else
                addSection (text, {}, "Pick a starting point to read why it's set the way it is for that instrument.");

            break;
        }
    }

    return text;
}

void CompressorExplainer::refresh()
{
    updateHelperButton();
    const auto text = buildText();
    const auto picture = getPicture();
    const auto s = processor.getSettings();
    const auto page = juce::String ((int) topic) + "/" + juce::String ((int) showingPreset) + "/" + juce::String (presetIndex) + "/" + juce::String (lessonStep);

    // The pictures follow the settings they show.
    auto key = text.getText() + juce::String ((int) picture);

    if (picture != Picture::none)
        key << s.thresholdDb << s.ratio << s.kneeDb << s.attackMs << s.releaseMs << (int) s.detection;

    if (page != shownPage)
    {
        shownPage = page;
        scroll = 0.0f;
    }
    else if (key == shownText)
    {
        return;
    }

    shownText = key;
    shownAttributed = text;
    shownPicture = picture;
    resized();
    repaint();
}

void CompressorExplainer::resized()
{
    auto r = getLocalBounds().reduced (14);

    if (lessonStep > 0)
    {
        auto buttons = r.removeFromBottom (28);
        lessonCancel.setBounds (buttons.removeFromRight (74));
        buttons.removeFromRight (6);
        lessonNext.setBounds (buttons.removeFromRight (74));
        buttons.removeFromRight (6);
        lessonHelper.setBounds (buttons.removeFromRight (150));
        r.removeFromBottom (8);
    }

    pictureArea = shownPicture != Picture::none ? r.removeFromBottom (140) : juce::Rectangle<int>();

    if (shownPicture != Picture::none)
        r.removeFromBottom (8);

    textArea = r;
    layout.createLayout (shownAttributed, (float) juce::jmax (10, textArea.getWidth()));
    scroll = juce::jlimit (0.0f, juce::jmax (0.0f, layout.getHeight() - (float) textArea.getHeight()), scroll);
}

void CompressorExplainer::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const float most = juce::jmax (0.0f, layout.getHeight() - (float) textArea.getHeight());

    if (most <= 0.0f)
    {
        Component::mouseWheelMove (e, wheel);
        return;
    }

    scroll = juce::jlimit (0.0f, most, scroll - wheel.deltaY * (wheel.isReversed ? -160.0f : 160.0f));
    repaint();
}

void CompressorExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, 0, 1, getHeight());

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (textArea);
        layout.draw (g, textArea.toFloat().translated (0.0f, -scroll).withHeight (juce::jmax ((float) textArea.getHeight(), layout.getHeight())));
    }

    // Fade the text out where there's more to scroll to.
    const auto area = textArea.toFloat();

    if (layout.getHeight() - scroll > area.getHeight() + 1.0f)
    {
        g.setGradientFill (juce::ColourGradient (theme::panel.withAlpha (0.0f), 0.0f, area.getBottom() - 34.0f, theme::panel, 0.0f, area.getBottom(), false));
        g.fillRect (area.withTop (area.getBottom() - 34.0f));
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("scroll for more", area.withTop (area.getBottom() - 13.0f), juce::Justification::centredRight);
    }

    if (scroll > 0.5f)
    {
        g.setGradientFill (juce::ColourGradient (theme::panel, 0.0f, area.getY(), theme::panel.withAlpha (0.0f), 0.0f, area.getY() + 24.0f, false));
        g.fillRect (area.withHeight (24.0f));
    }

    if (shownPicture == Picture::timing)
        drawTimingPicture (g, pictureArea.toFloat());
    else if (shownPicture == Picture::curve)
        drawCurvePicture (g, pictureArea.toFloat());
}

void CompressorExplainer::drawTimingPicture (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto s = processor.getSettings();
    const Topic what = lessonStep == 2 ? Topic::attack : lessonStep == 3 ? Topic::release : topic;

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    auto header = area.removeFromTop (18.0f).reduced (6.0f, 0.0f);
    g.drawText (what == Topic::attack ? "WHAT ATTACK DOES TO THREE HITS" : what == Topic::release ? "WHAT RELEASE DOES TO THREE HITS"
                                                                                                  : "PEAK OR RMS ON THREE HITS",
                header, juce::Justification::centredLeft);
    area = area.reduced (6.0f, 3.0f);
    auto legend = area.removeFromBottom (13.0f);

    // Three drum-like hits 400 ms apart, each jumping to 12 dB over the threshold and dying away.
    constexpr double rate = 4000.0, seconds = 1.2;
    const float peak = juce::jlimit (-40.0f, 6.0f, s.thresholdDb + 12.0f), floor = peak - 40.0f;
    std::vector<float> level ((size_t) (rate * seconds));

    for (size_t i = 0; i < level.size(); ++i)
    {
        const double t = (double) i / rate, sinceHit = std::fmod (t - 0.05 + 0.4, 0.4);
        level[i] = t < 0.05 ? floor : juce::jmax (floor, peak - (float) (75.0 * sinceHit));
    }

    const float top = peak + 3.0f, bottom = floor;
    auto toY = [&] (double db) { return juce::jmap ((float) juce::jlimit ((double) bottom, (double) top, db), bottom, top, area.getBottom(), area.getY()); };
    auto toX = [&] (size_t i) { return area.getX() + area.getWidth() * (float) i / (float) level.size(); };

    // The hits as they come in.
    juce::Path in;
    in.startNewSubPath (area.getX(), area.getBottom());

    for (size_t i = 0; i < level.size(); i += 4)
        in.lineTo (toX (i), toY (level[i]));

    in.lineTo (area.getRight(), area.getBottom());
    in.closeSubPath();
    g.setColour (theme::textDim.withAlpha (0.3f));
    g.fillPath (in);

    const float thresholdY = toY (s.thresholdDb);
    g.setColour (comptext::curveColour.withAlpha (0.7f));
    const float dashes[] = { 4.0f, 3.0f };
    g.drawDashedLine ({ area.getX(), thresholdY, area.getRight(), thresholdY }, dashes, 2, 1.0f);

    auto output = [&] (const comp::Settings& settings)
    {
        const auto reduction = comp::simulateReduction (settings, level, rate);
        juce::Path p;

        for (size_t i = 0; i < level.size(); i += 4)
        {
            const float y = toY (level[i] - reduction[i]);
            i == 0 ? p.startNewSubPath (toX (i), y) : p.lineTo (toX (i), y);
        }

        return p;
    };

    // Other settings in grey, so the difference shows.
    std::vector<std::pair<comp::Settings, juce::String>> examples;
    auto example = [&] (Knob k, float value)
    {
        auto e = s;
        (k == Knob::attack ? e.attackMs : e.releaseMs) = value;
        examples.push_back ({ e, comp::formatKnob (k, e) });
    };

    if (what == Topic::attack)
    {
        example (Knob::attack, 0.5f);
        example (Knob::attack, 40.0f);
    }
    else if (what == Topic::release)
    {
        example (Knob::release, 30.0f);
        example (Knob::release, 800.0f);
    }
    else
    {
        auto e = s;
        e.detection = s.detection == comp::Detection::rms ? comp::Detection::peak : comp::Detection::rms;
        examples.push_back ({ e, comp::formatKnob (Knob::detection, e) });
    }

    g.setColour (theme::textDim.withAlpha (0.6f));

    for (auto& [settings, name] : examples)
        g.strokePath (output (settings), juce::PathStrokeType (1.0f));

    g.setColour (comptext::curveColour);
    g.strokePath (output (s), juce::PathStrokeType (2.0f));

    juce::String names;
    for (auto& e : examples)
        names << e.second << "   ";

    g.setFont (juce::FontOptions (10.5f));
    g.setColour (theme::textDim);
    g.drawText ("filled: in.  grey: " + names, legend, juce::Justification::bottomLeft);
    const Knob shown = what == Topic::attack ? Knob::attack : what == Topic::release ? Knob::release : Knob::detection;
    g.setColour (comptext::curveColour);
    g.drawText (comp::formatKnob (shown, s) + " (yours)", header, juce::Justification::centredRight);
}

void CompressorExplainer::drawCurvePicture (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto s = processor.getSettings();

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    auto header = area.removeFromTop (18.0f).reduced (6.0f, 0.0f);
    g.drawText (topic == Topic::ratio ? "WHAT THE RATIO DOES" : "WHAT THE KNEE DOES (CLOSE UP)", header, juce::Justification::centredLeft);
    area = area.reduced (6.0f, 3.0f);
    auto legend = area.removeFromBottom (13.0f);

    // In and out around the threshold: wide for the ratio, close up for the knee.
    const float span = topic == Topic::ratio ? 24.0f : 12.0f;
    const float from = s.thresholdDb - span, to = s.thresholdDb + span;
    const auto plot = area.withSizeKeepingCentre (juce::jmin (area.getWidth(), area.getHeight() * 1.6f), area.getHeight());
    auto toX = [&] (double db) { return juce::jmap ((float) db, from, to, plot.getX(), plot.getRight()); };
    auto toY = [&] (double db) { return juce::jmap ((float) juce::jlimit ((double) from, (double) to, db), from, to, plot.getBottom(), plot.getY()); };

    const float dashes[] = { 4.0f, 3.0f };
    g.setColour (theme::gridBar);
    g.drawDashedLine ({ plot.getX(), plot.getBottom(), plot.getRight(), plot.getY() }, dashes, 2, 1.0f);
    g.setColour (comptext::curveColour.withAlpha (0.35f));
    g.fillRect (toX (s.thresholdDb), plot.getY(), 1.0f, plot.getHeight());

    auto curve = [&] (const comp::Settings& settings)
    {
        juce::Path p;

        for (int x = 0; x <= (int) plot.getWidth(); x += 2)
        {
            const double in = from + (to - from) * x / plot.getWidth();
            const float y = toY (in - comp::getReductionDb (settings, in));
            x == 0 ? p.startNewSubPath (plot.getX() + (float) x, y) : p.lineTo (plot.getX() + (float) x, y);
        }

        return p;
    };

    std::vector<comp::Settings> examples;
    juce::String names;

    for (float v : topic == Topic::ratio ? std::initializer_list<float> { 1.5f, 4.0f, 20.0f } : std::initializer_list<float> { 0.0f, 12.0f })
    {
        auto e = s;
        (topic == Topic::ratio ? e.ratio : e.kneeDb) = v;
        examples.push_back (e);
        names << comp::formatKnob (topic == Topic::ratio ? Knob::ratio : Knob::knee, e) << "   ";
    }

    g.setColour (theme::textDim.withAlpha (0.6f));

    for (auto& e : examples)
        g.strokePath (curve (e), juce::PathStrokeType (1.0f));

    g.setColour (comptext::curveColour);
    g.strokePath (curve (s), juce::PathStrokeType (2.0f));

    g.setFont (juce::FontOptions (10.5f));
    g.setColour (theme::textDim);
    g.drawText ("in: left to right, out: up.  grey: " + names, legend, juce::Justification::bottomLeft);
    g.setColour (comptext::curveColour);
    g.drawText (comp::formatKnob (topic == Topic::ratio ? Knob::ratio : Knob::knee, s) + " (yours)", header, juce::Justification::centredRight);
}
} // namespace daw
