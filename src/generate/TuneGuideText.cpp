#include "TuneGuideText.h"

namespace daw::gen::tuneguide
{
namespace
{
    const char* const names[numPages][2] = {
        { "What pitch correction is", "Measuring each sung note, and moving it when asked" },
        { "Cents: how far out is out", "100 cents to a semitone, and what ears notice" },
        { "Finding the pitch", "How DawIt Tune hears which note you sing" },
        { "Target notes", "Key, chords or every semitone" },
        { "Speed, amount and tolerance", "How fast and how far notes are moved" },
        { "Vibrato and the human touch", "Moving the middle of the note, not the swing" },
        { "Formants", "Moving the pitch without moving the voice's colour" },
        { "Hard tune", "When the steps are the point" },
        { "Using it well", "Check first, fix lightly, put it first" },
        { "Your track", "How in tune this track is, from what it heard" } };

    const char* const texts[numPages] = {
        R"tune(Pitch correction listens to one voice or instrument, finds the pitch of each note as it's sung, and can move it to the note it should be. DawIt Tune does both. In Check mode it only measures: the note, how many cents it's sharp or flat, and whether it belongs to the song's key. In Correct mode it also moves each note towards its target.

The picture shows a short line sung in A minor: E4, G4 and A4. The sung line is green where it's within 10 cents of a note, yellow within 25 and red beyond; the shaded rows are the notes of the key; the blue line is what DawIt Tune makes of it with the Natural fix starting point. The E starts 150 cents low, nearer to D than to E, and slides up. DawIt Tune can't know it's meant to be an E, so at first it holds the start towards D; once the slide is past the halfway point it moves to E, and the note settles there. The G sits 35 cents flat and is pulled up to G, with its vibrato kept. The A starts right and drifts 30 cents flat; the correction follows it back up.

A note can be wrong in two ways. It can be a little out, sharp or flat by some cents, on the right note. Or it can be the wrong note. Pitch correction fixes the first. It can't know which note you meant: it moves each note to the nearest target, so a note sung nearer the wrong note ends up on the wrong note, in tune.

Light correction is common in today's pop production, often so light that nobody notices it. Used hard, it becomes a sound of its own (Hard tune). It's no substitute for singing in tune: a weak take that's been corrected still sounds weak. The Singing techniques guide's Singing in tune shows how to practise.)tune",
        R"tune(A semitone, one key on the piano, is split into 100 cents; an octave is 1,200. Cents measure a ratio, not a number of hertz, so a cent is the same small step at any pitch. A4 is 440 Hz. 30 cents below it is 432.4 Hz and 20 cents below it 434.9 Hz. At A2, 110 Hz, the same 20 cents is only 1.3 Hz. That's why tuners count in cents.

How much can you hear? Played one after the other, two steady tones a few cents apart, roughly 3 to 10 depending on the pitch, the loudness and the listener, can be told apart. In a song it's different: a sung note usually has to be about 20 to 25 cents off before many listeners hear it as out of tune. Trained musicians notice smaller differences, and people differ a lot. Short, quick notes are heard less exactly than long held ones.

DawIt Tune colours notes with that in mind: green within 10 cents of a note (in tune), yellow within 25 (close: some listeners will hear it on a long note), red beyond 25 (out). These bands are DawIt's choice, not a standard.

A sung note never sits on one exact number. Even a good singer's pitch moves by a few cents, slides into notes and swings with vibrato. What counts is where the note settles, its middle, and whether it stays there on a long note.)tune",
        R"tune(A sung vowel repeats itself: the vocal folds open and close, and each cycle of the wave looks almost like the one before. The time one cycle takes is the period, and the pitch is how many periods fit in a second. A3, 220 Hz, repeats every 4.5 milliseconds.

DawIt Tune finds the period by comparing the sound with itself a moment later, for every delay from 0.5 ms (2,000 Hz) to 20 ms (50 Hz). When the delay is exactly one period, the two line up and the match is nearly perfect. The picture shows the match for a sung A3: a peak at 4.5 ms, and more at 9.1 and 13.6 ms, two and three periods. This is McLeod's method (2005), the one DawIt also uses for the pitch in the wave editor. It looks at about 40 milliseconds at a time and gives a new answer about every 5 ms.

Sounds without a cycle have no pitch. The s of "sun", f, sh and t are noise, made by air rushing past the tongue and teeth: the match never peaks, so DawIt Tune lets them through untouched. The same goes for breaths and silence.

Three things make pitch hard to find:
- Octave mistakes. A voice can look as if it repeats every two periods, an octave low, or, when its lowest harmonic is weak, every half period, an octave high. DawIt Tune compares each answer with the ones before it, and ignores a sudden octave jump that lasts only a moment.
- Chords and harmonies. Two notes at once don't make one repeating cycle, and DawIt Tune shows "no clear pitch". It follows one voice at a time.
- Reverb, delay and other sounds in the recording blur the cycle. Put DawIt Tune on the dry vocal, before reverb.)tune",
        R"tune(In Correct mode every note is moved towards a target: the nearest note DawIt Tune is allowed to use. Target chooses which notes those are.

Key: the notes of the song's key, set in the transport bar. In C major those are C, D, E, F, G, A and B. A note sung between two of them goes to the nearer one: an F# in C major is pulled down to F when it's sung a little flat, and up to G when it's a little sharp. This is the usual choice: a note that drifts can never be moved to a note outside the key.

Chords: only the notes of the chord in that bar on the chord track, such as A, C and E under Am. That's stricter: a passing note between chord notes is pulled onto one of them. Use it for long held notes over clear chords, not for a busy melody. Bars without a chord use the key.

Chromatic: every semitone. Nothing is moved by more than half a semitone, and notes from outside the key stay what they are. Use it when the song borrows notes from outside its key, or when you're not sure of the key.

Halfway between two targets is the point where a note changes target. A note that hovers right on it would flip between the two, so DawIt Tune keeps a note on its target until the pitch is 15 cents past the halfway point.

Mind the key's own notes. A minor's natural scale has G, but many songs in A minor sing G#, the raised seventh. With the key set to A minor, a sung G# is pulled to G or A. Set the key to A harmonic minor, or choose Chromatic, and it stays a G#.)tune",
        R"tune(Speed is how quickly a note is moved to its target, in milliseconds: the time it takes to get 95% of the way there. At 0 ms the move is instant. At 50 ms a note that starts low is still heard sliding up, then settles on the note. At 200 ms only long notes get there at all, and quick notes pass almost as they were sung. The picture shows the same scooped E corrected at 0, 50 and 200 ms. The scoop starts nearer to D, so at first it's held towards D, and it moves to E once it's past the halfway point: at 0 ms that's a jump.

Fast speeds sound more corrected. A real voice glides between notes and wavers a little; take that away and it sounds mechanical. Slow speeds sound natural but leave short notes as they were. With Antares Auto-Tune, the best-known tuner, many engineers start a natural fix somewhere between 10 and 50 ms and adjust by ear. Each tuner measures speed in its own way, DawIt Tune included, so the same number can sound different from one to the next.

Amount is how much of the error is removed. At 100% the note goes all the way to its target. At 60% a note 30 cents flat ends up 12 cents flat: closer, and still human.

Tolerance is a band around each target, in cents, where nothing is touched. With 10 cents, a note 8 cents flat is left exactly as it was, and a note 30 cents flat is corrected.

Start gentle, with Natural fix or Gentle polish, and listen with Hear the original on and off. If you can hear the correction working on a natural vocal, slow it down or lower the amount.)tune",
        R"tune(Vibrato is a regular swing in pitch around a held note, about 5 to 7 times a second; in classical singing it typically swings 50 to 75 cents above and below. The ear hears one note, at the middle of the swing. So a note with vibrato isn't out of tune: only its middle can be.

A fast correction that follows every moment of the pitch fights the vibrato and flattens it into a straight, buzzy tone. Keep vibrato stops that. With it on, DawIt Tune first smooths the pitch to find the note's middle, letting through only changes slower than about one and a half times a second. A 5.5 Hz vibrato keeps less than a tenth of its swing in that smoothed line. Then it moves the middle to the target, and the swing rides on top, unchanged. The picture shows a note 30 cents flat with a 40-cent vibrato: with Keep vibrato on, the middle moves to the note and the swing stays; off, at 0 ms, the swing is gone.

The same goes for the other things that make a voice sound alive: a scoop into a note, a fall at its end, a bend on an emotional word. They are style, not mistakes. A slower speed and a tolerance leave them in; Hard tune takes them out on purpose.

Finding the middle takes a moment, so with Keep vibrato on the correction of a new note starts a little later. DawIt Tune starts afresh at each new note to keep that short.)tune",
        R"tune(A voice has two parts that a tuner must keep apart. The pitch is how fast the vocal folds buzz. The formants are the resonances of the throat and mouth: they make an "ah" sound like "ah", and your voice sound like you. An "ah" has its first two formants near 730 and 1,090 Hz, whatever note it's sung on.

The simplest way to raise a pitch is to play the sound faster, like a tape at a higher speed. That moves everything up together, formants too. Up a semitone, the "ah" formant at 730 Hz moves to 773 Hz; up an octave, to 1,460 Hz, and a voice turns into a chipmunk. The picture shows an octave, so the move is easy to see.

DawIt Tune keeps the formants where they are. It cuts the voice into small overlapping pieces, each about two cycles of the folds long, and lays them closer together to raise the pitch or further apart to lower it. Each piece keeps the shape of its cycle, and with it the colour of the vowel; only how often the pieces repeat changes. This is called PSOLA (pitch-synchronous overlap-add, Moulines and Charpentier, 1990). It works best on one voice and on small moves, which is what pitch correction makes: usually less than a semitone.

DawIt Tune has no formant control. Moving the formants on purpose, for a deeper voice or a cartoon one, is a job for a different effect.)tune",
        R"tune(Set Speed to 0 ms and Amount to 100% and the voice jumps from note to note with no glide in between. Vibrato vanishes, slides turn into steps, and the voice gets a synthetic, warbling edge where it changes note. It's the sound of Cher's Believe (1998), often called the first big hit to use it on purpose, and of a lot of pop, hip-hop and R&B since.

It came from a correction tool. Antares Auto-Tune, made by the engineer Andy Hildebrand, came out in 1997 to fix notes quietly. The producers of Believe turned its speed to zero and made the steps the hook.

For the sound: use Key rather than Chromatic, so the steps land on the key's notes. Sing clear notes and move between them decisively; the warble is strongest on slides between notes, so slide on purpose where you want it. Hard tune shows every wrong note plainly: a note sung nearer the wrong target jumps there completely.

It's a style. Some listeners love it and some can't stand it, and on a quiet ballad it can pull attention away from the words. Choose it; don't fall into it.)tune",
        R"tune(Pitch correction works best as a light last step on a good take, not as a cure.

1. Check first. Put DawIt Tune on the vocal in Check mode and play the song. Your track lists where the notes are out. A few notes out is normal. A whole section flat often means tiredness, a key that doesn't suit your voice, or a headphone mix where you can't hear yourself well.
2. Fix what singing can fix. Another take, or a key that suits your voice (Singing techniques: Your voice and your song), beats correcting a strained one.
3. Put it first in the chain, on the dry vocal: before EQ, compression and above all reverb and delay, which blur the cycles DawIt Tune listens to. Antares gives the same advice for Auto-Tune.
4. Fix lightly. Try Natural fix or Gentle polish, then compare with Hear the original. Stop when a held note sounds right; you don't need every note at 0 cents.
5. Mind the wrong notes. Correction moves a note to the nearest target, not to the one you meant. A note sung nearer the wrong note needs another take, or Chromatic and a careful ear.
6. One voice at a time: put it on each vocal track, not on a bus of harmonies.

DawIt Tune has to hear a little ahead, so inside it the track is about 27 milliseconds late (29 at a 44.1 kHz sample rate). On an audio track DawIt plays the clips the same amount earlier, so the track stays in time with the rest. On an instrument track, a bus or a return it can't do that, and everything on it plays late: use DawIt Tune on the vocal's audio track.

Very low notes, below about A2 (110 Hz), are measured but not moved: one cycle is too long for the short time DawIt Tune hears ahead. That only touches the lowest notes of a bass or baritone voice.)tune",
        R"tune(DawIt Tune listens while the song plays and keeps what it heard: each note, the nearest note to it, and how many cents it was off. This page sums that up per section. It only knows what has played through it, so play the song through once; playing a part again replaces what it heard there. What it heard isn't saved with the project. Notes are measured against the nearest semitone, so a note outside the key counts as a different note, not as out of tune.)tune" };
}

juce::String getPageName (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (names[page][0]) : juce::String(); }
juce::String getPageSummary (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (names[page][1]) : juce::String(); }
juce::String getPageText (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (texts[page]) : juce::String(); }
}
