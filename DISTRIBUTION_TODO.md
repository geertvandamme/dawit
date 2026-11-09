# DawIt release TODO: macOS DMG, Windows EXE and Linux archive

Updated: 9 November 2026. A ticked box names its evidence: a file, a workflow step or a release record. An unticked box is open work, even where something partial exists. Section 1 says what is already done and what blocks publishing.

## 1. Where things stand

**Done:** a pushed `v*` tag runs [the release workflow](.github/workflows/release.yml). It builds ONNX Runtime from pinned source, builds DawIt and `DawItTests`, runs the tests and packages each platform. A last job creates a **draft** GitHub release with these assets:

```text
DawIt-<version>-macos-arm64.dmg         ad-hoc signed, not notarised
DawIt-<version>-windows-x64-setup.exe   per-user Inno Setup installer, unsigned
DawIt-<version>-linux-x64.tar.gz        built on Ubuntu 22.04
SHA256SUMS.txt
```

The DMG, the app bundle, the installed Windows folder and the Linux archive all hold `LICENSE` and the whole `licenses/` folder. The repository [geertvandamme/dawit](https://github.com/geertvandamme/dawit) is public.

| Draft | Commit | Outcome | Records |
|---|---|---|---|
| v0.1.2 | tag `v0.1.2` | Built; notices added after the tag are missing from its packages | [source audit](docs/release-v0.1.2-source-audit.md), [input manifest](docs/release-v0.1.2-inputs.json) |
| v0.1.3 | `d5ddd5a` | Built; checksums, notices and ONNX telemetry checked | [verification](docs/release-v0.1.3-verification.md), [source follow-up](docs/release-v0.1.3-source-followup.md) |

Neither draft is published. v0.1.3 shipped Microsoft's prebuilt ONNX Runtime. Since `7d01823`, releases build it from source, so the next tag needs its ONNX evidence collected again.

**What blocks publishing, in order of size:**

1. **Complete source and notices** (section 3): there is no complete-source archive yet, the notice check for each platform is unfinished, the curl notice question is open, and there is no *About DawIt* window.
2. **Compiler warnings** (section 9): 1,560 macOS and 452 Windows third-party warnings are not yet reviewed ([warning review](docs/release-third-party-warning-review.md)).
3. **Clean-machine tests** (sections 7–9): no package has been installed and used on a clean Mac, Windows PC or Linux system.
4. **Signing and first launch** (sections 5–7): choose a route for each platform and write tested first-open instructions.
5. **Release page and website** (section 10): release notes, source and checksum links beside the downloads, an issue template and website download links.

The workflow for tagging is [docs/release-workflow.md](docs/release-workflow.md); draft handling is [docs/draft-releases.md](docs/draft-releases.md).

## 2. Decisions and scope

- **Licence:** DawIt is `AGPL-3.0-only`. The root `LICENSE` holds the declaration, the complete text and the component inventory. JUCE is used under its AGPLv3 option, so no paid JUCE licence is needed.
- **No mandatory spending:** standard GitHub Actions runners for public repositories and GitHub Releases do the building and hosting. Paid signing is optional.
- **Targets:** macOS Apple Silicon (`arm64`), Windows x64 and Linux x64. Intel Mac, Windows ARM64, AppImage and distribution packages are later targets (section 13).
- **Full app:** built-in instruments, effects, guides and runtime resources are included. Users need no compiler, CMake, Python or developer tools.
- **Models:** Basic Pitch, Bravura and the CMU dictionary are built in. Demucs, Whisper and the MuseScore General SoundFont download on first use, after asking. A fully offline package is a later option.
- **Not shipped:** Ollama and its models, singing voices, vocoders, third-party plugins and sample libraries. The user installs them. Recheck the exact terms before bundling any of them.
- **Plugins for other DAWs:** optional assets (section 12). The app's built-ins don't need them.
- **Website:** the seven-page site in `website/`, maintained under [its guide](website/MAINTAINING.md). Deployment is a separate task.

A release is ready when sections 3–10 are done for every target it offers. Record deferred work here instead of ticking it.

## 3. Licence compliance and complete source

### 3.1 DawIt's own licence and notices

- [x] `AGPL-3.0-only` is declared consistently in `LICENSE`, the README and the project instructions.
- [x] `LICENSE` holds the complete unmodified AGPLv3 text and DawIt's copyright and licence statement.
- [x] The README's *License note* says DawIt is AGPLv3 and third-party components keep their own licences.
- [x] The Windows installer adds no terms of its own ([windows-installer.iss](tools/windows-installer.iss) has no licence page). Recheck if one is added.
- [ ] Add an *About DawIt* window with version, copyright, licence, no-warranty statement, redistribution rights, source link and third-party notices, readable offline. No such window exists in `src/` yet.
- [ ] Put a clearly labelled **complete** source link beside every binary download and in the release notes. The draft notes now link only to the tagged source tree.
- [ ] Decide on source-file licence notices or SPDX identifiers, keeping third-party files clearly apart from DawIt's code.
- [ ] Review the ownership and provenance of code, copied examples, artwork, icons, example audio and text. Get permission for anything lacking suitable rights, or replace it.
- [x] [CONTRIBUTING.md](CONTRIBUTING.md) gives the licence of contributions (`AGPL-3.0-only`, inbound equals outbound, no CLA), building, testing and the project conventions.
- [x] [SECURITY.md](SECURITY.md) points to GitHub's private vulnerability reporting, enabled on the repository on 9 November 2026.

### 3.2 Inventory of shipped components

The full inventory, with versions and hashes, is in [LICENSE](LICENSE). The notice texts are in [licenses/](licenses/README.md).

- [x] Exact licence and NOTICE files sit in `licenses/` with an index and their provenance.
- [x] The release packages copy `LICENSE` and `licenses/` into `DawIt.app/Contents/Resources/`, beside the app in the DMG, into the Windows installation folder and into the Linux archive. For v0.1.3, all 38 notices matched the tag in every package, and the Windows CI job compared the installed files ([v0.1.3 verification](docs/release-v0.1.3-verification.md#notices)).
- [x] The README describes the CMU dictionary licence in full and says it must go with binaries.
- [x] The README and `LICENSE` give the VST3 SDK bundled with JUCE 8.0.9 as GPLv3, not MIT. Moving to JUCE 8.0.11 (VST3 SDK 3.8, MIT) is optional and needs regression tests.
- [ ] Finish the notice check for each platform: what the ONNX source builds actually compile in, JUCE's enabled platform code, ggml, Eigen's applicable `COPYING.*` files and the Windows installer output ([work still required](docs/release-v0.1.2-source-audit.md#work-still-required)).
- [ ] Resolve the curl notice: the ONNX aggregate notice leaves out curl 8.20.0's copyright dates. A full copy is in [docs/release-v0.1.3-notices/](docs/release-v0.1.3-notices/) but not in `licenses/` or the packages. Check whether curl is in each shipped ONNX build at all now that it's built from source.
- [ ] Extend the direct-input manifest ([release-v0.1.2-inputs.json](docs/release-v0.1.2-inputs.json)) into a per-release dependency manifest: component, version or commit, URL, hash, licence, changes, whether it is in the binary and where its source is. Attach it to each release.
- [ ] Check Steinberg's trademark guidelines before using a VST logo or compatibility wording.
- [ ] Package notices with separately distributed plugins (section 12), listing only what those binaries use.

### 3.3 Source for each binary release

- [ ] Write a script that builds `DawIt-<version>-complete-source.tar.gz` from the exact released commit, and attach the archive in the release workflow. It holds DawIt's source, assets, build and packaging scripts, the dependency manifest, licences, local patches and the source of every shipped component: JUCE with the VST3 SDK, Eigen, demucs.cpp, whisper.cpp with ggml, TinySoundFont, stb_vorbis, and ONNX Runtime with the submodules and dependencies its build fetches.
- [ ] Include the model, font and dictionary material their licences require, with provenance, and say which models download separately.
- [ ] Write instructions for rebuilding and installing a modified version, including ad-hoc signing on macOS. Users must not need the maintainer's keys.
- [ ] Rebuild from that archive alone in a fresh environment, with no network and no checkout, and check nothing is missing.
- [ ] Keep the source available beside every release for as long as the licences require, including older binaries that stay downloadable.

GitHub's generated source archive holds only DawIt's own tagged files. For v0.1.3 it matched all 804 tagged files, but it holds no fetched dependencies, so it isn't the complete source. A dedicated archive is this project's choice, not a universal AGPL rule. AGPL source requirements: [sections 1, 4–6 and 13](https://www.gnu.org/licenses/agpl-3.0.en.html). Eigen: [MPL FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/). CMU dictionary: [its licence](https://github.com/cmusphinx/cmudict/blob/74790861f652b15e4ac49015a90074ad62a27690/LICENSE).

## 4. Repository and build automation

### 4.1 Repository

- [x] The repository is `geertvandamme/dawit`, public, with issues open. The installer names it as publisher URL and its issues page as support URL.
- [ ] Review tracked files and history for credentials, signing material, private recordings and other private data.
- [x] `.gitignore` ignores `dist/` (the packaging output), build folders and `external/`.
- [ ] Pin JUCE to a verified commit. CMake now uses any local `external/JUCE` as it is, and otherwise fetches the `8.0.9` tag without a hash check. Release builds should reject a different revision.
- [ ] Add install instructions, tested system requirements, model download sizes and first-open steps to the README. The *Building* section already covers building from source.

### 4.2 Build automation

- [x] A tag workflow builds all three platforms from one commit, checks the tag matches the CMake version and creates a draft release ([release.yml](.github/workflows/release.yml)).
- [x] Release builds and tests run on macOS 15 arm64, Windows 2025 with Visual Studio 2026 x64 and Ubuntu 22.04. A failing test fails the job.
- [x] ONNX Runtime 1.30.0 is built from pinned source on each platform. The build's input manifest, `deps.txt`, licence, notices and CMake cache are kept as artifacts, and `tools/verify-release-onnx.py` checks the app and tests carry that exact library.
- [x] The workflow has read-only permissions; only the draft job can write. Actions are pinned to commit hashes, and Inno Setup 6.4.3 to a SHA-256.
- [x] `SHA256SUMS.txt` is made from the final packages.
- [ ] Add a build-and-test workflow for pushes and pull requests, read-only and with no secrets.
- [ ] Record the runner image, compiler, CMake and SDK versions in a release manifest, not just in the logs.
- [ ] Fix DawIt's own warnings and review the third-party ones for the tagged commit (section 9).
- [ ] Check no build-machine paths are needed at run time and no host-specific CPU optimisations (ggml, Eigen, ONNX) exclude hardware the release claims to support.
- [ ] Attach the complete-source archive, dependency manifest and final release notes as well as the packages.

## 5. macOS DMG

### 5.1 Build and bundle

- [x] Built in CI for `arm64` with Xcode; tests pass. The bundle ID is `com.dawit.app`; keep it once released.
- [x] `libonnxruntime.1.dylib` is in `Contents/Frameworks` and loads through `@executable_path/../Frameworks`; the release check compares its hash with the source build.
- [x] Licences are copied into the bundle before signing.
- [ ] Inspect every binary in the bundle with `lipo -info` and `otool -L`, and check ggml's Metal needs (embedded or a separate resource).
- [ ] Find the true minimum macOS from the whole dependency set and test it. CMake sets at least 14.0; the full app isn't tested there.
- [ ] Check the bundle holds no caches, settings, songs or test programs.

### 5.2 Signing route

**Now: ad-hoc signing (free).** This doesn't name a trusted publisher, and Gatekeeper can block a downloaded copy.

- [x] The ONNX library is signed first, then the app; `codesign --verify --deep --strict` passes ([package-release-macos.sh](tools/package-release-macos.sh)).
- [ ] Download the DMG in a browser on a clean Mac or a separate account, keeping the quarantine flag, and record what macOS shows.
- [ ] Write first-open instructions using *System Settings → Privacy & Security → Open Anyway*, naming the tested macOS versions. Never tell users to turn Gatekeeper off.
- [ ] Test microphone access in the ad-hoc signed app: the prompt and recording.

**Optional: Developer ID and notarisation (paid Apple membership).** Check the current cost first.

- [ ] Get a Developer ID certificate and keep the credentials in protected CI secrets.
- [ ] Use the hardened runtime, a secure timestamp, the audio-input entitlement and `com.apple.security.cs.disable-library-validation` (for third-party plugins).
- [ ] Notarise with `xcrun notarytool`, staple and validate. Test first launch from a real download.

References: [developer accounts](https://developer.apple.com/help/account/basics/about-your-developer-account), [notarisation](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution), [library-validation entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.cs.disable-library-validation), [opening apps from unknown developers](https://support.apple.com/en-us/102445).

### 5.3 DMG

- [x] [package-release-macos.sh](tools/package-release-macos.sh) builds a compressed read-only `DawIt-<version>-macos-arm64.dmg` with the app, an `Applications` shortcut, `LICENSE.txt` and `licenses/`.
- [ ] Add a short read-me to the DMG: source URL, architecture, signing status and first-open steps.
- [ ] Mount, drag to Applications, eject and launch the installed copy on a clean Mac.
- [ ] Install over an older version and check settings, models and songs survive.
- [ ] Document removing the app separately from removing models and settings.

## 6. Windows installer EXE

### 6.1 Build and runtime files

- [x] Built in CI with Visual Studio 2026 for x64 (`-A x64`); tests pass. `DawIt.exe` and the source-built `onnxruntime.dll` are installed side by side, and CI checks the installed DLL matches the build.
- [ ] List DLL dependencies with `dumpbin /DEPENDENTS` for `DawIt.exe` and `onnxruntime.dll`.
- [ ] Settle the Visual C++ runtime. The draft notes say it "may be required". Either link statically where possible, install Microsoft's redistributable under its terms, or ship the permitted app-local DLLs, then test on a clean PC.
- [ ] Find and publish the tested minimum Windows version and CPU features.
- [ ] Test recording, playback, file dialogs, graphics, VST3 scanning and every inference path on a clean PC.

### 6.2 Installer

- [x] Inno Setup 6.4.3 is pinned by hash and its licence is shipped in `licenses/`.
- [x] [windows-installer.iss](tools/windows-installer.iss) and [package-release-windows.ps1](tools/package-release-windows.ps1) build `DawIt-<version>-windows-x64-setup.exe`.
- [x] Per-user install into `%LOCALAPPDATA%\Programs\DawIt` with no administrator rights; fixed AppId; Start-menu shortcut; desktop shortcut optional and off.
- [x] CI installs twice into a path with spaces, checks every installed notice, then uninstalls.
- [ ] Show licence and source information during or after installation.
- [ ] Check settings, models, recordings and projects live outside the install folder and survive an upgrade and an ordinary uninstall.
- [ ] On a clean PC, test a first install, an upgrade from the previous version, cancelling, uninstalling and reinstalling, plus a path with non-ASCII characters.

### 6.3 Signing route

**Now: unsigned (stated in the draft notes).** SmartScreen, Smart App Control or company policy may stop the installer.

- [ ] Download the installer in a browser on a clean PC and record exactly what Windows shows. Write first-run instructions from that.
- [ ] Optionally apply to [SignPath Foundation](https://signpath.org/terms). ONNX is now built from source, which may help with its rules on upstream binaries; check its current terms.
- [ ] If signing: sign the executable, the DLLs DawIt builds, the installer and the uninstaller, with timestamps, then compute hashes. Signing doesn't remove SmartScreen warnings straight away ([Microsoft's guidance](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation)).

## 7. Linux archive

- [x] Built in CI on Ubuntu 22.04 for x64; tests pass. [package-release-linux.sh](tools/package-release-linux.sh) packs the `DawIt` executable, `libonnxruntime.so.1` (loaded through `$ORIGIN`), `LICENSE.txt` and `licenses/` into `DawIt-<version>-linux-x64.tar.gz`.
- [ ] List the system libraries the binary needs (ALSA, JACK, FreeType, fontconfig, X11, OpenGL, curl) and their package names on Debian/Ubuntu and Fedora, and put them in the README and the release notes.
- [ ] State the glibc baseline (Ubuntu 22.04's glibc 2.35) and test on at least one other distribution.
- [ ] Add a short read-me to the archive, and optionally a `.desktop` file and icon with install instructions.
- [ ] Test on a clean system: start-up, audio (ALSA, and JACK or PipeWire), file dialogs, VST3 scanning, model downloads and inference.
- [ ] Decide whether the archive is enough for the first public release or whether an AppImage comes first (section 13).

## 8. Models, voices and offline behaviour

Current policies, to recheck every release: Ollama and its models, singing voices, vocoders, third-party plugins and sample libraries stay out of the packages.

- [ ] Record the exact Demucs and Whisper model URLs, revisions, hashes, sizes, origins and licences, including how converted weights relate to the originals.
- [ ] MuseScore General: check the download from the packaged app and that the licence file lands beside the `.sf3`. Decide whether to keep relying on the set's overall MIT statement for samples without their own licence.
- [ ] From the packaged app, test model downloads: progress, cancel, interrupted download, corrupt file, hash mismatch, retry, a full disk and a folder you can't write to.
- [ ] Check models go to a per-user data folder, survive upgrades and never need writing into the app bundle or install folder.
- [ ] Test the editor, playback, built-ins and guides with no network.
- [ ] Check ONNX telemetry stays off on clean installs of each platform, including Windows ETW events. The Windows switch relies on an internal ONNX Runtime 1.30.0 signal: recheck it whenever ONNX Runtime changes.

## 9. Release validation

Record each result against the exact candidate: version, commit, file and hash, OS version, architecture, signing status, tester and outcome.

- [x] The engine tests pass in CI on all three platforms (v0.1.3 and later tags).
- [ ] Fix DawIt's own warnings, and review the third-party warnings for the tagged commit. Open now: 1,560 `-Wshorten-64-to-32` in whisper.cpp/ggml on macOS, and 452 on Windows (C4267, C4244, C4200, C4047, C4018). Linux is reviewed ([warning review](docs/release-third-party-warning-review.md)).
- [ ] Download and install each package through a browser on a clean machine with no checkout, compiler or model cache.
- [ ] Launch from the installed location and check About, version, licence texts, source link and resources.
- [ ] Create a song with MIDI and audio tracks, play built-ins, add effects, change tempo and key, record audio and MIDI, save, reopen, and export audio, MIDI and PDF.
- [ ] Check the example song, piano roll, mixer, routing, guides, score rendering, menus and common shortcuts. For screenshots, follow [checking in the real app](docs/recipes.md#checking-in-the-real-app).
- [ ] Run audio to MIDI with the built-in Basic Pitch model; download and run Demucs and Whisper; check cancelling and the errors when a model or tool is missing.
- [ ] Try singing and Ollama with separately installed, permitted test content where the release says they work.
- [ ] Load a third-party plugin, save its state and reopen it. Record which formats and hosts were tried.
- [ ] Test microphone permission, changing audio devices, an ordinary user account, offline start-up and no network.
- [ ] Test upgrade and uninstall without losing songs, settings or models.
- [ ] Rebuild from the complete-source archive and follow its instructions.

## 10. Release page and website

- [x] Version, tag and asset names come from one place: the tag must equal `v` + the CMake version.
- [x] For v0.1.3 every draft asset was downloaded again and checked against `SHA256SUMS.txt`.
- [ ] Write the public release notes: systems and architectures, what's included, model downloads, external setup, signing status, installation and first-open steps, known issues, and the complete-source link. The draft notes are for testers only.
- [ ] Put source, licence, checksum and first-open links beside the downloads. Say that unsigned or ad-hoc builds can meet system security warnings.
- [ ] Check GitHub's asset size limits against the complete-source archive.
- [ ] Mark it a prerelease while the version is below 1.0, unless decided otherwise.
- [ ] Add an issue template asking for DawIt version, OS, architecture and steps; ask for logs without private music or credentials.
- [ ] Update the download links and install text in `website/` (following its guide; no eighth page; deploy only when asked). Show architectures, source and checksums, and only tested requirements.
- [ ] Publish the draft deliberately once sections 3–9 are done for every target it offers.

## 11. Release runbook, every release

1. Finish the changes and docs; resolve open licence and dependency questions.
2. Set the version in `CMakeLists.txt`; commit, then tag only when the maintainer asks ([release workflow](docs/release-workflow.md)).
3. The tag workflow builds, tests and packages every target and creates the draft.
4. Collect the evidence: ONNX artifacts, warnings, notices, checksums. Record it in `docs/release-<version>-*.md`.
5. Download the assets again; test clean installs, core features and the source rebuild.
6. If a candidate fails, fix it and make a new version. Never move a tag or replace a published binary.
7. Publish when the maintainer says so; update the website links and first-open steps.
8. Check live links, source and hashes. If a serious fault turns up, mark the release unsuitable, point to the last good version and keep its source available.

## 12. Optional plugin products

These don't block an app-only release.

- [x] DawIt's plugin scan recognises its own plugin copies so the add menus can hide them (`plugins::isDawItPluginCopy`). Test again with an installed release.
- [ ] Build the 13 plugin products from the same commit as the app. DawIt Tune stays app-only.
- [ ] Validate VST3s with a pinned `pluginval` and AUs with `auval`; test controls, guide, audio and MIDI, presets, automation and state recall.
- [ ] Keep manufacturer and product IDs and parameter IDs stable once released.
- [ ] Test the shipped bundles in real hosts. Logic (AU) is the documented manual test; list other hosts only once tested.
- [ ] Sign and notarise Mac plugin bundles by the chosen route, include notices and source access, and test with quarantine.
- [ ] Choose how plugins get installed: separate assets (`DawIt-<version>-plugins-<platform>`), copy instructions, or a future in-app command (that's feature work).
- [ ] Test per-user installation, rescanning, upgrading and removal on each platform.
- [ ] Check what happens when an older project names a DawIt plugin copy.

## 13. Later targets and options

- [ ] **Intel Mac or Universal:** an x86_64 ONNX build, CMake architecture selection and every library in both slices; test on Intel hardware.
- [ ] **Windows ARM64:** validate the compiler, ONNX build, audio, inference and ARM64 plugins on real hardware.
- [ ] **Linux AppImage, Flatpak or `.deb`:** choose one, bundle or declare its libraries, and test on several distributions. An AppImage doesn't guarantee every distribution works.
- [ ] **Portable ZIPs** for Mac or Windows, once settings and model folders are confirmed writable.
- [ ] **Package managers:** Homebrew cask, winget, Flathub, once packages are stable.
- [ ] **More plugin formats:** LV2 or AAX, each with its own SDK, licence and signing review.
- [ ] **Offline edition:** an optional model pack with notices, hashes and an import path the app actually uses.
- [ ] **Paid signing:** re-evaluate Apple and Windows signing if install friction warrants it, checking current prices.

## 14. Release files

| File | Status |
|---|---|
| [.github/workflows/release.yml](.github/workflows/release.yml) | Exists: tag builds, tests, packages, draft release |
| `tools/build-onnxruntime-source-{macos,linux}.sh`, `tools/build-onnxruntime-source-windows.ps1` | Exist: ONNX Runtime from pinned source |
| [tools/verify-release-onnx.py](tools/verify-release-onnx.py) | Exists: the shipped ONNX library is the one just built |
| [tools/package-release-macos.sh](tools/package-release-macos.sh) | Exists: ad-hoc signing and DMG |
| [tools/package-release-windows.ps1](tools/package-release-windows.ps1), [tools/windows-installer.iss](tools/windows-installer.iss) | Exist: Windows installer |
| [tools/package-release-linux.sh](tools/package-release-linux.sh) | Exists: Linux archive |
| `LICENSE`, [licenses/](licenses/README.md) | Exist; per-platform notice check open |
| Complete-source script and its workflow step | To create |
| Per-release dependency manifest | To create, from [release-v0.1.2-inputs.json](docs/release-v0.1.2-inputs.json) |
| Push and pull-request build workflow | To create |
| *About DawIt* window | To create, as an app feature through the normal plan |
| [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md) | Exist; private reporting enabled |
| Issue template | To create |
| README install section and website download links | To write |

App and release-infrastructure changes go in one commit, and website changes in another, following the project's commit rules.
