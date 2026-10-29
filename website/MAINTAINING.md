# Maintaining the DawIt website

These are the shared website instructions for Claude, Codex and ChatGPT. Read them before adding content, reviewing the site or changing its design. Project-wide rules still apply.

## Which site to edit

Maintain the seven-page site in `website/`. The redesign was promoted to this folder at the user's request on 29 October 2026, replacing the former site. Keep the current files and patterns; do not recreate a parallel legacy site. This local folder move does not mean the site has been published.

Read the relevant page, `assets/site.css` and the relevant behaviour in `assets/site.js` before changing its pattern. `STRUCTURE.md` explains the information architecture; `CONTENT-AUDIT.md` records the original research and corrections. Their historical findings are context, not proof that the app has stayed unchanged.

For an explicit content-update request, apply the authorised change using this guide; do not ask the user to approve routine page placement again. For a proposal-only request or a feature hand-off whose website scope has not been decided, propose the pages, copy and existing images first. Carry decisions and authorisation forward. Ask only for a material missing decision, such as an unknown download destination.

## What the site must explain

Keep DawIt's purpose clear: a music sketchpad that helps people learn while making music, with theory guides, playable examples, explanations beside controls, live pictures and help about their own song.

A full DAW supports developing and finishing a production. DawIt focuses on understanding and trying ideas, with MIDI and WAV as routes into a full DAW. DawIt also has real recording and mixing tools. Do not suggest it cannot record audio, or that full DAWs have no explanations or educational resources.

Write for a beginner: say what a tool helps them do before naming controls. Use British spelling, short sentences and concrete examples. Explain unfamiliar terms. Describe the app as it is, without “new”, “just added” or release-announcement wording. Do not invent testimonials, downloads, supported platforms, performance figures or capabilities.

Verify each changed claim against the implementation, relevant tests, theory sources or current project documentation. Search the large root README by heading and read only the relevant range. Treat conflicting older documentation as something to resolve, not copy. Use `src/generate/TheoryGuides.cpp` for guide names and the beginner path, `CMakeLists.txt` for pinned dependencies, and model-loading code for downloads and language restrictions. An idea or unchecked distribution task is not an implemented feature.

## What to add, and where

| Change | Main destination | Other pages to consider |
| --- | --- | --- |
| A user-visible feature | `features.html`, in the existing task group that fits | `learn.html` for its guide; `for-you.html` for the roles it helps. Home only if it changes a visitor's understanding of the app. |
| A theory guide or substantial learning topic | `learn.html`, in the relevant subject | Link to its feature; update the beginner path only if the in-app path changes. |
| A change to real-time help, live pictures or song-specific advice | The relevant feature and learning explanation | `why-dawit.html` if it changes how the site's central comparison should be explained. |
| A workflow for a particular audience | The matching role in `for-you.html` | Link to detailed features rather than duplicating their full instructions. |
| Installation, availability, requirements or optional setup | `get-started.html` | Update any affected availability claim elsewhere. Only link a verified repository, release or installer supplied by the project or user. |
| A library, model, licence, coding tool or outside resource | `about.html` | `get-started.html` if installation, model size, privacy, language or voice requirements change. |
| A change in DawIt's purpose or relationship to a full DAW | `why-dawit.html` | Keep Home and the getting-started answers consistent. |

Keep Home selective: introduction, the learning difference, audiences, feature routes and the hand-off to a full DAW. Put full explanations in Learn and Features. Keep the eight role entry points useful; do not add a role for every feature.

The top navigation stays **Why DawIt, Learn, Features, For you, How it’s built, Get started**; the logo links to Home. **How it’s built** (`about.html`) is also linked in the footer. Navigation is repeated in all seven HTML files. If the user authorises changing it, update every header and footer, each page's `aria-current`, and `STRUCTURE.md` together. Preserve established page URLs and fragment IDs so existing links keep working.

## Keep the current technical setup

The site is buildless static HTML with one shared stylesheet (`assets/site.css`) and one deferred script (`assets/site.js`). Edit these maintained files directly. There is no build-time template, framework, package installation or generator to run. Do not reconstruct the pages from the old website or a temporary migration script: that would lose edits to this site.

Use the existing paper/green visual direction, CSS variables, system fonts and reusable classes. Maintain both light and dark tokens, responsive rules, keyboard focus and reduced-motion support. Avoid per-page styles for a pattern that belongs in the shared stylesheet. Preserve the logo, page titles, descriptions, one main `h1`, sensible heading order, skip link and labelled navigation.

Content and ordinary links must remain in the HTML. Native `details`/`summary` provide expansion without scripts. The script adds search, filters, direct-link expansion, theme selection, mobile navigation and copying commands. Do not move the catalogue into a JavaScript-only data source or add external runtime libraries, fonts, analytics, accounts or backend services for routine content work.

Keep `main-nav`, `.menu-toggle`, `.theme-toggle`, their accessibility attributes and the `dawit-next-theme` local-storage preference compatible with `assets/site.js`. Clipboard failure must leave the commands readable and selectable.

## Adding or extending a feature

Prefer extending an existing entry when the capability belongs to it. Otherwise add a `details.feature-item` inside one of these sections:

| Group key | Task |
| --- | --- |
| `write` | Write & compose |
| `perform` | Play, sing & record |
| `analyse` | Explore a recording |
| `mix` | Mix & understand |
| `synth` | Make a sound |
| `workflow` | Work & export |

Copy the nearest existing entry. Preserve this contract:

```html
<details class="feature-item" id="stable-feature-id" data-item data-group-key="mix">
  <summary>
    <span>
      <strong>Feature name</strong>
      <span>What the visitor can do with it</span>
    </span>
    <span class="disclosure" aria-hidden="true">+</span>
  </summary>
  <div class="feature-body">
    <h3>A heading about the result</h3>
    <p>What it is for, where to open it, and what happens.</p>
    <ul>
      <li><strong>A useful action.</strong> Its result, with a reason or example.</li>
    </ul>
  </div>
</details>
```

Replace the example text and ID; never ship placeholders. Use a unique, stable lower-case hyphenated ID and link to `features.html#stable-feature-id`. Describe the main actions, learning support and material limitations, such as English-only vocals, optional models or plugin exceptions. Add a relevant existing screenshot only when it helps.

The enclosing `section.feature-group` has matching `id` and `data-group`; its filter button's `data-filter` and the entry's `data-group-key` must match exactly. New entries in existing groups need no JavaScript registration. Search indexes their text on page load and the result count is computed. Keep `.results-count`, its live-region attributes and `.empty-state`.

Keep `content-inventory.json` useful when adding, retiring or moving a feature: retain the original migration mappings, update an affected destination and add a row for a new entry with its name, group, destination and real source evidence (for example an implementation file or plan). Do not invent an old `website/features.html` origin. Do not freeze the original 53-entry count in checks or add exact counts to marketing copy without maintaining them.

## Adding or extending a guide

Add a `details.guide-card` inside the `#library .guide-grid.catalogue` in `learn.html`. Copy a nearby card with a stable `guide-...` ID, `data-item`, a `data-group-key`, a `summary` containing subject/name/description/`.guide-open`, and a `.guide-body` containing its explanation, optional existing screenshot and a link to the related feature.

Subject keys are case-sensitive: **Foundations**, **Chords & songs**, **Singing**, **Mixing**, **Effects**, **Synthesis**. Match the existing filter's `data-filter` exactly; encode `&` as `&amp;` in HTML attributes. Keep cards and the empty state inside `#library`, the scope used by the script. A new card in an existing subject needs no JavaScript entry.

Explain what someone learns and how they can try it in the app. Only claim a “Your song”, “Your track” or “Your clip” page, audible example or live view when it exists. Website images demonstrate the app; do not imply that the screenshot itself plays audio. Keep the beginner path consistent with the app and link each step to a real guide ID. Avoid exact total guide counts: a guide can have several website topics.

## Screenshots and asset provenance

The current instruction is to reuse existing app screenshots. Do not capture or generate new ones, alter their contents, or change the app to stage an image unless the user changes that instruction. A text-only entry is fine when no existing image represents it accurately. Mention a missing suitable image in the report rather than using a misleading one.

Use images inside `assets/screenshots/`, so the site remains standalone. Reuse an existing image there; if another authorised repository asset is needed, copy it unchanged and record its actual origin. The former `website/screenshots/` directory is no longer a current source location. Inspect the chosen image for relevance and legibility. Keep full-size image links, real pixel dimensions, descriptive alt text, `decoding="async"` and lazy loading except for the home hero. Caption what the image actually shows; identify an earlier UI when that matters.

`assets-manifest.json` contains site-relative `file`, source provenance and the file's SHA-256. Sources prefixed with `git:<revision>:` identify the former site's committed files; `legacy-local:` identifies a copied local file that was not committed before the move. They are historical provenance, not current filesystem paths. Add a matching record for another asset, using its actual source or capture details. Never alter a hash to conceal an unintended asset edit. Before removing an image, check all seven pages, root README links and the manifest. `content-inventory.json` uses the same historical source notation for the original feature mappings.

## Consistency to preserve

Keep relevant feature, guide and audience links in agreement. Follow an amended capability through related pages: for example, recording claims on Features, For you and Get started must agree. Model, voice and plugin requirements must agree between Features, Get started and How it’s built.

Retain accurate privacy wording: music processing and models run locally; optional downloads require a connection. Ollama is installed separately. English vocal support, separately supplied voices with their own terms, and the exclusion of Tune from exported plugins should be rechecked before changing them.

No installer, public source URL or deployment should be invented. New availability requires verified evidence. Do not publish, commit or push unless requested. Keep website commits separate from app changes. Do not change app code for a website-only task.

Update `STRUCTURE.md` only when routes or page responsibilities change; update the local README when setup or behaviour changes. Preserve `CONTENT-AUDIT.md` as the original audit and append material follow-up findings when useful rather than rewriting its history or logging every copy edit.

## Verification and hand-off

Preview from the repository root, or use the `website` launch configuration:

```sh
python3 -m http.server 8766 --bind 127.0.0.1 --directory website
node --check website/assets/site.js
```

Use HTTP preview for clipboard and navigation checks. A file URL blocked by a browser tool is a tool limitation, not a reason to alter the site. For a website-only change, do not build the C++ app or run audio-engine tests.

For changed pages, check desktop width and 375 px in light and dark themes. Check for overflow, clipped text, image loading and readable expanded content. Exercise the interactions affected by the edit: the matching filter, relevant searches, a zero-result search, the result count, keyboard expansion, a direct link that reveals the entry, and any changed navigation or theme behaviour.

Check local links and fragment IDs, duplicate IDs, image dimensions, meaningful alt text and manifest hashes for changed assets. Check new external links; distinguish an automated-client block from a missing destination. If navigation or shared CSS/JavaScript changed, check every page it affects. Reopen a hash link after filtering to ensure its target is revealed.

Report which pages changed and why, the content evidence, assets reused, checks performed and anything unverified. Report limitations accurately. Do not claim browser verification if only source checks were possible.

## Keeping agent instructions in sync

This file is the single source of detailed website instructions. The root `CLAUDE.md` (also read through `AGENTS.md`), root `codex.md`, this folder's `CLAUDE.md` and `AGENTS.md`, and the website agent/skill must continue to point here.

The Claude agent is `.claude/agents/dawWebsite.md`. The Codex skill is generated from it; update the Claude source first, then run from the repository root:

```sh
python3 .claude/sync-codex.py --only daw-website
```

Do not maintain a second copy of these detailed rules in either entrypoint. The targeted sync preserves unrelated skills. If the user later promotes or renames this site, update the pointers, page/asset paths and preview configuration together instead of silently reviving the legacy instructions.
