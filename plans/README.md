# Implementation plans

One plan per feature, written by the **dawPlanFeature** agent before the feature is built and followed step by step by **dawImplement**. The plan holds the feature's theory content too (sourced facts, the final text of every topic, the advice rules and the numbers the tests check); after the build, **dawTheory** may check what shipped against it. Each plan is linked from its idea in [FEATURE_IDEAS.html](../FEATURE_IDEAS.html) with a paragraph `<p><strong>Plan:</strong> <a href="plans/name.md">plans/name.md</a></p>`. [CLAUDE.md](../CLAUDE.md) holds the rules every plan follows.

A small feature (one recipe, no new saved data, no theory content, about five steps or fewer) gets a short plan written by dawImplement itself: *Goal*, *Decisions for you*, *What the user does and sees*, *Steps*, *Tests* and *Website hand-off*, under about 5 KB. Every other feature gets the full plan below from dawPlanFeature.

A plan is `plans/<feature-name-in-kebab-case>.md`. Keep it short: under about 15 KB for everything except the topic texts in *Theory*, which ship in the app. No code (no function bodies, class drafts or JSON), no copies of CLAUDE.md's rules (name the rule instead), no descriptions of existing code beyond the names dawImplement needs, and steps of one to three lines. Every plan is read several times, so every line in it is paid for more than once.

Before writing, gather (from the idea and the code it touches) the answers to:

1. **What the user does and sees:** where it lives, how it's opened (button, menu item, shortcut, double-click, right-click menu), what's on screen afterwards.
2. **What it changes in the song:** new data in the edit, how it's saved in the `.dawit` project, and how older projects load.
3. **The theory or DSP behind it:** the facts, rules and numbers (typical values, ranges, standards) with their sources. This is what the guide explains and the tests check.
4. **Which guide it belongs to:** a new guide, new topics in an existing one, or links from related guides; what the "your song" analysis points out.
5. **What it builds on:** existing code to reuse (`Chord`, `MusicalKey`, `Section`, `Lyrics`, the gain staging levels, `EffectTools`, `fxui`, `PluginWindowManager`, `ChordDiagramViews`, `PdfWriter`) and the ideas it ties in with.
6. **Libraries or models:** anything new, with its licence, size, pinned version and hash, and whether it runs fully on the user's machine.
7. **Settings:** what is remembered between sessions and what belongs to the project.
8. **How to check it:** the test checks, and the screenshots in the real app and on the website.
9. **What stays open:** the parts left for later, noted in FEATURE_IDEAS.html.

The format:

```markdown
# <The feature's title, as in FEATURE_IDEAS.html>

Status: Planned            <!-- Planned, Approved, In progress, Built, not committed, Released; ", theory checked" after dawTheory -->
Idea: [<title>](../FEATURE_IDEAS.html#<anchor>)

## Goal
What the user gets, in two or three sentences, and why it helps a beginner.

## Decisions for you
1. <A choice the user has to make.> Options: ... **Recommended:** ... because ...
   Answer: <filled in when the user decides>

## What the user does and sees
Where it lives (arrangement row, device bar, mixer, clip editor, a window, a menu), how it's opened (button, menu item with
"...", shortcut, double-click, right-click menu ending with the guide), the layout of what's on screen (a sketch in text),
every interaction (click, double-click, drag, Alt, Cmd, Delete, copy and paste), the tooltips, and what's remembered
between sessions (`engine.properties` keys).

## Data and saving
New data in the edit, how `ProjectFile.cpp` saves it (defaults left out), how older projects load, and the test that saves
and loads it.

## The guide
A new guide or new topics in an existing one. Per topic: name, one-line summary, what the picture shows (the text is in
*Theory*). The "your song" topic and what it points out. Where it opens from, and links from related guides. Its entry in
the Theory Guides window (`gen::guides`): area, summary, "also opens from", search words, on *Start here* or not.

## Code
The design, not the code (about 1 to 2 KB): the recipes in [docs/recipes.md](../docs/recipes.md) it follows, by name; the new
concepts and types in a line each; the rules and invariants the code must keep; threading (what's under the lock, what's
an atomic, what the audio thread owns); and the risky parts. Member names, signatures and per-file change lists are
dawImplement's to choose.

## Libraries and models
Anything new, with its licence, size, pinned version and hash, and whether it runs on the user's machine; or "None".

## Theory
Written by dawPlanFeature, pasted in as it is by dawImplement; "None" for a feature with no theory.
- **Facts:** one line each, with its source (or "derived", with the working), marked exact, typical or opinion.
- **Topic texts:** per topic its name, summary and final explanation; the tooltips and starting points' "why" texts.
- **Example data** for the pictures, with the numbers worked out.
- **"Your song" rules:** condition (with numbers), advice text, when it stays quiet.
- **Test numbers:** a table of what is checked, expected value and tolerance.

## Steps
Named groups, each built by its own fresh dawImplement run, ending with a passing build; the last is always
*Group C, check and docs*. About three to six steps a group.

Group A, engine (`./build.sh --test` at step 3):
- [ ] 1. <What to do, in a line or two.> Tests: ... Check: <how to see it work>. Done when: <what must be true; the group's `./build.sh --test` at its last step passes, and ...>
- [ ] 2. ...

Group B, UI (`./build.sh --test` at step N-2):
- [ ] ...

Group C, check and docs:
- [ ] N-1. Real-app check: the shots for `--screenshots` (what each selects, opens and captures; any setting or scene `ScreenshotRunner` needs added), and what each screenshot must show.
- [ ] N. Docs: README Features bullet, docs/tests.md, docs/code-layout.md, FEATURE_IDEAS status (and the guide's idea under *Theory guides and learning*), docs/ (a new
        recipe or a changed one, architecture, UI design).

## Tests
Every check to add to `tests/EngineTests.cpp`, in words, with the numbers it expects (the theory's from *Theory*).

## Website hand-off
For dawWebsite (which first asks the user whether and where): the section for the features page, whether it matters on
the main page (its step, or the guide picker's area), the screenshots worth taking (window, song state, what they show),
and the How it's made page if a library, model or tool is added.

## Left for later
What this plan doesn't build, to note in FEATURE_IDEAS.html.

## Changes while building
Filled in by dawImplement: what was done differently from the plan, and why, and after each group a short hand-off for the
next (what it made that the next group uses, by name; anything half-done); and by dawTheory: what its check corrected.
```

Plans written before the theory moved into the plan keep it in a separate `plans/<feature>-theory.md`, such as [buses-theory.md](buses-theory.md). Plans written before this folder existed may sit next to FEATURE_IDEAS.html, such as [AUDIO_ANALYSIS_PLAN.md](../AUDIO_ANALYSIS_PLAN.md).
