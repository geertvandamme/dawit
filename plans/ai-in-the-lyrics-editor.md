# AI in the lyrics editor: replace a selection, or write more

Status: Built, not committed · Theory checked, with open points
Idea: [AI in the lyrics editor: replace a selection, or write more](../FEATURE_IDEAS.html#ai-in-the-lyrics-editor-replace-a-selection-or-write-more)

## Goal
The Lyrics window (Edit... in the Lyrics header, or a double-click on the row) gets a *Write with AI* panel beside the text. It writes the whole song for the song's sections, adds or rewrites a section, writes the next line, rewrites a selection, and finds rhymes for a word. It uses the user's local Ollama model, so nothing leaves the computer. DawIt checks every suggestion's syllables and rhymes with the pronouncing dictionary, because language models count syllables badly. Two new topics in the lyrics guide explain rhymes from strong to loose, and what the AI is good and bad at. A beginner gets unstuck without handing the song over: they keep the lines they like and ask for help with the rest.

## Decisions for you
1. **Where the AI lives.** Options: a panel in the Lyrics window; a separate window. **Recommended:** the panel, because the writer works on the text and the suggestions side by side, and *Use* edits the text they are looking at.
   Answer: The panel (recommended).
2. **Make the Lyrics window non-modal.** Right now it is a modal dialog, so a guide opened from it can't be clicked. Options: non-modal through `PluginWindowManager` (the song keeps playing and the guide works); keep it modal and open no guide from it. **Recommended:** non-modal. If the lyrics change outside the window while it's open, a note says so (see *What the user does and sees*).
   Answer: Non-modal through PluginWindowManager.
3. **The song brief is saved in the project.** This is what the song is about, its mood and its point of view. Options: saved in the project; kept only while the window is open. **Recommended:** the project, because the brief belongs to the song and is reused by every request.
   Answer: Saved in the project.
4. **Where rhymes come from.** The CMU dictionary has about 126,000 words, many of them names and rare words, and it has no word frequencies. Options: (a) dictionary rhymes, with the AI picking the ones that suit the line; (b) add a word-frequency list as a new dependency; (c) the AI only. **Recommended:** (a), with DawIt labelling every rhyme the AI suggests by its sound. It works without Ollama too (the dictionary list alone).
   Answer: (a) Dictionary rhymes, with the AI picking the ones that suit the line.
5. **Syllables are counted by sound everywhere.** Hyphenated words count their parts. Any other word gets the dictionary's vowel count, and failing that the spelling split. *Your lyrics* now counts an unhyphenated word as one syllable. Options: *Your lyrics* uses the new count too; it keeps counting places in time. **Recommended:** the new count, so the guide and the panel agree ("lonely" is 2).
   Answer: The new count in *Your lyrics* too.
6. **Lines the AI writes are hyphenated** (`to-ge-ther`) when the spelling split agrees with the dictionary's count, as detected lyrics are. Otherwise the word stays whole. Options: hyphenate; plain words. **Recommended:** hyphenate, so each syllable gets its own place in time.
   Answer: Hyphenate.
7. **How many alternatives.** **Recommended:** 3 for a line or a selection, 2 for a section, 1 for the whole song, with *Try again* for more. Longer answers take a 7B model on a laptop tens of seconds.
   Answer: 3 for a line or selection, 2 for a section, 1 for the whole song.
8. **The whole song without sections.** Options: write a default form (Verse, Chorus, Verse, Chorus, Bridge, Chorus, 4 lines each) and say so; refuse and ask for sections first. **Recommended:** the default form, with the note "No sections yet: written as verse, chorus, verse, chorus, bridge, chorus".
   Answer: The default form, with the note.
9. **The ownership and memorised-lyrics paragraphs** in *Writing with AI* (US Copyright Office, the Munich court). Options: include them, carefully worded and not legal advice; leave them out. **Recommended:** include them, because a beginner releasing AI-assisted lyrics needs to know both.
   Answer: Include them, carefully worded, not legal advice.

## What the user does and sees
The Lyrics window grows from 560 × 600 to 980 × 660 (resizable, at least 860 × 520). It opens through `PluginWindowManager::show`, keyed by the lyrics lane, and comes to the front when opened again.

```
┌ Lyrics ─────────────────────────────────────────────────────────────────────┐
│ hint (as now)                                                               │
│ ┌ text ─────────────────────────┐ ┌ Write with AI ────────────────── [?] ┐ │
│ │ [Verse]                       │ │ Model [qwen2.5:7b ▾] [⟳]             │ │
│ │ I walked a-long the ri-ver late│ │ About [a night walk after a break-up]│ │
│ │ the ci-ty lights be-hind me   │ │ Mood [Longing ▾]  Point of view [I ▾]│ │
│ │ ▒I feel so sad and lone-ly now▒│ │ [Write the song] [Section ▾]         │ │
│ │ the way it used to find me    │ │ [Rewrite selection ⌘R] [Rhymes ⇧⌘R]  │ │
│ │                               │ │ Ask: [More concrete][Simpler][Keep   │ │
│ │                               │ │  syllables✓][Keep the rhyme✓][...]   │ │
│ │                               │ │ Your words: [make it about the sea ] │ │
│ │                               │ │ ── Line 3 of Verse · 8 syllables ──  │ │
│ │                               │ │ 1 your coat still hangs be-side ...  │ │
│ │                               │ │   8 syllables, like the original     │ │
│ │                               │ │   a picture instead of a feeling     │ │
│ │                               │ │ 2 ...                    [Use] [Try  │ │
│ │                               │ │ status / Ollama note        again]   │ │
│ └───────────────────────────────┘ └──────────────────────────────────────┘ │
│ [x] Keep the timing of the words that didn't change          [Apply][Cancel]│
└─────────────────────────────────────────────────────────────────────────────┘
```

- **The panel** is 360 px wide on the right. A *Write with AI* arrow above it folds it away, which brings back the old width (`view.lyricsAiOpen`). The model box and ⟳ share the piano roll generator's settings (`generator.model`, `generator.ollamaUrl`).
- **The brief:** *About* (free text), *Mood* (Any, Hopeful, Happy, Sad, Longing, Angry, Calm, Defiant, Playful, Dark) and *Point of view* (Any, I, You, We, A story: he, she, they). It is saved with the song.
- **The actions** follow the cursor and the selection:
  - *Write the song* writes every sung section of the Sections row: a verse for each verse section, with new words each time; the chorus once, under `[Chorus]`; the pre-chorus and the bridge. Nothing goes on intros, solos, drops or outros. Each section gets bars ÷ 2 lines, from 2 to 8. If the box has text, *Use* first asks "Replace all the lyrics?".
  - *Section ▾* is a menu:
    - *Rewrite [Verse 2]*, for the section the cursor is in;
    - *Write the next section without words: Chorus*, from the Sections row;
    - *Add a verse / pre-chorus / chorus / bridge / outro here*, at the cursor, under its heading;
    - and last, *How lyrics are written...*.
  - *Rewrite selection* (Cmd+R) rewrites the selected words or lines. With nothing selected, the button reads *Write the next line* and continues after the cursor's line.
  - *Rhymes* (Shift+Cmd+R) finds rhymes for the selected word, or the word at the cursor.
- **Ask** offers toggle chips: More concrete, Simpler words, Keep the syllables (on), Keep the rhyme (on), New words, Another angle, Shorter, Longer. A *Your words* box takes free text ("make it about the sea").
- **The results:**
  - A heading names the target ("Line 3 of Verse · 8 syllables").
  - The text being replaced is highlighted while the panel shows results for it.
  - Each alternative shows its lines, its syllables against the original's and the rhyme it lands on (*The "your song" rules*), plus the model's few words on what it changed.
  - Click an alternative to select it; double-click it or press *Use* to put it in place.
  - *Try again* asks again with a new seed. *Cancel* stops a running request.
  - For *Write the song*, the result is the whole text with a line per section ("Verse 1: 4 lines, 7 to 9 syllables").
- **Rhymes** shows two lists:
  - the AI's picks, each labelled by DawIt (perfect, family, near, guessed);
  - *From the dictionary*: perfect rhymes grouped by syllables, then family rhymes.
  Double-click a rhyme (or *Use*) to replace the word. Without Ollama, only the dictionary list shows.
- **Use** replaces exactly the range the request was made for, through the text box, so Cmd+Z takes it back. If the text in that range changed since the request, *Use* is off and the note says "The text changed here: ask again". Nothing reaches the song until *Apply*, which works as now (`lyrics::setText` keeps the timing).
- **The text box's right-click menu** keeps cut, copy and paste, then lists *Rewrite with AI...*, *Write the next line...*, *Rhymes for "night"...* and, last, *How lyrics are written...*.
- **The ? button** opens the lyrics guide at *Writing with AI*. A ? next to the rhyme lists opens *Finding a rhyme*.
- **Notes in the status line:**
  - Ollama isn't reachable: the client's message ("Can't reach Ollama at ... Start the Ollama app (or run "ollama serve") and try again.");
  - no model: "No chat models installed. In a terminal, run: ollama pull qwen2.5:7b";
  - a running request: a spinner and the characters received;
  - the lyrics changed outside the window: "The lyrics changed outside this window. Apply replaces them with this text."
- **Tooltips** go on every control (see *Theory*, tooltips).

## Data and saving
- **The song brief** is new song data, held by `Engine` behind a getter and a setter (recipe *Song data that's saved*, copying the lyrics). It holds the about text, the mood and the point of view, and it is cleared in `resetEdit()`.
- `ProjectFile.cpp` writes it as an optional `lyricsBrief` object and leaves it out when it is empty. A project without it loads with an empty brief. No format version change: an older DawIt ignores the key.
- Test: a save-and-load round trip, and a project without the key loads empty.
- **App settings:** `view.lyricsAiOpen` (default true). The quick choices are remembered under `lyrics.aiAsk`. `generator.model` is shared.

## The guide
New topics in the lyrics guide (recipe *Topics in an existing guide*). Both use the `words` example kind, so no new picture code is needed beyond the colours.
- **Finding a rhyme** (after *Rhyme*): "Perfect, family, assonance: from strong to loose". The picture has a row per kind of rhyme for "night", each word coloured by the kind's rhyme colour, with a note: perfect, identical, family, additive, assonance, consonance, two syllables.
- **Writing with AI** (after *Writing tips*): "What the model does well, and what you still decide". The picture is the XAXA verse with line 3 struck as the original, and three alternatives with their labels.
- **Fix the *Rhyme* topic:** a perfect rhyme matches the *last stressed* vowel and everything after it, and starts with a different sound. Today it says "the vowel and everything after it", which is too loose.
- The "your song" side is the panel's labels on each suggestion. The existing *Your lyrics* topic switches to sound-based rhymes and syllables (decision 5).
- The guide's `gen::guides` entry: add "finding rhymes, writing with AI" to the summary, add "the Lyrics window's Write with AI panel" to `opensFrom`, and add the search words "ai ollama rewrite rhymes rhyming dictionary near rhyme slant family assonance consonance". It isn't on *Start here* (unchanged). The topic list follows by itself.

## Code
- **Recipes:** *Topics in an existing guide*; *Song data that's saved* (the brief); *A window* (the Lyrics window, now in its own `LyricsEditWindow.{h,cpp}`, moved out of `LyricsLane.cpp`); the GeneratePanel pattern for Ollama (`juce::Thread::launch`, a request id, `callAsync` with a SafePointer, a stop flag), not a progress window, since the panel shows progress itself.
- **`gen::rhyme`** (`src/generate/Rhyme.{h,cpp}`): the sound tools.
  - The rhyme part is the last stressed vowel and everything after it.
  - The onset is the sound before the rhyme part.
  - The classifier sorts two words into perfect, identical, family, additive, assonance, consonance, none, or unknown (falling back to `lyricsguide::getRhymeKey`'s spelling).
  - The consonant families are plosive, fricative and nasal. l, r, w, y and hh have no family.
  - It counts a line's syllables (decision 5), gives a line's stress pattern (primary stress strong; one-syllable function words weak, other one-syllable words strong), finds the dictionary rhymes for a word and hyphenates a line (decision 6).
  - `pronunciation`'s table must keep each vowel's stress (0, 1, 2). `lookupCmu` keeps returning sounds without stress. The reverse index from rhyme part to words is built once, lazily, and is safe to call from a worker thread.
- **`gen::lyricsai`** (`src/generate/LyricsAi.{h,cpp}`) holds no UI and needs no model in the tests.
  - A request kind: song, section (add or rewrite), next line, selection, rhymes.
  - It builds the context from the text and a character range: the verse and heading the range sits in, the line numbers, the rhyme partners by the verse's scheme, and the syllables and stress pattern of the lines it replaces.
  - It builds the system prompt (the guide's craft rules, and "never reuse lines from existing songs"), the user prompt (the brief, key, tempo, the sections with bars and lines wanted, the whole lyrics with the target marked, the asks) and the JSON schema. The schema has exact item counts, and the schema text also goes in the prompt (Ollama's advice).
  - It parses and checks the reply into alternatives with labels. It cleans each line (numbering, quotes, empty lines), drops lines that repeat the original and duplicates, and forces the line count for a selection of whole lines. If no alternative is within 2 syllables of a line target, it asks once more.
  - It makes the new text with the range replaced.
- **Invariants:**
  - The window never changes the edit before *Apply*.
  - *Use* changes only the captured range, and only if that range is unchanged.
  - Generating never touches tracks or instruments.
  - The worker gets plain copies (text, sections, key, tempo, brief) taken on the message thread, so no engine lock is needed.
  - Nothing runs on the audio thread.
- **Risk:** making the window non-modal (decision 2). `LyricsLane::editText` becomes a callback that `ArrangementView` serves with `pluginWindows`.

## Libraries and models
None new. It uses the user's own Ollama (`OllamaClient`) and the CMU Pronouncing Dictionary already built in.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | A perfect rhyme: the stressed vowel and all sounds after it are the same, and the sound before it (the onset) differs: night / light | Wikipedia, *Perfect and imperfect rhymes* | exact |
| 2 | An identical rhyme has the same onset too (leave / believe, homophones: night / knight) | same | exact |
| 3 | Songwriting teachers rank rhymes from most to least settled: perfect, family, additive / subtractive, assonance, consonance (Pat Pattison's scale, as his students describe it) | iZotope (D. Bawiec), Tunedly | opinion |
| 4 | Consonant families as songwriters group them: plosives p b t d k g; fricatives f v th (thin) th (this) s z sh zh ch j; nasals m n ng. Phonetics calls ch and j affricates | iZotope, Tunedly | opinion (a teaching grouping) |
| 5 | Settled rhymes suit resolved lines, loose rhymes unresolved ones | iZotope, Tunedly | opinion |
| 6 | Family: same vowel, final consonant from the same family (believe / belief). Additive / subtractive: consonants added or removed (feel / feels). Assonance: same vowel, consonants from another family. Consonance: different vowels, same final consonants | iZotope, Tunedly | exact (definitions) |
| 7 | CMU dictionary: about 126,000 words, 39 sounds, vowels marked 0 (no stress), 1 (primary) or 2 (secondary); North American English | CMU dictionary page (already under *Singing*), the embedded file | exact |
| 8 | Some rhymes depend on the accent: spaghetti / already rhyme in American accents | Wikipedia (fact 1) | exact |
| 9 | A syllable has one vowel sound; a line is usually 2 bars, so an 8-bar section takes 4 lines | the lyrics guide (*How a computer sings*, *Lines and phrases*) | typical |
| 10 | PhonologyBench (2024): syllable counting in sentences, people 90.0%, best model (Claude 3 Sonnet) 55.3%, GPT-4 23.3%, open models such as Mistral 7B about 7% | arXiv 2404.02456 | typical (dated models) |
| 11 | Ollama: a JSON schema in `format` constrains the reply; also putting the schema in the prompt is advised | docs.ollama.com, structured outputs | exact |
| 12 | Ollama's temperature defaults to 0.8; higher answers more creatively | Ollama Modelfile docs | exact |
| 13 | US Copyright Office, Part 2 (29 January 2025): prompts alone don't make the output protectable; human expression that is perceptible, and creative choices, arrangement or changes, can be; using AI as a tool doesn't bar protection; decided case by case | copyright.gov NewsNet 1060, LoC blog | exact (what it says), US only |
| 14 | Munich Regional Court, 11 November 2025 (GEMA v OpenAI): GPT-4 and 4o had memorised song lyrics and reproduced them nearly word for word, which infringed copyright; not final, under appeal | gema.de, Bird & Bird | exact (what it found) |
| 15 | Example syllable counts: "I walked a-long the ri-ver late" 8, "the ci-ty lights be-hind me" 7, "I feel so sad and lone-ly now" 8, "your coat still hangs be-side the door" 8, "two cups, and one is ne-ver full" 8, "the emp-ty chair where you would sit" 8 | derived (counted) | exact |
| 16 | Lines per section = bars ÷ 2, from 2 to 8: 4 bars give 2, 8 give 4, 16 give 8 | derived from fact 9 | exact |

### Topic texts

**Finding a rhyme** · *Perfect, family, assonance: from strong to loose*

Rhyme is about sound, not spelling. Night and bite rhyme, though they're spelt differently. Cough and though are spelt alike and don't.

What has to match: find the last stressed syllable of the word, the one you say louder. A perfect rhyme matches that syllable's vowel and every sound after it, and starts with a different sound. Night / light, polite / night, river / shiver and behind me / find me are all perfect. When the sound before the vowel matches too, it's an identical rhyme: night / knight, night / tonight, leave / believe. It sounds like the same word said twice, so it lands weakly.

Rhymes from strong to loose. Songwriting teachers such as Pat Pattison of Berklee rank rhymes from the most settled to the least:
  Perfect: night / light.
  Family: the same vowel, with the last consonant swapped for one of the same family: night / ride. T and d are both plosives, sounds made by stopping the air.
  Additive and subtractive: the same vowel, with a consonant added or taken away: feel / heels.
  Assonance: the same vowel, with consonants from another family: night / time, home / coat.
  Consonance: different vowels, the same consonants after them: night / boat.

The families, as songwriters group them, are these. The plosives stop the air: p, b, t, d, k, g. The fricatives hiss through a gap: f, v, th, s, z, sh, the s of measure, ch and j. The nasals hum through the nose: m, n, ng. L, r, w and y belong to none.

Use the scale. A perfect rhyme closes a thought, so it suits the last line of a chorus. A looser rhyme leaves it open, which suits a question or a line that leads on. Near rhymes aren't mistakes. Modern songs use them all the time, and they give you many more words to choose from.

In DawIt, select a word in the Lyrics window and click Rhymes. DawIt looks the word up in the CMU Pronouncing Dictionary: about 126,000 American English words, with the stressed vowels marked. It lists the perfect rhymes by syllables, then the family rhymes. The dictionary is full of names and rare words, so with Ollama running the AI also picks the rhymes that suit your line, and DawIt labels each one by its sound. A word the dictionary doesn't know, such as a name or a made-up word, gets rhymes from its spelling, marked as guesses.

What it can't hear: your accent. The dictionary is American English, and some words rhyme in one accent and not another. Spaghetti and already rhyme in most American accents, for example. Say the pair out loud in your own voice before you keep it.

**Writing with AI** · *What the model does well, and what you still decide*

The Write with AI panel in the Lyrics window asks a language model for words. The model runs on your own computer through Ollama, so your lyrics never leave it. It can write the whole song for your sections, add or rewrite a section, write the next line, rewrite a few words, or find rhymes. It's for getting unstuck and hearing other ways to say a line. It doesn't replace your ear.

How it works: a language model writes the next likely words, one after another, from what it learnt reading a great deal of text. So it is good at fluent, grammatical lines, and its first idea is often the most familiar one: hearts that fall apart, fire and desire. Ask for something specific. More concrete swaps a named feeling for a picture. Your words ("make it about the night bus home") gives it details only your song has. The more of the song it can read, the better it follows your story, point of view and rhyme scheme.

What DawIt checks, and why: models are poor at counting syllables, because they read words as chunks of text, not sounds. In a 2024 test called PhonologyBench, people counted the syllables of a sentence right 90% of the time. The best model managed 55%, and open models such as Mistral 7B, the size Ollama runs on a laptop, about 7%. So DawIt counts every suggestion itself with the pronouncing dictionary and shows it against the line it replaces ("8 syllables, like the original"). It also says which line it rhymes with. A line with the same syllables and stresses fits the same melody; a line 3 syllables longer has to be squeezed in.

Getting the best from it:
  Write the brief first: what the song is about, the mood, and who is singing to whom.
  Keep the lines you like and ask about the ones that don't work yet. Three alternatives for one line teach you more than a whole song written for you.
  Mix and edit: take the picture from one suggestion and the rhyme from another, then make it sound like you.
  Sing every line out loud before you keep it.

Who owns the words: in the United States, the Copyright Office said in January 2025 that prompts alone don't make a model's text yours to protect. The parts you write, and what you choose, arrange and change, can be. Using AI as a tool doesn't stop a song from being protected, and each case is judged on its own. Other countries have their own rules. This isn't legal advice: if you release a song, keep the lyrics mostly your own and check the rules where you live.

Lines it may have learnt: a model can repeat lyrics it saw while it was trained. In November 2025 a court in Munich found that ChatGPT had memorised song lyrics and reproduced them nearly word for word (the ruling may still be appealed). DawIt asks for new lines, but if a suggestion sounds familiar, search for it before you use it.

What it doesn't do: it doesn't write words to a melody's notes yet, and its syllable and rhyme checks work for English only.

**Rhyme** (changed sentence): "A perfect rhyme matches the last stressed vowel and everything after it, and starts with a different sound (night / light, behind me / find me). The Finding a rhyme topic shows the looser kinds."

### Tooltips
- Write the song: "Writes words for every sung section on the Sections row: a verse per verse, the chorus once (it goes on every chorus), the pre-chorus and the bridge, a line per 2 bars. You see it before it goes in."
- Section menu: "Rewrite the section the cursor is in, or add a verse, chorus, bridge or other section at the cursor, written to follow what's there."
- Rewrite selection / Write the next line: "Three other ways to write the selected words or lines, with the same syllables and rhyme where you ask for them (Cmd+R). With nothing selected: the next line after the cursor."
- Rhymes: "Rhymes for the selected word or the word at the cursor: from the pronouncing dictionary, and picked for your line by the AI (Shift+Cmd+R)."
- About / Mood / Point of view: "What the song is about, in your own words: the AI reads this for every request. Saved with the song." / "The feeling the words should carry." / "Who is singing to whom: I (a confession), you (to someone), we, or a story about others."
- More concrete: "Things you can see, hear or touch instead of named feelings: \"your coat by the door\" instead of \"I miss you\"."
- Simpler words: "Everyday words, said the way people speak: lyrics are heard once, at speed."
- Keep the syllables: "As many syllables as the original, so it fits the same melody."
- Keep the rhyme: "End on a word that rhymes with the line it pairs with."
- New words: "The same idea without the original's words." · Another angle: "A different picture or point of view on the same idea." · Shorter / Longer: "Fewer or more syllables than the original."
- Your words: "Anything else to ask for, such as \"make it about the sea\" or \"use the word harbour\"."
- An alternative: "Double-click (or Use) to put it in place of the highlighted text. Cmd+Z in the text takes it back."
- Try again: "Asks for new alternatives with the same request."
- Model / ⟳: "The Ollama model that writes, the same as the piano roll's generator." / "Reload the list of models from Ollama."
- Fold arrow: "Show or hide the Write with AI panel."

### Example data
- *Finding a rhyme*, rows (label, words, note):
  - Perfect: night · light · polite, "the vowel and all after it match; the sound before differs"
  - Identical: night · knight · tonight, "the sound before matches too: it sounds like the same word"
  - Family: night · ride · hide, "t and d: both plosives"
  - Additive: feel · heels, "an s added"
  - Assonance: night · time · mine, "the same vowel, other consonants"
  - Consonance: night · boat, "only the t matches"
  - Two syllables: river · shiver · giver, "the rhyme starts at the stressed syllable"
- *Writing with AI*:
  - the verse rows X/A/X/A from the *Rhyme* topic, with line 3 "I feel so sad and lonely now" (8) marked as the original (wrong = true);
  - then "your coat still hangs beside the door": "8 syllables, like the original · a picture instead of a feeling";
  - "two cups, and one is never full": "8 syllables, like the original · a detail only this song has";
  - "the empty chair where you would sit": "8 syllables, like the original · a place that says they've gone".

### "Your song" rules (the labels on each suggestion)
- **Syllables** (line targets): the same count gives "8 syllables, like the original". A difference of 1 gives "9 syllables, 1 more than the original". A difference of 2 or more is shown in `meterYellow`: "11 syllables, 3 more than the original: harder to sing to the same melody". It stays quiet for *Write the song* and *Add a section*, which show "Verse 1: 4 lines, 7 to 9 syllables" instead.
- **Rhyme:**
  - If the replaced line's last word rhymed with a partner line in its verse and the new one does too: "ends on light: rhymes with night (line 2)", with the kind when it isn't perfect ("a family rhyme with night").
  - If it rhymed before and doesn't now: "doesn't rhyme with night (line 2) any more", in `meterYellow`.
  - For a line with no partner: nothing.
  - For *Write the next line*: "rhymes with night (line 2)" when it rhymes with any earlier line of the verse, otherwise nothing.
- **Dropped:** an alternative identical to the original, duplicates, or empty lines.
- **Guessed:** "guessed from the spelling" when a word isn't in the dictionary.
- **Rhyme list:** each word labelled perfect, identical (listed last), family, additive, assonance or consonance.

### Test numbers
| Check | Expected |
|---|---|
| classify night / light, polite / night, river / shiver | perfect |
| classify night / knight, night / tonight, leave / believe | identical |
| classify night / ride | family |
| classify feel / heels | additive |
| classify night / time, home / coat | assonance |
| classify night / boat | consonance |
| classify night / table | none |
| rhyme part of "river" | ih v er (2 syllables) |
| the six example lines' syllable counts | 8, 7, 8, 8, 8, 8 (exact) |
| "lonely" unhyphenated, "lone-ly" | 2 and 2 |
| stress pattern of "I feel so sad and lonely now" | weak-strong ×4 |
| dictionary rhymes for "night" | contain light, bright, polite; not night, knight, tonight in the perfect list |
| lines wanted for a section of 4, 8, 16, 2, 40 bars | 2, 4, 8, 2, 8 |
| existing rhyme scheme tests (XAXA, AABB, ABAB) | unchanged |
| every lyrics topic has a name, summary, text and example; the topic count is 15 | exact |
| search "assonance", "ollama", "rhyming dictionary" | finds the lyrics guide |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. Keep vowel stress in `pronunciation`'s table, and add `gen::rhyme` (`Rhyme.{h,cpp}`, both CMake targets): rhyme part, onset, classify, families, syllable count, stress pattern, dictionary rhymes, hyphenate. Tests: `runRhymeTests` with the *Test numbers* rows. Done when the rhyme rows pass and the Sing this tests still pass.
- [x] 2. The lyrics guide: `analyse` and `findRhymeScheme` use `gen::rhyme`, falling back to spelling for unknown words and keeping identical words as rhyming (repetition). Fix the *Rhyme* sentence, and add the topics *Finding a rhyme* and *Writing with AI* with their examples. Update the `gen::guides` entry, and add the SOURCES.md lines (done in planning; check they are there). Tests: the topic count, text and examples, the search words, and the old scheme tests unchanged.
- [x] 3. The song brief: data, `Engine` getter and setter, `resetEdit`, and saving in `ProjectFile.cpp`. Tests: the round trip, and a project without the key.
- [x] 4. `gen::lyricsai` (`LyricsAi.{h,cpp}`, both targets): context, prompts, schemas, parse and check, apply, and the canned replies the screenshot scene uses. Tests: `runLyricsAiTests` without a model (see *Tests*). Add a real run to `runOllamaTests` under `--ollama`. Check: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 8):
- [x] 5. Move the editor into `src/ui/LyricsEditWindow.{h,cpp}`, opened non-modally through `pluginWindows` from `ArrangementView` (the lane's `editText` asks for it), with Apply, Cancel and keep timing as now, and the "changed outside" note. Check: Edit... opens it, a second click brings it to the front, and the guide opens and can be clicked while it's open.
- [x] 6. The *Write with AI* panel: the fold arrow, model box, brief fields (bound to the engine's brief), actions, Ask chips, your words, the results list with Use, Try again and Cancel, and the status notes. Requests run on a worker thread as in `GeneratePanel`. Check by hand with Ollama running: rewriting a line shows 3 labelled alternatives, *Use* replaces it, and Cmd+Z undoes it.
- [x] 7. Rhymes (the two lists), the text box's right-click menu, Cmd+R and Shift+Cmd+R, highlighting the target range, *Use* turned off when the range changed, the ? buttons into `ArrangementView::showGuide` at the right topics, and every tooltip.
- [x] 8. `LyricsExplainer`: colours for the *Finding a rhyme* rows (a kind per rhyme colour) and the struck original in *Writing with AI*. Check: `--export-guides` still passes; `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 9. Real-app check: add a `lyricsEditor` scene to `ScreenshotRunner`, with `"lyricsAi": "rewrite" | "song" | "rhymes" | "noOllama"`. It shows the canned results from step 4 without a model, on the pop song with line 3 of Verse 1 selected. Shots: each of the four states (`capture: window`), plus the guide at *Finding a rhyme* and *Writing with AI*. Look for: labels not cut off, counts of 8 in the example, the yellow label, the Ollama note, and the dictionary list.
- [x] 10. Docs:
  - the README Lyrics bullet: the panel, its actions and shortcuts, and the new topics in the guide bullet;
  - `docs/tests.md` (rhyme, lyrics AI, brief);
  - `docs/code-layout.md` (Rhyme, LyricsAi, LyricsEditWindow, and LyricsLane's line without "the text window");
  - architecture.md's *AI generation* bullet (LyricsAi beside GeneratePanel);
  - FEATURE_IDEAS: this idea's status, and *Generate lyrics with AI* set to *Partly done* (structure-driven writing, rewriting a line and rhymes done; melody-driven still open).

## Tests
- `runRhymeTests`: the *Test numbers* rows. A word not in the dictionary classifies by spelling and is flagged as guessed. Hyphenating "together forever" gives "to-ge-ther for-e-ver" only where the counts agree, otherwise the word stays whole.
- `runLyricsAiTests` (no model):
  - **Context:** for the pop song's lyrics with a range inside line 3 of a verse, the context names the verse heading, line 3, its syllables and its rhyme partner (none for an X line; line 2 for line 4).
  - **Prompts:** the user prompt contains the brief, the key, every sung section with its lines wanted (an 8-bar verse wants 4), the chorus once, and the asks. The schema's item count is 3 for a line, 2 for a section and 1 for the song.
  - **Parsing:** a canned reply with numbering, quotes, a duplicate and the original gives the clean alternatives without them. A reply of 2 lines for a 1-line target is cut or rejected as the plan says. A malformed reply gives an error, not a crash.
  - **Labels:** the syllable and rhyme labels per the rules, including the yellow cases.
  - **Apply:** replacing a range leaves the text before and after it byte-identical, and a range whose text changed is refused. *Write the song* gives text that `lyrics::place` puts on every chorus section.
  - **Brief:** a project with a brief saves and loads it; a project without one loads empty.
- Lyrics guide: 15 topics, each with text and an example; *Writing with AI* mentions Ollama and PhonologyBench; the search words find the guide.
- Optional `--ollama <model>`: one rewrite and one rhymes request return parseable replies.

## Website hand-off
- **Features page, Lyrics section:** a paragraph on *Write with AI* (on your own computer, the actions, DawIt's syllable and rhyme checks), with a screenshot of the window from the `lyricsEditor` scene, `rewrite` state (line 3 of Verse 1 with three alternatives labelled "8 syllables, like the original").
- A second shot: the rhymes state for "night", with both lists.
- The guide picker's Songwriting area: the two new topics.
- How it's made: no new library; the CMU dictionary row can add "rhymes".
- Main page: only if the AI step there lists what the AI writes (add "lyrics").

## Left for later
- Writing lyrics to a melody's notes (rhythm patterns, stresses on strong beats): in *Generate lyrics with AI*.
- Checking each alternative's stress pattern against the original's. The pattern goes to the model only: one-syllable stress from a dictionary is too rough to grade.
- Locking lines before *Write the song* runs again, and "write only the sections without words" after the structure changes.
- Other languages: the dictionary and the checks are English only.
- Cloud models: see *Cloud models: OpenAI, Gemini, Claude*.
- Synonyms and a thesaurus, which need a word list that DawIt doesn't have.

### Group A hand-off (engine, built and tested)
- **`gen::rhyme`** (`src/generate/Rhyme.{h,cpp}`): `classify (a, b)` gives `Kind`, `getRhymePart`, `getFamily`, `countSyllables (line)`, `getStressPattern (line)` (vector of bool, true = strong), `hyphenate (line)`, `isKnown (word)`, `findRhymes (word, maxPerKind = 30)` giving `RhymeList { guessed, items }` (perfect by syllables, then family, then identical last; the cap applies per kind and syllable count). The first `findRhymes` call builds an index, about a second: call it on a worker thread. `pronunciation::lookupCmuStressed` and `forEachCmuWord` are new; `lookupCmu` still returns sounds without stress. A phrase is classified by its last word only.
- **Lyrics guide**: `Topic::findRhyme` (after `rhyme`) and `Topic::writeWithAi` (after `craft`). In the example for *Finding a rhyme*, each row's `rhyme` index is the kind (perfect 0, identical 1, family 2, additive 3, assonance 4, consonance 5, the two-syllable row 0): group B's `LyricsExplainer` needs a colour for each. In *Writing with AI*, line 3 (index 2) has `wrong = true` tokens (the struck original); the alternatives are lines 6 to 8 (index 5 to 7) with their notes. `lyricsguide::analyse` counts syllables by sound, and `rhymes()` counts perfect and identical rhymes (sound first, spelling when a word is unknown).
- **Brief**: `LyricsBrief { about, mood, pointOfView }` in `engine/Lyrics.h` with `lyrics::brief::getMoods()`, `getPointsOfView()` (pairs of id and name, "Any" first with an empty id) and `tidy`; `Engine::getLyricsBrief()` / `setLyricsBrief()` (sends a change message on a change); saved as the optional `lyricsBrief` object.
- **`gen::lyricsai`** (`src/generate/LyricsAi.{h,cpp}`): build a `Request` (kind, the box's text, `rangeStart`/`rangeEnd`, the sections, key, bpm, brief, asks, `addHeading`/`addBars` for `sectionAdd`, a seed), then `buildContext`, and on the worker `buildSystemPrompt`, `buildUserPrompt (r, c)` and `buildSchema (r, c)` into `OllamaClient::ChatOptions`, and `parseReply (r, c, reply)` giving `Result { alternatives, rhymes, note, error, shouldRetry }`. `Context::error` is set when the selection is unusable (show it). `applyAlternative (currentText, c, alt)` and `applyWord (currentText, c, word)` return `Applied { text, start, end }` or nullopt when the range's text changed ("The text changed here: ask again"); `c.replaceStart/replaceEnd` is the range to highlight (for an insertion the two are equal). `getCannedReply (r, c, withLongOne)` gives the model's JSON for the screenshot scene (`withLongOne` makes the last alternative 13 syllables, for the yellow label). Syllable and rhyme labels are in `Alternative` (`syllableLabel/syllableWarn`, `rhymeLabel/rhymeWarn`, `guessedLabel`, `summary`, and `sectionSummaries` for the song).
- **Group B**: the panel's *Write the song* request needs `c.target` = the whole text, so *Use* should ask "Replace all the lyrics?" when `c.target` is not empty, then call `applyAlternative`. The alternatives count asked of the model is `getAlternativeCount (kind)`. A rhymes request needs the cursor or selection in a word. The Ollama call for the rhymes kind runs `rhyme::findRhymes` inside `buildUserPrompt`.

## Changes while building
- The song's schema is `sections` (one entry per item of the plan, not an `alternatives` list): the song is always one version, so there is nothing to count.
- The theory-guide search test "a search names the topic that matches" now checks that the Rhyme topic is among the topics named, since *Finding a rhyme* also matches "rhyme".
- Near rhymes (family, additive, assonance, consonance) count as rhymes in the labels ("a family rhyme with night"); only perfect and identical ones count in the verse's scheme (`findRhymeScheme`), as before.
- The *Your lyrics* note about unhyphenated long words now says each word gets one place in time (counts by sound no longer need hyphens), and the closing line says rhymes are by sound.
- Theory check: *Finding a rhyme* now says phonetics calls ch and j affricates, and that l, r, w and y are in none of the three families (the sources list only the three).
- Theory check: the Additive row's note says "an s (a z sound) added", since heels ends in a z sound.
- Theory check: *Writing with AI* says the Munich ruling isn't final because OpenAI said it would appeal (was "may still be appealed"); no appeal ruling found by October 2026.
- Theory check: *Your lyrics* closing line says the rhyme scheme counts only perfect rhymes, matching `findRhymeScheme`.
- Open: an identical rhyme in a suggestion's label reads "rhymes with night", not "an identical rhyme with night" as *Your song rules* asks (needs a switch case moved in `rhymeSentence`, LyricsAi.cpp).

### Group B hand-off (UI, built; `./build.sh --test` passes without warnings)
- **`src/ui/LyricsEditWindow.{h,cpp}`** (in the `DawIt` target): `LyricsEditWindow (Engine&, int firstBar)`, with `onShowGuide (Topic)` and `onClose`. `ArrangementView::showLyricsEditor()` opens it through `pluginWindows.show (&lyricsEditor, "Lyrics", ..., { 560, 520 })`; `LyricsLane::editText()` now just calls the new `onEditText` callback (so `DevScreenshots`' `lyricsText` scene still works). The old modal `LyricsEditor` is gone from `LyricsLane.cpp`.
- The panel is in the same class: model box (`generator.model`, `generator.ollamaUrl`), the brief bound to `engine.setLyricsBrief`, the four actions, Ask chips (`lyrics.aiAsk`), Your words, results (`ResultsView`: alternatives with labels, or rhyme chips in the AI's picks and the dictionary groups), Use / Try again / Cancel, status line, `?` buttons (`Topic::writeWithAi`, and `Topic::findRhyme` beside rhyme results), fold arrow (`view.lyricsAiOpen`), Cmd+R and Shift+Cmd+R, and the right-click entries (ids 1001 to 1004). `launch (Request)` runs the worker (`Thread::launch`, request id, stop flag, one retry on `shouldRetry`); `deliver` and `showResult` show the result; `useSelected` / `applyText` put it in through the box (Cmd+Z undoes), asking "Replace all the lyrics?" for the song.
- **For Group C's scene:** nothing in the window takes canned replies yet. A scene needs a public way to show a result without a model: add something like `LyricsEditWindow::showCanned (ai::Kind, bool noOllama)` that builds the `Request` (selection on line 3 of Verse 1 of the pop song), calls `ai::buildContext`, `ai::getCannedReply (r, c, true)` and `ai::parseReply`, fills `lastRequest/lastContext/lastResult/lastDictionary` (rhymes: `gen::rhyme::findRhymes ("night")`) and calls `showResult (chatError)`. `noOllama` = the status with the Ollama message and no results. The window is reached with `arrangement.showLyricsEditor()` (private: add a friend or make it public; `lyricsEditor.getComponent()` gives the window). Hide nothing else: the model box is filled by `refreshModels()`, which will say "Can't reach Ollama" in a scratch run; the scene should skip it or set `modelsLoaded`.
- **Differences from the plan:** the window's minimum size is 560 x 520 (not 860 x 520) so that folding the panel can bring back the narrow width; unfolding makes the window at least 940 wide. The window opens at 980 x 660 with the panel, 600 x 660 without. "The text changed here: ask again" shows once when the range changed; *Try again* is also off then. The song brief's *About* field is not overwritten from the engine while it has the keyboard focus. `LyricsExplainer::getRhymeColour` has a sixth colour (teal) so the six rhyme kinds each differ, and every `wrong` word in a words picture is now struck out as well as red (the original in *Writing with AI*).
- **Not tried by hand:** a live Ollama request (no model run in this group); the real-app check is Group C's.

### Group C hand-off (check and docs, done)
- Scene `lyricsEditor` in `ScreenshotRunner` (`"lyricsAi": "rewrite" | "song" | "rhymes" | "noOllama"`), through the new `LyricsEditWindow::showCanned (state)`. Rhymes selects "light" (line 2 of the first chorus); the others select line 3 of the first verse. Use `"capture": "window"`, `"scale": 1`.
- The window now opens at 980 x 780 (was 660 tall) so that three alternatives and the yellow label show without scrolling.
- Shots (scratchpad, `shots/`): rewrite.png, song.png, rhymes.png, noollama.png, guide-rhyme.png (guide `lyrics`, topic "Finding a rhyme"), guide-ai.png ("Writing with AI"). For the website: rewrite.png (three alternatives, yellow 17-syllable label) and rhymes.png (both lists).
- Docs done: README bullet, tests.md, code-layout.md, architecture.md; ideas set (this one Done, Generate lyrics with AI Partly done).
