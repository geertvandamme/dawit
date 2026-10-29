#include "BuiltInReverb.h"
#include "BuiltInFormat.h"
#include "ReverbKinds.h"

namespace daw
{
namespace verb
{
    namespace
    {
        // The delay lines at the largest size, in ms: no two share a factor, so their echoes rarely line up.
        constexpr double lineMs[Network::numLines] = { 29.7, 37.1, 41.1, 43.7, 53.0, 59.9, 67.7, 79.3 };

        // Where the input goes into the lines, and how the lines make the left and right outputs.
        constexpr float inputSigns[Network::numLines] = { 1, -1, -1, 1, -1, 1, 1, -1 };
        constexpr float leftSigns[Network::numLines]  = { 1, -1, 1, -1, 1, -1, 1, -1 };
        constexpr float rightSigns[Network::numLines] = { 1, 1, -1, -1, 1, 1, -1, -1 };
        constexpr float outputGain = 1.6f;   // steady noise comes out about as loud as it goes in

        // The all-passes that smear the input before the lines, in ms.
        constexpr double diffuserMs[Network::numDiffusers] = { 4.77, 3.59, 2.73, 1.79 };
        constexpr float diffusion = 0.6f;

        // The early reflections, as multiples of the first one's time, and how far left (+) or right (-) each is.
        constexpr double tapTimes[Network::numTaps] = { 1.0, 1.21, 1.43, 1.62, 1.87, 2.08, 2.31, 2.6, 2.86, 3.13, 3.44, 3.8 };
        constexpr float tapPans[Network::numTaps] = { 0.35f, -0.4f, -0.2f, 0.45f, 0.1f, -0.45f, 0.3f, -0.15f, 0.4f, -0.35f, 0.2f, -0.25f };

        /** How much the size stretches the delay lines: 0.2 at 0% to 1 at 100%. */
        double getLineScale (float sizePercent)
        {
            return 0.2 + 0.8 * juce::jlimit (0.0f, 100.0f, sizePercent) / 100.0;
        }

        /** Mixes the eight lines into each other without adding or losing energy (a Hadamard matrix). */
        void mixLines (float (&v)[Network::numLines])
        {
            for (int span = 1; span < Network::numLines; span *= 2)
                for (int i = 0; i < Network::numLines; i += 2 * span)
                    for (int j = i; j < i + span; ++j)
                    {
                        const float a = v[j], b = v[j + span];
                        v[j] = a + b;
                        v[j + span] = a - b;
                    }

            constexpr float scale = 0.35355339f;   // 1 / sqrt (8)

            for (auto& x : v)
                x *= scale;
        }

        inline double runStages (const eq::Stages& stages, double (&memory)[2][2], double x)
        {
            for (int st = 0; st < stages.num; ++st)
            {
                const auto& f = stages.filters[(size_t) st];
                auto& z = memory[st];
                const double y = f.b0 * x + z[0];
                z[0] = f.b1 * x - f.a1 * y + z[1];
                z[1] = f.b2 * x - f.a2 * y;
                x = y;
            }

            return x;
        }
    }

    double getRoomMetres (float sizePercent)
    {
        return 5.0 * std::pow (8.0, juce::jlimit (0.0f, 100.0f, sizePercent) / 100.0);
    }

    double getFirstReflectionMs (float sizePercent)
    {
        // The nearest wall adds about a fifth of the room's width to the way the sound travels.
        return getTravelMs (0.2 * getRoomMetres (sizePercent));
    }

    double getLoopGain (double delaySeconds, double decaySeconds)
    {
        return std::pow (10.0, -3.0 * delaySeconds / juce::jmax (0.01, decaySeconds));
    }

    double getDampingPole (double loopGain, double dampingHz, double sampleRate)
    {
        if (dampingHz >= 0.49 * sampleRate)
            return 0.0;

        // |g (1 - b) / (1 - b e^-jw)| = g^2 at the damping frequency, solved for b.
        const double r2 = loopGain * loopGain, c = std::cos (juce::MathConstants<double>::twoPi * dampingHz / sampleRate);
        const double a = 1.0 - r2, half = 1.0 - r2 * c;

        if (a < 1.0e-9)
            return 0.0;

        return juce::jlimit (0.0, 0.999, (half - std::sqrt (juce::jmax (0.0, half * half - a * a))) / a);
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        auto percent = [] (float v) { return juce::String (juce::roundToInt (v)) + "%"; };
        auto hz = [] (float v)
        {
            if (v >= 1000.0f)
                return juce::String (v / 1000.0f, 1) + " kHz";

            return juce::String (juce::roundToInt (v)) + " Hz";
        };

        switch (k)
        {
            case Knob::predelay: return fx::formatNumber (s.predelayMs, s.predelayMs < 10.0f && s.predelayMs > 0.05f ? 1 : 0) + " ms";
            case Knob::size:     return percent (s.sizePercent);
            case Knob::decay:    return juce::String (s.decaySeconds, s.decaySeconds < 10.0f ? 2 : 1) + " s";
            case Knob::damping:  return isDampingOn (s.dampingHz) ? hz (s.dampingHz) : juce::String ("Off");
            case Knob::lowCut:   return isLowCutOn (s.lowCutHz) ? hz (s.lowCutHz) : juce::String ("Off");
            case Knob::early:    return percent (s.earlyPercent);
            case Knob::width:    return percent (s.widthPercent);
            case Knob::mix:      return percent (s.mixPercent);
        }

        return {};
    }

    const Preset& getReturnPreset()
    {
        for (auto& preset : getPresets())
            if (preset.name == "Return: plate")
                return preset;

        return getPresets().front();
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;
            auto add = [&list] (juce::String name, juce::String why, Settings s, std::array<juce::String, numKnobs> notes)
            {
                list.push_back ({ std::move (name), std::move (why), s, std::move (notes), {} });
            };

            add ("Neutral start", "A medium room with a moderate tail: something to hear every knob on. Turn one knob at a time and "
                                  "listen to what changes.",
                 { 20.0f, 50.0f, 1.8f, 6000.0f, 150.0f, 50.0f, 100.0f, 25.0f },
                 { "20 ms: a short gap between the sound and its reverb, so the start of each note stays clear.",
                   "50%: a room about 14 m across, like a large hall in a school.",
                   "1.8 s: the tail of a small concert hall.",
                   "6 kHz: the highest overtones die away sooner than the rest, as they do in a real room.",
                   "150 Hz: the reverb has no deep lows, so it doesn't make the bass muddy.",
                   "50%: the first echoes off the walls are clearly there, but not on top.",
                   "100%: the reverb comes from all around, as wide as the speakers go.",
                   "25%: the dry sound leads, the room is behind it." });

            add ("Lead vocal", "A vocal should sound close and clear, with a space around it that you feel more than hear. A longer "
                               "pre-delay keeps the words out of the reverb, and a low cut keeps the reverb from clouding them.",
                 { 70.0f, 55.0f, 1.6f, 7000.0f, 250.0f, 25.0f, 100.0f, 18.0f },
                 { "70 ms: every word is heard dry first, then the reverb follows. That gap keeps the singer up front and the words "
                   "understandable. At the song's tempo, try a 1/16 or 1/32 note.",
                   "55%: a medium-large room: big enough to sound smooth, not so big it sounds like a church.",
                   "1.6 s: long enough to carry the end of a phrase, short enough to clear before the next. Slower songs can take 2-3 s.",
                   "7 kHz: keeps the shine of the voice in the reverb, but softens the s and t sounds, which would otherwise hiss in the tail.",
                   "250 Hz: a voice's low end in the reverb only muddies the mix.",
                   "25%: early reflections would make the voice sound like it was recorded in a room; a little is enough.",
                   "100%: the reverb around the voice, while the dry voice stays in the middle.",
                   "18%: it should be just audible. Turn it up until you hear it, then back a little: if you miss it when it's off, it's right." });

            add ("Ballad vocal", "In a slow song there is time between the phrases for a long, lush tail. The pre-delay keeps the words "
                                 "clear despite the long decay.",
                 { 90.0f, 80.0f, 3.0f, 5500.0f, 250.0f, 20.0f, 100.0f, 22.0f },
                 { "90 ms: a clear gap, so the long tail doesn't swallow the words.",
                   "80%: a large hall.",
                   "3 s: the tail rings into the pauses between phrases.",
                   "5.5 kHz: a darker tail sounds warm and far away, and doesn't compete with the voice itself.",
                   "250 Hz: keeps the long tail out of the low end.",
                   "20%: mostly tail, little room: it sounds lush rather than like a real place.",
                   "100%.",
                   "22%." });

            add ("Small room", "A small room around a dry recording: it sounds like it was played in a real space, without sounding "
                               "reverberant. Good on guitars, percussion and keys recorded direct.",
                 { 5.0f, 20.0f, 0.6f, 8000.0f, 100.0f, 70.0f, 80.0f, 20.0f },
                 { "5 ms: in a small room the walls are close, so the reflections come almost at once.",
                   "20%: a room about 8 m across, like a living room.",
                   "0.6 s: a furnished room dies away quickly.",
                   "8 kHz: a little softer in the highs, like curtains and furniture.",
                   "100 Hz.",
                   "70%: in a small room, the early reflections are most of what you hear: they give the sense of the walls.",
                   "80%: a little narrower than full, so it stays close.",
                   "20%." });

            add ("Drum room", "Drums recorded close sound small. A short, bright room with strong early reflections puts the kit back "
                              "in a space and makes it sound bigger and more alive.",
                 { 0.0f, 30.0f, 0.9f, 9000.0f, 120.0f, 85.0f, 100.0f, 22.0f },
                 { "0 ms: the room starts with the hit, like the room microphones of a real kit.",
                   "30%: a studio live room, about 9 m across.",
                   "0.9 s: short enough that the hits don't run into each other. At a fast tempo, shorter.",
                   "9 kHz: cymbals stay bright, without a hissy tail.",
                   "120 Hz: the kick's boom stays dry and tight.",
                   "85%: the early reflections are the sound of a room on drums: they make each hit thicker.",
                   "100%.",
                   "22%. More on the snare and toms, less or none on the kick." });

            add ("Snare plate", "A bright, dense tail with no early reflections, like the steel plate reverbs of 1970s and 80s records: "
                                "it gives a snare a long shimmering crack.",
                 { 10.0f, 45.0f, 1.5f, 12000.0f, 300.0f, 0.0f, 100.0f, 25.0f },
                 { "10 ms: the crack comes first, then the shimmer.",
                   "45%: a plate has no walls, but its tail is dense like a mid-sized room's.",
                   "1.5 s: rings between the snare hits. To the tempo: it should fade just before the next snare.",
                   "12 kHz: a plate is bright; the highs ring almost as long as the rest.",
                   "300 Hz: only the crack and the wires go into the reverb, not the body.",
                   "0%: a plate has no walls, so no early reflections: just the tail.",
                   "100%.",
                   "25%." });

            add ("Piano or acoustic guitar", "An acoustic instrument in a hall: natural, with early reflections that place it in the "
                                             "space and a warm tail.",
                 { 25.0f, 70.0f, 2.2f, 5000.0f, 100.0f, 45.0f, 100.0f, 25.0f },
                 { "25 ms: like a player a few metres from the nearest wall.",
                   "70%: a concert hall, about 22 m across.",
                   "2.2 s: the decay of a concert hall that suits solo instruments.",
                   "5 kHz: a hall's air and seats soften the highs; a warm tail sounds natural.",
                   "100 Hz: the piano's deepest notes can make the reverb boom.",
                   "45%: enough to hear the hall.",
                   "100%.",
                   "25%." });

            add ("Strings or choir", "Strings, choirs and orchestral sounds belong in a big hall: a long, smooth, warm tail ties the "
                                     "voices together.",
                 { 30.0f, 90.0f, 3.2f, 4500.0f, 60.0f, 50.0f, 100.0f, 30.0f },
                 { "30 ms.",
                   "90%: a large hall, over 30 m across.",
                   "3.2 s: long and smooth; slow music lives in it.",
                   "4.5 kHz: warm: in a big hall the air itself absorbs the highs.",
                   "60 Hz: cellos and basses keep a little of their low reverb.",
                   "50%.",
                   "100%: a wide, enveloping space.",
                   "30%: more than on a pop track, because the hall is part of the sound." });

            add ("Ambient wash", "A huge, dark tail that turns any sound into a pad. For atmospheres, long notes and transitions; not for "
                                 "anything that should stay clear.",
                 { 100.0f, 100.0f, 8.0f, 3500.0f, 200.0f, 10.0f, 100.0f, 45.0f },
                 { "100 ms.",
                   "100%: as big as it goes, about 40 m.",
                   "8 s: far longer than any real room: the tail holds the sound like a pedal.",
                   "3.5 kHz: dark, so the tail becomes a soft cloud behind the sound.",
                   "200 Hz: keeps the long tail out of the low end.",
                   "10%: almost no walls, just the tail.",
                   "100%.",
                   "45%: the tail is half the sound. For a pure wash, turn on Hear only the reverb." });

            add ("Just a touch", "A tiny, short room that you don't hear as reverb at all: it only takes away the dryness of sounds "
                                 "from a synth or a direct input, so they blend with the rest.",
                 { 0.0f, 10.0f, 0.4f, 9000.0f, 150.0f, 90.0f, 70.0f, 12.0f },
                 { "0 ms.",
                   "10%: a small room, about 6 m.",
                   "0.4 s: over before you notice it.",
                   "9 kHz.",
                   "150 Hz.",
                   "90%: almost all early reflections: thicker rather than reverberant.",
                   "70%.",
                   "12%: switch it off and on: you should hear the sound get thinner and drier when it's off, not hear a reverb when it's on." });

            add ("Bass", "Basses and kicks need little or no reverb: a tail in the low end turns into mud. If any, a very short, narrow "
                         "room with all the lows cut out.",
                 { 0.0f, 15.0f, 0.3f, 4000.0f, 400.0f, 60.0f, 0.0f, 8.0f },
                 { "0 ms.",
                   "15%: a small room.",
                   "0.3 s: very short.",
                   "4 kHz.",
                   "400 Hz: only the bass's upper tone goes into the reverb; the lows stay dry and steady.",
                   "60%.",
                   "0%: mono, so the low end stays in the middle.",
                   "8%: just enough to give it a place." });

            add ("Return: plate", "For a return track: one bright, smooth plate that vocals, snare and keys can share. The mix is at 100%, "
                                  "so the return carries only reverb and each track's send sets how much it gets.",
                 { 40.0f, 45.0f, 1.6f, 9000.0f, 250.0f, 0.0f, 100.0f, 100.0f },
                 { "40 ms: each sound starts dry, then the reverb follows, so words and hits stay clear.",
                   "45%: a plate has no walls, but its tail is dense like a mid-sized room's.",
                   "1.6 s: carries the end of a phrase and clears before the next at most tempos.",
                   "9 kHz: a plate is bright; softening only the very top keeps s and t sounds from hissing in the tail.",
                   "250 Hz: the shared reverb takes no lows from any track, so the low end stays clear for all of them.",
                   "0%: a plate has no walls, so no early reflections: just the tail.",
                   "100%: the reverb spreads around every sound that sends to it.",
                   "100%: on a return, only the reverb. The dry sound comes from the tracks themselves, and the sends set the amounts." });

            add ("Return: short room", "For a return track: a small, short room that puts dry tracks in one space without a tail you notice. "
                                       "Send a little of the drums, guitars and keys to it, so they sound played together.",
                 { 5.0f, 25.0f, 0.7f, 8000.0f, 200.0f, 75.0f, 100.0f, 100.0f },
                 { "5 ms: in a small room the walls are close, so the reflections come almost at once.",
                   "25%: a room about 8 m across.",
                   "0.7 s: short enough that it isn't heard as a tail, only as a space.",
                   "8 kHz: a little softer in the highs, like a furnished room.",
                   "200 Hz: keeps the kick and bass out of the room.",
                   "75%: in a small room, the early reflections are most of what you hear.",
                   "100%.",
                   "100%: on a return, only the reverb." });

            add ("Return: hall", "For a return track: a warm concert hall for ballads, strings, pads and choirs, shared so they sit in one "
                                 "big space.",
                 { 30.0f, 75.0f, 2.4f, 5500.0f, 200.0f, 40.0f, 100.0f, 100.0f },
                 { "30 ms: keeps the start of each note clear.",
                   "75%: a hall about 24 m across.",
                   "2.4 s: a concert hall's tail; slower songs can take 3 s.",
                   "5.5 kHz: a hall's air and seats soften the highs, so the tail sounds warm and far away.",
                   "200 Hz: keeps the long tail out of the low end.",
                   "40%: enough early reflections to hear the walls of a hall.",
                   "100%: a wide, enveloping space.",
                   "100%: on a return, only the reverb." });

            for (auto& preset : list)
            {
                if (preset.name == "Neutral start")
                {
                    preset.settings.type = Type::digital;
                    preset.typeNote = "Digital: DawIt Reverb's own, with every knob doing just what it says. Switch Type to hear the same settings as a room, a hall, a chamber, a plate or a spring.";
                }
                if (preset.name == "Lead vocal")
                {
                    preset.settings.type = Type::plate;
                    preset.typeNote = "Plate: bright and dense, with no early reflections, so the voice sounds close and the words stay in front. The usual vocal reverb since the 1960s.";
                }
                if (preset.name == "Ballad vocal")
                {
                    preset.settings.type = Type::hall;
                    preset.typeNote = "Hall: a slow song has room for a long, smooth tail that swells after each phrase; the pre-delay keeps the words clear.";
                }
                if (preset.name == "Small room")
                {
                    preset.settings.type = Type::room;
                    preset.typeNote = "Room: close walls and quick early reflections make a dry recording sound played in a real space.";
                }
                if (preset.name == "Drum room")
                {
                    preset.settings.type = Type::room;
                    preset.typeNote = "Room: a kit is usually recorded with room microphones as well; a short room with strong early reflections does the same.";
                }
                if (preset.name == "Snare plate")
                {
                    preset.settings.type = Type::plate;
                    preset.typeNote = "Plate: the dense, bright tail of 1970s and 80s snares, with no early reflections to make it boxy.";
                }
                if (preset.name == "Piano or acoustic guitar")
                {
                    preset.settings.type = Type::hall;
                    preset.typeNote = "Hall: an acoustic instrument sounds natural in the kind of space it's played in, with early reflections that place it there.";
                }
                if (preset.name == "Strings or choir")
                {
                    preset.settings.type = Type::hall;
                    preset.typeNote = "Hall: the space orchestras and choirs play in: a long, smooth swell that ties the voices together.";
                }
                if (preset.name == "Ambient wash")
                {
                    preset.settings.type = Type::digital;
                    preset.typeNote = "Digital: an 8-second tail with almost no walls is no real place, so the general setting fits.";
                }
                if (preset.name == "Just a touch")
                {
                    preset.settings.type = Type::room;
                    preset.typeNote = "Room: a tiny room isn't heard as reverb, only as the sound being somewhere.";
                }
                if (preset.name == "Bass")
                {
                    preset.settings.type = Type::room;
                    preset.typeNote = "Room: if a bass gets any reverb at all, a very short, narrow room; a long tail in the lows turns to mud.";
                }
                if (preset.name == "Return: plate")
                {
                    preset.settings.type = Type::plate;
                    preset.typeNote = "Plate: one bright, dense plate suits most of what is sent to it: vocals, snare, keys.";
                }
                if (preset.name == "Return: short room")
                {
                    preset.settings.type = Type::room;
                    preset.typeNote = "Room: a small shared room puts dry tracks in one space.";
                }
                if (preset.name == "Return: hall")
                {
                    preset.settings.type = Type::hall;
                    preset.typeNote = "Hall: one big shared space for slow, sustained parts.";
                }
                if (preset.name == "Lead vocal")
                {
                    preset.settings.earlyPercent = 0.0f;
                    preset.notes[(size_t) Knob::early] = "0%: a plate has no walls, so no early reflections; this knob does nothing on a plate.";
                    preset.notes[(size_t) Knob::size] = "55%: on a plate, how big the sheet is: big enough for a smooth tail.";
                }
            }
            add ("Echo chamber", "The sound of 1950s and 60s studios: a hard room that a loudspeaker plays into and a microphone listens to. A dense, even tail that still sounds like a real place, for vocals, strings and horns.", { 15, 15, 1.8f, 8000, 200, 60, 100, 20, Type::chamber },
                 { "15 ms: a short gap, so the start of each note stays clear. Some studios fed their chambers through a tape delay for a longer one.",
                   "15%: a small room, about 7 m across.",
                   "1.8 s: hard walls ring long for a room this small. Capitol's chambers can ring up to 5 s, but most songs want less.",
                   "8 kHz: hard walls keep the highs; the speaker and microphone soften the very top.",
                   "200 Hz: keeps a small room's boom out of the mix.",
                   "60%: the chamber's first reflections, already blended by the hard walls.",
                   "100%: the reverb all around the sound.",
                   "20%." });
            list.back().typeNote = "Chamber: a hard, bare room heard through a microphone: dense soon after it starts, and longer-ringing than its size suggests.";
            add ("Spring guitar", "The reverb of a guitar amplifier: a spring tank's repeats and chirps on a clean electric guitar, as in surf music. Also good on organ, and as an effect on drums.", { 0, 50, 2.5f, 6000, 150, 0, 60, 30, Type::spring },
                 { "0 ms: a spring brings its own gap: the first repeat comes after one trip along it.",
                   "50%: a long spring, repeating every 55 ms.",
                   "2.5 s: a long tank; real ones are fixed somewhere between about 1.75 and 4 s.",
                   "6 kHz: a spring carries little above a few kHz anyway; this softens the top of the drip.",
                   "150 Hz: keeps the low strings from booming in the tank.",
                   "0%: a spring has no walls; this knob does nothing here.",
                   "60%: a tank is mono; two springs of different lengths, one per side, give a little width.",
                   "30%: surf guitar is soaked in reverb; on a rhythm part, 15-20%." });
            list.back().typeNote = "Spring: the reverb built into guitar amps since the early 1960s.";

            return list;
        }();

        return presets;
    }

    //==============================================================================
    void Network::prepare (double sampleRate)
    {
        rate = sampleRate > 0.0 ? sampleRate : 44100.0;
        auto samples = [this] (double ms) { return (int) std::ceil (ms * rate / 1000.0) + 2; };

        predelay.setMaximum (samples (maxPredelay + getFirstReflectionMs (100.0f) * 8.0));

        for (int d = 0; d < numDiffusers; ++d)
        {
            diffuserSamples[(size_t) d] = juce::jmax (1, (int) std::round (diffuserMs[d] * rate / 1000.0));
            diffusers[(size_t) d].setMaximum (samples (diffuserMs[d] * 2.0));
        }

        for (auto& side : chamberDiffusers)
            for (int d = 0; d < numDiffusers; ++d)
                side[(size_t) d].setMaximum (samples (diffuserMs[d] * 2.0));
        for (int i = 0; i < numLines; ++i)
            lines[(size_t) i].setMaximum (samples (lineMs[i]));

        for (int side = 0; side < 2; ++side)
        {
            springLoops[(size_t) side].setMaximum (samples (100.0));
            springPickups[(size_t) side].setMaximum (samples (50.0));
            for (auto& stage : springDispersion[(size_t) side]) stage.setMaximum (3);
        }
        typeFade.reset (rate, 0.02);
        typeFade.setCurrentAndTargetValue (1.0f);
        setSettings (settings);
        reset();
    }

    void Network::reset()
    {
        std::fill (predelay.data.begin(), predelay.data.end(), 0.0f);

        for (auto& d : diffusers) std::fill (d.data.begin(), d.data.end(), 0.0f);
        for (auto& d : lines) std::fill (d.data.begin(), d.data.end(), 0.0f);
        for (auto& side : chamberDiffusers)
            for (auto& stage : side) std::fill (stage.data.begin(), stage.data.end(), 0.0f);

        for (auto& delay : springLoops) std::fill (delay.data.begin(), delay.data.end(), 0.0f);
        for (auto& delay : springPickups) std::fill (delay.data.begin(), delay.data.end(), 0.0f);
        for (auto& side : springDispersion)
            for (auto& stage : side) std::fill (stage.data.begin(), stage.data.end(), 0.0f);
        springState.fill (0.0f);
        for (auto& memory : springOutputMemory) memory.fill (0.0f);
        typeFade.setCurrentAndTargetValue (1.0f);
        lineState.fill (0.0f);
        earlyStateL = earlyStateR = 0.0f;
        std::fill_n (&lowCutMemory[0][0][0], 8, 0.0);
    }

    void Network::setSettings (const Settings& s)
    {
        const bool changedType = settings.type != s.type;
        settings = s;

        if (lines[0].data.size() <= 1)
            return;   // prepare() applies them
        if (changedType)
        {
            reset();
            typeFade.setCurrentAndTargetValue (0.0f);
            typeFade.setTargetValue (1.0f);
        }
        diffusionAmount = s.type == Type::plate ? 0.78f : s.type == Type::chamber ? 0.72f : s.type == Type::room ? 0.5f : diffusion;
        for (int d = 0; d < numDiffusers; ++d)
        {
            const double stretch = s.type == Type::plate ? 0.11 : s.type == Type::room ? 0.25 : s.type == Type::chamber ? 0.42 : 1.0;
            diffuserSamples[(size_t) d] = juce::jmax (1, (int) std::round (diffuserMs[d] * stretch * rate / 1000.0));
        }
        for (int side = 0; side < 2; ++side)
        {
            const double trip = (30.0 + 0.5 * s.sizePercent) * (side == 0 ? 1.0 : 1.12) / 1000.0;
            springSamples[(size_t) side] = juce::jmax (2, (int) std::round (trip * rate) - 50);
            springArrival[(size_t) side] = (int) std::round (trip * rate * 0.5);
            springGain[(size_t) side] = (float) getLoopGain (trip, s.decaySeconds);
            springPole[(size_t) side] = isDampingOn (s.dampingHz) ? (float) getDampingPole (springGain[(size_t) side], s.dampingHz, rate) : 0.0f;
        }
        springOutputPole = (float) std::exp (-juce::MathConstants<double>::twoPi * 4500.0 / rate);
        predelaySamples = juce::jlimit (0, (int) predelay.data.size() - 1, (int) std::round (s.predelayMs * rate / 1000.0));

        // The tail: longer lines in a bigger room. Each keeps what it must to die away in the decay time.
        const double scale = getLineScale (s.sizePercent);
        double meanSquare = 0.0;

        for (int i = 0; i < numLines; ++i)
        {
            const int length = juce::jlimit (2, (int) lines[(size_t) i].data.size(), (int) std::round (lineMs[i] * scale * rate / 1000.0));
            const double gain = getLoopGain (length / rate, s.decaySeconds);
            lineSamples[(size_t) i] = length;
            lineGain[(size_t) i] = (float) gain;
            linePole[(size_t) i] = isDampingOn (s.dampingHz) ? (float) getDampingPole (gain, s.dampingHz, rate) : 0.0f;
            meanSquare += gain * gain / numLines;
        }

        // A long tail builds up more: feed it less, so every decay ends up about as loud as what goes in.
        inputGain = (float) std::sqrt (juce::jmax (0.0, 1.0 - meanSquare));

        // The early reflections: spaced by the room, quieter the later they come.
        const double first = getFirstReflectionMs (s.sizePercent);
        float sumSquares = 0.0f;
        std::array<float, numTaps> levels {};

        for (int k = 0; k < numTaps; ++k)
        {
            tapSamples[(size_t) k] = juce::jmax (1, (int) std::round (first * (s.type == Type::hall ? 1.0 + 0.6 * k : s.type == Type::chamber ? 1.0 + 0.13 * k : s.type == Type::room ? 1.0 + 0.16 * k : tapTimes[k]) * rate / 1000.0));
            levels[(size_t) k] = std::pow (0.85f, (float) k) * (k % 3 == 2 ? -1.0f : 1.0f);
            sumSquares += levels[(size_t) k] * levels[(size_t) k];
        }

        const float norm = 1.0f / std::sqrt (sumSquares);

        for (int k = 0; k < numTaps; ++k)
        {
            tapLeft[(size_t) k] = levels[(size_t) k] * norm * (0.5f + tapPans[k]) * 1.4f;
            tapRight[(size_t) k] = levels[(size_t) k] * norm * (0.5f - tapPans[k]) * 1.4f;
        }

        earlyGain = s.type == Type::plate || s.type == Type::spring ? 0.0f : juce::jlimit (0.0f, 1.0f, s.earlyPercent / 100.0f);
        // Balance the mechanisms on steady noise, without changing the original Digital path.
        typeGain = s.type == Type::plate ? 1.1f : s.type == Type::hall ? 1.5f : s.type == Type::chamber ? 1.4f : 1.0f;
        earlyPole = isDampingOn (s.dampingHz) && s.dampingHz < 0.49 * rate
                        ? (float) std::exp (-juce::MathConstants<double>::twoPi * s.dampingHz / rate) : 0.0f;

        const bool cut = isLowCutOn (s.lowCutHz);

        if (cut)
        {
            if (! lowCutOn)
                std::fill_n (&lowCutMemory[0][0][0], 8, 0.0);

            lowCut = eq::makeStages ({ true, eq::BandType::lowCut, s.lowCutHz, 0.0f, eq::flatQ, 12 }, rate);
        }

        lowCutOn = cut;
    }

    void Network::process (const float* inL, const float* inR, float* outL, float* outR, int numSamples)
    {
        const float width = juce::jlimit (0.0f, 1.0f, settings.widthPercent / 100.0f);

        for (int n = 0; n < numSamples; ++n)
        {
            const float x = inR != nullptr ? 0.5f * (inL[n] + inR[n]) : inL[n];
            predelay.push (x);

            // Early reflections: single echoes off the nearest walls.
            float earlyL = 0.0f, earlyR = 0.0f;

            if ((withEarly || settings.type == Type::hall || settings.type == Type::chamber) && earlyGain > 0.0f)
            {
                for (int k = 0; k < numTaps; ++k)
                {
                    const float v = predelay.read (predelaySamples + tapSamples[(size_t) k]);
                    earlyL += v * tapLeft[(size_t) k];
                    earlyR += v * tapRight[(size_t) k];
                }

                if (settings.type == Type::chamber)
                    for (int d = 0; d < numDiffusers; ++d)
                        for (int side = 0; side < 2; ++side)
                        {
                            float& value = side == 0 ? earlyL : earlyR;
                            auto& stage = chamberDiffusers[(size_t) side][(size_t) d];
                            const float before = stage.read (diffuserSamples[(size_t) d] - 1);
                            const float w = value + diffusionAmount * before;
                            stage.push (w);
                            value = before - diffusionAmount * w;
                        }
                earlyStateL = earlyL + earlyPole * (earlyStateL - earlyL);
                earlyStateR = earlyR + earlyPole * (earlyStateR - earlyR);
                earlyL = earlyStateL * earlyGain;
                earlyR = earlyStateR * earlyGain;
            }

            // The tail: smeared by the all-passes, then round and round the lines.
            float tailL = 0.0f, tailR = 0.0f;

            if (withTail && settings.type != Type::spring)
            {
                float t = predelay.read (predelaySamples);
                if (settings.type == Type::hall || settings.type == Type::chamber)
                {
                    // Feed the tail independently of the Early output switch and knob.
                    t = 0.0f;
                    for (int k = 0; k < numTaps; ++k)
                        t += predelay.read (predelaySamples + tapSamples[(size_t) k]) * (tapLeft[(size_t) k] + tapRight[(size_t) k]) * 0.5f;
                }

                for (int d = 0; d < numDiffusers; ++d)
                {
                    auto& line = diffusers[(size_t) d];
                    const float before = line.read (diffuserSamples[(size_t) d] - 1);
                    const float w = t + diffusionAmount * before;
                    line.push (w);
                    t = before - diffusionAmount * w;
                }

                if (settings.type == Type::plate)
                {
                    // A dense input diffuser is audible immediately, without an artificial wall echo.
                    tailL = t * inputGain * 0.65f;
                    tailR = t * inputGain * 0.65f;
                }
                float v[numLines];

                for (int i = 0; i < numLines; ++i)
                {
                    const float o = lines[(size_t) i].read (lineSamples[(size_t) i] - 1);
                    const float p = linePole[(size_t) i];
                    lineState[(size_t) i] = lineGain[(size_t) i] * (1.0f - p) * o + p * lineState[(size_t) i];
                    v[i] = lineState[(size_t) i];
                    tailL += v[i] * leftSigns[i];
                    tailR += v[i] * rightSigns[i];
                }

                mixLines (v);
                const float in = inputGain * t * 0.35355339f;

                for (int i = 0; i < numLines; ++i)
                    lines[(size_t) i].push (v[i] + in * inputSigns[i]);

                tailL *= outputGain;
                tailR *= outputGain;
            }

            if (settings.type == Type::spring)
            {
                tailL = tailR = 0.0f;
                for (int side = 0; side < 2; ++side)
                {
                    const auto index = (size_t) side;
                    auto& loop = springLoops[index];
                    float v = loop.read (springSamples[index] - 1);
                    for (auto& stage : springDispersion[index])
                    {
                        const float before = stage.read (1);
                        const float w = v - 0.6f * before;
                        stage.push (w);
                        v = before + 0.6f * w;
                    }
                    const float pole = springPole[index];
                    springState[index] = (1.0f - pole) * v + pole * springState[index];
                    const float gain = springGain[index];
                    const float drive = predelay.read (predelaySamples);
                    const float travelling = drive * std::sqrt (1.0f - gain * gain) + springState[index] * gain;
                    loop.push (travelling);
                    springPickups[index].push (travelling);
                    float pickup = springPickups[index].read (springArrival[index]);
                    for (auto& memory : springOutputMemory[index])
                    {
                        memory = (1.0f - springOutputPole) * pickup + springOutputPole * memory;
                        pickup = memory;
                    }
                    if (withTail)
                    {
                        if (side == 0) tailL = pickup * 4.0f;
                        else tailR = pickup * 4.0f;
                    }
                }
            }
            if (! withEarly) earlyL = earlyR = 0.0f;
            const float fade = typeFade.getNextValue();
            float l = (earlyL + tailL) * typeGain * fade, r = (earlyR + tailR) * typeGain * fade;

            if (lowCutOn)
            {
                l = (float) runStages (lowCut, lowCutMemory[0], l);
                r = (float) runStages (lowCut, lowCutMemory[1], r);
            }

            const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * width;
            outL[n] = mid + side;
            outR[n] = mid - side;
        }
    }

    juce::AudioBuffer<float> renderImpulse (const Settings& s, double sampleRate, double seconds, bool early, bool tail)
    {
        Network network;
        network.withEarly = early;
        network.withTail = tail;
        network.setSettings (s);
        network.prepare (sampleRate);

        const int total = juce::jmax (1, (int) (seconds * sampleRate));
        juce::AudioBuffer<float> out (2, total);
        std::vector<float> in (512, 0.0f);

        for (int at = 0; at < total; at += 512)
        {
            const int n = juce::jmin (512, total - at);
            in[0] = at == 0 ? 1.0f : 0.0f;
            network.process (in.data(), nullptr, out.getWritePointer (0, at), out.getWritePointer (1, at), n);
        }

        return out;
    }

    std::vector<float> getEnvelopeDb (const juce::AudioBuffer<float>& response, double sampleRate, double windowMs)
    {
        const int window = juce::jmax (1, (int) (windowMs * sampleRate / 1000.0));
        std::vector<float> levels;

        for (int start = 0; start + window <= response.getNumSamples(); start += window)
        {
            double sum = 0.0;

            for (int ch = 0; ch < response.getNumChannels(); ++ch)
                for (int i = 0; i < window; ++i)
                    sum += (double) response.getSample (ch, start + i) * response.getSample (ch, start + i);

            levels.push_back ((float) (10.0 * std::log10 (juce::jmax (1.0e-12, sum / (window * response.getNumChannels())))));
        }

        return levels;
    }

    double measureDecay (const juce::AudioBuffer<float>& response, double sampleRate)
    {
        const int total = response.getNumSamples();
        std::vector<double> remaining ((size_t) total + 1, 0.0);

        for (int i = total; --i >= 0;)
        {
            double e = 0.0;

            for (int ch = 0; ch < response.getNumChannels(); ++ch)
                e += (double) response.getSample (ch, i) * response.getSample (ch, i);

            remaining[(size_t) i] = remaining[(size_t) i + 1] + e;
        }

        if (remaining[0] <= 0.0)
            return 0.0;

        int at5 = -1, at25 = -1;

        for (int i = 0; i < total; ++i)
        {
            const double db = 10.0 * std::log10 (juce::jmax (1.0e-30, remaining[(size_t) i] / remaining[0]));

            if (at5 < 0 && db <= -5.0) at5 = i;
            if (at25 < 0 && db <= -25.0) { at25 = i; break; }
        }

        return at5 >= 0 && at25 > at5 ? 3.0 * (at25 - at5) / sampleRate : 0.0;
    }
} // namespace verb

//==============================================================================
namespace
{
    using verb::Knob;

    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    /** How a parameter reads in the editor and to hosts: the same as formatKnob. */
    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float v, int)
            {
                verb::Settings s;
                s.predelayMs = s.sizePercent = s.decaySeconds = s.dampingHz = s.lowCutHz = s.earlyPercent = s.widthPercent = s.mixPercent = v;
                return verb::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return k == Knob::damping ? verb::maxDamping : verb::minLowCut;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();

                if (text.containsIgnoreCase ("khz"))
                    return v * 1000.0f;

                return k == Knob::decay && text.containsIgnoreCase ("ms") ? v / 1000.0f : v;
            });
    }
}

const char* BuiltInReverb::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::predelay: return "predelay";
        case Knob::size:     return "size";
        case Knob::decay:    return "decay";
        case Knob::damping:  return "damping";
        case Knob::lowCut:   return "lowcut";
        case Knob::early:    return "early";
        case Knob::width:    return "width";
        case Knob::mix:      return "mix";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInReverb::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const verb::Settings d;
    auto add = [&layout] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    add (Knob::predelay, "Pre-delay", { 0.0f, verb::maxPredelay, 0.1f, 0.5f }, d.predelayMs);
    add (Knob::size, "Size", { 0.0f, 100.0f, 1.0f }, d.sizePercent);
    add (Knob::decay, "Decay", logRange (verb::minDecay, verb::maxDecay), d.decaySeconds);
    add (Knob::damping, "Damping", logRange (verb::minDamping, verb::maxDamping), d.dampingHz);
    add (Knob::lowCut, "Low cut", logRange (verb::minLowCut, verb::maxLowCut), d.lowCutHz);
    add (Knob::early, "Early reflections", { 0.0f, 100.0f, 1.0f }, d.earlyPercent);
    add (Knob::width, "Width", { 0.0f, 100.0f, 1.0f }, d.widthPercent);
    add (Knob::mix, "Mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "type", 1 }, "Type",
                 juce::StringArray { "Digital", "Room", "Hall", "Chamber", "Plate", "Spring" }, 0));
    return layout;
}

BuiltInReverb::BuiltInReverb()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItReverb", createLayout())
{
    for (int k = 0; k < verb::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));
    typeValue = parameters.getRawParameterValue ("type");
}

juce::PluginDescription BuiltInReverb::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Reverb";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44527631;   // "DRv1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInReverb::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInReverb::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

verb::Settings BuiltInReverb::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    verb::Settings s;
    s.predelayMs = value (Knob::predelay);
    s.sizePercent = value (Knob::size);
    s.decaySeconds = value (Knob::decay);
    s.dampingHz = value (Knob::damping);
    s.lowCutHz = value (Knob::lowCut);
    s.earlyPercent = value (Knob::early);
    s.widthPercent = value (Knob::width);
    s.mixPercent = value (Knob::mix);
    s.type = (verb::Type) juce::jlimit (0, verb::numTypes - 1, (int) typeValue->load());
    return s;
}

juce::RangedAudioParameter* BuiltInReverb::getTypeParameter() const
{
    return parameters.getParameter ("type");
}

void BuiltInReverb::setType (verb::Type kind)
{
    auto* parameter = getTypeParameter();
    parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) kind));
}

void BuiltInReverb::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInReverb::setSettings (const verb::Settings& s)
{
    setType (s.type);
    setKnob (Knob::predelay, s.predelayMs);
    setKnob (Knob::size, s.sizePercent);
    setKnob (Knob::decay, s.decaySeconds);
    setKnob (Knob::damping, s.dampingHz);
    setKnob (Knob::lowCut, s.lowCutHz);
    setKnob (Knob::early, s.earlyPercent);
    setKnob (Knob::width, s.widthPercent);
    setKnob (Knob::mix, s.mixPercent);
}

double BuiltInReverb::getTailLengthSeconds() const
{
    const auto s = getSettings();
    return s.predelayMs / 1000.0 + 1.2 * s.decaySeconds + 0.1;
}

//==============================================================================
bool BuiltInReverb::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInReverb::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    applied = getSettings();
    appliedOnce = true;
    network.setSettings (applied);
    network.prepare (rate);
    wet.setSize (2, juce::jmax (64, maximumBlockSize));
    clap.prepare (rate);
    for (int kind = 0; kind < verb::numTypes; ++kind)
        auditionSettings[(size_t) kind] = verb::kinds::getShowcase ((verb::Type) kind).settings;
    auditionNetwork.prepare (rate);
    auditionClap.prepare (rate);
    auditionInput.setSize (2, wet.getNumSamples());
    auditionWet.setSize (2, wet.getNumSamples());
    auditionRemaining = 0;
    auditionActive = false;

    const bool only = listenToEffect.load();
    const auto [dry, wetLevel] = fx::getMixGains (applied.mixPercent);
    dryGain.reset (rate, 0.05);
    dryGain.setCurrentAndTargetValue (only ? 0.0f : dry);
    wetGain.reset (rate, 0.05);
    wetGain.setCurrentAndTargetValue (only ? 1.0f : wetLevel);
}

void BuiltInReverb::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (numChannels == 0)
        return;

    if (auto* playHead = getPlayHead())
        if (const auto position = playHead->getPosition())
            if (const auto bpm = position->getBpm())
                tempoBpm = *bpm;

    clap.addTo (buffer, numChannels);

    const auto s = getSettings();

    if (! appliedOnce || s != applied)
    {
        network.setSettings (s);
        applied = s;
        appliedOnce = true;
    }

    const int requested = auditionRequest.exchange (-1);
    if (requested >= 0 && requested < verb::numTypes)
    {
        const auto& showcase = auditionSettings[(size_t) requested];
        auditionNetwork.setSettings (showcase);
        auditionNetwork.reset();
        auditionClap.request();
        auditionRemaining = (int) ((showcase.decaySeconds * 1.5 + showcase.predelayMs / 1000.0 + 0.25) * rate);
        auditionActive = true;
    }
    const bool only = listenToEffect.load();
    const auto [dry, wetLevel] = fx::getMixGains (s.mixPercent);
    dryGain.setTargetValue (only ? 0.0f : dry);
    wetGain.setTargetValue (only ? 1.0f : wetLevel);
    constexpr int chunk = fx::LevelHistory::chunk;

    for (int start = 0; start < total;)
    {
        const int n = juce::jmin (total - start, wet.getNumSamples());
        float* l = buffer.getWritePointer (0, start);
        float* r = numChannels > 1 ? buffer.getWritePointer (1, start) : nullptr;
        float* wetL = wet.getWritePointer (0);
        float* wetR = wet.getWritePointer (1);
        network.process (l, r, wetL, wetR, n);

        for (int c = 0; c < n; c += chunk)
        {
            const int m = juce::jmin (chunk, n - c);
            float inPeak = 0.0f, wetPeak = 0.0f;

            for (int i = c; i < c + m; ++i)
            {
                const float dg = dryGain.getNextValue(), wg = wetGain.getNextValue();
                const float wl = wetL[i] * wg, wr = wetR[i] * wg;
                inPeak = juce::jmax (inPeak, std::abs (l[i]), r != nullptr ? std::abs (r[i]) : 0.0f);
                wetPeak = juce::jmax (wetPeak, std::abs (wl), std::abs (wr));

                if (r != nullptr)
                {
                    l[i] = l[i] * dg + wl;
                    r[i] = r[i] * dg + wr;
                }
                else
                {
                    l[i] = l[i] * dg + 0.5f * (wl + wr);
                }
            }

            history.push ({ juce::Decibels::gainToDecibels (inPeak, -100.0f), juce::Decibels::gainToDecibels (wetPeak, -100.0f) });
        }

        if (auditionRemaining > 0)
        {
            auditionInput.setSize (2, n, false, false, true);
            auditionInput.clear();
            auditionClap.addTo (auditionInput, 2);
            auditionNetwork.process (auditionInput.getReadPointer (0), auditionInput.getReadPointer (1),
                                     auditionWet.getWritePointer (0), auditionWet.getWritePointer (1), n);
            const auto gains = fx::getMixGains (50.0f);
            for (int i = 0; i < n; ++i)
            {
                const float al = auditionInput.getSample (0, i) * gains.first + auditionWet.getSample (0, i) * gains.second;
                const float ar = auditionInput.getSample (1, i) * gains.first + auditionWet.getSample (1, i) * gains.second;
                l[i] += r != nullptr ? al : (al + ar) * 0.5f;
                if (r != nullptr) r[i] += ar;
            }
            auditionRemaining -= n;
            auditionActive = auditionRemaining > 0;
        }
        start += n;
    }

    if (numChannels == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, total);
}

juce::AudioProcessorEditor* BuiltInReverb::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInReverb::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInReverb::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        bool hasType = false;
        for (const auto& child : state)
            hasType = hasType || child.getProperty ("id").toString() == "type";
        if (! hasType)
        {
            juce::ValueTree choice ("PARAM");
            choice.setProperty ("id", "type", nullptr);
            choice.setProperty ("value", 0.0f, nullptr);
            state.addChild (choice, -1, nullptr);
        }
        parameters.replaceState (state);
    }
}
} // namespace daw
