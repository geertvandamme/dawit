# Content research and checks

Research was based on the complete local website source, not an assumed deployed version. No live site URL was supplied.

Historical note: the original audit below predates the folder promotion on 29 October 2026. Its `website/` paths describe the former site at that time. The current seven-page site now occupies `website/`; former source paths in the asset and content inventories are explicitly recorded as Git revisions or local legacy provenance.

## Material reviewed

- `website/index.html`: positioning, three-stage overview, every guide-picker entry, captions, feature highlights and privacy claims.
- `website/features.html`: all 53 feature sections and their screenshots, including audio recording, inspector, returns, plugin limitations and export.
- `website/usecases.html`: all eight roles, recommended first actions, related features and screenshots.
- `website/made.html`: all tool, library, model, licence and maker credits.
- `website/site.css`, the existing logo and screenshot inventory.
- Project rules through `AGENTS.md` / `CLAUDE.md`, the website recipe and screenshot rules. The requested root `codex.md` file is absent.
- Targeted README sections, `src/generate/TheoryGuides.cpp`, audio recording in `src/engine/Engine.cpp` and `Engine.h`, pinned versions in `CMakeLists.txt`, and the distribution checklist.

`content-inventory.json` maps every original feature section to its replacement destination. `assets-manifest.json` records the origin and SHA-256 of the copied images.

## Findings applied to the replacement

| Finding in the existing material | Treatment |
| --- | --- |
| The explanation of where DawIt fits beside a full DAW comes late in the overview. | Explain the purpose on Home and give it a dedicated comparison page. |
| Theory support is spread across the overview and individual feature descriptions. | Give Learn a primary navigation link and distinguish guides, help while working and live feedback. |
| Detailed features make one long sequential page. | Keep all details, but group by task and use search, filters and native expandable sections. |
| Role descriptions say microphone audio cannot be recorded, while the features page describes audio recording. | Confirmed audio capture in the engine and corrected both affected role statements in the replacement. |
| Guide counts and beginner-path counts differ between pages and current code. | Avoid claiming a total guide count. Use the eight-step beginner path in `TheoryGuides.cpp`; explain that an older reused screenshot shows an earlier list. |
| Song templates and Styles of music are in the app and targeted README content, but absent from the site’s main guide list. | Include a concise Styles of music topic and first-session instructions. No screenshot was invented for it. |
| The distribution work is a checklist, not evidence of released installers. | Use an honest source-build route; do not invent a download, release URL, public repository URL, signing status or platform installer. |
| The README’s Known limitations section contains older statements contradicted by implemented features, including saving, undo, sends and tempo maps. | Use current feature descriptions and implementation evidence rather than copying that paragraph. |
| Plugin support has an exception. | Retain thirteen exportable effects/synths; Tune stays in the app. AU is macOS only, and plugin builds are from source. |
| Vocal tools have language and voice requirements. | Retain English-only detection/singing and the requirement to add a separately licensed voice. |
| The original site uses external fonts. | Use system fonts and update only the replacement’s website credits. |

## Verification

- All seven pages opened in the browser at its normal desktop width (1254 px) and at 375 px, in both light and dark themes. No horizontal document overflow in these checks.
- Visually reviewed the home page, comparison, guide details, feature details, role page, getting-started content and credits; refined the comparison layout for phones.
- Checked feature search, guide search, subject filters, zero-result messages, native expansion, mobile navigation, theme persistence and direct links that open a feature.
- Checked local page links, fragment destinations, unique IDs, image existence, alt attributes and declared PNG dimensions.
- Compared every copied app screenshot byte for byte with its original. All 130 match. The logo is also copied unchanged.
- The JavaScript passes `node --check`.
- Checked 26 reused external links with curl: 25 returned HTTP 200 after redirects. `https://chatgpt.com` returned HTTP 403 to the automated client; its original logo credit link is retained.
- No app build or audio-engine test was needed for a separate static site. No app code, original website files or user settings were edited. Nothing was committed or published.

The site does not provide a public app source URL because one is not supplied by the current website. Supplying the intended public repository or released installer would let Get started link directly to it.

## Follow-up: How it’s built (29 October 2026)

Redesigned `about.html` around human direction, AI-assisted development, the app's audio and learning parts, local model requirements and sound checks. All 23 existing credits are retained in four expandable groups, including Bravura. How it’s built is linked from the main navigation and footer on every page; `about.html#models` remains valid.

Corrected the blanket dependency-hash statement to match CMake's mix of versions, tags and checked downloads. Made Ollama's default local address and configurable server setting explicit. Replaced absolute claims about every revision passing with descriptions of the actual checks.

Verification: local links, fragments, headings and navigation checked; all seven pages checked for overflow and the new link at 375, 1000 and 1254 px in both themes (42 combinations); native credit expansion checked with keyboard and pointer, including all groups open on a phone. No app screenshots or assets changed. See `HOW-ITS-BUILT-REVIEW.md` for the review, evidence and further proposals.

## Follow-up: promote the maintained site (29 October 2026)

At the user's request, replaced the former website folder with the seven-page redesign. Preserved the redesign's local content, styling and screenshot edits, and kept an external recovery copy of the former site, including its uncommitted inspector work. Updated repository links, maintenance entrypoints and the preview configuration for `website/`. Recorded former asset/content sources as historical provenance, preserving the asset hashes. This is a local reorganisation, not a deployment.

Verification: all 143 HTML, CSS, JavaScript and image files matched the pre-move snapshots byte for byte. Checked all seven pages' local links, fragment targets and duplicate IDs, screenshot dimensions/alt text, all 134 manifest hashes and the feature-mapping destinations. README and instruction links resolve; JavaScript syntax and the generated website skill validate. Opened the home page and the inspector's direct feature link through the local HTTP preview, confirming the feature expands. No layout or styling changed, so the earlier responsive review remains historical rather than a newly repeated visual test.

Commit preparation: removed inherited trailing whitespace in `features.html` to pass the staged whitespace check. Its lines otherwise match the pre-move snapshot; the other 142 HTML, CSS, JavaScript and image files remain byte-identical. Rechecked links, fragments, assets, mappings and JavaScript syntax after this cleanup.
