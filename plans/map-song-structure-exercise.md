# Guided exercise: map a song's structure

Idea: https://github.com/geertvandamme/dawit/issues/200
Status: Released

## Goal
Guide the user through importing a recording, detecting its tempo and sections, and looping a chorus. Checks reflect the song and the existing tempo detection record, not button clicks. Do not change music without an explicit action.

## Decisions for you
- Approved: reuse the existing section menu's “Loop this section” and guide the user to a chorus; do not add a duplicate looping control. The final check requires an enabled loop matching a chorus's boundaries.
- Approved: retain the shared exercise's existing resume behaviour: the current step survives closing and reopening during the same session and project; a new project or restart resets it. Issue #200 does not specify persistence across restarts.

## Steps
1. Add exercise definitions and state checks, reusing the chord exercise's audio and tempo detection helpers. Detect an enabled loop matching a chorus; explain missing audio, sections or chorus and how to proceed. Use existing guides and their actual topic names.
2. Register the exercise in the shared model, window and menu, with actions and Show me targets for import, tempo, structure and the section lane. Reuse the existing “Loop this section” menu; no automatic edit. Preserve current exercise progress behaviour.
3. Add headless tests for definitions, links, prerequisite paths, real tempo and section state, matching loop boundaries, and invalidated state; cover resume and relevant action routing.
4. Check the real app with the screenshot runner, including missing prerequisites and the loop step; update README, tests and code-layout documentation. Check Start your song, the inspector welcome and First Song Exercise for needed changes.
5. Run `./build.sh --test` to `All checks passed`, assess warnings and update issue #200 with the outcome and any remaining work. Keep it open until committed.

## Dependencies and theory
No new dependency or guide content is expected. Reuse existing, sourced guide topics. If implementation changes factual guide text or adds a dependency, update SOURCES.md or the licence inventory, notices and packaging as required by project instructions.

## Changes while building
- Added four live steps to the shared exercise and a Help menu entry. The existing Sections menu loops the chorus; the tick checks the enabled loop and its exact boundaries within the recording.
- Added headless checks for progress and updated app documentation. No dependency or guide text changed; the existing guide topics are reused.
- The shared exercise chooser remains open in #195; this exercise is available from Help. The Start your song panel, inspector welcome and First Song Exercise need no changes for this separate exercise.
- Real-app screenshots remain to be checked in the main chat, where desktop app control is available. `./build.sh --test` passed with `All checks passed` and no warnings.
