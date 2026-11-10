# v0.1.7 packages, source, notices and dependency manifest verification

Checked 10 November 2026 against tag `v0.1.7`, commit
`b5a0030a4aebe4e24b87e978933fa457a7f3e5d7`, and the successful
[release workflow](https://github.com/geertvandamme/dawit/actions/runs/42843989807).
These checks were made on the draft. The maintainer then published it on
10 November 2026 as a prerelease, an early preview, with the open points below
still open. This record says what was checked and what is still open.

Since v0.1.6 the Windows installer checks for Microsoft's Visual C++ runtime
(14.51 or later for this build, the MSVC toolset's version) and offers
Microsoft's download when it's missing; DawIt ships none of the runtime. The
release notes link that download (`aka.ms/vc14`, 14.51.36247) instead of the
older `aka.ms/vs/17` one (14.44), and name Debian 13. The release workflow
gained the Linux start test and the emulated older-processor checks. No
dependency, `LICENSE` or `licenses/` file changed.

## Packages

- Downloaded all six draft assets and checked each against the draft's
  `SHA256SUMS.txt`: all match. The checksums come from the same release, so
  they check the transfer, not an independent signature.
- The macOS app says version 0.1.7, and its ad-hoc signature passes
  `codesign --verify --deep --strict`. The app isn't notarised.
- The release notes say version 0.1.7 throughout, have no placeholders left,
  and link the 14.51 runtime. The Windows job logged that the installer asks
  for the Visual C++ runtime 14.51 or later, and its install test found no
  runtime DLL in the install folder.
- The Linux `DawIt` binary's source link is
  `https://github.com/geertvandamme/dawit/releases/download/v0.1.7/DawIt-0.1.7-complete-source.tar.gz`,
  which works only once the release is published.

## New checks in the release run

- **Linux on fresh systems** (`linux-start`): in Ubuntu 22.04 and 24.04,
  Debian 12 and 13 and Fedora containers, the README's install line was
  enough for every linked and X11 library, and DawIt started and drew its
  window.
- **An older processor, emulated:** under QEMU emulating Nehalem (no AVX),
  `DawItTests --lyrics-cpu-check` found AVX2, FMA and F16C missing, and
  loading Whisper failed with the explanation instead of crashing.
- **The whole app under that emulation** didn't finish: it ran for the 15
  minutes allowed with no output and was stopped (exit 124). A missing
  instruction would have stopped it at once, so it looks slow or stuck under
  QEMU 6.2's user mode rather than crashing. The step is allowed to fail, and
  stays so until this is understood.

## Complete source archive

- `DawIt-0.1.7-complete-source.tar.gz` (419 MB, SHA-256
  `0786901d4325262d014a4c597770e9921d5cb72adf29ec09c7563293adc9e1e4`) holds
  DawIt at the tag and 29 upstream files.
- Running `tools/make-complete-source.py --commit v0.1.7` on a Mac gave a
  byte-identical archive to the one the Linux CI job attached.

## Dependency manifest and build records

- `DawIt-0.1.7-dependencies.json` lists the same 30 components and 5 app
  downloads as v0.1.6, with the Windows and Linux system requirements updated.
  Running `tools/make-dependency-manifest.py` on the downloaded source archive
  gives the same components, app downloads and system requirements.
- The Linux package's `DawIt` and `libonnxruntime.so.1` match the hashes their
  build record took as built, byte for byte.
- The runner images, compilers and SDKs are the same as v0.1.6's
  ([table](release-v0.1.5-verification.md#dependency-manifest-and-build-records)),
  and every binary needs the same libraries.

## ONNX Runtime

The as-built macOS (`7c041d84…`) and Linux (`cc9bab3c…`) libraries are again
the same as v0.1.4's, so the
[v0.1.4 notice audit](release-v0.1.4-onnx-notice-audit.md) covers them. The
Windows DLL differs again (`5470d67e…`), as MSVC builds always do, and needs
the same DLLs.

## Notices

All 46 files of the tagged `LICENSE` and `licenses/` are present and identical
(after normalising line endings) in the Linux archive, in the DMG beside the
app, and in the app bundle's `Contents/Resources`. The Windows CI job installs
the installer and compares the installed licence files with the tag, and it
passed.

## Still open

- The installer's runtime prompt on a real PC without the runtime (Windows
  Sandbox), and the lyrics message on a real PC without AVX2.
- Why the whole app doesn't finish starting under QEMU's emulation.
- The Windows installer's contents, checked independently of CI.
- Signing, notarisation, the website's download links and clean-machine
  tests, as listed in `DISTRIBUTION_TODO.md`.
