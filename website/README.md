# DawIt website

The maintained seven-page static website for DawIt. The redesign has replaced the former site in this folder; deployment remains a separate task.

**For AI agents:** read [MAINTAINING.md](MAINTAINING.md) before adding content or changing the site. Claude, Codex and ChatGPT share those instructions.

## Preview

From the repository root:

```sh
python3 -m http.server 8766 --bind 127.0.0.1 --directory website
```

Open <http://127.0.0.1:8766/>. No package installation or build step is required. The pages also work as local HTML files; use a server to test clipboard support.

## Pages

- `index.html` — the introduction and main routes into the site.
- `why-dawit.html` — DawIt’s purpose beside a full DAW.
- `learn.html` — explanations, real-time help, a beginner path and a searchable guide library.
- `features.html` — the searchable feature catalogue.
- `for-you.html` — the eight roles and their starting points.
- `get-started.html` — availability, source builds, a first session, optional tools and answers.
- `about.html` — How it’s built: human decisions, AI-assisted development, audio tools, local models, checks and expandable credits.

[STRUCTURE.md](STRUCTURE.md) explains the current information architecture. [CONTENT-AUDIT.md](CONTENT-AUDIT.md) records the research, corrections and verification. `content-inventory.json` maps the original feature anchors to their destinations.

[HOW-ITS-BUILT-REVIEW.md](HOW-ITS-BUILT-REVIEW.md) records the development-page review, the redesign decisions and further proposals.

## Assets and behaviour

`assets/site.css` is the shared stylesheet. `assets/site.js` adds search, filters, theme selection, mobile navigation, direct-link expansion and copying build commands. Content, ordinary links and native expandable sections are present in the HTML; scripts are not needed to read it.

The logo and app screenshots are kept in `assets/`, making this folder independently hostable. `assets-manifest.json` records each asset's source and SHA-256. Historical `git:<revision>:` and `legacy-local:` sources refer to the former site before it was replaced, not current file locations. Full-size screenshot links use this folder's assets. Screenshots load lazily except for the home hero.

System fonts are used. There are no external runtime libraries, font requests, analytics, forms, accounts or backend services. The only persistent browser preference is the chosen colour theme, under `dawit-next-theme` in local storage.

The folder move does not deploy the site. Get started and How it’s built link to the public [GitHub repository](https://github.com/geertvandamme/dawit), as does every footer. Get started explains that source publication is being prepared and installers are not available yet, followed by source-build instructions.
