#include "RecordingGuide.h"

#include <algorithm>
#include <cmath>

#include "GainStaging.h"
#include "SoundGuide.h"

namespace daw::recordingguide
{
namespace
{
    juce::String oneDecimal (double value)
    {
        return juce::String (value, 1);
    }
}

//==============================================================================
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::air:            return "From the air to a file";
        case Topic::audioInterface: return "The audio interface";
        case Topic::chooseInput:    return "Choosing an input";
        case Topic::level:          return "Setting the level";
        case Topic::buffers:        return "Buffers and latency";
        case Topic::monitoring:     return "Hearing yourself";
        case Topic::moved:          return "Why takes are moved earlier";
        case Topic::takes:          return "Takes and loop recording";
        case Topic::yourSession:    return "Your session";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::air:            return "What happens between your voice and the take";
        case Topic::audioInterface: return "Mic, instrument and line inputs, and 48 V phantom power";
        case Topic::chooseInput:    return "Which socket a track records, mono or stereo";
        case Topic::level:          return "Loudest peaks around -18 to -10 dB, never at 0";
        case Topic::buffers:        return "Why sound through the computer comes out a little late";
        case Topic::monitoring:     return "Direct monitoring, the Monitor button, and headphones";
        case Topic::moved:          return "Latency correction and Measure latency";
        case Topic::takes:          return "Several passes, keep the best";
        case Topic::yourSession:    return "Your device, its latency and your last take";
    }

    return {};
}

juce::String explain (Topic topic)
{
    switch (topic)
    {
        case Topic::air:
            return "Recording turns sound in the room into a clip on a track. On the way it passes five stops, the boxes in the picture.\n\n"
                   "1. The microphone turns the moving air into a tiny electrical signal, a few thousandths of a volt. An electric "
                   "guitar's pickups do the same from the strings.\n"
                   "2. The audio interface's preamp makes that signal bigger, often by 30 to 60 dB (30 to 1,000 times); its gain knob "
                   "sets how much. Its converter then measures the signal 48,000 times a second at 48 kHz and turns it into numbers "
                   "(What is sound? explains samples). A laptop's built-in microphone has all of this inside the computer.\n"
                   "3. The numbers arrive in small batches, the buffer, for example 128 samples at a time. DawIt takes a batch in and "
                   "sends the next batch of the song out, hundreds of times a second.\n"
                   "4. DawIt writes what comes in to a WAV file while you play, so a crash doesn't lose the take. A minute of mono at "
                   "48 kHz and 24 bits is about 8.6 MB.\n"
                   "5. When you stop, the take becomes a clip on the track, moved earlier by the delay of the trip (Why takes are "
                   "moved earlier says why).\n\n"
                   "What is saved is the input as it came in. The track's effects and fader change what you hear, not the take, so "
                   "you can change them afterwards without recording again.\n\n"
                   "To record: arm an audio track with its R button, choose its input in the chip next to R, and press Record (or R). "
                   "The count-in and metronome work as for MIDI. Press Record again, or Stop, to end the take.";

        case Topic::audioInterface:
            return "An audio interface is the box between your instruments and the computer: inputs with gain knobs, converters both "
                   "ways, and outputs for speakers and headphones. The Mac's built-in microphone is fine for trying ideas out; an "
                   "interface sounds cleaner, takes proper microphones and instruments, and lets you hear yourself without delay.\n\n"
                   "Three kinds of signal come in, at very different strengths:\n"
                   "- Mic level: a few thousandths of a volt (about 2.5 mV is a usual figure). It needs the most gain.\n"
                   "- Instrument level: an electric guitar or bass straight from its jack. Stronger than a microphone, weaker than "
                   "line level, and different for every guitar. Use the input marked Inst or Hi-Z: it's made for pickups, which sound "
                   "thin and dull on an ordinary input.\n"
                   "- Line level: keyboards, drum machines, a mixer's output. Studio gear works at +4 dBu, about 1.23 volts; home "
                   "gear at -10 dBV, about 0.32 volts. It needs little or no gain.\n\n"
                   "Most interfaces have combo sockets that take a microphone's XLR plug or a jack.\n\n"
                   "Phantom power, the 48V button, sends 48 volts up the microphone cable to run a condenser microphone's electronics "
                   "(the standard, IEC 61938, calls it P48). Dynamic microphones such as the Shure SM58 don't need it, and on a normal "
                   "balanced cable it does them no harm. Ribbon microphones are the exception: leave it off unless the maker says "
                   "yours needs it.\n\n"
                   "DawIt records at the sample rate set in Audio / MIDI Settings and saves 24-bit WAV files. 44.1 or 48 kHz is fine "
                   "for music.\n\n"
                   "This topic doesn't say which interface or microphone to buy; the number of inputs you need at once is the first "
                   "thing to decide.";

        case Topic::chooseInput:
            return "Each socket on the interface is a numbered input: Input 1, Input 2 and so on. A track records the one shown next "
                   "to its R button; click it to choose.\n\n"
                   "Mono or stereo? One microphone or one guitar is one channel: record it mono, on Input 1 or Input 2. Mono isn't "
                   "worse. It's one sound from one place, and the pan knob puts it where you want it. A stereo pair (Inputs 1+2) is "
                   "for sources that really are two channels: a keyboard's left and right outputs, or two microphones set up as a "
                   "pair. A single microphone recorded as stereo gives one silent side, or two identical ones if both inputs hear "
                   "it: twice the file for nothing.\n\n"
                   "Arm several tracks to record several inputs at once: a singer on Input 1 and a guitar on Input 2 become two "
                   "takes, mixed separately later. Two tracks on the same input record the same sound twice.\n\n"
                   "The picture shows your interface's inputs and which armed track takes each. DawIt opens the inputs only while an "
                   "audio track is armed; on a Mac the microphone dot in the menu bar goes away when you disarm.\n\n"
                   "The first time, macOS asks whether DawIt may use the microphone. If you said no, DawIt hears only silence: allow "
                   "it in System Settings > Privacy & Security > Microphone, then quit and reopen DawIt.";

        case Topic::level:
            return "The gain knob on the interface sets how loud the take is. Set it before recording, while you sing or play the "
                   "loudest part of the song: an armed track's meter shows the input.\n\n"
                   "A common aim is loudest peaks around -18 to -10 dB, and always under -6 dB. The meter marks -6 and 0 dB, as the "
                   "gain staging badges do.\n\n"
                   "Why not as loud as possible? The converter has a hard ceiling, 0 dBFS (full scale): the biggest number it can "
                   "write. Anything louder has the top of its wave cut flat, the right of the picture, and sounds like crackle and "
                   "buzz. Inside DawIt a track can go over 0 dB and be turned down again; at the converter it can't, and no effect "
                   "repairs it. Singers get louder in the chorus than in the sound check, so leave room.\n\n"
                   "Why not very quiet? A 24-bit file has about 144 dB between its smallest step and its top, so a take that peaks "
                   "at -30 dB has lost nothing you'd hear. But a quiet take is hard to see and judge, and effects' starting points "
                   "expect a normal level (the compressor's Lead vocal starting point, with its threshold at -20 dB, does nothing to a voice that never reaches it). "
                   "Below about -24 dB, turn the gain up.\n\n"
                   "Engineers differ on the exact aim. Some say peaks near -6 dB, Sound On Sound an average around -18 dB. At 24 bits "
                   "any of these is fine; what matters is staying clear of 0.\n\n"
                   "The level also changes with distance: twice as far from the microphone is about 6 dB quieter (The microphone, in "
                   "Singing techniques). So set the gain where you'll really stand.";

        case Topic::buffers:
            return "Latency is the delay between a sound going in and coming out again. The computer can't handle one sample at a "
                   "time, so it works in batches, the buffer. While it fills one batch from the input and plays the one before, time "
                   "passes.\n\n"
                   "One buffer lasts its size divided by the sample rate:\n"
                   "- 64 samples at 48 kHz: 1.3 ms\n"
                   "- 128 samples at 48 kHz: 2.7 ms\n"
                   "- 256 samples at 48 kHz: 5.3 ms\n"
                   "- 512 samples at 44.1 kHz: 11.6 ms\n"
                   "- 1024 samples at 48 kHz: 21.3 ms\n\n"
                   "A sound passes at least two buffers, one in and one out, plus a little for the converters and the driver. So the "
                   "round trip at 128 samples and 48 kHz is at least 5.3 ms, and interfaces measure about 6 to 12 ms there, depending "
                   "on the interface and its driver.\n\n"
                   "How much is too much? Sound travels about 34 cm in a millisecond, so 5.3 ms is like standing 1.8 m from a "
                   "speaker, which guitarists do all the time. Singers notice most, because they also hear their own voice inside "
                   "their head with no delay at all. Shure recommends 5 ms or less for singers' in-ear monitors, and a 2007 study "
                   "(Lester and Boley, AES paper 7198) found that what players accept ranges from about 42 ms down to under 1.4 ms, "
                   "depending on the instrument and how they listen.\n\n"
                   "Smaller isn't free. With a smaller buffer the computer has less time for each batch, and with many effects it runs "
                   "out: you hear clicks and dropouts. A common way of working is 64 or 128 samples while recording and 512 or 1024 "
                   "while mixing. Change it in Audio / MIDI Settings.\n\n"
                   "Latency matters for what you hear while you play. The take itself is put back in time afterwards (Why takes are "
                   "moved earlier).";

        case Topic::monitoring:
            return "Monitoring is hearing what you record while you record it. There are two ways, the two paths in the picture.\n\n"
                   "Direct monitoring: most interfaces can send the input straight to the headphones before it reaches the computer, "
                   "with a knob or button marked Direct Monitor or Mix. There's no delay you'd notice, so it's the best way for "
                   "singers. You hear yourself dry, without DawIt's effects.\n\n"
                   "Through DawIt: the Monitor button on an armed audio track plays the input through the track's effects and fader, "
                   "so you can hear your voice with its reverb. It comes back a round trip late (Your session shows how late), plus "
                   "whatever delay the track's effects add. Use one way, not both: with both on you hear yourself twice a few "
                   "milliseconds apart, which sounds hollow (What is sound?, Phase cancellation).\n\n"
                   "Use headphones, not speakers. A microphone that hears the speakers playing itself makes a loop, and once the loop "
                   "gives back as much as it gets, it howls: feedback. That's why Monitor is off whenever DawIt starts. Even without "
                   "monitoring, speakers leak the song and the metronome into the take; closed-back headphones keep them out.\n\n"
                   "What you hear isn't what's recorded: the take is always the input as it came in, without the effects you monitor "
                   "through.";

        case Topic::moved:
            return "You play along with what you hear, and what you hear left the computer a moment ago. What you play then takes a "
                   "moment to come back in. So a note played exactly on the beat arrives in DawIt late by the whole round trip: the "
                   "output's latency plus the input's.\n\n"
                   "DawIt moves each take earlier by that much, so it lands where you played it. For example, a device reporting "
                   "3.1 ms in and 3.3 ms out has a take moved 6.4 ms earlier. Uncorrected, 6.4 ms doesn't sound like an echo, but a "
                   "tight part feels a little behind the drums, and a second take played along with the first ends up later "
                   "still.\n\n"
                   "DawIt uses the latency the device reports. Some drivers leave part out, such as the converters' own delay. "
                   "Measure latency, in Audio / MIDI Settings, finds the real round trip: it plays eight clicks and listens for them "
                   "coming back. Connect an output to an input with a cable (the most exact), or put the microphone close to a "
                   "speaker, and set the level so the clicks are clear but not clipping. DawIt remembers the difference for that "
                   "interface and adds it to every take. Measure again when you change the interface or its sample rate; a buffer "
                   "change is already in the reported part.\n\n"
                   "Correction fixes the trip, not the playing: a note you played late stays late.";

        case Topic::takes:
            return "A take is one recording of a part. Few performances are right the first time, so record a few and keep the "
                   "best.\n\n"
                   "With the loop on (the loop range in the ruler), Record goes round and round, and every pass becomes its own take "
                   "at the same place on the track. The newest pass that reached the end of the loop plays; the older ones stay underneath, muted. "
                   "Right-click a take and choose Takes to hear another one: it plays and the others there are muted. Delete other "
                   "takes keeps only the one you chose. A pass you stopped half-way through is kept too, muted, unless it's the only one.\n\n"
                   "MIDI recording works differently: with the loop on, each pass adds its notes to the same clip, because notes can "
                   "be layered and deleted one by one. Audio can't.\n\n"
                   "Recording over a clip already on the track mutes the old clip instead of deleting it, so it becomes one more take "
                   "to go back to.\n\n"
                   "Comping, short for compiling, is the studio way of building one performance from the best parts of several "
                   "takes: the first line from take 2, the chorus from take 4. DawIt doesn't comp yet. Split the takes (right-click, "
                   "Split at playhead) and mute the parts you don't want, or choose a whole take.\n\n"
                   "A take with nothing in it, quieter than -60 dB from start to end, is removed, and DawIt says so. It usually means "
                   "the wrong input was chosen.";

        case Topic::yourSession:
            return "This topic reads your device and your recording settings as they are now: the interface and its sample rate, "
                   "the buffer, how late a sound is by the time it comes back, what is armed and how loud your last take was. Under it "
                   "is advice that appears only when something is worth changing.";
    }

    return {};
}

//==============================================================================
double getBufferMs (int bufferSamples, double sampleRate)
{
    return sampleRate > 0.0 ? 1000.0 * bufferSamples / sampleRate : 0.0;
}

double getMinimumRoundTripMs (int bufferSamples, double sampleRate)
{
    return 2.0 * getBufferMs (bufferSamples, sampleRate);
}

double getDistanceMetres (double ms)
{
    return ms * (double) soundguide::speedOfSound / 1000.0;
}

double getBitRangeDb (int bits)
{
    return 20.0 * std::log10 (std::pow (2.0, bits));
}

double getVoltsFromDbu (double dbu)
{
    return 0.775 * std::pow (10.0, dbu / 20.0);
}

double getVoltsFromDbv (double dbv)
{
    return std::pow (10.0, dbv / 20.0);
}

double getGainDb (double lowVolts, double highVolts)
{
    return 20.0 * std::log10 (highVolts / lowVolts);
}

double getBytesPerMinute (double sampleRate, int bits, int numChannels)
{
    return sampleRate * bits / 8.0 * numChannels * 60.0;
}

juce::String describeBytes (double bytes)
{
    return juce::String (bytes / 1.0e6, 2) + " MB";
}

//==============================================================================
std::vector<BufferRow> getBufferTable (double sampleRate, double extraMs)
{
    std::vector<BufferRow> rows;

    for (int size : { 64, 128, 256, 512, 1024 })
    {
        BufferRow row;
        row.samples = size;
        row.bufferMs = getBufferMs (size, sampleRate);
        row.roundTripMs = getMinimumRoundTripMs (size, sampleRate) + extraMs;
        rows.push_back (row);
    }

    return rows;
}

std::vector<SignalLevel> getSignalLevels()
{
    return { { "Microphone", 0.0025, true },
             { "Instrument", 0.0, false },
             { "Home line (-10 dBV)", getVoltsFromDbv (-10.0), true },
             { "Studio line (+4 dBu)", getVoltsFromDbu (4.0), true } };
}

std::vector<LevelExample> getLevelExamples()
{
    std::vector<LevelExample> examples;

    auto add = [&examples] (const juce::String& label, double peakDb, double amplitude, double drive, bool clipped)
    {
        LevelExample e;
        e.label = label;
        e.peakDb = peakDb;
        e.amplitude = amplitude;
        e.drive = drive;
        e.clipped = clipped;
        examples.push_back (e);
    };

    add ("Too quiet", -36.0, 0.0158, 1.0, false);
    add ("Good", -12.0, 0.251, 1.0, false);
    add ("Clipped", 0.0, 1.0, 2.0, true);
    return examples;
}

std::vector<float> getPhrase (const LevelExample& example, int numPoints)
{
    std::vector<float> wave;

    for (int i = 0; i < numPoints; ++i)
    {
        const double t = numPoints > 1 ? (double) i / (numPoints - 1) : 0.0;
        const double swell = std::sin (juce::MathConstants<double>::pi * t);          // a phrase rises and falls
        const double value = swell * std::sin (juce::MathConstants<double>::twoPi * 6.0 * t) * example.amplitude * example.drive;
        wave.push_back ((float) juce::jlimit (-1.0, 1.0, value));
    }

    return wave;
}

std::vector<TakeBar> getTakesExample()
{
    std::vector<TakeBar> takes (4);
    takes[0] = { 1, 10.0, 13.0, false, false };   // started in the middle of the loop
    takes[1] = { 2, 9.0, 13.0, false, false };
    takes[2] = { 3, 9.0, 13.0, false, true };     // the newest complete pass plays
    takes[3] = { 4, 9.0, 11.0, true, false };     // stopped at bar 11
    return takes;
}

//==============================================================================
juce::String describeInput (int firstChannel, bool stereo)
{
    return stereo ? "Inputs " + juce::String (firstChannel + 1) + "+" + juce::String (firstChannel + 2) + " (stereo)"
                  : "Input " + juce::String (firstChannel + 1) + " (mono)";
}

double getRoundTripMs (const Snapshot& s)
{
    return s.reportedInputMs + s.reportedOutputMs + (s.measured ? s.measuredExtraMs : 0.0);
}

juce::String describeLatency (const Snapshot& s)
{
    auto text = "Round trip " + oneDecimal (getRoundTripMs (s)) + " ms: " + oneDecimal (s.reportedInputMs) + " in, "
                + oneDecimal (s.reportedOutputMs) + " out, ";

    if (! s.measured)
        return text + "reported";

    return text + "measured, " + oneDecimal (std::abs (s.measuredExtraMs)) + " ms "
           + (s.measuredExtraMs < 0.0 ? "less" : "more") + " than reported";
}

juce::String describeSession (const Snapshot& s)
{
    juce::String text;
    const auto bufferMs = getBufferMs (s.bufferSamples, s.sampleRate);

    text << "Input: " << (s.inputDevice.isEmpty() ? juce::String ("none found") : s.inputDevice + ", "
                                                                                   + juce::String (s.numInputs) + (s.numInputs == 1 ? " input" : " inputs"))
         << ". Output: " << (s.outputDevice.isEmpty() ? juce::String ("none") : s.outputDevice) << ". "
         << juce::String (s.sampleRate / 1000.0, s.sampleRate / 1000.0 == std::floor (s.sampleRate / 1000.0) ? 0 : 1) << " kHz, buffer "
         << s.bufferSamples << " samples (" << oneDecimal (bufferMs) << " ms).\n";

    text << "Round trip " << oneDecimal (getRoundTripMs (s)) << " ms: " << oneDecimal (s.reportedInputMs) << " ms in, "
         << oneDecimal (s.reportedOutputMs) << " ms out, as the device reports";

    if (s.measured)
        text << "; " << oneDecimal (std::abs (s.measuredExtraMs)) << " ms " << (s.measuredExtraMs < 0.0 ? "less" : "more") << " as measured";

    text << ".\n";

    if (s.armed.empty())
    {
        text << "Armed: nothing. Arm an audio track with its R button.\n";
    }
    else
    {
        juce::StringArray parts;

        for (auto& a : s.armed)
            parts.add (a.name + " on " + describeInput (a.inputChannel, a.stereo) + ", Monitor " + (a.monitor ? "on" : "off"));

        text << "Armed: " << parts.joinIntoString ("; ") << ".\n";
    }

    text << "Last take: ";

    if (s.hasLastTake)
        text << s.lastTakeName << ", loudest peak " << oneDecimal (s.lastTakePeakDb) << " dB.";
    else
        text << "none yet this session.";

    return text;
}

juce::String getZeroInputMessage()
{
    return "DawIt hears nothing at all from the input. If macOS asked for the microphone and you said no, allow DawIt in "
           "System Settings > Privacy & Security > Microphone, then quit and reopen DawIt.";
}

std::vector<Advice> analyse (const Snapshot& s)
{
    std::vector<Advice> advice;
    const bool anyArmed = ! s.armed.empty();
    const double roundTrip = getRoundTripMs (s);

    auto add = [&advice] (const juce::String& text, Topic topic) { advice.push_back ({ text, topic }); };

    if (s.numInputs <= 0)
        add ("DawIt can't find an audio input. Plug in an audio interface, or choose an input device in Audio / MIDI Settings "
             "(the Mac's built-in microphone counts).", Topic::audioInterface);

    if (anyArmed && s.inputAllZero)
        add (getZeroInputMessage(), Topic::chooseInput);

    const bool monitoring = std::any_of (s.armed.begin(), s.armed.end(), [] (auto& a) { return a.monitor; });

    if (monitoring && roundTrip >= slowRoundTripMs)
        add ("You hear yourself " + oneDecimal (roundTrip) + " ms late through DawIt. Try a buffer of 128 or 64 samples, or turn "
             "Monitor off and use your interface's direct monitoring.", Topic::monitoring);

    if (anyArmed && s.bufferSamples >= largeBufferSamples)
        add ("A buffer of " + juce::String (s.bufferSamples) + " samples suits mixing. For recording, 128 or 64 makes playing and "
             "monitoring feel immediate; raise it again if you hear clicks.", Topic::buffers);

    if (s.hasLastTake)
    {
        const auto peak = s.lastTakePeakDb;
        const auto peakText = oneDecimal (peak);

        if (peak >= clippedPeakDb)
            add ("Your last take reached 0 dB: its loudest moments were cut off and can't be repaired. Turn the gain down and "
                 "record it again.", Topic::level);
        else if (peak >= closePeakDb)
            add ("Your last take peaked at " + peakText + " dB, close to the top. A little lower, peaks around -18 to -10 dB, leaves "
                 "room for a louder moment.", Topic::level);
        else if (peak >= quietPeakDb)
            add ("Your last take peaked at " + peakText + " dB: a good level.", Topic::level);
        else if (peak >= (double) gainstaging::silentBelowDb)
            add ("Your last take peaked at " + peakText + " dB: quiet but not lost, since 24 bits has room. Turn the gain up so "
                 "peaks reach about -18 to -10 dB; then the meters and effects' starting points match it.", Topic::level);
    }

    if (! s.measured && s.inputDevice.isNotEmpty())
        add ("Takes are timed by the latency your device reports (" + oneDecimal (s.reportedInputMs + s.reportedOutputMs)
             + " ms). Measure it once in Audio / MIDI Settings so takes land exactly in time.", Topic::moved);

    if (s.measured && std::abs (s.measuredExtraMs) >= measuredNoteMs)
        add ("Measuring found " + oneDecimal (std::abs (s.measuredExtraMs)) + " ms " + (s.measuredExtraMs < 0.0 ? "less" : "more")
             + " than the device reports; takes move by that too.", Topic::moved);

    return advice;
}
} // namespace daw::recordingguide
