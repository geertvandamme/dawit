#include "VocoderGuideText.h"

namespace daw::gen::vocoderguide
{
namespace
{
    int rotate (int mask, int by)
    {
        by = ((by % 12) + 12) % 12;
        return ((mask << by) | (mask >> (12 - by))) & 0xfff;
    }

    juce::String signedText (double value, int decimals)
    {
        return (value > 0.0 ? "+" : "") + juce::String (value, decimals);
    }
}

juce::String getControlTitle (Control control)
{
    switch (control)
    {
        case Control::bands:      return "Bands";
        case Control::smoothing:  return "Smoothing";
        case Control::consonants: return "Consonants";
        case Control::voice:      return "Voice";
        case Control::low:        return "Low";
        case Control::high:       return "High";
        case Control::source:     return "Source";
        case Control::wave:       return "Wave";
        case Control::root:       return "Root";
        case Control::shape:      return "Shape";
        case Control::range:      return "Range";
        case Control::carrier:    return "Carrier level";
        case Control::formant:    return "Formant";
        case Control::mix:        return "Mix";
    }

    return {};
}

juce::String getControlText (Control control)
{
    switch (control)
    {
        case Control::bands:      return "How many slices the voice is cut into. More bands give finer detail, but clear words also need a rich carrier.";
        case Control::smoothing:  return "How slowly each band's meter falls. Short is crisp and rough; long is smooth and blurry.";
        case Control::consonants: return "Adds noise to the top bands when the input is hissy, to help sounds such as s. It detects high-frequency energy, not individual consonants.";
        case Control::voice:      return "How much of your own voice is heard under the result.";
        case Control::low:        return "Where the lowest band starts. 100 Hz suits most voices; go lower for a deep voice or drums.";
        case Control::high:       return "Where the highest band ends. Start at 8,000 Hz; try higher if s sounds lose their brightness. The sample rate may limit the top frequency.";
        case Control::source:     return "Where the carrier's notes come from: the song's chord, notes you pick, or keys you play. If they are missing you hear your chosen notes.";
        case Control::wave:       return "The carrier's sound. Saw and Pulse are buzzy, Choir is a detuned group, Noise is a whisper.";
        case Control::root:       return "The note played by Notes, and whenever another source is missing.";
        case Control::shape:      return "The notes played by Notes, and whenever another source is missing: the root alone, with its fifth, or a major or minor chord.";
        case Control::range:      return "Adds octaves above the notes from Song chord or Notes. Keys uses the pitches you play; Noise has no pitched notes.";
        case Control::carrier:    return "The level of the shaped carrier, including consonant noise, before Voice and Mix are combined.";
        case Control::formant:    return "Moves the carrier's filter bands without changing its notes. +12 semitones doubles their centre frequencies; up is brighter, down is darker.";
        case Control::mix:        return "0% is your input only. 100% is the shaped carrier plus any dry voice added with Voice.";
    }

    return {};
}

juce::String formatHz (double hz)
{
    const int n = juce::roundToInt (hz);
    return n >= 1000 ? juce::String (n / 1000) + "," + juce::String (n % 1000).paddedLeft ('0', 3) + " Hz" : juce::String (n) + " Hz";
}

juce::String formatKnob (Control control, const vocoder::Settings& s)
{
    switch (control)
    {
        case Control::bands:      return juce::String (s.bands);
        case Control::smoothing:  return juce::String (juce::roundToInt (s.smoothingMs)) + " ms";
        case Control::consonants: return juce::String (juce::roundToInt (s.consonantsPercent)) + "%";
        case Control::voice:      return s.voiceDb <= vocoder::minVoiceDb + 0.05f ? juce::String ("off") : juce::String (s.voiceDb, 1) + " dB";
        case Control::low:        return formatHz (s.lowHz);
        case Control::high:       return formatHz (s.highHz);
        case Control::source:     return s.source == vocoder::Source::songChord ? "Song chord" : s.source == vocoder::Source::notes ? "Notes" : "Keys";
        case Control::wave:       return s.wave == vocoder::Wave::saw ? "Saw" : s.wave == vocoder::Wave::pulse ? "Pulse" : s.wave == vocoder::Wave::choir ? "Choir" : "Noise";
        case Control::root:       return vocoder::getPitchName (s.root);
        case Control::shape:      return s.shape == vocoder::Shape::note ? "Note" : s.shape == vocoder::Shape::fifth ? "Fifth" : s.shape == vocoder::Shape::major ? "Major" : "Minor";
        case Control::range:      return juce::String (s.range) + (s.range == 1 ? " octave" : " octaves");
        case Control::carrier:    return signedText (s.carrierDb, 1) + " dB";
        case Control::formant:    return signedText (s.formantSemitones, 1) + " semitones";
        case Control::mix:        return juce::String (juce::roundToInt (s.mixPercent)) + "%";
    }

    return {};
}

juce::String getLeadLine (vocoder::Source source, HostKind host, bool armedElsewhere, bool usingFallback)
{
    juce::String line;

    if (host == HostKind::instrument)
        line = "On a MIDI track the voice is the instrument's sound. ";
    else if (host == HostKind::bus || host == HostKind::returnTrack)
        line = juce::String ("On a ") + (host == HostKind::bus ? "bus" : "return") + " it vocodes the whole mix, so the voice is everything on it. ";

    if (source == vocoder::Source::keys)
        line << (armedElsewhere ? "Live keys go to the armed MIDI tracks; other tracks use their chosen notes."
                                : "For live keys, select an audio or MIDI track; with nothing held you hear your chosen notes.");
    else if (source == vocoder::Source::songChord && usingFallback)
        line << "No chord in this bar: playing your chosen notes.";

    return line.trim();
}

juce::String getChordName (int mask)
{
    mask &= 0xfff;

    if (mask == 0)
        return {};

    constexpr int major = 0x091, minor = 0x089, diminished = 0x049;

    for (int root = 0; root < 12; ++root)
    {
        const int shape = rotate (mask, -root);
        const auto name = vocoder::getPitchName (root);

        if (shape == major)      return name;
        if (shape == minor)      return name + "m";
        if (shape == diminished) return name + "dim";
    }

    juce::StringArray names;

    for (int pc = 0; pc < 12; ++pc)
        if ((mask & (1 << pc)) != 0)
            names.add (vocoder::getPitchName (pc));

    return names.joinIntoString (" ");
}

juce::String getCarrierLabel (vocoder::Source source, bool usingFallback, int carrierMask, int chordMask)
{
    const auto dot = juce::String::fromUTF8 (" \xc2\xb7 ");

    if (source == vocoder::Source::songChord)
        return usingFallback ? juce::String ("Notes (no chord in this bar)") : "Song chord" + dot + getChordName (chordMask);

    if (source == vocoder::Source::keys)
        return usingFallback ? juce::String ("Notes (no key held)") : "Keys" + dot + getChordName (carrierMask);

    return "Notes" + dot + getChordName (carrierMask);
}

juce::String describeBand (int band, double lowEdgeHz, double highEdgeHz, float voiceDb, float carrierDb)
{
    auto level = [] (float db) { return db <= -90.0f ? juce::String ("silent") : juce::String (juce::roundToInt (db)) + " dB"; };
    return "Band " + juce::String (band + 1) + juce::String::fromUTF8 (" \xc2\xb7 ") + formatHz (lowEdgeHz).upToLastOccurrenceOf (" Hz", false, false)
           + " to " + formatHz (highEdgeHz) + juce::String::fromUTF8 (" \xc2\xb7 ") + "voice " + level (voiceDb) + juce::String::fromUTF8 (" \xc2\xb7 ") + "carrier " + level (carrierDb);
}

namespace
{
    const char* const pageNames[numPages][2] = {
        { "What a vocoder does", "Your voice shapes the colour of a synth chord." },
        { "Modulator and carrier", "Which sound says the words and which one plays the notes." },
        { "Bands", "How the voice is cut into slices and measured." },
        { "Why the carrier needs harmonics", "A sine has nothing for most bands to shape." },
        { "How many bands", "More bands give finer detail; intelligibility also depends on the carrier." },
        { "Consonants", "Hissy sounds need energy in the carrier's top bands." },
        { "Playing the carrier", "Song chord, chosen notes and keys, and what happens when one is missing." },
        { "Vocoder or pitch correction?", "The carrier supplies the notes; your sung pitch is not the target." },
        { "Using it well", "Give the bands a clear voice and a rich carrier." },
        { "Your track", "What the vocoder made of this track in the last 8 seconds." } };

    const char* const pageTexts[numPages] = {
        R"voc(A vocoder makes one sound speak with the words of another. You talk or sing into it. It measures how loud your voice is in many narrow slices of the spectrum, all the time, and uses those loudness values to shape a second sound, usually a synth chord. What you hear has the words of your voice and the notes of the synth. Say "ah" while the synth plays C, E and G and the chord opens like a mouth; say "s" and it hisses.

Homer Dudley at Bell Labs began the idea in 1928 to send speech through less bandwidth, by describing the voice rather than sending its whole waveform. In music the band levels shape another sound. In DawIt Vocoder your track is the voice, and a carrier made inside the effect plays the notes.

The example pictures are simplified illustrations, not measurements of the listening examples. The three-strip example uses a saw playing C major over two octaves. Your track shows live meters when input has been heard.)voc",
        R"voc(The modulator is the sound whose movement you keep: your voice, or a drum loop. The carrier is the sound that gets shaped: a synth chord, or noise. The result has the carrier's pitch and the modulator's movement.

In DawIt Vocoder the modulator is the track the effect sits on. On a MIDI track that means the instrument's sound; on a bus or return it means the mix arriving there. The left and right input channels are averaged for analysis, so opposite-polarity sounds can cancel. The carrier is made inside, so you can play it from the song's chords, from notes you pick, or from a keyboard. What the vocoder keeps of your voice is its rhythm and broad spectral shape. It does not track your sung pitch to set the carrier's notes.)voc",
        R"voc(The vocoder cuts the voice into bands, spaced so that each is the same musical width. With 16 bands from 100 to 8,000 Hz each band is about 4.7 semitones wide, a ratio of 1.32: the lowest band is 100 to 131 Hz and the highest 6,083 to 8,000 Hz. Each band has a band-pass filter and an envelope follower, a meter that rises fast when the band gets louder and falls at the speed of Smoothing. The same filters are applied to the carrier, and each one is turned up and down by its meter. The sum is the result. Vowels are colours made of peaks called formants: in a typical man's voice "ah" has peaks near 730 and 1,090 Hz and "ee" near 270 and 2,290 Hz. The bands see those peaks and hand them to the carrier.

Low and High set where the bands start and end. The default is 100 to 8,000 Hz. Conventional telephone speech uses about 300 to 3,400 Hz and remains understandable, but that does not guarantee clarity through a synth carrier. A narrow range gives finer bands: 24 bands from 400 to 4,000 Hz are 1.7 semitones each. Low ranges from 50 to 400 Hz and High from 4,000 to 12,000 Hz. Try a lower Low for a deep voice or drums, or a higher High for bright s sounds. High is limited to 0.45 times the sample rate: at 22,050 Hz it cannot exceed 9,922.5 Hz. A wider range with the same number of bands makes each band wider.

Smoothing sets the envelope's release time, from 5 to 100 ms; the default is 15 ms. Short values keep quick changes but can sound grainy; long values blur syllables. Formant moves the carrier's filter centres by up to an octave either way without changing its notes: +12 semitones doubles the centres. At high frequencies the sample-rate limit also restricts this shift.)voc",
        R"voc(A band can only turn up what the carrier has in it. A sine wave at 110 Hz has energy at 110 Hz only. The band around 110 Hz responds most strongly, with some response in neighbouring filters because they overlap. It cannot reproduce the voice's higher formants, so you mainly hear a hum. An ideal sawtooth has a harmonic at every multiple of its pitch, each with 1/n of the first harmonic's amplitude (the 2nd is 6 dB quieter, the 10th 20 dB quieter). A digital saw limits its harmonics below the sample rate's upper frequency limit. White noise has energy across the spectrum and no fixed pitch, which is why a noise carrier gives a whisper.

Even a saw leaves gaps: with a 110 Hz saw and 16 bands at the default range, bands below about 350 Hz are narrower than the 110 Hz between harmonics, so some hold none. That is why low bands can sound patchy, and why a chord of several saws fills the gaps better than one note.

The listening buttons use the window's source and notes, not necessarily a single 110 Hz note. For a single sine example, choose Notes with Shape set to Note, Range 1 and Consonants 0%. Added consonant noise would otherwise supply extra high frequencies.)voc",
        R"voc(More bands give finer spectral detail, but do not guarantee clearer words. In 1995 Shannon and colleagues found high recognition of words in simple sentences with only three or four noise bands, after listeners had practised. Those bands and their spacing differed from DawIt's, and noise fills bands more evenly than a synth. The result shows how much speech information can survive coarse filtering; it is not a promise that four bands here will make every word understandable. Apple's EVOC manual suggests about 10 to 15 bands as a compromise between detail and processing cost, rather than a universal intelligibility threshold.

In DawIt Vocoder, 4 bands (edges at 100, 299, 894, 2,675 and 8,000 Hz at the default range) give a coarse, robotic sound. Try 8 or 12 for more detail, or the default 16. With 24 the bands are narrowest: a rich carrier matters because some bands may fall between its harmonics. Compare the listening buttons with your own material.)voc",
        R"voc(Vowels usually have a pitched source. An s is made by turbulent air and has strong high-frequency noise; some speakers have prominent peaks near 8 to 10 kHz, but there is no fixed range for every voice. The sh sound is typically lower, f has weaker, more spread-out noise, and t has a brief burst rather than a sustained hiss. A synth carrier may have too little energy in these regions, so consonants can blur.

Consonants adds noise to bands whose upper edges exceed 4 kHz; the top band is always available, including when High is exactly 4,000 Hz. The detector compares energy above 4 kHz with energy below 1.5 kHz. It detects hiss, not words: background noise or a bright instrument can trigger it too. Start near 40 to 50%. At 100% it can sound harsh; at 0% consonants may lose definition unless the carrier already has enough high-frequency energy. Other vocoders can mix in the voice's own top end instead.

The picture illustrates extra energy in the top band, not a measured dB improvement. Its curves do not follow all the controls. Use Say "ah ee s" to hear Consonants with the current settings.)voc",
        R"voc(Song chord plays the chord of the current bar, placed from C3 upwards: C major is C3 130.8 Hz, E3 164.8 Hz and G3 196.0 Hz, and Range adds octaves above. The song gives only the notes of the chord, not which one is lowest, so inversions are not followed.

Notes plays the root and shape you choose: one note, a fifth, or a major or minor chord. Range adds one or two octaves above these notes or the song chord. Keys uses the pitches you actually play, up to 16 held notes, without adding octaves. Noise has no pitched notes, whatever the source label says.

For Keys, select the track first. While MIDI tracks are armed, live keys go to those tracks instead; arming an audio track does not redirect MIDI. Buses and returns receive no live keys, so Keys uses Notes there. On an audio track the carrier notes cannot be recorded. On a MIDI track, MIDI clips and live keys also reach the instrument, so the voice input is its sound. A bounce has no live keys, but MIDI clips on a MIDI track can still supply notes if preceding plugins pass the MIDI through. Another track's sound cannot be the carrier, and another MIDI track's clips cannot supply its notes.

Whatever is missing (a bar with no chord, no key held), you hear your chosen Notes instead of silence.)voc",
        R"voc(Pitch correction, such as DawIt Tune, processes your original voice and moves its pitch towards an allowed target note. DawIt Vocoder uses the band levels to shape a separate carrier. It does not track your sung pitch to choose its notes: speak on one pitch and a synth chord still plays its own notes. Your input pitch can still affect the band levels and their fluctuations. Voice or Mix can also let the original voice through. A harmoniser, such as DawIt Harmoniser, adds copies of your voice at other pitches.

A talk box makes a related sound by sending an instrument through a tube into your mouth, where your mouth shapes it. Two other processes share the vocoder name: a phase vocoder uses frequency analysis for tasks such as time stretching and pitch shifting; the neural vocoder in Sing this turns a spectrogram into sound. They work differently from this filter-bank effect.)voc",
        R"voc(The bands follow whatever is loudest, so use a close, dry, steady voice. Many engineers put a compressor before the vocoder so quiet syllables still move the bands, de-ess lightly if the "s" is piercing, and put reverb or delay after it, because the bands would measure the echoes; others vocode first and compress after, so take this as a start, not a rule.

Song chord follows the chord track, including any notes outside the key; a missing chord uses Notes instead. Check that the chosen notes suit the song. The result may be quieter than the voice: use Carrier level to adjust it. This control scales the shaped carrier and added consonant noise, before the dry voice is mixed in. A soft limiter keeps the mixed output within full scale when the effect is mixed in; Mix at 0% passes the input unchanged.

Mix below 100% blends in the original input; try 30 to 50% if you want recognisable natural voice too. Voice adds dry input inside the effect's wet path, so even Mix at 100% contains the original voice when Voice is raised. The band pictures show the shaped carrier before Voice, Mix and the limiter, so their levels need not match the final output.)voc",
        R"voc(The advice summarises roughly the last 8 seconds. When input has been heard, the bands show the current voice, carrier and shaped result, not an 8-second average. The result bars exclude the dry Voice contribution, Mix and the final limiter. The carrier bars include added consonant noise.

With no recent input above the display threshold, the picture shows a simplified illustration of the made-up phrase "ah ee s", with a saw playing C major over two octaves. It is not your track or a prediction of the current Wave, Source, Formant or Consonants settings. Open this guide from the effect on your track for live advice. When opened on no track, it uses a separate example processor.)voc" };
}

juce::String getPageName (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (pageNames[page][0]) : juce::String(); }

juce::String getPageSummary (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (pageNames[page][1]) : juce::String(); }

juce::String getPageText (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (pageTexts[page]) : juce::String(); }
}
