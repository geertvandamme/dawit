# DawIt release TODO: GitHub, macOS DMG and Windows EXE

Updated: 29 October 2026. This replaces the earlier distribution checklist. It records work to do; it does not claim that the installers, public repository or release workflows already exist.

## 1. Release decisions and scope

- **DawIt is fully open source under AGPLv3 only (`AGPL-3.0-only`).** The root `LICENSE` contains the declaration and complete text. No paid JUCE licence is needed when complying with its AGPL option.
- **The existing plan targets distribution without mandatory spending.** Standard public-repository GitHub Actions runners and GitHub Releases are the intended build and download services. Paid signing remains optional; check service limits before enabling jobs.
- **Proposed first binary targets:** macOS Apple Silicon (`arm64`) and Windows Intel/AMD 64-bit (`x64`). Confirm them before implementation. Intel Mac, Windows ARM64 and Linux packages are later targets, unless explicitly included in the release.
- **Full app:** include the built-in instruments, effects, guides and required runtime resources. End users should not need CMake, a compiler, Python or developer tools.
- **Models:** keep Demucs and Whisper as first-use downloads initially. Basic Pitch, Bravura and the pronunciation dictionary are already embedded. A completely offline package is a separate option.
- **External tools and content:** keep Ollama, its models, singing voices, vocoders and third-party plugins separately installed until redistribution permission for the exact files is established.
- **Plugins for other DAWs:** optional release assets. DawIt's built-in sounds and effects do not depend on installing their VST3 or AU copies.
- **Website:** maintain the seven-page site in `website/` under [its maintenance guide](website/MAINTAINING.md). The former site has been replaced; deployment remains a separate task.

The first release is ready when sections 3–10 are satisfied for the chosen targets and signing route. Sections 11–12 retain optional work. A task only applies when its platform, package or feature is included. Record deferred work explicitly rather than marking it complete.

## 2. Current state and known gaps

This snapshot comes from [CMakeLists.txt](CMakeLists.txt), [the plugin build helper](cmake/DawItBuiltIns.cmake), [build.sh](build.sh) and the [README licence note](README.md#license-note).

| Area | Current state | Release work |
|---|---|---|
| App version | `0.1.0` in CMake | Keep executable, installer, tag and notes consistent |
| JUCE | Local checkout is 8.0.9; fallback fetch also selects 8.0.9 | Record the actual revision used; make clean builds consistent |
| VST3 | The SDK bundled with JUCE 8.0.9 offers GPLv3 or proprietary terms | Use its GPLv3 option with the AGPL release; do not label it MIT |
| ONNX Runtime | 1.30.0; copied into the app or beside the executable | Package its notices, matching source and all required runtime files |
| Mac architectures | ONNX package selection always uses macOS ARM64 | Ship Apple Silicon first; Intel needs an additional dependency/build path |
| Licences | Root `LICENSE` and upstream notice snapshots in `licenses/` exist; the build still only copies Bravura's licence | Finish per-platform notice checks and binary/source packaging |
| Models | Demucs and Whisper download on first use | Verify provenance, hashes, notices, progress, cancellation and storage |
| Plugin builds | `--plugins` builds 13 built-ins as VST3, plus AU on macOS; Tune remains app-only | Validate and package only the formats being released |
| Release infrastructure | No `.github` release workflows found during this review | Add build, packaging, source archive and draft-release workflows |

## 3. Licence compliance and complete source — required

### 3.1 DawIt's own licence and notices

- [x] Declare `AGPL-3.0-only` consistently in `LICENSE`, the README and project instructions, matching the AGPLv3 release discussed with the owner.
- [x] Add `LICENSE` containing the complete unmodified AGPLv3 text and a clear copyright/licence statement for DawIt.
- [x] Update the README to state that DawIt is AGPLv3, with the third-party components retaining their own licences.
- [ ] Establish consistent source-file licence notices or SPDX identifiers; preserve third-party notices and distinguish third-party files from DawIt code.
- [ ] Add an accessible *About DawIt* entry showing the version, copyright, licence, lack of warranty, redistribution rights, source link and third-party notices. Make the legal notices available offline.
- [ ] Put a clearly labelled source link beside every binary download and in each release's notes.
- [ ] Ensure installer terms do not prohibit copying, modification or redistribution allowed by the licences.
- [ ] Review ownership and the provenance of code, copied examples, artwork, icons, audio examples and text being published. Obtain permission or replace anything without suitable rights.
- [ ] Add `CONTRIBUTING.md` covering licence terms for contributions, builds, tests and project conventions. A CLA is optional, not a prerequisite for releasing AGPL software; record the chosen contribution policy.

### 3.2 Inventory every shipped component

The table is a starting inventory, not a completed legal audit. Verify the licence files at the exact revisions used and check the transitive components selected by each platform build.

| Component | Current licence route | What to prepare |
|---|---|---|
| JUCE 8.0.9 | AGPLv3 | Licence, copyright notices and corresponding source |
| VST3 SDK bundled with JUCE 8.0.9 | GPLv3 option | GPL text, relevant notices and source; applies to app hosting as well as plugin builds |
| AudioUnitSDK, where included | Apache 2.0 | Licence and applicable notices |
| demucs.cpp | MIT | Copyright, full licence and matching source |
| Eigen | MPL 2.0 with `EIGEN_MPL2_ONLY`, plus applicable third-party notices | Covered source, modifications and notices; do not provide just the MPL text |
| Demucs `htdemucs` weights | MIT, as recorded in the project | Verify the converted file's provenance and accompanying terms |
| ONNX Runtime | MIT plus bundled component notices | Full licence, `ThirdPartyNotices.txt`, exact binary provenance and matching source |
| Basic Pitch model and any reused implementation | Apache 2.0 | Licence, copyright, applicable NOTICE and change notices |
| whisper.cpp and ggml | MIT plus any relevant bundled notices | Licence notices and matching source |
| Whisper weights | MIT | Original and converted model provenance and licence notices |
| Bravura font | SIL Open Font License 1.1 | Copyright and full OFL text; preserve applicable font naming conditions |
| CMU Pronouncing Dictionary | Its pinned BSD-style licence | Full copyright, conditions and disclaimer; a one-line credit alone is insufficient |
| JUCE's embedded codecs, image/text libraries and other enabled dependencies | Varies | Inspect what is actually compiled or shipped and include its required notices |
| Runtime redistributables, installer and validation tools | Varies | Check the selected versions' terms and any redistribution requirements |

- [ ] Create a dependency manifest recording component, version/commit, original URL, hash, licence, modifications, binary inclusion and source location.
- [ ] Collect exact licence and NOTICE files into a maintained directory, with clear filenames and an index. Preserve attribution and modification notices; do not replace them with a table of licence names.
- [ ] Copy the app licence, notice index and required texts into `DawIt.app/Contents/Resources/` on macOS and the installation directory on Windows.
- [ ] Package notices with separately distributed plugins too; only include components those binaries actually use.
- [ ] Inspect Eigen's applicable `COPYING.*` files, ONNX Runtime's notices and JUCE's enabled embedded dependencies on both platforms.
- [ ] Resolve the README's CMU dictionary description against the full licence at the pinned commit.
- [ ] Correct references that say the current VST3 SDK is MIT. Upgrading to JUCE 8.0.11 or another verified version containing VST3 SDK 3.8+ is optional and requires regression tests. Updating the fallback version alone does not replace `external/JUCE`.
- [ ] Check Steinberg's trademark guidelines before using VST logos or compatibility branding. SDK copyright permission does not grant unrestricted trademark use.

### 3.3 Source for each binary release

- [ ] Produce a dedicated `DawIt-<version>-complete-source.tar.gz` from the exact released commit. Include DawIt source, required assets, build/install scripts, dependency manifest, licences, local patches and the source needed for the shipped non-system components.
- [ ] Include the actual JUCE/VST3 sources used, Eigen, demucs.cpp, whisper.cpp/ggml and the matching ONNX Runtime source. Record upstream binary build information and any patches needed to explain the shipped libraries.
- [ ] Include necessary model/font/dictionary assets or source material as required by their licences, with their provenance. Document which optional models download separately.
- [ ] Include instructions for rebuilding and installing a modified version, including the free/ad-hoc signing path. Users must not need the maintainer's private signing keys to build and run their changes.
- [ ] Ensure there are no untracked local source changes or unrecorded dependency modifications behind the binaries.
- [ ] Rebuild from the complete source archive in a fresh environment. Confirm that no missing private files, submodule contents or local checkout are needed.
- [ ] Keep source available alongside every release for as long as required by the licences. Preserve source for older binaries that remain downloadable.
- [ ] Do not assume GitHub's automatically generated source ZIP includes fetched dependencies or nested repositories. Inspect it; the dedicated archive is the planned complete-source download.

AGPL distribution and source requirements are in [AGPLv3, particularly sections 1, 4–6 and 13](https://www.gnu.org/licenses/agpl-3.0.en.html). Eigen's source obligations are explained in [Mozilla's MPL FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/). The pinned dictionary's binary notice requirement appears in [its licence](https://github.com/cmusphinx/cmudict/blob/74790861f652b15e4ac49015a90074ad62a27690/LICENSE).

## 4. Public GitHub repository and builds — required outcome

### 4.1 Repository preparation

- [ ] Confirm the GitHub owner, repository name, canonical source URL and issue-reporting URL.
- [ ] Review tracked files and the history intended for publication for credentials, signing material, private recordings and other private data. Remove or resolve findings before publication; do not expose secret values in logs.
- [ ] Make `.gitignore` cover build output, generated installers, local settings, credentials and private signing files. Keep required source and release scripts tracked.
- [ ] Confirm how `external/JUCE` is obtained from a fresh clone. Record and verify its revision instead of accepting an arbitrary local copy.
- [ ] Publish build requirements, supported targets, installation instructions, model download sizes and known limitations in the README.
- [ ] Publish the repository with the agreed open-source licence. Keep issues available and add a contact route for security reports.

### 4.2 Build automation

GitHub Actions is recommended; a documented, repeatable manual build can deliver the same first-release outcome. Standard GitHub-hosted runners for public repositories are free, subject to service limits. Select runner architectures explicitly and check current availability. See [billing](https://docs.github.com/en/actions/concepts/billing-and-usage) and [runner specifications](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

- [ ] Add a pull-request/push build workflow for macOS ARM64 and Windows x64. Add other targets only when they are supported and tested.
- [ ] Record the OS image, architecture, compiler, CMake, SDK and dependency revisions in the build log or manifest. Match Windows generator architecture and ONNX package selection explicitly.
- [ ] Fetch pinned dependencies with checked hashes or verified commits. Report download failures and reject a mismatched local dependency checkout.
- [ ] Build Release and `DawItTests`; run the tests and require `All checks passed`, with no compiler warnings as required by the project. Preserve useful failure logs.
- [ ] Cache downloads/build inputs by platform, architecture and dependency hashes. Confirm that a cache-free build succeeds.
- [ ] Check that no build-machine paths are required at runtime and that no host-specific CPU optimisations make the binaries unusable on the claimed hardware.
- [ ] Add packaging jobs and a version-tag workflow that creates a **draft** GitHub release from one exact commit.
- [ ] Keep ordinary CI read-only where possible. Grant release-write permission only to release jobs; keep signing credentials in protected secrets and out of pull-request builds from forks.
- [ ] Pin workflow actions and external packaging/validation tools to reviewed versions, preferably commit hashes where applicable.
- [ ] Upload the final packages, complete source, manifest, release notes and checksums. Keep build logs sufficient to investigate failures.

## 5. macOS DMG — required for the Mac release

### 5.1 Build and bundle

- [ ] Build the chosen version on Apple Silicon using the documented Release configuration. The current app output is `build/Release/DawIt_artefacts/Release/DawIt.app` when using `build.sh`.
- [ ] Run `./build.sh --test`. Add `--plugins` when distributing the separate plugin products.
- [ ] Set the version, bundle identifier and icon consistently. Keep the current `com.dawit.app` identifier stable once released.
- [ ] Inspect the executable and all bundled libraries with `file`/`lipo -info` and `otool -L`. Require compatible architectures and resolve non-system dependencies inside the bundle.
- [ ] Verify that `Contents/Frameworks/libonnxruntime.1.dylib` loads through the relative runtime path. Inspect whisper/ggml backend and Metal resource requirements; package any files required by the actual build.
- [ ] Include licences and all required resources before signing. Check that no cache, developer settings, private songs or unnecessary test programs are in the bundle.
- [ ] Measure the minimum supported macOS version from the complete dependency set and test it. CMake's deployment target of 11.0 alone is not proof that the full app supports macOS 11.
- [ ] Test microphone permissions, recording, playback, third-party AU/VST3 loading and model inference in the packaged build.

### 5.2 Choose and validate one signing route

**No paid account: ad-hoc signing.** This does not identify a trusted publisher or provide notarisation. Gatekeeper can block a downloaded build, and the user may need an exception; managed devices may disallow one.

- [ ] Sign bundled executable code from the inside out, then sign the app. Verify signatures explicitly; do not rely on `codesign --deep` as a substitute for identifying and signing nested code correctly.
- [ ] Test the DMG downloaded through a browser on a separate account or clean Mac, preserving quarantine during the test.
- [ ] Write first-open instructions using the current macOS *System Settings → Privacy & Security → Open Anyway* route where available, with the tested OS versions. Do not instruct users to disable Gatekeeper globally.
- [ ] Document the route's limitations without promising that there is always just one warning or that every machine will allow the app.

**Optional paid route: Developer ID and notarisation.** Record the current programme cost and account eligibility before choosing it; do not make paid signing a hidden release requirement.

- [ ] Obtain the appropriate Developer ID certificate and configure signing/notarisation credentials outside the repository.
- [ ] Enable the hardened runtime and a secure timestamp. Use the microphone/audio-input entitlement where required; test recording after signing.
- [ ] Add `com.apple.security.cs.disable-library-validation` for loading third-party plugins signed by other developers. Evaluate further exceptions only if a tested dependency needs them.
- [ ] Sign nested libraries/plugins and the app in the correct order. Verify the completed app and keep its signing identity consistent across releases.
- [ ] Build the distribution container, submit using `xcrun notarytool`, wait for acceptance and inspect failures. Staple tickets to the appropriate supported app/container and validate them.
- [ ] Verify the installed app with signature checks and Gatekeeper assessment, and test first launch from an actual download. A successful build or signature alone is not notarisation approval.

See [Apple's developer account options](https://developer.apple.com/help/account/basics/about-your-developer-account), [notarisation workflow](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution), [plugin library-validation entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.cs.disable-library-validation) and [first-open guidance](https://support.apple.com/en-us/102445).

### 5.3 Assemble and test the DMG

- [ ] Add a repeatable DMG packaging script. Stage `DawIt.app`, an `/Applications` shortcut and a short installation/readme file with the source URL, architecture and signing status.
- [ ] Create a compressed, read-only image, for example with `hdiutil create`; name it `DawIt-<version>-macos-arm64.dmg`.
- [ ] If using notarisation, complete the container signing/notarisation/stapling sequence before calculating checksums. Do not alter signed app contents afterwards.
- [ ] Mount the DMG, drag the app into Applications, eject the image, and launch the installed copy without relying on the mounted image or the build directory.
- [ ] Check install over an older version, settings compatibility, model persistence and an ordinary account without developer tools.
- [ ] Document removal of the application separately from optional removal of models/settings. Preserve users' songs and data.

## 6. Windows installer EXE — required for the Windows release

### 6.1 Build and collect runtime files

- [ ] Build and test on Windows x64 using a documented C++20/MSVC and Windows SDK toolchain. Confirm the CMake generator actually selects x64 and the ONNX x64 package.
- [ ] Build `DawIt` and `DawItTests` in Release and run the tests. The executable sits under `DawIt_artefacts/Release/` in the selected build directory.
- [ ] Inspect executable and DLL dependencies using Windows SDK tools such as `dumpbin /DEPENDENTS`; recursively account for required non-system DLLs.
- [ ] Stage `DawIt.exe`, `onnxruntime.dll`, any additional required runtime/backend files, resources and licences. Match all executable architectures.
- [ ] Choose the C++ runtime strategy. Static linking DawIt alone does not remove runtime requirements of a prebuilt ONNX DLL. Check that library too; include an authorised Microsoft redistributable or other permitted deployment arrangement when needed.
- [ ] Confirm the minimum Windows version and CPU features supported by all selected dependencies. Publish tested requirements rather than assuming Windows 10 support from the app source alone.
- [ ] Test recording, playback, file dialogues, graphics, VST3 scanning and all included inference paths on a clean machine.

### 6.2 Installer behaviour

- [ ] Pin an installer builder and its licence. Inno Setup is a suitable option; preserve its required installer notices. Its [licence](https://jrsoftware.org/files/is/license.txt) permits use for commercial and non-commercial applications subject to its conditions.
- [ ] Add a version-controlled installer script that produces `DawIt-<version>-windows-x64-setup.exe` from the staged Release files.
- [ ] Choose per-user or machine-wide installation deliberately. Request administrator rights only for the chosen installation scope or shared plugin destination.
- [ ] Use a stable installer application ID, version, publisher and uninstall registration. Create a Start-menu shortcut and make a desktop shortcut optional.
- [ ] Show licence/source information and provide an uninstaller. Do not require surrendering AGPL rights to complete installation.
- [ ] If plugin products are included, make them an optional component and use the appropriate VST3 installation directory for the selected scope and architecture.
- [ ] Keep user settings, models, recordings and projects outside the application installation directory. Preserve them during upgrades and ordinary uninstall; any data deletion must be an explicit separate choice.
- [ ] Test a new installation, upgrade, cancellation, uninstall and reinstall. Check file paths containing spaces and non-ASCII characters, and ordinary-user operation after installation.
- [ ] Consider a portable ZIP only after confirming writable settings/model locations and the same runtime dependency completeness. It is optional for the first release.

### 6.3 Signing and first launch

- [ ] Record whether the release is unsigned or uses Authenticode signing. A self-signed certificate does not provide public publisher trust.
- [ ] For the no-spend route, test the actual downloaded unsigned installer and document what users see. SmartScreen, Smart App Control and organisational policy can prevent running an unsigned app; a bypass is not guaranteed.
- [ ] Optionally apply to SignPath Foundation. Check its current eligibility and upstream-binary/signing rules against JUCE and the prebuilt ONNX package; acceptance is not automatic. Confirm whether source-built dependencies or other changes would be needed.
- [ ] Alternatively evaluate a certificate or signing service, checking eligibility and costs for the actual publisher. Keep signing keys/service credentials in protected CI secrets or secure hardware.
- [ ] If signing, sign the project's executable code and installer/uninstaller as appropriate, retain upstream signatures where required, timestamp signatures and verify them. Check the signing service's allowed payloads before submitting dependencies.
- [ ] Compute hashes after signing and final packaging. Document the actual publisher name users should expect.
- [ ] Do not promise that signing removes SmartScreen warnings immediately. Test the final download and explain the observed installation experience.

See [Microsoft's SmartScreen guidance](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation) and [SignPath Foundation's conditions](https://signpath.org/terms).

## 7. Models, voices and offline behaviour — required for included features

- [ ] Record the exact Demucs and Whisper model URLs, revisions, hashes, sizes, origins and licences. Preserve notices for converted weights as well as the original models where applicable.
- [ ] Verify model downloads from the packaged app: progress, cancellation, interrupted download, corrupt file, hash mismatch, retry, insufficient disk space and an unwritable destination.
- [ ] Confirm models go to a per-user application-data location, remain available across upgrades, and never require writing into a signed application bundle or Program Files.
- [ ] Test the ordinary editor, playback, built-ins and guides offline. Explain that first-use models and separately installed AI/voice tools need setup.
- [ ] Confirm that music stays on the user's computer for these features. Distinguish fetching software/models from sending user recordings; document local Ollama configuration.
- [ ] Keep Ollama and its language models out of the installer initially. Verify each specific model's terms before proposing a bundled alternative; the runtime's licence does not determine the model's licence.
- [ ] Keep singing voicebanks and vocoders user-installed. Verify the maker's current terms before changing that policy; do not treat DiffSinger's code licence as permission to redistribute every voice or vocoder.
- [ ] Keep third-party commercial plugins and user sample libraries out of release packages.
- [ ] If offering an offline model pack later, audit each included file, package notices and hashes, measure download/disk requirements, and implement a documented install/import path. Copying weights into the DMG alone does not make the current app discover them.

## 8. Release validation — required

Record results against the exact candidate assets: version, commit, filename/hash, OS version, CPU architecture, signing status, tester and outcome. Re-test affected checks whenever the candidate changes.

- [ ] Build without warnings and pass the engine tests on each advertised platform. Use [docs/tests.md](docs/tests.md) for existing coverage.
- [ ] Download and install the candidate using a browser on clean machines with no development checkout, compiler, build tools or pre-populated model cache.
- [ ] Launch from the installed location; verify About, version, licence texts, source link and all required resources.
- [ ] Create a song, add MIDI and audio tracks, play built-in instruments, apply effects, change tempo/key, record audio/MIDI, save, close and reopen the project, and export audio/MIDI/PDF where supported.
- [ ] Test the example song, piano roll, mixer, routing, guides, score/font rendering, menus and common keyboard shortcuts.
- [ ] Check audio to MIDI with the bundled Basic Pitch model; download and run Demucs and Whisper; verify cancellation and clear errors when models/tools are missing.
- [ ] Exercise singing and Ollama with separately installed, permitted test content where those features are claimed to work on the target.
- [ ] Scan/load a representative third-party plugin, save its state and reopen it. Record tested formats and hosts without implying all plugins are supported.
- [ ] Test microphone permissions, audio-device changes, ordinary-user permissions, offline startup and missing-network behaviour.
- [ ] Test upgrade/uninstall behaviour without deleting users' songs, settings or model files unexpectedly.
- [ ] For UI checks, use permanent screenshot-runner paths and the process in [docs/recipes.md](docs/recipes.md#checking-in-the-real-app). `--screenshots` uses scratch settings; for other real-app runs, copy `DawIt.settings` aside and restore it afterwards without reading its contents.
- [ ] Rebuild the source archive separately and verify that the build instructions work. Record remaining platform limitations in the release notes.

## 9. GitHub release contents and download information — required

Example initial assets; substitute the actual version and publish only targets that passed validation:

```text
DawIt-0.1.0-macos-arm64.dmg
DawIt-0.1.0-windows-x64-setup.exe
DawIt-0.1.0-complete-source.tar.gz
DawIt-0.1.0-dependencies.json
SHA256SUMS.txt
```

- [ ] Choose the version and update all app/plugin/installer version fields together. Use a prerelease tag if this is an alpha, beta or release candidate.
- [ ] Tag the exact tested commit, for example `v0.1.0`. Build every asset from it, without unrelated working-tree changes.
- [ ] Write release notes: supported systems/architectures, what is included, model downloads, external setup, signing status, installation steps, known issues and the matching source link.
- [ ] Attach the complete-source archive and dependency manifest in addition to binary downloads and GitHub's automatic source archives.
- [ ] Generate `SHA256SUMS.txt` from the final signed/notarised/stapled assets. Verify every uploaded asset against it after downloading again.
- [ ] Put source, licence and first-open guidance beside the binary links. State that unsigned/ad-hoc releases can face platform security restrictions rather than attributing every warning simply to being free software.
- [ ] Check GitHub's current release-asset limits before uploading large source/model packages; do not assume arbitrary asset sizes are supported.
- [ ] Keep the release as a draft until installation and source checks pass. Publish it deliberately, with the correct stable/prerelease designation.
- [ ] Update download links and installation text in `website/`, following its maintenance guide and existing page structure. Do not add an eighth page or publish without a deployment request.
- [ ] Link website buttons to the appropriate GitHub release assets, identify architectures clearly and show source/checksum links. List only tested minimum requirements and signing behaviour.
- [ ] Provide an issue template asking for DawIt version, OS, architecture and steps to reproduce; request logs without private music or credentials.

[GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases) associates assets and release notes with a Git tag. Temporary Actions artifacts are not a substitute for a durable source/release download.

## 10. Repeatable release runbook — required for every release

1. Finish the chosen changes and documentation; resolve outstanding licence and dependency questions.
2. Set the version; review changes and release scope. Commit and tag only when authorised by the maintainer.
3. Build from that exact commit on every chosen target; run tests and collect logs/manifests.
4. Stage runtime files, resources, licences and the complete source archive; validate their completeness.
5. Sign/package/notarise using the selected routes. Calculate hashes only after the final bytes are fixed.
6. Upload to a draft GitHub release. Download the assets again and test clean installation, core features and source rebuilding.
7. Fix a failed candidate and repeat affected checks. If a public version already exists, issue a new version rather than silently replacing its binaries or moving its tag.
8. Publish the verified release when authorised; update the maintained website links and first-open instructions.
9. Check live links, source availability and hashes. Keep the release record and monitor reported installation failures.
10. If a serious fault is found, clearly mark the release unsuitable and point users to the last working version while preparing a fix. Preserve matching source and explain any withdrawal.

## 11. Optional plugin products

These tasks apply when publishing the built-ins for other DAWs; they do not block an app-only release.

- [ ] Build the 13 supported plugin products from the same commit/version as the app; keep DawIt Tune app-only until separately implemented and tested.
- [ ] Validate VST3s with a pinned `pluginval` build and AUs with `auval`; preserve results and failures. Test opening the controls and guide, audio/MIDI behaviour, presets, automation and state recall.
- [ ] Keep manufacturer/product identifiers and parameter IDs stable once users can save host sessions with them.
- [ ] Test the actual distributed bundles in selected hosts. Logic AU is the currently documented manual test; verify GarageBand, Reaper, Live, Bitwig, FL Studio and Cubase before listing them as tested.
- [ ] Include notices and source access for plugin downloads. Sign/notarise Mac bundles using the chosen route and test them with download quarantine present.
- [ ] Choose a Mac plugin installation method: a separate package, documented copy instructions, or a future *Install DawIt's plugins for other DAWs...* command. The latter is feature work and needs the normal plan/implementation process.
- [ ] Test per-user installation into `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`, rescan, upgrade and removal. Do not assume copying from inside DawIt removes all Gatekeeper/host restrictions.
- [ ] Add a Windows installer component using the appropriate VST3 destination, with reliable upgrade/removal behaviour and no duplicate copies.
- [x] DawIt's plugin scan recognises its own plugin copies so the add menus can hide them (`plugins::isDawItPluginCopy`). Re-test this with the installed release.
- [ ] Check what happens when opening an older project naming a DawIt plugin copy; do not promise automatic replacement by a built-in until verified.
- [ ] Consider separate `DawIt-<version>-plugins-macos-arm64` and `DawIt-<version>-plugins-windows-x64` assets so app users need not install external-host copies.

## 12. Later targets and distribution options

- [ ] **Intel Mac / Universal:** obtain or build an x86_64 ONNX Runtime, support architecture selection in CMake and include every other library/backend for each slice. Test on Intel hardware before publishing. A universal main executable with an ARM-only library is not a universal app.
- [ ] **Windows ARM64:** validate the compiler/generator architecture, ONNX package, audio devices, inference and ARM64 plugin compatibility on real hardware. A CMake branch alone is not validation.
- [ ] **Linux:** retain buildability; document distro packages, choose a minimum libc baseline, and test AppImage or tarball packaging including ONNX and plugin dependencies. AppImage does not guarantee compatibility with every distribution.
- [ ] **Portable archives:** provide Mac ZIP or Windows ZIP only after validating equivalent resources, notices, runtime files and installation instructions.
- [ ] **Package managers:** investigate Homebrew/cask signing rules (including custom taps), winget and Flathub only when the underlying packages are stable. Do not assume a custom Homebrew tap bypasses all current requirements.
- [ ] **Additional plugin formats:** plan/test LV2 or AAX separately. AAX requires its own SDK/licence/signing review; it is not implied by publishing VST3/AU.
- [ ] **Offline edition:** implement a verified optional model pack as described in section 7. Check hosting limits and download size before committing to it.
- [ ] **Paid signing:** re-evaluate Apple membership and Windows signing services if installation friction warrants it. Check current eligibility/prices rather than retaining historic quotes.
- [ ] **Future releases:** keep dependencies, notices, source archives, minimum-system claims and install guidance current; repeat signing/notarisation and clean-machine tests after relevant changes.

## 13. Implementation hand-off

The licence inventory and notice snapshots now exist. Other suggested files below are **to create**, not existing scripts or instructions to run now:

| Proposed file/work | Purpose |
|---|---|
| Existing `LICENSE` and `licenses/`; `CONTRIBUTING.md` still to create | Open-source terms, maintained notices and contribution policy; binary notice packaging remains open |
| CMake licence/resource staging changes | Same runtime resources and notices in local and CI builds |
| `tools/package-macos.sh` | Stage/sign the app and create/validate the DMG |
| `tools/package-windows.ps1` and an Inno Setup script | Stage Windows runtimes and build the installer |
| `tools/package-source.py` | Collect complete source, licences and exact dependency metadata |
| `.github/workflows/build.yml` | Platform builds and tests for changes |
| `.github/workflows/release.yml` | Tagged builds, packaging, checksums and draft release |
| README and maintained website updates | Accurate build, download, requirements and first-open instructions |

Implement app/UI changes through the project's normal feature workflow and documentation requirements. Keep app/release-infrastructure changes and website changes in separate commits, following the project rules on committing and publishing.
