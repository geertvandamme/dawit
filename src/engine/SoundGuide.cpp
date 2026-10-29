#include "SoundGuide.h"
#include "PitchTracker.h"
#include "StereoMeasure.h"
#include <juce_dsp/juce_dsp.h>

namespace daw::soundguide
{
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::air:       return "Sound is moving air";
        case Topic::waveform:  return "A waveform is pressure over time";
        case Topic::amplitude: return "Amplitude is loudness";
        case Topic::frequency: return "Frequency is pitch";
        case Topic::timbre:    return "The shape is the timbre";
        case Topic::numbers:   return "From air to numbers";
        case Topic::phase:     return "Phase";
        case Topic::cancellation: return "Phase cancellation";
        case Topic::mono:      return "Checking in mono";
        case Topic::vocals:    return "Taking the vocals out";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::air:       return "Squeezed and thinned-out air";
        case Topic::waveform:  return "What the line in a clip shows";
        case Topic::amplitude: return "How big the swing is, in dB";
        case Topic::frequency: return "How many cycles a second";
        case Topic::timbre:    return "Harmonics, and how a note starts and ends";
        case Topic::numbers:   return "Sample rate and bit depth";
        case Topic::phase:     return "Waves adding up or cancelling";
        case Topic::cancellation: return "A late copy, polarity, and fixes";
        case Topic::mono:      return "What's left when left and right add up";
        case Topic::vocals:    return "Why left minus right isn't enough";
    }

    return {};
}

juce::String explain (Topic topic)
{
    switch (topic)
    {
        case Topic::air:
            return "Sound is air moving back and forth. A speaker cone, a guitar string or your vocal cords push the air next to "
                   "them together (compression), then pull back and leave it thinned out (rarefaction). That air pushes the air "
                   "next to it, and so a wave of squeezed and thinned-out air travels away, at about 343 metres a second.\n\n"
                   "The air itself hardly goes anywhere: each bit of it only wobbles around where it was, like the dots in the "
                   "picture. What travels is the pattern. When it reaches your ear, it pushes and pulls your eardrum the same way, "
                   "and you hear it.\n\n"
                   "The line under the dots is the pressure: up where the air is squeezed, down where it is thinned out. "
                   "That line is a waveform, the same kind of line you see in an audio clip.\n\n"
                   "The picture is slowed down a lot: at 440 Hz the speaker moves back and forth 440 times a second. "
                   "Click Play tone to hear it, and change the frequency to see the waves get shorter or longer.";

        case Topic::waveform:
            return "A microphone works like an ear: the air pushes and pulls a thin membrane, and the microphone turns that movement "
                   "into a voltage that goes up and down with the pressure.\n\n"
                   "A waveform draws that: time goes from left to right, and the line goes up when the air is squeezed and down "
                   "when it is thinned out. The middle line is the air at rest: silence.\n\n"
                   "The picture shows the selected audio clip like the waveform editor does. Zoomed out, you see the outline of the "
                   "sound: where it is loud, where it is quiet, where a drum hits. Zoom in (scroll up over the waveform in the editor) "
                   "and the outline turns into the actual line of the air moving, down to single measurements.\n\n"
                   "Hover over the waveform in the editor and it tells you what you are looking at: silence, an attack, a fade, "
                   "or a held note and its pitch.";

        case Topic::amplitude:
            return "How far the line swings away from the middle is the amplitude. A bigger swing pushes the air harder and "
                   "sounds louder.\n\n"
                   "Levels are measured in decibels (dB). In a computer 0 dB is the biggest swing a file or a sound card can hold, "
                   "so levels are counted down from there: -6 dB, -12 dB, -20 dB. Every 6 dB is about twice or half the amplitude, "
                   "and 20 dB is ten times. A sound needs roughly 10 dB more to seem twice as loud to our ears.\n\n"
                   "The peak is the biggest swing. The RMS (the average level) says more about how loud it sounds: a sine's RMS is "
                   "3 dB below its peak, a drum hit's is far below its peak because it is short.\n\n"
                   "A swing bigger than 0 dB doesn't fit in a file. It is cut off flat (clipping), which you hear as crackle or "
                   "harshness. The waveform editor draws those parts red, and Bounce warns you when the mix peaks over 0 dB. "
                   "Gain staging (in the device bar) explains how to keep room for the loud moments.\n\n"
                   "Move the Level slider and see the swing change: half the height is -6 dB.";

        case Topic::frequency:
            return "How many times a second the air goes back and forth is the frequency, in hertz (Hz): cycles per second. "
                   "We hear it as pitch: the faster, the higher.\n\n"
                   "The A above middle C is 440 Hz. An octave higher doubles it, 880 Hz; an octave lower halves it, 220 Hz. "
                   "The low E of a bass guitar is about 41 Hz, the highest note of a piano about 4,186 Hz.\n\n"
                   "We hear from about 20 Hz (a rumble you feel more than hear) to about 20,000 Hz (20 kHz, a faint hiss). The top end "
                   "goes down with age: many adults hear little above 15 kHz.\n\n"
                   "The upper picture shows the tone: count the cycles in the time shown. The lower one shows your clip, as much "
                   "time as the tone: at the playhead while the song plays, otherwise in the middle of what the editor shows. When "
                   "the same shape repeats, DawIt counts it and names the note.";

        case Topic::timbre:
            return "A piano and a guitar playing the same A at 440 Hz both make the air go back and forth 440 times a second, but "
                   "they sound different. The difference is the shape of the wave, called the timbre or tone colour.\n\n"
                   "Every shape that repeats is made of sines: one at the pitch you hear (the fundamental), plus sines at 2, 3, 4 "
                   "times that frequency and more, the harmonics. The bars on the right show how loud each one is.\n\n"
                   "A sine is only the fundamental: pure and soft. A triangle adds a few quiet odd harmonics. A square has every odd "
                   "harmonic, hollow like a clarinet. A saw has all of them, bright and buzzy like strings or brass. Synths start "
                   "from shapes like these and filter harmonics away. An EQ turns harmonics up or down, which is why it changes the "
                   "colour of a sound.\n\n"
                   "The tone you play here is the guide's own plain generator: the bare shape, steady, with no filter and no envelope, "
                   "so you hear only what the shape does. A real synth starts from the same shapes and then shapes them. DawIt "
                   "Subtractive filters a saw, square, triangle or sine and gives every note an envelope; DawIt Additive builds a "
                   "sound from sines, one per harmonic. Hear them in What is subtractive synthesis? (The oscillators) and What is "
                   "additive synthesis?, in the Theory Guides window.\n\n"
                   "A sound also changes over the length of a note. That shape is the envelope, usually described in four parts, "
                   "ADSR:\n"
                   "- Attack: how long the sound takes to reach full level. A hi-hat or a plucked string gets there in a few "
                   "milliseconds. A slowly bowed violin note can be heard to start as much as 50 to 100 ms after it really begins.\n"
                   "- Decay: how long it then takes to fall to the level it holds.\n"
                   "- Sustain: the level it holds while the note goes on. A level, not a time.\n"
                   "- Release: how long it takes to die away at the end.\n\n"
                   "An electric organ note jumps to full level, holds it and stops. A piano or guitar note has no steady sustain: it "
                   "starts hard and keeps fading, even while the piano key is held. A pad swells in and fades out slowly.\n\n"
                   "The start matters to our ears. Cut off the first moment of a note and many instruments become hard to "
                   "recognise, a classic listening demonstration. Newer research finds it is one clue among several, next to the "
                   "harmonics and how they change during the note.\n\n"
                   "The envelope picture measures the note at the playhead or the mouse in the waveform editor. The grey line is its "
                   "level. The coloured lines are the attack, decay, sustain and release DawIt finds, with their times. Each time is "
                   "measured from 10% to 90% of the way, as engineers measure how fast a signal rises, so it comes out a little "
                   "shorter than a synth's knob says: a 50 ms straight-line attack measures 40 ms. From the audio alone DawIt can't "
                   "know when a key was let go, so in a clip the release is how the sound fades out at the end. In a recording of a "
                   "whole band, notes overlap and the envelope is of everything together. It's clearest on one instrument playing "
                   "one note at a time.\n\n"
                   "Pick a shape and click Play tone to hear the difference at the same pitch.";

        case Topic::numbers:
            return "A computer can't store a smooth line, so it measures it: thousands of times a second it writes down how high the "
                   "line is. Each measurement is a sample, the dots in the picture.\n\n"
                   "The sample rate is how many a second. CDs and streaming use 44,100 (44.1 kHz), video 48,000. You need at least "
                   "two samples per cycle to catch a frequency, so 44.1 kHz covers everything up to about 22 kHz, just past what we "
                   "hear.\n\n"
                   "The bit depth is how finely each sample is measured: the steps in the picture. 16-bit has 65,536 steps, 24-bit "
                   "almost 17 million. The tiny rounding error of each measurement is a very soft hiss, the noise floor: about 96 dB "
                   "below 0 dB for 16-bit, and so far down for 24-bit that you never hear it. A 32-bit float file stores levels "
                   "over 0 dB too, without clipping.\n\n"
                   "Mono stores one line, stereo two: one for the left speaker, one for the right.\n\n"
                   "These are the choices in Bounce to WAV. 44.1 kHz and 24-bit is a good choice for music.\n\n"
                   "The picture uses very few samples and steps so you can see them; your clip has far more.";

        case Topic::phase:
            return "Phase is where in its cycle a wave is at a moment. Two copies of a wave can line up, or one can be a little late.\n\n"
                   "When two waves meet, they add up. In step (0 degrees), the swings add: the same sound, twice the amplitude, +6 dB. "
                   "Half a cycle apart (180 degrees), one pushes while the other pulls, and they cancel: silence. In between, "
                   "the sum is somewhere between.\n\n"
                   "Move the Phase slider and see (and with Play tone, hear) the sum.\n\n"
                   "In a mix this happens by accident. A kick drum and a bass playing the same low note can cancel each other "
                   "partly, so the low end gets thinner when both play. Two microphones on one instrument at different distances "
                   "do the same. And a stereo sound whose left and right are opposite sounds wide, but vanishes when a phone or a "
                   "club plays it in mono.\n\n"
                   "If a low end sounds thin, try moving one of the sounds a few milliseconds, or flipping its polarity. "
                   "Phase cancellation, next, goes into this.";

        case Topic::cancellation:
            return "A tone has one frequency, so a copy is simply a number of degrees behind it. Real sounds have many frequencies "
                   "at once, and a copy that comes a fixed time later is a different part of a cycle late for each of them.\n\n"
                   "Take a copy 1 ms late. A 500 Hz wave lasts 2 ms, so the copy is half a cycle behind: they cancel. A 1000 Hz "
                   "wave lasts 1 ms, so the copy is a whole cycle behind, back in step: they add up, +6 dB. 1500 Hz cancels "
                   "again, 2000 Hz adds, and so on. The sound and its late copy together cancel a row of frequencies and boost "
                   "the ones between. Drawn against frequency it looks like the teeth of a comb, so it's called comb filtering. "
                   "The first notch is at 1 / (2 x the delay): 500 Hz for 1 ms, 50 Hz for 10 ms. The shorter the delay, the "
                   "higher and further apart the notches.\n\n"
                   "It sounds thin, hollow and boxy, like through a tube. When the delay slowly changes, the notches sweep up and "
                   "down: that whoosh is a flanger, an effect made on purpose from this.\n\n"
                   "Polarity is something else. Flipping the polarity turns the wave upside down: every push becomes a pull, at "
                   "every frequency at once. A flipped copy with no delay cancels everything: silence. With a delay, the comb turns "
                   "around: the lows cancel and the notches move to where the peaks were. The button marked with a slashed circle "
                   "on mixing desks and plugins, often called 'phase', is really this polarity switch.\n\n"
                   "Where it happens:\n"
                   "- Two microphones on one instrument, at different distances. Sound travels 34 cm in a millisecond, so a "
                   "microphone 34 cm further away hears it 1 ms later, with notches from 500 Hz up.\n"
                   "- A snare drum miked on top and underneath: the skin pushes towards one microphone while it pulls away from "
                   "the other, so they have opposite polarity. Flip one.\n"
                   "- A kick and a bass on the same low note, partly cancelling: the low end gets thinner when both play.\n"
                   "- Stereo that is wide because left and right differ: added together in mono (a phone, a club, a smart "
                   "speaker), parts of it cancel.\n"
                   "- Sound reflecting off a desk, a floor or a nearby wall into a microphone or your ears, a few ms later than "
                   "the direct sound.\n"
                   "- Two speakers playing the same bass in a room: in some places it cancels, so the bass sounds different in "
                   "every seat.\n"
                   "- On purpose: noise-cancelling headphones listen to the noise around you and play it flipped, so it cancels.\n\n"
                   "What to do about it:\n"
                   "- Listen in mono now and then: cancellation shows up as a sound getting thin or disappearing. Checking in mono, next, measures your clip and plays the song in mono.\n"
                   "- Try the polarity switch on one of two tracks of the same instrument, and keep whichever sounds fuller.\n"
                   "- Line up two tracks of the same sound: zoom into the waveforms until the samples show and move one so their "
                   "peaks line up. A millisecond or two makes the difference.\n"
                   "- With two microphones, put the second at least three times as far from the first as the first is from the "
                   "instrument (the 3:1 rule), so its copy is too quiet to cancel much.\n"
                   "- Keep the lowest notes in the middle and on one sound: one bass, one kick, the low end of a stereo sound mono. DawIt Stereo's Mono below does that last part.\n\n"
                   "Try it: set a delay, play the tone and move its frequency. Where the curve dips, the tone almost vanishes; "
                   "on a peak it gets louder. Flip the polarity and listen to the low tones disappear. Pick a saw or square to "
                   "hear the timbre change: each harmonic is turned up or down by the comb.";

        case Topic::mono:
            return "A lot of music is played in mono, with left and right added into one: from a phone with one speaker, many "
                   "Bluetooth and smart speakers, a laptop, the ceiling speakers in a shop, and the subwoofers of many club systems. "
                   "Phones with two speakers have them only a phone's length apart.\n\n"
                   "What's the same in both speakers adds up and stays. What's opposite cancels, as a flipped copy does in Phase "
                   "cancellation. Everything in between partly stays. So a stereo clip can sound full on headphones and thin on a "
                   "phone, and nothing warns you.\n\n"
                   "The picture measures your clip. Stereo is the average level of left and right. Mono is left and right added and "
                   "halved, as DawIt Stereo's Mono plays it, so a sound in the middle keeps its level exactly. Against that:\n"
                   "- a sound in the middle loses 0 dB;\n"
                   "- a sound on one side only loses 3 dB, and so do a left and right that have nothing to do with each other, like "
                   "two different guitar takes or a wide reverb;\n"
                   "- a left and right that partly work against each other lose more: 6 dB when they're equally loud with a "
                   "correlation of -0.5, and 10 dB when one side is the other flipped at half the level;\n"
                   "- exactly opposite, everything cancels.\n\n"
                   "The correlation is the number DawIt Stereo's meter shows: +1 when left and right move together, 0 when they're "
                   "unrelated, -1 when they're opposite. Between 0 and +1 is generally safe in mono.\n\n"
                   "The bands show where the loss is. A low end that cancels in mono is the worst kind: the bass and kick get thin on "
                   "phones and disappear from a club's mono subwoofers. That's one reason the lowest notes, below about 100 to "
                   "150 Hz, are usually kept in the middle. Highs that drop back in mono, like the air of a wide reverb or cymbals, "
                   "are usually fine.\n\n"
                   "Hear it: switch on Hear in mono and play the song. Both speakers then play left and right added together (a "
                   "bounce is never changed). Listen for anything important getting quieter or vanishing: the vocal, the kick, the "
                   "bass. Many engineers also check on one small speaker, which is closer to a phone. The aim isn't to make the mix "
                   "sound as good in mono, only to make sure nothing important disappears.\n\n"
                   "If something does, flip the polarity of one of two tracks of the same instrument, line up two microphones (Phase "
                   "cancellation), turn down a stereo widener, or make the low end mono with DawIt Stereo's Mono below. Mono in the "
                   "Stereo guide goes further.\n\n"
                   "This measures the clip as its file holds it, not the track's pan, effects or the rest of the mix. Hear in mono "
                   "plays the whole mix.";

        case Topic::vocals:
            return "A karaoke version is a song without its singer, to sing over yourself or to practise an instrument with. "
                   "There are two ways to make one from a recording, and only one of them works well.\n\n"
                   "The old trick is left minus right. In most mixes the lead vocal sits in the middle: the same in both speakers. "
                   "Take the right side away from the left, and whatever is the same in both cancels, as a flipped copy does in "
                   "Phase cancellation. The voice goes. Karaoke machines and old audio software had a 'vocal cancel' button that "
                   "did just this.\n\n"
                   "But the middle holds more than the voice. The kick drum, the snare and the bass are usually in the middle too, "
                   "and so is nearly everything below about 150 Hz, because low notes in the middle share the work between both "
                   "speakers. They cancel as well, so what's left sounds thin, without its low end and its punch. What stays is "
                   "everything that differs between left and right: guitars panned to the sides, wide keys and pads, and the reverb "
                   "and echo on the voice, which are spread over both speakers. So a faint, hollow ghost of the voice is left. And "
                   "since both speakers then play the same difference, the song is in mono.\n\n"
                   "The picture shows a typical mix from left to right, then what left minus right leaves of it, and what Remove "
                   "vocals leaves. With a clip in the waveform editor, it measures on your own clip how much quieter left minus "
                   "right makes it, and how much of its low end goes.\n\n"
                   "Remove vocals (karaoke version), in an audio clip's right-click menu, uses Demucs, the model behind Split into "
                   "stems. It doesn't look at where a sound is panned. It has learned from many songs, and their separate parts, "
                   "what a voice sounds like next to drums, bass and other instruments, and splits the mix into those four. The "
                   "four add up to the mix again, so the drums, bass and other sounds together are the song without the vocals, "
                   "with its low end, its stereo and its punch. It isn't perfect either: some reverb on the voice can stay "
                   "faintly, and so can a melody an instrument plays along with the singer. The backing vocals go with the lead.\n\n"
                   "Hear it for yourself: switch on Hear left minus right below the picture and play the song. Both speakers then "
                   "play only the difference between left and right (a bounce is never changed). Compare it with Remove vocals.";
    }

    return {};
}

//==============================================================================
juce::String getShapeName (Shape shape)
{
    switch (shape)
    {
        case Shape::sine:     return "Sine";
        case Shape::triangle: return "Triangle";
        case Shape::saw:      return "Saw";
        case Shape::square:   return "Square";
    }

    return {};
}

namespace
{
    /** Smooths a jump of 2 at phase 0 over one sample on each side (polyBLEP). */
    double blep (double t, double dt)
    {
        if (dt <= 0.0)
            return 0.0;

        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }

        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }

        return 0.0;
    }
}

float getSample (Shape shape, double phase, double increment)
{
    phase -= std::floor (phase);

    switch (shape)
    {
        case Shape::sine:
            return (float) std::sin (juce::MathConstants<double>::twoPi * phase);

        case Shape::triangle:
            return (float) (phase < 0.25 ? 4.0 * phase : phase < 0.75 ? 2.0 - 4.0 * phase : 4.0 * phase - 4.0);

        case Shape::saw:
        {
            // Starts at 0 going up like the sine, and drops halfway.
            double q = phase + 0.5;
            q -= std::floor (q);
            return (float) (2.0 * q - 1.0 - blep (q, increment));
        }

        case Shape::square:
        {
            double half = phase + 0.5;
            half -= std::floor (half);
            return (float) ((phase < 0.5 ? 1.0 : -1.0) + blep (phase, increment) - blep (half, increment));
        }
    }

    return 0.0f;
}

float getHarmonicLevel (Shape shape, int n)
{
    if (n < 1)
        return 0.0f;

    switch (shape)
    {
        case Shape::sine:     return n == 1 ? 1.0f : 0.0f;
        case Shape::triangle: return n % 2 == 1 ? 1.0f / (float) (n * n) : 0.0f;
        case Shape::saw:      return 1.0f / (float) n;
        case Shape::square:   return n % 2 == 1 ? 1.0f / (float) n : 0.0f;
    }

    return 0.0f;
}

//==============================================================================
juce::String getNoteName (float hz)
{
    if (hz <= 0.0f)
        return {};

    const int note = juce::roundToInt (69.0f + 12.0f * std::log2 (hz / 440.0f));
    return juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, note), true, true, 4);
}

juce::String formatHz (float hz)
{
    if (hz >= 10000.0f)
        return juce::String (hz / 1000.0f, 1) + " kHz";

    if (hz >= 1000.0f)
        return juce::String (hz / 1000.0f, 2) + " kHz";

    return juce::String (juce::roundToInt (hz)) + " Hz";
}

juce::String formatTime (double seconds)
{
    if (seconds >= 1.0)
        return juce::String (seconds, 1) + " s";

    if (seconds >= 0.1)
        return juce::String (seconds, 2) + " s";

    const double ms = seconds * 1000.0;
    return (ms >= 10.0 ? juce::String (juce::roundToInt (ms)) : juce::String (ms, ms >= 1.0 ? 1 : 2)) + " ms";
}

juce::String formatWavelength (float hz)
{
    const float metres = speedOfSound / juce::jmax (1.0f, hz);

    if (metres >= 1.0f)
        return (metres >= 10.0f ? juce::String (juce::roundToInt (metres)) : juce::String (metres, 1)) + " m";

    const float cm = metres * 100.0f;
    return (cm >= 10.0f ? juce::String (juce::roundToInt (cm)) : juce::String (cm, 1)) + " cm";
}

float estimatePitch (const float* x, int n, double rate)
{
    // The tracker DawIt Tune uses, asked once. Its plans are kept for the next call (the wave editor asks as the mouse moves).
    thread_local pitch::Tracker tracker;
    thread_local double preparedRate = 0.0;
    thread_local int preparedSize = 0;

    if (n < 16)
        return 0.0f;

    if (! juce::exactlyEqual (preparedRate, rate) || preparedSize < n)
    {
        preparedRate = rate;
        preparedSize = juce::jmax (n, 4096);
        tracker.prepare (rate, preparedSize);
    }

    const auto reading = tracker.analyse (x, n, 0.75f);   // no loudness limit: the old estimate only left out silence
    return reading.clarity >= 0.75f ? (float) pitch::hzFromMidi (reading.midi) : 0.0f;
}

juce::String describeMoment (float rmsBeforeDb, float rmsHereDb, float peakHereDb, float pitchHz)
{
    if (peakHereDb < -60.0f)
        return "Silence: the line stays on the middle, so the air (and your speaker) doesn't move.";

    juce::String text;
    const juce::String pitch = pitchHz > 0.0f ? "about " + formatHz (pitchHz) + ", close to " + getNoteName (pitchHz) : juce::String();

    if (rmsHereDb - rmsBeforeDb >= 10.0f)
        text << "An attack: the level jumps up by about " << juce::roundToInt (rmsHereDb - juce::jmax (rmsBeforeDb, -80.0f))
             << " dB in a few milliseconds, like a drum hit, a plucked string or the start of a word. Zoom in to see where the sound starts.";
    else if (rmsBeforeDb - rmsHereDb >= 6.0f)
        text << "Fading out: the swings get smaller, the decay or release of a sound" << (pitch.isNotEmpty() ? " (" + pitch + ")." : juce::String ("."));
    else if (pitchHz > 0.0f)
        text << "A held sound with a pitch, " << pitch << ". Zoom in and the same shape repeats, "
             << juce::roundToInt (pitchHz) << " times a second.";
    else
        text << "A sound without one clear pitch: noise, a cymbal, breath, or several notes at once. "
                "Zoom in and the shape doesn't repeat neatly.";

    return text << "\nLoudest here: " << juce::String (peakHereDb, 1) << " dB.";
}
float getCopySumDb (double hz, double delaySeconds, bool flipped)
{
    // |1 + c e^(-j 2 pi f t)|, with c = 1, or -1 for a flipped copy.
    const double angle = juce::MathConstants<double>::twoPi * hz * delaySeconds, sign = flipped ? -1.0 : 1.0;
    const double re = 1.0 + sign * std::cos (angle), im = -sign * std::sin (angle);
    return juce::jmax (-100.0f, (float) (20.0 * std::log10 (juce::jmax (1.0e-6, std::sqrt (re * re + im * im)))));
}

//==============================================================================
LeftMinusRight measureLeftMinusRight (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    LeftMinusRight result;
    const int n = audio.getNumSamples();

    if (n == 0 || audio.getNumChannels() == 0)
        return result;

    const float* left = audio.getReadPointer (0);
    const float* right = audio.getReadPointer (juce::jmin (1, audio.getNumChannels() - 1));

    // Two low-pass filters at lowCutHz in a row, so little above it gets through.
    const auto coefficients = juce::IIRCoefficients::makeLowPass (sampleRate, lowCutHz);
    std::array<juce::IIRFilter, 6> filters;
    for (auto& f : filters)
        f.setCoefficients (coefficients);

    double mix = 0.0, difference = 0.0, mixLow = 0.0, differenceLow = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const float l = left[i], r = right[i];
        const float d = (l - r) * juce::MathConstants<float>::sqrt2 * 0.5f;
        const float lowL = filters[1].processSingleSampleRaw (filters[0].processSingleSampleRaw (l));
        const float lowR = filters[3].processSingleSampleRaw (filters[2].processSingleSampleRaw (r));
        const float lowD = filters[5].processSingleSampleRaw (filters[4].processSingleSampleRaw (d));

        mix += 0.5 * ((double) l * l + (double) r * r);
        difference += (double) d * d;
        mixLow += 0.5 * ((double) lowL * lowL + (double) lowR * lowR);
        differenceLow += (double) lowD * lowD;
    }

    auto toDb = [n] (double sum) { return juce::Decibels::gainToDecibels ((float) std::sqrt (sum / n), -100.0f); };
    result.mixDb = toDb (mix);
    result.differenceDb = toDb (difference);
    result.mixLowDb = toDb (mixLow);
    result.differenceLowDb = toDb (differenceLow);
    result.stereo = audio.getNumChannels() > 1 && result.differenceDb > result.mixDb - 60.0f;   // not the same on both sides
    return result;
}

const std::vector<MixPart>& getTypicalMix()
{
    static const std::vector<MixPart> parts {
        { "Lead vocal",   0.0f,  0.0f, true,  false },
        { "Vocal reverb", 0.0f,  1.0f, true,  false },
        { "Kick",         0.0f,  0.0f, false, true },
        { "Snare",        0.0f,  0.0f, false, false },
        { "Bass",         0.0f,  0.0f, false, true },
        { "Hi-hat",       0.3f,  0.0f, false, false },
        { "Guitar",      -0.8f,  0.0f, false, false },
        { "Guitar",       0.8f,  0.0f, false, false },
        { "Keys",         0.0f,  0.7f, false, false },
    };

    return parts;
}

bool survivesLeftMinusRight (const MixPart& part)
{
    return std::abs (part.pan) > 0.05f || part.width > 0.2f;
}
//==============================================================================
namespace
{
    double medianOf (std::vector<float> values)
    {
        if (values.empty())
            return 0.0;

        std::nth_element (values.begin(), values.begin() + (long) values.size() / 2, values.end());
        return values[values.size() / 2];
    }
}

Envelope measureEnvelope (const juce::AudioBuffer<float>& audio, double rate, int focusSample, float gain)
{
    Envelope e;
    const int n = audio.getNumSamples();

    if (n == 0 || audio.getNumChannels() == 0 || rate <= 0.0)
        return e;

    const int block = juce::jmax (1, juce::roundToInt (0.005 * rate));
    const int focus = juce::jlimit (0, n - 1, focusSample);
    const int lo = juce::jmax (0, focus - juce::roundToInt (3.0 * rate));
    const int hi = juce::jmin (n, focus + juce::roundToInt (6.0 * rate));
    const int count = (hi - lo) / block;

    if (count < 3)
        return e;

    // Power in 5 ms blocks (the mean of both channels), then a light smoothing over three blocks so a low
    // note's ripple (a 55 Hz sine is only a quarter cycle per block) doesn't wobble the times.
    const float* left = audio.getReadPointer (0);
    const float* right = audio.getReadPointer (juce::jmin (1, audio.getNumChannels() - 1));
    const bool two = audio.getNumChannels() > 1;
    std::vector<double> raw ((size_t) count);

    for (int k = 0; k < count; ++k)
    {
        double sum = 0.0;

        for (int i = lo + k * block; i < lo + (k + 1) * block; ++i)
            sum += two ? 0.5 * ((double) left[i] * left[i] + (double) right[i] * right[i]) : (double) left[i] * left[i];

        raw[(size_t) k] = sum / block * (double) gain * gain;
    }

    std::vector<float> level ((size_t) count), db ((size_t) count), fast ((size_t) count);
    float loudest = 0.0f;

    for (int k = 0; k < count; ++k)
    {
        const double p = (raw[(size_t) juce::jmax (0, k - 1)] + raw[(size_t) k] + raw[(size_t) juce::jmin (count - 1, k + 1)]) / 3.0;
        level[(size_t) k] = (float) std::sqrt (p);
        fast[(size_t) k] = (float) std::sqrt (raw[(size_t) k]);
        db[(size_t) k] = juce::Decibels::gainToDecibels (level[(size_t) k], -100.0f);
        loudest = juce::jmax (loudest, level[(size_t) k]);
    }

    e.contour = level;
    e.contourStartSeconds = lo / rate;
    e.blockSeconds = block / rate;
    e.silent = loudest < 0.001f;
    auto at = [&] (const double index) { return (lo + (index + 0.5) * block) / rate; };
    auto L = [&] (int k) { return level[(size_t) k]; };

    const int f = juce::jmin (count - 1, (focus - lo) / block);

    if (L (f) < 0.001f)
        return e;

    // A rise of 10 dB or more within 30 ms starts here (the same 10 dB as describeMoment).
    auto riseStart = [&] (int k)
    {
        for (int j = 1; j <= 6 && k + j < count; ++j)
            if (db[(size_t) (k + j)] - db[(size_t) k] >= 10.0f)
                return true;

        return false;
    };

    // Walk back from the focus to where the note starts.
    int onset = -1;
    float running = L (f);

    for (int k = f;; --k)
    {
        running = juce::jmax (running, L (k));

        if (L (k) < 0.001f || L (k) < running * 0.01f)
        {
            onset = k + 1;
            break;
        }

        if (riseStart (k) && (k == 0 || L (k - 1) >= L (k)))
        {
            onset = k;
            break;
        }

        if (k == 0)
        {
            onset = lo == 0 ? 0 : -1;   // started before what we look at
            break;
        }
    }

    if (onset < 0)
        return e;

    // Walk forward to the peak and to where the note ends.
    const int limit = juce::jmin (count - 1, onset + juce::roundToInt (6.0 / (block / rate)));
    int peak = onset, end = limit;

    for (int k = onset; k <= limit; ++k)
    {
        if (L (k) > L (peak))
            peak = k;

        if (k > peak && L (k) <= L (peak) * 0.01f)
        {
            end = k;
            break;
        }

        if (k > peak + 2 && L (k) <= L (k - 1) && db[(size_t) k] <= db[(size_t) peak] - 3.0f && riseStart (k))
        {
            end = k;
            break;
        }
    }

    if (f > end)
        return e;

    const float peakLevel = L (peak);
    // The first time after `from` that the contour is at `threshold` going up (or down), in block units.
    auto crossingIn = [&] (const std::vector<float>& v, int from, int to, float threshold, bool rising) -> double
    {
        for (int k = from; k <= to; ++k)
        {
            if (rising ? v[(size_t) k] >= threshold : v[(size_t) k] <= threshold)
            {
                if (k == from)
                    return k;

                const double a = v[(size_t) k - 1], b = v[(size_t) k];
                return k - 1 + (threshold - a) / (b - a);
            }
        }

        return -1.0;
    };
    auto crossing = [&] (int from, int to, float threshold, bool rising) { return crossingIn (level, from, to, threshold, rising); };

    e.found = true;
    e.silent = false;
    e.onsetSeconds = at (onset);
    e.peakSeconds = at (peak);
    e.endSeconds = at (end);
    e.peakDb = db[(size_t) peak];

    // The attack is timed on the unsmoothed blocks, so a hard start isn't spread out by the smoothing.
    int fastPeak = onset;
    for (int k = onset; k <= juce::jmin (end, peak + 2); ++k)
        if (fast[(size_t) k] > fast[(size_t) fastPeak])
            fastPeak = k;

    const float fastLevel = fast[(size_t) fastPeak];
    e.attackStart = at (juce::jmax (0.0, crossingIn (fast, onset, fastPeak, 0.1f * fastLevel, true)));
    e.attackEnd = at (juce::jmax (0.0, crossingIn (fast, onset, fastPeak, 0.9f * fastLevel, true)));
    e.attack = juce::jmax (0.0, e.attackEnd - e.attackStart);

    // The body is from the peak to the release start; a first guess at the sustain finds where that is.
    auto middleThird = [&] (int from, int to)
    {
        const int a = from + (to - from) / 3, b = juce::jmax (a, from + 2 * (to - from) / 3);
        return medianOf (std::vector<float> (level.begin() + a, level.begin() + b + 1));
    };
    auto lastAbove = [&] (float threshold)
    {
        int last = peak;

        for (int k = peak; k <= end; ++k)
            if (L (k) >= threshold)
                last = k;

        return last;
    };

    float sustain = (float) middleThird (peak, end);
    int releaseStartBlock = lastAbove (0.9f * sustain);
    e.body = (releaseStartBlock - peak) * (block / rate);

    // A straight line through the body's dB: falling by 6 dB a second or more means it is fading.
    if (releaseStartBlock - peak >= 2)
    {
        double sumT = 0.0, sumDb = 0.0, sumTT = 0.0, sumTDb = 0.0;
        const int points = releaseStartBlock - peak + 1;

        for (int k = peak; k <= releaseStartBlock; ++k)
        {
            const double t = (k - peak) * (block / rate), v = db[(size_t) k];
            sumT += t; sumDb += v; sumTT += t * t; sumTDb += t * v;
        }

        const double denominator = points * sumTT - sumT * sumT;
        const double slope = denominator > 0.0 ? (points * sumTDb - sumT * sumDb) / denominator : 0.0;

        if (slope <= -6.0)
        {
            e.sustained = false;
            e.fadeDbPerSecond = (float) -slope;
            return e;
        }
    }

    e.sustained = true;
    sustain = (float) middleThird (peak, juce::jmax (peak, releaseStartBlock));
    releaseStartBlock = lastAbove (0.9f * sustain);
    e.sustainDb = juce::jmin (0.0f, juce::Decibels::gainToDecibels (sustain / peakLevel, -100.0f));
    e.hasDecay = e.sustainDb <= -1.0f;
    e.decayStart = e.peakSeconds;
    e.decayEnd = e.peakSeconds;

    if (e.hasDecay)
    {
        const double d10 = crossing (peak, end, peakLevel - 0.1f * (peakLevel - sustain), false);
        const double d90 = crossing (peak, end, peakLevel - 0.9f * (peakLevel - sustain), false);

        if (d10 >= 0.0 && d90 >= 0.0)
        {
            e.decayStart = at (d10);
            e.decayEnd = at (d90);
            e.decay = juce::jmax (0.0, e.decayEnd - e.decayStart);
        }
    }

    // The release: from 90% of the sustain down to 10% of it, after the last time it was held.
    const double r90 = crossing (releaseStartBlock, end, 0.9f * sustain, false);
    const double r10 = crossing (releaseStartBlock, end, 0.1f * sustain, false);
    e.releaseStart = at (r90 >= 0.0 ? r90 : releaseStartBlock);
    e.body = juce::jmax (0.0, e.releaseStart - e.peakSeconds);
    e.hold = juce::jmax (0.0, e.releaseStart - e.decayEnd);

    if (r10 >= 0.0 && r90 >= 0.0)
    {
        e.releaseFound = true;
        e.releaseEnd = at (r10);
        e.release = juce::jmax (0.0, e.releaseEnd - e.releaseStart);
    }

    return e;
}

juce::String describeEnvelope (const Envelope& e)
{
    if (! e.found)
        return e.silent ? "Silence: nothing to measure." : "No single note here. Move the playhead or the mouse onto a note.";

    const juce::String attack = formatTime (e.attack);
    juce::String text;

    if (e.body < 0.1)
        text << "A short sound: attack " << attack << ", gone after " << formatTime (e.endSeconds - e.onsetSeconds)
             << ". Like a drum hit or a click.";
    else if (! e.sustained)
        text << "Attack " << attack << ", then it keeps fading, about " << juce::roundToInt (e.fadeDbPerSecond)
             << " dB a second, with no steady sustain: like a piano, a plucked string or a struck drum.";
    else
    {
        const juce::String fadeOut = e.releaseFound ? ", then fades out in " + formatTime (e.release) : juce::String();

        if (e.hasDecay)
            text << "Attack " << attack << ", decay " << formatTime (e.decay) << " down to " << juce::String (e.sustainDb, 1)
                 << " dB, it holds for " << formatTime (e.hold) << fadeOut << ".";
        else
            text << "Attack " << attack << ", no decay: it holds at full level for " << formatTime (e.hold) << fadeOut << ".";
    }

    if (e.attack < 0.01)
        text << " A hard start, like a pluck, a hit or an electric organ key.";
    else if (e.attack >= 0.1)
        text << " A slow swell, like a pad or a softly bowed string.";

    return text;
}

//==============================================================================
namespace
{
    constexpr std::array<float, numMonoBands + 1> monoEdges { 0.0f, lowCutHz, 500.0f, 2000.0f, 6000.0f, 1.0e9f };
}

juce::String getMonoBandName (int index)
{
    switch (index)
    {
        case 0:  return "Low: bass and kick, below 150 Hz";
        case 1:  return "Low mids 150-500 Hz";
        case 2:  return "Mids 500 Hz-2 kHz";
        case 3:  return "Upper mids 2-6 kHz";
        case 4:  return "Highs above 6 kHz";
        default: break;
    }

    return {};
}

MonoCheck measureMono (const juce::AudioBuffer<float>& audio, double rate)
{
    MonoCheck result;
    const int n = audio.getNumSamples();

    if (n == 0 || audio.getNumChannels() == 0 || rate <= 0.0)
        return result;

    const float* left = audio.getReadPointer (0);
    const float* right = audio.getReadPointer (juce::jmin (1, audio.getNumChannels() - 1));
    double leftPower = 0.0, rightPower = 0.0, cross = 0.0, midPower = 0.0, sidePower = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const double l = left[i], r = right[i], mid = 0.5 * (l + r), side = 0.5 * (l - r);
        leftPower += l * l;
        rightPower += r * r;
        cross += l * r;
        midPower += mid * mid;
        sidePower += side * side;
    }

    const double stereoMs = 0.5 * (leftPower + rightPower) / n, monoMs = midPower / n;
    result.stereoDb = juce::Decibels::gainToDecibels ((float) std::sqrt (stereoMs), -100.0f);
    result.monoDb = juce::Decibels::gainToDecibels ((float) std::sqrt (monoMs), -100.0f);
    result.lossDb = juce::jmin (0.0f, result.monoDb - result.stereoDb);
    result.stereo = audio.getNumChannels() > 1 && sidePower * 2.0 > stereoMs * n * 1.0e-6;   // the same test as LeftMinusRight
    result.correlation = (float) stereo::correlation (leftPower, rightPower, cross).value_or (0.0);
    result.measurable = n >= juce::roundToInt (0.1 * rate) && result.stereoDb >= -60.0f;

    if (! result.stereo)
        return result;

    // Power per band from Hann-windowed FFT frames of mid and side: the stereo power is mid + side, the mono power is mid.
    constexpr int order = 12, size = 1 << order, hop = size / 2;
    juce::dsp::FFT fft (order);
    std::vector<float> window ((size_t) size), midData ((size_t) size * 2), sideData ((size_t) size * 2);
    double windowPower = 0.0;

    for (int i = 0; i < size; ++i)
    {
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);
        windowPower += (double) window[(size_t) i] * window[(size_t) i];
    }

    std::array<double, numMonoBands> midBand {}, sideBand {};
    int frames = 0;

    for (int start = 0; start == 0 || start + size <= n; start += hop)
    {
        std::fill (midData.begin(), midData.end(), 0.0f);
        std::fill (sideData.begin(), sideData.end(), 0.0f);

        for (int i = 0; i < size && start + i < n; ++i)
        {
            const float l = left[start + i], r = right[start + i];
            midData[(size_t) i] = 0.5f * (l + r) * window[(size_t) i];
            sideData[(size_t) i] = 0.5f * (l - r) * window[(size_t) i];
        }

        fft.performRealOnlyForwardTransform (midData.data(), true);
        fft.performRealOnlyForwardTransform (sideData.data(), true);

        for (int bin = 1; bin < size / 2; ++bin)
        {
            const float hz = (float) (bin * rate / size);
            int band = 0;

            while (band < numMonoBands - 1 && hz >= monoEdges[(size_t) band + 1])
                ++band;

            midBand[(size_t) band] += (double) midData[(size_t) bin * 2] * midData[(size_t) bin * 2] + (double) midData[(size_t) bin * 2 + 1] * midData[(size_t) bin * 2 + 1];
            sideBand[(size_t) band] += (double) sideData[(size_t) bin * 2] * sideData[(size_t) bin * 2] + (double) sideData[(size_t) bin * 2 + 1] * sideData[(size_t) bin * 2 + 1];
        }

        ++frames;
    }

    // Parseval: the mean square of a frame is 2 x the sum over the one-sided bins / (size x the window's power).
    const double toMeanSquare = 2.0 / ((double) size * windowPower * frames);
    float loudest = -100.0f;

    for (int b = 0; b < numMonoBands; ++b)
    {
        const double mid = midBand[(size_t) b] * toMeanSquare, side = sideBand[(size_t) b] * toMeanSquare;
        auto& band = result.bands[(size_t) b];
        band.stereoDb = (float) juce::jmax (-100.0, 10.0 * std::log10 (juce::jmax (1.0e-10, mid + side)));
        band.monoDb = (float) juce::jmax (-100.0, 10.0 * std::log10 (juce::jmax (1.0e-10, mid)));
        band.lossDb = mid + side > 1.0e-10 ? juce::jmin (0.0f, band.monoDb - band.stereoDb) : 0.0f;
        loudest = juce::jmax (loudest, band.stereoDb);
    }

    float worst = -1.0f;   // a loss has to reach 1 dB to count

    for (int b = 0; b < numMonoBands; ++b)
    {
        const auto& band = result.bands[(size_t) b];

        if (band.stereoDb >= loudest - 30.0f && band.lossDb <= worst)
        {
            worst = band.lossDb;
            result.mostLostBand = b;
        }
    }

    return result;
}

juce::String describeMono (const MonoCheck& m)
{
    if (! m.stereo)
        return "Your clip is mono: left and right are the same, so nothing changes in mono.";

    if (! m.measurable)
        return "Too short or too quiet to measure.";

    const juce::String loss = juce::String (-m.lossDb, 1), rho = (m.correlation >= 0.0f ? "+" : "") + juce::String (m.correlation, 2);
    juce::String text;

    if (m.lossDb >= -1.5f)
        text << "Mostly in the middle: in mono it's only " << loss << " dB quieter (correlation " << rho << "), so the clip as a whole holds up in mono.";
    else if (m.lossDb >= -4.0f && m.correlation >= 0.0f)
        text << "Wide, as stereo music often is: " << loss << " dB quieter in mono (correlation " << rho << "). Unrelated left and right lose 3 dB, "
                "so this is normal for wide pads, reverbs and doubled guitars. Listen whether the important parts stay clear.";
    else
        text << "Parts of this clip work against each other: in mono it loses " << loss << " dB, more than the 3 dB of unrelated left and right "
                "(correlation " << rho << "). Look for a flipped polarity, a stereo widener, or two microphones out of step (Phase cancellation).";

    if (m.mostLostBand >= 0 && m.bands[(size_t) m.mostLostBand].lossDb <= -6.0f)
    {
        const juce::String b = juce::String (-m.bands[(size_t) m.mostLostBand].lossDb, 1);

        switch (m.mostLostBand)
        {
            case 0:
                text << " The low end loses most: " << b << " dB below 150 Hz. The bass and kick will sound thin on phones and club subwoofers. "
                        "Try DawIt Stereo's Mono below 120 to 150 Hz on this track.";
                break;

            case 1:
            case 2:
                text << " The " << (m.mostLostBand == 1 ? "low mids" : "mids") << " lose most (" << b
                     << " dB), where vocals, guitars and keys sit. Listen in mono whether a part drops away.";
                break;

            default:
                text << " The " << (m.mostLostBand == 3 ? "upper mids" : "highs") << " lose most (" << b
                     << " dB): wide cymbals, reverbs or a widener. Usually fine; check that the vocal keeps its clarity.";
                break;
        }
    }

    return text;
}

//==============================================================================
juce::AudioBuffer<float> makeExampleClip (double rate)
{
    const int n = juce::roundToInt (8.0 * rate);
    juce::AudioBuffer<float> clip (2, n);
    clip.clear();

    // Unrelated air above 6 kHz on each side: white noise through four high-pass stages, scaled to RMS 0.02.
    std::array<std::vector<float>, 2> air;
    juce::Random random (1234);

    for (int ch = 0; ch < 2; ++ch)
    {
        const auto coefficients = juce::IIRCoefficients::makeHighPass (rate, 7000.0);
        std::array<juce::IIRFilter, 4> stages;
        for (auto& stage : stages)
            stage.setCoefficients (coefficients);

        auto& noise = air[(size_t) ch];
        noise.resize ((size_t) n);
        double power = 0.0;

        for (auto& v : noise)
        {
            float x = random.nextFloat() * 2.0f - 1.0f;
            for (auto& stage : stages)
                x = stage.processSingleSampleRaw (x);
            v = x;
            power += (double) x * x;
        }

        const float scale = 0.02f / (float) std::sqrt (power / n);
        for (auto& v : noise)
            v *= scale;
    }

    // One envelope for every part: A 20 ms, D 200 ms to 50%, held to 1.5 s, R 300 ms.
    auto envelope = [] (double t)
    {
        if (t < 0.0 || t >= 1.8) return 0.0;
        if (t < 0.02) return t / 0.02;
        if (t < 0.22) return 1.0 - 0.5 * (t - 0.02) / 0.2;
        if (t < 1.5)  return 0.5;
        return 0.5 * (1.0 - (t - 1.5) / 0.3);
    };

    const double increment = 440.0 / rate;

    for (int i = 0; i < n; ++i)
    {
        const double t = i / rate, local = std::fmod (t, 2.0);
        const float env = (float) envelope (local);
        const float saw = 0.4f * getSample (Shape::saw, 440.0 * local, increment);
        const float bass = (float) std::sin (juce::MathConstants<double>::twoPi * 55.0 * local);
        clip.setSample (0, i, env * (saw + 0.2f * bass + air[0][(size_t) i]));
        clip.setSample (1, i, env * (saw - 0.1f * bass + air[1][(size_t) i]));
    }

    return clip;
}
} // namespace daw::soundguide
