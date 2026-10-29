#include "ReverbKinds.h"
#include "TrackNameWords.h"

#include <cmath>

namespace daw::verb::kinds
{
    static const char* kindsText = R"TEXT(Every reverb copies one of a few real things that ring: a room, a concert hall, an echo chamber, a steel plate or a spring. Studios used the real things for decades. Digital reverbs copy them, or make spaces no building could have.

They differ in more than their length. Listen to how each one starts:

- a room: the first echoes come almost at once, close together, and the tail is short;
- a hall: a short gap, then a few strong echoes from far walls, and a long tail that swells;
- a chamber: a hard room heard through a loudspeaker and a microphone, dense and even soon after it starts;
- a plate: no walls, so no early echoes, only a dense, bright tail from the very start;
- a spring: the sound comes back in repeats, a few hundredths of a second apart, with a metallic boing;
- Digital: DawIt Reverb's own setting, which you can shape into any of them, or into none.

The picture puts one clap through each, with the starting point DawIt Reverb has for that kind (named in each cell). The top of a cell is the level over time, as on Direct, early, tail. The strip under it zooms into the first 100 milliseconds, one line per reflection: sparse and spiky in the room and the hall, a dense haze at once in the plate, evenly spaced bursts in the spring.

Click Hear each in turn to hear them one after the other, 3 seconds apart. In DawIt Reverb, Type picks the kind; the knobs keep their settings when you switch. The next pages take the kinds one at a time.)TEXT";
    static const char* roomText = R"TEXT(A room is any small space: a living room, a studio's live room, a garage. Its walls are a few metres away, so the first reflections arrive a few milliseconds after the direct sound, strong and close together. You don't hear them as echoes. They make the sound thicker, and tell your ear that it was played somewhere real.

The tail is short: 0.3 to 1 second in most rooms. Because the walls are close, the reflections multiply fast. In a living room of 60 cubic metres they pass 1,000 a second after only 11 milliseconds, and from about there they blur into a smooth tail. In a concert hall that takes about 170 milliseconds.

What it's for: making a sound that was recorded close, or made by a synth, sound as if it was played in a space. Drums above all: studios record a kit with a microphone close to each drum and a pair further away for the room, and mix in as much room as the song wants. Also guitar amps, percussion, and keys recorded direct.

Typical settings: decay 0.3-1 s, pre-delay 0-10 ms, Early 60-90%, mix 10-25%. Too much room on a busy part sounds boxy rather than big.

The picture shows the room from above, slowed down 20 times, with the first reflections as rays. Under it is what one clap becomes, with the first 100 ms zoomed in. Click Clap: room to hear the Small room starting point.)TEXT";
    static const char* hallText = R"TEXT(A concert hall's walls are far away. After the direct sound there is a gap before the first reflection arrives: at a seat in the middle of the floor about 15 to 35 milliseconds. The acoustician Leo Beranek, who measured dozens of halls, found that the best-liked ones had short gaps, about 15 to 25 ms. Boston's Symphony Hall has about 15 ms.

Then the reflections come in: few and strong at first, then more and more. In a hall of 15,000 cubic metres they reach 1,000 a second only after about 170 milliseconds, so the tail swells over the first tenth of a second or two, then dies away slowly and smoothly. Halls built for orchestras ring for about 1.7 to 2.1 seconds when they are full of people; Boston's for 1.9 s. An empty hall rings longer, because the audience soaks up sound.

Why it suits slow music: a long, smooth tail joins the notes into one sound and makes strings, choirs and piano sound full. It blurs fast parts, so drums and quick riffs usually get something shorter.

Typical settings: decay 1.8-3 s, pre-delay 20-40 ms, Size 70-95%, Early 30-50%, damping 4-6 kHz, because air and seats soften a hall's highs. Strings, choirs, piano ballads and pads, and vocals in slow songs.

The picture shows a hall from above, slowed down 20 times: the stage, a listener in the middle and the first reflections off the side walls, each with its time. Click Clap: hall to hear the Strings or choir starting point.)TEXT";
    static const char* chamberText = R"TEXT(Before there were reverb units, a studio that wanted reverb built a room for it: an echo chamber. A loudspeaker plays the recording into a bare room with hard walls, and a microphone, usually at the far end, picks up how the room answers. That is mixed back in with the dry sound.

An early famous one was a washroom. In 1947 Bill Putnam put a speaker and a microphone in the washroom of his Chicago studio for the Harmonicats' Peg o' My Heart, which went to number one. Abbey Road in London had a chamber from the 1930s. In 1956 Capitol Records built four under the car park of its Hollywood tower, about 9 metres down, and four more later. They are still used.

Capitol's chambers are concrete, coated with lacquer, with no two walls parallel, so no echo can bounce back and forth between two of them. Each is about 57 cubic metres and rings for up to 5 seconds. Sabine's rule shows how hard those walls are: 0.161 x 57 / 5 = 1.8 square metres of absorption for the whole room, about as much as one small open window.

How it sounds: like a room, because it is one, but a long-ringing one with no clear direct sound. The microphone hears the reflections already mixed together, so the tail is dense and even soon after it starts. The loudspeaker and the microphone colour it a little, and that is part of its sound.

What it's for: vocals, strings and horns with the sound of 1950s and 60s records, and anything that wants a smooth tail that still sounds like a real place.

Typical settings: decay 1.2-2.5 s, pre-delay 0-30 ms, Size 10-30%, mix 15-25%. Click Clap: chamber to hear the Echo chamber starting point.)TEXT";
    static const char* plateText = R"TEXT(In 1957 the German company EMT brought out the EMT 140, a reverb with no room at all. It is a thin steel sheet, about 1 by 2 metres and half a millimetre thick, hung by its corners on springs inside a large case. A driver, like the coil of a loudspeaker, shakes the plate with the sound, and pickups on the plate hear how it rings. The stereo 140S of 1961 has two pickups in different places, which give two slightly different signals.

A pad of fibreglass hangs next to the plate without touching it. Move it closer and it soaks up more of the plate's motion, so the reverb dies away sooner: the decay can be set from under a second to about 5 seconds.

Why it sounds the way it does. A plate has no walls for the sound to come back from. Its waves reach the edges within milliseconds and keep crossing, so there are no separate early reflections, only a dense tail from the first millisecond. And waves in a plate are dispersive: high frequencies travel faster than low ones. In a 0.5 mm steel plate a 10 kHz ripple crosses 2 metres in about 5 milliseconds, a 1 kHz ripple in 14 and a 100 Hz ripple in 46. A clap is smeared into a bright, splashy wash rather than a row of echoes. A plate's resonances are also spread evenly over all frequencies, where a room has more and more of them higher up: part of why a plate sounds smooth and a little metallic.

What it's for: the vocal and snare reverb of the 1960s to 80s, and still a favourite. It is bright and dense, and without early reflections the voice doesn't sound recorded in a room, so the words stay in front. Also on horns, keys and drums.

Typical settings: decay 1.2-2.5 s, pre-delay 0-40 ms (more on vocals, for clarity), damping 8 kHz or higher, low cut 200-300 Hz. Early does nothing on a plate.

The picture shows the plate in its frame with the driver, the pickups and the damper. After a clap, three ripples spread from the driver, slowed down 100 times: 10 kHz in front, then 1 kHz, then 100 Hz. Click Clap: plate to hear the Snare plate starting point.)TEXT";
    static const char* springText = R"TEXT(A spring reverb sends the sound down a coiled metal spring. A small driver at one end twists it, and a pickup at the other end turns the motion back into sound. Laurens Hammond patented it in 1941 to give his electric organs the ring of a church without a church. It was small and cheap, so it went into guitar amplifiers: Fender sold a stand-alone Reverb unit from 1961, and reverb was built into amps in the early 1960s. The tank inside an amp is a metal tray, usually with 2 or 3 springs side by side.

Why it sounds the way it does. A wave runs along the spring to the pickup. Part of it bounces back to the driver's end and returns, again and again, so the tail comes in repeats, evenly spaced by the time of one trip there and back: about 40 to 80 ms in typical tanks. A spring also carries its frequencies at different speeds, so each repeat is smeared into a quick chirp: the boing and the drip you hear on a hard pluck. It carries little above a few kilohertz (its limit is usually between 2 and 5 kHz), so its tail is darker than a plate's. Tanks have springs of different lengths so their repeats blur into each other.

Knock a guitar amp and you hear the springs crash: the same thing with a very loud input.

What it's for: clean electric guitar (the surf guitar of the early 1960s is soaked in it), dub reggae, where the reverb is part of the arrangement, organ, and vintage or lo-fi sounds; and as an effect on drums and vocals. Its repeats and chirps make it the least natural reverb, which is exactly why people choose it.

Typical settings: a real tank's decay is fixed, usually between 1.75 and 4 s. The mix is often higher than other reverbs get: 20-40% on a surf guitar. In DawIt Reverb, Size sets the time between repeats, 30 ms at 0% to 80 ms at 100%, and Early does nothing.

The picture shows the tank and its springs, with a pulse running from the driver to the pickup and back, wider and fainter on each trip, slowed down 20 times. Click Clap: spring to hear the Spring guitar starting point.)TEXT";
    static const char* digitalText = R"TEXT(Digital reverbs do with numbers what rooms do with walls. The first, the EMT 250, came out in 1976. The Lexicon 224, designed by David Griesinger and shown in 1978, became one of the most used studio reverbs ever. Inside, a few delay lines feed back into each other, each pass a little quieter and darker, until the echoes come thousands a second: a tail. Manfred Schroeder worked out in 1962 that a tail needs about 1,000 echoes a second or more to sound smooth rather than fluttery; for sharp clicks, about ten times that.

Because nothing in them is physical, digital reverbs can copy any of the real kinds (that is what DawIt Reverb's Type does) or make spaces no building could have: a 12-second tail, a huge room with no early reflections, a hall that stays bright to the end. Digital is DawIt Reverb's general setting, the one every new DawIt Reverb starts on; its knobs do just what they say, with nothing added.

Convolution reverbs work another way. They play a click or a slow sweep in a real place (a church, a chamber, a real plate) and record how it answers: its impulse response. Each moment of your sound is then replaced by a copy of that answer at its level, and all the copies are added up: that is convolution. The result is the reverb of that place, from that spot, exactly; but you can't move its walls afterwards. DawIt has no convolution reverb. Its plate and spring are models of how those things behave, not recordings of particular units.

The picture is a timeline of artificial reverb, from the first chambers to the digital designs most reverbs build on.)TEXT";
    static const char* effectsText = R"TEXT(Some reverbs aren't meant to sound like any place.

Gated: a big, bright reverb on a drum, cut off sharply a fraction of a second after each hit by a noise gate. The drum sounds huge, but its tail doesn't wash over the next beat: the 1980s drum sound of Phil Collins' In the Air Tonight. In DawIt: put DawIt Gate after DawIt Reverb on the drum and pick the gate's Gated reverb starting point.

Reverse: the tail comes before the note, so a word swells up out of nothing. Engineers turned the tape round, recorded the reverb of the backwards sound, then turned the tape back. In a DAW: reverse the clip, record its reverb only, and reverse that.

Shimmer: a reverb whose tail is shifted up an octave each time it goes round, so it rises into a glowing, organ-like cloud. For ambient music and slow guitar.

Modulated: the delay lines' lengths sway slowly by a few milliseconds, so the tail's pitch wobbles a little, like a chorus. It hides the metallic ringing a long digital tail can have.

DawIt Reverb has none of these built in; gated reverb is made with DawIt Gate, as above.

The picture shows what each does to one clap, as the level over time.)TEXT";
    static const char* yourTrackText = R"TEXT(This page looks at the track DawIt Reverb is on and at how the reverb is set, and says what to try.

It goes by the track's name: words like Vocal, BV, Snare, Kick, Drums, Bass, Guitar, Keys, Piano, Strings, Pad and Choir. From that it says which kinds of reverb are usually chosen for such a sound, and why. Then it compares the settings with the usual ranges: the mix, the low cut, the pre-delay on a vocal, the width on a bass, and the decay against the song's tempo.

These are habits that many engineers share, not rules. If it sounds right in the song, it is right.

On a return track it checks the mix, which should be 100%: the dry sound comes from the tracks that send to it.)TEXT";

    juce::String explain (Topic topic)
    {
        switch (topic)
        {
            case Topic::kinds: return juce::String::fromUTF8 (kindsText);
            case Topic::room: return juce::String::fromUTF8 (roomText);
            case Topic::hall: return juce::String::fromUTF8 (hallText);
            case Topic::chamber: return juce::String::fromUTF8 (chamberText);
            case Topic::plate: return juce::String::fromUTF8 (plateText);
            case Topic::spring: return juce::String::fromUTF8 (springText);
            case Topic::digital: return juce::String::fromUTF8 (digitalText);
            case Topic::effects: return juce::String::fromUTF8 (effectsText);
            case Topic::yourTrack: return juce::String::fromUTF8 (yourTrackText);
        }
        return {};
    }

    juce::String getName (Type type)
    {
        switch (type)
        {
            case Type::digital: return "Digital";
            case Type::room: return "Room";
            case Type::hall: return "Hall";
            case Type::chamber: return "Chamber";
            case Type::plate: return "Plate";
            case Type::spring: return "Spring";
        }
        return {};
    }

    juce::String getSummary (Type type)
    {
        switch (type)
        {
            case Type::digital: return "Delay lines, and recorded rooms";
            case Type::room: return "Close walls, quick echoes, a short tail";
            case Type::hall: return "Far walls, a gap, then a long swell";
            case Type::chamber: return "A hard room with a speaker and a microphone";
            case Type::plate: return "A steel sheet: dense and bright from the start";
            case Type::spring: return "Repeats along a coil: the boing of guitar amps";
        }
        return {};
    }

    juce::String explain (Type type)
    {
        switch (type)
        {
            case Type::digital: return explain (Topic::digital);
            case Type::room: return explain (Topic::room);
            case Type::hall: return explain (Topic::hall);
            case Type::chamber: return explain (Topic::chamber);
            case Type::plate: return explain (Topic::plate);
            case Type::spring: return explain (Topic::spring);
        }
        return {};
    }

    const Preset& getShowcase (Type type)
    {
        juce::String name;
        switch (type)
        {
            case Type::digital: name = "Neutral start"; break;
            case Type::room: name = "Small room"; break;
            case Type::hall: name = "Strings or choir"; break;
            case Type::chamber: name = "Echo chamber"; break;
            case Type::plate: name = "Snare plate"; break;
            case Type::spring: name = "Spring guitar"; break;
        }
        for (const auto& preset : getPresets())
            if (preset.name == name)
                return preset;
        jassertfalse;
        return getPresets().front();
    }
    juce::String getTypeDescription()
    {
        return "What makes the reverb: a room, a hall, a chamber, a plate, a spring, or Digital, DawIt Reverb's own general setting. The kinds differ in how the reverb starts, not only in how long it lasts. A room's first echoes come almost at once and close together. A hall's come later and further apart, and its tail swells. A chamber is a hard room heard through a microphone: dense and even. A plate has no walls, so no early reflections: just a dense, bright tail from the start. A spring repeats, with a boing.\n\nThe knobs mean the same on every type, and keep their settings when you switch. Early does nothing on a plate or a spring, which have no walls. More in What is reverb?, Kinds of reverb.";
    }
    juce::String getTypeTypical()
    {
        return "Vocals: Plate, or Hall in a slow song. Snare: Plate; the whole kit: Room. Guitars: Room, or Spring on a clean electric. Piano, strings, choirs and pads: Hall. For a 1950s or 60s sound: Chamber or Plate. Digital when you want to shape every part yourself, or a space no real one could be. These are habits, not rules: try two and keep what sounds right in the song.";
    }

    juce::String describeSettings (const Settings& settings)
    {
        switch (settings.type)
        {
            case Type::digital: return "Digital: DawIt Reverb's own reverb, shaped only by its knobs.";
            case Type::room: return "Room: close walls, strong early reflections close together, and a tail that starts at once.";
            case Type::hall: return "Hall: a few strong reflections from far walls, then a tail that swells and dies away slowly.";
            case Type::chamber: return "Chamber: a hard room heard through a microphone: the reflections blend into a dense, even tail within a few hundredths of a second.";
            case Type::plate: return "Plate: no walls, so no early reflections: a dense, bright tail from the first millisecond.";
            case Type::spring: return "Spring: the sound comes back every " + juce::String (springRoundTripMs (settings.sizePercent), 0) + " ms, each repeat smeared into a chirp.";
        }
        return {};
    }

    double reflectionRate (double volumeM3, double timeSeconds)
    {
        return 4.0 * juce::MathConstants<double>::pi * std::pow (343.0, 3.0) * timeSeconds * timeSeconds / juce::jmax (0.001, volumeM3);
    }

    double densityTimeMs (double volumeM3, double echoesPerSecond)
    {
        return 1000.0 * std::sqrt (juce::jmax (0.0, echoesPerSecond * volumeM3) / (4.0 * juce::MathConstants<double>::pi * std::pow (343.0, 3.0)));
    }

    double plateGroupVelocity (double frequencyHz)
    {
        const double kappa = std::sqrt (200.0e9 * 0.0005 * 0.0005 / (12.0 * 7850.0 * (1.0 - 0.3 * 0.3)));
        return 2.0 * std::sqrt (kappa * 2.0 * juce::MathConstants<double>::pi * juce::jmax (0.0, frequencyHz));
    }

    double springRoundTripMs (float sizePercent)
    {
        return 30.0 + 0.5 * juce::jlimit (0.0, 100.0, (double) sizePercent);
    }

    Source getSource (const juce::String& name)
    {
        const auto words = trackname::getWords (name);
        const auto has = [&words] (std::initializer_list<const char*> list) { return trackname::nameHasWord (words, list); };
        if (has ({ "vocal", "vox", "voice", "bv", "bvs", "backing", "harmony" })) return Source::vocal;
        if (has ({ "snare" })) return Source::snare;
        if (has ({ "kick" })) return Source::kick;
        if (has ({ "drum", "hat", "hats", "hihat", "tom", "toms", "cymbal", "crash", "ride", "overhead", "oh", "perc", "clap", "shaker", "808", "beat" })) return Source::drums;
        if (has ({ "bass", "sub" })) return Source::bass;
        if (has ({ "guitar", "gtr", "gtrs" })) return Source::guitar;
        if (has ({ "keys", "piano", "organ", "rhodes", "wurli" })) return Source::keys;
        if (has ({ "strings", "violin", "viola", "cello", "pad", "choir", "orchestra" })) return Source::ensemble;
        return Source::unknown;
    }

    juce::StringArray analyse (const juce::String& name, bool isReturn, const Settings& settings, double tempoBpm)
    {
        juce::StringArray advice;
        if (name.isEmpty())
        {
            advice.add ("This reverb isn't on a track; it's one to try things out with. Put DawIt Reverb on a track or a return to get advice for it.");
            return advice;
        }
        const bool inPlugin = name == trackname::pluginTrackName;
        if (inPlugin)
            advice.add ("As a plugin, DawIt Reverb doesn't know the name of the track it's on, so there is no advice for the kind of sound here. In DawIt, "
                        "this page goes by the track's name: Vocal, Snare, Guitar and so on.");
        // A plugin's track is an unknown source: the settings advice below still applies.
        const auto source = inPlugin ? Source::unknown : getSource (name);
        std::vector<Type> usual;
        juce::String sourceName, recommendationName;
        switch (source)
        {
            case Source::unknown: break;
            case Source::vocal: usual = { Type::plate, Type::hall, Type::chamber }; sourceName = "a vocal"; recommendationName = "Lead vocal"; break;
            case Source::snare: usual = { Type::plate, Type::room }; sourceName = "a snare"; recommendationName = "Snare plate"; break;
            case Source::kick: usual = { Type::room }; sourceName = "a kick"; recommendationName = "Bass"; break;
            case Source::drums: usual = { Type::room, Type::chamber }; sourceName = "drums"; recommendationName = "Drum room"; break;
            case Source::bass: usual = { Type::room }; sourceName = "a bass"; recommendationName = "Bass"; break;
            case Source::guitar: usual = { Type::room, Type::spring, Type::plate }; sourceName = "a guitar"; recommendationName = "Small room"; break;
            case Source::keys: usual = { Type::hall, Type::room, Type::plate }; sourceName = "keys"; recommendationName = "Piano or acoustic guitar"; break;
            case Source::ensemble: usual = { Type::hall, Type::digital }; sourceName = "strings, pads or a choir"; recommendationName = "Strings or choir"; break;
        }
        const auto number = [] (float value) { return juce::String (value, 0); };
        if (! usual.empty())
        {
            const Preset* example = &getShowcase (usual.front());
            for (const auto& preset : getPresets())
                if (preset.name == recommendationName && preset.settings.type == usual.front())
                {
                    example = &preset;
                    break;
                }
            const auto reason = example->typeNote.fromFirstOccurrenceOf (": ", false, false);
            if (settings.type == Type::digital && source != Source::ensemble)
                advice.add ("To try: Type " + getName (usual.front()) + ", the usual reverb on " + sourceName + ": " + reason);
            else if (settings.type != Type::digital && std::find (usual.begin(), usual.end(), settings.type) == usual.end())
                advice.add ("Your reverb is a " + getName (settings.type) + ". On " + sourceName + ", a " + getName (usual.front()) + " is the usual choice: " + reason
                            + " A " + getName (settings.type) + " works too when you want " + describeSettings (settings).fromFirstOccurrenceOf (": ", false, false));
        }
        else if (! isReturn && ! inPlugin)
            advice.add ("DawIt can't tell from the name '" + name + "' what this track holds. Name it Vocal, Snare, Guitar and so on for advice on the type.");

        if (! isReturn && settings.mixPercent > (source == Source::ensemble ? 50.0f : 40.0f))
            advice.add ("The mix is " + number (settings.mixPercent) + "%: the reverb is nearly as loud as the sound itself. Most tracks sit at 10-30%; this much makes it sound far away, which can be what you want.");
        if (isReturn && settings.mixPercent < 95.0f)
            advice.add ("On a return, set Mix to 100%: the dry sound comes from the tracks that send to it, and their sends set how much reverb each gets.");
        const bool lowSource = source == Source::bass || source == Source::kick;
        if (lowSource && settings.lowCutHz < 300.0f)
            advice.add ("On " + sourceName + ", keep the lows dry: a low cut of 300-400 Hz, or no reverb at all.");
        else if (settings.lowCutHz < 100.0f && source != Source::ensemble && source != Source::keys)
            advice.add ("The low cut is " + (isLowCutOn (settings.lowCutHz) ? number (settings.lowCutHz) + " Hz" : juce::String ("Off")) + ". Try 150-300 Hz: the reverb's lows mostly turn into mud.");
        if (lowSource && settings.widthPercent > 30.0f)
            advice.add ("Width " + number (settings.widthPercent) + "%: try 0-30% so the low end stays in the middle.");
        if (source == Source::vocal && settings.predelayMs < 20.0f && settings.decaySeconds >= 1.0f)
            advice.add ("Pre-delay " + number (settings.predelayMs) + " ms: try 30-100 ms, so each word is heard dry first and stays in front.");
        if (tempoBpm > 0.0 && source != Source::ensemble)
        {
            const double barSeconds = 240.0 / tempoBpm;
            if (settings.decaySeconds > 2.0 * barSeconds)
                advice.add ("The tail lasts " + juce::String (settings.decaySeconds, 1) + " s, about " + juce::String (settings.decaySeconds / barSeconds, 0) + " bars at "
                            + juce::String (tempoBpm, 0) + " BPM (a bar is " + juce::String (barSeconds, 1) + " s). If the parts blur, try a decay under " + juce::String (2.0 * barSeconds, 1) + " s.");
        }
        if (advice.isEmpty() && source != Source::unknown)
            advice.add ("Nothing to change: a " + getName (settings.type) + " set like this is a usual choice on " + sourceName + ".");
        return advice;
    }
}
